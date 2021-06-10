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
#include "verus.hpp"
#include "math.h"
#include <thread>
#include <pthread.h>
#include <cstdio>
#include <memory>
#include <mutex>


// #ifndef ALCC_H
// #define ALCC_H

using namespace std;

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


class circular_buffer {
public:
    circular_buffer(size_t size);
    void put(char item);
    int insert(char *item, int len, std::ofstream &log);
    int insert(char *item, int len);
    int remove(char*chunk , int len);
    char get(void);
    void reset(void);
    bool empty(void);
    bool full(void);
    size_t size(void);
    int length(ofstream &log);

private:
    std::mutex mutex_;
    char * buf_;
    size_t head_ = 0;
    size_t tail_ = 0;
    size_t size_;
    int filled;
};

class ALCCSocket
{
    private:
        int port;
        int maxWCurve;
        //int seq;
        //int leftoverBytes;
        //int64_t startSeq;
        //int64_t lastAckedSeq;
        unsigned long long seqLast;

        char command[512];

        bool slowStart;
        bool haveSpline;

        double wBar;
        double dMax;
        double dMaxLast;
        double dEst;
        double dMin;
        double dTBar;
        double wList[MAX_W_DELAY_CURVE];

        long long seq;
        long long pktSeq;
        long long startingSeq;
        //int64_t totalPktsSent;

        struct timeval startTime;
        struct timeval lastAckTime;

        std::atomic<long long> tempS;
        std::atomic<long long> wCrt;

        std::map <int, long long> sendingList;
        std::map <long long, struct timeval> seqNumbersList;

        std::vector<double> delaysEpochList;

        spline1dinterpolant spline;

        thread send_thread, receive_thread, verus_thread, curve_thread;

        pthread_mutex_t lockSendingList;
        pthread_mutex_t lockSPline;
        pthread_mutex_t lockWList;
        pthread_mutex_t restartLock;


        int calcSi (double wBar);
        double calcDelayCurve (double delay);
        double calcDelayCurveInv (double w);
        void updateUponReceivingPacket (double delay, int w, int numAcks);
        double ewma (double vals, double delay, double alpha);
        void restartSlowStart();

    public:
        bool terminateThreads;

        void write2Log (std::ofstream &logFile, std::string arg1, std::string arg2, std::string arg3, std::string arg4, std::string arg5);
        std::ofstream receiverLog;
        std::ofstream lossLog;
        std::ofstream verusLog;

	// yasir
        Circular_Queue *sending_queue;
        //circular_buffer *sending_queue;

        ALCCSocket(long queue_size, int sockfd, int port);
        // int socket(int family, int type, int protocol);
        // //int setsockopt(SOCKET socket, int level, int option_name, int * option_value, int optionlen);
        // int setsockopt(int socket_descriptor, int level, int option_name, char *option_value, int option_length);
        // //int accept(SOCKET socket, struct sockaddr * addr, int * addrlen);
        // int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
        // int bind(int sockfd, const struct sockaddr * addr, int addrlen);
        // int connect(int sockfd, const struct sockaddr * addr, int addrlen);
        // int listen(int sockfd, int backlogsize);
        // int send(int sockfd, const char * buffer, int buflen, int flags);
        // int sendto(int sockfd, const char * buffer,int buflen, int flags, const struct sockaddr * to, int tolen);
        // int recv(int sockfd, char * buffer, int buflen, int flags);
        // int recvfrom(int sockfd, char * buffer, int buflen, int flags, struct sockaddr * from, int * fromlen);
        int ALCCSend(int sockfd);
        int ALCCReceive(int sockfd);
        int ALCCVerus();
        int ALCCCurve();
};

// #endif ALCC_H
