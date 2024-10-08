#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A myRailIO decoder class providing the myRailIO decoder management-, supervision and configuration. myRailIO provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
#from ast import Try
import os
import sys
import time
import threading
import traceback
import weakref
import xml.etree.ElementTree as ET
import xml.dom.minidom
from momResources import *
from ui import *
from lgLinkLogic import *
from satLinkLogic import *
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('mqtt', '..\\mqtt\\mqtt.py')
from mqtt import mqtt, syncMqttRequest
imp.load_source('syslog', '..\\trace\\syslog.py')
from syslog import rSyslog
imp.load_source('alarmHandler', '..\\alarmHandler\\alarmHandler.py')
from alarmHandler import alarm
imp.load_source('mqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from mqttTopicsNPayloads import *
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\myRailIORc.py')
from rc import rc
imp.load_source('schema', '..\\schema\\schema.py')
from schema import *
imp.load_source('topologyMgr', '..\\topologyMgr\\topologyMgr.py')
from topologyMgr import topologyMgr
imp.load_source('parseXml', '..\\xml\\parseXml.py')
from parseXml import *
from config import *
# End Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Local internal constants
#################################################################################################################################################
# End Local internal constants
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: decoder
# Purpose: The decoder class provides decoder management-, supervision and configuration. myRailIO provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
# StdMethods:   The standard myRailIO Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), regSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################

class decoder(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name = None, mac = None, demo = False):
        self.win = win
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.provisioned = False
        self.sysNameReged = False
        self.schemaDirty = False
        childsSchemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.DECODER_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.MQTT_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.updating = False
        self.pendingBoot = False
        self.lgLinks.value = []
        self.satLinks.value = []
        self.lgLinkTopology = topologyMgr(self, DECODER_MAX_LG_LINKS)
        self.satLinkTopology = topologyMgr(self, DECODER_MAX_SAT_LINKS)
        self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
        if name:
            self.decoderSystemName.value = name
        else:
            self.decoderSystemName.value = "GJD-MyNewDecoderSysName"
        if mac:
            self.mac.value = mac
        else:
            self.mac.value = "00:00:00:00:00:00"
        self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
        self.userName.value = "MyNewDecoderUsrName"
        self.decoderMqttURI.value = "no.valid.uri"
        self.description.value = "MyNewdecoderDescription"
        self.commitAll()
        self.item = self.win.registerMoMObj(self, self.parentItem, self.nameKey.candidateValue, DECODER, displayIcon=DECODER_ICON)
        self.WIFI_Balarm = alarm(self, "WIFI STATUS", self.nameKey.value, ALARM_CRITICALITY_B, "WiFi Signal quality not optimal")
        self.WIFI_Aalarm = alarm(self, "WIFI STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "WiFi Signal quality not acceptable")
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder has not received a valid configuration")
        self.SERVER_UNAVAILalarm = alarm(self, "KEEP-ALIVE STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Server-side has reported missing keep-live messages from client")
        self.CLIENT_UNAVAILalarm = alarm(self, "KEEP-ALIVE STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Client-side has reported missing keep-live messages from server")
        self.CPU_LOAD_Calarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder has reached 75% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.CPU_LOAD_Balarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Decoder has reached 85% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.CPU_LOAD_Aalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder has reached 95% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.INT_MEM_Calarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder internal memory usage has reached 70%")
        self.INT_MEM_Balarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Decoder internal memory usage has reached 80%")
        self.INT_MEM_Aalarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder internal memory usage has reached 90%")
        self.LOG_OVERLOADalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder logging overloaded")
        self.CLI_ACCESSalarm = alarm(self, "AUDIT TRAIL", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder is being accessed by a CLI user")
        self.CLI_DEBUG_ACCESSalarm = alarm(self, "AUDIT TRAIL", self.nameKey.value, ALARM_CRITICALITY_B, "Decoder is being accessed by a CLI user which has set the debug-flag, risking configuration inconsistances") #NEEDS TO BE IMPLEMENTED
        self.NTP_SYNCHalarm = alarm(self, "TIME SYNCHRONIZATION", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder NTP Time synchronization failed") #NEEDS TO BE IMPLEMENTED
        self.INT_FAILalarm = alarm(self, "INTERNAL FAILURE", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder has experienced an internal error")
        self.CBLalarm = alarm(self, "CONTROL-BLOCK STATUS", self.nameKey.value, ALARM_CRITICALITY_C, "Parent object blocked resulting in a control-block of this object")
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.win.inactivateMoMObj(self.item)
        self.setOpStateDetail(OP_INIT[STATE] | OP_DISCONNECTED[STATE] | OP_NOIP[STATE] | OP_UNDISCOVERED[STATE] | OP_UNCONFIGURED[STATE])
        self.missedPingReq = 0
        self.supervisionActive = False
        self.restart = True
        self.syncMemStatRequest = None
        self.syncCpuStatRequest = None
        self.syncCoreDumpRequest = None
        self.syncSsidRequest = None
        self.syncSnrRequest = None
        self.syncIPAddrRequest = None
        self.syncBrokerUriRequest = None
        self.syncHwVerRequest = None
        self.syncSwVerRequest = None
        self.syncLogLvlRequest = None
        self.syncWwwUiRequest = None
        self.syncOpStateRequest = None
        if self.demo:
            for i in range(DECODER_MAX_LG_LINKS):
                self.addChild(LIGHT_GROUP_LINK, name="GJLL-" + str(i), config=False, demo=True)
            for i in range(DECODER_MAX_SAT_LINKS):
                self.addChild(SATELLITE_LINK, name=i, config=False, demo=True)
        trace.notify(DEBUG_INFO,"New decoder: " + self.nameKey.candidateValue + " created - awaiting configuration")
        
    @staticmethod
    def aboutToDelete(ref):
        print(">>>>>>>>>>>>>>>>>>>> aboutToDelete")
        ref.parent.decoderMacTopology.removeTopologyMember(ref.decoderSystemName.value)
        
    def aboutToDeleteWorkAround(self):                      #WORKAROUND CODE FOR ISSUE #123
        print(">>>>>>>>>>>>>>>>>>>> aboutToDeleteWorkAround")
        self.parent.decoderMacTopology.removeTopologyMember(self.decoderSystemName.value)

    def onXmlConfig(self, xmlConfig):
        try:
            decoderXmlConfig = parse_xml(xmlConfig,
                                            {"SystemName": MANSTR,
                                             "UserName": OPTSTR,
                                             "MAC": MANSTR,
                                             "URI": MANSTR,
                                             "Description": OPTSTR,
                                             "AdminState":OPTSTR
                                             }
                                        )
            self.decoderSystemName.value = decoderXmlConfig.get("SystemName")
            if decoderXmlConfig.get("UserName") != None:
                self.userName.value = decoderXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
            self.mac.value = decoderXmlConfig.get("MAC")
            self.decoderMqttURI.value = decoderXmlConfig.get("URI")
            if decoderXmlConfig.get("Description") != None:
                self.description.value = decoderXmlConfig.get("Description")
            else:
                self.description.value = ""
            if decoderXmlConfig.get("AdminState") != None:
                self.setAdmState(decoderXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Decoder, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + decoderXmlConfig.get("SystemName") + ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        for lightGroupsLinkXml in xmlConfig:
            if lightGroupsLinkXml.tag == "LightgroupsLink":
                res = self.addChild(LIGHT_GROUP_LINK, config=False, configXml=lightGroupsLinkXml,demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Light group link to " + decoderXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        for satelliteLinkXml in xmlConfig:
            if satelliteLinkXml.tag == "SatelliteLink":
                res = self.addChild(SATELLITE_LINK, config=False, configXml=satelliteLinkXml, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add satellite link to " + decoderXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        return rc.OK

    def updateReq(self, child, source, uploadNReboot = True):
        if source == self:
            if self.updating:
                return rc.ALREADY_EXISTS
            if uploadNReboot:
                self.updating = True
            else:
                self.updating = False
        res = self.parent.updateReq(self, source, uploadNReboot)
        self.updating = False
        return res

    def validate(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def regSysName(self, sysName):
        return self.parent.regSysName(sysName)
    
    def unRegSysName(self, sysName):
        return self.parent.unRegSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for decoder " + self.decoderSystemName.value)
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for decoder " + self.decoderSystemName.value)
                return res
        else:
            trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.value + " was not reconfigured, skiping re-configuration")
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
        self.provisioned = True
        return rc.OK
    
    def abort(self):
        trace.notify(DEBUG_TERSE, "Decoder " + self.decoderSystemName.candidateValue + " received configuration abort()")
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in self.childs.value:
                child.abort()
        self.abortAll()
        if not self.provisioned:
            self.delete(top = True)
        # WEE NEED TO CHECK IF THE ABORT WAS DUE TO THE CREATION OF THIS OBJECT AND IF SO DELETE OUR SELVES (self.delete)
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing decoder .xml configuration")
        decoderXml = ET.Element("Decoder")
        sysName = ET.SubElement(decoderXml, "SystemName")
        sysName.text = self.decoderSystemName.value
        usrName = ET.SubElement(decoderXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(decoderXml, "Description")
        descName.text = self.description.value
        mac = ET.SubElement(decoderXml, "MAC")
        mac.text = self.mac.value
        uri = ET.SubElement(decoderXml, "URI")
        uri.text = self.decoderMqttURI.value
        adminState = ET.SubElement(decoderXml, "AdminState")
        adminState.text = self.getAdmState()[STATE_STR]
        if includeChilds:
            childs = True
            try:
                self.childs.value
            except:
                childs = False
            if childs:
                for child in self.childs.value:
                    decoderXml.append(child.getXmlConfigTree(decoder=decoder))
        return minidom.parseString(ET.tostring(decoderXml)).toprettyxml(indent="   ") if text else decoderXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE & ~METHOD_EDIT & ~METHOD_DELETE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name = None, config = True, configXml = None, demo = False):
        if resourceType == LIGHT_GROUP_LINK:
            self.lgLinks.append(lgLink(self.win, self.item, self.rpcClient, self.mqttClient, name = name, demo = demo))
            self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
            trace.notify(DEBUG_INFO, "Light group link: " + self.lgLinks.candidateValue[-1].nameKey.candidateValue + "is being added to decoder " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.lgLinks.candidateValue[-1].nameKey.candidateValue
                res = self.lgLinks.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Light group link: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Light group link: " + self.lgLinks.value[-1].nameKey.value + " successfully added to decoder " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_lightgroupsLinkDialog(self.lgLinks.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
        elif resourceType == SATELLITE_LINK:
            self.satLinks.append(satLink(self.win, self.item, self.rpcClient, self.mqttClient, name = name, demo = demo))
            self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
            trace.notify(DEBUG_INFO, "Satellite link: " + self.satLinks.candidateValue[-1].nameKey.candidateValue + "is being added to decoder " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.satLinks.candidateValue[-1].nameKey.candidateValue
                res = self.satLinks.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Satellite link: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Satellite link: " + self.satLinks.value[-1].nameKey.value + " successfully added to decoder " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_satLinkDialog(self.satLinks.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Decoder could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo) + " \"replacement\": " + str(replacement))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Gen JMRI server (top decoder) only takes SATELLITE_LINK and LIGHT_GROPU_LINK as childs, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return child.canDelete()
        child.aboutToDeleteWorkAround()                      #WORKAROUND CODE FOR ISSUE #123
        try:
            self.lgLinks.remove(child)
        except:
            pass
        try:
            self.satLinks.remove(child)
        except:
            pass
        self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
        return rc.OK

    def view(self):
        self.dialog = UI_decoderDialog(self, self.rpcClient, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_decoderDialog(self, self.rpcClient, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, LIGHT_GROUP_LINK | SATELLITE_LINK)
        self.dialog.show()

    def delete(self, top=False):
        if self.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + self.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return self.canDelete()
        try:
            for child in self.child.value:
                child.delete()
        except:
            pass
        self.WIFI_Balarm.ceaseAlarm("Source object deleted")
        self.WIFI_Aalarm.ceaseAlarm("Source object deleted")
        self.NOT_CONNECTEDalarm.ceaseAlarm("Source object deleted")
        self.NOT_CONFIGUREDalarm.ceaseAlarm("Source object deleted")
        self.SERVER_UNAVAILalarm.ceaseAlarm("Source object deleted")
        self.CLIENT_UNAVAILalarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Calarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Balarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Aalarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Calarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Balarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Aalarm.ceaseAlarm("Source object deleted")
        self.LOG_OVERLOADalarm.ceaseAlarm("Source object deleted")
        self.CLI_ACCESSalarm.ceaseAlarm("Source object deleted")
        self.CLI_DEBUG_ACCESSalarm.ceaseAlarm("Source object deleted")
        self.NTP_SYNCHalarm.ceaseAlarm("Source object deleted")
        self.INT_FAILalarm.ceaseAlarm("Source object deleted")
        self.CBLalarm.ceaseAlarm("Source object deleted")
        self.parent.unRegSysName(self.decoderSystemName.value)
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq(self, self, uploadNReboot = True)
        return rc.OK

    def accepted(self):
        self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
            self.pendingBoot = True
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not configure " + nameKey + ", return code: " + rc.getErrStr(res))
            return res
        trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured from GUI")
        return rc.OK

    def rejected(self):
        self.abort()
        return rc.OK

    def getDecoderUri(self):
        return self.decoderMqttURI.value

    def decoderRestart(self):
        self.__decoderRestart()
        
    def getTopology(self):
        return "/" + self.decoderSystemName.value

    # Requests to decoders
    def getOpStateFromClient(self):
        opStateXmlStr = self.syncOpStateRequest.sendRequest()
        if not opStateXmlStr:
            trace.notify(DEBUG_ERROR, "OP State could not be fetched")
            return "-"
        opStateXmlTree = ET.ElementTree(ET.fromstring(opStateXmlStr))
        if str(opStateXmlTree.getroot().tag) != MQTT_DECODER_OPSTATERESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "OP State response missformatted: " + ipAddrXmlTree)
            return "-"
        return opStateXmlTree.getroot().text

    def getFirmwareVersionFromClient(self):
        swVerXmlStr = self.syncSwverRequest.sendRequest()
        if not swVerXmlStr:
            trace.notify(DEBUG_ERROR, "SW Version could not be fetched")
            return "-"
        swVerXmlTree = ET.ElementTree(ET.fromstring(swVerXmlStr))
        if str(swVerXmlTree.getroot().tag) != MQTT_DECODER_SWVERRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "SW Version response missformatted: " + ipAddrXmlTree)
            return "-"
        return swVerXmlTree.getroot().text
    
    def getHardwareVersionFromClient(self):
        hwVerXmlStr = self.syncHwverRequest.sendRequest()
        if not hwVerXmlStr:
            trace.notify(DEBUG_ERROR, "HW Version could not be fetched")
            return "-"
        hwVerXmlTree = ET.ElementTree(ET.fromstring(hwVerXmlStr))
        if str(hwVerXmlTree.getroot().tag) != MQTT_DECODER_HWVERRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "HW Version response missformatted: " + ipAddrXmlTree)
            return "-"
        return hwVerXmlTree.getroot().text
    
    def getIpAddressFromClient(self):
        ipAddrXmlStr = self.syncIPAddrRequest.sendRequest()
        if not ipAddrXmlStr:
            trace.notify(DEBUG_ERROR, "IP Address could not be fetched")
            return "-"
        ipAddrXmlTree = ET.ElementTree(ET.fromstring(ipAddrXmlStr))
        if str(ipAddrXmlTree.getroot().tag) != MQTT_DECODER_IPADDRRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "IP Address response missformatted: " + ipAddrXmlTree)
            return "-"
        return ipAddrXmlTree.getroot().text

    def getBrokerUriFromClient(self):
        brokerUriXmlStr = self.syncBrokerUriRequest.sendRequest()
        if not brokerUriXmlStr:
            trace.notify(DEBUG_ERROR, "Broker URI could not be fetched")
            return "-"
        brokerUriXmlTree = ET.ElementTree(ET.fromstring(brokerUriXmlStr))
        if str(brokerUriXmlTree.getroot().tag) != MQTT_DECODER_BROKERURIRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Broker URI response missformatted: " + ipAddrXmlTree)
            return "-"
        return brokerUriXmlTree.getroot().text

    def getWifiSsidFromClient(self):
        ssidXmlStr = self.syncSsidRequest.sendRequest()
        if not ssidXmlStr:
            trace.notify(DEBUG_ERROR, "SSID could not be fetched")
            return "-"
        ssidXmlTree = ET.ElementTree(ET.fromstring(ssidXmlStr))
        if str(ssidXmlTree.getroot().tag) != MQTT_DECODER_SSIDRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "SSID response missformatted: " + ssidXmlStr)
            return "-"
        return ssidXmlTree.getroot().text

    def getWifiSsidSnrFromClient(self):
        snrXmlStr = self.syncSnrRequest.sendRequest()
        if not snrXmlStr:
            trace.notify(DEBUG_ERROR, "WIFI SNR could not be fetched")
            return "-"
        snrXmlTree = ET.ElementTree(ET.fromstring(snrXmlStr))
        if str(snrXmlTree.getroot().tag) != MQTT_DECODER_SNRRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "WIFI SNR response missformatted: " + snrXmlStr)
            return "-"
        return snrXmlTree.getroot().text
    
    def getLoglevelFromClient(self):
        logLvlXmlStr = self.syncLogLvlRequest.sendRequest()
        if not logLvlXmlStr:
            trace.notify(DEBUG_ERROR, "Log level could not be fetched")
            return "-"
        logLvlTree = ET.ElementTree(ET.fromstring(logLvlXmlStr))
        if str(logLvlTree.getroot().tag) != MQTT_DECODER_LOGLVLRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Log level response missformatted: " + logLvlXmlStr)
            return "-"
        return logLvlTree.getroot().text
    
    def getMemUsageFromClient(self):
        memUsageXmlStr = self.syncMemStatRequest.sendRequest()
        if not memUsageXmlStr:
            trace.notify(DEBUG_ERROR, "Memory stats could not be fetched")
            return "-"
        memUsageXmlTree = ET.ElementTree(ET.fromstring(memUsageXmlStr))
        if str(memUsageXmlTree.getroot().tag) != MQTT_DECODER_MEMSTATRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Memory stats response missformatted: " + memUsageXmlStr)
            return "-"
        return memUsageXmlTree.getroot().text
    
    def getCpuUsageFromClient(self):
        cpuUsageXmlStr = self.syncCpuStatRequest.sendRequest()
        if not cpuUsageXmlStr:
            trace.notify(DEBUG_ERROR, "CPU stats could not be fetched")
            return "-"
        cpuUsageXmlTree = ET.ElementTree(ET.fromstring(cpuUsageXmlStr))
        if str(cpuUsageXmlTree.getroot().tag) != MQTT_DECODER_CPUSTATRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "CPU stats response missformatted: " + cpuUsageXmlStr)
            return "-"
        return cpuUsageXmlTree.getroot().text
    
    def getCoreDumpIdFromClient(self):
        coreDumpXmlStr = self.syncCoreDumpRequest.sendRequest()
        if not coreDumpXmlStr:
            trace.notify(DEBUG_ERROR, "Coredump could not be fetched")
            return "-"
        coreDumpXmlTree = ET.ElementTree(ET.fromstring(coreDumpXmlStr))
        if str(coreDumpXmlTree.getroot().tag) != MQTT_DECODER_COREDUMPRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Core-dump response missformatted: " + coreDumpXmlStr)
            return "-"
        return coreDumpXmlTree.getroot().text.partition('Backtrace')[0].strip()

    def getCoreDumpFromClient(self):
        coreDumpXmlStr = self.syncCoreDumpRequest.sendRequest()
        if not coreDumpXmlStr:
            trace.notify(DEBUG_ERROR, "Coredump could not be fetched")
            return "Coredump could not be fetched"
        coreDumpXmlTree = ET.ElementTree(ET.fromstring(coreDumpXmlStr))
        if str(coreDumpXmlTree.getroot().tag) != MQTT_DECODER_COREDUMPRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Core-dump response missformatted: " + coreDumpXmlStr)
            return "Coredump could not be fetched"
        return coreDumpXmlTree.getroot().text

    def getDecoderWwwUiFromClient(self):
        wwwUiXmlStr = self.syncWwwUiRequest.sendRequest()
        if not wwwUiXmlStr:
            trace.notify(DEBUG_ERROR, "WWW UI URI could not be fetched")
            return "-"
        wwwUiXmlTree = ET.ElementTree(ET.fromstring(wwwUiXmlStr))
        if str(wwwUiXmlTree.getroot().tag) != MQTT_DECODER_WWWUIRESP_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "WWW UI URI response missformatted: " + wwwUiXmlStr)
            return "-"
        return wwwUiXmlTree.getroot().text

# Decoder notifications
    def onWifiStatus(self, topic, value):
        wifiStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(wifiStatusXmlTree.getroot().tag) != MQTT_DECODER_WIFISTATUS_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Wifi status message missformatted: " + value)
            return
        wifiStatus = wifiStatusXmlTree.getroot().text
        if wifiStatus == "GOOD":
            self.WIFI_Balarm.ceaseAlarm("WiFi Signal quality not optimal alarm ceased")
            self.WIFI_Aalarm.ceaseAlarm("WiFi Signal quality not acceptable alarm ceased")
        elif wifiStatus == "FAIR":
            self.WIFI_Balarm.raiseAlarm("WiFi Signal quality not optimal")
            self.WIFI_Aalarm.ceaseAlarm("WiFi Signal quality not acceptable alarm ceased")
        elif wifiStatus == "POOR":
            self.WIFI_Balarm.ceaseAlarm("WiFi Signal quality not optimal alarm ceased")
            self.WIFI_Aalarm.raiseAlarm("WiFi Signal quality not acceptable")
        else:
            trace.notify(DEBUG_ERROR, "Unknown wifi status: " + wifiStatus)


    def onMemStatus(self, topic, value):
        memStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(memStatusXmlTree.getroot().tag) != MQTT_DECODER_MEMSTATUS_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Memory status message missformatted: " + value)
            return
        memStatus = memStatusXmlTree.getroot().text
        if memStatus == "NORMAL":
            self.INT_MEM_Calarm.ceaseAlarm("Decoder internal memory usage is under 70%")
            self.INT_MEM_Balarm.ceaseAlarm("Decoder internal memory usage is under 80%")
            self.INT_MEM_Aalarm.ceaseAlarm("Decoder internal memory usage is under 90%")
        elif memStatus == "WARN":
            self.INT_MEM_Calarm.raiseAlarm("Decoder internal memory usage is over 70%")
            self.INT_MEM_Balarm.ceaseAlarm("Decoder internal memory usage is under 80%")
            self.INT_MEM_Aalarm.ceaseAlarm("Decoder internal memory usage is under 90%")
        elif memStatus == "HIGH":
            self.INT_MEM_Calarm.ceaseAlarm("Decoder internal memory usage is over 80%, raising alarm severity")
            self.INT_MEM_Balarm.raiseAlarm("Decoder internal memory usage is over 80%")
            self.INT_MEM_Aalarm.ceaseAlarm("Decoder internal memory usage is under 90%")
        elif memStatus == "CRITICAL":
            self.INT_MEM_Calarm.ceaseAlarm("Decoder internal memory usage is over 90%, raising alarm severity")
            self.INT_MEM_Balarm.ceaseAlarm("Decoder internal memory usage is over 90%, raising alarm severity")
            self.INT_MEM_Aalarm.raiseAlarm("Decoder internal memory usage is over 90%")
        else:
            trace.notify(DEBUG_ERROR, "Unknown memory status: " + memStatus)

    def onLogOverload(self, topic, value):
        logOverloadStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(logOverloadStatusXmlTree.getroot().tag) != MQTT_DECODER_LOGOVERLOAD_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Log overload status message missformatted: " + value)
            return
        logOverloadStatus = logOverloadStatusXmlTree.getroot().text
        if logOverloadStatus == "TRUE":
            self.LOG_OVERLOADalarm.raiseAlarm("Decoder logging overloaded")
        elif logOverloadStatus == "FALSE":
            self.LOG_OVERLOADalarm.ceaseAlarm("Decoder logging is no longer overloaded")
        else:
            trace.notify(DEBUG_ERROR, "Unknown log overload status: " + logOverloadStatus)
            
    def onCliAccess(self, topic, value):
        cliAccessStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(cliAccessStatusXmlTree.getroot().tag) != MQTT_DECODER_CLIACCESS_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "CLI access status message missformatted: " + value)
            return
        cliAccessStatus = cliAccessStatusXmlTree.getroot().text
        if cliAccessStatus != "NONE":
            self.CLI_ACCESSalarm.raiseAlarm("Decoder is being accessed by a CLI user: " + cliAccessStatus)
        elif cliAccessStatus == "NONE":
            self.CLI_ACCESSalarm.ceaseAlarm("Decoder is no longer being accessed by a CLI user")
        else:
            trace.notify(DEBUG_ERROR, "Unknown CLI access status: " + cliAccessStatus)
            
    def onNtpSynch(self, topic, value):
        ntpSynchStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(ntpSynchStatusXmlTree.getroot().tag) != MQTT_DECODER_NTPSTATUS_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "NTP synchronization status message missformatted: " + value)
            return
        ntpSynchStatus = ntpSynchStatusXmlTree.getroot().text
        if ntpSynchStatus == "SYNCED" or ntpSynchStatus == "SYNCING":
            self.NTP_SYNCHalarm.ceaseAlarm("NTP is synchronized")
            trace.notify(DEBUG_INFO, "NTP is synchronized")
        else:
            self.NTP_SYNCHalarm.raiseAlarm("NTP is not synchronized, status: " + ntpSynchStatus)
            trace.notify(DEBUG_ERROR, "NTP is not synchronized, status: " + ntpSynchStatus)

    def onDebugFlag(self, topic, value):
        debugFlagStatusXmlTree = ET.ElementTree(ET.fromstring(value))
        if str(debugFlagStatusXmlTree.getroot().tag) != MQTT_DECODER_NTPSTATUS_PAYLOAD_TAG:
            trace.notify(DEBUG_ERROR, "Debug flag status message missformatted: " + value)
            return
        cliDebugAccessStatus = debugFlagStatusXmlTree.getroot().text
        if cliDebugAccessStatus == "ACTIVE":
            self.CLI_DEBUG_ACCESSalarm.raiseAlarm("One of the debug flags have been set")
        elif cliDebugAccessStatus == "INACTIVE":
            self.CLI_DEBUG_ACCESSalarm.ceaseAlarm("Debug flags are no longer set")
        else:
            trace.notify(DEBUG_ERROR, "Unknown debug flag status: " + cliDebugAccessStatus)

    def __validateConfig(self):
        if not self.sysNameReged:
            res = self.parent.regSysName(self.decoderSystemName.candidateValue)
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "System name " + self.decoderSystemName.candidateValue + " already in use")
                return res
        self.sysNameReged = True
        weakSelf = weakref.ref(self, decoder.aboutToDelete)
        res = self.parent.decoderMacTopology.addTopologyMember(self.decoderSystemName.candidateValue, self.mac.candidateValue, weakSelf)
        if res:
            trace.notify(DEBUG_ERROR, "Decoder failed Mac-address topology validation for MAC: " + str(self.mac.candidateValue) + "return code: " + rc.getErrStr(res))
            return res
        res = self.parent.decoderUriTopology.addTopologyMember(self.decoderSystemName.candidateValue, self.decoderMqttURI.candidateValue, weakSelf)
        if res:
            trace.notify(DEBUG_ERROR, "Decoder failed URI topology validation for URI: " + str(self.decoderMqttURI.candidateValue) + "return code: " + rc.getErrStr(res))
            return res
        if len(self.lgLinks.candidateValue) > DECODER_MAX_LG_LINKS:
            trace.notify(DEBUG_ERROR, "Too many Lg links defined for decoder " + str(self.decoderSystemName.candidateValue) + ", " + str(len(self.lgLinks.candidateValue)) + "  given, " + str(DECODER_MAX_LG_LINKS) + " is maximum")
            return rc.RANGE_ERR
        linkNos = []
        for lgLink in self.lgLinks.candidateValue:
            try:
                linkNos.index(lgLink.lgLinkNo.candidateValue)
                trace.notify(DEBUG_ERROR, "LG Link No " + str(lgLink.lgLinkNo.candidateValue) + " already in use for decoder " + self.decoderSystemName.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        if len(self.satLinks.candidateValue) > DECODER_MAX_SAT_LINKS:
            trace.notify(DEBUG_ERROR, "Too many Sat links defined for decoder " + str(self.decoderSystemName.candidateValue) + ", " + str(len(self.satLinks.candidateValue)) + " given, " + str(DECODER_MAX_SAT_LINKS) + " is maximum")
            return rc.RANGE_ERR
        linkNos = []
        for satLink in self.satLinks.candidateValue:
            try:
                linkNos.index(satLink.satLinkNo.candidateValue)
                trace.notify(DEBUG_ERROR, "SAT Link No " + str(satLink.satLinkNo.candidateValue) + " already in use for decoder " + self.decoderSystemName.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        return rc.OK

    def __setConfig(self): 
        self.decoderOpDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.decoderOpUpStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.decoderRebootTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_REBOOT_TOPIC + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.decoderAdmDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM + self.getDecoderUri() + "/" + self.decoderSystemName.value
        self.unRegOpStateCb(self.__sysStateRespondListener)
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateRespondListener, OP_DISABLED[STATE] | OP_SERVUNAVAILABLE[STATE] | OP_CBL[STATE])
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CONFIGREQ_TOPIC + self.decoderMqttURI.value, self.__onDecoderConfigReq)
        self.mqttClient.subscribeTopic(self.decoderOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_WIFISTATUS_TOPIC + self.decoderMqttURI.value, self.onWifiStatus)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_MEMSTATUS_TOPIC + self.decoderMqttURI.value, self.onMemStatus)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_LOGOVERLOAD_TOPIC + self.decoderMqttURI.value, self.onLogOverload)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CLIACCESS_TOPIC + self.decoderMqttURI.value, self.onCliAccess)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_NTPSTATUS_TOPIC + self.decoderMqttURI.value, self.onNtpSynch)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_DEBUG_TOPIC + self.decoderMqttURI.value, self.onDebugFlag)
        self.syncMemStatRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_MEMSTATREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_MEMSTATREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_MEMSTATRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncCpuStatRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CPUSTATREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_CPUSTATREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CPUSTATRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncCoreDumpRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_COREDUMPREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_COREDUMPREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_COREDUMPRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncSsidRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SSIDREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_SSIDREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SSIDRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncSnrRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SNRREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_SNRREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SNRRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncIPAddrRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_IPADDRREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_IPADDRREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_IPADDRRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncBrokerUriRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_BROKERURIREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_BROKERURIREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_BROKERURIRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncHwverRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_HWVERREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_HWVERREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_HWVERRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncSwverRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SWVERREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_SWVERREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_SWVERRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncLogLvlRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_LOGLVLREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_LOGLVLREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_LOGLVLRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncWwwUiRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_WWWUIREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_WWWUIREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_WWWUIRESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.syncOpStateRequest = syncMqttRequest(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_OPSTATEREQ_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, MQTT_DECODER_OPSTATEREQ_PAYLOAD, MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_OPSTATERESP_TOPIC + self.getDecoderUri()  + "/" + self.decoderSystemName.value, self.mqttClient, 500)
        self.WIFI_Balarm.updateAlarmSrc(self.nameKey.value)
        self.WIFI_Aalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.SERVER_UNAVAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CLIENT_UNAVAILalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_MEM_Calarm.updateAlarmSrc(self.nameKey.value)
        self.INT_MEM_Balarm.updateAlarmSrc(self.nameKey.value)
        self.INT_MEM_Aalarm.updateAlarmSrc(self.nameKey.value)
        self.NTP_SYNCHalarm.updateAlarmSrc(self.nameKey.value)
        self.CLI_ACCESSalarm.updateAlarmSrc(self.nameKey.value)
        self.CLI_DEBUG_ACCESSalarm.updateAlarmSrc(self.nameKey.value)
        if self.getAdmState() == ADM_ENABLE:
            self.__startSupervision()
        return rc.OK

    def __sysStateRespondListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & (OP_DISABLED[STATE] | OP_SERVUNAVAILABLE[STATE] | OP_CBL[STATE]):
            trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " Informing client decoder about the new enable/disable state ")
            self.mqttClient.publish(self.decoderOpDownStreamTopic, self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & (OP_SERVUNAVAILABLE[STATE] | OP_CBL[STATE])))
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.decoderAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
                self.__startSupervision()
            else:
                self.mqttClient.publish(self.decoderAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)
                self.__stopSupervision()

    def __sysStateAllListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        #trace.notify(DEBUG_INFO, self.nameKey.value + " got a new OP Statr - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        opStateDetail = self.getOpStateDetail()
        print("__sysStateAllListener " + str(opStateDetail))
        if opStateDetail & OP_DISABLED[STATE]:
            self.win.inactivateMoMObj(self.item)
        elif opStateDetail & OP_CBL[STATE]:
            self.win.controlBlockMarkMoMObj(self.item)
        elif opStateDetail:
            self.win.faultBlockMarkMoMObj(self.item, True)
        else:
            self.win.faultBlockMarkMoMObj(self.item, False)
        if (changedOpStateDetail & OP_DISABLED[STATE]) and (opStateDetail & OP_DISABLED[STATE]):
            self.WIFI_Balarm.admDisableAlarm()
            self.WIFI_Aalarm.admDisableAlarm()
            self.NOT_CONNECTEDalarm.admDisableAlarm()
            self.NOT_CONFIGUREDalarm.admDisableAlarm()
            self.SERVER_UNAVAILalarm.admDisableAlarm()
            self.CLIENT_UNAVAILalarm.admDisableAlarm()
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.admDisableAlarm()
            self.INT_MEM_Calarm.admDisableAlarm()
            self.INT_MEM_Balarm.admDisableAlarm()
            self.INT_MEM_Aalarm.admDisableAlarm()
            self.LOG_OVERLOADalarm.admDisableAlarm()
            self.CLI_ACCESSalarm.admDisableAlarm()
            self.CLI_DEBUG_ACCESSalarm.admDisableAlarm()
            self.NTP_SYNCHalarm.admDisableAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.WIFI_Balarm.admEnableAlarm()
            self.WIFI_Aalarm.admEnableAlarm()
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.SERVER_UNAVAILalarm.admEnableAlarm()
            self.CLIENT_UNAVAILalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
            self.INT_MEM_Calarm.admEnableAlarm()
            self.INT_MEM_Balarm.admEnableAlarm()
            self.INT_MEM_Aalarm.admEnableAlarm()
            self.LOG_OVERLOADalarm.admEnableAlarm()
            self.CLI_ACCESSalarm.admEnableAlarm()
            self.CLI_DEBUG_ACCESSalarm.admEnableAlarm()
            self.NTP_SYNCHalarm.admEnableAlarm()
            if self.pendingBoot:
                self.updateReq(self, self, uploadNReboot = True)
                self.pendingBoot = False
            else:
                self.updateReq(self, self, uploadNReboot = False)
        if (((changedOpStateDetail & OP_INIT[STATE]) and (opStateDetail & OP_INIT[STATE])) or
            ((changedOpStateDetail & OP_DISCONNECTED[STATE]) and (opStateDetail & OP_DISCONNECTED[STATE])) or
            ((changedOpStateDetail & OP_NOIP[STATE]) and (opStateDetail & OP_NOIP[STATE])) or
            ((changedOpStateDetail & OP_UNDISCOVERED[STATE]) and (opStateDetail & OP_UNDISCOVERED[STATE]))):
            self.NOT_CONNECTEDalarm.raiseAlarm("Decoder has not connected, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker", p_sysStateTransactionId, True)
        elif (((changedOpStateDetail & OP_INIT[STATE]) and not (opStateDetail & OP_INIT[STATE])) or
            ((changedOpStateDetail & OP_DISCONNECTED[STATE]) and not (opStateDetail & OP_DISCONNECTED[STATE])) or
            ((changedOpStateDetail & OP_NOIP[STATE]) and not (opStateDetail & OP_NOIP[STATE])) or
            ((changedOpStateDetail & OP_UNDISCOVERED[STATE]) and not (opStateDetail & OP_UNDISCOVERED[STATE]))):
            self.NOT_CONNECTEDalarm.ceaseAlarm("Decoder has now successfully connected")
        if (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.raiseAlarm("Decoder has not been configured, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker, or the MAC address may not be correctly provisioned", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and not (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.ceaseAlarm("Decoder is now successfully configured")
        if (changedOpStateDetail & OP_SERVUNAVAILABLE[STATE]) and (opStateDetail & OP_SERVUNAVAILABLE[STATE]):
            self.SERVER_UNAVAILalarm.raiseAlarm("The server is missing keep-alive ping continuity messages from the client", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_SERVUNAVAILABLE[STATE]) and not (opStateDetail & OP_SERVUNAVAILABLE[STATE]):
            self.SERVER_UNAVAILalarm.ceaseAlarm("The server is now receiving keep-alive ping continuity messages from the client")
        if (changedOpStateDetail & OP_CLIEUNAVAILABLE[STATE]) and (opStateDetail & OP_CLIEUNAVAILABLE[STATE]):
            self.CLIENT_UNAVAILalarm.raiseAlarm("The client is missing keep-alive ping continuity messages from the server", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_CLIEUNAVAILABLE[STATE]) and not (opStateDetail & OP_CLIEUNAVAILABLE[STATE]):
            self.CLIENT_UNAVAILalarm.ceaseAlarm("The client is now receiving keep-alive ping continuity messages from the server")
        if (changedOpStateDetail & OP_INTFAIL[STATE]) and (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.raiseAlarm("Decoder is experiencing an internal error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INTFAIL[STATE]) and not (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.ceaseAlarm("Decoder is no longer experiencing any internal errors")
        if (changedOpStateDetail & OP_CBL[STATE]) and (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.raiseAlarm("Parent object for which this object is depending on has failed", p_sysStateTransactionId, False)
        elif (changedOpStateDetail & OP_CBL[STATE]) and not (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.ceaseAlarm("Parent object for which this object is depending on is now working")

    def __onDecoderOpStateChange(self, topic, value):
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " received a new OP State from client: " + value + " setting server OP-state accordingly")
        try:
            opBitMap = self.getOpStateDetailBitMapFromStr(value)
        except:
            trace.notify(DEBUG_ERROR, "Decoder " + self.nameKey.value + " received OP State not recognized, there is likely a missmatch beween server and client assumed OP States")
            return
        self.setOpStateDetail(opBitMap & ~OP_SERVUNAVAILABLE[STATE])
        self.unSetOpStateDetail(~opBitMap & ~OP_SERVUNAVAILABLE[STATE])

        #self.setOpStateDetail(opBitMap & ~OP_SERVUNAVAILABLE[STATE])
        #self.unSetOpStateDetail(~OP_SERVUNAVAILABLE[STATE] )

        #self.setOpStateDetail(opBitMap & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        #self.unSetOpStateDetail(~opBitMap & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        #self.mqttClient.publish(self.decoderOpDownStreamTopic, self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & (OP_SERVUNAVAILABLE[STATE] | OP_CBL[STATE])))

    def __decoderRestart(self):
        try:
            self.decoderRebootTopic
        except:
            trace.notify(DEBUG_ERROR, "Decoder " + self.nameKey.value + " could not be restarted")
            return
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested will be restarted")
        self.mqttClient.publish(self.decoderRebootTopic, REBOOT_PAYLOAD)
        self.restart = True

    def __onDecoderConfigReq(self, topic, value):
        if self.getAdmState() == ADM_DISABLE:
            trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested new configuration, but Adminstate is disabled - will not provide the configuration")
            return
        trace.notify(DEBUG_INFO, "Decoder " + self.nameKey.value + " requested new configuration")
        self.mqttClient.publish(MQTT_JMRI_PRE_TOPIC + MQTT_DECODER_CONFIG_TOPIC + self.decoderMqttURI.value, self.parent.getXmlConfigTree(decoder=True, text=True, onlyIncludeThisChild=self))
        self.restart = False

    def __startSupervision(self): #Improvement request - after disabled a quarantain period should start requiring consecutive DECODER_MAX_MISSED_PINGS before bringing it back - may require proportional slack in the __supervisionTimer - eg (self.parent.decoderMqttPingPeriod.value*1.1)
        if self.supervisionActive:
            return
        trace.notify(DEBUG_INFO, "Decoder supervision for " + self.nameKey.value + " is started/restarted")
        self.missedPingReq = 0
        self.supervisionActive = True
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)
        threading.Timer(self.parent.decoderMqttPingPeriod.value, self.__supervisionTimer).start()

    def __stopSupervision(self):
        trace.notify(DEBUG_INFO, "Decoder supervision for " + self.nameKey.value + " is stoped")
        self.missedPingReq = 0
        self.supervisionActive = False
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_UPSTREAM + self.decoderMqttURI.value, self.__onPingReq)

    def __supervisionTimer(self):
        trace.notify(DEBUG_VERBOSE, "Decoder " + self.nameKey.value + " received a supervision timer")
        if self.getAdmState() == ADM_DISABLE:
            self.missedPingReq = 0
            return
        if not self.supervisionActive:
            return
        threading.Timer(self.parent.decoderMqttPingPeriod.value, self.__supervisionTimer).start()
        self.missedPingReq += 1
        if self.missedPingReq >= DECODER_MAX_MISSED_PINGS:
            self.missedPingReq = DECODER_MAX_MISSED_PINGS
            #if not (self.getOpStateDetail() & OP_FAIL[STATE]):
            
            self.setOpStateDetail(OP_SERVUNAVAILABLE)

    def __onPingReq(self, topic, payload):
        trace.notify(DEBUG_VERBOSE, "Decoder " + self.nameKey.value + " received an upstream PING request")
        if self.getAdmState() == ADM_DISABLE:
            self.missedPingReq = 0
            return
        if not self.supervisionActive:
            return
        self.missedPingReq = 0
        #if (self.getOpStateDetail() & OP_FAIL[STATE]):
        self.unSetOpStateDetail(OP_SERVUNAVAILABLE)
        if not self.restart:
            self.mqttClient.publish(MQTT_JMRI_PRE_TOPIC + MQTT_SUPERVISION_DOWNSTREAM + self.decoderMqttURI.value, PING)
