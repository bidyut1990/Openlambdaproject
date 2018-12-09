#!/usr/bin/env python
import os
import psutil
cpuusage = psutil.cpu_percent(interval=0.1)
freemem = psutil.virtual_memory().free >> 20
freedisk = psutil.disk_usage('/').free >> 20
print cpuusage 
print freemem 
print freedisk
#ret = os.system("top -bn 2 -d 0.01 | grep '^%Cpu' | tail -n 1 | gawk '{print $2+$4+$6}'")

