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

def simplify_cdf(data):
	'''Return the cdf and data to plot
		Remove unnecessary points in the CDF in case of repeated data
		'''
	yvals_1 = np.arange(len(data))/float(len(data))
	return yvals_1, data


	data_len = len(data)
	assert data_len != 0
	cdf = np.arange(data_len) / data_len
	simple_cdf = [0]
	simple_data = [data[0]]

	if data_len > 1:
		simple_cdf.append(1.0 / data_len)
		simple_data.append(data[1])
		for cdf_value, data_value in zip(cdf, data):
			if data_value == simple_data[-1]:
				simple_cdf[-1] = cdf_value
			else:
				simple_cdf.append(cdf_value)
				simple_data.append(data_value)

	assert len(simple_cdf) == len(simple_data)

	# to have cdf up to 1
	simple_cdf.append(1)
	simple_data.append(data[-1])

	return simple_cdf, simple_data

def cdfplot(data_in):
	"""Plot the cdf of a data array
		Wrapper to call the plot method of axes
		"""
	# cannot shortcut lambda, otherwise it will drop values at 0
	data = sorted(filter(lambda x: (x is not None and ~np.isnan(x)
									and ~np.isinf(x)),
						 data_in))

	data_len = len(data)
	simple_cdf, simple_data = simplify_cdf(data)

	return simple_data, simple_cdf

def scale(a):
	return a/1000000.0

def parse_throughput(filename):
	times = []
	pktsize = []

	throughput_file = open(filename,"r")
	tokens = throughput_file.readline().strip().split()

	sTime = float(tokens[0])
	firstTime = sTime
	bucket = []
	bucket.append(float(tokens[1]))

	for line in throughput_file:
		tokens = line.strip().split()
		if float(tokens[0])< sTime+1.0:
			bucket.append(float(tokens[1]))
		else:
			pktsize.append(sum(bucket)*8/1000000.0)
			bucket = []
			times.append(sTime-firstTime)

			while float(tokens[0])-sTime > 1.0:
				sTime += 1.0

	throughput_file.close()
	return pktsize, times

def parse_delay_copa (filename):
	delays = []
	times = []
	cnt = 0
	shift = 0

	delay_file = open(filename,"r")

	for line in delay_file:
		tokens = line.strip().split(",")

		if tokens[0] == '--RTT--':
			delays.append((float(tokens[2])))

			if len(times) >0 and float(tokens[1])+shift < times[-1]:
				shift = times[-1]

			times.append(float(tokens[1])+shift)
	delay_file.close()

	return  delays, times

def parse_delay(filename):
	delays = []
	times = []
	cnt = 0

	delay_file = open(filename,"r")
	tokens = delay_file.readline().strip().split(",")
	sTime = float(tokens[0])
	delays.append((float(tokens[1])))
	times.append((float(tokens[0])-sTime))

	for line in delay_file:
		tokens = line.strip().split(",")

		if float(tokens[1]) < 10000.0:
			delays.append((float(tokens[1])))
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

	return tmp

sns.set_style("white")
fig, (ax1, ax2) = plt.subplots(2, figsize=(8,6), facecolor="w") 
fig.tight_layout(pad=2.0)

for trace in [sys.argv[1]]:
	labels = ["cubic","bic","reno","copa"]

	totalThroughput = {"cubic":[],"bic":[],"reno":[],"copa":[]}
	totalDelay = {"cubic":[],"bic":[],"reno":[],"copa":[]}


	colors = {'copa':'b', 'cubic':'r', 'reno':'g', 'bic':'m'}
	tsharkF = {"copa":"src", "alccCopa":"dst"}
	for algo in labels:
		print algo

		throughputDL = []
		timeDL = []
		for i in range(1,int(sys.argv[3])+1):
			if "copa" == algo:
				os.system("tshark -r ./copa/{0}{1}/log.pcap -T fields -e frame.time_epoch -e frame.len 'ip.{2}==100.64.0.4' > ./copa/{0}{1}/throughput.csv".format(trace,i,"src"))
			else:
				os.system("tshark -r ./alccCopa/{1}{2}/log.pcap -T fields -e frame.time_epoch -e frame.len 'ip.{3}==100.64.0.4' > ./alccCopa/{1}{0}{2}/throughput.csv".format(algo,trace,i,"dst"))

			if algo == 'copa':
				delays, delayTimes = parse_delay_copa("./copa/{0}{1}/info.out".format(trace,i))
				throughputDL, timeDL = parse_throughput("./copa/{0}{1}/throughput.csv".format(trace,i))
				totalThroughput[algo] += throughputDL
				totalDelay[algo] += delays
			else:
				delays, delayTimes = parse_delay(glob("./alccCopa/{1}{0}{2}/*/".format(algo,trace,1))[0]+"Receiver.out")
				throughputDL, timeDL = parse_throughput("./alccCopa/{1}{0}{2}/throughput.csv".format(algo,trace,1))
				totalThroughput[algo] += throughputDL
				totalDelay[algo] += delays

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
plt.savefig("./figures/copa_TCP_pdf_{0}.png".format(trace),dpi=300,bbox_inches='tight')

fig,(ax) = plt.subplots(1,figsize=(8,5),facecolor="w")

overallThroughput = []
overallDelay = []

for name in labels:
	overallThroughput.append(simple_cdf(totalThroughput[name]))
	overallDelay.append(simple_cdf(totalDelay[name]))

for i in range(len(labels)):

	x = (overallDelay[i][2]+overallDelay[i][0])/2.0
	y = (overallThroughput[i][2]+overallThroughput[i][0])/2.0
	ellipse = Ellipse(xy=(x,y), width=(overallDelay[i][2]-overallDelay[i][0]), 
		height=(overallThroughput[i][2]-overallThroughput[i][0]), edgecolor=colors[labels[i]], fc='None', lw=3,
		alpha=.9, label=labels[i])

	print labels[i], overallThroughput[i][2], overallThroughput[i][0]
	print labels[i], overallDelay[i][2], overallDelay[i][0]

	ax.add_patch(ellipse)
	plt.plot(overallDelay[i][1],overallThroughput[i][1],marker='x',mew=3,color=colors[labels[i]])

plt.legend(loc='lower left')
plt.grid()  

plt.ylabel("Throughput (Mbps)")
plt.xlabel("Delay (s)")

try:
	plt.xlim([0, 1.12*max(overallDelay[0][2],overallDelay[1][2])])
	plt.ylim([0, 1.12*max(overallThroughput[0][2],overallThroughput[1][2])])
except:
	pass

plt.savefig("./figures/copa_overall_TCP_"+trace+'.png',bbox_inches='tight')
