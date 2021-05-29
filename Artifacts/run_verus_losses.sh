#!/bin/bash

time=300
dir=Results/Verusloss
sudo sysctl -w net.ipv4.tcp_congestion_control=cubic
for trace in cellularGold; do 
	
	i=1
	end=1
	while [ $i -le $end ]; do
		echo $trace$i
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alccVerusCubic
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alccVerusCubicNL
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
    	i=$(($i+1))

	done
		
done
