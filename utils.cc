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
    std::cout << "Packet sent at Ipv4 layer!" << std::endl;
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
    std::cout << "Packet sent with size: " << packet->GetSize() << " bytes." << std::endl;
}

void rxTraceIpv4(std::string context, Ptr<const Packet> packet, Ptr<Ipv4> ptr_ipv4, uint32_t dontknow){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet received at Ipv4 layer!" << std::endl;
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
    std::cout << "Packet received with size: " << packet->GetSize() << " bytes." << std::endl;
}

void p2pDevMacTx(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe() > 0)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    } */
    std::cout << "Frame sent at MAC layer!" << std::endl;
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
    std::cout << "Frame sent from NetDev with size: " << packet->GetSize() << " bytes." << std::endl;
}

void p2pDevMacRx(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);
    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe() > 0)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    } */
    std::cout << "Frame received at MAC layer!" << std::endl;
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
    std::cout << "Frame received from NetDev with size: " << packet->GetSize() << " bytes." << std::endl;
}

void trace_PhyTxBegin(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet transmission beginning at physical layer!" << std::endl;
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
    std::cout << "PHY sent begin with size: " << packet->GetSize() << std::endl;

    /* if (tagPktRecv.GetSourceID() == SRC && tagPktRecv.GetDestID() == DEST && tagPktRecv.GetIsProbe()>0 && tagPktRecv.GetSandWichLargeID() == 4)
    {
        std::cout << context << "\t" << Now() << ", PktID= " << tagPktRecv.GetPktID()  << ": src:" << (uint32_t)tagPktRecv.GetSourceID() << " dest:" << (uint32_t)tagPktRecv.GetDestID() << " sdID=" << (uint32_t)tagPktRecv.GetSandWichID() << std::endl;
    } */
}

void trace_PhyTxEnd(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet transmission ending at physical layer!" << std::endl;
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
    std::cout << "PHY sent end with size: " << packet->GetSize() << std::endl;
}

void trace_PhyRxEnd(std::string context, Ptr<const Packet> packet){
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet received at physical layer!" << std::endl;
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
    std::cout << "PHY received end with size: " << packet->GetSize() << std::endl;
}

void dropQueueTrace(std::string context, Ptr<const Packet> packet){
     // "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/Queue/Drop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped from queue!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

void dropIpv4Trace(std::string context, const Ipv4Header &header, Ptr<const Packet> packet,
    Ipv4L3Protocol::DropReason reason, ns3::Ptr<ns3::Ipv4> ipv4, unsigned int dontknow){
     // "/NodeList/*/$ns3::Ipv4L3Protocol/Drop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped at IPv4 layer!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

void dropMacTxTrace(std::string context, Ptr<const Packet> packet){
     // "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDrop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped during MAC transmission!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

void dropPhyTxTrace(std::string context, Ptr<const Packet> packet){
     // "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxDrop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped during physical transmission!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

void dropPhyRxTrace(std::string context, Ptr<const Packet> packet){
     // "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDrop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped during physical receive!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

void dropRxBufferTrace(std::string context, Ptr<const Packet> packet){
     // "/NodeList/*/$ns3::PacketSink/RxBuffer/Drop"
    SDtag tagPktRecv;
    packet->PeekPacketTag(tagPktRecv);

    std::cout << "Packet dropped at receive buffer!" << std::endl;

    if (tagPktRecv.GetIsProbe() == 1)
    {   // Packet transmitted is a probe.
        std::cout << context << "\t" << Now() << ", ProbeID=" << tagPktRecv.GetProbeID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
        std::cout << ", PktID=" << tagPktRecv.GetPktID() << std::endl;
    }
    else if (tagPktRecv.GetIsBckgrd() == 1)
    {   // Packet transmitted is a background packet.
        std::cout << context << "\t" << Now() << ", BckgrdID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
    else
    {   // Packet transmitted is not a probe or background packet. It is a data packet.
        std::cout << context << "\t" << Now() << ", PktID=" << tagPktRecv.GetPktID()  << ", src:"
        << (uint32_t)tagPktRecv.GetSourceID() << ", dest:" << (uint32_t)tagPktRecv.GetDestID() << std::endl;
    }
}

}
