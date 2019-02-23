/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/quic-utils.h"
#include "ns3/internet-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/quic-utils.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/* Default Network Topology
 * Primary traffic will be sent from n0 to n4.
 * Intereference traffic will be sent from n1 to n5.


  n0 ---             
         \          
          n2 ---- n3 ---- n4
         /          
  n1 ---              
*/
using namespace std;

void split(const string &s, vector<int> &sv, const char flag = ' ')
{
  sv.clear();
  istringstream iss(s);
  string temp;

  while (getline(iss, temp, flag))
  {
    sv.push_back(stoi(temp));
  }
  return;
}

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("quic-mapreduce");

int main(int argc, char *argv[])
{
  Time::SetResolution(Time::NS);
  bool verbose = false;
  int nLeft = 5;
  int nRight = 1;
  uint32_t bufferSize = 2000;
  bool pacing = false;
  uint32_t maxBytes = 100;
  std::string trace_name = "quic-mapreduce.pcap";
  std::string animFile = "quic-mapreduce.xml";
  std::string dataRate = "10Mbps";
  std::string delay = "5ms";
  bool flow_monitor = true;

  int round = 4;

  CommandLine cmd;
  cmd.AddValue("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue("pacing", "Enable packet pacing", pacing);
  cmd.AddValue("buffer", "Routers' buffer size (bytes)", bufferSize);
  cmd.AddValue("trace", "Trace file name", trace_name);
  cmd.AddValue("maxBytes", "Bytes", maxBytes);
  cmd.AddValue("leaf", "", nLeft);
  cmd.AddValue("dataRate", " ", dataRate);
  cmd.AddValue("delay", " ", delay);
  cmd.AddValue("round", " ", round);
  cmd.AddValue("flow_monitor", "Enable flow monitor", flow_monitor);

  cmd.Parse(argc, argv);

  if (verbose)
  {
    LogComponentEnable("PointToPointNetDevice", LOG_LEVEL_DEBUG);
    LogComponentEnable("DropTailQueue", LOG_LEVEL_DEBUG);
    LogComponentEnable("PacketSink", LOG_LEVEL_ALL);
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_ALL);
  }

  time_t t;
  t = time(NULL);
  RngSeedManager::SetRun(t);

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
// uv->GetInteger(1, 1)
  nLeft = uv->GetInteger(2, 5);
  nLeft = 1;
  // Create the point-to-point link helpers
  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetDeviceAttribute("DataRate", StringValue(dataRate));
  pointToPointRouter.SetChannelAttribute("Delay", StringValue(delay));
  pointToPointRouter.SetQueue("ns3::DropTailQueue", "Mode", StringValue("QUEUE_MODE_BYTES"), "MaxBytes", UintegerValue(bufferSize));

  PointToPointHelper pointToPointLeft;
  pointToPointLeft.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
  pointToPointLeft.SetChannelAttribute("Delay", StringValue("10ms"));

  PointToPointHelper pointToPointRight;
  pointToPointRight.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
  pointToPointRight.SetChannelAttribute("Delay", StringValue("5ms"));

  PointToPointDumbbellHelper dumbbell(nLeft, pointToPointLeft,
                                      nRight, pointToPointRight,
                                      pointToPointRouter);

  InternetStackHelper stack;
  stack.Install(dumbbell.GetRight());
  stack.Install(dumbbell.GetLeft());

  for (int i = 0; i < nLeft; i++)
  {
    stack.Install(dumbbell.GetLeft(i));
  }

  for (int i = 0; i < nRight; i++)
  {
    stack.Install(dumbbell.GetRight(i));
  }

  Ptr<Node> leftRouter = dumbbell.GetLeft();

  for (int i = 0; i < nLeft; ++i)
  {
    Ptr<PointToPointNetDevice> device = DynamicCast<PointToPointNetDevice>(leftRouter->GetDevice(i));

    Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel>(device->GetChannel());

    Ptr<NormalRandomVariable> nv = CreateObject<NormalRandomVariable>();
    Ptr<NormalRandomVariable> nv1 = CreateObject<NormalRandomVariable>();

    double meanDataRateKbps = 5000 + 4000 * sin((i + 1) * 10) * sin((i + 1) * 53);
    double varDataRateKbps = meanDataRateKbps / 6;
    double dataRateKbps = nv->GetValue(meanDataRateKbps, varDataRateKbps, meanDataRateKbps + 6 * varDataRateKbps);

    // std::cout << std::to_string(dataRateKbps)  << std::endl;
    device->SetDataRate(dataRateKbps * 1000);

    double meanDelay = 70 + 65 * sin((i + 1) * 127);
    double varDelay = meanDelay / 3;

    double delay = nv1->GetValue(meanDelay, varDelay, meanDelay + 3 * varDelay);
    // std::cout << std::to_string(delay)  << std::endl;

    channel->SetAttribute("Delay", StringValue(std::to_string(delay).append("ms")));

    Ptr<NormalRandomVariable> nv2 = CreateObject<NormalRandomVariable>();

    double meanErr = (50 + 49 * sin((i + 1) * 37)) / 1000;
    double varErr = meanErr / 3;
    double err = nv2->GetValue(meanErr, varErr, meanErr + 3 * varErr);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(err));
    em->SetAttribute("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
    device->SetAttribute("ReceiveErrorModel", PointerValue(em));
  }

  std::string leftIp = "10.4.1.0";

  if (round > 0 && round < 65536)
  {
    vector<int> sv;
    split(leftIp, sv, '.');
    sv[1] = round % 255;
    round /= 255;
    while (round > 0)
    {
      sv[0] += round % 255;
      round /= 255;
    }

    leftIp = "";
    leftIp.append(to_string(sv[0]));
    leftIp.append(".");
    leftIp.append(to_string(sv[1]));
    leftIp.append(".");
    leftIp.append(to_string(sv[2]));
    leftIp.append(".");
    leftIp.append(to_string(sv[3]));
  }

  // std::cout << leftIp << std::endl;
  dumbbell.AssignIpv4Addresses(Ipv4AddressHelper(leftIp.c_str(), "255.255.255.0"),
                               Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                               Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

  uint16_t port = 6121;

  ApplicationContainer spokeApps;

  QuicClientHelper clientHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(dumbbell.GetRightIpv4Address(0), port), true, maxBytes);

  uint64_t total_size = 0;
  for (int i = 0; i < nLeft; i++)
  {
    Ptr<NormalRandomVariable> nv = CreateObject<NormalRandomVariable>();
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

    double meanSize = 32 * (i + 1) + 10 * uv->GetValue(-1, 1);
    double varSize = meanSize / 3;
    uint64_t size = nv->GetInteger(3*meanSize, varSize, meanSize + 3 * varSize) * 1460;

    total_size += size;

    clientHelper.SetAttribute("MaxBytes", UintegerValue(size));
    spokeApps.Add(clientHelper.Install(dumbbell.GetLeft(i)));
  }

  // std::cout << total_size << std::endl;

  spokeApps.Start(Seconds(1.0));
  spokeApps.Stop(Seconds(5000.0));

  QuicServerHelper serverHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(dumbbell.GetRightIpv4Address(0), port),
                                maxBytes);
  ApplicationContainer serverApps = serverHelper.Install(dumbbell.GetRight(0));
  serverApps.Start(Seconds(0.0));
  serverApps.Stop(Seconds(5000.0));

  /* Build a routing database and initialize the routing tables of the nodes in the simulation.
  Makes all nodes in the simulation into routers. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  // dumbbell.BoundingBox(1, 1, 100, 100);
  // AnimationInterface anim(animFile);

  pointToPointRouter.EnablePcapAll(trace_name);
  if (flow_monitor)
  {
    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    Simulator::Stop(Seconds(5000));
    Simulator::Run();

    uint32_t txPacketsum = 0;
    uint32_t rxPacketsum = 0;
    uint32_t DropPacketsum = 0;
    uint32_t LostPacketsum = 0;
    double Delaysum = 0;

    flowMonitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());

    std::map<FlowId, FlowMonitor::FlowStats> stats = flowMonitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
    {
    
      txPacketsum += iter->second.txPackets;
      rxPacketsum += iter->second.rxPackets;
      LostPacketsum += iter->second.lostPackets;
      DropPacketsum += iter->second.packetsDropped.size();
      Delaysum += iter->second.delaySum.GetSeconds();
      Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(iter->first);
      // "Flow ID: "  iter->first
      std::cout << tuple.sourceAddress <<"-" << tuple.destinationAddress << ",";
      // "Tx Packets = " 
      std::cout << iter->second.txPackets << ",";
      // "Rx Packets = "
      std::cout << iter->second.rxPackets <<",";
      // "delay = "
      std::cout << (iter->second.timeLastTxPacket.GetSeconds() - iter->second.timeFirstRxPacket.GetSeconds()) << ",";
      // "Throughput="
      std::cout << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024 << "Kbps" << std::endl;
      
    }

    // std::cout << "\n";
    // std::cout << "\n";
    // std::cout << "  All Tx Packets: " << txPacketsum << "\n";
    // std::cout << "  All Rx Packets: " << rxPacketsum << "\n";
    // std::cout << "  All Delay: " << Delaysum / txPacketsum << "\n";
    // std::cout << "  All Lost Packets: " << LostPacketsum << "\n";
    // std::cout << "  All Drop Packets: " << DropPacketsum << "\n";
    // std::cout << "  Packets Delivery Ratio: " << ((rxPacketsum * 100) / txPacketsum) << "%"
    //           << "\n";
    // std::cout << "  Packets Lost Ratio: " << ((LostPacketsum * 100) / txPacketsum) << "%"
    //           << "\n";
    // flowMonitor->SerializeToXmlFile("data/quic_mapreduce_" + std::to_string(maxBytes) + "_" + dataRate + "_" + delay + "_" + std::to_string(nLeft) + ".xml", false, true);
  }
  // Stop as soon as there is no more scheduled events
  Simulator::Destroy();
  return 0;
}
