#include "SDtag.h"

#include "ns3/internet-module.h"
#include "ns3/network-module.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SDtag");
NS_OBJECT_ENSURE_REGISTERED(SDtag);

TypeId SDtag::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SDtag")
                            .SetParent<Tag>()
                            .AddConstructor<SDtag>()
                            .AddAttribute("SourceID",
                                          "ID of the Source",
                                          EmptyAttributeValue(),
                                          MakeUintegerAccessor(&SDtag::GetSourceID),
                                          MakeUintegerChecker<uint8_t>())
                            .AddAttribute("DestID",
                                          "ID of the Destination",
                                          EmptyAttributeValue(),
                                          MakeUintegerAccessor(&SDtag::GetDestID),
                                          MakeUintegerChecker<uint8_t>())
                            .AddAttribute("PktID",
                                          "ID of the Packet",
                                          EmptyAttributeValue(),
                                          MakeUintegerAccessor(&SDtag::GetPktID),
                                          MakeUintegerChecker<uint32_t>())
                            .AddAttribute("ProbeID",
                                          "ID of the Probe",
                                          EmptyAttributeValue(),
                                          MakeUintegerAccessor(&SDtag::GetProbeID),
                                          MakeUintegerChecker<uint32_t>());
    return tid;
}

TypeId SDtag::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

uint32_t SDtag::GetSerializedSize(void) const
{
    return 20;
}

void SDtag::Serialize(TagBuffer i) const
{   // Serialize and deserialize must be in same order.
    i.WriteU8(SourceID);
    i.WriteU8(DestID);
    i.WriteU8(IsBckgrd);
    i.WriteU8(IsProbe);
    i.WriteU32(ProbeID);
    i.WriteU32(PktID);
    i.WriteU64(StartTime);
    //i.WriteU8(ueID);
    //i.WriteU8(currentHop);
    //i.WriteU8(IsQueued);
    //i.WriteU8(bwpID);
}

void SDtag::Deserialize(TagBuffer i)
{
    SourceID = i.ReadU8();
    DestID = i.ReadU8();
    IsBckgrd = i.ReadU8();
    IsProbe = i.ReadU8();
    ProbeID = i.ReadU32();
    PktID = i.ReadU32();
    StartTime = i.ReadU64();
    //ueID = i.ReadU8();
    //currentHop = i.ReadU8();
    //IsQueued = i.ReadU8();
    //bwpID = i.ReadU8();
}

void SDtag::Print(std::ostream& os) const
{
    os << "source=" << (uint32_t)SourceID << ", Dest=" << (uint32_t)DestID
       << std::endl;
}

void SDtag::SetSourceID(uint8_t value)
{
    SourceID = value;
}

uint8_t SDtag::GetSourceID(void) const
{
    return SourceID;
}

void SDtag::SetDestID(uint8_t value)
{
    DestID = value;
}

uint8_t SDtag::GetDestID(void) const
{
    return DestID;
}

// void SDtag::SetUeID(uint8_t value)
// {
//     ueID = value;
// }

// uint8_t SDtag::GetUeID(void) const
// {
//     return ueID;
// }

// void SDtag::SetBwpID(uint8_t value)
// {
//     bwpID = value;
// }

// uint8_t SDtag::GetBwpID(void) const
// {
//     return bwpID;
// }

// void SDtag::SetCurrentHop(uint8_t value)
// {
//     currentHop = value;
// }

// void SDtag::AddCurrentHop(void)
// {
//     currentHop += 1;
// }

// uint8_t SDtag::GetCurrentHop(void) const
// {
//     return currentHop;
// }

void SDtag::SetIsBckgrd(uint8_t value)
{
    IsBckgrd = value;
}

uint8_t SDtag::GetIsBckgrd(void) const
{
    return IsBckgrd;
}

void SDtag::SetIsProbe(uint8_t value)
{
    IsProbe = value;
}

uint8_t SDtag::GetIsProbe(void) const
{
    return IsProbe;
}

// uint8_t SDtag::GetIsQueued(void) const
// {
//     return IsQueued;
// }

// void SDtag::SetIsQueued(uint8_t value)
// {
//     IsQueued = value;
// }

void SDtag::SetProbeID(uint32_t value)
{
    ProbeID = value;
}

uint32_t SDtag::GetProbeID(void) const
{
    return ProbeID;
}

void SDtag::SetPktID(uint32_t value)
{
    PktID = value;
}

uint32_t SDtag::GetPktID(void) const
{
    return PktID;
}

void SDtag::SetStartTime(uint64_t value)
{
    StartTime = value;
}

uint64_t SDtag::GetStartTime(void) const
{
    return StartTime;
}

/** ueTag **/
// uint32_t ueTag::GetSerializedSize(void) const
// {
//     return 8;
// }

// void ueTag::Serialize(TagBuffer i) const
// {
//     i.WriteU64(StartTime);
// }

// void ueTag::Deserialize(TagBuffer i)
// {
//     StartTime = i.ReadU64();
// }

// TypeId ueTag::GetTypeId(void)
// {
//     static TypeId tid = TypeId("ns3::ueTag")
//                             .SetParent<Tag>()
//                             .AddConstructor<SDtag>()
//                             .AddAttribute("StartTime",
//                                           "Start time of the Packet",
//                                           EmptyAttributeValue(),
//                                           MakeUintegerAccessor(&SDtag::GetSourceID),
//                                           MakeUintegerChecker<uint8_t>())
//                             .AddAttribute("DestID",
//                                           "ID of the Destiny",
//                                           EmptyAttributeValue(),
//                                           MakeUintegerAccessor(&SDtag::GetDestID),
//                                           MakeUintegerChecker<uint8_t>())
//                             .AddAttribute("CurrentHop",
//                                           "Current hop index",
//                                           EmptyAttributeValue(),
//                                           MakeUintegerAccessor(&SDtag::GetCurrentHop),
//                                           MakeUintegerChecker<uint8_t>())
//                             .AddAttribute("PktID",
//                                           "ID of the Packet",
//                                           EmptyAttributeValue(),
//                                           MakeUintegerAccessor(&SDtag::GetPktID),
//                                           MakeUintegerChecker<uint32_t>());
//     return tid;
// }

// TypeId ueTag::GetInstanceTypeId(void) const
// {
//     return GetTypeId();
// }

// uint64_t ueTag::GetStartTime(void) const
// {
//     return StartTime;
// }

// void ueTag::SetStartTime(uint64_t value)
// {
//     StartTime = value;
// }

// void ueTag::Print(std::ostream& os) const
// {
//     os << "source=" << (uint64_t)StartTime << std::endl;
// }

} // namespace ns3