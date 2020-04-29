#!/bin/bash
sudo dmesg -c
sudo ./queue_implement_scheduler < OS_PJ1_Test/TIME_MEASUREMENT.txt
dmesg > time.txt
python3 count_time.py
rm time.txt
