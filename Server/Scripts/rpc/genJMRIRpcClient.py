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
#       unRegEventCb(type, sysName, cb, allCb=False): returns genJMRIRc.rc
#           Description: Un Registers a client call-back function for JMRI object state changes
#           Parameters:  type: JMRI object type: genJMRIObj.x
#                        str:sysName: JMRI object system name
#                        fun:cb: Call-back function reference
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
import copy
sys.path.append(os.path.realpath('..'))
import traceback
import keyboard
import xmlrpc.client
import threading
import time
import xmltodict
import imp
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
        return None
# END <Helper class and fuctions> ---------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: jmriRpcClient, see header description of this file
# Description: Provides all public proxy RPC methods provided by the library
#################################################################################################################################################
class jmriRpcClient():
    def __init__(self):
        pass
    
    def start(self, uri="localhost", portBase = 8000, errCb=None):
        trace.notify(DEBUG_INFO, "Starting RPC client - remote end-points: " + uri + ":" + str(portBase) + " and "  + uri + ":" + str(portBase+1))
        self.rpc = xmlrpc.client.ServerProxy("http://" + uri + ":" + str(portBase) + "/")
        self.cbRpc = xmlrpc.client.ServerProxy("http://" + uri + ":" + str(portBase + 1) + "/")
        self.running = False
        self.regCbs = {}
        self.errCb = errCb
        self.mqttPubRecordDict = {}
        self.mqttSubRecordDict = {}
        self.running = True
        self.missedKeepAlive = 0
        self.keepAlive = self.getKeepaliveInterval()
        self.jmriRpcCbThread = threading.Thread(target=self.cbGenerator)
        self.jmriRpcCbThread.start()
        self.keepaliveTimerHandle = threading.Timer(self.keepAlive, self.keepAliveTimer)
        self.keepaliveTimerHandle.start()
        trace.notify(DEBUG_INFO, "RPC client started")

    def stop(self):
        trace.notify(DEBUG_INFO, "Stopping RPC client")
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
        try:
            self.keepaliveTimerHandle.cancel()
        except:
            pass
        self.running = False
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

    def regEventCb(self, type, sysName, cb):
        trace.notify(DEBUG_INFO, "Registering RPC client callback: " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        if self.regCbs.get(jmriObj.getObjTypeStr(type)) == None:
            self.regCbs[jmriObj.getObjTypeStr(type)] = {sysName:[cb]}
            if self.rpc.rpcListen(type, sysName) != rc.OK:
                self.unRegEventCb(type, sysName, cb)
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - " + rc.getErrStr(rc.GEN_ERR))
                return rc.GEN_ERR
        elif self.regCbs[jmriObj.getObjTypeStr(type)].get(sysName) == None:
            self.regCbs[jmriObj.getObjTypeStr(type)][sysName] = [cb]
            if self.rpc.rpcListen(type, sysName) != rc.OK:
                self.unRegEventCb(type, sysName, cb)
                trace.notify(DEBUG_ERROR, "Registering RPC client callback failed - " + rc.getErrStr(rc.GEN_ERR))
                return rc.GEN_ERR
        else:
            self.regCbs[jmriObj.getObjTypeStr(type)][sysName].append(cb)
        return rc.OK

    def unRegEventCb(self, type, sysName, cb, allCb=False):
        if allCb:
            trace.notify(DEBUG_INFO, "Un-Registering ALL RPC client callbacks: " + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        else:
            trace.notify(DEBUG_INFO, "Un-Registering RPC client callback: " + str(cb.__name__) + ", for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
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

    def regMqttPub(self, type, sysName, topic, payloadMap):
        trace.notify(DEBUG_INFO, "Register MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        try:
            self.mqttPubRecordDict[sysName]
        except:
            pass
        else:
            trace.notify(DEBUG_INFO, "MQTT pub event already registered for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
            self.unRegMqttPub(type, sysName)
        rcRegMqttPub = self.rpc.rpcRegMqttPub(type, sysName, topic, payloadMap)
        if rcRegMqttPub != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not register MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcRegMqttPub))
        self.mqttPubRecordDict[sysName] = type
        return rcRegMqttPub

    def unRegMqttPub(self, type, sysName):
        trace.notify(DEBUG_INFO, "Un-Register MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        try:
            self.mqttPubRecordDict[sysName]
        except:
            trace.notify(DEBUG_INFO, "Cannot unregister MQTT pub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - record of registration does not exist")
            return rc.DOES_NOT_EXIST
        rcUnRegMqttPub = self.rpc.rpcUnRegMqttPub(type, sysName)
        if rcUnRegMqttPub != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not Unregister MQTT publishment: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcUnRegMqttPub))
        else:
            del self.mqttPubRecordDict[sysName]
        return rcUnRegMqttPub

    def regMqttSub(self, type, sysName, topic, payloadMap):
        trace.notify(DEBUG_INFO, "Register MQTT sub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", Topic: " + str(topic) + ", Payload-map: " + str(payloadMap))
        try:
            self.mqttSubRecordDict[sysName]
        except:
            pass
        else:
            trace.notify(DEBUG_INFO, "MQTT sub event already registered for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - replacing it")
            self.unRegMqttSub(type, sysName)
        rcRegMqttSub = self.rpc.rpcRegMqttSub(type, sysName, topic, payloadMap)
        if rcRegMqttSub != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not register MQTT sub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcRegMqttPub))
        self.mqttSubRecordDict[sysName] = type
        return rcRegMqttSub

    def unRegMqttSub(self, type, sysName):
        trace.notify(DEBUG_INFO, "Un-Register MQTT sub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        try:
            self.mqttSubRecordDict[sysName]
        except:
            trace.notify(DEBUG_INFO, "Cannot unregister MQTT sub event for object type: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + " - record of registration does not exist")
            return rc.DOES_NOT_EXIST
        rcUnRegMqttSub = self.rpc.rpcUnRegMqttSub(type, sysName)
        if rcUnRegMqttSub != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not Unregister MQTT Subscription: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcUnRegMqttSub))
        else:
            del self.mqttSubRecordDict[sysName]
        return rcUnRegMqttSub

    def createObject(self, type, sysName):
        trace.notify(DEBUG_INFO, "Create object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        rcCreateObject = self.rpc.rpcCreateObject(type, sysName)
        if rcCreateObject != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not create object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcCreateObject))
        return rcCreateObject

    def getObjectConfig(self, type, sysName):
        return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetConfigsXmlByType(type))).get(jmriObj.getObjTypeStr(type)).get(sysName)

    def canDeleteObject(self, type, sysName):
        canDeleteObject = self.rpc.rpcCanDeleteObject(type, sysName)
        trace.notify(DEBUG_INFO, "Can delete object request?: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + str(canDeleteObject))
        return canDeleteObject

    def deleteObject(self, type, sysName):
        trace.notify(DEBUG_INFO, "Deleting object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        rcDeleteObject = self.rpc.rpcDeleteObject(type, sysName)
        if rcDeleteObject != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not delete object: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcDeleteObject))
        return rcDeleteObject

    def getConfigsByType(self, type):
        return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetConfigsXmlByType(type)))

    def getUserNameBySysName(self, type, sysName):
        return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetUserNameXmlBySysName(type, sysName))).get("usrName")

    def setUserNameBySysName(self, type, sysName, usrName):
        trace.notify(DEBUG_INFO, "Setting user name: " + usrName + " to: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        rcSetUserName = self.rpc.rpcSetUserNameBySysName(type, sysName, usrName)
        if rcSetUserName != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set user name: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcSetUserName))
        return rcSetUserName

    def getCommentBySysName(self, type, sysName):
        return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetCommentXmlBySysName(type, sysName))).get("comment")

    def setCommentBySysName(self, type, sysName, comment):
        trace.notify(DEBUG_INFO, "Setting comment: " + comment + " to: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        rcSetComment = self.rpc.rpcSetCommentBySysName(type, sysName, comment)
        if rcSetComment != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set comment: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcSetComment))
        return rcSetComment

    def getStateBySysName(self, type, sysName):
        return dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetStateXmlBySysName(type, sysName))).get("state")

    def getValidStatesBySysName(self, type, sysName):
        states = dictEscapeing.dictUnEscape(xmltodict.parse(self.rpc.rpcGetValidStatesBySysName(type, sysName))).get("states").strip("[]").split(",")
        for i in range(len(states)):
            states[i] = states[i].strip()
        return states

    def setStateBySysName(self, type, sysName, state):
        trace.notify(DEBUG_INFO, "Setting state: " + state + " to: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName))
        rcSetState = self.rpc.rpcSetStateBySysName(type, sysName, state)
        if rcSetState != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not set state: " + jmriObj.getObjTypeStr(type) + " System name: " + str(sysName) + ", result: " + rc.getErrStr(rcSetState))
        return rcSetState

    def setRpcServerDebugLevel(self, globalDebugLevelStr):
        trace.notify(DEBUG_INFO, "Setting global debug level to " + globalDebugLevelStr)
        return self.rpc.rpcsetRpcServerDebugLevel(globalDebugLevelStr)

    def getKeepaliveInterval(self):
        return self.rpc.rpcGetKeepaliveInterval()

    def setKeepaliveInterval(self, keepaliveInterval):
        trace.notify(DEBUG_INFO, "Setting rpc keep-alive interval to " + str(keepaliveInterval))
        try:
            self.keepaliveTimerHandle.cancel()
        except:
            pass
        self.keepAlive = keepaliveInterval
        self.keepaliveTimerHandle = threading.Timer(self.keepAlive, self.keepAliveTimer)
        self.keepaliveTimerHandle.start()
        return self.rpc.rpcSetKeepaliveInterval(keepaliveInterval)

    def cbGenerator(self):
        while self.running:
            trace.notify(DEBUG_VERBOSE, "Waiting for call-back")
            callBackDict = dictEscapeing.dictUnEscape(xmltodict.parse(self.cbRpc.rpcOnStateChange()))
            trace.notify(DEBUG_VERBOSE, "Got a RPC CB message: " + str(callBackDict))
            if isRelease(callBackDict):
                trace.notify(DEBUG_INFO, "Got a RPC CB release event")
                break
            if isKeepAlive(callBackDict):
                trace.notify(DEBUG_VERBOSE, "Got a Keep-alive message: ")
                self.missedKeepAlive = 0
            elif isStateChange(callBackDict): 
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
            else:
                trace.notify(DEBUG_ERROR, "Got a RPC CB garbage")
                pass

    def keepAliveTimer(self):
        self.missedKeepAlive += 1
        trace.notify(DEBUG_VERBOSE, "RPC keepalive timer report " + str(self.missedKeepAlive) + " keep-alive messages missed...")
        if self.missedKeepAlive > MAX_MISSED_KEEPALIVE:
            if self.errCb != None:
                self.errCb(rc.KEEPALIVE_TIMEOUT)
            trace.notify(DEBUG_PANIC, "RPC keepalive error,  more than " + str(MAX_MISSED_KEEPALIVE) + " keep-alive messages missed")

        else:
            self.keepaliveTimerHandle = threading.Timer(self.keepAlive, self.keepAliveTimer)
            self.keepaliveTimerHandle.start()
# END < Class: jmriRpcClient> -------------------------------------------------------------------------------------------------------------------
