import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *



class actuator(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, name, parentItem, demo=False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("actuator")
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.JMRIAddr = "0"
        self.JMRISystemName = name
        self.description = "Acruator description"
        self.nameKey = self.JMRISystemName + " - " + self.description
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey, ACTUATOR, displayIcon=ACTUATOR_ICON)
        self.JMRIUserName = "Actuator user name"
        self.actPort = 0
        self.actType = "SERVO"
        self.logVerbosity = "INFO"
        self.actState = "+75%"

    def setJMRISystemName(self, name):
        super().setJMRISystemName(name)
        super().setNameKey(self.getJMRISystemName() + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def setDescription(self, description):
        super().setDescription(description)
        super().setNameKey(self.getJMRISystemName() + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def getMethods(self):
        return METHOD_VIEW | METHOD_EDIT | METHOD_COPY | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG

    def getActivMethods(self):
        activeMethods = METHOD_VIEW | METHOD_EDIT | METHOD_DELETE | METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True, demo=False):
        pass

    def delChild(self, child):
        pass

    def view(self):
        self.dialog = UI_actuatorDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_actuatorDialog(self, edit=True)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)
