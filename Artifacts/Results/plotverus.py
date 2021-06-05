#-----------------------------Plot Run Commands------------------------------------#
#  python plotverus.py Highway 5 300 <-------python plotverus.py channeltrace numberoffiles timedurationoftheruns
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

if not os.path.exists('figures'):
	os.makedirs('figures')	

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

		if sTime - firstTime > float(sys.argv[3]):
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

		if (float(tokens[0])-sTime) < float(sys.argv[3]):
			delays.append((float(tokens[2])))
			times.append((float(tokens[0])-sTime))

	delay_file.close()

	return  delays, times


NUM_RUNS = int(sys.argv[2])

for trace in [sys.argv[1]]:
	totalThroughput = {'verus':[], 'alccVerus':[]}
	totalDelay = {'verus':[], 'alccVerus':[]}

	totalThroughput2 = {'verus':[[] for i in range(NUM_RUNS)], 'alccVerus':[[] for i in range(NUM_RUNS)]}
	totalDelay2 = {'verus':[[] for i in range(NUM_RUNS)], 'alccVerus':[[] for i in range(NUM_RUNS)]}

	for i in range(1,NUM_RUNS+1):
		print i

		#try:
		if True:
			fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8,5), facecolor='w', sharex=True)
			ax = plt.gca()

			# plotting the trace file
			f1 = open ("../../channelTraces/{}".format(trace),"r")
			BW = []
			nextTime = 2900
			cnt = 0
			for line in f1:
				#print line
				if int(line.strip()) > nextTime:
					BW.append(cnt*1492*8)
					cnt = 0
					nextTime+=1000
				else:
					cnt+=1
			f1.close()

			ax1.fill_between(range(len(BW)), 0, list(map(scale,BW)),color='#D3D3D3')

			colors = {"alccVerus":"r","verus":"b"}
			tsharkF = {"alccVerus":"src"}
			for algo in ["verus","alccVerus"]:
				print algo

				if algo=="alccVerus":
					os.system("tshark -r ./{0}/{1}{2}/log.pcap -T fields -e frame.time_epoch -e frame.len 'tcp.srcport==60001' > ./{0}/{1}{2}/throughput.csv".format(algo,trace,i))
				
				print "./{0}/{1}{2}/".format(algo,trace,i)+"Receiver.out"
				delays, delayTimes = parse_delay("./{0}/{1}{2}/".format(algo,trace,i)+"Receiver.out")

				#print delays
				ax2.plot(delayTimes, delays, color=colors[algo], lw=3, rasterized=True, label=algo)
				totalDelay[algo] += delays
				totalDelay2[algo][i-1] = delays

				# plotting throughput
				throughputDL = []
				timeDL = []

				if 'alccVerus' == algo:
					throughputDL, timeDL = parse_throughput("./{0}/{1}{2}/throughput.csv".format(algo,trace,i))
				else:
					throughputDL, timeDL = parse_throughput("./{0}/{1}{2}/client_60001.out".format(algo,trace,i))
				ax1.plot(timeDL, throughputDL, color=colors[algo], lw=3, rasterized=True, label=algo)

				totalThroughput[algo] += throughputDL
				totalThroughput2[algo][i-1] = throughputDL

			ax1.set_ylabel("Throughput\n(Mbps)")
			ax1.legend(loc='best', prop={'size':12})
			ax1.set_xlim([0,int(sys.argv[3])])
			ax2.set_xlim([0,int(sys.argv[3])])
			ax2.set_xlabel('Time (s)')
			ax2.set_ylabel('Delay (ms)')
			ax2.set_yscale('log',basey=10) 
			ax2.grid(True, which="both")

			plt.savefig('./figures/{0}_{1}{2}.png'.format(algo,trace,i),bbox_inches='tight')
			plt.close()		
		#except:
		#	print ("Error with", algo, trace, i)
		#	continue

	sns.set_style("white")
	fig, (ax1, ax2) = plt.subplots(2, figsize=(8,6), facecolor="w") 
	fig.tight_layout(pad=2.0)

	colors= {'alccVerus':"red", "verus":"blue"}

	for name in ["alccVerus","verus"]:
		print totalThroughput
		
		sns.kdeplot(totalThroughput[name] , color=colors[name], shade=True, ax = ax1, lw=3)
		sns.kdeplot(totalDelay[name] , color=colors[name], label=name, shade=True, ax = ax2, lw=3)

	ax1.set_ylabel('Probability')
	ax1.set_xlabel('Throughput (Mbps)')

	ax2.set_ylabel('Probability')
	ax2.set_xlabel('Delay (ms)')

	plt.xlim([0,800])

	plt.legend(loc='best')
	ax1.grid()
	ax2.grid()
	plt.savefig("./figures/verus_pdf_{0}.png".format(trace),dpi=300,bbox_inches='tight')
	plt.close()

	fig, (ax1, ax2) = plt.subplots(2, figsize=(8,5), facecolor="w") 
	fig.tight_layout(pad=2.0)

	colors= {"alccVerus":"red", 'verus':"blue"}
	for name in ["alccVerus","verus"]:

		a,b = cdfplot(totalThroughput[name])
		ax1.plot(a, b, label=name, color=colors[name])

		a,b = cdfplot(totalDelay[name])
		ax2.plot(a, b, label=name, color=colors[name])


	ax1.set_ylabel('Throughput CDF')
	ax1.set_xlabel('Throughput (Mbps)')
	ax2.set_ylabel('Delay CDF')
	ax2.set_xlabel('Delay (ms)')


	plt.xlim([0,800])
	plt.legend(loc='best')
	ax1.grid()
	ax2.grid()
	plt.savefig("./figures/verus_cdf_{0}.png".format(trace),dpi=300,bbox_inches='tight')
	plt.close()

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


	labels = ["alccVerus","verus"]
	colors=['r','b']

	fig, (ax) = plt.subplots(1, figsize=(8,5), facecolor="w") 

	overallThroughput = []
	overallDelay = []

	for name in labels:
		overallThroughput.append(simple_cdf(totalThroughput[name]))
		overallDelay.append(simple_cdf(totalDelay[name]))

	for i in range(len(labels)):

		x = (overallDelay[i][2]+overallDelay[i][0])/2.0
		y = (overallThroughput[i][2]+overallThroughput[i][0])/2.0
		ellipse = Ellipse(xy=(x,y), width=(overallDelay[i][2]-overallDelay[i][0]), 
			height=(overallThroughput[i][2]-overallThroughput[i][0]), edgecolor=colors[i], fc='None', lw=3,
			alpha=.9, label=labels[i])

		print labels[i], overallThroughput[i][2], overallThroughput[i][0]
		print labels[i], overallDelay[i][2], overallDelay[i][0]

		ax.add_patch(ellipse)
		plt.plot(overallDelay[i][1],overallThroughput[i][1],marker='x',mew=2,color=colors[i])

	plt.legend(loc='best')
	plt.grid()  

	plt.ylabel("Throughput (Mbps)")
	plt.xlabel("Delay (s)")

	try:
		plt.xlim([0, 1.2*max(overallDelay[0][2],overallDelay[1][2])])
		plt.ylim([0, 1.2*max(overallThroughput[0][2],overallThroughput[1][2])])
	except:
		pass

	plt.savefig("./figures/verus_overall_"+trace+'.png',bbox_inches='tight')
	plt.close()
