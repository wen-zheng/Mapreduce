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
#include "ns3/netanim-module.h"


/* Default Network Topology
 * Primary traffic will be sent from n0 to n4.
 * Intereference traffic will be sent from n1 to n5.


  n0 ---              --- n4
         \          /
          n2 ---- n3
         /          \
  n1 ---              --- n5
*/


using namespace ns3;

uint64_t bytes_dropped(Ptr<FlowMonitor> fm) {
  uint64_t count = 0;
  for(auto e : fm->GetFlowStats())
    for(uint64_t tot : e.second.bytesDropped)
      count += tot;
  return count;
}

Time last_time(Ptr<FlowMonitor> fm) {
  Time t = Time::Min();
  for(auto e : fm->GetFlowStats())
    t = std::max(t, e.second.timeLastRxPacket);
  return t;
}

NS_LOG_COMPONENT_DEFINE ("PacketPacingExample");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  bool verbose = false;
  int nLeaf = 5;
  int nRight = 1;
  uint32_t bufferSize = 2000;
  bool pacing = false;
  uint32_t maxBytes = 1000000;
  std::string trace_name = "quic-example-dumbbell.pcap";
  std::string animFile = "quic-example-dumbbell-animation.xml";
  std::string dataRate = "1Mbps";
  std::string delay = "100ms";

  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("pacing", "Enable packet pacing", pacing);
  cmd.AddValue ("buffer", "Routers' buffer size (bytes)", bufferSize);
  cmd.AddValue ("trace", "Trace file name", trace_name);
  cmd.AddValue ("maxBytes", "Bytes", maxBytes);
  cmd.AddValue ("leaf", "", nLeaf);
  cmd.AddValue("dataRate", " ", dataRate);
  cmd.AddValue("delay", " ", delay);
  cmd.Parse (argc,argv);

  if (verbose)
    {
      LogComponentEnable ("PointToPointNetDevice", LOG_LEVEL_DEBUG);
      //LogComponentEnable ("DropTailQueue", LOG_LEVEL_DEBUG);
      //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
      //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_ALL);
    }

  time_t t;
  t = time(NULL);
  RngSeedManager::SetRun(t);

  // Create the point-to-point link helpers, links from USP to UK
  PointToPointHelper pointToPointRouter;
  pointToPointRouter.SetDeviceAttribute  ("DataRate", StringValue (dataRate));
  pointToPointRouter.SetChannelAttribute ("Delay", StringValue (delay));
  pointToPointRouter.SetQueue ("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_BYTES"), "MaxBytes", UintegerValue(bufferSize));

  PointToPointHelper pointToPointLeaf;
  pointToPointLeaf.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeaf.SetChannelAttribute   ("Delay", StringValue ("50ms"));

  PointToPointHelper pointToPointLeafR;
  pointToPointLeafR.SetDeviceAttribute    ("DataRate", StringValue ("10Mbps"));
  pointToPointLeafR.SetChannelAttribute   ("Delay", StringValue ("5ms"));

  PointToPointDumbbellHelper dumbbell (nLeaf, pointToPointLeaf,
                                nRight, pointToPointLeafR,
                                pointToPointRouter);

  InternetStackHelper stack;
  stack.Install(dumbbell.GetRight());
  stack.Install(dumbbell.GetLeft());

  for(int i = 0; i < nLeaf; i++) {
    stack.Install(dumbbell.GetLeft(i));
  }

  for(int i = 0; i < nRight; i++) {
    stack.Install(dumbbell.GetRight(i));
  }
  
  for (int i = 0; i < nLeaf; ++i)
  {
    Ptr<Node> spoke = dumbbell.GetLeft(i);
    Ptr<PointToPointNetDevice> device = DynamicCast<PointToPointNetDevice>(spoke->GetDevice(0));
    Ptr<PointToPointChannel> channel = DynamicCast<PointToPointChannel>(device->GetChannel());

    
    Ptr<NormalRandomVariable> nv = CreateObject<NormalRandomVariable>();
    Ptr<NormalRandomVariable> nv1 = CreateObject<NormalRandomVariable>();
    

    double meanDataRateKbps = 5000 + 4000*sin((i+1)*10)*sin((i+1)*53);
    // uv->GetValue((i+1)*1000, (i+3)*1000);
    double varDataRateKbps = meanDataRateKbps/6;
    // uv->GetValue(meanDataRateKbps/10, meanDataRateKbps/6);
    double dataRateKbps = nv->GetValue(meanDataRateKbps, varDataRateKbps, meanDataRateKbps+6*varDataRateKbps);

    // std::cout << std::to_string(dataRateKbps)  << std::endl;
    device->SetDataRate(dataRateKbps * 1000);

    // nv->Initialize();
    double meanDelay = 70 + 65*sin((i+1)*127);
    double varDelay = meanDelay/3;
    // uv->GetValue(meanDelay/10, meanDelay/6);

    double delay = nv1->GetValue(meanDelay, varDelay, meanDelay+3*varDelay);
    // std::cout << std::to_string(delay)  << std::endl;
    channel->SetAttribute("Delay", StringValue(std::to_string(delay).append("ms")));


  }

  Ptr<Node> leftRouter = dumbbell.GetLeft();

  for (int i = 0; i < nLeaf; ++i){
    Ptr<PointToPointNetDevice> device = DynamicCast<PointToPointNetDevice>(leftRouter->GetDevice(i));
    Ptr<NormalRandomVariable> nv2 = CreateObject<NormalRandomVariable>();

    double meanErr = (50 + 49*sin((i+1)*37))/1000;
    double varErr = meanErr/3;
    double err = nv2->GetValue(meanErr, varErr, meanErr+3*varErr);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (err));
    em->SetAttribute ("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
    device->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  }

  dumbbell.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"),
                                Ipv4AddressHelper ("10.2.1.0", "255.255.255.0"),
                                Ipv4AddressHelper ("10.3.1.0", "255.255.255.0"));

  uint16_t port = 6121;

  // Client for (n0) and (n1)

  ApplicationContainer spokeApps;

  QuicClientHelper clientHelper ("ns3::UdpSocketFactory",
        InetSocketAddress(dumbbell.GetRightIpv4Address(0), port), true, maxBytes);

  for(int i = 0; i < nLeaf; i++) {
    // QuicClientHelper clientHelper ("ns3::UdpSocketFactory",
    //     InetSocketAddress(dumbbell.GetRightIpv4Address(0), port), true, maxBytes);

    Ptr<NormalRandomVariable> nv = CreateObject<NormalRandomVariable>();
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

    double meanSize = 1100*(i+1) + 1000*uv->GetValue(-1,1);
    double varSize = meanSize/3;
    uint64_t size = uint64_t(((nv->GetValue(meanSize, varSize, meanSize+3*varSize)))*1460);

    // std::cout << size/1460 << std::endl;
    clientHelper.SetAttribute("MaxBytes", UintegerValue(size));
    spokeApps.Add(clientHelper.Install (dumbbell.GetLeft (i)));

    // app.Start (Seconds (0.0));
  }

  spokeApps.Start(Seconds(1.0));
  spokeApps.Stop(Seconds(10.0));

  QuicServerHelper serverHelper ("ns3::UdpSocketFactory",
                          InetSocketAddress ( dumbbell.GetRightIpv4Address(0), port),
                          maxBytes);
  ApplicationContainer serverApps = serverHelper.Install (dumbbell.GetRight (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop(Seconds(10.0));

  /* Build a routing database and initialize the routing tables of the nodes in the simulation.
  Makes all nodes in the simulation into routers. */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  dumbbell.BoundingBox(1, 1, 100, 100);
  AnimationInterface anim(animFile);

  //AsciiTraceHelper ascii;
  pointToPointRouter.EnablePcapAll(trace_name);

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  // Stop as soon as there is no more scheduled events

  Simulator::Stop(Seconds(5000));
  Simulator::Run ();

  // std::cout << "Now: " << last_time(flowMonitor).As(Time::S) << std::endl;
  // std::cout << "Bytes dropped: " << bytes_dropped(flowMonitor) << std::endl;
  flowMonitor->SerializeToXmlFile("data/quic_dumbbell_" + std::to_string(maxBytes) + "_" + dataRate + "_" + delay + "_" + std::to_string(nLeaf) + ".xml", true, true);
  Simulator::Destroy ();
  return 0;
}
