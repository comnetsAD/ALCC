#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include <stdlib.h>
#include "libalcc.h"
#include <time.h>
#include <cstdio>
#include <memory>
#include <mutex>
#include <linux/netlink.h>
#include <math.h>

#define MYPROTO NETLINK_USERSOCK
# define NETLINK_TEST 17

std::map < int, ALCCSocket * > alccSocketList;

Circular_Queue::Circular_Queue(int queue_length) {
	this->MAX = queue_length;
	this->cqueue_arr = new char[MAX];
	this->front = 0;
	this->rear = 0;
	pthread_mutex_init( & lockBuffer, NULL);
}

int Circular_Queue::insert(char * item, int len, ofstream & log) {
	if (front == rear)
		len = fmin(len, MAX);
	else if (front < rear)
		len = fmin(len, MAX - rear + front - 1);
	else
		len = fmin(len, front - rear - 1);

	if (len == 0) {
		return len;
	}

	pthread_mutex_lock( & lockBuffer);
	if ((front <= rear && MAX - rear - 1 >= len) || rear < front) {
		memcpy(cqueue_arr + rear, item, len);
		rear = (rear + len) % MAX;
	} else {
		memcpy(cqueue_arr + rear, item, MAX - rear);
		memcpy(cqueue_arr, item + MAX - rear, len - (MAX - rear));
		rear = len - (MAX - rear);
	}
	pthread_mutex_unlock( & lockBuffer);

	return len;
}

int Circular_Queue::remove(char * chunk, int len) {
	pthread_mutex_lock( & lockBuffer);
	if (front == rear) {
		pthread_mutex_unlock( & lockBuffer);
		return 0;
	} else if (front < rear)
		len = fmin(len, rear - front);
	else
		len = fmin(len, MAX - front + rear);

	if (front < rear || (rear < front && MAX - front >= len)) {
		memcpy(chunk, cqueue_arr + front, len);
		front = (front + len) % MAX;
	} else {
		memcpy(chunk, cqueue_arr + front, MAX - front);
		memcpy(chunk + MAX - front, cqueue_arr, len - (MAX - front));
		front = len - (MAX - front);
	}
	pthread_mutex_unlock( & lockBuffer);

	return len;
}

int Circular_Queue::length(ofstream & log) {
	int len;
	pthread_mutex_lock( & lockBuffer);

	if (front <= rear)
		len = rear - front;
	else
		len = MAX - front + rear;

	pthread_mutex_unlock( & lockBuffer);
	return len;
}

ALCCSocket::ALCCSocket(long queue_length, int new_sockfd, int port) {
	this->rcvSeq = 0;
	this->rcvSeqLast = 0;
	this->tcpSeq = 0;
	this->terminateThreads = false;
	this->sentSeq = 0;
	this->tempS = 1;
	this->pktsInFlight = 0;
	this->sending_queue = new Circular_Queue(queue_length);
	this->port = port;

	struct stat info;
	time_t now;
	time( & now);
	struct tm * now_tm;
	now_tm = localtime( & now);
	char timeString[80];
	strftime(timeString, 80, "%Y-%m-%d_%H:%M:%S", now_tm);

	if (stat(timeString, & info) != 0) {
		sprintf(command, "exec mkdir /tmp/%s", timeString);
		system(command);
	}

	sprintf(command, "/tmp/%s/info.out", timeString);
	infoLog.open(command, ios::out);
	sprintf(command, "/tmp/%s/Receiver.out", timeString);
	receiverLog.open(command, ios::out);

	gettimeofday( & startTime, NULL);

	ack_receiver_thread = thread( & ALCCSocket::ack_receiver, this, new_sockfd);
	pkt_sender_thread = thread( & ALCCSocket::pkt_sender, this, new_sockfd);
}

int ALCCSocket::pkt_sender(int sockfd) {
	int ret, z, size;
	int sPkts;
	char * data;
	struct timeval pktTime;

	data = (char * ) malloc(MTU);
	usleep(100000);
	// write2Log(infoLog, "ALCC sending thread started", "", "", "", "");

	while (!terminateThreads) {
		while (tempS > 0 && !terminateThreads) {
			sPkts = tempS;

			// recording the sending times for each packet sent
			gettimeofday( & pktTime, NULL);

			for (int i = 0; i < sPkts; i++) {
				size = sending_queue->remove(data, MTU);

				if (size == 0) // queue is empty and tempS is still big
					break;

				sentSeq++;

				z = 0;
				while (z < size) {
					ret = ::send(sockfd, data + z, size - z, 0);
					z += ret;
				}

				tempS -= 1;
				pktsInFlight += 1;

				write2Log(infoLog, "Sent pkt", std::to_string(sentSeq), std::to_string(size), std::to_string(pktsInFlight), std::to_string(cwnd));

				seqNumbersList[sentSeq] = pktTime;
			}
		}
	}
	return 0;
}

int ALCCSocket::ack_receiver(int sockfd) {
	struct timeval receivedtime;
	struct timeval sendTime;
	double delay;

	int sock;
	int numAcks;
	int group = NETLINK_TEST;

	unsigned src_port;
	unsigned dst_port;
	unsigned long rcv_seq;
	unsigned long ack_seq;

	char buffer[257];
	int ret;

	struct sockaddr_nl addr;
	struct msghdr msg;
	struct iovec iov;

	iov.iov_base = (void * ) buffer;
	iov.iov_len = sizeof(buffer);
	msg.msg_name = (void * ) & (addr);
	msg.msg_namelen = sizeof(addr);
	msg.msg_iov = & iov;
	msg.msg_iovlen = 1;

	sock = socket(AF_NETLINK, SOCK_RAW, MYPROTO);
	if (sock < 0) {
		printf("sock < 0.\n");
		return sock;
	}

	memset((void * ) & addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();

	if (bind(sock, (struct sockaddr * ) & addr, sizeof(addr)) < 0) {
		printf("bind < 0.\n");
		return -1;
	}

	if (setsockopt(sock, 270, NETLINK_ADD_MEMBERSHIP, & group, sizeof(group)) < 0) {
		printf("setsockopt < 0\n");
		return -1;
	}

	if (sock < 0)
		return sock;

	while (!terminateThreads) {
		ret = recvmsg(sock, & msg, 0);

		if (ret < 0) {
			printf("ret < 0.\n");
			write2Log(infoLog, "error ret < 0", "", "", "", "");
		} else {
			sscanf((char * ) NLMSG_DATA((struct nlmsghdr * ) & buffer), "%u %u %lu %lu", & src_port, & dst_port, & rcv_seq, & ack_seq);

			// getting the starting sequence number 
			if (tcpSeq == 0 && src_port == 60001) {
				tcpSeq = rcv_seq;
				write2Log(infoLog, "Starting Seq", std::to_string(tcpSeq), "", "", "");
				cc_loigc_thread = thread( & ALCCSocket::CC_logic, this);
				continue;
			} else if (src_port == 60001)
				continue;

			// write2Log(receiverLog, "KERN", std::to_string(rcv_seq), std::to_string(ack_seq), std::to_string(rcvSeq), std::to_string((ack_seq - tcpSeq) / MTU));

			int64_t diff = tcpSeq - ack_seq;

			gettimeofday( & receivedtime, NULL);

			if (tcpSeq > 0 && (ack_seq >= tcpSeq || diff > pow(2, 31))) {
				int num = (ack_seq - tcpSeq) / MTU;

				// checking for seq numbers wrap-around 
				if (num < 0) {
					rcvSeq += (pow(2, 32) - tcpSeq + ack_seq) / MTU;
				} else {
					rcvSeq += num;
				}
				tcpSeq = ack_seq;

				if (seqNumbersList.begin()->first <= rcvSeq && seqNumbersList.size() > 0) {

					if (seqNumbersList.find(rcvSeq) != seqNumbersList.end()) {
						sendTime = seqNumbersList.find(rcvSeq)->second;
						delay = (receivedtime.tv_sec - sendTime.tv_sec) * 1000.0 + (receivedtime.tv_usec - sendTime.tv_usec) / 1000.0;

						write2Log(receiverLog, std::to_string(rcvSeq), std::to_string(delay), "", "", "");

						numAcks = rcvSeq - rcvSeqLast;
						pktsInFlight -= numAcks;

						if (rcvSeq >= rcvSeqLast + 1) {
							rcvSeqLast = rcvSeq;
						}

						if (seqNumbersList.find(rcvSeq) != seqNumbersList.end()) {
							seqNumbersList.erase(rcvSeq);
						}
					}
				}
			}
		}
	}

	return 0;
}

int ALCCSocket::CC_logic() {
	// fixed cwnd
	cwnd = 20;

	while (!terminateThreads) {
		tempS += fmax(0, cwnd - pktsInFlight - tempS);

		// write2Log(infoLog, "pkts in flight", std::to_string(pktsInFlight), "cwnd", std::to_string(cwnd), "");

		usleep(EPOCH);
	}
	infoLog.close();
	receiverLog.close();

	return 0;
}

void ALCCSocket::write2Log(std::ofstream & logFile, std::string arg1, std::string arg2, std::string arg3, std::string arg4, std::string arg5) {
	double relativeTime;
	struct timeval currentTime;

	gettimeofday( & currentTime, NULL);
	relativeTime = (currentTime.tv_sec - startTime.tv_sec) + (currentTime.tv_usec - startTime.tv_usec) / 1000000.0;

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

	return;
}

int alcc_accept(int sockfd, struct sockaddr * addr, socklen_t * addrlen) {
	int new_sockfd = ::accept(sockfd, addr, addrlen);

	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);

	getpeername(new_sockfd, (struct sockaddr * ) & sin, & len);

	ALCCSocket * alcc = new ALCCSocket(10000000, new_sockfd, ntohs(sin.sin_port));
	alccSocketList[new_sockfd] = alcc;

	int sndbuf = 10000000;
	if (::setsockopt(new_sockfd, SOL_SOCKET, SO_SNDBUF, & sndbuf, sizeof(sndbuf)) < 0) {
		alcc->infoLog << "Couldnt set the TCP send buffer size" << endl;
		perror("socket error() set buf");
	}

	int rcvbuf = 10000000;
	if (::setsockopt(new_sockfd, SOL_SOCKET, SO_RCVBUF, & rcvbuf, sizeof(rcvbuf)) < 0) {
		alcc->infoLog << "Couldnt set the TCP send buffer size" << endl;
		perror("socket error() set buf");
	}

	int flag = 1;
	int result = ::setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY, (char * ) & flag, sizeof(int));
	if (result < 0)
		perror("socket error() set TCP_NODELAY");

	return new_sockfd;
}

int alcc_send(int sockfd,
	const char * buffer, int buflen, int flags) {
	ALCCSocket * alcc;

	if (alccSocketList.find(sockfd) != alccSocketList.end()) {
		alcc = alccSocketList.find(sockfd)->second;
	}

	return alcc->sending_queue->insert((char * ) buffer, buflen, alcc->infoLog);
}

int alcc_close(int sockfd) {
	ALCCSocket * alcc;

	if (alccSocketList.find(sockfd) != alccSocketList.end()) {
		alcc = alccSocketList.find(sockfd)->second;
	}

	while (alcc->sending_queue->length(alcc->infoLog) > 0)
		usleep(1);

	alcc->terminateThreads = true;
	alcc->write2Log(alcc->infoLog, "closing connection", "", "", "", "");
	delete alcc;
	close(sockfd);

	return 1;
}