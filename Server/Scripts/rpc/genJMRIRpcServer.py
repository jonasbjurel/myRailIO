#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A JMRI server-side module used to configure-, monitor-, and define atonoumus actions through JMRI using XML-RPC.
# It provides methods for monitor and control of objects such as JMRI masts, turnouts, lights, sensors, and memories. Callback capabilities
# triggered by changes of these objects and capabilities to configure real-time MQTT publications at state change of these objects, emitted
# on the server side - to avoid RPC latency
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################

#################################################################################################################################################
# Module/Library dependance
#################################################################################################################################################
import os
import sys
import traceback
import java
import java.beans
import jmri
from SimpleXMLRPCServer import SimpleXMLRPCServer
import threading
import time
import imp
# Of course below absolute path definitions needs to be fixed to something portable ...
imp.load_source('JMRIObjects', 'C:\\Users\\jonas\\OneDrive\\Projects\\ModelRailway\\GenericJMRIdecoder\\Server\\Scripts\\rpc\\JMRIObjects.py')
from JMRIObjects import jmriObj
imp.load_source('genJMRIRc', 'C:\\Users\\jonas\\OneDrive\\Projects\\ModelRailway\\GenericJMRIdecoder\\Server\\Scripts\\rc\\genJMRIRc.py')
from genJMRIRc import rc
imp.load_source('dictEscapeing', 'C:\\Users\\jonas\\OneDrive\\Projects\\ModelRailway\\GenericJMRIdecoder\\Server\\Scripts\\rpc\\dictEscapeing.py')
from dictEscapeing import *
imp.load_source('myTrace', 'C:\\Users\\jonas\\OneDrive\\Projects\\ModelRailway\\GenericJMRIdecoder\\Server\\Scripts\\trace\\trace.py')
from myTrace import *
# END <Module/Library dependance> ---------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# Parameters
#################################################################################################################################################
JMRI_DEFAULT_RPC_MAX_MISSED_KEEPALIVE = 3
JMRI_DEFAULT_RPC_KEEPALIVE_INTERVAL = 10
JMRI_RPC_KEEPALIVE_INTERVAL_ADDR = "IM_GENJMRI_RPC_KEEPALIVE_INTERVAL"
JMRI_DEFAULT_RPC_SERVER_GLOBAL_DEBUG_LEVEL = DEBUG_INFO
JMRI_RPC_SERVER_GLOBAL_DEBUG_LEVEL_ADDR = "IM_GENJMRI_GLOBAL_DEBUG_LEVEL"
# END <Parameters> ------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Helper class and fuctions
#################################################################################################################################################
def getObjType(type):
    if type == jmriObj.MASTS:
        return masts
    if type == jmriObj.TURNOUTS:
        return turnouts
    if type == jmriObj.LIGHTS:
        return lights
    if type == jmriObj.SENSORS:
        return sensors
    if type == jmriObj.MEMORIES:
        return memories

def dicttoxml(dictionary, comment = "", itter = 0):
    if itter == 0:
        xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        if comment != "":
            xml = xml + comment + "\n"
    else:
        xml = ""
    for key in dictionary:
        if isinstance(dictionary[key], dict):
            xml = xml + ("\t"*itter) + "<" + str(key) + ">" + "\n" + dicttoxml(dictionary[key], itter = itter + 1) + ("\t"*itter) + "</" + str(key) + ">" + "\n"
        else:
            xml = xml + ("\t"*itter) + "<" + str(key) + ">" + str(dictionary[key]) + "</" + str(key) + ">" + "\n"
    return xml

def state2stateStr(type, sysName, state):
    if type == jmriObj.TURNOUTS:
        if state == turnouts.getBySystemName(sysName).THROWN:
            return "THROWN"
        elif state == turnouts.getBySystemName(sysName).CLOSED:
            return "CLOSED"
        else:
            return "UNKNOWN"
    elif type == jmriObj.LIGHTS:
        if state == lights.getBySystemName(sysName).ON:
            return "ON"
        elif state == lights.getBySystemName(sysName).OFF:
            return "OFF"
        else:
            return "UNKNOWN"
    elif type == jmriObj.SENSORS:
        if state == sensors.getBySystemName(sysName).ACTIVE:
            return "ACTIVE"
        elif state == sensors.getBySystemName(sysName).INACTIVE:
            return "INACTIVE"
        else:
            return "UNKNOWN"
    else:
        return state

def stateStr2state(type, sysName, stateStr):
    if type == jmriObj.TURNOUTS:
        if stateStr == "THROWN":
            return turnouts.getBySystemName(sysName).THROWN
        else:
            return turnouts.getBySystemName(sysName).CLOSED
    elif type == jmriObj.LIGHTS:
        if stateStr == "ON":
            return lights.getBySystemName(sysName).ON
        else:
            return lights.getBySystemName(sysName).OFF
            stateStr = "OFF"
    elif type == jmriObj.SENSORS:
        if stateStr == "ACTIVE":
            return sensors.getBySystemName(sysName).ACTIVE
        else:
            return sensors.getBySystemName(sysName).INACTIVE
    else:
        return stateStr
# END <Helper class and fuctions> ---------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# jmriAPIShim
# Description: Provides methods to interact with JMRI
#################################################################################################################################################
class jmriAPIShim(object):
    @staticmethod
    def createObject(type, sysName):
        trace.notify(DEBUG_TERSE, "Creating JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName)
        try:
            if type == jmriObj.MASTS:
                getObjType(type).provideSignalMast(sysName)
            elif type == jmriObj.TURNOUTS:
                getObjType(type).provide(sysName)
            elif type == jmriObj.LIGHTS:
                getObjType(type).provideLight(sysName)
            elif type == jmriObj.SENSORS:
                getObjType(type).provide(sysName)
            elif type == jmriObj.MEMORIES:
                getObjType(type).provideMemory(sysName)
            else:
                trace.notify(DEBUG_ERROR, "Could not create JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + ", type: " + jmriObj.getObjTypeStr(type) + "does not exist - " + rc.getErrStr(rc.DOES_NOT_EXIST))
                return rc.DOES_NOT_EXIST
        except:
            trace.notify(DEBUG_ERROR, "Could not create JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR
        return rc.OK

    @staticmethod
    def canDeleteObject(type, sysName):
        try:
            getObjType(type).deleteBean(sysName, "CanDelete")
            return rc.OK
        except:
            trace.notify(DEBUG_INFO, "Cannot delete JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.CANNOT_DELETE) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.CANNOT_DELETE

    @staticmethod
    def deleteObject(type, sysName):
        trace.notify(DEBUG_TERSE, "Deleting JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName)
        try:
            getObjType(type).deleteBean(getObjType(type).getBySystemName(sysName),"DoDelete")
            return rc.OK
        except:
            trace.notify(DEBUG_INFO, "Could not delete JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.CANNOT_DELETE) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.CANNOT_DELETE

    @staticmethod
    def getConfigsByType(type):
        objs = jmriAPIShim.getObjsByType(type)
        objConfigs = {}
        objConfigs[getObjType(type).getBeanTypeHandled()] = {}
        for obj in objs[getObjType(type).getBeanTypeHandled()]:
            objConfigs[getObjType(type).getBeanTypeHandled()][str(obj)] = {}
            objConfigs[getObjType(type).getBeanTypeHandled()][str(obj)].update(jmriAPIShim.getUserNameBySysName(type, str(obj)))
            objConfigs[getObjType(type).getBeanTypeHandled()][str(obj)].update(jmriAPIShim.getCommentBySysName(type, str(obj)))
        return objConfigs

    @staticmethod
    def getUserNameBySysName(type, sysName):
        try:
            return {"usrName" : str(getObjType(type).getBySystemName(sysName).getUserName())}
        except:
            trace.notify(DEBUG_ERROR, "Could not get user name by system name: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def getCommentBySysName(type, sysName):
        try:
            return {"comment" : getObjType(type).getBySystemName(sysName).getComment()}
        except:
            trace.notify(DEBUG_ERROR, "Could not get comment by system name: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def getObjsByType(type):
        objs = {}
        objs[str(getObjType(type).getBeanTypeHandled())] = {}
        for obj in getObjType(type).getNamedBeanSet():
            objs[str(getObjType(type).getBeanTypeHandled())].update({str(obj) : obj})
        return objs

    @staticmethod
    def getStateBySysName(type, sysName):
        try:
            if type == jmriObj.MASTS:
                state = getObjType(type).getBySystemName(sysName).getAspect()
            elif type == jmriObj.TURNOUTS or type == jmriObj.LIGHTS or type == jmriObj.SENSORS:
                state = state2stateStr(type, sysName, getObjType(type).getBySystemName(sysName).getKnownState())
            elif type == jmriObj.MEMORIES:
                state = getObjType(type).getBySystemName(sysName).getValue()
            else:
                trace.notify(DEBUG_ERROR, "Could not get current state for JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + ", type " + jmriObj.getObjTypeStr(type) + " does not exist - " + rc.getErrStr(rc.DOES_NOT_EXIST))
                return rc.DOES_NOT_EXIST
            return {"state" : state}
        except:
            trace.notify(DEBUG_INFO, "Could not get current state for JMRI object: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def getValidStatesBySysName(type, sysName):
        if type == jmriObj.MASTS:
            return {"states":str(getObjType(jmriObj.MASTS).getBySystemName(sysName).getValidAspects())}
        elif type == jmriObj.SENSORS:
            return {"states":"[ACTIVE, INACTIVE]"}
        elif type == jmriObj.TURNOUTS:
            return {"states":"[THROWN, CLOSED]"}
        elif type == jmriObj.LIGHTS:
            return {"states":"[ON, OFF]"}
        elif type == jmriObj.MEMORIES:
            return {"states":"[*]"}
        else:
            trace.notify(DEBUG_ERROR, "Could not get valid states for JMRI object : " + jmriObj.getObjTypeStr(type) + ":" + sysName + ", type " + jmriObj.getObjTypeStr(type) + " does not exist - " + rc.getErrStr(rc.DOES_NOT_EXIST))
            return rc.DOES_NOT_EXIST

    @staticmethod
    def setStateBySysName(type, sysName, stateStr):
        trace.notify(DEBUG_TERSE, "Setting state: " + stateStr + " for JMRI object:" + jmriObj.getObjTypeStr(type) + ":" + sysName)
        try:
            if type == jmriObj.MASTS:
                getObjType(type).getBySystemName(sysName).setAspect(stateStr)
            elif type == jmriObj.MEMORIES:
                getObjType(type).getBySystemName(sysName).setValue(stateStr)
            elif type == jmriObj.SENSORS or type == jmriObj.TURNOUTS or type == jmriObj.LIGHTS:
                getObjType(type).getBySystemName(sysName).setCommandedState(stateStr2state(type, sysName, stateStr))
            else:
                trace.notify(DEBUG_ERROR, "Could not set state for JMRI object:" + jmriObj.getObjTypeStr(type) + ":" + sysName + ", type " + jmriObj.getObjTypeStr(type) + " does not exist - " + rc.getErrStr(rc.DOES_NOT_EXIST))
                return rc.DOES_NOT_EXIST
            return rc.OK
        except:
            trace.notify(DEBUG_INFO, "Could not set state for JMRI object:" + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def setRpcServerDebugLevel(debugLevelStr):
        trace.notify(DEBUG_INFO, "Setting debugLevel to " + debugLevelStr)
        if jmriAPIShim.setStateBySysName(jmriObj.MEMORIES, JMRI_RPC_SERVER_GLOBAL_DEBUG_LEVEL_ADDR, debugLevelStr) != rc.OK:
            jmriAPIShim.createObject(jmriObj.MEMORIES, JMRI_RPC_SERVER_GLOBAL_DEBUG_LEVEL_ADDR)
            jmriAPIShim.setStateBySysName(jmriObj.MEMORIES, JMRI_RPC_SERVER_GLOBAL_DEBUG_LEVEL_ADDR, debugLevelStr)
        trace.setGlobalDebugLevel(trace.getSeverityFromSeverityStr(debugLevelStr))
        return rc.OK

    @staticmethod
    def getKeepaliveInterval():
        try:
            return int(jmriAPIShim.getStateBySysName(jmriObj.MEMORIES, JMRI_RPC_KEEPALIVE_INTERVAL_ADDR)["state"])
        except:
            return JMRI_DEFAULT_RPC_KEEPALIVE_INTERVAL

    @staticmethod
    def setKeepaliveInterval(keepAliveInterval):
        trace.notify(DEBUG_INFO, "Setting keep-alive interval to " + str(keepAliveInterval))
        try:
            jmriListener.keepaliveTimerHandle.cancel()
        except:
            pass
        if jmriAPIShim.setStateBySysName(jmriObj.MEMORIES, JMRI_RPC_KEEPALIVE_INTERVAL_ADDR, str(keepAliveInterval)) != rc.OK:
            jmriAPIShim.createObject(jmriObj.MEMORIES, JMRI_RPC_KEEPALIVE_INTERVAL_ADDR)
            jmriAPIShim.setStateBySysName(jmriObj.MEMORIES, JMRI_RPC_KEEPALIVE_INTERVAL_ADDR, str(keepAliveInterval))
        jmriListener.keepAlivePeriod = keepAliveInterval
        jmriListener.keepaliveTimerHandle = threading.Timer(jmriListener.keepAlivePeriod, jmriListener.keepAlive)
        jmriListener.keepaliveTimerHandle.start()
        return rc.OK

    @staticmethod
    def open(): #NEEDS DEVELOPMENT
        pass

    @staticmethod
    def save(): #NEEDS DEVELOPMENT
        pass

    @staticmethod
    def saveAs(fn): #NEEDS DEVELOPMENT
        pass
# END <jmriAPIShim> -----------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# mqttPubEvents
# Description: Provides mechanisms to emit MQTT publications at JMRI object events
#################################################################################################################################################
class mqttPubEvents():
    def __init__(self, type, sysName, topic, payloadMap):
        print("#################################### INIT")
        self.type = type
        self.sysName = sysName
        self.topic = topic
        self.payloadMap = payloadMap
        trace.notify(DEBUG_INFO, "Creating an MQTT publisher for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        getObjType(type).getBySystemName(sysName).addPropertyChangeListener(self.onStateChange)

    def update(self, type, sysName, topic, payloadMap):
        trace.notify(DEBUG_INFO, "Updating the MQTT publisher for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        #getObjType(type).getBySystemName(self.sysName).removePropertyChangeListener(self.sysName, self.onStateChange) ISSUE #99
        self.type = type
        self.sysName = sysName
        self.topic = topic
        self.payloadMap = payloadMap
        #getObjType(type).getBySystemName(sysName).addPropertyChangeListener(self.onStateChange) ISSUE #99

    def onStateChange(self, event):
        trace.notify(DEBUG_VERBOSE, "Sending an MQTT message for JMRI MQTT object event publisher: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", New JMRI object state: " + str(event.newValue) + ", Previous JMRI object state: " + str(event.oldValue) + ", MQTT Topic:" + self.topic + ", MQTT Payload: " + str(self.payloadMap.get(state2stateStr(self.type, self.sysName, event.newValue))))
        try:
           self.payloadMap["*"]
           print("XXXXXXXX Sending topic: " + self.topic)
           MQTT.publish(self.topic, state2stateStr(self.type, self.sysName, event.newValue))
        except:
            print("YYYYYYY Sending topic: " + self.topic)
            MQTT.publish(self.topic, self.payloadMap.get(state2stateStr(self.type, self.sysName, self.payloadMap[event.newValue])))

    def __del__(self): #ISSUE #99
        trace.notify(DEBUG_INFO, "Deleting MQTT publisher for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        getObjType(type).getBySystemName(sysName).removePropertyChangeListener(self.onStateChange) #ISSUE #99
# END <mqttPubEvents> ---------------------------------------------------------------------------------------------------------------------------



##################################################################################################################################################
# Class: mqttListener
# Purpose:  Listen and dispatches incomming mqtt mesages
# Data structures: None
#################################################################################################################################################
class mqttListener(jmri.jmrix.mqtt.MqttEventListener):
    def __init__(self, cb):
        self.cb = cb

    def notifyMqttMessage(self, topic, message):
        trace.notify(DEBUG_VERBOSE, "Received an MQTT message - topic: " + topic + ", payload: " + message,)
        threading.Thread(target=self.detach, args=(topic, message, )).start()

    def detach(self, topic, message):
        try:
            self.cb(topic, message)
        except :
            trace.notify(DEBUG_VERBOSE, "Exception in mqtt message handling routines:\n" + str(traceback.format_exc()))



#################################################################################################################################################
# mqttSubEvents
# Description: Provides mechanisms to subscribe to MQTT events and set JMRI object states accordingly
#################################################################################################################################################
class mqttSubEvents():
    def __init__(self, type, sysName, topic, payloadMap):
        self.type = type
        self.sysName = sysName
        self.topic = topic
        self.payloadMap = payloadMap
        trace.notify(DEBUG_INFO, "Creating an MQTT listener for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        self.myMqttListener = mqttListener(self.onStateChange)
        MQTT.subscribe(topic, self.myMqttListener)
        
    def update(self, type, sysName, topic, payloadMap):
        self.type = type
        self.sysName = sysName
        self.topic = topic
        self.payloadMap = payloadMap
        trace.notify(DEBUG_INFO, "Updating the MQTT listener for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        MQTT.unsubscribe(topic, self.myMqttListener)
        del self.myMqttListener
        self.myMqttListener = mqttListener(self.onStateChange)
        MQTT.subscribe(topic, self.myMqttListener)

    def onStateChange(self, topic, message):
        print("XXXXXXXX " + str(message))
        trace.notify(DEBUG_VERBOSE, "Received an MQTT event - topic: " + str(topic) + ", message: " + str(message) +
                                    " for JMRI Object:" + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName +
                                    " - Setting JMRI object state according to payload map: " +
                                    str(self.payloadMap))
        try:
            self.payloadMap["*"]
        except:
            jmriAPIShim.setStateBySysName(self.type, self.sysName, self.payloadMap[message])
        else:
            jmriAPIShim.setStateBySysName(self.type, self.sysName, message)

    def __del__(self): #ISSUE #100
        trace.notify(DEBUG_INFO, "Deleting MQTT listener for JMRI object: " + jmriObj.getObjTypeStr(self.type) + ":" + self.sysName + ", Topic:" + self.topic + ", payload map: " + str(self.payloadMap))
        MQTT.unsubscribe(topic, self.myMqttListener)
        del self.myMqttListener
        
# END <mqttSubEvents> ---------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# jmriRpcShimAPI
# Description: Provides thin RPC methods for the client to access JMRI objects, actual business logic is outside this class
#################################################################################################################################################
class jmriRpcShimAPI(jmriAPIShim):
    @staticmethod
    def start():
        trace.notify(DEBUG_INFO, "Starting JMRI RPC API Shim layer, regestring RPC exposed methods")
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcRegMqttPub)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcUnRegMqttPub)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcRegMqttSub)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcUnRegMqttSub)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcCreateObject)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcCanDeleteObject)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcDeleteObject)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetConfigsXmlByType)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetUserNameXmlBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcSetUserNameBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetCommentXmlBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcSetCommentBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetStateXmlBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetValidStatesBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcSetStateBySysName)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcsetRpcServerDebugLevel)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetKeepaliveInterval)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcSetKeepaliveInterval)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcGetFile)
        jmriRpcServer.regFn(jmriRpcShimAPI.rpcListDir)
        jmriRpcShimAPI.mqttPubRecordDict = {}
        jmriRpcShimAPI.mqttSubRecordDict = {}

    @staticmethod
    def rpcRegMqttPub(type, sysName, topic, payloadMap):
        print("XXXXXXXXXXXX sysnameDict" + str(jmriRpcShimAPI.mqttPubRecordDict))
        try:
            jmriRpcShimAPI.mqttPubRecordDict[sysName]
            print("XXXXXXXXXXXX Object did exist, updating it")
            jmriRpcShimAPI.mqttPubRecordDict[sysName].update(type, sysName, topic, payloadMap)
        except:
            print("XXXXXXXXXXXX Object didn't exists, creating it")
            jmriRpcShimAPI.mqttPubRecordDict[sysName] = mqttPubEvents(type, sysName, topic, payloadMap)
        return rc.OK

    @staticmethod
    def rpcUnRegMqttPub(type, sysName): #ISSUE #99
        try:
            del jmriRpcShimAPI.mqttPubRecordDict[sysName]
        except:
            pass
        return rc.OK

    @staticmethod
    def rpcRegMqttSub(type, sysName, topic, payloadMap):
        try:
            jmriRpcShimAPI.mqttSubRecordDict[sysName]
            print("XXXXXXXXXXXX Object did exist, updating it")
            jmriRpcShimAPI.mqttSubRecordDict[sysName].update(type, sysName, topic, payloadMap)
        except:
            print("XXXXXXXXXXXX Object didn't exists, creating it")
            jmriRpcShimAPI.mqttSubRecordDict[sysName] = mqttSubEvents(type, sysName, topic, payloadMap)
        return rc.OK

    @staticmethod
    def rpcUnRegMqttSub(type, sysName): #ISSUE #100
        try:
            del jmriRpcShimAPI.mqttSubRecordDict[sysName]
        except:
            pass
        return rc.OK

    @staticmethod
    def rpcCreateObject(type, sysName):
        return jmriRpcShimAPI.createObject(type, sysName)

    @staticmethod
    def rpcCanDeleteObject(type, sysName):
        return jmriRpcShimAPI.canDeleteObject(type, sysName)

    @staticmethod
    def rpcDeleteObject(type, sysName):
        return jmriRpcShimAPI.deleteObject(type, sysName)

    @staticmethod
    def rpcGetConfigsXmlByType(type):
        return dicttoxml(dictEscapeing.dictEscape(jmriRpcShimAPI.getConfigsByType(type)))

    @staticmethod
    def rpcGetUserNameXmlBySysName(type, sysName):
        return dicttoxml(dictEscapeing.dictEscape(jmriRpcShimAPI.getUserNameBySysName(type, sysName)))

    @staticmethod
    def rpcSetUserNameBySysName(type, sysName, usrName):
        getObjType(type).getBySystemName(sysName).userName=usrName
        return rc.OK

    @staticmethod
    def rpcGetCommentXmlBySysName(type, sysName):
        return dicttoxml(dictEscapeing.dictEscape(jmriRpcShimAPI.getCommentBySysName(type, sysName)))

    @staticmethod
    def rpcSetCommentBySysName(type, sysName, comment):
        getObjType(type).getBySystemName(sysName).comment=comment
        return rc.OK

    @staticmethod
    def rpcGetStateXmlBySysName(type, sysName):
        return dicttoxml(dictEscapeing.dictEscape(jmriRpcShimAPI.getStateBySysName(type, sysName)))

    @staticmethod
    def rpcGetValidStatesBySysName(type, sysName):
        return dicttoxml(dictEscapeing.dictEscape(jmriRpcShimAPI.getValidStatesBySysName(type, sysName)))

    @staticmethod
    def rpcSetStateBySysName(type, sysName, state):
        return jmriRpcShimAPI.setStateBySysName(type, sysName, state)

    @staticmethod
    def rpcsetRpcServerDebugLevel(globalDebugLevelStr):
        return jmriRpcShimAPI.setRpcServerDebugLevel(globalDebugLevelStr)

    @staticmethod
    def rpcGetKeepaliveInterval():
        return jmriRpcShimAPI.getKeepaliveInterval()

    @staticmethod
    def rpcSetKeepaliveInterval(keepaliveInterval):
        return jmriRpcShimAPI.setKeepaliveInterval(keepaliveInterval)

    @staticmethod
    def rpcGetFile(fileName):
        try:
            f = open(fileName, "r")
            fileContent = f.read()
            f.close()
            return fileContent
        except:
            return None

    @staticmethod
    def rpcListDir(path):
        try:
            return os.listdir(path)
        except:
            return None
# END <jmriRpcShimAPI> -------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# jmriListener
# Description: Provides a listener which emulates RPC client callbacks at JMRI object state changes
#################################################################################################################################################
class jmriListener():
    keepAlivePeriod = jmriAPIShim.getKeepaliveInterval()
    stateChangeSem = threading.Semaphore(0)
    cbWaiting = False
    stateChangeList = []
    

    @staticmethod
    def start():
        trace.notify(DEBUG_INFO, "Starting the JMRI object state listener, used for virtual RPC callbacks")
        jmriRpcServer.regFn(jmriListener.rpcListen)
        jmriRpcServer.regFn(jmriListener.rpcUnListen)
        jmriRpcServer.regCbFn(jmriListener.rpcOnStateChange)
        jmriRpcServer.regFn(jmriListener.rpcOnStateChangeRelease)
        jmriRpcServer.regFn(jmriListener.rpcOnStateChangePurge)
        jmriListener.keepaliveTimerHandle = threading.Timer(jmriListener.keepAlivePeriod, jmriListener.keepAlive)
        jmriListener.keepaliveTimerHandle.start()

    @staticmethod
    def rpcListen(type, sysName):
        try:
            trace.notify(DEBUG_INFO, "Adding a JMRI object state listener for: " + jmriObj.getObjTypeStr(type) + ":" + sysName)
            for cb in getObjType(type).getBySystemName(sysName).getPropertyChangeListeners():
                if str(cb) == str(jmriListener.onStateChange):
                    trace.notify(DEBUG_INFO, "JMRI object state listener not added for: " + jmriObj.getObjTypeStr(type) + ":" + sysName + " - already existing")
                    return rc.OK
            getObjType(type).getBySystemName(sysName).addPropertyChangeListener(jmriListener.onStateChange)
            return rc.OK
        except:
            trace.notify(DEBUG_ERROR, "Could not add JMRI state listener for:" + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def rpcUnListen(type, sysName):
        trace.notify(DEBUG_INFO, "Removing JMRI object state listener for: " + jmriObj.getObjTypeStr(type) + ":" + sysName)
        try:
            getObjType(type).getBySystemName(sysName).removePropertyChangeListener(jmriListener.onStateChange)
            return rc.OK
        except:
            #trace.notify(DEBUG_ERROR, "Could not remove JMRI state listener for:" + jmriObj.getObjTypeStr(type) + ":" + sysName + " - " + rc.getErrStr(rc.GEN_ERR) + ", Details:\n" + str(traceback.format_exception(*sys.exc_info())))
            return rc.GEN_ERR

    @staticmethod
    def onStateChange(event):
        type = event.source.beanType
        sysName = event.source.systemName
        usrName = event.source.userName
        newState = event.newValue
        oldState = event.oldValue
        trace.notify(DEBUG_VERBOSE, "Got a JMRI object state listener state change from: " + type + ":" + sysName + "new state: " + state2stateStr(jmriObj.getGenJMRITypeFromJMRIType(type), sysName, event.newValue) + ", old state: " + state2stateStr(jmriObj.getGenJMRITypeFromJMRIType(type), sysName, event.oldValue))
        response = {"stateChange" : {"objType" : type, "sysName" : sysName, "usrName" : usrName, "newState" : state2stateStr(jmriObj.getGenJMRITypeFromJMRIType(type), sysName, event.newValue), 
                                                                                                 "oldState" : state2stateStr(jmriObj.getGenJMRITypeFromJMRIType(type), sysName, event.oldValue)}}
        jmriListener.stateChangeList.append(dicttoxml(dictEscapeing.dictEscape(response)))
        jmriListener.stateChangeSem.release()

    @staticmethod
    def rpcOnStateChangePurge():
        trace.notify(DEBUG_INFO, "Purging the JMRI object state listener call back queue")
        jmriListener.stateChangeList = []
        return rc.OK

    @staticmethod
    def rpcOnStateChangeRelease():
        trace.notify(DEBUG_INFO, "Releasing the JMRI object state listener call back session")
        response = {"release" : " "}
        jmriListener.stateChangeList.append(dicttoxml(dictEscapeing.dictEscape(response)))
        jmriListener.stateChangeSem.release()
        return rc.OK

    @staticmethod
    def rpcOnStateChange():
        trace.notify(DEBUG_VERBOSE, "Client is waiting for a callback")
        jmriListener.cbWaiting = True
        jmriListener.stateChangeSem.acquire()
        trace.notify(DEBUG_VERBOSE, "A callback - either from a release event, a keepalive event, or a JMRI object state change event is sent")
        jmriListener.cbWaiting = False
        return jmriListener.stateChangeList.pop(0)

    @staticmethod
    def keepAlive():
        trace.notify(DEBUG_VERBOSE, "A keepalive event is queued")
        print(">>>>>>>>>>>>>>>>>>PING>>>>>>>>>>>>>>>")
        jmriListener.keepaliveTimerHandle = threading.Timer(jmriListener.keepAlivePeriod, jmriListener.keepAlive)
        jmriListener.keepaliveTimerHandle.start()
        if jmriListener.cbWaiting:
            keepAliveMsg = {"keepAlive":""}
            jmriListener.stateChangeList.append(dicttoxml(dictEscapeing.dictEscape(keepAliveMsg)))
            jmriListener.stateChangeSem.release()
# END <jmriListener> ----------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# jmriRpcServer
# Description: Handles the xml-RPC server
#################################################################################################################################################
class jmriRpcServer(): #Make this static

    @staticmethod
    def start(listenInterface="localhost", listenPortBase=8000):
        trace.notify(DEBUG_INFO, "Starting RPC server, listening on " + listenInterface + ":" + str(listenPortBase) + " and " + listenInterface + ":" + str(listenPortBase + 1))
        jmriRpcServer.rpcEndPoint = SimpleXMLRPCServer((listenInterface, listenPortBase))
        jmriRpcServer.rpcCbEndPoint = SimpleXMLRPCServer((listenInterface, listenPortBase + 1))
        jmriRpcServer.rpcPollThread = threading.Thread(target=jmriRpcServer.rpcEndPoint.serve_forever)
        jmriRpcServer.rpcPollThread.start()
        jmriRpcServer.rpcCbPollThread = threading.Thread(target=jmriRpcServer.rpcCbEndPoint.serve_forever)
        jmriRpcServer.rpcCbPollThread.start()

    @staticmethod
    def stop():
        pass                #Need to define stop and create an exemption handler calling stop

    @staticmethod
    def regFn(fn):
        trace.notify(DEBUG_INFO, "Register RPC function " + str(fn))
        jmriRpcServer.rpcEndPoint.register_function(fn)

    @staticmethod
    def regCbFn(fn):
        trace.notify(DEBUG_INFO, "Register RPC call-back function " + str(fn))
        jmriRpcServer.rpcCbEndPoint.register_function(fn)
# END <jmriRpcServer> ---------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# monitor
# The RPC monitoring loop
#################################################################################################################################################
def monitor():
    trace.start()
    jmriRpcServer.start(listenInterface="127.0.0.2")
    global MQTT
    MQTT = jmri.InstanceManager.getDefault(jmri.jmrix.mqtt.MqttSystemConnectionMemo).getMqttAdapter()
    jmriRpcShimAPI.start()
    jmriListener.start()
    while True:
        if not jmriRpcServer.rpcPollThread.is_alive() or not jmriRpcServer.rpcCbPollThread.is_alive():
            trace.notify(DEBUG_PANIC, "RPC poll thread has unexpectedly terminated")
        time.sleep(1)

if __name__ == "__main__":
    threading.Thread(target=monitor).start()
