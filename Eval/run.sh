#!/bin/bash

time=20
dir=Results

for trace in cellularGold; do #Highway Corniche CityDrive City CampusWalk Campus cellularGold highwayGold rapidGold ATT-LTE-driving.down ATT-LTE-driving-2016.down TMobile-LTE-driving.down TMobile-UMTS-driving.down Verizon-EVDO-driving.down Verizon-LTE-driving.down
	
	i=1
	end=2
	while [ $i -le $end ]; do
		echo $trace$i

		# python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_copa
		# python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo verus
		python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo alcc_verus
		# python run.py -tr $trace -t $time --name $trace$i --dir $dir --algo copa
    	i=$(($i+1))

	done
		
done