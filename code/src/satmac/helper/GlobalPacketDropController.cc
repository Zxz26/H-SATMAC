// GlobalPacketDropController.cc
#include "GlobalPacketDropController.h"
#include "ns3/node.h"
#include "ns3/bsm-timetag.h"
#include "ns3/wifi-net-device.h"
#include "ns3/tdma-satmac.h"
#include "ns3/ocb-wifi-mac.h"

NS_LOG_COMPONENT_DEFINE("GlobalPacketDropController");

uint32_t GlobalPacketDropController::dropCount = 0;

GlobalPacketDropController& GlobalPacketDropController::GetInstance() {
    static GlobalPacketDropController instance;
    return instance;
}

GlobalPacketDropController::GlobalPacketDropController() : currentSlotStart(Simulator::Now()), slotDuration(MilliSeconds(1))
{}

bool GlobalPacketDropController::CheckIsNeedDrop(Time txTime, Ptr<YansWifiPhy> sender, std::string deviceType,  Ptr<const Packet> pkt) {
    // Check if we have entered a new slot
    if ((txTime - currentSlotStart).GetMilliSeconds() >= slotDuration.GetMilliSeconds()) {
        CleanOldEntries(txTime);
        currentSlotStart = txTime;
    }


    
    // Register the new packet send before checking for conflicts
    registeredSends[txTime] = {sender, deviceType};

    BsmTimeTag apptag;
    // Detect conflict after registering the new packet send
    if (DetectConflict(sender->GetMobility(), deviceType))
     {
        if(pkt->PeekPacketTag(apptag))
        {
            dropCount++; // Increment the drop count if a conflict is detected
            for (const auto& entry : registeredSends) {
                Ptr<Node> node = entry.second.sender->GetDevice()->GetNode();
                Ptr<MobilityModel> mobilityModel = node->GetObject<MobilityModel>();
                Vector position = mobilityModel->GetPosition();
                //NS_LOG_UNCOND("Registered send Node: "<< entry.second.sender->GetDevice()->GetNode()->GetId() <<" - Time: " << entry.first.GetMicroSeconds() << " ms, DeviceType: " << entry.second.deviceType);
                // std::cout<<"Registered send Node: "<<node->GetId() 
                //                   <<" - Time: " << entry.first.GetMicroSeconds() << " ms, DeviceType: " << entry.second.deviceType
                //                   <<" Position: ( "<<position.x<<","<<position.y<<")"<<std::endl;
            }
            Ptr<Node> senderNode = sender->GetDevice()->GetNode(); 
            Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(senderNode->GetDevice(0));
            Ptr<OcbWifiMac> ocb = DynamicCast<OcbWifiMac>(device->GetMac ());
            Ptr<TdmaSatmac> tdmaSatmac = DynamicCast<TdmaSatmac> (ocb->GetTdmaObject());
            if (tdmaSatmac) 
            {
                tdmaSatmac->NotifyConflictDetected(txTime, senderNode);  // 将冲突时间传递给源节点的 TdmaSatmac 实例
            }
            return true; // Indicate that the packet should be dropped
        }
        //NS_LOG_UNCOND("Conflict detected at time: " << txTime.GetMilliSeconds() << " ms, DeviceType: " << deviceType);
    }

    return false; // No conflict detected, no drop required
}



bool GlobalPacketDropController::DetectConflict(Ptr<MobilityModel> senderMobility, std::string deviceType) {
    for (const auto& entry : registeredSends) {
        // 首先判断传输是否发生在当前时隙内
        Time entrySlotStart = entry.first - NanoSeconds(entry.first.GetNanoSeconds() % slotDuration.GetNanoSeconds());
        Time currentSlotStartTime = Simulator::Now() - NanoSeconds(Simulator::Now().GetNanoSeconds() % slotDuration.GetNanoSeconds());

        if (entrySlotStart != currentSlotStartTime) {
            continue;  // 如果不在同一个时隙中，直接跳过
        }

        // Check if the device type is different and the distance is within a specific range (e.g., 158 meters)
        if (entry.second.deviceType != deviceType) {
            double distance = senderMobility->GetDistanceFrom(entry.second.sender->GetMobility());
            double distanceSquare = distance * distance;
            if (distance < 158.0 && distance > 0 && deviceType == "csma") { // Assuming 158 meters as the communication range
                NS_LOG_DEBUG("Conflict detected between different device types within communication range.");
                return true; // Conflict detected
            }
        }
    }

    return false; // No conflict detected
}

void GlobalPacketDropController::HandleConflict(Ptr<WifiPhy> sender) {
    NS_LOG_DEBUG("Conflict detected, dropping packet from " << sender);
    //sender->NotifyTxDrop(Create<Packet>());
}

void GlobalPacketDropController::CleanOldEntries(Time txTime) {
    // 检查是否进入了新的时隙
    if ((txTime - currentSlotStart).GetNanoSeconds() >= slotDuration.GetNanoSeconds()) {
        // 进入了新的时隙，删除所有旧记录
        registeredSends.clear();

        // 更新当前时隙的开始时间
        currentSlotStart = txTime - NanoSeconds(txTime.GetNanoSeconds() % slotDuration.GetNanoSeconds());
    }
}

uint32_t GlobalPacketDropController::GetDropCount() {
    return dropCount;
}
