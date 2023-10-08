#!/bin/python
#################################################################################################################################################
# Copyright (c) 2023 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A simple rsyslog implementation
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################

#################################################################################################################################################
# Dependencies
#################################################################################################################################################
# Python dependencies
import socketserver
import os
from threading import Thread
from threading import Lock
import traceback
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *



class RSyslogUDPHandler(socketserver.BaseRequestHandler):
	def handle(self):
		data = bytes.decode(self.request[0].strip())
		socket = self.request[1]
		#print( "%s : " % self.client_address[0], str(data))
		rSyslog.log(str(data))

class rSyslog(socketserver.BaseRequestHandler):

	@staticmethod
	def start(p_host, p_port, p_fileBaseName = None, p_rotateNo = 5, p_fileSize = 500000000):
		rSyslog.logLock = Lock()
		rSyslog.stop()
		rSyslog.host = p_host; 
		rSyslog.port = p_port;
		rSyslog.fileBaseName = p_fileBaseName
		rSyslog.rotateNo = p_rotateNo
		rSyslog.fileSize = p_fileSize
		rSyslog.openFileBaseName()
		try:
			rSyslog.server = socketserver.UDPServer((rSyslog.host, rSyslog.port), RSyslogUDPHandler)
			rSyslog.udpPollHandle = Thread(target=rSyslog.server.serve_forever)
			rSyslog.udpPollHandle.start()
		except (IOError, SystemExit):
			raise

	@staticmethod
	def stop():
		try:
			if rSyslog.server != None:
				rSyslog.server.shutdown()
				rSyslog.server.server_close()
		except:
			pass
		rSyslog.server = None
		rSyslog.closeFileBaseName()

	@staticmethod
	def log(p_logStr):
		splitSyslogPrio = p_logStr.split('<', 1)[1].split('>', 1)
		syslogPrio = splitSyslogPrio[0]
		syslogSplit = splitSyslogPrio[1].split()
		syslogHost = syslogSplit[0]
		syslogApp = syslogSplit[1].split(':')[0]
		syslogTimeStampTz = syslogSplit[2].split(':', -1)[0]
		syslogTimeStampDay = syslogSplit[3].split(':', -1)[0]
		syslogTimeStampToDSplit = syslogSplit[4].split(':', -1)
		syslogTimeStampToD = syslogTimeStampToDSplit[0] + ":" + syslogTimeStampToDSplit[1] + ":" + syslogTimeStampToDSplit[2]
		syslogVerbosity = syslogSplit[5].split(':', 1)[0]
		syslogFunction = syslogSplit[6].split(':', 1)[0]
		syslogFile = syslogSplit[7].split(':', 1)[0]
		syslogMsg = ""
		for msgFragment in syslogSplit[8:]:
			syslogMsg += msgFragment + " "
		syslogFormat = syslogTimeStampTz + ":" + syslogTimeStampDay + " " + syslogTimeStampToD + ": " + syslogApp + ":" +syslogHost + " " + syslogVerbosity + " " + syslogFile + " in " + syslogFunction + "(): " + syslogMsg
		with rSyslog.logLock:
			print(syslogFormat)
			if rSyslog.logFileHandle != None:
				rSyslog.logFileHandle.write("%s\n" % (syslogFormat))
				rSyslog.logFileHandle.flush()
		rSyslog.checkRotateFile()

	@staticmethod
	def setHost(p_host, p_port):
		rSyslog.host = p_host
		rSyslog.port = p_port

	@staticmethod
	def setFileBaseName(p_fileBaseName):
		rSyslog.fileBaseName = p_fileBaseName;
		rSyslog.openFileBaseName()

	@staticmethod
	def openFileBaseName(p_dontTakeLock = False):
		if not p_dontTakeLock:
			rSyslog.logLock.acquire()

		if rSyslog.logFileHandle != None:
			rSyslog.logFileHandle.close()
			rSyslog.logFileHandle = None
		try:
			rSyslog.logFileHandle = open(rSyslog.fileBaseName, 'a')
		except (IOError, SystemExit):
			print(">>>>>>>>>>>>Could not open file")
			raise
		if not p_dontTakeLock:
			rSyslog.logLock.release()

	@staticmethod
	def closeFileBaseName():
		with rSyslog.logLock:
			try:
				if rSyslog.logFileHandle != None:
					rSyslog.logFileHandle.close()
			except:
				pass
			rSyslog.logFileHandle = None

	@staticmethod
	def setHouseKeepingPolicy(p_fileSize, p_rotateNo):
		rSyslog.fileSize = p_fileSize
		rSyslog.rotateNo = p_rotateNo

	@staticmethod
	def checkRotateFile():
		if rSyslog.logFileHandle != None:
			if os.path.getsize(rSyslog.fileBaseName) > rSyslog.fileSize:
				rSyslog.rotateFile()

	@staticmethod
	def rotateFile():
		with rSyslog.logLock:
			trace.notify(DEBUG_INFO, "Initiatiating Log-rotation")
			rSyslog.logFileHandle.close()
			rSyslog.logFileHandle = None
			for fileRotateItter in range(rSyslog.rotateNo, -1, -1):
				if fileRotateItter == 0 and rSyslog.rotateNo == 0:
					trace.notify(DEBUG_TERSE, "Logrotation files set to 0, reseting log file without history")
					try:
						os.remove(rSyslog.fileBaseName)
					except(IOError, SystemExit) as err:
						trace.notify(DEBUG_ERROR, "Could not delete log-file: " + rSyslog.fileBaseName + " : Cause: " + err)
				elif fileRotateItter == rSyslog.rotateNo:
					pass
				elif fileRotateItter == 0 and rSyslog.rotateNo > 0:
					trace.notify(DEBUG_VERBOSE, "Renaming file: %s to %s" % (rSyslog.fileBaseName, rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1]))
					try:
						os.replace(rSyslog.fileBaseName, rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1])
						trace.notify(DEBUG_TERSE, "Renamed file %s to %s" % (rSyslog.fileBaseName, rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1]))
					except(IOError, SystemExit) as err:
						trace.notify(DEBUG_ERROR, "Could not rename logBaseFile to logRotateFile; from: %s to: %s: Cause: %s" % (rSyslog.fileBaseName, rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1], err))
				else:
					trace.notify(DEBUG_VERBOSE, "Renaming file %s to %s" % (rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter) + "." + rSyslog.fileBaseName.split(".", 1)[1], rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1]))
					try:
						os.replace(rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter) + "." + rSyslog.fileBaseName.split(".", 1)[1], rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1])
						trace.notify(DEBUG_TERSE,"Renamed file %s to %s" % (rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter) + "." + rSyslog.fileBaseName.split(".", 1)[1], rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1]))
					except(IOError, SystemExit) as err:
						trace.notify(DEBUG_VERBOSE, "Could not rename file to log rotate file-n to log rotate file+n; from: %s to: %s: Cause:" % (rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter) + "." + rSyslog.fileBaseName.split(".", 1)[1], rSyslog.fileBaseName.split(".", 1)[0] + "-" + str(fileRotateItter + 1) + "." + rSyslog.fileBaseName.split(".", 1)[1], err))
			rSyslog.openFileBaseName(True)
			trace.notify(DEBUG_INFO, "Log-rotation finished")
