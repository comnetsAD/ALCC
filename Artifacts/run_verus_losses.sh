#!/bin/bash

time=300
dir=Results/Verusloss

# compile bftpd with alcc verus library
gnome-terminal -- sh -c 'echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make'

sudo sysctl -w net.ipv4.tcp_congestion_control=cubic
for trace in cellularGold; do 
	
	i=1
	end=1
	while [ $i -le $end ]; do
		echo $trace$i

		gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf'
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alccVerusCubic
		sudo killall bftpd

		gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf'
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alccVerusCubicNL
		sudo killall bftpd

		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
    	i=$(($i+1))

	done
		
done
