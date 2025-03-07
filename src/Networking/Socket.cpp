#include "Networking/Socket.hpp"
#include "Utils/UpdateLimiter.hpp"
#include <stdexcept>
#include <SFML/System/Clock.hpp>

using namespace udp;

//* initializer and deconstructor

Socket::Socket()
{
    if (const auto publicIP = sf::IpAddress::getPublicAddress(sf::seconds(1)))
        m_id = publicIP->toInteger();
    else
        m_id = 0;
    setPort(getLocalPort());
}

Socket::~Socket()
{   
    stopThreads();
    close();
}

// ------------------------------

//* Protected Thread Functions

void Socket::m_receive_packets_thread(std::stop_token sToken)
{
    sf::Packet packet;
    IpAddress_t senderIP(sf::IpAddress::LocalHost);
    unsigned short senderPort;

    while (!sToken.stop_requested()) {
        Status receiveStatus = this->receive(packet, senderIP, senderPort);
        switch (receiveStatus)
        {
        case sf::Socket::Status::Error:
            if (sToken.stop_requested()) return;
            throw std::runtime_error("Error Receiving Packet (Code: " + std::to_string((int)receiveStatus) + ")");
            break;
        
        case sf::Socket::Status::Disconnected:
            if (sToken.stop_requested()) return;
            break;
        }

        std::int8_t packetType;
        packet >> packetType;

        if (!senderIP.has_value())
            continue;
        
        switch (packetType)
        {
        case PacketType::Data:
            m_parse_data(packet, senderIP.value(), senderPort);
            break;
        
        case PacketType::ConnectionRequest:
            m_parse_connection_request(packet, senderIP.value(), senderPort);
            break;

        case PacketType::ConnectionClose:
            m_parse_connection_close(packet, senderIP.value(), senderPort);
            break;

        case PacketType::ConnectionConfirm:
            m_parse_connection_confirm(packet, senderIP.value(), senderPort);
            break;

        case PacketType::PasswordRequest:
            m_parse_password_request(packet, senderIP.value(), senderPort);
            break;

        case PacketType::Password:
            m_parse_password(packet, senderIP.value(), senderPort);
            break;

        default:
            m_parse_unkown(packet, senderIP.value(), senderPort);
            break;
        }

        packet.clear();
    }
}

void Socket::m_update_thread(std::stop_token sToken)
{
    UpdateLimiter updateLimit(m_socketUpdateRate);

    sf::Clock deltaClock;
    float deltaTime;
    float secondTime;
    
    while (!sToken.stop_requested())
    {
        deltaTime = deltaClock.restart().asSeconds();
        secondTime += deltaTime;
        m_connectionTime += deltaTime;
        
        // checking if second update should be called
        if (secondTime >= 1.f)
        {
            m_second_update_function();
            secondTime = 0.f;
        }

        // calling the fixed update function
        m_update_function(deltaTime);
        
        // if we are sending packets call the sending function
        if (m_sendingPackets) 
            m_packetSendFunction.invoke();
        
        updateLimit.wait();
    }
}

// ---------------------------

//* Protected Connection Functions

void Socket::m_reset_connection_data()
{
    m_needsPassword = false;
    setPassword("");
    m_connectionOpen = false;
    m_connectionTime = 0.f;
}

// -------------------------------

//* Socket Functions

void Socket::m_send(sf::Packet& packet, sf::IpAddress ip, PORT port)
{
    if (sf::UdpSocket::send(packet, sf::IpAddress(ip), port) != sf::Socket::Status::Done)
        throw std::runtime_error("Could not send packet (Socket::m_send Function)");
}

// -----------------

//* Public Thread functions

void Socket::startThreads()
{
    if (m_receiveThread == nullptr)
    {
        if (m_sSource != nullptr) delete(m_sSource);
        m_sSource = new std::stop_source;
        m_receiveThread = new std::jthread(&Socket::m_receive_packets_thread, this, m_sSource->get_token());
    }
    if (m_updateThread == nullptr) m_updateThread = new std::jthread(&Socket::m_update_thread, this, m_sSource->get_token());
}

void Socket::stopThreads()
{
    if (m_sSource == nullptr) return;
    m_sSource->request_stop();
    if (m_updateThread != nullptr)
    {
        m_updateThread->detach();
        delete(m_updateThread);
        m_updateThread = nullptr;
    }
    if (m_receiveThread != nullptr)
    {
        // Sending a packet to its self so the receive thread can continue execution and exit
        sf::Packet temp = DataPacketTemplate();
        m_send(temp, sf::IpAddress{m_id}, m_port);
        
        m_receiveThread->detach();
        delete(m_receiveThread);
        m_receiveThread = nullptr;
    }
    delete(m_sSource);
    m_sSource = nullptr;
}

void Socket::setThreadSafeOverride(bool override)
{
    m_overrideEvents = override;
}

bool Socket::getThreadSafeOverride() const
{
    return m_overrideEvents;
}


// ------------------------

//* Getter

ID Socket::getID() const
{ return (ID)m_id; }

IpAddress_t Socket::getIP() const
{ 
    if (m_id == 0)
        return std::nullopt;
    return sf::IpAddress(m_id); 
}

IpAddress_t Socket::getLocalIP() const
{ return sf::IpAddress::getLocalAddress(); }

double Socket::getConnectionTime() const
{ return m_connectionTime; }

double Socket::getOpenTime() const
{ return m_connectionTime; }

unsigned int Socket::getUpdateInterval() const
{ return m_socketUpdateRate; }

unsigned int Socket::getPort() const
{ return m_port; }

float Socket::getTimeout() const
{ return m_timeoutTime; }

std::string Socket::getPassword() const
{
    return m_password;
}

const funcHelper::func<void>& Socket::getPacketSendFunction() const
{
    return m_packetSendFunction;
}

// -------

//* Setters

void Socket::setUpdateInterval(unsigned int interval)
{ 
    if (this->isConnectionOpen()) return;

    this->m_socketUpdateRate = interval;
    onUpdateRateChanged.invoke(interval, m_threadSafeEvents, m_overrideEvents);
}

void Socket::sendingPackets(bool sendPackets)
{ 
    if (this->isConnectionOpen()) return;

    this->m_sendingPackets = sendPackets; 
}

void Socket::setPassword(const std::string& password)
{ 
    if (this->isConnectionOpen()) return;

    this->m_password = password; 
    onPasswordChanged.invoke(password, m_threadSafeEvents, m_overrideEvents);
}

void Socket::setTimeout(float timeout)
{ 
    if (this->isConnectionOpen()) return;

    m_timeoutTime = timeout; 
    onTimeoutChanged.invoke(timeout, m_threadSafeEvents, m_overrideEvents);
}

void Socket::setPacketSendFunction(const funcHelper::func<void>& packetSendFunction)
{
    if (this->isConnectionOpen()) return;

    m_packetSendFunction = packetSendFunction;
}

void Socket::setPort(PORT port)
{
    if (this->isConnectionOpen()) return;

    m_port = port;
    onPortChanged.invoke(m_port, m_threadSafeEvents, m_overrideEvents);
}

// --------

//* Boolean question Functions

bool Socket::isConnectionOpen() const
{ return m_connectionOpen; }

bool Socket::isReceivingPackets() const
{
    return (m_receiveThread != nullptr);
}

bool Socket::isSendingPackets() const
{ return m_sendingPackets && m_connectionOpen; }

bool Socket::NeedsPassword() const
{ return this->m_needsPassword; }

bool Socket::isValidIpAddress(const std::string& ipAddress)
{ return sf::IpAddress::resolve(ipAddress).has_value(); }

// ---------------------------

//* Template Functions

sf::Packet Socket::ConnectionCloseTemplate(std::string reason)
{
    sf::Packet out;
    out << PacketType::ConnectionClose;
    out << reason;
    return out;
}

sf::Packet Socket::ConnectionRequestTemplate()
{
    sf::Packet out;
    out << PacketType::ConnectionRequest;
    return out;
}

sf::Packet Socket::DataPacketTemplate()
{
    sf::Packet out;
    out << PacketType::Data;
    return out;
}

sf::Packet Socket::ConnectionConfirmPacket(std::uint32_t id)
{
    sf::Packet out;
    out << PacketType::ConnectionConfirm;
    out << id;
    return out;
}

sf::Packet Socket::PasswordRequestPacket()
{
    sf::Packet out;
    out << PacketType::PasswordRequest;
    return out;
}

sf::Packet Socket::PasswordPacket(const std::string& password)
{
    sf::Packet out;
    out << PacketType::Password;
    out << password;
    return out;
}

// -------------------