/**
 * Description: A class for storing all the meta information
 * and constant values of all the network topologies. It is
 * also responsible for all file I/O of the simulation.
 * The structures and properties of the topologies are read
 * in from files and after running the simulations, the
 * recorded measurements are written to the output files.
*/

#ifndef NET_W_H
#define NET_W_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"

// Unit conversion macros:
#define BITS_PER_BYTE 8
#define BITS_PER_KB 1000
#define MICROSECS_TO_MS 1.0e-3
#define MICROSECS_TO_SECS 1.0e-6
#define MS_TO_SECS 1.0e-3
#define NS_TO_MS 1.0e-6
#define SECS_TO_MS 1000
#define SECS_TO_MICROSECS 1e6

#define LISTENPORT 9  // the listen port number of the socket.


namespace ns3
{

enum BckgrdTrafficType
{
    NoBckgrd = 0,
    CBR = 1,       // Constant Bit Rate
    Poisson = 2,
    ParetoBurst = 3,
    LogNormal = 4
};

class netmeta
{
public:
    static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
    netmeta();
    netmeta(uint32_t topo_idx);
    ~netmeta();
    bool is_leaf_node(int topo_idx, int node_idx);
    bool is_leaf_node(int node_idx);
    bool all_pkts_and_probes_received();
    // IO functions:
    // IO for all topologies:
    void read_network_topologies();
    void write_pkt_delays();
    void write_probe_delays();
    // IO for current topology:
    void read_network_topologies_for_curr_topo();
    void write_pkt_delays_for_curr_topo();
    void write_probe_delays_for_curr_topo();

    /**
     * Packet metadata
    */
    uint32_t
    topo_idx = -1,                 // index of the current simulated topology
    host_idx = 0,                  // index of the host node
    pkt_payload_size = 1470, //1472,  // payload size of the data packet in bytes
    //MTU = pkt_payload_size + 28,   // maximum transmission unit in bytes
    // (frame size = 1470 bytes of data + 8 UDP header + 20 minimum IP header + 2 Eth header = 1500 bytes)
    // Beyond 1472 payload bytes, the packet is fragmented into multiple packets.
    // For an MTU of 1500 bytes, the maximum payload size is 1472 bytes and max frame size is 1502 bytes.
    probe_payload_size = 20, //22, //50            // payload size of the probe in bytes
    // (frame size = 20 bytes of data + 8 UDP header + 20 minimum IP header + 2 Eth header = 50 bytes)
    bckgrd_pkt_payload_size = 1470, //1472,  // payload size of the background traffic packet in bytes
    size_of_headers = 30,   // bytes, includes UDP, IP, and Ethernet headers
    phy_bckgrd_pkt_size = bckgrd_pkt_payload_size + size_of_headers, //1500, //1518, // bytes, actual size
    // of the packet in the physical layer including all headers
    passive_start_time = 1000,     // in milliseconds, time to wait before sending data packets
    active_start_time = 1500,      // ms, time to wait before sending probes and data packets
    bckgrd_traff_start_time = 100, // ms, time to wait before sending background traffic
    max_num_pkts_per_dest = 1e5, //1e4, //100, //500 // number of packets to send from source node S to each dest node d
    max_num_probes_per_pair = 1e5, //1e4, //10, //6    // number of probes to send from source node S for each probe pair
    // There should be 10 times as many data packets as there are probes.
    n_topos = 20,                  // number of tree topologies to read
    pkt_send_delay = 5, //100,     // in ms, time to wait before sending next data packet
    probe_send_delay = 5, //1,     // in ms, time to wait before sending next probe
    pkt_write_freq = 1, //10,      // how often to write passive delays to file, num data pkts received at last leaf node
    probe_write_freq = 1; //10;    // how often to write active delays to file, num probe pkts received at last leaf node
    // smaller write freqeuncy means less time spent running the simulation.
    //std::vector<bool> pkt_received;    // one bool for each leaf <dest_idx, received_or_not>
    //std::vector<bool> probe_received;  // one bool for each leaf <dest_idx, received_or_not>
    //double pkt_delay_secs = pkt_send_delay * MS_TO_SECS; // pkt delay in seconds
    bool send_bidirec_traff = false, // determines whether traffic is sent bidirectionally or unidirectionally
    // Note: When NS3 creates a bidirectional link of capacity C, it actually creates
    // two separate links, one from node a to b and the other from b to a, each link with capacity C.
    is_data_enabled = true,     // determines whether probes are sent or not
    is_probing_enabled = true;  // determines whether data packets are sent or not

    int link_capacity = 1, //10; // data rate of all links (in Gbps) // use 1 Gbps if downscaling by 10
        link_capacity_Mbps = link_capacity * BITS_PER_KB, // in Mbps
        link_capacity_kbps = link_capacity_Mbps * BITS_PER_KB, // in Kbps
        prop_delay = 0, // 100; // propagation delay (in  microseconds)
        max_queue_size = 1e6; //1e3; //1e6; //2e6; // size of all the transmission queues, in # of pkts


    // std::string topos_edges_lists_path = "./topos-edges-lists/"; // relative path
    // std::string routing_tables_path = "./topos-routing-tables/"; // relative path
    std::string topos_edges_lists_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/topos-edges-lists-K4-N20/";
    std::string routing_tables_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/topos-routing-tables-K4-N20/";
    std::string pkt_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/passive-measurements-K4-N20/";
    std::string probe_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/active-measurements-K4-N20/";

    // Specify the background traffic type
    BckgrdTrafficType bckgrd_traffic_type = BckgrdTrafficType::LogNormal;

    // Pareto distribution parameters
    double on_pareto_scale = 12.0,   // 5.0,
            on_pareto_shape = 2.04,  // 1.5,
            on_pareto_bound = 300,   // 200,s
            off_pareto_scale = 2.0,  // 1.0,
            off_pareto_shape = 1.0,
            off_pareto_bound = 100;

    // Log-normal bckgrd traffic params
    double log_normal_sigma = 0.3, //0.1, //1.0,
            T_rate_interval_us = 500, // in microseconds; 1000us = 1ms; 500us = 0.5ms;
            T_rate_interval_ms = T_rate_interval_us * MICROSECS_TO_MS,
            T_rate_interval_secs = T_rate_interval_ms * MS_TO_SECS;
            //^^specifies how often to sample a traffic rate from the log-norm distro

    uint32_t n_nodes,
            n_leaves,
            n_edges,
            n_routers;
    std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec;
    std::map<uint32_t, std::vector<uint32_t>> neighbors_map;
    std::map<std::pair<uint32_t, uint32_t>, double> edge_bckgrd_rates; // Background rates of all the links in a topology.
    std::set<uint32_t> dest_nodes; // a.k.a. leaf nodes
    std::map<uint32_t, uint32_t> dest_idx_to_path_idx;
    std::map<uint32_t, uint32_t> pkts_received_per_dest_node;
    std::map<uint32_t, uint32_t> probes_received_per_dest_node;

    std::vector<uint32_t> n_nodes_gt;
    std::vector<uint32_t> n_leaves_gt;
    std::vector<uint32_t> n_edges_gt;
    std::vector<uint32_t> n_routers_gt;
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> neighbors_vectors_gt;
    std::vector<std::map<uint32_t, std::vector<uint32_t>>> neighbors_maps_gt;
    std::vector<std::map<std::pair<uint32_t, uint32_t>, double>> edge_bckgrd_traffic_rates_gt; // in pkts/ms
    std::vector<std::set<uint32_t>> dest_nodes_gt; // dest nodes (leaf nodes) of all the topologies.
    std::vector<std::map<uint32_t, uint32_t>> dest_idx_to_path_idx_gt;
    // gt stands for "graph topology".

    // std::vector<std::vector<int>> routes; // don't need
    //std::map<std::string, std::vector<int>> routing_map; // don't need
    // Don't need to manually specify routing. It will automatically be configured by NS3.


    /**
     * Background metadeta
    */
    uint32_t pareto_wait_time = 10; // ms, wait time between bursts
    //double prob_burst = 0.002;    // for Poisson background traffic
    //uint32_t n_burst_pkts = 50;     // for Poisson; number of packets per burst

    std::map<uint32_t, std::vector<int64_t>> received_probes; // probes received at UEs
    std::vector<std::map<uint32_t, std::vector<int64_t>>> received_probes_gt; // for all topos
    // Pair probe packets with the same probe ID together.

    // Delays of each data packet on each path.
    // Each entry is a vector containing the following info in this order (3 elements):
    // idx of path, timestamp (in nanoseconds), delay of the packet on path i (in ns)
    // Passive measurements
    std::vector<std::vector<int64_t>> pkt_delays; // pkt delays for current topology
    std::vector<std::vector<std::vector<int64_t>>> pkt_delays_gt; // pkt delays of all topologies

    // Delays of each probe for each pair of paths.
    // Each entry is a vector containing the following info in this order (6 elements):
    // idx of path i, idx of path j, timestamp i (in nanoseconds), timestamp j,
    // delay of the packet for path i (in ns), delay of the packet for path j
    // Active measurements
    std::vector<std::vector<int64_t>> probe_delays; // probe delays for current topology
    std::vector<std::vector<std::vector<int64_t>>> probe_delays_gt; // probe delays of all topologies
    // First outermost vector is for the storing all the topologies.
    // Second vector is for storing all the delay data of the ith topology.
    // Third innermost vector is a single active/passive measurement.

private:
};

}

#endif