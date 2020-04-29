#!/bin/bash

set -o xtrace
sudo dmesg -c
timeMeasure="TIME_MEASUREMENT.txt"
unit_time=$(bash cal_time_unit.sh | grep avg | awk '{print $2}')
echo $unit_time

testDir="OS_PJ1_Test/"
outDir="output/"
for exp in "FIFO_" "RR_" "SJF_" "PSJF_";do
	for n in "1" "2" "3" "4" "5"; do
		sudo dmesg -c
		sudo ./queue_implement_scheduler < $testDir$exp$n.txt > $outDir$exp$n'_stdout.txt'
		dmesg | grep Project1 > $outDir$exp$n'_dmesg.txt'
		echo $exp'-'$n >> ExpResult.txt
		python3 eval.py --file $outDir$exp$n'_dmesg.txt' --perfect Vperfect.txt --unit $unit_time >> ExpResult.txt
		echo ""
		rm -rf Vperfect.txt
	done
done
