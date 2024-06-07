#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI satelite class providing the genJMRI satelite management- and supervision. genJMRI provides various sensor and actuators interworking
# with JMRI.
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
from sensorLogic import *
from actuatorLogic import *
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('mqtt', '..\\mqtt\\mqtt.py')
from mqtt import mqtt
imp.load_source('mqttTopicsNPayloads', '..\\mqtt\\jmriMqttTopicsNPayloads.py')
from mqttTopicsNPayloads import *
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
# Class: satelite
# Purpose:      Provides management- and supervision capabilities of genJMRI satelites. Implements the management-, configuration-, supervision-,
#               and control of genJMRI satelites.
#               See archictecture.md for more information
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class satelite(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name = None, demo = False):
        self.win = win
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.provioned = False
        self.sysNameReged = False
        self.schemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.SAT_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.sensors.value = []
        self.actuators.value = []
        self.sensTopology = topologyMgr(self, SAT_MAX_SENS_PORTS)
        self.actTopology = topologyMgr(self, SAT_MAX_ACTS_PORTS)
        self.commitAll()
        self.childs.value = self.sensors.candidateValue + self.actuators.candidateValue
        self.updating = False
        self.pendingBoot = False
        if name:
            self.satSystemName.value = name
        else:
            self.satSystemName.value = "GJSAT-MyNewSateliteSysName"
        self.nameKey.value = "Sat-" + self.satSystemName.candidateValue
        self.userName.value = "MyNewSateliteUsrName"
        self.description.value = "MyNewSateliteDescription"
        self.satLinkAddr.value = 0
        self.commitAll()
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, SATELITE, displayIcon=SATELITE_ICON)
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite has not received a valid configuration")
        self.SAT_EXCESSIVE_BER_ERRORalarm = alarm(self, "TRANSMISION ERROR", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite excessive transmission errors")
        self.SAT_GEN_ERRORalarm = alarm(self, "GENERAL RECOVERABLE ERROR", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite is experiencing a recoverable general error")
        self.INT_FAILalarm = alarm(self, "INTERNAL FAILURE", self.nameKey.value, ALARM_CRITICALITY_A, "Satelite has experienced an internal error")
        self.CBLalarm = alarm(self, "CONTROL-BLOCK STATUS", self.nameKey.value, ALARM_CRITICALITY_C, "Parent object blocked resulting in a control-block of this object")
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.win.inactivateMoMObj(self.item)
        self.setOpStateDetail(OP_INIT[STATE] | OP_UNCONFIGURED[STATE])
        self.clearStats()
        if self.demo:
            for i in range(SAT_MAX_SENS_PORTS):
                self.addChild(SENSOR, name="MS-" + str(i), config=False, demo=True)
            for i in range(SAT_MAX_ACTS_PORTS):
                self.addChild(ACTUATOR, name="actuator-" + str(i), config=False, demo=True)
            self.logStatsProducer = threading.Thread(target=self.__demoStatsProducer)
            self.logStatsProducer.start()
        trace.notify(DEBUG_INFO,"New Satelite link: " + self.nameKey.candidateValue + " created - awaiting configuration")

    @staticmethod
    def aboutToDelete(ref):
        ref.parent.satTopology.removeTopologyMember(ref.satSystemName.value)

    def onXmlConfig(self, xmlConfig):
        try:
            satXmlConfig = parse_xml(xmlConfig,
                                        {"SystemName": MANSTR,
                                         "UserName": OPTSTR,
                                         "LinkAddress": MANINT,
                                         "Description": OPTSTR,
                                         "AdminState": OPTSTR
                                        }
                                    )
            self.satSystemName.value = satXmlConfig.get("SystemName")
            if satXmlConfig.get("UserName") != None:
                self.userName.value = satXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            self.nameKey.value = "Sat-" + self.satSystemName.candidateValue
            self.satLinkAddr.value = satXmlConfig.get("LinkAddress")
            if satXmlConfig.get("Description") != None:
                self.description.value = satXmlConfig.get("Description")
            else:
                self.description.value = ""
            if satXmlConfig.get("AdminState") != None:
                self.setAdmState(satXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Satelite traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + satXmlConfig.get("SystemName") +
                                      ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        for sensor in xmlConfig:
            if sensor.tag == "Sensor":
                res = self.addChild(SENSOR, config=False, configXml=sensor, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Sensor " + satXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        for actuator in xmlConfig:
            if actuator.tag == "Actuator":
                res = self.addChild(ACTUATOR, config=False, configXml=actuator, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Actuator to " + satXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
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
        trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK
        return rc.OK

    def regSysName(self, sysName):
        return self.parent.regSysName(sysName)

    def unRegSysName(self, sysName):
        return self.parent.unRegSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    return res 
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Satelite " + self.satSystemName.value)
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Satelite " + self.satSystemName.value)
                return res
        else:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.value + " was not reconfigured, skiping re-configuration")
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
        trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.candidateValue + " received configuration abort()")
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
        trace.notify(DEBUG_TERSE, "Providing satelite .xml configuration")
        sateliteXml = ET.Element("Satelite")
        sysName = ET.SubElement(sateliteXml, "SystemName")
        sysName.text = self.satSystemName.value
        usrName = ET.SubElement(sateliteXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(sateliteXml, "Description")
        descName.text = self.description.value
        LinkAddress = ET.SubElement(sateliteXml, "LinkAddress")
        LinkAddress.text = str(self.satLinkAddr.value)
        adminState = ET.SubElement(sateliteXml, "AdminState")
        adminState.text = self.getAdmState()[STATE_STR]
        if includeChilds:
            for sensor in self.sensors.value:
                sateliteXml.append(sensor.getXmlConfigTree(decoder=decoder))
            for actuator in self.actuators.value:
                sateliteXml.append(actuator.getXmlConfigTree(decoder=decoder))
        return minidom.parseString(ET.tostring(sateliteXml)).toprettyxml(indent="   ") if text else sateliteXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE |METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE & ~METHOD_EDIT & ~METHOD_DELETE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name = None, config = True, configXml = None, demo = False):
        if resourceType == SENSOR:
            self.sensors.append(sensor(self.win, self.item, self.rpcClient, self.mqttClient, name = name, demo = demo))
            self.childs.value = self.sensors.candidateValue + self.actuators.candidateValue
            trace.notify(DEBUG_INFO, "Sensor: " + self.sensors.candidateValue[-1].nameKey.candidateValue + "is being added to satelite" + self.nameKey.value)
            if not config and configXml:
                nameKey = self.sensors.candidateValue[-1].nameKey.candidateValue
                res = self.sensors.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Sensor: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Sensor: " + self.sensors.value[-1].nameKey.value + " successfully added to satelite link " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_sensorDialog(self.sensors.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Satelite could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\": " +
                                      ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo))
            return rc.GEN_ERR
        elif resourceType == ACTUATOR:
            self.actuators.append(actuator(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=demo))
            self.childs.value = self.sensors.candidateValue + self.actuators.candidateValue
            trace.notify(DEBUG_INFO, "Sensor: " + self.actuators.candidateValue[-1].nameKey.candidateValue + "is being added to satelite" + self.nameKey.value)
            if not config and configXml:
                nameKey = self.actuators.candidateValue[-1].nameKey.candidateValue
                res = self.actuators.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure Actuator: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Actuator: " + self.actuators.value[-1].nameKey.value + " successfully added to satelite link " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_actuatorDialog(self.actuators.candidateValue[-1], self.rpcClient, edit=True, newConfig = True)
                self.dialog.show()
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Satelite could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\": " +
                                      ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Satelite only take ACTUATOR or SENSOR as child, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return self.canDelete()
        try:
            child.sensors.remove(child)
        except:
            pass
        try:
            self.actuators.remove(child)
        except:
            pass
        self.childs.value = self.sensors.candidateValue + self.actuators.candidateValue
        return rc.OK

    def view(self):
        self.dialog = UI_sateliteDialog(self, self.rpcClient, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_sateliteDialog(self, self.rpcClient, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, SENSOR | ACTUATOR)
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
        self.SAT_EXCESSIVE_BER_ERRORalarm.ceaseAlarm("Source object deleted")
        self.SAT_GEN_ERRORalarm.ceaseAlarm("Source object deleted")
        self.INT_FAILalarm.ceaseAlarm("Source object deleted")
        self.CBLalarm.ceaseAlarm("Source object deleted")
        self.parent.unRegSysName(self.satSystemName.value)
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq(self, self, uploadNReboot = True)
        return rc.OK

    def accepted(self):
        self.nameKey.value = "Sat-" + self.satSystemName.candidateValue
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

    def getDecoderUri(self):
        return self.parent.getDecoderUri()

    def clearStats(self):
        self.rxCrcErr = 0
        self.txCrcErr = 0
        self.wdErr = 0

    def __validateConfig(self):
        if not self.sysNameReged:
            res = self.parent.regSysName(self.satSystemName.candidateValue)
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "System name " + self.satSystemName.candidateValue + " already in use")
                return res
        self.sysNameReged = True
        weakSelf = weakref.ref(self, satelite.aboutToDelete)
        res = self.parent.satTopology.addTopologyMember(self.satSystemName.candidateValue, self.satLinkAddr.candidateValue, weakSelf)
        if res:
            print (">>>>>>>>>>>>> Satelite failed address/port topology validation for port/address: " + str(self.satLinkAddr.candidateValue) + "return code: " + rc.getErrStr(res))
            trace.notify(DEBUG_ERROR, "Satelite failed address/port topology validation for port/address: " + str(self.satLinkAddr.candidateValue) + rc.getErrStr(res))
            return res
        return rc.OK # Place holder for object config validation

    def __setConfig(self):
        self.satOpDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.satSystemName.value
        self.satOpUpStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM + self.parent.getDecoderUri() + "/" + self.satSystemName.value
        self.satAdmDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.satSystemName.value
        self.unRegOpStateCb(self.__sysStateRespondListener)
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateRespondListener, OP_DISABLED[STATE])
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value, self.__onStats)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value, self.__onStats)
        self.mqttClient.subscribeTopic(self.satOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.SAT_EXCESSIVE_BER_ERRORalarm.updateAlarmSrc(self.nameKey.value)
        self.SAT_GEN_ERRORalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
        return rc.OK

    def __sysStateRespondListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, "Satelite " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.satAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
            else:
                self.mqttClient.publish(self.satAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)

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
        if  (changedOpStateDetail & OP_DISABLED[STATE]) and (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admDisableAlarm()
            self.NOT_CONFIGUREDalarm.admDisableAlarm()
            self.SAT_EXCESSIVE_BER_ERRORalarm.admDisableAlarm()
            self.SAT_GEN_ERRORalarm.admDisableAlarm()
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.squelshAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.SAT_EXCESSIVE_BER_ERRORalarm.admEnableAlarm()
            self.SAT_GEN_ERRORalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
            if self.pendingBoot:
                self.updateReq(self, self, uploadNReboot = True)
                self.pendingBoot = False
            else:
                self.updateReq(self, self, uploadNReboot = False)
        if (changedOpStateDetail & OP_INIT[STATE]) and (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.raiseAlarm("Satelite has not connected, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INIT[STATE]) and not (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.ceaseAlarm("Satelite link has now successfully connected")
        if (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.raiseAlarm("Satelite has not been configured, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker, or the MAC address may not be correctly provisioned", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and not (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.ceaseAlarm("Satelite is now successfully configured")
        if (changedOpStateDetail & OP_ERRSEC[STATE]) and (opStateDetail & OP_ERRSEC[STATE]):
            self.SAT_EXCESSIVE_BER_ERRORalarm.raiseAlarm("Satelite is experiencing exsessive transmision errors", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_ERRSEC[STATE]) and not (opStateDetail & OP_ERRSEC[STATE]):
            self.SAT_EXCESSIVE_BER_ERRORalarm.ceaseAlarm("Satelite transmision error rate is now below the alarm threshold")
        if (changedOpStateDetail & OP_GENERR[STATE]) and (opStateDetail & OP_GENERR[STATE]):
            self.SAT_GEN_ERRORalarm.raiseAlarm("Satelite is experiencing a recoverable error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_GENERR[STATE]) and not (opStateDetail & OP_GENERR[STATE]):
            self.SAT_GEN_ERRORalarm.ceaseAlarm("The Satelite general error has ceased")
        if (changedOpStateDetail & OP_INTFAIL[STATE]) and (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.raiseAlarm("Satelite is experiencing an internal error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INTFAIL[STATE]) and not (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.ceaseAlarm("Satelite is no longer experiencing any internal errors")
        if (changedOpStateDetail & OP_CBL[STATE]) and (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.raiseAlarm("Parent object for which this object is depending on has failed", p_sysStateTransactionId, False)
        elif (changedOpStateDetail & OP_CBL[STATE]) and not (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.ceaseAlarm("Parent object for which this object is depending on is now working")
        return

    def __onDecoderOpStateChange(self, topic, value):
        trace.notify(DEBUG_INFO, "Satelite " + self.nameKey.value + " received a new OP State from client: " + value + " setting server OP-state accordingly")
        self.setOpStateDetail(self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        self.unSetOpStateDetail(~self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])

    def __onStats(self, topic, payload):
        # We expect a report every second
        trace.notify(DEBUG_VERBOSE, "Satelite " + self.nameKey.value + " received a statistics report")
        # statsXmlTree = ET.ElementTree(ET.fromstring(payload.decode('UTF-8')))
        statsXmlTree = ET.ElementTree(ET.fromstring(payload))
        if str(statsXmlTree.getroot().tag) != "statReport":
            trace.notify(DEBUG_ERROR, "Satelite statistics report missformated")
            return
        if not (self.getOpStateDetail() & OP_DISABLED[STATE]):
            statsXmlVal = parse_xml(statsXmlTree.getroot(),
                                    {"rxCrcErr": MANINT,
                                    "txCrcErr": MANINT,
                                    "wdErr": MANINT
                                    }
                                    )
            rxCrcErr = int(statsXmlVal.get("rxCrcErr"))
            txCrcErr = int(statsXmlVal.get("txCrcErr"))
            wdErr = int(statsXmlVal.get("wdErr"))
            self.rxCrcErr += rxCrcErr
            self.txCrcErr += txCrcErr
            self.wdErr += wdErr

    def __demoStatsProducer(self):
        while True:
            self.rxCrcErr += 1
            self.txCrcErr += 1
            self.wdErr += 1
            time.sleep(0.25)
# End Satelite
#------------------------------------------------------------------------------------------------------------------------------------------------