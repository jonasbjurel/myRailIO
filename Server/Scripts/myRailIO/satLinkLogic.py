#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A myRailIO sateliteLink class providing the myRailIO satelite link management- and supervision. myRailIO provides the concept of satelite links
# for daisy-chaining of satelites which provides sensor and actuator capabilities.
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
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
from sateliteLogic import *
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('mqtt', '..\\mqtt\\mqtt.py')
from mqtt import mqtt
imp.load_source('mqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from mqttTopicsNPayloads import *
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
# Class: satLink
# Purpose:      Provides management- and supervision capabilities of myRailIO satelite links. Implements the management-, configuration-,
#               supervision-, and control of myRailIO satelite links - interconnecting satelites in daisy-chains.
#               See archictecture.md for more information
# StdMethods:   The standard myRailIO Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), regSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class satLink(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.provioned = False
        self.sysNameReged = False
        self.schemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.SAT_LINK_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.satelites.value = []
        self.satTopology = topologyMgr(self, SATLINK_MAX_SATS)
        self.childs.value = self.satelites.candidateValue
        self.updating = False
        self.pendingBoot = False
        if name:
            self.satLinkSystemName.value = name
        else:
            self.satLinkSystemName.value = "GJSL-MyNewSatLinkSysName"
        self.nameKey.value = "SatLink-" + self.satLinkSystemName.candidateValue
        self.userName.value = "MyNewSatLinkUsrName"
        self.description.value = "MyNewSatlinkDescription"
        self.satLinkNo.value = 0
        self.commitAll()
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, SATELITE_LINK, displayIcon=LINK_ICON)
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite link reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A,"Satelite link has not received a valid configuration")
        self.LINK_SCAN_OVERLOADalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Satelite link scanning overloaded") #NEEDS TO BE IMPLEMENTED
        self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm = alarm(self, "TRANSMISION ERROR", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite link excessive transmission errors")
        self.LINK_SCAN_GEN_ERRORalarm = alarm(self, "GENERAL RECOVERABLE ERROR", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite link is experiencing a recoverable general error")
        self.INT_FAILalarm = alarm(self, "INTERNAL FAILURE", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite link has experienced an internal error")
        self.CBLalarm = alarm(self, "CONTROL-BLOCK STATUS", self.nameKey.value, ALARM_CRITICALITY_C, "Parent object blocked resulting in a control-block of this object")
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.win.inactivateMoMObj(self.item)
        self.setOpStateDetail(OP_INIT[STATE] | OP_UNCONFIGURED[STATE])
        self.clearStats()
        if self.demo:
            self.logStatsProducer = threading.Thread(target=self.__demoStatsProducer)
            self.logStatsProducer.start()
            for i in range(0, MAX_SATS):
                self.addChild(SATELITE, name="GJS-" + str(i), config=False, demo=True)
        trace.notify(DEBUG_INFO,"New Satelite link: " + self.nameKey.candidateValue + " created - awaiting configuration")

    @staticmethod
    def aboutToDelete(ref):
        ref.parent.satLinkTopology.removeTopologyMember(ref.satLinkSystemName.value)

    def onXmlConfig(self, xmlConfig):
        try:
            satLinkXmlConfig = parse_xml(xmlConfig,
                                            {"SystemName": MANSTR,
                                             "UserName":OPTSTR,
                                             "Link": MANINT,
                                             "Description": OPTSTR,
                                             "AdminState":OPTSTR
                                             }
                                        )
            self.satLinkSystemName.value = satLinkXmlConfig.get("SystemName")
            if satLinkXmlConfig.get("UserName") != None:
                self.userName.value = satLinkXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            self.nameKey.value = "SatLink-" + self.satLinkSystemName.candidateValue
            self.satLinkNo.value = satLinkXmlConfig.get("Link")
            if satLinkXmlConfig.get("Description") != None:
                self.description.value = satLinkXmlConfig.get("Description")
            else:
                self.description.value = ""
            if satLinkXmlConfig.get("AdminState") != None:
                self.setAdmState(satLinkXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Satelite link, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + satLinkXmlConfig.get("SystemName") + ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        for sateliteXml in xmlConfig:
            if sateliteXml.tag == "Satelite":
                res = self.addChild(SATELITE, config=False, configXml=sateliteXml, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Satelite to Satelite link - " + satLinkXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
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
        trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def regSysName(self, sysName):
        return self.parent.regSysName(sysName)
    
    def unRegSysName(self, sysName):
        return self.parent.unRegSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Satelite link " + self.satLinkSystemName.value)
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Satelite link " + self.satLinkSystemName.value)
                return res
        else:
            trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.value + " was not reconfigured, skiping re-configuration")
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
        trace.notify(DEBUG_TERSE, "Satelite link " + self.satLinkSystemName.candidateValue + " received configuration abort()")
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
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing satelite link .xml configuration")
        satLinkXml = ET.Element("SateliteLink")
        sysName = ET.SubElement(satLinkXml, "SystemName")
        sysName.text = self.satLinkSystemName.value
        usrName = ET.SubElement(satLinkXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(satLinkXml, "Description")
        descName.text = self.description.value
        satLink = ET.SubElement(satLinkXml, "Link")
        satLink.text = str(self.satLinkNo.value)
        adminState = ET.SubElement(satLinkXml, "AdminState")
        adminState.text = self.getAdmState()[STATE_STR]
        if includeChilds:
            childs = True
            try:
                self.childs.value
            except:
                childs = False
            if childs:
                for child in self.childs.value:
                    satLinkXml.append(child.getXmlConfigTree(decoder=decoder))
        return minidom.parseString(ET.tostring(satLinkXml)).toprettyxml(indent="   ") if text else satLinkXml

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

    def addChild(self, resourceType, name=None, config=True, configXml=None, demo=False):
        if resourceType == SATELITE:
            self.satelites.append(satelite(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=self.demo))
            self.childs.value = self.satelites.candidateValue
            trace.notify(DEBUG_INFO, "Satelite: " + self.satelites.candidateValue[-1].nameKey.candidateValue + "is being added to satelite link " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.satelites.candidateValue[-1].nameKey.candidateValue
                res = self.satelites.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Satelite: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Satelite: " + self.satelites.value[-1].nameKey.value + " successfully added to satelite link " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_sateliteDialog(self.satelites.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Satelite link could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Satelite link only takes SATELITE as child, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return child.canDelete()
        try:
            self.satelites.remove(child)
        except:
            pass
        self.childs.value = self.satelites.candidateValue
        return rc.OK

    def view(self):
        self.dialog = UI_satLinkDialog(self, self.rpcClient, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_satLinkDialog(self, self.rpcClient, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, SATELITE)
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
        self.LINK_SCAN_OVERLOADalarm.ceaseAlarm("Source object deleted")
        self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.ceaseAlarm("Source object deleted")
        self.LINK_SCAN_GEN_ERRORalarm.ceaseAlarm("Source object deleted")
        self.INT_FAILalarm.ceaseAlarm("Source object deleted")
        self.CBLalarm.ceaseAlarm("Source object deleted")
        self.parent.unRegSysName(self.satLinkSystemName.value)
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq(self, self, uploadNReboot = True)
        return rc.OK

    def accepted(self):
        self.nameKey.value = "SatLink-" + self.satLinkSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
            self.pendingBoot = True
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not configure " + nameKey + ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Configured")
        return rc.OK

    def rejected(self):
        self.abort()
        return rc.OK
    
    def getTopology(self):
        return self.parent.getTopology() + "/" + self.satLinkSystemName.value

    def getDecoderUri(self):
        return self.parent.getDecoderUri()

    def clearStats(self):
        self.rxCrcErr = 0
        self.remCrcErr = 0
        self.rxSymErr = 0
        self.rxSizeErr = 0
        self.wdErr = 0

    def __validateConfig(self):
        if not self.sysNameReged:
            res = self.parent.regSysName(self.satLinkSystemName.candidateValue)
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "System name " + self.satLinkSystemName.candidateValue + " already in use")
                return res
        self.sysNameReged = True
        weakSelf = weakref.ref(self, satLink.aboutToDelete)
        res = self.parent.satLinkTopology.addTopologyMember(self.satLinkSystemName.candidateValue, self.satLinkNo.candidateValue, weakSelf)
        if res:
            print (">>>>>>>>>>>>> Satelite Link failed address/port topology validation for Link No: " + str(self.satLinkNo.candidateValue) + "return code: " + rc.getErrStr(res))
            trace.notify(DEBUG_ERROR, "Satelite Link failed address/port topology validation for Link No: " + str(self.satLinkNo.candidateValue) + rc.getErrStr(res))
            return res
        if len(self.satelites.candidateValue) > SATLINK_MAX_SATS:
            trace.notify(DEBUG_ERROR, "Too many satelites defined for satelite link " + str(self.satLinkSystemName.candidateValue) + ", " + str(len(self.satelites.candidateValue)) + "  given, " + str(SATLINK_MAX_SATS) + " is maximum")
            return rc.RANGE_ERR
        satAddrs = []
        for sat in self.satelites.candidateValue:
            try:
                satAddrs.index(sat.satLinkAddr.candidateValue)
                trace.notify(DEBUG_ERROR, "Satelite address " + str(sat.satLinkAddr.candidateValue) + " already in use for satelite link " + self.satLinkSystemName.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        return rc.OK

    def __setConfig(self):
        self.satLinkOpDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SATLINK_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.satLinkSystemName.value
        self.satLinkOpUpStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SATLINK_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM + self.parent.getDecoderUri() + "/" + self.satLinkSystemName.value
        self.satLinkAdmDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SATLINK_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.satLinkSystemName.value
        self.unRegOpStateCb(self.__sysStateRespondListener)
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateRespondListener, OP_DISABLED[STATE])
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SATLINK_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satLinkSystemName.value, self.__onStats)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SATLINK_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satLinkSystemName.value, self.__onStats)
        self.mqttClient.subscribeTopic(self.satLinkOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.LINK_SCAN_OVERLOADalarm.updateAlarmSrc(self.nameKey.value)
        self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.updateAlarmSrc(self.nameKey.value)
        self.LINK_SCAN_GEN_ERRORalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
        return rc.OK

    def __sysStateRespondListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, "Satelite link " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.satLinkAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
            else:
                self.mqttClient.publish(self.satLinkAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)

    def __sysStateAllListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        #trace.notify(DEBUG_INFO, self.nameKey.value + " got a new OP State - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
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
            self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.admDisableAlarm()
            self.LINK_SCAN_GEN_ERRORalarm.admDisableAlarm()
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.admDisableAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.admEnableAlarm()
            self.LINK_SCAN_GEN_ERRORalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
            if self.pendingBoot:
                self.updateReq(self, self, uploadNReboot = True)
                self.pendingBoot = False
            else:
                self.updateReq(self, self, uploadNReboot = False)
        if (changedOpStateDetail & OP_INIT[STATE]) and (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.raiseAlarm("Satelite link has not connected, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INIT[STATE]) and not (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.ceaseAlarm("Satelite link has now successfully connected")
        if (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.raiseAlarm("Satelite link has not been configured, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker, or the MAC address may not be correctly provisioned", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and not (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.ceaseAlarm("Satelite link is now successfully configured")
        if (changedOpStateDetail & OP_ERRSEC[STATE]) and (opStateDetail & OP_ERRSEC[STATE]):
            self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.raiseAlarm("Satelite link is experiencing excessive transmision errors", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_ERRSEC[STATE]) and not (opStateDetail & OP_ERRSEC[STATE]):
            self.LINK_SCAN_EXCESSIVE_BER_ERRORalarm.ceaseAlarm("Satelite link transmision error rate is now below the alarm threshold")
        if (changedOpStateDetail & OP_GENERR[STATE]) and (opStateDetail & OP_GENERR[STATE]):
            self.LINK_SCAN_GEN_ERRORalarm.raiseAlarm("Satelite link is experiencing a recoverable error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_GENERR[STATE]) and not (opStateDetail & OP_GENERR[STATE]):
            self.LINK_SCAN_GEN_ERRORalarm.ceaseAlarm("The Satelite link general error has ceased")
        if (changedOpStateDetail & OP_INTFAIL[STATE]) and (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.raiseAlarm("Satelite link is experiencing an internal error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INTFAIL[STATE]) and not (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.ceaseAlarm("Satelite link is no longer experiencing any internal errors")
        if (changedOpStateDetail & OP_CBL[STATE]) and (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.raiseAlarm("Parent object for which this object is depending on has failed", p_sysStateTransactionId, False)
        elif (changedOpStateDetail & OP_CBL[STATE]) and not (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.ceaseAlarm("Parent object for which this object is depending on is now working")

    def __onDecoderOpStateChange(self, topic, value):
        trace.notify(DEBUG_INFO, "Satelite link " + self.nameKey.value + " received a new OP State from client: " + value + " setting server OP-state accordingly")
        self.setOpStateDetail(self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        self.unSetOpStateDetail(~self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])

    def __onStats(self, topic, payload):
        trace.notify(DEBUG_VERBOSE, self.nameKey.value + " received a statistics report")
        # statsXmlTree = ET.ElementTree(ET.fromstring(payload.decode('UTF-8')))
        statsXmlTree = ET.ElementTree(ET.fromstring(payload))
        if str(statsXmlTree.getroot().tag) != "statReport":
            trace.notify(DEBUG_ERROR, "SatLink statistics report missformated")
            return
        if not (self.getOpStateDetail() & OP_DISABLED[STATE]):
            statsXmlVal = parse_xml(statsXmlTree.getroot(),
                                    {"rxCrcErr": MANINT,
                                    "remCrcErr": MANINT,
                                    "rxSymErr": MANINT,
                                    "rxSizeErr": MANINT,
                                    "wdErr": MANINT
                                    }
                                    )
            rxCrcErr = int(statsXmlVal.get("rxCrcErr"))
            remCrcErr = int(statsXmlVal.get("remCrcErr"))
            rxSymErr = int(statsXmlVal.get("rxSymErr"))
            rxSizeErr = int(statsXmlVal.get("rxSizeErr"))
            wdErr = int(statsXmlVal.get("wdErr"))
            self.rxCrcErr += rxCrcErr
            self.remCrcErr += remCrcErr
            self.rxSymErr += rxSymErr
            self.rxSizeErr += rxSizeErr
            self.wdErr += wdErr

    def __demoStatsProducer(self):
        while True:
            self.rxCrcErr += 1
            self.remCrcErr += 1
            self.rxSymErr += 1
            self.rxSizeErr += 1
            self.wdErr += 1
            time.sleep(0.25)
# End Sat Link
#------------------------------------------------------------------------------------------------------------------------------------------------