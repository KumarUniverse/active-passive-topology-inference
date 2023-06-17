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
}

// Destructor
overlayApplication::~overlayApplication()
{
    NS_LOG_FUNCTION(this);

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
    meta = netw;
    SetTopoIdx(topoIdx);
    SetLocalID(localId);
    
    //send_sockets.resize(meta->n_nodes_gt[getTopoIdx()], 0);
    m_peerPort = 9;
    recv_socket = 0; // null pointer
}

void overlayApplication::SetLocalID(uint32_t localID)
{
    NS_LOG_FUNCTION(this);
    m_local_ID = localID;
}

uint32_t overlayApplication::GetLocalID(void) const
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

void overlayApplication::SetSocket(Address ip, uint32_t idx, uint32_t deviceID)
{
    NS_LOG_FUNCTION(this);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    send_sockets[idx] = Socket::CreateSocket(GetNode(), tid);
    if (Ipv4Address::IsMatchingType(ip) == true)
    {
        if (send_sockets[idx]->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        send_sockets[idx]->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(ip), m_peerPort));
    }
    else if (InetSocketAddress::IsMatchingType(ip) == true)
    {
        if (send_sockets[idx]->Bind() == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        send_sockets[idx]->Connect(ip);
    }
    else
    {
        NS_ASSERT_MSG(false, "Incompatible address type: " << ip);
    }
    send_sockets[idx]->SetAllowBroadcast(false);
}

// void overlayApplication::SendBackground(uint32_t idx)
// {
//     //NS_LOG_FUNCTION(this);
//     NS_ASSERT(pkt_event[idx].IsExpired());

//     std::string src_dest_key {std::to_string(m_local_ID) + " " + std::to_string(idx)};
//     std::vector<int> &route = meta->routing_map[src_dest_key];
//     SDtag tagToSend;
//     SetTag(tagToSend, m_local_ID, idx, 1, m_sent, 1);
    
//     Ptr<Packet> p = Create<Packet>(meta->pkt_size);
//     p->AddPacketTag(tagToSend);

//     //tagToSend.SetUeID(idx); // idx is the ID of the destination UE.
//     //tagToSend.SetBwpID(0);
//     p->ReplacePacketTag(tagToSend);
//     //std::cout << route[1] << std::endl; // for debugging. route vector works as expected.
//     send_sockets[route[1]]->Send(p); // ERROR: assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"

//     meta->pkt_received[src_dest_key] = false;
//     ++m_sent; // increment the num of pkts sent.
    
//     pkt_event[idx] = Simulator::Schedule(dt, &overlayApplication::SendBackground, this, idx);
// }

// void overlayApplication::ScheduleBackground(Time dt, uint32_t idx)
// {
//     if (idx >= meta->unodes_start_idx)
//         pkt_event[idx] = Simulator::Schedule(dt, &overlayApplication::SendBackground, this, dt, idx);
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
     * Set up background traffic. No need for now.
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