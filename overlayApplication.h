/**
 * Description: An NS3 application layer class used to create
 * applications for routers and various other nodes in the network.
 * Extended by: hostApp and ueApp
*/

#ifndef OVERLAY_APPLICATION_H
#define OVERLAY_APPLICATION_H

#include <vector>
#include <cmath>
#include <assert.h>

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/udp-socket.h"
#include "ns3/log.h"
#include "SDtag.h"
#include "netmeta.h"

namespace ns3
{

class Socket;
class Packet;

class overlayApplication : public Application
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    /** Init **/
    overlayApplication();
    virtual ~overlayApplication();
    void InitApp(netmeta *netw, uint32_t localId, int topoIdx);
    void SetLocalID(uint32_t localID);
    uint8_t GetLocalID(void) const;
    void SetTopoIdx(int topoIdx);
    int getTopoIdx(void) const;

    /** Connection **/
    uint16_t m_peerPort = 9; // the destination port of the outbound packets
    uint16_t ListenPort = 9; // port on which to listen for incoming packets
    std::unordered_map<uint32_t, Ptr<Socket>> send_sockets; // used to send pkts to receiver node. 1 socker per receiver.
    Ptr<Socket> recv_socket; // to receive packets from other nodes.
    void SetSendSocket(Address remoteAddr, uint32_t destIdx);
    //void SetRecvSocket(void);
    void SetRecvSocket(Address myIP, uint32_t idx, uint32_t deviceID);
    void HandleRead(Ptr<Socket> socket);
    
    /** Functions **/
    // void HandleRead(Ptr<Socket> socket);
    // bool CheckCongestion(uint32_t deviceID, uint32_t src, uint32_t dest, uint16_t PktID);

    netmeta* meta;

protected:
    virtual void DoDispose(void);
//private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    void SetTag(SDtag& tagToUse, uint8_t SourceID, uint8_t DestID,
        uint32_t PktID = 0, uint8_t IsBckgrd = 0, uint8_t IsProbe = 0, uint32_t ProbeID = 0);

    /** Background traffic generation **/
    void SendParetoBackground(uint32_t destIdx);
    void SendLogNormBackground(uint32_t destIdx);
    void Helper_Send_Background_Traffic(uint32_t destIdx, double timeLeft, double bckgrdRate);
    void ScheduleBackground(Time dt);
    // Time bckgrd_interval;
    // std::vector<EventId> bckgrd_event;
    
    /** Basic Meta **/
    int topo_idx;
    uint32_t num_nodes = 0; // number of nodes in the current topology
    uint8_t m_local_ID;     // ID of current node
    uint32_t pktID = 1;     // ID of packet to be sent
    uint32_t probeID = 1;   // ID of probe to be sent
private:
    std::map<uint32_t, EventId> bkgrd_pkt_event;
    Ptr<ParetoRandomVariable> rand_burst_pareto; // for ON duration of bkgrd traffic
    Ptr<ParetoRandomVariable> off_pareto; // for OFF duration
    Ptr<LogNormalRandomVariable> rand_log_norm_var;
    bool keep_running = false;

    int num_bkgrd_pkts_received = 0;
};

}

#endif