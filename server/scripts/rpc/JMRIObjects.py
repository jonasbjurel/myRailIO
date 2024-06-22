#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# myRailIO common obect types
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
class jmriObj():
    MASTS = 0
    TURNOUTS = 1
    LIGHTS = 2
    SENSORS = 3
    MEMORIES = 4
    OBJ_STR = [""] * 256
    OBJ_STR[0] ="Signal Mast"
    OBJ_STR[1] = "Turnout"
    OBJ_STR[2] = "Light"
    OBJ_STR[3] = "Sensor"
    OBJ_STR[4] = "Memory"

    @staticmethod
    def getObjTypeStr(type):
        try:
            return jmriObj.OBJ_STR[type]
        except:
            pass


    @staticmethod
    def getMyRailIOTypeFromJMRIType(JMRIType):
        return jmriObj.OBJ_STR.index(JMRIType)
