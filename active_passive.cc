
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/onoff-application.h"
#include "ns3/on-off-helper.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
//#include "ns3/mac48-address.h"
// #include "ns3/node.h"
// #include "ns3/net-device.h"

// Flow monitor headers
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-classifier.h"

// Trace helper header:
#include "ns3/trace-helper.h"

// No need for 5G or wireless
//#include "ns3/nr-mac-scheduler-tdma-rr.h"
//#include "ns3/nr-module.h"
//#include <ns3/antenna-module.h>

// Local header files
#include "netmeta.h" // meta class
#include "overlayApplication.h"
#include "hostApp.h"
#include "ueApp.h"
#include "utils.h"

// C++ std
#include <string>
#include <vector>
#include <algorithm>

#define LISTENPORT 9

using namespace ns3;

/*
Simulation for active-passive research.
Part of this code contains reused code from Yudi Huang's Hex network NS3 project.
*/

NS_LOG_COMPONENT_DEFINE("active_passive");

uint64_t pktsPerMsToKbps(double pktsPerMs)
{
    uint64_t kbps = (uint64_t) (pktsPerMs * 1.2e10); // pktsPerMs * 1000 * 1500 * 8 * 1000
    return kbps;
}

// void installLinksAndSockets(std::vector<Ptr<overlayApplication>> &vec_app, Ipv4AddressHelper &address,
//                     NodeContainer &treeNodes, netmeta &meta, NetDeviceContainer &netDeviceContainer)
// {
//     /**
//      * Install the links of the network and sockets on the UEs.
//     */
//     std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec = meta.neighbors_vectors_gt[meta.topo_idx];
//     //NetDeviceContainer netDeviceContainer;
//     std::vector<Ptr<Ipv4>> linkIpv4(2);
//     std::vector<Ipv4InterfaceAddress> linkIpv4Addr(2);
//     std::vector<uint32_t> n_devices_perNode(2);
//     std::vector<Ptr<Node>> endnodes(2);

//     PointToPointHelper link;
//     link.DisableFlowControl();
//     link.SetChannelAttribute("Delay", StringValue(std::to_string(0) + "us")); // no prop delay for now.
//     link.SetDeviceAttribute("DataRate", StringValue(std::to_string(10) + "Gbps")); // link capacity: 10 Gbps

//     Ipv4Address hostIPAddr;

//     for (auto const& srcdest : neighbors_vec)
//     {
//         Ptr<overlayApplication> first_vec_app = vec_app[srcdest.first];
//         Ptr<overlayApplication> second_vec_app = vec_app[srcdest.second];
//         endnodes[0] = treeNodes.Get(srcdest.first);
//         endnodes[1] = treeNodes.Get(srcdest.second);
        
//         // Create a link between 2 nodes in the underlay.
//         netDeviceContainer = link.Install(endnodes[0], endnodes[1]);
//         address.Assign(netDeviceContainer);
//         address.NewNetwork();

//         for (int i = 0; i < 2; i++)
//         {
//             linkIpv4[i] = endnodes[i]->GetObject<Ipv4>();
//             n_devices_perNode[i] = endnodes[i]->GetNDevices();
//             linkIpv4Addr[i] = linkIpv4[i]->GetAddress(n_devices_perNode[i] - 1, 0); // IPv4 interfaces (device ID) are 0-indexed.
//             // In GetAddress(), the 2nd argument is 0 bcuz we want to configure the 1st address of that interface.
//             // An interface can have multiple IP addresses, but this is not necessary for regular use cases.
//             // So unless you're using multiple IP addresses per interface, keep it 0.
//         }

//         if (srcdest.first == meta.host_idx)
//         {
//             hostIPAddr = linkIpv4Addr[0].GetAddress();
//         }
//         else if (srcdest.second == meta.host_idx)
//         {
//             hostIPAddr = linkIpv4Addr[1].GetAddress();
//         }
        
//         // Print nodes IP addresses for debugging.
//         // std::cout << "Node " << srcdest.first << " :" << linkIpv4Addr[0].GetAddress() << std::endl;
//         // std::cout << "Node " << srcdest.first << " :" << linkIpv4Addr[1].GetAddress() << std::endl;
        

//         // Install sockets ONLY on all the UE nodes.
//         if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.first))
//         {   // First node is the UE node.
//             Ptr<ueApp> first_ue_app = DynamicCast<ueApp>(first_vec_app);
//             vec_app[meta.host_idx]->send_sockets[srcdest.first]
//                 = first_ue_app->SetSocket(hostIPAddr, (uint32_t) meta.host_idx, (uint32_t) (n_devices_perNode[0] - 1));
//             // For debugging:
//             // std::cout << "Host idx: " << meta.host_idx << ", Dest node: " << srcdest.first;
//             // std::cout << "UE socket: " << vec_app[meta.host_idx]->send_sockets[srcdest.first] << std::endl;
//         }
//         else if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.second))
//         {   // Second node is the UE node.
//             Ptr<ueApp> second_ue_app = DynamicCast<ueApp>(second_vec_app);
//             vec_app[meta.host_idx]->send_sockets[srcdest.second]
//                 = second_ue_app->SetSocket(hostIPAddr, (uint32_t) meta.host_idx, (uint32_t) (n_devices_perNode[1] - 1));
//         }
//     } // end neighbors_vec for loop

//         // Set 2 sockets: 1 from source node to destination node and the other from destination to source node.
//         // if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.first))
//         // {
//         //     Ptr<ueApp> first_ue_app = DynamicCast<ueApp>(first_vec_app);
//         //     second_vec_app->send_sockets[srcdest.first]
//         //         = first_ue_app->SetSocket(linkIpv4Addr[1].GetAddress(),
//         //             (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[0] - 1));
//         //     first_ue_app->send_sockets[srcdest.second]
//         //         = second_vec_app->SetSocket(linkIpv4Addr[0].GetAddress(),
//         //             (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[1] - 1));
//         // }
//         // else if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.second))
//         // {
//         //     Ptr<ueApp> second_ue_app = DynamicCast<ueApp>(second_vec_app);
//         //     second_ue_app->send_sockets[srcdest.first]
//         //         = first_vec_app->SetSocket(linkIpv4Addr[1].GetAddress(),
//         //             (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[0] - 1));
//         //     first_vec_app->send_sockets[srcdest.second]
//         //         = second_ue_app->SetSocket(linkIpv4Addr[0].GetAddress(),
//         //             (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[1] - 1));
//         // }
//         // else
//         // {
//         //     second_vec_app->send_sockets[srcdest.first]
//         //         = first_vec_app->SetSocket(linkIpv4Addr[1].GetAddress(),
//         //             (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[0] - 1));
//         //     first_vec_app->send_sockets[srcdest.second]
//         //         = second_vec_app->SetSocket(linkIpv4Addr[0].GetAddress(),
//         //             (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[1] - 1));
//         // }

//         // For debugging: NEED TO MODIFY
//         /* for (int k = 0; k < 2; k++)
//         {
//             std::cout << "Node ID = " << NetDevices[i].Get(k)->GetNode()->GetId() << "; ";
//             linkIpv4[k] = NetDevices[i].Get(k)->GetNode()->GetObject<Ipv4> ();
//             n_devices_perNode[k] = NetDevices[i].Get(k)->GetNode()->GetNDevices();
//             linkIpv4Addr[k] = linkIpv4[k]->GetAddress( n_devices_perNode[k]-1, 0 );
//             std::cout << "Address = " << linkIpv4Addr[k].GetLocal() << std::endl;
//             for (uint32_t l = 0; l < n_devices_perNode[k]; l++) NS_LOG_INFO( "device ID: " << l << " with address: " << linkIpv4[k]->GetAddress( l, 0 ).GetLocal() );
//         }
//         */
//     //} end neighbors_vec for loop

//     // May not be needed:
//     // Assign IP addresses to the network devices of all the nodes.
//     // uint32_t num_nodes = meta.n_nodes_gt[meta.topo_idx];
//     // for (uint32_t node_idx = 0; node_idx < num_nodes; num_nodes++)
//     // {
//     //     Ptr<Node> node = treeNodes.Get(node_idx);
//     //     Ptr<NetDevice> device = node->GetDevice(0);
//     //     Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();

//     //     // Get the interface associated with the network device (e.g., interface at index 0)
//     //     uint32_t interfaceIndex = 0;
//     //     Ipv4InterfaceAddress interfaceAddress = ipv4->GetAddress(interfaceIndex, 0);
//     //     Ipv4Address ipAddress = interfaceAddress.GetLocal();

//     //     // Set the IP address for the network device
//     //     device->SetAddress(ipAddress);
//     // }
// }

// This may be UNNESSARY. This is done by installLinksAndSockets().
// void connectHostToDestNodes(std::vector<Ptr<overlayApplication>> &vec_app,
//     Ipv4AddressHelper &address, NodeContainer &treeNodes, netmeta &meta)
// {
//     /**
//      * Provide the host node with the sockets of the destination nodes
//      * so that the host can send packets to them.
//     */
//     uint32_t num_nodes = meta.n_nodes_gt[meta.topo_idx];

//     // Ptr<Node> hostNode = treeNodes.Get(0);
//     // Ptr<Ipv4> ipv4 = hostNode->GetObject<Ipv4>();
//     // Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0);
//     // Ipv4Address hostIPAddr = iaddr.GetLocal(); // might be the wrong ipv4 addr.

//     for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
//     {
//         if (meta.is_leaf_node((int) meta.topo_idx, (int) node_idx))
//         {
//             // Ptr<Node> destNode = treeNodes.Get(node_idx);
//             // Ptr<Ipv4> destipv4 = destNode->GetObject<Ipv4>();
//             // uint32_t n_devices = destNode->GetNDevices();
//             // Ipv4InterfaceAddress destiaddr = destipv4->GetAddress(n_devices - 1, 0);
//             // Ipv4Address destIPAddr = destiaddr.GetLocal();
//             vec_app[meta.host_idx]->send_sockets[node_idx] = vec_app[node_idx]->recv_socket;
//             //^^Error:assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"
//             // line=649 terminate called without an active exception

//             //vec_app[meta.host_idx]->SetSocket(destIPAddr, node_idx, n_devices - 1);
//             // assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"
//             //Ptr<ueApp> uApp = DynamicCast<ueApp>(vec_app[node_idx]);
//             // Set the callback function for the UE's socket
//             // so that it can read the packets that it receives.
//             //uApp->SetRecvSocketCallback(vec_app[meta.host_idx]->send_sockets[node_idx]);
//         }
//     }
// }

// void installApps(std::vector<Ptr<overlayApplication>> &vec_app, NodeContainer &treeNodes,
//                 netmeta &meta, double app_start_time, double sim_stop_time)
// {
//     /**
//      * Install Applications for all the nodes.
//      * It is generally recommended to install the applications
//      * after populating the routing tables.
//      * Install on/off background traffic applications for
//      * all the nodes as well.
//      **/
//     uint32_t topo_idx = meta.topo_idx;
//     uint32_t num_nodes = meta.n_nodes_gt[topo_idx];

//     ObjectFactory oa_fact; // overlayApp object factory
//     oa_fact.SetTypeId("ns3::overlayApplication");
//     oa_fact.Set("RemotePort", UintegerValue(LISTENPORT));
//     oa_fact.Set("ListenPort", UintegerValue(LISTENPORT));

//     ObjectFactory host_fact;
//     host_fact.SetTypeId("ns3::hostApp");
//     host_fact.Set("RemotePort", UintegerValue(LISTENPORT));
//     host_fact.Set("ListenPort", UintegerValue(LISTENPORT));

//     ObjectFactory ue_fact;
//     ue_fact.SetTypeId("ns3::ueApp");
//     ue_fact.Set("RemotePort", UintegerValue(LISTENPORT));
//     ue_fact.Set("ListenPort", UintegerValue(LISTENPORT));


//     // Install host app for host node.
//     Ptr<hostApp> host = host_fact.Create<hostApp>();
//     host->SetNode(treeNodes.Get(meta.host_idx)); // meta.host_idx = 0
//     //host->Bar(); // for debugging
//     host->InitApp(&meta, meta.host_idx, topo_idx);
//     host->SetStartTime(MicroSeconds(app_start_time));
//     host->SetStopTime(MilliSeconds(sim_stop_time));
//     treeNodes.Get(meta.host_idx)->AddApplication(host);
//     vec_app[meta.host_idx] = host;

//     for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
//     {
//         if (meta.is_leaf_node(topo_idx, node_idx))
//         {   // Install application for leaf nodes.
//             Ptr<ueApp> ue = ue_fact.Create<ueApp>();
//             ue->SetNode(treeNodes.Get(node_idx));
//             //vec_app[node_idx] = ue;
//             ue->InitUeApp(&meta, node_idx, topo_idx, *vec_app[meta.host_idx]);
//             ue->SetStartTime(MicroSeconds(app_start_time));
//             ue->SetStopTime(MilliSeconds(sim_stop_time));
//             treeNodes.Get(node_idx)->AddApplication(ue);
//             //ue->SetRecvSocket(); // potential ERROR you may get here: assert failed. cond="socketFactory"
//             //^^Potential error: failed to bind UE's receive socket
//             vec_app[node_idx] = ue;
//         }
//         else // router node
//         {   // Install application for router nodes.
//             Ptr<overlayApplication> router = oa_fact.Create<overlayApplication>();
//             router->SetNode(treeNodes.Get(node_idx));
//             router->InitApp(&meta, node_idx, topo_idx);
//             router->SetStartTime(MicroSeconds(app_start_time));
//             router->SetStopTime(MilliSeconds(sim_stop_time));
//             treeNodes.Get(node_idx)->AddApplication(router);
//             vec_app[node_idx] = router;
//         }
//     }

//     /**
//      * Install Background Traffic Applications
//     */
//     std::vector<ApplicationContainer> vec_onoff_apps;
//     std::map<uint32_t, std::vector<uint32_t>> neighbors_map = meta.neighbors_maps_gt[topo_idx];
//     std::map<std::pair<uint32_t, uint32_t>, double> edge_bkgrd_traffic_rates
//                                                 = meta.edge_bkgrd_traffic_rates_gt[topo_idx];
//     // Set up an on-off application at each node to send background traffic between the links.
//     for (uint32_t node_idx = 0; node_idx < num_nodes; node_idx++)
//     {
//         std::vector<uint32_t> neighbors = neighbors_map[node_idx];
//         Ptr<Node> node = treeNodes.Get(node_idx);
//         Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
//         uint32_t n_devices = node->GetNDevices();
//         Ipv4InterfaceAddress iaddr = ipv4->GetAddress(n_devices - 1,0);
//         Ipv4Address ipAddr = iaddr.GetLocal();
//         for (uint32_t neighbor_idx : neighbors)
//         {
//             auto src_dest_pair = std::pair<uint32_t, uint32_t>(node_idx, neighbor_idx);
//             double bkgrd_rate = edge_bkgrd_traffic_rates[src_dest_pair];
//             uint64_t kbps = pktsPerMsToKbps(bkgrd_rate);
//             std::string kbps_str = std::to_string(kbps) + "kbps";
//             OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address());
//             Ptr<RandomVariableStream> onPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for on
//             onPareto->SetAttribute("Scale", DoubleValue(5.0)); // set the scale parameter of the Pareto distribution
//             onPareto->SetAttribute("Shape", DoubleValue(1.5)); // set the shape parameter of the Pareto distribution
//             // When the scale parameter is equal to 1, the distribution starts at (1, shapeValue).
//             // The scale parameter for the on period typically represents the minimum burst length.
//             onOffHelper.SetAttribute("OnTime", PointerValue(onPareto));
//             // onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=5]"));
//             // onTime is the amount of time in seconds the on/off application send packets before turning off again.
//             Ptr<RandomVariableStream> offPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for off
//             offPareto->SetAttribute("Scale", DoubleValue(1.0)); // set the scale parameter of the Pareto distribution
//             offPareto->SetAttribute("Shape", DoubleValue(1.0)); // set the shape parameter of the Pareto distribution
//             onOffHelper.SetAttribute("OffTime", PointerValue(offPareto));
//             // The scale parameter for the off period typically represents the minimum off length.
//             // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
//             // offTime is the amount of time in seconds the on/off app stays off before turning on again.
//             //std::cout << "Data rate: " << kbps_str << std::endl; // for debugging.
//             onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(kbps_str)));
//             onOffHelper.SetAttribute("PacketSize", UintegerValue(meta.pkt_size));
//             AddressValue remoteAddress(InetSocketAddress(ipAddr, LISTENPORT));
//             onOffHelper.SetAttribute("Remote", remoteAddress);
//             ApplicationContainer onOffAppCont = onOffHelper.Install(treeNodes.Get(node_idx));
//             onOffAppCont.Start(MicroSeconds(app_start_time));
//             // The first app in the container is the onoff app. // Not needed.
//             // Ptr<OnOffApplication> onoffApp = onOffAppCont.Get(0);
//             // onoffApp->SetStartTime(MicroSeconds(app_start_time));
//             //vec_onoff_apps.push_back(onoffApp);
//             vec_onoff_apps.push_back(onOffAppCont);
//         }
//     }
// }

int main(int argc, char* argv[])
{
    // Logging
    bool logging = false;
    if (logging)
    {
        LogComponentEnable("netmeta", LOG_LEVEL_INFO);
        LogComponentEnable("overlayApplication", LOG_LEVEL_FUNCTION);
    }
    
    netmeta meta = netmeta(); // contains network meta info
    //uint32_t num_topos = meta.n_topos; // 20

    //for (uint32_t topo_idx = 0; topo_idx < num_topos; topo_idx++)
    for (uint32_t topo_idx = 0; topo_idx < 1; topo_idx++) // for debugging
    {
    meta.topo_idx = topo_idx;

    uint32_t num_nodes = meta.n_nodes_gt[topo_idx];
    // uint32_t num_leaves = meta.n_leaves_gt[topo_idx];   // unused
    // uint32_t num_edges = meta.n_edges_gt[topo_idx];     // unused
    // uint32_t num_routers = meta.n_routers_gt[topo_idx]; // unused

    double app_start_time = 22000; //21200; //0; // in microseconds; use delay for BS apps
    // It takes 380 seconds to send 20 probes to all 19 dest pairs.
    // 0.1 is the amount of time in seconds to send 1 data packet to a leaf node.
    double stop_buffer_time = 100; // in milliseconds
    double sim_stop_time = stop_buffer_time + (app_start_time/1000.0)
                            + std::max((int) (0.1*meta.max_num_pkts_per_dest),
                                (int) (meta.max_num_probes_per_pair))*1000.0; // in milliseconds
    // sim_stop_time = 2000; // ms, REMOVE AFTER DEBUGGING
    //std::cout << "Simulation stop time (ms): " << sim_stop_time << std::endl; // for debugging
    Time time_stop_simulation = MilliSeconds(sim_stop_time);

    /**
     * Tree Nodes
     *
     */
    NodeContainer treeNodes;
    treeNodes.Create(num_nodes);

    /**
     * Host Node
     */
    // Every tree has one host node, (node 0)
    NodeContainer hostNode;
    hostNode.Add(treeNodes.Get(meta.host_idx));

    /**
     * UE Nodes (a.k.a. leaf nodes or destination nodes)
     */
    // uint32_t num_leaves = meta.n_leaves_gt[topo_idx];
    NodeContainer ueNodes;

    /**
     * Router Nodes
     */
    // uint32_t num_routers = meta.n_routers_gt[topo_idx];
    NodeContainer routerNodes;
    
    // Besides the host nodes, sort the rest of the
    // tree nodes into ueNodes and routerNodes.
    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        Ptr<Node> treeNode = treeNodes.Get(node_idx);
        if (meta.is_leaf_node(topo_idx, node_idx))
        {   // node is a leaf node
            // if (topo_idx == 0) // print leaf node indices for debugging. Works properly
            // {
            //     std::cout << "leaf node: " << node_idx << std::endl;
            // }
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


    // installApps(vec_app, treeNodes, meta, app_start_time, sim_stop_time);
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
    host->SetStopTime(MilliSeconds(sim_stop_time));
    treeNodes.Get(meta.host_idx)->AddApplication(host);
    vec_app[meta.host_idx] = host;

    // Install applications for leaf and router nodes.
    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        if (meta.is_leaf_node(topo_idx, node_idx))
        {   // Install application for leaf node.
            Ptr<ueApp> ue = ue_fact.Create<ueApp>();
            ue->SetNode(treeNodes.Get(node_idx));
            ue->InitUeApp(&meta, node_idx, topo_idx, *vec_app[meta.host_idx]);
            ue->SetStartTime(MicroSeconds(app_start_time));
            ue->SetStopTime(MilliSeconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(ue);
            vec_app[node_idx] = ue;
            //ue->SetRecvSocket(); // potential ERROR you may get here: assert failed. cond="socketFactory"
            //^^Potential error: failed to bind UE's receive socket
        }
        else // router node
        {   // Install application for router node.
            Ptr<overlayApplication> router = oa_fact.Create<overlayApplication>();
            router->SetNode(treeNodes.Get(node_idx));
            router->InitApp(&meta, node_idx, topo_idx);
            router->SetStartTime(MicroSeconds(app_start_time));
            router->SetStopTime(MilliSeconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(router);
            vec_app[node_idx] = router;
        }
    }

    //std::cout << "Host idx: " << meta.host_idx << std::endl; // for debugging, meta.host_idx = 0

    /**
     * Install Background Traffic Applications (UNCOMMENT AFTER DEBUGGING)
    */
    // std::vector<Ptr<OnOffApplication>> vec_onoff_apps(num_nodes*2);
    // int vec_onoff_i = 0;
    // //ApplicationContainer onOffAppCont; // use either this container or vec_onoff_apps
    // std::map<uint32_t, std::vector<uint32_t>> neighbors_map = meta.neighbors_maps_gt[topo_idx];
    // std::map<std::pair<uint32_t, uint32_t>, double> edge_bkgrd_traffic_rates
    //                                             = meta.edge_bkgrd_traffic_rates_gt[topo_idx];
    // // Set up an on-off application at the sender node
    // // to send background traffic between the links.
    // // POTENTIAL ERROR WITH FOR LOOP.
    // for (uint32_t node_idx = 0; node_idx < num_nodes; node_idx++)
    // {
    //     std::vector<uint32_t> neighbors = neighbors_map[node_idx];
    //     Ptr<Node> node = treeNodes.Get(node_idx);
    //     // Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>(); // not used
    //     // uint32_t n_devices = node->GetNDevices();
    //     // Ipv4InterfaceAddress iaddr = ipv4->GetAddress(n_devices - 1,0);
    //     // Ipv4Address ipAddr = iaddr.GetLocal(); // not used
    //     for (uint32_t neighbor_idx : neighbors)
    //     {
    //         Ptr<Node> neighbor_node = treeNodes.Get(neighbor_idx);
    //         Ptr<Ipv4> neighbor_ipv4 = neighbor_node->GetObject<Ipv4>();
    //         uint32_t neighbor_node_n_devices = neighbor_node->GetNDevices();
    //         Ipv4InterfaceAddress neighbor_iaddr = neighbor_ipv4->GetAddress(neighbor_node_n_devices - 1,0);
    //         Ipv4Address neighbor_ip_addr = neighbor_iaddr.GetLocal();
    //         auto src_dest_pair = std::pair<uint32_t, uint32_t>(node_idx, neighbor_idx);
    //         double bkgrd_rate = edge_bkgrd_traffic_rates[src_dest_pair];
    //         uint64_t kbps = pktsPerMsToKbps(bkgrd_rate);
    //         std::string kbps_str = std::to_string(kbps) + "kbps";
    //         //OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address()); // probably incorrect
    //         //OnOffHelper onOffHelper("ns3::UdpSocketFactory", ipAddr); // may not be needed
    //         // OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ipAddr, LISTENPORT)); // could be wrong
    //         OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(neighbor_ip_addr, LISTENPORT));
    //         Ptr<RandomVariableStream> onPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for on
    //         onPareto->SetAttribute("Scale", DoubleValue(5.0)); // set the scale parameter of the Pareto distribution
    //         onPareto->SetAttribute("Shape", DoubleValue(1.5)); // set the shape parameter of the Pareto distribution
    //         // When the scale parameter is equal to 1, the distribution starts at (1, shapeValue).
    //         // The scale parameter for the on period typically represents the minimum burst length.
    //         onOffHelper.SetAttribute("OnTime", PointerValue(onPareto));
    //         // onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=5]"));
    //         // onTime is the amount of time in seconds the on/off application send packets before turning off again.
    //         Ptr<RandomVariableStream> offPareto = CreateObject<ParetoRandomVariable>(); // Pareto distribution for off
    //         offPareto->SetAttribute("Scale", DoubleValue(1.0)); // set the scale parameter of the Pareto distribution
    //         offPareto->SetAttribute("Shape", DoubleValue(1.0)); // set the shape parameter of the Pareto distribution
    //         onOffHelper.SetAttribute("OffTime", PointerValue(offPareto));
    //         // The scale parameter for the off period typically represents the minimum off length.
    //         // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    //         // offTime is the amount of time in seconds the on/off app stays off before turning on again.
    //         //std::cout << "Data rate: " << kbps_str << std::endl; // for debugging.
    //         onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(kbps_str)));
    //         onOffHelper.SetAttribute("PacketSize", UintegerValue(meta.pkt_size));
    //         // AddressValue remoteAddress(InetSocketAddress(neighbor_ip_addr, LISTENPORT)); // prolly not needed
    //         // onOffHelper.SetAttribute("Remote", remoteAddress);
    //         ApplicationContainer tmpAppContainer = onOffHelper.Install(node);
    //         //onOffAppCont.Add(tmpAppContainer);
            
    //         // The first app in the temporary container is the onoff app.
    //         Ptr<OnOffApplication> onOffApp = DynamicCast<OnOffApplication>(tmpAppContainer.Get(0));
    //         onOffApp->SetStartTime(MicroSeconds(app_start_time));
    //         onOffApp->SetStopTime(MicroSeconds(sim_stop_time));
    //         vec_onoff_apps[vec_onoff_i] = onOffApp;
    //         treeNodes.Get(node_idx)->AddApplication(vec_onoff_apps[vec_onoff_i++]);
    //     }
    // }
    //onOffAppCont.Start(MicroSeconds(app_start_time));
    //onOffAppCont.Stop(MicroSeconds(sim_stop_time));
    std::cout << "Installed applications on network nodes..." << std::endl;

    /**
     *  Install the p2p links btwn the tree nodes and sockets on the UEs.
    */
    // installLinksAndSockets(vec_app, address, treeNodes, meta, netDeviceContainer);
    std::vector<std::pair<uint32_t, uint32_t>> neighbors_vec = meta.neighbors_vectors_gt[meta.topo_idx];
    NetDeviceContainer netDeviceContainer;
    std::vector<Ptr<Ipv4>> linkIpv4(2);
    std::vector<Ipv4InterfaceAddress> linkIpv4Addr(2);
    std::vector<uint32_t> n_devices_perNode(2);
    std::vector<Ptr<Node>> endnodes(2);

    PointToPointHelper link;
    link.DisableFlowControl();
    link.SetChannelAttribute("Delay", StringValue(std::to_string(meta.prop_delay) + "us")); // no prop delay for now.
    link.SetDeviceAttribute("DataRate", StringValue(std::to_string(meta.link_capacity) + "Gbps")); // link capacity: 10 Gbps

    // Ipv4Address hostIPAddr;
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
        

        for (int i = 0; i < 2; i++)
        {
            linkIpv4[i] = endnodes[i]->GetObject<Ipv4>();
            n_devices_perNode[i] = endnodes[i]->GetNDevices();
            linkIpv4Addr[i] = linkIpv4[i]->GetAddress(n_devices_perNode[i] - 1, 0); // IPv4 interfaces (device ID) are 0-indexed.
            // In GetAddress(), the 2nd argument is 0 bcuz we want to configure the 1st address of that interface.
            // An interface can have multiple IP addresses, but this is not necessary for regular use cases.
            // So unless you're using multiple IP addresses per interface, keep it 0.
        }

        // Store the IP address of the host in a variable.
        // if (srcdest.first == meta.host_idx)
        // {
        //     hostIPAddr = linkIpv4Addr[0].GetAddress();
        // }
        // else if (srcdest.second == meta.host_idx)
        // {
        //     hostIPAddr = linkIpv4Addr[1].GetAddress();
        // }

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
        

        // Install receiver socket ONLY on the UEs and sender socker ONLY on the host.
        if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.first))
        {   // First node is the UE node.
            // std::cout << "First leaf " << srcdest.first << std::endl; // for debugging
            // The if block never seems to run, but keep it just in case.
            Ptr<ueApp> first_ue_app = DynamicCast<ueApp>(first_vec_app);
            first_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.first, (uint32_t) (n_devices_perNode[0] - 1));
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.first); //send_sockets[srcdest.first]
            // vec_app[meta.host_idx]->send_sockets[srcdest.first]
            //     = first_ue_app->SetSocket(hostIPAddr, (uint32_t) meta.host_idx, (uint32_t) (n_devices_perNode[0] - 1));
        }
        else if (meta.is_leaf_node((int) meta.topo_idx, (int) srcdest.second))
        {   // Second node is the UE node.
            // std::cout << "Second leaf " << srcdest.second << std::endl; // for debugging
            Ptr<ueApp> second_ue_app = DynamicCast<ueApp>(second_vec_app);
            second_ue_app->SetRecvSocket(ueIPAddr, (uint32_t) srcdest.second, (uint32_t) (n_devices_perNode[1] - 1));
            vec_app[meta.host_idx]->SetSendSocket(ueIPAddr, srcdest.second); //send_sockets[srcdest.second]
            // vec_app[meta.host_idx]->send_sockets[srcdest.second]
            //     = second_ue_app->SetSocket(hostIPAddr, (uint32_t) meta.host_idx, (uint32_t) (n_devices_perNode[1] - 1));
            // For debugging:
            // std::cout << "Host idx: " << meta.host_idx << ", Dest node: " << srcdest.first;
            // std::cout << "UE socket: " << vec_app[meta.host_idx]->send_sockets[srcdest.first] << std::endl;
        }
    } // end neighbors_vec for loop
    std::cout << "Installed p2p links and UE sockets..." << std::endl;
    //^^Potential error: assert failed. cond="m_ptr", msg="Attempted to dereference zero pointer"

    // Set up sockets from the source node (node 0) to all the dest nodes.
    // MAY NO LONGER BE NEEDED:
    //connectHostToDestNodes(vec_app, address, treeNodes, meta);

    // Populate the routing table.
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    std::cout << "Populated routing table..." << std::endl;

    // installApps(vec_app, treeNodes, meta, app_start_time);

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

    } // end of topology for loop


    // UNCOMMENT AFTER FINISHING DEBUGGING:
    /* Write pkt and probe stats of all the topologies to .csv files. */
    // std::string pkt_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/passive_measurements/";
    // std::string probe_delays_path = "/home/akash/ns-allinone-3.36.1/ns-3.36.1/scratch/active_passive/active_measurements/";
    // meta.write_pkt_delays(pkt_delays_path);
    // meta.write_probe_delays(probe_delays_path);
    // std::cout << "Finished writing packet stats and probe stats to .csv files." << std::endl;


    return 0;
}