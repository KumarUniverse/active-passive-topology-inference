#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "utils.h"
#include "SDtag.h"

namespace ns3
{

void txTraceIpv4(std::string context, Ptr<const Packet> packet, Ptr<Ipv4> ptr_ipv4, uint32_t dontknow){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe() > 0)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    } */
    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    // std::cout << context << "\t" << Now() << ": packet sent with size: " << packet->GetSize() << std::endl;
}
void p2pDevMacTx(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe() > 0)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    } */
    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe()>0 && tagPktRecv.GetSandWichLargeID() == 4)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << " sdID=" << (uint32_t)tagPktRecv.GetSandWichID() << std::endl;
    } */
    
    // std::cout << context << "\t" << Now() << ": packet sent from NetDev with size: " << packet->GetSize() << std::endl;
}
void p2pDevMacRx(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe() > 0)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    } */
    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    // std::cout << context << "\t" << Now() << ": packet received from NetDev with size:" << packet->GetSize() << std::endl;
}
void trace_PhyTxBegin(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    // std::cout << context << "\t" << Now() << ": PHY sent begin with size: " << packet->GetSize() << std::endl;

    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe()>0 && tagPktRecv.GetSandWichLargeID() == 4)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << " sdID=" << (uint32_t)tagPktRecv.GetSandWichID() << std::endl;
    } */
}
void trace_PhyTxEnd(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    // std::cout << context << "\t" << Now() << ": PHY sent end with size: " << packet->GetSize() << std::endl;
}
void trace_PhyRxEnd(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    // std::cout << context << "\t" << Now() << ": PHY received end with size: " << packet->GetSize() << std::endl;
}



}

