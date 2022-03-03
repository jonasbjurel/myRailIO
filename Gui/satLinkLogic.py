import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *
from sateliteLogic import *



class satLink(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, linkNo, parentItem, demo=False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("satLink")
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.satelites = []
        self.childs = self.satelites
        self.satLinkNo = linkNo
        self.description = "SatLink description"
        self.nameKey = "Satelite-link-" + str(self.satLinkNo) + " - " + self.description
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey, SATELITE_LINK, displayIcon=LINK_ICON)
        self.logVerbosity = "INFO"
        self.rxCrcErr = 0
        self.remCrcErr = 0
        self.rxSymErr = 0
        self.rxSizeErr = 0
        self.wdErr = 0
        self.logStatsProducer = threading.Thread(target=self.statsProducer)
        self.logStatsProducer.start()

        if self.demo:
            for i in range(8):
                self.addChild(SATELITE, name=i, config=False, demo=True)

    def setSatLinkNo(self, satLinkNo):
        super().setSatLinkNo(satLinkNo)
        super().setNameKey("Satelite-link-" + str(self.getSatLinkNo()) + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def setDescription(self, description):
        super().setDescription(description)
        super().setNameKey("Satelite-link-" + str(self.getSatLinkNo()) + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name=0, config=True, demo=False):
        if resourceType == SATELITE:
            self.satelites.append(satelite(self.win, name, self.item, demo=demo))
            self.childs = self.satelites
            self.reEvalOpState()
            if config:
                self.dialog = UI_sateliteDialog(self.satelites[-1], edit=True)
                self.dialog.show()
            return 0
        else:
            print("Error")
            return 1

    def delChild(self, child):
        self.satelites.remove(child)
        self.childs = self.satelites

    def view(self):
        self.dialog = UI_satLinkDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_satLinkDialog(self, edit=True)
        self.dialog.show()
    def add(self):

        self.dialog = UI_addDialog(self, SATELITE)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def statsProducer(self):
        while True:
            self.rxCrcErr += 1
            self.remCrcErr += 1
            self.rxSymErr += 1
            self.rxSizeErr += 1
            self.wdErr += 1
            time.sleep(0.25)

    def clearStats(self):
        self.rxCrcErr = 0
        self.remCrcErr = 0
        self.rxSymErr = 0
        self.rxSizeErr = 0
        self.wdErr = 0

    def startLog(self, logTargetHandle):
        self.logTargetHandle = logTargetHandle
        self.terminateLog = False
        self.logProducerHandle = threading.Thread(target=self.logProducer)
        self.logProducerHandle.start()

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)
