
#include "ns3/application.h"
#include "ns3/applications-module.h"
#include "ns3/onoff-application.h"
#include "ns3/on-off-helper.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
//#include "ns3/nr-mac-scheduler-tdma-rr.h" // no need for 5G
//#include "ns3/nr-module.h"                // no need for 5G
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/traffic-control-layer.h"
//#include <ns3/antenna-module.h>        // no need for wireless
#include "netmeta.h" // meta class
#include "overlayApplication.h"
#include "hostApp.h"
#include "ueApp.h"
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

int pktsPerMsToKbps(double pktsPerMs)
{
    int kbps = (int) (pktsPerMs * 1.2e10); // pktsPerMs * 1000 * 1500 * 8 * 1000
    return kbps;
}

void installLinks(std::vector<Ptr<overlayApplication>> &vec_app,
    std::vector<std::pair<uint32_t, uint32_t>> &neighbors_vec, Ipv4AddressHelper &address, NodeContainer &nc)
{
    /**
     * Used to install the overlay applications on the underlay nodes.
    */    
    NetDeviceContainer NetDevice;
    std::vector<Ptr<Ipv4>> linkIpv4(2);
    std::vector<Ipv4InterfaceAddress> linkIpv4Addr(2);
    std::vector<uint32_t> n_devices_perNode(2);
    std::vector<Ptr<Node>> endnodes(2);

    PointToPointHelper link;
    link.DisableFlowControl();
    link.SetChannelAttribute("Delay", StringValue(std::to_string(0) + "us")); // no transmission delay for now.
    link.SetDeviceAttribute("DataRate", StringValue(std::to_string(10) + "Gbps")); // link capacity: 10 Gbps

    for (auto const& srcdest : neighbors_vec)
    {
        endnodes[0] = nc.Get(srcdest.first);
        endnodes[1] = nc.Get(srcdest.second);
        
        // Create a link between 2 nodes in the underlay.
        NetDevice = link.Install(endnodes[0], endnodes[1]);
        address.Assign(NetDevice);
        address.NewNetwork();

        for (int i = 0; i < 2; i++)
        {
            linkIpv4[i] = endnodes[i]->GetObject<Ipv4>();
            n_devices_perNode[i] = endnodes[i]->GetNDevices();
            linkIpv4Addr[i] = linkIpv4[i]->GetAddress(n_devices_perNode[i] - 1, 0); // IPv4 interfaces (device ID) are 0-indexed.
            // In GetAddress(), the 2nd argument is 0 bcuz we want to configure the 1st address of that interface.
            // An interface can have multiple IP addresses, but this is not necessary for regular cases.
            // So unless you're using multiple IP addresses per interface, keep it 0.
        }

        // Set 2 sockets: 1 from source node to destination node and the other from destination to source.
        //vec_app[srcdest.first]->Foo(); // for debugging
        vec_app[srcdest.first]->SetSocket(linkIpv4Addr[1].GetAddress(),
            ((uint32_t) srcdest.second), ((uint32_t) (n_devices_perNode[0] - 1)));
        vec_app[srcdest.second]->SetSocket(linkIpv4Addr[0].GetAddress(),
            ((uint32_t) srcdest.first), ((uint32_t) (n_devices_perNode[1] - 1)));

        // For debugging:
        /* for (int k = 0; k < 2; k++)
        {
            std::cout << "Node ID = " << NetDevices[i].Get(k)->GetNode()->GetId() << "; ";
            linkIpv4[k] = NetDevices[i].Get(k)->GetNode()->GetObject<Ipv4> ();
            n_devices_perNode[k] = NetDevices[i].Get(k)->GetNode()->GetNDevices();
            linkIpv4Addr[k] = linkIpv4[k]->GetAddress( n_devices_perNode[k]-1, 0 );
            std::cout << "Address = " << linkIpv4Addr[k].GetLocal() << std::endl;
            for (uint32_t l = 0; l < n_devices_perNode[k]; l++) NS_LOG_INFO( "device ID: " << l << " with address: " << linkIpv4[k]->GetAddress( l, 0 ).GetLocal() );
        }
        */
    }
}

void installHostSockets(std::vector<Ptr<overlayApplication>> &vec_app,
    Ipv4AddressHelper &address, NodeContainer &treeNodes, netmeta &meta, int topo_idx)
{
    uint32_t num_nodes = meta.n_nodes_gt[topo_idx];

    Ptr<Node> hostNode = treeNodes.Get(0);
    // Ptr<Ipv4> ipv4 = hostNode->GetObject<Ipv4>();
    // Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0);
    // Ipv4Address hostIPAddr = iaddr.GetLocal(); // might be the wrong ipv4 addr.

    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        Ptr<Node> treeNode = treeNodes.Get(node_idx);
        if (meta.is_leaf_node(topo_idx, (int) node_idx))
        {
            Ptr<Node> destNode = treeNodes.Get(node_idx);
            Ptr<Ipv4> destipv4 = destNode->GetObject<Ipv4>();
            Ipv4InterfaceAddress destiaddr = destipv4->GetAddress(1,0);
            Ipv4Address destIPAddr = destiaddr.GetLocal();
            vec_app[0]->SetSocket(destIPAddr, (uint32_t) node_idx, (uint32_t) -1);
        }
    }
}

int
main(int argc, char* argv[])
{
    uint32_t num_topos = 1; // 20
    for (uint32_t topo_idx = 0; topo_idx < num_topos; topo_idx++)
    {

    bool logging = false;
    if (logging)
    {
        LogComponentEnable("netw", LOG_LEVEL_INFO);
    }

    netmeta meta = netmeta(); // contains network meta info
    num_topos = meta.n_topos; // 20

    uint32_t num_nodes = meta.n_nodes_gt[topo_idx];
    uint32_t num_leaves = meta.n_leaves_gt[topo_idx];
    // uint32_t num_edges = meta.n_edges_gt[topo_idx];     // unused
    // uint32_t num_routers = meta.n_routers_gt[topo_idx]; // unused

    double app_start_time = 0; // 21200; // in microseconds; use delay for BS apps
    // It takes 380 seconds to send 20 probes to all 19 dest pairs.
    // 0.1 is the amount of time in seconds to send 1 data packet to a leaf node.
    double sim_stop_time = (app_start_time/1000.0)
        + std::max((int) 0.1*meta.max_num_pkts_per_dest,
                    ((int) num_leaves-1)*meta.max_num_probes_per_pair)*1000.0; // in milliseconds

    /**
     * Tree Nodes
     *
     */
    NodeContainer treeNodes;
    treeNodes.Create(num_nodes);

    /**
     * Host Node
     */
    // Every tree has one host node, node 0.
    NodeContainer hostNode;
    hostNode.Add(treeNodes.Get(0));

    /**
     * UE Nodes (a.k.a. leaf nodes or destination nodes)
     */
    // uint32_t num_leaves = meta.n_leaves_gt[0];
    NodeContainer ueNodes;

    /**
     * Router Nodes
     */
    // uint32_t num_routers = meta.n_routers_gt[0];
    NodeContainer routerNodes;
    
    // Besides the host nodes, sort the rest of the
    // tree nodes into ueNodes and routerNodes.
    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        Ptr<Node> treeNode = treeNodes.Get(node_idx);
        if (meta.is_leaf_node(topo_idx, node_idx))
        {
            ueNodes.Add(treeNode);
        }
        else
        {
            routerNodes.Add(treeNode);
        }
    }
    
    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Add IP/TCP/UCP functionality to the tree nodes.
    InternetStackHelper internet;
    internet.Install(treeNodes);

    /**
     * Install Applications
     **/
    std::vector<Ptr<overlayApplication>> vec_app(num_nodes);
    ObjectFactory fact;
    fact.SetTypeId("ns3::overlayApplication");
    fact.Set("RemotePort", UintegerValue(LISTENPORT));
    fact.Set("ListenPort", UintegerValue(LISTENPORT));

    Ptr<hostApp> host = fact.Create<hostApp>();
    host->InitApp(&meta, 0, topo_idx);
    host->SetStartTime(MicroSeconds(app_start_time));
    // vec_app[0i]->SetStopTime(MilliSeconds(sim_stop_time));
    treeNodes.Get(0)->AddApplication(host);
    vec_app[0] = host;

    for (uint32_t node_idx = 1; node_idx < num_nodes; node_idx++)
    {
        if (meta.is_leaf_node(topo_idx, node_idx))
        {
            Ptr<ueApp> ue = fact.Create<ueApp>();
            ue->InitUeApp(&meta, node_idx, topo_idx, *vec_app[0]);
            ue->SetStartTime(MicroSeconds(app_start_time));
            // app->SetStopTime(MilliSeconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(ue);
            ue->SetRecvSocket(); // potential ERROR you may get here: assert failed. cond="socketFactory"
            vec_app[node_idx] = ue;
        }
        else // router node
        {
            Ptr<overlayApplication> router = fact.Create<overlayApplication>();
            router->InitApp(&meta, node_idx, topo_idx);
            router->SetStartTime(MicroSeconds(app_start_time));
            // router->SetStopTime(MilliSeconds(sim_stop_time));
            treeNodes.Get(node_idx)->AddApplication(router);
            vec_app[node_idx] = router;
        }
    }

    /**
     * Install Background Traffic Applications
    */
    //std::vector<Ptr<OnOffApplication>> vec_onoff_apps;
    std::vector<ApplicationContainer> vec_onoff_apps;
    std::map<uint32_t, std::vector<uint32_t>> neighbors_map = meta.neighbors_maps_gt[topo_idx];
    std::map<std::pair<uint32_t, uint32_t>, double> edge_bkgrd_traffic_rates
                                                = meta.edge_bkgrd_traffic_rates_gt[topo_idx];
    // Set up an on-off application at the sender node
    // to send background traffic between the links.
    for (uint32_t node_idx = 0; node_idx < num_nodes; node_idx++)
    {
        std::vector<uint32_t> neighbors = neighbors_map[node_idx];
        Ptr<Node> node = treeNodes.Get(node_idx);
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        Ipv4InterfaceAddress iaddr = ipv4->GetAddress(1,0);
        Ipv4Address ipAddr = iaddr.GetLocal();
        for (uint32_t neighbor_idx : neighbors)
        {
            auto src_dest_pair = std::pair<uint32_t, uint32_t>(node_idx, neighbor_idx);
            double bkgrd_rate = edge_bkgrd_traffic_rates[src_dest_pair];
            int kbps = pktsPerMsToKbps(bkgrd_rate);
            std::string kbps_str = std::to_string(kbps) + "kbps";
            OnOffHelper onoffHelper("ns3::TcpSocketFactory", Address());
            onoffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            onoffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            onoffHelper.SetAttribute("StartTime", StringValue("ns3::ParetoRandomVariable[Shape=1.5]"));
            onoffHelper.SetAttribute("DataRate", DataRateValue(DataRate(kbps_str)));
            onoffHelper.SetAttribute("PacketSize", UintegerValue(meta.pkt_size));
            AddressValue remoteAddress(InetSocketAddress(ipAddr, LISTENPORT));
            onoffHelper.SetAttribute("Remote", remoteAddress);
            ApplicationContainer onoffAppCont = onoffHelper.Install(treeNodes.Get(node_idx));
            onoffAppCont.Start(MicroSeconds(app_start_time));
            // The first app in the container is the onoff app.
            // Ptr<OnOffApplication> onoffApp = onoffAppCont.Get(0);
            // onoffApp->SetStartTime(MicroSeconds(app_start_time));
            //vec_onoff_apps.push_back(onoffApp);
            vec_onoff_apps.push_back(onoffAppCont);
        }
    }

    // Install the p2p links btwn the tree nodes.
    installLinks(vec_app, meta.neighbors_vectors_gt[topo_idx], address, treeNodes);

    // Set up sockets from the source node (node 0) to all the dest nodes.
    installHostSockets(vec_app, address, treeNodes, meta, topo_idx);


    /**
     * Run Simulation
    */
    NS_LOG_INFO("Run Simulation.");
    std::cout << "Before run" << std::endl;
    Time time_stop_simulation = MilliSeconds(sim_stop_time);
    // Simulator::Schedule(time_stop_simulation, stop_NR, vec_NrHelper);
    Simulator::Stop(time_stop_simulation);
    Simulator::Run();
    std::cout << "After run" << std::endl;

    // Write pkt and probe stats of all the topologies to .csv files.
    std::string pkt_delays_path = "./passive_measurements/";
    std::string probe_delays_path = "./active_measurements/";
    meta.write_pkt_delays(pkt_delays_path);
    meta.write_probe_delays(probe_delays_path);

    } // end of topology for loop

    return 0;
}