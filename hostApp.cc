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

    StopApplication();
    overlayApplication::StopApplication();
}

// App initialization method
void hostApp::InitHostApp(netmeta *netw, uint32_t localId, int topoIdx)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::InitApp(netw, localId, topoIdx);
}

std::vector<int> generateDistinctRandomNumbers(int start, int end, int sampleSize)
{
    /**
     * Generate sampleSize number of distinct random numbers
     * from the range [start, end] (inclusive).
     * The returned list of numbers is sorted in ascending order.
    */

    // In case sample size is too big:
    sampleSize = sampleSize > end - start + 1 ? end - start + 1 : sampleSize;

    // Create a vector to store the generated numbers
    std::vector<int> numbers;
    for (int i = start; i <= end; ++i) {
        numbers.push_back(i);
    }
    
    // Random number generator
    std::random_device rd;
    std::mt19937 rng(rd());
    
    // Perform shuffle on the vector
    std::shuffle(numbers.begin(), numbers.end(), rng);
    
    // Sort the first sampleSize elements in ascending order
    std::sort(numbers.begin(), numbers.begin() + sampleSize);
    
    // Return the first sampleSize elements from the sorted vector
    return std::vector<int>(numbers.begin(), numbers.begin() + sampleSize);
}

void hostApp::SendPacket(Time dt, uint8_t destIdx)
{
    /**
     * Send a packet from the host/source node to one of the leaf/dest nodes.
     * After every dt time interval, another packet is sent to the same leaf node.
    */
    NS_LOG_FUNCTION(this);
    NS_ASSERT(pkt_event[destIdx].IsExpired());

    // Stop sending packets once the packet limit has been reached.
    if (num_pkts_sent_per_dest[destIdx] >= meta->max_num_pkts_per_dest)
        return;

    // std::cout << "Sending pkt: " << "src: " << (int)m_local_ID << ", dest: " << (int)destIdx
    //     << ", PktID: " << pktID << std::endl; // for debugging, tag info correct.
    SDtag tagToSend;
    SetTag(tagToSend, m_local_ID, destIdx, pktID++);
    
    Ptr<Packet> p = Create<Packet>(meta->pkt_size);
    p->AddPacketTag(tagToSend);
    send_sockets[destIdx]->Send(p); // send pkt using to dest node

    // Regularly write the pkt delays to the data files. // Not needed since we do this in ueApp.cc
    // if (num_pkts_sent_per_dest[destIdx] % 10 == 0)
    //     meta->write_pkt_delays_for_curr_topo();

    //meta->pkt_received[destIdx] = false; // probably not needed
    ++num_pkts_sent_per_dest[destIdx]; // increment the num of pkts sent.
    
    // if (topo_idx == 0 && destIdx == 4) // for debugging
    // {
    //     std::cout << "Number of pkts sent to dest 4: " << num_pkts_sent_per_dest[destIdx] << std::endl;
    // }
    
    pkt_event[destIdx] = Simulator::Schedule(dt, &hostApp::SendPacket, this, dt, destIdx);
    
}

void hostApp::SchedulePackets(Time dt)
{
    /** Schedules sending of data packets to all the leaf nodes.*/
    NS_LOG_FUNCTION(this);

    // Add a random delay between the intial calls to SendPackets()
    // to de-synchronize the data packets. 0-100ms random delay
    // Note: The delays must be in increasing order.
    int min_start_delay = 0, max_start_delay = meta->pkt_delay; // in ms
    std::vector<int> init_pkt_delays = generateDistinctRandomNumbers(min_start_delay, max_start_delay,
                            meta->n_leaves);
    // Only schedule a packet to be sent if the destination node is a leaf node.
    int rand_delay_idx = 0;
    for (uint32_t destNodeIdx = 1; destNodeIdx < num_nodes; destNodeIdx++)
    {
        if (meta->is_leaf_node(destNodeIdx))
        {
            int rand_delay = dt.ToInteger(Time::Unit(5)) + init_pkt_delays[rand_delay_idx++]
                                + meta->bkgrd_traff_delay; // Time::Unit(5) = milliseconds
            Time init_dt = Time(MilliSeconds(rand_delay));
            pkt_event[destNodeIdx] = Simulator::Schedule(init_dt, &hostApp::SendPacket, this, dt, destNodeIdx);
        }
    }
    init_pkt_delays.clear();
}

void hostApp::SendProbe(Time dt, uint8_t destIdx1, uint8_t destIdx2)
{
    /**
     * Send a probe consisting of 2 back-to-back packets from the host/source node
     * to 2 of the leaf/dest nodes. One packet per leaf node.
     * After every dt time interval, another probe is sent to the same 2 leaf nodes.
    */
    NS_LOG_FUNCTION(this);
    auto probe_pair = std::make_pair(destIdx1, destIdx2);
    NS_ASSERT(probe_event[probe_pair].IsExpired());

    // NEED TO FIX. Should be per pair, not per dest.
    // Stop sending probes once the probe limit has been reached.
    if (num_probes_sent_per_pair[probe_pair] >= meta->max_num_probes_per_pair)
        return;

    // std::cout << "Sending probe: " << "src: " << m_local_ID << ", dest: " << destIdx1
    //     << ", ProbeID: " << probeID << std::endl; // for debugging
    SDtag tagToSend1;
    SetTag(tagToSend1, m_local_ID, destIdx1, 0, 0, 1, probeID);
    SDtag tagToSend2;
    SetTag(tagToSend2, m_local_ID, destIdx2, 0, 0, 1, probeID++);
    
    Ptr<Packet> p1 = Create<Packet>(meta->pkt_size);
    p1->AddPacketTag(tagToSend1);
    Ptr<Packet> p2 = Create<Packet>(meta->pkt_size);
    p2->AddPacketTag(tagToSend2);

    // Send probe:
    send_sockets[destIdx1]->Send(p1);
    send_sockets[destIdx2]->Send(p2);

    // Regularly write the pkt delays to the data files. // Not needed since we do this in ueApp.cc
    // if (num_probes_sent_per_pair[probe_pair] % 10 == 0)
    //     meta->write_probe_delays_for_curr_topo();

    // meta->probe_received[destIdx1] = false; // probably not needed
    // meta->probe_received[destIdx2] = false;
    ++num_probes_sent_per_pair[probe_pair]; // increment the num of probes sent to pair (idx1, idx2)

    // if (topo_idx == 0 && destIdx1 == 4 && destIdx2 == 7) // for debugging
    // {
    //     std::cout << "Number of pkts sent to dest pair (4,7): " << num_probes_sent_per_pair[probe_pair] << std::endl;
    // }

    probe_event[probe_pair]
        = Simulator::Schedule(dt, &hostApp::SendProbe, this, dt, destIdx1, destIdx2);
}

void hostApp::ScheduleProbes(Time dt)
{
    /** Schedules sending of probe packets to all possible leaf node pairs.*/
    NS_LOG_FUNCTION(this);

    // Add a random delay between the intial calls to SendProbes()
    // to de-synchronize the probes. 0-1000ms random delay
    // Note: The delays must be in increasing order.
    int min_start_delay = 0, max_start_delay = meta->probe_delay*SECS_TO_MS; // in ms
    std::vector<int> init_probe_delays = generateDistinctRandomNumbers(min_start_delay, max_start_delay,
                            (int)(meta->n_leaves*(meta->n_leaves-1)/2));

    // Only schedule a probe if both of the destination nodes are leaves.
    int rand_delay_idx = 0;
    for (uint8_t destNodeIdx1 = 1; destNodeIdx1 < num_nodes; destNodeIdx1++)
    {
        for (uint8_t destNodeIdx2 = destNodeIdx1+1; destNodeIdx2 < num_nodes; destNodeIdx2++)
        {
            auto probe_pair = std::make_pair(destNodeIdx1, destNodeIdx2);

            if (meta->is_leaf_node(destNodeIdx1) && meta->is_leaf_node(destNodeIdx2))
            {
                int rand_delay = dt.ToInteger(Time::Unit(5)) + init_probe_delays[rand_delay_idx++]
                                    + meta->bkgrd_traff_delay; // in ms
                Time init_dt = Time(MilliSeconds(rand_delay));
                probe_event[probe_pair]
                    = Simulator::Schedule(init_dt, &hostApp::SendProbe, this, dt, destNodeIdx1, destNodeIdx2);
            }
        }
    }
    init_probe_delays.clear();
}

void hostApp::StartApplication(void)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::StartApplication();
    
    /**
     * Schedule packet and probe sending.
    */
    SchedulePackets(Time(MilliSeconds(meta->pkt_delay)));
    ScheduleProbes(Time(Seconds(meta->probe_delay)));
}

void hostApp::StopApplication(void)
{
    NS_LOG_FUNCTION(this);

    // Stop sending packets.
    for (uint32_t destNodeIdx = 1; destNodeIdx < num_nodes; destNodeIdx++)
    {
        if (meta->is_leaf_node(destNodeIdx))
        {
            num_pkts_sent_per_dest[destNodeIdx] = meta->max_num_pkts_per_dest;
        }
    }

    // Stop sending probes.
    for (auto const& key_value_pair : num_probes_sent_per_pair)
    {
        std::pair<uint32_t, uint32_t> probe_pair = key_value_pair.first;
        num_probes_sent_per_pair[probe_pair] = meta->max_num_probes_per_pair;
    }

    overlayApplication::StopApplication();
}

}