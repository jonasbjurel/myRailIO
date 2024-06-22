#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A myRailIO server management object model (MOM) verification test script - verifying the transactional model, as well as the type verification
# of all used myRailIO Managed objects (MO) defined data types
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################

#################################################################################################################################################
# Dependencies
#################################################################################################################################################
# Python dependencies
import os
import sys
# Internal project dependencies
import imp
imp.load_source('jmriRpcClient', '..\\rpc\\myRailIORpcClient.py')
from jmriRpcClient import *
imp.load_source('moTypes', '..\\schema\\moTypes.py')
from moTypes import *
imp.load_source('schema', '..\\schema\\schema.py')
from schema import schema

#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------

#################################################################################################################################################
# System constants
#################################################################################################################################################
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: System constants
#------------------------------------------------------------------------------------------------------------------------------------------------

#================================================================================================================================================
# Good/Bad myRailIO MO Value/Type generator functions
#================================================================================================================================================
def genEstr_t(good=True):
    if good: return ["My good estr_t"]
    else: return [1, ["My bad estr_t"], {"My bad estr_t":"My bad estr_t"}, ("My bad estr_t",)]

def genEint_t(good=True):
    if good: return[1]
    else: return ["My bad eint_t", 1.0, [1], {"My bad eint_t":1}, ("My bad eint_t")]

def genEfloat_t(good=True):
    if good: return [1.0, 1.00, 1.000]
    else: return ["My bad efloat_t", 1, [1.0], {"My bad efloat_t":1.0}, ("My bad efloat_t")]

def genEdict_t(good=True):
    if good: return [{}, {"My good edict":None}]
    else: return ["My bad edict_t", 1, [1.0], ("My bad edict_t")]

def genElist_t(good=True):
    if good: return [[], ["My good edict"]]
    else: return ["My bad elist_t", 1, 1.0, {"My bad elist_t":1.0}, ("My bad elist_t")]

def genEtuple_t(good=True):
    if good: return [(), ("My good tuple_t")]
    else: return ["My bad tuple_t", 1, 1.0, {"My bad tuple_t":1.0}]

def genDate_t(good=True):
    if good: return ["2021-01-01"]
    else: return ["2021/01/01"]

def genTime_t(good=True):
    if good: return ["1:00:00", "01:00:00", "13:00:00"]
    else: return ["25:00:00", "1/00/00", "1,00,00", "1.00.00"]

def genTz_t(good=True):
    if good: return [-12, 0, 12]
    else: return ["0", -13, 13, 0.5]

def genAdminState_t(good=True):
    if good: return ["ENABLE", "DISABLE"]
    else: return [1, "BAD_ADMIN_STATE"]

def genMac_t(good=True):
    if good: return ["01:23:45:67:89:AB", "CD:EF:01:23:45:69", "01-23-45-67-89-AB", "CD-EF-01-23-45-69"]
    else: return [1, "GH:IJ:KL:MN:OP:QR", "01.23.45.67.89.AB"]

def genIpPort_t(good=True):
    if good: return [1, 65535]
    else: return [0, 65536, "1", "65535"]

def genIpProtocol_t(good=True):
    if good: return ["TCP", "UDP"]
    else: return [0, "MY_BAD_PROTOCOL"]

def genUri_t(good=True):
    if good: return ["my.domain.com", "my.second.domain.com", "192.168.1.255"]
    else: return [0, "http://mybad.domain.com", "mybad.domain.com:80", "256.256.256.0", "-1.-1.-1.0", "255.255.255.255.255"]

def genMultiChoiceUri_t(good=True):
    if good: return [["my.domain.com"], ["first.domain.com", "second.domain.com", "192.168.1.1"]]
    else: return [1, "mybad.domain.com", ["http://mybad.first.domain.com"], ["mybad.first.domain.com:80"], ["256.1.1.1"], ["1.1.1.1.1"]]

def genMultiChoicePort_t(good=True):
    if good: return [[1], [1, 65355]]
    else: return [1, [-1], [0], [65356]]

def genLogVerbosity_t(good=True):
    if good: return ["DEBUG-PANIC", "DEBUG-ERROR", "DEBUG-INFO", "DEBUG-TERSE", "DEBUG-VERBOSE"]
    else: return [1, "DEBUG-MY_BAD_DEBUG_LEVEL"]

def genDecoderSystemName_t(good=True):
    if good: return ["GJD-MY_DECODER"]
    else: return [1, "ABC-MY_BAD_DECODER"]

def genLgLinkSystemName_t(good=True):
    if good: return ["GJLL-MY_LG_LINK"]
    else: return [1, "ABC-MY_BAD_LG_LINK"]

def genSatLinkSystemName_t(good=True):
    if good: return ["GJSL-MY_SAT_LINK"]
    else: return [1, "ABC-MY_BAD_SAT_LINK"]

def genSatSystemName_t(good=True):
    if good: return ["GJSAT-MY_SAT"]
    else: return [1, "ABC-MY_BAD_SAT"]

def myRailIOSensSystemName_t(good=True):
    if good: return ["MS-MY_SENS"]
    else: return [1, "ABC-MY_BAD_SENS"]

def genSensPortNo_t(good=True):
    if good: return [0, SATELITE_MAX_SENS_PORTS-1]
    else: return ["1", -1, SATELITE_MAX_SENS_PORTS]

def genSensType_t(good=True):
    if good: return ["DIGITAL"]
    else: return ["NON_DIGITAL"]

def myRailIOActSystemName_t(good=True):
    if good: return ["MT-MY TURNOUT", "ML-MY LIGHT", "IM-MY MEMORY"]
    else: return ["SOMETHING ELSE"]

def genActPortNo_t(good=True):
    if good: return [0, SATELITE_MAX_ACT_PORTS-1]
    else: return ["0", -1, SATELITE_MAX_ACT_PORTS]

def genActType_t(good=True):
    if good: return ["SOLENOID", "SERVO", "PWM", "ONOFF", "PULSE"]
    else: return ["NON VALID ACT TYPE"]

def myRailIOLgSystemName_t(good=True):
    if good: return ["IF$vsm:MyMast($1)", "IF$vsm:MyMast($01)", "IF$vsm:MyMast($001)", "IF$vsm:MyMast($0001)"]
    else: return ["IF$vsm:MyBadMast($00001)", "IF$vsm:MyBadMast(1)", "IF$vsm:MyBadMast$1", "iF$vsm:MyBadMast($1)"]

def genLgLinkNo_t(good=True):
    if good: return [0, DECODER_MAX_LG_LINKS-1]
    else: return ["0", -1, DECODER_MAX_LG_LINKS]

def genLgLinkAddr_t(good=True):
    if good: return [0, MAX_LG_ADRESSES-1]
    else: return ["0", -1, MAX_LG_ADRESSES]

def genLinkType_t(good=True):
    if good: return ["SATLINK", "LGLINK"]
    else: return ["NON VALID LINK TYPE"]

def genSatLinkNo_t(good=True):
    if good: return [0, DECODER_MAX_SAT_LINKS-1]
    else: return ["0", -1, DECODER_MAX_SAT_LINKS]

def genSatLinkAddr_t(good=True):
    if good: return [0, MAX_SAT_ADRESSES-1]
    else: return ["0", -1, MAX_SAT_ADRESSES]
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Good/Bad myRailIO MO Value/Type generator functions
#------------------------------------------------------------------------------------------------------------------------------------------------



#================================================================================================================================================
# Type test vector mapping - myRailIO-type to type generator
#================================================================================================================================================
typeTestMethods = {str(estr_t):genEstr_t, str(eint_t):genEint_t, str(efloat_t):genEfloat_t, str(edict_t):genEdict_t, str(elist_t):genElist_t,
                   str(etuple_t):genEtuple_t, str(date_t):genDate_t, str(time_t):genTime_t, str(tz_t):genTz_t, str(adminState_t):genAdminState_t,
                   str(mac_t):genMac_t, str(ipPort_t):genIpPort_t, str(ipProtocol_t):genIpProtocol_t, str(uri_t):genUri_t,
                   str(multiChoiceUri_t):genMultiChoiceUri_t, str(multiChoicePort_t):genMultiChoicePort_t, str(logVerbosity_t):genLogVerbosity_t,
                   str(decoderSystemName_t):genDecoderSystemName_t, str(lgLinkSystemName_t):genLgLinkSystemName_t,
                   str(satLinkSystemName_t):genSatLinkSystemName_t, str(satSystemName_t):genSatSystemName_t, str(jmriSensSystemName_t):myRailIOSensSystemName_t,
                   str(sensPortNo_t):genSensPortNo_t, str(sensType_t):genSensType_t, str(jmriActSystemName_t):myRailIOActSystemName_t,
                   str(actPortNo_t):genActPortNo_t, str(actType_t):genActType_t, str(jmriLgSystemName_t):myRailIOLgSystemName_t, str(lgLinkNo_t):genLgLinkNo_t,
                   str(lgLinkAddr_t):genLgLinkAddr_t, str(satLinkNo_t):genSatLinkNo_t, str(satLinkAddr_t):genSatLinkAddr_t}
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Type test vector mapping - myRailIO-type to type generator
#------------------------------------------------------------------------------------------------------------------------------------------------



#================================================================================================================================================
# All defined MO schemas
#================================================================================================================================================
allSchemas = [schema.BASE_SCHEMA, schema.ADMIN_SCHEMA, schema.GIT_SCHEMA, schema.MQTT_SCHEMA, schema.JMRI_RPC_SCHEMA, schema.SERVICES_SCHEMA,
              schema.ADM_STATE_SCHEMA, schema.DECODER_SCHEMA, schema.LG_LINK_SCHEMA, schema.SAT_LINK_SCHEMA, schema.SAT_SCHEMA, schema.SENS_SCHEMA,
              schema.ACT_SCHEMA, schema.LG_SCHEMA, schema.CHILDS_SCHEMA]
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: All defined MO schemas
#------------------------------------------------------------------------------------------------------------------------------------------------



#================================================================================================================================================
# Schema verification class
#================================================================================================================================================
class mySchemaVerificationClass(schema):
    def __init__(self):
        super().__init__()
        for schema in allSchemas:
            self.verifySchemas(schema)

    def verifySchemas(self, schema):
        # Itterating all managed objects (MOs) in the data schema provided
        self.setSchema(schema)
        for mo in schema:
            # Getting the MO Type generator
            typeGenerator = typeTestMethods[str(schema[mo])]
            # Itterating valid MO values
            for moVal in typeGenerator(good=True):
                try:
                    self.__dict__[mo].value = moVal
                    print("PASS - MO \"" + mo + "\" passed posetive type-check for value: " + str(moVal))
                except:
                    print("ERROR - MO \"" + mo + "\" Did not pass posetive type-check for value: " + str(moVal) + " - reason: " + str(traceback.print_exc()))
                    return
            # Itterating non valid MO values
            for moVal in typeGenerator(good=False):
                try:
                    self.__dict__[mo].value = moVal
                    print("ERROR - MO \"" + mo + "\" Did not pass negative type-check for value: " + str(moVal))
                    return
                except:
                    print("PASS - MO \"" + mo + "\" passed negative type-check for value: " + str(moVal))

            # Aborting all earlier MO settings
            self.__dict__[mo].abort()
            try:
                if (self.__dict__[mo].value != None):
                    print("ERROR - MO \"" + mo + "\" Existanse of MO (value) was not expected after abort")
                    print(self.__dict__[mo].value)
                    print(self.__dict__[mo].candidateValue)
                    return
            except:
                pass
            try:
                if (self.__dict__[mo].candidateValue != None):
                    print("ERROR - MO \"" + mo + "\" Existanse of MO (candidateValue) was not expected after abort")
                    return
            except:
                pass

            # Setting good MO value without commiting it
            self.__dict__[mo].value = typeGenerator(good=True)[0]
            try:
                if self.__dict__[mo].candidateValue != typeGenerator(good=True)[0]:
                    print("ERROR - MO \"" + mo + "\" MO candidateValue was not as expected - actual: " + str(self.__dict__[mo].candidateValue) + " , expected: " + str(typeGenerator(good=True)[0]))
                    return
            except:
                print("ERROR - MO \"" + mo + "\" Existanse of MO (candidateValue) was expected but not found - reason: " + str(traceback.print_exc()))
                return
            try:
                if self.__dict__[mo].value != None:
                    print("ERROR - MO \"" + mo + "\" Existanse of MO (value) was not expected as it was not committed")
                    return
            except:
                pass

            # Commiting good MO value
            self.__dict__[mo].commit()
            try:
                if self.__dict__[mo].candidateValue != typeGenerator(good=True)[0]:
                    print("ERROR - MO \"" + mo + "\" MO candidateValue was not as expected - actual: " + str(self.__dict__[mo].candidateValue) + " , expected: " + str(typeGenerator(good=True)[0]))
                    return
                if self.__dict__[mo].value != typeGenerator(good=True)[0]:
                    print("ERROR - MO \"" + mo + "\" MO value was not as expected - actual: " + str(self.__dict__[mo].candidateValue) + " , expected: " + str(typeGenerator(good=True)[0]))
                    return
            except:
                print("ERROR - MO \"" + mo + "\" MO value/candidateValue was expected but not found - reason: : "  + str(traceback.print_exc()))
                return
            print("PASS - MO \"" + mo + " passed all checks")
        print("PASS - Schema passed verification")
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Schema verification class
#------------------------------------------------------------------------------------------------------------------------------------------------



#================================================================================================================================================
# Schema verification main
#================================================================================================================================================
if __name__ == '__main__':
    trace.start()
    trace.setGlobalDebugLevel(DEBUG_PANIC)

    jmriRpcClient.start(uri = "127.0.0.2")
    mySchemaVerificationClass()
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Schema verification main
#------------------------------------------------------------------------------------------------------------------------------------------------
