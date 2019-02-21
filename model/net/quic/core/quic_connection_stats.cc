// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/core/quic_connection_stats.h"

namespace net {

QuicConnectionStats::QuicConnectionStats()
    : bytes_sent(0),
      packets_sent(0),
      stream_bytes_sent(0),
      packets_discarded(0),
      bytes_received(0),
      packets_received(0),
      packets_processed(0),
      stream_bytes_received(0),
      bytes_retransmitted(0),
      packets_retransmitted(0),
      bytes_spuriously_retransmitted(0),
      packets_spuriously_retransmitted(0),
      packets_lost(0),
      slowstart_packets_sent(0),
      slowstart_packets_lost(0),
      slowstart_bytes_lost(0),
      packets_dropped(0),
      crypto_retransmit_count(0),
      loss_timeout_count(0),
      tlp_count(0),
      rto_count(0),
      min_rtt_us(0),
      srtt_us(0),
      max_packet_size(0),
      max_received_packet_size(0),
      estimated_bandwidth(QuicBandwidth::Zero()),
      packets_reordered(0),
      max_sequence_reordering(0),
      max_time_reordering_us(0),
      tcp_loss_events(0),
      connection_creation_time(QuicTime::Zero()),
      blocked_frames_received(0),
      blocked_frames_sent(0) {}

QuicConnectionStats::QuicConnectionStats(const QuicConnectionStats& other) =
    default;

QuicConnectionStats::~QuicConnectionStats() {}

std::ostream& operator<<(std::ostream& os, const QuicConnectionStats& s) {
  os <<  s.bytes_sent <<",";
  os << s.packets_sent;
  os << s.stream_bytes_sent << ",";
  os << s.packets_discarded << ",";
  os << s.bytes_received << ",";
  os << s.packets_received << ",";
  os << s.packets_processed << ",";
  os << s.stream_bytes_received << ",";
  os << s.bytes_retransmitted << ",";
  os << s.packets_retransmitted << ",";
  os << s.bytes_spuriously_retransmitted << ",";
  
  os   << s.packets_spuriously_retransmitted << ",";
  os << s.packets_lost << ",";
  os << s.slowstart_packets_sent << ",";
  os << s.slowstart_packets_lost << ",";
  os << s.slowstart_bytes_lost << ",";
  os << s.packets_dropped << ",";
  os << s.crypto_retransmit_count << ",";
  os << s.loss_timeout_count << ",";
  os << s.tlp_count << ",";
  os << s.rto_count << ",";
  os << s.min_rtt_us << ",";
  os << s.srtt_us << ",";
  os << s.max_packet_size << ",";
  os << s.max_received_packet_size << ",";
  os << s.estimated_bandwidth << ",";
  os << s.packets_reordered << ",";
  os << s.max_sequence_reordering << ",";
  os << s.max_time_reordering_us << ",";
  os << s.tcp_loss_events << ",";
  
   os  << s.connection_creation_time.ToDebuggingValue() << ",";
  os << s.blocked_frames_received << ",";
  os << s.blocked_frames_sent ;

  return os;
}

}  // namespace net
