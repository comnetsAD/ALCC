import os

TIME=300
DIR='Results'
NUM_RUNS=20

os.system('sudo sysctl -w net.ipv4.tcp_congestion_control=cubic')

# compile bftpd with alcc verus library
os.system('echo "compiling bftpd for alcc copa" && cd ../Applications/bftpd && cp Makefile_copa Makefile && make')

for trace in ['CampusWalk', 'Highway', 'cellularGold', 'Corniche']:
	for i in range(1,NUM_RUNS+1):
		print (trace)

		os.system('''gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf' ''')
		os.system('python run.py -tr {0} -t {1} --name {0}{2} --dir {3} --algo alcc_copa'.format(trace,TIME,i,DIR))
		os.system('sudo killall bftpd')

		os.system('python run.py -tr {0} -t {1} --name {0}{2} --dir {3} --algo copa'.format(trace,TIME,i,DIR))


