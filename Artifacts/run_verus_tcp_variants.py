import os

TIME=300
DIR='Results'
NUM_RUNS=20

# compile bftpd with alcc verus library
os.system('echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make')

for trace in ['highwayGold', 'CampusWalk']:
	for i in range(1,NUM_RUNS+1):
		os.system('python run.py -tr {0} -t {1} --name {0}{2} --dir {3} --algo verus'.format(trace,TIME,i,DIR))

		for tcp_flavor in ['cubic','bic','reno']:
			os.system('sudo sysctl -w net.ipv4.tcp_congestion_control={0}'.format(tcp_flavor))

			print (trace)

			os.system('''gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf' ''')
			os.system('python run.py -tr {0} -t {1} --name {0}{4}{2} --dir {3} --algo alcc_verus'.format(trace,TIME,i,DIR,tcp_flavor))
			os.system('sudo killall bftpd')
