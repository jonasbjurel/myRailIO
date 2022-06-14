#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI server management object model (MOM) schema class providing various genJMRI schema fractions, and transactional handling of those.
# The class holds one or several schema fractions with one or several managed objects (MOs) each.
#
# Setting a MO value is done by "mo.value = value" or "setattr(mo, "value", value")" which will validate the type-, syntax and range defined for the 
# specific MO in question, and will store it in the respective MO's "self.candidateValue" which is just a store for a future potential running 
# configuration value.
# "self.commit(mo)" or self.commitAll() will copy the MO's "self.candidateValue" into the "self.value" running configuration value; "self.abort(mo)"
# as well as "self.abort(all) will abort the transaction and copy the running configuration "self.value" to the candidate configuration
# "self.candidateValue".
#
# Reading of individual MO's can be performe through "mo.value" which will get the running configuration, or "mo.candidateValue" which will get
# the candidate MO value pending commit.
#
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################

#################################################################################################################################################
# Dependencies
#################################################################################################################################################
# Python dependencies
import os
import sys
import imp
# Internal project dependencies
imp.load_source('moTypes', '..\\schema\\moTypes.py')
from moTypes import *
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# System constants
#################################################################################################################################################
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: System constants
#------------------------------------------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# Class schema
# Constants:
#   BASE_SCHEMA, ADMIN_SCHEMA, GIT_SCHEMA etc all represents schema fragments that can be used for the schema composition for a particular 
#   genJMRI functional class, E.g. Topdecoder, Decoder, LG-Link, etc...
#
# Methods:
#   __init__():ret None - Object initialisation
#   setSchema(schema):ret None - Set a schema fraction
#                     schema: dict with MO keys and MO types
#   appendSchema(schema):ret None - Append a schema fraction
#                        schema: dict with MO keys and MO types
#   commitAll():ret None - Commit all MO changes performed to all the schema fragments in ths schema
#   abortAll():ret None - Abort all MO changes performed to all the schema fragments in ths schema
#   commit(mo):ret None - Commits changes to a specific MO item of this schema
#                         mo.commit() has the same effect as described above - but is global and does not need to be part of this schema
#   abort(mo):ret None - Aborts changes to a specific MO item of this schema
#                         mo.abort() has the same effect as described above - but is global and does not need to be part of this schema
#   isDirty():ret Bool - If True, there has been a write access to the MO since last commit/abort
#   isTruelyDirty():ret Bool - If True, committed value is different to candidate value

#################################################################################################################################################
class schema():
    BASE_SCHEMA =       {"userName":estr_t, "nameKey":estr_t, "description":estr_t}
    ADMIN_SCHEMA =      {"version":estr_t, "author":estr_t, "versionHistory":elist_t, "date":date_t, "time":time_t}
    GIT_SCHEMA =        {"gitBranch":estr_t, "gitTag":estr_t, "gitUrl":uri_t}
    MQTT_SCHEMA =       {"decoderMqttURI":uri_t, "decoderMqttPort":ipPort_t, "decoderMqttTopicPrefix":estr_t, "decoderMqttKeepalivePeriod":efloat_t}
    JMRI_RPC_SCHEMA =   {"jmriRpcURI":uri_t, "jmriRpcPortBase":ipPort_t, "JMRIRpcKeepAlivePeriod":efloat_t}
    SERVICES_SCHEMA =   {"ntpUri":multiChoiceUri_t, "ntpPort":ipPort_t, "ntpProtocol":ipProtocol_t, "tz":tz_t, "rsysLogUri":uri_t,
                         "rsysLogPort":ipPort_t, "rsysLogProtocol":ipProtocol_t, "logVerbosity":logVerbosity_t, "snmpUri":multiChoiceUri_t,
                         "snmpPort":ipPort_t, "snmpProtocol":ipProtocol_t}
    ADM_STATE_SCHEMA =  {"admState":adminState_t}
    TOP_DECODER_SCHEMA =    {"decoders":elist_t, "decoderFailSafe":ebool_t, "trackFailSafe":ebool_t}

    DECODER_SCHEMA =    {"decoderSystemName":decoderSystemName_t, "decoderMqttURI":uri_t, "mac":mac_t, "lgLinks":elist_t, "satLinks":elist_t}
    LG_LINK_SCHEMA =    {"lgLinkSystemName":lgLinkSystemName_t, "lgLinkNo":lgLinkNo_t, "lightGroups":elist_t}
    SAT_LINK_SCHEMA =   {"satLinkSystemName":satLinkSystemName_t, "satLinkNo":satLinkNo_t, "satelites":elist_t}
    SAT_SCHEMA =        {"satSystemName":satSystemName_t, "satLinkAddr":satLinkAddr_t, "sensors":elist_t, "actuators":elist_t}
    SENS_SCHEMA =       {"jmriSensSystemName":jmriSensSystemName_t, "sensPort":sensPort_t, "sensType":sensType_t}
    ACT_SCHEMA =        {"jmriActSystemName":jmriActSystemName_t, "actPort":actPort_t, "actType":actType_t, "actSubType":actSubType_t}
    LG_SCHEMA =         {"jmriLgSystemName":jmriLgSystemName_t, "lgLinkAddr":lgLinkAddr_t, "lgType":estr_t, "lgProperty1":estr_t, "lgProperty2":estr_t,
                         "lgProperty3":estr_t}
    CHILDS_SCHEMA =     {"childs":elist_t}

    def __init__(self):
        self.schema = {}
        self.schemaObjects = {}

    def setSchema(self, schema):
        self.schema = schema
        for moName in schema:
            object = schema[moName]()
            self.schemaObjects[moName] = object
            self.__dict__.update({moName:object})

    def appendSchema(self, schema):
        self.schema.update(schema)
        for moName in schema:
            object = schema[moName]()
            self.schemaObjects[moName] = object
            self.__dict__.update({moName:object})

    def commitAll(self):
        for moName in self.schemaObjects:
            try:
                self.schemaObjects[moName].commit()
            except:
                pass

    def abortAll(self):
        for moName in self.schemaObjects:
            try:
                self.schemaObjects[moName].abort()
            except:
                pass

    def commit(self, mo):
        mo.commit()

    def abort(self, mo):
        mo.abort()

    def isDirty(self):
        for moName in self.schemaObjects:
            if self.schemaObjects[moName].dirty:
                return True
        return False

    def isTruelyDirty(self):
        for moName in self.schemaObjects:
            if self.schemaObjects[moName].truelyDirty:
                return True
        return False
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Class schema
#------------------------------------------------------------------------------------------------------------------------------------------------
