// GeohashHelper.h
#ifndef GEOHASH_HELPER_H
#define GEOHASH_HELPER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include <unordered_map>

using namespace ns3;

class GeohashHelper
{
public:
    // Get the singleton instance of GeohashHelper
    static GeohashHelper& GetInstance();

    int Encode(double x, double y) const;
    void PrintGeohash(Ptr<Node> node);
    void PrintSlotMapping(int slotsPerUnit) const;
    std::unordered_map<int, int> GenerateSlotMapping(int slotsPerUnit) const;
private:
    // Private constructor for singleton pattern
    GeohashHelper(double minX, double maxX, double minY, double maxY, int precision);
    //std::unordered_map<int, int> GenerateSlotMapping(int slotsPerUnit) const;
    double m_minX;
    double m_maxX;
    double m_minY;
    double m_maxY;
    int m_precision;
    std::unordered_map<int, int> m_slotMapping;
};

#endif // GEOHASH_HELPER_H
