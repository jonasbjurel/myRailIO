import sys
import time
import threading
import traceback
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc


# Constants and definitions
# =========================
#
# Administrative states
# ---------------------
ADM_ENABLE =                [0,"ENABLE"]            # The ADMIN state ENABLE
ADM_DISABLE =               [1, "DISABLE"]          # The ADMIN state DISABLE
ADM_STATES =                [ADM_ENABLE, ADM_DISABLE]

# Operational states detail
# -------------------------
OP_WORKING =                [0b00000000, "WORKING"] # The OP state has the atribute WORKING, the object is working
OP_INIT =                   [0b00000001, "INIT"]    # The OP state has the atribute INIT - the object is initializing, but not yet operational.
OP_CONFIG =                 [0b00000010, "CONFIG"]  # The OP state has the atribute CONFIG - configured, but not yet operational.
OP_DISABLE =                [0b00000100, "DISABLE"] # The OP state has the atribute DISABLE - disabled by the administrative state of the object.
OP_CONTROLBLOCK =           [0b00001000, "CTRLBLCK"]# The OP state has the atribute CONTROLBLOCK - disabled by the operational state of parent object.
OP_ERRSEC =                 [0b00010000, "ERRSEC"]  # The OP state has the atribute ERRSEC - the object has experienced errors over the past second exceeding set treshold.
OP_UNUSED =                 [0b00100000, "UNUSED"]  # The OP state has the atribute UNUSED - the object is not used anywere.
OP_UNAVAIL =                [0b01000000, "UNAVAIL"] # The OP state has the atribute UNAVAIL - the object has experienced a ping timeout error.
OP_FAIL =                   [0b10000000, "FAIL"]    # The OP state has the atribute FAIL - the object has experienced a general error.
                                                    # A collection of potential operational states - helper for translation of operational state to descriptive strings
OP_DETAIL_STATES =          [OP_WORKING, OP_INIT,
                                OP_CONFIG, OP_DISABLE,
                                OP_CONTROLBLOCK,
                                OP_ERRSEC, OP_UNUSED,
                                OP_FAIL]

OP_SUMMARY_AVAIL =          [0, "AVAIL"]
OP_SUMMARY_UNAVAIL =        [1, "UNAVAIL"]
OP_SUMMARY_STATES =         [OP_SUMMARY_AVAIL,
                             OP_SUMMARY_UNAVAIL]

STATE =                     0                       # The op/admState value litteral
STATE_STR =                 1                       # The op/admState string

class systemState():
    def __init__(self):
        self.parent = None
        self.opStateCbs = []
        self.admState = ADM_ENABLE
        self.opStateSummary = OP_SUMMARY_AVAIL
        self.opStateDetail = OP_WORKING[STATE] # admStateDetail only holds the opState bit matrix
        self.startTime = time.time()
        self.disable()

    def canDelete(self):
        if self.admState != ADM_DISABLE:
            return rc.NOT_DISABLE
        try:
            self.childs.value
        except:
            return rc.OK
        if self.childs.value != [] and self.childs.value != None:
            return rc.CHILDS_EXIST
        return rc.OK

    def getAdmState(self):
        return self.admState

    def getAdmStateStr(self):
        return self.admState[STATE_STR]

    def setAdmState(self, admState):
        if admState == ADM_ENABLE[STATE_STR]:
            return self.enable()
        elif admState == ADM_DISABLE[STATE_STR]:
            return self.disable()

    def setAdmStateRecurse(self, admState):
        if admState == ADM_ENABLE[STATE_STR]:
            return self.enableRecurse()
        elif admState == ADM_DISABLE[STATE_STR]:
            return self.disableRecurse()

    def enable(self):
        try:
            self.parent
            assert self.parent != None
        except:
            self.admState = ADM_ENABLE
            self.unSetOpStateDetail(OP_DISABLE)
            return rc.OK
        if self.isParentDisabled():
            return rc.PARENT_NOT_ENABLE
        self.admState = ADM_ENABLE
        self.unSetOpStateDetail(OP_DISABLE)
        return rc.OK

    def enableRecurse(self):
        if self.isParentDisabled():
            return rc.PARENT_NOT_ENABLE
        self.enable()
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            return rc.OK
        for child in self.childs.value:
            child.enableRecurse()
        return rc.OK

    def disable(self):
        if not self.areChildsDisabled():
            return rc.CHILD_NOT_DISABLE
        self.admState = ADM_DISABLE
        self.setOpStateDetail(OP_DISABLE)
        return rc.OK

    def disableRecurse(self):
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            self.disable()
            return rc.OK
        for child in self.childs.value:
            child.disableRecurse()
        self.disable()
        return rc.OK

    def areChildsDisabled(self):
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            return True
        for child in self.childs.value:
            if child.getAdmState() == ADM_ENABLE:
                return False
        return True

    def isParentDisabled(self):
        if self.parent == None:
            return False
        else:
            if self.parent.getAdmState() == ADM_DISABLE:
                return True
            else:
                return False

    def getOpStateSummary(self):
        return self.opStateSummary

    def getOpStateDetail(self):
        return self.opStateDetail

    def setOpStateDetail(self, opState):
        self.opStateDetail = self.opStateDetail | opState[STATE]
        if self.opStateDetail:
            self.opStateSummary = OP_SUMMARY_UNAVAIL
            try:
                self.childs.value
                assert self.childs.value != None
            except:
                self.callOpStateCbs()
                return rc.OK
            for child in self.childs.value:
                child.setOpStateDetail(OP_CONTROLBLOCK)
            self.callOpStateCbs()
            return rc.OK

    def unSetOpStateDetail(self, opState):
        prevOpStateDetail = self.opStateDetail
        self.opStateDetail = self.opStateDetail & ~opState[STATE]
        if not self.opStateDetail:
            self.opStateSummary = OP_SUMMARY_AVAIL
        if not self.opStateDetail and prevOpStateDetail:
            self.startTime = time.time()
            try:
                self.childs.value
                assert self.childs.value != None
            except:
                self.callOpStateCbs()
                return rc.OK
            for child in self.childs.value:
                child.unSetOpStateDetail(OP_CONTROLBLOCK)
            self.callOpStateCbs()
            return rc.OK

    def reEvalOpState(self):
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            return rc.OK

        if self.opStateDetail:
            for child in self.childs.value:
                child.setOpStateDetail(OP_CONTROLBLOCK)
        else:
            for child in self.childs.value:
                child.unSetOpStateDetail(OP_CONTROLBLOCK)
        return rc.OK

    def getOpStateDetailStr(self):
        opStateDetailStr = ""
        for opDetailStateItter in OP_DETAIL_STATES:
            if opDetailStateItter[STATE] & self.opStateDetail:
                opStateDetailStr += opDetailStateItter[STATE_STR] + " | "
        if len(opStateDetailStr) == 0:
            return OP_WORKING[STATE_STR]
        else:
            return opStateDetailStr.rstrip(" | ")

    def getOpStateSummaryStr(self, opSummaryState):
        return opSummaryState[STATE_STR]

    def getUptime(self):
        if self.opStateDetail == OP_WORKING[STATE]:
            return round((time.time() - self.startTime)/60, 0)

    def regOpStateCb(self, cb):
        self.opStateCbs.append(cb)

    def unRegOpStateCb(self, cb):
        try:
            self.opStateCbs.remove(cb)
        except:
            pass

    def callOpStateCbs(self):
        for cb in self.opStateCbs:
            cb()
