#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI actuator class providing the JMRI-genJMRI interactions for various actuators providing the bridge between following JMRI objects:
# TURNOUTS, LIGHTS and MEMORIES, and the genJMRI satelite actuator object atributes: SERVO, SOLENOID, PWM, ON/OFF, and Pulses.
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
# Class: actuator
# Purpose:      Provides the JMRI-genJMRI interactions for various actuators providing the bridge between following JMRI objects:
#               TURNOUTS, LIGHTS and MEMORIES, and the genJMRI satelite actuator object atributes: SERVO, SOLENOID, PWM, ON/OFF, and Pulses
#               Implements the management-, configuration-, supervision-, and control of genJMRI actuators.
#               See archictecture.md for more information
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class actuator(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.ACT_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.jmriActSystemName.value = name
        else:
            self.jmriActSystemName.value = "MT-NewTurnoutSysName"
        self.nameKey.value = "Turn-" + self.jmriActSystemName.candidateValue
        self.userName.value = "MT-NewTurnoutUsrName"
        self.actPort.value = 0
        self.actType.value = "TURNOUT"
        self.actSubType.value = "SOLENOID"
        self.actState = "CLOSED"
        self.description.value = "MS-NewActDescription"
        trace.notify(DEBUG_INFO,"New Actuator: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, ACTUATOR, displayIcon=ACTUATOR_ICON)
        self.commitAll()
        self.sensTopic = MQTT_JMRI_PRE_TOPIC + MQTT_SENS_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value

    def onXmlConfig(self, xmlConfig):
        try:
            actuatorXmlConfig = parse_xml(xmlConfig,
                                          {"JMRISystemName": MANSTR,
                                           "JMRIUserName": OPTSTR,
                                           "JMRIDescription": OPTSTR,
                                           "Type": MANSTR,
                                           "SubType":MANSTR,
                                           "Port": MANINT,
                                           "AdminState": OPTSTR
                                          }
                                         )
            if actuatorXmlConfig.get("JMRIUserName") != None:
                self.userName.value = actuatorXmlConfig.get("JMRIUserName")
            else:
                self.userName.value = ""
            self.actPort.value = actuatorXmlConfig.get("Port")
            self.actType.value = actuatorXmlConfig.get("Type")
            self.actSubType.value = actuatorXmlConfig.get("SubType")
            self.jmriActSystemName.value = actuatorXmlConfig.get("JMRISystemName")
            if self.actType.candidateValue == "TURNOUT":
                self.nameKey.value = "Turn-" + self.jmriActSystemName.candidateValue
            elif self.actType.candidateValue == "LIGHT":
                self.nameKey.value = "Light-" + self.jmriActSystemName.candidateValue
            elif self.actType.candidateValue == "MEMORY":
                self.nameKey.value = "Mem-" + self.jmriActSystemName.candidateValue
            if actuatorXmlConfig.get("JMRIDescription") != None:
                self.description.value = actuatorXmlConfig.get("JMRIDescription")
            else:
                self.description.value = ""
            if actuatorXmlConfig.get("AdminState") != None:
                self.setAdmState(actuatorXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Actuator, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of, or setting of configuration failed - initiated by configuration change of: " + actuatorXmlConfig.get("JMRISystemName") + ", return code: " + trace.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        return rc.OK

    def updateReq(self):
        return self.parent.updateReq() #Just from the template - not applicable for this object leaf

    def validate(self):
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def checkSysName(self, sysName): #Just from the template - not applicable for this object leaf
        return self.parent.checkSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " received configuration commit0()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " was not reconfigured, skiping config commitment")
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Actuator " + self.jmriActSystemName.value)
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Actuator " + self.jmriActSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.value + " was not reconfigured, skiping re-configuration")
        self.unSetOpStateDetail(OP_INIT)
        self.unSetOpStateDetail(OP_CONFIG)
        return rc.OK

    def abort(self):
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " received configuration abort()")
        self.abortAll()
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing actuator .xml configuration")
        actuatorXml = ET.Element("Actuator")
        sysName = ET.SubElement(actuatorXml, "JMRISystemName")
        sysName.text = self.jmriActSystemName.value
        usrName = ET.SubElement(actuatorXml, "JMRIUserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(actuatorXml, "JMRIDescription")
        descName.text = self.description.value
        type = ET.SubElement(actuatorXml, "Type")
        type.text = self.actType.value
        subType = ET.SubElement(actuatorXml, "SubType")
        subType.text = self.actSubType.value
        port = ET.SubElement(actuatorXml, "Port")
        port.text = str(self.actPort.value)
        if not decoder:
            adminState = ET.SubElement(actuatorXml, "AdminState")
            adminState.text = self.getAdmState()[STATE_STR]
        return minidom.parseString(ET.tostring(actuatorXml)).toprettyxml(indent="   ") if text else actuatorXml

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
        self.dialog = UI_actuatorDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_actuatorDialog(self, edit=True)
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
        if self.actType.candidateValue == "TURNOUT":
            self.nameKey.value = "Turn-" + self.jmriActSystemName.candidateValue
        elif self.actType.candidateValue == "LIGHT":
            self.nameKey.value = "Light-" + self.jmriActSystemName.candidateValue
        elif self.actType.candidateValue == "MEMORY":
            self.nameKey.value = "Mem-" + self.jmriActSystemName.candidateValue
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

    def __validateConfig(self):
        res = self.parent.checkSysName(self.jmriActSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.jmriActSystemName.candidateValue + " already in use")
            return res
        return rc.OK

    def __setConfig(self):
        try:
            if self.actType.value == "TURNOUT":
                actuators = self.rpcClient.getConfigsByType(jmriObj.TURNOUTS)
                actuator = actuators[jmriObj.getObjTypeStr(jmriObj.TURNOUTS)][self.jmriActSystemName.value]
                self.rpcClient.setUserNameBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.description.value)
                self.actState = self.rpcClient.getStateBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value)
            elif self.actType.value == "LIGHT":
                actuators = self.rpcClient.getConfigsByType(jmriObj.LIGHTS)
                actuator = actuators[jmriObj.getObjTypeStr(jmriObj.LIGHTS)][self.jmriActSystemName.value]
                self.rpcClient.setUserNameBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.description.value)
                self.actState = self.rpcClient.getStateBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value)
            elif self.actType.value == "MEMORY":
                actuators = self.rpcClient.getConfigsByType(jmriObj.MEMORIES)
                actuator = actuators[jmriObj.getObjTypeStr(jmriObj.MEMORIES)][self.jmriActSystemName.value]
                self.rpcClient.setUserNameBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.description.value)
                self.actState = self.rpcClient.getStateBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value)
            trace.notify(DEBUG_INFO, "System name " + self.jmriActSystemName.value + " already configured in JMRI, re-using it")
        except:
            trace.notify(DEBUG_INFO, "System name " + self.jmriActSystemName.value + " doesnt exist in JMRI, creating it")
            if self.actType.value == "TURNOUT":
                self.rpcClient.createObject(jmriObj.TURNOUTS, self.jmriActSystemName.value)
                self.rpcClient.setUserNameBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.description.value)
            elif self.actType.value == "LIGHT":
                self.rpcClient.createObject(jmriObj.LIGHTS, self.jmriActSystemName.value)
                self.rpcClient.setUserNameBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.description.value)
            elif self.actType.value == "MEMORY":
                self.rpcClient.createObject(jmriObj.MEMORIES, self.jmriActSystemName.value)
                self.rpcClient.setUserNameBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.userName.value)
                self.rpcClient.setCommentBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.description.value)
        #SORT IN UNDER EACH CATEGORY BELOW
        self.rpcClient.unRegEventCb(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.__actChangeListener)
        self.rpcClient.unRegEventCb(jmriObj.LIGHTS, self.jmriActSystemName.value, self.__actChangeListener)
        self.rpcClient.unRegEventCb(jmriObj.MEMORIES, self.jmriActSystemName.value, self.__actChangeListener)
        self.rpcClient.unRegMqttPub(jmriObj.TURNOUTS, self.jmriActSystemName.value)
        self.rpcClient.unRegMqttPub(jmriObj.LIGHTS, self.jmriActSystemName.value)
        self.rpcClient.unRegMqttPub(jmriObj.MEMORIES, self.jmriActSystemName.value)
        if self.actType.value == "TURNOUT":
            self.rpcClient.setUserNameBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.TURNOUTS, self.jmriActSystemName.value, MQTT_TURNOUT_TOPIC + MQTT_STATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
            self.actOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_TURNOUT_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
            self.actAdmTopic = MQTT_JMRI_PRE_TOPIC + MQTT_TURNOUT_TOPIC + MQTT_ADMSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value

        elif self.actType.value == "LIGHT":
            self.rpcClient.setUserNameBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.LIGHTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.LIGHTS, self.jmriActSystemName.value, MQTT_LIGHT_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
            self.actOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_LIGHT_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
            self.actAdmTopic = MQTT_JMRI_PRE_TOPIC + MQTT_LIGHT_TOPIC + MQTT_ADMSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value

        elif self.actType.value == "MEMORY":
            self.rpcClient.setUserNameBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.MEMORIES, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.MEMORIES, self.jmriActSystemName.value, MQTT_MEMORY_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
            self.actOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_MEMORY_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
            self.actAdmTopic = MQTT_JMRI_PRE_TOPIC + MQTT_MEMORY_TOPIC + MQTT_ADMSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value

        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        return rc.OK

    def __actChangeListener(self, event):
        trace.notify(DEBUG_VERBOSE, "Actuator  " + self.nameKey.value + " changed value from " + str(event.oldState) + " to " + str(event.newState))
        self.actState = event.newState

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Actuator  " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()))
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.actOpTopic, OP_AVAIL_PAYLOAD)
        else:
            self.mqttClient.publish(self.actOpTopic, OP_UNAVAIL_PAYLOAD)
        if self.getAdmState() == ADM_ENABLE:
            self.mqttClient.publish(self.actAdmTopic, ADM_ON_LINE_PAYLOAD)
        else:
            self.mqttClient.publish(self.actAdmTopic, ADM_OFF_LINE_PAYLOAD)
# End Actuators
#------------------------------------------------------------------------------------------------------------------------------------------------
