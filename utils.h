#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/udp-socket.h"
#include "ns3/log.h"
#include <math.h>
#include <unistd.h>

namespace ns3
{

// #define SRC 17
// #define DEST 3
// #define ISNR trues

void txTraceIpv4(std::string context, Ptr<const Packet> packet, Ptr<Ipv4> ptr_ipv4, uint32_t dontknow);
void p2pDevMacTx(std::string context, Ptr<const Packet> packet);
void p2pDevMacRx(std::string context, Ptr<const Packet> packet);
// void trace_udpClient(std::string context, Ptr<const Packet> packet);
void trace_PhyTxBegin(std::string context, Ptr<const Packet> packet);
void trace_PhyTxEnd(std::string context, Ptr<const Packet> packet);
// void trace_PhyRxBegin(std::string context, Ptr<const Packet> packet);
void trace_PhyRxEnd(std::string context, Ptr<const Packet> packet);

}


#endif