#ifndef CLIENT_SOCKET_HPP
#define CLIENT_SOCKET_HPP

#pragma once

#include "Socket.hpp"

namespace udp

{

/// @note if the server is hosted on the same computer as the client the ID given to onDataReceived will be the same as this id
class Client : public Socket
{
private:

    //* Client Variables

        IpAddress_t m_serverIP;
        /// @brief unsigned short _serverPort;
        bool m_wrongPassword = false;
        /// @brief Time since last packet from server
        float m_timeSinceLastPacket = 0.0;
        unsigned short m_serverPort = 7777;

    // -----------------

    //* Thread Functions

        virtual void m_update_function(float deltaTime) override;
        inline virtual void m_second_update_function() override {} // dont want to do anything with the second update as a client

    // -----------------

    //* Protected Connection Functions
    
        /// @brief use only when connection is closed
        virtual void m_reset_connection_data() override;
    
    // ---------------------

    //* Packet Parsing Functions

        virtual void m_parse_data(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_connection_close(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_connection_confirm(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        virtual void m_parse_password_request(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;
        // virtual void m_parse_wrong_password(sf::Packet& packet, sf::IpAddress senderIP, PORT senderPort) override;

    // -------------------------

public:

    //* Initializer and Deconstructor

        Client(sf::IpAddress serverIP, PORT serverPort);
        Client(PORT serverPort);
        ~Client();

    // ------------------------------

    //* Events

        /// @brief Called when ever password is requested
        /// @note password is requested when wrong password is sent
        EventHelper::Event onPasswordRequest;
        /// @brief Invoked when the server port is changed 
        /// @note Optional parameter PORT (unsigned short)
        EventHelper::EventDynamic<PORT> onServerPortChanged;
        /// @brief Invoked when the server ip is changed
        /// @note Optional parameter sf::IpAddress
        EventHelper::EventDynamic<sf::IpAddress> onServerIpChanged;

    // -------

    //* Connection Functions
        
        /// @brief is true until another password is sent
        /// @note password status is unknown until this is true or connection is open
        /// @return true is wrong password
        bool wasIncorrectPassword();
        void setAndSendPassword(const std::string& password);
        void sendPasswordToServer();
        /// @note does nothing if the connection is open
        /// @note a std::nullopt IP will result in no data being set
        /// @returns if the data was set or not
        bool setServerData(IpAddress_t serverIP, PORT serverPort);
        /// @note does nothing if the connection is open
        /// @note a std::nullopt IP will result in no data being set
        /// @returns if the data was set or not
        bool setServerData(IpAddress_t serverIP);
        /// @note does nothing if the connection is open
        bool setServerData(PORT port);
        /// @brief sends the packet to the server
        /// @warning must not send data when there is an invalid server IP set
        void sendToServer(sf::Packet& packet);
        /// @brief returns the time in seconds
        float getTimeSinceLastPacket() const;
        IpAddress_t getServerIP() const;
        unsigned int getServerPort() const;

        //* Pure Virtual Definitions

            /// @brief attempts to connect to the server with the current server data
            /// @returns true for successful send of connection attempt (DOES NOT MEAN THERE IS A CONNECTION CONFIRMATION)
            virtual bool tryOpenConnection();
            /// @brief closes the connection to the server
            virtual void closeConnection(const std::string& reason = "Client Closed Connection");
        
        // -------------------------

    // ------------------------------------------------

};

}

#endif
