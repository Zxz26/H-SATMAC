// SlotGroupTag.cc
#include "SlotGroupTag.h"

namespace ns3 {

SlotGroupTag::SlotGroupTag() : m_globalSti(0), m_frameLen(0), m_slotgroup(-1) {
    for (size_t i = 0; i < SG_INFO_SIZE; ++i) {
        m_sgInfo[i].geohash = -1;
        m_sgInfo[i].t_valid = 0;
        m_sgInfo[i].sg_busy = 0;
    }
}

TypeId SlotGroupTag::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::SlotGroupTag")
        .SetParent<Tag>()
        .AddConstructor<SlotGroupTag>();
    return tid;
}

TypeId SlotGroupTag::GetInstanceTypeId(void) const {
    return GetTypeId();
}

void SlotGroupTag::Serialize(TagBuffer i) const {
    i.WriteU32(m_globalSti);
    i.WriteU32(m_frameLen);
    i.WriteU32(m_slotgroup);
    i.WriteU32(SG_INFO_SIZE); // 固定大小为 128
    for (size_t j = 0; j < SG_INFO_SIZE; ++j) {
        i.WriteU32(m_sgInfo[j].geohash);
        i.WriteU32(m_sgInfo[j].t_valid);
        i.WriteU8(m_sgInfo[j].sg_busy);
    }
}

void SlotGroupTag::Deserialize(TagBuffer i) {
    m_globalSti = i.ReadU32();
    m_frameLen = i.ReadU32();
    m_slotgroup = i.ReadU32();
    uint32_t size = i.ReadU32();
    for (size_t j = 0; j < SG_INFO_SIZE; ++j) {
        m_sgInfo[j].geohash = i.ReadU32();
        m_sgInfo[j].t_valid = i.ReadU32();
        m_sgInfo[j].sg_busy = i.ReadU8();

    }
}

uint32_t SlotGroupTag::GetSerializedSize(void) const {
    return 4 + 4 + 4 + 4 + SG_INFO_SIZE * (8 + 1); // globalSti + frameLen + size + 每个 slot_group_info 的大小
}

void SlotGroupTag::Print(std::ostream &os) const {
    os << "Global STI: " << m_globalSti << ", Frame Length: " << m_frameLen << ", Slot Group Info Size: 128";
}

void SlotGroupTag::SetGlobalSti(int globalSti) {
    m_globalSti = globalSti;
}

int SlotGroupTag::GetGlobalSti(void) const {
    return m_globalSti;
}

void SlotGroupTag::SetFrameLen(int frameLen) {
    m_frameLen = frameLen;
}

int SlotGroupTag::GetFrameLen(void) const {
    return m_frameLen;
}

void SlotGroupTag::SetSlotGroupInfo(const std::array<slot_group_info, SLOT_GROUP_LENGTH>& sgInfo) {
    m_sgInfo = sgInfo;
}

const std::array<slot_group_info, SLOT_GROUP_LENGTH>& SlotGroupTag::GetSlotGroupInfo() const {
    return m_sgInfo;
}

void SlotGroupTag::Set_MSlotGroup(int slotgroup) {
	m_slotgroup = slotgroup;
}

int SlotGroupTag::Get_MSlotGroup(void) const {
    return m_slotgroup;
}

} // namespace ns3
