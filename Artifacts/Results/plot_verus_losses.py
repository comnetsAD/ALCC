#-----------------------------Plot Run Commands------------------------------------#

import os
import numpy as np
import matplotlib.pyplot as plt
import sys
from glob import glob
import pandas as pd
import seaborn as sns
import math
import matplotlib.gridspec as gridspec
from matplotlib.patches import Ellipse

plt.rcParams['text.latex.preamble']=[r'\boldmath']
params = {
		  'font.size' : 40,
		  'legend.fontsize': 30,
		  'text.latex.unicode': True,
		  }
plt.rcParams.update(params)

plt.rcParams['ytick.labelsize'] = 40
plt.rcParams['xtick.labelsize'] = 40
plt.rcParams["font.weight"] = "bold"
plt.rcParams["axes.labelweight"] = "bold"

def scale(a):
	return a/1000000.0

def parse_throughput(filename):
	times = []
	pktsize = []

	throughput_file = open(filename,"r")

	if '/verus/' in filename:
		tokens = throughput_file.readline().strip().split(",")
	else:
		tokens = throughput_file.readline().strip().split()

	sTime = float(tokens[0])
	firstTime = sTime
	bucket = []

	if '/verus/' in filename:
		bucket.append(1500)
	else:
		bucket.append(float(tokens[1]))

	for line in throughput_file:
		if '/verus/' in filename:
			tokens = line.strip().split(",")
		else:
			tokens = line.strip().split()

		if float(tokens[0])< sTime+1.0:
			if '/verus/' in filename:
				bucket.append(1500)
			else:
				bucket.append(float(tokens[1]))
		else:
			pktsize.append(sum(bucket)*8/1000000.0)
			bucket = []
			times.append(sTime-firstTime)

			while float(tokens[0])-sTime > 1.0:
				sTime += 1.0

		if sTime - firstTime > float(sys.argv[2]):
			break

	throughput_file.close()
	return pktsize, times

def parse_delay_alccVerus(filename):
	delays = []
	times = []
	cnt = 0

	delay_file = open(filename,"r")
	tokens = delay_file.readline().strip().split()
	sTime = float(tokens[0])
	delays.append((float(tokens[1])*1000.0))
	times.append((float(tokens[0])))

	for line in delay_file:
		tokens = line.strip().split()

		# if float(tokens[1]) < 10000.0:
		delays.append((float(tokens[1])*1000))
		times.append((float(tokens[0])-sTime))

	delay_file.close()

	return  delays, times

def parse_delay(filename):
	delays = []
	times = []
	cnt = 0

	delay_file = open(filename,"r")
	tokens = delay_file.readline().strip().split(",")
	sTime = float(tokens[0])

	for line in delay_file:
		tokens = line.strip().split(",")

		if '/verus/' in filename:
			pass
		elif 'KERNEL' in line or len(tokens) != 6 or float(tokens[2]) < 20:
			continue

		if (float(tokens[0])-sTime) < float(sys.argv[2]):
			delays.append((float(tokens[2])))
			times.append((float(tokens[0])-sTime))

	delay_file.close()

	return  delays, times

def simple_cdf(data):
	data_sorted = np.sort(data)

	# calculate the proportional values of samples
	cdf = 1. * np.arange(len(data)) / (len(data) - 1)

	tmp = []

	for k in range(len(cdf)):
		if cdf[k] >= 0.25:
			tmp.append(data_sorted[k])
			break

	for k in range(len(cdf)):
		if cdf[k] >= 0.5:
			tmp.append(data_sorted[k])
			break

	for k in range(len(cdf)):
		if cdf[k] >= 0.75:
			tmp.append(data_sorted[k])
			break

	for k in range(len(cdf)):
		if cdf[k] >= 0.95:
			tmp.append(data_sorted[k])
			break

	return tmp


for trace in [sys.argv[1]]:
	labels = ["alccVerusCubic","alccVerusCubicNL", "verus"]

	delays1 = []
	delayTimes1 = []
	delays2 = []
	delayTimes2 = []
	delays3 = []
	delayTimes3 = []
	throughputDL1 = []
	timeDL1 = []
	throughputDL2 = []
	timeDL2 = []
	throughputDL3=[]
	timeDL3=[]

	if True:
		
		for algo in labels:
			if "alccVerusCubic" == algo:
				os.system("tshark -r ./Verusloss/{0}/{1}/log.pcap -T fields -e frame.time_epoch -e frame.len 'tcp.srcport==60001' > ./Verusloss/{0}/{1}/throughput.csv 2> /dev/null".format(algo,trace))
				throughputDL1, timeDL1 = parse_throughput("./Verusloss/{0}/{1}/throughput.csv".format(algo,trace))
				delays1, delayTimes1 = parse_delay("./Verusloss/{0}/{1}/".format(algo,trace)+"Receiver.out")
			elif "alccVerusCubicNL" == algo:
				print (algo)
				os.system("tshark -r ./Verusloss/{0}/{1}/log.pcap -T fields -e frame.time_epoch -e frame.len 'tcp.srcport==60001' > ./Verusloss/{0}/{1}/throughput.csv 2> /dev/null".format(algo,trace))
				throughputDL2, timeDL2 = parse_throughput("./Verusloss/{0}/{1}/throughput.csv".format(algo,trace))

				print(timeDL2)
				print(throughputDL2[:100])
				delays2, delayTimes2 = parse_delay("./Verusloss/{0}/{1}/".format(algo,trace)+"Receiver.out")
			else:
				throughputDL3, timeDL3 = parse_throughput("./Verusloss/{0}/{1}/client_60001.out".format(algo,trace))
				delays3, delayTimes3 = parse_delay("./Verusloss/{0}/{1}/".format(algo,trace)+"Receiver.out")


	sns.set_style("white")
	fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(18,10), facecolor='w', sharex=True)
	
	ax2.plot(delayTimes1, delays1, color="g", lw=5)
	ax2.plot(delayTimes2, delays2, color="#ff0000", lw=5)
	ax2.plot(delayTimes3, delays3, color="#0000ff", lw=5)
	ax2.set_xlim([0,250])
	ax2.set_xlabel('Time (s)')
	ax2.set_ylabel('Delay (ms)')
	ax2.set_yscale('log',basey=10)
	ax2.grid(True, which="both")

	f1=f1 = open ("../../channelTraces/"+sys.argv[1],"r")
	BW = []
	nextTime = 2900
	cnt = 0
	for line in f1:
	    if int(line.strip()) > nextTime:
	        BW.append(cnt*1492*8)
	        cnt = 0
	        nextTime+=1000
	    else:
	        cnt+=1
	f1.close()
	ax1.fill_between(range(len(BW)), 0, list(map(scale,BW)),color='#D3D3D3')
	

	p1, = ax1.plot(timeDL2, throughputDL2, color="#ff0000", lw=5, label='alccVerus (No loss)')
	p2, = ax1.plot(timeDL1, throughputDL1, color="g", lw=5, label='alccVerus (loss=1%)')
	p3, = ax1.plot(timeDL3, throughputDL3, color="#0000ff", lw=5, label='verus (loss=1%)')
	ax1.set_ylabel("Throughput\n(Mbps)")
	ax1.set_xlabel("Time (s)")
	ax1.set_ylim([0,50])

	fig.legend((p1,p2,p3),(p1.get_label(),p2.get_label(),p3.get_label()),ncol=3,loc="upper center")
	plt.subplots_adjust(top=0.85)
	if not os.path.exists('figures'):
		os.makedirs('figures')
	fig.savefig('./figures/verusloss-{0}.png'.format(sys.argv[1]),bbox_inches='tight')
	plt.close(fig)
