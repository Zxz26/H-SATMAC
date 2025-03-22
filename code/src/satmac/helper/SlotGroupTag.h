// SlotGroupTag.h
#ifndef SLOT_GROUP_TAG_H
#define SLOT_GROUP_TAG_H

#include <ns3/tag.h>
#include <ns3/uinteger.h>
#include <ns3/vector.h>
#include <ns3/buffer.h>
#include <memory>
#include <array>
#include "ns3/satmac-common.h"

namespace ns3 {
class SlotGroupTag : public Tag {
public:
    SlotGroupTag();

    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;

    virtual void Serialize(TagBuffer i) const;
    virtual void Deserialize(TagBuffer i);
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Print(std::ostream &os) const;

    void SetGlobalSti(int globalSti);
    int GetGlobalSti(void) const;

    void SetFrameLen(int frameLen);
    int GetFrameLen(void) const;

    void Set_MSlotGroup(int slotgroup);
    int Get_MSlotGroup(void) const;

    void SetSlotGroupInfo(const std::array<slot_group_info, SLOT_GROUP_LENGTH>& sgInfo);
    const std::array<slot_group_info, SLOT_GROUP_LENGTH>& GetSlotGroupInfo() const;

private:
    std::array<slot_group_info, SLOT_GROUP_LENGTH> m_sgInfo;
    static constexpr size_t SG_INFO_SIZE = 128;  // 固定大小
    int m_globalSti;
    int m_frameLen;
    int m_slotgroup;
};

} // namespace ns3

#endif // SLOT_GROUP_TAG_H
