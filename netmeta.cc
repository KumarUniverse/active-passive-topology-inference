#include <fstream>
#include <sstream>
#include <iostream>
#include "netmeta.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("netmeta");
NS_OBJECT_ENSURE_REGISTERED(netmeta);

TypeId netmeta::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::netmeta")
		.SetParent<Object> ();
		// .AddConstructor<netmeta> (name_input_files);
	return tid;
}

TypeId netmeta::GetInstanceTypeId (void) const
{
  	return netmeta::GetTypeId ();
}

// Constructor
netmeta::netmeta()
{
	// set_background_type(CrossType::ParetoBurst);
	// probe_type = ProbeType::naive;

    // Initialize all gt vectors
    n_nodes_gt.resize(n_topos);
    n_leaves_gt.resize(n_topos);
    n_edges_gt.resize(n_topos);
    n_routers_gt.resize(n_topos);
    neighbors_vectors_gt.resize(n_topos);
    neighbors_maps_gt.resize(n_topos);
    edge_bkgrd_traffic_rates_gt.resize(n_topos);
    dest_nodes_gt.resize(n_topos);
    pkt_received.resize(100);
    probe_received.resize(100);

    std::string topos_edges_lists_path = "./topos-edges-lists/";
    std::string routing_tables_path = "./topos-routing-tables/";
    netmeta::read_network_topologies(topos_edges_lists_path, routing_tables_path);

    // neighbors_vec = {{0, 1}, {0, 2}, {1, 2}, {3, 4}, {2, 3}, {0, 5}};
    // src_dest_pairs =
    // {
    //     std::pair<int, int>(0, 12), std::pair<int, int>(12, 0),
    //     std::pair<int, int>(1, 12), std::pair<int, int>(12, 1),
    //     std::pair<int, int>(2, 12), std::pair<int, int>(12, 2),
    //     std::pair<int, int>(3, 12), std::pair<int, int>(12, 3),
    //     std::pair<int, int>(0, 1), std::pair<int, int>(1, 0)
    // };

    // Assume no packets or probes are sent before the simulation starts.
    // for (uint32_t i = 0; i < n_topos; i++)
    // {
    //     for (uint32_t dest : dest_nodes_gt[i])
    //     {   // Before sending any packets or probes, assume they are all received.
    //         pkt_received_gt[i].insert(std::pair<uint32_t, bool>(dest, true));
    //         probe_received_gt[i].insert(std::pair<uint32_t, bool>(dest, true));
    //     }

    //     pkts_sent_per_leaf_gt[i].resize(n_leaves_gt[i]);
    //     for (uint32_t j = 0; j < n_leaves_gt[i]; j++)
    //     {
    //         pkts_sent_per_leaf_gt[i][j] = 0;
    //     }
    //     probes_sent_per_leaf_gt[i].resize(n_leaves_gt[i]);
    //     for (uint32_t j = 0; j < n_leaves_gt[i]; j++)
    //     {
    //         probes_sent_per_leaf_gt[i][j] = 0;
    //     }
    // }
}

netmeta::~netmeta()
{
    n_nodes_gt.clear();
    n_leaves_gt.clear();
    n_edges_gt.clear();
    n_routers_gt.clear();
    neighbors_vectors_gt.clear();
    neighbors_maps_gt.clear();
    edge_bkgrd_traffic_rates_gt.clear();
    dest_nodes_gt.clear();
    // pkts_sent_per_leaf_gt.clear();
    // probes_sent_per_leaf_gt.clear();
    // pkt_received_gt.clear();
    // probe_received_gt.clear();
    pkt_delays_gt.clear();
    probe_delays_gt.clear();
    received_probes.clear();
}

bool netmeta::is_leaf_node(int topo_idx, int node_idx)
{
    return (dest_nodes_gt[topo_idx].find(node_idx) != dest_nodes_gt[topo_idx].end());
}

void netmeta::read_network_topologies(std::string topos_edges_lists_path, std::string routing_tables_path)
{
    for (uint32_t i = 0; i < n_topos; i++)
    {
        std::string tree_topo_filename = "Tree" + std::to_string(i) + ".graph";
        tree_topo_filename = topos_edges_lists_path + tree_topo_filename;
        std::string routing_filename = "route_table" + std::to_string(i) + ".txt";
        routing_filename = routing_tables_path + routing_filename;
        std::ifstream topo_infile(tree_topo_filename);
        std::ifstream route_infile(routing_filename);

        std::string line; // to read in file input line by line
        std::string tmp_str, tmp_str2;
        uint32_t src, dest;
        double edge_bkgrd_rate;
        std::vector<std::string> tokens; // Vector of string to save tokens

        // Src-dest links between the underlay nodes using a map.
        std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec;
        std::map<uint32_t, std::vector<uint32_t>> neighbors_map;
        // Link capacities of all the links in a topology.
        std::map<std::pair<uint32_t, uint32_t>, double> edge_bkgrd_rates;
        std::set<uint32_t> dest_nodes; // the source node is always the route node S (0).

        
        /**
         * Get underlay information
         **/
        // check if files are open
        if (!topo_infile.is_open()) {
            std::cerr << "Error opening file: " << tree_topo_filename << std::endl;
        }
        if (!route_infile.is_open()) {
            std::cerr << "Error opening file: " << routing_filename << std::endl;
        }
        std::cout << "Topology_file: " << tree_topo_filename << std::endl;
        std::cout << "Routing_file: " << routing_filename << std::endl;

        // Read the tree topology file.
        while (getline(topo_infile, line))
        {
            if (line.empty())
            {
                break;
            }

            std::istringstream iss(line);
            if (line.substr(0, 5).compare("NODES") == 0)
            {
                iss >> tmp_str >> n_nodes_gt[i];
            }
            else if (line.substr(0, 5).compare("EDGES") == 0)
            {
                iss >> tmp_str >> n_edges_gt[i];
            }
            else if (line.substr(0, 5).compare("edge_") == 0)
            {
                iss >> tmp_str >> edge_bkgrd_rate;

                std::stringstream token_stream(tmp_str);
                // Tokenize w.r.t. underscore '_'
                while(getline(token_stream, tmp_str2, '_'))
                {
                    tokens.push_back(tmp_str2);
                }
                src = (uint32_t) std::stoi(tokens[1]);  // convert string to int
                dest = (uint32_t) std::stoi(tokens[2]);
                if (neighbors_map.find(src) != neighbors_map.end())
                {
                    neighbors_map[src] = std::vector<uint32_t>{dest};
                }
                else
                {
                    neighbors_map[src].emplace_back(dest);
                }
                if (neighbors_map.find(dest) != neighbors_map.end())
                {
                    neighbors_map[dest] = std::vector<uint32_t>{src};
                }
                else
                {
                    neighbors_map[dest].emplace_back(src);
                }
                auto src_dest_pair = std::pair<uint32_t, uint32_t>(src, dest);
                auto dest_src_pair = std::pair<uint32_t, uint32_t>(dest,src);
                neighbors_vec.emplace_back(src_dest_pair);
                // Note: Half of the background traffic goes from the source node to dest node.
                // The other half of the background traffic goes from dest node to source node.
                double half_bkgrd_rate = edge_bkgrd_rate / 2;
                edge_bkgrd_rates.insert(std::pair<std::pair<uint32_t, uint32_t>, double>(src_dest_pair, half_bkgrd_rate));
                edge_bkgrd_rates.insert(std::pair<std::pair<uint32_t, uint32_t>, double>(dest_src_pair, half_bkgrd_rate));
                tokens.clear();
            }
        }
        neighbors_vectors_gt[i] = neighbors_vec;
        neighbors_maps_gt[i] = neighbors_map;
        edge_bkgrd_traffic_rates_gt[i] = edge_bkgrd_rates;

        // Read the routing table file.
        while (getline(route_infile, line))
        {
            if (line.empty())
            {
                break;
            }

            if (line.substr(0, 5).compare("PATHS") == 0)
            {
                std::istringstream iss(line);
                iss >> tmp_str >> n_leaves_gt[i];
            }
            else if (line.find(": ") != std::string::npos)
            {
                std::stringstream token_stream(line);
                // Tokenize w.r.t. underscore ':'
                while(getline(token_stream, tmp_str, ':'))
                {
                    tokens.push_back(tmp_str);
                }
                tmp_str = tokens[0];
                std::istringstream iss(tmp_str);
                iss >> src >> dest;
                dest_nodes.insert(dest);
                // Ignore route contained in tokens[1] for now.
                tokens.clear();
            }
        }
        dest_nodes_gt[i] = dest_nodes;
        n_routers_gt[i] = n_nodes_gt[i] - n_leaves_gt[i] - 1;
    }
}

void netmeta::write_pkt_delays(std::string output_path)
{   // Output path: "./passive_measurments/"
    for (uint32_t i = 0; i < n_topos; i++)
    {
        std::string output_filename = output_path + "Pkt_Delays_" + std::to_string(i) + ".csv";
        std::ofstream wrfile(output_filename);
        
        for (auto it = pkt_delays_gt[i].begin(); it != pkt_delays_gt[i].end(); it++)
        {
            // idx of path i,
            // timestamp (in microseconds),
            // delay of the packet on path i (in microseconds)
            std::vector<int64_t> pkt_delay_meas = *it;
            wrfile << std::to_string(pkt_delay_meas[0]) << ", ";
            wrfile << std::to_string(pkt_delay_meas[1]) << ", ";
            wrfile << std::to_string(pkt_delay_meas[2]) << std::endl;
        }
        wrfile.close();
    }
}

void netmeta::write_probe_delays(std::string output_path)
{   // Output path: "./active_measurments/"
    for (uint32_t i = 0; i < n_topos; i++)
    {
        std::string output_filename = output_path + "Probe_Delays_" + std::to_string(i) + ".csv";
        std::ofstream wrfile(output_filename);

        for (auto it = probe_delays_gt[i].begin(); it != probe_delays_gt[i].end(); it++)
        {
            // idx of path i, idx of path j,
            // timestamp (in microseconds),
            // delay of the packet for path i (in microseconds), delay of the packet for path j
            std::vector<int64_t> probe_delay_meas = *it;
            wrfile << std::to_string(probe_delay_meas[0]) << ", ";
            wrfile << std::to_string(probe_delay_meas[1]) << ", ";
            wrfile << std::to_string(probe_delay_meas[2]) << ", ";
            wrfile << std::to_string(probe_delay_meas[3]) << ", ";
            wrfile << std::to_string(probe_delay_meas[4]) << std::endl;
        }
        wrfile.close();
    }
}

} // namespace ns3