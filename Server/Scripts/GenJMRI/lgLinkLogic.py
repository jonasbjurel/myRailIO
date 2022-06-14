#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI lightgroupLink class providing the genJMRI lightGroup link management- and supervision. genJMRI provides the concept of lightgroup
# links for daisy-chaining of lightgroups such as signal masts, etc
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
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
from lightGroupLogic import *
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
# Class: satLink
# Purpose:      Provides management- and supervision capabilities of genJMRI satelite links. Implements the management-, configuration-,
#               supervision-, and control of genJMRI satelite links - interconnecting satelites in daisy-chains.
#               See archictecture.md for more information
# StdMethods:   The standard genJMRI Managed Object Model API methods are all described in archictecture.md including: __init__(), onXmlConfig(),
#               updateReq(), validate(), checkSysName(), commit0(), commit1(), abort(), getXmlConfigTree(), getActivMethods(), addChild(), delChild(),
#               view(), edit(), add(), delete(), accepted(), rejected()
# SpecMethods:  No class specific methods
#################################################################################################################################################
class lgLink(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.LG_LINK_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.lightGroups.value = []
        self.childs.value = self.lightGroups.candidateValue
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT)
        if name:
            self.lgLinkSystemName.value = name
        else:
            self.lgLinkSystemName.value = "GJLL-NewLgLinkSysName"
        self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
        self.userName.value = "GJLL-NewLgLinkUsrName"
        self.lgLinkNo.value = 0
        self.description.value = "New Light group link"
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, LIGHT_GROUP_LINK, displayIcon=LINK_ICON)
        trace.notify(DEBUG_INFO,"New Light group link: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.commitAll()
        if self.demo:
            for i in range(8):
                self.addChild(LIGHT_GROUP, name="GJLG-"+str(i), config=False, demo=True)

    def onXmlConfig(self, xmlConfig):
        try:
            lgLinkXmlConfig = parse_xml(xmlConfig,
                                            {"SystemName": MANSTR,
                                             "UserName": OPTSTR,
                                             "Link": MANINT,
                                             "Description": OPTSTR,
                                             "AdminState":OPTSTR
                                             }
                                        )

            self.lgLinkSystemName.value = lgLinkXmlConfig.get("SystemName")
            self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
            if lgLinkXmlConfig.get("UserName") != None:
                self.userName.value = lgLinkXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            self.lgLinkNo.value = int(lgLinkXmlConfig.get("Link"))
            if lgLinkXmlConfig.get("Description") != None:
                self.description.value = lgLinkXmlConfig.get("Description")
            else:
                lgLinkXmlConfig.get("Description")
            if lgLinkXmlConfig.get("AdminState") != None:
                self.setAdmState(lgLinkXmlConfig.get("AdminState"))
            else:
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Light group link, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + lgLinkXmlConfig.get("SystemName") + ", return code: " + rc.getErrStr(res))
            return res
        else:
            trace.notify(DEBUG_INFO, self.nameKey.value + " Successfully configured")
        for lightGroupXml in xmlConfig:
            if lightGroupXml.tag == "LightGroup":
                res = self.addChild(LIGHT_GROUP, config=False, configXml=lightGroupXml, demo=False)
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to add Light group to " + lgLinkXmlConfig.get("SystemName") + " - return code: " + rc.getErrStr(res))
                    return res
        return rc.OK

    def updateReq(self):
        return self.parent.updateReq()

    def validate(self):
        trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " received configuration validate()")
        self.schemaDirty = self.isDirty()
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " - No childs to validate")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.validate()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " - configurations has been changed - validating them")
            return self.__validateConfig()
        else:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " - configuration has NOT been changed - skipping validation")
            return rc.OK

    def checkSysName(self, sysName):
        return self.parent.checkSysName(sysName)

    def commit0(self):
        trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " received configuration commit0()")
        childs = True
        try:
            self.childs.candidateValue
        except:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " - No childs to commit(0)")
            childs = False
        if childs:
            for child in self.childs.candidateValue:
                res = child.commit0()
                if res != rc.OK:
                    return res
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " was reconfigured, commiting configuration")
            self.commitAll()
            self.win.reSetMoMObjStr(self.item, self.nameKey.value)
            return rc.OK
        else:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " was not reconfigured, skiping config commitment")
            return rc.OK
        return rc.OK

    def commit1(self):
        trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.value + " received configuration commit1()")
        if self.schemaDirty:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.value + " was reconfigured - applying the configuration")
            res = self.__setConfig()
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Light group link " + self.lgLinkSystemName.value)
                return rc.GEN_ERR
        else:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.value + " was not reconfigured, skiping re-configuration")
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
        trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.candidateValue + " received configuration abort()")
        childs = True
        try:
            self.childs.value
        except:
            childs = False
        if childs:
            for child in childs.value:
                child.abort()
        self.abortAll()
        self.unSetOpStateDetail(OP_CONFIG)
        if self.getOpStateDetail() & OP_INIT[STATE]:
            self.delete()
        return rc.OK

    def getXmlConfigTree(self, decoder=False, text=False, includeChilds=True):
        trace.notify(DEBUG_TERSE, "Providing Light group link .xml configuration")
        lgLinkXml = ET.Element("LightgroupsLink")
        sysName = ET.SubElement(lgLinkXml, "SystemName")
        sysName.text = self.lgLinkSystemName.value
        usrName = ET.SubElement(lgLinkXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(lgLinkXml, "Description")
        descName.text = self.description.value
        satLink = ET.SubElement(lgLinkXml, "Link")
        satLink.text = self.lgLinkNo.value
        if not decoder:
            adminState = ET.SubElement(satLinkXml, "AdminState")
            adminState.text = self.getAdmState()[STATE_STR]
        elif decoder:
            ################# PROVIDE MASTS DEFINITION ###############
            pass
        if includeChilds:
            childs = True
            try:
                self.childs.value
            except:
                childs = False
            if childs:
                for child in self.childs.value:
                    lgLinkXml.append(chlild.getXmlConfigTree())
        return minidom.parseString(ET.tostring(lgLinkXml)).toprettyxml(indent="   ") if text else lgLinkXml

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name=None, config=True, configXml=None, demo=False):
        if resourceType == LIGHT_GROUP:
            self.lightGroups.append(lightGroup(self.win, self.item, self.rpcClient, self.mqttClient, name=name, demo=self.demo))
            self.childs.value = self.lightGroups.candidateValue
            trace.notify(DEBUG_INFO, "Light group: " + self.lightGroups.candidateValue[-1].nameKey.candidateValue + "is being added to light group link " + self.nameKey.value)
            if not config and configXml:
                nameKey = self.lightGroups.candidateValue[-1].nameKey.candidateValue
                res = self.lightGroups.candidateValue[-1].onXmlConfig(configXml)
                self.reEvalOpState()
                if res != rc.OK:
                    trace.notify(DEBUG_ERROR, "Failed to configure light group: " + nameKey + " - return code: " + rc.getErrStr(res))
                    return res
                trace.notify(DEBUG_INFO, "Light group: " + self.lightGroups.value[-1].nameKey.value + " successfully added to light group link " + self.nameKey.value)
                return rc.OK
            if config:
                self.dialog = UI_lightGroupDialog(self.lightGroups.candidateValue[-1], edit=True)
                self.dialog.show()
                trace.notify(DEBUG_INFO, "Light group: " + self.lightGroups.value[-1].nameKey.value + " successfully added to light group link " + self.nameKey.value)
                self.reEvalOpState()
                return rc.OK
            trace.notify(DEBUG_ERROR, "Light group link could not handele \"addChild\" permutation of \"config\" : " + str(config) + ", \"configXml\: " + ("Provided" if configXml else "Not provided") + " \"demo\": " + str(demo))
            return rc.GEN_ERR
        else:
            trace.notify(DEBUG_ERROR, "Light group link only takes LIGHT_GROUP as child, given child was: " + str(resourceType))
            return rc.GEN_ERR

    def delChild(self, child):
        if child.canDelete() != rc.OK:
            trace.notify(DEBUG_INFO, "Could not delete " + child.nameKey.candidateValue + " - as the object or its childs are not in DISABLE state")
            return child.canDelete()
        try:
            self.lightGroups.remove(child)
        except:
            pass
        self.childs.value = self.lightGroups.candidateValue
        return rc.OK

    def view(self):
        self.dialog = UI_lightgroupsLinkDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_lightgroupsLinkDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, LIGHT_GROUP)
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
        self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        res = self.parent.updateReq()
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

    def __validateConfig(self):
        res = self.parent.checkSysName(self.lgLinkSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.lgLinkSystemName.candidateValue + " already in use")
            return res
        if len(self.lightGroups.candidateValue) > LG_LINK_MAX_LIGHTGROUPS:
            trace.notify(DEBUG_ERROR, "Too many Light groups defined for light group link " + str(self.lgLinkSystemName.candidateValue) + ", " + str(len(self.lightGroups.candidateValue)) + "  given, " + str(LG_LINK_MAX_LIGHTGROUPS) + " is maximum")
            return rc.RANGE_ERR
        lgAddrs = []
        for lg in self.childs.candidateValue:
            try:
                lgAddrs.index(lg.lgLinkAddr.candidateValue)
                trace.notify(DEBUG_ERROR, "Light group address" + str(lg.lgLinkAddr.candidateValue) + " already in use for lightgroup link " + self.nameKey.candidateValue)
                return rc.ALREADY_EXISTS
            except:
                pass
        return rc.OK

    def __setConfig(self):
        self.lgLinkOpTopic = MQTT_JMRI_PRE_TOPIC + MQTT_LGLINK_TOPIC + MQTT_OPSTATE_TOPIC + self.parent.getDecoderUri() + "/" + self.lgLinkSystemName.value
        self.unRegOpStateCb(self.__sysStateListener)
        self.regOpStateCb(self.__sysStateListener)
        return rc.OK

    def __sysStateListener(self):
        trace.notify(DEBUG_INFO, "Light group link " + self.nameKey.value + " got a new OP State: " + self.getOpStateSummaryStr(self.getOpStateSummary()))
        if self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_AVAIL):
            self.mqttClient.publish(self.lgLinkOpTopic, ON_LINE)
        elif self.getOpStateSummaryStr(self.getOpStateSummary()) == self.getOpStateSummaryStr(OP_SUMMARY_UNAVAIL):
            self.mqttClient.publish(self.lgLinkOpTopic, OFF_LINE)
