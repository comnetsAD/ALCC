#!/bin/bash

time=50
dir=Results

for trace in rapidGold CityDrive Corniche highwayGold; do 
	
	i=1
	end=2
	while [ $i -le $end ]; do
		echo $trace$i
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_verus
		# python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
    	i=$(($i+1))

	done
		
done
