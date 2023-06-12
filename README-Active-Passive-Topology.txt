file: topology_K4_N20.mat
description: ground truth topology for N = 20 (leaves) and K = 4 (height in #nodes); 20 instances
data structures:
- T_gt: T_gt{i} is the adjacency matrix of the i-th topology (T_gt{i}(m,n) = 1 iff link (m,n) exists)
- Path: Path{i} is the set of root-to-leaf paths in the i-th topology; Path{i}{j} is the node sequence (from root to leaf)
- node_weight_gt: node_weight_gt{i} is the array of node parameter (residual capacity) in the i-th topology, node_weight_gt{i}(v) is the residual capacity of node v (in packets/ms)

Note: This is the queueing network topology. You need to convert it to routing topology (where each node in queueing topology corresponds to a link in the routing topology). This can be done by:
1) Generate a network of the same topology as the queueing topology, but with an extra link and node before the root; see example in Fig. 1 of paper
2) All links have capacity of 10 Gbps. All packets have size 1500 bytes. This implies a link capacity of 830 packets/ms.
(My calculated link capacity / Data rate: 9960 mbps)
3) Determine background traffic rate for each link l in routing topology: look at the residual capacity from node l in queueing topology (say it is node_weight_gt{i}(l)); 
   830 - node_weight_gt{i}(l) - path_load(l) is the rate of background traffic (in packets/ms) for link l.
4) 'path_load(l)' denotes the total traffic from root-to-leaf paths on link l. Suggest sending 10 packets/s on each path (0.01*number_of_paths_traversing_l, measured in packets/ms, is path_load(l)). 
   These are treated as "data packets", and the delays of these packets are the passive measurements.
5) Suggest sending active probes at rate 1 probe/s for each pair of paths (each probe consists of two back-to-back packets, one per path). The delays of these packet pairs are the active measurements. 
Reminder: The only multi-hop traffic is for active/passive measurements; all background traffic is only one-hop. All packets have size 1500 bytes (different from Yudi's setting).
   Use Yudi's way of controlling time intervals between background packets (you need to compute the parameters to generate the required background traffic rate). 

Output: You need to compute and store the following statistics to be used as input for subsequent processing (in Matlab).
- delays of each probe for each pair of paths (e.g., you can store timestamp, i, j, delay of the packet for path i, delay of the packet for path j, for each probe); these are the active measurements
- delay of each data packet on each path (e.g., you can store timestamp, i, delay of a packet on path i, for each data packet); these are the passive measurements
The reason I ask for 'timestamp' is to be able to extract measurements in time windows of different lengths, so that we can show the inference accuracy vs. measurement time. 
