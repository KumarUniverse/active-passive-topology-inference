#ifndef HOSTAPP_H
#define HOSTAPP_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/udp-socket.h"
#include "ns3/log.h"
#include "overlayApplication.h"
#include "netmeta.h"
#include "SDtag.h"

namespace ns3
{

class hostApp : public overlayApplication
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    hostApp();
    virtual ~hostApp();

    void InitApp(netmeta *netw, uint32_t localId, int topoIdx);

    /** Connection **/
    //void SetSocket(Address ip, uint32_t idx, uint32_t deviceID);

    /** Functions **/
    /** Packet Sending **/
    void SendPacket(Time dt, uint32_t node_idx);
    void SchedulePackets(Time dt);
    /** Probing **/
    void SendProbe(Time dt, uint32_t node_idx1, uint32_t node_idx2);
    void ScheduleProbes(Time dt);
private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    std::map<uint32_t, uint32_t> num_pkts_sent_per_dest; // number of packets already sent (whether successfully received or not)
    std::map<uint32_t, uint32_t> num_probes_sent_per_dest;
    bool keep_sending_pkts = true;
    bool keep_sending_probes = true;

    std::map<uint32_t, EventId> pkt_event;
    std::map<std::pair<uint32_t, uint32_t>, EventId> probe_event;

    /** Connection **/
    //Ptr<Socket> recv_socket;  // for ueApp.h
    //std::unordered_map<uint32_t, uint32_t> map_neighbor_device; // <idx_neighbor, deviceID>
    uint32_t num_nodes = 0;
};

}

#endif