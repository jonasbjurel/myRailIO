#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI topDecoder class providing the genJMRI decoder management-, supervision and configuration. genJMRI provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
import os
import sys
from socket import *
import time
from datetime import datetime
import pytz
import threading
import traceback
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom
import paho.mqtt.client as pahomqtt
from momResources import *
from ui import *
from decoderLogic import *
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import trace
imp.load_source('syslog', '..\\trace\\syslog.py')
from syslog import rSyslog
imp.load_source('alarmHandler', '..\\alarmHandler\\alarmHandler.py')
from alarmHandler import *
imp.load_source('mqtt', '..\\mqtt\\mqtt.py')
from mqtt import mqtt
imp.load_source('jmriMqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from jmriMqttTopicsNPayloads import *
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"\\..\\rpc\\")
from genJMRIRpcClient import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc
imp.load_source('parseXml', '..\\xml\\parseXml.py')
from parseXml import *
from config import *



# ==============================================================================================================================================
# Constants
# ==============================================================================================================================================
# ----------------------------------------------------------------------------------------------------------------------------------------------
# System constants
# ----------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Local internal constants
#################################################################################################################################################
# End Local internal constants
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: topDecoder
# Purpose: The decoder class provides decoder management-, supervision and configuration. genJMRI provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
MQTT_DISCONNECTED_FAILURE = 0b00000001
RPC_DISCONNECTED_FAILURE =  0b00000010


#TODO:
#HANDLE CLIENTID IN A MORE GENERIC WAY
class topDecoder(systemState, schema):
    def __init__(self, win, demo=False):
        self.topDecoderOpDownStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM)[:-1]
        self.topDecoderOpUpStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM)[:-1]
        self.topDecoderAdmDownStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM)[:-1]
        trace.start()
        trace.notify(DEBUG_INFO, "Starting Top-decoder")
        alarmHandler.start()
        self.win = win
        self.parent = None
        self.demo = demo
        self.mqttClient = None
        self.mqttClientId = MQTT_CLIENT_ID
        self.rpcClient = None
        self.schemaDirty = False
        childsSchemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.setSchema(schema.TOP_DECODER_SCHEMA)
        self.appendSchema(schema.ADMIN_SCHEMA)
        self.appendSchema(schema.GIT_SCHEMA)
        self.appendSchema(schema.MQTT_SCHEMA)
        self.appendSchema(schema.JMRI_RPC_SCHEMA)
        self.appendSchema(schema.SERVICES_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.decoders.value = []
        self.childs.value = self.decoders.candidateValue
        self.nameKey.value = "topDecoder"
        self.gitUrl.value = "my.git.com"
        self.gitBranch.value = DEFAULT_GIT_BRANCH
        self.gitTag.value = DEFAULT_GIT_TAG
        self.gitBranch.value = DEFAULT_GIT_BRANCH
        self.author.value = "-"
        self.description.value = "-"
        self.version.value = "-"
        self.versionHistory.value = []
        self.date.value = "-"
        self.time.value = "-"
        self.decoderMqttURI.value = DEFAULT_DECODER_MQTT_URI
        self.decoderMqttPort.value = DEFAULT_DECODER_MQTT_PORT
        self.decoderMqttTopicPrefix.value = DEFAULT_DECODER_MQTT_TOPIC_PREFIX
        self.decoderMqttKeepalivePeriod.value = DEFAULT_DECODER_KEEPALIVE_PERIOD
        self.jmriRpcURI.value = DEFAULT_JMRI_RPC_URI
        self.jmriRpcPortBase.value = DEFAULT_JMRI_RPC_PORT_BASE
        self.JMRIRpcKeepAlivePeriod.value = DEFAULT_JMRI_RPC_KEEPALIVE_PERIOD
        self.ntpUri.value = DEFAULT_NTP_SERVER
        self.ntpPort.value = DEFAULT_NTP_PORT
        #SET SAME TIMEZONE AS SERVER
        self.tz.value = 0
        probeSocket = socket(AF_INET, SOCK_DGRAM)                                                       # Get local host IP address to use as default
        probeSocket.settimeout(0)
        try:
            probeSocket.connect(('10.0.0.0', 0))                                                       # doesn't even have to be reachable
            self.rsyslogUrl.value = probeSocket.getsockname()[0]
        except Exception:
            self.rsyslogUrl.value = DEFAULT_RSYSLOG_SERVER
        finally:
            probeSocket.close()
        self.rsyslogPort.value = DEFAULT_RSYSLOG_PORT
        self.rsyslogProtocol.value = DEFAULT_RSYSLOG_PROTOCOL
        self.logVerbosity.value = DEFAULT_LOG_VERBOSITY
        self.logFile.value = DEFAULT_LOG_FILE
        self.logRotateNoFiles.value = DEFAULT_LOG_ROTATION_NO_FILES
        self.logFileSize.value = DEFAULT_LOG_FILE_SIZE_KB
        self.snmpUri.value = DEFAULT_SNMP_SERVER
        self.snmpPort.value = DEFAULT_SNMP_PORT
        self.snmpProtocol.value = DEFAULT_SNMP_PROTOCOL
        self.decoderFailSafe.value = True
        self.trackFailSafe.value = DEFAULT_TRACK_FAILSAFE
        self.commitAll()
        self.MQTTalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "MQTT client disconnected")
        self.RPCalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "RPC client disconnected")
        self.LOGLEVELalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "High log-level enabled, this may lead to extensive system load and excessive performance degradation")
        self.subConnectionOpState = MQTT_DISCONNECTED_FAILURE
        self.subConnectionOpState = MQTT_DISCONNECTED_FAILURE | RPC_DISCONNECTED_FAILURE
        self.topItem = self.win.registerMoMObj(self, 0, "topDecoder", TOP_DECODER, displayIcon=SERVER_ICON)
        self.win.inactivateMoMObj(self.topItem)
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_DISCONNECTED[STATE])
        self.xmlConfig = None
        self.nestedUpdate = 0
        if self.demo:
            for i in range(6):
                self.addChild(DECODER, name="GJD-" + str(i), config=False, demo=True)
        else:
            self.rpcClient = jmriRpcClient()
            self.rpcClient.start(uri = self.jmriRpcURI.value, portBase=self.jmriRpcPortBase.value, errCb=self.__onRpcErr)
            self.mqttClient = mqtt(self.decoderMqttURI.value, port=self.decoderMqttPort.value, onConnectCb=self.__onMQTTConnect, onDisconnectCb=self.__onMQTTDisconnect, clientId=MQTT_CLIENT_ID)
            self.commitAll()

    def onXmlConfig(self, xmlConfig):
        self.xmlConfig = xmlConfig
        trace.notify(DEBUG_INFO, "Starting to configure top decoder class and all subordinate class objects")
        try:
            controllersXmlTree = ET.ElementTree(ET.fromstring(self.xmlConfig))
        except:
            trace.notify(DEBUG_PANIC, "Error parsing XML configuration\n" + str(traceback.print_exc()))
            return rc.PARSE_ERR
        if str(controllersXmlTree.getroot().tag) != "genJMRI":
            trace.notify(DEBUG_ERROR, "XML configuration missformated")
            return rc.PARSE_ERR
        for versionHistory in controllersXmlTree.getroot():
            if versionHistory.tag == "VersionHistory":
                for version in versionHistory:
                    if version.tag == "Version":
                        versionHistoryXmlItem = parse_xml(version,
                                               {"VersionName": MANSTR,
                                                "Author": MANSTR,
                                                "Date": MANSTR,
                                                "Time": MANSTR,
                                                "VersionDescription": MANSTR,
                                                "gitBranch": OPTSTR,
                                                "gitTag" : OPTSTR,
                                                "gitUrl" : OPTSTR
                                               }
                                        )
                        self.versionHistory.append({"VersionName":versionHistoryXmlItem.get("VersionName"),
                                                    "Author":versionHistoryXmlItem.get("Author"),
                                                    "Date":versionHistoryXmlItem.get("Date"),
                                                    "Time":versionHistoryXmlItem.get("Time"),
                                                    "VersionDescription":versionHistoryXmlItem.get("VersionDescription"),
                                                    "gitBranch":versionHistoryXmlItem.get("gitBranch"),
                                                    "gitTag":versionHistoryXmlItem.get("gitTag"), 
                                                    "gitUrl":versionHistoryXmlItem.get("gitUrl")})
                break
        try:
            topDecoderXmlConfig = parse_xml(controllersXmlTree.getroot(),
                                                   {"Author": OPTSTR,
                                                    "Description": OPTSTR,
                                                    "Version": OPTSTR,
                                                    "Date": OPTSTR,
                                                    "Time": OPTSTR,
                                                    "gitBranch": OPTSTR,
                                                    "gitTag": OPTSTR,
                                                    "gitUrl": OPTSTR,
                                                    "DecoderMqttURI": MANSTR,
                                                    "DecoderMqttPort": OPTSTR,
                                                    "JMRIRPCURI" : OPTSTR,
                                                    "JMRIRPCPortBase" : OPTINT,
                                                    "DecoderMqttTopicPrefix": OPTSTR,
                                                    "NTPServer": OPTSTR,
                                                    "NTPPort": OPTINT,
                                                    "TimeZoneGmtOffset": OPTINT,
                                                    "RSyslogServer": OPTSTR,
                                                    "RSyslogPort" : OPTINT,
                                                    "RSyslogProtocol" : OPTSTR,
                                                    "LogLevel": OPTSTR,
                                                    "RSyslogFile" : OPTSTR,
                                                    "RSyslogRotateNoFiles" : OPTINT,
                                                    "RSyslogFileSize" : OPTINT,
                                                    "SNMPServer": OPTSTR,
                                                    "SNMPPort": OPTINT,
                                                    "SNMPProtocol": OPTSTR,
                                                    "TracksFailSafe" : OPTSTR,
                                                    "DecodersFailSafe" : OPTSTR, 
                                                    "DecoderPingPeriod" : OPTFLOAT,
                                                    "JMRIRpcKeepAlivePeriod" : OPTFLOAT,
                                                    "AdminState":OPTSTR
                                                   }
                                            )
            if topDecoderXmlConfig.get("Author") != None: self.author.value = topDecoderXmlConfig.get("Author")
            if topDecoderXmlConfig.get("Description") != None : self.description.value = topDecoderXmlConfig.get("Description")
            if topDecoderXmlConfig.get("Version") != None: self.version.value = topDecoderXmlConfig.get("Version")
            if topDecoderXmlConfig.get("Date") != None: self.date.value = topDecoderXmlConfig.get("Date")
            if topDecoderXmlConfig.get("Time") != None: self.time.value = topDecoderXmlConfig.get("Time")
            if topDecoderXmlConfig.get("gitUrl") != None: self.gitUrl.value = topDecoderXmlConfig.get("gitUrl")
            if topDecoderXmlConfig.get("gitTag") != None: self.gitTag.value = topDecoderXmlConfig.get("gitTag")
            if topDecoderXmlConfig.get("gitBranch") != None: self.gitBranch.value = topDecoderXmlConfig.get("gitBranch")
            if topDecoderXmlConfig.get("DecoderMqttURI") != None: self.decoderMqttURI.value = topDecoderXmlConfig.get("DecoderMqttURI")
            if topDecoderXmlConfig.get("DecoderMqttPort") != None: self.decoderMqttPort.value = int(topDecoderXmlConfig.get("DecoderMqttPort"))
            if topDecoderXmlConfig.get("JMRIRPCURI") != None: self.jmriRpcURI.value = topDecoderXmlConfig.get("JMRIRPCURI")
            if topDecoderXmlConfig.get("JMRIRPCPortBase") != None: self.jmriRpcPortBase.value = int(topDecoderXmlConfig.get("JMRIRPCPortBase"))
            if topDecoderXmlConfig.get("DecoderMqttTopicPrefix") != None: self.decoderMqttTopicPrefix.value = topDecoderXmlConfig.get("DecoderMqttTopicPrefix")
            if topDecoderXmlConfig.get("NTPServer") != None: self.ntpUri.value = topDecoderXmlConfig.get("NTPServer")
            if topDecoderXmlConfig.get("NTPPort") != None: self.ntpPort.value = int(topDecoderXmlConfig.get("NTPPort"))
            if topDecoderXmlConfig.get("TimeZoneGmtOffset") != None: self.tz.value = int(topDecoderXmlConfig.get("TimeZoneGmtOffset"))
            if topDecoderXmlConfig.get("RSyslogServer") != None: self.rsyslogUrl.value = topDecoderXmlConfig.get("RSyslogServer")
            if topDecoderXmlConfig.get("RSyslogPort") != None: self.rsyslogPort.value = topDecoderXmlConfig.get("RSyslogPort")
            if topDecoderXmlConfig.get("RSyslogProtocol") != None: self.rsyslogProtocol.value = topDecoderXmlConfig.get("RSyslogProtocol")
            if topDecoderXmlConfig.get("LogLevel") != None:
                self.logVerbosity.value = topDecoderXmlConfig.get("LogLevel")
                if trace.getSeverityFromSeverityStr(self.logVerbosity.candidateValue) == None:
                    trace.notify(DEBUG_ERROR, "Specified debug-level is not valid, will use default debug-level")
            if topDecoderXmlConfig.get("RSyslogFile") != None: self.logFile.value = topDecoderXmlConfig.get("RSyslogFile")
            if topDecoderXmlConfig.get("RSyslogRotateNoFiles") != None: self.logRotateNoFiles.value = topDecoderXmlConfig.get("RSyslogRotateNoFiles")
            if topDecoderXmlConfig.get("RSyslogFileSize") != None: self.logFileSize.value = topDecoderXmlConfig.get("RSyslogFileSize")
            if topDecoderXmlConfig.get("SNMPServer") != None: self.snmpUri.value = topDecoderXmlConfig.get("SNMPServer")
            if topDecoderXmlConfig.get("SNMPPort") != None: self.snmpPort.value = int(topDecoderXmlConfig.get("SNMPPort"))
            if topDecoderXmlConfig.get("SNMPProtocol") != None: self.snmpProtocol.value = topDecoderXmlConfig.get("SNMPProtocol")

            if topDecoderXmlConfig.get("SNMPServer") != None: self.snmpUri.value = topDecoderXmlConfig.get("SNMPServer")
            if topDecoderXmlConfig.get("TracksFailSafe") != None: 
                if topDecoderXmlConfig.get("TracksFailSafe") == "Yes": 
                    self.trackFailSafe.value = True
                elif topDecoderXmlConfig.get("TracksFailSafe") == "No": 
                    self.trackFailSafe.value = False
                else:
                    notify(DEBUG_INFO, "\"TracksFailSafe\" not set to yes/no, setting it to No")
                    self.trackFailSafe.value = False
            else:
                trace.notify(DEBUG_INFO, "\"TracksFailSafe\" not set, setting it to no")
                self.trackFailSafe.value = False
            if topDecoderXmlConfig.get("DecodersFailSafe") != None: 
                if topDecoderXmlConfig.get("DecodersFailSafe") == "Yes": 
                    self.decoderFailSafe.value = True
                elif topDecoderXmlConfig.get("DecodersFailSafe") == "No":
                    self.decoderFailSafe.value = False
                else: 
                    trace.notify(DEBUG_INFO, "\"DecodersFailSafe\" not set to yes/no, setting it to no")
                    self.decoderFailSafe.value = False
            else:
                trace.notify(DEBUG_INFO, "\"DisableAllDecodersAtFault\" not set, setting it to no")
                self.decoderFailSafe.value = False
            if topDecoderXmlConfig.get("DecoderPingPeriod") != None: 
                self.decoderMqttKeepalivePeriod.value = float(topDecoderXmlConfig.get("DecoderPingPeriod"))
            else: trace.notify(DEBUG_INFO, "\"DecoderPingPeriod\" not set, using default " + str(DEFAULT_DECODER_KEEPALIVE_PERIOD))
            if topDecoderXmlConfig.get("JMRIRpcKeepAlivePeriod") != None: 
                self.JMRIRpcKeepAlivePeriod.value = float(topDecoderXmlConfig.get("JMRIRpcKeepAlivePeriod"))
            else: trace.notify(DEBUG_INFO, "\"JMRIRpcKeepAlivePeriod\" not set, using default " + str(DEFAULT_JMRI_RPC_KEEPALIVE_PERIOD))
            if topDecoderXmlConfig.get("AdminState") != None:
                self.setAdmState(topDecoderXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for topDecoder - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
                trace.notify(DEBUG_ERROR, "XML configuration missformated, topDecoder section is missing mandatory tags/values or values did not pass type/range check: " + str(traceback.print_exc()))
                return rc.PARSE_ERR
        res = self.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of topDecoder, return code: " + trace.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Configured")
        for child in controllersXmlTree.getroot():
            if str(child.tag) == "Decoder":
                res = self.addChild(DECODER, config=False, configXml=child, demo=False)
                if res != rc.OK:
                        trace.notify(DEBUG_ERROR, "Failed to add decoder to topDecoder - return code: " + rc.getErrStr(res))
                        return res
            for child in self.childs.value:
                child.decoderRestart()
        return rc.OK

    def generateDiscoveryResponse(self, xmlConfig):
        self.xmlConfig = xmlConfig
        trace.notify(DEBUG_INFO, "Generating discovery response")
        try:
            controllersXmlTree = ET.ElementTree(ET.fromstring(self.xmlConfig))
        except:
            trace.notify(DEBUG_PANIC, "Error parsing XML configuration\n" + str(traceback.print_exc()))
            return rc.PARSE_ERR
        if str(controllersXmlTree.getroot().tag) != "genJMRI":
            trace.notify(DEBUG_ERROR, "XML configuration missformated")
            return rc.PARSE_ERR
        discoveryConfig = ET.Element("DiscoveryResponse")
        for decoder in controllersXmlTree.getroot():
            if decoder.tag == "Decoder":
                try:
                    discoveryDecoder =  ET.SubElement(discoveryConfig, "Decoder")
                    decoderConfig = parse_xml(decoder, {"MAC":MANSTR,  "URI":MANSTR})
                    discoverymac = ET.SubElement(discoveryDecoder, "MAC")
                    discoverymac.text = decoderConfig.get("MAC")
                    discoveryuri = ET.SubElement(discoveryDecoder, "URI")
                    discoveryuri.text = decoderConfig.get("URI")
                except:
                    trace.notify(DEBUG_ERROR, "XML configuration missformated, decoder section(s) is missing mandatory tags/values: " + str(traceback.print_exc()))
                    return rc.PARSE_ERR
        self.discoveryConfigXML = ET.tostring(discoveryConfig, method="xml").decode()
        trace.notify(DEBUG_TERSE, "Discovery response xml configuration created: \n" +
                        minidom.parseString(self.discoveryConfigXML).toprettyxml())
        return rc.OK

    def updateReq(self):
        self.nestedUpdate += 1
        res = self.validate()
        self.nestedUpdate -= 1
        if self.nestedUpdate == 0:
            for child in self.childs.value:
                child.decoderRestart()
        return res

    def validate(self):
        trace.notify(DEBUG_TERSE, "topDecoder received configuration validate()")
        self.sysNames = []
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "topDecoder - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    trace.notify(DEBUG_INFO, "Configuration validation failed, Aborting configuration")
                    self.abort()
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "topDecoder - configuration has been changed - validating it")
            if self.decoderMqttURI.value != self.decoderMqttURI.candidateValue or\
               self.decoderMqttPort.value != self.decoderMqttPort.candidateValue or\
               self.decoderMqttTopicPrefix.value != self.decoderMqttTopicPrefix.candidateValue:
                self.mqttConfigChanged = True
            else:
                self.mqttConfigChanged = False
            if self.jmriRpcURI.value != self.jmriRpcURI.candidateValue or\
               self.jmriRpcPortBase.value != self.jmriRpcPortBase.candidateValue:
                self.rpcConfigChanged = True
            else:
                self.rpcConfigChanged = False
            res = self.__validateConfig()
            if res != rc.OK:
                trace.notify(DEBUG_INFO, "Configuration validation failed, Aborting configuration")
                self.abort()
                return res
        trace.notify(DEBUG_INFO, "Configuration validation successful, Committing configuration - commit(0)")
        res = self.commit0()
        if res != rc.OK:
            trace.notify(DEBUG_PANIC, "Configuration commit unsuccessful - return code: " + rc.getErrStr(res) + " we have a broken system!")
        self.generateDiscoveryResponse(self.getXmlConfigTree(text = True))
        return rc.OK

    def checkSysName(self, sysName):
        try:
            self.sysNames.index(sysName)
            return rc.ALREADY_EXISTS
        except:
            self.sysNames.append(sysName)
            return rc.OK

    def commit0(self):
        trace.notify(DEBUG_TERSE, "topDecoder received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "topDecoder - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Could not commit validated configuration")
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "topDecoder was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.topItem, self.nameKey.value)
        else:
            trace.notify(DEBUG_TERSE, "topDecoder was not reconfigured, skiping config commitment")
        return self.commit1()

    def commit1(self):
        trace.notify(DEBUG_TERSE, "topDecoder received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "topDecoder was reconfigured - applying the configuration")
                res = self.__setConfig()
            except Exception:
                traceback.print_exc() 
                trace.notify(DEBUG_ERROR, "topDecoder Could not set validated configuration")

            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "topDecoder Could not set validated configuration")
                return res
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in self.childs.value:
                res = child.commit1()
                if res != rc.OK:
                    return res
        return rc.OK

    def abort(self):
        trace.notify(DEBUG_TERSE, "topDecoder received configuration abort()")
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in self.childs.value:
                child.abort()
        self.abortAll()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True, onlyIncludeThisChild=False):
            trace.notify(DEBUG_TERSE, "Providing top decoder over arching .xml configuration")
            topXml = ET.Element("genJMRI")
            if not decoder:
                childXml = ET.SubElement(topXml, "Author")
                if self.author.value: childXml.text = str(self.author.value)
                childXml = ET.SubElement(topXml, "Description")
                if self.description.value: childXml.text = self.description.value
                childXml = ET.SubElement(topXml, "Version")
                if self.version.value: childXml.text = self.version.value
                childXml = ET.SubElement(topXml, "Date")
                if self.date.value: childXml.text = self.date.value
                childXml = ET.SubElement(topXml, "Time")
                if self.time.value: childXml.text = self.time.value
                childXml = ET.SubElement(topXml, "gitBranch")
                if self.gitBranch.value: childXml.text = self.gitBranch.value
                childXml = ET.SubElement(topXml, "gitTag")
                if self.gitTag.value: childXml.text = self.gitTag.value
                childXml = ET.SubElement(topXml, "gitUrl")
                if self.gitUrl.value: childXml.text = self.gitUrl.value
                versionHistoryXml = ET.SubElement(topXml, "VersionHistory")

                for version in self.versionHistory.value:
                    versionXml = ET.SubElement(versionHistoryXml, "Version")
                    versionNameXml = ET.SubElement(versionXml, "VersionName")
                    if version["VersionName"]: versionNameXml.text = version["VersionName"]
                    authorXml = ET.SubElement(versionXml, "Author")
                    if version["Author"]: authorXml.text = version["Author"]
                    dateXml = ET.SubElement(versionXml, "Date")
                    if version["Date"]: dateXml.text = version["Date"]
                    timeXml = ET.SubElement(versionXml, "Time")
                    if version["Time"]: timeXml.text = version["Time"]
                    descXml = ET.SubElement(versionXml, "VersionDescription")
                    if version["VersionDescription"]: descXml.text = version["VersionDescription"]
                    gitBranchXml = ET.SubElement(versionXml, "gitBranch")
                    if version["gitBranch"]: gitBranchXml.text = version["gitBranch"]
                    gitTagXml = ET.SubElement(versionXml, "gitTag")
                    if version["gitTag"]: gitTagXml.text = version["gitTag"]
                    gitUrlXml = ET.SubElement(versionXml, "gitUrl")
                    if version["gitUrl"]: gitUrlXml.text = version["gitUrl"]

            childXml = ET.SubElement(topXml, "DecoderMqttURI")
            childXml.text = self.decoderMqttURI.value
            childXml = ET.SubElement(topXml, "DecoderMqttPort")
            childXml.text = str(self.decoderMqttPort.value)
            childXml = ET.SubElement(topXml, "DecoderMqttTopicPrefix")
            childXml.text = self.decoderMqttTopicPrefix.value
            childXml = ET.SubElement(topXml, "DecoderPingPeriod")
            childXml.text = str(self.decoderMqttKeepalivePeriod.value)
            if not decoder:
                childXml = ET.SubElement(topXml, "JMRIRPCURI")
                childXml.text = self.jmriRpcURI.value
                childXml = ET.SubElement(topXml, "JMRIRPCPortBase")
                childXml.text = str(self.jmriRpcPortBase.value)
                childXml = ET.SubElement(topXml, "JMRIRpcKeepAlivePeriod")
                childXml.text = str(self.JMRIRpcKeepAlivePeriod.value)
            childXml = ET.SubElement(topXml, "NTPServer")
            if self.ntpUri.value: childXml.text = self.ntpUri.value
            childXml = ET.SubElement(topXml, "NTPPort")
            if self.ntpPort.value: childXml.text = str(self.ntpPort.value)
            childXml = ET.SubElement(topXml, "TimeZoneGmtOffset")
            if self.tz.value: childXml.text = "%+d" % (self.tz.value)
            childXml = ET.SubElement(topXml, "RSyslogServer")
            if self.rsyslogUrl.value: childXml.text = self.rsyslogUrl.value
            childXml = ET.SubElement(topXml, "RSyslogPort")
            if self.rsyslogPort.value: childXml.text = str(self.rsyslogPort.value)
            childXml = ET.SubElement(topXml, "RSyslogProtocol")
            if self.rsyslogProtocol.value: childXml.text = self.rsyslogProtocol.value
            if not decoder:
                childXml = ET.SubElement(topXml, "RSyslogFile")
                if self.logFile.value: childXml.text = self.logFile.value
                childXml = ET.SubElement(topXml, "RSyslogRotateNoFiles")
                if self.logRotateNoFiles.value: childXml.text = str(self.logRotateNoFiles.value)
                childXml = ET.SubElement(topXml, "RSyslogFileSize")
                if self.logFileSize.value: childXml.text = str(self.logFileSize.value)
            childXml = ET.SubElement(topXml, "LogLevel")
            if self.logVerbosity.value: childXml.text = self.logVerbosity.value
            if not decoder:
                childXml = ET.SubElement(topXml, "SNMPServer")
                if self.snmpUri.value: childXml.text = self.snmpUri.value
                childXml = ET.SubElement(topXml, "SNMPPort")
                if self.snmpPort.value: childXml.text = str(self.snmpPort.value)
                childXml = ET.SubElement(topXml, "SNMPProtocol")
                if self.snmpProtocol.value: childXml.text = self.snmpProtocol.value
            if not decoder:
                childXml = ET.SubElement(topXml, "TracksFailSafe")
                childXml.text = "Yes" if self.trackFailSafe.value else "No"
            childXml = ET.SubElement(topXml, "DecodersFailSafe")
            childXml.text = "Yes" if self.decoderFailSafe.value else "No"
            if not decoder:
                adminState = ET.SubElement(topXml, "AdminState")
                adminState.text = self.getAdmState()[STATE_STR]
            if includeChilds:
                if not onlyIncludeThisChild:
                    for decoderItter in self.decoders.value:
                        topXml.append(decoderItter.getXmlConfigTree(decoder=decoder, text=False))
                else:
                    topXml.append(onlyIncludeThisChild.getXmlConfigTree(decoder=decoder, text=False))
            return minidom.parseString(ET.tostring(topXml, 'unicode')).toprettyxml() if text else topXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name=None, config=True, configXml=None, demo=False):
        if resourceType == DECODER:
            self.decoders.append(decoder(self.win, self.topItem, self.rpcClient, self.mqttClient, name=name, demo=demo))
            self.childs.value = self.decoders.candidateValue
            trace.notify(DEBUG_INFO, "Decoder: " + self.decoders.candidateValue[-1].nameKey.candidateValue + " has been added to genJMRI server (top decoder) - awaiting configuration")
            if not config and configXml:
                nameKey = self.decoders.candidateValue[-1].nameKey.candidateValue
                res = self.decoders.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure decoder from xml: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Decoder: " + self.decoders.candidateValue[-1].nameKey.value + " successfully configured")
                return rc.OK
            if config:
                self.dialog = UI_decoderDialog(self.decoders.candidateValue[-1], edit=True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Top decoder could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo) + " \"replacement\": " + str(replacement))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Gen JMRI server (top decoder) only take DECODER as childs, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        self.decoders.value.remove(child)
        self.childs.value = self.decoders.candidateValue

    def view(self):
        self.dialog = UI_topDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_topDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, DECODER)
        self.dialog.show()

    def delete(self):
        pass

    def restart(self):
        print("Restarting genJMRI server...")

    def setLogVerbosity(self, logVerbosity):
        trace.setGlobalDebugLevel(trace.getSeverityFromSeverityStr(logVerbosity))
        self.rpcClient.setRpcServerDebugLevel(logVerbosity)
        if trace.getSeverityFromSeverityStr(logVerbosity) < DEBUG_INFO:
            self.LOGLEVELalarm.raiseAlarm("Log level set at " + logVerbosity)
        else:
            self.LOGLEVELalarm.ceaseAlarm("Log level set at " + logVerbosity)

    def startLog(self, logTargetHandle):
        self.logTargetHandle = logTargetHandle
        trace.regNotificationCb(self.guiLogProducer)

    def guiLogProducer(self, unifiedTime, callerFunFrame, severity, notification, defaultFormatNotification, termination):
        self.logTargetHandle(defaultFormatNotification)

    def stopLog(self):
        trace.unRegNotificationCb(self.guiLogProducer)

    def gitCi(self):
        print("Checking in tag: " + self.gitTag)

    def gitCo(self):
        print("Checking out tag: " + self.gitTag)

    def accepted(self):
        if self.version.value != self.version.candidateValue:
            self.__setVersion(self.version.value)
            self.schemaObjects["versionHistory"].commit()
        res = self.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not configure " + self.nameKey.candidateValue + ", return code: " + rc.getErrStr(res))
            return res
        trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured from GUI")
        return rc.OK

    def rejected(self):
        self.abort()
        return rc.OK

    def __validateConfig(self):
        return rc.OK # Place holder for object config validation

    def __setConfig(self):
        # Set RPC parameters
        if self.rpcConfigChanged:
            self.rpcClient.setRpcUri(self.jmriRpcURI.value)
            self.rpcClient.setRpcPortBase(self.jmriRpcPortBase.value)
        self.rpcClient.setKeepaliveInterval(self.JMRIRpcKeepAlivePeriod.value)
        self.rpcClient.setRpcServerDebugLevel(self.logVerbosity.value)
        # Set Syslog parameters
        trace.setGlobalDebugLevel(trace.getSeverityFromSeverityStr(self.logVerbosity.value))
        self.rpcClient.setRpcServerDebugLevel(self.logVerbosity.value)
        trace.notify(DEBUG_VERBOSE, "Starting/Restarting RSyslog server")
        rSyslog.start(self.rsyslogUrl.value, self.rsyslogPort.value, self.logFile.value, self.logRotateNoFiles.value, self.logFileSize.value * 1000)
        trace.notify(DEBUG_INFO, "Starting/re-starting local RSyslog producer")
        trace.startSyslog(self.rsyslogUrl.value, self.rsyslogPort.value)
        # Set MQTT parameters
        if self.mqttConfigChanged:
            trace.notify(DEBUG_INFO, "Connecting/re-connecting MQTT client genJMRIServer to MQTT brooker: " + self.decoderMqttURI.value + ":" + str(self.decoderMqttPort.value))
            self.mqttClient.restart(self.decoderMqttURI.value, port=self.decoderMqttPort.value, onConnectCb=self.__onMQTTConnect, onDisconnectCb=self.__onMQTTDisconnect, clientId = MQTT_CLIENT_ID)
        self.mqttClient.setTopicPrefix(self.decoderMqttTopicPrefix.value)
        self.setLogVerbosity(self.logVerbosity.value)
        # Set NTP system state topics
        self.topDecoderOpDownStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM)[:-1]
        self.topDecoderOpUpStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM)[:-1]
        self.topDecoderAdmDownStreamTopic = (MQTT_JMRI_PRE_TOPIC + MQTT_TOPDECODER_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM)[:-1]
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        #self.mqttClient.unSubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_DISCOVERY_REQUEST_TOPIC, self.__onDiscoveryReq)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_DISCOVERY_REQUEST_TOPIC, self.__onDiscoveryReq)
        # Update alarm source names
        self.MQTTalarm.updateAlarmSrc(self.nameKey.value)
        self.RPCalarm.updateAlarmSrc(self.nameKey.value)
        self.LOGLEVELalarm.updateAlarmSrc(self.nameKey.value)
        return rc.OK

    def __setVersion(self, version):
        self.__updateVerHist()
        self.time.value = datetime.now(tz=pytz.UTC).strftime("%H:%M:%S")
        self.schemaObjects["time"].commit()
        self.date.value = datetime.now(tz=pytz.UTC).strftime('%Y-%m-%d')
        self.schemaObjects["date"].commit()

    def __updateVerHist(self):
        self.versionHistory.append({"VersionName":self.version.value,
                            "Author":self.author.value,
                            "Date":self.date.value,
                            "Time":self.time.value,
                            "VersionDescription":self.description.value,
                            "gitBranch":self.gitBranch.value,
                            "gitTag":self.gitTag.value, 
                            "gitUrl":self.gitUrl.value})

    def __onMQTTConnect(self, mqttObj, clientId, rc):
        if rc != pahomqtt.MQTT_ERR_SUCCESS:
            trace.notify(DEBUG_ERROR, "MQTT client: " + clientId + " could not connect to brooker")
            self.subConnectionOpState = self.subConnectionOpState | MQTT_DISCONNECTED_FAILURE
            if self.getOpStateDetail() & OP_DISCONNECTED[STATE]:
                self.reSetOpStateDetail(OP_DISCONNECTED[STATE])
            else:
                self.setOpStateDetail(OP_DISCONNECTED[STATE])
        else:
            trace.notify(DEBUG_INFO, "MQTT client: " + clientId + " connected to brooker")
            self.subConnectionOpState = self.subConnectionOpState & ~MQTT_DISCONNECTED_FAILURE
            if not self.subConnectionOpState:
                self.unSetOpStateDetail(OP_DISCONNECTED[STATE])
            else:
                self.reSetOpStateDetail(OP_DISCONNECTED[STATE])

    def __onMQTTDisconnect(self, mqttObj, clientId, rc):
        self.subConnectionOpState = self.subConnectionOpState | MQTT_DISCONNECTED_FAILURE
        if self.getOpStateDetail() & OP_DISCONNECTED[STATE]:
            self.reSetOpStateDetail(OP_DISCONNECTED[STATE])
        else:
            self.setOpStateDetail(OP_DISCONNECTED[STATE])
        if rc != pahomqtt.MQTT_ERR_SUCCESS:
            trace.notify(DEBUG_ERROR, "MQTT client: " + clientId + " disconnected from brooker, return code: " + str(rc))
        else:
            trace.notify(DEBUG_INFO, "MQTT client: " + clientId + " disconnected from brooker")

    def __onRpcErr(self, conStatus):
        if conStatus != rc.OK:
            trace.notify(DEBUG_ERROR, "RPC client disconnected from RPC server")
            self.subConnectionOpState = self.subConnectionOpState | RPC_DISCONNECTED_FAILURE
            if self.getOpStateDetail() & OP_DISCONNECTED[STATE]:
                self.reSetOpStateDetail(OP_DISCONNECTED[STATE])
            else:
                self.setOpStateDetail(OP_DISCONNECTED[STATE])
        else:
            trace.notify(DEBUG_INFO, "RPC client connected to RPC server")
            self.subConnectionOpState = self.subConnectionOpState & ~RPC_DISCONNECTED_FAILURE
            if not self.subConnectionOpState:
                self.unSetOpStateDetail(OP_DISCONNECTED[STATE])
            else:
                self.reSetOpStateDetail(OP_DISCONNECTED[STATE])

    def __sysStateAllListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, self.nameKey.value + " got a new OP State - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        opStateDetail = self.getOpStateDetail()
        if opStateDetail & OP_DISABLED[STATE]:
            self.win.inactivateMoMObj(self.topItem)
        elif opStateDetail & OP_CBL[STATE]:
            self.win.controlBlockMarkMoMObj(self.topItem)
        elif opStateDetail:
            self.win.faultBlockMarkMoMObj(self.topItem, True)
        else:
            self.win.faultBlockMarkMoMObj(self.topItem, False)
        if (changedOpStateDetail & OP_DISABLED[STATE]) and (opStateDetail & OP_DISABLED[STATE]):
            self.MQTTalarm.admDisableAlarm()
            self.RPCalarm.admDisableAlarm()
            if not self.subConnectionOpState & MQTT_DISCONNECTED_FAILURE:
                self.mqttClient.publish(self.topDecoderAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD) #SHOLD THIS BE MOVED TO DECODER
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.MQTTalarm.admEnableAlarm()
            self.RPCalarm.admEnableAlarm()
            if not self.subConnectionOpState & MQTT_DISCONNECTED_FAILURE:
                self.mqttClient.publish(self.topDecoderAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD) #SHOLD THIS BE MOVED TO DECODER
        if (changedOpStateDetail & OP_DISCONNECTED[STATE]) and (opStateDetail & OP_DISCONNECTED[STATE]):
            if self.subConnectionOpState & MQTT_DISCONNECTED_FAILURE:
                self.MQTTalarm.raiseAlarm("MQTT client: " + MQTT_CLIENT_ID + " disconnected from brooker", p_sysStateTransactionId, True)
            else:
                self.MQTTalarm.ceaseAlarm("MQTT client: " + MQTT_CLIENT_ID + " connected to brooker")
            if self.subConnectionOpState & RPC_DISCONNECTED_FAILURE:
                self.RPCalarm.raiseAlarm("RPC client disconnected from remote RPC server", p_sysStateTransactionId, True)
            else:
                self.RPCalarm.ceaseAlarm("RPC client connected to remote RPC server")
        elif (changedOpStateDetail & OP_DISCONNECTED[STATE]) and not (opStateDetail & OP_DISCONNECTED[STATE]):
            self.MQTTalarm.ceaseAlarm("MQTT client: " + MQTT_CLIENT_ID + " connected to brooker")
            self.RPCalarm.ceaseAlarm("RPC client connected to remote RPC server")

    def __onDiscoveryReq(self, topic, payload):
        try:
            self.discoveryConfigXML
        except:
            trace.notify(DEBUG_INFO, "Top decoder received a discovery request, but it was not yet configured - cannot respond")
        else:
            trace.notify(DEBUG_INFO, "Top decoder received a discovery request - responding")
            self.mqttClient.publish(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_DISCOVERY_RESPONSE_TOPIC, self.discoveryConfigXML)
# End TopDecoder
#------------------------------------------------------------------------------------------------------------------------------------------------