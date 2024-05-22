#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A JMRI client-side module used to configure and monitor JMRI using XML-RPC.
# It provides methods for monitor and control of objects such as JMRI masts, turnouts, lights, sensors, and memories. Callback capabilities
# triggered by changes of these objects and capabilities to configure real-time MQTT publications at state change of these objects, emitted
# on the server side - to avoid RPC latency
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
# Public API:
# Class: jmriRpcClient - static class
#   Methods:
#       __init__(str:uri="localhost", int:portBase = 8000, fun:errCb=None, int:keepAlive=DEFAULT_KEEPALIVE_INTERVAL): Handle
#           Description: Starts the jmriRPC client providing RPC calls to a jmriRPC server.
#           Parameters:  str:uri: URI of the JMRI RPC server
#                        int:portBase: Port base of the JMRI RPC server, several ports are used starting from portBase
#                        fun:errCb: Callback in case of RPC client errors
#                        int:keepAlive: Keep-alive intervale RPC callback procedure
#
#       stop(): returns genJMRIDecoderRc.x
#           Description: Stops the jmriRPC client, all registered callbacks and MQTT pub registrations are canceled/unregistered.
#
#       regEventCb(type, sysName, cb): returns genJMRIRc.rc
#           Description: Registers a client call-back function for JMRI object state changes
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        fun:cb: Call-back function reference
#
#       reRegEventCbs(): returns genJMRIRc.rc
#           Description: Re-registers previously registered events
#           Parameters: -
#
#       unRegEventCb(type, sysName, cb, allCb=False): returns genJMRIRc.rc
#           Description: Un Registers a client call-back function for JMRI object state changes
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        fun:cb: Call-back function reference
#                        allCb: if set to true - all duplicate callback references will be removed
#
#       regMqttStatusEventCb(cb): returns genJMRIRc.rc
#           Description: Registers a callback for RPC server MQTT status
#           Parameters:  callback
#
#       reRegMqttStatusEventCb(): returns genJMRIRc.rc
#           Description: Re-registers previously registered MQTT status callbacks
#           Parameters: -
#
#       unRegMqttStatusEventCb(cb, allCb=False): returns genJMRIRc.rc
#           Description: Un Registers a client call-back function for MQTT status callbacks
#           Parameters:  fun:cb: Call-back function reference
#                        allCb: if set to true - all duplicate callback references will be removed
#
#       regMqttPub(type, sysName, topic, payloadMap): returns genJMRIRc.rc
#           Description: Registers an RPC server side MQTT publish emmit event at a JMRI object state change - this avoids any latenceies involved with RPC
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        str:topic: postpended topic to JMRI MQTT defined prepended topic Eg "track/light/")
#                        dict:payloadMap: A dictionary mapping the event value to a published payload - e.g. {"ON":"<"+str(sysName)+">"+"TurnOnMyLight"+"</"+str(sysName)+">","OFF:....
#                                         if any key matches "*", the event payload will transparently be copied from the event value. eg: {"*":"*"}.
#
#       unRegMqttPub(type, sysName): returns genJMRIRc.rc
#           Description: Un-registers an RPC server side MQTT publish emmit event
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       regMqttSub(type, sysName, topic, payloadMap): returns genJMRIRc.rc
#           Description: Registers an RPC server side MQTT subscription event, which when triggered will set the JMRI object state according to the given payload map -
#           this avoids any latenceies involved with RPC
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        str:topic: postpended topic to JMRI MQTT defined prepended topic Eg "track/light/")
#                        dict:payloadMap: A dictionary mapping the event value to a published payload - e.g. {"ON":"<"+str(sysName)+">"+"TurnOnMyLight"+"</"+str(sysName)+">","OFF:....
#                                         if any key matches "*", the MQTT payload will transparently be copied to the JMRI object state. eg: {"*":"*"}.
#
#       unRegMqttSub(type, sysName): returns genJMRIRc.rc
#           Description: Un-registers an RPC server side MQTT publish emmit event
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       createObject(type, sysName): returns genJMRIRc.rc
#           Description: Creates a JMRI object of type "type"
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       getObjectConfig(type, sysName): returns dict: {"type":type,"usrName":userName","comment":comment}
#           Description: Retreives a JMRI object's configuration
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       canDeleteObject(type, sysName): returns genJMRIRc.rc
#           Description: Checks if a JMRI object can be deleted
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       deleteObject(type, sysName): returns genJMRIRc.rc
#           Description: Delete a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       getConfigsByType(type):returns dict: {"sysName:{"type":type,"usrName":userName","comment":comment}, "sysName":....
#           Description: Get the configuration for all JMRI objects of a certain type
#           Parameters:  type: JMRI object type: genJMRIObj.x
#
#       getUserNameBySysName(type, sysName): returns str: userName
#           Description: Gets the userName of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       setUserNameBySysName(type, sysName, usrName): returns genJMRIRc.rc
#           Description: Sets the userName of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        str:usrName: JMRI object user name
#
#       getCommentBySysName(type, sysName):: returns str: comment
#           Description: Gets the comment of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       setCommentBySysName(type, sysName, comment): returns genJMRIRc.rc
#           Description: Sets the comment of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        str:comment: JMRI object user name
#
#       getStateBySysName(type, sysName): returns genJMRIObj.x
#           Description: Sets the current state of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#
#       getValidStatesBySysName(type, sysName): returns [str state, str:state,....]
#           Description: Get all valid states for a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name#
#
#       setStateBySysName(type, sysName, state): returns genJMRIRc.rc
#           Description: Sets the state of a JMRI object
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        str:state: JMRI object state
#
#       setRpcServerDebugLevel(GlobalDebugLevelStr): returns genJMRIRc.rc
#           Description: Sets current global debug level
#           Parameters: str:GlobalDebugLevelStr Global debug level string
#
#       getKeepaliveInterval(): returns int: keep alive interval (s)
#           Description: get current keep-alive interval (s)
#           Parameters: -
#
#       setKeepaliveInterval(keepaliveInterval): returns genJMRIRc.rc
#           Description: set current keep-alive interval (s)
#           Parameters: int: keepaliveInterval keep alive interval
#
#       getFile(fileName): returns fileContent or None
#           Description: get file content on RPC server side
#           Parameters: str: filepath\filename
#
# Callbacks:
#       errcb(int:ErrNo): returns None
#           Description: Called when a RPC client error has occured, callback is registered by jmriRpcClient.start
#           Parameters:  int:errNo - genJMRIRc.rc
#
#       eventCb(cbEvent): returns None
#           Description: Called when a JMRI object state change has occurred, callback is registered by jmriRpcClient.regEventCb
#           Parameters:  cbEvent
#
# Important data structures:
#       cbEvent:
#           cbEvent.obj: Source Cb JMRI object
#           cbEvent.type: genJMRIObj.jmriObj.OBJ_STR: JMRI Object type
#           cbEvent.sysName: str: JMRI Object system name
#           cbEvent.usrName: str: JMRI Object user name
#           cbEvent.oldState: str: Old JMRI object state
#           cbEvent.newState: str: New JMRI object state
#
#       Return codes:
#           See genJMRIRc.rc
#
#       jmriObjects:
#        - see genJMRIObj.jmriObj



#################################################################################################################################################
# Module/Library dependance
#################################################################################################################################################

import os
import sys
import time
#import copy
sys.path.append(os.path.realpath('..'))
import traceback
#import keyboard
import xmlrpc.client
import threading
#import time
import xmltodict
import imp
imp.load_source('config', '..\\genJMRI\\config.py')
from config import *
imp.load_source('jmriObj', '..\\rpc\\JMRIObjects.py')
from jmriObj import jmriObj
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('dictEscapeing', '..\\rpc\\dictEscapeing.py')
from dictEscapeing import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc
# END <Module/Library dependance> ---------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Parameters
#################################################################################################################################################
MAX_MISSED_KEEPALIVE = 3
# END <Parameters> ------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Helper class and fuctions
#################################################################################################################################################
class cbEvent():
    obj = None
    type = ""
    sysName = ""
    usrName = ""
    oldState = ""
    newState = ""

def isKeepAlive(checkDict):
    if "keepAlive" in checkDict: return True
    else: return False

def isRelease(checkDict):
    if "release" in checkDict: return True
    else: return False

def isStateChange(checkDict):
    if "stateChange" in checkDict: return True
    else: return False

def getCallbackType(checkDict):
    if not isStateChange(checkDict):
        return None
    try:
        return checkDict.get("stateChange").get("objType")
    except:
        return None

def getCallBackSysName(checkDict):
    if not isStateChange(checkDict):
        return None
    try:
        return checkDict.get("stateChange").get("sysName")
    except:
        return None

def getCallBackUsrName(checkDict):
    if not isStateChange(checkDict):
        return None
    try:
        return checkDict.get("stateChange").get("usrName")
    except:
        return None

def getCallBackOldState(checkDict):
    if not isStateChange(checkDict):
        return None
    try:
        return checkDict.get("stateChange").get("oldState")
    except:
        return None

def getCallBackNewState(checkDict):
    if not isStateChange(checkDict):
        return None
    try:
        return checkDict.get("stateChange").get("newState")
    except:
        return 

def isMqttStateChange(checkDict):
    if "mqttstateChange" in checkDict: return True
    else: return False

def getMQTTCallbackState(checkDict):
    if not isMqttStateChange(checkDict):
        return None
    try:
        return checkDict.get("mqttstateChange").get("state")
    except:
        return None
# END <Helper class and fuctions> ---------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: jmriRpcClient, see header description of this file
# Description: Provides all public proxy RPC methods provided by the library
#################################################################################################################################################
RPC_CONNECTED =                             0
RPC_CONNECTING =                            1
RPC_CONNECTED_NO_KEEPALIVE =                3
RPC_NOT_CONNECTED =                         4
RPC_NOT_ACTIVE =                            5

class jmriObjRecord():
    type : str | None = None
    sysName : str | None = None
    userName : str | None = None
    description : str | None = None
    state : str | None = None

class mqttPubRecord():
    type : str | None = None
    sysName : str | None = None
    topic : str | None = None
    payloadMap : str | None = None

class mqttSubRecord():
    type : str | None = None
    sysName : str | None = None
    topic : str | None = None
    payloadMap : str | None = None

class jmriRpcClient():
    def __init__(self):
        pass

    def start(self, uri="localhost", portBase = 8000, errCb = None, keepAliveInterval = DEFAULT_JMRI_RPC_KEEPALIVE_PERIOD):
        trace.notify(DEBUG_INFO, "Starting RPC client - remote end-points: " + uri + ":" + str(portBase) + " and "  + uri + ":" + str(portBase+1))
        self.retry = False
        self.connected = False
        self.retryStateMachine = RPC_CONNECTED
        self.uri = uri
        self.portBase = portBase
        self.keepAliveInterval = keepAliveInterval
        self.rpc = xmlrpc.client.ServerProxy("http://" + self.uri + ":" + str(portBase) + "/")
        self.cbRpc = xmlrpc.client.ServerProxy("http://" + self.uri + ":" + str(portBase + 1) + "/")
        self.regCbs = {}
        self.errCb = errCb
        self.mqttPubRecordDict = {}
        self.mqttSubRecordDict = {}
        self.jmriObjDict = {}
        self.cbLoopRunning = False
        self.runCbLoop = True
        self.jmriRpcCbThread = threading.Thread(target=self.cbGenerator)
        self.jmriRpcCbThread.start()
        if self.errCb != None:
            self.errCb(rc.OK)
        self.setRpcServerDebugLevel(DEFAULT_LOG_VERBOSITY)
        self.missedKeepAlive = 0
        self.setKeepaliveInterval(self.keepAliveInterval)
        self.startKeepAliveHandler()
        trace.notify(DEBUG_INFO, "RPC client started")
         
    def stop(self):
        trace.notify(DEBUG_TERSE, "Stopping RPC client")
        for typeStr in self.regCbs.copy():
            for sysName in self.regCbs.get(typeStr).copy():
                self.unRegEventCb(jmriObj.getGenJMRITypeFromJMRIType(typeStr), sysName, None, allCb=True)
        mqttPubRecordDictItter = dict(self.mqttPubRecordDict)
        for sysName in mqttPubRecordDictItter:
            if self.unRegMqttPub(self.mqttPubRecordDict[sysName], sysName) != rc.OK:
                trace.notify(DEBUG_ERROR, "Failed to unregister MQTT pub event for " + jmriObj.getObjTypeStr(self.mqttPubRecordDict[sysName]) + ":" + sysName)
        del mqttPubRecordDictItter 
        if self.mqttPubRecordDict != {}:
            trace.notify(DEBUG_ERROR, "Failed to unregister all MQTT pub events when stopping RPC client - following MQTT reccords remain: " + str(self.mqttPubRecordDict))
        mqttSubRecordDictItter = dict(self.mqttSubRecordDict)
        for sysName in mqttSubRecordDictItter:
            if self.unRegMqttSub(self.mqttSubRecordDict[sysName], sysName) != rc.OK:
                trace.notify(DEBUG_ERROR, "Failed to unregister MQTT pub event for " + jmriObj.getObjTypeStr(self.mqttSubRecordDict[sysName]) + ":" + sysName)
        del mqttSubRecordDictItter
        if self.mqttSubRecordDict != {}:
            trace.notify(DEBUG_ERROR, "Failed to unregister all MQTT sub events when stopping RPC client - following MQTT reccords remain: " + str(self.mqttSubRecordDict))
        self.stopKeepAliveHandler()
        self.runCbLoop = False
        self.rpc.rpcOnStateChangeRelease()
        self.jmriRpcCbThread.join(timeout=1)
        self.rpc.rpcOnStateChangePurge()
        del self.rpc
        del self.cbRpc
        if self.jmriRpcCbThread.is_alive():
            trace.notify(DEBUG_ERROR, "Failed to stop RPC client - " + rc.getErrStr(rc.GEN_ERR))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_INFO, "RPC client stoped")
            return rc.OK

    def setRpcUri(self, uri):
        self.uri = uri
        self._reConnect(rc.GEN_COM_ERR)

    def getRpcUri(self):
        return self.uri

    def setRpcPortBase(self, portBase):
        self.portBase = portBase
        self._reConnect(rc.GEN_COM_ERR)

    def getRpcPortBase(self):
        return self.portBase

    def _tryReconnect(self):
        trace.notify(DEBUG_INFO, "Reconnecting to RPC server")
        self.stopKeepAliveHandler()
        if self.cbLoopRunning == True:
            try:
                self.rpc.rpcOnStateChangePurge()
                self.rpc.rpcOnStateChangeRelease()
                self.runCbLoop = False
                time.sleep(5)
            except:
                pass
            try:
                self.jmriRpcCbThread.join(timeout=1)
            except:
                pass
            if self.jmriRpcCbThread.is_alive():
                trace.notify(DEBUG_ERROR, "Failed to stop RPC client - " + rc.getErrStr(rc.GEN_ERR))
                return rc.GEN_ERR
        trace.notify(DEBUG_INFO, "RPC client stoped")
        try:
            self.rpc = xmlrpc.client.ServerProxy("http://" + self.uri + ":" + str(self.portBase) + "/")
            self.cbRpc = xmlrpc.client.ServerProxy("http://" + self.uri + ":" + str(self.portBase + 1) + "/")
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Failed to start RPC client - " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Failed to start RPC client - " + str(err))
            return rc.GEN_ERR
        if self.setRpcServerDebugLevel(self.globalDebugLevelStr) != rc.OK:
            trace.notify(DEBUG_ERROR, "Failed to set debug level")
            return rc.GEN_COM_ERR
        self.missedKeepAlive = 0
        if self.setKeepaliveInterval(self.keepAliveInterval) != rc.OK:
            trace.notify(DEBUG_ERROR, "Failed to set keepAlive interval")
            return rc.GEN_COM_ERR
        self.startKeepAliveHandler()
        self.rpc.rpcOnStateChangePurge()

        self.runCbLoop = True
        self.jmriRpcCbThread = threading.Thread(target=self.cbGenerator)
        self.jmriRpcCbThread.start()
        trace.notify(DEBUG_INFO, "RPC client restarted, re-populating JMRI objects, MQTT subscriptions- and publication requests as well as RPC call-backs")
        if self.reCreateObjects() != rc.OK:
            trace.notify(DEBUG_ERROR, "RPC client failed to re-populating JMRI objects...")
            return rc.GEN_COM_ERR
        if self.reRegMqttPubs() != rc.OK:
            trace.notify(DEBUG_ERROR, "RPC client failed to re-registrate MQTT publications...")
            return rc.GEN_COM_ERR
        if self.reRegMqttSubs() != rc.OK:
            rc.notify(DEBUG_ERROR, "RPC client failed to re-registrate MQTT subscriptions...")
            return rc.GEN_COM_ERR
        if self.reRegEventCbs() != rc.OK:
            trace.notify(DEBUG_ERROR, "RPC client failed to re-register JMRI state RPC call-backs...")
            return rc.GEN_COM_ERR
        if self.reRegMqttStatusEventCb() != rc.OK:
            trace.notify(DEBUG_ERROR, "RPC client failed to re-register JMRI MQTT state RPC call-backs...")
            return rc.GEN_COM_ERR
        trace.notify(DEBUG_INFO, "RPC client has successfully connected...")
        return rc.OK

    def regEventCb(self, type, sysName, cb):
        trace.notify(DEBUG_INFO, "Registering RPC client callback: " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        if self.regCbs.get(jmriObj.getObjTypeStr(type)) == None:
            self.regCbs[jmriObj.getObjTypeStr(type)] = {sysName:[cb]}
            try:
                res = self.rpc.rpcListen(type, sysName)
            except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + err.errmsg)
                return rc.GEN_COM_ERR
            except Exception as err:
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + str(err))
                return rc.GEN_ERR
            if res != rc.OK:
                self.unRegEventCb(type, sysName, cb)
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + rc.getErrStr(res))
                return res
        elif self.regCbs[jmriObj.getObjTypeStr(type)].get(sysName) == None:
            self.regCbs[jmriObj.getObjTypeStr(type)][sysName] = [cb]
            try:
                res = self.rpc.rpcListen(type, sysName)
            except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + err.errmsg)
                return rc.GEN_COM_ERR
            except Exception as err:
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + str(err))
                return rc.GEN_ERR
            if res != rc.OK:
                self.unRegEventCb(type, sysName, cb)
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - rc: " + rc.getErrStr(res))
                return res
        else:
            self.regCbs[jmriObj.getObjTypeStr(type)][sysName].append(cb)
        trace.notify(DEBUG_INFO, "RPC client callback successfully registered : " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def reRegEventCbs(self):
        trace.notify(DEBUG_TERSE, "Re-registering RPC client callbacks")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        for rpcCbTypeItter in self.regCbs:
            if rpcCbTypeItter == "mqttStateChange":                 # Not so nice, but the easiest way out
                continue
            for rpcCbSysNameItter in self.regCbs[rpcCbTypeItter]:
                try:
                    self.rpc.rpcUnListen(jmriObj.getGenJMRITypeFromJMRIType(rpcCbTypeItter), rpcCbSysNameItter)
                except:
                    pass
                try:
                    res = self.rpc.rpcListen(jmriObj.getGenJMRITypeFromJMRIType(rpcCbTypeItter), rpcCbSysNameItter)
                except xmlrpc.client.ProtocolError as err:
                    trace.notify(DEBUG_ERROR, "Re-registering RPC client callback failed - rc: " + err.errmsg)
                    return rc.GEN_COM_ERR
                except Exception as err:
                    trace.notify(DEBUG_ERROR, "Re-registering RPC client callback failed - rc: " + str(err))
                    return rc.GEN_ERR
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Re-registering RPC client callbacks failed - rc: " + rc.getErrStr(res))
                    return res
        trace.notify(DEBUG_INFO, "Successfully Re-registered RPC client callbacks")
        return rc.OK

    def unRegEventCb(self, type, sysName, cb, allCb=False):
        if allCb:
            trace.notify(DEBUG_TERSE, "Un-Registering ALL RPC client callbacks: " + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        else:
            trace.notify(DEBUG_TERSE, "Un-Registering RPC client callback: " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            if allCb:
                self.regCbs[jmriObj.getObjTypeStr(type)][sysName] = []
            else:
                self.regCbs[jmriObj.getObjTypeStr(type)][sysName].remove(cb)
        except:
            trace.notify(DEBUG_INFO, "Un-Registering RPC client callback: " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", All Callbacks = " + str(allCb) + " - failed - " + rc.getErrStr(rc.DOES_NOT_EXIST))
            return rc.DOES_NOT_EXIST
        try:
            if self.regCbs[jmriObj.getObjTypeStr(type)][sysName] == []:
                self.rpc.rpcUnListen(type, sysName)
                self.regCbs[jmriObj.getObjTypeStr(type)].pop(sysName)
                if self.regCbs.get(jmriObj.getObjTypeStr(type)) == {}:
                    self.regCbs.pop(jmriObj.getObjTypeStr(type))
        except:
            pass
        return rc.OK

    def regMqttStatusEventCb(self, cb):
        trace.notify(DEBUG_INFO, "Registering MQTT status RPC client callback")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR

        if self.regCbs.get("mqttStateChange") == None:
            self.regCbs["mqttStateChange"] = [cb]
        else:
            self.regCbs["mqttStateChange"].append(cb)
        try:
            res = self.rpc.rpcMqttStateChangeListen()
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Registering MQTT status RPC client callback failed - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Registering MQTT status RPC client callback failed - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            self.unRegMqttStatusEventCb(cb)
            trace.notify(DEBUG_ERROR, "Registering MQTT status RPC client callback failed - rc: " + rc.getErrStr(res))
            return res

        trace.notify(DEBUG_INFO, "MQTT status RPC client callback successfully registered : " + str(cb.__name__))
        return rc.OK

    def reRegMqttStatusEventCb(self):
        trace.notify(DEBUG_TERSE, "Re-registering MQTT status RPC client callback")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.rpc.rpcMqttStateChangeUnListen()
        except:
            pass
        try:
           if self.regCbs["mqttStateChange"]:
               self.rpc.rpcMqttStateChangeListen()
        except:
            return rc.GEN_COM_ERR
        return rc.OK

    def unRegMqttStatusEventCb(self, cb, allCb=False):
        if allCb:
            trace.notify(DEBUG_TERSE, "Un-Registering ALL MQTT status RPC client callbacks")
        else:
            trace.notify(DEBUG_TERSE, "Un-Registering MQTT status RPC client callback: " + str(cb.__name__))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            if allCb:
                self.regCbs["mqttStateChange"] = []
            else:
                self.regCbs["mqttStateChange"].remove(cb)
        except:
            trace.notify(DEBUG_INFO, "Un-Registering RPC client callback: " + str(cb.__name__) + " - failed - " + rc.getErrStr(rc.DOES_NOT_EXIST))
            return rc.DOES_NOT_EXIST
        return rc.OK

    def regMqttPub(self, type, sysName, topic, payloadMap):
        trace.notify(DEBUG_INFO, "Registering MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.mqttPubRecordDict[sysName]
        except:
            pass
        else:
            trace.notify(DEBUG_INFO, "MQTT pub event already registered for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
            self.unRegMqttPub(type, sysName)
        try:
            res = self.rpc.rpcRegMqttPub(type, sysName, topic, payloadMap)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "MQTT pub event Registration failed - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "MQTT pub event Registration failed - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not register MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        self.mqttPubRecordDict[sysName] = mqttPubRecord()
        self.mqttPubRecordDict[sysName].type = type
        self.mqttPubRecordDict[sysName].sysName = sysName
        self.mqttPubRecordDict[sysName].topic = topic
        self.mqttPubRecordDict[sysName].payloadMap = payloadMap
        trace.notify(DEBUG_INFO, "MQTT pub event succesfully registered for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        return rc.OK

    def reRegMqttPubs(self):
        trace.notify(DEBUG_TERSE, "Re-registering RPC pub events")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        for sysNameItter in self.mqttPubRecordDict:
            try:
                self.rpc.rpcUnRegMqttPub(self.mqttPubRecordDict[sysNameItter].type, self.mqttPubRecordDict[sysNameItter].sysName)
            except:
                pass
            try:
                res = self.rpc.rpcRegMqttPub(self.mqttPubRecordDict[sysNameItter].type, self.mqttPubRecordDict[sysNameItter].sysName, self.mqttPubRecordDict[sysNameItter].topic, self.mqttPubRecordDict[sysNameItter].payloadMap)
            except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "MQTT pub event Re-registration failed - rc: " + err.errmsg)
                return rc.GEN_COM_ERR
            except Exception as err:
                trace.notify(DEBUG_ERROR, "MQTT pub event Re-registration failed - rc: " + str(err))
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "Re-registering RPC client callbacks failed - rc: " + rc.getErrStr(res))
                return res
        trace.notify(DEBUG_INFO, "Successfully Re-registered MQTT pubs over RPC")
        return rc.OK

    def unRegMqttPub(self, type, sysName):
        trace.notify(DEBUG_TERSE, "Un-Register MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.mqttPubRecordDict[sysName]
        except:
            trace.notify(DEBUG_INFO, "Cannot unregister MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - record of registration does not exist")
            return rc.DOES_NOT_EXIST
        try:
            res = self.rpc.rpcUnRegMqttPub(type, sysName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not Unregister MQTT publishment - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not Unregister MQTT publishment - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not Unregister MQTT publishment: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        else:
            del self.mqttPubRecordDict[sysName]
        trace.notify(DEBUG_INFO, "Successfully Un-registered MQTT pubs over RPC")
        return rc.OK

    def regMqttSub(self, type, sysName, topic, payloadMap):
        trace.notify(DEBUG_TERSE, "Registering MQTT subscription event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.mqttSubRecordDict[sysName]
        except:
            pass
        else:
            trace.notify(DEBUG_INFO, "MQTT sub event already registered for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
            self.unRegMqttSub(type, sysName)
        try:
            res = self.rpc.rpcRegMqttSub(type, sysName, topic, payloadMap)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not register MQTT sub event - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not register MQTT sub event - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not register MQTT sub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        self.mqttSubRecordDict[sysName] = mqttPubRecord()
        self.mqttSubRecordDict[sysName].type = type
        self.mqttSubRecordDict[sysName].sysName = sysName
        self.mqttSubRecordDict[sysName].topic = topic
        self.mqttSubRecordDict[sysName].payloadMap = payloadMap
        trace.notify(DEBUG_INFO, "Successfully registered MQTT subscription event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        return rc.OK

    def reRegMqttSubs(self):
        trace.notify(DEBUG_TERSE, "Re-registering MQTT subscription events")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        for sysNameItter in self.mqttSubRecordDict:
            try:
                self .rpc.rpcUnRegMqttSub(self.mqttSubRecordDict[sysNameItter].type, self.mqttSubRecordDict[sysNameItter].sysName)
            except:
                pass
            try:
                res = self.rpc.rpcRegMqttSub(self.mqttSubRecordDict[sysNameItter].type, self.mqttSubRecordDict[sysNameItter].sysName, self.mqttSubRecordDict[sysNameItter].topic, self.mqttSubRecordDict[sysNameItter].payloadMap)
            except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "Could not Re-register MQTT sub event - rc: " + err.errmsg)
                return rc.GEN_COM_ERR 
            except Exception as err:
                trace.notify(DEBUG_ERROR, "Could not Re-register MQTT sub event - rc: " + str(err))
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "Could not Re-register MQTT sub event - rc: " + jmriObj.getObjTypeStr(self.mqttSubRecordDict[sysNameItter].type) + " System name: " + str(sysNameItter) + ", result: " + rc.getErrStr(res))
                return res
        trace.notify(DEBUG_INFO, "Done re-registering RPC sub events")
        return rc.OK

    def unRegMqttSub(self, type, sysName):
        trace.notify(DEBUG_TERSE, "Un-Register MQTT subscription for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.mqttSubRecordDict[sysName]
        except:
            trace.notify(DEBUG_ERROR, "Cannot un-register MQTT subscription for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - record of registration does not exist")
            return rc.DOES_NOT_EXIST
        try:
            res = self.rpc.rpcUnRegMqttSub(type, sysName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "un-register MQTT subscription - rc: " + err.errmsg)
            rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "un-register MQTT subscription - rc: " + str(err))
            rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not un-register MQTT subscription: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        del self.mqttSubRecordDict[sysName]
        trace.notify(DEBUG_INFO, "Successfully un-registered MQTT subscription for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def createObject(self, type, sysName):
        trace.notify(DEBUG_TERSE, "Creating JMRI object for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            self.jmriObjDict[sysName]
        except:
            pass
        else:
            trace.notify(DEBUG_ERROR, "Could not create JMRI object - already exists - Type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
            try:
                self.rpc.rpcDeleteObject(type, sysName)
            except:
                pass
        try:
            res = self.rpc.rpcCreateObject(type, sysName)
        except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "Could not create JMRI object - rc: " + err.errmsg)
                rc.GEN_COM_ERR
        except Exception as err:
                trace.notify(DEBUG_ERROR, "Could not create JMRI object - rc: " + str(err))
                rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not create JMRI object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        self.jmriObjDict[sysName] = jmriObjRecord()
        self.jmriObjDict[sysName].type = type
        self.jmriObjDict[sysName].sysName = sysName
        trace.notify(DEBUG_INFO, "Successfully created JMRI object for object type: - Type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
        return rc.OK

    def reCreateObjects(self):
        trace.notify(DEBUG_TERSE, "Re-consiliating JMRI objects")
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        for sysNameItter in self.jmriObjDict:
            try:
                res = self.rpc.rpcCreateObject(self.jmriObjDict[sysNameItter].type, self.jmriObjDict[sysNameItter].sysName)
            except xmlrpc.client.ProtocolError as err:
                trace.notify(DEBUG_ERROR, "Could not Re-consiliate JMRI object \"" + sysNameItter + "\" - rc: " + err.errmsg)
                return rc.GEN_COM_ERR
            except Exception as err:
                trace.notify(DEBUG_ERROR, "Could not Re-consiliate JMRI object \"" + sysNameItter + "\" - rc: " + str(err))
                return rc.GEN_COM_ERR
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "Could not Re-consiliate JMRI object \"" + sysNameItter + "\" - rc: " + ", result: " + rc.getErrStr(res))
                return res
            if self.jmriObjDict[sysNameItter].userName != None:
                try:
                    res = self.rpc.rpcSetUserNameBySysName(self.jmriObjDict[sysNameItter].type, self.jmriObjDict[sysNameItter].sysName, self.jmriObjDict[sysNameItter].userName)
                except xmlrpc.client.ProtocolError as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate Username for JMRI object \"" + sysNameItter + "\" - rc: " + err.errmsg)
                    return rc.GEN_COM_ERR
                except Exception as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate Username for JMRI object \"" + sysNameItter + "\" - rc: " + str(err))
                    return rc.GEN_ERR
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate Username for JMRI object \"" + sysNameItter + "\" - rc: " + rc.getErrStr(res))
                    return res

            if self.jmriObjDict[sysNameItter].description != None:
                try:
                    res = self.rpc.rpcSetCommentBySysName(self.jmriObjDict[sysNameItter].type, self.jmriObjDict[sysNameItter].sysName, self.jmriObjDict[sysNameItter].description)
                except xmlrpc.client.ProtocolError as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate Description for JMRI object \"" + sysNameItter + "\" - rc: " + err.errmsg)
                    return rc.GEN_COM_ERR
                except Exception as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate Description for JMRI object \"" + sysNameItter + "\" - rc: " + str(err))
                    return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "Could not Re-consiliate Description for JMRI object \"" + sysNameItter + "\" - rc: " + rc.getErrStr(res))
                return res
            if self.jmriObjDict[sysNameItter].state != None:
                try:
                    res = self.rpc.rpcSetStateBySysName(self.jmriObjDict[sysNameItter].type, self.jmriObjDict[sysNameItter].sysName, self.jmriObjDict[sysNameItter].state)
                except xmlrpc.client.ProtocolError as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate State for JMRI object \"" + sysNameItter + "\" - rc: " + err.errmsg)
                    return rc.GEN_COM_ERR
                except Exception as err:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate State for JMRI object \"" + sysNameItter + "\" - rc: " + str(err))
                    return rc.GEN_ERR
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Could not Re-consiliate State for JMRI object \"" + sysNameItter + "\" - rc: " + rc.getErrStr(res))
                    return rc.res
        trace.notify(DEBUG_TERSE, "Done re-consiliating JMRI objects ")
        return rc.OK

    def getObjectConfig(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetConfigsXmlByType(type))).get(jmriObj.getObjTypeStr(type)).get(sysName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get objectConfig \"" + sysName + "\" - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get objectConfig \"" + sysName + "\" - rc: " + str(err))
            return None

    def canDeleteObject(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return False
        try:
            canDeleteObject = self.rpc.rpcCanDeleteObject(type, sysName)
        except xmlrpc.client.ProtocolError as err:
            traceback.notify(DEBUG_ERROR, "Could not evaluate if JMRI object can be deleted\"" + sysName + "\" - rc: " + err.errmsg)
            return rc.CANNOT_DELETE
        except Exception as err:
            traceback.notify(DEBUG_ERROR, "Could not evaluate if JMRI object can be deleted\"" + sysName + "\" - rc: " + str(err))
            return rc.CANNOT_DELETE
        trace.notify(DEBUG_INFO, "Can delete object request?: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + str(canDeleteObject))
        return canDeleteObject

    def deleteObject(self, type, sysName):
        trace.notify(DEBUG_TERSE, "Deleting object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            res = self.rpc.rpcDeleteObject(type, sysName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not delete JMRI object \"" + sysName + "\" - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not delete JMRI object \"" + sysName + "\" - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not delete JMRI object \"" + sysName + "\" - rc: " + rc.getErrStr(res))
            return rc.res
        del self.jmriObjDict[sysName]
        trace.notify(DEBUG_INFO, "Successfully deleted object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def getConfigsByType(self, type):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetConfigsXmlByType(type)))
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get type configuration for type \"" + type + "\" - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get type configuration for type \"" + type + "\" - rc: " + str(err))
            return None

    def getUserNameBySysName(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            userName = dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetUserNameXmlBySysName(type, sysName))).get("usrName")
            jmriObjDict[sysName].userName = userName
            return userName
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get userName for systemName  \"" + sysName + "\" - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get userName for systemName  \"" + sysName + "\" - rc: " + str(err))
            return None

    def setUserNameBySysName(self, type, sysName, userName):
        trace.notify(DEBUG_TERSE, "Setting user name: " + userName + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            res = self.rpc.rpcSetUserNameBySysName(type, sysName, userName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set userName for systemName \"" + sysName + "\" - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set userName for systemName \"" + sysName + "\" - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set userName for systemName \"" + sysName + "\" - rc: " + rc.getErrStr(res))
            return res
        self.jmriObjDict[sysName].userName = userName
        trace.notify(DEBUG_INFO, "Successfully set user name: " + userName + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def getCommentBySysName(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            comment = dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetCommentXmlBySysName(type, sysName))).get("comment")
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set comment for systemName \"" + sysName + "\" - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set comment for systemName \"" + sysName + "\" - rc: " + str(err))
            return None
        self.jmriObjDict[sysName].description = comment
        return comment

    def setCommentBySysName(self, type, sysName, comment):
        trace.notify(DEBUG_TERSE, "Setting comment: " + comment + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            res = self.rpc.rpcSetCommentBySysName(type, sysName, comment)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set comment for systemName \"" + sysName + "\" - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set comment for systemName \"" + sysName + "\" - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set comment for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        self.jmriObjDict[sysName].description = comment
        trace.notify(DEBUG_INFO, "Successfully set comment: " + comment + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def setStateBySysName(self, type, sysName, state):
        trace.notify(DEBUG_TERSE, "Setting state to: " + state + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            res = self.rpc.rpcSetStateBySysName(type, sysName, state)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(res))
            return res
        self.jmriObjDict[sysName].state = state
        trace.notify(DEBUG_INFO, "Successfully set state to: " + state + " for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        return rc.OK

    def getStateBySysName(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetStateXmlBySysName(type, sysName))).get("state")
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + str(err))
            return None

    def getValidStatesBySysName(self, type, sysName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            states = dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetValidStatesBySysName(type, sysName))).get("states").strip("[]").split(",")
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get state for: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + str(err))
            return None
        for i in range(len(states)):
            states[i] = states[i].strip()
        return states

    def setRpcServerDebugLevel(self, globalDebugLevelStr):
        trace.notify(DEBUG_TERSE, "Setting global debug level to " + globalDebugLevelStr)
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        self.globalDebugLevelStr = globalDebugLevelStr
        try:
            res = self.rpc.rpcsetRpcServerDebugLevel(globalDebugLevelStr)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set global debug level - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set global debug level - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set global debug level - rc: " + rc.getErrStr(res))
            return res
        trace.notify(DEBUG_INFO, "Successfully set global debug level to " + globalDebugLevelStr)
        return rc.OK

    def getKeepaliveInterval(self):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return 0
        try:
            return self.rpc.rpcGetKeepaliveInterval()
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get keep-alive interval - rc: " + err.errmsg)
            return 0
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get keep-alive interval - rc: " + str(err))
            return 0

    def setKeepaliveInterval(self, keepaliveInterval):
        trace.notify(DEBUG_TERSE, "Setting RPC keep-alive interval (Server and Client) to: " + str(keepaliveInterval))
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return rc.GEN_COM_ERR
        try:
            res = self.rpc.rpcSetKeepaliveInterval(keepaliveInterval)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not set keep-alive interval - rc: " + err.errmsg)
            return rc.GEN_COM_ERR
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not set keep-alive interval - rc: " + str(err))
            return rc.GEN_ERR
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set keep-alive interval - rc: " + res)
            return res
        self.keepAliveInterval = keepaliveInterval
        #self.startKeepAliveHandler()
        trace.notify(DEBUG_INFO, "Successfully set RPC keep-alive interval (Server and Client) to: " + str(keepaliveInterval))
        return rc.OK

    def getFile(self, fileName):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            return self.rpc.rpcGetFile(fileName)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get file - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get file - rc: " + str(err))
            return None

    def listDir(self, path):
        if self.retryStateMachine != RPC_CONNECTED and self.retryStateMachine != RPC_CONNECTING:
            trace.notify(DEBUG_ERROR, "RPC connection failed - connection lost")
            return None
        try:
            return self.rpc.rpcListDir(path)
        except xmlrpc.client.ProtocolError as err:
            trace.notify(DEBUG_ERROR, "Could not get file - rc: " + err.errmsg)
            return None
        except Exception as err:
            trace.notify(DEBUG_ERROR, "Could not get file - rc: " + str(err))
            return None

    def cbGenerator(self):
        self.cbLoopRunning = True
        while self.runCbLoop:
            trace.notify(DEBUG_VERBOSE, "Waiting for call-back")
            try:
                callBackDict = dictEscapeing.dictUnEscape(xmltodict.parse(self.cbRpc.rpcOnStateChange()))
            except:
                trace.notify(DEBUG_ERROR, "CB Generator failed to communicate with server - reconnecting...")
                self._reConnect(rc.SOCK_ERR)
                return
            trace.notify(DEBUG_VERBOSE, "Got a RPC CB message: " + str(callBackDict))
            if isRelease(callBackDict):
                trace.notify(DEBUG_INFO, "Got a RPC CB release event")
                break
            if isKeepAlive(callBackDict):
                trace.notify(DEBUG_VERBOSE, "Got a Keep-alive message")
                self._stopRetryConnect()
                self.missedKeepAlive = 0
            elif isStateChange(callBackDict):
                trace.notify(DEBUG_VERBOSE, "Got a state change message for: " + getCallBackSysName(callBackDict))
                cbEvent.type = getCallbackType(callBackDict)
                cbEvent.sysName = getCallBackSysName(callBackDict)
                cbEvent.usrName = getCallBackUsrName(callBackDict)
                cbEvent.oldState = getCallBackOldState(callBackDict)
                cbEvent.newState = getCallBackNewState(callBackDict)
                try:
                    for cb in self.regCbs[cbEvent.type][cbEvent.sysName]:
                        cb(cbEvent)
                except:
                    pass
            elif isMqttStateChange(callBackDict):
                trace.notify(DEBUG_VERBOSE, "Got an MQTT state change message")
                try:
                    for cb in self.regCbs["mqttStateChange"]:
                        cb(getMQTTCallbackState(callBackDict))

                except:
                    pass
            else:
                trace.notify(DEBUG_ERROR, "Got RPC CB garbage: " + str(callBackDict))
        self.cbLoopRunning = False

    def startKeepAliveHandler(self):
        self.stopKeepAliveHandler()
        if self.keepAliveInterval != 0:
            self.keepAliveTimerActive = True
            self.keepAliveTimerHandle = threading.Timer(self.keepAliveInterval, self.keepAliveTimer)
            self.keepAliveTimerHandle.start()

    def stopKeepAliveHandler(self):
        self.keepAliveTimerActive = False
        try:
            self.keepAliveTimerHandle.cancel()
        except:
            pass

    def keepAliveTimer(self):
        if not self.keepAliveTimerActive:
            return
        self.missedKeepAlive += 1
        if self.missedKeepAlive > MAX_MISSED_KEEPALIVE:
            trace.notify(DEBUG_ERROR, "RPC keepalive error,  more than " + str(MAX_MISSED_KEEPALIVE) + " keep-alive messages missed")
            self._reConnect(rc.KEEPALIVE_TIMEOUT)
        else:
            trace.notify(DEBUG_VERBOSE, "RPC keepalive timer report " + str(self.missedKeepAlive) + " keep-alive messages missed...")
            if self.keepAliveInterval != 0:
                if not self.keepAliveTimerActive:
                    return
                self.keepAliveTimerHandle = threading.Timer(self.keepAliveInterval, self.keepAliveTimer)
                self.keepAliveTimerHandle.start()

    def _reConnect(self, err):
        if not self.retry:
            self.runCbLoop = False
            self.retry = True
            self.retryStateMachine = RPC_NOT_CONNECTED
            self.connected = False
            trace.notify(DEBUG_ERROR, "RPC connection broken, reconnecting...")
            if self.errCb != None:
                self.errCb(err)
            threading.Timer(RPC_RETRY_PERIOD_S, self._retryConnectLoop).start()

    def _stopRetryConnect(self, reconnected = True):
        if not self.retry:
            return
        self.retry = False
        if reconnected:
            trace.notify(DEBUG_INFO, "RPC client successfully re-connected, receiving Ping responces")
            self.retryStateMachine = RPC_CONNECTED
            if not self.connected:
                if self.errCb != None:
                    self.errCb(rc.OK)
            self.connected = True
        else:
            trace.notify(DEBUG_INFO, "RPC client reconnect process interupted")
            self.retryStateMachine = RPC_NOT_ACTIVE

    def _retryConnectLoop(self):
        if self.retry:
            if self.retryStateMachine == RPC_NOT_CONNECTED:
                try:
                    self.retryStateMachine = RPC_CONNECTING
                    assert self._tryReconnect() == rc.OK
                    trace.notify(DEBUG_INFO, "RPC client successfully re-connected to server, waiting for RPC keep alive messages...")
                    self.retryStateMachine = RPC_CONNECTED_NO_KEEPALIVE
                    threading.Timer(3 * self.keepAliveInterval, self._retryConnectLoop).start()
                    return
                except Exception:
                    traceback.print_exc() #Should this be removed or should it be based on verbosity?
                    trace.notify(DEBUG_ERROR, "RPC client could not re-connect to server, retrying...")
                    self.retryStateMachine = RPC_NOT_CONNECTED
                    threading.Timer(RPC_RETRY_PERIOD_S, self._retryConnectLoop).start()
                    return
            elif self.retryStateMachine == RPC_CONNECTED_NO_KEEPALIVE:
                trace.notify(DEBUG_ERROR, "RPC client did not receive any keep-alive messages within 3 keep-alive periods, trying to re-connect...")
                self.retryStateMachine = RPC_NOT_CONNECTED
                threading.Timer(RPC_RETRY_PERIOD_S, self._retryConnectLoop).start()
                return
            else:
                trace.notify(DEBUG_PANIC, "RPC client retry loop is in an undefined state")
        else:
            trace.notify(DEBUG_VERBOSE, "RPC client reconnect tries were canceled, leaving the _retryConnectLoop")
# END < Class: jmriRpcClient> -------------------------------------------------------------------------------------------------------------------
