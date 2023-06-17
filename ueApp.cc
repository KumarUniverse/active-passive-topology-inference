#include "ueApp.h"

namespace ns3
{


NS_LOG_COMPONENT_DEFINE("ueApp");
NS_OBJECT_ENSURE_REGISTERED(ueApp);

TypeId ueApp::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::ueApp")
                            .SetParent<overlayApplication>()
                            .SetGroupName("Applications")
                            .AddConstructor<ueApp>();
    return tid;
}

TypeId ueApp::GetInstanceTypeId (void) const
{
  	return ueApp::GetTypeId ();
}

// Constructor
ueApp::ueApp()
{
    NS_LOG_FUNCTION(this);
}

// Destructor
ueApp::~ueApp()
{
    NS_LOG_FUNCTION(this);
    //meta->received_probes_gt[topo_idx].clear(); // already done in meta class upon destruction
}


void ueApp::InitUeApp(netmeta *netw, uint32_t localId, int topoIdx, overlayApplication &app_interface)
{
    overlayApplication::InitApp(netw, localId, topoIdx);

    m_local_ID = app_interface.GetLocalID();
    oa_interface = &app_interface;
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    recv_socket = Socket::CreateSocket(GetNode(), tid);
    recv_socket_1 = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), PORT);
    InetSocketAddress local_1 = InetSocketAddress(Ipv4Address::GetAny(), PORT+1);
    if (recv_socket->Bind(local) == -1)
    {
        std::cout << "Failed to bind socket" << std::endl;
        // NS_FATAL_ERROR("Failed to bind socket 0");
    }
    if (recv_socket_1->Bind(local_1) == -1)
    {
        std::cout << "Failed to bind socket" << std::endl;
        // NS_FATAL_ERROR("Failed to bind socket 1");
    }

    recv_socket->SetRecvCallback(MakeCallback(&ueApp::HandleRead, this));
    recv_socket_1->SetRecvCallback(MakeCallback(&ueApp::HandleRead, this));

    //max_probes = app_interface.meta->_MAXPKTNUM; // add later
}

void ueApp::SetRecvSocket(void)
{
    /**
     * Set up socket for forwarding
     **/
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    recv_socket = Socket::CreateSocket(GetNode(), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), ListenPort);
    if (recv_socket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    recv_socket->SetRecvCallback(MakeCallback(&ueApp::HandleRead, this));
}

void ueApp::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);

    Ptr<Packet> packet;
    Address from;
    Address localAddress;
    while ((packet = socket->RecvFrom(from)))
    {
        socket->GetSockName(localAddress);

        SDtag tagPktRecv;
        packet->PeekPacketTag(tagPktRecv);
        std::string keys{std::to_string(tagPktRecv.GetSourceID()) + ' ' + std::to_string(tagPktRecv.GetDestID())};
        // NS_LOG_INFO("Node ID: " << m_local_ID << "; pkt received -- " << keys);

        if (tagPktRecv.GetDestID() == GetLocalID()) // the current node is the intended receiver of the packet
        {
            // if the received pkt is a probe, do this:
            if (tagPktRecv.GetIsProbe() > 0)
            {
                uint32_t probeID = tagPktRecv.GetProbeID();
                if (meta->received_probes_gt[topo_idx].find(probeID) == meta->received_probes_gt[topo_idx].end())
                {   // store probe in a map if matching probe pkt is not found.
                    int64_t probe_start_time = tagPktRecv.GetStartTime();
                    int64_t probe_delay = Simulator::Now().GetNanoSeconds() - probe_start_time;
                    
                    std::vector<int64_t> probe_stats2;
                    probe_stats2.push_back(tagPktRecv.GetDestID());
                    probe_stats2.push_back(probe_start_time);
                    probe_stats2.push_back(probe_delay);
                    
                    meta->received_probes_gt[topo_idx][probeID] = probe_stats2;
                }
                else // matching probe pkt already received.
                {
                    int64_t probe_start_time = tagPktRecv.GetStartTime();
                    int64_t probe_delay = Simulator::Now().GetNanoSeconds() - probe_start_time;

                    std::vector<int64_t> probe_stats1;
                    probe_stats1.push_back(tagPktRecv.GetDestID());
                    probe_stats1.push_back(probe_start_time);
                    probe_stats1.push_back(probe_delay);
                    
                    std::vector<int64_t> probe_stats2 = meta->received_probes_gt[topo_idx][probeID];
                    if (probe_stats2[0] < probe_stats1[0])
                    {   // Swap probe stats to make the first stat
                        // the one with the smaller path idx.
                        std::vector<int64_t> temp = probe_stats1;
                        probe_stats1 = probe_stats2;
                        probe_stats2 = temp;
                    }
                    std::vector<int64_t> probe_stats;
                    probe_stats.reserve(probe_stats1.size() + probe_stats2.size());
                    probe_stats.insert(probe_stats.end(), probe_stats1.begin(), probe_stats1.end());
                    probe_stats.insert(probe_stats.end(), probe_stats2.begin(), probe_stats2.end());
                    meta->probe_delays_gt[topo_idx].emplace_back(probe_stats);
                    meta->received_probes_gt[topo_idx].erase(probeID);
                }
            }
            else if (tagPktRecv.GetSourceID() == 0) // if received pkt is sent from source node
            {
                int64_t pkt_start_time = tagPktRecv.GetStartTime();
                int64_t pkt_delay = Simulator::Now().GetNanoSeconds() - pkt_start_time;

                std::vector<int64_t> pkt_stats;
                pkt_stats.push_back(tagPktRecv.GetDestID());
                pkt_stats.push_back(pkt_start_time);
                pkt_stats.push_back(pkt_delay);

                meta->pkt_delays_gt[topo_idx].emplace_back(pkt_stats);
            }
        }
    }
}


void ueApp::StartApplication(void)
{
    // std::cout << "UE_App Start at: " << local_ID_ << " " << std::endl;
    NS_LOG_FUNCTION(this);

    overlayApplication::StartApplication();
}

void ueApp::StopApplication(void)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::StopApplication();

    // If the receive socket 1 is still open, close it.
    if (recv_socket_1 != 0)
    {
        // std::cout << "iter Node ID: " << m_local_ID << " recv_socket" << std::endl;
        recv_socket_1->Close();
        recv_socket_1->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    //meta->received_probes_gt[topo_idx].clear(); // already done in meta class upon destruction
}

}