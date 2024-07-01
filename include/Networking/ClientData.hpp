#ifndef CLIENT_DATA_HPP
#define CLIENT_DATA_HPP

#pragma once

#include <SFML/Config.hpp>
#include <unordered_set>

#include "Networking/Socket.hpp"

namespace udp
{

class Server;

class ClientData
{
public:
    ClientData(unsigned short port, ID id);
    
    const unsigned short port = 0;
    const ID id = 0;

    unsigned int getPacketsPerSecond() const;
    double getConnectionTime() const;
    float getTimeSinceLastPacket() const;

private:
    friend Server;

    /// @brief TEMP var for storing how many packets are sent between seconds
    unsigned int m_packetsSent = 0;
    /// @brief Should be updated once every second using PacketsSent
    unsigned int m_packetsPerSecond = 0;

    double m_connectionTime = 0.f;
    float m_timeSinceLastPacket = 0.f;
};

}

namespace std {
    template <>
    struct hash<udp::ClientData> {
        inline size_t operator()(const udp::ClientData& data) const noexcept
        {
            return hash<size_t>{}(data.id);
        }
    };
    template <>
    struct hash<udp::ClientData*> {
        inline size_t operator()(const udp::ClientData* data) const noexcept
        {
            if (data == nullptr)
                return 0;
            return hash<size_t>{}(data->id);
        }
    };
    template <>
    struct equal_to<udp::ClientData> {
        inline bool operator()(const udp::ClientData& data, const udp::ClientData& data2) const noexcept
        {
            return data.id == data2.id;
        }
    };
    template <>
    struct equal_to<udp::ClientData*> {
        inline bool operator()(const udp::ClientData* data, const udp::ClientData* data2) const noexcept
        {
            return data->id == data2->id;
        }
    };
}

#endif // CLIENTDATA_H
