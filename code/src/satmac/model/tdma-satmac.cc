/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/assert.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "tdma-satmac.h"
#include "tdma-mac.h"
#include "tdma-mac-low.h"
#include "satmac-packet.h"
#include "ns3/abort.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/wifi-utils.h"
#include <map>

#include <string>
#include <fstream>
#include "ns3/mobility-module.h"
//#include "ns3/random-variable-stream.h"
//#include "ns3/rng-seed-manager.h"

#include "ns3/node-list.h"
#include "ns3/ipv4.h"
#include "ns3/ipv4-interface.h"
#include "ns3/arp-cache.h"
#include "ns3/MacLayerController.h"
#include "ns3/SlotGroupTag.h"
#include "ns3/SlotGroupHeader.h"
#include "ns3/AperiodicTag.h"
#include "ns3/GeohashHelper.h"
#include <random>
#include "ns3/run_number.h"
#include <sys/stat.h>  // 用于 mkdir
#include "ns3/bsm-timetag.h"



NS_LOG_COMPONENT_DEFINE ("TdmaSatmac");


#define MY_DEBUG(x) \
  NS_LOG_DEBUG (Simulator::Now () << " " << this << " " << x)

namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (TdmaSatmac);

Time
TdmaSatmac::GetDefaultSlotTime (void)
{
  return MicroSeconds (1000);
}

Time
TdmaSatmac::GetDefaultGuardTime (void)
{
  return MicroSeconds (5);
}

DataRate
TdmaSatmac::GetDefaultDataRate (void)
{
  NS_LOG_DEBUG ("Setting default");
  return DataRate ("12000000b/s");
}

int TdmaSatmac::GetDefaultFrameLen(void) 
{
  return 64;
}

int TdmaSatmac::GetDefaultSlotLife(void) 
{
  return 3;
}

int TdmaSatmac::GetDefaultC3HThreshold(void) 
{
  return 2;
}
int TdmaSatmac::GetDefaultAdjThreshold(void) 
{
  return 5;
}
int TdmaSatmac::GetDefaultRandomBchIfSingle(void) 
{
  return 1;
}

int TdmaSatmac::GetDefaultChooseBchRandomSwitch(void)
{
  return 1;
}

int TdmaSatmac::GetDefaultAdjEnable(void) 
{
  return 1;
}
int TdmaSatmac::GetDefaultAdjFrameEnable(void)
{
  return 1;
}

int TdmaSatmac::GetDefaultAdjFrameLowerBound(void)
{
  return 32;
}

int TdmaSatmac::GetDefaultAdjFrameUpperBound(void) 
{
  return 128;
}

int TdmaSatmac::GetDefaultSlotMemory(void) 
{
  return 1;
}
double TdmaSatmac::GetDefaultFrameadjCutRatioEhs()
{
	return 0.6;
}
double TdmaSatmac::GetDefaultFrameadjCutRatioThs()
{
	return 0.4;
}
double TdmaSatmac::GetDefaultFrameadjExpRatio()
{
	return 0.9;
}
/*************************************************************
 * Tdma Controller Class Functions
 ************************************************************/
TypeId
TdmaSatmac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3:TdmaSatmac")
    .SetParent<TdmaMac> ()
    .AddConstructor<TdmaSatmac> ()
    .AddAttribute ("DataRate",
                   "The default data rate for point to point links",
                   DataRateValue (GetDefaultDataRate ()),
                   MakeDataRateAccessor (&TdmaSatmac::SetDataRate,
                                         &TdmaSatmac::GetDataRate),
                   MakeDataRateChecker ())
    .AddAttribute ("SlotTime", "The duration of a Slot in microseconds.",
                   TimeValue (GetDefaultSlotTime ()),
                   MakeTimeAccessor (&TdmaSatmac::SetSlotTime,
                                     &TdmaSatmac::GetSlotTime),
                   MakeTimeChecker ())
    .AddAttribute ("GuardTime", "GuardTime between TDMA slots in microseconds.",
                   TimeValue (GetDefaultGuardTime ()),
                   MakeTimeAccessor (&TdmaSatmac::SetGuardTime,
                                     &TdmaSatmac::GetGuardTime),
                   MakeTimeChecker ())
    .AddAttribute ("InterFrameTime", "The wait time between consecutive tdma frames.",
                   TimeValue (MicroSeconds (0)),
                   MakeTimeAccessor (&TdmaSatmac::SetInterFrameTimeInterval,
                                     &TdmaSatmac::GetInterFrameTimeInterval),
                   MakeTimeChecker ())

//	  .AddAttribute ("STI", "",
//					 IntegerValue (-1),
//					 MakeIntegerAccessor (&TdmaSatmac::SetGlobalSti,
//									   &TdmaSatmac::GetGlobalSti),
//					 MakeIntegerChecker<int> (0,9999))
	  .AddAttribute ("FrameLen", "",
					 IntegerValue (GetDefaultFrameLen()),
					 MakeIntegerAccessor (&TdmaSatmac::SetFrameLen,
									   &TdmaSatmac::GetFrameLen),
					 MakeIntegerChecker<int> (1,129))
	  .AddAttribute ("SlotLife", "",
					 IntegerValue (GetDefaultSlotLife()),
					 MakeIntegerAccessor (&TdmaSatmac::SetSlotLife,
									   &TdmaSatmac::GetSlotLife),
					 MakeIntegerChecker<int> (0,20))
	  .AddAttribute ("C3HThreshold", "",
					 IntegerValue (GetDefaultC3HThreshold()),
					 MakeIntegerAccessor (&TdmaSatmac::SetC3HThreshold,
									   &TdmaSatmac::GetC3HThreshold),
					 MakeIntegerChecker<int> (0,10))
	  .AddAttribute ("AdjThreshold", "",
					 IntegerValue (GetDefaultAdjThreshold()),
					 MakeIntegerAccessor (&TdmaSatmac::SetAdjThreshold,
									   &TdmaSatmac::GetAdjThreshold),
					 MakeIntegerChecker<int> (0,50))
	  .AddAttribute ("RandomBchIfSingle", "",
					 IntegerValue (GetDefaultRandomBchIfSingle()),
					 MakeIntegerAccessor (&TdmaSatmac::SetRandomBchIfSingle,
									   &TdmaSatmac::GetRandomBchIfSingle),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("ChooseBchRandomSwitch", "",
					 IntegerValue (GetDefaultChooseBchRandomSwitch()),
					 MakeIntegerAccessor (&TdmaSatmac::setChooseBchRandomSwitch,
									   &TdmaSatmac::getChooseBchRandomSwitch),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("VemacMode", "",
					 IntegerValue (0),
					 MakeIntegerAccessor (&TdmaSatmac::setVemacMode,
									   &TdmaSatmac::getVemacMode),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("AdjEnable", "",
					 IntegerValue (GetDefaultAdjEnable()),
					 MakeIntegerAccessor (&TdmaSatmac::SetAdjEnable,
									   &TdmaSatmac::GetAdjEnable),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("AdjFrameEnable", "",
					 IntegerValue (GetDefaultAdjFrameEnable()),
					 MakeIntegerAccessor (&TdmaSatmac::SetAdjFrameEnable,
									   &TdmaSatmac::GetAdjFrameEnable),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("AdjFrameLowerBound", "",
					 IntegerValue (GetDefaultAdjFrameLowerBound()),
					 MakeIntegerAccessor (&TdmaSatmac::SetAdjFrameLowerBound,
									   &TdmaSatmac::GetAdjFrameLowerBound),
					 MakeIntegerChecker<int> (0,129))
	  .AddAttribute ("AdjFrameUpperBound", "",
					 IntegerValue (GetDefaultAdjFrameUpperBound()),
					 MakeIntegerAccessor (&TdmaSatmac::SetAdjFrameUpperBound,
									   &TdmaSatmac::GetAdjFrameUpperBound),
					 MakeIntegerChecker<int> (0,129))
	  .AddAttribute ("SlotMemory", "",
					 IntegerValue (GetDefaultSlotMemory()),
					 MakeIntegerAccessor (&TdmaSatmac::SetSlotMemory,
									   &TdmaSatmac::GetSlotMemory),
					 MakeIntegerChecker<int> (0,1))
	  .AddAttribute ("FrameadjExpRatio", "",
					 DoubleValue (GetDefaultFrameadjExpRatio()),
					 MakeDoubleAccessor (&TdmaSatmac::setFrameadjExpRatio,
									   &TdmaSatmac::getFrameadjExpRatio),
					 MakeDoubleChecker<double> (0,1))
	  .AddAttribute ("FrameadjCutRatioThs", "",
					 DoubleValue (GetDefaultFrameadjCutRatioThs()),
					 MakeDoubleAccessor (&TdmaSatmac::setFrameadjCutRatioThs,
									   &TdmaSatmac::getFrameadjCutRatioThs),
					 MakeDoubleChecker<double> (0,1))
	  .AddAttribute ("FrameadjCutRatioEhs", "",
					 DoubleValue (GetDefaultFrameadjCutRatioEhs()),
					 MakeDoubleAccessor (&TdmaSatmac::setFrameadjCutRatioEhs,
									   &TdmaSatmac::getFrameadjCutRatioEhs),
					 MakeDoubleChecker<double> (0,1))
	  .AddAttribute ("LPFTraceFile", "",
					 StringValue ("lpf-output.txt"),
					 MakeStringAccessor (&TdmaSatmac::setTraceOutFile),
					 MakeStringChecker())

	// Added to trace the transmission of v2x message
	.AddTraceSource ("BCHTrace",
					 "trace to track the announcement of bch messages",
					 MakeTraceSourceAccessor (&TdmaSatmac::m_BCHTrace),
					 "ns3::TdmaSatmac::BCHTracedCallback")
  ;
  return tid;
}

TdmaSatmac::TdmaSatmac ()
{
  global_sti = 599;
  NS_LOG_FUNCTION (this);
  //m_traceOutFile = "lpf-output.txt";
  m_wifimaclow_flag = 0;
  m_low = CreateObject<TdmaMacLow> ();
  m_queue = CreateObject<TdmaMacQueue> ();
  m_queue->SetTdmaMacTxDropCallback (MakeCallback (&TdmaSatmac::NotifyTxDrop, this));
  m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
  m_transmissionListener = new TransmissionListenerUseless ();

  adj_single_slot_ena_ = 0;
  bch_slot_lock_ = 5;
  adj_ena_ = 1;
  adj_free_threshold_ = 5;
  adj_frame_ena_ = 1;
  adj_frame_lower_bound_  = 32;
  adj_frame_upper_bound_ = 128;
  slot_memory_ = 1;
  frameadj_cut_ratio_ths_ = 0.4;
  frameadj_cut_ratio_ehs_ = 0.6;
  frameadj_exp_ratio_ = 0.9;
  testmode_init_flag_ = 0;
  random_bch_if_single_switch_ = 0;
  choose_bch_random_switch_ = 1;
  slot_adj_candidate_ = -1;

  vemac_mode_ = 0;


//  frameadj_cut_ratio_ths_ = 0.4;
//  frameadj_cut_ratio_ehs_ = 0.6;
//  frameadj_exp_ratio_ = 0.9;

  m_queue->SetMacPtr (this);
  m_low->SetRxCallback (MakeCallback (&TdmaSatmac::Receive, this));
  m_channel_utilization_OutFile = "channel_utilization.txt";
  TdmaMac::DoInitialize ();
}

TdmaSatmac::~TdmaSatmac ()
{
  m_channel = 0;
  m_bps = 0;
//  delete[] m_sg_info;
//  m_sg_info = NULL;
}

void
TdmaSatmac::Start (void)
{
  NS_LOG_FUNCTION (this);
  total_slot_count_ = 0;
  slot_count_ = 0;
  slot_num_ = (slot_count_+1)% m_frame_len; //slot_num_初始化为当前的下一个时隙。

  global_psf = 0;
  collected_fi_ = new Frame_info(512);
  collected_fi_->sti = this->global_sti;
  received_fi_list_= NULL;

  node_state_ = NODE_INIT;
  slot_state_ = BEGINING;
  collision_count_ = 0;
  localmerge_collision_count_ = 0;
  request_fail_times = 0;
  waiting_frame_count = 0;
  frame_count_ = 0;
  this->enable=0;
  this->packet_sended = 0;
  this->packet_received =0;

  recv_fi_count_ = 0;
  send_fi_count_ = 0;

//  std::cout<<"Start time:" << Simulator::Now().GetMicroSeconds() << "ID: " << this->GetGlobalSti() << std::endl;
//NanoSeconds

  //m_start_delay_frames = 0;
  //m_start_delay_frames = m_uniformRandomVariable->GetInteger(0,10);
  int starttime = 1;
  location_initialed_ = false;
  direction_initialed_ = false;
  //std::cout<<"Start time:" << m_start_delay_frames << " ID: " << this->GetGlobalSti() << std::endl;
  Simulator::Schedule (MilliSeconds (starttime),&TdmaSatmac::slotHandler, this);
  //slotHandler();

  m_slot_group = -1;
  slot_group_length = 4;
  total_slot_group_count = 0;
  slot_group_count = 0;//当前时隙组index
  isNeedSg = 0;
  slotgroup_ena = RunNumber::GetInstance().GetEnaSg();
  adj_ena_sg = RunNumber::GetInstance().GetAdjEnaSg();
  adjRatio_low_sg = RunNumber::GetInstance().GetAdjRatioLowSg();
  adjRatio_high_sg = RunNumber::GetInstance().GetAdjRatioHighSg();
  isNeed_adj_Sg = 0;

 //TODO
 Simulator::Schedule ((MilliSeconds (starttime) * (slot_group_length +1)),&TdmaSatmac::slotgroupHandler, this);

}

void
TdmaSatmac::Initialize ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Start();
}

void
TdmaSatmac::DoDispose (void)
{
  m_low->Dispose ();
  m_low = 0;
  m_device = 0;
  m_queue = 0;
  TdmaMac::DoDispose ();
}

void
TdmaSatmac::NotifyTx (Ptr<const Packet> packet)
{
  m_macTxTrace (packet);
}

void
TdmaSatmac::NotifyTxDrop (Ptr<const Packet> packet)
{
  m_macTxDropTrace (packet);
}

void
TdmaSatmac::NotifyRx (Ptr<const Packet> packet)
{
  m_macRxTrace (packet);
}

void
TdmaSatmac::NotifyPromiscRx (Ptr<const Packet> packet)
{
  m_macPromiscRxTrace (packet);
}

void
TdmaSatmac::NotifyRxDrop (Ptr<const Packet> packet)
{
  m_macRxDropTrace (packet);
}

void
TdmaSatmac::SetDevice (Ptr<TdmaNetDevice> device)
{
  m_device = device;
  m_low->SetDevice (m_device);
}

Ptr<TdmaNetDevice>
TdmaSatmac::GetDevice (void) const
{
  return m_device;
}

Ptr<TdmaMacLow>
TdmaSatmac::GetTdmaMacLow (void) const
{
  return m_low;
}

void
TdmaSatmac::SetForwardUpCallback (Callback<void, Ptr<Packet>, const WifiMacHeader*> upCallback)
{
  NS_LOG_FUNCTION (this);
  m_upCallback = upCallback;
}

void
TdmaSatmac::SetLinkUpCallback (Callback<void> linkUp)
{
  linkUp ();
}
void
TdmaSatmac::SetLinkDownCallback (Callback<void> linkDown)
{
}
void
TdmaSatmac::SetTxQueueStartCallback (Callback<bool,uint32_t> queueStart)
{
  NS_LOG_FUNCTION (this);
  m_queueStart = queueStart;
}

void
TdmaSatmac::SetTxQueueStopCallback (Callback<bool,uint32_t> queueStop)
{
  NS_LOG_FUNCTION (this);
  m_queueStop = queueStop;
}

uint32_t
TdmaSatmac::GetQueueState (uint32_t index)
{
  if (m_queue->GetMaxSize () == m_queue->GetSize ())
    {
      return 0;
    }
  else
    {
      return 1;
    }
}

uint32_t
TdmaSatmac::GetNQueues (void)
{
  //TDMA currently has only one queue
  return 1;
}


void
TdmaSatmac::SetMaxQueueSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_queue->SetMaxSize (size);
}
void
TdmaSatmac::SetMaxQueueDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_queue->SetMaxDelay (delay);
}


Mac48Address
TdmaSatmac::GetAddress (void) const
{
  if (!m_wifimaclow_flag)
    return m_low->GetAddress ();
  else
	return m_wifimaclow->GetAddress();
}
Ssid
TdmaSatmac::GetSsid (void) const
{
  return m_ssid;
}
void
TdmaSatmac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (address);
  m_low->SetAddress (address);
  m_low->SetBssid (address);
}
void
TdmaSatmac::SetSsid (Ssid ssid)
{
  NS_LOG_FUNCTION (ssid);
  m_ssid = ssid;
}
Mac48Address
TdmaSatmac::GetBssid (void) const
{
  return m_low->GetBssid ();
}

void
TdmaSatmac::Queue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  if (!m_queue->Enqueue (packet, hdr))
    {
      NotifyTxDrop (packet);
    }


#ifdef PRINT_SLOT_STATUS
  printf("I'm node %d, in slot %d, I Queue a data packet\n", global_sti, slot_count_);
#endif
  //Cannot request for channel access in tdma. Tdma schedules every node in round robin manner
  //RequestForChannelAccess();
}

void
TdmaSatmac::SetSlotTime (Time slotTime)
{
  NS_LOG_FUNCTION (this << slotTime);
  m_slotTime = slotTime.GetMicroSeconds ();
}

Time
TdmaSatmac::GetSlotTime (void) const
{
  return MicroSeconds (m_slotTime);
}

void
TdmaSatmac::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this << bps);
  m_bps = bps;
}

DataRate
TdmaSatmac::GetDataRate (void) const
{
  return m_bps;
}

void
TdmaSatmac::SetChannel (Ptr<SimpleWirelessChannel> c)
{
  if (c != 0)
	{
	  m_channel = c;
	  m_low->SetChannel (m_channel);
	}
}


Ptr<SimpleWirelessChannel>
TdmaSatmac::GetChannel (void) const
{
  NS_LOG_FUNCTION (this);
  return m_channel;
}

void
TdmaSatmac::SetGuardTime (Time guardTime)
{
  NS_LOG_FUNCTION (this << guardTime);
  //guardTime is based on the SimpleWirelessChannel's max range
  if (m_channel != 0)
    {
      m_guardTime = Seconds (m_channel->GetMaxRange () / 300000000.0).GetMicroSeconds ();
    }
  else
    {
      m_guardTime = guardTime.GetMicroSeconds ();
    }
}

Time
TdmaSatmac::GetGuardTime (void) const
{
  return MicroSeconds (m_guardTime);
}

void
TdmaSatmac::SetInterFrameTimeInterval (Time interFrameTime)
{
  NS_LOG_FUNCTION (interFrameTime);
  m_tdmaInterFrameTime = interFrameTime.GetMicroSeconds ();
}

Time
TdmaSatmac::GetInterFrameTimeInterval (void) const
{
  return MicroSeconds (m_tdmaInterFrameTime);
}

void TdmaSatmac::SetGlobalSti(int sti)
{
  global_sti = sti;
}
int TdmaSatmac::GetGlobalSti(void) const
{
  return global_sti;
}

void TdmaSatmac::SetFrameLen(int framelen)
{
  m_frame_len = framelen;
}
int TdmaSatmac::GetFrameLen(void) const
{
  return m_frame_len;
}

void TdmaSatmac::SetSlotLife(int slotlife_perframe)
{
  slot_lifetime_frame_ = slotlife_perframe;
}

int TdmaSatmac::GetSlotLife(void) const
{
  return slot_lifetime_frame_;
}

void TdmaSatmac::SetC3HThreshold(int c3h_threshold)
{
  c3hop_threshold_ = c3h_threshold;
}

int TdmaSatmac::GetC3HThreshold(void) const
{
  return c3hop_threshold_;
}

void TdmaSatmac::SetAdjThreshold(int adj_threshold)
{
  adj_free_threshold_ = adj_threshold;
}

int TdmaSatmac::GetAdjThreshold(void) const
{
  return adj_free_threshold_;
}

void TdmaSatmac::SetRandomBchIfSingle(int flag)
{
  random_bch_if_single_switch_ = flag;
}

int TdmaSatmac::GetRandomBchIfSingle(void) const
{
  return random_bch_if_single_switch_;
}

int TdmaSatmac::getChooseBchRandomSwitch() const {
	return choose_bch_random_switch_;
}

void TdmaSatmac::setChooseBchRandomSwitch(int chooseBchRandomSwitch) {
	choose_bch_random_switch_ = chooseBchRandomSwitch;
}

void TdmaSatmac::SetAdjEnable(int flag)
{
  adj_ena_ = flag;
}

int TdmaSatmac::GetAdjEnable(void) const
{
  return adj_ena_;
}

void TdmaSatmac::SetAdjFrameEnable(int flag)
{
  adj_frame_ena_ = flag;
}

int TdmaSatmac::GetAdjFrameEnable(void) const
{
  return adj_frame_ena_;
}

void TdmaSatmac::SetAdjFrameLowerBound(int lowerbound)
{
  adj_frame_lower_bound_ = lowerbound;
}

int TdmaSatmac::GetAdjFrameLowerBound(void) const
{
  return adj_frame_lower_bound_;
}

void TdmaSatmac::SetAdjFrameUpperBound(int upperbound)
{
  adj_frame_upper_bound_ = upperbound;
}

int TdmaSatmac::GetAdjFrameUpperBound(void) const
{
  return adj_frame_upper_bound_;
}

void TdmaSatmac::SetSlotMemory(int flag)
{
  slot_memory_ = flag;
}

int TdmaSatmac::GetSlotMemory(void) const
{
  return slot_memory_;
}

void TransmissionListenerUseless::EndTxNoAck()
{
	  txok = true;
}

void
TdmaSatmac::Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_DATA);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();
  Queue (packet, hdr);
}
void
TdmaSatmac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_DATA);
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (m_low->GetAddress ());
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();
  Queue (packet, hdr);
  NS_LOG_FUNCTION (this << packet << to);
}

void TdmaSatmac::Enqueue (Ptr<const Packet> packet, WifiMacHeader hdr)
{
	Queue (packet, hdr);
}

bool
TdmaSatmac::SupportsSendFrom (void) const
{
  return true;
}

void
TdmaSatmac::TxOk (const WifiMacHeader &hdr)
{
}
void
TdmaSatmac::TxFailed (const WifiMacHeader &hdr)
{
}

void
TdmaSatmac::TxQueueStart (uint32_t index)
{
  NS_ASSERT (index < GetNQueues ());
  m_queueStart (index);
}
void
TdmaSatmac::TxQueueStop (uint32_t index)
{
  NS_ASSERT (index < GetNQueues ());
  m_queueStop (index);
}

void TdmaSatmac::show_slot_occupation() {
	int i,free_count = 0;
	std::map<unsigned int,int> omap;
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
	for(i=0 ; i < m_frame_len; i++){
		if(fi_local_[i].busy== SLOT_FREE)
			free_count++;
		else {
			if (omap[fi_local_[i].sti])
				NS_LOG_DEBUG("Node " << fi_local_[i].sti <<" has occupied more than one slot!");
			else
				omap[fi_local_[i].sti] = 1;
		}
	}
	NS_LOG_DEBUG("FREE SLOT: " << free_count);
}

void TdmaSatmac::clear_others_slot_status() {
	slot_tag *fi_local = this->collected_fi_->slot_describe;
	int count;
	for (count=0; count < m_frame_len; count++){
		if (fi_local[count].sti != global_sti) {
			fi_local[count].busy = SLOT_FREE;
			fi_local[count].sti = 0;
			fi_local[count].count_2hop = 0;
			fi_local[count].count_3hop = 0;
			fi_local[count].psf = 0;
			fi_local[count].locker = 0;
		}
	}
}

//初始化一个fi记录
void TdmaSatmac::clear_FI(Frame_info *fi){
	//fi->frame_len;
	//fi->index;
	//fi->sti;
	fi->valid_time = 0;
	fi->remain_time = 0;
	fi->recv_slot = -1;
	if(fi->slot_describe != NULL){
		delete[] fi->slot_describe;
	}
	fi->slot_describe = new slot_tag[512];
}

/*
 * allocate a new fi and add insert in the head of received_fi_list;
 */
Frame_info * TdmaSatmac::get_new_FI(int slot_count){
	Frame_info *newFI= new Frame_info(slot_count);
//	newFI->next_fi = this->received_fi_list_;
	Frame_info *tmp;

	if (received_fi_list_ == NULL)
		received_fi_list_ = newFI;
	else {
		for (tmp = received_fi_list_; tmp->next_fi != NULL; tmp = tmp->next_fi) {}
		tmp->next_fi = newFI;
	}
	newFI->next_fi = NULL;

	return newFI;
}

void TdmaSatmac::print_slot_status(void) {
	slot_tag *fi_local = this->collected_fi_->slot_describe;
	int i, count;
	int free_count_ths = 0, free_count_ehs = 0;
	for(i=0 ; i < m_frame_len; i++){
		if (fi_local[i].busy== SLOT_FREE)
			free_count_ths++;
		if(fi_local[i].busy== SLOT_FREE && fi_local[i].count_3hop == 0)
			free_count_ehs++;
	}
	NS_LOG_DEBUG("I'm node "<<global_sti<<" in slot " <<slot_count_<<" FreeThs: "<<free_count_ths<<", Ehs "
			<<free_count_ehs<<" total "<< m_frame_len<<" status: ");
	for (count=0; count < m_frame_len; count++){
		NS_LOG_DEBUG("|| "<< fi_local[count].sti<<" ");
		switch (fi_local[count].busy) {
		case SLOT_FREE:
			NS_LOG_DEBUG("(0,0) ");
			break;
		case SLOT_1HOP:
			NS_LOG_DEBUG("(1,0) ");
			break;
		case SLOT_2HOP:
			NS_LOG_DEBUG("(0,1) ");
			break;
		case SLOT_COLLISION:
			NS_LOG_DEBUG("(1,1) ");
			break;
		}

		NS_LOG_DEBUG("c:"<< fi_local[count].count_2hop<<"/"<<fi_local[count].count_3hop<<" ");
	}
	NS_LOG_DEBUG("");
}

bool TdmaSatmac::isNewNeighbor(int sid) {
	slot_tag *fi_local = this->collected_fi_->slot_describe;
	int count;
	for (count=0; count < m_frame_len; count++){
		if (fi_local[count].sti == sid)
			return false;
	}
	return true;
}

/* This function is used to pick up a random slot of from those which is free. */
int TdmaSatmac::determine_BCH(bool strict){
	//std::cout<<"Node: "<<this->getNodePtr()->GetId()<<" determine_BCH "<<Simulator::Now().GetMicroSeconds()<<std::endl;
	int i=0,chosen_slot=0;
//	int loc;
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
//	int s1c[256];
	int s2c[256];
	int s0c[256];
	int s0_1c[128];
	int s0_2c[128];
//	int s2_1c[128];


//	int s1c_num = 0, s2_1c_num = 0;
	int s2c_num = 0, s0c_num = 0;
	int s0_1c_num = 0;
	int s0_2c_num = 0;
	int free_count_ths = 0, free_count_ehs = 0;

	for(i=0 ; i < m_frame_len; i++){
		if((fi_local_[i].busy== SLOT_FREE || (!strict && fi_local_[i].sti==global_sti)) && !fi_local_[i].locker) {
			if (vemac_mode_) {
				if (i < m_frame_len/2)
					s0_1c[s0_1c_num++] = i;
				else
					s0_2c[s0_2c_num++] = i;
			} else if (adj_ena_) {
				s2c[s2c_num++] = i;
//				if (i < m_frame_len/2)
//					s2_1c[s2_1c_num++] = i;

				if (fi_local_[i].count_3hop  == 0) {
					s0c[s0c_num++] = i;
//					s1c[s1c_num++] = i;
					if (i < m_frame_len/2)
						s0_1c[s0_1c_num++] = i;
				} else if (fi_local_[i].count_3hop < c3hop_threshold_ ){
//					s1c[s1c_num++] = i;
				}

			} else {
				s0c[s0c_num++] = i;
			}
		}
	}
	if (vemac_mode_) {

		if (direction_ > 0) {
			if (s0_1c_num > 0) {
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s0_1c_num-1);
				return s0_1c[chosen_slot];
			} else if (s0_2c_num > 0) {
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s0_2c_num-1);
				return s0_2c[chosen_slot];
			} else
				return -1;
		} else {
			if (s0_2c_num > 0) {
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s0_2c_num-1);
				return s0_2c[chosen_slot];
			} else if (s0_1c_num > 0) {
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s0_1c_num-1);
				return s0_1c[chosen_slot];
			} else
				return -1;
		}
	}


	for(i=0 ; adj_frame_ena_ && i < m_frame_len; i++){
		if (fi_local_[i].busy== SLOT_FREE)
			free_count_ths++;
		if(fi_local_[i].busy== SLOT_FREE && fi_local_[i].count_3hop == 0)
			free_count_ehs++;
	}

	//Choose slot only in the fist half of frame (when adjusting slot)
	if (adj_frame_ena_&& m_frame_len > adj_frame_lower_bound_
					  &&  (((float)(m_frame_len - free_count_ehs))/m_frame_len) <= frameadj_cut_ratio_ehs_
					  && (((float)(m_frame_len - free_count_ths))/m_frame_len) <= frameadj_cut_ratio_ths_)
	{
		if (s0_1c_num != 0) {
			chosen_slot = m_uniformRandomVariable->GetInteger (0, s0_1c_num-1);
			return s0_1c[chosen_slot];
		}
	}

	if (testmode_init_flag_ && choose_bch_random_switch_ == 2) {
		testmode_init_flag_ = 0;
		switch (global_sti) {
		case 1: return 0;
		case 2: return 1;
		case 3: return 2;
		case 4: return 0;
		default: return global_sti -1;
		}
	}

	if (!adj_ena_) {
		if (s0c_num > 0) {
			chosen_slot = m_uniformRandomVariable->GetInteger (0, s0c_num-1);
			return s0c[chosen_slot];
		} else {

//	show_slot_occupation();
//	print_slot_status();

			return -1;
		}
	} else {
		if (/*strict &&*/ s0c_num >= adj_free_threshold_) {
			if (choose_bch_random_switch_) {
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s0c_num-1);
			} else
				chosen_slot = 0;
			return s0c[chosen_slot];
		} else if (s2c_num != 0) {
			if (choose_bch_random_switch_)
				chosen_slot = m_uniformRandomVariable->GetInteger (0, s2c_num-1);
			else
				chosen_slot = 0;
			return s2c[chosen_slot];
		} else {

//	show_slot_occupation();
//	print_slot_status();

			return -1;
		}
	}

}

void
TdmaSatmac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
    if (hdr->IsSatmacData())
  	{
		if (m_start_delay_frames > 0)
			return;
		recvFI(packet);
		SlotGroupHeader sg_hdr;
		SlotGroupTag tag;
		if(packet->PeekPacketTag(tag))
		{
			//std::cout<<"recv a sgi "<<std::endl;
			merge_sgi(sg_hdr);
		}
		recv_fi_count_++;
  	}
	else
	{
		ForwardUp (packet, hdr);
	}
}

void
TdmaSatmac::ForwardUp (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  //NotifyRx(packet);

	m_upCallback (packet, hdr);
}


/**
 * 把收到的FI包解序列化后存到received_fi_list_中。
 */
void TdmaSatmac::recvFI(Ptr<Packet> p){
	unsigned int bit_pos=7, byte_pos=0;
 	unsigned long value=0;
	unsigned int recv_fi_frame_fi = 0;
	unsigned int i=0;
	unsigned int tmp_sti;

	//unsigned int bit_remain,index;
	Frame_info *fi_recv;
	satmac::FiHeader fihdr;
	//unsigned char buffer = p->accessdata();
	p->RemoveHeader(fihdr);

	value = fihdr.decode_value(byte_pos,bit_pos,BIT_LENGTH_STI);
	tmp_sti = (unsigned int)value;

	value = fihdr.decode_value(byte_pos,bit_pos,BIT_LENGTH_FRAMELEN);
	if (adj_frame_ena_)
		recv_fi_frame_fi = pow(2, value);
	else
		recv_fi_frame_fi = m_frame_len;
	
	fi_recv = this->get_new_FI(recv_fi_frame_fi);
	fi_recv->sti = tmp_sti;
	fi_recv->frame_len = recv_fi_frame_fi;
	fi_recv->recv_slot = this->slot_count_;
	//fi_recv->type = TYPE_FI;

	fi_recv->valid_time = this->m_frame_len;
	fi_recv->remain_time = fi_recv->valid_time;

//
//	for (int j = 0; j < tlen; j++)
//		printf("%x ", buffer[j]);
//	printf("\n");

	for(i=0; i<(unsigned int)recv_fi_frame_fi; i++){
		fihdr.decode_slot_tag(byte_pos, bit_pos, i, fi_recv);
	}

//	NS_LOG_DEBUG("slot "<<slot_count_<<" node "<<global_sti<<" recv a FI from node "<<fi_recv->sti<<": ");
//	for(i=0; i<(unsigned int)recv_fi_frame_fi; i++){
//		slot_tag* fi=fi_recv->slot_describe;
//		NS_LOG_DEBUG("|"<<fi[i].sti<<" b:"<<fi[i].busy<<" c:"<<fi[i].count_2hop);
//	}
#ifdef PRINT_SLOT_STATUS
//    std::ofstream out (m_traceOutFile, std::ios::app);
//    //*m_log_lpf_stream->GetStream()
//	out << "Time:"<<Simulator::Now().GetMicroSeconds()<<"  slot "<<slot_count_<<", node "<<global_sti<<
//			"recv a FI from node "<<fi_recv->sti<<": " ;
//	for(i=0; i<(unsigned int)recv_fi_frame_fi; i++){
//		slot_tag* fi=fi_recv->slot_describe;
//		out << "|"<<fi[i].sti<<" b:"<<fi[i].busy<<" c:"<<fi[i].count_2hop<<" ";
//	}
//	out<<std::endl;
//	out.close ();
	printf("Time: %ld  ", Simulator::Now().GetMicroSeconds());
	printf("slot %d, node %d recv a FI from node %d, loc: %.1f,%.1f", slot_count_, global_sti, fi_recv->sti,
			(this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().x,(this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().y);
	for(i=0; i<(unsigned int)recv_fi_frame_fi; i++){
		slot_tag* fi=fi_recv->slot_describe;
		printf("|%d b:%d c:%d ", fi[i].sti, fi[i].busy, fi[i].count_2hop);
	}
	printf("\n");


#endif
	return;
}

void TdmaSatmac::merge_fi(Frame_info* base, Frame_info* append, Frame_info* decision){
	int count=0;
	slot_tag *fi_local_ = base->slot_describe;
	slot_tag *fi_append = append->slot_describe;
	slot_tag recv_tag;
	int recv_fi_frame_len = append->frame_len;

//	printf("I'm n%d, start merge fi from n %d\n", global_sti,append->sti);
	// status of our BCH should be updated first.
	for (count=0; count < m_frame_len; count++){
		recv_tag = fi_append[count];
		if (count == recv_fi_frame_len)
			break;
		
		if (fi_local_[count].sti == global_sti ) {//我自己的时隙
//			if (count != slot_num_ && count != slot_adj_candidate_) {
////				printf("I'm node %d, I recv a strange pkt..\n",global_sti);
//				continue;
//			}
			if (fi_local_[count].sti != recv_tag.sti && recv_tag.sti != 0) {//FI记录的id和我不一致
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						if (recv_tag.psf > fi_local_[count].psf) {
							fi_local_[count].life_time = slot_lifetime_frame_;
							fi_local_[count].sti = recv_tag.sti;
							fi_local_[count].count_2hop ++;
							fi_local_[count].count_3hop += recv_tag.count_2hop;
							if (recv_tag.sti == append->sti) { //FI发送者是该时隙的占有者
								fi_local_[count].busy = SLOT_1HOP;
							} else {
								fi_local_[count].busy = SLOT_2HOP;						
							}
						} else if (recv_tag.psf == fi_local_[count].psf) {
							fi_local_[count].busy = SLOT_COLLISION;
						}
						break;
					case SLOT_2HOP:
						fi_local_[count].count_3hop += recv_tag.count_2hop;
						break;
					case SLOT_FREE:
						//出现了隐藏站
						fi_local_[count].busy = SLOT_COLLISION;
						break;
					case SLOT_COLLISION:
						fi_local_[count].life_time = slot_lifetime_frame_;
						fi_local_[count].sti = recv_tag.sti;
						fi_local_[count].count_2hop = 1;
						fi_local_[count].count_3hop = 1;
						fi_local_[count].busy = SLOT_2HOP;
						break;
				}
			} else if (fi_local_[count].sti == recv_tag.sti){ //FI记录的id和我一致
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
//						if (recv_tag.count_2hop > 1)
//							fi_local_[count].count_3hop += recv_tag.count_2hop;
						break;
					case SLOT_2HOP:
//						//出现了隐藏站
//						fi_local_[count].busy = SLOT_COLLISION;
						break;
					case SLOT_FREE:
						//出现了隐藏站
						fi_local_[count].busy = SLOT_COLLISION;
						break;
					case SLOT_COLLISION:
						break;
				}
			} else { //STI-slot == 0
				if (recv_tag.busy == SLOT_FREE && count != slot_adj_candidate_) {
					if (!isNewNeighbor(append->sti)) {
						//出现了隐藏站
						fi_local_[count].busy = SLOT_COLLISION;
					}
				} else {
					//error state.
				}
			}
		}
	}

	//遍历每一个时隙
	for (count=0; count < ((recv_fi_frame_len > m_frame_len)?recv_fi_frame_len:m_frame_len); count++){
		if (count == recv_fi_frame_len)
			break;

		if (count >= m_frame_len ) {
			if (fi_local_[count].sti != 0)
				printf("merge_fi: node %d Protocol ERROR!!\n", global_sti);
		}

 		if (fi_local_[count].locker == 1)
			continue;

		//merge the recv_tag to fi_local_[slot_pos]
		recv_tag = fi_append[count];
		if (fi_local_[count].sti == global_sti || recv_tag.sti == global_sti)
			continue;
		else if (fi_local_[count].busy == SLOT_1HOP && fi_local_[count].sti != global_sti) {//直接邻居占用
			if (fi_local_[count].sti != recv_tag.sti && recv_tag.sti != 0) {
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						if (recv_tag.sti == append->sti) { //FI发送者是该时隙的占有者
							if (recv_tag.psf > fi_local_[count].psf) {
								fi_local_[count].life_time = slot_lifetime_frame_;
								fi_local_[count].sti = recv_tag.sti;
								fi_local_[count].count_2hop ++;
								fi_local_[count].count_3hop += recv_tag.count_2hop;
								fi_local_[count].busy = SLOT_1HOP;
							} else if (recv_tag.psf == fi_local_[count].psf) {
								fi_local_[count].life_time = slot_lifetime_frame_;
								fi_local_[count].busy = SLOT_COLLISION;
							}
						} else {
							fi_local_[count].count_2hop ++;
							fi_local_[count].count_3hop += recv_tag.count_2hop;
						}
						break;
					case SLOT_2HOP:
						fi_local_[count].count_3hop += recv_tag.count_2hop;
						break;
					case SLOT_FREE:
						break;
					case SLOT_COLLISION:
						fi_local_[count].life_time = slot_lifetime_frame_;
						fi_local_[count].sti = recv_tag.sti;
						fi_local_[count].count_2hop = 1;
						fi_local_[count].count_3hop = 1;
						fi_local_[count].busy = SLOT_2HOP;
						break;
				}
			} else if (fi_local_[count].sti == recv_tag.sti){ //FI记录的id和我一致
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						if (recv_tag.sti == append->sti) { //FI发送者是该时隙的占有者
								fi_local_[count].life_time = slot_lifetime_frame_;
							if (fi_local_[count].c3hop_flag == 0) {
								fi_local_[count].count_2hop ++;
								fi_local_[count].count_3hop += recv_tag.count_2hop;
								fi_local_[count].c3hop_flag = 1;
							}
						} else {
							fi_local_[count].existed = 1;
							// do nothing.
						}

						break;
					case SLOT_2HOP:
					case SLOT_FREE:
					case SLOT_COLLISION:
						break;
				}
			} else { //STI-slot == 0
				if (append->sti == fi_local_[count].sti) {
					fi_local_[count].life_time = 0;
					fi_local_[count].sti = 0;
					fi_local_[count].count_2hop = 0;
					fi_local_[count].count_3hop = 0;
					fi_local_[count].busy = SLOT_FREE;
					fi_local_[count].locker = 1;
 				}
			}
		}else if (fi_local_[count].busy == SLOT_2HOP) {//两跳邻居占用
			if (fi_local_[count].sti != recv_tag.sti && recv_tag.sti != 0) {
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						fi_local_[count].count_2hop ++;
						fi_local_[count].count_3hop += recv_tag.count_2hop;
						break;
					case SLOT_2HOP:
					case SLOT_FREE:
						break;
					case SLOT_COLLISION:
						fi_local_[count].life_time = slot_lifetime_frame_;
						fi_local_[count].sti = recv_tag.sti;
						fi_local_[count].count_2hop = 1;
						fi_local_[count].count_3hop = 1;
						fi_local_[count].busy = SLOT_2HOP;
						break;
				}	
			} else if (fi_local_[count].sti == recv_tag.sti){ //FI记录的id和我一致
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						if (recv_tag.sti == append->sti) { //FI发送者是该时隙的占有者
							fi_local_[count].busy = SLOT_1HOP;
							fi_local_[count].life_time = slot_lifetime_frame_;
							if (fi_local_[count].c3hop_flag == 0) {
								fi_local_[count].c3hop_flag = 1;
								fi_local_[count].count_2hop ++;
								fi_local_[count].count_3hop += recv_tag.count_2hop;
							}
						} else {
							fi_local_[count].life_time = slot_lifetime_frame_;
							if (fi_local_[count].c3hop_flag == 0) {
								fi_local_[count].c3hop_flag = 1;
								fi_local_[count].count_2hop ++;
								fi_local_[count].count_3hop += recv_tag.count_2hop;
							}
						}
						break;
					case SLOT_2HOP:
					case SLOT_FREE:
					case SLOT_COLLISION:		
						break;
				}				
			} else { //STI-slot == 0
				if (append->sti == fi_local_[count].sti) {
					fi_local_[count].life_time = 0;
					fi_local_[count].sti = 0;
					fi_local_[count].count_2hop = 0;
					fi_local_[count].count_3hop = 0;
					fi_local_[count].busy = SLOT_FREE;
					fi_local_[count].locker = 1;
				}
			}
		} else if (fi_local_[count].busy == SLOT_FREE && fi_local_[count].sti == 0){ //空闲时隙
			if (fi_local_[count].sti != recv_tag.sti) {
				switch (recv_tag.busy)
				{
					case SLOT_1HOP:
						fi_local_[count].life_time = slot_lifetime_frame_;
						fi_local_[count].sti = recv_tag.sti;
						fi_local_[count].count_2hop = 1;
						fi_local_[count].count_3hop = recv_tag.count_2hop;
						fi_local_[count].c3hop_flag = 1;
						if (recv_tag.sti == append->sti) { //FI发送者是该时隙的占有者
							fi_local_[count].busy = SLOT_1HOP;
						} else {
							fi_local_[count].busy = SLOT_2HOP;						
						}
						break;
					case SLOT_2HOP:
						fi_local_[count].count_3hop += recv_tag.count_2hop;
						break;
					case SLOT_FREE:
						break;
					case SLOT_COLLISION:
						fi_local_[count].life_time = slot_lifetime_frame_;
						fi_local_[count].sti = recv_tag.sti;
						fi_local_[count].count_2hop = 1;
						fi_local_[count].count_3hop = 1;
						fi_local_[count].busy = SLOT_2HOP;
						break;
				}	
			}
		}

		if (count >= m_frame_len && fi_local_[count].sti != 0) {
			NS_LOG_DEBUG("I'm node "<<global_sti<<" I restore frame len from "<<m_frame_len<<" to "<<recv_fi_frame_len);
			m_frame_len = recv_fi_frame_len;
		}
	}
	return;
}

bool TdmaSatmac::isSingle(void) {
	slot_tag *fi_local = this->collected_fi_->slot_describe;
	int count;
	for (count=0; count < m_frame_len; count++){
		if (fi_local[count].sti != 0 && fi_local[count].sti != global_sti)
			return false;
	}
	return true;
}

/*
 * reduce the remain_time of each of received_fi_list_
 * if the argument time ==0 then clear the received_fi_list_;
 */
void TdmaSatmac::fade_received_fi_list(int time){
	Frame_info *current, *previous;
	current=this->received_fi_list_;
	previous=NULL;

	while(current != NULL){
		current->remain_time -= time;
		if(current->remain_time <= 0 || time == 0){
			if(previous == NULL){
				this->received_fi_list_ = current->next_fi;
				delete current;
				current = this->received_fi_list_;
				continue;
			}
			else{
				previous->next_fi= current->next_fi;
				delete current;
				current = previous->next_fi;
				continue;
			}
		}
		else{
			previous = current;
			current = current->next_fi;
		}
	}
}
void TdmaSatmac::synthesize_fi_list(){
	Frame_info * processing_fi = received_fi_list_;
	int count;
	slot_tag *fi_local = this->collected_fi_->slot_describe;
	bool unlock_flag = 0;

	if (node_state_ != NODE_LISTEN && slot_memory_) {
		for (count=0; count < m_frame_len; count++){
			if (fi_local[count].locker && fi_local[count].sti != 0) {
				fi_local[count].locker = 0; //the locker must be locked in the last frame.

			} else if (fi_local[count].locker)
				unlock_flag = 1;

			if ((fi_local[count].sti == global_sti && (count == slot_num_ || count == slot_adj_candidate_))
					|| fi_local[count].sti == 0)
				continue;
			if (fi_local[count].life_time > 0)
				fi_local[count].life_time--;

			if (fi_local[count].life_time == 0) {
				if (fi_local[count].busy == SLOT_2HOP) {
					fi_local[count].busy = SLOT_FREE;
					fi_local[count].sti = 0;
					fi_local[count].count_2hop = 0;
					fi_local[count].count_3hop = 0;
					fi_local[count].psf = 0;
					fi_local[count].c3hop_flag = 0;
					fi_local[count].life_time = 0;
					fi_local[count].locker = 0;
				} else if (fi_local[count].busy == SLOT_1HOP && fi_local[count].existed == 1) {
					fi_local[count].busy = SLOT_2HOP;
					fi_local[count].life_time = slot_lifetime_frame_-1;
					fi_local[count].locker = 0;
				} else  {
					fi_local[count].busy = SLOT_FREE;
					fi_local[count].sti = 0;
					fi_local[count].count_2hop = 0;
					fi_local[count].count_3hop = 0;
					fi_local[count].psf = 0;
					fi_local[count].c3hop_flag = 0;
					fi_local[count].life_time = 0;
					fi_local[count].locker = 1; // lock the status for one frame.
				}

			} else if (fi_local[count].busy != SLOT_COLLISION
					&& fi_local[count].life_time == 0) {

				fi_local[count].busy = SLOT_FREE;
			}

			fi_local[count].existed = 0;
		}
	}

	while(processing_fi != NULL){
		merge_fi(this->collected_fi_, processing_fi, this->decision_fi_);
		processing_fi = processing_fi->next_fi;
	}

	if (unlock_flag) {
		for (count=0; count < m_frame_len; count++){
			if (fi_local[count].locker && fi_local[count].sti == 0) {
				fi_local[count].locker = 0; //the locker must be locked in the last frame.
			}
		}
	}
	
	print_slot_status();

}

bool TdmaSatmac::adjust_is_needed(int slot_num) {
	slot_tag *fi_collection = this->collected_fi_->slot_describe;
	int i,free_count_ths = 0, free_count_ehs = 0;

	int s0_1c_num = 0;

	for(i=0 ; i < m_frame_len; i++){
		if (fi_collection[i].busy== SLOT_FREE)
			free_count_ths++;  //两跳范围内空闲时隙数量
		if(fi_collection[i].busy== SLOT_FREE && fi_collection[i].count_3hop == 0) 
		{
			free_count_ehs++;//三跳范围内空闲时隙数量
			if (i < m_frame_len/2)
				s0_1c_num++;
		}
	}

	// if(node_state_ == NODE_WORK_FI)
	// {
	// 	std::cout<<(((float)(m_frame_len - free_count_ths))/m_frame_len)<<std::endl;
	// 	std::cout<<this->get_available_slot_group_ratio()<<std::endl;
	// }

	if (adj_ena_ && fi_collection[slot_num].count_3hop >= c3hop_threshold_ && free_count_ehs >= adj_free_threshold_) 
	{
		return true;
	} 
	else if (adj_frame_ena_ && slot_num >= m_frame_len/2
			&& m_frame_len > adj_frame_lower_bound_
			&& (((float)(m_frame_len - free_count_ehs))/m_frame_len) <= frameadj_cut_ratio_ehs_
			&& (((float)(m_frame_len - free_count_ths))/m_frame_len) <= frameadj_cut_ratio_ths_
			&& s0_1c_num != 0)
		return true;
	else if(adj_ena_sg 
			&&  (((float)(m_frame_len - free_count_ths))/m_frame_len) > adjRatio_low_sg
			&&  (((float)(m_frame_len - free_count_ths))/m_frame_len) < adjRatio_high_sg
			//&& this->get_available_slot_group_ratio() <= 0.5
			&& isNeedSg)
		{
			isNeed_adj_Sg = 1;
			return true;
		}
	else
		return false;
}


void TdmaSatmac::adjFrameLen()
{
	if (!adj_frame_ena_)
		return;
	//calculate slot utilization
	int i;
	int free_count_ths = 0, free_count_ehs = 0;
	float utilrate_ths, utilrate_ehs;
	bool cutflag = true;
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
	for(i=0 ; i < m_frame_len; i++)
	{
		if(fi_local_[i].busy != SLOT_FREE)
		{
			if (i >= m_frame_len/2)
				cutflag = false;
		}
		if (fi_local_[i].busy== SLOT_FREE)
			free_count_ths++;
		if(fi_local_[i].busy== SLOT_FREE && fi_local_[i].count_3hop == 0)
			free_count_ehs++;
	}
	//TODO
	bool slotGroupUsed;
	for(int i =0; i < m_frame_len / slot_group_length; i++)
	{
		if(m_sg_info[i].sg_busy != SLOT_GROUP_FREE)
		{
			if(i >= m_frame_len/slot_group_length/2)
			{
				slotGroupUsed = true;
			}
		}
	}

	utilrate_ths = (float)(m_frame_len - free_count_ths)/m_frame_len;
	utilrate_ehs = (float)(m_frame_len - free_count_ehs)/m_frame_len;
	if (free_count_ths <= adj_free_threshold_)
		utilrate_ths = 1;
	if (free_count_ehs <= adj_free_threshold_)
		utilrate_ehs = 1;

	if (utilrate_ehs >= frameadj_exp_ratio_ && m_frame_len < adj_frame_upper_bound_)
	{
		m_frame_len *= 2;
	}
	else if (cutflag
			&& utilrate_ths <= frameadj_cut_ratio_ths_
			&& utilrate_ehs <= frameadj_cut_ratio_ehs_
			&& m_frame_len > adj_frame_lower_bound_
			&& !slotGroupUsed)
	{
		m_frame_len /= 2;
	}

	switch (m_frame_len) {
	case 16:
		adj_free_threshold_ = 3;
		break;
	case 32:
		adj_free_threshold_ = 3;
		break;
	case 64:
		adj_free_threshold_ = 4;
		break;
	case 128:
		adj_free_threshold_ = 5;
		break;
	default:
		adj_free_threshold_ = 5;
	}
}


Time
TdmaSatmac::CalculateTxTime (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (*packet);
  NS_ASSERT_MSG (packet->GetSize () < 1500,"PacketSize must be less than 1500B, it is: " << packet->GetSize ());
  return m_bps.CalculateBytesTxTime (packet->GetSize ());
}

void TdmaSatmac::generate_send_FI_packet(){
#ifdef PRINT_SLOT_STATUS

	std::cout<<"Time "<<Simulator::Now().GetMicroSeconds()<<" I'm node "<<global_sti<<" in slot "<<slot_count_<<
			", I send an FI, loc:" << (this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().x << ", " <<
			(this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().y <<
			" channel util: " << get_channel_utilization() <<std::endl;

//	if(global_sti == 9) {
//		double t = get_channel_utilization() ;
//		std::cout<< t << std::endl;
//		std::cout<< std::endl;
//		std::cout<< std::endl;
//	}
#endif
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
	Ptr<Packet> p = Create<Packet> ();
	satmac::FiHeader fihdr(m_frame_len, global_sti, fi_local_);
	p->AddHeader (fihdr);
	WifiMacHeader wifihdr;
	wifihdr.SetNoMoreFragments();
	wifihdr.SetType (WIFI_MAC_SATMAC);
	wifihdr.SetAddr1 (Mac48Address::GetBroadcast());
	wifihdr.SetAddr2 (GetAddress ());
	if (m_wifimaclow_flag)
	{
		wifihdr.SetAddr3 (m_wifimaclow->GetAddress());
	} else
		wifihdr.SetAddr3 (m_low->GetAddress ());
	wifihdr.SetDsFrom ();
	wifihdr.SetDsNotTo ();

	if(slotgroup_ena)
	{
		sg_opration(p);
	}

	if (m_wifimaclow_flag)
	{
		SendFiDown(p, wifihdr);
	} else {
		Time packetTransmissionTime = CalculateTxTime (p);
		m_slotRemainTime = m_slotTime - packetTransmissionTime.GetMicroSeconds ();

		NS_LOG_DEBUG ("FI TransmissionTime(microSeconds): " << packetTransmissionTime.GetMicroSeconds () << "usec");
		NS_ASSERT_MSG(packetTransmissionTime < MicroSeconds (m_slotTime), "FATAL: cannot transmit FI.");

		Simulator::Schedule (packetTransmissionTime, &TdmaSatmac::SendFiDown, this, p, wifihdr);
	}

	send_fi_count_++;
}

void TdmaSatmac::WaitWifiState()
{
	if (m_transmissionListener->isTxok())
//	if (!(this->getWifiPhy()->IsStateTx ()) && !(this->getWifiPhy()->IsStateSwitching ()))
	{
		m_transmissionListener->setTxok(false);
		switch (this->node_state_)
		{
		case NODE_WORK_FI:
		case NODE_WORK_ADJ:
			StartTransmission(m_slotRemainTime);
			break;
		default: break;
		}
	} else {
		m_slotRemainTime -= 50;
		Simulator::Schedule (MicroSeconds(50), &TdmaSatmac::WaitWifiState, this);
	}
}

  void 
  TdmaSatmac::SendSgiDown(Ptr<Packet> packet, WifiMacHeader header)
  {
		MacLowTransmissionParameters params;
		//params.DisableOverrideDurationId ();
		params.DisableRts ();
		params.DisableAck ();
		params.DisableNextData ();
		//std::cout<<"send a sgi at "<<Simulator::Now().GetMicroSeconds()<<std::endl;
		m_wifimaclow->StartTransmission (packet,
											&header,
											params,
											m_transmissionListener);

  }


void
TdmaSatmac::SendFiDown (Ptr<Packet> packet, WifiMacHeader header)
{
  if (m_wifimaclow_flag)
  {
	  MacLowTransmissionParameters params;
	  //params.DisableOverrideDurationId ();
	  params.DisableRts ();
	  params.DisableAck ();
	  params.DisableNextData ();
	  WifiTxVector txVector = this->getWifiMacLow()->GetDataTxVector (packet, &header);
	  //Time txDuration = this->getWifiPhy()->CalculateTxDuration (packet->GetSize (), txVector,  this->getWifiPhy()->GetFrequency ());
	  Time fiTxDuration = this->getWifiPhy()->CalculateTxDuration (GetSize (packet, &header, false), txVector, this->getWifiPhy()->GetFrequency ());
	  fiTxDuration += this->getWifiPhy()->GetGuardInterval();
//			  this->getWifiPhy()->CalculateTxDuration (packet->GetSize (), txVector, WIFI_PREAMBLE_LONG, this->getWifiPhy()->GetFrequency ());
//	  txDuration += this->getWifiMacLow()->GetSifs();
	  m_slotRemainTime -= fiTxDuration.GetMicroSeconds();
	  m_wifimaclow->StartTransmission (packet,
	                                 &header,
	                                 params,
	                                 m_transmissionListener);

	  switch (this->node_state_)
	  {
	  case NODE_WORK_FI:
	  case NODE_WORK_ADJ:
		Simulator::Schedule (fiTxDuration, &TdmaSatmac::StartTransmission, this,m_slotRemainTime);
	  	break;
	  default: break;
	  }
//	  WaitWifiState();
  } else {
	  m_low->StartTransmission (packet, &header);
	  NotifyTx (packet);
	  switch (this->node_state_)
	  {
		case NODE_WORK_FI:
		case NODE_WORK_ADJ:
			StartTransmission(m_slotRemainTime);
			break;
		default: break;
	  }
  }
}

void
TdmaSatmac::StartTransmission (uint64_t transmissionTimeUs)
{
  NS_LOG_DEBUG (transmissionTimeUs << " usec");

  Time totalTransmissionSlot = MicroSeconds (transmissionTimeUs);
  if (m_queue->IsEmpty ())
    {
      NS_LOG_DEBUG ("queue empty");
      return;
    }

  //printf("Time: %ld  ", Simulator::Now().GetMicroSeconds());
  //printf("I'm node %d, I start send data pkt\n",global_sti);

  WifiMacHeader header;
  Ptr<const Packet> peekPacket = m_queue->Peek (&header);

  if (m_wifimaclow_flag)
  {
	  WifiTxVector txVector = this->getWifiMacLow()->GetDataTxVector (peekPacket, &header);
	  Time txDuration = this->getWifiPhy()->CalculateTxDuration (GetSize (peekPacket, &header, false), txVector, this->getWifiPhy()->GetFrequency ());

//	  txDuration += this->getWifiMacLow()->GetSifs();
	  if (m_slotRemainTime >= txDuration.GetMicroSeconds())
	  {
		  m_slotRemainTime -= txDuration.GetMicroSeconds();
		  m_lastpktUsedTime = txDuration.GetMicroSeconds();
		  SendPacketDown(MicroSeconds(m_slotRemainTime));
	  } else {
		  //std::cout << "Packet takes more time to transmit than the slot allotted. Will send in next slot" << std::endl;
		  BsmTimeTag tag;
		  if(peekPacket->PeekPacketTag(tag))
		  {
				uint32_t delta = Simulator::Now().GetMicroSeconds() - tag.getSendingTimeUs();
				if(delta > 100000)
				{
					//std::cout<<delta<<std::endl;
					m_queue->Dequeue(&header);
				}

		  }

		  NS_LOG_DEBUG ("Packet takes more time to transmit than the slot allotted. Will send in next slot");
	  }
  } else {
	  Time packetTransmissionTime = CalculateTxTime (peekPacket);
	  m_lastpktUsedTime = packetTransmissionTime.GetMicroSeconds();
	  NS_LOG_DEBUG ("Packet TransmissionTime(microSeconds): " << packetTransmissionTime.GetMicroSeconds () << "usec");
	  if (packetTransmissionTime < totalTransmissionSlot)
		{
		  totalTransmissionSlot -= packetTransmissionTime;
		  m_slotRemainTime -= packetTransmissionTime.GetMicroSeconds ();
		  Simulator::Schedule (packetTransmissionTime, &TdmaSatmac::SendPacketDown, this,totalTransmissionSlot);
		}
	  else
		{
		  NS_LOG_DEBUG ("Packet takes more time to transmit than the slot allotted. Will send in next slot");
		}
  }
}



void
TdmaSatmac::SendPacketDown (Time remainingTime)
{
  WifiMacHeader header;
  Ptr<const Packet> packet = m_queue->Dequeue (&header);
  //std::cout<<this->getNodePtr()->GetId()<<" Time:  "<<Simulator::Now().GetMicroSeconds()<<std::endl;
  	if (m_wifimaclow_flag)
  {
	  MacLowTransmissionParameters params;
//	  params.DisableOverrideDurationId ();
	  params.DisableRts ();

//	  if (header.GetAddr1 ().IsGroup())
//	  {
//		  params.DisableAck();
//	  } else
//		  params.EnableAck ();
	  params.DisableAck ();
//	  params.DisableNextData ();
	  header.SetDuration(MicroSeconds(m_lastpktUsedTime));
	  //std::cout<<"1"<<std::endl;
	  m_wifimaclow->StartTransmission (packet,
	                                 &header,
	                                 params,
	                                 m_transmissionListener);

	//  std::cout<<"satmac send a pkt Size "<<m_lastpktUsedTime <<" fromSti = " << this->GetGlobalSti()<<" to addr "<<header.GetAddr1 () << " from addr "<< header.GetAddr2 ()<< " Time "<< Simulator::Now().GetMicroSeconds()<< std::endl;
  } else
	  m_low->StartTransmission (packet, &header);
//  TxQueueStart (0);
  NotifyTx (packet);
//  TxQueueStart (0);
  if (m_wifimaclow_flag)
  {
//	  Time txDuration = this->getWifiMacLow()->GetLastPktTxDuration();
//	  m_slotRemainTime -=
	  Simulator::Schedule (MicroSeconds(m_lastpktUsedTime), &TdmaSatmac::StartTransmission, this,remainingTime.GetMicroSeconds ());
  } else
	  StartTransmission (remainingTime.GetMicroSeconds ());
}

double TdmaSatmac::get_channel_utilization()
{
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
	int count = 0;
	for(int i=0 ; i < m_frame_len; i++){
		if(fi_local_[i].busy != SLOT_FREE) {
			count++;
		}
	}

    // Calculate the maximum consecutive free slots for each slot group
    for (int i = 0; i < m_frame_len / slot_group_length; i++)
    {
        if (m_sg_info[i].geohash != -1 && m_sg_info[i].sg_busy != SLOT_GROUP_FREE && m_sg_info[i].t_valid > 0)
        {
            int max_consecutive_free = 0;
            int current_consecutive_free = 0;

            // Iterate over each slot in the slot group
            for (int j = i * slot_group_length; j < (i + 1) * slot_group_length; j++)
            {
                if (fi_local_[j].busy == SLOT_FREE)
                {
                    current_consecutive_free++;
                    if (current_consecutive_free > max_consecutive_free)
                    {
                        max_consecutive_free = current_consecutive_free;
                    }
                }
                else
                {
                    current_consecutive_free = 0;
                }
            }
            // Add the maximum consecutive free slots to the count
            count += max_consecutive_free;
        }
    }

	return ((double)count)/((double)m_frame_len);
}

int getdir(Vector cur, Vector last)
{
	double deltax = cur.x - last.x;
	double deltay = cur.y - last.y;
	if (deltax < 1.0 || deltax > -1.0) {
		if (deltay>0) return 1;
		else return 0;
	} else {
		if (deltax>0) return 1;
		else return 0;
	}
	return 1;
}

void
TdmaSatmac::slotHandler ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if(this->getNodePtr()->GetId() == 0)
  {
	return;
  }
  slot_tag *fi_collection = this->collected_fi_->slot_describe;
  // Restart timer for next slot.
  total_slot_count_ = total_slot_count_+1;
  slot_count_ = total_slot_count_ %  m_frame_len;
  Simulator::Schedule (GetSlotTime(), &TdmaSatmac::slotHandler, this);

  //std::cout<<"slot_count_"<<slot_count_<<std::endl;

//  for(int i = 0;i < m_frame_len; i++)
//  {
//	  std::cout<<"slot "<<i<<" node "<<fi_collection[i].sti-1<<std::endl;
//  }

//  std::cout<<"Start time:" << Simulator::Now().GetMicroSeconds() << std::endl;

  if (vemac_mode_ == 1 && location_initialed_ == false) {
		Mobility_ = m_nodePtr->GetObject<MobilityModel> ();
		location_initialed_ = true;
		curr_location_ = Mobility_->GetPosition();
		last_location_ = curr_location_;
		return;
  		//Vector dir = model->GetVelocity();

		//printf("%f, %f, %f, %d\n", current_loc.x, current_loc.y, current_loc.z, direction_);
  }

  if (vemac_mode_ == 1 && direction_initialed_ == false)
  {
	    curr_location_ = m_nodePtr->GetObject<MobilityModel> ()->GetPosition();
		if (curr_location_.x == last_location_.x && curr_location_.y == last_location_.y) {


			  synthesize_fi_list();
			  this->fade_received_fi_list(1);
			return;
		}
		direction_initialed_ = true;
		direction_ = getdir(curr_location_, last_location_);
		last_location_ = curr_location_;
  }
  m_slotRemainTime = m_slotTime;

  slot_state_ = BEGINING;

  this->fade_received_fi_list(1);
/*
  if(this->enable == 0){
	  double x,y,z;
	  ((CMUTrace *)this->downtarget_)->getPosition(&x,&y,&z);
	  if(z == 0){
		  this->enable = 1;
		  slot_num_ = (slot_count_+1)% m_frame_len; //slot_num_初始化为当前的下一个时隙。
	  }
  }
  else if(this->enable == 1){
	  double x,y,z;
	  ((CMUTrace *)this->downtarget_)->getPosition(&x,&y,&z);
	  if(z > 0 ){
		  this->enable = 0;
		  //slot_num_ = (slot_count_+1)% m_frame_len;
	  }
  }
*/

//  if(this->enable == 0){
//	  return;
//  }

  if (Simulator::Now().GetMilliSeconds() - last_log_time_.GetMilliSeconds() >= 1000) {
	  last_log_time_ = Simulator::Now();

	  /**
	   * LPF
	   */
	  double x = (this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().x;
	  double y = (this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().y;
      //std::ofstream out (m_traceOutFile, std::ios::app);
      //*m_log_lpf_stream->GetStream()
//	  out << "m "<<(Simulator::Now ()).GetMilliSeconds ()<<" t["<<slot_num_<<"] _"<<global_sti<<
//	  	"_ LPF "<<waiting_frame_count<<" "<<request_fail_times<<" "<<collision_count_<<" "<<
//	  	frame_count_<<" "<<continuous_work_fi_max_<<" "<<adj_count_success_<<" "<<
//	  	adj_count_total_<<" "<<send_fi_count_<<" "<<recv_fi_count_<<" "<<
//	  	no_avalible_count_<<" "<<slot_num_<<" "<<m_frame_len<<" "<<localmerge_collision_count_
//		<<" "<<"x:" << x <<" "<<"y"<< y <<" "<<get_channel_utilization()<<std::endl;

	uint32_t numNodes = NodeList::GetNNodes();
	// 获取 run_num
      int run_num = RunNumber::GetInstance().GetRunNum();
	  std::__cxx11::string channel_file =  "channel_utilization_run" + std::to_string(run_num) + ".txt";

      //std::ofstream out (m_channel_utilization_OutFile, std::ios::app);
	  std::ofstream out (channel_file, std::ios::app);
	  
      out << get_channel_utilization() <<std::endl;

	  out.close ();
  }

  NS_ASSERT_MSG (slot_num_ <= m_frame_len, "FATAL! slot_num_ > m_frame_len" << this->GetGlobalSti());

  if (slot_count_ == slot_num_){
	  frame_count_++;
	  if (m_start_delay_frames > 0) {
		  --m_start_delay_frames;
		  return;
	  }
	  //std::cout<<this->getNodePtr()->GetId()<<std::endl;

	  if (vemac_mode_ == 1 ) {

	  		curr_location_ = m_nodePtr->GetObject<MobilityModel> ()->GetPosition();
	  		//Vector dir = model->GetVelocity();
	  		direction_ = getdir(curr_location_, last_location_);
	  		last_location_ = curr_location_;
			//printf("%f, %f, %f, %d\n", current_loc.x, current_loc.y, current_loc.z, direction_);
	  }

	  switch (node_state_) {
	  case NODE_INIT:// the first whole slot of a newly initialized node, it begin to listen
		  node_state_ = NODE_LISTEN;
		  waiting_frame_count =0;
		  request_fail_times = 0;
		  collision_count_ = 0;
		  continuous_work_fi_ = 0;
		  continuous_work_fi_max_ = 0;
		  adj_count_success_ = 0;
		  adj_count_total_ = 0;
		  last_log_time_ = Simulator::Now();
		  no_avalible_count_ = 0;
		  backoff_frame_num_ = 0;
		  return;
	  case NODE_LISTEN:
		  waiting_frame_count++;

		  if (backoff_frame_num_) {
//			  printf("%d : %d\n",global_sti,backoff_frame_num_);
			  backoff_frame_num_--;
			  return;
		  }

		  //根据自己的fi-local，决定自己要申请的slot，修改自己的slot_num_
		  this->clear_FI(this->collected_fi_); //初始化
		  fi_collection = this->collected_fi_->slot_describe;
		  synthesize_fi_list();
		  slot_num_ = determine_BCH(0);
		  if(slot_num_ < 0){
			  node_state_ = NODE_LISTEN;
			  slot_num_ = slot_count_;
			  no_avalible_count_++;
			  backoff_frame_num_ = m_uniformRandomVariable->GetInteger (0, 20);
#ifdef PRINT_SLOT_STATUS
				printf("I'm node %d, in slot %d, NODE_LISTEN and I cannot choose a BCH!!\n", global_sti, slot_count_);
#endif
			  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_LISTEN and I cannot choose a BCH!!");

			  return;
		  }
#ifdef PRINT_SLOT_STATUS
			printf("I'm node %d, in slot %d, NODE_LISTEN, choose: %d\n", global_sti, slot_count_, slot_num_);
#endif
		  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_LISTEN, choose: "<<slot_num_);

		  //如果正好决定的时隙就是本时隙，那么直接发送
		  if(slot_num_== slot_count_){
			  fi_collection[slot_count_].busy = SLOT_1HOP;
			  fi_collection[slot_count_].sti = global_sti;
			  fi_collection[slot_count_].count_2hop = 1;
			  fi_collection[slot_count_].count_3hop = 1;
			  fi_collection[slot_count_].psf = 0;
			  generate_send_FI_packet(); //必须在BCH状态设置完之后调用。
			  node_state_ = NODE_REQUEST;
			  return;
		  }
		  else{//否则等待发送时隙
			  node_state_ = NODE_WAIT_REQUEST;
			  return;
		  }
		  break;
	  case NODE_WAIT_REQUEST:

		  waiting_frame_count++;
		  if (!slot_memory_) {
			  this->clear_others_slot_status();
			  fi_collection = this->collected_fi_->slot_describe;
		  }
		  synthesize_fi_list();
		  if((fi_collection[slot_count_].sti == global_sti && fi_collection[slot_count_].busy == SLOT_1HOP)
				  || fi_collection[slot_count_].sti == 0) {
			  fi_collection[slot_count_].busy = SLOT_1HOP;
			  fi_collection[slot_count_].sti = global_sti;
			  fi_collection[slot_count_].count_2hop = 1;
			  fi_collection[slot_count_].count_3hop = 1;
			  fi_collection[slot_count_].psf = 0;
			  generate_send_FI_packet();
			  node_state_ = NODE_REQUEST;
			  return;
		  } else {
			  if (fi_collection[slot_count_].sti == global_sti) {
				  fi_collection[slot_count_].busy = SLOT_FREE;
				  fi_collection[slot_count_].sti = 0;
				  fi_collection[slot_count_].count_2hop = 0;
				  fi_collection[slot_count_].count_3hop = 0;
				  fi_collection[slot_count_].psf = 0;
				  fi_collection[slot_count_].locker = 1;
			  }
			  request_fail_times++;


			  slot_num_ = determine_BCH(0);
			  if(slot_num_ < 0 || slot_num_== slot_count_){
				  node_state_ = NODE_LISTEN;
				  no_avalible_count_ ++;
				  backoff_frame_num_ = m_uniformRandomVariable->GetInteger (0, 20);
				  slot_num_ = slot_count_;
				  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_WAIT_REQUEST and I cannot choose a BCH!!");
#ifdef PRINT_SLOT_STATUS
					printf("I'm node %d, in slot %d, NODE_WAIT_REQUEST and I cannot choose a BCH!!\n", global_sti, slot_count_);
#endif
				  return;
			  }
#ifdef PRINT_SLOT_STATUS
				printf("I'm node %d, in slot %d, NODE_WAIT_REQUEST and current bch is unvalid, choose: %d\n", global_sti, slot_count_, slot_num_);
#endif
			  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_WAIT_REQUEST and current bch is unvalid, choose: "<<slot_num_);
			  node_state_ = NODE_WAIT_REQUEST;
			  return;
		  }
		  break;
	  case NODE_REQUEST:// or node_state_ = NODE_WORK;;
		  if (!slot_memory_) {
			  this->clear_others_slot_status();
			  fi_collection = this->collected_fi_->slot_describe;
		  }
		  synthesize_fi_list();
		  if((fi_collection[slot_count_].sti == global_sti && fi_collection[slot_count_].busy == SLOT_1HOP)
				  || fi_collection[slot_count_].sti == 0) {

			  node_state_ = NODE_WORK_FI;

			  generate_send_FI_packet();
		  } else {
			  waiting_frame_count++;
			  if (fi_collection[slot_count_].sti == global_sti) {
				  fi_collection[slot_count_].busy = SLOT_FREE;
				  fi_collection[slot_count_].sti = 0;
				  fi_collection[slot_count_].count_2hop = 0;
				  fi_collection[slot_count_].count_3hop = 0;
				  fi_collection[slot_count_].psf = 0;
				  fi_collection[slot_count_].locker = 1;
			  }

			  request_fail_times++;

			  slot_num_ = determine_BCH(0);
			  if(slot_num_ < 0 || slot_num_== slot_count_){
				  node_state_ = NODE_LISTEN;
				  no_avalible_count_ ++;
				  backoff_frame_num_ = m_uniformRandomVariable->GetInteger (0, 20);
				  slot_num_ = slot_count_;
#ifdef PRINT_SLOT_STATUS
					printf("I'm node %d, in slot %d, NODE_REQUEST and I cannot choose a BCH!!\n", global_sti, slot_count_);
#endif
				  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_REQUEST and I cannot choose a BCH!!");
				  return;
			  }
#ifdef PRINT_SLOT_STATUS
				printf("I'm node %d, in slot %d, NODE_REQUEST and current bch is unvalid, choose: %d\n", global_sti, slot_count_, slot_num_);
#endif
			  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_REQUEST and current bch is unvalid, choose: "<<slot_num_);
			  node_state_ = NODE_WAIT_REQUEST;
			  return;
		  }
		  break;
	  case NODE_WORK_FI:
		  if (!slot_memory_) {
			  this->clear_others_slot_status();
			  fi_collection = this->collected_fi_->slot_describe;
		  }
		  synthesize_fi_list();

		  if((fi_collection[slot_count_].sti == global_sti && fi_collection[slot_count_].busy == SLOT_1HOP)
				  || fi_collection[slot_count_].sti == 0)//BCH可用
		  {
			  continuous_work_fi_ ++;
			  continuous_work_fi_max_ = (continuous_work_fi_max_ > continuous_work_fi_)?continuous_work_fi_max_:continuous_work_fi_;
			  if (adjust_is_needed(slot_num_)) 
			  {
				if(isNeed_adj_Sg)
				{
					int result = adj_BCH_Sg();
					if(result != -1)
					{
						slot_adj_candidate_ = result;
						isNeed_adj_Sg = 0;
						// std::cout<<"I am node "<<this->getNodePtr()->GetId()<<" i adj BCH for sg to "<<slot_adj_candidate_
						// 					<<" slot_num_ = "<<slot_num_<<" Time: "<<Simulator::Now().GetMicroSeconds()
						// 					<<" available_slot_group_ratio: "<<get_available_slot_group_ratio()<<std::endl;
						// slot_tag *fi_local_= this->collected_fi_->slot_describe;
						// for(int i = 0;i < m_frame_len; i++)
						// {
						// 	std::cout<<"slot "<<i<<" node "<<fi_local_[i].sti-1<<std::endl;
						// }
					}
				}
				else
				{
				  slot_adj_candidate_ = determine_BCH(1);
				}
				  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_WORK_FI ADJ is needed! choose: "<<slot_adj_candidate_);
				  if (slot_adj_candidate_ >= 0) {
					  if (adj_single_slot_ena_) {
						  node_state_ = NODE_WORK_FI;
						  adj_count_success_++;
						  slot_num_ = slot_adj_candidate_;
						  fi_collection[slot_count_].busy = SLOT_FREE;
						  fi_collection[slot_count_].sti = 0;
						  fi_collection[slot_count_].count_2hop = 0;
						  fi_collection[slot_count_].count_3hop = 0;
						  fi_collection[slot_count_].psf = 0;
						  fi_collection[slot_count_].locker = 0;

						  fi_collection[slot_num_].busy = SLOT_1HOP;
						  fi_collection[slot_num_].sti = global_sti;
						  fi_collection[slot_num_].count_2hop = 1;
						  fi_collection[slot_num_].count_3hop = 1;
						  fi_collection[slot_num_].psf = 0;
					  } else {
						  node_state_ = NODE_WORK_ADJ;
						  adj_count_total_++;
						  fi_collection[slot_adj_candidate_].busy = SLOT_1HOP;
						  fi_collection[slot_adj_candidate_].sti = global_sti;
						  fi_collection[slot_adj_candidate_].count_2hop = 1;
						  fi_collection[slot_adj_candidate_].count_3hop = 1;
						  fi_collection[slot_adj_candidate_].psf = 0;
					  }
				  }
			  } else
				  adjFrameLen();
			  generate_send_FI_packet();

			  m_BCHTrace();

			  if (random_bch_if_single_switch_ && isSingle()) {
				  if (bch_slot_lock_-- == 0) {
					  slot_num_ = determine_BCH(0);
					  fi_collection[slot_count_].busy = SLOT_FREE;
					  fi_collection[slot_count_].sti = 0;
					  fi_collection[slot_count_].count_2hop = 0;
					  fi_collection[slot_count_].count_3hop = 0;
					  fi_collection[slot_count_].psf = 0;
					  fi_collection[slot_count_].locker = 1;

					  fi_collection[slot_num_].busy = SLOT_1HOP;
					  fi_collection[slot_num_].sti = global_sti;
					  fi_collection[slot_num_].count_2hop = 1;
					  fi_collection[slot_num_].count_3hop = 1;
					  fi_collection[slot_num_].psf = 0;

					  bch_slot_lock_ = 5;
				  }
			  } else
				  bch_slot_lock_ = 5;

		  } else {
			  continuous_work_fi_ = 0;
			  if (fi_collection[slot_count_].sti == global_sti) {
				  fi_collection[slot_count_].busy = SLOT_FREE;
				  fi_collection[slot_count_].sti = 0;
				  fi_collection[slot_count_].count_2hop = 0;
				  fi_collection[slot_count_].count_3hop = 0;
				  fi_collection[slot_count_].psf = 0;
				  fi_collection[slot_count_].locker = 1;
			  }

			  collision_count_++;

			  slot_num_ = determine_BCH(0);
			  if(slot_num_ < 0 || slot_num_== slot_count_){
				  node_state_ = NODE_LISTEN;
				  slot_num_ = slot_count_;
				  no_avalible_count_ ++;
				  backoff_frame_num_ = m_uniformRandomVariable->GetInteger (0, 20);
				  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_WORK_FI and I cannot choose a BCH!!");
				  return;
			  }

			  NS_LOG_DEBUG("I'm node "<<global_sti<<", in slot "<<slot_count_<<", NODE_WORK_FI and current bch is unvalid, choose: "<<slot_num_);
			  node_state_ = NODE_WAIT_REQUEST;
			  return;
		  }
		  break;
	  case NODE_WORK_ADJ:
		  if (!slot_memory_) {
			  this->clear_others_slot_status();
			  fi_collection = this->collected_fi_->slot_describe;
		  }
		  synthesize_fi_list();
		  if((fi_collection[slot_count_].sti == global_sti && fi_collection[slot_count_].busy == SLOT_1HOP)
				  || fi_collection[slot_count_].sti == 0)//BCH依然可用
		  {
			  if ((fi_collection[slot_count_].count_3hop >= fi_collection[slot_adj_candidate_].count_3hop)
					  && ((fi_collection[slot_adj_candidate_].sti == global_sti && fi_collection[slot_adj_candidate_].busy == SLOT_1HOP)
							  || fi_collection[slot_adj_candidate_].sti == 0)) //ADJ时隙可用)
			  {
				  int oldbch = slot_num_;
				  node_state_ = NODE_WORK_FI;
				  slot_num_ = slot_adj_candidate_;
				  adj_count_success_++;
				  fi_collection[oldbch].busy = SLOT_FREE;
				  fi_collection[oldbch].sti = 0;
				  fi_collection[oldbch].count_2hop = 0;
				  fi_collection[oldbch].count_3hop = 0;
				  fi_collection[oldbch].psf = 0;
				  fi_collection[oldbch].locker = 1;
			  } else {
				  node_state_ = NODE_WORK_FI;
				  fi_collection[slot_adj_candidate_].busy = SLOT_FREE;
				  fi_collection[slot_adj_candidate_].sti = 0;
				  fi_collection[slot_adj_candidate_].count_2hop = 0;
				  fi_collection[slot_adj_candidate_].count_3hop = 0;
				  fi_collection[slot_adj_candidate_].psf = 0;
				  fi_collection[slot_adj_candidate_].locker = 1;
			  }

			  generate_send_FI_packet();
		  } else { //BCH已经不可用

			  if((fi_collection[slot_adj_candidate_].sti == global_sti && fi_collection[slot_adj_candidate_].busy == SLOT_1HOP)
					  || fi_collection[slot_adj_candidate_].sti == 0) { //ADJ时隙可用
				  node_state_ = NODE_WORK_FI;
				  adj_count_success_++;
				  slot_num_ = slot_adj_candidate_;
			  } else { //ADJ时隙不可用
				  collision_count_++;

				  if (fi_collection[slot_adj_candidate_].sti == global_sti) {
					  fi_collection[slot_adj_candidate_].busy = SLOT_FREE;
					  fi_collection[slot_adj_candidate_].sti = 0;
					  fi_collection[slot_adj_candidate_].count_2hop = 0;
					  fi_collection[slot_adj_candidate_].count_3hop = 0;
					  fi_collection[slot_adj_candidate_].psf = 0;
					  fi_collection[slot_adj_candidate_].locker = 1;
				  }
				  slot_num_ = determine_BCH(0);
				  if(slot_num_ < 0 || slot_num_== slot_count_){
					  node_state_ = NODE_LISTEN;
					  slot_num_ = slot_count_;
					  no_avalible_count_ ++;
					  backoff_frame_num_ = m_uniformRandomVariable->GetInteger (0, 20);
					  return;
				  } else {
					  node_state_ = NODE_WAIT_REQUEST;
					  return;
				  }
			  }
		  }

		  slot_adj_candidate_ = -1; //
		  break;
	  }
  }

  return;

}

void TdmaSatmac::NotifyConflictDetected(Time conflictTime, Ptr<Node> conflictNode)
{
	int slotNumberInFrame = (conflictTime.GetMilliSeconds()) % (m_frame_len);
	slot_tag *fi_local_= this->collected_fi_->slot_describe;
	// std::cout<<"I am node: "<<this->GetGlobalSti()
	// 				<<" I use csma collision at slot "<< slotNumberInFrame
	// 				<<" txTime "<< conflictTime.GetMilliSeconds()
	// 				<<" m_slot_group "<< m_slot_group
	// 				<<" m_frame_len "<< m_frame_len
	// 				<<" fi_local_[slotNumberInFrame].busy="<< (int)fi_local_[slotNumberInFrame].busy
	// 				<<" fi_local_[slotNumberInFrame].sti= "<<fi_local_[slotNumberInFrame].sti
	// 				<<std::endl;

	fi_local_[slotNumberInFrame].busy = SLOT_COLLISION;
	fi_local_[slotNumberInFrame].life_time = this->GetSlotLife();
}

void TdmaSatmac::slotgroupHandler()
{
	total_slot_group_count = total_slot_group_count + 1;
	slot_group_count = total_slot_group_count % (m_frame_len / slot_group_length);

	slotgroupHandlerEvent = Simulator::Schedule (GetSlotTime() * slot_group_length, &TdmaSatmac::slotgroupHandler, this);


//	std::cout<<"slot_count_ "<<slot_count_<<std::endl;
//	std::cout<<"slot_group_count= "<<slot_group_count<<std::endl;
//	std::cout<<"Time: "<<Simulator::Now().GetMilliSeconds()<<std::endl;
	if (slot_count_ == 0 && slot_group_count == 0) {
	    for (int i = 0; i < m_frame_len / slot_group_length; i++) {
	        if (m_sg_info[i].t_valid > 0 && --m_sg_info[i].t_valid == 0) {
	            m_sg_info[i].geohash = -1;
	            m_sg_info[i].sg_busy = SLOT_GROUP_FREE;

	            if (i == m_slot_group) {
	                m_slot_group = -1;
	            }
	        }
	    }
	}


//	  slot_tag *fi_local_= this->collected_fi_->slot_describe;
//	  for(int i = 0;i < m_frame_len; i++)
//	  {
//		  std::cout<<"slot "<<i<<" node "<<fi_local_[i].sti-1<<std::endl;
//	  }

    if(slot_group_count == m_slot_group)
    {
        // 找出最大连续空闲时隙的起点和终点
        slot_tag *fi_local_ = this->collected_fi_->slot_describe;
        int maxFreeSlots = 0;
        int currentFreeSlots = 0;
        int startSlot = -1;
        int endSlot = -1;
        const int TOTAL_SLOTS_TO_CHECK = 4;

        for (uint32_t i = slot_count_; i < slot_count_ + TOTAL_SLOTS_TO_CHECK; ++i)
        {

            if (fi_local_[i].busy == SLOT_FREE)
            {
                currentFreeSlots++;
                if (currentFreeSlots > maxFreeSlots && currentFreeSlots >= 2)
                {
                    maxFreeSlots = currentFreeSlots;
                    startSlot = i - maxFreeSlots + 1;
                    endSlot = i;
                }
            }
            else
            {
                currentFreeSlots = 0;
            }
        }

       if(slot_count_ <= startSlot)
       {
    	   if(slot_count_ == startSlot)
    	   {
       	//    std::cout<<"Node: "<<this->GetGlobalSti()<<" Use Slot Group: "<<m_slot_group
       	// 		   <<" startSlot: "<<startSlot<<" endSlot: "<<endSlot<<" maxFreeSlots: "<<maxFreeSlots
		// 			   <<" Time "<<Simulator::Now().GetMicroSeconds()
		// 			   <<" slot_count_ "<<slot_count_
		// 			   <<" m_frame_len "<<m_frame_len<<std::endl;
               SlotGroupStart();
               StartMidSlotListening();
               Simulator::Schedule(GetSlotTime() * (maxFreeSlots - 1) - NanoSeconds(1), &TdmaSatmac::SlotGroupEnd, this);
    	   }
    	   else
    	   {
    	       Time timeToStartSlot = GetSlotTime() * (startSlot - slot_count_);
    	       Simulator::Schedule(timeToStartSlot, &TdmaSatmac::ExecuteSlotGroupOperations, this, maxFreeSlots, startSlot, endSlot);
    	   }

       }
    }
}

void TdmaSatmac::ExecuteSlotGroupOperations(int maxFreeSlots, int startSlot, int endSlot)
{
//    std::cout<<"Node: "<<getNodePtr()->GetId()<<" Use Slot Group: "<<m_slot_group
//		   <<" startSlot: "<<startSlot<<" endSlot: "<<endSlot<<std::endl;
    SlotGroupStart();
    StartMidSlotListening();
    Simulator::Schedule(GetSlotTime() * (maxFreeSlots - 1) - NanoSeconds(1), &TdmaSatmac::SlotGroupEnd, this);
}


void TdmaSatmac::SlotGroupStart()
{
    NS_LOG_FUNCTION(this << "Slot group start: " << slot_group_count);

    // TODO: 切至CSMA之前，再次检查本地信息
    // 检查当前的slot_count_是否等于startSlot
    slot_tag *fi_local_ = this->collected_fi_->slot_describe;
    int maxFreeSlots = 0;
    int currentFreeSlots = 0;
    int startSlot = -1;
    int endSlot = -1;
    const int TOTAL_SLOTS_TO_CHECK = 4;
    
    // 再次检查时隙的空闲状态
    for (uint32_t i = slot_count_; i < slot_count_ + TOTAL_SLOTS_TO_CHECK; ++i)
    {
        if (fi_local_[i].busy == SLOT_FREE)
        {
            currentFreeSlots++;
            if (currentFreeSlots > maxFreeSlots && currentFreeSlots >= 2)
            {
                maxFreeSlots = currentFreeSlots;
                startSlot = i - maxFreeSlots + 1;
                endSlot = i;
            }
        }
        else
        {
            currentFreeSlots = 0;
        }
    }

    // 如果当前时隙不等于startSlot，说明要等待
    if (slot_count_ != startSlot)
    {
        NS_LOG_INFO("Slot count has not reached startSlot yet. Waiting for startSlot " << startSlot);
        // 如果还没到startSlot，则需要调度下一步操作
        Time timeToStartSlot = GetSlotTime() * (startSlot - slot_count_);
        Simulator::Schedule(timeToStartSlot, &TdmaSatmac::SlotGroupStart, this);
        return;
    }

    // 执行设备切换操作
    Ptr<MacLayerController> macCtrler = this->getNodePtr()->GetObject<MacLayerController>();
    Ptr<WifiNetDevice> csma = DynamicCast<WifiNetDevice>(this->getNodePtr()->GetDevice(1));
	macCtrler->SwitchToDevice(csma);

	Simulator::Schedule(GetSlotTime() * (maxFreeSlots - 1) - NanoSeconds(1), &TdmaSatmac::SlotGroupEnd, this);
}

void TdmaSatmac::StartMidSlotListening()
{
    // 启动持续监听操作
    midSlotListeningEvent = Simulator::ScheduleNow(&TdmaSatmac::MidSlotListening, this);
}

void TdmaSatmac::MidSlotListening()
{
    NS_LOG_FUNCTION(this << "Mid slot listening: " << slot_group_count);

    // 在时隙组的中间时间进行持续操作
    //std::cout<<midSlotListeningEvent.IsRunning()<<std::endl;
    //std::cout << "Current simulation time: " << Simulator::Now().GetMicroSeconds() << " MicroSeconds" << std::endl;

    // 持续调度下一个监听操作（例如每 0.5 个时隙进行一次）
    midSlotListeningEvent = Simulator::Schedule(Seconds(GetSlotTime().GetDouble() * 1), &TdmaSatmac::MidSlotListening, this);
}

void TdmaSatmac::SlotGroupEnd()
{
    NS_LOG_FUNCTION(this << "Slot group end: " << slot_group_count);
    //std::cout<<"SlotGroupEnd:"<<Simulator::Now().GetMilliSeconds()<<std::endl;
    // 停止中间时间的监听操作
    if (midSlotListeningEvent.IsRunning())
    {
        Simulator::Cancel(midSlotListeningEvent);
    }
    // 时隙组结束时的操作

    // 其他清理操作
    Ptr<MacLayerController> macCtrler = this->getNodePtr()->GetObject<MacLayerController>();
    Ptr<WifiNetDevice> tdma = DynamicCast<WifiNetDevice>(this->getNodePtr()->GetDevice(0));
    Ptr<WifiNetDevice> csma = DynamicCast<WifiNetDevice>(this->getNodePtr()->GetDevice(1));
    macCtrler->SwitchToDevice(tdma);
}

int TdmaSatmac::GetGeohash()
{
	GeohashHelper& geohashHelper = GeohashHelper::GetInstance();
	double x = (this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().x;
	double y = (this->getNodePtr())->GetObject<MobilityModel> ()->GetPosition ().y;
	int geohash = geohashHelper.Encode(x, y);
	return geohash;
}

struct SlotGroupData {
    int sg_index;           // 时隙组索引
    int max_continuity;     // 最大连续空闲时隙数
    int total_free_slots;   // 总空闲时隙数

    SlotGroupData(int index, int continuity, int total)
        : sg_index(index), max_continuity(continuity), total_free_slots(total) {}
};

int TdmaSatmac::determine_SG()
{
    int chosen_sg = -1;
    std::array<slot_group_info, SLOT_GROUP_LENGTH> sgi_local_ = m_sg_info;
    slot_tag* fi_local_ = this->collected_fi_->slot_describe;
    std::list<SlotGroupData> sg_free_continuity;

//    // 首先查找相同 geohash 的时隙组
//    for(int i = 0; i < m_frame_len / slot_group_length; i++)
//    {
//        if(sgi_local_[i].geohash == this->GetGeohash())
//        {
// 			chosen_sg = i;
// 			return chosen_sg;
//        }
//    }

    // 初始化每个时隙组的信息
    for (int sg = 0; sg < m_frame_len / slot_group_length; sg++) {
        sg_free_continuity.push_back(SlotGroupData(sg, 0, 0));
    }

    // 统计每个时隙组中的空闲时隙数目和最大连续空闲时隙数
    for (int i = 0; i < m_frame_len; i++) {
        int sg_index = i / slot_group_length;
        if (fi_local_[i].busy == SLOT_FREE) {
            auto it = std::next(sg_free_continuity.begin(), sg_index);
            it->total_free_slots++;
            int current_continuity = 1;

            while (i + 1 < m_frame_len && fi_local_[i + 1].busy == SLOT_FREE && (i + 1) / slot_group_length == sg_index) {
                current_continuity++;
                i++;
            }

            if (current_continuity > it->max_continuity) {
                it->max_continuity = current_continuity;
            }
        }
    }

    // 选择好的时隙组并随机从中选择一个
    std::vector<SlotGroupData> candidate_sgs;
    for (auto& sg : sg_free_continuity) {
        if (sgi_local_[sg.sg_index].geohash == -1 && sg.max_continuity > 1)
        {
        	//td::cout<<sg.sg_index<<"  "<<sg.max_continuity<<std::endl;
            candidate_sgs.push_back(sg);
        }
    }

    // 对候选时隙组按最大连续空闲时隙数降序排序
    std::sort(candidate_sgs.begin(), candidate_sgs.end(), [](const SlotGroupData& a, const SlotGroupData& b) {
        return a.max_continuity > b.max_continuity;
    });

    // 从前几个优秀的候选中随机选择一个
    int top_n = std::min(5, static_cast<int>(candidate_sgs.size()));  // 选择前 top_n 个候选
    if (top_n > 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, top_n - 1);
        chosen_sg = candidate_sgs[dis(gen)].sg_index;
    }

    return chosen_sg;
}

void TdmaSatmac::merge_sgi(SlotGroupHeader recv_sgi_hdr)
{
   int index = 0;
   int recv_frame_len = recv_sgi_hdr.GetFrameLen();
   int recv_node_sti = recv_sgi_hdr.GetGlobalSti();
   int recv_node_slotgroup = recv_sgi_hdr.Get_MSlotGroup();
   std::array<slot_group_info, SLOT_GROUP_LENGTH>& sgi_local = m_sg_info;
   std::array<slot_group_info, SLOT_GROUP_LENGTH> sgi_append = recv_sgi_hdr.GetSlotGroupInfo();
   slot_group_info recv_sgi;
   //TODO:merge_sgi
   //std::cout<<"node "<<this->getNodePtr()->GetId()<<" recv a sgi "<<std::endl;

   for(index = 0; index < (m_frame_len / slot_group_length); index++)
   {
       recv_sgi = sgi_append[index];
       if(m_slot_group < 0)
           break;
       if(index == (recv_frame_len / slot_group_length))
           break;
       // TODO:节点自己使用的时隙组
       if(sgi_local[index].sg_busy == SLOT_GROUP_MINE && m_slot_group == index)
       {
       	//收到信息的geohash和我不同
       	if(recv_sgi.geohash != sgi_local[index].geohash)
       	{
				//判断一下距离
       		continue;
       	}
       }
   }

   // 遍历每一个时隙组
    for(index = 0; index < ((recv_frame_len > m_frame_len) ? recv_frame_len : m_frame_len) / slot_group_length; index++)
   {
       if(index == recv_frame_len / slot_group_length)
           break;

    //    if(index >= (m_frame_len / slot_group_length))
    //    {
    //        if(sgi_local[index].geohash != -1)
    //        {
    //        	// std::cout<<"m_frame_len= "<<m_frame_len
    //        	// 		<<" recv_frame_len= "<<recv_frame_len
	// 		// 			<<" index= "<<index
	// 		// 			<<"recv_node_slotgroup= "<<recv_node_slotgroup
    //        	// 		<<" sgi_local[index].geohash= "<<sgi_local[index].geohash<<std::endl;
    //             printf("merge_sgi: node %d Protocol ERROR!!\n", global_sti);
    //        }
    //    }

       recv_sgi = sgi_append[index];

       // 本节点的时隙组
       if(sgi_local[index].sg_busy == SLOT_GROUP_MINE && m_slot_group == index)
       {
           // 直接跳过，前面已经处理过
           continue;
       }
       //sgi发送者使用的时隙组
       else if(index == recv_node_slotgroup && index != m_slot_group)
       {
       	//sgi记录的和我不一样
       	if(sgi_local[index].geohash != recv_sgi.geohash)
       	{
       		if(sgi_local[index].geohash == -1)
       		{
       	        sgi_local[index].geohash = recv_sgi.geohash;
       	        sgi_local[index].t_valid = recv_sgi.t_valid;
       	        sgi_local[index].sg_busy = SLOT_GROUP_1HOP;
       		}
       		else if(sgi_local[index].t_valid < recv_sgi.t_valid)
       	    {
       	        sgi_local[index].geohash = recv_sgi.geohash;
       	        sgi_local[index].t_valid = recv_sgi.t_valid;
       	        sgi_local[index].sg_busy = SLOT_GROUP_1HOP;
       	    }
       	}
       	//sgi记录的和我一样
       	else if(sgi_local[index].geohash == recv_sgi.geohash)
       	{
       		if(sgi_local[index].geohash != -1 && sgi_local[index].sg_busy == SLOT_GROUP_2HOP)
       		{
       			sgi_local[index].sg_busy = SLOT_GROUP_1HOP;
       		}
       	}
       }
       //其他时隙组
       else if(index != m_slot_group && index != recv_node_slotgroup)
       {
       	//sgi记录的和我不一样
       	if(sgi_local[index].geohash != recv_sgi.geohash  && recv_sgi.sg_busy == SLOT_GROUP_1HOP)
       	{
       		if(sgi_local[index].t_valid < recv_sgi.t_valid)
       	    {
       	        sgi_local[index].geohash = recv_sgi.geohash;
       	        sgi_local[index].t_valid = recv_sgi.t_valid;
       	        sgi_local[index].sg_busy = SLOT_GROUP_2HOP;
       	    }
           	//空时隙组
       		else if(sgi_local[index].geohash == -1)
           	{
       	        sgi_local[index].geohash = recv_sgi.geohash;
       	        sgi_local[index].t_valid = recv_sgi.t_valid;
       	        sgi_local[index].sg_busy = SLOT_GROUP_2HOP;
           	}
       	}
       }
   }
}

void TdmaSatmac::CheckIsNeedSG()
{
    WifiMacHeader header;
    AperiodicTag aperiodicTag;
    std::vector<std::pair<Ptr<const Packet>, WifiMacHeader>> tempQueue;
    // 逐个出队，检查每个数据包
    while (!m_queue->IsEmpty())
    {
        Ptr<const Packet> packet = m_queue->Dequeue(&header);
        tempQueue.push_back(std::make_pair(packet, header));  // 保存出队的数据包

        if (packet->PeekPacketTag(aperiodicTag))
        {
            isNeedSg = true;  // 找到带有 AperiodicTag 的数据包
            break;
        }
    }
    // 将所有数据包重新入队
    for (auto& item : tempQueue)
    {
        m_queue->Enqueue(item.first, item.second);  // 重新入队
    }
}

double TdmaSatmac::get_available_slot_group_ratio()
 {
    // 获取时隙描述数组
    slot_tag *fi_local_ = this->collected_fi_->slot_describe;

    int available_slot_groups = 0;
    int total_slot_groups = m_frame_len / slot_group_length; // 总时隙组数

    // 遍历每个时隙组
    for (int i = 0; i < total_slot_groups; i++) {
        int free_count = 0;
        bool has_consecutive_free_slots = false;

        // 检查当前时隙组内是否有两个或更多连续空闲时隙
        for (int j = i * slot_group_length; j < (i + 1) * slot_group_length - 1; j++) {
            if (fi_local_[j].busy == SLOT_FREE && fi_local_[j + 1].busy == SLOT_FREE) {
                has_consecutive_free_slots = true;
                break;  // 一旦发现两个连续空闲时隙，跳出当前时隙组的检查
            }
        }

        // 如果当前时隙组存在两个连续空闲时隙，增加可用时隙组计数
        if (has_consecutive_free_slots) {
            available_slot_groups++;
        }
    }

    // 返回可用时隙组的比率
    if (total_slot_groups > 0) {
        return static_cast<double>(available_slot_groups) / total_slot_groups;
    } else {
        return 0.0;  // 如果没有时隙组，返回 0
    }
}

// int TdmaSatmac::adj_BCH_Sg() 
// {
//     slot_tag *fi_local_ = this->collected_fi_->slot_describe;  
//     int i, j;
//     int chosen_slot = -1;

//     // 临时变量记录每个时隙组内的连续空闲时隙数量
//     std::vector<int> consecutive_free_in_group(m_frame_len / slot_group_length, 0);
//     std::vector<std::pair<int, int>> candidate_slots;  // 存储空闲时隙ID和其count_3hop值的对

//     // 创建一个虚拟的fi_local_来模拟时隙的调整
//     std::vector<slot_tag> virtual_fi_local(fi_local_, fi_local_ + m_frame_len);

//     // 计算当前可用时隙组的比率
//     double initial_available_ratio = 0.0;
//     int available_count = 0;
//     for (i = 0; i < m_frame_len / slot_group_length; i++) 
//     {
//         int consecutive_free = 0;

//         for (j = i * slot_group_length; j < (i + 1) * slot_group_length; j++) 
//         {
//             if (virtual_fi_local[j].busy == SLOT_FREE) 
//                 consecutive_free++;
//             else 
//                 consecutive_free = 0;

//             consecutive_free_in_group[i] = std::max(consecutive_free_in_group[i], consecutive_free);
//         }

//         if (consecutive_free_in_group[i] >= 2) 
//             available_count++;
//     }
//     initial_available_ratio = (double)available_count / (m_frame_len / slot_group_length);

//     // 收集所有不可用时隙组中的空闲时隙
//     for (i = 0; i < m_frame_len / slot_group_length; i++) 
//     {
//         if (consecutive_free_in_group[i] < 2) 
//         {
//             for (j = i * slot_group_length; j < (i + 1) * slot_group_length; j++) 
//             {
//                 if (virtual_fi_local[j].busy == SLOT_FREE) 
//                 {
//                     candidate_slots.push_back({j, virtual_fi_local[j].count_3hop});
//                 }
//             }
//         }
//     }

//     if (!candidate_slots.empty()) 
//     {
//         // 根据 count_3hop 排序
//         std::sort(candidate_slots.begin(), candidate_slots.end(),
//                   [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
//                       return a.second < b.second; 
//                   });

//         // 对前 10 个候选时隙进行随机尝试
//         int top_n = std::min(10, static_cast<int>(candidate_slots.size()));
//         std::vector<std::pair<int, int>> top_candidates(candidate_slots.begin(), candidate_slots.begin() + top_n);

//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::vector<int> attempted_slots;

//         for (size_t k = 0; k < top_candidates.size(); ++k) 
//         {
//             // 随机选择一个尚未尝试的时隙
//             std::uniform_int_distribution<> dis(0, top_candidates.size() - 1);
//             int random_index = dis(gen);
//             while (std::find(attempted_slots.begin(), attempted_slots.end(), random_index) != attempted_slots.end()) 
//             {
//                 random_index = dis(gen);
//             }
//             attempted_slots.push_back(random_index);

//             chosen_slot = top_candidates[random_index].first;

//             // 更新虚拟时隙状态
//             virtual_fi_local[slot_num_].busy = SLOT_FREE;
//             virtual_fi_local[chosen_slot].busy = SLOT_1HOP;

//             // 重新计算可用时隙组比率
//             available_count = 0;
//             for (i = 0; i < m_frame_len / slot_group_length; i++) 
//             {
//                 int consecutive_free = 0;
//                 for (j = i * slot_group_length; j < (i + 1) * slot_group_length; j++) 
//                 {
//                     if (virtual_fi_local[j].busy == SLOT_FREE)
//                         consecutive_free++;
//                     else 
//                         consecutive_free = 0;

//                     consecutive_free_in_group[i] = std::max(consecutive_free_in_group[i], consecutive_free);
//                 }

//                 if (consecutive_free_in_group[i] >= 2) 
//                     available_count++;
//             }
//             double final_available_ratio = (double)available_count / (m_frame_len / slot_group_length);

//             // 如果比率有提升，返回选中的时隙
//             if (final_available_ratio > initial_available_ratio) 
//                 return chosen_slot;

//             // 恢复虚拟时隙状态，继续尝试下一个候选时隙
//             virtual_fi_local[chosen_slot].busy = SLOT_FREE;
//             virtual_fi_local[slot_num_].busy = SLOT_1HOP;
//         }
//     }

//     // 所有候选时隙都尝试过，没有找到提升的方案
//     return -1;
// }

int TdmaSatmac::adj_BCH_Sg() 
{
    slot_tag *fi_local_ = this->collected_fi_->slot_describe;  
    int i, j;
    int chosen_slot = -1;

    //std::cout << "开始时隙调整逻辑..." << std::endl;

    // 1. 检查当前时隙组是否可用
    int current_slot_group = slot_num_ / slot_group_length;
    int consecutive_free = 0;
    // std::cout << "当前节点时隙编号：" << slot_num_ 
    //           << "，所在时隙组：" << current_slot_group << std::endl;

    for (j = current_slot_group * slot_group_length; 
         j < (current_slot_group + 1) * slot_group_length; 
         j++) 
    {
        if (fi_local_[j].busy == SLOT_FREE) 
        {
            consecutive_free++;
        }
        else 
        {
            consecutive_free = 0;
        }

        if (consecutive_free >= 2) 
        {
            // std::cout << "当前时隙组可用，满足至少2个连续空闲时隙，时隙组编号：" 
            //           << current_slot_group << std::endl;
            break;
        }
    }

    if (consecutive_free < 2) 
    {
        //std::cout << "当前时隙组不可用，不需要调整，直接返回-1" << std::endl;
		slot_tag *fi_local_= this->collected_fi_->slot_describe;
		// for(int i = 0;i < m_frame_len; i++)
		// {
		// 	std::cout<<"slot "<<i<<" node "<<fi_local_[i].sti-1<<std::endl;
		// }
        return -1;
    }

    // 2. 收集不可用时隙组中的候选时隙
    std::vector<std::pair<int, int>> candidate_slots;

	for (i = 0; i < m_frame_len / slot_group_length; i++) 
	{
		int consecutive_free = 0;
		bool is_group_available = false; // 标记当前时隙组是否可用

		// 遍历时隙组内所有时隙
		for (j = i * slot_group_length; j < (i + 1) * slot_group_length; j++) 
		{
			if (fi_local_[j].busy == SLOT_FREE) 
			{
				consecutive_free++;
			} 
			else 
			{
				consecutive_free = 0; // 遇到非空闲时隙，重置连续空闲计数
			}

			// 如果找到两个连续空闲时隙，标记该组为可用
			if (consecutive_free >= 2) 
			{
				is_group_available = true;
				break; // 当前组已经可用，无需继续检查
			}
		}

		// 如果当前时隙组不可用，收集空闲时隙
		if (!is_group_available) 
		{
			//std::cout << "发现不可用时隙组，编号：" << i << std::endl;

			for (j = i * slot_group_length; j < (i + 1) * slot_group_length; j++) 
			{
				if (fi_local_[j].busy == SLOT_FREE) 
				{
					// std::cout << "    不可用时隙组中的空闲时隙：" << j 
					// 		<< "，C3H值：" << fi_local_[j].count_3hop << std::endl;
					candidate_slots.push_back({j, fi_local_[j].count_3hop});
				}
			}
		}
	}


    // 3. 从候选时隙中随机选择一个
    if (!candidate_slots.empty()) 
    {
        // 根据 count_3hop 排序
        std::sort(candidate_slots.begin(), candidate_slots.end(),
                  [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                      return a.second < b.second; 
                  });

        // std::cout << "候选时隙排序后：" << std::endl;
        // for (const auto& slot : candidate_slots)
        // {
        //     std::cout << "    候选时隙：" << slot.first 
        //               << "，C3H值：" << slot.second << std::endl;
        // }

        // 对前 10 个候选时隙进行随机尝试
        int top_n = std::min(10, static_cast<int>(candidate_slots.size()));
        std::vector<std::pair<int, int>> top_candidates(candidate_slots.begin(), candidate_slots.begin() + top_n);

        //std::cout << "从前" << top_n << "个候选时隙中随机选择..." << std::endl;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, top_candidates.size() - 1);
        int random_index = dis(gen);

        chosen_slot = top_candidates[random_index].first; // 随机选择一个时隙
        //std::cout << "随机选择的时隙：" << chosen_slot << std::endl;
    }
    else
    {
        //std::cout << "没有找到任何候选时隙，返回-1" << std::endl;
        return -1;
    }

    // 4. 返回选择的时隙编号
    //std::cout << "时隙调整成功，新时隙编号：" << chosen_slot << std::endl;
    return chosen_slot;
}




void TdmaSatmac::sg_opration(Ptr<Packet> p)
{
	CheckIsNeedSG();
	if(isNeedSg)
	{
		//TODO:initialize slot group
		if(node_state_ == NODE_WORK_FI)
		{
			Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
			std::array<int, 4> possible_values = {8, 10, 12, 14};
			int random_index = rand->GetInteger(0, possible_values.size() - 1);

			//std::cout<<"Time: "<<Simulator::Now().GetMicroSeconds()<<std::endl;
			if(m_slot_group == -1)
			{
				bool found = false;
				for (int i = 0; i < m_frame_len / slot_group_length; i++)
				{
				    if (m_sg_info[i].geohash == this->GetGeohash()) {
				        m_slot_group = i;
				        m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_MINE;
				        found = true;
				        break;
				    }
				}
				if (!found)
				{
				    m_slot_group = determine_SG();
				    m_sg_info[m_slot_group].geohash = this->GetGeohash();
				    m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_MINE;
				    m_sg_info[m_slot_group].t_valid = possible_values[random_index];
				}

				// std::cout<<"node "<<this->getNodePtr()->GetId()<<
				// 		" geohash : "<<this->GetGeohash()<<
				// 		" first choose slot_group : "<<m_slot_group
				// 		<<std::endl;
			}
			//节点离开原先的geohash区域
			else if(m_slot_group != -1 && m_sg_info[m_slot_group].geohash != this->GetGeohash() && m_sg_info[m_slot_group].geohash != -1)
			{
				// std::cout<<"Node: "<<this->getNodePtr()->GetId()<<
				// 			" leave the geohash: "<<m_sg_info[m_slot_group].geohash<<
				// 			" to: "<<this->GetGeohash()<<std::endl;

				// for(int i = 0; i < m_frame_len / slot_group_length; i++)
				// {
				// 	std::cout<<"Node: "<<this->getNodePtr()->GetId()<<
				// 				" slot_group "<<i<<" geohash: "<<m_sg_info[i].geohash<<
				// 				" t_valid: "<<m_sg_info[i].t_valid<<std::endl;
				// }

				if(m_sg_info[m_slot_group].t_valid > 0)
				{
					m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_1HOP;
				}

				// for(int i = 0; i < m_frame_len / slot_group_length; i++)
				// {
				// 	std::cout<<"Node: "<<this->getNodePtr()->GetId()<<
				// 				" slot_group "<<i<<" geohash: "<<m_sg_info[i].geohash<<
				// 				" t_valid: "<<m_sg_info[i].t_valid<<std::endl;
				// }

				//重新选择一个时隙组
				m_slot_group = determine_SG();
				if(m_sg_info[m_slot_group].geohash == -1)
				{
					m_sg_info[m_slot_group].geohash = this->GetGeohash();
					m_sg_info[m_slot_group].t_valid = possible_values[random_index];
					m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_MINE;
				}
				else if(m_sg_info[m_slot_group].geohash ==this->GetGeohash())
				{
					m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_MINE;
				}
				// std::cout<<"node "<<this->getNodePtr()->GetId()<<
				// 		" geohash : "<<this->GetGeohash()<<
				// 		" change slot_group to: "<<m_slot_group<<std::endl;
			}
			  SlotGroupTag tag;
			//  tag.SetFrameLen(m_frame_len);
			//  tag.SetGlobalSti(global_sti);
			//  tag.SetSlotGroupInfo(m_sg_info);
			//  tag.Set_MSlotGroup(m_slot_group);
			  p->AddPacketTag(tag);
			 SlotGroupHeader hdr;
			 hdr.SetFrameLen(m_frame_len);
			 hdr.SetGlobalSti(global_sti);
			 hdr.SetSlotGroupInfo(m_sg_info);
			 hdr.Set_MSlotGroup(m_slot_group);
			 p->AddHeader(hdr);
		}
	}
	else  //节点已经不需要时隙组
	{
	    if (m_slot_group != -1)
	    {
	        // std::cout << "node " << this->getNodePtr()->GetId()
	        //           << " release slot_group: " << m_slot_group << std::endl;

			if(m_sg_info[m_slot_group].t_valid > 0)
			{
				m_sg_info[m_slot_group].sg_busy = SLOT_GROUP_1HOP;
			}
	        m_slot_group = -1; // 重置为未分配状态
	    }
	}
}
}
