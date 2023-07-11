#ifndef OVERLAY_APPLICATION_H
#define OVERLAY_APPLICATION_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include "ns3/udp-socket.h"
#include "ns3/log.h"
#include "SDtag.h"
#include <vector>
#include "netmeta.h"

namespace ns3
{

class Socket; // May need in the future to prevent an error.
class Packet;

class overlayApplication : public Application
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    /** Init **/
    overlayApplication();
    virtual ~overlayApplication();
    void InitApp(netmeta *netw, uint32_t localId, int topoIdx); //, uint32_t MaxPktSize);
    void SetLocalID(uint32_t localID);
    uint8_t GetLocalID(void) const;
    void SetTopoIdx(int topoIdx);
    int getTopoIdx(void) const;

    /** Connection **/
    std::unordered_map<uint32_t, Ptr<Socket>> send_sockets; // used to send pkts to receiver node. 1 socker per receiver.
    Ptr<Socket> recv_socket; // to receive packets from other nodes.
    //void Foo(); // for debugging
    //Ptr<Socket> SetSocket(Address ip, uint32_t idx, uint32_t deviceID);
    //void SetRecvSocket(void);
    void SetSendSocket(Address remoteAddr, uint32_t destIdx);
    
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
        uint32_t PktID = 0, uint8_t IsProbe = 0, uint32_t ProbeID = 0);

    /** Background traffic generation **/
    // void SendBackground(uint32_t idx);
    // void ScheduleBackground(Time dt, uint32_t idx);
    // Time bckgrd_interval;
    // std::vector<EventId> bckgrd_event;

    /** Connection **/
    uint16_t m_peerPort = 9;
    uint16_t ListenPort = 9;
    //std::unordered_map<uint32_t, Ptr<Socket>> send_sockets;
    // Ptr<Socket> recv_socket;
    
    /** Basic Meta **/
    int topo_idx;
    uint8_t m_local_ID;    // ID of current node
    uint32_t pktID = 1;    // starting packet ID
    uint32_t probeID = 1;  // starting probe ID
private:
};

}

#endif