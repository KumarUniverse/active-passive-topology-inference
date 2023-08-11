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
    meta.topo_idx = topo_idx;

    uint32_t num_nodes = meta.n_nodes_gt[topo_idx];
    // uint32_t num_leaves = meta.n_leaves_gt[topo_idx];   // unused
    // uint32_t num_edges = meta.n_edges_gt[topo_idx];     // unused
    // uint32_t num_routers = meta.n_routers_gt[topo_idx]; // unused

    double start_buffer_time = 10000; //1000; // in microseconds; to ensure background traffic starts before other apps
    double app_start_time = 21200 + start_buffer_time; //22000; // in us;
    // 0.1s is the amount of time it takes to send 1 data packet to a leaf node.
    double stop_buffer_time = 1000; //3000; //100; // in milliseconds; to give apps time to stop
    double sim_stop_time = stop_buffer_time + (app_start_time*MICROSECS_TO_MS)
                                + std::max((int) std::ceil(meta.pkt_delay_secs*(meta.max_num_pkts_per_dest+1)),
                                    (int) (meta.probe_delay*(meta.max_num_probes_per_pair+1)))*SECS_TO_MS; // in ms
    sim_stop_time = sim_stop_time * MS_TO_SECS; // convert time back to secs for precision
    // sim_stop_time = 11123; // in ms; when 100 pkts per dest and 10 probes per dest pair are sent
    //std::cout << "Simulation stop time (ms): " << sim_stop_time << std::endl; // for debugging
    Time time_stop_simulation = Seconds(sim_stop_time);

    // Time class experiment
    // Kind works. Time objects can store the time value as a double when the unit is in seconds.
    // However, when the unit is in microseconds, the double gets truncated to an integer.
    // std::cout << "Testing if Time can be stored as a double..." << std::endl;
    // double foo_s = 2.7; // in secs
    // Time foo_time = Seconds(foo_s);
    // foo_s = foo_time.GetDouble(); // 2.7e9; in nanoseconds
    // std::cout << "Foo time (2.7s to ns): " << foo_s << std::endl;
    // foo_s = 2.7; // in ms
    // foo_time = MilliSeconds(foo_s);
    // foo_s = foo_time.GetDouble(); // 2e6; in nanoseconds
    // std::cout << "Foo time2 (2.7ms to ns): " << foo_s << std::endl;
    // foo_s = 2.7; // in microseconds
    // foo_time = MicroSeconds(foo_s);
    // foo_s = foo_time.GetDouble(); // 2e3; in nanoseconds
    // std::cout << "Foo time3 (2.7us to ns): " << foo_s << std::endl;
    // foo_s = 21;
    // foo_time = MicroSeconds(foo_s);
    // foo_s = foo_time.GetDouble(); // 2e3; in nanoseconds
    // std::cout << "Foo time4 (21us to ns): " << foo_s << std::endl;


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
        if (meta.is_leaf_node(topo_idx, node_idx))
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
        if (meta.is_leaf_node(topo_idx, node_idx))
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
     * For storing OnOff Background traffic apps:
    */
    // std::vector<Ptr<OnOffApplication>> vec_onoff_apps(num_nodes*2);
    // int vec_onoff_i = 0;
    // // std::map<uint32_t, std::vector<uint32_t>> neighbors_map = meta.neighbors_maps_gt[topo_idx]; // not needed
    // std::map<std::pair<uint32_t, uint32_t>, double> edge_bkgrd_traffic_rates
    //                                             = meta.edge_bkgrd_traffic_rates_gt[topo_idx];
    

    /**
     *  Install the p2p links btwn the tree nodes and sockets on the UEs.
    */
    std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec = meta.neighbors_vectors_gt[meta.topo_idx];
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
        first_vec_app->SetSendSocket(linkIpv4Addr[1].GetLocal(), srcdest.second);
        second_vec_app->SetSendSocket(linkIpv4Addr[0].GetLocal(), srcdest.first);

        // Store the IP address of the UE in a variable.
        if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.first))
        {
            ueIPAddr = linkIpv4Addr[0].GetLocal();
        }
        else if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.second))
        {
            ueIPAddr = linkIpv4Addr[1].GetLocal();
        }
        
        // Print nodes IP addresses for debugging.
        // std::cout << "Node " << srcdest.first << " :" << linkIpv4Addr[0].GetAddress() << std::endl;
        // std::cout << "Node " << srcdest.second << " :" << linkIpv4Addr[1].GetAddress() << std::endl;
        

        // Install receiver sockets sender socket on the host.
        if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.first))
        {   // First node is the UE node.
            // std::cout << "First leaf " << srcdest.first << std::endl; // for debugging
            // The if block never seems to run, but keep it just in case.
            Ptr<ueApp> first_ue_app = DynamicCast<ueApp>(first_vec_app);
            first_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[0] - 1));
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.first); //send_sockets[srcdest.first]
            second_vec_app->SetRecvSocket(linkIpv4Addr[1].GetLocal(),
            (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[1] - 1));
        }
        else if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.second))
        {   // Second node is the UE node.
            // std::cout << "Second leaf " << srcdest.second << std::endl; // for debugging
            Ptr<ueApp> second_ue_app = DynamicCast<ueApp>(second_vec_app);
            second_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[1] - 1));
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.second); //send_sockets[srcdest.second]
            first_vec_app->SetRecvSocket(linkIpv4Addr[0].GetLocal(),
            (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[0] - 1));
            // For debugging:
            // std::cout << "Host idx: " << meta.host_idx << ", Dest node: " << srcdest.first;
            // std::cout << "UE socket: " << vec_app[meta.host_idx]->send_sockets[srcdest.first] << std::endl;
        }
        else // neither of the nodes are leaf nodes
        {   // So install overlay application receiver sockets on both of them.
            first_vec_app->SetRecvSocket(linkIpv4Addr[0].GetLocal(),
            (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[0] - 1));
            second_vec_app->SetRecvSocket(linkIpv4Addr[1].GetLocal(),
            (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[1] - 1));
        }
        

        /**
         * Install background apps on the endnodes to send background traffic
         * between all the nodes in the network. WORKS!
        */
        // for (int i = 0; i < num_endnodes; i++)
        // {
        //     uint32_t node_idx = endnodes[i]->GetId();
        //     uint32_t neighbor_idx = endnodes[(i+1)%num_endnodes]->GetId();
        //     Ipv4Address neighbor_ip_addr = linkIpv4Addr[(i+1)%num_endnodes].GetLocal();
        //     auto src_dest_pair = std::make_pair(node_idx, neighbor_idx);
        //     double bkgrd_rate = edge_bkgrd_traffic_rates[src_dest_pair];
        //     uint64_t kbps = pktsPerMsToKbps(bkgrd_rate);
        //     std::string kbps_str = std::to_string(kbps) + "kbps";
        //     //std::string kbps_str = "10000kbps"; // for debugging
        //     //std::cout << "Node i: " << (int)node_idx << ", Node j: " << (int)neighbor_idx
        //     << ", kbps: " << kbps_str << std::endl; // for debuging, usually 1-3 kbps

        //     // OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ipAddr, 0));
        //     OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(neighbor_ip_addr, LISTENPORT));
        //     // onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));  // for debugging
        //     // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]")); // for debugging
        //     Ptr<RandomVariableStream> onPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for on
        //     onPareto->SetAttribute("Scale", DoubleValue(meta.on_pareto_scale)); // set the scale parameter of the distribution
        //     onPareto->SetAttribute("Shape", DoubleValue(meta.on_pareto_shape)); // set the shape parameter of the distribution
        //     onPareto->SetAttribute("Bound", DoubleValue(meta.on_pareto_bound)); // set the upper limit of the distribution
        //     onOffHelper.SetAttribute("OnTime", PointerValue(onPareto)); // in seconds
        //     //onOffHelper.SetAttribute("OnTime", TimeValue(MicroSeconds(onPareto->GetValue()))); // ERROR
        //     Ptr<RandomVariableStream> offPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for off duration
        //     offPareto->SetAttribute("Scale", DoubleValue(meta.off_pareto_scale));
        //     offPareto->SetAttribute("Shape", DoubleValue(meta.off_pareto_shape));
        //     onPareto->SetAttribute("Bound", DoubleValue(meta.off_pareto_bound));
        //     onOffHelper.SetAttribute("OffTime", PointerValue(offPareto)); // in seconds
        //     //onOffHelper.SetAttribute("OffTime", TimeValue(MicroSeconds(offPareto->GetValue()))); // ERROR
        //     onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(kbps_str)));
        //     onOffHelper.SetAttribute("PacketSize", UintegerValue(meta.pkt_size));    // pkt_size = 1500 bytes;
        //     ApplicationContainer tmpAppContainer = onOffHelper.Install(endnodes[i]);
            
        //     // The first app in the temporary container is the onoff app.
        //     Ptr<OnOffApplication> onOffApp = DynamicCast<OnOffApplication>(tmpAppContainer.Get(0));
        //     // Start bckgrd apps earlier than other apps
        //     onOffApp->SetStartTime(MicroSeconds(app_start_time - start_buffer_time));
        //     //std::cout << "Setting bkgrd traffic stop time for node " << node_idx << std::endl; // for debugging
        //     onOffApp->SetStopTime(Seconds(sim_stop_time));
        //     vec_onoff_apps[vec_onoff_i] = onOffApp;
        //     treeNodes.Get(node_idx)->AddApplication(vec_onoff_apps[vec_onoff_i++]);
        // }
    } // end neighbors_vec for loop
    std::cout << "Installed p2p links and UE sockets..." << std::endl;
    std::cout << "Installed background traffic applications on network nodes..." << std::endl;
    //^^Potential error: assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"

    // Set up sockets from the source node (node 0) to all the dest nodes.
    // MAY NO LONGER BE NEEDED:
    //connectHostToDestNodes(vec_app, address, treeNodes, meta);

    // Populate the routing table.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    std::cout << "Populated routing table..." << std::endl;

    // Print routing tables for debugging.
    // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);
    // // Host node:
    // Ipv4RoutingHelper::PrintRoutingTableAt (MicroSeconds (5), treeNodes.Get(0), routingStream, Time::Unit::NS);
    // // Router node:
    // Ipv4RoutingHelper::PrintRoutingTableAt (MicroSeconds (5), treeNodes.Get(1), routingStream, Time::Unit::NS);
    // // Leaf node:
    // Ipv4RoutingHelper::PrintRoutingTableAt (MicroSeconds (5), treeNodes.Get(4), routingStream, Time::Unit::NS);
    // //Ipv4RoutingHelper::PrintRoutingTableAllAt (MilliSeconds (5), routingStream, Time::Unit::S);

    // // Print packet flows for debugging.
    // FlowMonitorHelper flowmonHelper;
    // Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install (hostNode);
    // // Set flow monitor's histogram attributes:
    // monitor->SetAttribute ("DelayBinWidth", DoubleValue (0.001));
    // monitor->SetAttribute ("JitterBinWidth", DoubleValue (0.001));
    // monitor->SetAttribute ("PacketSizeBinWidth", DoubleValue (20));

    
    /**
     * Packet Tracing
    */
    // AsciiTraceHelper ascii;
    // PointToPointHelper pointToPoint;
    // // Generate trace file of all packets in the network.
    // pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("active_passive_pkt_trace.tr"));
    // pointToPoint.EnablePcapAll ("active_passive_pkt_trace"); // generate pcap file of 
    
    // Print packet traces using NS3 trace methods.
    // Config::Connect( "/NodeList/*/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&txTraceIpv4) );
    // Config::Connect( "/NodeList/*/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&txTraceIpv4) );
    // Config::Connect( "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTx", MakeCallback(&p2pDevMacTx) );
    // Config::Connect( "/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacRx", MakeCallback(&p2pDevMacRx) );
    
    // Config::Connect( "/NodeList/0/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&txTraceIpv4) ); // host node transmit
    // Config::Connect( "/NodeList/0/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&txTraceIpv4) ); // host node receive
    // // Config::Connect( "/NodeList/1/DeviceList/*/$ns3::PointToPointNetDevice/MacRx", MakeCallback(&p2pDevMacRx) ); // first router receive in MAC layer
    // Config::Connect( "/NodeList/1/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&txTraceIpv4) ); // first router transmit
    // Config::Connect( "/NodeList/1/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&txTraceIpv4) ); // first router receive
    // Config::Connect( "/NodeList/4/$ns3::Ipv4L3Protocol/Tx", MakeCallback(&txTraceIpv4) ); // leaf node transmit
    // Config::Connect( "/NodeList/4/$ns3::Ipv4L3Protocol/Rx", MakeCallback(&txTraceIpv4) ); // leaf node receive

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


    // Print per-flow statistics
    // monitor->CheckForLostPackets ();
    // Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmonHelper.GetClassifier ());
    // FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();

    // double averageFlowThroughput = 0.0;
    // double averageFlowDelay = 0.0;

    // std::ofstream outFile;
    // std::string simTag = "default";
    // std::string outputDir = "./";
    // std::string filename = outputDir + "/" + simTag;
    // outFile.open (filename.c_str (), std::ofstream::out | std::ofstream::trunc);
    // if (!outFile.is_open ())
    // {
    //     std::cerr << "Can't open file " << filename << std::endl;
    //     return 1;
    // }

    // outFile.setf (std::ios_base::fixed);

    // double flowDuration = (time_stop_simulation - Time(MicroSeconds(app_start_time))).GetSeconds();
    // for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    // {
    //     Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
    //     std::stringstream protoStream;
    //     protoStream << (uint16_t) t.protocol;
    //     if (t.protocol == 6) protoStream.str ("TCP"); 
    //     if (t.protocol == 17) protoStream.str ("UDP");
    //     outFile << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> " << t.destinationAddress << ":" << t.destinationPort << ") proto " << protoStream.str () << "\n";
    //     outFile << "  Tx Packets: " << i->second.txPackets << "\n";
    //     outFile << "  Tx Bytes:   " << i->second.txBytes << "\n";
    //     outFile << "  TxOffered:  " << i->second.txBytes * 8.0 / flowDuration / 1000.0 / 1000.0  << " Mbps\n";
    //     outFile << "  Rx Bytes:   " << i->second.rxBytes << "\n";
    //     outFile << "  Pkts Dropped:   " << i->second.lostPackets << "\n";
    //     if (i->second.rxPackets > 0)
    //     {
    //         // Measure the duration of the flow from receiver's perspective
    //         averageFlowThroughput += i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000;
    //         averageFlowDelay += 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets;

    //         outFile << "  Throughput: " << i->second.rxBytes * 8.0 / flowDuration / 1000 / 1000  << " Mbps\n";
    //         outFile << "  Mean delay:  " << 1000 * i->second.delaySum.GetSeconds () / i->second.rxPackets << " ms\n";
    //         //outFile << "  Mean upt:  " << i->second.uptSum / i->second.rxPackets / 1000/1000 << " Mbps \n";
    //         outFile << "  Mean jitter:  " << 1000 * i->second.jitterSum.GetSeconds () / i->second.rxPackets  << " ms\n";
    //     }
    //     else
    //     {
    //         outFile << "  Throughput:  0 Mbps\n";
    //         outFile << "  Mean delay:  0 ms\n";
    //         outFile << "  Mean jitter: 0 ms\n";
    //     }
    //     outFile << "  Rx Packets: " << i->second.rxPackets << "\n";
    // }

    // outFile << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size () << "\n";
    // outFile << "  Mean flow delay: " << averageFlowDelay / stats.size () << "\n";

    // outFile.close ();

    // std::ifstream f (filename.c_str ());

    // if (f.is_open ())
    // {
    //     std::cout << f.rdbuf ();
    // }

    /* Write pkt and probe stats of the current topology to .csv files. */
    std::string pkt_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/passive_measurements/";
    std::string probe_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/active_measurements/";
    meta.write_pkt_delays_for_curr_topo(pkt_delays_path);
    meta.write_probe_delays_for_curr_topo(probe_delays_path);
    std::cout << "Finished writing passive and active measurements to .csv files "
        << "for topology " << meta.topo_idx << "." << std::endl;

    std::chrono::steady_clock::time_point prog_end_time = std::chrono::steady_clock::now();
    int64_t total_elapsed_time =
        std::chrono::duration_cast<std::chrono::seconds> (prog_end_time - prog_start_time).count();
    int64_t total_elapsed_hrs = int64_t(total_elapsed_time / 60.0 / 60.0);
    int64_t total_elapsed_mins = int64_t(total_elapsed_time / 60) % 60;
    int64_t total_elapsed_secs = total_elapsed_time % 60;
    std::cout << "Total time taken to run simulation " << (int)topo_idx << ": " << total_elapsed_hrs << " hrs, "
            << total_elapsed_mins << " mins and " << total_elapsed_secs << "secs." << std::endl;
}

int main(int argc, char* argv[])
{
    bool logging = false;
    if (logging)
    {
        LogComponentEnable("netmeta", LOG_LEVEL_INFO);
        LogComponentEnable("overlayApplication", LOG_LEVEL_FUNCTION);
    }
    
    netmeta meta = netmeta(); // contains network's meta info
    uint32_t num_topos = meta.n_topos; // 20;
    int num_processes = 4;

    for (uint32_t topo_idx = 0; topo_idx < num_topos; topo_idx++)
    {
        meta = netmeta(); // create new meta object for each topo
        
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


    // COMMENT OUT WHEN DEBUGGING: (probably no longer needed)
    /* Write pkt and probe stats of all the topologies to .csv files. */
    // std::string pkt_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/passive_measurements/";
    // std::string probe_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/active_measurements/";
    // meta.write_pkt_delays(pkt_delays_path);
    // meta.write_probe_delays(probe_delays_path);
    // std::cout << "Finished writing passive and active measurements to .csv files." << std::endl;


    return 0;
}