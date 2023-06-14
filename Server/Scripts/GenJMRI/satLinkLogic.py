#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI sateliteLink class providing the genJMRI satelite link management- and supervision. genJMRI provides the concept of satelite links
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
import xml.etree.ElementTree as ET
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
class satLink(systemState, schema):
    def __init__(self, win, parentItem, rpcClient, mqttClient, name=None, demo=False):
        self.win = win
        self.demo = demo
        self.rpcClient = rpcClient
        self.mqttClient = mqttClient
        self.schemaDirty = False
        systemState.__init__(self)
        schema.__init__(self)
        self.setSchema(schema.BASE_SCHEMA)
        self.appendSchema(schema.SAT_LINK_SCHEMA)
        self.appendSchema(schema.ADM_STATE_SCHEMA)
        self.appendSchema(schema.CHILDS_SCHEMA)
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.satelites.value = []
        self.childs.value = self.satelites.candidateValue
        self.setAdmState(ADM_DISABLE[STATE_STR])
        self.setOpStateDetail(OP_INIT[STATE] | OP_UNCONFIGURED[STATE])
        if name:
            self.satLinkSystemName.value = name
        else:
            self.satLinkSystemName.value = "GJSL-NewSatLinkSysName"
        self.nameKey.value = "SatLink-" + self.satLinkSystemName.candidateValue
        self.userName.value = "GJSL-NewSatLinkUsrName"
        self.satLinkNo.value = 0
        self.description.value = "New Satelite link"
        trace.notify(DEBUG_INFO,"New Satelite link: " + self.nameKey.candidateValue + " created - awaiting configuration")
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey.candidateValue, SATELITE_LINK, displayIcon=LINK_ICON)
        self.commitAll()
        self.clearStats()
        if self.demo:
            self.logStatsProducer = threading.Thread(target=self.__demoStatsProducer)
            self.logStatsProducer.start()
            for i in range(SATLINK_MAX_SATS):
                self.addChild(SATELITE, name="GJS-" + str(i), config=False, demo=True)

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
        res = self.parent.updateReq()
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "Validation of- or setting of configuration failed - initiated by configuration change of: " + satLinkXmlConfig.get("SystemName") + ", return code: " + trace.getErrStr(res))
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

    def updateReq(self):
        return self.parent.updateReq()

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

    def checkSysName(self, sysName):
        return self.parent.checkSysName(sysName)

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
        # WEE NEED TO CHECK IF THE ABORT WAS DUE TO THE CREATION OF THIS OBJECT AND IF SO DELETE OUR SELVES (self.delete)
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
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
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
                self.dialog = UI_sateliteDialog(self.satelites.candidateValue[-1], edit=True)
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
        self.dialog = UI_satLinkDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_satLinkDialog(self, edit=True)
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
        self.parent.delChild(self)
        self.win.unRegisterMoMObj(self.item)
        if top:
            self.parent.updateReq()
        return rc.OK

    def accepted(self):
        self.nameKey.value = "SatLink-" + self.satLinkSystemName.candidateValue
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
        self.remCrcErr = 0
        self.rxSymErr = 0
        self.rxSizeErr = 0
        self.wdErr = 0

    def __validateConfig(self):
        res = self.parent.checkSysName(self.satLinkSystemName.candidateValue)
        if res != rc.OK:
            trace.notify(DEBUG_ERROR, "System name " + self.satLinkSystemName.candidateValue + " already in use")
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
        return rc.OK

    def __sysStateRespondListener(self, changedOpStateDetail):
        trace.notify(DEBUG_INFO, "Satelite link " + self.nameKey.value + " got a new OP State generated by the server - informing the client accordingly - changed opState: " + self.getOpStateDetailStrFromBitMap(self.getOpStateDetail() & changedOpStateDetail) + " - the composite OP-state is now: " + self.getOpStateDetailStr())
        if changedOpStateDetail & OP_DISABLED[STATE]:
            if self.getAdmState() == ADM_ENABLE:
                self.mqttClient.publish(self.satLinkAdmDownStreamTopic, ADM_ON_LINE_PAYLOAD)
            else:
                self.mqttClient.publish(self.satLinkAdmDownStreamTopic, ADM_OFF_LINE_PAYLOAD)

    def __sysStateAllListener(self, changedOpStateDetail):
        # UPDATE GUI LIVE IF POSSIBLE
        # ADD TO ALARM LIST - LATER
        return

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