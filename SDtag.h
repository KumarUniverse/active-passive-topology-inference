/**
 * Description: A custom tag class used for creating
 * informative packet tags that can be attached to
 * a packet before sending it. This provides an easier
 * alternative to having to read the packet's header
 * to obtain it's meta info.
 * "SD" stands for "Source Destination".
*/

#ifndef SD_TAG_H
#define SD_TAG_H

#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

// class ueTag : public Tag
// {
//   public:
//     virtual void Serialize(TagBuffer i) const;
//     virtual void Deserialize(TagBuffer i);
//     static TypeId GetTypeId(void);
//     virtual TypeId GetInstanceTypeId(void) const;
//     virtual uint32_t GetSerializedSize(void) const;

//     uint64_t GetStartTime(void) const;
//     void SetStartTime(uint64_t value);
//     void Print(std::ostream& os) const;

//   private:
//     uint64_t StartTime;
// };

class SDtag : public Tag
{
  public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize(TagBuffer i);
    virtual void Print(std::ostream& os) const;

    void SetSourceID(uint8_t value);
    uint8_t GetSourceID(void) const;
    void SetDestID(uint8_t value);
    uint8_t GetDestID(void) const;
    //void SetUeID(uint8_t value);
    //uint8_t GetUeID(void) const;
    //void SetBwpID(uint8_t value);
    //uint8_t GetBwpID(void) const;
    // void SetCurrentHop(uint8_t value);
    // void AddCurrentHop(void);
    // uint8_t GetCurrentHop(void) const;
    void SetIsBckgrd(uint8_t value);
    uint8_t GetIsBckgrd(void) const;
    uint8_t GetIsProbe(void) const;
    void SetIsProbe(uint8_t value);
    void SetProbeID(uint32_t value);
    uint32_t GetProbeID(void) const;
    //uint8_t GetIsQueued(void) const;
    // void SetIsQueued(uint8_t value);
    void SetPktID(uint32_t value);
    uint32_t GetPktID(void) const;
    void SetStartTime(uint64_t value);
    uint64_t GetStartTime(void) const;

  private:
    uint8_t SourceID;
    uint8_t DestID;
    uint8_t IsBckgrd;
    uint8_t IsProbe;
    uint32_t ProbeID;
    uint32_t PktID;
    uint64_t StartTime;
    //uint8_t ueID = 0;
    //uint8_t currentHop;
    //uint8_t IsQueued;
    //uint8_t bwpID = 1;
    // Total number of bytes required for tag = 1*4 + 4*2 + 8*1 = 4 + 8 + 8 = 20
};

} // namespace ns3

#endif