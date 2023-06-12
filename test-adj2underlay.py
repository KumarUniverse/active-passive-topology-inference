##################################################################
# Paper: Topology Inference Using Active and Passive Measurements
# Author: Akash Kumar
##################################################################

import scipy.io as sio
import numpy as np

# Load the MATLAB variables from the MAT file.
#graph_filename = "Tree.graph"

# Assign variable names for each value in the dictionary.
# vector containing tree topologies rep as adjcency matrices
# 1 - edge is present. 0 - edge is not present.
# Since the graph is undirected, the adjaceny matrix is symmetric.
instance = [[0,1,1,0,0,0],
            [1,0,0,1,0,0],
            [1,0,0,0,1,1],
            [0,1,0,0,0,0],
            [0,0,1,0,0,0],
            [0,0,1,0,0,0]]
# (T_gt{i}(m,n) = 1 iff link (m,n) exists)
edge_capacities = [1,2,3,4,5,6] # residual capacity of the edges, BFS traversal of tree.
root2leaf_paths = [[1,2,4],[1,3,5],[1,3,6]] # set of root-to-leaf paths

# DEBUG
# print(topo_gt.shape)            # (1, 20)
# print(edge_capacities_gt.shape) # (1, 20)
# print(root2leaf_paths_gt.shape) # (1, 20)

# DEBUG
# print(instance.shape) # (37, 37)
# print(edge_capacities.shape) # (37,)
num_nodes = len(instance) + 1  # +1 bcuz of adding source node s as parent of root.
edges = ["edge_0 0 1 " + str(edge_capacities[0])]
num_edges = 1 # start by counting the first edge l1 from source s to first router
# Note: Edge l1 is not included in the adj matrix.

# Count the number of edges in the tree.
# Don't count edges twice, so only look at top-right of adj matrix.
# range(4): 0, 1, 2, 3
for i in range(num_nodes-1):
    for j in range(i, num_nodes-1):
        if instance[i][j] == 1:
            # Format: edge_{edge_i} start_vertex end_vertex residual_capacity
            edges.append("edge_" + str(num_edges) + " " + str(i+1) + " " + str(j+1)
                        + " " + str(edge_capacities[num_edges]))
            num_edges += 1

num_paths = len(root2leaf_paths)
paths = []
for i in range(num_paths):
    path = root2leaf_paths[i]
    # path = [x.tolist() for x in path] #[0][0]
    path = [0] + list(map(lambda x: x, path))
    paths.append(str(path)[1:-1].replace(", ", " "))

# Output the results.
print("NODES " + str(num_nodes) + "\n")
print("EDGES " + str(num_edges))
for edge in edges:
    print(edge)

print("\nPATHS " + str(num_paths))
for path in paths:
    print(path)
