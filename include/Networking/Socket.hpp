#ifndef SOCKET_HPP
#define SOCKET_HPP

#pragma once

#include <thread>

#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Network/Packet.hpp>

#include "Utils/funcHelper.hpp"
#include "Utils/EventHelper.hpp"

/// @note adds the size of the data then the data stored in the given packet
inline sf::Packet& operator <<(sf::Packet& packet, const sf::Packet& otherPacket)
{
    packet << otherPacket.getDataSize();
    packet.append(otherPacket.getData(), otherPacket.getDataSize());
    return packet;
}

inline sf::Packet& operator >>(sf::Packet& packet, sf::Packet& otherPacket)
{
    size_t sizeOfData;
    packet >> sizeOfData;
    std::vector<sf::Int8> data(sizeOfData, 0);
    for (size_t i = 0; i < sizeOfData; i++)
    {
        packet >> data[i];
    }
    otherPacket.append(&data.front(), sizeOfData);
    return packet;
}

namespace udp
{

// ID = Uint32
typedef sf::Uint32 ID;
// IP = ID (Uint32)
typedef ID IP;
typedef unsigned short PORT;

enum PacketType : sf::Int8
{
    Data = 0,
    ConnectionRequest = 1,
    ConnectionClose = 2,
    ConnectionConfirm = 3,
    PasswordRequest = 4,
    Password = 5
};

class Socket : protected sf::UdpSocket
{
protected:

    //* Connection Data
    
        IP m_ip = 0;
        bool m_needsPassword = false;
        std::string m_password = "";
        unsigned short m_port = 777;
        /// @brief if the server is open or the client is connected
        bool m_connectionOpen = false;
        /// @brief time that the connection has been up
        double m_connectionTime = 0.f;
        float m_timeoutTime = 20.f; 
        funcHelper::func<void> m_packetSendFunction = {[](){}};

    // ----------------

    //* Thread Variables and Functions

        const bool m_threadSafeEvents = true;
        bool m_overrideEvents = false;
        // stop source is universal
        std::stop_source* m_sSource = nullptr;
        // receiving thread
        std::jthread* m_receiveThread = nullptr;
        // sending/updating thread
        std::jthread* m_updateThread = nullptr;
        // if we should be sending packets in the thread
        bool m_sendingPackets = true;
        // in updates/second
        unsigned int m_socketUpdateRate = 64;

        /// @brief called every update (at the socket update rate)
        /// @note must be thread safe
        virtual void m_update_function(float deltaTime) = 0;
        /// @brief called every second when the update thread is running
        /// @note must be thread safe
        /// @note has a maximum of once every second but could end up being slower
        virtual void m_second_update_function() = 0;
        virtual void m_receive_packets_thread(std::stop_token sToken);
        virtual void m_update_thread(std::stop_token sToken);

    // -------------------------

    //* Protected Connection Functions

        /// @brief use only when connection is closed
        virtual void m_reset_connection_data();
    
    // ---------------------

    //* Packet Parsing

        /// @brief Called when a data packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_data(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when a connection request packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_connection_request(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when a connection close packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_connection_close(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when a connection confirm packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_connection_confirm(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when a password request packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_password_request(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when a password packet is received
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_password(sf::Packet& packet, sf::IpAddress ip, PORT port) {}
        /// @brief Called when the packet identifier is unknown
        /// @param packet the packet data (Do NOT store this packet, ONLY parse the data)
        /// @param ip the senders IpAddress
        /// @param port the sender Port
        inline virtual void m_parse_unkown(sf::Packet& packet, sf::IpAddress ip, PORT port) {}

    // -------------------------

    //* Socket Functions

        /// @brief attempts to send a packet to the given ip and port
        /// @note if the packet fails to send throws runtime error
        void m_send(sf::Packet& packet, sf::IpAddress ip, PORT port);

    // -----------------

public:

    //* Events
        
        /// @brief Invoked when data is received
        /// @note Optional parameter sf::Packet
        /// @note Optional parameter ID the senders ID
        EventHelper::EventDynamic2<sf::Packet, ID> onDataReceived; // TODO optimize this so that there is less copies of the packet made
        /// @brief Invoked when the update rate is changed
        /// @note Optional parameter unsigned int
        EventHelper::EventDynamic<unsigned int> onUpdateRateChanged;
        /// @brief Invoked when the client timeout time has been changed
        /// @note Optional parameter float (timeout in seconds)
        EventHelper::EventDynamic<float> onTimeoutChanged;
        /// @brief Invoked when this port is changed 
        /// @note Optional parameter Port (unsigned short)
        EventHelper::EventDynamic<PORT> onPortChanged;
        /// @brief Invoked when the password is changed
        /// @note Optional parameter New Password (string)
        EventHelper::EventDynamic<std::string> onPasswordChanged;
        /// @note Server -> Open
        /// @note Client -> Connection Confirmed
        EventHelper::Event onConnectionOpen;
        /// @note Server -> Closed
        /// @note Client -> Disconnected
        /// @note Optional parameter the reason for connection close
        EventHelper::EventDynamic<std::string> onConnectionClose;

    // ------

    //* Initializer and Deconstructor

        Socket();
        ~Socket();

    // ------------------------------

    //* Public Thread Functions

        /// @brief this will NOT reset any state data (connection open, ect.)
        void startThreads();
        /// @brief this will NOT reset any state data (connection open, ect.)
        void stopThreads();
        /// @brief if true then anytime and event is called multiple times in one frame only the last call will be invoked at EventHelper::Event::ThreadSafe::update()
        /// @note default: false
        void setThreadSafeOverride(bool override);
        bool getThreadSafeOverride() const;

    // ---------------------

    //* Connection Functions

        //* Pure Virtual Functions

            virtual bool tryOpenConnection() = 0;
            virtual void closeConnection() = 0;

        // -----------------------

    // ---------------------

    //* Getters

        /// @returns ID
        ID getID() const;
        /// @returns IP as IPAddress
        sf::IpAddress getIP() const;
        /// @returns Local IP as IPAddress
        sf::IpAddress getLocalIP() const;
        /// @returns IP as integer
        IP getIntIP() const;
        /// @returns the time in seconds
        double getConnectionTime() const;
        /// @returns the time in seconds
        double getOpenTime() const;
        /// @returns the update interval in updates per second
        unsigned int getUpdateInterval() const;
        /// @returns this port
        unsigned int getPort() const;
        /// @returns current client timeout time in seconds
        float getTimeout() const;
        /// @returns the current password
        std::string getPassword() const;
        /// @returns the function that is called when sending a packet
        const funcHelper::func<void>& getPacketSendFunction() const;

    // -------

    //* Setters

        /// @brief sets the update interval in updates/second 
        /// @note DEFAULT = 64 (64 updates/second)
        /// @note this will only take effect after the socket is restarted
        /// @note does not do anything if the connection is open
        void setUpdateInterval(unsigned int interval);
        /// @returns true if packets are being sent at the interval that was set
        /// @note does not do anything if the connection is open
        void sendingPackets(bool sendPackets);
        /// @brief sets this password
        /// @note if this derived class is the server, sets the server password, else sets the password that will be sent to server
        /// @note does not do anything if the connection is open
        void setPassword(const std::string& password);
        /// @brief sets the time for a client to timeout if no packets are sent or received (seconds)
        /// @note does not do anything if the connection is open
        void setTimeout(float timeout);
        /// @brief sets the sending packet function
        /// @note if the socket connection is open then you cannot set the function
        /// @note does not do anything if the connection is open
        void setPacketSendFunction(const funcHelper::func<void>& packetSendFunction = {[](){}});
        /// @note does not do anything if the connection is open
        void setPort(PORT port);

    // --------

    //* Boolean Question Functions

        /// @returns true if the client is connected or server is open
        bool isConnectionOpen() const;
        /// @returns if the receiving thread is running
        bool isReceivingPackets() const;
        /// @returns if this is sending packets
        bool isSendingPackets() const;
        /// @returns if this needs a password
        bool NeedsPassword() const;
        /// @brief Checks if the given ipAddress is valid
        /// @note if it is invalid program will freeze for a few seconds
        static bool isValidIpAddress(sf::IpAddress ipAddress);
        /// @brief Checks if the given ipAddress is valid
        /// @note if it is invalid program will freeze for a few seconds
        static bool isValidIpAddress(sf::Uint32 ipAddress);
        /// @brief Checks if the given ipAddress is valid
        /// @note if it is invalid program will freeze for a few seconds
        static bool isValidIpAddress(const std::string& ipAddress);

    // ---------------------------

    //* Template Functions

        static sf::Packet ConnectionCloseTemplate(std::string reason);
        static sf::Packet ConnectionRequestTemplate();
        static sf::Packet DataPacketTemplate();
        /// @param id the id that the client should use for identification
        static sf::Packet ConnectionConfirmPacket(sf::Uint32 id);
        static sf::Packet PasswordRequestPacket();
        static sf::Packet PasswordPacket(const std::string& password);

    // -------------------
};

}

#endif // SOCKETBASE_H
