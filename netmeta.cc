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
    dest_idx_to_path_idx_gt.resize(n_topos);

    pkt_delays_gt.resize(n_topos);
    probe_delays_gt.resize(n_topos);
    received_probes_gt.resize(n_topos);

    // std::string topos_edges_lists_path = "./topos-edges-lists/"; // relative path
    // std::string routing_tables_path = "./topos-routing-tables/"; // relative path
    std::string topos_edges_lists_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/topos-edges-lists/";
    std::string routing_tables_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/topos-routing-tables/";
    netmeta::read_network_topologies(topos_edges_lists_path, routing_tables_path);
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
    dest_idx_to_path_idx_gt.clear();

    pkt_delays_gt.clear();
    probe_delays_gt.clear();
    received_probes_gt.clear();
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

        std::map<uint32_t, uint32_t> dest_idx_to_path_idx;
        uint32_t path_idx = 0;

        /**
         * Get underlay information
         **/
        // Check if topology file is open.
        if (!topo_infile.is_open()) {
            std::cerr << "Error opening topology file: " << tree_topo_filename << std::endl;
        }
        // else {
        //     std::cout << "Topology_file: " << tree_topo_filename << std::endl;
        // }
        if (!route_infile.is_open()) {
            std::cerr << "Error opening routing file: " << routing_filename << std::endl;
        }
        // else {
        //     std::cout << "Routing_file: " << routing_filename << std::endl;
        // }

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
                
                if (neighbors_map.find(src) == neighbors_map.end())
                {   // src key not found
                    // if (i == 0) // for debugging
                    //     std::cout << "Adding to neighbors map (src: " << src << ", dest: " << dest << ")." << std::endl;
                    neighbors_map[src] = std::vector<uint32_t>{dest};
                }
                else
                {   // src key found
                    neighbors_map[src].emplace_back(dest);
                    // if (i == 0) // for debugging
                    // {
                    //     auto neigh_it = neighbors_map.find(src);
                    //     int position = std::distance(neighbors_map.begin(), neigh_it);
                    //     std::cout << "src key idx: " << position << std::endl;
                    //     std::cout << "emplacing to neighbors map (src: " << src << ", dest: " << dest << ")." << std::endl;
                    //     std::cout << "Size of neighbors_map: " << neighbors_map[src].size() << std::endl;
                    // }
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
                //std::cout << "Emplaced back to neighbors_vec" << src << " " << dest << std::endl; // for debugging.
                // Note: Half of the background traffic goes from the source node to dest node.
                // The other half of the background traffic goes from dest node to source node.
                double half_bkgrd_rate = edge_bkgrd_rate / 2;
                //std::cout << "Half background rate: " << half_bkgrd_rate << std::endl; // for debugging, works
                edge_bkgrd_rates.insert(std::make_pair(src_dest_pair, half_bkgrd_rate));
                edge_bkgrd_rates.insert(std::make_pair(dest_src_pair, half_bkgrd_rate));
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
                dest_idx_to_path_idx[dest] = path_idx++;
                // Ignore route contained in tokens[1] for now.
                tokens.clear();
            }
        }
        dest_nodes_gt[i] = dest_nodes;
        dest_idx_to_path_idx_gt[i] = dest_idx_to_path_idx;
        n_routers_gt[i] = n_nodes_gt[i] - n_leaves_gt[i] - 1;
    }
}

void netmeta::write_pkt_delays(std::string output_path)
{   // Output path: "./passive_measurements/"
    for (int topo_idx = 0; topo_idx < (int) n_topos; topo_idx++)
    {
        std::string output_filename = output_path + "Pkt_Delays_" + std::to_string(topo_idx+1) + ".csv";
        std::ofstream wrfile(output_filename);
        
        for (auto it = pkt_delays_gt[topo_idx].begin(); it != pkt_delays_gt[topo_idx].end(); it++)
        {
            std::vector<int64_t> pkt_delay_meas = *it;
            wrfile << std::to_string(pkt_delay_meas[0]) << ", ";      // idx of path i
            wrfile << std::to_string(pkt_delay_meas[1]) << ", ";      // creation timestamp (in ns)
            wrfile << std::to_string(pkt_delay_meas[2]) << std::endl; // delay of the packet on path i (in ns)
        }
        wrfile.close();
    }
}

void netmeta::write_probe_delays(std::string output_path)
{   // Output path: "./active_measurements/"
    for (int topo_idx = 0; topo_idx < (int) n_topos; topo_idx++)
    {
        std::string output_filename = output_path + "Probe_Delays_" + std::to_string(topo_idx+1) + ".csv";
        std::ofstream wrfile(output_filename);

        for (auto it = probe_delays_gt[topo_idx].begin(); it != probe_delays_gt[topo_idx].end(); it++)
        {
            std::vector<int64_t> probe_delay_meas = *it;
            wrfile << std::to_string(probe_delay_meas[0]) << ", ";      // idx of path i
            wrfile << std::to_string(probe_delay_meas[3]) << ", ";      // idx of path j
            wrfile << std::to_string(probe_delay_meas[1]) << ", ";      // creation timestamp (in ns)
            wrfile << std::to_string(probe_delay_meas[2]) << ", ";      // delay of the packet for path i (in ns)
            wrfile << std::to_string(probe_delay_meas[5]) << std::endl; // delay of the packet for path j (in ns)
        }
        wrfile.close();
    }
}

void netmeta::write_pkt_delays_for_curr_topo(std::string output_path)
{   // Output path: "./passive_measurements/"
    std::string output_filename = output_path + "Pkt_Delays_" + std::to_string(topo_idx+1) + ".csv";
    std::ofstream wrfile(output_filename);
    
    for (auto it = pkt_delays_gt[topo_idx].begin(); it != pkt_delays_gt[topo_idx].end(); it++)
    {
        std::vector<int64_t> pkt_delay_meas = *it;
        wrfile << std::to_string(pkt_delay_meas[0]) << ", ";      // idx of path i
        wrfile << std::to_string(pkt_delay_meas[1]) << ", ";      // creation timestamp (in ns)
        wrfile << std::to_string(pkt_delay_meas[2]) << std::endl; // delay of the packet on path i (in ns)
    }
    wrfile.close();
}

void netmeta::write_probe_delays_for_curr_topo(std::string output_path)
{   // Output path: "./active_measurements/"
    std::string output_filename = output_path + "Probe_Delays_" + std::to_string(topo_idx+1) + ".csv";
    std::ofstream wrfile(output_filename);

    for (auto it = probe_delays_gt[topo_idx].begin(); it != probe_delays_gt[topo_idx].end(); it++)
    {
        std::vector<int64_t> probe_delay_meas = *it;
        wrfile << std::to_string(probe_delay_meas[0]) << ", ";      // idx of path i
        wrfile << std::to_string(probe_delay_meas[3]) << ", ";      // idx of path j
        wrfile << std::to_string(probe_delay_meas[1]) << ", ";      // creation timestamp (in ns)
        wrfile << std::to_string(probe_delay_meas[2]) << ", ";      // delay of the packet for path i (in ns)
        wrfile << std::to_string(probe_delay_meas[5]) << std::endl; // delay of the packet for path j (in ns)
    }
    wrfile.close();
}

} // namespace ns3