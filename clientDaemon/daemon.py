#!/usr/bin/env python
import sys
import requests
import threading
import os
import psutil
import json

def register_self():
	r = requests.get('http://10.10.1.2:9080/register')
	try:
		response = r.json()
		id = response.id
		return id
	
	except Exception as error:
		pass
		#raise Exception()

stats_skeleton = '{"cpu":"","freemem":"","freedisk":""}'
stats_skeleton_json = json.loads(stats_skeleton)

def collect_stats():
	threading.Timer(1.0, collect_stats).start()
	#collect cpu stats
	cpuusage = psutil.cpu_percent(interval=0.1)

	#collect memory stats
	freemem = psutil.virtual_memory().free >> 20

	#collect storage stats
	freedisk = psutil.disk_usage('/').free >> 20

	#pack all stats
	stats_skeleton_json['cpu'] = cpuusage
	stats_skeleton_json['freemem'] = freemem
	stats_skeleton_json['freedisk'] = freedisk
	#send the stats to the scheduler
	r = requests.post('http://10.10.1.2:9081/statistics', json = json.dumps(stats_skeleton_json))
	print r.status_code

	#print json.dumps(stats_skeleton_json)

def main():
	#register self with scheduler
	#id = register_self()
	#collect stats
	collect_stats()
		
if __name__ == '__main__':
	main()	

