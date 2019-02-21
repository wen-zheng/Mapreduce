// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_QUIC_SIMPLE_DISPATCHER_H_
#define NET_TOOLS_QUIC_QUIC_SIMPLE_DISPATCHER_H_

#include "net/quic/core/quic_server_session_base.h"
#include "net/tools/quic/quic_dispatcher.h"
#include "net/tools/quic/quic_http_response_cache.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
namespace net {
    enum connectionStatsIndex{ 
        BYTES_SENT_MIN,
        BYTES_SENT_MAX,
        BYTES_SENT_MEAN,
        PACKETS_SENT_MIN,
        PACKETS_SENT_MAX,
        PACKETS_SENT_MEAN,
        STREAM_BYTES_SENT_MIN,
        STREAM_BYTES_SENT_MAX,
        STREAM_BYTES_SENT_MEAN,
        PACKETS_DISCARDED_MIN,
        PACKETS_DISCARDED_MAX,
        PACKETS_DISCARDED_MEAN,
        BYTES_RECEIVED_MIN,
        BYTES_RECEIVED_MAX,
        BYTES_RECEIVED_MEAN,
        PACKETS_RECEIVED_MIN,
        PACKETS_RECEIVED_MAX,
        PACKETS_RECEIVED_MEAN,
        PACKETS_PROCESSED_MIN,
        PACKETS_PROCESSED_MAX,
        PACKETS_PROCESSED_MEAN,
        STREAM_BYTES_RECEIVED_MIN,
        STREAM_BYTES_RECEIVED_MAX,
        STREAM_BYTES_RECEIVED_MEAN,
        BYTES_RETRANSMITTED_MIN,
        BYTES_RETRANSMITTED_MAX,
        BYTES_RETRANSMITTED_MEAN,
        PACKETS_RETRANSMITTED_MIN,
        PACKETS_RETRANSMITTED_MAX,
        PACKETS_RETRANSMITTED_MEAN,
        BYTES_SPURIOUSLY_RETRANSMITTED_MIN,
        BYTES_SPURIOUSLY_RETRANSMITTED_MAX,
        BYTES_SPURIOUSLY_RETRANSMITTED_MEAN,
        PACKETS_SPURIOUSLY_RETRANSMITTED_MIN,
        PACKETS_SPURIOUSLY_RETRANSMITTED_MAX,
        PACKETS_SPURIOUSLY_RETRANSMITTED_MEAN,
        PACKETS_LOST_MIN,
        PACKETS_LOST_MAX,
        PACKETS_LOST_MEAN,
        SLOWSTART_PACKETS_SENT_MIN,
        SLOWSTART_PACKETS_SENT_MAX,
        SLOWSTART_PACKETS_SENT_MEAN,
        SLOWSTART_PACKETS_LOST_MIN,
        SLOWSTART_PACKETS_LOST_MAX,
        SLOWSTART_PACKETS_LOST_MEAN,
        SLOWSTART_BYTES_LOST_MIN,
        SLOWSTART_BYTES_LOST_MAX,
        SLOWSTART_BYTES_LOST_MEAN,
        PACKETS_DROPPED_MIN,
        PACKETS_DROPPED_MAX,
        PACKETS_DROPPED_MEAN,
        CRYPTO_RETRANSMIT_COUNT_MIN,
        CRYPTO_RETRANSMIT_COUNT_MAX,
        CRYPTO_RETRANSMIT_COUNT_MEAN,
        LOSS_TIMEOUT_COUNT_MIN,
        LOSS_TIMEOUT_COUNT_MAX,
        LOSS_TIMEOUT_COUNT_MEAN,
        TLP_COUNT_MIN,
        TLP_COUNT_MAX,
        TLP_COUNT_MEAN,
        RTO_COUNT_MIN,
        RTO_COUNT_MAX,
        RTO_COUNT_MEAN,
        MIN_RTT_US_MIN,
        MIN_RTT_US_MAX,
        MIN_RTT_US_MEAN,
        SRTT_US_MIN,
        SRTT_US_MAX,
        SRTT_US_MEAN,
        MAX_PACKET_SIZE_MIN,
        MAX_PACKET_SIZE_MAX,
        MAX_PACKET_SIZE_MEAN,
        MAX_RECEIVED_PACKET_SIZE_MIN,
        MAX_RECEIVED_PACKET_SIZE_MAX,
        MAX_RECEIVED_PACKET_SIZE_MEAN,
        ESTIMATED_BANDWIDTH_MIN,
        ESTIMATED_BANDWIDTH_MAX,
        ESTIMATED_BANDWIDTH_MEAN,
        PACKETS_REORDERED_MIN,
        PACKETS_REORDERED_MAX,
        PACKETS_REORDERED_MEAN,
        MAX_SEQUENCE_REORDERING_MIN,
        MAX_SEQUENCE_REORDERING_MAX,
        MAX_SEQUENCE_REORDERING_MEAN,
        MAX_TIME_REORDERING_US_MIN,
        MAX_TIME_REORDERING_US_MAX,
        MAX_TIME_REORDERING_US_MEAN,
        TCP_LOSS_EVENTS_MIN,
        TCP_LOSS_EVENTS_MAX,
        TCP_LOSS_EVENTS_MEAN,
        CONNECTION_CREATION_TIME_MIN,
        CONNECTION_CREATION_TIME_MAX,
        CONNECTION_CREATION_TIME_MEAN,
        BLOCKED_FRAMES_RECEIVED_MIN,
        BLOCKED_FRAMES_RECEIVED_MAX,
        BLOCKED_FRAMES_RECEIVED_MEAN,
        BLOCKED_FRAMES_SENT_MIN,
        BLOCKED_FRAMES_SENT_MAX,
        BLOCKED_FRAMES_SENT_MEAN,
    };
    
    template <typename K,typename T>
    struct Node{
        K key;
        T data;
        Node*prev,*next;
    };

    template <typename K,typename T>
    class LRUCache{
    public:
        LRUCache(size_t size);
        ~LRUCache();
        void Put(K key,T data);
        bool Get(K key, T &value);
        int getCount(){return count;}
    private:
        void attach(Node<K,T> *node);
        void detach(Node<K,T> *node);

        int count;//cache miss count
        unordered_map<K,Node<K,T>* >hashmap_;
        vector< Node<K,T>* >free_entries_;
        Node<K,T>*head_,*tail_;
        Node<K,T>*entries_;
    };

    template <typename K,typename T>
    LRUCache<K,T>::LRUCache(size_t size)
    {
        count=0;
        entries_ = new Node<K,T>[size];
        for(unsigned int i=0;i<size;++i)
            free_entries_.push_back(entries_+i);
        head_ = new Node<K,T>;
        tail_ = new Node<K,T>;
        head_->prev = NULL;
        head_->next = tail_;
        tail_->prev = head_;
        tail_->next = NULL;
    }
    template <typename K,typename T>
    LRUCache<K,T>::~LRUCache()
    {
        delete head_;
        delete tail_;
        delete[] entries_;
    }

    template <typename K,typename T>
    void LRUCache<K,T>::Put(K key,T data)
    {
        Node<K,T>*node = hashmap_[key];
        if(NULL!=node)
        {
            detach(node);
            node->key=key;
            node->data=data;
            attach(node);
        }
        else
        {
            if(free_entries_.empty())
            {
                node=tail_->prev;
                detach(node);
                hashmap_.erase(node->key);
            }
            else
            {
                node = free_entries_.back();
                free_entries_.pop_back();
            }
            node->key=key;
            node->data=data;
            hashmap_[key]=node;
            attach(node);
        }
    }
    template <typename K,typename T>
    bool LRUCache<K,T>::Get(K key, T &value)
    {
        Node<K,T> *node = hashmap_[key];
        if(NULL!=node)
        {
            detach(node);
            attach(node);
            value=node->data;
            return true;
        }
        else
        {
            ++count;
            // Put(key,T());
            // return nullptr;
            // return T();
            return false;
        }
    }

    template <typename K,typename T>
    void LRUCache<K,T>::attach(Node<K,T> *node)
    {
        node->next=head_->next;
        head_->next=node;
        node->next->prev=node;
        node->prev=head_;
    }

    template <typename K,typename T>
    void LRUCache<K,T>::detach(Node<K,T> *node)
    {
        node->prev->next=node->next;
        node->next->prev=node->prev;
    }

    class ConnectionStatsQuery{
        public:
            ConnectionStatsQuery();
            ConnectionStatsQuery(size_t size);
            ConnectionStatsQuery(size_t size, size_t clusterNum);
            void Initialize(const string clusterStatsLogPath, 
                    const string ipToClusterLogPath);
            ~ConnectionStatsQuery();
            bool Query(string ip, vector<double> &stats);
        private:
            size_t size_ = 2000;
            size_t clusterNum_=100;
            LRUCache<string, vector<double>> *cluster_to_stats_;
            LRUCache <string,string> *ip_to_cluster_;
    };

class QuicSimpleDispatcher : public QuicDispatcher {
 public:
  QuicSimpleDispatcher(
      const QuicConfig& config,
      const QuicCryptoServerConfig* crypto_config,
      QuicVersionManager* version_manager,
      std::unique_ptr<QuicConnectionHelperInterface> helper,
      std::unique_ptr<QuicCryptoServerStream::Helper> session_helper,
      std::unique_ptr<QuicAlarmFactory> alarm_factory,
      QuicHttpResponseCache* response_cache);

  ~QuicSimpleDispatcher() override;

  int GetRstErrorCount(QuicRstStreamErrorCode rst_error_code) const;

  void OnRstStreamReceived(const QuicRstStreamFrame& frame) override;



 protected:
  QuicServerSessionBase* CreateQuicSession(
      QuicConnectionId connection_id,
      const QuicSocketAddress& client_address,
      QuicStringPiece alpn) override;

  QuicHttpResponseCache* response_cache() { return response_cache_; }

 private:
  bool enable_query_ = false;
  QuicHttpResponseCache* response_cache_;  // Unowned.
  ConnectionStatsQuery* connection_stats_query_;
  // The map of the reset error code with its counter.
  std::map<QuicRstStreamErrorCode, int> rst_error_map_;
};

}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_SIMPLE_DISPATCHER_H_
