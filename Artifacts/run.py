from subprocess import Popen, PIPE, call
from argparse import ArgumentParser
from multiprocessing import Process
import signal
import subprocess
import sys
import os
import threading
from time import sleep, time
import threading

USER_NAME= 'alcc'
PASSWORD = 'password'
IP_ADDRESS = '10.0.2.15'
FILE_TO_DOWNLOAD = 'sampleVideo.mp4'

def positive_int(arg):
        arg = int(arg)
        if arg <= 0:
                raise argparse.ArgumentError('Argument must be a positive integer')
        return arg

def simpleRun():
	print args

	if args.algo == 'alcc_copa':
		RunAlccCopa()
	elif args.algo == 'alcc_verus':
	 	RunAlccVerus()
	elif args.algo == 'verus':
	 	RunVERUS()
	elif args.algo == 'copa':
	 	RunCopa()
	elif args.algo == 'alccVerusCubic':
		RunAlccVerusCubic()
	elif args.algo == 'alccVerusCubicNL':
		RunAlccVerusCubicNL()	
	print ("Finished")
	return

def RunCopa():
	if not os.path.exists(args.dir + '/copa'):
		os.makedirs(args.dir+ "/copa")
	if not os.path.exists(args.dir + '/copa/'+str(args.name)):
		os.makedirs(args.dir+ "/copa/"+str(args.name))

	args.dir = args.dir+"/copa/"

	command = "./../Applications/genericCC/receiver &"
	pro = Popen(command, stdout=PIPE, shell=True, preexec_fn=os.setsid)
	sleep(2)

	print("Begin " + str(args.time) + " seconds of copa transmission")
	command = "mm-delay 10 mm-link --meter-uplink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)

	print command
	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("sudo tcpdump -i any -s 96 -w " + args.dir + "/" + args.name + "/log.pcap & \n export MIN_RTT=1000 && ../Applications/genericCC/sender serverip=$MAHIMAHI_BASE offduration=0 onduration=900000000 cctype=markovian delta_conf=do_ss:auto:0.02 traffic_params=byte_switched,num_cycles=5 > info.out & \n sleep " + str(args.time) + " \n exit")

	os.system("ps | pgrep -f /genericCC/sender | xargs kill -9")
	os.system("ps | pgrep -f /genericCC/receiver | xargs kill -9")
	os.system("ps | pgrep -f tcpdump | sudo xargs kill -9")
	os.system("mv info.out "+args.dir+str(args.name)+"/")

def RunAlccCopa():
	output = "Not"
	while output != "":
		output = os.popen('netstat -o | grep 60001').read()
		if output != "":
			os.system("sudo kill -9 `sudo lsof -t -i:60001`")
			print output.split()[-1]
		sleep (1)

	if not os.path.exists(args.dir + '/alccCopa'):
		os.makedirs(args.dir+ "/alccCopa")
	if not os.path.exists(args.dir + '/alccCopa/'+str(args.name)):
		os.makedirs(args.dir+ "/alccCopa/"+str(args.name))

	args.dir = args.dir+"/alccCopa/"

	print("Begin " + str(args.time) + " seconds of AlCC_Copa transmission")
	command = "mm-delay 10 mm-link --meter-downlink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)

	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("sudo tcpdump -i any -s 96 -w " + args.dir + "/" + args.name + "/log.pcap & \n wget ftp://{0}:{1}@{2}/{3} & \n sleep ".format(USER_NAME, PASSWORD, IP_ADDRESS, FILE_TO_DOWNLOAD) + str(args.time) + " \n exit")

	os.system("ps | pgrep -f wget | sudo xargs kill -9")
	os.system("ps | pgrep -f tcpdump | sudo xargs kill -9")
	os.system("sudo kill -9 `sudo lsof -t -i:60001`")
	os.system("sudo mv ~/Desktop/2021-* "+args.dir+str(args.name)+"/")
	os.system("sudo rm -r ~/Desktop/2021-*")
	os.system("sudo chown {0}:{0} ".format(USER_NAME)+args.dir+str(args.name)+"/*")
	os.system("sudo rm sampleVideo.mp4*")

def RunAlccVerus():
	output = "Not"
	while output != "":
		output = os.popen('netstat -o | grep 60001').read()
		if output != "":
			os.system("sudo kill -9 `sudo lsof -t -i:60001`")
			print output.split()[-1]
		sleep (1)

	if not os.path.exists(args.dir + '/alccVerus'):
		os.makedirs(args.dir+ "/alccVerus")
	if not os.path.exists(args.dir + '/alccVerus/'+str(args.name)):
		os.makedirs(args.dir+ "/alccVerus/"+str(args.name))

	args.dir = args.dir+"/alccVerus/"
	
	print("Begin " + str(args.time) + " seconds of Model VERUS transmission")
	command = "mm-delay 10 mm-link --meter-downlink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)

	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("sudo tcpdump -i any -s 96 -w " + args.dir + "/" + args.name + "/log.pcap & \n wget ftp://{0}:{1}@{2}/{3} & \n sleep ".format(USER_NAME, PASSWORD, IP_ADDRESS, FILE_TO_DOWNLOAD) + str(args.time) + " \n exit")

	os.system("ps | pgrep -f wget | sudo xargs kill -9")
	os.system("ps | pgrep -f tcpdump | sudo xargs kill -9")
	os.system("sudo kill -9 `sudo lsof -t -i:60001`")
	os.system("sudo mv ~/Desktop/2021-*/* "+args.dir+str(args.name)+"/")
	os.system("sudo rm -r ~/Desktop/2021-*")
	os.system("sudo chown {0}:{0} ".format(USER_NAME)+args.dir+str(args.name)+"/*")
	os.system("sudo rm sampleVideo.mp4*")

def RunVERUS():
	if not os.path.exists(args.dir + '/verus'):
		os.makedirs(args.dir+ "/verus")
	if not os.path.exists(args.dir + '/verus/'+str(args.name)):
		os.makedirs(args.dir+ "/verus/"+str(args.name))
	
	print("Begin " + str(args.time) + " seconds of verus transmission")
	command = "./../Applications/verus/src/verus_server -name "+args.dir + "/verus/"+str(args.name)+" -p 60001 -t "+ str(args.time)#+" > rubbishVerus"
	print command
	pro = Popen(command, stdout=PIPE, shell=True, preexec_fn=os.setsid)
	
	command = "mm-delay 10 mm-link --meter-downlink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)
	print command

	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("./../Applications/verus/src/verus_client $MAHIMAHI_BASE -p 60001\nexit\n")

	os.system("ps | pgrep -f verus_server | xargs kill -9")
	os.system("ps | pgrep -f verus_client | xargs kill -9")
	#os.system("sudo kill -9 `sudo lsof -t -i:60001`")
	os.system("mv client_60001* "+args.dir + "/verus/"+str(args.name)+"/")

def RunAlccVerusCubic():
	output = "Not"
	while output != "":
		output = os.popen('netstat -o | grep 60001').read()
		if output != "":
			os.system("sudo kill -9 `sudo lsof -t -i:60001`")
			print output.split()[-1]
		sleep (1)

	if not os.path.exists(args.dir + '/alccVerusCubic'):
		os.makedirs(args.dir+ "/alccVerusCubic")
	if not os.path.exists(args.dir + '/alccVerusCubic/'+str(args.name)):
		os.makedirs(args.dir+ "/alccVerusCubic/"+str(args.name))

	args.dir = args.dir+"/alccVerusCubic/"
	
	print("Begin " + str(args.time) + " seconds of Model VERUS transmission")
	command = "mm-delay 10 mm-loss uplink 0.01 mm-link --meter-downlink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)

	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("sudo tcpdump -i any -s 96 -w " + args.dir + "/" + args.name + "/log.pcap & \n wget ftp://{0}:{1}@{2}/{3} & \n sleep ".format(USER_NAME, PASSWORD, IP_ADDRESS, FILE_TO_DOWNLOAD) + str(args.time) + " \n exit")

	os.system("ps | pgrep -f wget | sudo xargs kill -9")
	os.system("ps | pgrep -f tcpdump | sudo xargs kill -9")
	os.system("sudo kill -9 `sudo lsof -t -i:60001`")
	os.system("sudo mv /tmp/2021-*/* "+args.dir+str(args.name)+"/")
	os.system("sudo rm -r /tmp/2021-*")
	os.system("sudo chown muhammad:muhammad "+args.dir+str(args.name)+"/*")
	os.system("sudo rm sampleVideo.mp4*")
	
def RunAlccVerusCubicNL():
	output = "Not"
	while output != "":
		output = os.popen('netstat -o | grep 60001').read()
		if output != "":
			os.system("sudo kill -9 `sudo lsof -t -i:60001`")
			print output.split()[-1]
		sleep (1)

	if not os.path.exists(args.dir + '/alccVerusCubicNL'):
		os.makedirs(args.dir+ "/alccVerusCubicNL")
	if not os.path.exists(args.dir + '/alccVerusCubicNL/'+str(args.name)):
		os.makedirs(args.dir+ "/alccVerusCubicNL/"+str(args.name))

	args.dir = args.dir+"/alccVerusCubicNL/"
	
	print("Begin " + str(args.time) + " seconds of Model VERUS transmission")
	command = "mm-delay 10 mm-link --meter-downlink ../Eval/channelTraces/" + str(args.trace) + " ../Eval/channelTraces/"  + str(args.trace)

	p = Popen(command, stdin=PIPE,shell=True)
	p.communicate("sudo tcpdump -i any -s 96 -w " + args.dir + "/" + args.name + "/log.pcap & \n wget ftp://{0}:{1}@{2}/{3} & \n sleep ".format(USER_NAME, PASSWORD, IP_ADDRESS, FILE_TO_DOWNLOAD) + str(args.time) + " \n exit")

	os.system("ps | pgrep -f wget | sudo xargs kill -9")
	os.system("ps | pgrep -f tcpdump | sudo xargs kill -9")
	os.system("sudo kill -9 `sudo lsof -t -i:60001`")
	os.system("sudo mv /tmp/2021-*/* "+args.dir+str(args.name)+"/")
	os.system("sudo rm -r /tmp/2021-*")
	os.system("sudo chown muhammad:muhammad "+args.dir+str(args.name)+"/*")
	os.system("sudo rm sampleVideo.mp4*")

if __name__ == '__main__':
	parser = ArgumentParser(description="Shallow queue tests")
	parser.add_argument('--dir', '-d',help="Directory to store outputs",required=True)
	parser.add_argument('--trace', '-tr',help="Cellsim traces to be used",required=True)
	parser.add_argument('--time', '-t',help="Duration (sec) to run the experiment",type=int,default=10)
	parser.add_argument('--name', '-n',help="name of the experiment",required=True)
	parser.add_argument('--algo',help="Algorithm under which we are running the simulation",required=True)
	parser.add_argument('--tcp_probe',help="whether tcp probe should be run or not",action='store_true',default=False)
	parser.add_argument('--command', '-c', help="mm-link command to run", required=False)                
	parser.add_argument('--queue', type=positive_int, help='Queue size in mahimahi')
	args = parser.parse_args()
	simpleRun()


