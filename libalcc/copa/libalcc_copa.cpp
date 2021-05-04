#include <stdio.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> 
#include <stdlib.h>
#include "libalcc_copa.h"
#include <time.h>
#include <cstdio>
#include <memory>
#include <mutex>
#include <linux/netlink.h>
#include <math.h>
#include <chrono>

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

ALCCSocket::ALCCSocket(long queue_length, int new_sockfd, int port)
	: delta(1.0),
	  unacknowledged_packets(),
      min_rtt(),
      external_min_rtt(false),
      rtt_unacked(alpha_rtt),
      rtt_window(),
      prev_intersend_time(),
      cur_intersend_time(),
      interarrival(0.05),
      is_uniform(32),
      last_update_time(),
      update_dir(),
      prev_update_dir(),
      pkts_per_rtt(),
      update_amt(),
      intersend_time_vel(),
      prev_intersend_time_vel(),
      prev_rtt(),
      prev_rtt_update_time(),
      prev_avg_sending_rate(),
      rand_gen(),
      num_pkts_lost(),
      num_pkts_acked(),
      num_pkts_sent(),
      utility_mode(CONSTANT_DELTA),
      operation_mode(DEFAULT_MODE),
      do_slow_start(false),
      keep_ext_min_rtt(false),
      default_delta(0.02), //(0.5),
      flow_length(),
      delay_bound(),
      prev_delta_update_time(),
      prev_delta_update_time_loss(),
      max_queuing_delay_estimate(),
      loss_rate(),
      reduce_on_loss(),
      loss_in_last_rtt(),
      behavior(),
      interarrival_ewma(1.0 / 32.0),
      prev_ack_time(),
      exp_increment(),
      prev_delta(1.0),
      slow_start(),
      slow_start_threshold(),
      flow_id(++ flow_id_counter),
      cur_tick(),
      train_length( 1 ),
      _last_send_time( 0.0 ),
      _largest_ack( -1 ),
      tot_time_transmitted( 0 ),
      tot_delay( 0 ),
      tot_bytes_transmitted( 0 ),
      tot_packets_transmitted( 0 )
	{
	this->rcvSeq = 0;
	this->rcvSeqLast = 0;
	this->tcpSeq = 0;
	this->terminateThreads = false;
	this->sentSeq = 0;
	this->tempS = 1;
	this->pktsInFlight = 0;
	this->sending_queue = new Circular_Queue(queue_length);
	this->port = port;

    pthread_mutex_init( & lockList, NULL);

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

	// COPA init taken from intercept_config_str
	do_slow_start = true;
	utility_mode = AUTO_MODE;
	default_delta = 0.02;

	flow_id_counter = 0;
	
	_intersend_time = 0;
	_the_window = num_probe_pkts;
	if (external_min_rtt != 0)
	_intersend_time = external_min_rtt / _the_window;
	if (keep_ext_min_rtt)
	min_rtt = external_min_rtt;
	else
	min_rtt = numeric_limits<double>::max();
	_timeout = 1000;

	if (utility_mode != CONSTANT_DELTA)
	delta = 1;

	unacknowledged_packets.clear();

	rtt_unacked.reset();
	rtt_window.clear();
	prev_intersend_time = 0;
	cur_intersend_time = 0;
	interarrival.reset();
	is_uniform.reset();

	last_update_time = 0;
	update_dir = 1;
	prev_update_dir = 1;
	pkts_per_rtt = 0;
	update_amt = 1;

	intersend_time_vel = 0;
	prev_intersend_time_vel = 0;
	prev_rtt = 0;
	prev_rtt_update_time = 0;
	prev_avg_sending_rate = 0;

	num_pkts_acked = num_pkts_lost = num_pkts_sent = 0;
	operation_mode = DEFAULT_MODE;
	flow_length = 0;
	prev_delta_update_time = 0;
	prev_delta_update_time_loss = 0;
	max_queuing_delay_estimate = 0;

	loss_rate.reset();
	reduce_on_loss.reset();
	loss_in_last_rtt = false;
	interarrival_ewma.reset();
	prev_ack_time = 0.0;
	exp_increment = 1.0;
	prev_delta.reset();
	slow_start = true;
	slow_start_threshold = 1e10;
}

void ALCCSocket::init() {
  if (num_pkts_acked != 0) {
    cout << "% Packets Lost: " << (100.0 * num_pkts_lost) /
      (num_pkts_acked + num_pkts_lost) << " at " << current_timestamp() << " " << num_pkts_acked << " " << num_pkts_sent << " min_rtt= " << min_rtt << " " << num_pkts_acked << " " << num_pkts_lost << endl;
    if (slow_start)
      cout << "Finished while in slow-start at window " << _the_window << endl;
  }

  _intersend_time = 0;
  _the_window = num_probe_pkts;
  if (external_min_rtt != 0)
    _intersend_time = external_min_rtt / _the_window;
  if (keep_ext_min_rtt)
    min_rtt = external_min_rtt;
  else
    min_rtt = numeric_limits<double>::max();
  _timeout = 1000;
  
  if (utility_mode != CONSTANT_DELTA)
    delta = 1;
  
  unacknowledged_packets.clear();

  rtt_unacked.reset();
  rtt_window.clear();
  prev_intersend_time = 0;
  cur_intersend_time = 0;
  interarrival.reset();
  is_uniform.reset();

  
  last_update_time = 0;
  update_dir = 1;
  prev_update_dir = 1;
  pkts_per_rtt = 0;
  update_amt = 1;
  
  intersend_time_vel = 0;
  prev_intersend_time_vel = 0;
  prev_rtt = 0;
  prev_rtt_update_time = 0;
  prev_avg_sending_rate = 0;

  num_pkts_acked = num_pkts_lost = num_pkts_sent = 0;
  operation_mode = DEFAULT_MODE;
  flow_length = 0;
  prev_delta_update_time = 0;
  prev_delta_update_time_loss = 0;
  max_queuing_delay_estimate = 0;

  loss_rate.reset();
  reduce_on_loss.reset();
  loss_in_last_rtt = false;
  interarrival_ewma.reset();
  prev_ack_time = 0.0;
  exp_increment = 1.0;
  prev_delta.reset();
  slow_start = true;
  slow_start_threshold = 1e10;
}

double ALCCSocket::current_timestamp( void ){
  return cur_tick;
}

void ALCCSocket::update_delta(bool pkt_lost __attribute((unused)), double cur_rtt) {
  double cur_time = current_timestamp();
  if (utility_mode == AUTO_MODE) {
    if (pkt_lost) {
      is_uniform.update(rtt_window.get_unjittered_rtt());
      //cout << "Packet lost: " << cur_time << endl;
    }
    if (!rtt_window.is_copa()) {
      operation_mode = LOSS_SENSITIVE_MODE;
    }
    else {
      operation_mode = DEFAULT_MODE;
    }
  }

  if (operation_mode == DEFAULT_MODE && utility_mode != TCP_COOP) {
    if (prev_delta_update_time == 0. || prev_delta_update_time_loss + cur_rtt < cur_time) {
      if (delta < default_delta)
        delta = default_delta; //1. / (1. / delta - 1.);
      delta = min(delta, default_delta);
      prev_delta_update_time = cur_time;
    }
  }
  else if (utility_mode == TCP_COOP || operation_mode == LOSS_SENSITIVE_MODE) {
    if (prev_delta_update_time == 0)
      delta = default_delta;
    if (pkt_lost && prev_delta_update_time_loss + cur_rtt < cur_time) {
      delta *= 2;
      // double decrease = 0.5 * _the_window * rtt_window.get_min_rtt() / (rtt_window.get_unjittered_rtt());
      // if (decrease >= 1. / (2. * delta))
      // 	delta = default_delta;
      // else
      // 	delta = 1. / (1. / (2. * delta) - decrease);
      prev_delta_update_time_loss = cur_time;
    }
    else {
      if (prev_delta_update_time + cur_rtt < cur_time) {
        delta = 1. / (1. / delta + 1.);
        //cout << "DU " << cur_time << " " << flow_id << " " << delta << endl;
        prev_delta_update_time = cur_time;
      }
    }
    delta = min(delta, default_delta);
  }
}

double ALCCSocket::randomize_intersend(double intersend) {
  //return rand_gen.exponential(intersend);
  //return rand_gen.uniform(0.99*intersend, 1.01*intersend);
  return intersend;
}


void ALCCSocket::update_intersend_time() {
  double cur_time __attribute((unused)) = current_timestamp();
  // if (external_min_rtt == 0) {
  //   cout << "External min. RTT estimate required." << endl;
  //   exit(1);
  // }
  
  // First two RTTs are for probing
  if (num_pkts_acked < 2 * num_probe_pkts - 1)
    return;

  // Calculate useful quantities
  double rtt = rtt_window.get_unjittered_rtt();
  double queuing_delay = rtt - min_rtt;

  double target_window;
  if (queuing_delay == 0)
    target_window = numeric_limits<double>::max();
  else
    target_window = rtt / (queuing_delay * delta);

  // Handle start-up behavior
  if (slow_start) {
    if (do_slow_start || target_window == numeric_limits<double>::max()) {
      _the_window += 1;
      if (_the_window >= target_window)
        slow_start = false;
    }
    //else {
    	// assert(false);
      // _the_window = rtt / ((min_rtt + rtt_window.get_max()) * 0.5 - min_rtt);
      // cout << "Fast Start: " << rtt_window.get_min() << " " << rtt_window.get_max() << " " << _the_window << endl;
      // slow_start = false;
    //}
  }
  // Update the window
  else {
    if (last_update_time + rtt_window.get_latest_rtt() < cur_time) {
      if (prev_update_dir * update_dir > 0) {
        if (update_amt < 0.006)
          update_amt += 0.005;
        else
          update_amt = (int)update_amt * 2;
      }
      else {
        update_amt = 1.;
        prev_update_dir *= -1;
      }
      last_update_time = cur_time;
      pkts_per_rtt = update_dir = 0;
    }
    if (update_amt > _the_window * delta) {
      update_amt /= 2;
    }
    update_amt = max(update_amt, 1.);
    ++ pkts_per_rtt;

    if (_the_window < target_window) {
      ++ update_dir;
      _the_window += update_amt / (delta * _the_window);
    }
    else {
      -- update_dir;
      _the_window -= update_amt / (delta * _the_window);
    }
  }

  //cout << "time= " << cur_time << " window= " << _the_window << " target= " << target_window << " rtt= " << rtt << " min_rtt= " << min_rtt << " delta= " << delta << " update_amt= " << update_amt << endl;
  // Set intersend time and perform boundary checks.
  _the_window = max(2.0, _the_window);
  cur_intersend_time = 0.5 * rtt / _the_window;
  _intersend_time = randomize_intersend(cur_intersend_time);
}

void ALCCSocket::onACK(int ack, 
			double receiver_timestamp __attribute((unused)),
			double sent_time, int delta_class __attribute((unused))) {
  // int seq_num = ack - 1;
  int seq_num = ack;
  double cur_time = current_timestamp();
  // assert(cur_time > sent_time);

  write2Log(receiverLog, std::to_string(cur_time - sent_time), "", "", "", "");

  rtt_window.new_rtt_sample(cur_time - sent_time, cur_time);
  min_rtt = rtt_window.get_min_rtt(); //min(min_rtt, cur_time - sent_time);
  // assert(rtt_window.get_unjittered_rtt() >= min_rtt);

  // write2Log(receiverLog, "min_rtt", std::to_string(cur_time - sent_time),std::to_string(cur_time),std::to_string(sent_time),"");

  // loss_rate = loss_rate * (1.0 - alpha_loss);

  if (prev_ack_time != 0) {
    interarrival_ewma.update(cur_time - prev_ack_time, cur_time / min_rtt);
    interarrival.push(cur_time - prev_ack_time);
  }
  prev_ack_time = cur_time;

  update_delta(false, cur_time - sent_time);
  update_intersend_time();

  bool pkt_lost = false;
  bool reduce = false;

  pthread_mutex_lock( & lockList);

  if (unacknowledged_packets.count(seq_num) != 0) {
    int tmp_seq_num = -1;
    for (auto x : unacknowledged_packets) {
    	// write2Log(infoLog, "3333", std::to_string(x.first), std::to_string(tmp_seq_num), "", "");
      // assert(tmp_seq_num <= x.first);
    	if (x.first < seq_num) {
    		unacknowledged_packets.erase(x.first);
    		break;
    	}

      tmp_seq_num = x.first;
      if (x.first > seq_num)
        break;
      prev_intersend_time = x.second.intersend_time;
      prev_intersend_time_vel = x.second.intersend_time_vel;
      prev_rtt = x.second.rtt;
      prev_rtt_update_time = x.second.sent_time;
      prev_avg_sending_rate = x.second.prev_avg_sending_rate;

      if (x.first < seq_num) {
        ++ num_pkts_lost;
        pkt_lost = true;
        reduce |= reduce_on_loss.update(true, cur_time, rtt_window.get_latest_rtt());
      }
      // write2Log(infoLog, "5555", "", "", "", "");
	  unacknowledged_packets.erase(x.first);
      // write2Log(infoLog, "6666" , std::to_string(unacknowledged_packets.size()), std::to_string(x.first), std::to_string(tmp_seq_num), "");
    }
  }

  pthread_mutex_unlock( & lockList);

  // write2Log(infoLog, "++++", "", "", "", "");

  if (pkt_lost) {
    update_delta(true);
    //cout << "LOST! --------------------" << endl;
  }
  reduce |= reduce_on_loss.update(false, cur_time, rtt_window.get_latest_rtt());
  if (reduce) {
    _the_window *= 0.7;
    _the_window = max(2.0, _the_window);
    cur_intersend_time = 0.5 * rtt_window.get_unjittered_rtt() / _the_window;
    _intersend_time = randomize_intersend(cur_intersend_time);
  }

  ++ num_pkts_acked;

  // write2Log(infoLog, "****", "", "", "", "");
}

void ALCCSocket::onPktSent(int seq_num) {
  double cur_time = current_timestamp();
  // double tmp_prev_avg_sending_rate = 0.0;
  // if (prev_intersend_time != 0.0)
  //   tmp_prev_avg_sending_rate = 1.0 / prev_intersend_time;
  pthread_mutex_lock( & lockList);

  // write2Log(receiverLog, "adding to unack Seq ", std::to_string(seq_num), "", "", "");

  unacknowledged_packets[seq_num] = {cur_time,
                                     cur_intersend_time,
                                     intersend_time_vel,
                                     rtt_window.get_unjittered_rtt(),
                                     unacknowledged_packets.size() / (cur_time - prev_rtt_update_time)
  };
  rtt_unacked.force_set(rtt_window.get_unjittered_rtt(), cur_time / min_rtt);

  for (auto & x : unacknowledged_packets) {
    if (cur_time - x.second.sent_time > rtt_unacked) {
      rtt_unacked.update(cur_time - x.second.sent_time, cur_time / min_rtt);
      prev_intersend_time = x.second.intersend_time;
      prev_intersend_time_vel = x.second.intersend_time_vel;
      continue;
    }
    break;
  }

  pthread_mutex_unlock( & lockList);

  //update_intersend_time();
  ++ num_pkts_sent;

  _intersend_time = randomize_intersend(cur_intersend_time);
}

int ALCCSocket::pkt_sender(int sockfd) {
	int ret, z, size;
	int sPkts;
	char * data;
	struct timeval pktTime;

	data = (char * ) malloc(MTU);

	// copa
	// for flow control
	int seq_num = 1;
	_largest_ack = -1;

	// for maintaining performance statistics
	double delay_sum = 0;
	int num_packets_transmitted = 0;
	int transmitted_bytes = 0;

  	start_time_point = chrono::high_resolution_clock::now();

	double cur_time = current_timestamp(start_time_point);
	set_min_rtt(20.0); // TODO fixme

	usleep(100000);
	// write2Log(infoLog, "ALCC sending thread started", "", "", "", "");

	_last_send_time = cur_time;
	
 	cur_time = current_timestamp( start_time_point );
 	set_timestamp(cur_time);
 	init();

	while (!terminateThreads) {
		// while (tempS > 0 && !terminateThreads) {
			// sPkts = tempS;

		while ((seq_num < _largest_ack + 1 + get_the_window()) &&
            (_last_send_time + get_intersend_time() * train_length <= cur_time)) {

			// write2Log(infoLog, "aaaa", "", "", "", "");

			cur_time = current_timestamp( start_time_point );

			// recording the sending times for each packet sent
			gettimeofday( & pktTime, NULL);

			size = sending_queue->remove(data, MTU);

			if (size == 0) // queue is empty and tempS is still big
				break;

			z = 0;
			while (z < size) {
				ret = ::send(sockfd, data + z, size - z, 0);
				z += ret;
			}

			// write2Log(infoLog, "Sent pkt", std::to_string(seq_num), std::to_string(cur_time), "", "");

			seqNumbersList[seq_num] = pktTime;
			senderTimestampList[seq_num] = cur_time;

			_last_send_time += get_intersend_time();

			if (seq_num % train_length == 0) {
				set_timestamp(cur_time);
				onPktSent( seq_num / train_length );
			}
			seq_num++;

			// write2Log(infoLog, "bbbb", "", "", "", "");
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
	int numAckPkts;

	// for estimating bottleneck link rate
	double link_rate_estimate = 0.0;
	double last_recv_time = 0.0;

	unsigned src_port;
	unsigned dst_port;
	unsigned long rcv_seq;
	unsigned long ack_seq;

	char buffer[257];
	int ret;

	struct sockaddr_nl addr;
	struct msghdr msg;
	struct iovec iov;

	double cur_time;
	double last_ack_time = cur_time;

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

		cur_time = current_timestamp( start_time_point );
		double timeout = _last_send_time + 1000; //get_timeout(); // everything in milliseconds
		if(get_the_window() > 0)
		  timeout = min( 1000.0, _last_send_time + get_intersend_time()*train_length - cur_time );


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
			// write2Log(infoLog, "1111", "", "", "", "");


			int64_t diff = tcpSeq - ack_seq;

			if (tcpSeq > 0 && (ack_seq >= tcpSeq || diff > pow(2, 31))) {

				// int num = (ack_seq - tcpSeq) / MTU;
				int numAcks = (ack_seq - tcpSeq) / MTU;

				// fix for seq numbers wraparound
				if (numAcks < 0) {
					numAcks = (pow(2, 32) - tcpSeq + ack_seq) / MTU;
				}
				// write2Log(receiverLog, "Received packets", std::to_string(numAcks), std::to_string(ack_seq), std::to_string(tcpSeq), std::to_string((ack_seq - tcpSeq)));

				for (numAckPkts=0; numAckPkts < numAcks; numAckPkts++) {
					rcvSeq += 1;
					tcpSeq += MTU;

					if (tcpSeq > pow(2,32)) {
						tcpSeq = tcpSeq - pow(2,32);
					}

					// write2Log(receiverLog, "Received seq", std::to_string(rcvSeq), "", "", "");

					gettimeofday( & receivedtime, NULL);
					cur_time = current_timestamp( start_time_point );
					last_ack_time = cur_time; //fixme

					// write2Log(infoLog, "2222", "", "", "", "");

					if (seqNumbersList.begin()->first <= rcvSeq && seqNumbersList.size() > 0) {
						if (seqNumbersList.find(rcvSeq) != seqNumbersList.end()) {
							sendTime = seqNumbersList.find(rcvSeq)->second;
							delay = (receivedtime.tv_sec - sendTime.tv_sec) * 1000.0 + (receivedtime.tv_usec - sendTime.tv_usec) / 1000.0;

							// write2Log(receiverLog, std::to_string(rcvSeq), std::to_string(delay), "", "", "");


							// COPA logic
							// Estimate link rate
						    if ((rcvSeq - 1) % train_length != 0 && last_recv_time != 0.0) {
						      double alpha = 1 / 16.0;
						      if (link_rate_estimate == 0.0)
						        link_rate_estimate = 1 * (cur_time - last_recv_time);
						      else
						        link_rate_estimate = (1 - alpha) * link_rate_estimate + alpha * (cur_time - last_recv_time);
						      // Use estimate only after enough datapoints are available
						      if (rcvSeq > 2 * train_length)
						        onLinkRateMeasurement(1e3 / link_rate_estimate );
						    }

						    // // Track performance statistics
						    // delay_sum += cur_time - ack_header.sender_timestamp;
						    // tot_delay += cur_time - ack_header.sender_timestamp;

						    // transmitted_bytes += data_size;
						    // tot_bytes_transmitted += data_size;

						    // num_packets_transmitted += 1;
						    // tot_packets_transmitted += 1;

						   	write2Log(infoLog, "-- Rcvd pkt", std::to_string(rcvSeq), std::to_string(senderTimestampList.find(rcvSeq)->second), "", "");

						    if ((rcvSeq - 1) % train_length == 0) {
						      set_timestamp(cur_time);

						      onACK(rcvSeq / train_length,
						                     NULL,
						                     senderTimestampList.find(rcvSeq)->second);
						    }
						    _largest_ack = fmax(_largest_ack, rcvSeq);


							if (rcvSeq >= rcvSeqLast + 1) {
								rcvSeqLast = rcvSeq;
							}

							// if (seqNumbersList.find(rcvSeq) != seqNumbersList.end()) {
							// 	seqNumbersList.erase(rcvSeq);
							// }
						}
					}
				}
				last_recv_time = cur_time;
				// write2Log(infoLog, "3333", "", "", "", "");
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