/**
 * Description: An NS3 application layer class used to create
 * applications for host nodes in the network.
 * Parent class: overlayApplication
*/

#ifndef HOSTAPP_H
#define HOSTAPP_H

// These are already included in overlayApplication.h
// #include "ns3/application.h"
// #include "ns3/event-id.h"
// #include "ns3/ptr.h"
// #include "ns3/ipv4-address.h"
// #include "ns3/traced-callback.h"
// #include "ns3/udp-socket.h"
// #include "ns3/log.h"

// Local header files
#include "overlayApplication.h"
#include "netmeta.h"
#include "SDtag.h"

// C++ std
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>

namespace ns3
{

class hostApp : public overlayApplication
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    hostApp();
    virtual ~hostApp();

    void InitHostApp(netmeta *netw, uint32_t localId, int topoIdx);

    /** Packet Sending **/
    void SendPacket(Time send_interval_dt, uint8_t destIdx);
    void SchedulePackets(Time init_start_dt);
    /** Probing **/
    void SendProbe(Time send_interval_dt, uint8_t destIdx1, uint8_t destIdx2);
    void ScheduleProbes(Time init_start_dt);
private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    std::map<uint32_t, uint32_t> num_pkts_sent_per_dest; // number of packets already sent (successfully received or not)
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> num_probes_sent_per_pair; // pairs consist of leaf nodes


    std::map<uint32_t, EventId> pkt_event;
    std::map<std::pair<uint32_t, uint32_t>, EventId> probe_event;

    /** Connection **/
    //void SetSocket(Address ip, uint32_t idx, uint32_t deviceID);
    //Ptr<Socket> recv_socket;  // for ueApp.h
    //std::unordered_map<uint32_t, uint32_t> map_neighbor_device; // <idx_neighbor, deviceID>

    /** Timing and debugging. **/
    std::chrono::steady_clock::time_point host_start_time;
    uint32_t probe_print_freq = 100;
    uint32_t probe_next_print_count = probe_print_freq;
    uint32_t pkt_print_freq = probe_print_freq;
    uint32_t pkt_next_print_count = pkt_print_freq;
    uint8_t last_leaf_idx;
    uint8_t second_last_leaf_idx;
};

}

#endif