// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/quic_simple_dispatcher.h"

#include "net/tools/quic/quic_simple_server_session.h"
#include "net/quic/core/congestion_control/tcp_cubic_sender_bytes.h"
// #include "net/tools/mapreduce/lrucache.h"
using namespace std;
namespace net {
      void split(const string& s, vector<string>& sv,const char flag = ' ') {
        sv.clear();
        istringstream iss(s);
        string temp;

        while (getline(iss, temp, flag)) {
            sv.push_back(temp);
        }
        return; 
    }

    ConnectionStatsQuery::ConnectionStatsQuery(){
        cluster_to_stats_ = new LRUCache<string, vector<double>>(clusterNum_);
        ip_to_cluster_ = new LRUCache<string,string>(size_);
    }

    ConnectionStatsQuery::ConnectionStatsQuery(size_t size){
        size_ = size;
        cluster_to_stats_ = new LRUCache<string, vector<double>>(clusterNum_);
        ip_to_cluster_ = new LRUCache<string,string>(size);
    }

    ConnectionStatsQuery::ConnectionStatsQuery(size_t size, size_t clusterNum){

        size_ = size;
        clusterNum_ = clusterNum;
        cluster_to_stats_ = new LRUCache<string, vector<double>>(clusterNum);
        ip_to_cluster_ = new LRUCache<string,string>(size);
    }

    ConnectionStatsQuery::~ConnectionStatsQuery(){
        cluster_to_stats_->~LRUCache();
        ip_to_cluster_->~LRUCache();
    }

    void ConnectionStatsQuery::Initialize(const string clusterStatsLogPath, 
                  const string clusterToIPLogPath){
        std::ifstream clusterStatsLog( clusterStatsLogPath, std::ios::in);

        if(!clusterStatsLog.is_open()){
          cout << "Error opening file: " << clusterStatsLogPath <<std::endl;
          exit (1);
        }
        string s;
        while(getline(clusterStatsLog,s)){   
            vector<string> sv;
            split(s,sv,';');
            vector<string> sv1;
            vector<string> sv2;
            vector<string> sv3;
            split(sv[0],sv1,',');
            split(sv[1],sv2,',');
            split(sv[2],sv3,',');

            string clusterID = sv1.front();
            sv1.erase(sv1.begin());
            vector<double> stats;
            for(unsigned int i=0; i<sv3.size(); i++){
                stats.push_back(atof(sv1[i].c_str()));
                stats.push_back(atof(sv2[i].c_str()));
                stats.push_back(atof(sv3[i].c_str()));
                
            }
            cluster_to_stats_->Put(clusterID,stats);
        }

        clusterStatsLog.close();

        std::ifstream clusterToIPLog( clusterToIPLogPath, std::ios::ate);
        if(!clusterToIPLog.is_open()){
          cout << "Error opening file: " << clusterToIPLogPath <<std::endl;
          exit (1);
        }
        clusterToIPLog.seekg(-2, clusterToIPLog.cur);
        int nCurentPos = -2;
        for(unsigned int i = 0; i < size_; i++){       
            while( clusterToIPLog.peek() != clusterToIPLog.widen('\n') ){
                nCurentPos = clusterToIPLog.tellg();
                if (nCurentPos == clusterToIPLog.beg)
                    break;
                clusterToIPLog.seekg(-1, clusterToIPLog.cur );
            }
            if (nCurentPos == clusterToIPLog.beg)
                break;
            clusterToIPLog.seekg(-1, clusterToIPLog.cur);
        }
        if (nCurentPos == clusterToIPLog.beg){
          clusterToIPLog.clear();
          clusterToIPLog.seekg(0,clusterToIPLog.beg);
        }
        else{
            clusterToIPLog.seekg(2, clusterToIPLog.cur);
        }
        vector<string> ip_clusterID;

        while(getline(clusterToIPLog, s)){
            split(s,ip_clusterID,',');
            ip_to_cluster_->Put(ip_clusterID[0], ip_clusterID[1]);
            // std::cout << ip_clusterID[0] << " " << ip_clusterID[1] <<  std::endl;
        }

        clusterToIPLog.close();

    }

    bool ConnectionStatsQuery::Query(string ip, vector<double> &stats){
        string clusterID;
        if (!ip_to_cluster_->Get(ip, clusterID)){
            return false;
        }
        // std::cout << clusterID << std::endl;
        if(!cluster_to_stats_->Get(clusterID, stats)){            
            return false;
        }
        // for (long unsigned int i=0;i<stats.size();i++){
        //       std::cout << stats[i]<< " ";
        //     }
        //       cout << endl;
        return true;
    }

QuicSimpleDispatcher::QuicSimpleDispatcher(
    const QuicConfig& config,
    const QuicCryptoServerConfig* crypto_config,
    QuicVersionManager* version_manager,
    std::unique_ptr<QuicConnectionHelperInterface> helper,
    std::unique_ptr<QuicCryptoServerStream::Helper> session_helper,
    std::unique_ptr<QuicAlarmFactory> alarm_factory,
    QuicHttpResponseCache* response_cache)
    : QuicDispatcher(config,
                     crypto_config,
                     version_manager,
                     std::move(helper),
                     std::move(session_helper),
                     std::move(alarm_factory)),
      response_cache_(response_cache) {
        connection_stats_query_ = new ConnectionStatsQuery();
        
        connection_stats_query_->Initialize("./cluster-min-max-mean","./ip-cluster");
      }

QuicSimpleDispatcher::~QuicSimpleDispatcher() {}

int QuicSimpleDispatcher::GetRstErrorCount(
    QuicRstStreamErrorCode error_code) const {
  auto it = rst_error_map_.find(error_code);
  if (it == rst_error_map_.end()) {
    return 0;
  } else {
    return it->second;
  }
}

void QuicSimpleDispatcher::OnRstStreamReceived(
    const QuicRstStreamFrame& frame) {
  auto it = rst_error_map_.find(frame.error_code);
  if (it == rst_error_map_.end()) {
    rst_error_map_.insert(std::make_pair(frame.error_code, 1));
  } else {
    it->second++;
  }
}

QuicServerSessionBase* QuicSimpleDispatcher::CreateQuicSession(
    QuicConnectionId connection_id,
    const QuicSocketAddress& client_address,
    QuicStringPiece /*alpn*/) {
  
  // The QuicServerSessionBase takes ownership of |connection| below.
  QuicConnection* connection = new QuicConnection(
      connection_id, client_address, helper(), alarm_factory(),
      CreatePerConnectionWriter(),
      /* owns_writer= */ true, Perspective::IS_SERVER, GetSupportedVersions());
  bool enableQuery = true;
  if(enableQuery){
    string clientIp = client_address.host().ToString();
    vector<double> stats;
    // std::cout << clientIp << std::endl;
    if(connection_stats_query_->Query(clientIp,stats)){
      TcpCubicSenderBytes * sendAlgorithm = dynamic_cast<TcpCubicSenderBytes *> (
        connection->sent_packet_manager_not_const().GetSendAlgorithmNotConst());
      RttStats * rttstats = connection->sent_packet_manager_not_const().GetRttStatsNotConst();

      // sendAlgorithm->SetInitialCongestionWindowInBytesPublic(140*kDefaultTCPMSS);
      sendAlgorithm->SetInitialCongestionWindowFromBandwidthAndRttPublic(
       QuicBandwidth::FromBitsPerSecond(1000*stats[ESTIMATED_BANDWIDTH_MAX]),
         QuicTime::Delta::FromMicroseconds(stats[MIN_RTT_US_MAX]));
      sendAlgorithm->SetCongestionWindowFromBandwidthAndRttPublic(
       QuicBandwidth::FromBitsPerSecond(1000*stats[ESTIMATED_BANDWIDTH_MAX]),
         QuicTime::Delta::FromMicroseconds(stats[MIN_RTT_US_MEAN]));
      // sendAlgorithm->SetSlowstartthresholdFromBandwidthAndRttPublic(
      //  QuicBandwidth::FromBitsPerSecond(1000*stats[ESTIMATED_BANDWIDTH_MAX]),
      //    QuicTime::Delta::FromMicroseconds(stats[MIN_RTT_US_MAX]));
      sendAlgorithm->SetMinSlowstartExitWindowFromBandwidthAndRttPublic(
        QuicBandwidth::FromBitsPerSecond(1000*stats[ESTIMATED_BANDWIDTH_MEAN]),
          QuicTime::Delta::FromMicroseconds(stats[MIN_RTT_US_MEAN]));

      rttstats->set_initial_rtt_us(stats[MIN_RTT_US_MEAN]);
      rttstats->set_initial_srtt_us(stats[SRTT_US_MEAN]);
    }
  }
  QuicServerSessionBase* session = new QuicSimpleServerSession(
      config(), connection, this, session_helper(), crypto_config(),
      compressed_certs_cache(), response_cache_);
  session->Initialize(); 
  return session;
}

}  // namespace net
