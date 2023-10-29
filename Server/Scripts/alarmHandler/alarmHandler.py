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
# The alarmHandler- and the alarm classes together form a lightweight alarm framework which keeps track of lists for active - and inactive alarms
# produced by an arbitrarhy alarm producer. An alarm consumer can subscribe to alarm events and can request the lists of active- and inactive
# alarms for presentation or otherwise. The alarm lists are volatile and will be gone after a restart of the alarmHandler static class object.
#
# Use pattern:
# ============
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
#       myAlarmClient.myAlarm.src = "topdecoder:decoder-1:satlink-1"
#       myAlarmClient.myAlarm.criticality = ALARM_CRITICALITY_A
#       myAlarmClient.myAlarm.sloganDescription = "Link failure"
#
#   def onOpStateLinkFailureCRC (p_unAvailable):
#       if p_unavailable: myAlarmClient.myAlarm.raiseAlarm("Excessive CRC Errors")
#       else:  myAlarmClient.myAlarm.ceaseAlarm()
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
#################################################################################################################################################
from distutils.command.install_egg_info import safe_name
import os
import sys
import time
from datetime import datetime
import threading
from PyQt5 import QtCore
from PyQt5 import Qt, QtGui
from PyQt5.QtGui import QFont, QColor, QTextCursor, QIcon
from copy import deepcopy

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
# Public Methods:
# - start()                                             Starts and initiates the alarmHandler
# - regAlarmCb(p_criticality, p_cb, p_metaData)         Registers a callback which is called whenever the number of active alarms for the given criticality
#                                                           is changed - the cb prototype looks like this: cb(criticality, numberOfActiveAlarms, p_metaData).
#   - unRegAlarmCb(p_criticality, p_cb)                 Unregisters a callback for a given alarm criticality.
#   - getNoOfActiveAlarms(p_criticality)                Returns number of active alarms for the given alarm criticality
#   - getActiveAlarmListPerCriticality(p_criticality)   Returns a list of active alarm objects (see further down on the alarm object
#                                                           class)  in time order for a given alarm criticality
#   - getActiveAlarmListTimeOrder()                     Returns a list of active alarm objects (see further down on the alarm object class) 
#                                                           in time order - no matter of the alarm criticality
#   - getCeasedAlarmListPerCriticality(p_criticality):  Returns a list of ceased alarm objects (see further down on the alarm object
#                                                           class) in time order for a given alarm criticality
#   - getCeasedAlarmListTimeOrder(p_criticality, p_ceaseTimeOrder = false) Returns a list of ceased alarm objects (see further
#                                                                               down on the alarm object class) in time order no matter
#                                                                               of the alarm p_criticality. If p_ceaseTimeOrder is set to True,
#                                                                               the sorting will be based on the cease time of the alarm,
#                                                                               otherwise the raise time is used for sorting.
#   - getSrcs(p_history = False)                        Returns a list of all alarm sources for either active alarms (p_history = False) or ceased alarms
#                                                           (p_history = True)
#   - getAlarmObjHistory()                              Returns a list of lists representing the history of raided alarms [AlarmClassObjUid][AlarmInstanceUid]
#
# Private Methods and objects only to be used internally:
# =======================================================
# Private data-structures:
# ------------------------
# _alarmHandlerLock                                     Alarm lock
# _activeSrcs                                           Active alarm sources
# _historySrcs                                          Ceased alarm sources
# _activeAlarms                                         Active alarms
# _ceasedAlarms                                         Ceased alarms
#._alarmObjHistory                                      Holds a list of active and historical alarms for each alarm class object [AlarmClassObjUid][AlarmInstanceUid]
# _cb                                                   List of alarm call-backs
# _globalAlarmClassObjUid                               Alarm class object UID counter
# _globalAlarmInstanceUid                               Alarm instance UID counter
#
# Private Methods:
# ----------------
#   - _regGlobalAlarmClassObjUid()                      Registers a new alarm object, returns the corresponding AlarmClassObjUid
#   - _raiseAlarm(p_alarmObj)                           Raises an alarm
#   - _ceaseAlarm(p_alarmObj)                           Ceases an alarm
#   - _updateAlarmMetaData(p_alarmObj)                  Updates the p_alarm object meta-data through-out the active- and ceased alarm lists
#   - _sendCb(criticality)                              Is a private method that traverses all registered callback requests and sends an update on the alarm
#                                                           situation
#################################################################################################################################################
class alarmHandler:
    def start():
        alarmHandler._alarmHandlerLock = threading.Lock()
        alarmHandler._activeSrcs = []
        alarmHandler._historySrcs = []
        alarmHandler._activeAlarms = []
        alarmHandler._ceasedAlarms = []
        alarmHandler._alarmObjHistory = []
        alarmHandler._cb = []
        alarmHandler.CB = 0
        alarmHandler.CB_METADATA = 1
        alarmHandler._globalAlarmClassObjUid = 0
        alarmHandler._globalAlarmInstanceUid = 0
        for criticalityItter in range(3):
            alarmHandler._activeAlarms.append([])
            alarmHandler._ceasedAlarms.append([])
            alarmHandler._cb.append([])

    def _regGlobalAlarmClassObjUid():
        with alarmHandler._alarmHandlerLock:
            alarmHandler._globalAlarmClassObjUid += 1
            alarmHandler._alarmObjHistory.append([])
            return alarmHandler._globalAlarmClassObjUid

    def regAlarmCb(p_criticality, p_cb, p_metaData = None):
        with alarmHandler._alarmHandlerLock:
            alarmHandler._cb[p_criticality].append([p_cb, p_metaData])
        p_cb(p_criticality, len(alarmHandler._activeAlarms[p_criticality]), p_metaData)

    def unRegAlarmCb(p_criticality, p_cb):
        with alarmHandler._alarmHandlerLock:
            for cbItter in range(0, len(alarmHandler._cb[p_criticality])):
                if alarmHandler._cb[p_criticality][cbItter][alarmHandler.CB] == p_cb:
                    del alarmHandler._cb[p_criticality][cbItter]

    def _raiseAlarm(p_alarmObj):
        with alarmHandler._alarmHandlerLock:
            try:
                alarmHandler._activeSrcs.index(p_alarmObj.src)
            except:
                alarmHandler._activeSrcs.append(p_alarmObj.src)
                alarmHandler._activeSrcs.sort()
            for alarmObjItter in alarmHandler._activeAlarms[p_alarmObj.criticality]:
                if alarmObjItter._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                    return 0
            alarmHandler._globalAlarmInstanceUid += 1
            p_alarmObj._globalAlarmInstanceUid = alarmHandler._globalAlarmInstanceUid
            alarmHandler._activeAlarms[p_alarmObj.criticality].append(p_alarmObj)
            alarmHandler._alarmObjHistory[p_alarmObj._globalAlarmClassObjUid].append(deepcopy(p_alarmObj))
        alarmHandler._sendCb(p_alarmObj.criticality)
        return alarmHandler._globalAlarmInstanceUid

    def _ceaseAlarm(p_alarmObj):
        with alarmHandler._alarmHandlerLock:
            try:
                alarmHandler._historySrcs.index(p_alarmObj.src)
            except:
                alarmHandler._historySrcs.append(p_alarmObj.src)
                alarmHandler._historySrcs.sort()
            for alarmObjItter in range(0, len(alarmHandler._activeAlarms[p_alarmObj.criticality])):
                if alarmHandler._activeAlarms[p_alarmObj.criticality][alarmObjItter]._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                    del alarmHandler._activeAlarms[p_alarmObj.criticality][alarmObjItter]
                    alarmHandler._ceasedAlarms[p_alarmObj.criticality].append(p_alarmObj)
                    break
            found = False
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
                for alarmObjItter in alarmHandler._activeAlarms[criticalityItter]:
                    if alarmObjItter.src == p_alarmObj.src:
                        found = True
                        break
            if not found:
                alarmHandler._activeSrcs.remove(p_alarmObj.src)
        alarmHandler._sendCb(p_alarmObj.criticality)
        return

    def _updateAlarmMetaData(p_alarmObj):
        with alarmHandler._alarmHandlerLock:
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
                for alarmObjItter in range(0, len(alarmHandler._activeAlarms[criticalityItter])):
                    if alarmHandler._activeAlarms[criticalityItter][alarmObjItter]._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                        alarmHandler._activeAlarms[criticalityItter][alarmObjItter].src = p_alarmObj.src
                        alarmHandler._activeAlarms[criticalityItter][alarmObjItter].sloganDescription = p_alarmObj.sloganDescription
                        alarmHandler._activeAlarms[criticalityItter][alarmObjItter].contextDescription = p_alarmObj.contextDescription
                        return
                for alarmObjItter in range(0, len(alarmHandler._ceasedAlarms[criticalityItter])):
                    if alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter]._globalAlarmClassObjUid == p_alarmObj._globalAlarmClassObjUid:
                        alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter].src = p_alarmObj.src
                        alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter].sloganDescription = p_alarmObj.sloganDescription
                        alarmHandler._ceasedAlarms[criticalityItter][alarmObjItter].contextDescription = p_alarmObj.contextDescription
                        return

    def getNoOfActiveAlarms(p_criticality):
        with alarmHandler._alarmHandlerLock:
            return len(alarmHandler._activeAlarms[p_criticality])

    def getActiveAlarmListPerCriticality(p_criticality):
        with alarmHandler._alarmHandlerLock:
            return alarmHandler._activeAlarms[p_criticality]

    def getActiveAlarmListTimeOrder():
        with alarmHandler._alarmHandlerLock:
            alarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
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

    def getCeasedAlarmListPerCriticality(p_criticality):
        with alarmHandler._alarmHandlerLock:
            return alarmHandler._ceasedAlarms[p_criticality]

    def getCeasedAlarmListTimeOrder(p_ceaseTimeOrder = False):
        with alarmHandler._alarmHandlerLock:
            alarmTimeOrder = []
            for criticalityItter in range(ALARM_CRITICALITY_A, ALARM_CRITICALITY_C):
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

    def getSrcs(p_history = False):
        if p_history:
            return alarmHandler._historySrcs
        else:
            return alarmHandler._activeSrcs

    def getAlarmObjHistory():
        return alarmHandler._alarmObjHistory

    def _sendCb(p_criticality): # DO WE NEED A LOCK FOR THIS, IS len atomic, is append and del atomic?
            for cbItter in alarmHandler._cb[p_criticality]:
                cbItter[alarmHandler.CB](p_criticality, len(alarmHandler._activeAlarms[p_criticality]), cbItter[alarmHandler.CB_METADATA])



#################################################################################################################################################
# Class: alarm
# Purpose: alarm is a simple client side class of the alarm handler. It provides means for alarm producers to raise- and cease alarms which
#          will provide context to a statefull alarm, provide a description to it, will provide a uniqe alarm Id to it, etc.
# Public Methods and objects to be used by the alarm producers:
# =============================================================
# Public data-structures:
# ---------------------
#   type:                                   A static string that on a high level describes the alarm nature - E.g. Operational state down, 
#                                               or exsessive errors, or...
#                                           This should follow a global convention - but it is outside the scope of this implementation.
#   src:                                    Should reflect the object ID with contextual information - eg: topDecoder:myDecoder:MySatLink1:mySat,
#                                               the contextual representation of the src's is outside the scope of this implementation.
#   criticality:                            Alarm criticality - any of: ALARM_CRITICALITY_A, ALARM_CRITICALITY_B, ALARM_CRITICALITY_C:
#                                               - ALARM_CRITICALITY_A: Indicates that the service may be out of service all together
#                                               - ALARM_CRITICALITY_B  Indicates limitted service performance that may cause service interuption
#                                               - ALARM_CRITICALITY_C  Indicates peripherial service limitations that is likely not to impact
#                                                   the core business logic execution, but may impact system observability, LCM operations of the system,
#                                                   etc.
#   sloganDescription:                      A static high level description of the alarm (on slogan level), E.g. link down
#   contextRaiseReason:                     A runtime/context based description on what may have lead up to this alarm, E.g. Excessive CRC errors
#   contextCeaseReason:                     A runtime/context based description on what lead to the cease of this alarm
#
# Publi methods:
# -------------
# -raiseAlarm(contextDescription):          Raise an alarm with an optional contextDescription - triggered by the client side
# -ceaseAlarm()                             Cease an alarm: Cease an alarm - triggered by the client side
# -updateAlarmMetaData()                    Updates the alarm meta data
# -getGlobalAlarmClassObjUid():             Returns the Alarm Class UID
# -getGlobalAlarmInstanceUid()              Returns tha Alarm instance UID
# -getIsActive()                            Returs the alarm status
# -getRaiseUtcTime()                        Returns the time in YYYY-MM-DD HH:MM:SS.uS for when the alarm was raised
# -getCeaseUtcTime()                        Returns the time in YYYY-MM-DD HH:MM:SS.uS for when the alarm was ceased
# -getLastingTimeS()                        Returns the duration of an alarm in secons after it has been ceased
# -getLastingTime()                         Returns the duration of an alarm in HH:MM:SS.uS format after it has been ceased
# 
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
#   _globalAlarmClassObjUid                 A global unique alarm identifier for a specific alarm class object
#   _globalAlarmInstanceUid:                A unique alarm identifier assigned by the alarmHandler for a particular alarm event
#   _active:                                Indicates if the alarm is active (sits in the active alarm list), or ceased
#                                               (sits in the historical ceased alarm list)
#   _raiseEpochTime:                        Epoch time (s) when the alarm was raised
#   _raiseUtcTime:                          UTC time/timeOfDay when the alarm was raised in format: %Y-%m-%d %H:%M:%S.%f
#   _ceaseEpochTime:                        Epoch time (s) when the alarm was ceased
#..._ceaseUtcTime:                          UTC time/timeOfDay when the alarm was ceased in format: %Y-%m-%d %H:%M:%S.%f
#   _lastingTimeS:                          Alarm lasting time in seconds
#   _lastingTime:                           Alarm lasting time in format: %Y-%m-%d %H:%M:%S.%f
#
# Private methods:
# ----------------
#
#################################################################################################################################################
ALARM_CRITICALITY_A =       0                                                       # Indicates service out of order
ALARM_CRITICALITY_B =       1                                                       # Indicates limitted service performance
ALARM_CRITICALITY_C =       2                                                       # Indicates peripherial service limitations
ALARM_ALL_CRITICALITY =     255
class alarm:
    def __init__(self):
        self.type : str = None                                                      # Alarm type
        self.src : str = None                                                       # Source object ID
        self._globalAlarmClassObjUid : int = None                                   # Global unique alarm class object id
        self._globalAlarmInstanceUid : int = None                                   # Global unique alarm event id
        self.criticality : int = ALARM_CRITICALITY_A                                # Criticality
        self.sloganDescription : str = None                                         # Static Alarm slogan decription
        self.contextRaiseReason : str = None                                        # Specific information for this raise alarm context/occurance
        self.contextCeaseReason : str = None                                        # Specific information for this cease alarm context/occurance
        self._active : bool = False                                                 # Alarm is active or not
        self._raiseEpochTime : int = None                                           # Epoch time (s) for raising the alarm
        self._raiseUtcTime : str = None                                             # Date and time of day for raising the alarm
        self._ceaseEpochTime : int = None                                           # Epoch time time for ceasing the alarm
        self._ceaseUtcTime : str = None                                             # Date and time of day for ceasing the alarm
        self._lastingTimeS : int = None                                             # Time period the alarm lasted for in seconds
        self._lastingTime : str = None                                              # Time period the alarm lasted for in %Y-%m-%d %H:%M:%S.%f
        self._globalAlarmClassObjUid = alarmHandler._regGlobalAlarmClassObjUid()

    def raiseAlarm(self, p_contextDescription = ""):
        if self._active:
            return
        self._active = True
        self._raiseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._raiseEpochTime = datetime.utcnow().timestamp()
        self._ceaseTime = None
        self._lastingTime = None
        self.contextRaiseReason = p_contextDescription
        self._globalAlarmInstanceUid = alarmHandler._raiseAlarm(self)

    def ceaseAlarm(self, p_contextDescription = ""):
        if not self._active:
            return
        self._active = False
        self._ceaseUtcTime = datetime.utcnow().strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        self._ceaseEpochTime = datetime.utcnow().timestamp()
        self._lastingTimeS = self._ceaseEpochTime - self._raiseEpochTime
        self._lastingTime = time.strftime('%H:%M:%S', time.gmtime(self._lastingTimeS))
        self.contextCeaseReason = p_contextDescription
        alarmHandler._ceaseAlarm(deepcopy(self))
        self._raiseUtcTime = None
        self._raiseEpochTime = None
        self._ceaseUtcTime = None
        self._ceaseEpochTime = None
        self._lastingTimeS = None
        self._lastingTime = None

    def updateAlarmMetaData(self):
        alarmHandler._updateAlarmMetaData(self)

    def getGlobalAlarmClassObjUid(self):
        return self._globalAlarmClassObjUid

    def getGlobalAlarmInstanceUid(self):
        return self._globalAlarmInstanceUid

    def getIsActive(self):
        return self._active

    def getRaiseUtcTime(self):
        return self._raiseUtcTime

    def getCeaseUtcTime(self):
        return self._ceaseUtcTime

    def getLastingTimeS(self):
        return self._lastingTimeS

    def getLastingTime(self):
        return self._lastingTime



#################################################################################################################################################
# Class: alarmTableModel
# Purpose: alarmTableModel is a QAbstractTable model for alarms. It provides the capabilities to represent alarms in a QTableView table.
#
# Public Methods and objects to be used by the alarm producers:
# =============================================================
# Public data-structures:
# ---------------------
#
# Publi methods:
# -------------
# -setAlarmHistory(p_alarmHistory)          Sets the viewed alarm history (Active/History)
# -getAlarmHistory()                        Returns viewed alarm history
# -setAlarmCriticality(p_criticality)       Sets viewed alarm criticality
# - getAlarmCriticality()                   Returns the viewed alarm criticality
# -setSrcFilter(p_srcFilter)                Sets the viewed alarm source filter
# -getSrcFilter()                           Returns the viewed alarm source filter
# -setFreeSearchFilter(p_freeSearchFilter)  Sets the freesearch regex filter
# -getFreeSearchFilter()                    Returns the freesearch regex filter
# -formatAlarmTable(p_alarmHistory, p_criticality, p_srcFilter, p_freeSearchFilter, p_verbosity = False)
#                                           Returns a list of formatted alarm table rows
# -formatSelectedAlarmsCsv()                Format the viewed alarms into a csv table
# -formatAllAlarmsCsv()                     Format all allarms (Active/Ceased) into a ´csv table
# -rowCount(p_parent=Qt.QModelIndex())      Returns the row count
# -columnCount(p_parent=Qt.QModelIndex())   Returns the column count
# -headerData(section, orientation, role, p_verbosity = False)
#                                           Returns the alarm table header
# -data(index, role = QtCore.Qt.DisplayRole)Returns alarm table cell data
# -<*>Col(p_verbosity = False)              Provides the corespinding column index as provided with formatAlarmTable()
# 
# Private Methods and objects only to be used internally or by the alarmHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_parent                                  Calling UI object
# -_alarmHistory                            Alarm history filter
# -_criticality                             Alarm criticality filter
# -_srcFilter                               Alarm source filter
# -_freeSearchFilter                        Alarm free search regex filter
#.-_alarmTable                              Viewed (filtered) alarm table
# -_colCnt                                  Alarm table column count
# -_rowCnt                                  Alarm table row count
#
# Private methods:
# ----------------
# -_onAlarmChange(p_criticality, p_noOfAlarms, p_metaData)
#                                           Alarm change call back trigerring alarm table update
# -_reLoadData()                            Reload alarm content
#################################################################################################################################################
class alarmTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent, p_alarmHistory = False, p_criticality = ALARM_ALL_CRITICALITY):
        self._parent = p_parent
        self._alarmHistory = p_alarmHistory
        self._criticality = p_criticality
        self._srcFilter = "*"
        self._freeSearchFilter = "*"
        self._alarmTable = []
        self._colCnt = 0
        self._rowCnt = 0
        QtCore.QAbstractTableModel.__init__(self)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_A, self._onAlarmChange)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_B, self._onAlarmChange)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_C, self._onAlarmChange)
        self._reLoadData()

    def setAlarmHistory(self, p_alarmHistory):
        self._alarmHistory = p_alarmHistory
        self._reLoadData()

    def getAlarmHistory(self):
        return self._alarmHistory

    def setAlarmCriticality(self, p_criticality):
        self._criticality = p_criticality
        self._reLoadData()

    def getAlarmCriticality(self):
        return self._criticality

    def setSrcFilter(self, p_srcFilter):
        self._srcFilter = p_srcFilter
        self._reLoadData()

    def getSrcFilter(self):
        return self._srcFilter

    def setFreeSearchFilter(self, p_freeSearchFilter):
        self._freeSearchFilter = p_freeSearchFilter

    def getFreeSearchFilter(self):
        return self._freeSearchFilter

    def _onAlarmChange(self, p_criticality, p_noOfAlarms, p_metaData):
        self._reLoadData()

    def _reLoadData(self):
        self.beginResetModel()
        self._alarmTable = self.formatAlarmTable(self._alarmHistory, self._criticality, self._srcFilter, self._freeSearchFilter)
        try:
            self._colCnt = len(self._alarmTable[0])
        except:
            self._colCnt = 0
        self._rowCnt = len(self._alarmTable)
        self.endResetModel()
        self._parent.alarmsTableView.resizeColumnsToContents()
        self._parent.alarmsTableView.resizeRowsToContents()

    def formatAlarmTable(self, p_alarmHistory, p_criticality, p_srcFilter, p_freeSearchFilter, p_verbosity = False):
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
            if p_srcFilter != "*" and alarmListItter.src != p_srcFilter:
                continue
            row = []
            row.append(str(alarmListItter._globalAlarmInstanceUid))
            if alarmListItter.criticality == ALARM_CRITICALITY_A:
                row.append("A")
            elif alarmListItter.criticality == ALARM_CRITICALITY_B:
                row.append("C")
            elif alarmListItter.criticality == ALARM_CRITICALITY_C:
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
                row.append(alarmListItter.contextRaiseReason)
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
                    if alarmListItter.contextCeaseReason == None or alarmListItter.contextCeaseReason == "":
                        row.append("-")
                    else:
                        row.append(alarmListItter.contextCeaseReason)
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
            row.append(alarmListItter.type)
            row.append(alarmListItter.src)
            row.append(alarmListItter.sloganDescription)
            if p_verbosity:
                row.append(str(alarmListItter._globalAlarmClassObjUid))
            alarmTable.append(row)
        return alarmTable

    def formatSelectedAlarmsCsv(self):
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

    def formatAllAlarmsCsv(self):
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

    def rowCount(self, p_parent=Qt.QModelIndex()):
        return self._rowCnt

    def columnCount(self, p_parent=Qt.QModelIndex()):
        return self._colCnt

    def headerData(self, section, orientation, role, p_verbosity = False):
        if role != QtCore.Qt.DisplayRole:
            return None
        if orientation == QtCore.Qt.Horizontal:
            if p_verbosity:
                return ("Alarm instance ID:", "Severity:", "Active", "Raised[UTC]:", "Raised[EPOCH]", "Raise reason:", "Ceased[UTC]:", "Ceased[EPOCH]:", "Cease reason:", "Duration:", "Duration[s]:", "Type:", "Source:", "Slogan:", "Alarm object ID:")[section]
            else:
                return ("Alarm instance ID:", "Severity:", "Raised[UTC]:", "Ceased[UTC]:", "Duration:", "Type:", "Source:", "Slogan:")[section]
        else:
            return f"{section}"

    def data(self, index, role = QtCore.Qt.DisplayRole):
        column = index.column()
        row = index.row()
        if role == QtCore.Qt.DisplayRole:
            return self._alarmTable[row][column]
        if role == QtCore.Qt.ForegroundRole:
            if self._alarmHistory:
                return QtGui.QBrush(QtGui.QColor('#505050'))
            elif self._alarmTable[row][1] == "A":
                return QtGui.QBrush(QtGui.QColor('#FF0000'))
            elif self._alarmTable[row][1] == "B":
                return QtGui.QBrush(QtGui.QColor('#779638'))
            elif self._alarmTable[row][1] == "C":
                return QtGui.QBrush(QtGui.QColor('#FFC800'))
        if role == QtCore.Qt.TextAlignmentRole:
            return QtCore.Qt.AlignLeft

    def instanceIdCol(self, p_verbosity = False): return 0
    def severityCol(self, p_verbosity = False): return 1
    def activeCol(self, p_verbosity = False): return 2 if p_verbosity else None
    def raiseTimeUtcCol(self, p_verbosity = False): return 3 if p_verbosity else 2
    def raiseTimeEpochCol(self, p_verbosity = False): return 4 if p_verbosity else None
    def raiseReasonCol(self, p_verbosity = False): return 5 if p_verbosity else None
    def ceaseTimeUtcCol(self, p_verbosity = False): return 6 if p_verbosity else 3
    def ceaseTimeEpochCol(self, p_verbosity = False): return 7 if p_verbosity else None
    def ceaseReasonCol(self, p_verbosity = False): return 8 if p_verbosity else None
    def durationCol(self, p_verbosity = False): return 9 if p_verbosity else 4
    def durationSCol(self, p_verbosity = False): return 10 if p_verbosity else None
    def typeCol(self, p_verbosity = False): return 11 if p_verbosity else 5
    def sourceCol(self, p_verbosity = False): return 12 if p_verbosity else 6
    def sloganCol(self, p_verbosity = False): return 13 if p_verbosity else 7
    def objIdCol(self, p_verbosity = False): return 14 if p_verbosity else None
