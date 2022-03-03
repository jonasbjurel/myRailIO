import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *
from sensorLogic import *
from actuatorLogic import *



class satelite(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, satAddr, parentItem, demo=False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("satelite")
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        super().__init__()
        self.satAddr = satAddr
        self.description = "Satelite description"
        self.nameKey = "Satelite-" + str(self.satAddr) + " - " + self.description
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey, SATELITE, displayIcon=SATELITE_ICON)
        self.logVerbosity = "INFO"
        self.rxCrcErr = 0
        self.txCrcErr = 0
        self.wdErr = 0
        self.logStatsProducer = threading.Thread(target=self.statsProducer)
        self.logStatsProducer.start()
        self.sensors = []
        self.actuators = []
        if self.demo:
            for i in range(8):
                self.addChild(SENSOR, name="sensor-" + str(i), config=False, demo=True)
            for i in range(4):
                self.addChild(ACTUATOR, name="actuator-" + str(i), config=False, demo=True)

    def setDescription(self, description):
        super().setDescription(description)
        super().setNameKey("Satelite-" + str(self.getSatAddr()) + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def setSatAddr(self, satAddr):
        super().setSatAddr(satAddr)
        super().setNameKey("Satelite-" + str(self.getSatAddr()) + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def getMethods(self):
        return METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_ADD | METHOD_EDIT | METHOD_DELETE |METHOD_ENABLE | METHOD_ENABLE_RECURSIVE | METHOD_DISABLE | METHOD_DISABLE_RECURSIVE | METHOD_LOG | METHOD_RESTART
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE & ~METHOD_ENABLE_RECURSIVE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE & ~METHOD_DISABLE_RECURSIVE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True, demo=False):
        if resourceType == SENSOR:
            self.sensors.append(sensor(self.win, name, self.item, demo=demo))
            self.childs = self.sensors + self.actuators
            self.reEvalOpState()
            if config:
                self.dialog = UI_sensorDialog(self.sensors[-1], edit=True)
                self.dialog.show()
            return 0
        elif resourceType == ACTUATOR:
            self.actuators.append(actuator(self.win, name, self.item, demo=demo))
            self.childs = self.sensors + self.actuators
            self.reEvalOpState()
            if config:
                self.dialog = UI_actuatorDialog(self.actuators[-1], edit=True)
                self.dialog.show()
            return 0
        else:
            print("Error")
            return 1

    def delChild(self, child):
        try:
            self.sensors.remove(child)
        except:
            pass
        try:
            self.actuators.remove(child)
        except:
            pass
        self.childs = self.sensors + self.actuators

    def view(self):
        self.dialog = UI_sateliteDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_sateliteDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, SENSOR | ACTUATOR)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def statsProducer(self):
        while True:
            self.rxCrcErr += 1
            self.txCrcErr += 1
            self.wdErr += 1
            time.sleep(0.25)

    def clearStats(self):
        self.rxCrcErr = 0
        self.txCrcErr = 0
        self.wdErr = 0

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)
