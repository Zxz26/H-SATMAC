#include "GeohashHelper.h"
#include "ns3/mobility-module.h"
#include "ns3/log.h"

#include <cmath>
#include <bitset>
#include <iostream>

NS_LOG_COMPONENT_DEFINE("GeohashHelper");

GeohashHelper& GeohashHelper::GetInstance()
{
	//每个geohash单元长130*90。对角线为158，正好与通信距离相等
    //static GeohashHelper instance(0.0, 2080.0, 0.0, 1440.0, 4);
    //static GeohashHelper instance(0.0, 2080.0, 0.0, 1440.0, 5);
    //static GeohashHelper instance(0.0, 3120.0, 0.0, 2160.0, 5);
    //UAVs
    static GeohashHelper instance(0.0, 2000.0, 0.0, 2000.0, 5);
    return instance;
}

GeohashHelper::GeohashHelper(double minX, double maxX, double minY, double maxY, int precision)
    : m_minX(minX), m_maxX(maxX), m_minY(minY), m_maxY(maxY), m_precision(precision)
{
	m_slotMapping = GenerateSlotMapping(16);
}

int GeohashHelper::Encode(double x, double y) const
{
    // Normalize x and y to be between 0 and 1
    double normalizedX = (x - m_minX) / (m_maxX - m_minX);
    double normalizedY = (y - m_minY) / (m_maxY - m_minY);

    //NS_LOG_UNCOND("Normalized X: " << normalizedX << ", Normalized Y: " << normalizedY);

    // Convert to geohash using interleaving bits
    uint8_t geohash = 0;
    for (int i = 0; i < m_precision; ++i)
    {
        normalizedX *= 2;
        normalizedY *= 2;

        int bitX = static_cast<int>(normalizedX);
        int bitY = static_cast<int>(normalizedY);

        // Remove the integer part after calculating the bit
        normalizedX -= bitX;
        normalizedY -= bitY;

        //NS_LOG_UNCOND("Iteration " << i << ": bitX = " << bitX << ", bitY = " << bitY);

        // Interleave the bits
        geohash |= (bitX << (2 * i + 1)) | (bitY << (2 * i));
        //NS_LOG_UNCOND("Geohash after iteration " << i << ": " << geohash);
    }
    return geohash;
}

std::unordered_map<int, int> GeohashHelper::GenerateSlotMapping(int slotsPerUnit) const
{
    std::unordered_map<int, int> slotMapping;

    // 将 256 个 geohash 分成 16 个大单元，每个大单元包含 16 个子单元
    double unitWidth = 520.0;  // 2080 / 4
    double unitHeight = 360.0; // 1440 / 4
    double subUnitWidth = 130.0;
    double subUnitHeight = 90.0;

    for (int unitX = 0; unitX < 4; ++unitX) {
        for (int unitY = 0; unitY < 4; ++unitY) {
            for (int subX = 0; subX < 4; ++subX) {
                for (int subY = 0; subY < 4; ++subY) {
                    double x = unitX * unitWidth + subX * subUnitWidth + m_minX;
                    double y = unitY * unitHeight + subY * subUnitHeight + m_minY;
                    int geohashValue = Encode(x, y);

                    int slotGroup;
                    if (slotsPerUnit == 8) {
                        // 为 slotsPerUnit = 8 的情况按给定的映射逻辑进行映射
                        if (subY == 0) {
                            slotGroup = subX;
                        } else if (subY == 1) {
                            slotGroup = subX + 4;
                        } else if (subY == 2) {
                            slotGroup = (subX + 2) % 4;
                        } else { // subY == 3
                            slotGroup = (subX + 2) % 4 + 4;
                        }
                    } else if (slotsPerUnit == 32) {
                        // slotsPerUnit = 32 的情况下，大单元内的子单元映射到时隙组 16-31
                        slotGroup = 16 + (subX + subY * 4) % 16;
                    } else {
                        // slotsPerUnit = 16 的情况下，按原逻辑映射
                        slotGroup = (subX + subY * 4) % slotsPerUnit;
                    }

                    // 保存映射关系
                    slotMapping[geohashValue] = slotGroup;
                }
            }
        }
    }

    return slotMapping;
}


void GeohashHelper::PrintGeohash(Ptr<Node> node)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    if (mobility == nullptr)
    {
        NS_LOG_ERROR("Mobility model not found for node " << node->GetId());
        return;
    }

    Vector position = mobility->GetPosition();
    int geohash = Encode(position.x, position.y);
    NS_LOG_UNCOND("Node " << node->GetId() << " Position: (x: " << position.x << ", y: " << position.y << ") Geohash: " << geohash);

}

void GeohashHelper::PrintSlotMapping(int slotsPerUnit) const
{
    double unitWidth = 520.0;  // 2080 / 4
    double unitHeight = 360.0; // 1440 / 4
    double subUnitWidth = 130.0;
    double subUnitHeight = 90.0;

    for (int unitX = 0; unitX < 4; ++unitX) {
        for (int unitY = 0; unitY < 4; ++unitY) {
            for (int subX = 0; subX < 4; ++subX) {
                for (int subY = 0; subY < 4; ++subY) {
                    double x = unitX * unitWidth + subX * subUnitWidth + m_minX;
                    double y = unitY * unitHeight + subY * subUnitHeight + m_minY;
                    int geohashValue = Encode(x, y);

                    std::unordered_map<int, int> slotMapping = GenerateSlotMapping(slotsPerUnit);

                    if (slotMapping.find(geohashValue) != slotMapping.end()) {
                        int slotGroup = slotMapping.at(geohashValue);
                        std::cout << "Geohash " << geohashValue << " (Unit: " << unitX << ", " << unitY << ", SubUnit: " << subX << ", " << subY << ") is mapped to Slot Group " << slotGroup << std::endl;
                    }
                }
            }
        }
    }
}


