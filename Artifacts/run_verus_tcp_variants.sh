#!/bin/bash

time=300
dir=Results
dir1=Results/alccVerus

for algo in cubic bic reno; do #reno cubic bic
	sudo sysctl -w net.ipv4.tcp_congestion_control=$algo
	for trace in highwayGold CampusWalk; do
		i=1
		end=5
		while [ $i -le $end ]; do
			echo $trace$i
			python run.py -tr $trace -t $time --name $trace$i --dir $dir1$algo --algo alcc_verus
	    	i=$(($i+1))

		done
			
	done
done

#If you have Verus runs for highwayGold and Campus Walk already. Then comment out the following code i.e., from line 23-34
for trace in highwayGold CampusWalk; do
		end=5
		while [ $i -le $end ]; do
			echo $trace$i
			python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
	    	i=$(($i+1))

		done
			
	done
