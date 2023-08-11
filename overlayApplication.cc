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
    bkgrd_pkt_event.clear(); // for bckgrd traffic
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

    num_nodes = meta->n_nodes_gt[topoIdx];
    m_peerPort = 9;
    recv_socket = 0; // null pointer
    keep_running = true;

    rand_burst_pareto = CreateObject<ParetoRandomVariable>();
    rand_burst_pareto->SetAttribute ("Scale", DoubleValue (meta->on_pareto_scale));
    rand_burst_pareto->SetAttribute ("Shape", DoubleValue (meta->on_pareto_shape));
    rand_burst_pareto->SetAttribute ("Bound", DoubleValue (meta->on_pareto_bound));

    off_pareto = CreateObject<ParetoRandomVariable>();
    off_pareto->SetAttribute ("Scale", DoubleValue (meta->off_pareto_scale));
    off_pareto->SetAttribute ("Shape", DoubleValue (meta->off_pareto_shape));
    off_pareto->SetAttribute ("Bound", DoubleValue (meta->off_pareto_bound));

    rand_log_norm_var = CreateObject<LogNormalRandomVariable>();
    rand_log_norm_var->SetAttribute ("Sigma", DoubleValue (meta->log_normal_sigma));
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

void overlayApplication::SetRecvSocket(Address myIP, uint32_t idx, uint32_t deviceID)
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
     * Handle the reading of data packets and probes. 
     **/
    NS_LOG_FUNCTION(this << socket);

    //std::cout << "UE Socket HandleRead() called." << std::endl; // for debugging

    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);
        //std::cout << "UE Socket MAC address: " << localAddress << std::endl; // for debugging

        SDtag tagPktRecv;
        packet->PeekPacketTag(tagPktRecv);
        // std::string keys{std::to_string(tagPktRecv.GetSourceID()) + ' ' + std::to_string(tagPktRecv.GetDestID())};
        // NS_LOG_INFO("Node ID: " << m_local_ID << "; pkt received -- " << keys);
        // std::cout << "Packet received at UE " << (int)m_local_ID << "." << std::endl;
        // std::cout << "DestID: " << (int)tagPktRecv.GetDestID() << ", LocalID: " << (int)GetLocalID() << std::endl;

        if (tagPktRecv.GetDestID() == GetLocalID()
            && tagPktRecv.GetIsBckgrd() == 1) // background pkt received
        {
            num_bkgrd_pkts_received++;
        }
    }
}

double pktsPerMsToKbps(double pktsPerMs)
{
    double kbps = pktsPerMs * 12000; // pktsPerMs * 1500 * 8 * / 1000 * 1000
    return kbps;
}

// uint64_t bandwidthDelay(uint32_t pkt_size, int num_pkts_sent, uint64_t bkgrd_rate)
// {   // Returns the bandwidth delay in nanoseconds.
//     // bkgrd_rate is in kb/s.
//     uint64_t t = (int) (pkt_size*8*num_pkts_sent / bkgrd_rate * 1e9);
//     return t;
// }

void overlayApplication::SendParetoBackground(uint32_t destIdx)
{
    /**
     * Send a Pareto burst of background traffic packets from the current
     * node to one of the neighboring nodes according to a given background rate.
     * After some random waiting period dt, another burst of packets are sent.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bkgrd_pkt_event[destIdx].IsExpired());

    uint32_t rng_val = rand_burst_pareto->GetInteger(); // ON duration

    std::vector<Ptr<Packet>> vec_burst_pkt(rng_val);
    double time_to_send_pkts = 0;
    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bkgrd_rate_pkts_per_ms = meta->edge_bkgrd_traffic_rates_gt[topo_idx][src_dest_pair];
    double bkgrd_rate_kbps = pktsPerMsToKbps(bkgrd_rate_pkts_per_ms);
    for (uint32_t i = 0; i < rng_val; i++)
    {
        time_to_send_pkts += (long double)(meta->pkt_size*BITS_PER_BYTE)
                                / (bkgrd_rate_kbps*BITS_PER_KB) * 1000; // in microseconds
        SDtag tag_to_send; // set packet tag to identify background traffic
        SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
        vec_burst_pkt[i] = Create<Packet>(meta->pkt_size);
        vec_burst_pkt[i]->AddPacketTag(tag_to_send);
        send_sockets[destIdx]->Send(vec_burst_pkt[i]);
    }
    
    rng_val = off_pareto->GetInteger(); // OFF duration
    if (keep_running == true) // keep sending traffic until application is stopped
    {
        Time dt = Time(MicroSeconds(time_to_send_pkts + rng_val));
        bkgrd_pkt_event[destIdx] = Simulator::Schedule(dt,
                                &overlayApplication::SendParetoBackground, this, destIdx);
    }
}

void overlayApplication::Helper_Send_Background_Traffic(uint32_t destIdx,
    double timeLeft, double bckgrdRate) {
    double time_to_send_pkts = double( meta->pkt_size * BITS_PER_BYTE ) / bckgrdRate * 1e3; // bits/kbps * 1e3 = us

    SDtag tag_to_send; // set packet tag to identify background traffic
    SetTag(tag_to_send, m_local_ID, destIdx, 0, 1);
    Ptr<Packet> pkt = Create<Packet>(meta->pkt_size);
    pkt->AddPacketTag(tag_to_send);
    send_sockets[destIdx]->Send(pkt);
    timeLeft -= time_to_send_pkts;

    if (!keep_running)
    {   // Stop sending bckgrd traffic if application is stopped
        return;
    }

    if (timeLeft > 0)
    {   // there's still time left to send more packets in this interval
        Simulator::Schedule(MicroSeconds(time_to_send_pkts),
            &overlayApplication::Helper_Send_Background_Traffic, this, destIdx, timeLeft, bckgrdRate);
    }
    else
    {   // no time left in this interval. Move on to the next time interval.
        Simulator::Schedule(MicroSeconds(time_to_send_pkts),
        &overlayApplication::SendLogNormBackground, this, destIdx);
    }
}

void overlayApplication::SendLogNormBackground(uint32_t destIdx)
{
    /**
     * Send log normal background traffic from the current
     * node to one of the neighboring nodes according to a given background rate.
     * Sample a new log normal traffic rate every T_rate_interval_us microseconds.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(bkgrd_pkt_event[destIdx].IsExpired());

    auto src_dest_pair = std::make_pair((uint32_t)m_local_ID, destIdx);
    double bkgrd_rate_pkts_per_ms = meta->edge_bkgrd_traffic_rates_gt[topo_idx][src_dest_pair];
    double bkgrd_rate_kbps = pktsPerMsToKbps(bkgrd_rate_pkts_per_ms);
    double log_normal_mu = log(bkgrd_rate_kbps) - 0.5*meta->log_normal_sigma*meta->log_normal_sigma;
    //^^depends on sigma and traffic rate
    rand_log_norm_var->SetAttribute ("Mu", DoubleValue (log_normal_mu));
    double log_norm_bckgrd_rate = rand_log_norm_var->GetValue(); // in kbps

    if (keep_running) // keep sending traffic until application is stopped
    {
        // std::cout << "Sending background traffic packet..." << std::endl; // for debugging
        Helper_Send_Background_Traffic(destIdx, meta->T_rate_interval_us, log_norm_bckgrd_rate);
    }
}

void overlayApplication::ScheduleBackground(Time dt)
{
    /**
     * Schedules sending of background traffic packets to neighboring nodes.
     * Note: Don't need this if you are using NS3's On-Off Application.
    */
    std::vector<uint32_t> neighbors = meta->neighbors_maps_gt[topo_idx][m_local_ID];
    for (uint32_t destIdx: neighbors)
    {
        // bkgrd_pkt_event[destIdx] = Simulator::Schedule(dt, &overlayApplication::SendParetoBackground, this, destIdx);
        bkgrd_pkt_event[destIdx] = Simulator::Schedule(dt, &overlayApplication::SendLogNormBackground, this, destIdx);
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
    ScheduleBackground(Time(MilliSeconds(meta->bkgrd_traff_delay)));
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
}

}