#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A JMRI server-side script to control the generic JMRI MQTT signals, lights, turn-outs, sensosr and actuators
# decoders as defined in here: https://github.com/jonasbjurel/GenericJMRIdecoder
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################
import os
import sys
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *



# XML parse filters
# -----------------
MANSTR = {"expected" : True, "type" : "str", "values" : None, "except": "PANIC"}
OPTSTR = {"expected" : None, "type" : "str", "values" : None, "except": "PANIC"}
MANINT = {"expected" : True, "type" : "int", "values" : None, "except": "PANIC"}
OPTINT = {"expected" : None, "type" : "int", "values" : None, "except": "PANIC"}
MANFLOAT = {"expected" : True, "type" : "float", "values" : None, "except": "PANIC"}
OPTFLOAT = {"expected" : None, "type" : "float", "values" : None, "except": "PANIC"}
MANSPEED = {"expected" : True, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANSPEED = {"expected" : None, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANLVL = {"expected" : True, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
OPTLVL = {"expected" : None, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
MANSPEED = {
    "expected": True,
    "type": "str",
    "values": ["SLOW", "NORMAL", "FAST"],
    "except": "PANIC",
}
OPTSPEED = {
    "expected": None,
    "type": "str",
    "values": ["SLOW", "NORMAL", "FAST"],
    "except": "PANIC",
}
MANLVL = {
    "expected": True,
    "type": "str",
    "values": ["LOW", "NORMAL", "HIGH"],
    "except": "PANIC",
}
OPTLVL = {
    "expected": None,
    "type": "str",
    "values": ["LOW", "NORMAL", "HIGH"],
    "except": "PANIC",
}



# ==============================================================================================================================================
# Helper fuction: parse_xml
# Purpose: Parse xmlTrees and find keys, atributes and values
# ==============================================================================================================================================
def parse_xml(xmlTree,tagDict) :
#   tagDict: {"Tag" : {"expected" : bool, "type" : "int/float/int", "values" : [], "except": "error/panic/info}, ....}
    res = {}
    for child in xmlTree :
        tagDesc = tagDict.get(child.tag)
        if tagDesc == None :
            continue
        value = validateXmlText(child.text, tagDesc.get("type"), tagDesc.get("values"))
        if tagDesc.get("expected") == None :
            if value != None:
                res[child.tag] = value
            else : 
                continue
        elif tagDesc.get("expected") == True : 
            if value != None:
                    res[child.tag] = value
            else : through_xml_error(tagDesc.get("except"), "Tag: " + str(child.tag) + " Tag.text: " + str(child.text) + " did not pass type checks")
        else : through_xml_error(tagDesc.get("except"), "Tag: " + str(child.tag) + " was not expected")

    for tag in tagDict :
        if tagDict.get(tag).get("expected") != None :
            if res.get(tag) == None :
                if tagDict.get(tag).get("expected") == True : through_xml_error(tagDict.get(tag).get("except"), "Tag: " + tag + " was expected but not found")
    return res

def validateXmlText(txt, type, values) :
    if txt == None :
        return None
    if txt.strip() == "":
        return None
    if type == None :
        return txt.strip()
    elif type == "str" : res = str(txt).strip()
    elif type == "int" : res = int(txt)
    elif type == "float" : res = float(txt)
    else : 
        return None

    if values == None :
        return res
    else :
        for value in values :
            if res == value :
                return res
        return None

def through_xml_error(_except, errStr) : #THIS NEEDS HEAVY LIFTING
    print(errStr)
    if _except == "DEBUG_VERBOSE" : debug = DEBUG_VERBOSE
    elif _except == "DEBUG_TERSE" : debug = DEBUG_TERSE
    elif _except == "INFO" : debug = DEBUG_INFO
    elif _except == "ERROR" : debug = DEBUG_ERROR
    elif _except == "PANIC" : debug = DEBUG_PANIC
    else : debug = DEBUG_PANIC
    trace.notify(debug, errStr)
    return RC_OK
