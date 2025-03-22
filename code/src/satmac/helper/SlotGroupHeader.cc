// SlotGroupHeader.cc
#include "SlotGroupHeader.h"

namespace ns3 {

SlotGroupHeader::SlotGroupHeader() : m_globalSti(0), m_frameLen(0), m_slotgroup(-1) {
    for (size_t i = 0; i < SG_INFO_SIZE; ++i) {
        m_sgInfo[i].geohash = -1;
        m_sgInfo[i].t_valid = 0;
        m_sgInfo[i].sg_busy = 0;
    }
}

TypeId SlotGroupHeader::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::SlotGroupHeader")
        .SetParent<Header>()
        .AddConstructor<SlotGroupHeader>();
    return tid;
}

TypeId SlotGroupHeader::GetInstanceTypeId(void) const {
    return GetTypeId();
}

void SlotGroupHeader::Serialize(Buffer::Iterator i) const {
    for (size_t j = 0; j < SG_INFO_SIZE; ++j) 
     {
         i.WriteU32(m_sgInfo[j].geohash);
         i.WriteU32(m_sgInfo[j].t_valid);
     }
}

uint32_t SlotGroupHeader::Deserialize(Buffer::Iterator start) 
{
    Buffer::Iterator i = start;
     for (size_t j = 0; j < SG_INFO_SIZE; ++j) {
         m_sgInfo[j].geohash = i.ReadU32();
         m_sgInfo[j].t_valid = i.ReadU32();
    }

    return GetSerializedSize();
}


uint32_t SlotGroupHeader::GetSerializedSize(void) const {
    return SG_INFO_SIZE * (10 + 2 );
}

void SlotGroupHeader::Print(std::ostream &os) const {
}

void SlotGroupHeader::SetGlobalSti(int globalSti) {
    m_globalSti = globalSti;
}

int SlotGroupHeader::GetGlobalSti(void) const {
    return m_globalSti;
}

void SlotGroupHeader::SetFrameLen(int frameLen) {
    m_frameLen = frameLen;
}

int SlotGroupHeader::GetFrameLen(void) const {
    return m_frameLen;
}

void SlotGroupHeader::SetSlotGroupInfo(const std::array<slot_group_info, SLOT_GROUP_LENGTH>& sgInfo) {
    m_sgInfo = sgInfo;
}

const std::array<slot_group_info, SLOT_GROUP_LENGTH>& SlotGroupHeader::GetSlotGroupInfo() const {
    return m_sgInfo;
}

void SlotGroupHeader::Set_MSlotGroup(int slotgroup) {
    m_slotgroup = slotgroup;
}

int SlotGroupHeader::Get_MSlotGroup(void) const {
    return m_slotgroup;
}

} // namespace ns3
