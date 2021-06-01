#!/bin/bash

time=10
dir=Results

sudo sysctl -w net.ipv4.tcp_congestion_control=cubic

# compile bftpd with alcc verus library
gnome-terminal -- sh -c 'echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make'

for trace in highwayGold CityDrive Corniche rapidGold; do

	i=1
	end=1
	while [ $i -le $end ]; do
		echo $trace$i

		gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf'
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_verus
		sudo killall bftpd

		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
    	i=$(($i+1))

	done

done
