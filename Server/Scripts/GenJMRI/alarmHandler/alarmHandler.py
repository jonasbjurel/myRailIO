#!/bin/python
#################################################################################################################################################
# Copyright (c) 2023 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# <HIGH LEVEL DESCRIPTION>
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
from dis import Instruction
import os
import sys
import time
from datetime import datetime
import threading


#################################################################################################################################################
# Module: 
#################################################################################################################################################

# Use pattern:
# ============
# The alarmHandler- and the alarm classes together form a lightweight alarm framework which keeps track of lists for active - and inactive alarms
# produced by an arbitrarhy alarm producer. An alarm consumer can subscribe to alarm events and can request the lists of active- and inactive
# alarms for presentation or otherwise. The alarm lists are volatile and will be gone after a restart of the alarmHandler static class object.
# Alarm consumer side:
# --------------------
#   def setup:
#       alarmHandler.start()
#       alarmHandler.regAlarmCb(ALARM_CRITICALITY_A, alarmHandler.myCriticalAlarms):
#
#   def myCriticalAlarms(p_criticality, p_noOfAlarms):
#       if p_noOfAlarms:
#           <allert code...>
#           <activeAlarmListBySeverityCritical = alarmHandler.getActiveAlarmListPerCriticality(ALARM_CRITICALITY_A)>
#               <process activeAlarmList by the severity given>
#           <activeAlarmListByTimeOrder = alarmHandler.getActiveAlarmListTimeOrder()>
#               <process activeAlarmListByTimeOrder by the severity given>
#       else:
#           <unAllertCode>
#           <ceasedAlarmListBySeverityCritical = alarmHandler.getActCeasedAlarmListPerCriticality(ALARM_CRITICALITY_A)>
#               <process ceasedAlarmList by the severity given>
#           <ceasedAlarmListByTimeOrderRaised = alarmHandler.getCeasedAlarmListTimeOrder():
#               <process ceasedAlarmList by the time order raised>
#           <ceasedAlarmListByTimeOrderRaised = alarmHandler.getCeasedAlarmListTimeOrder(true):
#               <process ceasedAlarmList by the time order ceased>
#
# Client side:
# ------------
#   class myAlarmClient:
#   def setup:
#       myAlarmClient.myAlarm = alarm
#       myAlarmClient.myAlarm.type = "Unavailable OPState"
#       myAlarmClient.myAlarm.oid = "topdecoder:decoder-1:satlink-1"
#       myAlarmClient.myAlarm.criticality = ALARM_CRITICALITY_A
#       myAlarmClient.myAlarm.sloganDescription = "Link failure"

#   def onOpStateLinkFailureCRC (p_unAvailable):
#       if p_unavailable: myAlarmClient.myAlarm.raise("Excessive CRC Errors")
#       else:  myAlarmClient.myAlarm.cease()



#################################################################################################################################################
# Class: alarmHandler
# Purpose: A simple alarm class that keeps track of active - and inactive alarm lists generated from different alarm sources and with
#          different alarm criticalities. It depends on the alarm class further down which basically defines the producer/client side API for
#          the alarm handler. The alarmHandler class it self - does not provide any visualization - that is left for the consumers of the class.
#          However it provides means for consuming the alarm situation through requests and call-backs. Three alarm criticalities are supported
#          (chosen from a simplicity view):
#          - ALARM_CRITICALITY_A: Indicates that the opertations may be out of service all together
#          - ALARM_CRITICALITY_B  Indicates limitted opertions performance that may cause service interuption
#          - ALARM_CRITICALITY_C  Indicates peripherial operations limitations that is likely not to impact the core business logic execution,
#                                 but may impact system observability, LCM operations of the system, etc.
#          alarmHandler is an ephemereal class, thus it does not provide any retention of the alarm lists in between sessions/reboots.
# OpenMethods:
#   - start: Starts and initiates the alarmHandler
#   - regAlarmCb(criticality, cb): registers a callback which is called whenever the number of active alarms for the given criticality
#                                  is changed - the cb prototype looks like this: cb(criticality, numberOfActiveAlarms).
#   - unRegAlarmCb(criticality, cb): Unregisters a callback for a given alarm criticality.
#   - getNoOfActiveAlarms(p_criticality): Returns number of active alarms for the given alarm criticality
#   - getActiveAlarmListPerCriticality(criticality): Returns a list of active alarm objects (see further down on the alarm object
#                                                    class)  in time order for a given alarm criticality
#   - getActiveAlarmListTimeOrder(): Returns a list of active alarm objects (see further down on the alarm object class) 
#                                    in time order - no matter of the alarm criticality
#   - getCeasedAlarmListPerCriticality(criticality): Returns a list of ceased alarm objects (see further down on the alarm object
#                                                    class) in time order for a given alarm criticality
#   - getCeasedAlarmListTimeOrder(criticality, ceaseTimeOrder = false): Returns a list of ceased alarm objects (see further
#                                                                       down on the alarm object class) in time order no matter
#                                                                       of the alarm criticality. If ceaseTimeOrder is set to True,
#                                                                       the sorting will be based on the cease time of the alarm,
#                                                                       otherwise the raise time is used for sorting.
# Private Methods: 
#   - _sendCb(criticality): Is a private method that traverses all registered callback requests and sends an update on the alarm
#                           situation
#################################################################################################################################################
class alarmHandler:
    def start:
        alarmHandler._alarmHandlerLock = threading.Lock()
        globalUid = 1
        for criticalityItter in range(3)
            alarmHandler.activeAlarms.append([])
            alarmHandler.ceasedAlarms.append([])
            alarmHandler.cb.append([])

    def regAlarmCb(p_criticality, p_cb):
        with _alarmHandlerLock:
            alarmHandler.cb[p_criticality].append(p_cb)

    def unRegAlarmCb(p_criticality, p_cb):
        with _alarmHandlerLock:
            for cbItter in range(0, len(alarmHandler.cb[p_criticality])):
                if alarmHandler.cb[p_criticality][cbItter] = p_cb:
                    del alarmHandler.cb[p_criticality][cbItter]

    def _raise(p_alarmObj):
        with _alarmHandlerLock:
            for alarmObjItter in alarmHandler.activeAlarms[p_alarmObj.criticality]:
                if alarmObjItter._uid == p_alarmObj._uid:
                    return 0
            p_alarmObj._uid = globalUid
            globalUid = globalUid + 1
            alarmHandler.activeAlarms[p_alarmObj.criticality].append(p_alarmObj)
            alarmHandler._sendCb(p_alarmObj.criticality)
            return globalUid - 1

    def _cease(p_alarmObj):
        with _alarmHandlerLock:
            for alarmObjItter in range(0, len(alarmHandler.activeAlarms[p_alarmObj.criticality])):
                if alarmHandler.activeAlarms[p_alarmObj.criticality][alarmObjItter]._uid == p_alarmObj._uid:
                    del alarmHandler.activeAlarms[p_alarmObj.criticality][alarmObjItter]
                    alarmHandler.ceasedAlarms[p_alarmObj.criticality].append(p_alarmObj)
                    alarmHandler._sendCb(p_alarmObj.criticality)
                    return

    def getNoOfActiveAlarms(p_criticality):
        with _alarmHandlerLock:
            return cbItter(p_criticality, len(alarmHandler.activeAlarms[p_criticality]))

    def getActiveAlarmListPerCriticality(p_criticality):
        with _alarmHandlerLock:
            return alarmHandler.activeAlarms[p_criticality]

    def getActiveAlarmListTimeOrder:
        with _alarmHandlerLock:
            alarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
                for alarmObjItter in alarmHandler.activeAlarms[criticalityItter]:
                    for alarmTimeOrderItter in range(0, len(alarmTimeOrder)):
                        if alarmObjItter._raiseEpochTime > AlarmTimeOrder[AlarmTimeOrderItter]._raiseEpochTime:
                            alarmTimeOrder.insert(alarmTimeOrderItter, alarmObjItter)
                            break
            return alarmHandler.activeAlarms[p_criticality]

    def getCeasedAlarmListPerCriticality(p_criticality):
        with _alarmHandlerLock:
            return alarmHandler.ceasedAlarms[p_criticality]

    def getCeasedAlarmListTimeOrder(p_ceaseTimeOrder = false):
        with _alarmHandlerLock:
            AlarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
                for alarmObjItter in alarmHandler.ceasedAlarms[criticalityItter]:
                    for AlarmTimeOrderItter in range(0, len(AlarmTimeOrder)):
                        if p_ceaseTimeOrder:
                            if alarmObjItter._ceaseEpochTime > AlarmTimeOrder[AlarmTimeOrderItter]._ceaseEpochTime:
                                AlarmTimeOrder.insert(AlarmTimeOrderItter, alarmObjItter)
                                break
                        else:
                            if alarmObjItter._raiseEpochTime > AlarmTimeOrder[AlarmTimeOrderItter]._raiseEpochTime:
                                AlarmTimeOrder.insert(AlarmTimeOrderItter, alarmObjItter)
                                break
            return AlarmTimeOrder

    def _sendCb(p_criticality):
        with _alarmHandlerLock:
            for cbItter in alarmHandler.cb[p_criticality]:
                cbItter(p_criticality, len(alarmHandler.activeAlarms[p_criticality]))



#################################################################################################################################################
# Class: alarm
# Purpose: alarm is a simple client side class of the alarm handler. It provides means for alarm producers to raise- and cease alarms which
#          will provide context to a statefull alarm, provide a description to it, will provide a uniqe alarm Id to it, etc.
# Open Methods and objects to be used by the alarm producers:
# ===========================================================
# Open data-structures:
# ---------------------
#   type:                           A static string that on a high level describes the alarm nature - E.g. Operational state down, or exsessive errors, or...
#                                   This should follow a global convention - but it is outside the scope of this implementation.
#   oid:                            Should reflect the object ID with contextual information - eg: topDecoder:myDecoder:MySatLink1:mySat, the contextual representation
#                                   of the oid's is outside the scope of this implementation.
#   criticality:                    Alarm criticality - any of: ALARM_CRITICALITY_A, ALARM_CRITICALITY_B, ALARM_CRITICALITY_C:
#                                       - ALARM_CRITICALITY_A: Indicates that the service may be out of service all together
#                                       - ALARM_CRITICALITY_B  Indicates limitted service performance that may cause service interuption
#                                       - ALARM_CRITICALITY_C  Indicates peripherial service limitations that is likely not to impact the core business logic execution,
#                                                      but may impact system observability, LCM operations of the system, etc.
#   sloganDescription:              A static high level description of the alarm (on slogan level), E.g. link down
#   contextDescription:             A runtime/context based description on what may have lead up to this alarm, E.g. Excessive CRC errors
#
# Open methods:
# -------------
#   raise(contextDescription):      Raise an alarm with an optional contextDescription - triggered by the client side
#   cease():                        Cease an alarm: Cease an alarm - triggered by the client side
#
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
#   _uid:                           A globally unique alarm identifier assigned by the alarmHandler on the server side
#   _active:                        Indicates if the alarm is active (sits in the active alarm list), or ceased (sits in the historical ceased alarm list)
#   _raiseEpochTime:                Epoch time (s) when the alarm was raised
#   _raiseUtcTime:                  UTC time/timeOfDay when the alarm was raised in format: %Y-%m-%d %H:%M:%S.%f
#   _ceaseEpochTime:                Epoch time (s) when the alarm was ceased
#..._ceaseUtcTime:                  UTC time/timeOfDay when the alarm was ceased in format: %Y-%m-%d %H:%M:%S.%f
#   _lastingTimeS:                  Alarm lasting time in seconds
#   _lastingTime:                   Alarm lasting time in format: %Y-%m-%d %H:%M:%S.%f
#
# Private methods:
# ----------------
#   _getUid:                        Returns the unique alarm identifier assigned by the alarmHandler
#   _getIsActive:                   Returns/Checks if the alarm is active or historical(ceased)
#   _getRaiseTime                   Reurns UTC Raise-time of an alarm in format: %Y-%m-%d %H:%M:%S.%f
#   _getCeaseTime:                  Reurns UTC Cease-time of an alarm in format: %Y-%m-%d %H:%M:%S.%f
#   _getLastingTimeS:               Returns Lasting time for an alarm in seconds
#   _getLastingTime:                Returns Lasting time for an alarm in format: %Y-%m-%d %H:%M:%S.%f
#
#################################################################################################################################################
ALARM_CRITICALITY_A         0                                                       # Indicates service out of order
ALARM_CRITICALITY_B         1                                                       # Indicates limitted service performance
ALARM_CRITICALITY_C         2                                                       # Indicates peripherial service limitations

class alarm:
    type : str                                                                      # Alarm type
    oid : str                                                                       # Source object ID
    _uid : str = 0                                                                  # Unique alarm event id
    criticality : int = ALARM_CRITICALITY_A                                         # Criticality
    sloganDescription : str                                                         # Static Alarm slogan decription
    contextDescription : str                                                        # Specific information for this alarm context/occurance
    _active : bool = false                                                          # Alarm is active or not
    _raiseEpochTime : int                                                           # Epoch time (s) for raising the alarm
    _raiseUtcTime : str                                                             # Date and time of day for raising the alarm
    _ceaseEpochTime : int                                                           # Epoch time time for ceasing the alarm
    _ceaseUtcTime : str                                                             # Date and time of day for ceasing the alarm
    _lastingTimeS : int                                                             # Time period the alarm lasted for in seconds
    _lastingTime : str                                                              # Time period the alarm lasted for in %Y-%m-%d %H:%M:%S.%f

    def raise(p_contextDescription = ""):
        self._active = True
        self._raiseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._raiseEpochTime = datetime.utcnow().timestamp()
        self._ceaseTime = None
        self._lastingTime = None
        self.contextDescription = p_contextDescription
        self._uid = alarmHandler._raise(self)

    def cease():
        self._active = False
        self._ceaseTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._ceaseEpochTime = datetime.utcnow().timestamp()
        self._lastingTimeS = self._ceaseEpochTime - self._raiseEpochTime
        self._lastingTime = str(datetime.timedelta(self._lastingTimeS))
        alarmHandler._cease(self)
        self.info = None
        self._raiseTime = None
        self._raiseEpochTime = None
        self._ceaseTime = None
        self._ceaseEpochTime = None
        self._lastingTime = None

    def _getUid:
        return _self.uid

    def _getIsActive:
        return self._active

    def _getRaiseTime:
        return self._raiseTime

    def _getCeaseTime:
        return self._ceaseTime = None

    def _getLastingTimeS:
        return self._lastingTimeS

    def _getLastingTime:
        return self._lastingTime
