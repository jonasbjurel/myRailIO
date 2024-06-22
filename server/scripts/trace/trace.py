#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A common gen JMRI server trace/log module.
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
# Public API:
# Class: trace - static class
# Purpose:  The trace module provides the visibility and program tracing capabilities needed to understand what is ongoing, needed to debug
#           the program and the overall solution. Depending on log-level set by the setDebugLevel(debugLevel, output = ...) the verbosity of the
#           logs, and the log receivers can be set. Log receivers can be any of stdOut, MQTT or Rsyslog
# Data structures: Almost stateless.
# Methods:
#   start() : None
#       Description: Starts the trace service
#       Parameters: -
#       Returns: None
#
#   setGlobalDebugLevel(int:globalDebugLevel) : None
#           Description: Setting the global debug level valid for all functions provided no special debug level has been set for current running function
#           Parameters: int:globalDebugLevel : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   getGlobalDebugLevel() : int
#       Description: Get current global debug level
#       Parameters: -
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   setFunDebugLevel(obj:fun, int:funDebugLevel) : None
#       Description: Set an overriding debug level for a particular function, kicks in if the fun debug level is lower (more verbose) than the global debug level
#       Parameters: obj:fun - Function object
#                   int funDebugLevel : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: None
#
#   removeFunDebugLevel(obj:fun) : None
#       Description: Removes a function specific debug level
#       Parameters: obj:fun - Function object
#       Returns: None
#
#   getDebugLevel(str:funDescrStr) : int
#       Description: Get current debug level, global or function specific - which ever is the more verbose
#       Parameters: str:funDescrStr - "moduleNme.funName                                                    !!!!!!TODO ClassName should be part of the funDescrStr 
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   getCallStackDebugLevel() : int
#       Description: Get the lowest (least severe) debuglevel for which a call stack is provided
#       Parameters: -
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   setCallStackDebugLevel(int:callStackDebugLevel) : None
#       Description: Set the lowest (least severe) debuglevel for which a call stack is provided
#       Parameters: int:callStackDebugLevel DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: None
#
#   getFileDebugLevel() : int
#       Description: Get the lowest (least severe) debuglevel for which a file reference is provided
#       Parameters: - 
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   setFileDebugLevel(fileDebugLevel) : None
#       Description: Set the lowest (least severe) debuglevel for which a file reference is provided
#       Parameters: int:fileDebugLevel : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: None
#
#   getLineDebugLevel() : int
#       Description: Get the lowest (least severe) debuglevel for which a line reference is provided
#       Parameters: - 
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   setLineDebugLevel(lineDebugLevel) : None
#       Description: Set the lowest (least severe) debuglevel for which a line reference is provided
#       Parameters: int:fileDebugLevel : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: None
#
#   getTerminatingLevel() : int
#       Description: Get the lowest (least severe) debuglevel for which the program is terminated
#       Parameters: - 
#       Returns: int : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#
#   setTerminatingLevel(terminatingLevel) : None
#       Description: Set the lowest (least severe) debuglevel for which the program is terminated
#       Parameters: int:terminatingLevel : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: None
#
#   getSeverityStr(int:severity) : str
#       Description: Provides a severity string corresponding to the severity
#       Parameters: int:severity : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#       Returns: str:severity string
#
#   regNotificationCb(funObj:cb) : None
#       Description: Registers a call back function for debug notifications
#       Parameters: funObj:cb
#       Returns: None
#
#   unRegNotificationCb(funObj:cb) : None
#       Description: Un-registers a call back function for debug notifications
#       Parameters: funObj:cb
#       Returns: None
#
#   notify(int:severity, str:notificationStr) : None
#       Description: Provides a debug notification
#       Parameters: int:severity : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#                   str:notificationStr : A notification string
#
# Callbacks:
# Notification callBack:
#   cb(str:unifiedTime, frameObj:callerFunFrame, int:severity, str:notificationStr, str:defaultFormatNotification, bool:terminate)
#       Description: Provides a debug notification callback
#       Parameters: str:unifiedTime : Local time
#                   frameObj:callerFunFrame : Stack frame from where the notification was called
#                   int:severity : DEBUG_VERBOSE | DEBUG_TERSE | DEBUG_INFO | DEBUG_ERROR | DEBUG_PANIC
#                   str:notificationStr: Notification string
#                   str:defaultFormatNotification : A default formatted notification string
#                   bool:terminate : Indicates if the notification will cause program termination
#################################################################################################################################################



#################################################################################################################################################
# Module/Library dependance
#################################################################################################################################################
import os
import sys
import inspect
import time
from datetime import datetime
import threading
import traceback
from socket import *



# ==============================================================================================================================================
# Constants
# ==============================================================================================================================================
# ----------------------------------------------------------------------------------------------------------------------------------------------
# System constants
# ----------------------------------------------------------------------------------------------------------------------------------------------

# Debug levels
# ------------
DEBUG_VERBOSE = 0
DEBUG_TERSE =   1
DEBUG_INFO =    2
DEBUG_ERROR =   3
DEBUG_PANIC =   4

DEBUG_STR = [""]*5
DEBUG_STR[0] =   "DEBUG-VERBOSE"
DEBUG_STR[1] =   "DEBUG-TERSE"
DEBUG_STR[2] =   "DEBUG-INFO"
DEBUG_STR[3] =   "DEBUG-ERROR"
DEBUG_STR[4] =   "DEBUG-PANIC"



#################################################################################################################################################
# Class: trace, see header description of this file
#################################################################################################################################################
class trace:
    @staticmethod
    def start():
        trace.lock = threading.Lock()
        trace.globalDebugLevel = DEBUG_VERBOSE
        trace.fileDebugLevel = DEBUG_ERROR
        trace.lineDebugLevel = DEBUG_ERROR
        trace.callStackDebugLevel = DEBUG_PANIC
        trace.terminatingLevel = DEBUG_PANIC
        trace.funDebugLevel = {}
        trace.notifyCb = []
        trace.RSyslog = False

    @staticmethod
    def setGlobalDebugLevel(globalDebugLevel):
        trace.globalDebugLevel = globalDebugLevel

    @staticmethod
    def getGlobalDebugLevel():
        return trace.globalDebugLevel

    @staticmethod
    def setFunDebugLevel(fun, funDebugLevel):
        trace.funDebugLevel[fun.__module__ + "." + fun.__name__] = funDebugLevel # !!!!!!TODO ClassName should be part of the funDescrStr 

    @staticmethod
    def removeFunDebugLevel(fun):
        trace.funDebugLevel.remove(fun)

    @staticmethod
    def getDebugLevel(funDescStr):
        try:
            return (trace.globalDebugLevel if trace.globalDebugLevel < trace.funDebugLevel[funDescStr] else trace.funDebugLevel[funDescStr])
        except:
            return trace.globalDebugLevel

    @staticmethod
    def getCallStackDebugLevel():
        return trace.callStackDebugLevel

    @staticmethod
    def setCallStackDebugLevel(callStackDebugLevel):
        trace.callStackDebugLevel = callStackDebugLevel

    @staticmethod
    def getFileDebugLevel():
        return trace.fileDebugLevel

    @staticmethod
    def setFileDebugLevel(fileDebugLevel):
        trace.fileDebugLevel = fileDebugLevel

    @staticmethod
    def getLineDebugLevel():
        return trace.lineDebugLevel

    @staticmethod
    def setLineDebugLevel(lineDebugLevel):
        trace.lineDebugLevel = lineDebugLevel
    
    @staticmethod
    def getTerminatingLevel():
        return trace.terminatingLevel

    @staticmethod
    def setTerminatingLevel(terminatingLevel):
        trace.terminatingLevel = terminatingLevel

    @staticmethod
    def getSeverityStr(severity):
        try:
            return DEBUG_STR[severity]
        except:
            return None

    @staticmethod
    def getSeverityFromSeverityStr(severityStr):
        for severity in range(len(DEBUG_STR)):
            if DEBUG_STR[severity] == severityStr:
                return severity
        return None

    @staticmethod
    def regNotificationCb(cb):
        try:
            trace.notifyCb.index(cb)
        except:
            trace.notifyCb.append(cb)

    @staticmethod
    def unRegNotificationCb(cb):
        try:
            trace.notifyCb.remove(cb)
        except:
            pass

    @staticmethod
    def startSyslog(p_rSyslogAddr, p_rSyslogPort):
        trace.rSysLogSocket = socket(AF_INET, SOCK_DGRAM)
        trace.rSysLogSocket.settimeout(1)
        trace.rSysLogAddr = (p_rSyslogAddr, p_rSyslogPort)
        try:
            trace.rSysLogSocket.connect(('10.0.0.0', 0))
            if trace.rSysLogSocket.getsockname()[0] == p_rSyslogAddr:
                trace.localRSyslog = True
            else:
                trace.localRSyslog = False
            trace.RSyslog = True 
        except:
            trace.localRSyslog = False
            trace.RSyslog = False
            return

    @staticmethod
    def stopSyslog():
        trace.RSyslog = False

    @staticmethod
    def notify(severity, notificationStr, guiPopUp=False):
        unifiedTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        callerFunFrame = sys._getframe(1)

        if severity == DEBUG_VERBOSE:
            if trace.getDebugLevel(inspect.currentframe().f_back.f_globals["__name__"] + "." + inspect.currentframe().f_back.f_code.co_name) <= DEBUG_VERBOSE:
                trace.__deliverNotification(unifiedTime, inspect.currentframe().f_back, severity, notificationStr)
            return

        if severity == DEBUG_TERSE:
            if trace.getDebugLevel(inspect.currentframe().f_back.f_globals["__name__"] + "." + inspect.currentframe().f_back.f_code.co_name) <= DEBUG_TERSE:
                trace.__deliverNotification(unifiedTime, inspect.currentframe().f_back, severity, notificationStr)
            return

        if severity == DEBUG_INFO:
            if trace.getDebugLevel(inspect.currentframe().f_back.f_globals["__name__"] + "." + inspect.currentframe().f_back.f_code.co_name) <= DEBUG_INFO:
                trace.__deliverNotification(unifiedTime, inspect.currentframe().f_back, severity, notificationStr)
            return

        if severity == DEBUG_ERROR:
            if trace.getDebugLevel(inspect.currentframe().f_back.f_globals["__name__"] + "." + inspect.currentframe().f_back.f_code.co_name) <= DEBUG_ERROR:
                trace.__deliverNotification(unifiedTime, inspect.currentframe().f_back, severity, notificationStr)
            return

        if severity == DEBUG_PANIC:
            if trace.getDebugLevel(inspect.currentframe().f_back.f_globals["__name__"] + "." + inspect.currentframe().f_back.f_code.co_name) <= DEBUG_PANIC:
                trace.__deliverNotification(unifiedTime, inspect.currentframe().f_back,severity, notificationStr)
            return

    @staticmethod
    def __deliverNotification(unifiedTime, callerFunFrame, severity, notification):
        with trace.lock:
            defaultSysLogFormatNotification = "<" + str(trace.getSyslogPrio(severity)) + ">" + "TopDecoder myRailIOServer: " + str(unifiedTime) + ": " + str(trace.getSeverityStr(severity)).split("-")[1] + ": " + callerFunFrame.f_code.co_name + ": " + callerFunFrame.f_code.co_filename.rpartition("\\")[-1] + "-" + str(callerFunFrame.f_lineno) + ": " + notification + (" , TERMINATING ..." if severity >= trace.getTerminatingLevel() else "")
            defaultConsoleFormatNotification = str(unifiedTime) + ": TopDecoder:myRailIOServer " + str(trace.getSeverityStr(severity)).split("-")[1] + ": " + callerFunFrame.f_code.co_filename.rpartition("\\")[-1] + "-" + str(callerFunFrame.f_lineno) + " in " + callerFunFrame.f_code.co_name + "(): " + notification + (" , TERMINATING ..." if severity >= trace.getTerminatingLevel() else "")
            for cb in trace.notifyCb:
                cb(unifiedTime, callerFunFrame, severity, notification, defaultConsoleFormatNotification, (True if severity >= trace.getTerminatingLevel() else False))
            if trace.RSyslog and trace.localRSyslog:
                try:
                    trace.rSysLogSocket.sendto(defaultSysLogFormatNotification.encode(), trace.rSysLogAddr)
                except:
                    print("Could not send log entry to Local RSyslog server")
                    traceback.print_exc()
            elif trace.RSyslog:
                print(defaultConsoleFormatNotification)
                try:
                    trace.rSysLogSocket.sendto(defaultSysLogFormatNotification.encode(), trace.rSysLogAddr)
                except:
                    print("Could not send log entry to Remote RSyslog server")
            else:
                print(defaultConsoleFormatNotification)
            if severity >= trace.getCallStackDebugLevel():
                print("Traceback, last call last: ")
                stackTrace = []
                for stack in trace.__getCallStack():
                    stackTrace.append(stack)
                for stackLevel in range(len(stackTrace) - 3):
                    print(stackTrace[stackLevel])
            if severity >= trace.getTerminatingLevel():
                trace.__terminate(unifiedTime)

    @staticmethod
    def getSyslogPrio(severity):
        PRI_EMERGENCY = 0
        PRI_ALERT =     1
        PRI_CRITICAL =  2
        PRI_ERROR =     3
        PRI_WARNING =   4
        PRI_NOTICE =    5
        PRI_INFO =      6
        PRI_DEBUG =     7
        FAC_USER =      1
        FAC_LOCAL0 =    16
        FAC_LOCAL1 =    17
        FAC_LOCAL2 =    18
        FAC_LOCAL3 =    19
        FAC_LOCAL4 =    20
        FAC_LOCAL5 =    21
        FAC_LOCAL6 =    22
        FAC_LOCAL7 =    23
        if severity == DEBUG_VERBOSE:
            return (8 * FAC_USER) + PRI_DEBUG
        elif severity == DEBUG_TERSE:
            return (8 * FAC_USER) + PRI_INFO
        elif severity == DEBUG_INFO:
            return (8 * FAC_USER) + PRI_NOTICE
        elif severity == DEBUG_ERROR:
            return (8 * FAC_USER) + PRI_ERROR
        elif severity == DEBUG_PANIC:
            return (8 * FAC_USER) + PRI_EMERGENCY
        else:
            assert False, "Non existant Log severity provided"

    @staticmethod
    def __getCallStack():
        return traceback.format_stack()

    @staticmethod
    def __terminate(unifiedTime):
        print(str(unifiedTime) + " - Terminating in 1 second...")
        time.sleep(1)
        sys.exit()

# END < Class: trace> ---------------------------------------------------------------------------------------------------------------------------
