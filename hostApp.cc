#include "hostApp.h"

namespace ns3
{


NS_LOG_COMPONENT_DEFINE("hostApp");
NS_OBJECT_ENSURE_REGISTERED(hostApp);

TypeId hostApp::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::hostApp")
                            .SetParent<overlayApplication>()
                            .SetGroupName("Applications")
                            .AddConstructor<hostApp>();
    return tid;
}

TypeId hostApp::GetInstanceTypeId (void) const
{
  	return hostApp::GetTypeId ();
}

// Constructor
hostApp::hostApp()
{
    NS_LOG_FUNCTION(this);
}

// Destructor
hostApp::~hostApp()
{
    NS_LOG_FUNCTION(this);
}

void hostApp::Bar() // for debugging
{
    std::cout << "Bar" << std::endl;
}

// App initialization method
void hostApp::InitApp(netmeta *netw, uint32_t localId, int topoIdx)
{
    overlayApplication::InitApp(netw, localId, topoIdx);

    num_nodes = meta->n_nodes_gt[topoIdx];
    //num_pkts_sent_per_dest.resize(num_nodes);
    //num_probes_sent_per_pair.resize(num_nodes * (num_nodes-1));
    //pkt_event.resize(num_nodes);
}

void hostApp::SendPacket(Time dt, uint32_t node_idx)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(pkt_event[node_idx].IsExpired());

    // Stop sending packets once the packet limit has been reached.
    if (num_pkts_sent_per_dest[node_idx] > meta->max_num_pkts_per_dest)
        return;

    //std::string src_dest_key {std::to_string(m_local_ID) + " " + std::to_string(node_idx)};
    SDtag tagToSend;
    SetTag(tagToSend, m_local_ID, node_idx, pktID++);
    
    Ptr<Packet> p = Create<Packet>(meta->pkt_size);
    p->AddPacketTag(tagToSend);

    //SetSocket(ip, node_idx, 0); // need to do this in main file active_passive.cc
    // TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    // send_sockets[node_idx] = Socket::CreateSocket(GetNode(), tid);
    // InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 0);
    // send_sockets[node_idx]->Bind(local);
    // InetSocketAddress remote = InetSocketAddress(m_peerPort, ListenPort);
    // send_sockets[node_idx]->Connect(remote);
    send_sockets[node_idx]->Send(p);

    meta->pkt_received[node_idx] = false;
    num_pkts_sent_per_dest[node_idx]++; // increment the num of pkts sent.
    
    pkt_event[node_idx] = Simulator::Schedule(dt, &hostApp::SendPacket, this, dt, node_idx);
    
}

void hostApp::SchedulePackets(Time dt)
{
    /** Schedules a new packet to be sent to node with index idx. */
    // Only schedule a packet to be sent if the current node is
    // an underlay node.
    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        if (meta->is_leaf_node(topo_idx, node_idx))
        {
            pkt_event[node_idx] = Simulator::Schedule(dt, &hostApp::SendPacket, this, dt, node_idx);
        }
    } 
}

void hostApp::SendProbe(Time dt, uint32_t node_idx1, uint32_t node_idx2)
{
    NS_LOG_FUNCTION(this);
    auto probe_pair = std::pair<uint32_t, uint32_t>(node_idx1, node_idx2);
    NS_ASSERT(probe_event[probe_pair].IsExpired());

    // Stop sending probes once the probe limit has been reached.
    if (num_probes_sent_per_dest[node_idx1] > meta->max_num_probes_per_pair ||
        num_probes_sent_per_dest[node_idx2] > meta->max_num_probes_per_pair)
        return;

    std::string src_dest1_key {std::to_string(m_local_ID) + " " + std::to_string(node_idx1)};
    std::string src_dest2_key {std::to_string(m_local_ID) + " " + std::to_string(node_idx2)};
    SDtag tagToSend1;
    SetTag(tagToSend1, m_local_ID, node_idx1, -1, 1, probeID);
    SDtag tagToSend2;
    SetTag(tagToSend2, m_local_ID, node_idx2, -1, 1, probeID++);
    
    Ptr<Packet> p1 = Create<Packet>(meta->pkt_size);
    p1->AddPacketTag(tagToSend1);
    Ptr<Packet> p2 = Create<Packet>(meta->pkt_size);
    p2->AddPacketTag(tagToSend2);

    send_sockets[node_idx1]->Send(p1);
    send_sockets[node_idx2]->Send(p2);

    meta->probe_received[node_idx1] = false;
    meta->probe_received[node_idx2] = false;
    num_probes_sent_per_dest[node_idx1]++; // increment the num of probes sent.
    num_probes_sent_per_dest[node_idx2]++; // increment the num of probes sent.
    
    probe_event[probe_pair]
        = Simulator::Schedule(dt, &hostApp::SendProbe, this, dt, node_idx1, node_idx2);
}

void hostApp::ScheduleProbes(Time dt)
{
    for (uint32_t node_idx1 = 1; node_idx1 < num_nodes; node_idx1++)
    {
        for (uint32_t node_idx2 = node_idx1+1; node_idx2 < num_nodes; node_idx2++)
        {
            auto probe_pair = std::pair<uint32_t, uint32_t>(node_idx1, node_idx2);

            if (meta->is_leaf_node(topo_idx, node_idx1) && meta->is_leaf_node(topo_idx, node_idx2))
            {
                probe_event[probe_pair] = Simulator::Schedule(dt, &hostApp::SendProbe, this, dt, node_idx1, node_idx2);
            }
        }
    } 
}

void hostApp::StartApplication(void)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::StartApplication();
    
    /**
     * Send packets and probes
    */
    uint32_t pkt_delay = 100; // in milliseconds
    uint32_t probe_delay = 1; // in seconds
    // ^^the time to wait before sending the next packet/probe.
    SchedulePackets(Time(MilliSeconds(pkt_delay)));
    ScheduleProbes(Time(Seconds(probe_delay)));
}

void hostApp::StopApplication(void)
{
    overlayApplication::StopApplication();
}

}