#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A myRailIO actuator class providing the JMRI-myRailIO interactions for various actuators providing the bridge between following JMRI objects:
# TURNOUTS, LIGHTS and MEMORIES, and the myRailIO satellite actuator object atributes: SERVO, SOLENOID, PWM, ON/OFF, and Pulses.
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
imp.load_source('rc', '..\\rc\\myRailIORc.py')
from rc import rc
imp.load_source('schema', '..\\schema\\schema.py')
from schema import *
imp.load_source('jmriObj', '..\\rpc\\JMRIObjects.py')
from jmriObj import *
imp.load_source('jmriRpcClient', '..\\rpc\\myRailIORpcClient.py')
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
# Purpose:      Provides the JMRI-myRailIO interactions for various actuators providing the bridge between following JMRI objects:
#               TURNOUTS, LIGHTS and MEMORIES, and the myRailIO satellite actuator object atributes: SERVO, SOLENOID, PWM, ON/OFF, and Pulses
#               Implements the management-, configuration-, supervision-, and control of myRailIO actuators.
#               See archictecture.md for more information
# StdMethods:   The standard myRailIO Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), regSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class actuator(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name = None, demo = False):
        self.win = win
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.provisioned = False
        self.sysNameReged = False
        self.schemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.ACT_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.updating = False
        self.pendingBoot = False
        if name:
            self.jmriActSystemName.value = name
        else:
            self.jmriActSystemName.value = "MT-MyNewTurnoutSysName"
        self.nameKey.value = "Turn-" + self.jmriActSystemName.candidateValue
        self.userName.value = "MyNewTurnoutUsrName"
        self.description.value = "MyNewTurnoutDescription"
        self.actPort.value = 0
        self.actType.value = "TURNOUT"
        self.actSubType.value = "SOLENOID"
        self.contigiousPorts = 1
        self.commitAll()
        self.actState = "CLOSED"
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, ACTUATOR, displayIcon=ACTUATOR_ICON)
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Actuator reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Actuator has not received a valid configuration")
        self.INT_FAILalarm = alarm(self, "INTERNAL FAILURE", self.nameKey.value, ALARM_CRITICALITY_A, "Actuator has experienced an internal error")
        self.CBLalarm = alarm(self, "CONTROL-BLOCK STATUS", self.nameKey.value, ALARM_CRITICALITY_C, "Parent object blocked resulting in a control-block of this object")
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.win.inactivateMoMObj(self.item)
        self.setOpStateDetail(OP_INIT[STATE] | OP_UNCONFIGURED[STATE])
        trace.notify(DEBUG_INFO,"New Actuator: " + self.nameKey.candidateValue + " created - awaiting configuration")

    @staticmethod
    def aboutToDelete(ref):
        ref.parent.actTopology.removeTopologyMember(ref.jmriActSystemName.value)

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
            if actuatorXmlConfig.get("SubType") == "SOLENOID":
                self.contigiousPorts = 2
            else:
                self.contigiousPorts = 1
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
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of, or setting of configuration failed - initiated by configuration change of: " + actuatorXmlConfig.get("JMRISystemName") + ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
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
        print(">>>>>>>>>>>>>>>>>>>>Actuator validate")
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def regSysName(self, sysName): #Just from the template - not applicable for this object leaf
        return self.parent.regSysName(sysName)

    def unRegSysName(self, sysName): #Just from the template - not applicable for this object leaf
        return self.parent.regSysName(sysName)

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
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Actuator " + self.jmriActSystemName.value)
                return res
        else:
            trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.value + " was not reconfigured, skiping re-configuration")
        self.provisioned = True
        return rc.OK

    def abort(self):
        print(">>>>>>>>>>>>>>>>>>>>abort")
        trace.notify(DEBUG_TERSE, "Actuator " + self.jmriActSystemName.candidateValue + " received configuration abort()")
        self.abortAll()
        if not self.provisioned:
            print(">>>>>>>>>>>>>>>>>>>>removing myself")
            self.delete(top = True)
        # WEE NEED TO CHECK IF THE ABORT WAS DUE TO THE CREATION OF THIS OBJECT AND IF SO DELETE OUR SELVES (self.delete)
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
        adminState = ET.SubElement(actuatorXml, "AdminState")
        adminState.text = self.getAdmState()[STATE_STR]
        return minidom.parseString(ET.tostring(actuatorXml)).toprettyxml(indent="   ") if text else actuatorXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_EDIT & ~METHOD_DELETE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True, demo=False): #Just from the template - not applicable for this object leaf
        pass

    def delChild(self, child): #Just from the template - not applicable for this object leaf
        pass

    def view(self):
        self.dialog = UI_actuatorDialog(self, self.rpcClient, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_actuatorDialog(self, self.rpcClient, edit=True)
        self.dialog.show()

    def add(self): #Just from the template - not applicable for this object leaf
        pass

    def delete(self, top=False):
        if self.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + self.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return self.canDelete()
        self.NOT_CONNECTEDalarm.ceaseAlarm("Source object deleted")
        self.NOT_CONFIGUREDalarm.ceaseAlarm("Source object deleted")
        self.INT_FAILalarm.ceaseAlarm("Source object deleted")
        self.CBLalarm.ceaseAlarm("Source object deleted")
        self.parent.actTopology.removeTopologyMember(self.jmriActSystemName.value)
        self.parent.unRegSysName(self.jmriActSystemName.value)
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq(self, self, uploadNReboot = True)
        return rc.OK

    def accepted(self):
        if self.actType.candidateValue == "TURNOUT":
            self.nameKey.value = "Turn-" + self.jmriActSystemName.candidateValue
        elif self.actType.candidateValue == "LIGHT":
            self.nameKey.value = "Light-" + self.jmriActSystemName.candidateValue
        elif self.actType.candidateValue == "MEMORY":
            self.nameKey.value = "Mem-" + self.jmriActSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        if self.actSubType.candidateValue == "SOLENOID":
            self.contigiousPorts = 2
        else:
            self.contigiousPorts = 1
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
        return self.parent.getTopology() + "/" + self.jmriActSystemName.value

    def __validateConfig(self):
        if not self.sysNameReged:
            res = self.parent.regSysName(self.jmriActSystemName.candidateValue)
            if res != rc.OK:
                trace.notify(DEBUG_ERROR, "System name " + self.jmriActSystemName.candidateValue + " already in use")
                return res
        self.sysNameReged = True

        #!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        #HERE IS WHERE WE NEED TO ADD THE TOPOLOGY VALIDATION OF THE CONFIGURATION
        # We need to define additional subsequent ports needed for this actuator type: self.contigousPorts
        self.parent.actTopology.removeTopologyMember(self.jmriActSystemName.value)
        print(">>>>>>>>>>>>>>>>> About to allocate ports in topology")
        weakSelf = weakref.ref(self, actuator.aboutToDelete)
        for portItter in range (0, self.contigiousPorts):
            print(">>>>>>>>>>>>>>>>> Allocating port: " + str(self.actPort.candidateValue + portItter))
            res = self.parent.actTopology.addTopologyMember(self.jmriActSystemName.candidateValue, self.actPort.candidateValue + portItter, weakSelf)
            if res:
                print(">>>>>>>>>>>>>>>>Add failed")
                trace.notify(DEBUG_ERROR, "Actuator failed address/port topology validation for port/address: " + str(self.actPort.candidateValue + portItter) + rc.getErrStr(res))
                return res
            print(">>>>>>>>>>>>>>>>Add Succeeded")

        return rc.OK

    def __setConfig(self):
        trace.notify(DEBUG_INFO, "Creating Actuator - System name " + self.jmriActSystemName.value)
        if self.actType.value == "TURNOUT":
            self.rpcClient.unRegEventCb(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.unRegMqttPub(jmriObj.TURNOUTS, self.jmriActSystemName.value)
            self.rpcClient.createObject(jmriObj.TURNOUTS, self.jmriActSystemName.value)
            self.rpcClient.setUserNameBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.TURNOUTS, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.TURNOUTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.TURNOUTS, self.jmriActSystemName.value, MQTT_TURNOUT_TOPIC + MQTT_STATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
        elif self.actType.value == "LIGHT":
            self.rpcClient.unRegEventCb(jmriObj.LIGHTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.unRegMqttPub(jmriObj.LIGHTS, self.jmriActSystemName.value)
            self.rpcClient.createObject(jmriObj.LIGHTS, self.jmriActSystemName.value)
            self.rpcClient.setUserNameBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.LIGHTS, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.LIGHTS, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.LIGHTS, self.jmriActSystemName.value, MQTT_LIGHT_TOPIC + MQTT_STATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
        elif self.actType.value == "MEMORY":
            self.rpcClient.unRegEventCb(jmriObj.MEMORIES, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.unRegMqttPub(jmriObj.MEMORIES, self.jmriActSystemName.value)
            self.rpcClient.createObject(jmriObj.MEMORIES, self.jmriActSystemName.value)
            self.rpcClient.setUserNameBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.userName.value)
            self.rpcClient.setCommentBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value, self.description.value)
            self.actState = self.rpcClient.getStateBySysName(jmriObj.MEMORIES, self.jmriActSystemName.value)
            self.rpcClient.regEventCb(jmriObj.MEMORIES, self.jmriActSystemName.value, self.__actChangeListener)
            self.rpcClient.regMqttPub(jmriObj.MEMORIES, self.jmriActSystemName.value, MQTT_MEMORY_TOPIC + MQTT_STATE_TOPIC+ self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value, {"*":"*"})
        else:
            trace.notify(DEBUG_INFO, "Could not create Actuator type " + self.actType.value + " for " + self.nameKey.value +" , type not supported")
            return rc.PARAM_ERR
        self.actOpDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_ACT_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
        self.actOpUpStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_ACT_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
        self.actAdmDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_ACT_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.jmriActSystemName.value
        self.unRegOpStateCb(self.__sysStateRespondListener)
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateRespondListener, OP_DISABLED[STATE] | OP_SERVUNAVAILABLE[STATE])
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.mqttClient.subscribeTopic(self.actOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
        return rc.OK

    def __actChangeListener(self, event):
        trace.notify(DEBUG_VERBOSE, "Actuator  " + self.nameKey.value + " changed value from " + str(event.oldState) + " to " + str(event.newState))
        self.actState = event.newState

    def __sysStateRespondListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, "Sensor " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.actAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
            else:
                self.mqttClient.publish(self.actAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)

    def __sysStateAllListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
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
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.admDisableAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
            if self.pendingBoot:
                self.updateReq(self, self, uploadNReboot = True)
                self.pendingBoot = False
            else:
                self.updateReq(self, self, uploadNReboot = False)
        if (changedOpStateDetail & OP_INIT[STATE]) and (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.raiseAlarm("Actuator has not connected, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INIT[STATE]) and not (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.ceaseAlarm("Actuator has now successfully connected")
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.raiseAlarm("Actuator has not been configured, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker, or the MAC address may not be correctly provisioned", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and not (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.ceaseAlarm("Actuator is now successfully configured")
        if (changedOpStateDetail & OP_INTFAIL[STATE]) and (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.raiseAlarm("Actuator is experiencing an internal error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INTFAIL[STATE]) and not (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.ceaseAlarm("Actuator is no longer experiencing any internal errors")
        if (changedOpStateDetail & OP_CBL[STATE]) and (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.raiseAlarm("Parent object for which this object is depending on has failed", p_sysStateTransactionId, False)
        elif (changedOpStateDetail & OP_CBL[STATE]) and not (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.ceaseAlarm("Parent object for which this object is depending on is now working")

    def __onDecoderOpStateChange(self, topic, value):
        trace.notify(DEBUG_INFO, "Actuator " + self.nameKey.value + " received a new OP State from client: " + value + " setting server OP-state accordingly")
        self.setOpStateDetail(self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        self.unSetOpStateDetail(~self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])

# End Actuators
#------------------------------------------------------------------------------------------------------------------------------------------------
