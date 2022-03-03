import sys
import time
import threading
import traceback
from dataModel import *
from sysState import *
from log import *
from momResources import *
from ui import *
from lightGroupLogic import *
from satLinkLogic import *



class decoder(genJMRIDataModels, systemState, componentLog):
    def __init__(self, win, name, parentItem, demo=False):
        self.win = win
        self.demo = demo
        super(genJMRIDataModels, self).__init__()
        super(systemState, self).__init__()
        super(componentLog, self).__init__()
        super().setLogName("decoder")
        self.parentItem = parentItem
        self.parent = parentItem.getObj()
        self.lightGroups = []
        self.satLinks = []
        self.childs = self.lightGroups + self.satLinks
        self.uri = name
        self.mac = "01:23:45:67:89:AB"
        self.description = "Decoder description"
        self.nameKey = self.uri + " - " + self.description
        self.logVerbosity = "INFO"
        self.item = self.win.registerMoMObj(self, parentItem, self.nameKey, DECODER, displayIcon=DECODER_ICON)
        if self.demo:
            for i in range(16):
                self.addChild(LIGHT_GROUP, name="lightGroup-" + str(i), config=False, demo=True)
            for i in range(2):
                self.addChild(SATELITE_LINK, name=i, config=False, demo=True)
                
    def setUri(self, uri):
        super().setUri(uri)
        super().setNameKey(self.getUri() + " - " + self.getDescription())
        self.win.reSetMoMObjStr(self.item, self.getNameKey())

    def setDescription(self, description):
        super().setDescription(description)
        super().setNameKey(self.getUri() + " - " + description)
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

    def addChild(self, resourceType, name="", config=True, demo=False):
        if resourceType == LIGHT_GROUP:
            self.lightGroups.append(lightGroup(self.win, name, self.item))
            self.childs = self.lightGroups + self.satLinks
            self.reEvalOpState()
            if config:
                self.dialog = UI_lightGroupDialog(self.lightGroups[-1], edit=True)
                self.dialog.show()
            return 0
        elif resourceType == SATELITE_LINK:
            self.satLinks.append(satLink(self.win, name, self.item, demo=demo))
            self.childs = self.lightGroups + self.satLinks
            self.reEvalOpState()
            if config:
                self.dialog = UI_satLinkDialog(self.satLinks[-1], edit=True)
                self.dialog.show()
            return 0
        else:
            print("Error")
            return 1

    def delChild(self, child):
        try:
            self.lightGroups.remove(child)
        except:
            pass
        try:
            self.satLinks.remove(child)
        except:
            pass
        self.childs = self.lightGroups + self.satLinks

    def view(self):
        self.dialog = UI_decoderDialog(self, edit=False)
        self.dialog.show()

    def edit(self):
        self.dialog = UI_decoderDialog(self, edit=True)
        self.dialog.show()

    def add(self):
        self.dialog = UI_addDialog(self, LIGHT_GROUP | SATELITE_LINK)
        self.dialog.show()

    def delete(self):
        if self.canNotDelete():
            return self.canNotDelete()
        self.parent.delChild(self)
        return self.win.unRegisterMoMObj(self.item)

    def setLogVerbosity(self, logVerbosity):
        genJMRIDataModels.setLogVerbosity(self, logVerbosity)
        componentLog.setLogVerbosity(self, logVerbosity)