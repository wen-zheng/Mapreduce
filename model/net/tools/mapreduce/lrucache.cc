#include "net/tools/mapreduce/lrucache.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace net{
    

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

    void ConnectionStatsQuery::Initialize(const string clusterStatsLogPath, const string clusterToIPLogPath){
        std::ifstream clusterStatsLog( clusterStatsLogPath, std::ios::in);
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
            for(int i=0; i<sv3.size(); i++){
                stats.push_back(atof(sv1[0].c_str()));
                stats.push_back(atof(sv2[0].c_str()));
                stats.push_back(atof(sv3[0].c_str()));
                
            }
            cluster_to_stats_->Put(clusterID,stats);
        }
        std::ifstream clusterToIPLog( clusterToIPLogPath, std::ios::in);

        clusterToIPLog.seekg(-2, clusterToIPLog.cur);
        int nCurentPos = 0;
        for(int i = 0; i < size_; i++){       
            while( clusterToIPLog.peek() != clusterToIPLog.widen('\n') ){
                nCurentPos = clusterToIPLog.tellg();
                if (nCurentPos == 0)
                    break;
                clusterToIPLog.seekg(-1, clusterToIPLog.cur );
            }
            if (nCurentPos == 0)
                break;
            clusterToIPLog.seekg(-1, clusterToIPLog.cur);
        }
        clusterToIPLog.seekg(2, clusterToIPLog.cur);
        vector<string> ip_clusterID;

        while(getline(clusterToIPLog, s)){
            split(s,ip_clusterID,',');
            ip_to_cluster_->Put(ip_clusterID[0], ip_clusterID[1]);
        }

    }

    bool ConnectionStatsQuery::Query(string ip, vector<double> &stats){
        string clusterID;
        if (!ip_to_cluster_->Get(ip, clusterID)){
            return false;
        }
        
        if(!cluster_to_stats_->Get(clusterID, stats)){
            return false;
        }
        return true;
    }
}