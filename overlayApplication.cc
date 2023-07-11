#include "overlayApplication.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/net-device.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/queue.h"
#include "ns3/uinteger.h"
#include "ns3/random-variable-stream.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-routing-table-entry.h"
#include "SDtag.h"
#include <assert.h>
#include "netmeta.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("overlayApplication");
NS_OBJECT_ENSURE_REGISTERED(overlayApplication);

TypeId overlayApplication::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::overlayApplication")
                            .SetParent<Application>()
                            .SetGroupName("Applications")
                            .AddConstructor<overlayApplication>()
                            .AddAttribute("RemotePort",
                                            "The destination port of the outbound packets.",
                                            UintegerValue(0),
                                            MakeUintegerAccessor(&overlayApplication::m_peerPort),
                                            MakeUintegerChecker<uint16_t>())
                            .AddAttribute("ListenPort", "Port on which to listen for incoming packets.",
                                            UintegerValue(0),
                                            MakeUintegerAccessor(&overlayApplication::ListenPort),
                                            MakeUintegerChecker<uint16_t>());

    return tid;
}

TypeId overlayApplication::GetInstanceTypeId (void) const
{
  	return overlayApplication::GetTypeId ();
}

// Constructor
overlayApplication::overlayApplication()
{
    NS_LOG_FUNCTION(this);
    recv_socket = 0;
}

// Destructor
overlayApplication::~overlayApplication()
{
    NS_LOG_FUNCTION(this);

    StopApplication();
    send_sockets.clear();
    recv_socket = 0;
    // m_sendEvent.clear(); // for bckgrd traffic
    pktID = 0;
    probeID = 0;
}

void overlayApplication::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

// App initialization method
void overlayApplication::InitApp(netmeta *netw, uint32_t localId, int topoIdx) //, uint32_t MaxPktSize)
{
    NS_LOG_FUNCTION(this);

    meta = netw;
    SetTopoIdx(topoIdx);
    SetLocalID(localId);
    
    m_peerPort = 9;
    recv_socket = 0; // null pointer
}

void overlayApplication::SetLocalID(uint32_t localID)
{
    NS_LOG_FUNCTION(this);
    m_local_ID = (uint8_t)localID;
}

uint8_t overlayApplication::GetLocalID(void) const
{
    NS_LOG_FUNCTION(this);
    return m_local_ID;
}

void overlayApplication::SetTopoIdx(int topoIdx)
{
    NS_LOG_FUNCTION(this);
    topo_idx = topoIdx;
}

int overlayApplication::getTopoIdx(void) const
{
    NS_LOG_FUNCTION(this);
    return topo_idx;
}

// void overlayApplication::Foo() // for debugging
// {
//     std::cout << "Foo" << std::endl;
// }

// Ptr<Socket> overlayApplication::SetSocket(Address ip, uint32_t idx, uint32_t deviceID)
// {
//     NS_LOG_FUNCTION(this);

//     TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
//     recv_socket = Socket::CreateSocket(GetNode(), tid);
    
//     if (Ipv4Address::IsMatchingType(ip) == true)
//     {
//         if (recv_socket->Bind() == -1)
//         {
//             NS_FATAL_ERROR("Failed to bind socket");
//         }
//         recv_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(ip), m_peerPort));
//     }
//     else if (InetSocketAddress::IsMatchingType(ip) == true)
//     {
//         if (recv_socket->Bind() == -1)
//         {
//             NS_FATAL_ERROR("Failed to bind socket");
//         }
//         recv_socket->Connect(ip);
//     }
//     else
//     {
//         NS_ASSERT_MSG(false, "Incompatible address type: " << ip);
//     }
//     //recv_socket->SetAllowBroadcast(false);

//     return recv_socket;
// }

void overlayApplication::SetSendSocket(Address remoteAddr, uint32_t destIdx)
{
    /**
     * Set up a new socket for receiving packets and reading them.
     * remoteAddr - The IP address of destination node.
     * destIdx - The index of the destination node.
     **/
    NS_LOG_FUNCTION(this);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> send_socket = Socket::CreateSocket(GetNode(), tid);

    // std::cout << "Overlay App SetSendSocket() called! Idx: " << (uint32_t) GetLocalID()
    // << ", DestIdx: " << destIdx << std::endl; // for debugging
    
    if (Ipv4Address::IsMatchingType(remoteAddr) == true)
    {
        if (send_socket->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        // std::cout << "Binded send socket to Ipv4 address." << std::endl; // for debugging, this gets called
        send_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(remoteAddr), m_peerPort));
    }
    else if (InetSocketAddress::IsMatchingType(remoteAddr) == true)
    {
        if (send_socket->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        // std::cout << "Binded send socket to InetSocket address." << std::endl; // for debugging, not this
        send_socket->Connect(remoteAddr);
    }
    else
    {
        NS_ASSERT_MSG(false, "Incompatible address type: " << remoteAddr);
    }
    send_socket->SetAllowBroadcast(false);
    send_sockets[destIdx] = send_socket; // save the socket in send_sockets map
}

// Don't need if you're using NS3's On-Off Application.
// void overlayApplication::SendBackground(uint32_t idx)
// {
//     //NS_LOG_FUNCTION(this);
//     NS_ASSERT(pkt_event[idx].IsExpired());

//     SDtag tagToSend;
//     SetTag(tagToSend, m_local_ID, idx, 1, m_sent, 1);
    
//     Ptr<Packet> p = Create<Packet>(meta->pkt_size);
//     p->AddPacketTag(tagToSend);

//     //tagToSend.SetUeID(idx); // idx is the ID of the destination UE.
//     //tagToSend.SetBwpID(0);
//     p->ReplacePacketTag(tagToSend);
//     send_sockets[route[1]]->Send(p); // ERROR: assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"
//     ++m_sent; // increment the num of pkts sent.
    
//     pkt_event[idx] = Simulator::Schedule(dt, &overlayApplication::SendBackground, this, idx);
// }

// void overlayApplication::ScheduleBackground(Time dt, uint32_t idx)
// {
//     pkt_event[idx] = Simulator::Schedule(dt, &overlayApplication::SendBackground, this, dt, idx);
// }


void overlayApplication::SetTag(SDtag& tagToUse, uint8_t SourceID, uint8_t DestID,
    uint32_t PktID, uint8_t IsProbe, uint32_t ProbeID)
{
    tagToUse.SetSourceID(SourceID);
    tagToUse.SetDestID(DestID);
    tagToUse.SetPktID(PktID);
    tagToUse.SetIsProbe(IsProbe);
    tagToUse.SetProbeID(ProbeID);
    tagToUse.SetStartTime(Simulator::Now().GetNanoSeconds());
}

void overlayApplication::StartApplication(void)
{
    NS_LOG_FUNCTION(this);
    /**
     * Set up background traffic. Don't need if you're using NS3's On-Off Application.
     * Background traffic is generated using on/off application in main file.
    */
    // Background type: Pareto Burst
    // std::vector<uint32_t> neighbors = meta->neighbors_maps_gt[topo_idx][m_local_ID];
    // for (uint32_t neighbor_ID : neighbors)
    // {
    //     // std::cout << "test background: " << m_local_ID << " " << j << std::endl;
    //     Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    //     Time random_offset = MicroSeconds(rand->GetValue(0, 50));
    //     ScheduleBackground(random_offset, neighbor_ID);
    // }
}

void overlayApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);

    // std::cout << "Node ID: " << m_local_ID << " stop Application" << std::endl;
    // For each send socket, if the socket is still open, close it.
    for (uint32_t i = 0; i < send_sockets.size(); i++)
    {
        // std::cout << "iter Node ID: " << m_local_ID << " i" << i << std::endl;
        if (send_sockets[i] != 0)
        {
            send_sockets[i]->Close();
            send_sockets[i]->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        }
    }
    // If the receive socket is still open, close it.
    if (recv_socket != 0)
    {
        // std::cout << "iter Node ID: " << m_local_ID << " recv_socket" << std::endl;
        recv_socket->Close();
        recv_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    // std::cout << "iter Node ID: " << m_local_ID << " complete" << std::endl;
}

}