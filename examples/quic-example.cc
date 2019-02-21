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
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/quic-utils.h"
#include "ns3/flow-monitor.h"
#include "ns3/flow-monitor-helper.h"

// Network topology (default)
//
//        n2 n3 n4              .
//         \ | /                .
//          \|/                 .
//     n1--- n0---n5            .
//          /|\                 .
//         / | \                .
//        n8 n7 n6              .
//

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StarAnimation");

int main(int argc, char *argv[])
{

  time_t t;

  // t = time(NULL);
  // RngSeedManager::SetSeed (t);

  t = time(NULL);
  RngSeedManager::SetRun(t);

  uint64_t maxBytes = 10;

  //
  // Set up some default values for the simulation.
  //
  // Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(137));

  // ??? try and stick 15kb/s into the data rate
  // Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("14kb/s"));

  //
  // Default number of nodes in the star.  Overridable by command line argument.
  //
  uint32_t nSpokes = 8;
  std::string animFile = "quic-star-animation.xml";
  uint8_t useIpv6 = 0;
  Ipv6Address ipv6AddressBase = Ipv6Address("2001::");
  Ipv6Prefix ipv6AddressPrefix = Ipv6Prefix(64);

  CommandLine cmd;
  cmd.AddValue("nSpokes", "Number of spoke nodes to place in the star", nSpokes);
  cmd.AddValue("animFile", "File Name for Animation Output", animFile);
  cmd.AddValue("useIpv6", "use Ipv6", useIpv6);

  cmd.Parse(argc, argv);

  NS_LOG_INFO("Build star topology.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  pointToPoint.SetChannelAttribute("Delay", StringValue("10ms"));

  Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

  nSpokes = uv->GetInteger(10,20);

  PointToPointStarHelper star(nSpokes, pointToPoint);

  NS_LOG_INFO("Install internet stack on all nodes.");
  InternetStackHelper internet;
  star.InstallStack(internet);

  NS_LOG_INFO("Assign IP Addresses.");
  if (useIpv6 == 0)
  {
    star.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"));
  }
  else
  {
    star.AssignIpv6Addresses(ipv6AddressBase, ipv6AddressPrefix);
  }

  //
  // configure each channel and netdevice attribute
  //


  for (uint32_t i = 0; i < star.SpokeCount(); ++i)
  {
    Ptr<Node> spoke = star.GetSpokeNode(i);
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

  Ptr<Node> hub = star.GetHub(); 

  for (uint32_t i = 0; i < star.SpokeCount(); ++i){
    Ptr<PointToPointNetDevice> device = DynamicCast<PointToPointNetDevice>(hub->GetDevice(i));

    Ptr<NormalRandomVariable> nv2 = CreateObject<NormalRandomVariable>();
    double meanErr = (50 + 49*sin((i+1)*37))/1000;
    double varErr = meanErr/3;
    double err = nv2->GetValue(meanErr, varErr, meanErr+3*varErr);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
    em->SetAttribute ("ErrorRate", DoubleValue (err));
    em->SetAttribute ("ErrorUnit", StringValue("ERROR_UNIT_PACKET"));
    device->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  }

  NS_LOG_INFO("Create applications.");
  //
  // Create a packet sink on the star "hub" to receive packets.
  //
  uint16_t port = 6121;
  Address hubLocalAddress;

  if (useIpv6 == 0)
  {
    hubLocalAddress = InetSocketAddress(Ipv4Address::GetAny(), port);
  }
  else
  {
    hubLocalAddress = Inet6SocketAddress(Ipv6Address::GetAny(), port);
  }

  // PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", hubLocalAddress);
  QuicServerHelper serverHelper ("ns3::UdpSocketFactory",
                         hubLocalAddress,
                         maxBytes);

  ApplicationContainer hubApp = serverHelper.Install(star.GetHub());
  hubApp.Start(Seconds(1.0));
  hubApp.Stop(Seconds(10.0));



  // OnOffHelper onOffHelper("ns3::TcpSocketFactory", Address());
  // onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  // onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));

  QuicClientHelper clientHelper ("ns3::UdpSocketFactory",
                         Address(), true, maxBytes);


  ApplicationContainer spokeApps;

  for (uint32_t i = 0; i < star.SpokeCount(); ++i)
  {
    AddressValue remoteAddress;
    if (useIpv6 == 0)
    {
      remoteAddress = AddressValue(InetSocketAddress(star.GetHubIpv4Address(i), port));
    }
    else
    {
      remoteAddress = AddressValue(Inet6SocketAddress(star.GetHubIpv6Address(i), port));
    }


    Ptr<NormalRandomVariable> nv = CreateObject<NormalRandomVariable>();
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();

    double meanSize = 110*(i+1) + 100*uv->GetValue(-1,1);
    double varSize = meanSize/3;
    uint64_t size = uint64_t(((nv->GetValue(meanSize, varSize, meanSize+3*varSize)))*1460*1000);

    // std::cout << size/1460 << std::endl;
    clientHelper.SetAttribute("MaxBytes", UintegerValue(size));
    clientHelper.SetAttribute("ServerAddress", remoteAddress);

    spokeApps.Add(clientHelper.Install(star.GetSpokeNode(i)));
  }

  spokeApps.Start(Seconds(1.0));
  spokeApps.Stop(Seconds(10.0));

  NS_LOG_INFO("Enable static global routing.");
  //
  // Turn on global static routing so we can actually be routed across the star.
  //
  if (useIpv6 == 0)
  {
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  }

  // Set the bounding box for animation
  star.BoundingBox(1, 1, 100, 100);

  // Create the animation object and configure for specified output

  AnimationInterface anim(animFile);

  
  // Ptr<FlowMonitor> flowMonitor;
  // FlowMonitorHelper flowHelper;
  // flowMonitor = flowHelper.InstallAll();
  // flowMonitor->SerializeToXmlFile("data/quic_star" + std::to_string(t) + ".xml", false, false);

  std::string trace_name = "quic-star.pcap";
  pointToPoint.EnablePcapAll(trace_name);
  NS_LOG_INFO("Run Simulation.");
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");

  return 0;
}
