from importlib.resources import open_binary
import sys
import time
import threading
import traceback
import imp
from trace import *
#from genJMRIRc import rc
#imp.load_source('myTrace', '..\\trace\\trace.py')
#from myTrace import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc


# Constants and definitions
# =========================
#
# Administrative states
# ---------------------
ADM_ENABLE =                [0,"ENABLE", "Decoder object is Adminstratively enabled"]                                   # The ADMIN state ENABLE
ADM_DISABLE =               [1, "DISABLE", "Decoder object is Adminstratively disabled"]                                # The ADMIN state DISABLE
ADM_STATES =                [ADM_ENABLE, ADM_DISABLE]

OP_WORKING =                [0b0000000000000, "WO", "Decoder is Working"]                                               # Object working
OP_INIT =                   [0b0000000000001, "INIT", "Decoder client is initializing"]                                 # Object initializing, not started - originated from client side
OP_DISCONNECTED =           [0b0000000000010, "DISC", "Decoder client is disconnected from MQTT"]                       # Object disconnected to its server - originated from client side
OP_NOIP =                   [0b0000000000100, "NOIP", "Decoder client has no IP-address"]                               # Object has no IP address - originated from client side(but could never be sent to server)
OP_UNDISCOVERED =           [0b0000000001000, "UDISC", "Decoder client is undiscovered"]                                # Object not discovered - originated from client side (but could never be sent to server)
OP_UNCONFIGURED =           [0b0000000010000, "UCONF", "Decoder client is Unconfigured"]                                # Object not configured - originated from client side
OP_DISABLED =               [0b0000000100000, "DABL", "Decoder object is disabled"]                                     # Object disbled from server - generated internally from admstate - at each of the server and client side
OP_SERVUNAVAILABLE =        [0b0000001000000, "SUAVL", "Decoder server missing MQTT Pings"]                             # Object unavailable from server - server decoder did not detect MQTT Ping messages from client side
OP_CLIEUNAVAILABLE =        [0b0000010000000, "CUAVL", "Decoder client missing MQTT Pings"]                             # Object unavailable from client - client decoder did not detect MQTT Ping responses messages from server side
OP_ERRSEC =                 [0b0000100000000, "ESEC", "Decoder object has experienced excessive disturbances"]          # Oject has experienced excessive PM degradation over the past second - originated from client side
OP_GENERR =                 [0b0001000000000, "ERR", "Decoder object has a recoverable error"]                          # Object has experienced a recoverable generic error - originated from client side
OP_INTFAIL =                [0b0010000000000, "FAIL", "Decoder object has a unrecoverable failure"]                     # Object has experienced a non-recoverable generic error - originated from server and client side
OP_CBL =                    [0b0100000000000, "CBL", "Decoder object is control blocked due to parent blocking state"]  # Object control-blocked from any parent opState block reasons - generated internally at each of the server and client side
OP_UNUSED =                 [0b1000000000000, "UUSED", "Decoder object is unused"]                                      # Object not in use - originated from client side
OP_ALL =                    [0b1111111111111, "ALL"]
                                                                # A collection of potential operational states - helper for translation of operational state to descriptive strings
OP_DETAIL_STATES =          [OP_WORKING, OP_INIT,
                             OP_DISCONNECTED, OP_NOIP,
                             OP_UNDISCOVERED, OP_UNCONFIGURED,
                             OP_DISABLED, OP_SERVUNAVAILABLE,
                             OP_CLIEUNAVAILABLE, OP_ERRSEC,
                             OP_GENERR, OP_INTFAIL, 
                             OP_CBL, OP_UNUSED]

OP_SUMMARY_AVAIL =          [0, "AVAIL", "Available"]
OP_SUMMARY_UNAVAIL =        [1, "UNAVAIL", "Unavailable"]
OP_SUMMARY_STATES =         [OP_SUMMARY_AVAIL,
                             OP_SUMMARY_UNAVAIL]

STATE =                     0                       # The op/admState value litteral
STATE_STR =                 1                       # The op/admState string
STATE_DESC =                2                       # The op/admState description

class systemState():
    def __init__(self):
        self.parent = None
        self.opStateCbs = []
        self.admState = ADM_ENABLE
        self.opStateSummary = OP_SUMMARY_AVAIL
        self.opStateDetail = OP_WORKING[STATE] # opStateDetail only holds the opState bit matrix
        self.upTimeStart = time.time()
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
            self.unSetOpStateDetail(OP_DISABLED)
            return rc.OK
        if self.isParentDisabled():
            return rc.PARENT_NOT_ENABLE
        self.admState = ADM_ENABLE
        self.unSetOpStateDetail(OP_DISABLED)
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
        #self.unSetOpStateDetail(OP_ALL, publish=False)
        self.setOpStateDetail(OP_DISABLED)
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

    def setOpStateDetail(self, opState, publish=True):
        if isinstance(opState, list):
            normOpState = opState[STATE]
        else:
            normOpState = opState

        prevOpStateDetail = self.opStateDetail
        self.opStateDetail = self.opStateDetail | normOpState
        changedOpStateDetail = self.opStateDetail ^ prevOpStateDetail
        if not changedOpStateDetail:
            return rc.OK
        self.opStateSummary = OP_SUMMARY_UNAVAIL
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            if publish:
                self.callOpStateCbs(changedOpStateDetail)
            return rc.OK
        for child in self.childs.value:
            child.setOpStateDetail(OP_CBL, publish)
        if publish:
            self.callOpStateCbs(changedOpStateDetail)
        return rc.OK

    def unSetOpStateDetail(self, opState, publish=True):
        if isinstance(opState, list):
            normOpState = opState[STATE]
        else:
            normOpState = opState

        prevOpStateDetail = self.opStateDetail
        self.opStateDetail = self.opStateDetail & ~normOpState
        changedOpStateDetail = self.opStateDetail ^ prevOpStateDetail
        if not changedOpStateDetail:
            return rc.OK
        if self.opStateDetail == OP_WORKING[STATE]:
            self.opStateSummary = OP_SUMMARY_AVAIL
            self.upTimeStart = time.time()
            try:
                self.childs.value
                assert self.childs.value != None
            except:
                if publish:
                    self.callOpStateCbs(changedOpStateDetail)
                return rc.OK
            for child in self.childs.value:
                child.unSetOpStateDetail(OP_CBL, publish)
        if publish:
            self.callOpStateCbs(changedOpStateDetail)
        return rc.OK

    def reEvalOpState(self):
        try:
            self.childs.value
            assert self.childs.value != None
        except:
            return rc.OK

        if self.opStateDetail:
            for child in self.childs.value:
                child.setOpStateDetail(OP_CBL)
        else:
            for child in self.childs.value:
                child.unSetOpStateDetail(OP_CBL)
        return rc.OK

    def getOpStateDetailStrFromBitMap(self, bitMap):
        opStateDetailStr = ""
        for opDetailStateItter in OP_DETAIL_STATES:
            if opDetailStateItter[STATE] & bitMap:
                opStateDetailStr += opDetailStateItter[STATE_STR] + "|"
        if len(opStateDetailStr) == 0:
            return OP_WORKING[STATE_STR]
        else:
            return opStateDetailStr.rstrip("|")

    def getOpStateDetailStr(self):
        return self.getOpStateDetailStrFromBitMap(self.opStateDetail)

    def getOpStateDetailBitMapFromStr(self, opStr):
        opStateDetailBitMap = OP_WORKING[STATE]
        opStrList = opStr.split("|")
        strFound = False
        for opStateStrItter in opStrList:
            for opBitMapItter in OP_DETAIL_STATES:
                if opBitMapItter[STATE_STR] == opStateStrItter.strip():
                    opStateDetailBitMap = opStateDetailBitMap | opBitMapItter[STATE]
                    strFound = True
                    break
            assert strFound == True
        return opStateDetailBitMap

    def maskOpStateDetailBitMapToClient(self, opStateDetailBitmap):
        return opStateDetailBitmap & OP_SERVUNAVAILABLE

    def maskOpStateDetailStrMapToClient(self, opStateDetailStr):
        return self.getOpStateDetailStrFromBitMap(self.maskOpStateDetailBitMapToClient(self.getOpStateDetailBitMapFromStr(opStateDetailStr)))

    def maskOpStateDetailBitMapFromClient(self, opStateDetailBitmap):
        return opStateDetailBitmap & ~(OP_DISABLED | OP_SERVUNAVAILABLE | OP_CBL)

    def maskOpStateDetailStrFromClient(self, opStateDetailStr):
        return self.getOpStateDetailStrFromBitMap(self.maskOpStateDetailBitMapFromClient(self.getOpStateDetailBitMapFromStr(opStateDetailStr)))

    def getOpStateSummaryStr(self, opSummaryState):
        return opSummaryState[STATE_STR]

    def getUptime(self):
        if self.opStateDetail == OP_WORKING[STATE]:
            return round((time.time() - self.upTimeStart)/60, 0)
        else:
            return 0

    def regOpStateCb(self, cb, mask = OP_ALL):
        self.opStateCbs.append([cb, mask])

    def unRegOpStateCb(self, cb):
        for cbItter in self.opStateCbs:
            if cbItter[0] == cb:
                mask = cbItter[1]
                break
        try:
            self.opStateCbs.remove([cb, mask])
        except:
            pass

    def callOpStateCbs(self, changedOpStateDetail):
        for cbItter in self.opStateCbs:
            if cbItter[1] & changedOpStateDetail:
                cbItter[0](cbItter[1] & changedOpStateDetail)
