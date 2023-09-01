###################################################################################
# Description: A Python script that reads in queueing-based
#   tree topologies from a .MAT file, converts the topologies
#   to routing-based topologies, outputs the edge lists into
#   .graph files, and the root to leaf routes into .txt files.
# Paper: Queueing Network Topology Inference Using Passive and Active Measurements
# Author: Akash Kumar
###################################################################################

import scipy.io as sio
import numpy as np

# Load the MATLAB variables from the MAT file.
topo_vars = sio.loadmat('topology_K4_N20.mat')  # dict
#graph_filename = "Tree.graph"

# DEBUG
# print(topo_vars.keys())
# # dict_keys(['__header__', '__version__', '__globals__', 'Path', 'T_gt', 'node_weight_gt'])
# print(type(topo_vars['T_gt']))           # <class 'numpy.ndarray'>
# print(type(topo_vars['node_weight_gt'])) # <class 'numpy.ndarray'>
# print(type(topo_vars['Path']))           # <class 'numpy.ndarray'>

# Assign variable names for each value in the dictionary.
topo_gt = topo_vars['T_gt'] # vector containing tree topologies rep as adjcency matrices
# (T_gt{i}(m,n) = 1 iff link (m,n) exists)
edge_res_capacities_gt = topo_vars['node_weight_gt'] # residual capacity of the edges (in pkts/ms)
root2leaf_paths_gt = topo_vars['Path']               # set of root-to-leaf paths

# DEBUG
# print(topo_gt.shape)            # (1, 20)
# print(edge_res_capacities_gt.shape) # (1, 20)
# print(root2leaf_paths_gt.shape) # (1, 20)

# DEBUG
# root2leaf_paths = root2leaf_paths_gt[0][0] # 0, instance i
# path = root2leaf_paths[0]
# print(root2leaf_paths.shape) # (20, 1)
# print(type(root2leaf_paths)) # <class 'numpy.ndarray'>
# print(type(path))            # <class 'numpy.ndarray'>
# xs = [x.tolist() for x in path][0][0]
# print(xs)
# print(type(xs[0]))

num_instances = topo_gt.shape[1] # 20
print("number of instances:", num_instances)

link_capacity = 830 #83.33 # pkts/ms, max capacity of all the links
link_capacity /= 10 # downscale link capacity by a factor of 10
num_leaves = 20
num_data_pkts_per_ms_per_path = 0.01 # or 10 pkts/s per path
num_probes_per_ms_per_path = 0.001 * (num_leaves-1) # 0.019 pkt/ms or 19 pkt/s per path
#probe_load_l = 0.001 # pkts/ms or 10 pkts/s
#rate_buffer = 1 # to prevent link from being fully saturated

for inst_num in range(num_instances):
    instance = topo_gt[0][inst_num]  # 0, instance i
    # Residual capacities of topology instance i.
    edge_res_capacities = edge_res_capacities_gt[0][inst_num][0] # 0, instance i, 0
    # Downscale residual edge capacities by a factor of 10.
    for i in range(len(edge_res_capacities)):
        edge_res_capacities[i] /= 10
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

    root2leaf_paths = root2leaf_paths_gt[0][inst_num] # 0, instance i
    num_paths = len(root2leaf_paths)
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
    path_load_l = num_data_pkts_per_ms_per_path * num_paths_per_edge["edge_0_1"]
    probe_load_l = num_probes_per_ms_per_path * num_paths_per_edge["edge_0_1"]
    # Calc bckgrd traff rate for link (0, 1)
    bckgrd_traffic_rate_l = (link_capacity - edge_res_capacities[edge_i]
                            - path_load_l - probe_load_l)
    edges.append("edge_0_1 " + str(bckgrd_traffic_rate_l))
    edge_i += 1
    for i in range(num_nodes-1):
        for j in range(i, num_nodes-1):
            if instance[i][j] == 1: # there is an edge between nodes i and j.
                path_load_l = num_data_pkts_per_ms_per_path * num_paths_per_edge["edge_" + str(i) + "_" + str(j)]
                probe_load_l = num_probes_per_ms_per_path * num_paths_per_edge["edge_" + str(i) + "_" + str(j)]
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

    if inst_num == 0:
        graph_filename = "Tree" + str(inst_num) + ".graph" # "Tree.graph"
        with open(graph_filename, 'w') as gf:
            gf.write("NODES " + str(num_nodes) + "\n")
            gf.write("EDGES " + str(num_edges) + "\n")
            for edge in edges:
                gf.write(edge + "\n")

    # routing_filename = "route_table" + str(inst_num) + ".txt" # "route_table.txt"
    # with open(routing_filename, 'w') as rf:
    #     rf.write("PATHS " + str(num_paths) + "\n")
    #     for path in paths:
    #         path_nodes = path.split()
    #         src, dest = path_nodes[0], path_nodes[-1]
    #         new_path = src + " " + dest + ": " + path
    #         rf.write(new_path + "\n")
