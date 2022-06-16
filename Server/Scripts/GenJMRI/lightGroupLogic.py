 #!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI lightgroup class providing the JMRI-genJMRI interactions for lights and light signals, currently only signal masts are supported.
# But other lightgroups will be added such as sequential road block, lights, television flicker, fluorecent flicker, etc. 
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
# Class: lightGroup
# Purpose:      Provides the JMRI-genJMRI interactions for various lightGroups providing the bridge between following JMRI objects:
#               MASTS, and the genJMRI lightGroup concepts, additional mappings will be added
#               Implements the management-, configuration-, supervision-, and control of genJMRI lightgroups.
#               See archictecture.md for more information
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class lightGroup(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False

        systemState.__init__(self)
        schema.__init__(self)

        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.LG_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)

        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.jmriLgSystemName.value = name
        else:
            self.jmriLgSystemName.value = "IF$vsm:NewLightGroupSysName($001)"
        self.nameKey.value = "LightGroup-" + self.jmriLgSystemName.candidateValue
        self.userName.value = "GJLG-NewLightGroupUsrName"
        self.description.value = "GJLG-NewLightGroupUsrDescription"
        self.lgLinkAddr.value = 0
        self.lgType.value = "SIGNAL MAST"
        self.lgProperty1.value = ""
        self.lgProperty2.value = ""
        self.lgProperty3.value = ""
        trace.notify(DEBUG_INFO,"New Light group: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, LIGHT_GROUP, displayIcon=TRAFFICLIGHT_ICON)
        self.lgShowing = "UNKOWN" #NEED TO FIX BUSINESS LOGIC
        self.commitAll()

    def onXmlConfig(self, xmlConfig):
        try:
            lgXmlConfig = parse_xml(xmlConfig,
                                        {"JMRISystemName": MANSTR,
                                         "JMRIUserName": OPTSTR,
                                         "JMRIDescription": OPTSTR,
                                         "Type": MANSTR,
                                         "LinkAddress": MANINT,
                                         "Property1": OPTSTR,
                                         "Property2": OPTSTR,
                                         "Property3": OPTSTR,
                                         "AdminState": OPTSTR
                                        }
                                )
            self.jmriLgSystemName.value = lgXmlConfig.get("JMRISystemName")
            self.nameKey.value = "LightGroup-" + self.jmriLgSystemName.candidateValue
            if lgXmlConfig.get("JMRIUserName") != None:
                self.userName.value = lgXmlConfig.get("JMRIUserName")
            else:
                self.userName.value = ""
            if lgXmlConfig.get("JMRIDescription") != None:
                self.description.value = lgXmlConfig.get("JMRIDescription")
            else:
                self.description.value = ""
            self.lgType.value = lgXmlConfig.get("Type")
            self.lgLinkAddr.value = int(lgXmlConfig.get("LinkAddress"))
            if lgXmlConfig.get("Property1") != None:
                self.lgProperty1.value = lgXmlConfig.get("Property1")
            else:
                self.lgProperty1.value = ""
            if lgXmlConfig.get("Property2") != None:
                self.lgProperty2.value = lgXmlConfig.get("Property2")
            else:
                self.lgProperty2.value = ""
            if lgXmlConfig.get("Property3") != None:
                self.lgProperty3.value = lgXmlConfig.get("Property3")
            else:
                self.lgProperty3.value = ""
            if lgXmlConfig.get("AdminState") != None:
                self.setAdmState(lgXmlConfig.get("AdminState"))
            else:
                trace.notify(DEBUG_INFO, "\"AdminState\" not set for " + self.nameKey.candidateValue + " - disabling it")
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Light group, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of, or setting of configuration failed - initiated by configuration change of: " + lgXmlConfig.get("JMRISystemName") + ", return code: " + trace.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + "Successfully configured")
        return rc.OK

    def updateReq(self):
        return self.parent.updateReq()

    def validate(self):
        trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def checkSysName(self, sysName): #Just from the template - not applicable for this object leaf
        return self.parent.checkSysName(sysName)



    def commit0(self):
        trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " received configuration commit0()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
        else:
            trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " was not reconfigured, skiping config commitment")
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            try:
                trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Actuator " + self.jmriLgSystemName.value)
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Actuator " + self.jmriLgSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.value + " was not reconfigured, skiping re-configuration")
        self.unSetOpStateDetail(OP_INIT)
        self.unSetOpStateDetail(OP_CONFIG)
        return rc.OK

    def abort(self):
        trace.notify(DEBUG_TERSE, "Light group " + self.jmriLgSystemName.candidateValue + " received configuration abort()")
        self.abortAll()
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        lgXml = ET.Element("LightGroup")
        sysName = ET.SubElement(lgXml, "JMRISystemName")
        sysName.text = self.jmriLgSystemName.value
        usrName = ET.SubElement(lgXml, "JMRIUserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(lgXml, "JMRIDescription")
        descName.text = self.description.value
        type = ET.SubElement(lgXml, "Type")
        type.text = self.lgType.value
        linkAddr = ET.SubElement(lgXml, "LinkAddress")
        linkAddr.text = str(self.lgLinkAddr.value)
        property1 = ET.SubElement(lgXml, "Property1")
        property1.text = self.lgProperty1.value
        property2 = ET.SubElement(lgXml, "Property2")
        property2.text = self.lgProperty2.value
        property3 = ET.SubElement(lgXml, "Property3")
        property3.text = self.lgProperty3.value
        if not decoder:
            adminState = ET.SubElement(lgXml, "AdminState")
            adminState.text = self.getAdmState()[STATE_STR]
        return minidom.parseString(ET.tostring(lgXml)).toprettyxml(indent="   ") if text else lgXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_EDIT | METHOD_DELETE |  METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG
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
        self.dialog = UI_lightGroupDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_lightGroupDialog(self, edit=True)
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
        self.nameKey.value = "LightGroup-" + self.jmriLgSystemName.candidateValue
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
        res = self.parent.checkSysName(self.jmriLgSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.jmriLgSystemName.candidateValue + " already in use")
            return res
        return rc.OK

    def __setConfig(self):
        self.lgOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_LG_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.jmriLgSystemName.value
        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        return rc.OK

    def __lgChangeListener(self, event):
        pass

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Light group  " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()))
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.lgOpTopic, ON_LINE)
        elif self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_UNAVAIL):
            self.mqttClient.publish(self.lgOpTopic, OFF_LINE)
# End Lightgroups
#------------------------------------------------------------------------------------------------------------------------------------------------
