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

    m_connectionOpen = false;
    m_connectionTime = 0.f;
    closeConnection();
    this->onConnectionClose.invoke(reason, m_threadSafeEvents, m_overrideEvents);
}

void Client::m_parse_connection_confirm(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    m_connectionOpen = true;
    m_connectionTime = 0.f;
    packet >> m_ip; // getting the ip from the packet (the id that the server assigned)
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
    m_send(temp, getServerIP(), getServerPort());
}

void Client::setServerData(sf::IpAddress serverIP, PORT serverPort)
{
    if (isConnectionOpen())
        return;
        
    setServerData(serverIP);
    setServerData(serverPort);  
}

void Client::setServerData(sf::IpAddress serverIP)
{
    if (isConnectionOpen())
        return;
        
    if (serverIP == "")
        m_serverIP = sf::IpAddress::LocalHost;
    else
        m_serverIP = serverIP; 
    onServerIpChanged.invoke(getServerIP(), m_threadSafeEvents, m_overrideEvents);
}

void Client::setServerData(PORT port)
{
    if (isConnectionOpen())
        return;
        
    m_serverPort = port;
    onServerPortChanged.invoke(getServerPort(), m_threadSafeEvents, m_overrideEvents);
}

void Client::sendToServer(sf::Packet& packet)
{
    if (!m_connectionOpen) return;
    m_wrongPassword = false;
    m_send(packet, getServerIP(), getServerPort());
}

float Client::getTimeSinceLastPacket() const
{ return m_timeSinceLastPacket; }

sf::IpAddress Client::getServerIP() const
{ return m_serverIP; }

unsigned int Client::getServerPort() const
{ return m_serverPort; }

// * Pure Virtual Definitions

bool Client::tryOpenConnection()
{
    m_wrongPassword = false;

    sf::Packet connectionRequest = this->ConnectionRequestTemplate();

    if (getServerIP() != sf::IpAddress::None)
    {
        if (!this->isReceivingPackets())
        {
            this->bind(Socket::AnyPort);
            setPort(Socket::getLocalPort());
            startThreads(); //! needs to be called AFTER port binding
        }
    }
    else
    {
        return false;
    }

    // checking if connecting to localhost as ID will be different in that case
    if (getServerIP() == sf::IpAddress::LocalHost) m_ip = sf::IpAddress::LocalHost.toInteger();

    try
    {
        // if this fails socket did not open
        m_send(connectionRequest, getServerIP(), getServerPort());
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

void Client::closeConnection()
{
    if (!this->isConnectionOpen())
        return;

    sf::Packet close = this->ConnectionCloseTemplate("Client Connection Closed");
    this->sendToServer(close);
    this->onConnectionClose.invoke("Called \"closeConnection\"", m_threadSafeEvents, m_overrideEvents);
 
    m_reset_connection_data();
    stopThreads();
    sf::Socket::close();
}

// ---------------------------

// ---------------------
