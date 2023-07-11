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

void hostApp::Bar() // for debugging
{
    std::cout << "Bar" << std::endl;
}

// App initialization method
void hostApp::InitHostApp(netmeta *netw, uint32_t localId, int topoIdx)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::InitApp(netw, localId, topoIdx);
    num_nodes = meta->n_nodes_gt[topoIdx];
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
    
    // Perform Fisher-Yates shuffle on the vector
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

    //std::string src_dest_key {std::to_string(m_local_ID) + " " + std::to_string(destIdx)};
    SDtag tagToSend;
    std::cout << "Sending pkt: " << "src: " << (int)m_local_ID << ", dest: " << (int)destIdx
        << ", PktID: " << pktID << std::endl; // for debugging, tag info correct.
    SetTag(tagToSend, m_local_ID, destIdx, pktID++);
    // std::cout << "Sending pkt: " << "src: " << (int)tagToSend.GetSourceID() << ", dest: " << (int)tagToSend.GetDestID()
    //     << ", PktID: " << tagToSend.GetPktID() << std::endl; // for debugging, stays the same
    
    Ptr<Packet> p = Create<Packet>(meta->pkt_size);
    p->AddPacketTag(tagToSend);
    // UNCOMMENT:
    send_sockets[destIdx]->Send(p); // send pkt using dest node's socket
    // ERROR: assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"
    // SDtag tagPktRecv;
    // p->PeekPacketTag(tagPktRecv);
    // std::cout << "Sending pkt: " << "src: " << (int)tagPktRecv.GetSourceID() << ", dest: " << (int)tagPktRecv.GetDestID()
    //     << ", PktID: " << tagPktRecv.GetPktID() << std::endl; // for debugging, stays the same

    //meta->pkt_received[destIdx] = false; // probably not needed
    ++num_pkts_sent_per_dest[destIdx]; // increment the num of pkts sent.
    
    pkt_event[destIdx] = Simulator::Schedule(dt, &hostApp::SendPacket, this, dt, destIdx);
    
}

void hostApp::SchedulePackets(Time dt)
{
    /** Schedules sending of data packets to all the leaf nodes.*/
    NS_LOG_FUNCTION(this);

    // Add a random delay between the intial calls to SendPackets()
    // to de-synchronize the data packets. 0-100ms random delay
    // Note: The delays must be in increasing order.
    int min_start_delay = 0, max_start_delay = meta->pkt_delay;
    std::vector<int> init_pkt_delays = generateDistinctRandomNumbers(min_start_delay, max_start_delay,
                            meta->n_leaves_gt[meta->topo_idx]);
    // Only schedule a packet to be sent if the destination node is a leaf node.
    int rand_delay_idx = 0;
    for (uint32_t destNodeIdx = 1; destNodeIdx < num_nodes; destNodeIdx++)
    {
        if (meta->is_leaf_node(topo_idx, destNodeIdx))
        {
            int rand_delay = dt.ToInteger(Time::Unit(5)) + init_pkt_delays[rand_delay_idx++]; // 5 = milliseconds
            Time init_dt = Time(MilliSeconds(rand_delay));
            pkt_event[destNodeIdx] = Simulator::Schedule(init_dt, &hostApp::SendPacket, this, dt, destNodeIdx);
        }
    } 
}

void hostApp::SendProbe(Time dt, uint8_t destIdx1, uint8_t destIdx2)
{
    /**
     * Send a probe consisting of 2 back-to-back packets from the host/source node
     * to 2 of the leaf/dest nodes. One packet per leaf node.
     * After every dt time interval, another probe is sent to the same 2 leaf nodes.
    */
    NS_LOG_FUNCTION(this);
    auto probe_pair = std::pair<uint32_t, uint32_t>(destIdx1, destIdx2);
    NS_ASSERT(probe_event[probe_pair].IsExpired());

    // Stop sending probes once the probe limit has been reached.
    if (num_probes_sent_per_dest[destIdx1] >= meta->max_num_probes_per_pair ||
        num_probes_sent_per_dest[destIdx2] >= meta->max_num_probes_per_pair)
        return;

    SDtag tagToSend1;
    std::cout << "Sending probe: " << "src: " << m_local_ID << ", dest: " << destIdx1
        << ", ProbeID: " << probeID << std::endl;
    SetTag(tagToSend1, m_local_ID, destIdx1, 0, 1, probeID);
    SDtag tagToSend2;
    SetTag(tagToSend2, m_local_ID, destIdx2, 0, 1, probeID++);
    
    Ptr<Packet> p1 = Create<Packet>(meta->pkt_size);
    p1->AddPacketTag(tagToSend1);
    Ptr<Packet> p2 = Create<Packet>(meta->pkt_size);
    p2->AddPacketTag(tagToSend2);

    // UNCOMMENT to send packets:
    // send_sockets[destIdx1]->Send(p1);
    // send_sockets[destIdx2]->Send(p2);

    // meta->probe_received[destIdx1] = false; // probably not needed
    // meta->probe_received[destIdx2] = false;
    ++num_probes_sent_per_dest[destIdx1]; // increment the num of probes sent to node idx1.
    ++num_probes_sent_per_dest[destIdx2]; // increment the num of probes sent to node idx2.

    probe_event[probe_pair]
        = Simulator::Schedule(dt, &hostApp::SendProbe, this, dt, destIdx1, destIdx2);
}

void hostApp::ScheduleProbes(Time dt)
{
    /** Schedules sending of probe packets to all possible leaf node pairs.*/
    NS_LOG_FUNCTION(this);

    for (uint8_t destNodeIdx1 = 1; destNodeIdx1 < num_nodes; destNodeIdx1++)
    {
        for (uint8_t destNodeIdx2 = destNodeIdx1+1; destNodeIdx2 < num_nodes; destNodeIdx2++)
        {
            //auto probe_pair = std::pair<uint32_t, uint32_t>(destNodeIdx1, destNodeIdx2);

            if (meta->is_leaf_node(topo_idx, destNodeIdx1) && meta->is_leaf_node(topo_idx, destNodeIdx2))
            {
                //probe_event[probe_pair] = Simulator::Schedule(dt, &hostApp::SendProbe, this, dt, destNodeIdx1, destNodeIdx2);
            }
        }
    } 
}

void hostApp::StartApplication(void)
{
    NS_LOG_FUNCTION(this);

    overlayApplication::StartApplication();
    
    /**
     * Schedule packet and probe sending.
    */
    SchedulePackets(Time(MilliSeconds(meta->pkt_delay)));
    //ScheduleProbes(Time(Seconds(meta->probe_delay))); // UNCOMMENT
}

void hostApp::StopApplication(void)
{
    NS_LOG_FUNCTION(this);

    // Stop sending packets.
    for (uint32_t destNodeIdx = 1; destNodeIdx < num_nodes; destNodeIdx++)
    {
        if (meta->is_leaf_node(topo_idx, destNodeIdx))
        {
            num_pkts_sent_per_dest[destNodeIdx] = meta->max_num_pkts_per_dest;
        }
    }

    // Stop sending probes.
    for (uint32_t destNodeIdx = 1; destNodeIdx < num_nodes; destNodeIdx++)
    {
        if (meta->is_leaf_node(topo_idx, destNodeIdx))
        {
            num_probes_sent_per_dest[destNodeIdx] = meta->max_num_probes_per_pair;
        }
    }

    overlayApplication::StopApplication();
}

}