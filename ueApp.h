#ifndef UEAPP_H
#define UEAPP_H

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

#define PORT 1234

namespace ns3
{

class ueApp : public overlayApplication
{
public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    ueApp();
    virtual ~ueApp();

    void InitUeApp(netmeta *netw, uint32_t localId, int topoIdx, overlayApplication &app_interface);

    /** Connection **/
    //void SetSocket(Address ip, uint32_t idx, uint32_t deviceID);
    void SetRecvSocket(void);

    /** Functions **/
    void HandleRead(Ptr<Socket> socket); // need to implement.
private:
    //Ptr<Socket> recv_socket;  // already exists
    Ptr<Socket> recv_socket_1;
    overlayApplication *oa_interface;

    virtual void StartApplication(void);
    virtual void StopApplication(void);

    //static std::map<uint32_t, std::vector<int64_t>> received_probes;
    // Pair probe packets with the same probe ID together.
};

// Definition of the static map variable outside the class definition
//std::map<uint32_t, std::vector<int64_t>> ueApp::received_probes;

}

#endif