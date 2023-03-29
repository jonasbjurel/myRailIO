#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A JMRI server-side script to control the generic JMRI MQTT signals, lights, turn-outs, sensosr and actuators
# decoders as defined in here: https://github.com/jonasbjurel/GenericJMRIdecoder
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################


import os
import sys
import time
import xml.etree.ElementTree as ET
import xml.dom.minidom
from org.python.core.util import StringUtil
import inspect
import time
import threading
import traceback
import uuid
currentdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))
print(currentdir)
#sys.path.append(currentdir + "\\lib")
print(sys.path)
from dict2XML import *
import xml2dict


# ==============================================================================================================================================
# Constants
# ==============================================================================================================================================
# ----------------------------------------------------------------------------------------------------------------------------------------------
# System constants
# ----------------------------------------------------------------------------------------------------------------------------------------------
DEFAULT_DECODER_KEEPALIVE_PERIOD =       1
DEFAULT_RPC_KEEPALIVE_PERIOD =       1



# ----------------------------------------------------------------------------------------------------------------------------------------------
# Application constants
# ----------------------------------------------------------------------------------------------------------------------------------------------

# Config.xml variable
# -------------------
CONFIGXMLVAR = "IMgenericDecoderConfigXml"

# Southbound MQTT Topics
# -----------
MQTT_DISCOVERY_REQUEST_TOPIC = "track/discoveryreq/"
MQTT_DISCOVERY_RESPONSE_TOPIC = "track/discoveryres/"
MQTT_PING_UPSTREAM_TOPIC = "track/decoderSupervision/upstream/"
MQTT_PING_DOWNSTREAM_TOPIC = "track/decoderSupervision/downstream/"
MQTT_CONFIG_TOPIC = "track/decoderMgmt/"
MQTT_OPSTATE_TOPIC = "track/opState/"
MQTT_LOG_TOPIC = "track/log/"
MQTT_ASPECT_TOPIC = "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC = "track/turnout/"
MQTT_LIGHT_TOPIC = "track/light/"
MQTT_SATLINK_ADMBLOCK_TOPIC = "track/sateliteLink/admblock/"
MQTT_SATLINK_OPBLOCK_TOPIC = "track/sateliteLink/opblock/"
MQTT_SAT_ADMBLOCK_TOPIC = "track/satelite/admblock/"
MQTT_SAT_OPBLOCK_TOPIC = "track/satelite/opblock/"
MQTT_SAT_PANIC_TOPIC = "track/satelite/panic/" # IS NOT USED

# MQTT Payload
# ------------
DISCOVERY = "<DISCOVERY_REQUEST/>"
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
FAULT_ASPECT = "<Aspect>FAULT</Aspect>"
NOFAULT_ASPECT = "<Aspect>NOFAULT</Aspect>"
PING = "<Ping/>"
ADM_BLOCK = "<AdmState>BLOCK</AdmState>"
ADM_DEBLOCK = "<AdmState>DEBLOCK</AdmState>"
PANIC_MSG = "<PANIC/>"
NOPANIC_MSG = "<NOPANIC/>"


# Northbound MQTT API topics
# --------------------------
MQTT_NB_API_REQ = "genJMRIDecoder/NbAPI/req/"
MQTT_NB_API_RESP = "genJMRIDecoder/NbAPI/resp/"
MQTT_NB_API_ALERT = "genJMRIDecoder/NbAPI/alert/"


# XML parse filters
# -----------------
MANSTR = {"expected" : True, "type" : "str", "values" : None, "except": "PANIC"}
OPTSTR = {"expected" : None, "type" : "str", "values" : None, "except": "PANIC"}
MANINT = {"expected" : True, "type" : "int", "values" : None, "except": "PANIC"}
OPTINT = {"expected" : None, "type" : "int", "values" : None, "except": "PANIC"}
MANFLOAT = {"expected" : True, "type" : "float", "values" : None, "except": "PANIC"}
OPTFLOAT = {"expected" : None, "type" : "float", "values" : None, "except": "PANIC"}
MANSPEED = {"expected" : True, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANSPEED = {"expected" : None, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANLVL = {"expected" : True, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
OPTLVL = {"expected" : None, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
MANSPEED = {
    "expected": True,
    "type": "str",
    "values": ["SLOW", "NORMAL", "FAST"],
    "except": "PANIC",
}
OPTSPEED = {
    "expected": None,
    "type": "str",
    "values": ["SLOW", "NORMAL", "FAST"],
    "except": "PANIC",
}
MANLVL = {
    "expected": True,
    "type": "str",
    "values": ["LOW", "NORMAL", "HIGH"],
    "except": "PANIC",
}
OPTLVL = {
    "expected": None,
    "type": "str",
    "values": ["LOW", "NORMAL", "HIGH"],
    "except": "PANIC",
}

# ==============================================================================================================================================
# Helper fuction: parse_xml
# Purpose: Parse xmlTrees and find keys, atributes and values
# ==============================================================================================================================================
def parse_xml(xmlTree,tagDict) :
#   tagDict: {"Tag" : {"expected" : bool, "type" : "int/float/int", "values" : [], "except": "error/panic/info}, ....}
    res = {}
    for child in xmlTree :
        tagDesc = tagDict.get(child.tag)
        if tagDesc == None :
            continue
        value = validateXmlText(child.text, tagDesc.get("type"), tagDesc.get("values"))
        if tagDesc.get("expected") == None :
            if value != None :
                res[child.tag] = value
            else : 
                continue
        elif tagDesc.get("expected") == True : 
            if value != None :
                res[child.tag] = value
            else : through_xml_error(tagDesc.get("except"), "Tag: " + child.tag + " Tag.text: " + child.text + " did not pass type checks")
        else : through_xml_error(tagDesc.get("except"), "Tag: " + child.tag + " was not expected")

    for tag in tagDict :
        if tagDict.get(tag).get("expected") != None :
            if res.get(tag) == None :
                if tagDict.get(tag).get("expected") == True : through_xml_error(tagDict.get(tag).get("except"), "Tag: " + tag + " was expected but not found")
    return res

def validateXmlText(txt, type, values) :
    if txt == None :
        return None
    if type == None :
        return txt.strip()
    elif type == "str" : res = str(txt).strip()
    elif type == "int" : res = int(txt)
    elif type == "float" : res = float(txt)
    else : 
        return None

    if values == None :
        return res
    else :
        for value in values :
            if res == value :
                return res
        return None

def through_xml_error(_except, errStr) :
    if _except == "DEBUG_VERBOSE" : debug = DEBUG_VERBOSE
    elif _except == "DEBUG_TERSE" : debug = DEBUG_TERSE
    elif _except == "INFO" : debug = INFO
    elif _except == "ERROR" : debug = ERROR
    elif _except == "PANIC" : debug = PANIC
    else : debug = PANIC
    notify(None, debug, errStr)
    return RC_OK


# ==============================================================================================================================================
# Helper sm fuctions: decode system name
# Purpose: Decode system name and provide embedded properties
# Methods:
# smIsVirtual(...):
#           Returns True if the Signalmast is virtual
# smLgAddr(...):
#           Returns the signalmast lg address
# smType(...)
#           Returns the signalmast type
# ==============================================================================================================================================
def smIsVirtual(systemName):
    return __decodeSmSystemName(systemName)[0]

def smLgAddr(systemName):
    return __decodeSmSystemName(systemName)[1]

def smType(systemName):
    return __decodeSmSystemName(systemName)[2]

# returns [isVirtual, lgAddr, smType]
def __decodeSmSystemName(systemName):  # Refactoring needed
    if str(systemName).index("IF$vsm:") == 0:
        isVirtual = True
    else:
        isVirtual = False
    delimit = str(systemName).index("($")
    cnt = 0
    mastType = ""
    lgAddrStr = ""
    for x in str(systemName):
        cnt += 1
        if cnt > 7 and cnt <= delimit:
            mastType = mastType + x
        if cnt > delimit + 2 and x != ")":
            lgAddrStr = lgAddrStr + x
    return [isVirtual, int(lgAddrStr), mastType]

# ==============================================================================================================================================
# Helper actuator fuctions: decode system name
# Purpose: Decode system name and provide embedded properties
# Methods:
# smIsVirtual(...):
#           Returns True if the Signalmast is virtual
# smLgAddr(...):
#           Returns the signalmast lg address
# smType(...)
#           Returns the signalmast type
# ==============================================================================================================================================
def actType(systemName):
    return __decodeActSystemName(systemName)[0]

def actIsMqtt(systemName):
    return __decodeActSystemName(systemName)[1]

def actAddrStr(systemName):
    return __decodeActSystemName(systemName)[2]

# returns [isVirtual, lgAddr, smType]
def __decodeActSystemName(systemName):  # Refactoring needed
    if str(systemName)[1] == "T":
        type = "TURNOUT"
    elif str(systemName)[1] == "L":
        type = "GENERIC"
    else:
        type = "-"
    if str(systemName)[0] == "M":
        mqtt = True
    else:
        mqtt = False

    cnt = 0
    actAddrStr = ""
    for x in str(systemName):
        if cnt > 1:
            actAddrStr = actAddrStr + x
        cnt += 1
    return [type, mqtt, actAddrStr]




# ==============================================================================================================================================
# Class: resourceTracker
# Purpose: Implements a generic (port) resource tracker with the purpose to allocate, de-allocate (port) resources residing on arbitrary
# hierachys and hierarch levels.
# A tree of multiple branches defines the resource map, there can be arbitrary numbers of branches and trunks. the branch depth is arbitrary,
# but the leafs are always represented by a list of arbitrary object types which has allocated the (port-) resource represented by each list
# position.
#
# Data structures: self.resourceMap - a multi-dimentional dictionary representing the resource map:
#                  Ie
#                E.g. {"TopDecoder":{"SlaveDecoder"..:[Resource_0_Owner, Resource_1_Owner, Resource_2_Owner, ...]}}..} 
#
# Methods:
#    __init__(): Object constructur
#
#   registerResource(regResourceMap, delete=True|False):     Register/de-Register (allocate/de-allocate) one or more resources on one and the 
#                                                            same leaf resource tree. regResourceMap must only point to one leaf trunk, but several
#                                                            resources in that leaf value resource list can be allocated/de-allocated.
#                                                            Each resource list position should indicate an arbitrary resource identifier for
#                                                            allocation/de-allocation, or None if the resource should be left untoutched.
#                                                            If Delete is srt to True, it represents a request for de-allocation, if set to False
#                                                            or ommitted it represents a request for allocation.
#                                                            registerResource({"decoder0":{satelite0:[None,None, 0,1]}) will try to
#                                                            allocate port 2 and 3 to object id 0 and 1 for leaf decoder0/satelite0/ respectively.
#                                                            registerResource({"decoder0":{satelite0:[None,None, 0,1]},delete=true) will
#                                                            instead try to de-allocate port 2 and 3 to object id 0 and 1 for leaf decoder0/satelite0/
#                                                            respectively.
#                                                            This method is thread safe and mostly atomic and fail safe in that the full 
#                                                            transaction either succeeds with expected data structure modifications, or fails 
#                                                            in its whole with no changes of the data structures.
#                                                            regResourceMap must be a dictionary that uniquely points to one leaf trunk, it cannot 
#                                                            point to multiple trunks, but it can operate on multiple resources on leaf trunks.
#       Returns: [list]                                      Returns a list, where the first element contains the return code, and the re,aining
#                                                            potentially objects that were occuping resources preventing to allocate one or more
#                                                            resources, or that represented an object missmatch when trying to deallocate resources
#
# getResource(resourceMapLookup):                            Provides a list of resource ellements corresponding to the resourceMapLookup leaf trunk.
#       Returns: [list]                                      Returns a list, where the first element contains the return code, and the remaining a
#                                                            a list of the resource element's object owner.
#
# getResources:                                              Provides a full resource map.
#       Returns [dict]                                       Returns the full resource map dict.
# ==============================================================================================================================================
class resourceTracker:
    def __init__(self):
        self.resourceMap = {}
        self.transactId = 0
        self.lock = threading.Lock()

    def registerResource(self, regResourceMap, delete = False, resourceMap = None, topRecurse = True):
        tmpResourceMap = []
        returnCode =[]
        returnCode.append(RC_OK)
        if topRecurse:
            self.lock.acquire()
            resourceMap = self.resourceMap
            notify(self, DEBUG_TERSE, "Request for allocating/de-allocating resources:" +
                               str(regResourceMap) +
                               " ,delete request:" +
                               str(delete) +
                               " with transaction Id:" +
                               str(self.transactId))
            self.transactId += 1
            if self.__checkResourceMapParam(regResourceMap) != RC_OK:
                notify(self, ERROR, "Parameter error, the regResourceMap parameter either have multiple keys per level, "
                       + "or the the last level value is not a list")
                self.lock.release()
                return[RC_PARAM_ERR]
        if list(regResourceMap)[0] in resourceMap:
            if isinstance(regResourceMap[list(regResourceMap)[0]], dict) and isinstance(resourceMap[list(regResourceMap)[0]], dict):
                notify(self, DEBUG_VERBOSE, "Itterating the resourceMap resource data structure -" +
                      " recursive registerResource call using key: "
                      + str(regResourceMap[list(regResourceMap)[0]]))
                returnCode = self.registerResource(regResourceMap[list(regResourceMap)[0]], delete, resourceMap[list(regResourceMap)[0]], False)
                if not topRecurse:
                    return returnCode
                if delete and returnCode[0] == RC_OK:
                    self.__houseKeep()
                self.lock.release()
                return returnCode
            elif isinstance(regResourceMap[list(regResourceMap)[0]], list) and isinstance(resourceMap[list(regResourceMap)[0]], list):
                if not delete:
                    notify(self, DEBUG_VERBOSE, "Found requested regResourceMap key: " +
                           str(list(regResourceMap)[0]) + 
                           ", trying to allocate resources: " +
                           str(regResourceMap[list(regResourceMap)[0]]))
                else:
                    notify(self, DEBUG_VERBOSE, "Found requested regResourceMap key: " +
                           str(list(regResourceMap)[0]) + 
                           ", trying to de-allocate resources: " +
                           str(regResourceMap[list(regResourceMap)[0]]))
                returnCode[0] = RC_OK
                tmpResourceMap = list(resourceMap[list(resourceMap)[0]])
                for cnt in range(len(regResourceMap[list(regResourceMap)[0]])):
                    if cnt > len(tmpResourceMap) - 1:
                        tmpResourceMap.append(None)
                    if regResourceMap[list(regResourceMap)[0]][cnt] != None:
                        if not delete:
                            if tmpResourceMap[cnt] == None:
                                tmpResourceMap[cnt] = regResourceMap[list(regResourceMap)[0]][cnt]
                                notify(self, DEBUG_TERSE,
                                       "resource: " +
                                        str(cnt) +
                                        " was successfully allocated by object: " +
                                        str(tmpResourceMap[cnt]))
                            else:
                                returnCode[0] = RC_BUSY
                                returnCode.append(tmpResourceMap[cnt])
                                notify(self, ERROR,
                                       "resource: " +
                                       str(cnt) +
                                       " was requested to be allocated by object: " +
                                       str(regResourceMap[list(regResourceMap)[0]][cnt]) +
                                        " but was busy, - already allocated by object: " +
                                        str(tmpResourceMap[cnt]))
                        else:
                            if regResourceMap[list(regResourceMap)[0]][cnt] == tmpResourceMap[cnt]:
                                tmpResourceMap[cnt] = None
                                notify(self, DEBUG_TERSE,
                                       "resource: " +
                                       str(cnt) +
                                       " was successfully de-allocated by object: " +
                                       str(regResourceMap[list(regResourceMap)[0]][cnt]))
                            else:
                                returnCode[0] = RC_NOT_FOUND
                                returnCode.append(resourceMap[list(regResourceMap)[0]][cnt])
                                notify(self, ERROR,
                                       "resource: " +
                                       str(cnt) +
                                       " was requested to be de-allocated by object: " +
                                       str(regResourceMap[list(regResourceMap)[0]][cnt]) +
                                       " but was allocated by another object: " +
                                       str(tmpResourceMap[cnt]))
                if returnCode[0] == RC_OK:
                    resourceMap[list(regResourceMap)[0]] = tmpResourceMap
                    notify(self, DEBUG_TERSE, "All resources for registerResource Transaction id: " +
                            str(self.transactId) +
                            " were successfully allocated/de-allocated")
                    if delete and topRecurse:
                        self.__houseKeep()
                else:
                    notify(self, ERROR, "Some resources for registerResource Transaction id: " +
                           str(self.transactId) +
                           " could not be allocated/de-allocated - aborting the entire transaction")
                if topRecurse:
                    self.lock.release()
                return returnCode
            else:
                if topRecurse:
                    self.lock.release()
                return [RC_PARAM_ERR]
        elif delete:
            notify(self, ERROR, "Could not de-allocate resource, resource not found for registerResource Transaction id: " +
                           str(self.transactId) +
                           " - aborting the entire transaction")
            if topRecurse:
                self.lock.release()
            return [RC_NOT_FOUND]
        else:
            resourceMap[list(regResourceMap)[0]] = regResourceMap[list(regResourceMap)[0]]
            if topRecurse:
                self.lock.release()
            return [RC_OK]

    def getResource(self, resourceMapLookup, resourceMap = None, topRecurse = True):
        returnCode = []
        returnCode.append(RC_OK)
        if topRecurse:
            resourceMap = self.resourceMap
            self.lock.acquire()
        if len(list(resourceMapLookup)) != 1:
            returnCode[0] = RC_PARAM_ERR
            if topRecurse:
                self.lock.release()
            return returnCode
        try:
            resourceMap[list(resourceMapLookup)[0]]
        except:
            returnCode[0] = RC_NOT_FOUND
            if topRecurse:
                self.lock.release()
            return returnCode
        if isinstance(resourceMap[list(resourceMapLookup)[0]], list):
            returnCode.append(resourceMap[list(resourceMapLookup)[0]])
            if topRecurse:
                self.lock.release()
            return returnCode
        elif isinstance(resourceMap[list(resourceMapLookup)[0]], dict):
            if topRecurse:
                self.lock.release()
            return self.getResource(resourceMapLookup[list(resourceMapLookup)[0]], resourceMap[list(resourceMapLookup)[0]], False)
        else:
            returnCode[0] = RC_GEN_ERR
            if topRecurse:
                self.lock.release()
            return returnCode

    def getResources(self):
        return self.resourceMap

    def __checkResourceMapParam(self, resourceMapParam):
        if len(list(resourceMapParam)) != 1:
            return RC_PARAM_ERR
        if isinstance(resourceMapParam[list(resourceMapParam)[0]], dict):
            return self.__checkResourceMapParam(resourceMapParam[list(resourceMapParam)[0]])
        elif isinstance(resourceMapParam[list(resourceMapParam)[0]], list):
            return RC_OK
        else:
            return RC_PARAM_ERR

    def __houseKeep(self, resourceMap = None, topRecurse = True):
        if topRecurse:
            resourceMap = self.resourceMap
        if resourceMap == None:
            return RC_OK
        for branch in list(resourceMap):
            if isinstance(resourceMap[branch], dict):
                rc = self.__houseKeep(resourceMap[branch], False)
                if rc != RC_OK:
                    return rc
                if resourceMap[branch] == {}:
                    resourceMap.pop(branch)
            elif isinstance(resourceMap[branch], list):
                allResourcesFree = True
                for resource in resourceMap[branch]:
                    if resource != None:
                        allResourcesFree = False
                if allResourcesFree:
                    resourceMap.pop(branch)
            else:
                return RC_GEN_ERR
        return RC_OK

    def __del__(self):
        pass

def getOpBlockStateStr(opBlockState):
    if not opBlockState:
        return OP_WORKING_STR
    opBlockStateStr = ""
    if opBlockState & OP_INIT:
        opBlockStateStr = opBlockStateStr + OP_INIT_STR + ","
    if opBlockState & OP_CONFIG:
        opBlockStateStr = opBlockStateStr + OP_CONFIG_STR + ","
    if opBlockState & OP_DISABLE:
        opBlockStateStr = opBlockStateStr + OP_DISABLE_STR + ","
    if opBlockState & OP_CONTROLBLOCK:
        opBlockStateStr = opBlockStateStr + OP_CONTROLBLOCK_STR + ","
    if opBlockState & OP_ERRSEC:
        opBlockStateStr = opBlockStateStr + OP_ERRSEC_STR + ","
    if opBlockState & OP_UNUSED:
        opBlockStateStr = opBlockStateStr + OP_UNUSED_STR + ","
    if opBlockState & OP_FAIL:
        opBlockStateStr = opBlockStateStr + OP_FAIL_STR + ","
    if opBlockState & OP_UNAVAIL:
        opBlockStateStr = opBlockStateStr + OP_UNAVAIL_STR + ","
    return opBlockStateStr[:-1]

def getAdmStateStr(admState):
    if admState == ADM_ENABLE:
        return "ENABLE"
    elif admState == ADM_DISABLE:
        return "DISABLE"
    else:
        return "ERROR"

# ==============================================================================================================================================
# Class: topDeoderCordidinator
# Purpose:  The top decoder coordinator owns the top configuration common to all decoders, coordinates the collection of configuration data and
#           the provisioning of configuration of the decoders, the lightgroups, turnouts, sensors, etc. And coordinates bindings and start
#           of the decoders, lightgroups, turnpouts etc.
#
# Data structures: decoderTable: {decoderURI : decoderObject}, lightgroups - a handle to the light groups object,
# xml configuration fragment:
# 	<Top>
# 		<Author>Jonas Bjurel</Author>
# 		<Version>0.1</Version><Date>2021-03-31</Date>
# 		<Description>My decoder</Description>
# 		<ServerURI>lightcontroller1.bjurel.com</ServerURI>
# 		<ClientURL>lightcontroller1.bjurel.com</ClientURL>
# 		<NTPServer>pool.ntp.org</NTPServer>
# 		<TimeZoneGmtOffset>+1</TimeZoneGmtOffset>
# 		<RSyslogServer>jmri.bjurel.com</RSyslogServer>
# 		<LogLevel>INFO</LogLevel>
# 	</Top>
#
# Methods:
#   getXmlConfigTree(...)
#           purpose: provides the xml configuration for a decoder.
#   handleDecoderFault(...)
#           purpose: Coordinates the overall behaviour at a decoder fault
#   handleDecoderRecovery(...)
#           purpose: Coordinates the overall behaviour at a decoder recovery
#
# Operational states: NEEDS TO BEE AAAAAAADED FOR EVERY THING
#
# Concurrency and protected resources: NEEDS TO BEE AAAAAAADED FOR EVERY THING
#
# High level sequence diagram:
#
# topDeoderCordidinator             decoder             lightgroups             lightgroup/mast
#         !                            !                     !                        !
#      (init)                          !                     !                        !
#  (parse topXML)                      !                     !                        !
#         !------(configure)-----------+-------------------->!                        !
#         !                            !           (parse lightgroupsXML)             !
#         !                            !                     !-----(configure)------->!
#         !                            !                     !             (parse lightgroup/mastXML)
#         !                            !                     !<-----------------------!
#         !<---------------------------+---------------------!                        !
#         !-----(getDecoderURIs)-------+-------------------->!                        !
#         !                            !                     !----(getDecoderURI)---->!
#         !                            !                     !<-----------------------!
#         !<---------------------------+---------------------!                        !
# (buid Decoder table)                 !                     !                        !
#         !                            !----(register)------>!                        !
#         !                            !                     !------(register)------->!
#         !                            !                     !<-----------------------!
#         !                            +<--------------------!                        !
#         !<----(getXmlConfigTree)-----!                     !                        !
#         !-----(getXmlConfigTree)-----+-------------------->!                        !
#         !                            !                     !---(getXmlConfigTree)-->!
#         !                            !                     !<-----------------------!
#         !<---------------------------+---------------------!                        !
#         !--------------------------->!                     !                        !
#         !           (send xml configuration to decoder)    !                        !
#
# ==============================================================================================================================================
class topDeoderCordidinator:
    @staticmethod
    def start():
        topDeoderCordidinator.lightgroupTopology = None
        topDeoderCordidinator.actuatorTopology = None
        topDeoderCordidinator.sensorTopology = None
        topDeoderCordidinator.state = systemState()
        topDeoderCordidinator.decoderTable = {}
        topDeoderCordidinator.faultyDecoders = []
        topDeoderCordidinator.powerOffAtFault = True
        topDeoderCordidinator.lock = threading.Lock()
        topDeoderCordidinator.genDecoderXmlFile = jmriRpcClient.getStateBySysName(jmriObj.MEMORIES, CONFIGXMLVAR)
        if type(topDeoderCordidinator.genDecoderXmlFile) != str:
                trace.notify(PANIC, "No genericDecoderConfigXml file defined - please define the genericDecoderConfigXml in JMRI variable with system name: " +
                       CONFIGXMLVAR + " - exiting...")
        trace.notify(INFO, "Using genericDecoderConfigXml file :" + topDeoderCordidinator.genDecoderXmlFile)
        topDeoderCordidinator.__configure()
        topDeoderCordidinator.__start()

    @staticmethod
    def __configure(self):
        trace.notify(DEBUG_TERSE, "Starting to configure top decoder class and all subordinate class objects from \"" + 
               topDeoderCordidinator.genDecoderXmlFile + "\" and JMRI meta data")
        if not os.path.exists(topDeoderCordidinator.genDecoderXmlFile):
            trace.notify(PANIC, "File " + topDeoderCordidinator.genDecoderXmlFile + " does not exist")
            state.setOpStateDetail(OP_FAIL)
            return rc.DOES_NOT_EXIST
        try:
            controllersXmlTree = ET.parse(topDeoderCordidinator.genDecoderXmlFile)
        except:
            trace.notify(PANIC, "Error parsing file " + topDeoderCordidinator.genDecoderXmlFile)
            state.setOpStateDetail(OP_FAIL)
            return rc.PARSE_ERR

        if str(controllersXmlTree.getroot().tag) != "Decoders":
            trace.notify(PANIC, "Controllers .xml  missformated:\n")
            state.setOpStateDetail(OP_FAIL)
            return rc.PARSE_ERR
        else:
            discoveryConfig = ET.Element("DiscoveryResponse")
            for decoder in controllersXmlTree.getroot():
                if decoder.tag == "Decoder":
                    discoveryDecoder =  ET.SubElement(discoveryConfig, "Decoder")
                    decoderConfig = parse_xml(decoder, {"MAC":MANSTR, "URI":MANSTR})
                    discoverymac = ET.SubElement(discoveryDecoder, "MAC")
                    discoverymac.text = decoderConfig.get("MAC")
                    discoveryuri = ET.SubElement(discoveryDecoder, "URI")
                    discoveryuri.text = decoderConfig.get("URI")
            self.discoveryConfigXML = ET.tostring(discoveryConfig, method="xml").decode()
            trace.notify(self, DEBUG_TERSE, "Discovery response xml configuration created: \n" +
                   xml.dom.minidom.parseString(self.discoveryConfigXML).toprettyxml())
            # Set Top-decoder defaults
            topDeoderCordidinator.xmlDescription = ""
            topDeoderCordidinator.xmlDate = ""
            topDeoderCordidinator.TimeZone = 0
            topDeoderCordidinator.RsyslogReceiver = ""
            topDeoderCordidinator.NTPServer = None
            topDeoderCordidinator.TimeZone = 0
            topDeoderCordidinator.RsyslogReceiver = None
            topDeoderCordidinator.LoglevelStr = "DEBUG-INFO"
            topDeoderCordidinator.powerOffAtFault = False
            topDeoderCordidinator.disableDecodersAtFault = False
            topDeoderCordidinator.disableAllDecodersAtFault = False
            topDeoderCordidinator.pingPeriod = DEFAULT_DECODER_KEEPALIVE_PERIOD
            topDecoderXmlConfig = parse_xml(
                controllersXmlTree.getroot(),
                {
                    "Author": OPTSTR,
                    "Description": OPTSTR,
                    "Version": OPTSTR,
                    "Date": OPTSTR,
                    "NTPServer": OPTSTR,
                    "TimeZoneGmtOffset": OPTINT,
                    "RSyslogServer": OPTSTR,
                    "LogLevel": OPTSTR,
                    "PowerOffAtFault" : OPTSTR,
                    "DisableAllDecodersAtFault" : OPTSTR, 
                    "PingPeriod" : OPTFLOAT
                },
            )
            if topDecoderXmlConfig.get("Author") != None: topDeoderCordidinator.xmlAuthor = topDecoderXmlConfig.get("Author")
            if topDecoderXmlConfig.get("Description") != None : topDeoderCordidinator.xmlDescription = topDecoderXmlConfig.get("Description")
            if topDecoderXmlConfig.get("Version") != None: topDeoderCordidinator.xmlVersion = topDecoderXmlConfig.get("Version")
            if topDecoderXmlConfig.get("Date") != None: topDeoderCordidinator.xmlDate = topDecoderXmlConfig.get("Date")
            if topDecoderXmlConfig.get("NTPServer") != None: topDeoderCordidinator.NTPServer = topDecoderXmlConfig.get("NTPServer")
            if topDecoderXmlConfig.get("TimeZoneGmtOffset") != None: topDeoderCordidinator.TimeZone = str(topDecoderXmlConfig.get("TimeZoneGmtOffset"))
            if topDecoderXmlConfig.get("RsyslogReceiver") != None: topDeoderCordidinator.RsyslogReceiver = topDecoderXmlConfig.get("RsyslogReceiver")
            if topDecoderXmlConfig.get("LogLevel") != None:
                topDeoderCordidinator.logLevelStr = topDecoderXmlConfig.get("LogLevel")
                if getSeverityFromSeverityStr(topDeoderCordidinator.logLevelStr) == None:
                    trace.notify(self, DEBUG_ERROR, "Specified debug-level is not valid, will use default debug-level")
                else:
                    trace.setGlobalDebugLevel(getSeverityFromSeverityStr(topDeoderCordidinator.logLevelStr))
                    jmriRpcClient.setGlobalDebugLevelStr(topDeoderCordidinator.logLevelStr)
            if topDecoderXmlConfig.get("PowerOffAtFault") != None: 
                if topDecoderXmlConfig.get("PowerOffAtFault") == "Yes": 
                    self.powerOffAtFault = True
                elif topDecoderXmlConfig.get("PowerOffAtFault") == "No": 
                    self.powerOffAtFault = False
                else: notify(self, INFO, "\"PowerOffAtFault\" not set to yes/no, setting it to no")
            else:  notify(self, INFO, "\"PowerOffAtFault\" not set, setting it to no")
            self.disableDecodersAtFault = False
            if topDecoderXmlConfig.get("DisableAllDecodersAtFault") != None: 
                if topDecoderXmlConfig.get("DisableAllDecodersAtFault") == "Yes": 
                    topDeoderCordidinator.disableAllDecodersAtFault = True
                elif topDecoderXmlConfig.get("DisableAllDecodersAtFault") == "No":
                    topDeoderCordidinator.disableAllDecodersAtFault = False
                else: trace.notify(INFO, "\"DisableAllDecodersAtFault\" not set to yes/no, setting it to no")
            else: trace.notify(INFO, "\"DisableAllDecodersAtFault\" not set, setting it to no")
            if topDecoderXmlConfig.get("PingPeriod") != None: 
                topDeoderCordidinator.pingPeriod = int(topDecoderXmlConfig.get("PingPeriod"))
            else: notify(self, INFO, "\"PingPeriod\" not set, using default " + str(DEFAULT_DECODER_KEEPALIVE_PERIOD))
            HÄR NÅGONSTANS
            for child in controllersXmlTree.getroot():
                if str(child.tag) == "Lightgroups":
                    self.lightgroups = lightgroups(self)
                    if self.lightgroups.configure(child) != RC_OK:
                        notify(self, PANIC, "Failed to configure Lightgroups")
                        self.state.opBlock(OP_FAIL)
                        return RC_GEN_ERR
                    self.lightgroupTopology = self.lightgroups.getTopology()
                    notify(self, INFO, "Following Light Group Topology was discovered: " + str(self.lightgroupTopology))

                elif str(child.tag) == "Actuators":
                    self.actuators = actuators(self)
                    if self.actuators.configure(child) != RC_OK:
                        notify(self, PANIC, "Failed to configure Actuators")
                        self.state.opBlock(OP_FAIL)
                        return RC_GEN_ERR
                    self.actuatorTopology = self.actuators.getTopology()
                    notify(self, INFO, "Following Actuator Topology was discovered: " + str(self.actuatorTopology))

                elif str(child.tag) == "Sensors":
                    self.sensors = sensors(self)
                    if self.sensors.configure(child) != RC_OK:
                        notify(self, PANIC, "Failed to configure Sensors")
                        self.state.opBlock(OP_FAIL)
                        return RC_GEN_ERR
                    self.sensorTopology = self.sensors.getTopology()
                    notify(self, INFO, "Following Sensor Topology was discovered: " + str(self.sensorTopology))

                elif str(child.tag) == "Sounds":
                    notify(self, ERROR, "Sounds not implemented, skipping sounds")
            return RC_OK

    def __start(self):
        notify(self, DEBUG_TERSE, "Starting server side decoders")
        self.decoderTable = {}
        for type in [self.lightgroupTopology, self.actuatorTopology, self.sensorTopology]:
            if type != None:
                for decoderURI in type:
                    if not decoderURI in self.decoderTable:
                        self.decoderTable[decoderURI] = decoder(self, decoderURI)
                        self.state.addChild(self.decoderTable[decoderURI].state)
                        if self.decoderTable[decoderURI].register(self.lightgroups, self.actuators, self.sensors) != RC_OK:
                            notify(self, PANIC, "Topdecoder: Failed to register decoder: " + decoderURI)
                        if self.decoderTable[decoderURI].start() != RC_OK:
                            notify(self, PANIC, "Topdecoder: Failed to start decoder: " + decoderURI)
                        notify(self, INFO, "Topdecoder: Started server side decoder: " + decoderURI)
        notify(self, INFO, "All server side decoder instances started")
        MQTT.subscribe(MQTT_DISCOVERY_REQUEST_TOPIC, mqttListener(self.onDiscovery))
        self.state.opDeBlock(OP_INIT)
        return RC_OK

    def getXmlConfigTree(self, URI):
        try:
            notify(self, DEBUG_TERSE, "Providing top decoder over arching decoder .xml configuration")
            topXml = ET.Element("Top")
            childXml = ET.SubElement(topXml, "Author")
            childXml.text = self.xmlAuthor
            childXml = ET.SubElement(topXml, "Description")
            childXml.text = self.xmlDescription
            childXml = ET.SubElement(topXml, "Version")
            childXml.text = self.xmlVersion
            childXml = ET.SubElement(topXml, "Date")
            childXml.text = self.xmlDate
            childXml = ET.SubElement(topXml, "NTPServer")
            childXml.text = self.NTPServer
            childXml = ET.SubElement(topXml, "TimeZone")
            childXml.text = self.TimeZone
            childXml = ET.SubElement(topXml, "RsyslogReceiver")
            childXml.text = self.RsyslogReceiver
            childXml = ET.SubElement(topXml, "Loglevel")
            childXml.text = self.LoglevelStr
            childXml = ET.SubElement(topXml, "PingPeriod")
            childXml.text = str(self.pingPeriod)
            return topXml
        except:
            # ERROR PRINTOUT
            return None

    def onDiscovery(self, topic, message):
        notify(self, INFO,"Discovery request received")
        notify(self, INFO,"Delivering discovery response with global MAC-URI mappings")
        MQTT.publish(MQTT_DISCOVERY_RESPONSE_TOPIC, self.discoveryConfigXML)

    def onPanic(self, panic, objSrc, reason):
        if self.powerOffAtFault == True:
            if panic:
                try:
                    jmri.PowerManager.setPower(jmri.PowerManager.OFF) # Needs verification
                    notify(self,
                           INFO,
                           "Turning off track power due to PANIC from : " +
                           objSrc.__class__.__name__ +
                           ", reason: " +
                           reason
                           )
                except:
                    notify(self, ERROR, "Failed to power off the tracks")
            else:
                try:
                    jmri.PowerManager.setPower(jmri.PowerManager.ON) # Needs verification
                    notify(self, INFO, "Turning on track power due to panic ceaseing")
                except:
                    notify(self, ERROR, "Failed to power on the tracks")

    def onStateChange(self):
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "Top decoder coordinator down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )
        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self,
               DEBUG_TERSE,
               "Top decoder coordinator has changed operational status to: " + 
               getOpBlockStateStr(self.state.getOpState())
               )



# ==============================================================================================================================================
# Class: panic
# ==============================================================================================================================================
class panic:
    def __init__(self):
        self.cbTable = []
        self.engagedTable = {}

    def regCb(self, cb):
        if not cb in self.cbTable:
            self.cbTable.append(cb)
        return RC_OK

    def unRegCb(self, cb):
        if cb in self.cbTable:
            del self.paniccbTable[self.cbTable.index(cb)]
            return RC_OK
        else:
            return RC_NOT_FOUND

    def engage(self, objSrc, reason):
        engUuid = uuid.uuid4()
        self.engagedTable[engUuid] = [objSrc, reason]
        if len(self.engagedTable[engUuid]) == 1:
            for cb in self.cbTable:
                cb(True, objSrc, reason)
        return engUuid

    def disEngage(self, engUuid):
        if engUuid in self.engagedTable:
            if len(self.engagedTable[engUuid]) == 1:
                for cb in self.cbTable:
                    cb(False)
            del self.engagedTable[engUuid]
            return RC_OK

# ==============================================================================================================================================
# Class: decoder
# Purpose:  The decoder class is responsible fo all physical decoder configuration, communication, supervision and logging.
#
# Data structures: top: a reference handle to the top decoder coordinator object singelton
#                  lightgroupsObj: a reference to the signal groups object singelton
#                  lightgroupsTable: {<System Name" : lightgroupObj, ...} - Not used for anything currently, but maybe
#                  needed later....
#
# xml configuration fragment:
# 	<decoder>
#      fetched from other class objects
#   </decoder>
#
# Methods/callbacks:
#   start(...):
#           Configures and starts the decoder, note that it will initially be in a "disabled state" until the decoder has sent an "online"
#           MQTT indication
#   enable(...):
#           Enable the decoder, by enabling the decoder all lightgroup objects aspects will be (re)sent to the decoder and it will
#           receive any future aspect updates
#   disable(...):
#           Disabling a decoder, a special "Fault aspect" will be set, and the decoder will not receive any future aspect updates. disable()
#           is normaly initiated from the topDecoderCoordinator as a result of another fault,
#   onOpStateChange(...):
#           Decoder operational state change MQTT callback: "online/offline"
#   onPing(...):
#           A periodic ping callback has been received from the decoder
#
#
# TODO:
# ==============================================================================================================================================
class decoder:
    def __init__(self, top, URI):
        self.URI = URI # INITIATE ALL VARS
        self.state = systemState()
        self.state.regCb(self.onStateChange)
        self.state.opBlock(OP_INIT)
        self.state.admDeBlock()
        self.state.setParent(top)
        PANIC_HANDLER.regCb(self.onPanic)
        self.top = top
        self.lock = threading.Lock()

        notify(
            self,
            INFO,
            "New decoder: "
            + self.URI
            + " initialized"
        )

    def register(self, lightgroupsHandle, actuatorsHandle, sensorsHandle):
        notify(self, 
               DEBUG_TERSE,
               "Registering decoder " +
               self.URI +
               " and all its Satelite links, Satelites, Actuators, Sensors, and Light groups"
               )
        self.lightgroupsHandle = lightgroupsHandle
        self.actuatorsHandle = actuatorsHandle
        self.sensorsHandle = sensorsHandle
        self.satLinkTable = {}
        for type in [self.actuatorsHandle, self.sensorsHandle]:
            if type != None:
                for link in type.getTopology()[self.URI]:
                    if not link in self.satLinkTable:
                        self.satLinkTable[link] = sateliteLink(self, link)
                        self.state.addChild(self.satLinkTable[link].state)
                        rc = self.satLinkTable[link].register(self.actuatorsHandle, self.sensorsHandle)
                        if rc != RC_OK:
                            notify(self,
                                   ERROR,
                                   "Could not register satelite link: " +
                                   str(self.URI) + "/" +
                                   str(self.satLinkTable[link].getSatLink()) +
                                   "return code: " +
                                   str(rc)
                                   )
                            return rc
        if self.lightgroupsHandle != None:
            self.lightGroupTable = {}
            for link in self.lightgroupsHandle.getTopology()[self.URI]:
                for lightGroup in self.lightgroupsHandle.getTopology()[self.URI][link]:
                    self.lightGroupTable[lightGroup.getSystemName()] = lightGroup
                    rc = lightGroup.register(self, self.URI)
                    if rc != RC_OK:
                        notify(self,
                               ERROR,
                               "Could not register Light group: " +
                               str(self.URI) + "/" +
                               str(lightGroup.getSystemName()) +
                               "return code: " +
                               str(rc)
                               )
                        return rc
                    self.state.addChild(lightGroup.state)
        return RC_OK

    def start(self):
        notify(self, 
               DEBUG_TERSE,
               "Starting decoder " +
               str(self.URI) +
               " and all its satelite links, satelites, actuators, sensors, and light groups, " + 
               " OpState: " +
               getOpBlockStateStr(self.state.getOpState())
               )
        for link in self.satLinkTable:
            rc = self.satLinkTable[link].start()
            if rc != RC_OK:
                self.state.opBlock(OP_FAIL)
                notify(self, 
                       INFO,
                       "Decoder " +
                       str(self.URI) +
                       " Could not be started" + 
                       " OpState: " +
                       getOpBlockStateStr(self.state.getOpState()) +
                       " return code: " +
                       str(rc)
                       )
                return rc
        for lightGroup in self.lightGroupTable:
            rc = self.lightGroupTable[lightGroup].start()
            if rc != RC_OK:
                self.state.opBlock(OP_FAIL)
                notify(self, 
                       INFO,
                       "Decoder " +
                       str(self.URI) +
                       " Could not be started" + 
                       " OpState: " +
                       getOpBlockStateStr(self.state.getOpState()) +
                       " return code: " +
                       str(rc)
                       )
                return rc
            self.state.addChild(self.lightGroupTable[lightGroup].state)
        notify(self, DEBUG_TERSE, 
               "Starting compilation of decoder .xml configuration for decoder: " +
               str(self.URI))
        decoderXml = ET.Element("Decoder")
        cnt = 0
        for type in [self.top, self.lightgroupsHandle, self.actuatorsHandle, self.sensorsHandle]:
            if type != None:
                decoderXml.insert(cnt, type.getXmlConfigTree(self.URI))
                cnt += 1
        decoderXmlStr = ET.tostring(decoderXml, method="xml")

#        notify(
#               self,
#               INFO,
#               "Decoder " +
#               str(self.URI) +
#               " - XML configuration compiled, sending it to the decoder: \n" +
#               xml.dom.minidom.parseString(decoderXmlStr).toprettyxml(),
#               )

        MQTT.publish(MQTT_CONFIG_TOPIC +
                     str(self.URI) + "/",
                     decoderXmlStr
                     ) #Should this go after the subscribes?
        MQTT.subscribe(MQTT_PING_UPSTREAM_TOPIC + 
                       str(self.URI) + "/",
                       mqttListener(self.onPing))
        MQTT.subscribe(MQTT_OPSTATE_TOPIC +
                       str(self.URI) + "/",
                       mqttListener(self.onHWStateChange)
                       )
        MQTT.subscribe(MQTT_LOG_TOPIC +
                       str(self.URI) + "/",
                       mqttListener(self.onLogMessage)
                       )
        self.state.opBlock(OP_UNAVAIL)
        self.state.opDeBlock(OP_INIT)
        notify(
               self,
               INFO,
               "Decoder: " +
               str(self.URI) +
               " started"
               )
        return RC_OK

    def onHWStateChange(self, topic, payload):
        if payload == DECODER_UP:
            notify(self, INFO, "Decoder: " + str(self.URI) + " has declared it self as onLine - trying to recover it")
            self.state.opDeBlock(OP_FAIL)
        elif payload == DECODER_DOWN:
            notify(self, INFO, "Decoder: " + str(self.URI) + " has declared it self as offLine - bringing it down")
            self.state.opDeBlock(OP_FAIL)
        return RC_OK

    def onStateChange(self):
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "decoder " +
                                            str(self.URI) +
                                            "down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )
        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self, INFO,
               "Decoder: " +
               str(self.URI) +
               " has changed operational status to: " + 
               getOpBlockStateStr(self.state.getOpState())
               )

    def onPanic(self, panic):
        if panic:
            MQTT.publish(MQTT_PANIC_TOPIC + self.URI + "/", PANIC_MSG)
        else:
            MQTT.publish(MQTT_PANIC_TOPIC + self.URI + "/", NOPANIC_MSG)

    def admBlock(self):
        return self.state.admBlock()

    def admDeBlock(self):
        return self.state.admDeBlock()

    def getAdmState(self):
        return self.state.getAdmState()

    def getOpState(self):
        return self.state.getOpState()

    def getDecoderURI(self):
       try:
           return self.URI
       except:
           return None

    def onPing(self, topic, payload):
        if self.state.getOpState() == OP_UNAVAIL:
            self.__startSupervision()
        self.state.opDeBlock(OP_UNAVAIL)
        notify(self, DEBUG_VERBOSE, "Received a Ping from: " + self.URI)
        if not self.state.getOpState():
            notify(self, DEBUG_VERBOSE, "Returning the Ping from: " + self.URI)
            self.missedPingCnt = 0
            MQTT.publish(MQTT_PING_DOWNSTREAM_TOPIC + self.URI + "/", PING)

    def __onPingTimer(self):
        try:
            notify(self, DEBUG_VERBOSE, "Received a Ping timer for decoder: " + self.URI)
            action = None
            if self.missedPingCnt >= 2:
                self.state.opBlock(OP_UNAVAIL)
                self.__stopSupervision()
                notify(
                    self,
                    ERROR,
                    "Decoder "
                    + self.URI
                    + "  did not adhere to the ping supervision requirements",
                )
            else:
                self.missedPingCnt += 1
                threading.Timer(PING_INTERVAL, self.__onPingTimer).start()
        except:
            notify(self, PANIC, "Exception in ping timer handling routines:\n" + str(traceback.format_exc()))
            return RC_GEN_ERR
        return RC_OK

    def updateLgAspect(self, lgAddr, aspect, force=False):
        if self.state.getOpState() == OP_WORKING or force == True:
            MQTT.publish(MQTT_ASPECT_TOPIC + self.URI + "/" + str(lgAddr) + "/", aspect)
            notify(
                self,
                INFO,
                "New signal mast aspect set for decoder: "
                + self.URI
                + ", LGAddr: "
                + str(lgAddr)
                + ", new aspect: "
                + aspect,
            )
        else:
            notify(self, INFO, "Decoder: " + self.URI + " received a new LgAspect, but decoder OPState is not " + OP_WORKING + ", will do nothing")
        return RC_OK

    def onLogMessage(self, topic, payload):
        notify(self, INFO, "Remote log from: " + self.URI + ": " + payload + "\n" )
        return RC_OK

    def __startSupervision(self):
        notify(self, INFO, "Starting supervision of decoder: " + self.URI)
        self.missedPingCnt = 0
        try:
            self.supervision
        except:
            self.supervision = False
        if self.supervision:
            notify(self, DEBUG_TERSE, "Supervision of decoder: " + str(self.URI) + " already going, restarting it")
            self.pingTimer.cancel()
            return RC_OK
        self.pingTimer = threading.Timer(PING_INTERVAL, self.__onPingTimer)
        self.pingTimer.start()
        self.supervision = True
        return RC_OK

    def __stopSupervision(self):
        notify(self, INFO, "Stoping supervision of decoder: " + str(self.URI))
        self.supervision = False
        try:
            self.pingTimer.cancel()
        except:
            pass
        return RC_OK

    def __del__(self):
        pass

# ==============================================================================================================================================
# Class: mqttListener
# Purpose:  Listen and dispatches incomming mqtt mesages
#
# Data structures: None
# ==============================================================================================================================================
class mqttListener(jmri.jmrix.mqtt.MqttEventListener):
    def __init__(self, cb):
        self.cb = cb

    def notifyMqttMessage(self, topic, message):
        notify(
            self,
            INFO,
            "Received an MQTT message - topic: " + topic + ", payload: " + message,
        )
        threading.Thread(target=self.detach, args=(topic, message, )).start()

    def detach(self, topic, message):
        try:
            self.cb(topic, message)
        except :
            notify(self, PANIC, "Exception in mqtt message handling routines:\n" + str(traceback.format_exc()))


# ==============================================================================================================================================
# Class: lightgroups
# Purpose: Lightgrops is the parent of all lightgroups objects and is a singelton object. Each lightgroup object has a type which may be
#          a generic lightgroup, a signal mast light group, a sequence lightgroup, etc. The light group types are  extendable and the
#          lightgroups object has no notion of the particular logic/function of a light group. Instead each light group objects hosts
#          the specific logic. The purpose of the lightgroups object is primarilly to coordinate the configuration of the underlying
#          and to collect and concatenate xml fragments from the lightgroup objects needed for the decoder configuration.
#
# Data structures:
#          self.mastTable [lightgrouphandle1, lightgrouphandle2,....]
#
# Methods:
#         configure(...): Creates and configures the underlying lightgroup objects
#         getDecoderURIs(...): Provides a list of all the decoders and their respective URIs that is used by the underlying light group objects
#         register(...): Creates links between the lightgroup objects and their respective decoder
#         getXMLConfigTree(...): Constructs the lightgroups xml fragment based on input from underlying lightgroup objects needed to configure
#         the decoder.
# ==============================================================================================================================================
class lightgroups:
    def __init__(self, top):
        self.top = top
        self.mastTable = []
        self.topology = resourceTracker()

    def configure(self, lightgroupsXmlTree):
        if str(lightgroupsXmlTree.tag) != "Lightgroups":
            notify(self, PANIC, "Controllers .xml  missformated:\n")
            return RC_GEN_ERR
        for child in lightgroupsXmlTree:
            if str(child.tag) == "Masts":
                mastsXmlConfig = parse_xml(child, {"MastDefinitionPath": MANSTR})
                self.MastDefinitionPath = mastsXmlConfig.get("MastDefinitionPath")
                smCnt = 0
                notify(
                       self,
                       INFO,
                       "Aquiring signal mast instances from JMRI signal mast definitions",
                      )
                for sm in masts.getNamedBeanSet():
                    mastHandle = mast(self)
                    if mastHandle.configure(sm) != RC_OK:
                        del mastHandle
                        notify(self, INFO, "Mast: " + str(sm) + " was ommitted")
                    else:
                        self.mastTable.append(mastHandle)
                        notify(
                               self,
                               INFO,
                               "Mast: " + str(sm) + " was added to the mast table",
                              )
                        smCnt += 1
                if (smCnt == 0):  # Need to define an exception path to allow to move forward with othe lightgroups, turnouts, etc.
                    notify(
                           self, INFO, "No signal masts with propper configuration found")
                else:
                    notify(
                        self, INFO, str(smCnt) + " Masts were added to the mast table")
                self.mastAspects = mastAspects(self)
                if self.mastAspects.configure(self.MastDefinitionPath) != RC_OK:
                    notify(self, PANIC, "Could not configure mastAspects")
                    return PARAM_ERR
            elif str(child.tag) == "GenericLights":
                notify(self, ERROR, "Generic lights not yet supported, skipping it")
            elif str(child.tag) == "SequenceLights":
                notify(self, ERROR, "Sequence lights not yet supported, skipping it")
            else:
                notify(self, ERROR, str(child.tag) + " tag not recognized/supported, skipping it")
        return RC_OK

    def regTopology(self, topology):
        return self.topology.registerResource(topology)

    def getTopology(self):
        return self.topology.getResources()

    def getXmlConfigTree(self, decoderURI):
        notify(self, DEBUG_TERSE, "Providing lightgroups over arching decoder .xml configuration")
        xmlLightgroups = ET.Element("Lightgroups")
        xmlLightgroups.insert(0, self.mastAspects.getXmlConfigTree(decoderURI))
        for mast in self.mastTable:
            if mast.getDecoderURI() == decoderURI:
                mastXmlTree = mast.getXmlConfigTree(decoderURI)
                if mastXmlTree != None:
                    xmlLightgroups.insert(0, mastXmlTree)
        return xmlLightgroups


# ==============================================================================================================================================
# Class: mastAspects
# Purpose: mastaspects is a helper class for the lightgroup type (signal)masts. It provides the logic fo the mast objects in terms of aspect
#          appearances for each and every mast type. It is a singleton object, and produces needed xml aspect-appearance-mast defintions needed
#          by the decoders. The input is the mast signalling definition xml files for a given signalling system as defined by JMRI,
#          the path to these are given as a parameter in the configure() method.
#
# Data structures:
#          aspectTable: {AspectName : {MastType : {"headAspects" : headAspects[], "NoofPxl" : NoofPxl}}}
#
# xml configuration fragment:
#        <Aspect>
#            <AspectName>Stopp</AspectName>
# 			<Mast>
# 				<Type>Sweden-3HMS:SL-5HL></Type>
# 				<Head>UNLIT</Head>
# 				<Head>LIT</Head>
# 				<Head>UNLIT</Head>
# 				<Head>UNLIT</Head>
# 			    <Head>UNLIT</Head>
# 				<NoofPxl>6</NoofPxl>
# 			</Mast>
#           <Mast>
#             .
#             .
#           </Mast>
#       <Aspect>
#          .
#          .
#       </Aspect>
#
# Methods:
#         configure(...): Parses the JMRI sgnal definition xml files
#         getDecoderURIs(...): Provides a list of all the decoders and their respective URIs that is used by the underlying light group objects
#         getXMLConfigTree(...): Constructs the Aspects xml fragment based on JMRI signalling definition files
#         the decoder.
# ==============================================================================================================================================
class mastAspects:
    def __init__(self, lightgroups):
        self.lightgroups = lightgroups

    def configure(self, mastDefinitionPath):
        notify(self, INFO, "Configuring mastAspects")
        self.mastDefinitionPath = mastDefinitionPath
        self.aspectTable = {}
        try:
            aspectsXmlTree = ET.parse(mastDefinitionPath + "/aspects.xml")
        except:
            notify(self, PANIC, "aspects.xml not found")
        if str(aspectsXmlTree.getroot().tag) != "aspecttable":
            notify(self, PANIC, "aspects.xml missformated")
            return RC_PARAM_ERR
        found = False
        for child in aspectsXmlTree.getroot():
            if child.tag == "aspects":
                for subchild in child:
                    if subchild.tag == "aspect":
                        for subsubchild in subchild:
                            if subsubchild.tag == "name":
                                self.aspectTable[subsubchild.text] = None
                                found = True
                                break
        if found == False:
            notify(self, PANIC, "no Aspects found - aspects.xml  missformated")
            return RC_PARAM_ERR
        fileFound = False
        for filename in os.listdir(mastDefinitionPath):
            if filename.endswith(".xml") and filename.startswith("appearance-"):
                fileFound = True
                if mastDefinitionPath.endswith(
                    "\\"
                ):  # Is the directory/filename really defining the SM type?
                    mastType = (
                        mastDefinitionPath.split("\\")[-2]
                        + ":"
                        + (filename.split("appearance-")[-1]).split(".xml")[0]
                    )  # Fix UNIX portability
                else:
                    mastType = (
                        mastDefinitionPath.split("\\")[-1]
                        + ":"
                        + (filename.split("appearance-")[-1]).split(".xml")[0]
                    )
                notify(
                    self,
                    INFO,
                    "Parsing Appearance file: " + mastDefinitionPath + "\\" + filename,
                )
                appearanceXmlTree = ET.parse(mastDefinitionPath + "\\" + filename)
                if str(appearanceXmlTree.getroot().tag) != "appearancetable":
                    notify(self, PANIC, filename + " is  missformated")
                    return RC_PARAM_ERR
                found = False
                for child in appearanceXmlTree.getroot():
                    if child.tag == "appearances":
                        for subchild in child:
                            if subchild.tag == "appearance":
                                headAspects = []
                                aspectName = None
                                cnt = 0
                                for subsubchild in subchild:
                                    if subsubchild.tag == "aspectname":
                                        aspectName = subsubchild.text
                                    if aspectName != None and subsubchild.tag == "show":
                                        found = True
                                        headAspects.append(
                                            self.__decodeAppearance(subsubchild.text)
                                        )
                                        cnt += 1
                                        x = self.aspectTable.get(aspectName)
                                        if x == None:
                                            x = {
                                                mastType: {
                                                    "headAspects": headAspects,
                                                    "NoofPxl": -(-cnt // 3) * 3,
                                                }
                                            }
                                        else:
                                            x[mastType] = {
                                                "headAspects": headAspects,
                                                "NoofPxl": -(-cnt // 3) * 3,
                                            }
                                        self.aspectTable[aspectName] = x
                if found == False:
                    notify(self, PANIC, "No Appearances found in: " + filename)
                    return RC_PARAM_ERR
        if fileFound != True:
            notify(self, PANIC, "No Appearance file found")
            return RC_PARAM_ERR
        notify(self, INFO, "mastAspects successfulyy configured")
        return RC_OK

    def __decodeAppearance(self, appearance):
        if (
            appearance == "red"
            or appearance == "green"
            or appearance == "yellow"
            or appearance == "lunar"
        ):
            return "LIT"
        elif (
            appearance == "flashred"
            or appearance == "flashgreen"
            or appearance == "flashyellow"
            or appearance == "flashlunar"
        ):
            return "FLASH"
        elif appearance == "dark":
            return "UNLIT"
        else:
            notify(self, PANIC, "A non valid appearance: " + appearance + " was found")

    def getXmlConfigTree(self, URI):
        notify(self, DEBUG_TERSE, "Providing mastAspects over arching decoders .xml configuration")
        xmlSignalmastDesc = ET.Element("SignalMastDesc")
        xmlAspects = ET.SubElement(xmlSignalmastDesc, "Aspects")
        for aspect in self.aspectTable:
            xmlAspect = ET.SubElement(xmlAspects, "Aspect")
            xmlAspectName = ET.SubElement(xmlAspect, "AspectName")
            xmlAspectName.text = aspect
            for mast in self.aspectTable.get(aspect):
                xmlMast = ET.SubElement(xmlAspect, "Mast")
                xmlMastType = ET.SubElement(xmlMast, "Type")
                xmlMastType.text = str(mast)
                for headAspect in (
                    self.aspectTable.get(aspect).get(mast).get("headAspects")
                ):
                    xmlHead = ET.SubElement(xmlMast, "Head")
                    xmlHead.text = str(headAspect)
                xmlNoofpxl = ET.SubElement(xmlMast, "NoofPxl")
                xmlNoofpxl.text = str(
                    self.aspectTable.get(aspect).get(mast).get("NoofPxl")
                )
        return xmlSignalmastDesc

    def getOpState(self):
        return self.opState


# ==============================================================================================================================================
# Class: mast
# Purpose: The mast class is an instantiation of the geric lightgroup concept. There is one mast object per signal mast. the mast class together
#          with the mastAspects class defines the behaviour and logic of signal masts. masts objects are ceated by the lightgroups class and gets
#          configured from mast meta-data embedded in the username- eg.
#          !*[Decoder:decoder1.jmri.bjurel.com, Sequence:1 , Brightness:Normal, DimTime:Normal, FlashFreq:Normal,  FlashDuty:50 ]*!
#
# Data structures:
#          aspectTable: {AspectName : {MastType : {"headAspects" : headAspects[], "NoofPxl" : NoofPxl}}}
#
# xml configuration fragment:
# 		<Lightgroup>
# 			<LgAddr>1</LgAddr>
# 			<LgSeq>1</LgSeq>
# 			<LgSystemName>IF$vsm:Sweden-3HMS:SL-5HL($0001)</LgSystemName>
# 			<LgUserName>MQTT_5Head_Master_Test1</LgUserName>
# 			<LgType>Signal Mast</LgType>
# 			<LgDesc>
# 				<SmType>Sweden-3HMS:SL-5HL</SmType>
# 				<SmDimTime>NORMAL</SmDimTime>
# 				<SmFlashFreq>NORMAL</SmFlashFreq>
#                <SmFlashDuty>50</FlashDuty>
# 				<SmBrightness>NORMAL</SmBrightness>
# 			</LgDesc>
# 		</Lightgroup>
#
# Methods:
#   configure(...):
#           Parses the system name and the user name and extracts the mast configuration from those
#   getDecoderURIs(...):
#           Provides the decoderURI this mast belongs to
#   getXMLConfigTree(...):
#           Constructs the lightgroup (and mast extentions) xml fragment needed for the decoder configuration
#         the decoder.
#   register(...) :
#         links the mast object to its decoder
#   onSmChange(...):
#         A JMRI signal mast aspect change callback - requesting, indicating a new mast aspect
#   resendAspect(self) :
#         Re send currnt aspect
# =============================================================================================================================================="""
class mast:
    def __init__(self, lightGroups):
        self.lgSystemName = None #HAVE ALL VARIABLES DEFINED HERE
        self.lightGroups = lightGroups
        self.state = systemState()
        self.state.regCb(self.onStateChange)
        self.state.opBlock(OP_INIT)
        PANIC_HANDLER.regCb(self.onPanic)
        self.lock = threading.Lock()
        self.currentAspect = None
        self.sm = None
        self.ws28xxLink = 0 #NEEDS TO BE PROPPERLY FIXED
        # Get current aspect

    def configure(self, sm):
        self.sm = sm
        notify(self, INFO, "Signal mast: " + str(self.sm) + " found, trying to configure it")
        if smIsVirtual(sm) != 1:
            notify(
                self, INFO, "Signal mast: " + str(self.sm) + " is not virtal, ommiting it")
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        self.lgSystemName = str(self.sm)
        self.lgAddr = smLgAddr(self.lgSystemName)
        self.lgUserName = self.sm.getUserName()
        self.lgType = self.sm.getBeanType()
        self.smType = smType(self.lgSystemName)
        self.lgComment = self.sm.getComment()
        if self.__configProperties() != RC_OK:
            notify(
                self,
                INFO,
                "Signal mast: " + str(self.sm) + " could not be configured - ommiting it",
            )
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        sequenceMap = []
        for i in range(self.lgSequene):
            sequenceMap.append(None)
        sequenceMap.append(self)
        rc = self.lightGroups.regTopology({self.decoderURI:{self.ws28xxLink:sequenceMap}}) # Need to configure self.ws28xxLink
        if rc[0] == RC_BUSY:
            notify(
               self,
               PANIC,
               "Signal mast: " +
               str(self.sm) +
               " topology overlaps with: " +
               rc[1].getSystemName()
            )
            self.state.opBlock(OP_FAIL)
            return rc[0]
        elif rc[0] != RC_OK:
            notify(
                   self,
                   PANIC,
                   "A general error occured when registering topology for signal mast: " +
                   str(self.sm)
                  )
            self.state.opBlock(OP_FAIL)
            return rc[0]
        else:
            notify(
                   self,
                   INFO,
                   "Signal mast: " +
                   str(self.sm) +
                   " successfully created with following configuration: " +
                   "User name: " +
                   self.lgUserName +
                   ", Type: " +
                   self.smType +
                   ", Brightness: " +
                   self.smBrightness +
                   ", Dim time: " +
                   self.smDimTime +
                   ", Flash freq: " +
                   self.smFlashFreq +
                   ", Flash duty: " +
                   str(self.smFlashDuty)
                  )
        return RC_OK

    def __configProperties(self):
        properties = (self.lgUserName.split("!*[")[-1]).split("]*!")[0]
        if len(properties) < 2:
            notify(self, INFO, "No properties found for signal mast: " + str(self.sm))
            self.opState = OP_FAIL
            return RC_PARAM_ERR
        properties = properties.split(",")
        if len(properties) == 0:
            notify(self, INFO, "No properties found for signal mast: " + str(self.sm))
            self.opState = OP_FAIL
            return RC_PARAM_ERR
        self.smBrightness = "NORMAL"
        self.smDimTime = "NORMAL"
        self.smFlashFreq = "NORMAL"
        self.smFlashDuty = 50
        cnt = 0
        while cnt < len(properties):
            property = properties[cnt].split(":")
            if len(property) < 2:
                notify(
                    self,
                    ERROR,
                    "Properties missformated for signal mast: " + self.lgSystemName,
                )
                self.opState = OP_FAIL
                return RC_PARAM_ERR
            if property[0].strip().upper() == "DECODER":
                self.decoderURI = property[1].strip()
            elif property[0].strip().upper() == "SEQUENCE":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: Sequence is not an integer for signal mast: "
                        + str(self.sm),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.lgSequene = int(property[1].strip())
            elif property[0].strip().upper() == "BRIGHTNESS":
                if property[1].strip().upper() == "LOW":
                    self.smBrightness = "LOW"
                elif property[1].strip().upper() == "NORMAL":
                    self.smBrightness = "NORMAL"
                elif property[1].strip().upper() == "HIGH":
                    self.smBrightness = "HIGH"
                    notify(
                        self,
                        ERROR,
                        "Property: Brightness is not any of LOW/NORMAL/HIGH for signal mast: "
                        + str(self.sm)
                        + " - using default",
                    )
            elif property[0].strip().upper() == "DIMTIME":
                if property[1].strip().upper() == "SLOW":
                    self.smDimTime = "SLOW"
                elif property[1].strip().upper() == "NORMAL":
                    self.smDimTime = "NORMAL"
                elif property[1].strip().upper() == "FAST":
                    self.smDimTime = "FAST"
                else:
                    notify(
                        self,
                        ERROR,
                        "Property: Dimtime is not any of SLOW/NORMAL/FAST for signal mast: "
                        + str(self.sm)
                        + " - using default",
                    )
            elif property[0].strip().upper() == "FLASHFREQ":
                if property[1].strip().upper() == "SLOW":
                    self.smFlashFreq = "SLOW"
                elif property[1].strip().upper() == "NORMAL":
                    self.smFlashFreq = "NORMAL"
                elif property[1].strip().upper() == "FAST":
                    self.smFlashFreq = "FAST"
                else:
                    notify(
                        self,
                        ERROR,
                        "Property: Flashfreq is not any of SLOW/NORMAL/FAST for signal mast: "
                        + str(self.sm)
                        + " - using default"
                    )
            elif property[0].strip().upper() == "FLASHDUTY":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        ERROR,
                        "Property: Flashduty is not an integer for signal mast: "
                        + str(self.sm)
                        + " - using default"
                    )
                else:
                    self.smFlashDuty = int(property[1].strip())
            else:
                notify(
                    self,
                    ERROR,
                    property[0].strip()
                    + " is not a valid property for signal mast: "
                    + str(self.sm)
                    + " - ignoring"
                )
            cnt += 1
        try:
            self.decoderURI
        except:
            notify(
                self,
                ERROR,
                "Decoder property was not defined for signal mast: " + str(self.sm)
            )
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        try:
            self.lgSequene
        except:
            notify(
                self,
                ERROR,
                "Sequence property was not defined for signal mast: " + str(self.sm)
            )
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        return RC_OK

    def register(self, decoderHandle, decoderURI):
        if self.decoderURI != decoderURI:
            return RC_GEN_ERR
        else:
            self.decoderHandle = decoderHandle
            self.getCurrentAspect()
            sm = masts.getBySystemName(self.lgSystemName)
            sm.addPropertyChangeListener(self.onSmChange)
            notify(self, DEBUG_TERSE, "Mast: " + self.lgSystemName + " Registered, current opState: " + getOpBlockStateStr(self.state.getOpState()) + ", Decoder: " + self.decoderURI + ", Aspect: " + self.currentAspect)
            return RC_OK

    def start(self):
        notify(self, 
               DEBUG_TERSE,
               "Starting Lightgroup/Mast: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getWs28xxLink()) + "/" +
                str(self.getSequence()) +
                " (" + str(self.getSystemName) + ") " +
                "OpState: " +
                getOpBlockStateStr(self.state.getOpState())
               )
        if self.state.getOpState() & OP_FAIL or not (self.state.getOpState() & OP_INIT):
            notify(self, 
                   ERROR,
                   "Lightgroup/Mast: " +
                   str(self.getDecoderURI()) + "/" +
                   str(getWs28xxLink()) + "/" +
                   str(self.getSequence()) +
                   " (" + str(self.getSystemName) + ") " +
                   " could not be started, " +
                   "OpState: " +
                   getOpBlockStateStr(self.state.getOpState()) +
                   " return code : " +
                   str(RC_GEN_ERR)
                   )
            return RC_GEN_ERR
        self.state.opDeBlock(OP_INIT)
        notify(self, DEBUG_TERSE,
                "Lightgroup/Mast: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getWs28xxLink()) + "/" +
                str(self.getSequence()) +
                " (" + str(self.getSystemName) + ") " +
                " return code : " +
                "has been started, " +
                "OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        return RC_OK

    def getXmlConfigTree(self, decoderURI):
        if decoderURI != self.decoderURI:
            notify(self, PANIC, "URI does not match - XML configuration cannot be created")
            self.state.opBlock(OP_FAIL)
            return None
        try:
            notify(self, DEBUG_TERSE, "Providing mast decoders .xml configuration")
            lightgroup = ET.Element("Lightgroup")
            lgAddr = ET.SubElement(lightgroup, "LgAddr")
            lgAddr.text = str(self.lgAddr)
            lgSeq = ET.SubElement(lightgroup, "LgSequence")
            lgSeq.text = str(self.lgSequene)
            lgSystemName = ET.SubElement(lightgroup, "LgSystemName")
            lgSystemName.text = self.lgSystemName
            lgUserName = ET.SubElement(lightgroup, "LgUserName")
            lgUserName.text = self.lgUserName
            lgType = ET.SubElement(lightgroup, "LgType")
            lgType.text = self.lgType
            lgDesc = ET.SubElement(lightgroup, "LgDesc")
            smType = ET.SubElement(lgDesc, "SmType")
            smType.text = self.smType
            smDimTime = ET.SubElement(lgDesc, "SmDimTime")
            smDimTime.text = self.smDimTime
            smFlashFreq = ET.SubElement(lgDesc, "SmFlashFreq")
            smFlashFreq.text = self.smFlashFreq
            smFlashDuty = ET.SubElement(lgDesc, "SmFlashDuty")
            smFlashDuty.text = str(self.smFlashDuty)
            smBrightness = ET.SubElement(lgDesc, "SmBrightness")
            smBrightness.text = self.smBrightness
            return lightgroup
        except:
            notify(self, PANIC, "Mast is not configured - XML configuration cannot be created")
            return None

    def getCurrentAspect(self):
        self.lock.acquire()
        sm = masts.getBySystemName(self.lgSystemName) #probably removable
        if isinstance(masts.getBySystemName(self.lgSystemName), str):
            self.currentAspect = self.sm.getAspect()
        else:
            self.currentAspect = "UNKNOWN"
        self.lock.release()
        return self.currentAspect

    def onSmChange(self, event):
        self.lock.acquire()
        self.currentAspect = event.newValue
        self.lock.release()
        notify(self, DEBUG_TERSE, "Mast: " + str(self.lgSystemName) + " is set to: " + self.currentAspect + " aspect")
        self.decoderHandle.updateLgAspect(
            self.lgAddr, "<Aspect>" + self.currentAspect + "</Aspect>"
        )

    def onPanic(self, panic):
        if panic:
            notify(self, INFO, "Mast: " + str(self.lgSystemName) + " is set to Fault aspect")
            self.decoderHandle.updateLgAspect(
            self.lgAddr, FAULT_ASPECT, 
            force=True
            )
        else:
            notify(self, INFO, "Mast: " + str(self.lgSystemName) + " fault aspect ceased")
            self.decoderHandle.updateLgAspect(
            self.lgAddr, NOFAULT_ASPECT, 
            force=True
            )

    def onStateChange(self): 
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "Mast: " +
                                            str(self.getDecoderURI()) + "/" +
                                            " (" + str(self.lgSystemName) + ")"
                                            " down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )

        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self, INFO,
                "Mast: " +
                str(self.getDecoderURI()) + "/" +
                " (" + str(self.lgSystemName) + ")"
                " has changed operational status to: " + 
                getOpBlockStateStr(self.state.getOpState())
                )

    def resendAspect(self):
        notify(self, INFO, "Mast: " + str(self.lgSystemName) + " is resending its aspect")
        self.lock.acquire()
        if self.currentAspect != None:
            self.decoderHandle.updateLgAspect(
                self.lgAddr, "<Aspect>" + self.currentAspect + "</Aspect>"
            )
        self.lock.release()
        return RC_OK

    def getSystemName(self):
        try:
            return self.lgSystemName
        except:
            return None

    def getUserNameName(self):
        try:
            return self.lgUserName
        except:
            return None

    def getSequenceMap(self):
        try:
            return self.sequenceMap
        except:
            return None

    def getDecoderURI(self):
        try:
            return self.decoderURI
        except:
            return None

    def getWs28xxLink(self):
        try:
            return 0 # We need to add a parameter for this
        except:
            return None

    def getSequence(self):
        try:
            return self.lgSequene
        except:
            return None

    def getOpState(self):
        return self.state.getOpState()



# ==============================================================================================================================================
# Class: sateliteLink
#==============================================================================================================================================
class sateliteLink:
    def __init__(self, decoderHandle, satLink):
        self.decoderHandle = decoderHandle
        self.satLink = satLink
        self.state = systemState()
        self.state.regCb(self.onStateChange)
        self.state.opBlock(OP_INIT)
        self.state.admDeBlock()
        self.state.setParent(decoderHandle)

    def register(self, actuatorsHandle, sensorsHandle):
        self.actuatorsHandle = actuatorsHandle
        self.sensorsHandle = sensorsHandle
        self.satTable = {}
        for type in [self.actuatorsHandle, self.sensorsHandle]:
            if type != None:
                for sat in type.getTopology()[self.getDecoderURI()][self.satLink]:
                    if not sat in self.satTable:
                        self.satTable[sat] = satelite(self, sat)
                        self.state.addChild(self.satTable[sat].state)
                        rc = self.satTable[sat].register(self.actuatorsHandle, self.sensorsHandle)
                        if rc != RC_OK:
                            return rc
        return RC_OK

    def start(self):
        notify(self, 
               DEBUG_TERSE,
               "Starting Satelite link: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) + 
                " OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        for sat in self.satTable:
            self.satTable[sat].start()
        MQTT.subscribe(MQTT_SATLINK_OPBLOCK_TOPIC + 
                       str(self.getDecoderURI()) + "/" +
                       str(self.getSatLink()) + "/",
                       mqttListener(self.onHWStateChange)
                       )
        notify(self, 
               INFO,
               "Satelite link: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) +
                " started"
               " OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        return RC_OK

    def onHWStateChange(self, topic, message):
        notify(self, 
               INFO,
               "Satelite link: " +
               str(self.getDecoderURI()) + "/" +
               str(self.getSatLink()) +
               " OpState: " +
               message)

        if (int(message) & SAT_INIT) & ~(self.state.getOpState() & SAT_INIT):
            self.state.opBlock(OP_INIT)
        else:
            self.state.opDeBlock(OP_INIT)
        if (int(message) & SAT_FAIL) & ~(self.state.getOpState() & SAT_FAIL):
            self.state.opBlock(OP_FAIL)
        else:
            self.state.opDeBlock(OP_FAIL)
        if (int(message) & SAT_ERRSEC) & ~(self.state.getOpState() & SAT_ERRSEC):
            self.state.opBlock(OP_ERRSEC)
        else:
            self.state.opDeBlock(OP_ERRSEC)
        notify(self, INFO,
               "Satelite Link: " +
               str(self.getDecoderURI()) + "/" +
               str(self.getSatLink()) +
               " received a opstate change message from decoder: " + 
               str(message)
               )

    def onStateChange(self):
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "satelite link " +
                                            str(self.getDecoderURI()) + "/" +
                                            str(self.getSatLink()) +
                                            "down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )
        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self, INFO,
               "Satelite link: " +
               str(self.getDecoderURI()) + "/" +
               str(self.getSatLink()) +
               " has changed operational status to: " + 
               getOpBlockStateStr(self.state.getOpState())
               )

    def admBlock(self):
        rc = self.state.admBlock()
        if rc == RC_OK:
            MQTT.publish(MQTT_SATLINK_ADMBLOCK_TOPIC +
                         str(self.getDecoderURI()) + "/" +
                         str(self.getSatLink()) + "/"
                         , ADM_BLOCK
                         )
        return rc

    def admDeBlock(self):
        rc = self.state.admDeBlock()
        if rc == RC_OK:
            MQTT.publish(MQTT_SATLINK_ADMBLOCK_TOPIC +
                         str(self.getDecoderURI()) + "/" +
                         str(self.getSatLink()) + "/"
                         , ADM_DEBLOCK
                         )
        return rc

    def getAdmState(self):
        return self.state.getAdmState()

    def getOpState(self):
        return self.state.getOpState()

    def getDecoderURI(self):
        return self.decoderHandle.getDecoderURI()

    def getSatLink(self):
        return self.satLink

    def __del__(self):
        pass



# ==============================================================================================================================================
# Class: satelite
#==============================================================================================================================================
class satelite:
    def __init__(self, satLinkHandle, sat):
        self.satLinkHandle = satLinkHandle
        self.sat = sat
        self.state = systemState()
        self.state.regCb(self.onStateChange)
        self.state.opBlock(OP_INIT)
        self.state.admDeBlock()
        self.state.setParent(satLinkHandle)


    def register(self, actuatorsHandle, sensorsHandle):
        self.actuatorsHandle = actuatorsHandle
        self.sensorsHandle = sensorsHandle
        if self.actuatorsHandle != None:
            for act in self.actuatorsHandle.getTopology()[self.getDecoderURI()][self.getSatLink()][self.getSatAddr()]:
                if act != None:
                    rc = act.register(self)
                    self.state.addChild(act.state)
        if self.sensorsHandle != None:
            for sens in self.sensorsHandle.getTopology()[self.getDecoderURI()][self.getSatLink()][self.getSatAddr()]:
                if sens != None:
                    rc = sens.register(self)
                    self.state.addChild(sens.state)
        return RC_OK

    def start(self):
        notify(self, 
               DEBUG_TERSE,
               "Starting Satelite: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) + "/" +
                str(self.getSatAddr()) +
                " OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        if self.actuatorsHandle != None:
            for act in self.actuatorsHandle.getTopology()[self.getDecoderURI()][self.getSatLink()][self.getSatAddr()]:
                if act != None:
                    act.start()
        if self.sensorsHandle != None:
            for sens in self.sensorsHandle.getTopology()[self.getDecoderURI()][self.getSatLink()][self.getSatAddr()]:
                if sens != None:
                    sens.start()
        MQTT.subscribe(MQTT_SAT_OPBLOCK_TOPIC + 
                       str(self.getDecoderURI()) + "/" +
                       str(self.getSatLink()) + "/" +
                       str(self.getSatAddr()) + "/",
                       mqttListener(self.onHWStateChange)
                       )
        notify(self, 
               DEBUG_TERSE,
               "Satelite: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) + "/" +
                str(self.getSatAddr()) +
                "started, " +
                " OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        return RC_OK

    def onHWStateChange(self, topic, message):
        if int(message) & SAT_INIT:
            self.state.opBlock(OP_INIT)
        else:
            self.state.opDeBlock(OP_INIT)
        if int(message) & SAT_FAIL:
            self.state.opBlock(OP_FAIL)
        else:
            self.state.opDeBlock(OP_FAIL)
        if int(message) & SAT_ERRSEC:
            self.state.opBlock(OP_ERRSEC)
        else:
            self.state.opDeBlock(OP_ERRSEC)

        notify(self, DEBUG_TERSE,
               "Satelite: " +
               self.satLinkHandle.getDecoderURI() + "/" +
               str(self.satLinkHandle.getSatLink()) + "/"
               + str(self.sat) +
               " received a state change: " + 
               str(message)
               )

    def onStateChange(self):
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "Satelite " +
                                            str(self.getDecoderURI()) + "/" +
                                            str(self.getSatLink()) + "/" +
                                            str(self.getSatAddr) +
                                            "down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )
        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self, INFO,
               "Satelite: " +
               str(self.getDecoderURI()) + "/" +
               str(self.getSatLink()) + "/" +
               str(self.getSatAddr) +
               " has changed operational status to: " + 
               getOpBlockStateStr(self.state.getOpState())
               )

    def admBlock(self):
        rc = self.state.admBlock()
        if rc == RC_OK:
            MQTT.publish(MQTT_SAT_ADMBLOCK_TOPIC +
                         str(self.getDecoderURI()) + "/" +
                         str(self.getSatLink()) + "/" +
                         str(self.getSatAddr) + "/"
                         , ADM_BLOCK
                         )
        return rc

    def admDeBlock(self):
        rc = self.state.admDeBlock()
        if rc == RC_OK:
            MQTT.publish(MQTT_SAT_ADMBLOCK_TOPIC +
                         str(self.getDecoderURI()) + "/" +
                         str(self.getSatLink()) + "/" +
                         str(self.getSatAddr) + "/"
                         , ADM_DEBLOCK
                         )
        return rc

    def getAdmState(self):
        return self.state.getAdmState()

    def getOpState(self):
        return self.state.getOpState()

    def getDecoderURI(self):
        return self.satLinkHandle.getDecoderURI()

    def getSatLink(self):
        return self.satLinkHandle.getSatLink()

    def getSatAddr(self):
        return self.sat

    def getActuators(self):
        try:
            return self.actuatorsHandle[self.satLinkHandle.getDecoderURI()][self.satLinkHandle.getSatLink()][self.sat]
        except:
            return None

    def getSensors(self):
        try:
            return self.sensorsHandle[self.satLinkHandle.getDecoderURI()][self.satLinkHandle.getSatLink()][self.sat]
        except:
            return None

    def __del__(self):
        pass

# ==============================================================================================================================================
# Class: actuators
# Purpose: Actuators is the parent of all actuator objects and is a singelton object. Each actuator object has a type which may be
#          a turnout or a generic actuator. The actuator types are  extendable and the
#          actuators object has no notion of the particular logic/function of an actuator. Instead each actuator objects hosts
#          the specific logic. The purpose of the actuators object is primarilly to coordinate the configuration of the underlying actuator
#          object and to collect and concatenate xml fragments from the actuator objects needed for the decoder configuration.
#
# Data structures:
#          self.actuatorTable [actuatorhandle1, actuatorhandle2,....]
#
# Methods:
#         configure(...): Creates and configures the underlying actuator objects
#         getDecoderURIs(...): Provides a list of all the decoders and their respective URIs that is used by the underlying actuator objects
#         register(...): Creates links between the actuator objects and their respective decoder
#         getXMLConfigTree(...): Constructs the actuators xml fragment based on input from underlying actuator objects needed to configure
#         the decoder.
# ==============================================================================================================================================
class actuators:
    def __init__(self, top):
        self.top = top
        self.actuatorTable = []
        self.topology = resourceTracker()

    def configure(self, actuatorsXmlTree):
        if str(actuatorsXmlTree.tag) != "Actuators": #Currently no xml configuration objects in Actuators - but needs to be defined
            notify(self, PANIC, "Controllers .xml  missformated:\n")
            return RC_GEN_ERR
        actuatorCnt = 0
        for turnout in turnouts.getNamedBeanSet():
            turnoutHandle = actuator(self)
            if turnoutHandle.configure(turnout) != RC_OK:
                del turnoutHandle
                notify(self, INFO, "Turnout: " + str(turnout) + " could not be configured and is being ommitted")
            else:
                self.actuatorTable.append(turnoutHandle)
                notify(self, INFO, "Turnout: " + str(turnout) + " was added to the actuator table",)
                actuatorCnt += 1
        for light in lights.getNamedBeanSet():
            lightHandle = actuator(self)
            if lightHandle.configure(light) != RC_OK:
                del lightHandle
                notify(self, INFO, "Light: " + str(light) + " could not be configured and is being ommitted")
            else:
                self.actuatorTable.append(lightHandle)
                notify(self, INFO, "Light: " + str(light) + " was added to the actuator table",)
                actuatorCnt += 1
        if (actuatorCnt == 0):
            notify(self, INFO, "No actuator with propper configuration found")
        else:
            notify(self, INFO, str(actuatorCnt) + " Actuators were added to the actuator table")
        return RC_OK

    def regTopology(self, topology):
        return self.topology.registerResource(topology)

    def getTopology(self):
        return self.topology.getResources()

    def getXmlConfigTree(self, decoderURI):
        notify(self, DEBUG_TERSE, "Providing actuators over arching decoder .xml configuration")
        actuatorsXml = ET.Element("Actuators")
        for actuator in self.actuatorTable:
            if actuator.getDecoderURI() == decoderURI:
                actuatorXmlTree = actuator.getXmlConfigTree(decoderURI)
                if actuatorXmlTree != None:
                    actuatorsXml.insert(0, actuatorXmlTree)
        return actuatorsXml

    def __del__(self):
        pass

# ==============================================================================================================================================
# Class: actuator
# =============================================================================================================================================="""
class actuator:
    def __init__(self, actsHandle):
        self.actsHandle = actsHandle
        self.state = systemState()
        self.state.regCb(self.onStateChange)
        self.state.opBlock(OP_INIT)
        self.state.admDeBlock()
        self.lock = threading.Lock()

    def configure(self, jmriAct):
        if self.state.getOpState() & OP_FAIL:
            return RC_GEN_ERR
        self.jmriAct = jmriAct
        notify(self, INFO, "Actuator: " + str(self.jmriAct) + " found, trying to configure it...")
        if actIsMqtt(self.jmriAct) != True:
            notify(self, INFO, "Actuator: " + str(self.jmriAct) + " is not controlled through MQTT, ommiting it")
            self.state.opBlock(OP_FAIL)
            self.state.opDeBlock(OP_INIT)
            return RC_PARAM_ERR
        self.actuatorSystemName = str(self.jmriAct)
        self.actuatorAddr = actAddrStr(self.jmriAct)
        self.actuatorUserName = self.jmriAct.getUserName()
        self.actuatorType = actType(self.jmriAct)
        self.actComment = self.jmriAct.getComment()
        if self.__configProperties() != RC_OK:
            notify(
                self,
                ERROR,
                "Actuator: " + str(self.jmriAct) + " could not be configured - ommiting it",
            )
            self.state.opBlock(OP_FAIL)
            return RC_GEN_ERR
        actPortMap = []
        for i in range(self.satPortBase):
            actPortMap.append(None)
        actPortMap.append(self)
        if self.actuatorType == "TURNOUT" and self.actuatorSubType == "SOLENOID":
            actPortMap.append(self)
        rc = self.actsHandle.regTopology({self.decoderURI:{self.satLink:{self.satAddr:actPortMap}}})
        if rc[0] == RC_BUSY:
            notify(
               self,
               PANIC,
               "Actuator: " +
               str(self.actuatorSystemName) +
               " topology overlaps with: " +
               rc[1].getSystemName()
            )
            self.state.opBlock(OP_FAIL)
            return rc[0]
        elif rc[0] != RC_OK:
            notify(
                   self,
                   PANIC,
                   "A general error occured when registering topology for actuator: " +
                   str(self.actuatorSystemName)
                  )
            self.state.opBlock(OP_FAIL)
            return rc[0]
        if self.actuatorType == "TURNOUT" and self.actuatorSubType == "SERVO":
            notify(
                self,
                INFO,
                "Servo Turnout: "
                + str(self.jmriAct)
                + " successfully created with following configuration: "
                + "Actuator address: "
                + self.actuatorAddr
                + ", System name: "
                + self.actuatorSystemName
                + ", User name: "
                + self.actuatorUserName
                + ", Actuator Type: "
                + self.actuatorType
                + ", Decoder: "
                + self.decoderURI
                +", Satelite Link: "
                +  str(self.satLink)
                +", Satelite Link Address: "
                +  str(self.satAddr)
                +", Satelite Port: "
                +  str(self.satPortBase)
                +", Turnout Type: "
                + self.actuatorSubType
                +", Turnout Dircetion: "
                + self.turnoutDirection
                +", Turnout Thrown Position: "
                +  str(self.turnoutThrownPos)
                +", Turnout Closed Position: "
                +  str(self.turnoutClosedPos)
                +", Turnout Speed: "
                +  str(self.actuatorSpeed)
                )
        elif self.actuatorType == "TURNOUT" and self.actuatorSubType == "SOLENOID":
            notify(
                self,
                INFO,
                "Solenoid Turnout: "
                + str(self.jmriAct)
                + " successfully created with following configuration:"
                + "Actuator address: "
                + self.actuatorAddr
                + ", System name: "
                + self.actuatorSystemName
                + ", User name: "
                + self.actuatorUserName
                + ", Actuator Type: "
                + self.actuatorType
                + ", Decoder: "
                + self.decoderURI
                + ", Satelite Link: "
                + str(self.satLink)
                + ", Satelite Link Address: "
                + str(self.satAddr)
                + ", Satelite Port: "
                + str(self.satPortBase)
                + ", Turnout Type: "
                + self.actuatorSubType
                + ", Turnout Dircetion: "
                + self.turnoutDirection
                + ", Turnout Pulse: "
                + str(self.turnoutPulse)
                )
        elif self.actuatorType == "GENERIC":
            notify(
                self,
                INFO,
                "Generic Actuator: "
                + str(self.jmriAct)
                + " successfully created with following configuration:"
                "Actuator address: "
                + self.actuatorAddr
                + ", Decoder: "
                + self.decoderURI
                +", Satelite Link: "
                +  str(self.satLink)
                +", Satelite Link Address: "
                +  str(self.satAddr)
                +", Satelite Port: "
                +  str(self.satPortBase)
                + "System name: "
                + self.actuatorSystemName
                + ", User name: "
                + self.actuatorUserName
                +", Actuator Sub Type: "
                + self.actuatorSubType
                +", Invert: "
                + self.actuatorInvert
                +", OnVal: "
                + str(self.actuatorOnVal)
                +", OffVal: "
                + str(self.actuatorOffVal)
                +", Actuator Speed: "
                +  str(self.actuatorSpeed)
            )
        else:
            notify(
                self,
                PANIC,
                "Actuator: "
                + str(self.jmriAct)
                + " Actuator type "
                + self.actuatorType
                + " not supported"
            )
            self.state.opBlock(OP_FAIL)
            return RC_GEN_ERR
        return RC_OK

    def __configProperties(self):
        properties = (self.actuatorUserName.split("!*[")[-1]).split("]*!")[0]
        if len(properties) < 2:
            notify(self, INFO, "No properties found for actuator: " + str(self.jmriAct))
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        properties = properties.split(",")
        if len(properties) == 0:
            notify(self, PANIC, "No properties found for actuator: " + str(self.jmriAct))
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        cnt = 0
        while cnt < len(properties):
            property = properties[cnt].split(":")
            if len(property) < 2:
                notify(
                    self,
                    PANIC,
                    "Properties missformated for actuator: " + self.lgSystemName,
                )
                self.state.opBlock(OP_FAIL)
                return RC_PARAM_ERR

            if property[0].strip().upper() == "DECODER":
                self.decoderURI = property[1].strip().lower()
            elif property[0].strip().upper() == "SATLINK":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: SatLink is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.satLink = int(property[1].strip())
            elif property[0].strip().upper() == "SATLINKADDR":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: SatLinkAddr is not an integer for actuator: "
                        + str(self.jmriAct)
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.satAddr = int(property[1].strip())
            elif property[0].strip().upper() == "SATPORT":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: SatPort is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.satPortBase = int(property[1].strip())
            elif property[0].strip().upper() == "TYPE":
                self.actuatorSubType = property[1].strip().upper()
            elif property[0].strip().upper() == "DIRECTION":
                self.turnoutDirection = property[1].strip().upper()
            elif property[0].strip().upper() == "INVERT":
                self.actuatorInvert = property[1].strip().upper()
            elif property[0].strip().upper() == "THROWNPOS":
                try:
                    int(property[1].strip())
                except:
                    notify(
                            self,
                            PANIC,
                            "Property: ThrownPos is not an integer for actuator: "
                            + str(self.jmriAct),
                            )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.turnoutThrownPos = int(property[1].strip())
            elif property[0].strip().upper() == "CLOSEDPOS":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: ClosedPos is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.turnoutClosedPos = int(property[1].strip())
            elif property[0].strip().upper() == "ONVAL":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: OnVal is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.actuatorOnVal = int(property[1].strip())
            elif property[0].strip().upper() == "OFFVAL":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: OffVal is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.actuatorOffVal = int(property[1].strip())
            elif property[0].strip().upper() == "SPEED":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: Speed is not an integer for actuator: "
                        + str(self.jmriAct),
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.actuatorSpeed = int(property[1].strip())
            elif property[0].strip().upper() == "PULSE":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        PANIC,
                        "Property: Pulse is not an integer for actuator: "
                        + str(self.jmriAct)
                    )
                    self.state.opBlock(OP_FAIL)
                    return RC_PARAM_ERR
                self.turnoutPulse = int(property[1].strip())
            else:
                notify(self,
                       PANIC,
                       "Property: "
                       + property[0].strip()
                       + " not supported for actuator "
                       + str(self.jmriAct)
                       )
                self.state.opBlock(OP_FAIL)
                return RC_PARAM_ERR
            cnt += 1

# Verify properties configuration
        try:
            self.actuatorSystemName
            self.actuatorUserName
            self.actuatorType
            self.decoderURI
            self.satLink
            self.satAddr
            self.satPortBase
            self.actuatorSubType
        except:
            notify(
                self,
                PANIC,
                "Property: Missing general properties for actuator: "
                + str(self.jmriAct),
            )
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR

        if self.actuatorType == "TURNOUT" and self.actuatorSubType == "SOLENOID":
            try:
                self.turnoutDirection,
                self.turnoutPulse
            except:
                notify(
                    self,
                    PANIC,
                    "Property: Missing properties for solenoid turnout actuator: "
                    + str(self.jmriAct),
                )
                self.state.opBlock(OP_FAIL)
                return RC_PARAM_ERR
            self.satPorts = {"portBase":self.satPortBase, "portCnt":2} #NOT USED ANYWHERE
        elif self.actuatorType == "TURNOUT" and self.actuatorSubType == "SERVO":
            try:
                self.turnoutDirection
                self.turnoutThrownPos 
                self.turnoutClosedPos
                self.actuatorSpeed
            except:
                notify(
                    self,
                    PANIC,
                    "Property: Missing properties for servo turnout actuator: "
                    + str(self.jmriAct)
                )
                self.state.opBlock(OP_FAIL)
                return RC_PARAM_ERR
            self.satPorts = {"portBase":self.satPortBase, "portCnt":1}
        elif self.actuatorType == "GENERIC" and (self.actuatorSubType == "PWM1_25K" or self.actuatorSubType == "PWM100" or self.actuatorSubType == "PULSE" or self.actuatorSubType == "ONOFF"):
            try:
                self.actuatorInvert
                self.actuatorOnVal
                self.actuatorOffVal
                self.actuatorSpeed
            except:
                notify(
                    self,
                    PANIC,
                    "Property: Missing properties for generic actuator: "
                    + str(self.jmriAct),
                )
                self.state.opBlock(OP_FAIL)
                return RC_PARAM_ERR
            self.satPorts = {"portBase":self.satPortBase, "portCnt":1}
        else:
            notify(
                self,
                PANIC,
                "Property: configuration error for actuator: "
                + str(self.jmriAct),
                )
            self.state.opBlock(OP_FAIL)
            return RC_PARAM_ERR
        return RC_OK

    def register(self, satHandle):
        self.satHandle = satHandle
        if (self.state.getOpState() & OP_FAIL) or not (self.state.getOpState() & OP_INIT):
            return RC_GEN_ERR
        if (self.decoderURI != self.satHandle.getDecoderURI()) or (self.satLink != self.satHandle.getSatLink()) or (self.satAddr != self.satHandle.getSatAddr()) :
            return RC_GEN_ERR
        self.satHandle = satHandle
        self.state.setParent(self.satHandle)
        notify(self, DEBUG_TERSE,
                "Actuator: "
                + str(self.actuatorSystemName)
                + " Registered"
                + ", Decoder: "
                + str(self.satHandle.getDecoderURI())
                + ", Satelite Link: "
                + str(self.satHandle.getSatLink())
                + ", Satelite Address: "
                + str(self.satHandle.getSatAddr())
                + ", Actuator base port: "
                + str(self.satPortBase)
                + "has been registered"
                )
        return RC_OK

    def start(self):
        notify(self, 
               DEBUG_TERSE,
               "Starting Actuator: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) + "/" +
                str(self.getSatAddr()) + "/" +
                str(self.satPortBase) +
                " OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        if self.state.getOpState() & OP_FAIL or not (self.state.getOpState() & OP_INIT):
            notify(self, 
                   ERROR,
                    "Actuator: " +
                    str(self.getDecoderURI()) + "/" +
                    str(self.getSatLink()) + "/" +
                    str(self.getSatAddr()) + "/" +
                    str(self.satPortBase) +
                    " could not be started, " +
                    "OpState: " +
                    getOpBlockStateStr(self.state.getOpState()) +
                    " return code: " +
                    str(RC_GEN_ERR)
                )
            return RC_GEN_ERR

        self.state.opDeBlock(OP_INIT)
        notify(self, 
               DEBUG_TERSE,
               "Actuator: " +
                str(self.getDecoderURI()) + "/" +
                str(self.getSatLink()) + "/" +
                str(self.getSatAddr()) + "/" +
                str(self.satPortBase) +
                " has been started " +
                "OpState: " +
                getOpBlockStateStr(self.state.getOpState())
                )
        return RC_OK

    def getXmlConfigTree(self, decoderURI):
        if self.state.getOpState() & OP_FAIL:
            return RC_GEN_ERR
        try:
            notify(self, DEBUG_TERSE, "Providing actuator decoders .xml configuration")
            actuatorXml = ET.Element("Actuator")
            actAddr = ET.SubElement(actuatorXml, "ActAddr")
            actAddr.text = self.actuatorAddr
            satLink = ET.SubElement(actuatorXml, "SatLink")
            satLink.text = str(self.satLink)
            satAddr = ET.SubElement(actuatorXml, "SatAddr")
            satAddr.text = str(self.satAddr)
            satPort = ET.SubElement(actuatorXml, "SatPort")
            satPort.text = str(self.satPortBase)
            actSystemName = ET.SubElement(actuatorXml, "ActSystemName")
            actSystemName.text = self.actuatorSystemName
            actUserName = ET.SubElement(actuatorXml, "ActUserName")
            actUserName.text = self.actuatorUserName
            actType = ET.SubElement(actuatorXml, "ActType")
            actType.text = self.actuatorType
            actSubType = ET.SubElement(actuatorXml, "ActSubType")
            actSubType.text = self.actuatorSubType

            if self.actuatorType == "TURNOUT" and self.actuatorSubType == "SERVO":
                actTopic = ET.SubElement(actuatorXml, "Topic")
                actTopic.text = MQTT_TURNOUT_TOPIC
                actDesc = ET.SubElement(actuatorXml, "ActDesc")
                direction = ET.SubElement(actDesc, "Direction")
                direction.text = self.turnoutDirection
                thrownPos = ET.SubElement(actDesc, "ThrownPos")
                thrownPos.text = str(self.turnoutThrownPos)
                closedPos = ET.SubElement(actDesc, "ClosedPos")
                closedPos.text = str(self.turnoutClosedPos)
                speed = ET.SubElement(actDesc, "Speed")
                speed.text = str(self.actuatorSpeed)
            elif self.actuatorType == "TURNOUT" and self.actuatorSubType == "SOLENOID":
                actTopic = ET.SubElement(actuatorXml, "Topic")
                actTopic.text = MQTT_TURNOUT_TOPIC
                actDesc = ET.SubElement(actuatorXml, "ActDesc")
                direction = ET.SubElement(actDesc, "Direction")
                direction.text = self.turnoutDirection
                pulse = ET.SubElement(actDesc, "Pulse")
                pulse.text = str(self.turnoutPulse)
            elif self.actuatorType == "GENERIC":
                actTopic = ET.SubElement(actuatorXml, "Topic")
                actTopic.text = MQTT_LIGHT_TOPIC
                actDesc = ET.SubElement(actuatorXml, "ActDesc")
                invert = ET.SubElement(actDesc, "Invert")
                invert.text = self.actuatorInvert
                onVal = ET.SubElement(actDesc, "OnVal")
                onVal.text = str(self.actuatorOnVal)
                offVal = ET.SubElement(actDesc, "OffVal")
                offVal.text = str(self.actuatorOffVal)
                speed = ET.SubElement(actDesc, "Speed")
                speed.text = str(self.actuatorSpeed)
            else:
                notify(self,
                    PANIC,
                    "Actuator type: "
                    + self.actuatorType
                    + " not supported")
                return RC_GEN_ERR
            return actuatorXml
        except:
            return None

    def onStateChange(self):
        if self.state.getOpState():
            self.panicHandle = PANIC_HANDLER.engage(self,
                                            "Actuator: " +
                                            str(self.getDecoderURI()) + "/" +
                                            str(self.getSatLink()) + "/" +
                                            str(self.getSatAddr()) + "/" +
                                            str(self.getActPort()) + 
                                            " down - opState: " +
                                            getOpBlockStateStr(self.state.getOpState())
                                            )
        else:
            PANIC_HANDLER.disEngage(self.panicHandle)
        notify(self, INFO,
               "Actuator: " +
               str(self.getDecoderURI()) + "/" +
               str(self.getSatLink()) + "/" +
               str(self.getSatAddr()) + "/" +
               str(self.getActPort()) + 
               " has changed operational status to: " + 
               getOpBlockStateStr(self.state.getOpState())
               )

    def admBlock(self):
        return self.state.admBlock()

    def admDeBlock(self):
        return self.state.admDeBlock()

    def getAdmState(self):
        return self.state.getAdmState()

    def getOpState(self):
        return self.state.getOpState()

    def getDecoderURI(self):
        try:
            return self.decoderURI
        except:
            return RC_GEN_ERR

    def getSatLink(self):
        try:
            return self.satLink
        except:
            return RC_GEN_ERR

    def getSatAddr(self):
        try:
            return self.satAddr
        except:
            return RC_GEN_ERR

    def getActPort(self):
        try:
            return self.satAddr
        except:
            return RC_GEN_ERR

    def getSystemName(self):
        try:
            return self.actuatorSystemName
        except:
            return RC_GEN_ERR

    def getUserNameName(self):
        try:
            return self.actuatorUserName
        except:
            return RC_GEN_ERR

    def getPortMap(self):
        try:
            return self.portMap
        except:
            return RC_GEN_ERR

    def __del__(self):
        pass


class northAPI:
    def __init__(self, topDecoderHandle):
        self.topDecoderHandle = topDecoderHandle
        MQTT.subscribe(MQTT_NB_API_REQ, mqttListener(self.onAPIGetRequest))

    def onAPIGetRequest(self, topic, messageXml):
        response = {}

        message = json.dumps(xml2dict.parse(messageXml, schema), indent=4)
        if messsage["request"] == "getDecoders":
            decoders = self.topDecoderHandle.getDecoders()
            for decoder in decoders:
                response[decoder] = {"admState":getAdmStateStr(decoders[decoder].getAdmState()), "opState":getOpStateStr(decoders[decoder].getOpState())}
                message[response] = response
                MQTT.publish(MQTT_NB_API_RESP, dict2XML(message))


# __main__
topDecoder = topDeoderCordidinator()
northAPI(topDecoder)
# All test code below:
"""
global TRACE
TRACE = trace()
TRACE.setDebugLevel(INFO)
myPorts = resourceTracker()
print("---registerResource test-bench---")
print("test bench printouts are surrounded by --- message ---")
print("Initial port allocation map: " + str(myPorts.getResources()))
print("")

print("---Allocating ports from scratch: decoder0-satLink0-sat0 [None,None,0,1]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat0":[None,None,0,1]}}})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Allocating ports in a new branch: decoder0-satLink0-sat1 [None,None,2,3]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat1":[None,None,2,3]}}})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Allocating ports in a new top branch: decoder1-satLink0-sat0 [None,None,4,5]---")
print("---Returns: " + str(myPorts.registerResource({"decoder1":{"satLink0":{"sat0":[None,None,4,5]}}})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Allocating/extending ports: decoder1-satLink0-sat0 [None,None,None,None,6,7]---")
print("---Returns: " + str(myPorts.registerResource({"decoder1":{"satLink0":{"sat0":[None,None,None,None,6,7]}}})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Allocating/extending ports: decoder1-satLink0-sat0 [8, 9]---")
print("---Returns: " + str(myPorts.registerResource({"decoder1":{"satLink0":{"sat0":[8,9]}}})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Allocating ports in a shallow branch: decoder2 [8,None,9,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder2":[8,None,9,None]})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Trying to allocate already used ports in a deep branch: decoder0-satLink0-sat0 [None,None,0,1]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat0":[None,None,0,1]}}})))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Trying to allocate already used ports in a shallow branch: decoder2 [8,None,9,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder2":[8,None,9,None]})) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Requesting port allocation status for a deep branch: decoder0-satLink0-sat0 []---")
print("---Returns: " + str(myPorts.getResource({"decoder0":{"satLink0":{"sat0":[]}}})))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Requesting port allocation status for a shallow branch: decoder2 []---")
print("---Returns: " + str(myPorts.getResource({"decoder2":[]})))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Requesting port allocation status for a non existing deep branch: decoder0-satLink0-sat99 []---")
print("---Returns: " + str(myPorts.getResource({"decoder0":{"satLink0":{"sat99":[]}}})))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---Requesting port allocation status for a non existant shallow branch: decoder99 []---")
print("---Returns: " + str(myPorts.getResource({"decoder99":[]})))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating some but not all ports for a deep branch: decoder0-satLink0-sat0 [None,None,None,1]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat0":[None,None,None,1]}}}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating all ports for a deep branch: decoder0-satLink0-sat0 [None,None,0,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat0":[None,None,0,None]}}}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating some but not all ports for a shallow branch: decoder2 [8,None,None,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder2":[8,None,None,None]}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating all ports for a shallow branch: decoder2 [None,None,9,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder2":[None,None,9,None]}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating ports for a non existant deep branch: decoder0-satLink0-sat99 [None,None,0,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat99":[100,101,None,None]}}}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating ports for a non existant shallow branch: decoder99 [None,None,0,None]---")
print("---Returns: " + str(myPorts.registerResource({"decoder99":[100,101,None,None]}, delete=True)))
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---De-allocating all remaining ports for all branches---")
print("---Returns: " + str(myPorts.registerResource({"decoder0":{"satLink0":{"sat1":[None,None,2,3]}}}, delete=True)) + "---")
print("---Returns: " + str(myPorts.registerResource({"decoder1":{"satLink0":{"sat0":[None,None,4,5,6,7]}}}, delete=True)) + "---")
print("---Result: " + str(myPorts.getResources()) + "---")
print("")

print("---End registerResource test-bench---")
"""