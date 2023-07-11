#ifndef NET_W_H
#define NET_W_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>    // std::min_element, std::max_element

#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"
//#include "utils.h"

#define LISTENPORT 9
// LISTENPORT - the listen port number of the socket.


namespace ns3
{

class netmeta
{
public:
    static TypeId GetTypeId(void);
	virtual TypeId GetInstanceTypeId(void) const;
    netmeta();
    ~netmeta();
    bool is_leaf_node(int topo_idx, int node_idx);
    // IO functions:
    void read_network_topologies(std::string topos_edges_lists_path, std::string routing_tables_path);
    void write_pkt_delays(std::string output_path);
    void write_probe_delays(std::string output_path);
    
    /**
     * Packet metadata
    */
    uint32_t
    topo_idx = -1,                // index of the current simulated topology
    host_idx = 0,                 // index of the host node
    pkt_size = 1500,              // bytes, MTU for 5G
    max_num_pkts_per_dest = 1, //10,   // number of packets to send from source node S to each dest node d.
    max_num_probes_per_pair = 1,  // number of probes to send from source node S for each probe pair.
    // 10 times as many data packets as there are probes.
    n_topos = 20,                 // number of tree topologies to read.
    pkt_delay = 100,              // in milliseconds, time to wait before sending next data packet.
    probe_delay = 1;              // in seconds, time to wait before sending next probe.
    //std::vector<bool> pkt_received;    // one bool for each leaf <dest_idx, received_or_not>
    //std::vector<bool> probe_received;  // one bool for each leaf <dest_idx, received_or_not>

    int link_capacity = 10, // data rate of all links (Gbps)
        prop_delay = 0; // propagation delay (in  microseconds)
    std::vector<uint32_t> n_nodes_gt;
    std::vector<uint32_t> n_leaves_gt;
    std::vector<uint32_t> n_edges_gt;
    std::vector<uint32_t> n_routers_gt;
    std::vector<std::vector<std::pair<uint32_t, uint32_t>>> neighbors_vectors_gt;
    std::vector<std::map<uint32_t, std::vector<uint32_t>>> neighbors_maps_gt;
    std::vector<std::map<std::pair<uint32_t, uint32_t>, double>> edge_bkgrd_traffic_rates_gt;
    std::vector<std::set<uint32_t>> dest_nodes_gt; // dest nodes (leaf nodes) of all the topologies.
    // gt stands for "graph topology".
    
    // std::vector<std::vector<int>> routes; // don't need
    //std::map<std::string, std::vector<int>> routing_map; // don't need
    // Don't need to specify routing. It will automatically be configured by NS3.
    

    /**
     * Background metadeta
    */
    uint32_t pareto_wait_time = 10; // ms, wait time between bursts
    //double prob_burst = 0.002;    // for Poisson background traffic
    //uint32_t n_burst_pkts = 50;     // for Poisson; number of packets per burst
    double parato_scale = 12;
    double parato_shape = 2.04;
    uint32_t parato_bound = 300;

    std::vector<std::map<uint32_t, std::vector<int64_t>>> received_probes_gt; // probes received at UEs
    // Pair probe packets with the same probe ID together.
    
    // Delays of each data packet on each path.
    // Each entry is a vector containing the following info in this order (3 elements):
    // idx of path, timestamp (in microseconds), delay of the packet on path i (in microseconds)
    // Passive measurements
    std::vector<std::vector<std::vector<int64_t>>> pkt_delays_gt; // pkt delays of all topologies

    // Delays of each probe for each pair of paths.
    // Each entry is a vector containing the following info in this order (6 elements):
    // idx of path i, idx of path j, timestamp i (in microseconds), timestamp j,
    // delay of the packet for path i (in microseconds), delay of the packet for path j
    // Active measurements
    std::vector<std::vector<std::vector<int64_t>>> probe_delays_gt; // probe delays of all topologies
    // First outermost vector is for the storing all the topologies.
    // Second vector is for storing all the delay data of the ith topology.
    // Third innermost vector is a single active/passive measurement.

private:
};

}

#endif