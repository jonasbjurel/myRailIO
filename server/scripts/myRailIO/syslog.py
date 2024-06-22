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
import SocketServer
import os
from threading import Lock

class RSyslogUDPHandler(SocketServer.BaseRequestHandler):
	def handle(self):
		data = bytes.decode(self.request[0].strip())
		socket = self.request[1]
		print( "%s : " % self.client_address[0], str(data))
		logToFile(str(data))

class rSyslog(SocketServer.BaseRequestHandler):
	rSyslog.server = None
	rSyslog.logFileHandle = None

	@staticmethod
	def start(p_host, p_port, p_fileBaseName = None, p_rotateNo = 5, p_fileSize = 500000000):
		rSyslog.logLock = Lock()
		rSyslog.host = p_host; 
		rSyslog.port = p_port;
		rSyslog.fileBaseName = p_fileBaseName
		rSyslog.rotateNo = p_rotateNo
		rSyslog.fileSize = p_fileSize
		rSyslog.openFileBaseName()
	try:
		rSyslog.server = SocketServer.UDPServer((HOST,PORT), SyslogUDPHandler)
		rSyslog.server.serve_forever(poll_interval=0.5)
	except (IOError, SystemExit):
		raise

	@staticmethod
	def stop():
		if rSyslog.server != None
			rSyslog.server.shutdown()
			rSyslog.server.server_close()
			rSyslog.server = None
		rSyslog.closeFileBaseName()

	@staticmethod
	def log(p_logStr):
		with rSyslog.logLock:
			print(p_logStr)
			if rSyslog.logFileHandle:
				rSyslog.logFileHandle.write(p_logStr)
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
			rSyslog.logFileHandle = open(self.fileBaseName, 'a')
		except (IOError, SystemExit):
			raise
		if not p_dontTakeLock:
			rSyslog.logLock.release()

	@staticmethod
	def closeFileBaseName():
		with rSyslog.logLock:
			if rSyslog.logFileHandle != None:
				rSyslog.logFileHandle.close()
				rSyslog.logFileHandle = None

	@staticmethod
	def setHouseKeepingPolicy(p_fileSize, p_rotateNo):
		rSyslog.fileSize = p_fileSize
		rSyslog.rotateNo = p_rotateNo

	@staticmethod
	def checkRotateFile():
		if rSyslog.logFileHandle:
			if os.path.getsize("fileBaseName") > self.fileSize:
				rotateFile()

	@staticmethod
	def rotateFile():
		with rSyslog.logLock:
			rSyslog.logFileHandle.close()
			rSyslog.logFileHandle = None
			for fileRotateItter in range(rSyslog.rotateNo, 0):
				try:
					if fileRotateItter = 0 and rotateNo > 0:
						os.rename('fileBaseName', 'fileBaseName' + '1')
					elif fileRotateItter = 0 and rotateNo = 0:
						pass
					elif fileRotateItter = rSyslog.rotateNo:
						pass
					else:
						os.rename('fileBaseName' + str(fileRotateItter), 'fileBaseName' + str(fileRotateItter + 1))
				except(IOError, SystemExit):
					raise
			openFileBaseName(True)

class RSyslogView():
	def __init__(p_sourceFilter, p_applicationFilter, p......):

	def filter(p_sourceFilter, p_applicationFilter, p......)

	def filterSource(p_sourceFilter):

	def filterApplication(p_applicationFilter):

	def filterLevel(p_level):

	def tailFiltered(p_nEntries):

	def tailRaw(p_nEntries):

	def onLogRotateCb():

	def onDeleteCb():
