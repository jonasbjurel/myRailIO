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
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.SAT_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.sensors.value = []
        self.actuators.value = []
        self.childs.value = self.sensors.candidateValue + self.actuators.candidateValue
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.satSystemName.value = name
        else:
            self.satSystemName.value = "GJSAT-NewSatSysName"
        self.nameKey.value = "Sat-" + self.satSystemName.candidateValue
        self.userName.value = "GJSAT-NewSatLinkUsrName"
        self.satLinkAddr.value = 0
        self.description.value = "New Satelite"
        trace.notify(DEBUG_INFO,"New Satelite link: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, SATELITE, displayIcon=SATELITE_ICON)
        self.commitAll()
        self.rxCrcErr = 0
        self.txCrcErr = 0
        self.wdErr = 0
        if self.demo:
            for i in range(SAT_MAX_SENS_PORTS):
                self.addChild(SENSOR, name="MS-" + str(i), config=False, demo=True)
            for i in range(SAT_MAX_ACTS_PORTS):
                self.addChild(ACTUATOR, name="actuator-" + str(i), config=False, demo=True)
            self.logStatsProducer = threading.Thread(target=self.__demoStatsProducer)
            self.logStatsProducer.start()

    def onXmlConfig(self, xmlConfig):
        self.setOpStateDetail(OP_CONFIG)
        try:
            satXmlConfig = parse_xml(xmlConfig,
                                        {"SystemName": MANSTR,
                                         "UserName": OPTSTR,
                                         "Address": MANINT,
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
            self.satLinkAddr.value = satXmlConfig.get("Address")
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
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + satXmlConfig.get("SystemName") +
                                      ", return code: " + trace.getErrStr(res))
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

    def updateReq(self):
        return self.parent.updateReq()

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

    def checkSysName(self, sysName):
        return self.parent.checkSysName(sysName)

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
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.value + " was reconfigured - applying the configuration")

            res = self.__setConfig()
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Satelite " + self.satSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Satelite " + self.satSystemName.value + " was not reconfigured, skiping re-configuration")
        self.unSetOpStateDetail(OP_INIT)
        self.unSetOpStateDetail(OP_CONFIG)
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
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
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
        address = ET.SubElement(sateliteXml, "Address")
        address.text = str(self.satLinkAddr.value)
        if not decoder:
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
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name=None, config=True, configXml=None, demo=False):
        if resourceType == SENSOR:
            self.sensors.append(sensor(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=demo))
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
                self.dialog = UI_sensorDialog(self.sensors.candidateValue[-1], edit=True)
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
                self.dialog = UI_actuatorDialog(self.actuators.candidateValue[-1], edit=True)
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
        self.dialog = UI_sateliteDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_sateliteDialog(self, edit=True)
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
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.parent.updateReq()
        return rc.OK

    def accepted(self):
        self.setOpStateDetail(OP_CONFIG)
        self.nameKey.value = "Sat-" + self.satSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Could not configure " + nameKey + ", return code: " + trace.getErrStr(res))
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
        res = self.parent.checkSysName(self.satSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.satSystemName.candidateValue + " already in use")
            return res
        return rc.OK # Place holder for object config validation

    def __setConfig(self):
        self.satOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value
        self.satAdmTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_ADMSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value
        #self.mqttClient.unsubscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value, self.__onStats)
        self.mqttClient.subscribeTopic(MQTT_JMRI_PRE_TOPIC + MQTT_SAT_TOPIC + MQTT_STATS_TOPIC + self.parent.getDecoderUri() + "/" + self.satSystemName.value, self.__onStats)
        #trace.notify(DEBUG_PANIC, "SUBSCRIBED")
        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        return rc.OK

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Satelite  " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()))
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.satOpTopic, OP_AVAIL_PAYLOAD)
            self.clearStats()
        else:
            self.mqttClient.publish(self.satOpTopic, OP_UNAVAIL_PAYLOAD)
        if self.getAdmState() == ADM_ENABLE:
            self.mqttClient.publish(self.satAdmTopic, ADM_ON_LINE_PAYLOAD)
        else:
            self.mqttClient.publish(self.satAdmTopic, ADM_OFF_LINE_PAYLOAD)

    def __onStats(self, topic, payload):
        # We expect a report every second
        trace.notify(DEBUG_VERBOSE, self.nameKey.value + " received a statistics report")
        prevRxCrcErr = self.rxCrcErr
        prevTxCrcErr = self.txCrcErr
        prevWdErr = self.wdErr
        statsXmlTree = ET.ElementTree(ET.fromstring(payload.decode('UTF-8')))
        if str(statsXmlTree.getroot().tag) != "statReport":
            trace.notify(DEBUG_ERROR, "Satelite statistics report missformated")
            return
        statsXmlVal = parse_xml(statsXmlTree.getroot(),
                                {"rxCrcErr": MANINT,
                                 "txCrcErr": MANINT,
                                 "wdErr": MANINT
                                }
                               )
        rxCrcErr = int(statsXmlVal.get("rxCrcErr"))
        txCrcErr = int(statsXmlVal.get("txCrcErr"))
        wdErr = int(statsXmlVal.get("wdErr"))
        if not self.getOpStateDetail():
            self.rxCrcErr += rxCrcErr
            self.txCrcErr += txCrcErr
            self.wdErr += wdErr
            if rxCrcErr > SAT_CRC_ES_HIGH_TRESH or txCrcErr > SAT_CRC_ES_HIGH_TRESH or wdErr > SAT_WD_ES_HIGH_TRESH:
                trace.notify(DEBUG_INFO, self.nameKey.value + " entered into an \"Errored second\" opState")
                self.setOpStateDetail(OP_ERRSEC)
        elif self.getOpStateDetail() & OP_ERRSEC[STATE]:
            if rxCrcErr <= SAT_CRC_ES_LOW_TRESH and txCrcErr <= SAT_CRC_ES_LOW_TRESH and wdErr <= SAT_WD_ES_LOW_TRESH:
                trace.notify(DEBUG_INFO, self.nameKey.value + " exited the \"Errored second\" opState")
                self.unSetOpStateDetail(OP_ERRSEC)

    def __demoStatsProducer(self):
        while True:
            self.rxCrcErr += 1
            self.txCrcErr += 1
            self.wdErr += 1
            time.sleep(0.25)
# End Satelite
#------------------------------------------------------------------------------------------------------------------------------------------------