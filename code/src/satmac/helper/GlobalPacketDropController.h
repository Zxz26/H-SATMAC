// GlobalPacketDropController.h
#ifndef GLOBAL_PACKET_DROP_CONTROLLER_H
#define GLOBAL_PACKET_DROP_CONTROLLER_H

#include <map>
#include <ns3/simulator.h>
#include <ns3/ptr.h>
#include <ns3/wifi-phy.h>
#include <ns3/log.h>
#include "ns3/yans-wifi-phy.h"
#include "ns3/mobility-model.h"
#include "ns3/yans-wifi-phy.h"

using namespace ns3;


class GlobalPacketDropController {
public:
    static GlobalPacketDropController& GetInstance();
    bool CheckIsNeedDrop(Time txTime, Ptr<YansWifiPhy> sender, std::string deviceType, Ptr<const Packet> pkt);
    static uint32_t GetDropCount();

private:
    struct DeviceInfo {
        //Ptr<MobilityModel> senderMobility;
        Ptr<YansWifiPhy> sender;
        std::string deviceType;
    };

    std::map<Time, DeviceInfo> registeredSends;
    Time currentSlotStart = NanoSeconds(0); 
    Time slotDuration = MilliSeconds(1.0);   
    static uint32_t dropCount;

    GlobalPacketDropController();
    GlobalPacketDropController(const GlobalPacketDropController&) = delete;
    void operator=(const GlobalPacketDropController&) = delete;

    bool DetectConflict(Ptr<MobilityModel> senderMobility, std::string deviceType);
    void HandleConflict(Ptr<WifiPhy> sender);
    void CleanOldEntries(Time txTime);
};

#endif // GLOBAL_PACKET_DROP_CONTROLLER_H
