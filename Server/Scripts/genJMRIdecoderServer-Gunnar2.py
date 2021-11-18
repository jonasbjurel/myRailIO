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
import java
import java.beans
import javax.swing
import jmri
import time
from time import sleep
import xml.etree.ElementTree as ET
import xml.dom.minidom
from org.python.core.util import StringUtil
import inspect
import time
import threading
import traceback

# ==============================================================================================================================================
# Constants
# ==============================================================================================================================================
#
# System constants
# ----------------------------------------------------------------------------------------------------------------------------------------------

# Logging constants
DEBUG_VERBOSE = 4
DEBUG_TERSE = 3
INFO = 2
ERROR = 1
PANIC = 0

# OP states
OP_NONE = "None"
OP_INIT = "Init"
OP_CONFIG = "Configured"
OP_UNUSED = "Unused"
OP_CONNECTED = "Connected"
OP_WORKING = "Working"
OP_FAIL = "Failed"
OP_DISABLE = "Disabled"

# Application constants
# ----------------------------------------------------------------------------------------------------------------------------------------------

# Config.xml variable
CONFIGXMLVAR = "IMgenericDecoderConfigXml"

# MQTT Topics
MQTT_PING_UPSTREAM_TOPIC = "track/decoderSupervision/upstream/"
MQTT_PING_DOWNSTREAM_TOPIC = "track/decoderSupervision/downstream/"
MQTT_CONFIG_TOPIC = "track/decoderMgmt/"
MQTT_OPSTATE_TOPIC = "track/opState/"
MQTT_LOG_TOPIC = "track/log/"
MQTT_ASPECT_TOPIC = "track/lightgroups/lightgroup/"

# MQTT Payload
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
FAULT_ASPECT = "<Aspect>FAULT</Aspect>"
PING = "<Ping/>"

# XML parse filters
MANSTR = {"expected": True, "type": "str", "values": None, "except": "PANIC"}
OPTSTR = {"expected": None, "type": "str", "values": None, "except": "PANIC"}
MANINT = {"expected": True, "type": "int", "values": None, "except": "PANIC"}
OPTINT = {"expected": None, "type": "int", "values": None, "except": "PANIC"}
MANFLOAT = {"expected": True, "type": "float", "values": None, "except": "PANIC"}
OPTFLOAT = {"expected": None, "type": "float", "values": None, "except": "PANIC"}
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
def parse_xml(xmlTree, tagDict):
    #   tagDict: {"Tag" : {"expected" : bool, "type" : "int/float/int", "values" : [], "except": "error/panic/info}, ....}
    res = {}
    for child in xmlTree:
        tagDesc = tagDict.get(child.tag)

        if tagDesc == None:
            continue
        value = validateXmlText(
            child.text.strip(), tagDesc.get("type"), tagDesc.get("values")
        )
        if tagDesc.get("expected") == None:
            if value != None:
                res[child.tag] = value
            else:
                through_xml_error(
                    tagDesc.get("except"),
                    "Tag: "
                    + child.tag
                    + " Tag.text: "
                    + child.text
                    + " did not pass type checks",
                )
        elif tagDesc.get("expected") == True:
            if value != None:
                res[child.tag] = value
            else:
                through_xml_error(
                    tagDesc.get("except"),
                    "Tag: "
                    + child.tag
                    + " Tag.text: "
                    + child.text
                    + " did not pass type checks",
                )
        else:
            through_xml_error(
                tagDesc.get("except"), "Tag: " + child.tag + " was not expected"
            )

    for tag in tagDict:
        if tagDict.get(tag).get("expected") != None:
            if res.get(tag) == None:
                if tagDict.get(tag).get("expected") == True:
                    through_xml_error(
                        tagDict.get(tag).get("except"),
                        "Tag: " + tag + " was expected but not found",
                    )
    return res


def validateXmlText(txt, type, values):
    if txt == None:
        return None
    if type == None:
        return txt
    elif type == "str":
        res = str(txt)
    elif type == "int":
        res = int(txt)
    elif type == "float":
        res = float(txt)
    else:
        return None

    if values == None:
        return res
    else:
        for value in values:
            if res == value:
                return res
        return None


def through_xml_error(_except, errStr):
    if _except == "DEBUG_VERBOSE":
        debug = DEBUG_VERBOSE
    elif _except == "DEBUG_TERSE":
        debug = DEBUG_TERSE
    elif _except == "INFO":
        debug = INFO
    elif _except == "ERROR":
        debug = ERROR
    elif _except == "PANIC":
        debug = PANIC
    else:
        debug = PANIC
    notify(None, debug, errStr)
    return 0


# ==============================================================================================================================================
# Helper fuctions: decode system name
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
# Class: trace
# Purpose:  The trace module provides the visibility and program tracing capabilities needed to understand what is ongoing, needed to debug
#           the program and the overall solution. Depending on log-level set by the setDebugLevel(debugLevel, output = ...) the verbosity of the
#           logs, and the log receivers can be set. Log receivers can be any of stdOut, MQTT or Rsyslog
# Data structures: Almost stateless.
# Methods:
#   setDebugLevel(...)
#           purpose: Setting the debug level output and the proper log receiver:
#           setDebugLevel([DEBUG_VERBOSE | DEBUG_TERSE | INFO | ERROR | PANIC], output = {"Console" :  [True | False], "rSyslog" : [True | False],
#                         "Mmqtt" : [True | False], debugClasses = not implemented, leve at default, errCallStack = [True|False], errTerminate = [True|False) :)
#   notify(...)
#           purpose: notify trace receivers of debug events, gemeral events, Errors and PANICs
#           notify(context[classObj|None], severity[DEBUG_VERBOSE | DEBUG_TERSE | INFO | ERROR | PANIC], "notify text")
#
# TODO: Generalize as a common module for server and client, implement RSYSLOG, MQTT and also an MQTT Log receiver.
#       Locks for all potential outputs
# ==============================================================================================================================================
def notify(caller, severity, notificationStr):
    TRACE.notify(caller, severity, notificationStr)

class trace:
    def __init__(self):
        self.lock = threading.Lock()
        self.debugLevel = DEBUG_TERSE
        self.debugClasses = None
        self.console = True
        self.mqtt = False


    def setDebugLevel(
        self,
        debugLevel,
        output={"Console": True, "rSyslog": False},
        debugClasses=None,
        errCallStack=False,
        errTerminate=False,
    ):
        self.errCallStack = errCallStack
        self.errTerminate = errTerminate
        self.debugLevel = debugLevel
        self.console = output.get("Console")
        self.rSyslog = output.get("rSyslog")
        if debugClasses == None:
            self.debugClasses = None
        else:
            self.debugClasses = debugClasses

    def notify(self, caller, severity, notificationStr):
        if severity == DEBUG_VERBOSE:
            if self.debugLevel >= DEBUG_VERBOSE:
                notification = str(time.time()) + ": VERBOSE DEBUG:  " + notificationStr
                if self.debugClasses == None:
                    self.__deliverNotification(notification)
                    return
                else:
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == DEBUG_TERSE:
            if self.debugLevel >= DEBUG_TERSE:
                #                notification = time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.localtime()) + ": TERSE DEBUG:  " + notificationStr
                notification = str(time.time()) + ": TERSE DEBUG:  " + notificationStr
                if self.debugClasses == None:
                    self.__deliverNotification(notification)
                    return
                else:
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == INFO:
            if self.debugLevel >= INFO:
                #                notification = time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.localtime()) + ": INFO:  " + notificationStr
                notification = str(time.time()) + ": INFO:  " + notificationStr

                if self.debugClasses == None:
                    self.__deliverNotification(notification)
                    return
                else:
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == ERROR:
            if self.errCallStack == True:
                notification = (
                    str(time.time())
                    + ": ERROR!:  "
                    + notificationStr
                    + "\nCall Stack:\n"
                    + str(inspect.stack())
                )
            else:
                notification = str(time.time()) + ": ERROR!:  " + notificationStr
            self.__deliverNotification(notification)
            if self.errTerminate == True:
                # Call back to registered entries for emergency
                self.terminate(notification)
            else:
                return

        if severity == PANIC:
            notification = (
                str(time.time())
                + ": PANIC!:  "
                + notificationStr
                + "\nCall Stack:\n"
                + str(inspect.stack())
            )
            # Call back to registered entries for emergency
            self.terminate(notification)
            return 0

    def terminate(self, notification):
        self.__deliverNotification(notification)
        MQTT.publish("/trains/track/supervision/" + URL + "/", "offLine")
        info(self, INFO, "Terminating ....")
        time.sleep(1)
        sys.exit()
        return

    def __classNotification(self, caller, notification):  # Currently not supported
        callerClass = type(itertools.count(0)).__name__
        for debugClass in self.debugClasses:
            if debugClass == callerClass:
                self.__deliverNotification(notification)
                return

    def __deliverNotification(self, notification):
        self.lock.acquire()
        if self.console == True:
            print(notification)
        # if self.rSyslog == True :     #Implement rsyslog support
        # RSYSLOG.post(notiffication)
        self.lock.release()

    def rsyslogNotification(self):
        pass


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
# 		<ServerURL>lightcontroller1.bjurel.com</ServerURL>
# 		<ClientURL>lightcontroller1.bjurel.com</ClientURL>
# 		<NTPServer>pool.ntp.org</NTPServer>
# 		<TIMEZONE>+1</TIMEZONE>
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
# (buid Decoder table)                  !                     !                        !
#         !----(startLightgroups)----->!                     !                        !
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
    def __init__(self):
        self.opState = OP_INIT
        self.decoderTable = {}
        self.faultyDecoders = []
        self.powerOffAtFault = True 
        global TRACE
        TRACE = trace()
        TRACE.setDebugLevel(INFO)
        global MQTT
        MQTT = jmri.InstanceManager.getDefault(
            jmri.jmrix.mqtt.MqttSystemConnectionMemo
        ).getMqttAdapter()
        self.lock = threading.Lock()
        try:
            genDecoderXmlFile = memories.provideMemory(CONFIGXMLVAR)
            if genDecoderXmlFile == None or genDecoderXmlFile.value == None:
                raise Exception()
        except:
            notify(self, PANIC, "No genericDecoderConfigXml file defined - please define the genericDecoderConfigXml in JMRI variable with system name: " + CONFIGXMLVAR + " - exiting...")
        notify(self, INFO, "Using genericDecoderConfigXml file :" + genDecoderXmlFile.value)
        self.xmlFile = genDecoderXmlFile.value
        self.__configure()

    def __configure(self):
        notify(self, INFO, "Starting to configure top decoder class object and all subordinate class objects from \"" + self.xmlFile + "\" and JMRI meta data")
        controllersXmlTree = ET.parse(self.xmlFile)
        if str(controllersXmlTree.getroot().tag) != "Decoders":
            notify(self, PANIC, "Controllers .xml  missformated:\n")
            return 1
        else:
            #Defaults
            self.xmlDescription = ""
            self.xmlDate = ""
            self.TimeZone = 0
            self.RsyslogReceiver = ""
            self.Loglevel = "INFO"
            self.NTPServer = None
            self.TimeZone = 0
            self.RsyslogReceiver = None
            self.Loglevel = INFO
            self.powerOffAtFault = False
            self.disableDecodersAtFault = False
            self.pingPeriod = float(10)
            topDecoderXmlConfig = parse_xml(
                controllersXmlTree.getroot(),
                {
                    "Author": OPTSTR,
                    "Description": OPTSTR,
                    "Version": OPTSTR,
                    "Date": OPTSTR,
                    "NTPServer": OPTSTR,
                    "TIMEZONE": OPTINT,
                    "RSyslogServer": OPTSTR,
                    "LogLevel": OPTSTR,
                    "PowerOffAtFault" : OPTSTR,
                    "DisableAllDecodersAtFault" : OPTSTR, 
                    "PingPeriod" : OPTFLOAT
                },
            )
            if topDecoderXmlConfig.get("Author") != None: self.xmlAuthor = topDecoderXmlConfig.get("Author")
            if topDecoderXmlConfig.get("Description") != None : self.xmlDescription = topDecoderXmlConfig.get("Description")
            if topDecoderXmlConfig.get("Version") != None: self.xmlVersion = topDecoderXmlConfig.get("Version")
            if topDecoderXmlConfig.get("Date") != None: self.xmlDate = topDecoderXmlConfig.get("Date")
            if topDecoderXmlConfig.get("NTPServer") != None: self.NTPServer = topDecoderXmlConfig.get("NTPServer")
            if topDecoderXmlConfig.get("TIMEZONE") != None: self.TimeZone = str(topDecoderXmlConfig.get("TIMEZONE"))
            if topDecoderXmlConfig.get("RsyslogReceiver") != None: self.RsyslogReceiver = topDecoderXmlConfig.get("RsyslogReceiver")
            if topDecoderXmlConfig.get("LogLevel") != None:
                self.LoglevelStr = topDecoderXmlConfig.get("LogLevel")
                if topDecoderXmlConfig.get("LogLevel") == "DEBUG_VERBOSE": self.Loglevel = DEBUG_VERBOSE
                elif topDecoderXmlConfig.get("LogLevel") == "DEBUG_TERSE": self.Loglevel = DEBUG_TERSE
                elif topDecoderXmlConfig.get("LogLevel") == "INFO": self.Loglevel = INFO
                elif topDecoderXmlConfig.get("LogLevel") == "ERROR": self.Loglevel = ERROR
                elif topDecoderXmlConfig.get("LogLevel") == "PANIC": self.Loglevel = PANIC
            TRACE.setDebugLevel(self.Loglevel)
            if topDecoderXmlConfig.get("PowerOffAtFault") != None: 
                if topDecoderXmlConfig.get("PowerOffAtFault") == "Yes": self.powerOffAtFault = True
            if topDecoderXmlConfig.get("DisableAllDecodersAtFault") != None: 
                if topDecoderXmlConfig.get("DisableAllDecodersAtFault") == "Yes": 
                    self.disableDecodersAtFault = True
                elif topDecoderXmlConfig.get("DisableAllDecodersAtFault") == "No":
                    self.disableDecodersAtFault = False
                else: notify(self, INFO, "\"DisableAllDecodersAtFault\" not set to yes/no, setting it to no")
                global DISABLE_ALL_DECODERS_AT_DECODER_FAULT
                DISABLE_ALL_DECODERS_AT_DECODER_FAULT = self.disableDecodersAtFault
            if topDecoderXmlConfig.get("PingPeriod") != None: 
                self.pingPeriod = topDecoderXmlConfig.get("PingPeriod")
            global PING_INTERVAL
            PING_INTERVAL = self.pingPeriod

            for child in controllersXmlTree.getroot():
                if str(child.tag) == "Lightgroups":
                    self.lightgroups = lightgroups(self)
                    if self.lightgroups.configure(child) != 0:
                        notify(self, PANIC, "Failed to configure Lightgroups")
                        return 1
                    decoderURIs = self.lightgroups.getDecoderURIs()
                    notify(self, INFO, "Following DecoderURIs detected: " + str(decoderURIs))
                    self.__registerDecoderURIs(decoderURIs)

                elif str(child.tag) == "Turnouts":
                    notify(self, INFO, "Turnouts not yet supported")
                    pass
                elif str(child.tag) == "Sensors":
                    notify(self, INFO, "Sensors not yet supported")
                    pass
                elif str(child.tag) == "Actuators":
                    notify(self, INFO, "Actuators not yet supported")
                    pass
            self.__startDecoders()
            self.opState = OP_CONFIG
            return 0

    def __registerDecoderURIs(self, decoderURIs):
        notify(self, INFO, "Topdecoder Registering decoders URIs")
        for decoderURI in decoderURIs :
            notify(self, DEBUG_TERSE, "Topdecoder registering decoder URI: " + decoderURI)
            if self.decoderTable.get(decoderURI) == None :
                self.decoderTable[decoderURI] = None
        notify(self, INFO, "All decoders registered")
        return 0

    def __startDecoders(self):
        notify(self, INFO, "Starting decoders")
        for decoderURI in list(self.decoderTable.keys()):
            notify(self, DEBUG_TERSE, "Starting decoder obj for decoder: " + str(decoderURI))
            self.decoderTable[decoderURI] = decoder(self, decoderURI)
            self.decoderTable.get(decoderURI).start(self.lightgroups)
            self.opState = OP_WORKING
        notify(self, INFO, "All decoders started")
        return 0

    def getXmlConfigTree(self, URI):
        notify(self, DEBUG_TERSE, "Providing top decoder over arching decoder .xml configuration")
        top = ET.Element("Top")
        child = ET.SubElement(top, "Author")
        child.text = self.xmlAuthor
        child = ET.SubElement(top, "Description")
        child.text = self.xmlDescription
        child = ET.SubElement(top, "Version")
        child.text = self.xmlVersion
        child = ET.SubElement(top, "Date")
        child.text = self.xmlDate
        child = ET.SubElement(top, "NTPServer")
        child.text = self.NTPServer
        child = ET.SubElement(top, "TimeZone")
        child.text = self.TimeZone
        child = ET.SubElement(top, "RsyslogReceiver")
        child.text = self.RsyslogReceiver
        child = ET.SubElement(top, "Loglevel")
        child.text = self.LoglevelStr
        child = ET.SubElement(top, "PingPeriod")
        child.text = str(self.pingPeriod)
        top.insert(9, self.lightgroups.getXMLConfigTree(URI))
        return top

    def handleDecoderFault(self, decoderURI):
        self.lock.acquire()
        notify(self, INFO, "Decoder: " + decoderURI + " is reported as faulty or not yet operational")
        found = False
        for faultyDecoder in self.faultyDecoders:
            if faultyDecoder == decoderURI:
                found = True
                notify(
                    self,
                    DEBUG_TERSE,
                    "Decoder: "
                    + decoderURI
                    + " is reported as faulty/not operational, but was already marked as faulty/non operational - doing nothing more!",
                )
                break
        if found == False:
            self.faultyDecoders.append(decoderURI)
            if self.powerOffAtFault == True:
                try:
                    jmri.PowerManager.setPower(jmri.PowerManager.OFF) # Needs verification
                except:
                    notify(self, ERROR, "Failed to power off the tracks")
                notify(
                    self,
                    INFO,
                    "Decoder: "
                    + decoderURI
                    + ' is reported as faulty - Powering off the track as per the "faulty decoder" policy',
                )
            if self.disableDecodersAtFault == True:
                notify(
                    self,
                    INFO,
                    "Decoder: "
                    + decoderURI
                    + ' is reported as faulty - disabling all decoders as per the "faulty decoder policy"',
                )
                for decoder in list(self.decoderTable.keys()):
                    try:
                        self.decoderTable.get(decoder).disable()
                        notify(self, INFO, "Disabling decoder: " + str(decoder))
                    except :
                        notify(self, DEBUG_TERSE, "Could not disable decoder: " + str(decoder) + ", decoderTable not fully populated")
                    #FIX!!!!!
                    #Here it where it breaks when the self.decoderTable is not fully populated, use decoderHandle non positional arg instead
                    
        self.lock.release()
        notify(self, DEBUG_TERSE, "Following decoders are faulty: " + str(self.faultyDecoders))
        return 0

    def handleDecoderRecovery(self, decoderHandle, decoderURI):
        self.lock.acquire()
        notify(self, INFO, "Decoder: " + decoderURI + " is reported as recovered")
        try:
            self.faultyDecoders.remove(decoderURI)
        except:
            notify(
                self,
                DEBUG_TERSE,
                "Decoder: "
                + decoderURI
                + " is reported as recovered, but was never declared as faulty - doing nothing more!",
            )
        if len(self.faultyDecoders) == 0:
            notify(
                self,
                INFO,
                'All Decoders recovered, in case the track was powered off as per the "faulty decoder" policy, it needs to be powered on manually',
            )
            if DISABLE_ALL_DECODERS_AT_DECODER_FAULT == True:
                notify(self, INFO, "Enabling decoders")
                for decoder in list(self.decoderTable.keys()):
                    notify(self, DEBUG_TERSE, "Enabling decoder: " + str(decoder))
                    self.decoderTable.get(decoder).enable()
        else:
            if DISABLE_ALL_DECODERS_AT_DECODER_FAULT == True:
                notify(self, INFO, "All Decoders stil NOT recovered, disabling decoder: " +  decoderURI + " as per the faulty decoder policy")
                self.decoderTable.get(decoderURI).disable()
            notify(self, DEBUG_TERSE, "Following decoders are still faulty: " + str(self.faultyDecoders))
        self.lock.release()
        return 0

    def getOpState(self):
        self.lock.acquire()
        opState = self.opState
        self.lock.release()
        return opState


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
        self.top = top
        self.lock = threading.Lock()
        self.URI = URI
        self.opState = OP_INIT
        self.supervision = False
        self.missedPingCnt = 0
        notify(
            self,
            INFO,
            "New decoder: "
            + self.URI
            + " initialized, operational state is now: "
            + OP_INIT,
        )
        return

    def start(self, lightgroupsObj):
        self.lightgroupsObj = lightgroupsObj
        self.lightgroupsTable = lightgroupsObj.register(self, self.URI)
        notify(self, INFO, "Starting compilation of decoder .xml configuration for: " + self.URI)
        decoder = ET.Element("Decoder")
        decoder.insert(0, self.top.getXmlConfigTree(self.URI))
        decoderXMLstr = xml.dom.minidom.parseString(
            ET.tostring(decoder, method="xml").decode()
        ).toprettyxml()
        notify(
            self,
            INFO,
            "Decoder: "
            + self.URI
            + " XML configuration compiled, sending it to the decoder: \n"
            + decoderXMLstr,
        )
        MQTT.publish(MQTT_CONFIG_TOPIC + self.URI, decoderXMLstr)
        MQTT.subscribe(MQTT_PING_UPSTREAM_TOPIC + self.URI, mqttListener(self.onPing))
        MQTT.subscribe(MQTT_OPSTATE_TOPIC + self.URI, mqttListener(self.onOpStateChange))
        MQTT.subscribe(MQTT_LOG_TOPIC + self.URI, mqttListener(self.onLogMessage))
        self.opState = OP_CONFIG
        notify(
            self,
            INFO,
            "Decoder: "
            + self.URI
            + " started, operational state is set to: "
            + OP_CONFIG,
        )
        self.__handleDecoderFault() # Set decoder fault until it has announced itself through MQTT "onLine"
        return 0

    def enable(self):
        action = None
        self.lock.acquire()
        if self.opState == OP_DISABLE:
            action = "enable"
            notify(
                self,
                INFO,
                "Decoder "
                + self.URI
                + "  enabled, operational state changed to: "
                + OP_WORKING
                + ", re-setting all signal mast aspects",
            )
        self.lock.release()
        if action == "enable":
            self.lock.acquire()
            self.opState = OP_WORKING
            self.lock.release()
            self.__unSetFaultAspects()
        self.__startSupervision()
        return 0

    def disable(self):
        action = None
        self.lock.acquire()
        if self.opState == OP_WORKING:
            self.opState = OP_DISABLE
            action = "setFaultAspects"
            notify(
                self,
                INFO,
                "Decoder "
                + self.URI
                + "  disabled, operational state changed to: "
                + self.opState,
            )
        else:
            notify(
                self,
                INFO,
                "Decoder "
                + self.URI
                + "  was ordered to be disabled, but Operational state was not: "
                + OP_WORKING
                + " but: "
                + self.opState
                + " - will do nothing",
            )
        self.lock.release()
        if action == "setFaultAspects":
            self.__setFaultAspects()
        return 0

    def onOpStateChange(self, topic, payload):
        self.lock.acquire()
        action = None
        if payload == DECODER_UP:
            notify(self, INFO, "Decoder: " + self.URI + " has declared it self as onLine - trying to recover it")
            action = "handleDecoderRecovery"
        elif payload == DECODER_DOWN:
            notify(self, INFO, "Decoder: " + self.URI + " has declared it self as offLine - bringing it down")
            action = "handleDecoderFault"
        self.lock.release()
        if action == "handleDecoderFault":
            self.__handleDecoderFault()
        elif action == "handleDecoderRecovery":
             self.__handleDecoderRecovery()
        return 0

    def onPing(self, topic, payload):
        self.lock.acquire()
        notify(self, DEBUG_VERBOSE, "Received a Ping from: " + self.URI)
        if self.opState == OP_WORKING or self.opState == OP_DISABLE:
            notify(self, DEBUG_VERBOSE, "Returning the Ping from: " + self.URI)
            self.missedPingCnt = 0
            MQTT.publish(MQTT_PING_DOWNSTREAM_TOPIC + self.URI, PING)
        self.lock.release()
        return 0

    def __onPingTimer(self):
        try:
            notify(self, DEBUG_VERBOSE, "Received a Ping timer for decoder: " + self.URI)
            if self.opState != OP_WORKING and self.opState != OP_DISABLE:
                self.lock.acquire()
                self.supervision = False
                notify(self, INFO, "Stoping supervision of decoder: " + self.URI)
                self.lock.release()
                return 0

            self.lock.acquire()
            action = None
            self.supervision = False
            if self.missedPingCnt >= 2:
                action = "handleDecoderFault"
                notify(
                    self,
                    ERROR,
                    "Decoder "
                    + self.URI
                    + "  did not adhere to the ping supervision requirements",
                )
                notify(self, INFO, "Stoping supervision of decoder: " + self.URI)

            else:
                self.missedPingCnt += 1
                threading.Timer(PING_INTERVAL, self.__onPingTimer).start()
            self.lock.release()
            if action == "handleDecoderFault" :
                self.__handleDecoderFault()
        except:
            notify(self, PANIC, "Exception in ping timer handling routines:\n" + str(traceback.format_exc()))
        return 0

    def updateLgAspect(self, lgAddr, aspect, force=False):
        self.lock.acquire()
        if self.opState == OP_WORKING or force == True:
            MQTT.publish(MQTT_ASPECT_TOPIC + self.URI + "/" + str(lgAddr), aspect)
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
        self.lock.release()
        return 0

    def onLogMessage(self, topic, payload):
        notify(self, INFO, "Remote log from: " + self.URI + ": " + payload + "\n" )
        return 0

    def __setFaultAspects(self):
        notify(
            self,
            INFO,
            "Fault aspects ordered - Fault aspect set for all signal masts belonging to decoder: "
            + self.URI,
        )
        for lightgroup in list(self.lightgroupsTable.keys()):
            notify(self, DEBUG_TERSE, "Setting fault aspect for: " + str(lightgroup))
            self.lightgroupsTable.get(lightgroup).setFaultAspect()
        return 0

    def __unSetFaultAspects(self):
        notify(
            self,
            INFO,
            "Cease of fault aspects ordered - Reseting aspects for all signal masts belonging to decoder: "
            + self.URI,
        )
        for lightgroup in list(self.lightgroupsTable.keys()):
            notify(self, DEBUG_TERSE, "Ceasing fault aspect for: " + str(lightgroup))
            self.lightgroupsTable.get(lightgroup).resendAspect()

        return 0

    def __handleDecoderFault(self):
        self.lock.acquire()
        self.opState = OP_FAIL
        self.lock.release()
        notify(
            self,
            ERROR,
            "Decoder "
            + self.URI
            + " is reported as down, Operational state changed to " +  self.opState + ", stoping suppervision of it and waiting for it to reappear",
            )
        self.__setFaultAspects()
        self.top.handleDecoderFault(self.URI)
        return 0

    def __handleDecoderRecovery(self):
        self.lock.acquire()
        action = None
        if self.opState == OP_FAIL or self.opState == OP_CONFIG:
            action = "unSetFaultAspects"
            self.opState = OP_WORKING
            notify(
                self,
                INFO,
                "Decoder "
                + self.URI
                + " has reporting it self as working, Operational state changed to: " + self.opState +  ", starting suppervision of it",
                )
        self.lock.release()
        if action == "unSetFaultAspects":
            self.__unSetFaultAspects()
        self.__startSupervision()
        self.top.handleDecoderRecovery(self, self.URI)
        return 0

    def __startSupervision(self):
        notify(self, INFO, "Starting supervision of decoder: " + self.URI)
        self.lock.acquire()
        self.missedPingCnt = 0
        if self.supervision == True:
            notify(self, DEBUG_TERSE, "Supervision of decoder: " + self.URI + " already going, letting it continue")
            self.lock.release()
            return 0
        threading.Timer(PING_INTERVAL, self.__onPingTimer).start()
        self.supervision = True
        self.lock.release()
        return 0

    def getOpState(self):
        self.lock.acquire()
        opState = self.opState
        self.lock.release()
        return opState


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
        self.opState = OP_INIT
        self.top = top
        self.mastTable = []

    def configure(self, lightgroupsXmlTree):
        if str(lightgroupsXmlTree.tag) != "Lightgroups":
            notify(self, PANIC, "Controllers .xml  missformated:\n")
            return 1
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
                    mastHandle = mast()
                    if mastHandle.configure(sm) != 0:
                        del mastHandle
                        notify(self, INFO, "Mast: " + str(sm) + " was ommitted")
                    else:
                        self.mastTable.append(
                            mastHandle
                        )  # Check if this is how to append mastTable needs to be initialized
                        notify(
                            self,
                            INFO,
                            "Mast: " + str(sm) + " was added to the mast table",
                        )
                        smCnt += 1
                if (
                    smCnt == 0
                ):  # Need to define an exception path to allow to move forward with othe lightgroups, turnouts, etc.
                    notify(
                        self, INFO, "No signal masts with propper configuration found"
                    )
                    noMasts = True
                else:
                    notify(
                        self, INFO, str(smCnt) + " Masts were added to the mast table"
                    )
                    noMasts = False
                self.mastAspects = mastAspects()
                if self.mastAspects.configure(self.MastDefinitionPath) != 0:
                    notify(self, PANIC, "Could not configure mastAspects")
                    return 1
            elif str(child.tag) == "GenericLights":
                notify(self, ERROR, "Generic lights not yet supported")
            elif str(child.tag) == "SequenceLights":
                notify(self, ERROR, "Sequence lights not yet supported")
        self.opState = OP_CONFIG
        return 0

    def getDecoderURIs(self):
        decoderURIs = []
        for mast in self.mastTable:
            decoderURI = mast.getDecoderURI()
            try:
                decoderURIs.index(decoderURI)
            except:
                decoderURIs.append(decoderURI)
        return decoderURIs

    def register(self, decoderHandle, decoderURI):
        notify(self, DEBUG_TERSE, "Registering decoder: " + decoderURI + " to lightgroups")
        lightgroupsObjTable = {}
        for mast in self.mastTable:
            x = mast.register(decoderHandle, decoderURI)
            if x != None:
                lightgroupsObjTable[list(x.keys())[0]] = list(x.values())[0]
        # more lightgroup objects
        self.opState = OP_WORKING
        return lightgroupsObjTable  # {"System Name" : object}

    def getXMLConfigTree(self, decoderURI):
        notify(self, DEBUG_TERSE, "Providing lightgroups over arching decoder .xml configuration")
        xmlLightgroups = ET.Element("Lightgroups")
        xmlLightgroups.insert(0, self.mastAspects.getXMLConfigTree(decoderURI))
        for mast in self.mastTable:
            mastXMLTree = mast.getXMLConfigTree(decoderURI)
            if mastXMLTree != None:
                xmlLightgroups.insert(0, mastXMLTree)
        return xmlLightgroups

    def getOpState(self):
        self.lock.acquire()
        opState = self.opState
        self.lock.release()
        return opState


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
    def __init__(self):
        self.opState = OP_INIT

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
            return 1
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
            return 1
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
                    return 1
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
                    return 1
        if fileFound != True:
            notify(self, PANIC, "No Appearance file found")
            return 1
        self.opState = OP_CONFIG
        notify(self, INFO, "mastAspects successfulyy configured")
        return 0

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

    def getXMLConfigTree(self, URI):
        notify(self, DEBUG_TERSE, "Providing mastAspects over arching decoders .xml configuration")
        xmlSignalmastDesc = ET.Element("SignalMastDesc")
        xmlAspects = ET.SubElement(xmlSignalmastDesc, "Aspects")
        for aspect in self.aspectTable:
            xmlAspect = ET.SubElement(xmlAspects, "Aspect")
            xmlAspectName = ET.SubElement(xmlAspect, "AspectName")
            xmlAspectName.text = aspect
            for mast in self.aspectTable.get(aspect):
                xmlMast = ET.SubElement(xmlAspectName, "Mast")
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
    def __init__(self):
        self.opState = OP_INIT
        self.lock = threading.Lock()
        self.currentAspect = None
        # Get current aspect

    def configure(self, sm):
        notify(self, INFO, "Signal mast: " + str(sm) + " found, trying to configure it")
        if smIsVirtual(sm) != 1:
            notify(
                self, INFO, "Signal mast: " + str(sm) + " is not virtal, ommiting it"
            )
            self.opState = OP_FAIL
            return 1
        self.lgSystemName = str(sm)
        self.lgAddr = smLgAddr(self.lgSystemName)
        self.lgUserName = sm.getUserName()
        self.lgType = sm.getBeanType()
        self.smType = smType(self.lgSystemName)
        self.lgComment = sm.getComment()
        if self.__configProperties() != 0:
            notify(
                self,
                INFO,
                "Signal mast: " + str(sm) + " could not be configured - ommiting it",
            )
            self.opState = OP_FAIL
            return 1
        notify(
            self,
            INFO,
            "Signal mast: "
            + str(sm)
            + " successfully created with following configuration:",
        )
        notify(
            self,
            INFO,
            "User name: "
            + self.lgUserName
            + ", Type: "
            + self.smType
            + ", Brightness: "
            + self.smBrightness
            + ", Dim time: "
            + self.smDimTime
            + ", Flash freq: "
            + self.smFlashFreq
            + ", Flash duty: "
            + str(self.smFlashDuty),
        )
        return 0

    def __configProperties(self):
        properties = (self.lgUserName.split("!*[")[-1]).split("]*!")[0]
        if len(properties) < 2:
            notify(self, INFO, "No properties found for signal mast: " + str(sm))
            self.opState = OP_FAIL
            return 1
        properties = properties.split(",")
        if len(properties) == 0:
            notify(self, INFO, "No properties found for signal mast: " + str(sm))
            return 1
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
                return 1
            if property[0].strip().upper() == "DECODER":
                self.decoderURI = property[1].strip()
            elif property[0].strip().upper() == "SEQUENCE":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        ERROR,
                        "Property: Sequence is not an integer for signal mast: "
                        + str(sm),
                    )
                    self.opState = OP_FAIL
                    return 1
                self.lgSequene = int(property[1].strip())
            elif property[0].strip().upper() == "BRIGHTNESS":
                if property[1].strip().upper() == "LOW":
                    self.smBrightness = "LOW"
                elif property[1].strip().upper() == "NORMAL":
                    self.smBrightness = "NORMAL"
                elif property[1].strip().upper() == "HIGH":
                    self.smBrightness = "HIGH"
                else:
                    notify(
                        self,
                        ERROR,
                        "Property: Brightness is not any of LOW/NORMAL/HIGH for signal mast: "
                        + str(sm)
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
                        + str(sm)
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
                        + str(sm)
                        + " - using default",
                    )
            elif property[0].strip().upper() == "FLASHDUTY":
                try:
                    int(property[1].strip())
                except:
                    notify(
                        self,
                        ERROR,
                        "Property: Flashduty is not an integer for signal mast: "
                        + str(sm)
                        + " - using default",
                    )
                else:
                    self.smFlashDuty = int(property[1].strip())
            else:
                notify(
                    self,
                    ERROR,
                    property[0].strip()
                    + " is not a valid property for signal mast: "
                    + str(sm)
                    + " - ignoring",
                )
            cnt += 1
        try:
            self.decoderURI
        except:
            notify(
                self,
                ERROR,
                "Decoder property was not defined for signal mast: " + str(sm),
            )
            self.opState = OP_FAIL
            return 1
        try:
            self.lgSequene
        except:
            notify(
                self,
                ERROR,
                "Sequence property was not defined for signal mast: " + str(sm),
            )
            self.opState = OP_FAIL
            return 1
        self.opState = OP_CONFIG
        return 0

    def getDecoderURI(self):
        return self.decoderURI

    def register(self, decoderHandle, decoderURI):
        if self.decoderURI != decoderURI:
            return None
        else:
            self.decoderHandle = decoderHandle
            self.getCurrentAspect()
            sm = masts.getBySystemName(self.lgSystemName)
            sm.addPropertyChangeListener(self.onSmChange)
            self.opState = OP_WORKING
            notify(self, INFO, "Mast: " + self.lgSystemName + " Registered, current opState: " + self.opState + ", Decoder: " + self.decoderURI+ ", Aspect: " + self.currentAspect)
            return {self.lgSystemName: self}

    def getXMLConfigTree(self, decoderURI):
        # if self.configured != True :     Need to introduce OP States and check them
        # notify(self, PANIC, "Mast is not configured - XML configuration cannot be created")
        if decoderURI != self.decoderURI:
            return None

        notify(self, DEBUG_TERSE, "Providing mast decoders .xml configuration")
        lightgroup = ET.Element("Lightgroup")
        lgAddr = ET.SubElement(lightgroup, "LGAddr")
        lgAddr.text = str(self.lgAddr)
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

    def getCurrentAspect(self):
        self.lock.acquire()
        sm = masts.getBySystemName(self.lgSystemName)
        self.currentAspect = sm.getAspect()
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

    def setFaultAspect(self):
        notify(self, INFO, "Mast: " + str(self.lgSystemName) + " is set to Fault aspect")
        self.decoderHandle.updateLgAspect(
            self.lgAddr, FAULT_ASPECT, 
            force=True
        )

    def resendAspect(self):
        notify(self, INFO, "Mast: " + str(self.lgSystemName) + " is resending its aspect")
        self.lock.acquire()
        if self.currentAspect != None:
            self.decoderHandle.updateLgAspect(
                self.lgAddr, "<Aspect>" + self.currentAspect + "</Aspect>"
            )
        self.lock.release()

    def getOpState(self):
        return self.opState

# __main__
topDeoderCordidinator()
