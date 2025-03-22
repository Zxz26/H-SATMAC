// run_number.h
#ifndef RUN_NUMBER_H
#define RUN_NUMBER_H

#include "ns3/core-module.h"

namespace ns3 {

// RunNumber 类用于存储 run_num，采用单例模式
class RunNumber {
public:
    static RunNumber& GetInstance() {
        static RunNumber instance;  // 单例模式，确保只有一个实例
        return instance;
    }

    void SetRunNum(int run_num) {
        m_run_num = run_num;  // 设置 run_num
    }

    int GetRunNum() const {
        return m_run_num;  // 获取 run_num
    }

    void SetEnaSg(bool slotgroup_ena) {
        m_slotgroup_ena = slotgroup_ena; 
    }

    bool GetEnaSg() const {
        return m_slotgroup_ena; 
    }

    void SetAdjEnaSg(bool adj_ena_sg) {
        m_adj_ena_sg = adj_ena_sg; 
    }

    bool GetAdjEnaSg() const {
        return m_adj_ena_sg; 
    }

    void SetAdjRatioLowSg(double adjRatio_low_sg) {
        m_adjRatio_low_sg = adjRatio_low_sg;  
    }

    double GetAdjRatioLowSg() const {
        return m_adjRatio_low_sg;  
    }

    void SetAdjRatioHighSg(double adjRatio_high_sg) {
        m_adjRatio_high_sg = adjRatio_high_sg;  
    }

    double GetAdjRatioHighSg() const {
        return m_adjRatio_high_sg;  
    }

    void SetVariablePacketSizeEna(bool variable_packet_size_ena){
        m_variable_packet_size_ena = variable_packet_size_ena;
    }

    bool GetVariablePacketSizeEna(){
        return m_variable_packet_size_ena;
    }
private:
    int m_run_num;  // 存储 run_num
    bool m_slotgroup_ena;
    bool m_adj_ena_sg;
    double m_adjRatio_low_sg;
    double m_adjRatio_high_sg;
    bool m_variable_packet_size_ena;



    RunNumber() : m_run_num(0){
        // m_adj_ena_sg = 0;
        // m_adjRatio_low_sg = 0.4;
        // m_adjRatio_low_sg = 0.8;
    }  // 默认构造函数，初始化 run_num 为 0
    RunNumber(const RunNumber&) = delete;  // 禁止复制构造
    RunNumber& operator=(const RunNumber&) = delete;  // 禁止赋值操作
};

}  // namespace ns3

#endif  // RUN_NUMBER_H
