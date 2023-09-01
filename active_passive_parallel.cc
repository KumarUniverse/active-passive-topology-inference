/***********************************************************************************
 * Description: Simulation for active-passive topology inference research.
        After simulating various kinds of tree topologies, the results of
        the passive (data packets) and active (probes) measurements are
        saved to data files for further analysis.
 * Paper: Queueing Network Topology Inference Using Passive and Active Measurements
 * Author: Akash Kumar
 * Note: Part of this code contains reused code from Yudi Huang's
         Hex network NS3 project.
***********************************************************************************/

// NS3 header files
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/onoff-application.h"
#include "ns3/on-off-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-classifier.h"
#include "ns3/trace-helper.h"

// Local header files
#include "netmeta.h" // meta class
#include "overlayApplication.h"
#include "hostApp.h"
#include "ueApp.h"
#include "utils.h"

// C++ std library
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#define LISTENPORT 9

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("active_passive");

double pktsPerMsToKbps(double pktsPerMs)
{
    double kbps = pktsPerMs * 12000; // pktsPerMs * 1500 * 8 * / 1000 / 1000
    return kbps;
}

void runSimulation(uint32_t topo_idx, netmeta& meta)
{
    std::chrono::steady_clock::time_point prog_start_time = std::chrono::steady_clock::now();

    uint32_t num_nodes = meta.n_nodes;

    double app_start_time = 21200; //22000; // in us;
    double stop_buffer_time = 1000; //100; // in milliseconds; to give time for remaining pkts to be received
    double sim_stop_time = stop_buffer_time + (app_start_time*MICROSECS_TO_MS)
                                + std::max((int) std::ceil(meta.pkt_delay_secs*(meta.max_num_pkts_per_dest+1)),
                                    (int) (meta.probe_delay*(meta.max_num_probes_per_pair+1)))*SECS_TO_MS; // in ms
    sim_stop_time = sim_stop_time * MS_TO_SECS; // convert time back to secs for precision
    // sim_stop_time = 11123; // in ms; when 100 pkts per dest and 10 probes per dest pair are sent
    std::cout << "Simulation stop time (ms): " << sim_stop_time << std::endl; // for debugging
    Time time_stop_simulation = Seconds(sim_stop_time);

    /**
     * Tree Nodes
     *
     */
    NodeContainer treeNodes;
    treeNodes.Create(num_nodes);

    /**
     * Host Node
     */
    // Every tree has one host/root node, (e.g., node 0)
    NodeContainer hostNode;
    hostNode.Add(treeNodes.Get(meta.host_idx));

    /**
     * UE Nodes (a.k.a. leaf/destination nodes)
     */
    NodeContainer ueNodes;

    /**
     * Router Nodes
     */
    NodeContainer routerNodes;
    
    // Besides the host nodes, sort the rest of the
    // tree nodes into ueNodes and routerNodes.
    for (uint32_t node_idx = 0; node_idx < num_nodes; node_idx++)
    {
        Ptr<Node> treeNode = treeNodes.Get(node_idx);
        if (meta.is_leaf_node(node_idx))
        {   // node is a leaf node
            ueNodes.Add(treeNode);
        }
        else
        {   // node is a router node
            routerNodes.Add(treeNode);
        }
    }
    
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Aggregate IP/TCP/UCP functionality to the tree nodes.
    InternetStackHelper internet;
    internet.Install(treeNodes);


    // Vector to store the applications of all the nodes.
    std::vector<Ptr<overlayApplication>> vec_app(num_nodes);


    /**
     * Install Applications for all the nodes.
     * It is generally recommended to install the applications
     * after populating the routing tables.
     * Install on/off background traffic applications for
     * all the nodes as well.
     **/
    ObjectFactory oa_fact; // overlayApp object factory
    oa_fact.SetTypeId("ns3::overlayApplication");
    oa_fact.Set("RemotePort", UintegerValue(LISTENPORT));
    oa_fact.Set("ListenPort", UintegerValue(LISTENPORT));

    ObjectFactory host_fact;
    host_fact.SetTypeId("ns3::hostApp");
    host_fact.Set("RemotePort", UintegerValue(LISTENPORT));
    host_fact.Set("ListenPort", UintegerValue(LISTENPORT));

    ObjectFactory ue_fact;
    ue_fact.SetTypeId("ns3::ueApp");
    ue_fact.Set("RemotePort", UintegerValue(LISTENPORT));
    ue_fact.Set("ListenPort", UintegerValue(LISTENPORT));


    // Install host app for host node.
    Ptr<hostApp> host = host_fact.Create<hostApp>();
    host->SetNode(treeNodes.Get(meta.host_idx)); // meta.host_idx = 0
    host->InitHostApp(&meta, meta.host_idx, topo_idx);
    host->SetStartTime(MicroSeconds(app_start_time));
    host->SetStopTime(Seconds(sim_stop_time));
    treeNodes.Get(meta.host_idx)->AddApplication(host);
    vec_app[meta.host_idx] = host;

    // Install applications for leaf and router nodes.
    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        if (meta.is_leaf_node(node_idx))
        {   // Install application for leaf node.
            Ptr<ueApp> ue = ue_fact.Create<ueApp>();
            ue->SetNode(treeNodes.Get(node_idx));
            ue->InitUeApp(&meta, node_idx, topo_idx);
            ue->SetStartTime(MicroSeconds(app_start_time));
            ue->SetStopTime(Seconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(ue);
            vec_app[node_idx] = ue;
        }
        else // router node
        {   // Install application for router node.
            Ptr<overlayApplication> router = oa_fact.Create<overlayApplication>();
            router->SetNode(treeNodes.Get(node_idx));
            router->InitApp(&meta, node_idx, topo_idx);
            router->SetStartTime(MicroSeconds(app_start_time));
            router->SetStopTime(Seconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(router);
            vec_app[node_idx] = router;
        }
    }

    std::cout << "Installed applications on network nodes..." << std::endl;
    //std::cout << "Host idx: " << meta.host_idx << std::endl; // for debugging, meta.host_idx = 0

    /**
     *  Install the p2p links btwn the tree nodes and sockets on the UEs.
    */
    std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec = meta.neighbors_vec;
    int num_endnodes = 2; // number of nodes a single link is attached to.
    NetDeviceContainer netDeviceContainer;
    std::vector<Ptr<Ipv4>> linkIpv4(num_endnodes);
    std::vector<Ipv4InterfaceAddress> linkIpv4Addr(num_endnodes);
    std::vector<uint32_t> n_devices_perNode(num_endnodes);
    std::vector<Ptr<Node>> endnodes(num_endnodes);

    PointToPointHelper link;
    link.DisableFlowControl();
    link.SetChannelAttribute("Delay", StringValue(std::to_string(meta.prop_delay) + "us")); // no prop delay for now.
    link.SetDeviceAttribute("DataRate", StringValue(std::to_string(meta.link_capacity) + "Gbps")); // link capacity: 10 Gbps
    // Set the max size of the queue buffers. Default is 100 packets.
    link.SetQueue("ns3::DropTailQueue", "MaxSize", QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, meta.max_queue_size)));

    Ipv4Address ueIPAddr;

    for (auto const& srcdest : neighbors_vec)
    {
        Ptr<overlayApplication> first_vec_app = vec_app[srcdest.first];
        Ptr<overlayApplication> second_vec_app = vec_app[srcdest.second];
        endnodes[0] = treeNodes.Get(srcdest.first);
        endnodes[1] = treeNodes.Get(srcdest.second);
        
        // Create a link between 2 nodes in the underlay.
        NetDeviceContainer tmpLinkContainer = link.Install(endnodes[0], endnodes[1]);
        address.Assign(tmpLinkContainer);
        address.NewNetwork();
        netDeviceContainer.Add(tmpLinkContainer);
        

        for (int i = 0; i < num_endnodes; i++)
        {
            linkIpv4[i] = endnodes[i]->GetObject<Ipv4>();
            n_devices_perNode[i] = endnodes[i]->GetNDevices();
            linkIpv4Addr[i] = linkIpv4[i]->GetAddress(n_devices_perNode[i] - 1, 0);
            // IPv4 interfaces (device ID) are 0-indexed.
            // In GetAddress(), the 2nd argument is 0 bcuz we want to configure the 1st address of that interface.
            // An interface can have multiple IP addresses, but this is not necessary for regular use cases.
            // So unless you're using multiple IP addresses per interface, keep it 0.
        }

        // Create and install sending sockets for both end nodes to
        // send packets (especially background traffic) to each other.
        first_vec_app->SetSendSocket(linkIpv4Addr[1].GetLocal(), srcdest.second, (uint32_t) (n_devices_perNode[0] - 1));
        second_vec_app->SetSendSocket(linkIpv4Addr[0].GetLocal(), srcdest.first, (uint32_t) (n_devices_perNode[1] - 1));

        // Store the IP address of the UE in a variable.
        if (meta.is_leaf_node((int) srcdest.first))
        {
            ueIPAddr = linkIpv4Addr[0].GetLocal();
        }
        else if (meta.is_leaf_node((int) srcdest.second))
        {
            ueIPAddr = linkIpv4Addr[1].GetLocal();
        }
        
        // Print nodes IP addresses for debugging.
        // std::cout << "Node " << srcdest.first << " :" << linkIpv4Addr[0].GetAddress() << std::endl;
        // std::cout << "Node " << srcdest.second << " :" << linkIpv4Addr[1].GetAddress() << std::endl;
        

        // Install receiver sockets sender socket on the host.
        if (meta.is_leaf_node((int) srcdest.first))
        {   // First node is the UE node.
            // std::cout << "First leaf " << srcdest.first << std::endl; // for debugging
            // The if block never seems to run, but keep it just in case.
            Ptr<ueApp> first_ue_app = DynamicCast<ueApp>(first_vec_app);
            first_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.first);
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.first, 0); //send_sockets[srcdest.first]
            second_vec_app->SetRecvSocket(linkIpv4Addr[1].GetLocal(), (uint32_t) srcdest.second);
        }
        else if (meta.is_leaf_node((int) srcdest.second))
        {   // Second node is the UE node.
            // std::cout << "Second leaf " << srcdest.second << std::endl; // for debugging
            Ptr<ueApp> second_ue_app = DynamicCast<ueApp>(second_vec_app);
            second_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.second);
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.second, 0); //send_sockets[srcdest.second]
            first_vec_app->SetRecvSocket(linkIpv4Addr[0].GetLocal(), (uint32_t) srcdest.first);
            // For debugging:
            // std::cout << "Host idx: " << meta.host_idx << ", Dest node: " << srcdest.first;
            // std::cout << "UE socket: " << vec_app[meta.host_idx]->send_sockets[srcdest.first] << std::endl;
        }
        else // neither of the nodes are leaf nodes
        {   // So install overlay application receiver sockets on both of them.
            first_vec_app->SetRecvSocket(linkIpv4Addr[0].GetLocal(), (uint32_t) srcdest.first);
            second_vec_app->SetRecvSocket(linkIpv4Addr[1].GetLocal(), (uint32_t) srcdest.second);
        }
    } // end neighbors_vec for loop

    // Clear temporary vectors
    linkIpv4.clear();
    linkIpv4Addr.clear();
    n_devices_perNode.clear();
    endnodes.clear();

    std::cout << "Installed p2p links and UE sockets..." << std::endl;
    std::cout << "Installed background traffic applications on network nodes..." << std::endl;

    // Populate the routing table.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    std::cout << "Populated routing table..." << std::endl;

    /**
     * Run Simulation
    */
    NS_LOG_INFO("Run Simulation.");
    std::cout << "Running network simulation " << topo_idx << "..." << std::endl;
    // Simulator::Schedule(time_stop_simulation, stop_NR, vec_NrHelper);
    Simulator::Stop(time_stop_simulation);
    Simulator::Run();
    std::cout << "Finished running network simulation " << topo_idx << "..." << std::endl;
    // Destroy simulation
    std::cout << "Destroying simulation " << topo_idx << "..." << std::endl;
    Simulator::Destroy();
    NS_LOG_INFO("Simulation Complete.");

    /* Write pkt and probe stats of the current topology to .csv files. */
    std::string pkt_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/passive_measurements/";
    std::string probe_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/active_measurements/";
    meta.write_pkt_delays_for_curr_topo();
    meta.write_probe_delays_for_curr_topo();
    std::cout << "Finished writing passive and active measurements to .csv files "
        << "for topology " << meta.topo_idx << "." << std::endl;

    std::chrono::steady_clock::time_point prog_end_time = std::chrono::steady_clock::now();
    int64_t total_elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds> (prog_end_time - prog_start_time).count();
    int64_t total_elapsed_hrs = int64_t(total_elapsed_time / 60.0 / 60.0);
    int64_t total_elapsed_mins = int64_t(total_elapsed_time / 60) % 60;
    int64_t total_elapsed_secs = total_elapsed_time % 60;
    std::cout << "Total time taken to run simulation " << (int)topo_idx << ": " << total_elapsed_hrs << " hrs, "
            << total_elapsed_mins << " mins and " << total_elapsed_secs << " secs." << std::endl;
} // end of runSimulation()

int main(int argc, char* argv[])
{
    bool logging = false;
    if (logging)
    {
        LogComponentEnable("netmeta", LOG_LEVEL_INFO);
        LogComponentEnable("overlayApplication", LOG_LEVEL_FUNCTION);
    }
    
    netmeta meta = netmeta(0); // contains network's meta info
    uint32_t num_topos = meta.n_topos; //20;
    uint32_t starting_topo = 1; //0
    int num_processes = 8;

    for (uint32_t topo_idx = starting_topo; topo_idx < num_topos; topo_idx++)
    {
        meta = netmeta(topo_idx); // create new meta object for each topo
        
        pid_t pid = fork(); // create new child process
        if (pid < 0) {
            std::cerr << "Error creating child process." << std::endl;
            return 1;
        } else if (pid == 0) {
            // Child process: handle the topology with index topo_idx
            std::cout << "Child process " << getpid() << " handling topology "
                << (int)topo_idx << "." << std::endl;
            // Run the simulation for the topology.
            runSimulation(topo_idx, meta);
            // Exit the child process after completing the job.
            exit(0);
        }

        // If the maximum number of concurrent processes is reached,
        // wait for any child process to finish before simulating the next topo.
        if (topo_idx >= num_processes-1) {
            wait(NULL);
        }
    } // end of topology for loop

    // Wait for the remaining child processes to finish.
    for (int i = 0; i < num_processes; ++i) {
        wait(NULL);
    }

    std::cout << "******All topologies simulated.******" << std::endl;


    return 0;
}