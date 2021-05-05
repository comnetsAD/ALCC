#!/bin/bash

time=300
dir=Results

for trace in CampusWalk Highway cellularGold Corniche; do 
	
	i=1
	end=20
	while [ $i -le $end ]; do
		echo $trace$i
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_copa
		# python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo copa
    	i=$(($i+1))

	done
		
done