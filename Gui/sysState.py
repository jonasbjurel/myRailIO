import sys
import time
import threading
import traceback
from rc import *


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
        self.childs = []
        self.admState = ADM_ENABLE
        self.opStateSummary = OP_SUMMARY_AVAIL
        self.opStateDetail = OP_WORKING[STATE] # admStateDetail only holds the opState bit matrix
        self.startTime = time.time()
        self.disable()

    def canNotDelete(self):
        if self.admState != ADM_DISABLE:
            return RC_NOT_DISABLE
        if self.childs != []:
            return RC_CHILDS_EXIST
        return RC_OK

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
        if self.parent == None:
            self.admState = ADM_ENABLE
            self.unSetOpStateDetail(OP_DISABLE)
            return RC_OK
        if self.isParentDisabled():
            return RC_PARENT_NOT_ENABLE
        self.admState = ADM_ENABLE
        self.unSetOpStateDetail(OP_DISABLE)
        return RC_OK

    def enableRecurs(self):
        self.enable()
        for child in self.childs:
            child.enableRecurs()
        return RC_OK

    def disable(self):
        if not self.areChildsDisabled():
            return RC_CHILD_NOT_DISABLE
        self.admState = ADM_DISABLE
        self.setOpStateDetail(OP_DISABLE)
        return RC_OK

    def disableRecurs(self):
        for child in self.childs:
            child.disableRecurs()
        self.disable()
        return 0

    def areChildsDisabled(self):
        for child in self.childs:
            if child.getAdmState() == ADM_ENABLE:
                return False
        return True

    def isParentDisabled(self):
        if self.parent == None:
            return True
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
            for child in self.childs:
                child.setOpStateDetail(OP_CONTROLBLOCK)

    def unSetOpStateDetail(self, opState):
        prevOpStateDetail = self.opStateDetail
        self.opStateDetail = self.opStateDetail & ~opState[STATE]
        if not self.opStateDetail:
            self.opStateSummary = OP_SUMMARY_AVAIL
        if not self.opStateDetail and prevOpStateDetail:
          self.startTime = time.time()
          for child in self.childs:
                child.unSetOpStateDetail(OP_CONTROLBLOCK)

    def reEvalOpState(self):
        if self.opStateDetail:
            for child in self.childs:
                child.setOpStateDetail(OP_CONTROLBLOCK)
        else:
            for child in self.childs:
                child.unSetOpStateDetail(OP_CONTROLBLOCK)

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
        else:
            return 0