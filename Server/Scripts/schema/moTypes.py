#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# Managed object types for genJMRI, providing functions for transactional handling as well as object type- syntax and range validation.
# The base_t class provides the transactional handling of any arbitrary type (including transactional handling such as commit() and abort(),
# while ther is a specific class for each genJMRI MO type, providing the specific MO's value type-, syntax-, and range checking. 
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################

#################################################################################################################################################
# Dependencies
#################################################################################################################################################
# Python dependencies
import os
import sys
import copy
import re
import inspect
import imp
# Internal project dependencies
imp.load_source('jmriRpcClient', '..\\rpc\\genJMRIRpcClient.py')
from jmriRpcClient import *
imp.load_source('jmriObjectTypes', '..\\rpc\\JMRIObjects.py')
from jmriObjectTypes import *
imp.load_source('jmriObjectTypes', '..\\sysState\\sysState.py')
from sysState import *
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# System constants
#################################################################################################################################################
DECODER_MAX_LG_LINKS =          2
MAX_LG_ADRESSES =               16
DECODER_MAX_SAT_LINKS =         2
MAX_SAT_ADRESSES =              16
SATELITE_MAX_SENS_PORTS =       8
SATELITE_MAX_ACT_PORTS =        4
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: System constants
#------------------------------------------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# Class base_t
# Constants:
#   -
# Methods:
#   __init__():ret None - Object initialisation
#   __setattr__(attr["value"], value)(attr, value):ret None - Set MO attribute hook - triggering MO value validation and setting candidate config
#   __getattr__(attr["value"|"candidateValue"]):ret None - Get MO attribute hook - getting running or candidate config
#                        schema: dict with MO keys and MO types
#   commit():ret None - Commits changes made to the MO
#   abort(mo):ret None - Aborts changes  made to the MO
#################################################################################################################################################
class base_t():
    def __init__(self):
        super().__setattr__("dirty", False)
        super().__setattr__("truelyDirty", False)
        super().__setattr__("parent", inspect.currentframe().f_back.f_locals['self'])


    def __setattr__(self, attr, value):
        assert (attr == "value" or attr == "dirty"), "Can only set attr \"" + self.__class__.__name__ + "." + "value\"  or \"dirty\", not \"" + attr + "\""
        if attr == "value":
            self.validate(value)
            super().__setattr__("candidateValue", value)
            super().__setattr__("dirty", True)
            try:
                if value != super().__getattribute__("value"):
                    super().__setattr__("truelyDirty", True)
            except:
                super().__setattr__("truelyDirty", True)
        elif attr == "dirty":
            super().__setattr__("dirty", True)

    def __getattr__(self, attr):
        assert (attr == "value" or attr == "candidateValue" or attr == "dirty"), "Can only get attr \"" + self.__class__.__name__ + "." + "value\" or \"candidateValue\" or \"dirty\", not \"" + attr + "\""
        try:
            return super().__getattribute__(attr)
        except:
            pass
            #trace.notify(DEBUG_INFO, "Could not get MO attribute \"" + self.__class__.__name__ + "." + attr + "\" - does not exist")

    def commit(self):
        super().__setattr__("value", copy.copy(self.candidateValue))
        super().__setattr__("dirty", False)
        super().__setattr__("truelyDirty", False)

    def abort(self):
        try:
            super().__getattribute__("value")
        except:
            super().__delattr__("candidateValue")
            super().__setattr__("dirty", False)
            super().__setattr__("truelyDirty", False)
            return
        super().__setattr__("candidateValue", copy.deepcopy(self.value))
        super().__setattr__("dirty", False)
        super().__setattr__("truelyDirty", False)

#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Class base_t
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# genJMRI type specific validation classes
# Constants:
#   -
# Methods:
#   validate(value):ret None - Validates the specific type value properties - type, syntax, range
#   It can also add- or override methods for a genJMRI type class - like __add__, etc.
#################################################################################################################################################
class estr_t(base_t):
    def validate(self, value):
        assert type(value) == str, "estr_t didnt pass type check"

class eint_t(base_t):
    def validate(self, value):
        assert type(value) == int, "eint_t didnt pass type check"

class efloat_t(base_t):
    def validate(self, value):
        assert type(value) == float, "efloat_t didnt pass type check"

class ebool_t(base_t):
    def validate(self, value):
        assert type(value) == bool, "ebool_t didnt pass type check"

class edict_t(base_t):
    def validate(self, value):
        assert type(value) == dict, "edict_t didnt pass type check"

class elist_t(base_t):
    def validate(self, value):
        assert type(value) == list, "elist_t didnt pass type check"

    def append(self, value):
        self.candidateValue.append(value)
        super().__setattr__("dirty", True)

    def remove(self, value):
        self.candidateValue.remove(value)
        super().__setattr__("dirty", True)

class etuple_t(base_t):
    def validate(self, value):
        assert type(value) == tuple, "etuple_t didnt pass type check"

class date_t(base_t):
    def validate(self, value):
        assert type(value) == str, "date_t didnt pass type check"

class time_t(base_t):
    def validate(self, value):
        assert type(value) == str, "time_t didnt pass type check"

class tz_t(base_t):
    def validate(self, value):
        assert type(value) == str, "tz_t didnt pass type check"

class adminState_t(base_t):
    def validate(self, value):
        assert type(value) == list and type(value[STATE_STR]) == str, "adminState_t didnt pass type check"
        assert (value[STATE_STR].strip().upper() == "ENABLE" or value[STATE_STR].strip().upper() == "DISABLE"), "adminState_t didnt pass syntax check"

class mac_t(base_t):
    def validate(self, value):
        assert type(value) == str, "mac_t didnt pass type check"
        macRegex = re.compile(r"""(^([0-9A-F]{2}[-]){5}([0-9A-F]{2})$
                                     |^([0-9A-F]{2}[:]){5}([0-9A-F]{2})$)
                              """, re.VERBOSE|re.IGNORECASE)
        assert (re.match(macRegex, value) is not None), "mac_t didnt pass MAC syntax check"

class ipPort_t(base_t):
    def validate(self, value):
        assert type(value) == int, "ipPort_t didnt pass type check" 
        assert (value > 0 and value < 65536), "ipPort_t didnt pass range check"

class ipProtocol_t(base_t):
    def validate(self,value):
        assert type(value) == str, "ipProtocol_t didnt pass type check"
        assert (value.strip().upper() =="TCP" or value == "UDP"), "ipProtocol_t didnt pass syntax check"

class uri_t(base_t):
    def validate(self, value):
        assert type(value) == str, "uri_t didnt pass type check"
        uriRegex = re.compile(r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|' #domain...
                              r'localhost|' #localhost...
                              r'[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' # ...or ip
                              , re.IGNORECASE)
        assert (re.match(uriRegex, value) is not None), "uri_t didnt pass uri syntax check"

class multiChoiceUri_t(base_t):
    def validate(self, value):
        assert type(value) == list, "multiChoiceUri_t didnt pass type check"
        for uri in value:
            assert type(uri) == str, "multiChoiceUri_t didnt pass type check"
            uriRegex = re.compile(r'(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}\.?)|' #domain...
                                  r'localhost|' #localhost...
                                  r'[0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])' # ...or ip
                                  , re.IGNORECASE)
            assert (re.match(uriRegex, uri) is not None), "multiChoiceUri_t didnt pass uri syntax check"

    def __add__(self, value):
        self.validate(value)
        self.value.super(multiChoiceUri_t, self).__add__(value)

class multiChoicePort_t(base_t):
    def validate(self, value):
        assert type(value) == list, "multiChoicePort_t didnt pass type check"
        for port in value:
            assert type(port) == int, "multiChoicePort_t didnt pass type check"
            assert (port < 65536 and value > 0), "multiChoicePort_t didnt pass range check"

    def __add__(self, value):
        self.validate(value)
        self.value.super(multiChoicePort_t, self).__add__(value)

class logVerbosity_t(base_t):
    def validate(self, data):
        assert type(data) == str, "logVerbosity_t didnt pass type check"
        assert data.strip().upper() == "DEBUG-PANIC" or data == "DEBUG-ERROR" or data == "DEBUG-INFO" or data == "DEBUG-TERSE" or data == "DEBUG-VERBOSE", "logVerbosity_t didnt pass syntax check"

class decoderSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "decoderSystemName_t didnt pass type check" 
        assert re.match(r'^GJD', value), "decoderSystemName_t didnt pass syntax check"

class lgLinkSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "lgLinkSystemName_t didnt pass type check" 
        assert re.match(r'^GJLL', value), "lgLinkSystemName_t didnt pass syntax check" 
        try: self.nameKey.value = "LgLink-" + value
        except: pass

class satLinkSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "satLinkSystemName_t didnt pass type check" 
        assert re.match(r'^GJSL', value), "satLinkSystemName_t didnt pass syntax check"
        try: self.nameKey.value = "SatLink-" + value
        except: pass

class satSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "satSystemName_t didnt pass type check" 
        assert re.match(r'^GJSAT', value), "satSystemName_t didnt pass syntax check"
        try: self.nameKey.value = "Satelite-" + value
        except: pass

class jmriSensSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "jmriSensSystemName_t didnt pass type check" 
        assert re.match(r'^MS', value), "jmriSensSystemName_t didnt pass syntax check"
        try: self.nameKey.value = "Sensor-" + value
        except: pass

class sensPort_t(base_t):
    def validate(self, value):
        assert type(value) == int, "sensPort_t didnt pass type check" 
        assert (value < SATELITE_MAX_SENS_PORTS and value >= 0), "sensPort_t didnt pass range check" 

class sensType_t(base_t):
    def validate(self, value):
        assert type(value) == str, "sensType_t didnt pass type check" 
        assert (value.strip().upper() == "DIGITAL"), "sensType_t didnt pass syntax check" 

class jmriActSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "jmriActSystemName_t didnt pass type check" 
        assert (re.match(r'^MT', value) != None or re.match(r'^ML', value) != None or re.match(r'^IM', value) != None), "jmriActSystemName_t didnt pass syntax check"
        if re.match(r'^MT', value) != None:
            try: self.nameKey.value = "Actuator-Turnout-" + value
            except: pass
        elif re.match(r'^ML', value) != None:
            try: self.nameKey.value = "Actuator-Light-" + value
            except: pass
        else:
            try: self.nameKey.value = "Actuator-Memory-" + value
            except: pass

class actPort_t(base_t):
    def validate(self, value):
        assert type(value) == int, "actPort_t didnt pass type check" 
        assert (value < SATELITE_MAX_ACT_PORTS and value >= 0), "actPort_t didnt pass range check"
        if super().__getattribute__("parent").actSubType.candidateValue == "SOLENOID":
            assert ((value % 2) == 0), "actPort_t didnt pass range check, SOLENOID sub-type requires even port-base number"

class actType_t(base_t):
    def validate(self, value):
        assert type(value) == str, "actType_t didnt pass type check, expected a string" 
        assert (value.strip().upper() == "TURNOUT" or value.strip().upper() == "LIGHT" or value.strip().upper() == "MEMORY"), "actType_t didnt pass syntax check, was none of TURNOUT/LIGHT/MEMORY" 

class actSubType_t(base_t):
    def validate(self, value):
        assert type(value) == str, "actSubType_t didnt pass type check, expected a string"
        if super().__getattribute__("parent").actType.candidateValue.strip().upper()  == "TURNOUT":
            assert (value.strip().upper() == "SERVO" or value.strip().upper() == "SOLENOID"), "actSubType_t didnt pass type check for TURNOUT, expected SERVO/SOLENOID"
        elif super().__getattribute__("parent").actType.candidateValue.strip().upper() == "LIGHT":
            assert (value.strip().upper() == "ONOFF" or "DIM"), "actSubType_t didnt pass syntax check for LIGHT, expected ONOFF/DIM"
        elif super().__getattribute__("parent").actType.candidateValue.strip().upper() == "MEMORY":
            assert (value.strip().upper() == "ONOFF" or "SOLENOID" or "SERVO" or "PWM" or "PULSE"), "actSubType_t didnt pass syntax check for MEMORY, expected SOLENOID/SERVO/PWM/PULSE"

class jmriLgSystemName_t(base_t):
    def validate(self, value):
        assert type(value) == str, "jmriLgSystemName_t didnt pass type check" 
        assert (re.search(r'(?=.*^IF\$vsm:)(?=.*\(\$[0-9]{1,4}\)$)', value) != None or re.search(r'$ML', value) != None), "jmriLgSystemName_t didnt pass syntax check"
        try: self.nameKey.value = "LightGroup-Mast-" + value
        except: pass

class lgLinkNo_t(base_t):
    def validate(self, value):
        assert type(value) == int, "lgLinkNo_t didnt pass type check" 
        assert (value < DECODER_MAX_LG_LINKS and value >= 0), "lgLinkNo_t didnt pass range check"

class lgLinkAddr_t(base_t):
    def validate(self, value):
        assert type(value) == int, "lgLinkAddr_t didnt pass type check" 
        assert (value < MAX_LG_ADRESSES and value >= 0), "lgLinkAddr_t didnt pass range check"

class satLinkNo_t(base_t):
    def validate(self, value):
        assert type(value) == int, "satLinkNo_t didnt pass type check" 
        assert (value < DECODER_MAX_SAT_LINKS and value >= 0), "satLinkNo_t didnt pass range check" 

class satLinkAddr_t(base_t):
    def validate(self, value):
        assert type(value) == int, "satLinkAddr_t didnt pass type check" 
        assert (value < MAX_SAT_ADRESSES and value >= 0), "satLinkAddr_t didnt pass range check" 
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: genJMRI type specific validation classes
#------------------------------------------------------------------------------------------------------------------------------------------------
