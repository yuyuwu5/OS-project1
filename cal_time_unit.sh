#!/bin/bash
sudo dmesg -c
#sudo ./for_implement < OS_PJ1_Test/TIME_MEASUREMENT.txt
sudo ./queue_implement < OS_PJ1_Test/TIME_MEASUREMENT.txt
dmesg > time.txt
python3 count_time.py
rm time.txt
