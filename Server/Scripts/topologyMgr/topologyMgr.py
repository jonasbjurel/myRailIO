#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A genJMRI topologymgr class providing tracking of address/port assets, in order to avoid objects with overlapping addresses/ports:
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
from dataclasses import dataclass
from http.client import FOUND
import imp
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc


# End Dependencies
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Local internal constants
#################################################################################################################################################
# End Local internal constants
#------------------------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Class: topologyMgr
# Purpose:      tracking of address/port assets, in order to avoid objects with overlapping addresses/ports:
#               See archictecture.md for more information
# StdMethods:   addTopologyMember(), updateTopologyMember(), removeTopologyMember()
# SpecMethods:  No class specific methods
#################################################################################################################################################
@dataclass
class topologyMember():
    sysName: str
    address: int
    reference: ...



class topologyMgr(object):
    def __init__(self, parent, noOfAddresses):
        self.parent = parent
        self.maxAddresses = noOfAddresses
        self.remainingAddresses = noOfAddresses
        self.members = []
    
    def __del__(self):
        pass
    
    def addTopologyMember(self, sysName, address, reference):
        print(">>>>>>>>>>>>>>>>>>>> addTopologyMember: " + str(address) + " to system name: " + sysName + " Members: " + str(self.members))
        if self.remainingAddresses == 0:
            print(">>>>>>>>>>>>>>>>>>>> NO_RESOURCES")
            return rc.NO_RESOURCES
        for memberItter in self.members:
            if memberItter.sysName != sysName and memberItter.address == address:
                print(">>>>>>>>>>>>>>>>>>>> RESOURCES_IN_USE")
                return rc.RESOURCES_IN_USE
            if memberItter.sysName == sysName and memberItter.address == address:
                return rc.OK
            if memberItter.sysName == sysName and memberItter.address != address:
                memberItter.address = address
                return rc.OK
        if self.maxAddresses != -1:
            self.remainingAddresses -= 1
        self.members.append(topologyMember(sysName, address, reference))
        return rc.OK
    
    def updateTopologyMember(self, sysName, address, reference):
        found = False
        for memberItter in self.members:
            if memberItter.sysName == sysName:
                memberItter.address = address
                memberItter.reference = reference
                found = True
                
        if found:
            return rc.OK
        else:
            return rc.DOES_NOT_EXIST
    
    def removeTopologyMember(self, sysName):
        print(">>>>>>>>>>>>>>>>>>>> removeTopologyMember")
        found = False
        for memberItter in self.members:
            if memberItter.sysName == sysName:
                self.members.remove(memberItter)
                self.remainingAddresses += 1
                found = True
        if found:
            return rc.OK
        else:
            print(">>>>>>>>>>>>>>>>>>>> NOT FOUND")
            return rc.DOES_NOT_EXIST
# End Class: topologyMgr
#------------------------------------------------------------------------------------------------------------------------------------------------

