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

using namespace std;

#define  MTU 1448
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

        thread pkt_sender_thread, ack_receiver_thread, cc_loigc_thread;

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
};