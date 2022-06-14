#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI sensor class providing the JMRI-genJMRI interactions for various sensors providing the bridge between following JMRI objects:
# SENSORS and MEMORIES, and the genJMRI satelite sensor object atributes: DIGOTAL sensor
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
from topDecoderLogic import *   #Should go away
from momResources import *
from ui import *
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
imp.load_source('jmriObj', '..\\rpc\\JMRIObjects.py')
from jmriObj import *
imp.load_source('jmriRpcClient', '..\\rpc\\genJMRIRpcClient.py')
from jmriRpcClient import *
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
# Class: sensor
# Purpose:      Provides the JMRI-genJMRI interactions for various sensors providing the bridge between following JMRI objects:
#               SENSORS-, and MEMORIES, and the genJMRI satelite sensor object atributes: DIGITAL.
#               Implements the management-, configuration-, supervision-, and control of genJMRI sensors.
#               See archictecture.md for more information
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class sensor(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.SENS_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.jmriSensSystemName.value = name
        else:
            self.jmriSensSystemName.value = "MS-NewSensSysName"
        self.nameKey.value = "Sens-" + self.jmriSensSystemName.candidateValue
        self.userName.value = "MS-NewSensUsrName"
        self.sensPort.value = 0
        self.sensType.value = "DIGITAL"
        self.sensState = "INACTIVE"
        self.description.value = "MS-NewSensDescription"
        trace.notify(DEBUG_INFO,"New Sensor: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, SENSOR, displayIcon=SENSOR_ICON)
        self.commitAll()
        self.sensTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SENS_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriSensSystemName.value

    def onXmlConfig(self, xmlConfig):
        self.setOpStateDetail(OP_CONFIG)
        try:
            sensorXmlConfig = parse_xml(xmlConfig,
                                        {"JMRISystemName": MANSTR,
                                         "JMRIUserName": OPTSTR,
                                         "JMRIDescription": OPTSTR,
                                         "Port": MANINT,
                                         "Type": MANSTR,
                                         "AdminState": OPTSTR
                                        }
                                       )
            self.jmriSensSystemName.value = sensorXmlConfig.get("JMRISystemName")
            if sensorXmlConfig.get("JMRIUserName") != None:
                self.userName.value = sensorXmlConfig.get("JMRIUserName")
            else:
                self.userName.value = ""
            self.nameKey.value = "Sens-" + self.jmriSensSystemName.candidateValue
            self.sensPort.value = sensorXmlConfig.get("Port")
            self.sensType.value = sensorXmlConfig.get("Type")
            if sensorXmlConfig.get("JMRIDescription") != None:
                self.description.value = sensorXmlConfig.get("JMRIDescription")
            else:
                self.description.value = ""
            if sensorXmlConfig.get("AdminState") != None:
                self.setAdmState(sensorXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Sensor, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of, or setting of configuration failed - initiated by configuration change of: " + sensorXmlConfig.get("JMRISystemName") + ", return code: " + trace.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        return rc.OK

    def updateReq(self):
        return self.parent.updateReq() #Just from the template - not applicable for this object leaf

    def validate(self):
        trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        if self.schemaDirty :
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")

            return rc.OK

    def checkSysName(self, sysName): #Just from the template - not applicable for this object leaf
        return self.parent.checkSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " received configuration commit0()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.value + " was reconfigured - applying the configuration")
            res = self.__setConfig()
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Sensor " + self.jmriSensSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.value + " was not reconfigured, skiping re-configuration")
        self.unSetOpStateDetail(OP_INIT)
        self.unSetOpStateDetail(OP_CONFIG)
        return rc.OK

    def abort(self):
        trace.notify(DEBUG_TERSE, "Sensor " + self.jmriSensSystemName.candidateValue + " received configuration abort()")
        self.abortAll()
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing sensor .xml configuration")
        sensorXml = ET.Element("Sensor")
        sysName = ET.SubElement(sensorXml, "JMRISystemName")
        sysName.text = self.jmriSensSystemName.value
        usrName = ET.SubElement(sensorXml, "JMRIUserName")
        sysName.text = self.userName.value
        descName = ET.SubElement(sensorXml, "JMRIDescription")
        descName.text = self.description.value
        type = ET.SubElement(sensorXml, "Type")
        type.text = self.sensType.value
        if not decoder:
            adminState = ET.SubElement(sensorXml, "AdminState")
            adminState.text = self.getAdmState()[STATE_STR]
        return minidom.parseString(ET.tostring(sensorXml)).toprettyxml(indent="   ") if text else sensorXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True, demo=False): #Just from the template - not applicable for this object leaf
        pass

    def delChild(self, child): #Just from the template - not applicable for this object leaf
        pass

    def view(self):
        self.dialog = UI_sensorDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_sensorDialog(self, edit=True)
        self.dialog.show()

    def add(self): #Just from the template - not applicable for this object leaf
        pass

    def delete(self, top=False):
        if self.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + self.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return self.canDelete()
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.parent.updateReq()
        return rc.OK

    def accepted(self):
        self.setOpStateDetail(OP_CONFIG)
        self.nameKey.value = "Sens-" + self.jmriSensSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save namkey as it may be gone after an abort from updateReq()
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

    def __validateConfig(self):
        res = self.parent.checkSysName(self.jmriSensSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.jmriSensSystemName.candidateValue + " already in use")
            return res
        return rc.OK

    def __setConfig(self):
        try:
            sensors = self.rpcClient.getConfigsByType(jmriObj.SENSORS)
            sensor = sensors[jmriObj.getObjTypeStr(jmriObj.SENSORS)][self.jmriSensSystemName.value]
            trace.notify(DEBUG_INFO, "System name " + self.jmriSensSystemName.value + " already configured in JMRI, re-using it")
        except:
            trace.notify(DEBUG_INFO, "System name " + self.jmriSensSystemName.value + " doesnt exist in JMRI, creating it")
            self.rpcClient.createObject(jmriObj.SENSORS, self.jmriSensSystemName.value)
        self.rpcClient.setUserNameBySysName(jmriObj.SENSORS, self.jmriSensSystemName.value, self.userName.value)
        self.rpcClient.setCommentBySysName(jmriObj.SENSORS, self.jmriSensSystemName.value, self.description.value)
        self.sensState = self.rpcClient.getStateBySysName(jmriObj.SENSORS, self.jmriSensSystemName.value)
        self.rpcClient.unRegEventCb(jmriObj.SENSORS, self.jmriSensSystemName.value, self.__senseChangeListener)
        self.rpcClient.unRegMqttSub(jmriObj.SENSORS, self.jmriSensSystemName.value)
        self.rpcClient.regEventCb(jmriObj.SENSORS, self.jmriSensSystemName.value, self.__senseChangeListener)
        self.rpcClient.regMqttSub(jmriObj.SENSORS, self.jmriSensSystemName.value, MQTT_SENS_TOPIC + MQTT_STATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriSensSystemName.value, {"*":"*"})
        self.sensOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SENS_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriSensSystemName.value
        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        return rc.OK

    def __senseChangeListener(self, event):
        trace.notify(DEBUG_VERBOSE, "Sensor  " + self.nameKey.value + " changed value from " + str(event.oldState) + " to " + str(event.newState))
        self.sensState = event.newState

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Sensor  " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()))
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.sensOpTopic, ON_LINE)
        elif self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_UNAVAIL):
            self.mqttClient.publish(self.sensOpTopic, OFF_LINE)
# End Sensors
#------------------------------------------------------------------------------------------------------------------------------------------------
