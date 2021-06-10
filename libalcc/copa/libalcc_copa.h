/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> /* for close() for socket */
#include <stdlib.h>
#include <iostream>
#include <string>
#include "math.h"
#include <thread>
#include <pthread.h>
#include <cstdio>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <map>
#include <fstream>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>

// copa
#include "random.hh"
#include "estimators.hh"
#include "rtt-window.hh"
#include <chrono>
#include <functional>

using namespace std;

#define  MTU 1484
#define  EPOCH 5e3 // Verus epoch in microseconds

int alcc_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int alcc_send(int sockfd, const char * buffer, int buflen, int flags);
int alcc_close(int sockfd);

class Circular_Queue
{
    private:
        char *cqueue_arr;
        int front, rear;
        int MAX;
        pthread_mutex_t lockBuffer;

    public:
        Circular_Queue(int queue_length);
        int insert(char *item, int len, std::ofstream &log);
        int remove(char *chunk, int len);
        int length(ofstream &log);
};

class ALCCSocket
{
    private:
        int port;

        char command[512];
        unsigned long long sentSeq;
        unsigned long long rcvSeq;
        unsigned long long rcvSeqLast;
        unsigned long long tcpSeq;
        unsigned long long cwnd;

        struct timeval startTime;

        std::atomic<long long> tempS;
        std::atomic<long long> pktsInFlight;

        std::map <unsigned long long, struct timeval> seqNumbersList;
        std::map <unsigned long long, double> senderTimestampList;

        thread pkt_sender_thread, ack_receiver_thread, cc_loigc_thread;

        pthread_mutex_t lockList;

    public:
        bool terminateThreads;

        void write2Log (std::ofstream &logFile, std::string arg1, std::string arg2, std::string arg3, std::string arg4, std::string arg5);
        std::ofstream receiverLog;
        std::ofstream infoLog;

        Circular_Queue *sending_queue;

        ALCCSocket(long queue_size, int sockfd, int port);

        int pkt_sender (int sockfd);
        int ack_receiver (int sockfd);
        int CC_logic();

        // COPA
        double delta;
        int train_length;
        double _last_send_time;
        int _largest_ack;
        double tot_time_transmitted;
        double tot_delay;
        int tot_bytes_transmitted;
        int tot_packets_transmitted;

        double _the_window;
        double _intersend_time;
        double _timeout; // in ms

        typedef double Time;
        typedef int SeqNum;
  
        // Some adjustable parameters
        static constexpr double alpha_rtt = 1.0 / 1.0;
        // This factor is normalizes w.r.t expected Newreno window size for
        // TCP cooperation
        static constexpr double alpha_loss = 1.0 / 2.0;
        static constexpr double alpha_rtt_long_avg = 1.0 / 4.0;
        static constexpr double rtt_averaging_interval = 0.1;
        static constexpr int num_probe_pkts = 10;
        static constexpr double copa_k = 2;

        struct PacketData {
        Time sent_time;
        Time intersend_time;
        Time intersend_time_vel;
        Time rtt;
        double prev_avg_sending_rate;
        };

        std::map<SeqNum, PacketData> unacknowledged_packets;

        Time min_rtt;
        // If min rtt is supplied externally, preserve across flows.
        double external_min_rtt;
        PlainEwma rtt_unacked;
        RTTWindow rtt_window;
        Time prev_intersend_time;
        Time cur_intersend_time;
        Percentile interarrival;
        IsUniformDistr is_uniform;

        Time last_update_time;
        int update_dir;
        int prev_update_dir;
        int pkts_per_rtt;
        double update_amt;

        Time intersend_time_vel;
        Time prev_intersend_time_vel;
        double prev_rtt; // Measured RTT one-rtt ago.
        double prev_rtt_update_time;
        double prev_avg_sending_rate;

        #ifdef SIMULATION_MODE
          RNG rand_gen;
        #else
          RandGen rand_gen;
        #endif

        int num_pkts_lost;
        int num_pkts_acked;
        int num_pkts_sent;

        // Variables for expressing explicit utility
        enum {CONSTANT_DELTA, PFABRIC_FCT, DEL_FCT, BOUNDED_DELAY, BOUNDED_DELAY_END,
        MAX_THROUGHPUT, BOUNDED_QDELAY_END, BOUNDED_FDELAY_END, TCP_COOP,
            CONST_BEHAVIOR, AUTO_MODE} utility_mode;
        enum {DEFAULT_MODE, LOSS_SENSITIVE_MODE, TCP_MODE} operation_mode;
        bool do_slow_start;
        bool keep_ext_min_rtt;
        double default_delta;
        int flow_length;
        double delay_bound;
        double prev_delta_update_time;
        double prev_delta_update_time_loss;
        double max_queuing_delay_estimate;
        // To cooperate with TCP, measured in fraction of RTTs with loss
        LossRateEstimate loss_rate;
        ReduceOnLoss reduce_on_loss;
        bool loss_in_last_rtt;
        // Behavior constant
        double behavior;
        PlainEwma interarrival_ewma;
        double prev_ack_time;
        double exp_increment;
        PlainEwma prev_delta;
        bool slow_start;
        double slow_start_threshold;

        // Find flow id
        int flow_id_counter;
        int flow_id;

        chrono::high_resolution_clock::time_point start_time_point;

        Time cur_tick;

        double current_timestamp();

        double current_timestamp( chrono::high_resolution_clock::time_point &start_time_point ){
          using namespace chrono;
          high_resolution_clock::time_point cur_time_point = high_resolution_clock::now();
          // convert to milliseconds, because that is the scale on which the
          // rats have been trained
          return duration_cast<duration<double>>(cur_time_point - start_time_point).count()*1000;
        }

        double randomize_intersend(double);
        virtual void onLinkRateMeasurement( double measured_link_rate __attribute((unused)) ) {};
        
        void update_intersend_time();

        void update_delta(bool pkt_lost, double cur_rtt=0);

        virtual void init();
        virtual void onACK(int ack, double receiver_timestamp, 
             double sent_time, int delta_class=-1);
        
        double get_intersend_time(){ return _intersend_time; }
        virtual void onPktSent(int seq_num);
        void onTinyPktSent() {num_pkts_acked ++;}

        bool send_tiny_pkt() {return false;}//num_pkts_acked < num_probe_pkts-1;}
  
        //#ifdef SIMULATION_MODE
        void set_timestamp(double s_cur_tick) {cur_tick = s_cur_tick;}
        //#endif

        void set_flow_length(int s_flow_length) {flow_length = s_flow_length;}
        void set_min_rtt(double x) {
            if (external_min_rtt == 0)
              external_min_rtt = x;
            else
              external_min_rtt = std::min(external_min_rtt, x);
            init();
            std::cout << "Set min. RTT to " << external_min_rtt << std::endl;
            }
            int get_delta_class() const {return 0;}
            double get_the_window() {return _the_window;}
            void set_delta_from_router(double x) {
            if (utility_mode == BOUNDED_DELAY) {
              if (delta != x)
                std::cout << "Delta changed to: " << x << std::endl;
              delta = x;
            }
        }
};