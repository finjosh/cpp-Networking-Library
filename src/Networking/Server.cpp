#include "Networking/Server.hpp"

using namespace udp;

//* Initializer and Deconstructor

Server::Server(unsigned short port, bool passwordRequired)
{
    setPort(port);
}

Server::~Server()
{
    closeConnection();
}

// ------------------------------

//* Protected Connection Functions
    
void Server::m_reset_connection_data()
{
    Socket::m_reset_connection_data(); // reseting the default data
    m_clientData.clear(); // reseting server specific data
}

// ---------------------

//* Server Functions

ClientData* Server::m_getClientData(ID clientID)
{
    ClientData temp = ClientData{0, clientID}; // port does not matter only id
    auto iter = m_clientData.find(&temp);
    if (iter == m_clientData.end())
        return nullptr;
    return *iter;
}

void Server::m_update_function(float deltaTime) 
{
    for (auto& clientData: m_clientData)
    {
        clientData->m_timeSinceLastPacket += deltaTime;
        if (clientData->m_timeSinceLastPacket >= m_timeoutTime)
        {
            this->disconnectClient(clientData->id, "Timedout");
        }
        clientData->m_connectionTime += deltaTime;
    }
}

void Server::m_second_update_function() 
{
    for (auto& clientData: m_clientData)
    {
        clientData->m_packetsPerSecond = clientData->m_packetsSent;
        clientData->m_packetsSent = 0;
    }
}

// -----------------

//* Packet Parsing Functions

void Server::m_parse_data(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    IP ip = senderIP.toInteger();

    ClientData* client = m_getClientData((ID)ip);
    // checking if the sender is a current client
    if (client != nullptr) 
    {
        client->m_timeSinceLastPacket = 0.0;
        client->m_packetsSent++;
        this->onDataReceived.invoke(packet, (ID)ip, m_threadSafeEvents, m_overrideEvents);
    }
    // if the sender is not a current client add them if possible
    else
    {   
        if (m_allowClientConnection) // no connection should happen
            return;
        if (!m_needsPassword) // send password request if needed
        {
            m_clientData.insert(new ClientData{senderPort, (ID)ip});

            sf::Packet Confirmation = this->ConnectionConfirmPacket(ip);
            m_send(Confirmation, senderIP, senderPort);

            this->onClientConnected.invoke((ID)(ip), m_threadSafeEvents, m_overrideEvents);
        }
        else
        {
            sf::Packet needPassword = this->PasswordRequestPacket();
            m_send(needPassword, senderIP, senderPort);
        }
    }
}

void Server::m_parse_connection_request(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    if (!m_allowClientConnection) return;
    IP ip = senderIP.toInteger();
    if (this->m_needsPassword)
    {
        sf::Packet needPassword = this->PasswordRequestPacket();
        m_send(needPassword, senderIP, senderPort);
        return; // dont want to confirm a connection if need password
    }
    else
    {
        // checking if the client is not already connected
        if (m_getClientData((ID)ip) == nullptr)
        {
            m_clientData.insert(new ClientData{senderPort, (ID)ip});
        }
        // we still want to send a confirmation as the confirmation packet may have been lost
    }

    sf::Packet Confirmation = this->ConnectionConfirmPacket(senderIP.toInteger());
    m_send(Confirmation, senderIP, senderPort);
    this->onClientConnected.invoke((ID)ip, m_threadSafeEvents, m_overrideEvents);
}

void Server::m_parse_connection_close(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    std::string reason;
    if (packet.endOfPacket())
        reason = "Unknown";
    else
        packet >> reason;
    disconnectClient(senderIP.toInteger(), reason);
    this->onClientDisconnected.invoke((ID)senderIP.toInteger(), reason, m_threadSafeEvents, m_overrideEvents);
}

void Server::m_parse_password(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort)
{
    std::string sentPassword;
    packet >> sentPassword;
    IP ip = senderIP.toInteger();

    ClientData* client = m_getClientData((ID)ip);
    if (client != nullptr) // if client is already connected
    {
        // make sure the client knows they are connected by sending another connection confirmation
        sf::Packet Confirmation = this->ConnectionConfirmPacket(senderIP.toInteger());
        m_send(Confirmation, senderIP, senderPort);
        return;
    }
    
    if (m_password == sentPassword) // if password is correct
    {
        m_clientData.insert(new ClientData{senderPort, (ID)ip});
       
        // send confirmation as password was correct
        sf::Packet Confirmation = this->ConnectionConfirmPacket(senderIP.toInteger());
        m_send(Confirmation, senderIP, senderPort);
        this->onClientConnected.invoke((ID)ip, m_threadSafeEvents, m_overrideEvents);
    }
    else
    {
        sf::Packet passwordRequest;
        passwordRequest = this->PasswordRequestPacket();
        m_send(passwordRequest, senderIP, senderPort);
    }
}

// --------------------------

//* Connection Functions

void Server::setPasswordRequired(bool requirePassword)
{ 
    if (isConnectionOpen()) // since connection is open we dont want to edit this
        return;

    this->m_needsPassword = requirePassword; 
    if (!this->m_needsPassword) 
        setPassword(""); 
}

void Server::setPasswordRequired(bool requirePassword, const std::string& password)
{ 
    if (isConnectionOpen()) // since connection is open we dont want to edit this
        return;

    this->m_needsPassword = requirePassword; 
    setPassword(password); 
}

bool Server::isPasswordRequired() const
{
    return m_needsPassword;
}

void Server::sendToAll(sf::Packet& packet, std::list<ID> blacklist)
{
    for (auto& client: m_clientData)
    {
        if (std::find(blacklist.begin(), blacklist.end(), client->id) == blacklist.end()) // if we did not find the client then it is NOT in the black list
            m_send(packet, sf::IpAddress((IP)client->id), client->port);
    }
}

bool Server::sendTo(sf::Packet& packet, ID id)
{
    if (id != 0) 
    {
        ClientData* client = m_getClientData(id);

        // if the client was not found
        if (client == nullptr) return false;

        m_send(packet, sf::IpAddress((IP)id), client->port);
        return true;
    }
    return false;
}

bool Server::disconnectClient(ID id, const std::string& reason)
{
    ClientData temp = ClientData{0, id}; // port does not matter only id
    auto iter = m_clientData.find(&temp);
    if (iter != m_clientData.end())
    {
        sf::Packet removePacket = this->ConnectionCloseTemplate(reason);
        sendTo(removePacket, id);
        delete(*iter); // freeing the memory as we store client data as a pointer
        m_clientData.erase(iter);
        this->onClientDisconnected.invoke(id, reason, m_threadSafeEvents, m_overrideEvents);
        return true;
    }
    else return false;
}

void Server::disconnectAllClients(const std::string& reason)
{
    sf::Packet removePacket = this->ConnectionCloseTemplate(reason);

    this->sendToAll(removePacket);

    m_clientData.clear();
}

const std::unordered_set<ClientData*>& Server::getClients() const
{ return m_clientData;}

sf::Uint32 Server::getClientsSize() const
{ return (sf::Uint32)m_clientData.size();}

const ClientData* Server::getClientData(ID clientID) const
{
    ClientData temp = ClientData{0, clientID}; // port does not matter only id
    auto iter = m_clientData.find(&temp);
    if (iter == m_clientData.end())
        return nullptr;
    return *iter;
}

void Server::allowClientConnection(bool allowed)
{
    m_allowClientConnection = allowed;
}

bool Server::isClientConnectionAllowed()
{
    return m_allowClientConnection;
}

//* Pure Virtual Definitions

bool Server::tryOpenConnection()
{
    if (this->bind(getPort()) != sf::Socket::Status::Done)
        return false;

    m_connectionOpen = true;
    startThreads();
    this->onConnectionOpen.invoke(m_threadSafeEvents, m_overrideEvents);
    return true;
}

void Server::closeConnection()
{
    if (!isConnectionOpen()) return;

    disconnectAllClients("Server Closing");

    m_reset_connection_data();
    stopThreads();
    close();

    onConnectionClose.invoke("Called \"closeConnection\"", m_threadSafeEvents, m_overrideEvents);
}

// -------------------------

// ---------------------
