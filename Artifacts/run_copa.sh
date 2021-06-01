#!/bin/bash

time=300
dir=Results

sudo sysctl -w net.ipv4.tcp_congestion_control=cubic

# compile bftpd with alcc verus library
gnome-terminal -- sh -c 'echo "compiling bftpd for alcc copa" && cd ../Applications/bftpd && cp Makefile_copa Makefile && make'

for trace in CampusWalk Highway cellularGold Corniche; do

	i=1
	end=10
	while [ $i -le $end ]; do
		echo $trace$i

		gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf'
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_copa
		sudo killall bftpd
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo copa

    	i=$(($i+1))

	done

done
