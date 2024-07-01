#include "Networking/ClientData.hpp"

using namespace udp;

ClientData::ClientData(unsigned short port, ID id) : port(port), id(id)
{}

unsigned int ClientData::getPacketsPerSecond() const
{
    return m_packetsPerSecond;
}

double ClientData::getConnectionTime() const
{
    return m_connectionTime;
}

float ClientData::getTimeSinceLastPacket() const
{
    return m_timeSinceLastPacket;
}

