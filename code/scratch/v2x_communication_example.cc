/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
  This software was developed at the National Institute of Standards and
  Technology by employees of the Federal Government in the course of
  their official duties. Pursuant to titleElement 17 Section 105 of the United
  States Code this software is not subject to copyright protection and
  is in the public domain.
  NIST assumes no responsibility whatsoever for its use by other parties,
  and makes no guarantees, expressed or implied, about its quality,
  reliability, or any other characteristic.

  We would appreciate acknowledgement if the software is used.

  NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
  DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
  FROM THE USE OF THIS SOFTWARE.

 * Modified by: Fabian Eckermann <fabian.eckermann@udo.edu> (CNI)
 *              Moritz Kahlert <moritz.kahlert@udo.edu> (CNI)
 */

#include "ns3/lte-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/lte-v2x-helper.h"
#include "ns3/config-store.h"
#include "ns3/lte-hex-grid-enb-topology-helper.h"
#include <ns3/buildings-helper.h>
#include <ns3/cni-urbanmicrocell-propagation-loss-model.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/spectrum-analyzer-helper.h>
#include <ns3/multi-model-spectrum-channel.h>
#include "ns3/ns2-mobility-helper.h"
#include <cfloat>
#include <sstream>

#include "ns3/wifi-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/applications-module.h"
#include "ns3/itu-r-1411-los-propagation-loss-model.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store-module.h"
#include "ns3/integer.h"
//#include "ns3/wave-bsm-helper.h"
#include "ns3/wave-helper.h"
#include "ns3/topology.h"

#include "ns3/bsm-timetag.h"
#include "ns3/wave-bsm-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/wifi-phy-state-helper.h"
#include "ns3/GeohashHelper.h"
#include "ns3/GlobalPacketDropController.h"
#include "ns3/run_number.h"
#include "ns3/TxCounter.h"



//--building=1 --buildingfile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/9gong/buildings.xml --tracefile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/9gong/9gong.ns2
//./waf --run "v2x_communication_example2 --numVeh=900 --building=0 --buildingfile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/newyork/buildings.xml --tracefile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/newyork/newyorkmobility.ns2"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("v2x_communication_mode_4");

// Output 
std::string simtime = "log_simtime_v2x.csv"; 
std::string deltatime_file_bsm = "log_deltatime_bsm.txt";
std::string deltatime_file_aper = "log_deltatime_aper.txt";
std::string lpfoutfile = "lpf-output.txt";
//std::string rx_data = "log_rx_data_v2x.csv";
//std::string tx_data = "log_tx_data_v2x.csv";
//std::string connections = "log_connections_v2x.csv";
//std::string positions = "log_positions_v2x.csv";

//Ptr<OutputStreamWrapper> log_connections;
Ptr<OutputStreamWrapper> log_simtime;
Ptr<OutputStreamWrapper> log_deltatime_bsm;
Ptr<OutputStreamWrapper> log_deltatime_aper;
//Ptr<OutputStreamWrapper> log_positions;
//Ptr<OutputStreamWrapper> log_rx_data;
//Ptr<OutputStreamWrapper> log_tx_data;

// Global variables
uint32_t ctr_totRx = 0; 	// Counter for total received packets
uint32_t ctr_totTx = 0; 	// Counter for total transmitted packets
uint16_t lenCam = 100;  // Length of CAM message in bytes [50-300 Bytes]
//double baseline= 2550.0;     // Baseline distance in meter (150m for urban, 320m for freeway)

int testing = 0;

int m_tdma_enable = 1;
int m_tdma_slottime_us = 1000;

// Initialize some values
// NOTE: commandline parser is currently (05.04.2019) not working for uint8_t (Bug 2916)

// Create node container to hold all UEs
//NodeContainer allNodesCon;

NodeContainer allNodesCon;
NetDeviceContainer tdmaDataDevices;
NetDeviceContainer csmaDataDevices;
Ipv4InterfaceContainer tdmaIpInterfaces;

uint16_t simTime = 100;                 // Simulation time in seconds
uint32_t numVeh = 201;                  // Number of vehicles
double txPower = 6.7;                // Transmission power in dBm
int testdistance = 150;


int run_num = 0;
bool slotgroup_ena = 1;//是否启用时隙组
bool adj_ena_sg = 1;//是否启用时隙调整for时隙组
bool variable_packet_size_ena = 1;//是否启用可变数据包大小
double adjRatio_low_sg = 0.4;
double adjRatio_high_sg = 0.8;

double frameadj_cut_ratio_ths_ = 0.4;
double frameadj_cut_ratio_ehs_ = 0.6;
double frameadj_exp_ratio_ = 0.9;
int adjEna = 1;
int adjFrameEna =1;
int framelen = 64;
int framelenUp = 128;
int framelenLow = 32;

int m_wavePacketSize = 200;
double m_waveInterval = 0.1;
double m_gpsAccuracyNs = 10000; ///< GPS accuracy
std::vector <double> m_txSafetyRanges; ///< list of ranges
double m_txMaxDelayMs = 10;
int64_t m_streamIndex = 0;
WaveBsmHelper m_waveBsmHelper; ///< helper

std::string tracefile;                  // Name of the tracefile
std::string tracefile_200 = "/home/zxz/idle-satmac/mobility/map.mobility-200n.tcl";
std::string tracefile_400 = "/home/zxz/idle-satmac/mobility/map.mobility-400n.tcl";
std::string tracefile_600 = "/home/zxz/idle-satmac/mobility/map.mobility-600n.tcl";
std::string tracefile_800 = "/home/zxz/idle-satmac/mobility/map.mobility-800n.tcl";
std::string tracefile_1000 = "/home/zxz/idle-satmac/mobility/map.mobility-1000n.tcl";
std::string tracefile_1200 = "/home/zxz/idle-satmac/mobility/map.mobility-1200n.tcl";
std::string tracefile_1400 = "/home/zxz/idle-satmac/mobility/map.mobility-1400n.tcl";
std::string tracefile_1600 = "/home/zxz/idle-satmac/mobility/map.mobility-1600n.tcl";
//std::string tracefile="/home/wu/workspace/ns-3_c-v2x-master/mobility/city-big/updated-350-adj-all_1.tcl";                  // Name of the tracefile

//std::string m_flowOutFile;
//std::string m_outputPrefix;
//std::string m_netFileString;
//std::string m_osmFileString;

int m_loadBuildings = 0;
std::string bldgFile;// = "src/wave/examples/9gong/buildings.xml";

// Responders users 
//NodeContainer ueVeh;

static uint32_t collision_count=0;
static uint32_t PktsSent_count=0;
static uint32_t PeriPktsRecv_count=0;
static uint32_t AperiPktsRecv_count=0;
static uint32_t PeriPktsDelay_count=0;
static uint32_t AperiPktsDelay_count=0;
GlobalPacketDropController& dropController = GlobalPacketDropController::GetInstance();

// void 
// PrintStatus (uint32_t s_period, Ptr<OutputStreamWrapper> log_simtime)
// {
// 	double pir_mean, delta_mean;
// 	int pir_max, pir_min, delta_max, delta_min;
// 	pir_mean = m_waveBsmHelper.GetWaveBsmStats ()->GetPIR(&pir_max, &pir_min);
// 	m_waveBsmHelper.GetWaveBsmStats ()->ResetRecvTimeMap();

// 	delta_mean = m_waveBsmHelper.GetWaveBsmStats ()->GetPktRecvDeltaTimeUs(&delta_max, &delta_min);

// 	std::vector<int> *tmpvec = m_waveBsmHelper.GetWaveBsmStats ()->GetPktRecvDeltaTimeUs_vec();
// 	std::vector<int>::iterator iter;
// 	// for(iter = tmpvec->begin(); iter != tmpvec->end(); iter++)
// 	// {
// 	// 	*log_deltatime->GetStream() << *iter << std::endl;
// 	// }

// 	m_waveBsmHelper.GetWaveBsmStats ()->ResetPktRecvDeltaTimeUs();

// 	//std::cout << delta_mean << " " << delta_max << " " << delta_min << std::endl;

// 	int wavePktsSent = m_waveBsmHelper.GetWaveBsmStats ()->GetTxPktCount ();
// 	//PktsSent_count += wavePktsSent;
// 	int wavePktsReceived = m_waveBsmHelper.GetWaveBsmStats ()->GetRxPktCount ();
// 	PktsRecv_count += wavePktsReceived;
// 	double bsmpdr = m_waveBsmHelper.GetWaveBsmStats ()->GetBsmPdr(1);
// 	double culbsmpdr = m_waveBsmHelper.GetWaveBsmStats ()->GetCumulativeBsmPdr(1);
// 	//  NS_LOG_UNCOND ("At t=" << (Simulator::Now ()).GetSeconds () << "s BSM_PDR1=" << wavePDR1_2 );

// 	//LPF output format
// 	// *log_simtime->GetStream() << Simulator::Now ().GetSeconds () << " "<<collision_count << " "
// 	// 		<<wavePktsSent<<" "<<wavePktsReceived<< " " << bsmpdr << " " << culbsmpdr << " "
// 	// 		<< pir_mean << " " << pir_max << " " << pir_min << " "
// 	// 		<< (int)delta_mean << " " << delta_max << " " << delta_min << std::endl;

// 	uint32_t txCount = TxCounter::GetInstance().GetCount();

// 	double pcr = (double)(collision_count + dropController.GetDropCount()) / (double)txCount;
// 	std::cout << "t=" <<  Simulator::Now().GetSeconds() <<  " Collision "<<collision_count
// 			<<" tdma/csma Collision "<<dropController.GetDropCount()
// 			<<" PCR "<< std::setprecision(3) <<pcr
// 			<<" Rx_count "<< PktsRecv_count
// 			<<" Tx_count "<< txCount
// 			<<" Rx/PDR/CPDR " << std::setprecision(3) << wavePktsReceived << " " << bsmpdr << " " << culbsmpdr
// 			<< " PIR Mean/Max/Min "<< std::setprecision(20) << pir_mean << " " << pir_max << " " << pir_min
// 			<<" Delta Mean/Max/Min " << std::setprecision(20) << (int)delta_mean << " " << delta_max << " " << delta_min << std::endl;

// 	m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktCount (0);
// 	m_waveBsmHelper.GetWaveBsmStats ()->SetTxPktCount (0);
// 	for (int index = 1; index <= 1; index++)
// 	{
// 	  m_waveBsmHelper.GetWaveBsmStats ()->SetExpectedRxPktCount (index, 0);
// 	  m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktInRangeCount (index, 0);
// 	}

//     Simulator::Schedule(Seconds(s_period), &PrintStatus, s_period,log_simtime);
// }

void PrintStatus(uint32_t s_period)
{
    // 获取周期性和非周期性统计对象
    Ptr<WaveBsmStats> periodicStats = m_waveBsmHelper.GetWaveBsmStats();
    Ptr<WaveBsmStats> aperiodicStats = m_waveBsmHelper.GetAperiodicStats();

    // 周期性数据包统计
    int periodicPktsSent = periodicStats->GetTxPktCount();
    int periodicPktsReceived = periodicStats->GetRxPktCount();

    double periodicDelayMean = 0;
    int periodicDelayMax, periodicDelayMin;
    periodicDelayMean = periodicStats->GetPktRecvDeltaTimeUs(&periodicDelayMax, &periodicDelayMin);

	std::vector<int> *tmpvec1 = periodicStats->GetPktRecvDeltaTimeUs_vec();
	std::vector<int>::iterator iter1;
	for(iter1 = tmpvec1->begin(); iter1 != tmpvec1->end(); iter1++)
	{
		*log_deltatime_bsm->GetStream() << *iter1 << std::endl;
	}

	std::vector<int> *tmpvec2 = aperiodicStats->GetPktRecvDeltaTimeUs_vec();
	std::vector<int>::iterator iter2;
	for(iter2 = tmpvec2->begin(); iter2 != tmpvec2->end(); iter2++)
	{
		*log_deltatime_aper->GetStream() << *iter2 << std::endl;
	}

	double bsmpdr = periodicStats->GetBsmPdr(1);
	double culbsmpdr = periodicStats->GetCumulativeBsmPdr(1);

    // 非周期性数据包统计
    int aperiodicPktsSent = aperiodicStats->GetTxPktCount();
    int aperiodicPktsReceived = aperiodicStats->GetRxPktCount();

    double aperiodicDelayMean= 0;
    int aperiodicDelayMax, aperiodicDelayMin;
    aperiodicDelayMean = aperiodicStats->GetPktRecvDeltaTimeUs(&aperiodicDelayMax, &aperiodicDelayMin);

	double aperpdr = aperiodicStats->GetBsmPdr(1);
	double culbaperpdr = aperiodicStats->GetCumulativeBsmPdr(1);

    uint32_t txCount_per = TxCounter::GetInstance().GetPeriodicCount();
    uint32_t txCount_aper = TxCounter::GetInstance().GetAperiodicCount();

    PeriPktsRecv_count += periodicPktsReceived;
    AperiPktsRecv_count += aperiodicPktsReceived;

    double pcr = (double)(collision_count + dropController.GetDropCount()) / (double)(txCount_per + txCount_aper);

	double PeriPktsDelay_MeanCount = 0.0;
	double AperiPktsDelay_MeanCount = 0.0;
	if(Simulator::Now() > Seconds(1.0))
	{
		PeriPktsDelay_count += (int)periodicDelayMean;
		AperiPktsDelay_count += (int)aperiodicDelayMean;
		PeriPktsDelay_MeanCount = PeriPktsDelay_count / ((int)Simulator::Now().GetSeconds() - 1);
		AperiPktsDelay_MeanCount = AperiPktsDelay_count / ((int)Simulator::Now().GetSeconds() - 1);
	}

    // 打印到控制台
    std::cout << "t=" << Simulator::Now().GetSeconds()
              << " Collision=" << collision_count
              << " tdma/csma Collision=" << dropController.GetDropCount()
              << " PCR=" << std::setprecision(3) << pcr
              << " Periodic:"
              << " Tx=" << txCount_per
              << " Rx=" << PeriPktsRecv_count
			  << " PDR/CPDR "<<std::setprecision(3)<<bsmpdr<<" "<<culbsmpdr
              << " DelayMean/DelayMeanCount=" << std::setprecision(20) << (int)periodicDelayMean << "/" << PeriPktsDelay_MeanCount 
              << " Aperiodic:"
              << " Tx=" << txCount_aper
              << " Rx=" << AperiPktsRecv_count
			  << " PDR/CPDR "<<std::setprecision(3)<<aperpdr<<" "<<culbaperpdr
              << " DelayMean/DelayMeanCount=" << std::setprecision(20) << (int)aperiodicDelayMean << "/" << AperiPktsDelay_MeanCount
              << std::endl;

    // 重置统计数据
    periodicStats->SetTxPktCount(0);
    periodicStats->SetRxPktCount(0);
    aperiodicStats->SetTxPktCount(0);
    aperiodicStats->SetRxPktCount(0);

    periodicStats->ResetPktRecvDeltaTimeUs();
    aperiodicStats->ResetPktRecvDeltaTimeUs();

    for (int index = 1; index <= 1; index++) {
        periodicStats->SetExpectedRxPktCount(index, 0);
        periodicStats->SetRxPktInRangeCount(index, 0);
        aperiodicStats->SetExpectedRxPktCount(index, 0);
        aperiodicStats->SetRxPktInRangeCount(index, 0);
    }

    // 调度下一次打印
    Simulator::Schedule(Seconds(s_period), &PrintStatus, s_period);
}


void
PhyRxCollisionDropTrace(Ptr<const Packet> p)
{
	collision_count++;
}

void config();
void CheckThroughput ();

int
main (int argc, char *argv[])
  {

    LogComponentEnable ("v2x_communication_mode_4", LOG_INFO);

    // Command line arguments
    CommandLine cmd;
    cmd.AddValue ("test", "", testing);
    cmd.AddValue ("time", "Simulation Time", simTime);
    cmd.AddValue ("node", "Number of Vehicles", numVeh);
     cmd.AddValue ("lenCam", "Packetsize in Bytes", lenCam);
    cmd.AddValue ("log_collision", "name of the simtime logfile", simtime);
    cmd.AddValue ("log_lpf", "name of the lpf logfile", lpfoutfile);
    cmd.AddValue ("log_delta", "name of the delta time logfile", deltatime_file_bsm);

//    cmd.AddValue ("log_rx_data", "name of the rx data logfile", rx_data);
//    cmd.AddValue ("log_tx_data", "name of the tx data logfile", tx_data);
//    cmd.AddValue ("tracefile", "Path of ns-3 tracefile", tracefile);
    //cmd.AddValue ("baseline", "Distance in which messages are transmitted and must be received", baseline);

    cmd.AddValue ("tdma", "enable tdma or not (802.11p)", m_tdma_enable);

	cmd.AddValue ("slottime", "slot time in us.", m_tdma_slottime_us);

    cmd.AddValue ("bsm", "(WAVE) BSM size (bytes)", m_wavePacketSize);
    cmd.AddValue ("interval", "(WAVE) BSM interval (s)", m_waveInterval);


    cmd.AddValue ("FrameadjExpRatio", "",  frameadj_exp_ratio_);
    cmd.AddValue ("FrameadjCutRatioThs", "",  frameadj_cut_ratio_ths_);
    cmd.AddValue ("FrameadjCutRatioEhs", "",  frameadj_cut_ratio_ehs_);
    cmd.AddValue ("AdjEnable", "",  adjEna);
    cmd.AddValue ("AdjFrameEnable", "",  adjFrameEna);
    cmd.AddValue ("FrameLen", "",  framelen);
    cmd.AddValue ("FrameLenUp", "",  framelenUp);
    cmd.AddValue ("FrameLenLow", "",  framelenLow);


    //NOTICE:obstacle shadowing model is disabled.
    cmd.AddValue ("building", "Use obstacle building shadowing.", m_loadBuildings);
    cmd.AddValue ("buildingfile", "Path of building file", bldgFile);


	cmd.AddValue("run_num", "The run number", run_num);  // 将 run_num 作为命令行参数传递
	cmd.AddValue("adj_ena_sg", "Enable timeslot adjustment for timeslot groups", adj_ena_sg); 
	cmd.AddValue("adjRatio_low_sg", "Timeslot adjustment for timeslot groups low ratio", adjRatio_low_sg); 
	cmd.AddValue("adjRatio_high_sg", "Timeslot adjustment for timeslot groups high ratio", adjRatio_high_sg); 
	cmd.AddValue("slotgroup_ena", "Timeslot adjustment for timeslot groups high ratio", slotgroup_ena); 
	cmd.AddValue("variable_packet_size_ena", "Whether to enable variable packet size", variable_packet_size_ena); 
	// 设置 run_num


    //--building=1 --tracefile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/newyork/newyorkmobility.ns2 --buildingfile=/home/wu/workspace/ns-3_c-v2x-master/src/wave/examples/newyork/buildings.xml
    cmd.Parse (argc, argv);

    RunNumber::GetInstance().SetRunNum(run_num);  // 将 run_num 设置到 RunNumber 实例中
	RunNumber::GetInstance().SetAdjEnaSg(adj_ena_sg);
	RunNumber::GetInstance().SetAdjRatioLowSg(adjRatio_low_sg);
	RunNumber::GetInstance().SetAdjRatioHighSg(adjRatio_high_sg);
	RunNumber::GetInstance().SetEnaSg(slotgroup_ena);
	RunNumber::GetInstance().SetVariablePacketSizeEna(variable_packet_size_ena);

    ns3::RngSeedManager::SetSeed(13);

    AsciiTraceHelper ascii;
 //   log_simtime = ascii.CreateFileStream(simtime);
 	log_deltatime_bsm = ascii.CreateFileStream(deltatime_file_bsm);
	log_deltatime_aper = ascii.CreateFileStream(deltatime_file_aper);
//    log_rx_data = ascii.CreateFileStream(rx_data);
//    log_tx_data = ascii.CreateFileStream(tx_data);
//    log_connections = ascii.CreateFileStream(connections);
//    log_positions = ascii.CreateFileStream(positions);

    NS_LOG_INFO ("Starting network configuration...");

    config();

	//*log_simtime->GetStream() << "Simtime;TotalRx;TotalTx;PRR" << std::endl;
	//Simulator::Schedule(Seconds(1), &PrintStatus, 1, log_simtime);
	Simulator::Schedule(Seconds(1), &PrintStatus, 1);
	CheckThroughput ();

	NS_LOG_INFO ("Starting Simulation...");
	Simulator::Stop(MilliSeconds(simTime*1000+40));
	Simulator::Run();

	Simulator::Destroy();

	NS_LOG_INFO("Simulation done.");
	return 0;
}
/* ********************************************************
 * 			TDMA  Configuration
 *********************************************************/
void satmac_par_config(){

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/SlotTime", TimeValue(MicroSeconds(m_tdma_slottime_us)));

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/AdjEnable", IntegerValue(adjEna));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/AdjFrameEnable", IntegerValue(adjFrameEna));

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/FrameLen", IntegerValue(framelen));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/AdjFrameLowerBound", IntegerValue(framelenLow));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/AdjFrameUpperBound", IntegerValue(framelenUp));

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/SlotMemory", IntegerValue(1));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/SlotLife", IntegerValue(3));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/C3HThreshold", IntegerValue(3));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/AdjThreshold", IntegerValue(3));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/RandomBchIfSingle", IntegerValue(0));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/ChooseBchRandomSwitch", IntegerValue(1));

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/FrameadjExpRatio", DoubleValue(frameadj_exp_ratio_));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/FrameadjCutRatioThs", DoubleValue(frameadj_cut_ratio_ths_));
	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/FrameadjCutRatioEhs", DoubleValue(frameadj_cut_ratio_ehs_));

	Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/Tdma/LPFTraceFile", StringValue(lpfoutfile));
}

void config()
{
	//double freq = 5.9e9;
//	if (m_lossModel == 1)
//	{
//	  m_lossModelName = "ns3::FriisPropagationLossModel";
//	}
//	else if (m_lossModel == 2)
//	{
//	  m_lossModelName = "ns3::ItuR1411LosPropagationLossModel";
//	}
//	else if (m_lossModel == 3)
//	{
//	  m_lossModelName = "ns3::TwoRayGroundPropagationLossModel";
//	}
//	else if (m_lossModel == 4)
//	{
//	  m_lossModelName = "ns3::LogDistancePropagationLossModel";
//	}
//	else
//	{
//	  // Unsupported propagation loss model.
//	  // Treating as ERROR
//	}

	std::cout << "Creating " << (unsigned)numVeh << " nodes " << "\n";
	allNodesCon.Create (numVeh);
	// Name nodes
	for (uint32_t i = 0; i < numVeh; ++i)
	 {
	   std::ostringstream os;
	   // Set the Node name to the corresponding IP host address
	   os << "node-" << i+1;
	   Names::Add (os.str (), allNodesCon.Get (i));
	 }

	YansWifiChannelHelper wifiChannel2;
	wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	Config::SetDefault ("ns3::CniUrbanmicrocellPropagationLossModel::Frequency", DoubleValue(5800e6));
	Config::SetDefault ("ns3::CniUrbanmicrocellPropagationLossModel::LosEnabled", BooleanValue(true));
	wifiChannel2.AddPropagationLoss ("ns3::CniUrbanmicrocellPropagationLossModel");

	Ptr<YansWifiChannel> channel = wifiChannel2.Create ();
	YansWifiPhyHelper wifiPhy2 =  YansWifiPhyHelper::Default ();
	wifiPhy2.SetChannel (channel);
	wifiPhy2.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11);
	NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
	Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

	// Setup 802.11p stuff
	wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
								   "DataMode",StringValue ("OfdmRate12MbpsBW10MHz"),
								   "ControlMode",StringValue ("OfdmRate12MbpsBW10MHz"),
				   "NonUnicastMode", StringValue ("OfdmRate12MbpsBW10MHz"));

	// Set Tx Power
	wifiPhy2.Set ("TxPowerStart",DoubleValue (txPower));
	wifiPhy2.Set ("TxPowerEnd", DoubleValue (txPower));

	wifiPhy2.Set ("EnergyDetectionThreshold", DoubleValue (-85));//about 200m

	tdmaDataDevices = wifi80211p.Install (wifiPhy2, wifi80211pMac, allNodesCon, 1);

	// CSMA设备的物理层配置
	YansWifiChannelHelper csmaWifiChannel;
	csmaWifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	csmaWifiChannel.AddPropagationLoss("ns3::CniUrbanmicrocellPropagationLossModel", "Frequency", DoubleValue(5900e6), "LosEnabled", BooleanValue(true));

	Ptr<YansWifiChannel> csmaChannel = csmaWifiChannel.Create();
	YansWifiPhyHelper csmaWifiPhy = YansWifiPhyHelper::Default();
	csmaWifiPhy.SetChannel(csmaChannel);
	csmaWifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11);
	csmaWifiPhy.Set("TxPowerStart", DoubleValue(txPower));
	csmaWifiPhy.Set("TxPowerEnd", DoubleValue(txPower));
	csmaWifiPhy.Set("EnergyDetectionThreshold", DoubleValue(-85)); // 设置检测阈值

	// 设置CSMA的802.11p MAC和PHY
	NqosWaveMacHelper csmaWaveMac = NqosWaveMacHelper::Default();
	Wifi80211pHelper csmaWifi = Wifi80211pHelper::Default();
	csmaWifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
	                                 "DataMode", StringValue("OfdmRate12MbpsBW10MHz"),
	                                 "ControlMode", StringValue("OfdmRate12MbpsBW10MHz"),
	                                 "NonUnicastMode", StringValue("OfdmRate12MbpsBW10MHz"));

	// 为CSMA设备安装物理层和MAC层
	csmaDataDevices = csmaWifi.Install(csmaWifiPhy, csmaWaveMac, allNodesCon);



	if (m_tdma_enable) {
		//Config TDMA parameters.
		satmac_par_config();
	}
	//InstallInternetStack
    // Install the IP stack on the UEs
    NS_LOG_INFO ("Installing IP stack...");
    InternetStackHelper internet;
    internet.Install (allNodesCon);

    Ipv4AddressHelper addressAdhocData;
    addressAdhocData.SetBase("10.1.0.0", "255.255.0.0");
    tdmaIpInterfaces = addressAdhocData.Assign (tdmaDataDevices);

    // 为 CSMA 设备手动分配与 TDMA 相同的 IP 地址
    	for (uint32_t i = 0; i < allNodesCon.GetN(); ++i)
    	{
    	    Ptr<Node> node = allNodesCon.Get(i);

    	    // 获取 TDMA 接口的 IP 地址
    	    Ipv4Address tdmaIp = tdmaIpInterfaces.GetAddress(i);

    	    // 获取节点的 Ipv4 对象
    	    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    	    Ptr<NetDevice> csmaDevice = csmaDataDevices.Get(i);

    	    // 获取 CSMA 设备的接口索引
    	    int32_t ifIndex = ipv4->GetInterfaceForDevice(csmaDevice);
    	    if (ifIndex == -1)
    	    {
    	        ifIndex = ipv4->AddInterface(csmaDevice);
    	    }

    	    // 为 CSMA 设备分配与 TDMA 相同的 IP 地址
    	    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress(tdmaIp, Ipv4Mask("255.255.0.0"));
    	    ipv4->AddAddress(ifIndex, ipv4Addr);
    	    ipv4->SetMetric(ifIndex, 1); // 可以根据需要设置优先级
    	    ipv4->SetUp(ifIndex);
    	}

    	for (NodeContainer::Iterator i = allNodesCon.Begin(); i != allNodesCon.End(); ++i) {
    	    Ptr<Node> node = *i;
    	    // 获取当前节点的TDMA和CSMA设备

    	    Ptr<WifiNetDevice> tdmaDevice = DynamicCast<WifiNetDevice>(tdmaDataDevices.Get(i - allNodesCon.Begin()));
    	    Ptr<WifiNetDevice> csmaDevice = DynamicCast<WifiNetDevice>(csmaDataDevices.Get(i - allNodesCon.Begin()));
    	    NS_ASSERT(tdmaDevice != nullptr);
    	    NS_ASSERT(csmaDevice != nullptr);
    	    // 创建并初始化 MacLayerController
    	    Ptr<MacLayerController> macController = CreateObject<MacLayerController>();
    	    macController->Initialize(tdmaDevice, csmaDevice, node);
    	    node->AggregateObject(macController);
    	    // 打印调试信息
    	    //std::cout << "Node ID: " << node->GetId() << " MacLayerController: " << macController << std::endl;
    	}



    // Create Ns2MobilityHelper with the specified trace log file as parameter
//    Ns2MobilityHelper ns2 = Ns2MobilityHelper (m_traceFile);
//    ns2.Install (); // configure movements for each node, while reading trace file

    if (testing)
    {
    	//std::cout<<"@@@@@@TESTING MODE@@@@@@" << std::endl;

		MobilityHelper mobility;
		Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

		positionAlloc->Add (Vector (0.0, 30.0, 0.0));
		positionAlloc->Add (Vector (30.0, 0.0, 0.0));
		positionAlloc->Add (Vector (45.0, 0.0, 0.0));
		positionAlloc->Add (Vector (60.0, 0.0, 0.0));
		positionAlloc->Add (Vector (75.0, 0.0, 0.0));
		positionAlloc->Add (Vector (90.0, 0.0, 0.0));
		positionAlloc->Add (Vector (105.0, 0.0, 0.0));
		positionAlloc->Add (Vector (120.0, 0.0, 0.0));
		positionAlloc->Add (Vector (130.0, 0.0, 0.0));
		positionAlloc->Add (Vector (140.0, 0.0, 0.0));

		mobility.SetPositionAllocator (positionAlloc);
		mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
		mobility.Install (allNodesCon);

    } else {

   	if (numVeh <=201)
   		tracefile = tracefile_200;
   	else if (numVeh <=401)
   		tracefile = tracefile_400;
   	else if (numVeh <=601)
   		tracefile = tracefile_600;
   	else if (numVeh <=801)
   		tracefile = tracefile_800;
   	else if (numVeh <=1001)
   		tracefile = tracefile_1000;
      	else if (numVeh <=1201)
       	tracefile = tracefile_1200;
      	else if (numVeh <=1401)
       	tracefile = tracefile_1400;
      	else if (numVeh <=1601)
        	tracefile = tracefile_1600;

        std::cout<<"===Loading trace file...===" << tracefile << std::endl;

        Ns2MobilityHelper ns2 = Ns2MobilityHelper(tracefile);
        ns2.Install();
    }

    if (!bldgFile.empty()) {
    	std::cout<<"Loading buildings file " << bldgFile << std::endl;
    	Topology::LoadBuildings(bldgFile);
    }

    //if (!m_tdma_enable) {

    	  int chAccessMode = 0;
    	  m_txSafetyRanges.resize (1, 0);
    	  m_txSafetyRanges[0] = 160;

    	  m_waveBsmHelper.GetWaveBsmStats ()->SetLogging (0);
    	  // initially assume all nodes are not moving
    	  WaveBsmHelper::GetNodesMoving ().resize (numVeh, 1);

    	  if (testing)
			  m_waveBsmHelper.Install (tdmaIpInterfaces,
									   Seconds (simTime),
									   m_wavePacketSize,
									   Seconds (m_waveInterval),
									   // GPS accuracy (i.e, clock drift), in number of ns
									   m_gpsAccuracyNs,
									   m_txSafetyRanges,
									   chAccessMode,
									   // tx max delay before transmit, in ms
									   MilliSeconds (m_txMaxDelayMs));
    	  else
			  m_waveBsmHelper.Install (tdmaIpInterfaces,
									   Seconds (simTime),
									   m_wavePacketSize,
									   Seconds (m_waveInterval),
									   // GPS accuracy (i.e, clock drift), in number of ns
									   m_gpsAccuracyNs,
									   m_txSafetyRanges,
									   chAccessMode,
									   // tx max delay before transmit, in ms
									   MilliSeconds (m_txMaxDelayMs));

    	  // fix random number streams
    	  m_streamIndex += m_waveBsmHelper.AssignStreams (allNodesCon, m_streamIndex);


    //}

//    uint16_t application_port = 8000; // Application port to TX/RX
//    //Individual Socket Traffic Broadcast everyone
//    Ptr<Socket> host = Socket::CreateSocket(tdmaDataDevices.Get(0)->GetNode(),TypeId::LookupByName ("ns3::UdpSocketFactory"));
//    host->Bind(InetSocketAddress (tdmaIpInterfaces.GetAddress(0), application_port));
//    host->Connect(InetSocketAddress(tdmaIpInterfaces.GetAddress(1),application_port));
//    host->SetAllowBroadcast(true);
//    //host->ShutdownRecv();
//
//    Ptr<Socket> sink = Socket::CreateSocket(tdmaDataDevices.Get(1)->GetNode(),TypeId::LookupByName ("ns3::UdpSocketFactory"));
//    sink->Bind(InetSocketAddress (Ipv4Address::GetAny (), application_port));
//    sink->SetRecvCallback (MakeCallback (&ReceivePacket));

    //Ptr<TdmaSatmac> tdmaMac = DynamicCast<TdmaSatmac>( tdmaDataDevices.Get (0)->GetObject<WifiNetDevice> ()->GetMac () );

//    Ptr<NetDevice> t1 = tdmaDataDevices.Get (0);
//    Ptr<WifiNetDevice> t2 = t1->GetObject<WifiNetDevice> ();
//    Ptr<OcbWifiMac> t3 = DynamicCast<OcbWifiMac>(t2->GetMac ());
//    Ptr<TdmaSatmac> t4 = DynamicCast<TdmaSatmac> (t3->GetTdmaObject());
//    //t4->TraceConnectWithoutContext ("BCHTrace", MakeBoundCallback (&SidelinkV2xAnnouncementMacTrace, host));
//    t4->TraceConnectWithoutContext ("BCHTrace", MakeBoundCallback (&SidelinkV2xAnnouncementMacTrace, host));
    tdmaDataDevices.Get (0) ->GetObject<WifiNetDevice> () -> GetPhy ()->TraceConnectWithoutContext ("PhyRxCollisionDrop", MakeCallback (&PhyRxCollisionDropTrace));
    csmaDataDevices.Get (0) ->GetObject<WifiNetDevice> () -> GetPhy ()->TraceConnectWithoutContext ("PhyRxCollisionDrop", MakeCallback (&PhyRxCollisionDropTrace));
//PhyRxCollisionDrop
}


void CheckThroughput ()
{
	/*
//  double wavePDR = 0.0;
	double pir_mean;
	int pir_max, pir_min;
	pir_mean = m_waveBsmHelper.GetWaveBsmStats ()->GetPIR(&pir_max, &pir_min);
	m_waveBsmHelper.GetWaveBsmStats ()->ResetRecvTimeMap();

	//std::cout << pir_mean << " " << pir_max << " " << pir_min << std::endl;

  int wavePktsSent = m_waveBsmHelper.GetWaveBsmStats ()->GetTxPktCount ();
  int wavePktsReceived = m_waveBsmHelper.GetWaveBsmStats ()->GetRxPktCount ();


  std::ofstream out (lpfoutfile.c_str (), std::ios::app);

//  NS_LOG_UNCOND ("At t=" << (Simulator::Now ()).GetSeconds () << "s BSM_PDR1=" << wavePDR1_2 );

  //LPF output format
  out << "m "<<(Simulator::Now ()).GetMilliSeconds ()<<" t["<<0<<"] _"<<0<<
  	"_ DDD " <<wavePktsSent<<" "<<wavePktsReceived<<" "<<
  	0<<" "<<0<<" "<<0<<" "<<0
	<<" "<<"x:" << 0 <<" "<<"y"<< 0 <<" "<<0  << " "
	<< pir_mean << " " << pir_max << " " << pir_min << std::endl;

  out.close ();


  m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktCount (0);
  m_waveBsmHelper.GetWaveBsmStats ()->SetTxPktCount (0);
  for (int index = 1; index <= 1; index++)
    {
      m_waveBsmHelper.GetWaveBsmStats ()->SetExpectedRxPktCount (index, 0);
      m_waveBsmHelper.GetWaveBsmStats ()->SetRxPktInRangeCount (index, 0);
    }

//  double currentTime = (Simulator::Now ()).GetSeconds ();
//  if (currentTime <= (double) m_cumulativeBsmCaptureStart)
//    {
//      for (int index = 1; index <= 10; index++)
//        {
//          m_waveBsmHelper.GetWaveBsmStats ()->ResetTotalRxPktCounts (index);
//        }
//    }

  Simulator::Schedule (Seconds (1), &CheckThroughput);

  */
}
