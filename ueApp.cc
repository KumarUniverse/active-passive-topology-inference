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

    StopApplication();
    overlayApplication::StopApplication();
}


void ueApp::InitUeApp(netmeta *netw, uint32_t localId, int topoIdx)
{
    NS_LOG_FUNCTION(this);
    
    overlayApplication::InitApp(netw, localId, topoIdx);
}

void ueApp::SetRecvSocket(Address myIP, uint32_t idx)
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
    recv_socket->SetRecvCallback(MakeCallback(&ueApp::HandleRead, this));
}

void ueApp::HandleRead(Ptr<Socket> socket)
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

        if (tagPktRecv.GetDestID() == GetLocalID()) // the current node is the intended recepient of the packet
        {
            // if the received pkt is a probe, do this:
            if (tagPktRecv.GetIsProbe() == 1)
            {
                num_probes_received++;
                uint32_t probeID = tagPktRecv.GetProbeID();
                //std::cout << "The received packet is a probe. Probe ID: " << probeID << std::endl;
                if (meta->received_probes.find(probeID) == meta->received_probes.end())
                {   // store probe in a map if matching probe pkt is not found.
                    int64_t probe_start_time = tagPktRecv.GetStartTime();
                    int64_t probe_delay = Simulator::Now().GetNanoSeconds() - probe_start_time;
                    
                    std::vector<int64_t> probe_stats2;
                    probe_stats2.push_back(meta->dest_idx_to_path_idx[tagPktRecv.GetDestID()]+1);
                    probe_stats2.push_back(probe_start_time);
                    probe_stats2.push_back(probe_delay);
                    
                    // Store probe stats in meta class.
                    meta->received_probes[probeID] = probe_stats2;
                }
                else // if matching probe pkt was already received,
                {   // then record the active measurement.
                    int64_t probe_start_time = tagPktRecv.GetStartTime();
                    int64_t probe_delay = Simulator::Now().GetNanoSeconds() - probe_start_time;

                    std::vector<int64_t> probe_stats1;
                    probe_stats1.push_back(meta->dest_idx_to_path_idx[tagPktRecv.GetDestID()]+1);
                    probe_stats1.push_back(probe_start_time);
                    probe_stats1.push_back(probe_delay);
                    
                    std::vector<int64_t> probe_stats2 = meta->received_probes[probeID];
                    if (probe_stats2[0] < probe_stats1[0]) // second stat has a smaller path idx than first stat
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
                    meta->probe_delays.emplace_back(probe_stats);
                    meta->received_probes.erase(probeID);
                }

                // Regularly write out the probe delays measured to a .csv file
                // and clear the probe delays vector to avoid consuming too much memory.
                if (GetLocalID() == *(meta->dest_nodes.rbegin())
                    && num_probes_received % meta->probe_write_freq == 0)
                {
                    meta->write_probe_delays_for_curr_topo();
                    // if (num_probes_received % (meta->probe_write_freq*10) == 0)
                    //     std::cout << "Num. of probes received at last node: " << num_probes_received << std::endl;
                }
            }
            else if (tagPktRecv.GetSourceID() == meta->host_idx) // if received pkt is sent from source node,
            {   // then record the passive measurement.
                //std::cout << "Source ID = 0. Packet is a data packet." << std::endl; // for debugging
                num_data_pkts_received++;
                int64_t pkt_start_time = tagPktRecv.GetStartTime();
                int64_t pkt_delay = Simulator::Now().GetNanoSeconds() - pkt_start_time; // in ns

                std::vector<int64_t> pkt_stats;
                pkt_stats.push_back(meta->dest_idx_to_path_idx[tagPktRecv.GetDestID()]+1);
                pkt_stats.push_back(pkt_start_time);
                pkt_stats.push_back(pkt_delay);

                // For debugging:
                // std::cout << "Packet stats:" << std::endl;
                // for (int i = 0; i < (int)pkt_stats.size(); i++)
                // {
                //     std::cout << pkt_stats[i] << " ";
                // }
                // std::cout << std::endl;

                // Store packet stats in meta class.
                meta->pkt_delays.emplace_back(pkt_stats);

                // Regularly write out the pkt delays measured to a .csv file
                // and clear the pkt delays vector to avoid consuming too much memory.
                if (GetLocalID() == *(meta->dest_nodes.rbegin())
                    && num_data_pkts_received % meta->pkt_write_freq == 0)
                {
                    meta->write_pkt_delays_for_curr_topo();
                    // if (num_data_pkts_received % (meta->pkt_write_freq*10) == 0)
                    //     std::cout << "Num. of pkts received at last node: " << num_data_pkts_received << std::endl;
                }
            }
            else if (tagPktRecv.GetIsBckgrd() == 1) { // received pkt is a bckgrd pkt.
                num_bkgrd_pkts_received++;
                // std::cout << "Background traffic packet received." << std::endl;
                // std::cout << "LocalID: " << (int)GetLocalID() << ", DestID: " << (int)tagPktRecv.GetDestID() << std::endl;
            }
            else { // all other packets
                num_other_pkts_received++;
            }
        }
        // Else don't read the packet.
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

    // If the receive socket is still open, close it.
    if (recv_socket != 0)
    {
        // std::cout << "iter Node ID: " << m_local_ID << " recv_socket" << std::endl;
        recv_socket->Close();
        recv_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    // For debugging
    // std::cout << "LocalID: " << (int)GetLocalID() << ", Num. of pkts received: " << num_data_pkts_received
    //     << ", Num. of probes received: " << num_probes_received
    //     << ", Num. of bkgrd packets received: " << num_bkgrd_pkts_received << std::endl;
}

}