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
		  'font.size' : 20,
		  'legend.fontsize': 16,
		  'text.latex.unicode': True,
		  }
plt.rcParams.update(params)

plt.rcParams['ytick.labelsize'] = 20
plt.rcParams['xtick.labelsize'] = 20
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

		delays.append((float(tokens[1])*1000))
		times.append((float(tokens[0])-sTime))

	delay_file.close()

	lastTime = times[-1]

	if "0" in filename:
		print "---------------------"
		cnt = 1
		file = filename.split("0")
		file = file[0]+str(cnt)+file[1]
		print file

		while os.path.isfile(file):
			print file

			delay_file = open(file,"r")
			delay_file = open(filename,"r")
			tokens = delay_file.readline().strip().split()
			sTime = float(tokens[0])
			delays.append((float(tokens[1])*1000.0))
			times.append((float(tokens[0])))

			for line in delay_file:
				tokens = line.strip().split()

				delays.append((float(tokens[1])*1000))
				times.append((float(tokens[0])-sTime)+lastTime)


			cnt+=1
			file = filename.split("0")
			file = file[0]+str(cnt)+file[1]
			lastTime = times[-1]

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


for trace in [sys.argv[1]]: #sys.arv[1] is the trace name i.e., CampusWalk or highwayGold for Fig.11 a,b,c d. 
	labels = ["cubic","bic","reno","verus"]

	sns.set_style("white")
	fig, (ax1, ax2) = plt.subplots(2, figsize=(8,6), facecolor="w") 
	fig.tight_layout(pad=2.0)

	# fig, (ax) = plt.subplots(1, figsize=(8,5), facecolor="w") 

	totalThroughput = {'verus':[], 'cubic':[], 'reno':[], 'bic':[]}
	totalDelay = {'verus':[], 'cubic':[], 'reno':[], 'bic':[]}

	if True:
		colors = {'verus':'b', 'cubic':'r', 'reno':'g', 'bic':'m'}
		tsharkF = {"alccVerus":"src"}
		for algo in labels:

			# plotting throughput
			throughputDL = []
			timeDL = []
			for i in range(1,int(sys.argv[3])+1):
				print algo + str(i)

				if 'verus' not in algo:
					os.system("tshark -r ./alccVerus/{1}{0}{2}/log.pcap -T fields -e frame.time_epoch -e frame.len 'tcp.srcport==60001' > ./alccVerus/{1}{0}{2}/throughput.csv 2> /dev/null".format(algo,trace,i))
					throughputDL, timeDL = parse_throughput("./alccVerus/{1}{0}{2}/throughput.csv".format(algo,trace,i))
					delays, delayTimes = parse_delay("./alccVerus/{1}{0}{2}/".format(algo,trace,1)+"Receiver0.out")
					totalDelay[algo] += delays
					totalThroughput[algo] += throughputDL
				else:
					throughputDL, timeDL = parse_throughput("./{0}/{1}{2}/client_60001.out".format(algo,trace,i))
					delays, delayTimes = parse_delay("./{0}/{1}{2}/".format(algo,trace,1)+"Receiver.out")
					totalDelay[algo] += delays
					totalThroughput[algo] += throughputDL

		for name in labels:			
			sns.kdeplot(totalThroughput[name] , color=colors[name], shade=True, ax = ax1, lw=3)
			sns.kdeplot(totalDelay[name] , color=colors[name], label=name, shade=True, ax = ax2, lw=3)

	ax1.set_ylabel('Probability')
	ax1.set_xlabel('Throughput (Mbps)')

	ax2.set_ylabel('Probability')
	ax2.set_xlabel('Delay (ms)')

	plt.xlim([0,max(totalThroughput[algo])])
	plt.xlim([0,800])

	plt.legend(loc='best')
	ax1.grid()
	ax2.grid()
	plt.savefig("./figures/verus_TCP_pdf_{0}.png".format(trace),dpi=300,bbox_inches='tight')
	plt.close()

	fig, (ax) = plt.subplots(1, figsize=(8,5), facecolor="w") 
	overallThroughput = []
	overallDelay = []

	for name in labels:
		overallThroughput.append(simple_cdf(totalThroughput[name]))
		overallDelay.append(simple_cdf(totalDelay[name]))
	# print("---------",overallThroughput,"-----------")	
	# print("---------",overallDelay,"------------")
	for i in range(len(labels)):
		if labels[i] != 'verus':
			x = (overallDelay[i][2]+overallDelay[i][0])/2.0
			y = (overallThroughput[i][2]+overallThroughput[i][0])/2.0
			ellipse = Ellipse(xy=(x,y), width=(overallDelay[i][2]-overallDelay[i][0]), 
				height=(overallThroughput[i][2]-overallThroughput[i][0]), edgecolor=colors[labels[i]], fc='None', lw=3,
				alpha=.9, label='alcc'+labels[i])
		else:
			x = (overallDelay[i][2]+overallDelay[i][0])/2.0
			y = (overallThroughput[i][2]+overallThroughput[i][0])/2.0
			ellipse = Ellipse(xy=(x,y), width=(overallDelay[i][2]-overallDelay[i][0]), 
				height=(overallThroughput[i][2]-overallThroughput[i][0]), edgecolor=colors[labels[i]], fc='None', lw=3,
				alpha=.9, label='verus')	
		#print labels[i], overallThroughput[i][2], overallThroughput[i][0]
		#print labels[i], overallDelay[i][2], overallDelay[i][0]

		ax.add_patch(ellipse)
		plt.plot(overallDelay[i][1],overallThroughput[i][1],marker='x',mew=3,color=colors[labels[i]])

plt.legend(loc='best')
plt.grid()  

plt.ylabel("Throughput (Mbps)")
plt.xlabel("Delay (s)")

try:
	plt.xlim([0, 1.2*max(overallDelay[0][2],overallDelay[1][2])])
	plt.ylim([0, 1.2*max(overallThroughput[0][2],overallThroughput[1][2])])
except:
	pass
if not os.path.exists('figures'):
		os.makedirs('figures')
plt.savefig("./figures/verus_overall_TCP_"+trace+'.png',bbox_inches='tight')
plt.close()
