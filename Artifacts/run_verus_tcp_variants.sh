#!/bin/bash

time=300
dir=Results
dir1=Results/alccVerus

# compile bftpd with alcc verus library
gnome-terminal -- sh -c 'echo "compiling bftpd for alcc verus" && cd ../Applications/bftpd && cp Makefile_verus Makefile && make'

for algo in cubic bic reno; do #reno cubic bic
	sudo sysctl -w net.ipv4.tcp_congestion_control=$algo
	for trace in highwayGold CampusWalk; do
		i=1
		end=5
		while [ $i -le $end ]; do
			echo $trace$i

			gnome-terminal -- sh -c 'echo "Running bftpd server" && cd ../Applications/bftpd && pwd && sudo ./bftpd -D -c bftpd.conf'
			python run.py -tr $trace -t $time --name $trace$i --dir $dir1$algo --algo alcc_verus
			sudo killall bftpd

	    	i=$(($i+1))

		done
			
	done
done

#If you have Verus runs for highwayGold and Campus Walk already. Then comment out the following code i.e., from line 29-39
for trace in highwayGold CampusWalk; do
		end=5
		while [ $i -le $end ]; do
			echo $trace$i
			python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
	    	i=$(($i+1))

		done
			
	done
