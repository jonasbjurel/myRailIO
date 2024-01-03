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
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.demo = demo
        self.schemaDirty = False
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.LG_LINK_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.lightGroups.value = []
        self.childs.value = self.lightGroups.candidateValue
        self.updated = True
        if name:
            self.lgLinkSystemName.value = name
        else:
            self.lgLinkSystemName.value = "GJLL-NewLgLinkSysName"
        self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
        self.userName.value = "GJLL-NewLgLinkUsrName"
        self.description.value = "New Light group link"
        self.lgLinkNo.value = 0
        self.mastDefinitionPath.value = "C:\\Program Files (x86)\\JMRI\\xml\\signals\\Sweden-3HMS"
        self.commitAll()
        self.mastTypes = []
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, LIGHT_GROUP_LINK, displayIcon=LINK_ICON)
        self.NOT_CONNECTEDalarm = alarm(self, "CONNECTION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Light-group link reported disconnected")
        self.NOT_CONFIGUREDalarm = alarm(self, "CONFIGURATION STATUS", self.nameKey.value, ALARM_CRITICALITY_A, "Light-group link has not received a valid configuration")
        self.LINK_SCAN_OVERLOADalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Light-group link scanning overloaded") #NEEDS TO BE IMPLEMENTED
        self.LINK_FLASH_OVERLOADalarm = alarm(self, "PERFORMANCE WARNING", self.nameKey.value, ALARM_CRITICALITY_B, "Flash load overloaded") #NEEDS TO BE IMPLEMENTED
        self.INT_FAILalarm = alarm(self, "INTERNAL FAILURE", self.nameKey.value, ALARM_CRITICALITY_A, "Light-group link has experienced an internal error""Light-group link has experienced an internal error")
        self.CBLalarm = alarm(self, "CONTROL-BLOCK STATUS", self.nameKey.value, ALARM_CRITICALITY_C, "Parent object blocked resulting in a control-block of this object")
        systemState.__init__(self)
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.win.inactivateMoMObj(self.item)
        self.setOpStateDetail(OP_INIT[STATE] | OP_UNCONFIGURED[STATE])
        if self.demo:
            for i in range(8):
                self.addChild(LIGHT_GROUP, name="GJLG-"+str(i), config=False, demo=True)
        trace.notify(DEBUG_INFO,"New Light group link: " + self.nameKey.candidateValue + " created - awaiting configuration")

    def onXmlConfig(self, xmlConfig):
        try:
            lgLinkXmlConfig = parse_xml(xmlConfig,
                                            {"SystemName": MANSTR,
                                             "UserName": OPTSTR,
                                             "Link": MANINT,
                                             "Description": OPTSTR,
                                             "MastDefinitionPath" : MANSTR,
                                             "AdminState":OPTSTR
                                             }
                                        )

            self.lgLinkSystemName.value = lgLinkXmlConfig.get("SystemName")
            self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
            if lgLinkXmlConfig.get("UserName") != None:
                self.userName.value = lgLinkXmlConfig.get("UserName")
            else:
                self.userName.value = ""
            if lgLinkXmlConfig.get("Description") != None:
                self.description.value = lgLinkXmlConfig.get("Description")
            else:
                lgLinkXmlConfig.get("Description")
            self.lgLinkNo.value = int(lgLinkXmlConfig.get("Link"))
            self.mastDefinitionPath.value = lgLinkXmlConfig.get("MastDefinitionPath")
            if lgLinkXmlConfig.get("AdminState") != None:
                self.setAdmState(lgLinkXmlConfig.get("AdminState"))
            else:
                self.setAdmState(ADM_DISABLE[STATE_STR])
        except:
            trace.notify(DEBUG_ERROR, "Configuration validation failed for Light group link, traceback: " + str(traceback.print_exc()))
            return rc.TYPE_VAL_ERR
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
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

    def updateReq(self, child, source, uploadNReboot = True):
        if uploadNReboot: 
            self.updated = True
        else:
            self.updated = False
        return self.parent.updateReq(self, source, uploadNReboot)

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
            try:
                trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.value + " was reconfigured - applying the configuration")
                res = self.__setConfig()
            except Exception as e:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Light group link " + self.lgLinkSystemName.value)
                return rc.GEN_ERR
            if res != rc.OK:
                trace.notify(DEBUG_PANIC, "Could not set new configuration for Light group link " + self.lgLinkSystemName.value)
                return res
        else:
            trace.notify(DEBUG_TERSE, "Light group link " + self.lgLinkSystemName.value + " was not reconfigured, skiping re-configuration")
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
            for child in self.childs.value:
                child.abort()
        self.abortAll()
        # WEE NEED TO CHECK IF THE ABORT WAS DUE TO THE CREATION OF THIS OBJECT AND IF SO DELETE OUR SELVES (self.delete)
        return rc.OK

    def getXmlConfigTree(self, decoder=True, text=False, includeChilds=True): #Change decoder value
        trace.notify(DEBUG_TERSE, "Providing Light group link .xml configuration")
        lgLinkXml = ET.Element("LightgroupsLink")
        sysName = ET.SubElement(lgLinkXml, "SystemName")
        sysName.text = self.lgLinkSystemName.value
        usrName = ET.SubElement(lgLinkXml, "UserName")
        usrName.text = self.userName.value
        descName = ET.SubElement(lgLinkXml, "Description")
        descName.text = self.description.value
        if not decoder:
            mastDef = ET.SubElement(lgLinkXml, "MastDefinitionPath")
            mastDef.text = self.mastDefinitionPath.value
        satLink = ET.SubElement(lgLinkXml, "Link")
        satLink.text = str(self.lgLinkNo.value)
        adminState = ET.SubElement(lgLinkXml, "AdminState")
        adminState.text = self.getAdmState()[STATE_STR]
        if decoder:
            lgLinkXml.append(self.__getXmlMastDesc())
        if includeChilds:
            childs = True
            try:
                self.childs.value
            except:
                childs = False
            if childs:
                for child in self.childs.value:
                    lgLinkXml.append(child.getXmlConfigTree(decoder=decoder))
        return minidom.parseString(ET.tostring(lgLinkXml)).toprettyxml(indent="   ") if text else lgLinkXml

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
                trace.notify(DEBUG_INFO, "Light group: " + self.lightGroups.candidateValue[-1].nameKey.value + " successfully added to light group link " + self.nameKey.value)
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
        self.NOT_CONNECTEDalarm.ceaseAlarm("Source object deleted")
        self.NOT_CONFIGUREDalarm.ceaseAlarm("Source object deleted")
        self.LINK_SCAN_OVERLOADalarm.ceaseAlarm("Source object deleted")
        self.LINK_FLASH_OVERLOADalarm.ceaseAlarm("Source object deleted")
        self.INT_FAILalarm.ceaseAlarm("Source object deleted")
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.updateReq(self, self, uploadNReboot = True)
        return rc.OK

    def accepted(self):
        self.nameKey.value = "LgLink-" + self.lgLinkSystemName.candidateValue
        nameKey = self.nameKey.candidateValue # Need to save nameKey as it may be gone after an abort from updateReq()
        if self.getAdmState() == ADM_ENABLE[STATE]:
            res = self.updateReq(self, self, uploadNReboot = True)
        else:
            res = self.updateReq(self, self, uploadNReboot = False)
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

    def getLightGroupTypes(self):
        pass

    def getMastTypes(self):
        self.__getXmlMastDesc()
        return self.mastTypes

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
        self.lgLinkOpDownStreamTopic =  MQTT_JMRI_PRE_TOPIC + MQTT_LGLINK_TOPIC + MQTT_OPSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.lgLinkSystemName.value
        self.lgLinkOpUpStreamTopic =    MQTT_JMRI_PRE_TOPIC + MQTT_LGLINK_TOPIC + MQTT_OPSTATE_TOPIC_UPSTREAM + self.parent.getDecoderUri() + "/" + self.lgLinkSystemName.value
        self.lgLinkAdmDownStreamTopic = MQTT_JMRI_PRE_TOPIC + MQTT_LGLINK_TOPIC + MQTT_ADMSTATE_TOPIC_DOWNSTREAM + self.parent.getDecoderUri() + "/" + self.lgLinkSystemName.value
        self.unRegOpStateCb(self.__sysStateRespondListener)
        self.unRegOpStateCb(self.__sysStateAllListener)
        self.regOpStateCb(self.__sysStateRespondListener, OP_DISABLED[STATE])
        self.regOpStateCb(self.__sysStateAllListener, OP_ALL[STATE])
        #self.mqttClient.unsubscribeTopic(self.lgLinkOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.mqttClient.subscribeTopic(self.lgLinkOpUpStreamTopic, self.__onDecoderOpStateChange)
        self.NOT_CONNECTEDalarm.updateAlarmSrc(self.nameKey.value)
        self.NOT_CONFIGUREDalarm.updateAlarmSrc(self.nameKey.value)
        self.LINK_SCAN_OVERLOADalarm.updateAlarmSrc(self.nameKey.value)
        self.LINK_FLASH_OVERLOADalarm.updateAlarmSrc(self.nameKey.value)
        self.INT_FAILalarm.updateAlarmSrc(self.nameKey.value)
        self.CBLalarm.updateAlarmSrc(self.nameKey.value)
        return rc.OK

    def __sysStateRespondListener(self, changedOpStateDetail, p_sysStateTransactionId = None):
        trace.notify(DEBUG_INFO, "Light group link " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.lgLinkAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
            else:
                self.mqttClient.publish(self.lgLinkAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)

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
            self.INT_FAILalarm.admDisableAlarm()
            self.CBLalarm.admDisableAlarm()
        elif (changedOpStateDetail & OP_DISABLED[STATE]) and not (opStateDetail & OP_DISABLED[STATE]):
            self.NOT_CONNECTEDalarm.admEnableAlarm()
            self.NOT_CONFIGUREDalarm.admEnableAlarm()
            self.INT_FAILalarm.admEnableAlarm()
            self.CBLalarm.admEnableAlarm()
            if not self.updated:
                self.updateReq(self, self, uploadNReboot = True)
        if (changedOpStateDetail & OP_INIT[STATE]) and (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.raiseAlarm("Light-group link has not connected, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INIT[STATE]) and not (opStateDetail & OP_INIT[STATE]):
            self.NOT_CONNECTEDalarm.ceaseAlarm("Light-group link has now successfully connected")
        if (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.raiseAlarm("Light-group link has not been configured, it might be restarting-, but may have issues to connect to the WIFI, LAN or the MQTT-brooker, or the MAC address may not be correctly provisioned", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_UNCONFIGURED[STATE]) and not (opStateDetail & OP_UNCONFIGURED[STATE]):
            self.NOT_CONFIGUREDalarm.ceaseAlarm("Light-group link is now successfully configured")
        if (changedOpStateDetail & OP_INTFAIL[STATE]) and (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.raiseAlarm("Light-group link is experiencing an internal error", p_sysStateTransactionId, True)
        elif (changedOpStateDetail & OP_INTFAIL[STATE]) and not (opStateDetail & OP_INTFAIL[STATE]):
            self.INT_FAILalarm.ceaseAlarm("Light-group link is no longer experiencing any internal errors")
        if (changedOpStateDetail & OP_CBL[STATE]) and (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.raiseAlarm("Parent object for which this object is depending on has failed", p_sysStateTransactionId, False)
        elif (changedOpStateDetail & OP_CBL[STATE]) and not (opStateDetail & OP_CBL[STATE]):
            self.CBLalarm.ceaseAlarm("Parent object for which this object is depending on is now working")

    def __onDecoderOpStateChange(self, topic, value):
        trace.notify(DEBUG_INFO, "Lg Link " + self.nameKey.value + " received a new OP State from client: " + value + " setting server OP-state accordingly")
        self.setOpStateDetail(self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])
        self.unSetOpStateDetail(~self.getOpStateDetailBitMapFromStr(value) & ~OP_DISABLED[STATE] & ~OP_SERVUNAVAILABLE[STATE] & ~OP_CBL[STATE])

    def __getXmlMastDesc(self):
        self.__getMastAspects()
        trace.notify(DEBUG_TERSE, "Providing mastAspects over arching decoders .xml configuration")
        xmlSignalmastDesc = ET.Element("SignalMastDesc")
        xmlAspects = ET.SubElement(xmlSignalmastDesc, "Aspects")
        for aspect in self.aspectTable:
            xmlAspect = ET.SubElement(xmlAspects, "Aspect")
            xmlAspectName = ET.SubElement(xmlAspect, "AspectName")
            xmlAspectName.text = aspect
            for mast in self.aspectTable.get(aspect):
                xmlMast = ET.SubElement(xmlAspect, "Mast")
                xmlMastType = ET.SubElement(xmlMast, "Type")
                xmlMastType.text = str(mast)
                for headAspect in (self.aspectTable.get(aspect).get(mast).get("headAspects")):
                    xmlHead = ET.SubElement(xmlMast, "Head")
                    xmlHead.text = str(headAspect)
                xmlNoofpxl = ET.SubElement(xmlMast, "NoofPxl")
                xmlNoofpxl.text = str(
                    self.aspectTable.get(aspect).get(mast).get("NoofPxl")
                )
        return xmlSignalmastDesc

    def __getMastAspects(self):
        trace.notify(DEBUG_INFO, "Configuring mastAspects")
        self.aspectTable = {}
        self.mastTypes = []
        try:
            aspectsXmlTree = ET.ElementTree(ET.fromstring(self.rpcClient.getFile(self.mastDefinitionPath.value + "/aspects.xml")))
        except:
            trace.notify(DEBUG_ERROR, "aspects.xml not found")
            return None

        if str(aspectsXmlTree.getroot().tag) != "aspecttable":
            trace.notify(DEBUG_PANIC, "aspects.xml missformated")
            assert False
        found = False
        for child in aspectsXmlTree.getroot():
            if child.tag == "aspects":
                for subchild in child:
                    if subchild.tag == "aspect":
                        for subsubchild in subchild:
                            if subsubchild.tag == "name":
                                self.aspectTable[subsubchild.text] = None
                                found = True
                                break
        if found == False:
            trace.notify(DEBUG_PANIC, "no Aspects found - aspects.xml  missformated")
            assert false
        fileFound = False

        for filename in self.rpcClient.listDir(self.mastDefinitionPath.value):
            if filename.endswith(".xml") and filename.startswith("appearance-"):
                fileFound = True
                if self.mastDefinitionPath.value.endswith("\\"):  # Is the directory/filename really defining the SM type?
                    mastType = (self.mastDefinitionPath.value.split("\\")[-2] + ":" + (filename.split("appearance-")[-1]).split(".xml")[0])  # Fix UNIX portability
                else:
                    mastType = (self.mastDefinitionPath.value.split("\\")[-1] + ":" + (filename.split("appearance-")[-1]).split(".xml")[0])
                self.mastTypes.append(mastType)
                trace.notify(DEBUG_INFO, "Parsing Appearance file: " + self.mastDefinitionPath.value + "\\" + filename)
                appearanceXmlTree = ET.ElementTree(ET.fromstring(self.rpcClient.getFile(self.mastDefinitionPath.value + "\\" + filename)))
                if str(appearanceXmlTree.getroot().tag) != "appearancetable":
                    trace.notify(DEBUG_PANIC, filename + " is  missformated")
                    assert false
                found = False
                for child in appearanceXmlTree.getroot():
                    if child.tag == "appearances":
                        for subchild in child:
                            if subchild.tag == "appearance":
                                headAspects = []
                                aspectName = None
                                cnt = 0
                                for subsubchild in subchild:
                                    if subsubchild.tag == "aspectname":
                                        aspectName = subsubchild.text
                                    if aspectName != None and subsubchild.tag == "show":
                                        found = True
                                        headAspects.append(
                                            self.__decodeAppearance(subsubchild.text)
                                        )
                                        cnt += 1
                                        x = self.aspectTable.get(aspectName)
                                        if x == None:
                                            x = {
                                                mastType: {
                                                    "headAspects": headAspects,
                                                    "NoofPxl": -(-cnt // 3) * 3,
                                                }
                                            }
                                        else:
                                            x[mastType] = {
                                                "headAspects": headAspects,
                                                "NoofPxl": -(-cnt // 3) * 3,
                                            }
                                        self.aspectTable[aspectName] = x
                if found == False:
                    trace.notify(DEBUG_PANIC, "No Appearances found in: " + filename)
                    assert false
        if fileFound != True:
            trace.notify(DEBUG_PANIC, "No Appearance file found")
            assert false
        trace.notify(DEBUG_INFO, "mastAspects successfulyy generated")
        return appearanceXmlTree

    def __decodeAppearance(self, appearance):
        if (
            appearance == "red"
            or appearance == "green"
            or appearance == "yellow"
            or appearance == "lunar"
        ):
            return "LIT"
        elif (
            appearance == "flashred"
            or appearance == "flashgreen"
            or appearance == "flashyellow"
            or appearance == "flashlunar"
        ):
            return "FLASH"
        elif appearance == "dark":
            return "UNLIT"
        else:
            trace.notify(DEBUG_PANIC, "A non valid appearance: " + appearance + " was found")
# End LgLink
#------------------------------------------------------------------------------------------------------------------------------------------------
