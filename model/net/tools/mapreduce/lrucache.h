#ifndef __LRU_H__
#define __LRU_H__
#include <iostream>
#include <unordered_map>
#include <vector>
#include <hash_map>
using namespace std;
namespace net {
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
        for(int i=0;i<size;++i)
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
            void Initialize(const string clusterStatsLogPath, const string ipToClusterLogPath);
            ~ConnectionStatsQuery();
            bool Query(string ip, vector<double> &stats);
        private:
            size_t size_ = 1000;
            size_t clusterNum_=100;
            LRUCache<string, vector<double>> *cluster_to_stats_;
            LRUCache <string,string> *ip_to_cluster_;
    };
}

#endif