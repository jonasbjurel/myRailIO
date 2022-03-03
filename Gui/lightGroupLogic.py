import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *



class lightGroup(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, name, parentItem, demo = False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("lightGroup")
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.JMRIAddr = "0"
        self.JMRISystemName = name
        self.description = "Lightgroup description"
        self.nameKey = self.JMRISystemName + " - " + self.description
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey, LIGHT_GROUP, displayIcon=TRAFFICLIGHT_ICON)
        self.JMRIUserName = "Light group user name"
        self.lgLink = 0
        self.lg = 0
        self.lgSeq = 0
        self.lgType = "SIGNAL MAST"
        self.lgProperty1 = ""
        self.lgProperty2 = ""
        self.lgProperty3 = ""
        self.logVerbosity = "INFO"
        self.lgShowing = "STOPP"

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
        activeMethods = METHOD_VIEW | METHOD_EDIT | METHOD_DELETE |  METHOD_ENABLE | METHOD_DISABLE | METHOD_LOG
        if self.getAdmState() == ADM_ENABLE:
            activeMethods = activeMethods & ~METHOD_ENABLE
        elif self.getAdmState() == ADM_DISABLE:
            activeMethods = activeMethods & ~METHOD_DISABLE
        else: activeMethods = ""
        return activeMethods

    def addChild(self, resourceType, name="", config=True):
        pass

    def delChild(self, child):
        pass

    def view(self):
        self.dialog = UI_lightGroupDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_lightGroupDialog(self, edit=True)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def delChild(self):
        pass

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)
