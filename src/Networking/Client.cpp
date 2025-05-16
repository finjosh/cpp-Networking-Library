#include "Networking/Client.hpp"

using namespace udp;

//* Initializer and Deconstructor

Client::Client(sf::IpAddress serverIP, PORT serverPort) 
{ 
    setServerData(serverIP, serverPort);
}

Client::Client(PORT serverPort)
{
    setServerData(serverPort);
}

Client::~Client()
{
    closeConnection();
}

// ------------------------------

//* Thread Functions

void Client::m_update_function(float deltaTime)
{
    if (this->isConnectionOpen()) 
    {
        m_timeSinceLastPacket += deltaTime;
    }
    if (m_timeSinceLastPacket >= m_timeoutTime) 
    { 
        this->closeConnection(); 
    }
}

// -----------------

//* Protected Connection Functions

void Client::m_reset_connection_data()
{
    Socket::m_reset_connection_data(); // reseting the default socket data
    m_wrongPassword = false;
    m_timeSinceLastPacket = 0.f;
}

// ---------------------

//* Packet Parsing Functions

void Client::m_parse_data(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    m_timeSinceLastPacket = 0.f;
    this->onDataReceived.invoke(packet, (ID)senderIP.toInteger(), m_threadSafeEvents, m_overrideEvents);
}

void Client::m_parse_connection_close(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    std::string reason;
    if (packet.endOfPacket())
        reason = "Unknown";
    else
        packet >> reason;

    closeConnection();
}

void Client::m_parse_connection_confirm(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    m_connectionOpen = true;
    m_connectionTime = 0.f;
    packet >> m_id; // getting the ip from the packet (the id that the server assigned)
    this->onConnectionOpen.invoke(m_threadSafeEvents, m_overrideEvents);
}

void Client::m_parse_password_request(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    if (m_needsPassword)
        m_wrongPassword = true;
    else
        m_wrongPassword = false;
    m_needsPassword = true;
    this->onPasswordRequest.invoke(m_threadSafeEvents, m_overrideEvents);
}

// -------------------------

//* Connection Functions

bool Client::wasIncorrectPassword()
{ return m_wrongPassword; }

void Client::setAndSendPassword(const std::string& password)
{ 
    setPassword(password); 
    this->sendPasswordToServer(); 
}

void Client::sendPasswordToServer()
{
    m_wrongPassword = false;
    sf::Packet temp = this->PasswordPacket(m_password);
    if (getServerIP().has_value())
        m_send(temp, getServerIP().value(), getServerPort());
}

bool Client::setServerData(IpAddress_t serverIP, PORT serverPort)
{
    if (isConnectionOpen() || !serverIP.has_value())
        return false;
        
    setServerData(serverIP);
    setServerData(serverPort);

    return true;
}

bool Client::setServerData(IpAddress_t serverIP)
{
    if (isConnectionOpen() || !serverIP.has_value())
        return false;

    m_serverIP = serverIP; 
    onServerIpChanged.invoke(getServerIP().value(), m_threadSafeEvents, m_overrideEvents);

    return true;
}

bool Client::setServerData(PORT port)
{
    if (isConnectionOpen())
        return false;
        
    m_serverPort = port;
    onServerPortChanged.invoke(getServerPort(), m_threadSafeEvents, m_overrideEvents);

    return true;
}

void Client::sendToServer(sf::Packet& packet)
{
    if (!m_connectionOpen) return;
    m_wrongPassword = false;
    assert(getServerIP().has_value() && "Must not send data to server with an invalid serverIP");
    m_send(packet, getServerIP().value(), getServerPort());
}

float Client::getTimeSinceLastPacket() const
{ return m_timeSinceLastPacket; }

IpAddress_t Client::getServerIP() const
{ return m_serverIP; }

unsigned int Client::getServerPort() const
{ return m_serverPort; }

// * Pure Virtual Definitions

bool Client::tryOpenConnection()
{
    m_wrongPassword = false;

    sf::Packet connectionRequest = this->ConnectionRequestTemplate();

    if (getServerIP().has_value())
    {
        if (!this->isReceivingPackets())
        {
            if (this->bind(Socket::AnyPort) != sf::Socket::Status::Done)
                return false;
            setPort(Socket::getLocalPort());
            startThreads(); //! needs to be called AFTER port binding
        }
    }
    else
    {
        return false;
    }

    // checking if connecting to localhost as ID will be different in that case
    if (getServerIP() == sf::IpAddress::LocalHost) m_id = sf::IpAddress::LocalHost.toInteger();

    try
    {
        // if this fails socket did not open
        m_send(connectionRequest, getServerIP().value(), getServerPort());
    }
    catch(const std::exception& e)
    {
        // since socket did not open stop threads and close sf::Socket
        stopThreads();
        sf::Socket::close();
        return false; 
    }

    return true;
}

void Client::closeConnection(const std::string& reason)
{
    if (!this->isConnectionOpen())
        return;

    sf::Packet close = this->ConnectionCloseTemplate(reason);
    this->sendToServer(close);
 
    m_reset_connection_data();
    stopThreads();
    sf::Socket::close();

    this->onConnectionClose.invoke(reason, m_threadSafeEvents, m_overrideEvents);
}

// ---------------------------

// ---------------------
