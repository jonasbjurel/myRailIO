#!/bin/python
#################################################################################################################################################
# Copyright (c) 2024 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI time zone class that provide time zone related functionality.
#
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
import os
import sys
import csv
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Local internal constants
#################################################################################################################################################
zoneFileName = "zones.csv"
zoneFilePath = os.path.dirname(os.path.realpath(__file__))
zoneFile = zoneFilePath + "\\" + zoneFileName
#------------------------------------------------------------------------------------------------------------------------------------------------
# End Local internal constants
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: tz
# Purpose: This class provides time zone related functionality. It provide means to translate between clear text time zones and encoded time 
# zones. As well as providing a list of the most common time zones in clear text.
# The timezone information is stored in a csv file zones.csv in the same directory as the class file.
# Methods: 
#   getTzKeyValueDict():ret dict            - Returns a dictionary with the time zone information from the csv file, where the key is the clear
#                                               text time zone and the value is the corresponding encoded time zone, which has timezone as well
#                                               as daylight saving time information.
#   getClearTextTimeZones():ret list         - Returns a list of the most common time zones in clear text.
#   getEncodedTimeZones(ClearTextTz):ret str - Returns the encoded time zone for a given clear text time zone.
#################################################################################################################################################
class tz():
    @staticmethod
    def getTzKeyValueDict():
        zoneDict = {}
        tzDictReader = csv.DictReader(open(zoneFile))
        for row in tzDictReader:
            zoneDict[row["zone"]] = row["tzString"]
        return zoneDict

    @staticmethod
    def getClearTextTimeZones():
        return (tz.getTzKeyValueDict()).keys()

    def getEncodedTimeZones(ClearTextTz):
        return tz.getTzKeyValueDict()[ClearTextTz]
#------------------------------------------------------------------------------------------------------------------------------------------------
# END: Class tz
#------------------------------------------------------------------------------------------------------------------------------------------------    