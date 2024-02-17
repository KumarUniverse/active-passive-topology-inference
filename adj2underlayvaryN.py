###################################################################################
# Description: A Python script that reads in queueing-based
#   tree topologies from a .MAT file, converts the topologies
#   to routing-based topologies, outputs the edge lists into
#   .graph files, and the root to leaf routes into .txt files.
# Paper: Queueing Network Topology Inference Using Passive and Active Measurements
# Author: Akash Kumar
###################################################################################

import os
import scipy.io as sio
#import numpy as np

def GbpsToKbps(kbps):
    gbps = kbps * 1e6
    return gbps

def pktsPerMsToKbps(pktsPerMs):
    kbps = pktsPerMs * 12000  # pktsPerMs * 1500 * 8 * / 1000 * 1000
    return kbps

# Load the MATLAB variables from the MAT file.
mat_filename = 'topology_K4_vary_N.mat'
topo_vars = sio.loadmat(mat_filename)  # dict
#graph_filename = "Tree.graph"

# DEBUG
# print(topo_vars.keys())
# # dict_keys(['__header__', '__version__', '__globals__', 'Path', 'T_gt', 'node_weight_gt'])
# print(type(topo_vars['T_gt']))           # <class 'numpy.ndarray'>
# print(type(topo_vars['node_weight_gt'])) # <class 'numpy.ndarray'>
# print(type(topo_vars['Path']))           # <class 'numpy.ndarray'>

# Assign variable names for each value in the dictionary.
topo_gt = topo_vars['T_gt'] # vector containing tree topologies rep as adjacency matrices
# (T_gt{i}(m,n) = 1 iff link (m,n) exists)
edge_res_capacities_gt = topo_vars['node_weight_gt'] # residual capacity of the edges (in pkts/ms)
root2leaf_paths_gt = topo_vars['Path']               # set of root-to-leaf paths

# DEBUG
# print(topo_gt.shape)                # (5, 20)
# print(edge_res_capacities_gt.shape) # (5, 20)
# print(root2leaf_paths_gt.shape)     # (5, 20)

# DEBUG
# root2leaf_paths = root2leaf_paths_gt[4][0] # K-3, instance i; max: 4, 49
# path = root2leaf_paths[0]
# print(root2leaf_paths.shape) # (20, 1)
# print(type(root2leaf_paths)) # <class 'numpy.ndarray'>
# print(type(path))            # <class 'numpy.ndarray'>
# print(path.shape)            # (1,)
# xs = [x.tolist() for x in path][0][0]
# print(xs)
# print(type(xs[0]))

num_instances = 20 #topo_gt.shape[1]
print("number of instances per setting:", num_instances)

#link_capacity = 830 #83.33 # pkts/ms, max capacity of all the links
#link_capacity /= 10 # downscale link capacity by a factor of 10
#link_capacity = 83  # after downscaling by 10
#link_capacity = 10 # Gbps
link_capacity = 1  # Gbps, after downscaling by 10
link_capacity = GbpsToKbps(link_capacity)              # kbps
#probe_load_l = 0.001 # pkts/ms or 10 pkts/s
#rate_buffer = 1 # to prevent link from being fully saturated

for a in range(1, 6):
    N = 10*a
    for inst_num in range(num_instances):
        instance = topo_gt[a-1][inst_num]  # a-1, instance i
        # Residual capacities of topology instance i.
        edge_res_capacities = edge_res_capacities_gt[a-1][inst_num][0] # a-1, instance i, 0
        # Downscale residual edge capacities by a factor of 10.
        for i in range(len(edge_res_capacities)):
            edge_res_capacities[i] /= 10
            edge_res_capacities[i] = pktsPerMsToKbps(edge_res_capacities[i])  # convert unit to kbps
        # DEBUG
        # print(instance.shape) # (37, 37)
        # print(edge_res_capacities.shape) # (37,)
        num_nodes = instance.shape[0] + 1  # +1 bcuz of adding source node s as parent of root.
        edges = [] #["edge_0 0 1 " + str(edge_res_capacities[0])]
        num_paths_per_edge = {}
        # Default initialize all values in num_paths_per_edge to 0.
        for i in range(num_nodes-1):
            for j in range(i, num_nodes-1):
                if instance[i][j] == 1: # there is an edge between nodes i and j.
                    edge_key = "edge_" + str(i) + "_" + str(j)
                    num_paths_per_edge[edge_key] = 0
        # Note: Edge l1 is not included in the adj matrix.

        root2leaf_paths = root2leaf_paths_gt[a-1][inst_num] # a-1, instance i
        num_paths = len(root2leaf_paths)  # N = 20. N is the number of leaves as well as the number of paths

        num_ms_in_one_s = 1000
        num_bits_in_one_kb = 1000
        passive_frame_size = 1500    # 1472 bytes of payload + 8 UDP header + 20 IP header + 18 Ethernet header
        probe_frame_size = 50        # 50 bytes of payload + 8 UDP header + 20 IP header + 18 Ethernet header
        passive_send_interval = 5 # in ms
        active_send_interval = 5 # in ms
        passive_pkts_per_path_per_s = num_ms_in_one_s / passive_send_interval
        active_pkts_per_path_per_s = num_ms_in_one_s / active_send_interval
        data_kbps_per_path = passive_pkts_per_path_per_s * passive_frame_size * 8 / num_bits_in_one_kb
        probe_kbps_per_path = active_pkts_per_path_per_s * probe_frame_size * 8 / num_bits_in_one_kb * (num_paths-1)
        #data_kbps_per_path = 2400
        #probe_kbps_per_path = 80 * (num_paths-1)

        paths = []
        for i in range(num_paths):
            path = root2leaf_paths[i]
            path = [x.tolist() for x in path][0][0]
            path = [0] + list(map(lambda x: x, path))
            for j in range(1, len(path)):
                prev_node = path[j-1]
                curr_node = path[j]
                edge = "edge_" + str(prev_node) + "_" + str(curr_node)
                if edge in num_paths_per_edge.keys():
                    num_paths_per_edge[edge] += 1

            paths.append(str(path)[1:-1].replace(", ", " "))

        # Count the number of edges in the tree.
        # Don't count edges twice, so only look at top-right of symmetric adj matrix.
        # range(4): 0, 1, 2, 3
        edge_i = 0
        path_load_l = data_kbps_per_path * num_paths_per_edge["edge_0_1"]
        probe_load_l = probe_kbps_per_path * num_paths_per_edge["edge_0_1"]
        # Calc bckgrd traff rate for link (0, 1)
        bckgrd_traffic_rate_l = (link_capacity - edge_res_capacities[edge_i]
                                - path_load_l - probe_load_l)
        edges.append("edge_0_1 " + str(bckgrd_traffic_rate_l))
        edge_i += 1
        for i in range(num_nodes-1):
            for j in range(i, num_nodes-1):
                if instance[i][j] == 1: # there is an edge between nodes i and j.
                    path_load_l = data_kbps_per_path * num_paths_per_edge["edge_" + str(i) + "_" + str(j)]
                    probe_load_l = probe_kbps_per_path * num_paths_per_edge["edge_" + str(i) + "_" + str(j)]
                    bckgrd_traffic_rate_l = (link_capacity - edge_res_capacities[edge_i]
                                            - path_load_l - probe_load_l)
                    ### Format: edge_srcNode_destNode bckgrd_traffic_rate_for_link_l
                    edges.append("edge_" + str(i+1) + "_" + str(j+1)
                                + " " + str(bckgrd_traffic_rate_l))
                    edge_i += 1
        num_edges = edge_i

        # DEBUG
        # print(num_nodes)
        # print(num_edges)

        subdirectory = "./vary-N/" + "topos-edges-lists-K4" + "-N" + str(N) + "/"
        if not os.path.isdir(subdirectory):
            os.mkdir(subdirectory)
        graph_filename = "Tree" + str(inst_num) + ".graph" # "Tree.graph"
        with open(os.path.join(subdirectory, graph_filename), 'w') as gf:
            gf.write("NODES " + str(num_nodes) + "\n")
            gf.write("EDGES " + str(num_edges) + "\n")
            for edge in edges:
                gf.write(edge + "\n")

        subdirectory = "./vary-N/" + "topos-routing-tables-K4" + "-N" + str(N) + "/"
        if not os.path.isdir(subdirectory):
            os.mkdir(subdirectory)
        routing_filename = "route_table" + str(inst_num) + ".txt" # "route_table.txt"
        with open(os.path.join(subdirectory, routing_filename), 'w') as rf:
            rf.write("PATHS " + str(num_paths) + "\n")
            for path in paths:
                path_nodes = path.split()
                src, dest = path_nodes[0], path_nodes[-1]
                new_path = src + " " + dest + ": " + path
                rf.write(new_path + "\n")
