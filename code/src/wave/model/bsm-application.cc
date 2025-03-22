/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 North Carolina State University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Scott E. Carpenter <scarpen@ncsu.edu>
 *
 */

#include "ns3/bsm-application.h"
#include "ns3/log.h"
#include "ns3/wave-net-device.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/wave-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/mobility-helper.h"

#include "bsm-timetag.h"
#include "ns3/AperiodicTag.h"
#include <random>
#include "ns3/run_number.h"
#include "ns3/node-container.h"




NS_LOG_COMPONENT_DEFINE ("BsmApplication");

namespace ns3 {

// (Arbitrary) port for establishing socket to transmit WAVE BSMs
int BsmApplication::wavePort = 9080;

NS_OBJECT_ENSURE_REGISTERED (BsmApplication);

TypeId
BsmApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BsmApplication")
    .SetParent<Application> ()
    .SetGroupName ("Wave")
    .AddConstructor<BsmApplication> ()
    ;
  return tid;
}

BsmApplication::BsmApplication ()
  : m_waveBsmStats (0),
    m_aperiodicStats(0),
    m_txSafetyRangesSq (),
    m_TotalSimTime (Seconds (10)),
    m_wavePacketSize (200),
    m_numWavePackets (1),
    m_waveInterval (MilliSeconds (100)),
    m_gpsAccuracyNs (10000),
    m_adhocTxInterfaces (0),
    m_nodesMoving (0),
    m_unirv (0),
    m_nodeId (0),
    m_chAccessMode (0),
    m_txMaxDelay (MilliSeconds (10)),
    m_prevTxDelay (MilliSeconds (0))
{
  NS_LOG_FUNCTION (this);
}

BsmApplication::~BsmApplication ()
{
  NS_LOG_FUNCTION (this);
}

void
BsmApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  // chain up
  Application::DoDispose ();
}

void BsmApplication::StartApplication () 
{
    NS_LOG_FUNCTION (this);

    Time waveInterPacketInterval = m_waveInterval;

    Time startTime = Seconds (1.0);
    Time totalTxTime = m_TotalSimTime - startTime;
    m_numWavePackets = (uint32_t) (totalTxTime.GetDouble () / m_waveInterval.GetDouble ());

    TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

    tdmaDevice = DynamicCast<WifiNetDevice>(m_node->GetDevice(0)); 
    csmaDevice = DynamicCast<WifiNetDevice>(m_node->GetDevice(1)); 

    tdmaSocket = Socket::CreateSocket(m_node, tid);
    csmaSocket = Socket::CreateSocket(m_node, tid);

    tdmaSocket->BindToNetDevice(tdmaDevice);
    csmaSocket->BindToNetDevice(csmaDevice);

    tdmaSocket->SetRecvCallback(MakeCallback(&BsmApplication::ReceiveWavePacket, this));
    csmaSocket->SetRecvCallback(MakeCallback(&BsmApplication::ReceiveWavePacket, this));

    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), wavePort);
    tdmaSocket->Bind(local);
    tdmaSocket->SetAllowBroadcast(true);

    csmaSocket->Bind(local);
    csmaSocket->SetAllowBroadcast(true);

    InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), wavePort);
    tdmaSocket->Connect(remote);
    csmaSocket->Connect(remote);

    Time tDrift = NanoSeconds (m_unirv->GetInteger (0, m_gpsAccuracyNs));
    uint32_t d_ns = static_cast<uint32_t> (m_txMaxDelay.GetInteger ());
    Time txDelay = NanoSeconds (m_unirv->GetInteger (0, d_ns));
    m_prevTxDelay = txDelay;
    Time txTime = startTime + tDrift + txDelay;

    Time firstAperiodicTime = startTime + GetAperiodicRandomInterval();

    variable_packet_size_ena = RunNumber::GetInstance().GetVariablePacketSizeEna();

    NodeContainer nodes = NodeContainer::GetGlobal(); 
    uint32_t totalNodes = nodes.GetN(); 
    //bool sendAperiodic = (m_nodeId < totalNodes / 2); 
    bool sendAperiodic = true;
    if (sendAperiodic)
     {
        if (m_macLayerController->GetCurrentDevice() == tdmaDevice)
        {
            m_macLayerController->DisableDevice(csmaDevice);
            m_macLayerController->EnableDevice(tdmaDevice);
            Simulator::ScheduleWithContext(tdmaSocket->GetNode()->GetId(),
                                            txTime, &BsmApplication::GenerateWaveTraffic, this,
                                            tdmaSocket, m_wavePacketSize, m_numWavePackets, waveInterPacketInterval, m_nodeId);
            Simulator::ScheduleWithContext(tdmaSocket->GetNode()->GetId(),
                                            firstAperiodicTime, &BsmApplication::GenerateAperiodicTraffic, this,
                                            tdmaSocket, m_wavePacketSize, m_nodeId);
        }
        else
        {
            m_macLayerController->DisableDevice(tdmaDevice);
            m_macLayerController->EnableDevice(csmaDevice);
            Simulator::ScheduleWithContext(csmaSocket->GetNode()->GetId(),
                                            txTime, &BsmApplication::GenerateWaveTraffic, this,
                                            csmaSocket, m_wavePacketSize, m_numWavePackets, waveInterPacketInterval, m_nodeId);

            Simulator::ScheduleWithContext(csmaSocket->GetNode()->GetId(),
                                            firstAperiodicTime, &BsmApplication::GenerateAperiodicTraffic, this,
                                            csmaSocket, m_wavePacketSize, m_nodeId);
        }
    }
    else {
        if (m_macLayerController->GetCurrentDevice() == tdmaDevice)
        {
            m_macLayerController->DisableDevice(csmaDevice);
            m_macLayerController->EnableDevice(tdmaDevice);
            Simulator::ScheduleWithContext(tdmaSocket->GetNode()->GetId(),
                                            txTime, &BsmApplication::GenerateWaveTraffic, this,
                                            tdmaSocket, m_wavePacketSize, m_numWavePackets, waveInterPacketInterval, m_nodeId);
        }
        else
        {
            m_macLayerController->DisableDevice(tdmaDevice);
            m_macLayerController->EnableDevice(csmaDevice);
            Simulator::ScheduleWithContext(csmaSocket->GetNode()->GetId(),
                                            txTime, &BsmApplication::GenerateWaveTraffic, this,
                                            csmaSocket, m_wavePacketSize, m_numWavePackets, waveInterPacketInterval, m_nodeId);
        }
    }
}


void BsmApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
}

void
BsmApplication::Setup (Ipv4InterfaceContainer & i,
                       int nodeId,
                       Time totalTime,
                       uint32_t wavePacketSize, // bytes
                       Time waveInterval,
                       double gpsAccuracyNs,
                       std::vector <double> rangesSq,           // m ^2
                       Ptr<WaveBsmStats> waveBsmStats,
                       Ptr<WaveBsmStats> aperiodicStats,
                       std::vector<int> * nodesMoving,
                       int chAccessMode,
                       Time txMaxDelay)
{
  NS_LOG_FUNCTION (this);

  m_unirv = CreateObject<UniformRandomVariable> ();

  m_TotalSimTime = totalTime;
  m_wavePacketSize = wavePacketSize;
  m_waveInterval = waveInterval;
  m_gpsAccuracyNs = gpsAccuracyNs;
  int size = rangesSq.size ();
  m_waveBsmStats = waveBsmStats;
  m_aperiodicStats = aperiodicStats;
  m_nodesMoving = nodesMoving;
  m_chAccessMode = chAccessMode;
  m_txSafetyRangesSq.clear ();
  m_txSafetyRangesSq.resize (size, 0);

  for (int index = 0; index < size; index++)
    {
      // stored as square of value, for optimization
      m_txSafetyRangesSq[index] = rangesSq[index];
    }

  m_adhocTxInterfaces = &i;
  m_nodeId = nodeId;
  m_txMaxDelay = txMaxDelay;
  m_macLayerController = m_node->GetObject<MacLayerController>();
}

void
BsmApplication::GenerateWaveTraffic (Ptr<Socket> socket, uint32_t pktSize,
                                     uint32_t pktCount, Time pktInterval,
                                     uint32_t sendingNodeId)
{
  NS_LOG_FUNCTION (this);
  // more packets to send?
    if (sendingNodeId == 0) 
    {
        return;  // Skip if node 0
    }

  if (pktCount > 0)
    {
      // for now, we cannot tell if each node has
      // started mobility.  so, as an optimization
      // only send if  this node is moving
      // if not, then skip
	  //std::cout<<m_node->GetId()<<" Time:  "<<Simulator::Now().GetMicroSeconds()<<std::endl;
      int txNodeId = sendingNodeId;
      Ptr<Node> txNode = GetNode (txNodeId);
      Ptr<MobilityModel> txPosition = txNode->GetObject<MobilityModel> ();
      NS_ASSERT (txPosition != 0);

      int senderMoving = m_nodesMoving->at (txNodeId);
      if (senderMoving != 0)
      {
    	  BsmTimeTag tag;
    	  tag.setSendingTimeUs(Simulator::Now().GetMicroSeconds());
    	  Ptr<Packet> pkt = Create<Packet> (pktSize);
    	  pkt->AddPacketTag(tag);
    	  //std::cout<<Simulator::Now().GetMicroSeconds()<<std::endl;
	      // send it!
    	  if(m_macLayerController->GetCurrentDevice() == tdmaDevice)
    	  {
    		  tdmaSocket->Send(pkt);
    	  }
    	  else
    	  {
    		  csmaSocket->Send(pkt);
    	  }
    	  //tdmaSocket->Send(pkt);
    	  //csmaSocket->Send(pkt);
          // count it
          m_waveBsmStats->IncTxPktCount ();
          m_waveBsmStats->IncTxByteCount (pktSize);

          // find other nodes within range that would be
          // expected to receive this broadbast
          int nRxNodes = m_adhocTxInterfaces->GetN ();
          int expectedRxNodes = 0;
          for (int i = 0; i < nRxNodes; i++)
            {
              Ptr<Node> rxNode = GetNode (i);
              int rxNodeId = rxNode->GetId ();
              // Skip node 0 from expected receive statistics
              if (rxNodeId == 0) {
                  continue;  // Skip node 0
              }
              
              if (rxNodeId != txNodeId)
                {
                  Ptr<MobilityModel> rxPosition = rxNode->GetObject<MobilityModel> ();
                  NS_ASSERT (rxPosition != 0);
                  // confirm that the receiving node
                  // has also started moving in the scenario
                  // if it has not started moving, then
                  // it is not a candidate to receive a packet
                  int receiverMoving = m_nodesMoving->at (rxNodeId);
                  if (receiverMoving == 1)
                    {
                      double distSq = MobilityHelper::GetDistanceSquaredBetween (txNode, rxNode);
                      if (distSq > 0.0)
                        {
                          // dest node within range?
                          int rangeCount = m_txSafetyRangesSq.size ();
                          for (int index = 1; index <= rangeCount; index++)
                            {
                              if (distSq <= m_txSafetyRangesSq[index - 1])
                                {
                                  // we should expect dest node to receive broadcast pkt
                                  m_waveBsmStats->IncExpectedRxPktCount (index);
                                  expectedRxNodes++;
                                }
                            }
                        }
                    }
                }
            }
      }

      // every BSM must be scheduled with a tx time delay
      // of +/- (5) ms.  See comments in StartApplication().
      // we handle this as a tx delay of [0..10] ms
      // from the start of the pktInterval boundary
      uint32_t d_ns = static_cast<uint32_t> (m_txMaxDelay.GetInteger ());
      Time txDelay = NanoSeconds (m_unirv->GetInteger (0, d_ns));
      Time txTime = pktInterval - m_prevTxDelay + txDelay;
      m_prevTxDelay = txDelay;

      Simulator::ScheduleWithContext (tdmaSocket->GetNode ()->GetId (),
                                      txTime, &BsmApplication::GenerateWaveTraffic, this,
									  tdmaSocket, pktSize, pktCount - 1, pktInterval,  tdmaSocket->GetNode ()->GetId ());
    }
  else
    {
      socket->Close ();
    }
}

void BsmApplication::ReceiveWavePacket(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this);
    Ptr<Packet> packet;
    Address senderAddr;

    if (m_nodeId == 0)
    {
        return;
    }
    while ((packet = socket->RecvFrom(senderAddr)))
    {
        Ptr<Node> rxNode = socket->GetNode();

        if (InetSocketAddress::IsMatchingType(senderAddr))
        {
            InetSocketAddress addr = InetSocketAddress::ConvertFrom(senderAddr);
            int nodes = m_adhocTxInterfaces->GetN();
            for (int i = 0; i < nodes; i++)
            {
                if (addr.GetIpv4() == m_adhocTxInterfaces->GetAddress(i))
                {
                    Ptr<Node> txNode = GetNode(i);

                    // Skip statistics for node 0
                    if (txNode->GetId() == 0)
                    {
                        continue; // Skip processing for node 0
                    }

                    // Check if the packet is aperiodic or periodic
                    AperiodicTag apic_tag;
                    bool isAperiodic = packet->PeekPacketTag(apic_tag);

                    // Remove the BsmTimeTag to calculate delay
                    BsmTimeTag tag;
                    packet->RemovePacketTag(tag);
                    uint32_t delta = Simulator::Now().GetMicroSeconds() - tag.getSendingTimeUs();

                    // Skip processing if delay exceeds 100ms (100,000 microseconds)
                    if (delta > 200000)
                    {
                        NS_LOG_WARN("Packet dropped due to excessive delay: " << delta << "us");
                        continue;
                    }

                    // Handle the packet based on type (aperiodic or periodic)
                    if (isAperiodic)
                    {
                        HandleReceivedAperiodicPacket(txNode, rxNode);
                        m_aperiodicStats->LogPktRecvDeltaTimeUs(delta);
                    }
                    else
                    {
                        HandleReceivedBsmPacket(txNode, rxNode);
                        m_waveBsmStats->LogPktRecvDeltaTimeUs(delta);
                    }
                }
            }
        }
    }
}


void BsmApplication::HandleReceivedBsmPacket (Ptr<Node> txNode,
                                              Ptr<Node> rxNode)
{
  NS_LOG_FUNCTION (this);

  m_waveBsmStats->IncRxPktCount ();

  m_waveBsmStats->LogPktRecvTime2Map(txNode->GetId(), rxNode->GetId());



  Ptr<MobilityModel> rxPosition = rxNode->GetObject<MobilityModel> ();
  NS_ASSERT (rxPosition != 0);
  // confirm that the receiving node
  // has also started moving in the scenario
  // if it has not started moving, then
  // it is not a candidate to receive a packet
  int rxNodeId = rxNode->GetId ();
  int receiverMoving = m_nodesMoving->at (rxNodeId);
  if (receiverMoving == 1)
    {
      double rxDistSq = MobilityHelper::GetDistanceSquaredBetween (rxNode, txNode);
      if (rxDistSq > 0.0)
        {
          int rangeCount = m_txSafetyRangesSq.size ();
          for (int index = 1; index <= rangeCount; index++)
            {
              if (rxDistSq <= m_txSafetyRangesSq[index - 1])
                {
                  m_waveBsmStats->IncRxPktInRangeCount (index);
                }
            }
        }
    }
}

int64_t
BsmApplication::AssignStreams (int64_t streamIndex)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_unirv);  // should be set by Setup() prevoiusly
  m_unirv->SetStream (streamIndex);

  return 1;
}

Ptr<Node>
BsmApplication::GetNode (int id)
{
  NS_LOG_FUNCTION (this);

  std::pair<Ptr<Ipv4>, uint32_t> interface = m_adhocTxInterfaces->Get (id);
  Ptr<Ipv4> pp = interface.first;
  Ptr<Node> node = pp->GetObject<Node> ();

  return node;
}

Ptr<WifiNetDevice>
BsmApplication::GetNetDevice (int id)
{
  NS_LOG_FUNCTION (this);
//  std::pair<Ptr<Ipv4>, uint32_t> interface = m_adhocTxInterfaces->Get (id);
//  Ptr<Ipv4> pp = interface.first;
//  Ptr<NetDevice> device = pp->GetObject<NetDevice> ();

}

void BsmApplication::GenerateAperiodicTraffic(Ptr<Socket> socket, uint32_t pktSize, uint32_t sendingNodeId)
{
    NS_LOG_FUNCTION(this);

    // Skip packet sending and statistics for node 0
    if (sendingNodeId == 0) {
        return;  // Skip if node 0
    }

    // 确定当前节点ID，获取节点位置信息
    int txNodeId = sendingNodeId;
    Ptr<Node> txNode = GetNode(txNodeId);
    Ptr<MobilityModel> txPosition = txNode->GetObject<MobilityModel>();
    NS_ASSERT(txPosition != 0);

    // 确认节点是否移动，如果移动，生成数据包
    int senderMoving = m_nodesMoving->at(txNodeId);
    if (senderMoving != 0)
    {
          pktSize = 200;
          //如果启用了随机包大小，随机生成数据包大小
          if (variable_packet_size_ena)
          {
              // 随机生成数据包大小，范围 [200, 1200]，步长为 200
              std::vector<uint32_t> pktSizes = {200, 400, 600, 800, 1000, 1200};
              std::random_device rd;
              std::mt19937 gen(rd());
              std::uniform_int_distribution<> dis(0, pktSizes.size() - 1);
              pktSize = pktSizes[dis(gen)];
          }

          // 生成数据包并添加时间戳标签
          BsmTimeTag tag;
          tag.setSendingTimeUs(Simulator::Now().GetMicroSeconds());
          AperiodicTag apic_tag;
          Ptr<Packet> pkt = Create<Packet>(pktSize);
          pkt->AddPacketTag(tag);
          pkt->AddPacketTag(apic_tag);

          // 根据 MAC 控制器的当前设备选择对应的 socket
          if (m_macLayerController->GetCurrentDevice() == tdmaDevice)
          {
              // 发送数据包使用 TDMA 设备
              tdmaSocket->Send(pkt);
          }
          else
          {
              // 发送数据包使用 CSMA 设备
              csmaSocket->Send(pkt);
          }

          // 统计发送数据包的数量和字节数
          m_aperiodicStats->IncTxPktCount();
          m_aperiodicStats->IncTxByteCount(pktSize);

          // 确定所有在范围内的节点
          int nRxNodes = m_adhocTxInterfaces->GetN();
          for (int j = 0; j < nRxNodes; j++)  // 遍历所有接收节点
          {
              Ptr<Node> rxNode = GetNode(j);
              int rxNodeId = rxNode->GetId();
              int expectedRxNodes = 0;
              // Skip node 0 from expected receive statistics
              if (rxNodeId == 0) {
                  continue;  // Skip node 0
              }

              if (rxNodeId != txNodeId)
              {
                  Ptr<MobilityModel> rxPosition = rxNode->GetObject<MobilityModel>();
                  NS_ASSERT(rxPosition != 0);
                  // 确认接收节点是否也在移动
                  int receiverMoving = m_nodesMoving->at(rxNodeId);
                  if (receiverMoving == 1)
                  {
                      double distSq = MobilityHelper::GetDistanceSquaredBetween(txNode, rxNode);
                      if (distSq > 0.0)
                      {
                          int rangeCount = m_txSafetyRangesSq.size();
                          for (int index = 1; index <= rangeCount; index++)
                          {
                              if (distSq <= m_txSafetyRangesSq[index - 1])
                              {
                                  // **记录每个数据包的预期接收节点数**
                                  m_aperiodicStats->IncExpectedRxPktCount(index);
                                  expectedRxNodes++;
                              }
                          }
                      }
                  }
              }
          }
      
      // 随机生成下一个数据包的间隔时间
      Time randomInterval = GetAperiodicRandomInterval();

      // 添加随机延迟以避免冲突
      uint32_t d_ns = static_cast<uint32_t>(m_txMaxDelay.GetInteger());
      Time txDelay = NanoSeconds(m_unirv->GetInteger(0, d_ns));
      Time txTime = randomInterval - m_prevTxDelay + txDelay;
      m_prevTxDelay = txDelay;
      // 调度下一次非周期性数据包发送
      if (m_macLayerController->GetCurrentDevice() == tdmaDevice)
      {
          Simulator::ScheduleWithContext(tdmaSocket->GetNode()->GetId(), txTime, &BsmApplication::GenerateAperiodicTraffic, this,
                                          tdmaSocket, pktSize, sendingNodeId);
      }
      else
      {
          Simulator::ScheduleWithContext(csmaSocket->GetNode()->GetId(), txTime, &BsmApplication::GenerateAperiodicTraffic, this,
                                          csmaSocket, pktSize, sendingNodeId);
      }
    }
}








void BsmApplication::HandleReceivedAperiodicPacket(Ptr<Node> txNode, Ptr<Node> rxNode)
{
    NS_LOG_FUNCTION(this);

    // 统计接收数据包
    m_aperiodicStats->IncRxPktCount();
    m_aperiodicStats->LogPktRecvTime2Map(txNode->GetId(), rxNode->GetId());

    Ptr<MobilityModel> rxPosition = rxNode->GetObject<MobilityModel>();
    NS_ASSERT(rxPosition != 0);

    int rxNodeId = rxNode->GetId();
    int receiverMoving = m_nodesMoving->at(rxNodeId);

    if (receiverMoving == 1)
    {
        double rxDistSq = MobilityHelper::GetDistanceSquaredBetween(rxNode, txNode);
        if (rxDistSq > 0.0)
        {
            int rangeCount = m_txSafetyRangesSq.size();
            for (int index = 1; index <= rangeCount; index++)
            {
                if (rxDistSq <= m_txSafetyRangesSq[index - 1])
                {
                    m_aperiodicStats->IncRxPktInRangeCount(index);
                }
            }
        }
    }
}



Time BsmApplication::GetAperiodicRandomInterval()
{
    // 创建一个均值为50毫秒的指数分布随机变量
    Ptr<ExponentialRandomVariable> expRandomVar = CreateObject<ExponentialRandomVariable>();
    expRandomVar->SetAttribute("Mean", DoubleValue(50.0));  

    // 随机延迟 = 50ms 固定时间 + 指数分布随机时间
    Time randomInterval = MilliSeconds(50) + MilliSeconds(expRandomVar->GetValue());
    //Time randomInterval = MilliSeconds(expRandomVar->GetValue());
    return randomInterval;
}



} // namespace ns3
