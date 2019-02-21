/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 CCSL IME-USP
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
 * Author: Diego Araújo <diegoamc@ime.usp.br>
 *
 */

#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/quic-utils.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Variants");

bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> ssThreshStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
Ptr<OutputStreamWrapper> nextTxStream;
Ptr<OutputStreamWrapper> nextRxStream;
Ptr<OutputStreamWrapper> inFlightStream;
uint32_t cWndValue;
uint32_t ssThreshValue;


static void
CwndTracer (uint32_t oldval, uint32_t newval)
{
  if (firstCwnd)
    {
      *cWndStream->GetStream () << "0.0 " << oldval << std::endl;
      firstCwnd = false;
    }
  *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  cWndValue = newval;

  if (!firstSshThr)
    {
      *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer (uint32_t oldval, uint32_t newval)
{
  if (firstSshThr)
    {
      *ssThreshStream->GetStream () << "0.0 " << oldval << std::endl;
      firstSshThr = false;
    }
  *ssThreshStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval << std::endl;
  ssThreshValue = newval;

  if (!firstCwnd)
    {
      *cWndStream->GetStream () << Simulator::Now ().GetSeconds () << " " << cWndValue << std::endl;
    }
}

static void
RttTracer (Time oldval, Time newval)
{
  if (firstRtt)
    {
      *rttStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRtt = false;
    }
  *rttStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
RtoTracer (Time oldval, Time newval)
{
  if (firstRto)
    {
      *rtoStream->GetStream () << "0.0 " << oldval.GetSeconds () << std::endl;
      firstRto = false;
    }
  *rtoStream->GetStream () << Simulator::Now ().GetSeconds () << " " << newval.GetSeconds () << std::endl;
}

static void
NextTxTracer (SequenceNumber32 old, SequenceNumber32 nextTx)
{
  *nextTxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
InFlightTracer (uint32_t old, uint32_t inFlight)
{
  *inFlightStream->GetStream () << Simulator::Now ().GetSeconds () << " " << inFlight << std::endl;
}

static void
NextRxTracer (SequenceNumber32 old, SequenceNumber32 nextRx)
{
  *nextRxStream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextRx << std::endl;
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  cWndStream = ascii.CreateFileStream (cwnd_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback (&CwndTracer));
}

static void
TraceSsThresh (std::string ssthresh_tr_file_name)
{
  AsciiTraceHelper ascii;
  ssThreshStream = ascii.CreateFileStream (ssthresh_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold", MakeCallback (&SsThreshTracer));
}

static void
TraceRtt (std::string rtt_tr_file_name)
{
  AsciiTraceHelper ascii;
  rttStream = ascii.CreateFileStream (rtt_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTT", MakeCallback (&RttTracer));
}

static void
TraceRto (std::string rto_tr_file_name)
{
  AsciiTraceHelper ascii;
  rtoStream = ascii.CreateFileStream (rto_tr_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/RTO", MakeCallback (&RtoTracer));
}

static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextTxStream = ascii.CreateFileStream (next_tx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence", MakeCallback (&NextTxTracer));
}

static void
TraceInFlight (std::string &in_flight_file_name)
{
  AsciiTraceHelper ascii;
  inFlightStream = ascii.CreateFileStream (in_flight_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight", MakeCallback (&InFlightTracer));
}


static void
TraceNextRx (std::string &next_rx_seq_file_name)
{
  AsciiTraceHelper ascii;
  nextRxStream = ascii.CreateFileStream (next_rx_seq_file_name.c_str ());
  Config::ConnectWithoutContext ("/NodeList/2/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence", MakeCallback (&NextRxTracer));
}

int main (int argc, char *argv[])
{
  std::string transport_prot = "TcpNewReno";
  double error_p = 0.0;
  std::string bandwidth = "2Mbps";
  std::string delay = "0.01ms";
  std::string access_bandwidth = "10Mbps";
  std::string access_delay = "45ms";
  bool tracing = true;
  std::string prefix_file_name = "TcpComparison";
  double data_mbytes = 1;
  uint32_t mtu_bytes = 400;
  uint16_t num_flows = 10;
  float duration = 100;
  uint32_t run = 0;
  bool flow_monitor = true;
  bool pcap = true;
  bool sack = true;
  std::string queue_disc_type = "ns3::PfifoFastQueueDisc";


  CommandLine cmd;
  cmd.AddValue ("transport_prot", "Transport protocol to compare with QUIC: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, TcpCubic ", transport_prot);
  cmd.AddValue ("error_p", "Packet error rate", error_p);
  cmd.AddValue ("bandwidth", "Bottleneck bandwidth", bandwidth);
  cmd.AddValue ("delay", "Bottleneck delay", delay);
  cmd.AddValue ("access_bandwidth", "Access link bandwidth", access_bandwidth);
  cmd.AddValue ("access_delay", "Access link delay", access_delay);
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.AddValue ("prefix_name", "Prefix of output trace file", prefix_file_name);
  cmd.AddValue ("data", "Number of Megabytes of data to transmit", data_mbytes);
  cmd.AddValue ("mtu", "Size of IP packets to send in bytes", mtu_bytes);
  cmd.AddValue ("num_flows", "Number of flows", num_flows);
  cmd.AddValue ("duration", "Time to allow flows to run in seconds", duration);
  cmd.AddValue ("run", "Run index (for setting repeatable seeds)", run);
  cmd.AddValue ("flow_monitor", "Enable flow monitor", flow_monitor);
  cmd.AddValue ("pcap_tracing", "Enable or disable PCAP tracing", pcap);
  cmd.AddValue ("queue_disc_type", "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)", queue_disc_type);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.Parse (argc, argv);

  transport_prot = std::string ("ns3::") + transport_prot;

  SeedManager::SetSeed (1);
  SeedManager::SetRun (run);

  // User may find it convenient to enable logging
  //LogComponentEnable("TcpComparison", LOG_LEVEL_ALL);
  //LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
  //LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header ();
  uint32_t ip_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader ();
  uint32_t tcp_header = temp_header->GetSerializedSize ();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
  NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

// Set the simulation start and stop time
  float start_time = 0.1;
  float stop_time = start_time + duration;

  // 4 MB of TCP buffer
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  // Select TCP variant
  if (transport_prot.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid), "TypeId " << transport_prot << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (transport_prot)));
    }

  // Create gateways, sources, and sinks
  NodeContainer gateways;
  gateways.Create (1);
  NodeContainer sources;
  sources.Create (num_flows);
  NodeContainer sinks;
  sinks.Create (num_flows);
  NodeContainer server;
  server.Create (1);
  NodeContainer client;
  client.Create (1);

  // Configure the error model
  // Here we use RateErrorModel with packet error rate
  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable> ();
  uv->SetStream (50);
  RateErrorModel error_model;
  error_model.SetRandomVariable (uv);
  error_model.SetUnit (RateErrorModel::ERROR_UNIT_PACKET);
  error_model.SetRate (error_p);

  PointToPointHelper UnReLink;
  UnReLink.SetDeviceAttribute ("DataRate", StringValue (bandwidth));
  UnReLink.SetChannelAttribute ("Delay", StringValue (delay));
  UnReLink.SetDeviceAttribute ("ReceiveErrorModel", PointerValue (&error_model));


  InternetStackHelper stack;
  stack.InstallAll ();

  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::PfifoFastQueueDisc");

  TrafficControlHelper tchCoDel;
  tchCoDel.SetRootQueueDisc ("ns3::CoDelQueueDisc");

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  // Configure the sources and sinks net devices
  // and the channels between the sources/sinks and the gateways
  PointToPointHelper LocalLink;
  LocalLink.SetDeviceAttribute ("DataRate", StringValue (access_bandwidth));
  LocalLink.SetChannelAttribute ("Delay", StringValue (access_delay));

  Ipv4InterfaceContainer sink_interfaces;

  DataRate access_b (access_bandwidth);
  DataRate bottle_b (bandwidth);
  Time access_d (access_delay);
  Time bottle_d (delay);

  Config::SetDefault ("ns3::CoDelQueueDisc::Mode", EnumValue (CoDelQueueDisc::QUEUE_DISC_MODE_BYTES));

  uint32_t size = (std::min (access_b, bottle_b).GetBitRate () / 8) *
    ((access_d + bottle_d) * 2).GetSeconds ();

  Config::SetDefault ("ns3::PfifoFastQueueDisc::Limit", UintegerValue (size / mtu_bytes));
  Config::SetDefault ("ns3::CoDelQueueDisc::MaxBytes", UintegerValue (size));
  NodeContainer gateway_server;
  gateway_server.Add(gateways);
  gateway_server.Add(server);
  NetDeviceContainer server_device = LocalLink.Install (gateway_server);
  Ipv4InterfaceContainer quic_interfaces = address.Assign (server_device);

  NodeContainer gateway_client;
  gateway_client.Add(gateways);
  gateway_client.Add(client);
  NetDeviceContainer client_device = UnReLink.Install (gateway_client);
  quic_interfaces.Add (address.Assign (client_device));

  address.SetBase ("10.2.1.0", "255.255.255.0");
  for (int i = 0; i < num_flows; i++)
    {
      NetDeviceContainer devices;
      devices = LocalLink.Install (sources.Get (i), gateways.Get (0));
      tchPfifo.Install (devices);
      address.NewNetwork ();
      Ipv4InterfaceContainer interfaces = address.Assign (devices);

      devices = UnReLink.Install (gateways.Get (0), sinks.Get (i));
      if (queue_disc_type.compare ("ns3::PfifoFastQueueDisc") == 0)
        {
          tchPfifo.Install (devices);
        }
      else if (queue_disc_type.compare ("ns3::CoDelQueueDisc") == 0)
        {
          tchCoDel.Install (devices);
        }
      else
        {
          NS_FATAL_ERROR ("Queue not recognized. Allowed values are ns3::CoDelQueueDisc or ns3::PfifoFastQueueDisc");
        }
      address.NewNetwork ();
      interfaces = address.Assign (devices);
      sink_interfaces.Add (interfaces.Get (1));
    }

  NS_LOG_INFO ("Initialize Global Routing.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  uint16_t quic_port = 6121;
  QuicClientHelper clientHelper ("ns3::UdpSocketFactory",
      InetSocketAddress(quic_interfaces.GetAddress (1), quic_port),
      true, uint64_t(data_mbytes * 1000000));

  QuicServerHelper serverHelper ("ns3::UdpSocketFactory",
      InetSocketAddress (quic_interfaces.GetAddress (1), quic_port),
      uint64_t(data_mbytes * 1000000));

  ApplicationContainer serverApp = serverHelper.Install (server.Get (0));
  serverApp.Start (Seconds (0.0));

  ApplicationContainer clientApp = clientHelper.Install (client.Get (0));
  clientApp.Start (Seconds (0.0));

  uint16_t port = 50000;
  Address sinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", sinkLocalAddress);

  for (uint16_t i = 0; i < sources.GetN (); i++)
    {
      AddressValue remoteAddress (InetSocketAddress (sink_interfaces.GetAddress (i, 0), port));
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (tcp_adu_size));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (tcp_adu_size));
      ftp.SetAttribute ("MaxBytes", UintegerValue (int(data_mbytes * 1000000)));

      ApplicationContainer sourceApp = ftp.Install (sources.Get (i));
      sourceApp.Start (Seconds (0.0));
      //sourceApp.Start (Seconds (start_time * i));
      //sourceApp.Stop (Seconds (stop_time - 3));

      sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      ApplicationContainer sinkApp = sinkHelper.Install (sinks);
      sinkApp.Start (Seconds (0.0));
      //sinkApp.Start (Seconds (start_time * i));
      //sinkApp.Stop (Seconds (stop_time));
    }

  // Set up tracing if enabled
  if (tracing)
    {
      std::ofstream ascii;
      Ptr<OutputStreamWrapper> ascii_wrap;
      ascii.open ((prefix_file_name + "-ascii").c_str ());
      ascii_wrap = new OutputStreamWrapper ((prefix_file_name + "-ascii").c_str (),
                                            std::ios::out);
      stack.EnableAsciiIpv4All (ascii_wrap);

      Simulator::Schedule (Seconds (0.00001), &TraceCwnd, prefix_file_name + "-cwnd.data");
      Simulator::Schedule (Seconds (0.00001), &TraceSsThresh, prefix_file_name + "-ssth.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRtt, prefix_file_name + "-rtt.data");
      Simulator::Schedule (Seconds (0.00001), &TraceRto, prefix_file_name + "-rto.data");
      Simulator::Schedule (Seconds (0.00001), &TraceNextTx, prefix_file_name + "-next-tx.data");
      Simulator::Schedule (Seconds (0.00001), &TraceInFlight, prefix_file_name + "-inflight.data");
      Simulator::Schedule (Seconds (0.1), &TraceNextRx, prefix_file_name + "-next-rx.data");
    }

  if (pcap)
    {
      UnReLink.EnablePcapAll (prefix_file_name, true);
      LocalLink.EnablePcapAll (prefix_file_name, true);
    }

  // Flow monitor
  FlowMonitorHelper flowHelper;
  if (flow_monitor)
    {
      flowHelper.InstallAll ();
    }

  Simulator::Stop (Seconds (stop_time + 2000));
  Simulator::Run ();

  if (flow_monitor)
    {
      flowHelper.SerializeToXmlFile ("data/" + prefix_file_name + "_" + transport_prot + "_" + std::to_string(data_mbytes) + ".xml", false, false);
    }

  Simulator::Destroy ();
  return 0;
}
