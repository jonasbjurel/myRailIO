#!/bin/python
#################################################################################################################################################
# Copyright (c) 2023 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################

#################################################################################################################################################
# Description:
# ============
# The alarmHandler-, the alarm- and the table model classes together form a lightweight alarm framework which keeps track of lists for active -
# and inactive alarms produced by arbitrary alarm producers. An alarm consumer can subscribe to alarm events and can request the lists of active-
# and inactive alarms for presentation or otherwise. The alarm lists are volatile and will be gone after a restart of the alarmHandler static
# class object.
# The alarmTableModel class provides a plugin to QT5 QTable graphical presentation of active and ceased alarms, as well as a detailed graphical
# view of every active alarms as well historical instances of alarms.
#
# Use pattern:
# ============
#
# Alarm start-up - common:
# ------------------------
#       alarmHandler.start()
#
# Producer/Client side:
# ---------------------
#   class myAlarmClient:
#       def __init__(self):
#           self.myAlarm = alarm(self, <alarmType : str>, <alarmSrc : str>, <alarmCriticality : int>, <alarmSlogan : str>)
#
#       def onSystemNameUpdate(self, p_sysName : str):
#           self.myAlarm.updateAlarmSrc(p_sysName)
#
#       def onOpStateLinkFailureCRC (p_opState):
#           if p_opState & OP_STATE_DISABLED: self.myAlarm.admDisableAlarm
#           else: self.myAlarm.admEnableAlarm
#           if p_opState & OP_EXCESS_CRC: self.myAlarm.raiseAlarm("Excessive CRC Errors")
#           else: self.myAlarm.raiseAlarm.ceaseAlarm("Number of CRC Errors is below the Excessive CRC Error threshold")
#
# Alarm consumer side:
# --------------------
#   class myAlarmConsumer()
#       def __init__(self):
#        alarmHandler.regAlarmCb(ALARM_CRITICALITY_A, alarmHandler.myCriticalAlarms):
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
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
from distutils.command.install_egg_info import safe_name
import os
import sys
from typing import List, Callable, Any
import time
from datetime import datetime
import threading
from xmlrpc.client import MAXINT
from PyQt5 import QtCore
from PyQt5 import Qt, QtGui
from PyQt5.QtGui import QFont, QColor, QTextCursor, QIcon
from copy import deepcopy



#################################################################################################################################################
# Class: alarm
# Purpose: alarm is a simple client side class of the alarm handler. It provides means for alarm producers to raise- and cease alarms which
#          will provide context to a statefull alarm, provide a description to it, will provide a uniqe alarm Id to it, etc.
#
# Public Methods and objects:
#============================
# Public data-structures:
# ---------------------
# -
#
# Public methods:
# -------------
# - __init__(<mo> : Any, <type> : str, <src> : str, <criticality> int, <slogan> : str)
#                                           Alarm object constructor: <mo> the object being source of this alarm
#                                                                     <type>: The alarm type identifier
#                                                                     <src>: The source identifier for this alarm
#                                                                     <criticality>: The criticality of this alarm:
#                                                                           ALARM_CRITICALITY_A | ALARM_CRITICALITY_B | ALARM_CRITICALITY_C
#                                                                     <slogan>: A generic/static alarm slogan (This will later be enriched
#                                                                           with detailed raise and cease causes)
# -raiseAlarm(<contextDescription : str = "">) -> int  | None:
#                                           Raise an alarm with an raise cause: <contextDescription>, returns the alarm instance ID (if 0, )
# -ceaseAlarm(<contextDescription : str = "">) -> None:
#                                           Ceases an alarm with cease cause <contextDescription>
# -updateAlarmSrc(<src> : str) -> None:     Updates the alarm src description
# -squelshAlarm(<squelsh> : bool = True) -> None:
#                                           (Un)Squelshes the alarm, if squelshed it will still sit in the alarm list
#                                               but will not be shown
# -admDisableAlarm() -> int | None          Administratively disables the alarm, to be used when a object being the source for the alarm
#                                               is administratively disabled. This ceases the alarm to become part of the alarm history list,
#                                               and creates a new squelshed alarm, part of the active alarm list. Interaction should continue
#                                               with the new alarm, so accurate updated alarm information is shown whenever the source object
#                                               is administratively enabled. Returns the instance ID of the new alarm.
# -admEnableAlarm() -> None                 Administratively enables the alarm, if the alarm is active it will show on the active alarm list
# -getGlobalAlarmClassObjUid() -> int | None:
#                                           Returns the Alarm Class identifier
# -getGlobalAlarmInstanceUid() -> int | None
#                                           Returns tha Alarm instance identifier
# -getType() -> str | None:                 Returns the alarm type
# -getSource() -> str | None:               Returns the alarm source
# -getSloganDescription() -> str | None:    Returns the Alarm slogan
# -getSeverity(<str> : bool = True) -> int | str | None:
#                                           Returns the Alarm severity, if <str> is True, the severity is returned as text ("A", "B", "C"),
#                                               otherwise in its integer definition
# -getIsActive() -> bool:                   Returs the alarm status (Active/Inactive)
# -getCurrentOriginActiveInstance(<history> : bool = False) -> int | None:
#                                           Returns the alarm instance ID that was the source for this alarm
# -getRaiseUtcTime() -> str | None:         Returns the time in YYYY-MM-DD HH:MM:SS.uS for when the alarm was raised
# -getRaiseEpochTime() -> float | None:     Returns the Epoch time (S)for when the alarm was raised
# -getRaiseReason(self) -> str | None:      Returns the reason for why the alarm was raised
# -getCeaseUtcTime() -> str | None:         Returns the time in YYYY-MM-DD HH:MM:SS.uS for when the alarm was ceased
# -getCeaseEpochTime() -> float | None:     Returns the Epoch time (S)for when the alarm was ceased
# -getCeaseReason(self) -> str | None:      Returns the reason for why the alarm was ceased
# -getLastingTime() -> str | None           Returns the lasting time for the alarm in YYYY-MM-DD HH:MM:SS.uS - once 
#                                               the alarm has been ceased
# -getLastingTimeS() -> float | None        Returns the lasting time for the alarm in Seconds - once 
#                                               the alarm has been ceased
# 
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_mo : str                                The alarm producer object name
# -_blockReferences : List[int]             A Stack of references to blocking origin of the alarm, if the alarm it self is not the origin
# -_blockReferences : int                   First origin alarm reference
# -_origin : bool                           Originating alarm, True if this is the originating alarm and not caused by another alarm (CBL)
# -_squelsh : bool                          Alarm squelshing
# -_admDisable : bool                       If alarm caused by object operatinal error state and the object is disabled
# -_type : str                              A static string that on a high level describes the alarm nature - E.g. Operational state down, 
#                                               or exsessive errors,...
#                                               This should follow a global convention - but it is outside the scope of this implementation.
# _src : str                                Should reflect the object ID with contextual information - eg: topDecoder:myDecoder:MySatLink1:mySat,
#                                               the contextual representation of the src's is outside the scope of this implementation.
# -_globalAlarmClassObjUid                  A global unique alarm identifier for a specific alarm class object
# -_globalAlarmInstanceUid:                 A unique alarm identifier assigned by the alarmHandler for a particular alarm event
# -_criticality:                            Alarm criticality - any of: ALARM_CRITICALITY_A, ALARM_CRITICALITY_B, ALARM_CRITICALITY_C:
#                                               - ALARM_CRITICALITY_A: Indicates that the service may be out of service all together
#                                               - ALARM_CRITICALITY_B  Indicates limitted service performance that may cause service interuption
#                                               - ALARM_CRITICALITY_C  Indicates peripherial service limitations that is likely not to impact
#                                                   the core business logic execution, but may impact system observability, LCM operations of the system,
# -_sloganDescription:                      A static high level description of the alarm (on slogan level), E.g. link down
# -_contextRaiseReason:                     A runtime/context based description on what may have lead up to this alarm, E.g. Excessive CRC errors
# -_contextCeaseReason:                     A runtime/context based description on what lead to the cease of this alarm
# -_active:                                 Indicates if the alarm is active (sits in the active alarm list), or ceased
#                                               (sits in the historical ceased alarm list)
# -_raiseEpochTime:                         Epoch time (s) when the alarm was raised
# -_raiseUtcTime:                           UTC time/timeOfDay when the alarm was raised in format: %Y-%m-%d %H:%M:%S.%f
# -_ceaseEpochTime:                         Epoch time (s) when the alarm was ceased
#.-_ceaseUtcTime:                           UTC time/timeOfDay when the alarm was ceased in format: %Y-%m-%d %H:%M:%S.%f
# -_lastingTimeS:                           Alarm lasting time in seconds
# -_lastingTime:                            Alarm lasting time in format: %Y-%m-%d %H:%M:%S.%f
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
ALARM_CRITICALITY_A : int =       0                                                 # Indicates service out of order
ALARM_CRITICALITY_B : int =       1                                                 # Indicates limitted service performance
ALARM_CRITICALITY_C : int =       2                                                 # Indicates peripherial service limitations
ALARM_ALL_CRITICALITY : int =     255

class alarm:
    def __init__(self, p_mo : Any, p_type : str, p_src : str, p_criticality : int, p_slogan : str):
        self._mo : str = str(p_mo)                                                  # Owning managed object
        self._blockReferences : List[int] = []                                      # Stack of references to the blocking origin of the alarm
        self._blockReference : int | None = None                                    # First origin alarm reference
        self._origin : bool | None = False                                          # True if this is the originating alarm and not caused by another alarm (CBL)
        self._squelsh : bool | None = True                                          # Alarm squelshing
        self._admDisable : bool = True                                              # If alarm caused by object operatinal error state and the object is disabled
        self._type : str = p_type                                                   # Alarm type
        self._src : str = p_src                                                     # Source object ID
        self._globalAlarmClassObjUid : int | None = None                            # Global unique alarm class object id
        self._globalAlarmInstanceUid : int | None = None                            # Global unique alarm event id
        self._criticality : int = p_criticality                                     # Criticality
        self._sloganDescription : str | None = p_slogan                             # Static Alarm slogan decription
        self._contextRaiseReason : str | None = None                                # Specific information for this raise alarm context/occurance
        self._contextCeaseReason : str | None = None                                # Specific information for this cease alarm context/occurance
        self._active : bool = False                                                 # Alarm is active or not
        self._raiseEpochTime : float | None = None                                  # Epoch time (s) for raising the alarm
        self._raiseUtcTime : str | None = None                                      # Date and time of day for raising the alarm
        self._ceaseEpochTime : float | None = None                                  # Epoch time time for ceasing the alarm
        self._ceaseUtcTime : str | None = None                                      # Date and time of day for ceasing the alarm
        self._lastingTimeS : float | None = None                                    # Time period the alarm lasted for in seconds
        self._lastingTime : str | None = None                                       # Time period the alarm lasted for in %Y-%m-%d %H:%M:%S.%f
        self._globalAlarmClassObjUid = alarmHandler._regGlobalAlarmClassObjUid(self)

    def raiseAlarm(self, p_contextDescription : str = "", p_reference : int = None, p_origin : bool = True) -> int | None:
        if self._active:
            if not p_origin and p_reference != None:
                self._blockReferences.append(p_reference)
            return None
        self._active = True
        if self._admDisable:
            self.squelshAlarm()
        if p_reference != None:
            self._blockReference = p_reference
            if not p_origin:
                self._blockReferences.append(p_reference)
        self._origin = p_origin
        self._raiseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._raiseEpochTime = datetime.utcnow().timestamp()
        self._ceaseTime = None
        self._lastingTime = None
        self._contextRaiseReason = p_contextDescription
        self._globalAlarmInstanceUid = alarmHandler._raiseAlarm(self)
        return self._globalAlarmInstanceUid

    def ceaseAlarm(self, p_contextDescription : str = "") -> None:
        if not self._active:
            return
        self._active = False
        self._ceaseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._ceaseEpochTime = datetime.utcnow().timestamp()
        self._lastingTimeS = self._ceaseEpochTime - self._raiseEpochTime
        self._lastingTime = time.strftime('%H:%M:%S', time.gmtime(self._lastingTimeS))
        self._contextCeaseReason = p_contextDescription
        alarmHandler._ceaseAlarm(deepcopy(self))
        self._contextRaiseReason = None
        self._raiseUtcTime = None
        self._raiseEpochTime = None
        self._contextCeaseReason = None
        self._ceaseUtcTime = None
        self._ceaseEpochTime = None
        self._lastingTimeS = None
        self._lastingTime = None

    def updateAlarmSrc(self, p_src : str) -> None:
        self._src = p_src
        alarmHandler._updateAlarmSrc(self)

    def squelshAlarm(self, p_squelsh = True) -> None:
        self._squelsh = p_squelsh
        alarmHandler._squelshAlarm(self)

    def admDisableAlarm(self) -> int | None:
        if self._active:
            contextRaiseReason = self._contextRaiseReason
            blockReference = self._blockReference
            origin = self._origin
            self.ceaseAlarm("Object administratively disabled, ceasing all active alarms")
            instanceId = self.raiseAlarm(contextRaiseReason, blockReference, "Object administratively disabled, ceasing all active alarms")
            self._origin = origin
        self._admDisable = True
        self.squelshAlarm()
        try:
            return instanceId
        except:
            return None

    def admEnableAlarm(self) -> None:
        if self._active:
            self._raiseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
            self._raiseEpochTime = datetime.utcnow().timestamp()
        self._admDisable = False
        self.squelshAlarm(False)

    def getGlobalAlarmClassObjUid(self) -> int | None:
        return self._globalAlarmClassObjUid

    def getGlobalAlarmInstanceUid(self) -> int | None:
        return self._globalAlarmInstanceUid

    def getType(self) -> str | None:
        return self._type

    def getSource(self) -> str | None:
        return self._src

    def getSloganDescription(self) -> str | None:
        return self._sloganDescription

    def getSeverity(self, p_str : bool = True) -> int | str | None:
        if not p_str:
            return self._criticality
        else:
            if self._criticality == ALARM_CRITICALITY_A:
                return "A"
            elif self._criticality == ALARM_CRITICALITY_B:
                return "B"
            elif self._criticality == ALARM_CRITICALITY_C:
                return "C"
            else:
                #PANIC
                pass

    def getIsActive(self) -> bool:
        return self._active

    def getCurrentOriginActiveInstance(self, p_history : bool = False) -> int:
        return alarmHandler.getCurrentOriginActiveInstance(self, p_history)

    def getRaiseUtcTime(self) -> str | None:
        return self._raiseUtcTime

    def getRaiseEpochTime(self) -> float | None:
        return self._raiseEpochTime

    def getRaiseReason(self) -> str | None:
        return self._contextRaiseReason

    def getCeaseUtcTime(self) -> str | None:
        return self._ceaseUtcTime

    def getCeaseEpochTime(self) -> float | None:
        return self._ceaseEpochTime

    def getCeaseReason(self) -> str | None:
        return self._contextCeaseReason

    def getLastingTime(self) -> str | None:
        return self._lastingTime

    def getLastingTimeS(self) -> float | None:
        return self._lastingTimeS



#################################################################################################################################################
# Class: alarmHandler
# Purpose: A simple server-side static alarm class that keeps track of active - and inactive alarm lists generated from different alarm sources
#          and with  different alarm criticalities. It depends on the client-side alarm class further above which basically defines the producer
#          API for the alarm handler. The alarmHandler class it self - does not provide any visualization - that is left for the consumers of the
#          class.
#          However it provides means for consuming the alarm situation through requests and call-backs. Three alarm criticalities are supported
#          (chosen from a simplicity view):
#          - ALARM_CRITICALITY_A: Indicates that the opertations may be out of service all together
#          - ALARM_CRITICALITY_B  Indicates limitted opertions performance that may cause service interuption
#          - ALARM_CRITICALITY_C  Indicates peripherial operations limitations that is likely not to impact the core business logic execution,
#                                 but may impact system observability, LCM operations of the system, etc.
#          alarmHandler is an ephemereal class, thus it does not provide any retention of the alarm lists in between sessions/reboots.
#
# Public Methods and objects:
# ==========================
# Public data-structures:
# -----------------------
# -
#
# Public Methods:
# ---------------
# -start() -> None                                      Starts and initiates the alarmHandler
# -regAlarmCb(<criticality> : int, <callBack> : Callable[[List[alarm], Any], None], <metaData> : Any = None) -> None:
#                                                       Registers a callback which is called whenever the number of active alarms for the given
#                                                       criticality is changed.
# -unRegAlarmCb(<criticality> : int, <callBack> : Callable[[List[alarm], Any], None]) -> None:
#                                                       Unregisters a callback for a given alarm criticality.
# -getAlarmClassObjects() -> List[alarm]:               Returns a list of all registered alarms
# -getCurrentOriginActiveInstance(<alarmObject> : alarm, <history> : bool = False) -> int:
#                                                       Returns the current originating Alarm instance ID for the <alarmObject>, if history is set
#                                                           the search will happen both in the active alarm list as well as in the ceased alarm
#                                                           list, otherwise the search only happens in the active alarm list
# -getNoOfActiveAlarms(<criticality> : int) -> int:     Returns the number of active alarms for the given alarm <criticality>
# -getActiveAlarmListPerCriticality(<criticality> : int) -> List[alarm]
#                                                       Returns a list of all active alarm objects in time order for the given alarm <criticality>
# -getActiveAlarmListTimeOrder() -> List[alarm]         Returns a list of all active alarm objects in time order - no matter of the alarm criticality
# -getCeasedAlarmListPerCriticality(<criticality> : int) -> List[alarm]:
#                                                       Returns a list of ceased alarm objects in time order for the given alarm <criticality>
# -getCeasedAlarmListTimeOrder(<criticality> : int, <ceaseTimeOrder> : bool = false) -> List[alarm]
#                                                       Returns a list of ceased alarm objects in time order no matter of the alarm p_criticality.
#                                                           If <ceaseTimeOrder> is set to True, the sorting is based on the cease time of the alarm,
#                                                           otherwise the raise time is used for sorting.
# -getSrcs(<history> : bool = False) -> List[str]       Returns a list of all alarm sources for either active alarms (<history> = False) or ceased alarms
#                                                           (<history> = True)
# - getAlarmObjHistory() -> List[List[int]]:            Returns a list of lists representing the history of raised alarms [AlarmClassObjUid][AlarmInstanceUid]
#
# Private Methods and objects only to be used internally:
# =======================================================
# Private data-structures:
# ------------------------
# _alarmHandlerLock : threading.Lock                    Alarm lock
# _alarmClassObjectInventory: List[alarm]               All registered alarsms
# _activeSrcs : List[str]                               Active alarm sources
# _historySrcs : List[str]                              Ceased alarm sources
# _activeAlarms : List[List[alarm]]                     Active alarms per criticality
# _ceasedAlarms : List[List[alarm]]                     Ceased alarms per criticality
#._alarmObjHistory : List[List[alarm]]                  Holds a list of active and historical alarms for each alarm class object [AlarmClassObjUid][AlarmInstanceUid]
# _cb : List[List[Callable[[int, int, Any], None]]]     List of alarm call-backs, callback prototype: cb(<criticality>, <noOfAlarms>, <metaData>)
# _globalAlarmClassObjUid : int                         Alarm class object identifier
# _globalAlarmInstanceUid : int                         Alarm instance identifier
#
# Private Methods:
# ----------------
# -_regGlobalAlarmClassObjUid(<alarmObject> : alarm) -> int:
#                                                       Register a new alarm object to be managed by the alarmHandler, return value is the 
#                                                           global Alarm Object ID
# -_raiseAlarm(<alarmObject> : alarm) -> int:           Raise a new alarm, return value is the global Alarm Instance ID
# -_ceaseAlarm(<alarmObject> : alarm) -> None:          Ceases an active alarm
# -_squelshAlarm(<alarmObject> : alarm) -> None:        Squelshes an active alarm, moves it from the active alarm list to the ceased alarm list
#                                                           and squelshes it from visability
# -_updateAlarmSrc(<alarmObject> : alarm) -> None       Updates the <alarmObject> meta-data through-out the active- and ceased alarm lists
# - _sendCb(<criticality>) -> None                      Traverses all registered callback requests for a given <criticality> and sends an 
#                                                           update on the alarm status
#################################################################################################################################################
class alarmHandler:
    def start() -> None:
        alarmHandler._alarmHandlerLock = threading.Lock()
        alarmHandler._alarmClassObjectInventory: List[alarm] = []
        alarmHandler._activeSrcs : List[str] = []
        alarmHandler._historySrcs : List[str] = []
        alarmHandler._activeAlarms : List[List[alarm]] = []
        alarmHandler._ceasedAlarms : List[List[alarm]] = []
        alarmHandler._alarmObjHistory : List[List[alarm]] = []
        alarmHandler._cb : List[List[Callable[[int, int, Any], None]]] = []
        alarmHandler.CB = 0
        alarmHandler.CB_METADATA = 1
        alarmHandler._globalAlarmClassObjUid : int = 0
        alarmHandler._globalAlarmInstanceUid : int = 0
        for criticalityItter in range(3):
            alarmHandler._activeAlarms.append([])
            alarmHandler._ceasedAlarms.append([])
            alarmHandler._cb.append([])

    def _regGlobalAlarmClassObjUid(p_alarmObj : alarm) -> int:
        with alarmHandler._alarmHandlerLock:
            alarmHandler._alarmClassObjectInventory.append(p_alarmObj)
            alarmHandler._globalAlarmClassObjUid += 1
            alarmHandler._alarmObjHistory.append([])
            return alarmHandler._globalAlarmClassObjUid

    def regAlarmCb(p_criticality : int, p_cb : Callable[[List[alarm], Any], None], p_metaData : Any = None) -> None:
        with alarmHandler._alarmHandlerLock:
            alarmHandler._cb[p_criticality].append([p_cb, p_metaData])
        p_cb(p_criticality, len(alarmHandler._activeAlarms[p_criticality]), p_metaData) #SHOULD WE ALSO PROVIDE CEASED ALARM LIST?

    def unRegAlarmCb(p_criticality : int, p_cb : Callable[[List[alarm], Any], None]) -> None:
        with alarmHandler._alarmHandlerLock:
            for cbItter in range(0, len(alarmHandler._cb[p_criticality])):
                if alarmHandler._cb[p_criticality][cbItter][alarmHandler.CB] == p_cb:
                    del alarmHandler._cb[p_criticality][cbItter]

    def _raiseAlarm(p_alarmObj : alarm) -> int:
        with alarmHandler._alarmHandlerLock:
            try:
                alarmHandler._activeSrcs.index(p_alarmObj._src)
            except:
                alarmHandler._activeSrcs.append(p_alarmObj._src)
                alarmHandler._activeSrcs.sort()
            for alarmObjItter in alarmHandler._activeAlarms[p_alarmObj._criticality]:
                if alarmObjItter._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                    return 0
            alarmHandler._globalAlarmInstanceUid += 1
            p_alarmObj._globalAlarmInstanceUid = alarmHandler._globalAlarmInstanceUid
            alarmHandler._activeAlarms[p_alarmObj._criticality].append(p_alarmObj)
            alarmHandler._alarmObjHistory[p_alarmObj._globalAlarmClassObjUid - 1].append(deepcopy(p_alarmObj))
        alarmHandler._sendCb(p_alarmObj._criticality)
        return alarmHandler._globalAlarmInstanceUid

    def _ceaseAlarm(p_alarmObj : alarm) -> None:
        with alarmHandler._alarmHandlerLock:
            try:
                alarmHandler._historySrcs.index(p_alarmObj._src)
            except:
                alarmHandler._historySrcs.append(p_alarmObj._src)
                alarmHandler._historySrcs.sort()
            for alarmObjItter in range(0, len(alarmHandler._activeAlarms[p_alarmObj._criticality])):
                if alarmHandler._activeAlarms[p_alarmObj._criticality][alarmObjItter]._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                    del alarmHandler._activeAlarms[p_alarmObj._criticality][alarmObjItter]
                    alarmHandler._ceasedAlarms[p_alarmObj._criticality].append(p_alarmObj)
                    break
            found = False
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                for alarmObjItter in alarmHandler._activeAlarms[criticalityItter]:
                    if alarmObjItter._src == p_alarmObj._src:
                        found = True
                        break
            if not found:
                try:
                    alarmHandler._activeSrcs.remove(p_alarmObj._src)
                except:
                    pass
        alarmHandler._sendCb(p_alarmObj._criticality)
        return

    def _squelshAlarm(p_alarmObj : alarm) -> None:
        if p_alarmObj._active:
            if p_alarmObj._squelsh:
                try:
                    alarmHandler._activeSrcs.remove(p_alarmObj._src)
                except:
                    pass
            else:
                try:
                    alarmHandler._activeSrcs.index(p_alarmObj._src)
                except:
                    alarmHandler._activeSrcs.append(p_alarmObj._src)
            alarmHandler._activeSrcs.sort()
        else:
            if p_alarmObj._squelsh:
                try:
                    alarmHandler._historySrcs.remove(p_alarmObj._src)
                except:
                    pass
            else:
                try:
                    alarmHandler._historySrcs.index(p_alarmObj._src)
                except:
                    alarmHandler._historySrcs.append(p_alarmObj._src)
            alarmHandler._historySrcs.sort()
        alarmHandler._sendCb(p_alarmObj._criticality)

    def _updateAlarmSrc(p_alarmObj : alarm) -> None:
        with alarmHandler._alarmHandlerLock:
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                for alarmObjItter in range(0, len(alarmHandler._ceasedAlarms[criticalityItter])):
                    if alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter]._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                        alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter]._src = p_alarmObj._src
                        return

    def getAlarmClassObjects() -> List[alarm]:
        return alarmHandler._alarmClassObjectInventory

    def getCurrentOriginActiveInstance(p_alarmObj : alarm, p_history : bool = False) -> int:
        if p_alarmObj._origin:
            return 0
        blockReferences = p_alarmObj._blockReferences
        blockReferences.sort()
        if not p_history:
            if len(p_alarmObj._blockReferences) == 0:
                return 0
        for referenceItter in range(0, len(blockReferences)):
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                for alarmObjItter in alarmHandler._activeAlarms[criticalityItter]:
                    if alarmObjItter._origin and not alarmObjItter._squelsh and alarmObjItter._blockReference == blockReferences[referenceItter]:
                        return alarmObjItter._globalAlarmInstanceUid
        if p_history:
            activeAlarmObjMatch = None
            found = False
            for referenceItter in range(0, len(blockReferences)):
                for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                    for alarmObjItter in alarmHandler._activeAlarms[criticalityItter]:
                        if alarmObjItter._origin and not alarmObjItter._squelsh and alarmObjItter._blockReference == blockReferences[referenceItter]:
                            activeAlarmObjMatch = alarmObjItter
                            found = True
                            break
                    if found:
                        break
                if found:
                    break
            ceasedalarmObjMatch = None
            found = False
            for referenceItter in range(0, len(blockReferences)):
                for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                    for alarmObjItter in alarmHandler._ceasedAlarms[criticalityItter]:
                        if alarmObjItter._origin and not alarmObjItter._squelsh and alarmObjItter._blockReference == blockReferences[referenceItter]:
                            ceasedalarmObjMatch = alarmObjItter
                            found = True
                            break
                    if found:
                        break
                if found:
                    break
            if activeAlarmObjMatch == None and ceasedalarmObjMatch == None:
                return 0
            elif activeAlarmObjMatch == None:
                return ceasedalarmObjMatch._globalAlarmInstanceUid
            elif ceasedalarmObjMatch == None:
                return activeAlarmObjMatch._globalAlarmInstanceUid
            elif activeAlarmObjMatch._blockReference < ceasedalarmObjMatch._blockReference:
                return activeAlarmObjMatch._globalAlarmInstanceUid
            elif activeAlarmObjMatch._blockReference > ceasedalarmObjMatch._blockReference:
                return ceasedalarmObjMatch._globalAlarmInstanceUid
            else:
                return 0
        return 0

    def getNoOfActiveAlarms(p_criticality : int) -> int:
        with alarmHandler._alarmHandlerLock:
            activeAlarmsNo = 0
            for alarmItter in alarmHandler._activeAlarms[p_criticality]:
                if not alarmItter._squelsh:
                    activeAlarmsNo += 1
            return activeAlarmsNo

    def getActiveAlarmListPerCriticality(p_criticality : int) -> List[alarm]:
        with alarmHandler._alarmHandlerLock:
            return alarmHandler._activeAlarms[p_criticality]

    def getActiveAlarmListTimeOrder():
        with alarmHandler._alarmHandlerLock:
            alarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                for alarmObjItter in alarmHandler._activeAlarms[criticalityItter]:
                    if len(alarmTimeOrder) == 0:
                        alarmTimeOrder.append(alarmObjItter)
                    else:
                        for alarmTimeOrderItter in range(0, len(alarmTimeOrder) + 1):
                            if alarmTimeOrderItter == len(alarmTimeOrder):
                                alarmTimeOrder.append(alarmObjItter)
                                break
                            if alarmObjItter._raiseEpochTime < alarmTimeOrder[alarmTimeOrderItter]._raiseEpochTime:
                                alarmTimeOrder.insert(alarmTimeOrderItter, alarmObjItter)
                                break
            return alarmTimeOrder

    def getCeasedAlarmListPerCriticality(p_criticality : int):
        with alarmHandler._alarmHandlerLock:
            return alarmHandler._ceasedAlarms[p_criticality]

    def getCeasedAlarmListTimeOrder(p_ceaseTimeOrder : bool = False):
        with alarmHandler._alarmHandlerLock:
            alarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
                for alarmObjItter in alarmHandler._ceasedAlarms[criticalityItter]:
                    if len(alarmTimeOrder) == 0:
                        alarmTimeOrder.append(alarmObjItter)
                    else:
                        for alarmTimeOrderItter in range(0, len(alarmTimeOrder) + 1):
                            if alarmTimeOrderItter == len(alarmTimeOrder):
                                alarmTimeOrder.append(alarmObjItter)
                                break
                            if p_ceaseTimeOrder:
                                if alarmObjItter._ceaseEpochTime < alarmTimeOrder[alarmTimeOrderItter]._ceaseEpochTime:
                                    alarmTimeOrder.insert(alarmTimeOrderItter, alarmObjItter)
                                    break
                            else:
                                if alarmObjItter._raiseEpochTime < alarmTimeOrder[alarmTimeOrderItter]._raiseEpochTime:
                                    alarmTimeOrder.insert(alarmTimeOrderItter, alarmObjItter)
                                    break
            return alarmTimeOrder

    def getSrcs(p_history : bool = False) -> List[str]:
        if p_history:
            return alarmHandler._historySrcs
        else:
            return alarmHandler._activeSrcs

    def getAlarmObjHistory() -> List[List[int]]:
        return alarmHandler._alarmObjHistory

    def _sendCb(p_criticality : int) -> None:
        for cbItter in alarmHandler._cb[p_criticality]:
            cbItter[alarmHandler.CB](p_criticality, len(alarmHandler._activeAlarms[p_criticality]), cbItter[alarmHandler.CB_METADATA])



#################################################################################################################################################
# Class: alarmTableModel
# Purpose: alarmTableModel is a QAbstractTable model for alarms. It provides the capabilities to represent alarms in a QTableView table.
#
# Public Methods and objects to be used by the alarm producers:
# =============================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent> : Any, <alarmHistory> : bool = False, <criticality> : int = ALARM_ALL_CRITICALITY)
#                                           Initialize an alarmTableModel
# -setAlarmHistory(<alarmHistory> : bool) -> None:
#                                           Sets the viewed alarm history (Active/History)
# -getAlarmHistory() -> bool:               Returns viewed alarm history
# -setAlarmCriticality(self, <criticality> : int) -> None:
#                                           Sets viewed alarm criticality
# -getAlarmCriticality() -> int             Returns the viewed alarm criticality
# -setSrcFilter(self, <srcFilter> : str) -> None:
#                                           Sets the viewed alarm source filter
# -getSrcFilter() -> str                    Returns the viewed alarm source filter
# -setFreeSearchFilter(<freeSearchFilter> : str) -> None:
#                                           Sets the freesearch regex filter
# -getFreeSearchFilter() -> str:            Returns current freesearch regex filter
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId                  Used to identify column 0 key information
# -getAlarmObjFromInstanceId(<alarmInstanceId> : int) -> int | None:
#                                           Returns the alarm class object ID for which the <alarmInstanceId> belongs to
# -formatAlarmTable(<alarmHistory> : bool, <criticality> : int, <srcFilter> : str, <freeSearchFilter> : str, 
#                   <verbosity> : bool = False) -> None:
#                                           Returns a list of formatted alarm table rows
# -formatSelectedAlarmsCsv() -> str         Format the viewed (filtered) alarms into a csv table string
# -formatAllAlarmsCsv() -> str              Format all alarms (no filtered) into a �csv table
# -rowCount(<parent> : Any = Qt.QModelIndex()) -> int:
#                                           Returns the current table view row count
# -columnCount(<parent> : Any = Qt.QModelIndex()) -> int
#                                           Returns the current table view column count
# -headerData(<section> : Any, <orientation> : Any, <role> : Any, <verbosity> : bool = False) -> List[str] | None:
#                                           Returns the alarm table header columns
# -data(<index> : Any, <role> : Any = QtCore.Qt.DisplayRole) -> str | Any:
#                                           Returns alarm table cell data
# -<*>Col(<verbosity> : bool = False) -> int
#                                           Provides the coresponding column index as provided with formatAlarmTable()
# 
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
# -_parent : Any                            Calling UI object
# -_alarmHistory : bool                     Alarm history filter
# -_criticality : int                       Alarm criticality filter
# -_srcFilter                               Alarm source filter
# -_freeSearchFilter                        Alarm free search regex filter
#.-_alarmTable                              Viewed (filtered) alarm table
# -_colCnt                                  Alarm table column count
# -_rowCnt                                  Alarm table row count
#
# Private methods:
# ----------------
# -_onAlarmChange(self, p_criticality : int , p_noOfAlarms : int, p_metaData : Any) -> None:
#                                           Alarm change call back trigerring alarm table update
# -_reLoadData() -> None                    Reload alarm content
#################################################################################################################################################
class alarmTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent : Any, p_alarmHistory : bool = False, p_criticality : int = ALARM_ALL_CRITICALITY):
        self.alarmTableReloadLock : threading.Lock = threading.Lock()
        self._parent : Any = p_parent
        self._alarmHistory : bool = p_alarmHistory
        self._criticality : int = p_criticality
        self._srcFilter : int = "*"
        self._freeSearchFilter : int = "*"
        self._alarmTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_A, self._onAlarmChange)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_B, self._onAlarmChange)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_C, self._onAlarmChange)
        self._reLoadData()

    def setAlarmHistory(self, p_alarmHistory : bool) -> None:
        self._alarmHistory = p_alarmHistory
        self._reLoadData()

    def getAlarmHistory(self) -> bool:
        return self._alarmHistory

    def setAlarmCriticality(self, p_criticality : int) -> None:
        self._criticality = p_criticality
        self._reLoadData()

    def getAlarmCriticality(self) -> int:
        return self._criticality

    def setSrcFilter(self, p_srcFilter : str) -> None:
        self._srcFilter = p_srcFilter
        self._reLoadData()

    def getSrcFilter(self) -> str:
        return self._srcFilter

    def setFreeSearchFilter(self, p_freeSearchFilter : str) -> None:
        self._freeSearchFilter = p_freeSearchFilter

    def getFreeSearchFilter(self) -> str:
        return self._freeSearchFilter

    def _onAlarmChange(self, p_criticality : int , p_noOfAlarms : int, p_metaData : Any) -> None:
        self._reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return False

    def isFirstColumnInstanceId(self) -> bool:
        return True

    def getAlarmObjFromInstanceId(self, p_alarmInstanceId : int) -> int | None:
        for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C + 1):
            for alarmItter in alarmHandler._activeAlarms[criticalityItter]:
                if alarmItter._globalAlarmInstanceUid == p_alarmInstanceId:
                    return alarmItter
            for alarmItter in alarmHandler._ceasedAlarms[criticalityItter]:
                if alarmItter._globalAlarmInstanceUid == p_alarmInstanceId:
                    return alarmItter
        return None

    def _reLoadData(self) -> None:
        with self.alarmTableReloadLock:
            self._alarmTable = self.formatAlarmTable(self._alarmHistory, self._criticality, self._srcFilter, self._freeSearchFilter)
            try:
                self._colCnt = len(self._alarmTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._alarmTable)
        self._parent.alarmsTableView.resizeColumnsToContents()
        self._parent.alarmsTableView.resizeRowsToContents()

    def formatAlarmTable(self, p_alarmHistory : bool, p_criticality : int, p_srcFilter : str, p_freeSearchFilter : str, p_verbosity : bool = False) -> None:
        alarmTable = []
        if p_criticality != ALARM_ALL_CRITICALITY:
            if p_alarmHistory:
                alarmList = alarmHandler.getCeasedAlarmListPerCriticality(p_criticality)
            else:
                alarmList = alarmHandler.getActiveAlarmListPerCriticality(p_criticality)
        else:
            if p_alarmHistory:
                alarmList = alarmHandler.getCeasedAlarmListTimeOrder()
            else:
                alarmList = alarmHandler.getActiveAlarmListTimeOrder()
        for alarmListItter in alarmList:
            if alarmListItter._squelsh:
                continue
            if p_srcFilter != "*" and alarmListItter._src != p_srcFilter:
                continue
            row = []
            row.append(str(alarmListItter._globalAlarmInstanceUid))
            if alarmListItter._criticality == ALARM_CRITICALITY_A:
                row.append("A")
            elif alarmListItter._criticality == ALARM_CRITICALITY_B:
                row.append("B")
            elif alarmListItter._criticality == ALARM_CRITICALITY_C:
                row.append("C")
            if p_verbosity:
                if alarmListItter._active:
                    row.append("True")
                else:
                    row.append("False")
            row.append(alarmListItter._raiseUtcTime)
            if p_verbosity:
                row.append(str(alarmListItter._raiseEpochTime))
            if p_verbosity:
                row.append(alarmListItter._contextRaiseReason)
            try:
                if alarmListItter._ceaseUtcTime == None or alarmListItter._ceaseUtcTime == "":
                    row.append("-")
                else:
                    row.append(alarmListItter._ceaseUtcTime)
            except:
                row.append("-")
            if p_verbosity:
                try:
                    if alarmListItter._ceaseEpochTime == None or alarmListItter._ceaseEpochTime == "":
                        row.append("-")
                    else:
                        row.append(str(alarmListItter._ceaseEpochTime))
                except:
                    row.append("-")
            if p_verbosity:
                try:
                    if alarmListItter._contextCeaseReason == None or alarmListItter._contextCeaseReason == "":
                        row.append("-")
                    else:
                        row.append(alarmListItter._contextCeaseReason)
                except:
                    row.append("-")
            try:
                if alarmListItter._lastingTime == None or alarmListItter._lastingTime == "":
                    row.append("-")
                else:
                    row.append(alarmListItter._lastingTime)
            except:
                row.append("-")
            if p_verbosity:
                try:
                    if alarmListItter._lastingTimeS == None or alarmListItter._lastingTimeS == "-":
                        row.append("-")
                    else:
                        row.append("{:.6f}".format(alarmListItter._lastingTimeS))
                except:
                    row.append("-")
            row.append(alarmListItter._type)
            row.append(alarmListItter._src)
            row.append(alarmListItter._sloganDescription)
            if p_verbosity:
                row.append(str(alarmListItter._globalAlarmClassObjUid))
            if p_verbosity:
                parentAlarm = alarmListItter.getCurrentOriginActiveInstance(p_alarmHistory)
                if parentAlarm == 0:
                    row.append("-")
                else:
                    row.append(str(parentAlarm))
            alarmTable.append(row)
        return alarmTable

    def formatSelectedAlarmsCsv(self) -> str:
        alarmTable = []
        alarmHeading = []
        column = 0
        while True:
            try:
                alarmHeading.append(self.headerData(column, QtCore.Qt.Horizontal, QtCore.Qt.DisplayRole, p_verbosity = True))
                column += 1
            except:
                break
        alarmTable.append(alarmHeading)
        alarmTable.extend(self.formatAlarmTable(self.getAlarmHistory(), self.getAlarmCriticality(), self.getSrcFilter(), self.getFreeSearchFilter(), p_verbosity = True))
        alarmCsv = ""
        for row in alarmTable:
            for column in row:
                alarmCsv += "\"" + column + "\"" + ","
            alarmCsv = alarmCsv[:-1]
            alarmCsv += "\n"
        return alarmCsv

    def formatAllAlarmsCsv(self) -> str:
        alarmTable = []
        alarmHeading = []
        column = 0
        while True:
            try:
                alarmHeading.append(self.headerData(column, QtCore.Qt.Horizontal, QtCore.Qt.DisplayRole, p_verbosity = True))
                column += 1
            except:
                break
        alarmTable.append(alarmHeading)
        activeAlarmTable = self.formatAlarmTable(False, ALARM_ALL_CRITICALITY, "*", "*", p_verbosity = True)
        historyAlarmTable = self.formatAlarmTable(True, ALARM_ALL_CRITICALITY, "*", "*", p_verbosity = True)
        while True:
            if len(activeAlarmTable) > 0 and len(historyAlarmTable) > 0:
                if activeAlarmTable[0][alarmHeading.index("Raised[UTC]:")] < historyAlarmTable[0][alarmHeading.index("Raised[UTC]:")]:
                    alarmTable.append(activeAlarmTable[0])
                    activeAlarmTable.pop(0)
                else:
                    alarmTable.append(historyAlarmTable[0])
                    historyAlarmTable.pop(0)
            else:
                if len(activeAlarmTable) > 0:
                    alarmTable.extend(activeAlarmTable)
                    break
                if len(historyAlarmTable) > 0:
                    alarmTable.extend(historyAlarmTable)
                    break
                break
        alarmCsv = ""
        for row in alarmTable:
            for column in row:
                alarmCsv += "\"" + column + "\"" + ","
            alarmCsv = alarmCsv[:-1]
            alarmCsv += "\n"
        return alarmCsv

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self.alarmTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self.alarmTableReloadLock:
            return self._colCnt

    def headerData(self, section : Any, orientation : Any, role : Any, p_verbosity : bool = False)  -> List[str] | None:
        with self.alarmTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                if p_verbosity:
                    return ("Alarm instance ID:", "Severity:", "Active", "Raised[UTC]:", "Raised[EPOCH]", "Raise reason:", "Ceased[UTC]:", "Ceased[EPOCH]:", "Cease reason:", "Duration:", "Duration[s]:", "Type:", "Source:", "Slogan:", "Alarm object ID:", "Parent Alarm-instance ID")[section]
                else:
                    return ("Alarm instance ID:", "Severity:", "Raised[UTC]:", "Ceased[UTC]:", "Duration:", "Type:", "Source:", "Slogan:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole) -> str | Any:
        with self.alarmTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._alarmTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if self._alarmHistory:
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                elif self._alarmTable[row][self.severityCol()] == "A":
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                elif self._alarmTable[row][self.severityCol()] == "B":
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                elif self._alarmTable[row][self.severityCol()] == "C":
                    return QtGui.QBrush(QtGui.QColor('#FFC800'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignLeft

    def instanceIdCol(self, p_verbosity : bool = False) -> int: return 0
    def severityCol(self, p_verbosity : bool = False) -> int: return 1
    def activeCol(self, p_verbosity : bool = False) -> int: return 2 if p_verbosity else None
    def raiseTimeUtcCol(self, p_verbosity : bool = False) -> int: return 3 if p_verbosity else 2
    def raiseTimeEpochCol(self, p_verbosity : bool = False) -> int: return 4 if p_verbosity else None
    def raiseReasonCol(self, p_verbosity : bool = False) -> int: return 5 if p_verbosity else None
    def ceaseTimeUtcCol(self, p_verbosity : bool = False) -> int: return 6 if p_verbosity else 3
    def ceaseTimeEpochCol(self, p_verbosity : bool = False) -> int: return 7 if p_verbosity else None
    def ceaseReasonCol(self, p_verbosity : bool = False) -> int: return 8 if p_verbosity else None
    def durationCol(self, p_verbosity : bool = False) -> int: return 9 if p_verbosity else 4
    def durationSCol(self, p_verbosity : bool = False) -> int: return 10 if p_verbosity else None
    def typeCol(self, p_verbosity : bool = False) -> int: return 11 if p_verbosity else 5
    def sourceCol(self, p_verbosity : bool = False) -> int: return 12 if p_verbosity else 6
    def sloganCol(self, p_verbosity : bool = False) -> int: return 13 if p_verbosity else 7
    def objIdCol(self, p_verbosity : bool = False) -> int: return 14 if p_verbosity else None
    def parentAlarmInstanceCol(self, p_verbosity : bool = False) -> int: return 15 if p_verbosity else None



#################################################################################################################################################
# Class: alarmInventoryTableModel
# Purpose: alarmInventoryTableModel is a QAbstractTable model for registered alarm inventory table model. It provides the capabilities to
# represent a registered alarms inventory in a QTableView table.
#
# Public Methods and objects to be used by the alarm producers:
# =============================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent>)
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId() -> bool        Used to identify column 0 key information
# -getAlarmObjFromObjId(<alarmObjectId> : int) -> alarm:
#                                           Returns the alarmobject from the alarm object ID
# -formatAlarmInventoryTable() -> List[List[str]]:
#                                           Formats the Alarm Inventory table: _alarmInventoryTable, and returns it.
# -rowCount(<parent> : Any = Qt.QModelIndex()) -> int:
#                                           Returns the current table view row count
# -columnCount(<parent> : Any = Qt.QModelIndex()) -> int
#                                           Returns the current table view column count
# -headerData(<section> : Any, <orientation> : Any, <role> : Any, <verbosity> : bool = False) -> List[str] | None:
#                                           Returns the alarm table header columns
# -data(<index> : Any, <role> : Any = QtCore.Qt.DisplayRole) -> str | Any:
#                                           Returns alarm table cell data
# -<*>Col(<verbosity> : bool = False) -> int
#                                           Provides the coresponding column index as provided with formatAlarmInventoryTable()
# 
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_alarmInventoryTableReloadLock : threading.Lock:
#                                           Alarm inventory Re-load lock
# -_parent : Any                            Calling UI object
# -_alarmInventoryTable : List[List[str]]   Inventory list of lists [row][column]
# -_colCnt                                  Alarm table column count
# -_rowCnt                                  Alarm table row count
#
# Private methods:
# ----------------
# -_reLoadData() -> None                    Reload alarm inventory content
#################################################################################################################################################
class alarmInventoryTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent):
        self._alarmInventoryTableReloadLock  = threading.Lock()
        self._parent : Any = p_parent
        self._alarmInventoryTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        self._reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return True

    def isFirstColumnInstanceId(self) -> bool: 
        return False

    def getAlarmObjFromObjId(self, alarmObjectId : int) -> alarm:
        for alarmItter in alarmHandler._alarmClassObjectInventory:
            if alarmItter._globalAlarmClassObjUid == alarmObjectId:
                return alarmItter
        return None

    def _reLoadData(self) -> None:
        with self._alarmInventoryTableReloadLock:
            self._alarmInventoryTable = self.formatAlarmInventoryTable()
            try:
                self._colCnt = len(self._alarmInventoryTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._alarmInventoryTable)
        self._parent.alarmInventoryTableView.resizeColumnsToContents()
        self._parent.alarmInventoryTableView.resizeRowsToContents()

    def formatAlarmInventoryTable(self) -> List[List[str]]:
        self._alarmInventoryTable = []
        alarmInventoryList = alarmHandler._alarmClassObjectInventory
        for alarmInventoryItter in alarmInventoryList:
            alarmInventoryRow = []
            alarmInventoryRow.append(str(alarmInventoryItter._globalAlarmClassObjUid))
            alarmInventoryRow.append(alarmInventoryItter._type)
            if alarmInventoryItter._criticality == ALARM_CRITICALITY_A:
                alarmInventoryRow.append("A")
            elif alarmInventoryItter._criticality == ALARM_CRITICALITY_B:
                alarmInventoryRow.append("B")
            elif alarmInventoryItter._criticality == ALARM_CRITICALITY_C:
                alarmInventoryRow.append("C")
            alarmInventoryRow.append(alarmInventoryItter._src)
            alarmInventoryRow.append(alarmInventoryItter._sloganDescription)
            alarmInventoryRow.append(str(alarmInventoryItter._active))
            self._alarmInventoryTable.append(alarmInventoryRow)
        return self._alarmInventoryTable

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self._alarmInventoryTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()):
        with self._alarmInventoryTableReloadLock:
            return self._colCnt

    def headerData(self, section, orientation, role):
        with self._alarmInventoryTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                return ("Alarm Obj ID", "Type:", "Severity:", "Source:", "Slogan", "Active:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole)-> str | Any:
        with self._alarmInventoryTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._alarmInventoryTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if self._alarmInventoryTable[row][self.activeCol()] == "False":
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                elif self._alarmInventoryTable[row][self.severityCol()] == "A":
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                elif self._alarmInventoryTable[row][self.severityCol()] == "B":
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                elif self._alarmInventoryTable[row][self.severityCol()] == "C":
                    return QtGui.QBrush(QtGui.QColor('#FFC800'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignLeft
    def objIdCol(self) -> int: return 0
    def typeCol(self) -> int: return 1
    def severityCol(self) -> int: return 2
    def sourceCol(self) -> int: return 3
    def sloganCol(self) -> int: return 4
    def activeCol(self) -> int: return 5
