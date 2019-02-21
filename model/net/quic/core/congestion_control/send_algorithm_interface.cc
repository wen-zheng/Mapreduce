// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/congestion_control/send_algorithm_interface.h"

#include "net/quic/core/congestion_control/bbr_sender.h"
#include "net/quic/core/congestion_control/tcp_cubic_sender_bytes.h"
#include "net/quic/core/congestion_control/tcp_cubic_sender_packets.h"
#include "net/quic/core/quic_packets.h"
#include "net/quic/platform/api/quic_bug_tracker.h"
#include "net/quic/platform/api/quic_flag_utils.h"
#include "net/quic/platform/api/quic_flags.h"
#include "net/quic/platform/api/quic_pcc_sender.h"

namespace net {

class RttStats;

// Factory for send side congestion control algorithm.
SendAlgorithmInterface* SendAlgorithmInterface::Create(
    const QuicClock* clock,
    const RttStats* rtt_stats,
    const QuicUnackedPacketMap* unacked_packets,
    CongestionControlType congestion_control_type,
    QuicRandom* random,
    QuicConnectionStats* stats,
    QuicPacketCount initial_congestion_window) {
  QuicPacketCount max_congestion_window = kDefaultMaxCongestionWindowPackets;
  // std::cout << "clock: " << clock << std::endl;
  // std::cout << "rtt_stats: " << rtt_stats << std::endl;
  // std::cout << "unacked_packets: " << unacked_packets << std::endl;
  // std::cout << "congestion_control_type: " << congestion_control_type << std::endl;
  // std::cout << "random: " << random << std::endl;
  // std::cout << "stats: " << stats << std::endl;
  // std::cout << "initial_congestion_window: " << initial_congestion_window << std::endl;

  switch (congestion_control_type) {
    case kBBR:
      return new BbrSender(rtt_stats, unacked_packets,
                           initial_congestion_window, max_congestion_window,
                           random);
    case kPCC:
      if (FLAGS_quic_reloadable_flag_quic_enable_pcc) {
        return CreatePccSender(clock, rtt_stats, unacked_packets, random, stats,
                               initial_congestion_window,
                               max_congestion_window);
      }
    // Fall back to CUBIC if PCC is disabled.
    case kCubic:
      if (!FLAGS_quic_reloadable_flag_quic_disable_packets_based_cc) {
        return new TcpCubicSenderPackets(
            clock, rtt_stats, false /* don't use Reno */,
            initial_congestion_window, max_congestion_window, stats);
      }
      QUIC_FLAG_COUNT_N(quic_reloadable_flag_quic_disable_packets_based_cc, 1,
                        2);
    case kCubicBytes:
      // std::cout << "congestion_control_type: " << congestion_control_type << std::endl;
      return new TcpCubicSenderBytes(
          clock, rtt_stats, false /* don't use Reno */,
          initial_congestion_window, max_congestion_window, stats);
    case kReno:
      if (!FLAGS_quic_reloadable_flag_quic_disable_packets_based_cc) {
        return new TcpCubicSenderPackets(clock, rtt_stats, true /* use Reno */,
                                         initial_congestion_window,
                                         max_congestion_window, stats);
      }
      QUIC_FLAG_COUNT_N(quic_reloadable_flag_quic_disable_packets_based_cc, 2,
                        2);
    case kRenoBytes:
      return new TcpCubicSenderBytes(clock, rtt_stats, true /* use Reno */,
                                     initial_congestion_window,
                                     max_congestion_window, stats);
  }
  return nullptr;
}

}  // namespace net
