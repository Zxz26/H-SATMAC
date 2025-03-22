// TxCounter.cc
#include "TxCounter.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TxCounter");

TxCounter& TxCounter::GetInstance()
{
    static TxCounter instance;
    return instance;
}

TxCounter::TxCounter()
    : m_periodic_count(0),
      m_aperiodic_count(0)
{
}

void TxCounter::Increment_periodic()
{
    m_periodic_count++;
}

uint32_t TxCounter::GetPeriodicCount() const
{
    return m_periodic_count;
}

void TxCounter::Increment_aperiodic()
{
    m_aperiodic_count++;
}

uint32_t TxCounter::GetAperiodicCount() const
{
    return m_aperiodic_count;
}

void TxCounter::PrintCount() const
{
    //NS_LOG_INFO ("TxCounter: Total Transmitted Packets = " << m_count);
}

} // namespace ns3
