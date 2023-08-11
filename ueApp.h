/**
 * Description: An NS3 application layer class used to create
 * applications for user equipment (UE) nodes in the network.
 * Parent class: overlayApplication
*/

#ifndef UEAPP_H
#define UEAPP_H

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

namespace ns3
{

class ueApp : public overlayApplication
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    ueApp();
    virtual ~ueApp();

    void InitUeApp(netmeta *netw, uint32_t localId, int topoIdx);

    /** Connection **/
    void SetRecvSocket(Address myIP, uint32_t idx, uint32_t deviceID);

    /** Functions **/
    void HandleRead(Ptr<Socket> socket);
private:
    //Ptr<Socket> recv_socket;  // already exists in superclass
    //overlayApplication *oa_interface; // not needed

    virtual void StartApplication(void);
    virtual void StopApplication(void);

    int num_pkts_received = 0;
    int num_probes_received = 0;
    int num_bkgrd_pkts_received = 0;
    int num_other_pkts_received = 0;
    //static std::map<uint32_t, std::vector<int64_t>> received_probes;
    // Pair probe packets with the same probe ID together.
};

// Definition of the static map variable outside the class definition
//std::map<uint32_t, std::vector<int64_t>> ueApp::received_probes;

}

#endif