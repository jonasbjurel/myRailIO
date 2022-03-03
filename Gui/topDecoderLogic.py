import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *
from decoderLogic import *

class topDecoder(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, demo=False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("genJMRI server")
        self.decoders = []
        self.childs = self.decoders
        self.topItem = self.win.registerMoMObj(self, 0, "topDecoder", TOP_DECODER, displayIcon=SERVER_ICON)
        self.nameKey = "GenJMRI Server"
        self.gitTag = "1.1"
        self.author = "Jonas Bjurel"
        self.description = "GUI test"
        self.version = "1.1"
        self.date = "Feb 20 2022"
        self.ntp = "192.168.1.1"
        self.tz = 1
        self.rsysLog = "mysyslog.org"
        self.logVerbosity = "INFO"
        self.supervisionPeriod = 10
        self.decoderFailSafe = True
        self.trackFailSafe = True
        if self.demo:
            for i in range(6):
                self.addChild(DECODER, name="decoder-" + str(i), config=False, demo=True)

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True, demo=False):
        if resourceType == DECODER:
            self.decoders.append(decoder(self.win, name, self.topItem, demo=demo))
            self.childs = self.decoders
            self.reEvalOpState()
            if config:
                self.dialog = UI_decoderDialog(self.decoders[-1], edit=True)
                self.dialog.show()
                return 0
        else:
            print("Error")
            return 1

    def delChild(self, child):
        self.decoders.remove(child)
        self.childs = self.decoders

    def restart(self):
        print("Restarting genJMRI server...")

    def view(self):
        self.dialog = UI_topDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_topDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, DECODER)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def gitCi(self):
        print("Checking in tag: " + self.gitTag)

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)

