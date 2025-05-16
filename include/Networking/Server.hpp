#ifndef SERVER_SOCKET_HPP
#define SERVER_SOCKET_HPP

#pragma once

#include <unordered_set>

#include "Socket.hpp"
#include "ClientData.hpp"

namespace udp
{

class Server : public Socket
{
private:  

    //* Server Variables and Functions

        /// @brief first value is for the ID and the second is for the client data
        std::unordered_set<ClientData*> m_clientData;
        std::atomic<bool> m_allowClientConnection = true;

        /// @returns the clientData ptr or nullptr if no client found with given id
        ClientData* m_getClientData(ID clientID);

        virtual void m_update_function(float deltaTime) override;
        /// @brief the function to be called every second in update
        virtual void m_second_update_function() override;

    // -----------------

    //* Protected Connection Functions
    
        /// @brief use only when connection is closed
        virtual void m_reset_connection_data() override;
    
    // ---------------------

    //* Packet Parsing Functions

        virtual void m_parse_data(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_connection_request(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_connection_close(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_password(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;

    // -------------------------

public:

    //* Initializer and Deconstructor

        Server(PORT port, bool passwordRequired = false);
        ~Server();

    // ------------------------------

    //* Events

        /// @brief Invoked when a client connects
        /// @note Optional parameter the ID of the connected client
        EventHelper::EventDynamic<ID> onClientConnected;
        /// @brief Invoked when a client disconnects
        /// @note Optional parameter the ID of the connected client
        /// @note Optional parameter a string holding the reason for a disconnect
        EventHelper::EventDynamic2<ID, std::string> onClientDisconnected;

    // -------

    //* Connection Functions

        /// @note does nothing if the connection is open
        void setPasswordRequired(bool requirePassword);
        /// @note does nothing if the connection is open
        void setPasswordRequired(bool requirePassword, const std::string& password);
        bool isPasswordRequired() const;
        /// @param reason the reason for the disconnect that the client will receive
        void disconnectAllClients(const std::string& reason = "Disconnect All Clients");
        /// @brief removes the client with the given ID
        /// @param reason the reason for the disconnect that the client will receive
        /// @returns true if the client was removed returns false if it was not found
        bool disconnectClient(ID id, const std::string& reason);
        /// @returns a pointer to the clients map
        const std::unordered_set<ClientData*>& getClients() const;
        /// @returns the number of clients
        std::uint32_t getClientsSize() const;
        /// @returns the clientData ptr or nullptr if no client found with given id
        const ClientData* getClientData(ID clientID) const;
        /// @brief Sends the given packet to every client currently connected
        /// @param blacklist the list of client IDs NOT to send this packet to
        void sendToAll(sf::Packet& packet, std::list<ID> blacklist = {});
        /// @brief tries to send the given packet to the client with the given ID
        /// @returns if the packet was sent (false if client was not found)
        bool sendTo(sf::Packet& packet, ID id);
        /// @brief sets if clients are allowed to connect with or without the password
        /// @note if there is a password the client still needs to enter it (if true)
        /// @note if false the client cannot connect until set true
        void allowClientConnection(bool allowed = true);
        /// @returns true if clients are able to connect
        bool isClientConnectionAllowed();

        //* Pure Virtual Definitions
            
            /// @returns true if server was started with the port that was previously set
            virtual bool tryOpenConnection() override;
            /// @brief closes the server and disconnects all clients
            virtual void closeConnection(const std::string& reason = "Server Closed Connection via function call") override;

        // -------------------------

    // ------------------------------------------------
};

}

#endif
