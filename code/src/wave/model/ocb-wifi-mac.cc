/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2013 Dalian University of Technology
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *         Junling Bu <linlinjavaer@gmail.com>
 */

#include "ns3/event-id.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/mac-rx-middle.h"
#include "ns3/ht-capabilities.h"
#include "ns3/vht-capabilities.h"
#include "wave-mac-low.h"
#include "ocb-wifi-mac.h"
#include "vendor-specific-action.h"
#include "higher-tx-tag.h"
#include "ns3/wifi-net-device.h"
#include "ns3/AperiodicTag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OcbWifiMac");

NS_OBJECT_ENSURE_REGISTERED (OcbWifiMac);

/// Wildcard BSSID
const static Mac48Address WILDCARD_BSSID = Mac48Address::GetBroadcast ();

TypeId
OcbWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OcbWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wave")
    .AddConstructor<OcbWifiMac> ()

    .AddAttribute ("Tdma", "The Tdma layer attached to this device.",
                   PointerValue (),
                   MakePointerAccessor (&OcbWifiMac::SetTdmaObject,
                                        &OcbWifiMac::GetTdmaObject),
                   MakePointerChecker<TdmaSatmac> ())
  ;
  return tid;
}

OcbWifiMac::OcbWifiMac (int tdma_enable)
{

}

OcbWifiMac::OcbWifiMac (void)
{
  NS_LOG_FUNCTION (this);
  // Let the lower layers know that we are acting as an OCB node
  SetTypeOfStation (OCB);
  // BSSID is still needed in the low part of MAC
  RegularWifiMac::SetBssid (WILDCARD_BSSID);
}

OcbWifiMac::~OcbWifiMac (void)
{
  NS_LOG_FUNCTION (this);
}

void OcbWifiMac::Initialize()
{

}



void OcbWifiMac::InitialTdma()
{
	m_tdma = CreateObject<TdmaSatmac> ();
	m_tdma->setWifiMacLow(m_low);
	m_tdma->setWifimaclowFlag(1);
	m_tdma->SetGlobalSti(this->getNode()->GetId()+1);
	m_tdma->setWifiPhy(this->getPhy());
	m_rxMiddle->SetForwardCallback (MakeCallback (&TdmaSatmac::Receive, m_tdma));

	m_tdma->SetForwardUpCallback(MakeCallback (&OcbWifiMac::Receive, this));
	m_tdma->Start();
}

void
OcbWifiMac::SendVsc (Ptr<Packet> vsc, Mac48Address peer, OrganizationIdentifier oi)
{
  NS_LOG_FUNCTION (this << vsc << peer << oi);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (peer);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (WILDCARD_BSSID);
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  VendorSpecificActionHeader vsa;
  vsa.SetOrganizationIdentifier (oi);
  vsc->AddHeader (vsa);

  if (GetQosSupported ())
    {
      uint8_t tid = QosUtilsGetTidForPacket (vsc);
      tid = tid > 7 ? 0 : tid;
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (vsc, hdr);
    }
  else
    {
	  if (getTdmaEnable())
		 m_tdma->Queue(vsc, hdr);
	  else
		 m_txop->Queue (vsc, hdr);
    }
}

void
OcbWifiMac::AddReceiveVscCallback (OrganizationIdentifier oi, VscCallback cb)
{
  NS_LOG_FUNCTION (this << oi << &cb);
  m_vscManager.RegisterVscCallback (oi, cb);
}

void
OcbWifiMac::RemoveReceiveVscCallback (OrganizationIdentifier oi)
{
  NS_LOG_FUNCTION (this << oi);
  m_vscManager.DeregisterVscCallback (oi);
}

void
OcbWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_WARN ("in OCB mode we should not call SetSsid");
}

Ssid
OcbWifiMac::GetSsid (void) const
{
  NS_LOG_WARN ("in OCB mode we should not call GetSsid");
  // we really do not want to return ssid, however we have to provide
  return RegularWifiMac::GetSsid ();
}


void
OcbWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_WARN ("in OCB mode we should not call SetBsid");
}

Mac48Address
OcbWifiMac::GetBssid (void) const
{
  NS_LOG_WARN ("in OCB mode we should not call GetBssid");
  return WILDCARD_BSSID;
}

void
OcbWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  // The approach taken here is that, from the point of view of a STA
  // in OCB mode, the link is always up, so we immediately invoke the
  // callback if one is set
  linkUp ();
}

void
OcbWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this << &linkDown);
  RegularWifiMac::SetLinkDownCallback (linkDown);
  NS_LOG_WARN ("in OCB mode the like will never down, so linkDown will never be called");
}

void
OcbWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //std::cout<<"1"<<std::endl;
  if (m_stationManager->IsBrandNew (to))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      if (GetHtSupported () || GetVhtSupported ())
        {
          m_stationManager->AddAllSupportedMcs (to);
          m_stationManager->AddStationHtCapabilities (to, GetHtCapabilities());
        }
      if (GetVhtSupported ())
        {
          m_stationManager->AddStationVhtCapabilities (to, GetVhtCapabilities());
        }
      m_stationManager->AddAllSupportedModes (to);
      m_stationManager->RecordDisassociated (to);
    }

  WifiMacHeader hdr;
  if (getTdmaEnable()) {
	  hdr.SetDuration(Seconds(0));
	  hdr.SetNoMoreFragments();
	  hdr.SetNoRetry();
	//  hdr.SetNoOrder();
	  //hdr.SetSequenceNumber(0);
	  //hdr.SetQosNoAck();
	  hdr.SetQosNoAmsdu();
  }
  // If we are not a QoS STA then we definitely want to use AC_BE to
  // transmit the packet. A TID of zero will map to AC_BE (through \c
  // QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  if (GetQosSupported ())
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      // About transmission of multiple frames,
      // in Ad-hoc mode TXOP is not supported for now, so TxopLimit=0;
      // however in OCB mode, 802.11p do not allow transmit multiple frames
      // so TxopLimit must equal 0
      hdr.SetQosTxopLimit (0);

      // Fill in the QoS control field in the MAC header
      tid = QosUtilsGetTidForPacket (packet);
      // Any value greater than 7 is invalid and likely indicates that
      // the packet had no QoS tag, so we revert to zero, which'll
      // mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetType (WIFI_MAC_DATA);
    }

  if (GetHtSupported () || GetVhtSupported ())
    {
      hdr.SetNoOrder (); // explicitly set to 0 for the time being since HT/VHT/HE control field is not yet implemented (set it to 1 when implemented)
    }
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (WILDCARD_BSSID);
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  if (GetQosSupported ())
    {
      // Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
	  if (getTdmaEnable())
	  {
		  //std::cout<<"tdma "<<this->getNode()->GetId()<<std::endl;
		  Ptr<WifiNetDevice> tdma = DynamicCast<WifiNetDevice>(this->getNode()->GetDevice(0));
		  m_macLayerController = this->getNode()->GetObject<MacLayerController>();
	//	  if(m_macLayerController->GetCurrentDevice() == tdma)
	//	  {
			  m_tdma->Queue(packet, hdr);
	//	  }
	  }
	  else
	  {
		 //std::cout<<"csma "<<this->getNode()->GetId()<<" Time: "<<Simulator::Now().GetMicroSeconds()<<std::endl;
		 AperiodicTag tag;
		 if(packet->PeekPacketTag(tag))
		 {
			 //std::cout<<"csma "<<this->getNode()->GetId()<<" Time: "<<Simulator::Now().GetMicroSeconds()<<std::endl;
			 m_txop->Queue (packet, hdr);
		 }
	  }
    }
}

/*
 * see 802.11p-2010 chapter 11.19
 * here we only care about data packet and vsa management frame
 */
void
OcbWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  //NS_ASSERT (hdr->GetAddr3 () == WILDCARD_BSSID);
  Mac48Address from = hdr->GetAddr2 ();
  Mac48Address to = hdr->GetAddr1 ();
  //std::cout<<"1"<<std::endl;

  if (m_stationManager->IsBrandNew (from))
    {
      //In ad hoc mode, we assume that every destination supports all
      //the rates we support.
      if (GetHtSupported () || GetVhtSupported ())
        {
          m_stationManager->AddAllSupportedMcs (from);
          m_stationManager->AddStationHtCapabilities (from, GetHtCapabilities());
        }
      if (GetVhtSupported ())
        {
          m_stationManager->AddStationVhtCapabilities (from, GetVhtCapabilities());
        }
      m_stationManager->AddAllSupportedModes (from);
      m_stationManager->RecordDisassociated (from);
    }

  if (hdr->IsData ())
    {
      if (hdr->IsQosData () && hdr->IsQosAmsdu ())
        {
          NS_LOG_DEBUG ("Received A-MSDU from" << from);
          DeaggregateAmsduAndForward (packet, hdr);
        }
      else
        {
          ForwardUp (packet, from, to);
        }
      return;
    }

  // why put check here, not before "if (hdr->IsData ())" ?
  // because WifiNetDevice::ForwardUp needs to m_promiscRx data packet
  // and will filter data packet for itself
  // so we need to filter management frame
  if (to != GetAddress () && !to.IsGroup ())
    {
      NS_LOG_LOGIC ("the management frame is not for us");
      NotifyRxDrop (packet);
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      // yes, we only care about VendorSpecificAction frame in OCB mode
      // other management frames will be handled by RegularWifiMac::Receive
      VendorSpecificActionHeader vsaHdr;
      packet->PeekHeader (vsaHdr);
      if (vsaHdr.GetCategory () == CATEGORY_OF_VSA)
        {
          VendorSpecificActionHeader vsa;
          packet->RemoveHeader (vsa);
          OrganizationIdentifier oi = vsa.GetOrganizationIdentifier ();
          VscCallback cb = m_vscManager.FindVscCallback (oi);

          if (cb.IsNull ())
            {
              NS_LOG_DEBUG ("cannot find VscCallback for OrganizationIdentifier=" << oi);
              return;
            }
          bool succeed = cb (this, oi,packet, from);

          if (!succeed)
            {
              NS_LOG_DEBUG ("vsc callback could not handle the packet successfully");
            }

          return;
        }
    }
  // Invoke the receive handler of our parent class to deal with any
  // other frames. Specifically, this will handle Block Ack-related
  // Management Action frames.

  RegularWifiMac::Receive (packet, hdr);
}

void
OcbWifiMac::ConfigureEdca (uint32_t cwmin, uint32_t cwmax, uint32_t aifsn, enum AcIndex ac)
{
  NS_LOG_FUNCTION (this << cwmin << cwmax << aifsn << ac);
  Ptr<Txop> dcf;
  switch (ac)
    {
    case AC_VO:
      dcf = RegularWifiMac::GetVOQueue ();
      dcf->SetMinCw ((cwmin + 1) / 4 - 1);
      dcf->SetMaxCw ((cwmin + 1) / 2 - 1);
      dcf->SetAifsn (aifsn);
      break;
    case AC_VI:
      dcf = RegularWifiMac::GetVIQueue ();
      dcf->SetMinCw ((cwmin + 1) / 2 - 1);
      dcf->SetMaxCw (cwmin);
      dcf->SetAifsn (aifsn);
      break;
    case AC_BE:
      dcf = RegularWifiMac::GetBEQueue ();
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (aifsn);
      break;
    case AC_BK:
      dcf = RegularWifiMac::GetBKQueue ();
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (aifsn);
      break;
    case AC_BE_NQOS:
      dcf = RegularWifiMac::GetTxop ();
      dcf->SetMinCw (cwmin);
      dcf->SetMaxCw (cwmax);
      dcf->SetAifsn (aifsn);
      break;
    case AC_UNDEF:
      NS_FATAL_ERROR ("I don't know what to do with this");
      break;
    }
}

void
OcbWifiMac::FinishConfigureStandard (enum WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  NS_ASSERT ((standard == WIFI_PHY_STANDARD_80211_10MHZ)
             || (standard == WIFI_PHY_STANDARD_80211a));

  uint32_t cwmin = 15;
  uint32_t cwmax = 1023;

  // The special value of AC_BE_NQOS which exists in the Access
  // Category enumeration allows us to configure plain old DCF.
  ConfigureEdca (cwmin, cwmax, 2, AC_BE_NQOS);

  // Now we configure the EDCA functions
  // see IEEE802.11p-2010 section 7.3.2.29
  // Wave CCH and SCHs set default 802.11p EDCA
  ConfigureEdca (cwmin, cwmax, 2, AC_VO);
  ConfigureEdca (cwmin, cwmax, 3, AC_VI);
  ConfigureEdca (cwmin, cwmax, 6, AC_BE);
  ConfigureEdca (cwmin, cwmax, 9, AC_BK);
}


void
OcbWifiMac::Suspend (void)
{
  NS_LOG_FUNCTION (this);
  m_channelAccessManager->NotifySleepNow ();
  m_low->NotifySleepNow ();
}

void
OcbWifiMac::Resume (void)
{
  NS_LOG_FUNCTION (this);
  // wake-up operation is not required in m_low object
  m_channelAccessManager->NotifyWakeupNow ();
}

void
OcbWifiMac::MakeVirtualBusy (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  m_channelAccessManager->NotifyMaybeCcaBusyStartNow (duration);
}

void
OcbWifiMac::CancleTx (enum AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);
  Ptr<QosTxop> queue = m_edca.find (ac)->second;
  NS_ASSERT (queue != 0);
  // reset and flush queue
  queue->NotifyChannelSwitching ();
}

void
OcbWifiMac::Reset (void)
{
  NS_LOG_FUNCTION (this);
  // The switching event is used to notify MAC entity reset its operation.
  m_channelAccessManager->NotifySwitchingStartNow (Time (0));
  m_low->NotifySwitchingStartNow (Time (0));
}

void
OcbWifiMac::EnableForWave (Ptr<WaveNetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  // To extend current OcbWifiMac for WAVE 1609.4, we shall use WaveMacLow instead of MacLow
  m_low = CreateObject<WaveMacLow> ();
  (DynamicCast<WaveMacLow> (m_low))->SetWaveNetDevice (device);
  m_low->SetRxCallback (MakeCallback (&MacRxMiddle::Receive, m_rxMiddle));
  m_channelAccessManager->SetupLow (m_low);
  m_txop->SetMacLow (m_low);
  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetMacLow (m_low);
      i->second->CompleteConfig ();
    }
}

void OcbWifiMac::SlotGroupEnque(Ptr<const Packet> packet,const WifiMacHeader &hdr)
{
	m_txop->Queue(packet, hdr);
}


void OcbWifiMac::TdmaEnque(Ptr<const Packet> packet,const WifiMacHeader &hdr)
{
	m_tdma->Queue(packet, hdr);
}
} // namespace ns3
