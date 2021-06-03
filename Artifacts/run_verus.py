import os

TIME=60
DIR='Results'
NUM_RUNS=5

os.system('sudo sysctl -w net.ipv4.tcp_congestion_control=cubic')

# compile bftpd with alcc verus library
os.system('echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make')

for trace in ['highwayGold']:  # CityDrive Corniche rapidGold; do
	for i in range(1,NUM_RUNS+1):
		print (trace)

		os.system('''gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf' ''')
		os.system('python run.py -tr {0} -t {1} --name {0}{2} --dir {3} --algo alcc_verus'.format(trace,TIME,i,DIR))
		os.system('sudo killall bftpd')

		os.system('python run.py -tr {0} -t {1} --name {0}{2} --dir {3} --algo verus'.format(trace,TIME,i,DIR))

