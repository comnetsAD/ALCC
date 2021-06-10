import os

TIME=300
DIR='Results/Verusloss'

# compile bftpd with alcc verus library
os.system('echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make')

os.system('sudo sysctl -w net.ipv4.tcp_congestion_control=cubic')

for trace in ['cellularGold', 'CampusWalk']:
	print (trace)

	os.system('python run.py -tr {0} -t {1} --name {0} --dir {2} --algo {3}'.format(trace,TIME,DIR,"verusLoss"))

	os.system('''gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf' ''')
	os.system('python run.py -tr {0} -t {1} --name {0} --dir {2} --algo {3}'.format(trace,TIME,DIR,"alccVerusCubic"))
	os.system('sudo killall bftpd')

	os.system('''gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf' ''')
	os.system('python run.py -tr {0} -t {1} --name {0} --dir {2} --algo {3}'.format(trace,TIME,DIR,"alccVerusCubicNL"))
	os.system('sudo killall bftpd')


