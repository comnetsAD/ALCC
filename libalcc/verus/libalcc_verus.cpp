/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> /* for close() for socket */
#include <stdlib.h>
#include "libalcc_verus.h"
#include <time.h>
#include <cstdio>
#include <memory>
#include <mutex>

#include <linux/netlink.h>
// #include <linux/tcp.h>
#include <math.h>

#include "../../Applications/bftpd/logging.h"

#include <sys/stat.h>
#include <sys/types.h>

/* Protocol family, consistent in both kernel prog and user prog. */
#define MYPROTO NETLINK_USERSOCK
/* Multicast group, consistent in both kernel prog and user prog. */
#define MYMGRP 17

std::map <int, ALCCSocket*> alccSocketList;

circular_buffer::circular_buffer(size_t size) :
    buf_(new char[size]),
    size_(size),
    filled(0)
{
    //empty constructor
}

void circular_buffer::put(char item)
{
    std::lock_guard<std::mutex> lock(mutex_);

    buf_[head_] = item;
    head_ = (head_ + 1) % size_;

    if(head_ == tail_)
    {
        tail_ = (tail_ + 1) % size_;
    }
    filled = filled+1;
}

int circular_buffer::insert(char *item, int len, std::ofstream &log)
{
    //return the number of items inserted in the buffer
    for(int i=0; i<len; i++)
    {
        if(!full())
            put(item[i]);
        else
            return i;
    }

    return len;
}

int circular_buffer::insert(char *item, int len)
{
    //return the number of items inserted in the buffer
    for(int i=0; i<len; i++)
    {
        if(!full())
            put(item[i]);
        else
            return i;
    }

    return len;
}

int circular_buffer::remove(char*chunk , int len)
{
    int i = 0;
    for(i=0; i<len; i++)
    {
        if(!empty())
            chunk[i]=get();
    }
    if(i!=len)
        printf("There is some error here");

    chunk[len] = '\0';
    return len;
}

char circular_buffer::get(void)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if(empty())
    {
        return '\0';
    }

    //Read data and advance the tail (we now have a free space)
    char val = buf_[tail_];
    tail_ = (tail_ + 1) % size_;
    filled = filled-1;
    return val;
}

void circular_buffer::reset(void)
{
    std::lock_guard<std::mutex> lock(mutex_);
    head_ = tail_;
}

bool circular_buffer::empty(void)
{
    //if head and tail are equal, we are empty
    return head_ == tail_;
}

bool circular_buffer::full(void)
{
    //If tail is ahead the head by 1, we are full
    return ((head_ + 1) % size_) == tail_;
}

size_t circular_buffer::size(void)
{
    return size_ - 1;
}

int circular_buffer::length (ofstream &log)
{
    return filled;
}



Circular_Queue::Circular_Queue(int queue_length)
    {
        this->MAX = queue_length;
        this->cqueue_arr = new char [MAX];
        this->front = 0;
        this->rear = 0;
        pthread_mutex_init(&lockBuffer, NULL);
    }

int Circular_Queue::insert(char *item, int len, ofstream &log)
    {
        if (front == rear)
            len = fmin(len,MAX);
        else if (front < rear)
            len = fmin(len,MAX-rear+front-1);
        else
            len = fmin(len, front-rear-1);

        if (len == 0){
            return len;
        }

        pthread_mutex_lock(&lockBuffer);
        //cout << "inserting with length " << len << endl;

        if ((front <= rear && MAX-rear-1 >= len) || rear < front) {
            memcpy (cqueue_arr+rear, item, len);
            rear=(rear+len)%MAX;
        }
        else {
            memcpy (cqueue_arr+rear, item, MAX-rear);
            memcpy (cqueue_arr, item+MAX-rear, len-(MAX-rear));
            rear = len-(MAX-rear);
        }

        //log << "front and rear " << front << " " << rear << " " << len << endl;
        //log.flush();
        //cout << "2 front and rear " << front << " " << rear << endl;
        pthread_mutex_unlock(&lockBuffer);

        return len;
    }

int Circular_Queue::remove (char *chunk, int len)
    {
        pthread_mutex_lock(&lockBuffer);
        if (front == rear) {
            pthread_mutex_unlock(&lockBuffer);
            return 0;
        }
        else if (front < rear)
            len = fmin (len, rear-front);
        else
            len = fmin (len, MAX-front+rear);

        //cout << "creating a chunk with size " << len << " front and rear " << front << " " << rear << endl;

        if (front < rear || (rear < front && MAX-front >= len)) {
            memcpy (chunk, cqueue_arr+front, len);
            // chunk[len]='\0';
            front = (front+len)%MAX;
        }
        else {
            memcpy (chunk, cqueue_arr+front, MAX-front);
            memcpy (chunk+MAX-front, cqueue_arr, len-(MAX-front));
            // chunk[len]='\0';
            front = len-(MAX-front);
        }
        //cout << " front and rear " << front << " " << rear << endl;
        pthread_mutex_unlock(&lockBuffer);

        return len;
    }

int Circular_Queue::length (ofstream &log)
    {
        int len;
        pthread_mutex_lock(&lockBuffer);

        if (front <= rear)
            len = rear-front;
        else
            len = MAX-front+rear;

        pthread_mutex_unlock(&lockBuffer);
        //log << "Queue size " << len << " " << front << " " << rear << endl;
        //log.flush();
        return len;
    }

ALCCSocket::ALCCSocket (long queue_length, int new_sockfd, int port)
{
    this->maxWCurve = 0;
    this->seq = 0;
    this->startingSeq = 0;
    //this->lastAckedSeq = 0;
    //this->startSeq = 0;
    this->seqLast = 0;
    //this->leftoverBytes = 0;
    this->terminateThreads = false;
    this->slowStart = true;
    this->haveSpline = false;
    this->wBar = 1;
    this->dMax = 0.0;
    this->dMaxLast = -10;
    this->dEst = 0.0;
    this->dMin = 1000.0;
    this->dTBar = 0.0;
    this->pktSeq = 0;
    //this->totalPktsSent = 0;
    this->tempS = 1;
    this->wCrt = 0;
    //yasir
    this->sending_queue = new Circular_Queue(queue_length);
    //this->sending_queue = new circular_buffer(queue_length);
    this->port = port;

    struct stat info;
    time_t now;
    time(&now);
    struct tm* now_tm;
    now_tm = localtime(&now);
    char timeString[80];
    strftime (timeString, 80, "%Y-%m-%d_%H:%M:%S", now_tm);

	if (stat(timeString, & info) != 0) {
        sprintf(command, "2021-alcc-verus");
        // int i = system(command);

    if (mkdir(command, 0777) == -1)
        bftpd_log ("Error %s: %d \n", command, errno);
    else
        bftpd_log ("Directory created");

    bftpd_log ("qqqq \n");
    }
	cout << timeString << endl;

   sprintf (command, "2021-alcc-verus/Verus.out", timeString);
   verusLog.open(command);
   sprintf (command, "2021-alcc-verus/Losses.out", timeString);
   lossLog.open(command,ios::out);
   sprintf (command, "2021-alcc-verus/Receiver.out", timeString);
   receiverLog.open(command,ios::out);

    gettimeofday(&startTime,NULL);

    curve_thread=thread(&ALCCSocket::ALCCCurve, this);
    verus_thread=thread(&ALCCSocket::ALCCVerus, this);
    receive_thread=thread(&ALCCSocket::ALCCReceive, this, new_sockfd);
    send_thread=thread(&ALCCSocket::ALCCSend, this, new_sockfd);

}

void ALCCSocket::write2Log (std::ofstream &logFile, std::string arg1, std::string arg2, std::string arg3, std::string arg4, std::string arg5) {
    double relativeTime;
    struct timeval currentTime;

    gettimeofday(&currentTime,NULL);
    relativeTime = (currentTime.tv_sec-startTime.tv_sec)+(currentTime.tv_usec-startTime.tv_usec)/1000000.0;

    logFile << relativeTime << "," << arg1;

    if (arg2 != "")
      logFile << "," << arg2;
    if (arg3 != "")
      logFile << "," << arg3;
    if (arg4 != "")
      logFile << "," << arg4;
    if (arg5 != "")
      logFile << "," << arg5;

    logFile << "\n";
    logFile.flush();

    //cout << arg1 << "," << arg2 << "," << arg3 << "," << arg4 << "," << arg5 << endl;

    return;
}

int ALCCSocket::calcSi (double wBar) {
    int n;

    n = (int) ceil(dTBar/(EPOCH/1000.0));

    if (n > 1)
//        return (int) fmax (0, (wBar+((int)(wCrt/MTU))*(2-n)/(n-1)));
        return (int) fmax (0, (wBar+((int)(wCrt))*(2-n)/(n-1)));
    else
//        return (int) fmax (0, (wBar - ((int)(wCrt/MTU)) - tempS));
        return (int) fmax (0, (wBar - ((int)(wCrt)) - tempS));
}

double ALCCSocket::ewma (double vals, double delay, double alpha) {
    // checking if the value is negative, meanning it has not been udpated
    if (vals < 0)
        return delay;
    else
        return vals * alpha + (1-alpha) * delay;
}

void ALCCSocket::restartSlowStart() {

    pthread_mutex_lock(&restartLock);

    maxWCurve = 0;
    dEst =0.0;
    wBar =1;
    dTBar = 0.0;
    wCrt = 0;
    dMin = 1000.0;

    //lastAckedSeq = (lastAckedSeq + (pktSeq - seqLast)*MTU) % ((int64_t) pow(2,32));
    //lastAckedSeq = (startSeq + totalPktsSent*MTU) % ((int64_t) pow(2,32));

    seq = 0;
    seqLast = 0;
    startingSeq = 0;
    pktSeq =0;
    dMax = 0.0;
    dMaxLast = -10;
    slowStart = true;
    haveSpline = false;

    //write2Log (lossLog, "Starting seq number", std::to_string(lastAckedSeq),"","","");

    // cleaning up
    pthread_mutex_lock(&lockSendingList);
    sendingList.clear();
    pthread_mutex_unlock(&lockSendingList);

    seqNumbersList.clear();
    delaysEpochList.clear();

    for (int i=0; i<MAX_W_DELAY_CURVE; i++)
        wList[i]=-1;

    // sending the first packet for slow start
    tempS = 1;

    pthread_mutex_unlock(&restartLock);
}

double ALCCSocket::calcDelayCurve (double delay) {
    int w;

    if (!haveSpline) {
        for (w=2; w < MAX_W_DELAY_CURVE; w++) {
            pthread_mutex_lock(&lockWList);
            if (wList[w] > delay) {
                pthread_mutex_unlock(&lockWList);
                return (w-1);
            }
            pthread_mutex_unlock(&lockWList);
        }
    }
    else {
        for (w=2; w < MAX_W_DELAY_CURVE; w++) {
            pthread_mutex_lock(&lockSPline);
            try {
                if (spline1dcalc(spline, w) > delay) {
                    pthread_mutex_unlock(&lockSPline);
                    return (w-1);
                }
            }
            catch (alglib::ap_error exc) {
                std::cout << "alglib1 " << exc.msg.c_str() << "\n";
                write2Log (lossLog, "CalcDelayCurve error", exc.msg.c_str(), "", "", "");
            }
            pthread_mutex_unlock(&lockSPline);
        }
    }

    if (!haveSpline)
        return calcDelayCurve(delay-DELTA2); // special case: when verus starts working and we don't have a curve.
    else
        return -1000.0;
}

double ALCCSocket::calcDelayCurveInv (double w) {
    double ret;

    if (!haveSpline) {
        if (w < MAX_W_DELAY_CURVE) {
            pthread_mutex_lock(&lockWList);
            ret = wList[(int)w];

            while (ret < 0 && w > 0) {
                w -= 1;
                ret = wList[(int)w];
            }
            pthread_mutex_unlock(&lockWList);
            return fmax(ret,dMin);
        }
        else
            return fmax(wList[MAX_W_DELAY_CURVE],dMin);
    }

    pthread_mutex_lock(&lockSPline);
    try{
        ret = spline1dcalc(spline, w);
    }
    catch (alglib::ap_error exc) {
        std::cout << "alglib2 " << exc.msg.c_str() << "\n";
        write2Log (lossLog, "Alglib error", exc.msg.c_str(), std::to_string(w), "", "");
    }
    pthread_mutex_unlock(&lockSPline);
    return fmax(ret,dMin);
}

void ALCCSocket::updateUponReceivingPacket (double delay, int w, int numAcks) {

    if (wCrt > 0)
       wCrt -= numAcks;

    // processing the delay and updating the verus parameters and the delay curve points
    delaysEpochList.push_back(delay);
    dTBar = ewma (dTBar, delay, 0.875);

    // updating the minimum delay
    if (delay < dMin)
        dMin = delay;

    pthread_mutex_lock(&lockWList);
    wList[w] = ewma (wList[w], delay, 0.875);
    pthread_mutex_unlock(&lockWList);

    if (maxWCurve < w)
        maxWCurve = w;

    return;
}

int ALCCSocket::ALCCSend(int sockfd)
 {
    int ret, z, size;
    int sPkts;
    char *data;
    verus_packet* pdu;

    struct timeval pktTime;
    struct sockaddr_in adr_clnt;

    // create mutex
    pthread_mutex_init(&lockSendingList, NULL);
    pthread_mutex_init(&lockSPline, NULL);
    pthread_mutex_init(&lockWList, NULL);
    pthread_mutex_init(&restartLock, NULL);

    cout << "SENDING thread  " << endl;
    write2Log (lossLog, "ALCC sending thread started", "", "", "", "");

    data = (char *) malloc (MTU);
    // data = (char *) malloc (MTU);

    usleep(100000);

    while (!terminateThreads) {
        pthread_mutex_lock(&restartLock);
        // write2Log (lossLog, "Sender", "" , "" , "" , "");

        while (tempS > 0 && !terminateThreads) {
            sPkts = tempS;

            // recording the sending times for each packet sent
            gettimeofday(&pktTime,NULL);

            for (int i=0; i<sPkts; i++) {
                // size = sending_queue->remove(data, MTU-sizeof(verus_header));

                size = sending_queue->remove(data, MTU);

                if (size == 0) // queue is empty and tempS is still big
                    break;

                pktSeq ++;

                // pdu = (verus_packet*)malloc(size+sizeof(verus_header));
                // pdu->header.payloadlength = 0;//size;
                // pdu->header.seq = 0; //pktSeq;
                // memcpy(&pdu->data, data, size);

                // z = 0;
                // while (z < size+sizeof(verus_header)) {
                //     ret = ::send(sockfd, pdu+z, size+sizeof(verus_header)-z, 0);
                //     z += ret;
                // }

                // pdu = (verus_packet*)malloc(size);
                // memcpy(&pdu->data, data, size);

                z = 0;
                while (z < size) {
                    // ret = ::send(sockfd, pdu+z, size+sizeof(verus_header)-z, 0);
                    ret = ::send(sockfd, data+z, size-z, 0);
                    z += ret;
                }

                tempS -= 1;
                wCrt += 1;
                write2Log (lossLog, "Sent pkt", std::to_string(pktSeq), "" , "" , "");

                seqNumbersList[pktSeq]=pktTime;
                
                // storing sending packet info in sending list with sending time
                pthread_mutex_lock(&lockSendingList);
                sendingList[pktSeq]=wBar;
                pthread_mutex_unlock(&lockSendingList);

                // free(pdu);
            }

            if (tempS > 0 && !slowStart && size > 0)
                write2Log (lossLog, "Epoch Error", "couldn't send everything within the epoch. Have more to send", std::to_string(tempS.load()), std::to_string(slowStart), "");
        }
        pthread_mutex_unlock(&restartLock);
    }
    return NULL;
}

int ALCCSocket::ALCCReceive(int sockfd)
 {
    unsigned int i;
    // socklen_t len_inet;
    // char buf[sizeof(verus_header)];
    struct timeval receivedtime;
    struct timeval sendTime;
    double delay;
    int w;

    // verus_header *ack;
    // len_inet = sizeof(struct sockaddr_in);
    // ack = (verus_header *) malloc(sizeof(verus_header));
    // int ret;


    int sock;
    int numAcks;
    int group = MYMGRP;
    struct sockaddr_nl addr;
    struct msghdr msg;
    struct iovec iov;
    char buffer[65536];
    int ret;
    int numAckPkts;
    
    __u16  src_port;
    __u16  dst_port;
    __u32  rcv_seq;
    __u32  ack_seq;

    iov.iov_base = (void *) buffer;
    iov.iov_len = sizeof(buffer);
    msg.msg_name = (void *) &(addr);
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
    if (sock < 0) {
        printf("sock < 0.\n");
        return sock;
    }

    memset((void *) &addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("bind < 0.\n");
        return -1;
    }

    if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        printf("setsockopt < 0\n");
        return -1;
    }

    if (sock < 0)
        return sock;

    while (!terminateThreads) {
        ret = recvmsg(sock, &msg, 0);

        if (ret < 0) {
            printf("ret < 0.\n");
            write2Log (lossLog, "error ret < 0", "", "", "", "");
        }
        else {
            sscanf((char *) NLMSG_DATA((struct nlmsghdr *) &buffer), "%u %u %lu %lu", &src_port, &dst_port, &rcv_seq, &ack_seq) ;
            
            // getting the starting sequence number 
            if (startingSeq == 0 && src_port == 60001) {
                startingSeq = rcv_seq;
                // printf("Starting seq number: %lu \n", startingSeq);
                write2Log (lossLog, "Starting Seq", std::to_string(startingSeq), "", "", "");
                continue;
            }
            else if (src_port == 60001)
                continue;


            write2Log (receiverLog, "KERNEL MSG", std::to_string(rcv_seq), std::to_string(ack_seq), std::to_string(src_port), std::to_string(dst_port));

            // if (dst_port != 60001 || ack_seq == 0)
                // continue;

            int64_t diff = startingSeq-ack_seq;
            
            // write2Log (receiverLog, "Received:", std::to_string(ack_seq), std::to_string(startingSeq), std::to_string(ack_seq-startingSeq),"");

            gettimeofday(&receivedtime,NULL);

            write2Log (receiverLog, "starting seq", std::to_string(startingSeq), std::to_string(ack_seq), "", "");

            if (startingSeq > 0 && (ack_seq >= startingSeq || diff > pow(2,31))) {
        
                //int num = (ack_seq-startingSeq)/MTU;

                // write2Log (receiverLog, "num", std::to_string(num),"","","");
                int tmp_numAcks = (ack_seq - startingSeq) / MTU;

                write2Log (receiverLog, "tmp_numAcks", std::to_string(tmp_numAcks), "", "", "");

                // fix for seq numbers wraparound
                if (tmp_numAcks < 0) {
                    tmp_numAcks = (pow(2, 32) - startingSeq + ack_seq) / MTU;
                }

                // // checking for seq numbers wrap-around 
                // if (num < 0){
                //     seq += (pow(2,32) - startingSeq + ack_seq)/MTU;
                //     // printf("Received (%d) messages: new seq is (%d)", (int)(pow(2,32) - startingSeq + ack_seq)/MTU, seq);
                // }
                // else{
                //     seq += num;
                //     // printf("Received (%d) messages: new seq is (%d) \n", num, seq);
                // }
                // startingSeq = ack_seq;


                // write2Log (receiverLog, "SEQ", std::to_string(seq), "", "", "");
                for (numAckPkts=0; numAckPkts < tmp_numAcks; numAckPkts++) {
                    seq += 1;
                    startingSeq += MTU;

                    write2Log (receiverLog, "for loop", std::to_string(seq), "", "", "");

                    if (seqNumbersList.begin()->first <= seq && seqNumbersList.size() > 0) {
                        //sendTime = seqNumbersList.begin()->second;
                        sendTime = seqNumbersList.find(seq)->second;

                        delay = (receivedtime.tv_sec-sendTime.tv_sec)*1000.0+(receivedtime.tv_usec-sendTime.tv_usec)/1000.0;

                        //fixme: get sending w of ack seq
                        if (sendingList.find(seq) != sendingList.end() ) {
                            w = sendingList.find(seq)->second;
                        } 
                        else {
                            write2Log (lossLog, "Sending List error", "couldnt find the seq number", "", "", "");
                            // displayError("Sending List error, couldnt find the seq number");
                        }

                        if (slowStart && (delay > SS_EXIT_THRESHOLD || seq > 5000)) { // if the current delay exceeds half a second during slow start we should time out and exit slow start
                            wBar = VERUS_M_DECREASE * w;
                            dEst = calcDelayCurveInv(wBar); //0.75 * dMin * VERUS_R; // setting dEst to half of the allowed maximum delay, for effeciency purposes
                            // lossPhase = true;
                            slowStart = false;
                            tempS = 0;

                            // exitSlowStart = true;

                            write2Log (lossLog, "Exit slow start", "exceeding SS_EXIT_THRESHOLD", std::to_string(w), "", "");
                        }

                        numAcks = 1; //seq-seqLast;
                        updateUponReceivingPacket (delay, w, numAcks);
                        write2Log (receiverLog, std::to_string(seq), std::to_string(delay), std::to_string(wCrt), std::to_string(wBar), std::to_string(numAcks));


                        if (seq >= seqLast+1) {
                            seqLast = seq;
                            gettimeofday(&lastAckTime,NULL);
                        }

                        pthread_mutex_lock(&lockSendingList);
                        if (sendingList.find(seq) != sendingList.end()) {
                            sendingList.erase(seq);
                        }
                        pthread_mutex_unlock(&lockSendingList);

                        if (seqNumbersList.find(seq) != seqNumbersList.end()) {
                            seqNumbersList.erase(seq);
                        }

                        // ------------------ verus slow start ------------------
                        if(slowStart) {
                            // since we received an ACK we can increase the sending window by 1 packet according to slow start
                            wBar ++;
                            tempS += fmax(0, wBar-wCrt-tempS);   // let the sending thread start sending the new packets
                        }
                        pthread_mutex_unlock(&restartLock);
                    }
                    else {
                        wCrt -= 1;
                    }
                } // end of foor loop
                write2Log (receiverLog, "END OF FOOR LOOP", "", "", "", "");
            }
            write2Log (lossLog, "END receiver loop", "", "", "", "");
        }
    }

    return NULL;
 }

int ALCCSocket::ALCCCurve ()
{
    std::vector<int> x;
    std::vector<double> y;

    int max_i;
    double N = 15.0;
    int cnt;
    double* xs = NULL;
    double* ys = NULL;
    real_1d_array xi;
    real_1d_array yi;
    ae_int_t info;
    spline1dfitreport rep;
    spline1dinterpolant splineTemp;

    cout << "Curve thread  " << endl;
    write2Log (lossLog, "ALCC curve thread started", "", "", "", "");

    while (!terminateThreads) {
        if (slowStart){
            xs = NULL;
            ys = NULL;

            // still in slowstart phase we should not calculate the delay curve
            usleep (1);
        }
        else{
            // in normal mode we should compute the delay curve and sleep for the delay curve refresh timer
            // clearing the vectors
            x.clear();
            y.clear();

            max_i = 1;
            cnt = 0;
            maxWCurve = MAX_W_DELAY_CURVE; //fixme: maxWCurve

            pthread_mutex_lock(&lockWList);
            for (int i=1; i<maxWCurve; i++) {
                if (wList[i] >=0) {
                    if (i < MAX_W_DELAY_CURVE) {
                        x.push_back(i);
                        y.push_back(wList[i]);

                        cnt++;
                        xs=(double*)realloc(xs,cnt*sizeof(double));
                        ys=(double*)realloc(ys,cnt*sizeof(double));

                        xs[cnt-1] = (double) i;
                        ys[cnt-1] = (double) wList[i];
                        max_i = i;
                    }
                    else{
                        wList[i] = -1;
                    }
                }
            }
            pthread_mutex_unlock(&lockWList);
            maxWCurve = 0;

            xi.setcontent(cnt, xs);
            yi.setcontent(cnt, ys);

            while (max_i/N < 5) {
                N--;
                if (N < 1) {
                    write2Log (lossLog, "Restart", "Alglib M<4!", std::to_string(N), "", "");
                    restartSlowStart();
                    //return NULL;
                }
            }
            if (max_i/N > 4 && N > 0) {
                try {
                    // if alglib takes long time to compute, we can use the previous spline in other threads
                    spline1dfitpenalized(xi, yi, max_i/N, 2.0, info, splineTemp, rep);

                    // copy newly calculated splinetemp to update spline
                    pthread_mutex_lock(&lockSPline);
                    spline=splineTemp;
                    pthread_mutex_unlock(&lockSPline);

                    haveSpline = true;
                }
                catch (alglib::ap_error exc) {
                    write2Log (lossLog, "Restart", "Spline exception", exc.msg.c_str(), "", "");
                    restartSlowStart();
                }
            }
            usleep (CURVE_TIMER);
        }
    }
    return NULL;
}

int ALCCSocket::ALCCVerus()
 {
    int timeoutCnt = 0;
    double bufferingDelay;
    double wBarTemp;
    bool dMinStop=false;
    struct timeval currentTime;

    for (int j=0; j<MAX_W_DELAY_CURVE; j++)
        wList[j]=-1;

    cout << "VERUS thread  " << endl;
    while (!terminateThreads) {
        // waiting for slow start to finish to start sending data
        if (!slowStart) {
            dMinStop=false;

            // -------------- Verus sender ------------------
            if (delaysEpochList.size() > 0) {
                dMax = *std::max_element(delaysEpochList.begin(),delaysEpochList.end());
                delaysEpochList.clear();
                //timeoutCnt = 0;
            }
            else {
                gettimeofday(&currentTime,NULL);
                bufferingDelay = (currentTime.tv_sec-lastAckTime.tv_sec)*1000.0+(currentTime.tv_usec-lastAckTime.tv_usec)/1000.0;
                dMax = fmax (dMaxLast, bufferingDelay);
                //timeoutCnt ++;
            }

            // if (timeoutCnt >= 100) { // 500ms of timeout not receiving any acks
            //     write2Log (lossLog, "Timeout", "havent received any acks for 500ms", "", "", "");
            //     restartSlowStart();
            // }

            // only first verus epoch, dMaxLast is intialized to -10
            if (dMaxLast == -10)
                dMaxLast = dMax;

            // normal verus protocol
            if (dMaxLast/dMin > VERUS_R) {
                dEst = fmax (dMin, (dEst-DELTA2));

//                if (dEst == dMin && (int)(wCrt/MTU) < 2) {
                if (dEst == dMin && (int)(wCrt) < 2) {
                    dMin += 10;
                }
                else if (dEst == dMin)
                    dMinStop=true;
            }
            else if (dMax - dMaxLast > 0.0001)
                dEst = fmax (dMin, (dEst-DELTA1));
            else
                dEst += DELTA2;

            wBarTemp = calcDelayCurve (dEst);

            if (wBarTemp < 0) {
                write2Log (lossLog, "Restart", "can't map delay on delay curve", std::to_string(dEst), std::to_string(wBarTemp), "");
                restartSlowStart();
                continue;
            }

            wBar = fmax (1.0, wBarTemp);
            dMaxLast = ewma(dMaxLast, dMax, 0.2);

            if (!dMinStop)
                tempS += calcSi (wBar);

            write2Log (verusLog, std::to_string(dEst), std::to_string(dMin), std::to_string(wCrt), std::to_string(wBar), std::to_string(tempS));
        }
        usleep (EPOCH);
    }
    verusLog.close();
    lossLog.close();
    receiverLog.close();

    return NULL;
 }


int alcc_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
  {
    int n;
    unsigned int m = sizeof(n);

    int new_sockfd = ::accept(sockfd, addr,addrlen);

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    // if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
    //     perror("getsockname");

    getpeername(new_sockfd,(struct sockaddr *)&sin, &len);
    cout << "port " << sin.sin_port << endl;


    ALCCSocket *alcc = new ALCCSocket (10000000, new_sockfd, ntohs(sin.sin_port));
    alccSocketList[new_sockfd] = alcc;

    int sndbuf = 10000000;
    if (::setsockopt(new_sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0){
        alcc->lossLog << "Couldnt set the TCP send buffer size" << endl;
        alcc->lossLog.flush();
        perror("socket error() set buf");
    }

    int rcvbuf = 10000000;
    if (::setsockopt(new_sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) < 0){
        alcc->lossLog << "Couldnt set the TCP send buffer size" << endl;
        alcc->lossLog.flush();
        perror("socket error() set buf");
    }

    int flag = 1;
    int result = ::setsockopt(new_sockfd,IPPROTO_TCP,TCP_NODELAY,(char *) &flag, sizeof(int));
    if (result < 0)
        perror("socket error() set TCP_NODELAY");

    return new_sockfd;
 }


int alcc_send(int sockfd, const char * buffer, int buflen, int flags)
 {
    ALCCSocket *alcc;

    if (alccSocketList.find(sockfd) != alccSocketList.end() ) {
            alcc = alccSocketList.find(sockfd)->second;
        }

    return alcc->sending_queue->insert((char*)buffer, buflen, alcc->lossLog);
 }

int alcc_close(int sockfd)
{
    int z, ret;
    ALCCSocket *alcc;
    // verus_header* pdu;

    if (alccSocketList.find(sockfd) != alccSocketList.end() ) {
            alcc = alccSocketList.find(sockfd)->second;
        }

    while (alcc->sending_queue->length(alcc->lossLog) > 0)
        usleep(1);

    // sending a final packet to the client indicating the end of the transfer
    // pdu = (verus_header*)malloc(sizeof(verus_header));
    // pdu->payloadlength = 0;
    // pdu->seq = 0;

    // z = 0;
    // while (z < sizeof(verus_header)) {
        // ret = ::send(sockfd, pdu+z, sizeof(verus_header)-z, 0);
        // z += ret;
    // }

    alcc->terminateThreads = true;
    // alcc->lossLog << "closing connection" << endl;
    alcc->write2Log (alcc->lossLog, "closing connection", "", "", "", "");
    alcc->lossLog.flush();
    // delete alcc;
    close(sockfd);

    return 1;
}
