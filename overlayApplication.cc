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

#include <ctime>
#include <cstdlib>


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
    bckgrd_pkt_event.clear(); // for bckgrd traffic
    pktID = 0;
    probeID = 0;
}

void overlayApplication::DoDispose(void)
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

// App initialization method
void overlayApplication::InitApp(netmeta *netw, uint32_t localId, int topoIdx)
{
    NS_LOG_FUNCTION(this);

    meta = netw;
    SetTopoIdx(topoIdx);
    SetLocalID(localId);

    num_nodes = meta->n_nodes;
    m_peerPort = 9;
    recv_socket = 0; // null pointer
    keep_running = true;

    // Set up random variables for background traffic generation.
    rand_uniform = CreateObject<UniformRandomVariable>();
    rand_exp = CreateObject<ExponentialRandomVariable>();

    rand_burst_pareto = CreateObject<ParetoRandomVariable>();
    rand_burst_pareto->SetAttribute ("Scale", DoubleValue (meta->on_pareto_scale));
    rand_burst_pareto->SetAttribute ("Shape", DoubleValue (meta->on_pareto_shape));
    rand_burst_pareto->SetAttribute ("Bound", DoubleValue (meta->on_pareto_bound));

    rand_off_pareto = CreateObject<ParetoRandomVariable>();
    rand_off_pareto->SetAttribute ("Scale", DoubleValue (meta->off_pareto_scale));
    rand_off_pareto->SetAttribute ("Shape", DoubleValue (meta->off_pareto_shape));
    rand_off_pareto->SetAttribute ("Bound", DoubleValue (meta->off_pareto_bound));

    rand_log_norm_var = CreateObject<LogNormalRandomVariable>();
    rand_log_norm_var->SetAttribute ("Sigma", DoubleValue (meta->log_normal_sigma));

    // Set up the background traffic function map.
    bckgrd_traff_fn_map[BckgrdTrafficType::CBR] = &overlayApplication::SendCBRBackground;
    bckgrd_traff_fn_map[BckgrdTrafficType::Poisson] = &overlayApplication::SendPoissonBackground;
    bckgrd_traff_fn_map[BckgrdTrafficType::ParetoBurst] = &overlayApplication::SendParetoBackground;
    bckgrd_traff_fn_map[BckgrdTrafficType::LogNormal] = &overlayApplication::SendLogNormBackground;
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

void overlayApplication::SetSendSocket(Address remoteAddr, uint32_t destIdx, uint32_t deviceID)
{
    /**
     * Set up a new socket for sending packets to a particular destination.
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
    map_neighbor_device.insert(std::pair<uint32_t, uint32_t>(destIdx, deviceID));
}

void overlayApplication::SetRecvSocket(Address myIP, uint32_t index)
{
    /**
     * Set up a new socket for receiving packets and reading them.
     * myIP - The IP address of the link interface directly connected to this node.
     * idx - The index of this node.
     * deviceID - The device ID of this node's socket.
     **/
    NS_LOG_FUNCTION(this);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    recv_socket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress localAddress = InetSocketAddress(Ipv4Address::ConvertFrom(myIP), ListenPort);
    //InetSocketAddress localAddress = InetSocketAddress(Ipv4Address::GetAny(), ListenPort);
    // You can also use Ipv4Address::GetAny() for the socket address to indicate that the
    // socket is willing to accept incoming packets on any available network interface or IP address.

    //std::cout << "UE App SetRecvSocket() called! Idx: " << idx << std::endl; // for debugging

    // Bind the socket to the local address of the node.
    if (recv_socket->Bind(localAddress) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    //recv_socket->SetAllowBroadcast(false); // Don't broadcast packets. Don't need for recv socket.
    // Set the receive callback function to read in the packets after receiving them.
    recv_socket->SetRecvCallback(MakeCallback(&overlayApplication::HandleRead, this));
}

void overlayApplication::HandleRead(Ptr<Socket> socket)
{
    /**
     * Handle the reading of data packets, probes and background packets.
     **/
    NS_LOG_FUNCTION(this << socket);

    //std::cout << "Router Socket HandleRead() called." << std::endl; // for debugging

    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        //std::cout << "Router Socket MAC address: " << localAddress << std::endl; // for debugging

        SDtag tagPktRecv;
        packet->PeekPacketTag(tagPktRecv);
        // std::string keys{std::to_string(tagPktRecv.GetSourceID()) + ' ' + std::to_string(tagPktRecv.GetDestID())};
        // NS_LOG_INFO("Node ID: " << m_local_ID << "; pkt received -- " << keys);
        // std::cout << "Packet received at UE " << (int)m_local_ID << "." << std::endl;
        // std::cout << "DestID: " << (int)tagPktRecv.GetDestID() << ", LocalID: " << (int)GetLocalID() << std::endl;

        if (tagPktRecv.GetDestID() == GetLocalID()
            && tagPktRecv.GetIsBckgrd() == 1) // background pkt received
        {
            num_bckgrd_pkts_received++;
        }
        else // pkt is not destined for the curr node
        {
            uint8_t srcIdx = tagPktRecv.GetSourceID();
            uint8_t destIdx = tagPktRecv.GetDestID();
            CheckCongestion(map_neighbor_device[destIdx], srcIdx, destIdx);
        }
    }
}

inline double pktsPerMsToKbps(double pktsPerMs)
{
    double kbps = pktsPerMs * 12000; // pktsPerMs * 1500 * 8 * / 1000 * 1000
    return kbps;
}

// uint64_t bandwidthDelay(uint32_t pkt_size, int num_pkts_sent, uint64_t bckgrd_rate)
// {   // Returns the bandwidth delay in nanoseconds.
//     // bckgrd_rate is in kb/s.
//     uint64_t t = (int) (pkt_size*8*num_pkts_sent / bckgrd_rate * 1e9);
//     return t;
// }

void overlayApplication::SendCBRBackground(uint32_t destIdx)
{
    /**
     * Send a constant bit rate (CBR) of background traffic packets from the current
     * node to one of the neighboring nodes according to a given background rate.
     * After waiting for the time to send a packet, another packet is sent.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bckgrd_pkt_event[destIdx].IsExpired());

    // Send at least one background packet.
    SDtag tag_to_send; // set packet tag to identify background traffic
    SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
    Ptr<Packet> pkt = Create<Packet>(meta->bckgrd_pkt_payload_size);
    pkt->AddPacketTag(tag_to_send);
    send_sockets[destIdx]->Send(pkt);
    //num_bckgrd_pkts_sent++; // for debugging

    // Inter-arrival time of background packets is constant.
    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bckgrd_rate_kbps = meta->edge_bckgrd_rates[src_dest_pair];
    double secs_to_send_pkt = ((meta->phy_bckgrd_pkt_size)*BITS_PER_BYTE)
                                /(bckgrd_rate_kbps * BITS_PER_KB);
    // We only count the time to send the payload, not the headers.
    // print out bckgrd interval for debugging
    // if (debug && m_local_ID == 0 && destIdx == 1)
    // {
    //     std::cout << secs_to_send_pkt << "s at " << (int)m_local_ID << " to " << destIdx << std::endl; // for debugging
    //     std::cout << "Bckgrd rate: " << bckgrd_rate_kbps << " kbps" << std::endl; // for debugging
    //     debug = false;
    //     //keep_running = false;
    // }

    if (keep_running)  // keep sending traffic until application is stopped
    {
        Time pkt_inter_arrival = Seconds( secs_to_send_pkt );
        bckgrd_pkt_event[destIdx] = Simulator::Schedule(pkt_inter_arrival,
                                &overlayApplication::SendCBRBackground, this, destIdx);
    }

}

void overlayApplication::SendPoissonBackground(uint32_t destIdx)
{
    /**
     * Send a Poisson burst of background traffic packets from the current
     * node to one of the neighboring nodes according to a given background rate.
     * After some random waiting period dt, another burst of packets are sent.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bckgrd_pkt_event[destIdx].IsExpired());

    // Send at least one background packet.
    SDtag tag_to_send; // set packet tag to identify background traffic
    SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
    Ptr<Packet> pkt = Create<Packet>(meta->bckgrd_pkt_payload_size);
    pkt->AddPacketTag(tag_to_send);
    send_sockets[destIdx]->Send(pkt);
    //num_bckgrd_pkts_sent++; // for debugging

    // Inter-arrival time of background packets is exponentially distributed.
    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bckgrd_rate_kbps = meta->edge_bckgrd_rates[src_dest_pair];
    double secs_to_send_pkt = ((meta->phy_bckgrd_pkt_size)*BITS_PER_BYTE)
                                /(bckgrd_rate_kbps * BITS_PER_KB);
    // print out bckgrd interval for debugging
    // if (m_local_ID == 0 && destIdx == 1)
    // {
    //     std::cout << time_to_send_pkt << " at " << (int)m_local_ID << " to " << destIdx << std::endl; // for debugging
    // }

    rand_exp->SetAttribute("Mean", DoubleValue(secs_to_send_pkt)); // mean background interval in seconds
    Time pkt_inter_arrival = Seconds( rand_exp->GetValue() );
    // std::cout << "Node ID: " << m_local_ID << " background at " << Simulator::Now().As(Time::US) << " to " << destIdx << " next time " << pkt_inter_arrival.As(Time::US) << std::endl; // for debugging

    if (keep_running)  // keep sending traffic until application is stopped
    {
        bckgrd_pkt_event[destIdx] = Simulator::Schedule(pkt_inter_arrival,
                                &overlayApplication::SendPoissonBackground, this, destIdx);
    }
}

void overlayApplication::SendParetoBackground(uint32_t destIdx)
{
    /**
     * Send a Pareto burst of background traffic packets from the current
     * node to one of the neighboring nodes according to a given background rate.
     * After some random waiting period dt, another burst of packets are sent.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bckgrd_pkt_event[destIdx].IsExpired());

    uint32_t rng_val = rand_burst_pareto->GetInteger(); // ON duration

    double secs_to_send_pkts = 0;
    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bckgrd_rate_pkts_per_ms = meta->edge_bckgrd_rates[src_dest_pair];
    double bckgrd_rate_kbps = pktsPerMsToKbps(bckgrd_rate_pkts_per_ms);
    for (uint32_t i = 0; i < rng_val; i++)
    {
        secs_to_send_pkts = ((meta->phy_bckgrd_pkt_size)*BITS_PER_BYTE)
                                /(bckgrd_rate_kbps * BITS_PER_KB);
        SDtag tag_to_send; // set packet tag to identify background traffic
        SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
        Ptr<Packet> burst_pkt = Create<Packet>(meta->bckgrd_pkt_payload_size);
        burst_pkt->AddPacketTag(tag_to_send);
        send_sockets[destIdx]->Send(burst_pkt);
        //num_bckgrd_pkts_sent++; // for debugging
    }

    rng_val = rand_off_pareto->GetInteger() * MICROSECS_TO_SECS; // OFF duration
    if (keep_running) // keep sending traffic until application is stopped
    {
        Time dt = Time(Seconds(secs_to_send_pkts + rng_val)); // ON + OFF duration in secs
        bckgrd_pkt_event[destIdx] = Simulator::Schedule(dt,
                                &overlayApplication::SendParetoBackground, this, destIdx);
    }
}

void overlayApplication::Helper_Send_Background_Traffic(uint32_t destIdx,
    double timeLeft, double bckgrdRateKbps) {
    /*
     * Helper function for the SendLogNormBackground() function.
     * Used to recursively send back-to-back background traffic packets
     * over a given interval. After timeLeft becomes <= 0, it then
     * calls SendLogNormBackground(), engaging in a mutual recursion.
     * destIdx - The index of the destination node.
     * timeLeft - The time left in the current interval in seconds (500us or 0.5e-3 secs).
     * bckgrdRateKbps - The background traffic rate in kbps.
     */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bckgrd_pkt_event[destIdx].IsExpired());

    SDtag tag_to_send; // set packet tag to identify background traffic
    SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
    Ptr<Packet> pkt = Create<Packet>(meta->bckgrd_pkt_payload_size);
    pkt->AddPacketTag(tag_to_send);
    send_sockets[destIdx]->Send(pkt);
    //num_bckgrd_pkts_sent++; // for debugging
    // std::cout << "Sending 1 background traffic packet from " << (int)m_local_ID
    //     << " to " << destIdx << std::endl; // for debugging
    //if (m_local_ID == 0 && destIdx == 1 && num_bckgrd_pkts_sent >= 1) keep_running = false; // for debugging
    double secs_to_send_pkt = ((meta->phy_bckgrd_pkt_size)*BITS_PER_BYTE)
                                /(bckgrdRateKbps * BITS_PER_KB);
    timeLeft -= secs_to_send_pkt;

    if (!keep_running)
    {   // Stop sending bckgrd traffic
        return;
    }

    if (timeLeft > 0)
    {   // there's still time left to send more packets in this interval
        bckgrd_pkt_event[destIdx] = Simulator::Schedule(Seconds(secs_to_send_pkt),
            &overlayApplication::Helper_Send_Background_Traffic, this, destIdx, timeLeft, bckgrdRateKbps);
    }
    else
    {   // no time left in this interval. Move on to the next time interval.
        bckgrd_pkt_event[destIdx] = Simulator::Schedule(Seconds(secs_to_send_pkt),
        &overlayApplication::SendLogNormBackground, this, destIdx);
    }
}

void overlayApplication::SendLogNormBackground(uint32_t destIdx)
{
    /**
     * Send log normal background traffic from the current
     * node to one of the neighboring nodes according to a given background rate.
     * Sample a new log normal traffic rate every T_rate_interval_sec seconds.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bckgrd_pkt_event[destIdx].IsExpired());

    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bckgrd_rate_kbps = meta->edge_bckgrd_rates[src_dest_pair]; // use if bckgrd rate is already in kbps
    // double bckgrd_rate_pkts_per_ms = meta->edge_bckgrd_rates[src_dest_pair];
    // double bckgrd_rate_kbps = pktsPerMsToKbps(bckgrd_rate_pkts_per_ms);
    double log_normal_mu = log(bckgrd_rate_kbps) - 0.5*meta->log_normal_sigma*meta->log_normal_sigma;
    //^^depends on sigma and traffic rate
    rand_log_norm_var->SetAttribute ("Mu", DoubleValue (log_normal_mu));
    double log_norm_bckgrd_rate = rand_log_norm_var->GetValue(); // in kbps

    if (keep_running) // keep sending traffic until application is stopped
    {
        // std::cout << "Sending background traffic packet..." << std::endl; // for debugging
        //Helper_Send_Background_Traffic(destIdx, meta->T_rate_interval_us, bckgrd_rate_kbps); // for debugging
        Helper_Send_Background_Traffic(destIdx, meta->T_rate_interval_secs, log_norm_bckgrd_rate);
    }
}

void overlayApplication::ScheduleBackground(Time dt)
{
    /**
     * Schedules sending of background traffic packets to neighboring nodes.
     * Note: Don't need this if you are using NS3's built-in On-Off Application.
    */
    std::vector<uint32_t> neighbors = meta->neighbors_map[m_local_ID];
    for (uint32_t destIdx: neighbors)
    {
        auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
        double bckgrd_rate_kbps = meta->edge_bckgrd_rates[src_dest_pair];
        // print out the current node id and the background rate for each neighbor
        // std::cout << "Bckgrd source: " << (int)m_local_ID <<
        //         ", dest: " << (int)destIdx << ", bckgrd_rate_kbps: " << bckgrd_rate_kbps << std::endl;
        //if (!(m_local_ID == 0 && destIdx == 1)) bckgrd_rate_kbps = 0; // for debugging
        if (bckgrd_rate_kbps == 0) continue; // no background traffic is sent for this neighbor
        // bckgrd_pkt_event[destIdx] = Simulator::Schedule(dt, &overlayApplication::SendParetoBackground, this, destIdx);
        srand(time(0));
        int rand_delay_ms = rand() % meta->bckgrd_traff_start_time;
        dt = Time(MilliSeconds(dt.ToInteger(Time::Unit(5)) + rand_delay_ms));

        // Get bckgrd traffic function from map, store in a variable and call it after time dt.
        if (bckgrd_traff_fn_map.find(meta->bckgrd_traffic_type) != bckgrd_traff_fn_map.end())
        {
            auto bckgrd_traff_fn = bckgrd_traff_fn_map[meta->bckgrd_traffic_type];
            bckgrd_pkt_event[destIdx] = Simulator::Schedule(dt, bckgrd_traff_fn, this, destIdx);
        }
        else if (meta->bckgrd_traffic_type != BckgrdTrafficType::NoBckgrd)
        {
            NS_FATAL_ERROR("Background traffic type not found.");
        }
        // bckgrd_pkt_event[destIdx] = Simulator::Schedule(dt, &overlayApplication::SendParetoBackground, this, destIdx);
    }
}

bool overlayApplication::CheckCongestion(uint32_t deviceID, uint32_t src, uint32_t dest)
{
    NS_LOG_FUNCTION(this);

    Ptr<NetDevice> net_raw = GetNode()->GetDevice(deviceID);
    Ptr<PointToPointNetDevice> net_device = DynamicCast<PointToPointNetDevice, NetDevice>(net_raw);
    Ptr<Queue<Packet>> net_queue = net_device->GetQueue();

    if (net_queue->GetNPackets() > 0)
    {
        std::cout << "Congestion at " << (int)m_local_ID << "from " << src << " to " << dest << "with " << net_queue->GetNPackets() << " pkts in queue and " << net_queue->GetNBytes() << " bytes." << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}

void overlayApplication::SetTag(SDtag& tagToUse, uint8_t SourceID, uint8_t DestID,
    uint32_t PktID, uint8_t IsBckgrd, uint8_t IsProbe, uint32_t ProbeID)
{
    tagToUse.SetSourceID(SourceID);
    tagToUse.SetDestID(DestID);
    tagToUse.SetPktID(PktID);
    tagToUse.SetIsBckgrd(IsBckgrd);
    tagToUse.SetIsProbe(IsProbe);
    tagToUse.SetProbeID(ProbeID);
    tagToUse.SetStartTime(Simulator::Now().GetNanoSeconds());
}

void overlayApplication::StartApplication(void)
{
    NS_LOG_FUNCTION(this);

    // Set up background traffic. COMMENT OUT if using NS3's On-Off Application.
    ScheduleBackground(Time(MilliSeconds(meta->bckgrd_traff_start_time)));
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
    keep_running = false;

    // Print current node idx and number of background packets received.
    std::cout << "Node ID: " << (int)m_local_ID << " received " << num_bckgrd_pkts_received << " background packets." << std::endl;
    num_bckgrd_pkts_received = 0;
}

}