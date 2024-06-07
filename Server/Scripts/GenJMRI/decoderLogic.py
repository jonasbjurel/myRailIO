#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI decoder class providing the genJMRI decoder management-, supervision and configuration. genJMRI provides the concept of decoders
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
from mqtt import mqtt
imp.load_source('syslog', '..\\trace\\syslog.py')
from syslog import rSyslog
imp.load_source('alarmHandler', '..\\alarmHandler\\alarmHandler.py')
from alarmHandler import alarm
imp.load_source('mqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from mqttTopicsNPayloads import *
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
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
# Purpose: The decoder class provides decoder management-, supervision and configuration. genJMRI provides the concept of decoders
# for controling various JMRI I/O resources such as lights-, light groups-, signal masts-, sensors and actuators throug various periperial
# devices and interconnect links.
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), regSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
'''
Alarms:
    CPU (Three levels)
    Memory (Three levels)
    CLI
    Debug mode
    NTP
    Log overload
    WIFI SNR
'''
class decoder(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name = None, demo = False):
        self.win = win
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.provioned = False
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
        self.nameKey.value = "Decoder-" + self.decoderSystemName.candidateValue
        self.userName.value = "MyNewDecoderUsrName"
        self.decoderMqttURI.value = "no.valid.uri"
        self.mac.value = "00:00:00:00:00:00"
        self.description.value = "MyNewdecoderDescription"
        self.commitAll()
        self.item = self.win.registerMoMObj(self, self.parentItem, self.nameKey.candidateValue, DECODER, displayIcon=DECODER_ICON)
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder has not received a valid configuration")
        self.SERVER_UNAVAILalarm = alarm(self, "KEEP-ALIVE STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Server-side has reported missing keep-live messages from client")
        self.CLIENT_UNAVAILalarm = alarm(self, "KEEP-ALIVE STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Client-side has reported missing keep-live messages from server")
        self.CPU_LOAD_Calarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder has reached 75% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.CPU_LOAD_Balarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Decoder has reached 85% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.CPU_LOAD_Aalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder has reached 95% CPU Load") #NEEDS TO BE IMPLEMENTED
        self.INT_MEM_Calarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder internal memory usage has reached 75%") #NEEDS TO BE IMPLEMENTED
        self.INT_MEM_Balarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Decoder internal memory usage has reached 85%") #NEEDS TO BE IMPLEMENTED
        self.INT_MEM_Calarm = alarm(self, "RESOURCE CONSTRAINT WARNING", self.nameKey.value, ALARM_CRITICALITY_A, "Decoder internal memory usage has reached 95%") #NEEDS TO BE IMPLEMENTED
        self.LOG_OVERLOADalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder logging overloaded") #NEEDS TO BE IMPLEMENTED
        self.CLI_ACCESSalarm = alarm(self, "AUDIT TRAIL", self.nameKey.value, ALARM_CRITICALITY_C, "Decoder is being accessed by a CLI user") #NEEDS TO BE IMPLEMENTED
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
        if self.demo:
            for i in range(DECODER_MAX_LG_LINKS):
                self.addChild(LIGHT_GROUP_LINK, name="GJLL-" + str(i), config=False, demo=True)
            for i in range(DECODER_MAX_SAT_LINKS):
                self.addChild(SATELITE_LINK, name=i, config=False, demo=True)
        trace.notify(DEBUG_INFO,"New decoder: " + self.nameKey.candidateValue + " created - awaiting configuration")

    @staticmethod
    def aboutToDelete(ref):
        ref.parent.decoderMacTopology.removeTopologyMember(ref.decoderSystemName.value)

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
        for sateliteLinkXml in xmlConfig:
            if sateliteLinkXml.tag == "SateliteLink":
                res = self.addChild(SATELITE_LINK, config=False, configXml=sateliteLinkXml, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add satelite link to " + decoderXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
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
        self.provioned = True
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
        if not self.provioned:
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
        elif resourceType == SATELITE_LINK:
            self.satLinks.append(satLink(self.win, self.item, self.rpcClient, self.mqttClient, name = name, demo = demo))
            self.childs.value = self.lgLinks.candidateValue + self.satLinks.candidateValue
            trace.notify(DEBUG_INFO, "Satelite link: " + self.satLinks.candidateValue[-1].nameKey.candidateValue + "is being added to decoder " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.satLinks.candidateValue[-1].nameKey.candidateValue
                res = self.satLinks.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Satelite link: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Satelite link: " + self.satLinks.value[-1].nameKey.value + " successfully added to decoder " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_satLinkDialog(self.satLinks.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Decoder could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo) + " \"replacement\": " + str(replacement))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Gen JMRI server (top decoder) only takes SATELITE_LINK and LIGHT_GROPU_LINK as childs, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return child.canDelete()
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
        self.dialog = UI_addDialog(self, LIGHT_GROUP_LINK | SATELITE_LINK)
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
        self.NOT_CONNECTEDalarm.ceaseAlarm("Source object deleted")
        self.NOT_CONFIGUREDalarm.ceaseAlarm("Source object deleted")
        self.SERVER_UNAVAILalarm.ceaseAlarm("Source object deleted")
        self.CLIENT_UNAVAILalarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Calarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Balarm.ceaseAlarm("Source object deleted")
        self.CPU_LOAD_Aalarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Calarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Balarm.ceaseAlarm("Source object deleted")
        self.INT_MEM_Calarm.ceaseAlarm("Source object deleted")
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
            print (">>>>>>>>>>>>> Decoder failed Mac-address topology validation for MAC: " + str(self.mac.candidateValue) + "return code: " + rc.getErrStr(res))
            trace.notify(DEBUG_ERROR, "Decoder failed Mac-address topology validation for MAC: " + str(self.mac.candidateValue) + "return code: " + rc.getErrStr(res))
            return res
        res = self.parent.decoderUriTopology.addTopologyMember(self.decoderSystemName.candidateValue, self.decoderMqttURI.candidateValue, weakSelf)
        if res:
            print (">>>>>>>>>>>>> Decoder failed URI topology validation for URI: " + str(self.decoderMqttURI.candidateValue) + "return code: " + rc.getErrStr(res))
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
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.SERVER_UNAVAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CLIENT_UNAVAILalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
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
        if opStateDetail & OP_DISABLED[STATE]:
            self.win.inactivateMoMObj(self.item)
        elif opStateDetail & OP_CBL[STATE]:
            self.win.controlBlockMarkMoMObj(self.item)
        elif opStateDetail:
            self.win.faultBlockMarkMoMObj(self.item, True)
        else:
            self.win.faultBlockMarkMoMObj(self.item, False)
        if (changedOpStateDetail & OP_DISABLED[STATE]) and (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admDisableAlarm()
            self.NOT_CONFIGUREDalarm.admDisableAlarm()
            self.SERVER_UNAVAILalarm.admDisableAlarm()
            self.CLIENT_UNAVAILalarm.admDisableAlarm()
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.admDisableAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.SERVER_UNAVAILalarm.admEnableAlarm()
            self.CLIENT_UNAVAILalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
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
