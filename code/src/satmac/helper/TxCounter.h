// TxCounter.h
#ifndef TX_COUNTER_H
#define TX_COUNTER_H

#include "ns3/core-module.h"

namespace ns3 {

class TxCounter
{
public:
    // 获取单例实例
    static TxCounter& GetInstance();

    // 增加数据包计数
    void Increment_periodic();
    void Increment_aperiodic();

    // 获取当前计数
    uint32_t GetPeriodicCount() const;
    uint32_t GetAperiodicCount() const;

    // 打印当前计数
    void PrintCount() const;

private:
    // 私有构造函数
    TxCounter();

    // 禁止拷贝构造和赋值操作
    TxCounter(const TxCounter&) = delete;
    TxCounter& operator=(const TxCounter&) = delete;

    // 数据包计数
    uint32_t m_periodic_count;
    uint32_t m_aperiodic_count;
};

} // namespace ns3

#endif // TX_COUNTER_H
