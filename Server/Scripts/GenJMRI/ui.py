import os
import sys
from PyQt5.QtWidgets import QApplication, QDialog, QWidget, QMainWindow, QMessageBox, QMenu, QFileDialog
from PyQt5.uic import loadUi
from PyQt5.Qt import QStandardItemModel, QStandardItem
from PyQt5.QtGui import QFont, QColor, QTextCursor, QIcon
from PyQt5 import QtCore
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom
import time
import threading
import traceback
from momResources import *
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"\\..\\trace\\")
import trace
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import rc
imp.load_source('parseXml', '..\\xml\\parseXml.py')
from parseXml import *




# Constants and definitions
# =========================
#
# MOM UI left click supported methods:
METHOD_VIEW =                   0b00000000001
METHOD_ADD =                    0b00000000010
METHOD_EDIT =                   0b00000000100
METHOD_COPY =                   0b00000001000
METHOD_DELETE =                 0b00000010000
METHOD_ENABLE =                 0b00000100000
METHOD_ENABLE_RECURSIVE =       0b00001000000
METHOD_DISABLE =                0b00010000000
METHOD_DISABLE_RECURSIVE =      0b00100000000
METHOD_LOG =                    0b01000000000
METHOD_RESTART =                0b10000000000

# UI widget definitions
MAIN_FRAME_UI = "ui/Main_Frame.ui"
TOP_DIALOG_UI = "ui/Top_Dialog.ui"
ADD_DIALOG_UI = "ui/Add_Dialog.ui"
DECODER_DIALOG_UI = "ui/Decoder_Dialog.ui"
LIGHTGROUPSLINK_DIALOG_UI = "ui/LightGroupsLink_Dialog.ui"
SATLINK_DIALOG_UI = "ui/SatLink_Dialog.ui"
LIGHTGROUP__LINK_DIALOG_UI = "ui/LightGroupsLink_Dialog.ui"
LIGHTGROUP_DIALOG_UI = "ui/LightGroup_Dialog.ui"
SATELITE_DIALOG_UI = "ui/Sat_Dialog.ui"
SENSOR_DIALOG_UI = "ui/Sensor_Dialog.ui"
ACTUATOR_DIALOG_UI = "ui/Actuator_Dialog.ui"
LOGOUTPUT_DIALOG_UI = "ui/Log_Output_Dialog.ui"
LOGSETTING_DIALOG_UI = "ui/Log_Setting_Dialog.ui"
CONFIGOUTPUT_DIALOG_UI = "ui/Config_Output_Dialog.ui"
AUTOLOAD_PREF_DIALOG_UI = "ui/autoLoad_Pref_Dialog.ui"

# UI Icon resources
SERVER_ICON = "icons/server.png"
DECODER_ICON = "icons/decoder.png"
LINK_ICON = "icons/link.png"
SATELITE_ICON = "icons/satelite.png"
TRAFFICLIGHT_ICON = "icons/traffic-light.png"
SENSOR_ICON = "icons/sensor.png"
ACTUATOR_ICON = "icons/servo.png"

def short2longVerbosity(shortVerbosity):
    return "DEBUG-" + shortVerbosity

def long2shortVerbosity(longVerbosity):
    return longVerbosity.split("-")[1]

# Class: Standard item
# Derived from: PyQt5.Qt.QStandardItem
#               see: https://doc.qt.io/qtforpython-5/PySide2/QtGui/QStandardItem.html
class StandardItem(QStandardItem):
    def __init__(self, obj, txt='', font_size=12, set_bold=False, color=QColor(0, 0, 0), icon = None, methods=0):
        super().__init__()
        fnt = QFont('Open Sans', font_size)
        fnt.setBold(set_bold)
        self.setEditable(False)
        self.setForeground(color)
        self.obj = obj
        self.setFont(fnt)
        self.setText(txt)
        if icon != None:
            self.setIcon(QIcon(icon))

    def getObj(self):
        return self.obj

    def __delete__(self):
        super().__delete__()



class UI_mainWindow(QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.configFileDialog = UI_fileDialog("genJMRI main configuration", self)
        loadUi(MAIN_FRAME_UI, self)
        self.actionSaveConfig.setEnabled(False)
        self.actionSaveConfigAs.setEnabled(False)
        self.autoLoadPreferences.setEnabled(False)
        self.connectActionSignalsSlots()
        self.connectWidgetSignalsSlots()
        self.MoMTreeModel = QStandardItemModel()
        self.MoMroot = self.MoMTreeModel.invisibleRootItem()
        self.topMoMTree.setModel(self.MoMTreeModel)
        self.topMoMTree.expandAll()

    def setParentObjHandle(self, parentObjHandle):
        self.parentObjHandle = parentObjHandle
        self.configFileDialog.regFileOpenCb(self.parentObjHandle.onXmlConfig)

    def registerMoMObj(self, objHandle, parentItem, string, type, displayIcon=None):
        if type == TOP_DECODER:
            fontSize = 14 
            setBold = True
            color = QColor(0, 0, 0)
            item = StandardItem(objHandle, txt=string, font_size=fontSize, set_bold=setBold, color=color, icon=displayIcon)
            self.MoMroot.appendRow(item)
            return item
        elif type == DECODER:
            fontSize = 13
            setBold = True
            color = QColor(0, 0, 0)
        elif type == LIGHT_GROUP_LINK:
            fontSize = 12
            setBold = True
            color = QColor(0, 0, 0)
        elif type == LIGHT_GROUP:
            fontSize = 10
            setBold = False
            color = QColor(0, 0, 0)
        elif type == SATELITE_LINK:
            fontSize = 12
            setBold = True
            color = QColor(0, 0, 0)
        elif type == SATELITE:
            fontSize = 11
            setBold = True
            color = QColor(0, 0, 0)
        elif type == SENSOR:
            fontSize = 10
            setBold = False
            color = QColor(0, 0, 0)
        elif type == ACTUATOR:
            fontSize = 10
            setBold = False
            color = QColor(0, 0, 0)
        else:
            return None

        try:
            item = StandardItem(objHandle, txt=string, font_size=fontSize, set_bold=setBold, color=color, icon=displayIcon)
            parentItem.appendRow(item)
            return item
        except:
            print("Error")
            return None

    def reSetMoMObjStr(self, item, string):
        item.setText(string)

    def unRegisterMoMObj(self, item):
        self.MoMTreeModel.removeRow(item.row(), parent=self.MoMTreeModel.indexFromItem(item).parent())

    def faultBlockMarkMoMObj(self, item, faultState):
        pass

    def inactivateMoMObj(self, item):
        pass

    def controlBlockMarkMoMObj(Index):
        pass

    def MoMMenuContextTree(self, point):
        try:
            MoMName = str(self.topMoMTree.selectedIndexes()[0].data())
        except:
            return
        item = self.topMoMTree.selectedIndexes()[0]
        stdItemObj = item.model().itemFromIndex(item)
        menu = QMenu()
        menu.setTitle(MoMName + " - actions")
        menu.toolTipsVisible()

        viewAction = menu.addAction("View") if stdItemObj.getObj().getMethods() & METHOD_VIEW else None
        if viewAction != None: viewAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_VIEW else viewAction.setEnabled(False)
        editAction = menu.addAction("Edit") if stdItemObj.getObj().getMethods() & METHOD_EDIT else None
        if editAction != None: editAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_EDIT else editAction.setEnabled(False)
        copyAction = menu.addAction("Copy") if stdItemObj.getObj().getMethods() & METHOD_COPY else None
        if copyAction != None: copyAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_COPY else copyAction.setEnabled(False)
        addAction = menu.addAction("Add") if stdItemObj.getObj().getMethods() & METHOD_ADD else None
        if addAction != None: addAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_ADD else addAction.setEnabled(False)
        deleteAction = menu.addAction("Delete") if stdItemObj.getObj().getMethods() & METHOD_DELETE else None
        if deleteAction != None: deleteAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_DELETE else deleteAction.setEnabled(False)
        menu.addSeparator()
        enableAction = menu.addAction("Enable") if stdItemObj.getObj().getMethods() & METHOD_ENABLE else None
        if enableAction != None: enableAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE else enableAction.setEnabled(False)
        enableRecursAction = menu.addAction("Enable - recursive") if stdItemObj.getObj().getMethods() & METHOD_ENABLE_RECURSIVE else None
        if enableRecursAction != None: enableRecursAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE_RECURSIVE else enableRecursAction.setEnabled(False)
        disableAction = menu.addAction("Disable") if stdItemObj.getObj().getMethods() & METHOD_DISABLE else None
        if disableAction != None: disableAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE else disableAction.setEnabled(False)
        disableRecursAction = menu.addAction("Disable - recursive") if stdItemObj.getObj().getMethods() & METHOD_DISABLE_RECURSIVE else None
        if disableRecursAction != None: disableRecursAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE_RECURSIVE else disableRecursAction.setEnabled(False)
        menu.addSeparator()
        logAction = menu.addAction("View log") if stdItemObj.getObj().getMethods() & METHOD_LOG else None
        if logAction != None: logAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_LOG else logAction.setEnabled(False)
        menu.addSeparator()
        restartAction = menu.addAction("Restart") if stdItemObj.getObj().getMethods() & METHOD_RESTART else None
        if restartAction != None: restartAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_RESTART else restartAction.setEnabled(False)

        action = menu.exec_(self.topMoMTree.mapToGlobal(point))
        res = 0
        try:
            if action == viewAction: res = stdItemObj.getObj().view()
            if action == addAction: res = stdItemObj.getObj().add()
            if action == editAction: res = stdItemObj.getObj().edit()
            if action == copyAction: res = stdItemObj.getObj().copy()
            if action == deleteAction: res = stdItemObj.getObj().delete(top=True)
            if action == enableAction: res = stdItemObj.getObj().enable()
            if action == enableRecursAction: res = stdItemObj.getObj().enableRecurse()
            if action == disableAction: res = stdItemObj.getObj().disable()
            if action == disableRecursAction: res = stdItemObj.getObj().disableRecurse()
            if action == logAction: 
                self.logDialog = UI_logDialog(stdItemObj.getObj())
                self.logDialog.show()
            if action == restartAction: res = stdItemObj.getObj().restart()
            if res != rc.OK and res != None:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not execute action: " + str(action.text()))
                msg.setInformativeText(rc.getErrStr(res))
                msg.setWindowTitle("Error")
                msg.exec_()
        except Exception:
            traceback.print_exc()

    def connectActionSignalsSlots(self):
        # Main window Menu actions
        # ========================
        # File actions (configuration)
        # ----------------------------
        self.actionOpenConfig.triggered.connect(self.openConfigFile)
        self.actionSaveConfig.triggered.connect(self.saveConfigFile)
        self.actionSaveConfigAs.triggered.connect(self.saveConfigFileAs)
        self.autoLoadPreferences.triggered.connect(self.setAutoLoadPreferences)

        # Edit actions
        self.actiongenJMRI_preferences.triggered.connect(self.editGenJMRIPreferences)

        # Help actions
        # ------------
        self.actionAbout_genJMRI.triggered.connect(self.about)

        # Main window push button actions
        # ===============================
        # Log actions
        # -----------
        self.actionOpenLogProp.triggered.connect(self.setLogProperty)
        self.actionOpen_Log_window.triggered.connect(self.log)

    def connectWidgetSignalsSlots(self):
        # MoM tree widget
        self.topMoMTree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.topMoMTree.customContextMenuRequested.connect(self.MoMMenuContextTree)

        # Log widget
        self.topLogPushButton.clicked.connect(self.log)

        # Restart widget
        self.restartPushButton.clicked.connect(self.restart)

    def openConfigFile(self):
        self.configFileDialog.openFileDialog()

    def saveConfigFile(self):
        self.configFileDialog.saveFileDialog(self.parentObjHandle.getXmlConfigTree(text=True))

    def saveConfigFileAs(self):
        self.configFileDialog.saveFileAsDialog(self.parentObjHandle.getXmlConfigTree(text=True))

    def saveConfigFileA(self):
        self.configFileDialog.saveFileAsDialog(self.parentObjHandle.getXmlConfigTree(text=True))

    def haveGoodConfiguration(self, haveIt):
        if haveIt:
            self.actionSaveConfig.setEnabled(True)
            self.actionSaveConfigAs.setEnabled(True)
            self.autoLoadPreferences.setEnabled(True)

        else:
            self.actionSaveConfig.setEnabled(False)
            self.actionSaveConfigAs.setEnabled(False)
            self.autoLoadPreferences.setEnabled(False)


    def setAutoLoadPreferences(self):
        print("Setting Autoload prefs")
        self.configFileDialog.setAutoloadPrefs()

    def log(self):
        self.log = UI_logDialog(self.parentObjHandle)
        self.log.show()

    def setLogProperty(self):
        self.logsetting = UI_logSettingDialog(self.parentObjHandle)
        self.logsetting.show()

    def editGenJMRIPreferences(self):
        self.dialog = UI_topDialog(self.parentObjHandle, edit=True)
        self.dialog.show()

    def restart(self):
        self.parentObjHandle.restart()

    def about(self):
        QMessageBox.about(
            self,
            "About genJMRI",
            "<p>genJMRI implements a generic signal masts-, actuators- and sensor decoder framework for JMRI, </p>"
            "<p>For more information see:</p>"
            "<p>https://github.com/jonasbjurel/GenericJMRIdecoder</p>",
        )



class UI_fileDialog(QWidget):
    def __init__(self, fileContext, parentObjHandle, autoLoad=None, autoLoadDelay=None, path=None, fileName=None):
        super().__init__()
        self.fileContext = fileContext
        self.parentObjHandle = parentObjHandle
        self.fileOpenCbList = []
        self.getFilePrefs()
        try:
            print(self.fileName)
        except:
            pass
        if autoLoad != None: self.autoLoad = autoLoad
        try:
            self.autoLoad
        except:
            self.autoLoad = False
        if autoLoadDelay != None: self.autoLoadDelay = autoLoadDelay
        try:
            self.autoLoadDelay
        except:
            self.autoLoadDelay = 0
        if path != None: self.path = path
        if fileName != None: self.fileName = fileName
        self.saveFilePrefs()

    def haveFilePrefs(self):
        try:
            self.autoLoad
            self.autoLoadDelay
            self.path
            self.fileName
            return True
        except:
            return False

    def getFilePrefs(self):
        try:
            filePrefsXmlTop = ET.parse(os.getcwd() + "\\." + self.fileContext)
            filePrefs = parse_xml(filePrefsXmlTop.getroot(),
                                    {"Path": MANSTR,
                                        "FileName": MANSTR,
                                        "AutoLoad": MANSTR,
                                        "AutoLoadDelay": MANSTR
                                    }
                                )
            self.path = filePrefs.get("Path")
            self.fileName = filePrefs.get("FileName")
            self.autoLoad = True if filePrefs.get("AutoLoad") == "Yes" else False
            self.autoLoadDelay = int(filePrefs.get("AutoLoadDelay"))
        except:
            #trace.notify(DEBUG_INFO,"Could not read preference file: " + os.getcwd() + "\\." + self.fileContext)
            print("Could not read preference file: " + os.getcwd() + "\\." + self.fileContext)
            print( str(traceback.print_exc()))

    def saveFilePrefs(self):
        try:
            filePrefsXml = ET.Element("genJMRIFilePreference")
            path = ET.SubElement(filePrefsXml, "Path")
            path.text = self.path
            file = ET.SubElement(filePrefsXml, "FileName")
            file.text = self.fileName
            autoLoad = ET.SubElement(filePrefsXml, "AutoLoad")
            autoLoad.text = "Yes" if self.autoLoad else "No"
            autoLoadDelay = ET.SubElement(filePrefsXml, "AutoLoadDelay")
            autoLoadDelay.text = str(self.autoLoadDelay) if self.autoLoadDelay else str(0)
            f = open(os.getcwd() + "\\." + self.fileContext, "w")
            print(filePrefsXml)
            f.write(minidom.parseString(ET.tostring(filePrefsXml, 'unicode')).toprettyxml())
            f.close()
        except:
            #trace.notify(DEBUG_INFO,"Could not write to preference file: " + os.getcwd() + "\\." + self.fileContext)
            print("FilePrefs could not be written")
            print( str(traceback.print_exc()))

    def setAutoLoad(self, autoLoad, autoLoadDelay):
        self.autoLoad = autoLoad
        self.autoLoadDelay = autoLoadDelay
        self.saveFilePrefs()

    def regFileOpenCb(self, cb):
        if self.autoLoad and self.haveFilePrefs():
            if self.autoLoadDelay:
                print("Entering delayed load file")
                autoLoadDelayHandle = threading.Timer(self.autoLoadDelay, self.loadFileContent, (cb,))
                self.fileOpenCbList.append([cb, autoLoadDelayHandle])
                autoLoadDelayHandle.start()
            else:
                print("Entering load file")
                self.fileOpenCbList.append([cb, None])
                self.loadFileContent(cb)
        else:
            print("No auto-load")
            self.fileOpenCbList.append([cb, None])

    def unRegFileOpenCb(self, cb):
        for cbItter in  self.fileOpenCbList:
            if cbItter[0] == cb:
                self.fileOpenCbList.remove(cbItter)

    def cancelFileOpenCbDelayTimers(self, cb): #CB can be str(*) - (all) or a a for a specific callback
        for cbItter in  self.fileOpenCbList:
            if cb == "*":
                if cbItter[1] != None: cbItter[1].cancel()
            elif cb == cbItter[0]:
                if cbItter[1] != None: cbItter[1].cancel()

    def openFileDialog(self):
        options = QFileDialog.Options()
        #options |= QFileDialog.DontUseNativeDialog
        try:
            fileName, extTxt = QFileDialog.getOpenFileName(self,"Select a file to open", self.path,"XML Files (*.xml);;All Files (*)", options=options)
        except:
            fileName, extTxt = QFileDialog.getOpenFileName(self,"Select a file to open", "","XML Files (*.xml);;All Files (*)", options=options)
        if fileName:
            self.cancelFileOpenCbDelayTimers("*")
            self.path = fileName.rsplit("/", 1)[0]
            self.fileName = fileName
            self.loadFileContent("*")
            self.saveFilePrefs()

    def loadFileContent(self, cb):
        print ("Trying to load file content")
        try:
            print("Open File: " + self.fileName)
            f = open(self.fileName, "r", encoding='utf-8-sig')
            fileContent = f.read()
            f.close()
            for cbItter in self.fileOpenCbList:
                if cb == "*":
                    cbItter[0](fileContent)
                else:
                    if cbItter[0] == cb: 
                        cbItter[0](fileContent)
        except:
            print("Could not load file content")
            print( str(traceback.print_exc()))
            trace.notify(DEBUG_ERROR, "Could not load file")

    def saveFileDialog(self, content):
            f = open(self.fileName, "w", encoding='utf-8-sig')
            f.write(content)
            f.close()

    def saveFileAsDialog(self, content):
        options = QFileDialog.Options()
        #options |= QFileDialog.DontUseNativeDialog
        try:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a file name for saving", self.path,"XML Files (*.xml);;All Files (*)", options=options)
        except:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a file name for saving", "","XML Files (*.xml);;All Files (*)", options=options)
        if fileName:
            try:
                ext = extTxt.split("(")[1].split(")")[0].split(".")[1]
                fileName = fileName + "." + ext
            except:
                pass
            f = open(fileName, "w")
            f.write(content)
            f.close()
            self.path = fileName.rsplit("/", 1)[0]
            self.fileName = fileName
            self.saveFilePrefs()

    def setAutoloadPrefs(self):
        self.autoLoadPrefs = UI_setAutoloadPrefsDialog(self, self.autoLoad, self.autoLoadDelay)
        self.autoLoadPrefs.show()



class UI_setAutoloadPrefsDialog(QDialog):
    def __init__(self, parentObjHandle, autoLoad, autoLoadDelay, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(AUTOLOAD_PREF_DIALOG_UI, self)
        self.autoLoadConfigComboBox.setCurrentText("Yes" if autoLoad else "No")
        self.autoLoadConfigDelaySpinBox.setValue(autoLoadDelay)
        self.connectWidgetSignalsSlots()

    def connectWidgetSignalsSlots(self):
        self.autoLoadConfigAcceptButtonBox.accepted.connect(self.accepted)
        self.autoLoadConfigAcceptButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.parentObjHandle.setAutoLoad(True if self.autoLoadConfigComboBox.currentText() else False, self.autoLoadConfigDelaySpinBox.value() if self.autoLoadConfigComboBox.currentText() == "Yes" else 0)
        self.close()

    def rejected(self):
        self.close()



class UI_logDialog(QDialog):
    def __init__(self, parentObjHandle, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LOGOUTPUT_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.setWindowTitle(self.parentObjHandle.nameKey.value + " stdout TTY")
        self.scrollToBottom = self.logOutputScrollCheckBox.isChecked()
        self.showLogStartCursor()
        self.parentObjHandle.startLog(self.logStream)
        self.closeEvent = self.close__

    def connectWidgetSignalsSlots(self):
        # Close button
        self.closePushButton.clicked.connect(self.close_)

        # Scroll check-box
        self.logOutputScrollCheckBox.stateChanged.connect(self.setScrollBarProperty)

        # Clear display button
        self.logOutputClearPushButton.clicked.connect(self.clearDisplay)

        # Copy widget
        self.logOutputCopyPushButton.clicked.connect(self.copyDisplay)

        # LogSetting widget
        self.logOutputSettingsPushButton.clicked.connect(self.setLogProperty)

    def showLogStartCursor(self):
        self.logOutputTextBrowser.append(self.parentObjHandle.nameKey.value + " stdout TTY - log level: " + self.parentObjHandle.logVerbosity.value + ">")

    def logStream(self, str):
        self.logOutputTextBrowser.append(str)
        if self.scrollToBottom:
            self.logOutputTextBrowser.verticalScrollBar().setValue(self.logOutputTextBrowser.verticalScrollBar().maximum())

    def setScrollBarProperty(self, state):
        if state == QtCore.Qt.Checked:
            self.scrollToBottom = True
            self.logOutputTextBrowser.verticalScrollBar().setValue(self.logOutputTextBrowser.verticalScrollBar().maximum())
        else:
            self.scrollToBottom = False

    def clearDisplay(self):
        self.logOutputTextBrowser.clear()
        self.showLogStartCursor()

    def copyDisplay(self):
        self.logOutputTextBrowser.selectAll()
        self.logOutputTextBrowser.copy()
        cursor = self.logOutputTextBrowser.textCursor()
        cursor.movePosition(QTextCursor.End)
        self.logOutputTextBrowser.setTextCursor(cursor)

    def setLogProperty(self):
        self.logsetting = UI_logSettingDialog(self.parentObjHandle)
        self.logsetting.show()

    def close__(self, unknown):
        self.close_()

    def close_(self):
        self.parentObjHandle.stopLog()
        self.close()



class UI_logSettingDialog(QDialog):
    def __init__(self, parentObjHandle, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LOGSETTING_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.logSettingVerbosityComboBox.setCurrentText(long2shortVerbosity(self.parentObjHandle.getLogVerbosity()))

    def connectWidgetSignalsSlots(self):
        self.logSettingConfirmButtonBox.accepted.connect(self.accepted)
        self.logSettingConfirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.parentObjHandle.setLogVerbosity(short2longVerbosity(self.logSettingVerbosityComboBox.currentText()))
        self.close()

    def rejected(self):
        self.close()



class UI_getConfig(QDialog):
    def __init__(self, parentObjHandle, configuration, parent=None):
        super().__init__(parent)
        self.configuration = configuration
        self.parentObjHandle = parentObjHandle
        loadUi(CONFIGOUTPUT_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.setWindowTitle(self.parentObjHandle.parentObjHandle.nameKey.value + " Configuration")
        self.showConfig()
        self.closeEvent = self.close_

    def connectWidgetSignalsSlots(self):
        # Close button
        self.configOutputClosePushButton.clicked.connect(self.close_)

        # Copy widget
        self.configOutputCopyPushButton.clicked.connect(self.copyDisplay)

        # Copy widget
        self.configOutputSaveasPushButton.clicked.connect(self.saveAs)

    def saveAs(self):
        file = UI_fileDialog(self.parentObjHandle.parentObjHandle.nameKey.value + " configuration", self)
        file.saveFileAsDialog(str(self.configOutputTextBrowser.toPlainText()))

    def showConfig(self):
        self.configOutputTextBrowser.append(self.configuration)

    def copyDisplay(self):
        self.configOutputTextBrowser.selectAll()
        self.configOutputTextBrowser.copy()

    def close_(self, unknown):
        self.close()



class UI_addDialog(QDialog):
    def __init__(self, parentObjHandle, resourceTypes, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(ADD_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        resourceTypeList = []
        if resourceTypes & TOP_DECODER: resourceTypeList.append("Top decoder")
        if resourceTypes & DECODER: resourceTypeList.append("Decoder")
        if resourceTypes & LIGHT_GROUP_LINK: resourceTypeList.append("Light group link")
        if resourceTypes & LIGHT_GROUP: resourceTypeList.append("Light group")
        if resourceTypes & SATELITE_LINK: resourceTypeList.append("Satelite link")
        if resourceTypes & SATELITE: resourceTypeList.append("Satelite")
        if resourceTypes & SENSOR: resourceTypeList.append("Sensor")
        if resourceTypes & ACTUATOR: resourceTypeList.append("Actuator")
        self.resourceTypeComboBox.addItems(resourceTypeList)

    def connectWidgetSignalsSlots(self):
        # Confirm/Cancel box
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        resourceTypeStr = self.resourceTypeComboBox.currentText()
        if resourceTypeStr == "Top decoder": resourceType = TOP_DECODER
        elif resourceTypeStr == "Decoder": resourceType = DECODER
        elif resourceTypeStr == "Light group link": resourceType = LIGHT_GROUP_LINK
        elif resourceTypeStr == "Light group": resourceType = LIGHT_GROUP
        elif resourceTypeStr == "Satelite link": resourceType = SATELITE_LINK
        elif resourceTypeStr == "Satelite": resourceType = SATELITE
        elif resourceTypeStr == "Sensor": resourceType = SENSOR
        elif resourceTypeStr == "Actuator": resourceType = ACTUATOR
        print("ResourceTypeStr: " + resourceTypeStr)
        self.parentObjHandle.addChild(resourceType) #Check return code and handle error
        self.close()

    def rejected(self):
        self.close()



class UI_topDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(TOP_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        # Git configuration/operation section
        self.gitBranchComboBox.setEnabled(False) #Missing functionality
        self.gitTagComboBox.setEnabled(False) #Missing functionality
        self.gitCiPushButton.setEnabled(False) #Missing functionality
        self.gitCoPushButton.setEnabled(False) #Missing functionality
        # General genJMRI Meta-data
        self.authorLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.versionLineEdit.setEnabled(True)
        self.dateLineEdit.setEnabled(True)
        # General genJMRI services configuration
        self.ntpLineEdit.setEnabled(True)
        self.timeZoneSpinBox.setEnabled(True)
        self.rsyslogLineEdit.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setEnabled(True)
        self.RPC_Port_LineEdit.setEnabled(True)
        self.JMRIKeepalivePeriodDoubleSpinBox.setEnabled(True)
        # MQTT Southbound API Configuration
        self.decoderKeepalivePeriodDoubleSpinBox.setEnabled(True)
        self.decoderFailSafeCheckBox.setEnabled(True)
        self.trackFailSafeCheckBox.setEnabled(True)
        self.MQTT_URI_LineEdit.setEnabled(True)
        self.MQTT_Port_LineEdit.setEnabled(True)
        self.MQTT_TOPIC_PREFIX_LineEdit.setEnabled(True)
        # genJMRI states
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        # Top-decoder/genJMRI preference Confirm or cancel
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        # Git configuration/operation section
        self.gitBranchComboBox.setEnabled(False) #Missing functionality
        self.gitTagComboBox.setEnabled(False) #Missing functionality
        self.gitCiPushButton.setEnabled(False) #Missing functionality
        self.gitCoPushButton.setEnabled(False) #Missing functionality
        # General genJMRI Meta-data
        self.authorLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.versionLineEdit.setEnabled(False)
        self.dateLineEdit.setEnabled(False)
        # General genJMRI services configuration
        self.ntpLineEdit.setEnabled(False)
        self.timeZoneSpinBox.setEnabled(False)
        self.rsyslogLineEdit.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        # MQTT Southbound API Configuration
        self.decoderKeepalivePeriodDoubleSpinBox.setEnabled(False)
        self.decoderFailSafeCheckBox.setEnabled(False)
        self.trackFailSafeCheckBox.setEnabled(False)
        self.MQTT_URI_LineEdit.setEnabled(False)
        self.MQTT_Port_LineEdit.setEnabled(False)
        self.MQTT_TOPIC_PREFIX_LineEdit.setEnabled(False)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setEnabled(False)
        self.RPC_Port_LineEdit.setEnabled(False)
        self.JMRIKeepalivePeriodDoubleSpinBox.setEnabled(False)
        # General genJMRI states
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.gitCiPushButton.setEnabled(False)
        # Confirm or cancel
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        # Git configuration/operation section
        self.gitBranchComboBox.setCurrentText(str(self.parentObjHandle.gitBranch.value))
        self.gitTagComboBox.setCurrentText(str(self.parentObjHandle.gitTag.value))
        # General genJMRI Meta-data
        self.authorLineEdit.setText(str(self.parentObjHandle.author.value))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.description.value))
        self.versionLineEdit.setText(str(self.parentObjHandle.version.value))
        self.dateLineEdit.setText(str(self.parentObjHandle.date.value))
        # General genJMRI services configuration
        self.ntpLineEdit.setText(str(self.parentObjHandle.ntpUri.value[0]))
        self.timeZoneSpinBox.setValue(self.parentObjHandle.tz.value)
        self.rsyslogLineEdit.setText(str(self.parentObjHandle.rsysLogUri.value))
        self.logVerbosityComboBox.setCurrentText(str(long2shortVerbosity(self.parentObjHandle.logVerbosity.value)))
        # MQTT Southbound API Configuration
        self.decoderKeepalivePeriodDoubleSpinBox.setValue(self.parentObjHandle.decoderMqttKeepalivePeriod.value)
        self.decoderFailSafeCheckBox.setChecked(self.parentObjHandle.decoderFailSafe.value)
        self.trackFailSafeCheckBox.setChecked(self.parentObjHandle.trackFailSafe.value)
        self.MQTT_URI_LineEdit.setText(self.parentObjHandle.decoderMqttURI.value)
        self.MQTT_Port_LineEdit.setText(str(self.parentObjHandle.decoderMqttPort.value))
        self.MQTT_TOPIC_PREFIX_LineEdit.setText(self.parentObjHandle.decoderMqttTopicPrefix.value)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setText(self.parentObjHandle.jmriRpcURI.value)
        self.RPC_Port_LineEdit.setText(str(self.parentObjHandle.jmriRpcPortBase.value))
        self.JMRIKeepalivePeriodDoubleSpinBox.setValue(self.parentObjHandle.JMRIRpcKeepAlivePeriod.value)
        # General genJMRI states
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        # Git configuration/operation section
        self.parentObjHandle.gitBranch.value = self.gitBranchComboBox.currentText()
        self.parentObjHandle.gitTag.value = self.gitTagComboBox.currentText()
        # General genJMRI Meta-data
        self.parentObjHandle.author.value = self.authorLineEdit.displayText()
        self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        self.parentObjHandle.version.value = self.versionLineEdit.displayText()
        self.parentObjHandle.date.value = self.dateLineEdit.displayText()
        # General genJMRI services configuration
        self.parentObjHandle.ntpUri.value[0] = self.ntpLineEdit.displayText()
        self.parentObjHandle.tz.value = self.timeZoneSpinBox.value()
        self.parentObjHandle.rsysLogUri.value = self.rsyslogLineEdit.displayText()
        self.parentObjHandle.logVerbosity.value = short2longVerbosity(self.logVerbosityComboBox.currentText())
        # MQTT Southbound API Configuration
        self.parentObjHandle.decoderMqttKeepalivePeriod.value = self.decoderKeepalivePeriodDoubleSpinBox.value()
        self.parentObjHandle.decoderFailSafe.value = self.decoderFailSafeCheckBox.isChecked()
        self.parentObjHandle.trackFailSafe.value = self.trackFailSafeCheckBox.isChecked()
        self.parentObjHandle.decoderMqttURI.value = self.MQTT_URI_LineEdit.displayText()
        self.parentObjHandle.decoderMqttPort.value = int(self.MQTT_Port_LineEdit.displayText())
        self.parentObjHandle.decoderMqttTopicPrefix.value = self.MQTT_TOPIC_PREFIX_LineEdit.displayText()
        # MQTT Southbound API Configuration
        self.parentObjHandle.jmriRpcURI.value = self.RPC_URI_LineEdit.displayText()
        self.parentObjHandle.jmriRpcPortBase.value = int(self.RPC_Port_LineEdit.displayText())
        self.parentObjHandle.JMRIRpcKeepAlivePeriod.value = self.JMRIKeepalivePeriodDoubleSpinBox.value()
        # General genJMRI states
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if res != rc.OK:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(res))
                msg.setWindowTitle("Error")
                msg.exec_()
                return res

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)
        self.gitCiPushButton.clicked.connect(self.parentObjHandle.gitCi)
        self.genConfigPushButton.clicked.connect(self.genConfig)

    def genConfig(self):
        self.configOutputDialog = UI_getConfig(self, self.parentObjHandle.getXmlConfigTree(text=True))
        self.configOutputDialog.show()

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return


    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_decoderDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(DECODER_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.sysNameLineEdit.setEnabled(True)
        self.usrNameLineEdit.setEnabled(True)
        self.uriLineEdit.setEnabled(True)
        self.macLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.sysNameLineEdit.setEnabled(False)
        self.usrNameLineEdit.setEnabled(False)
        self.uriLineEdit.setEnabled(False)
        self.macLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.sysNameLineEdit.setText(self.parentObjHandle.decoderSystemName.value)
        self.usrNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.uriLineEdit.setText(self.parentObjHandle.decoderMqttURI.value)
        self.macLineEdit.setText(self.parentObjHandle.mac.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        try:
            self.parentObjHandle.decoderSystemName.value = self.sysNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.usrNameLineEdit.displayText()
            self.parentObjHandle.decoderMqttURI.value = self.uriLineEdit.displayText()
            self.parentObjHandle.mac.value = self.macLineEdit.displayText()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
        if res != rc.OK:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not change Adm State")
            msg.setInformativeText('ReturnCode: ' +     rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)
        pass

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return


    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_lightgroupsLinkDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LIGHTGROUP__LINK_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.sysNameLineEdit.setEnabled(True)
        self.usrNameLineEdit.setEnabled(True)
        self.linkNoSpinBox.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.sysNameLineEdit.setEnabled(False)
        self.usrNameLineEdit.setEnabled(True)
        self.linkNoSpinBox.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.sysNameLineEdit.setText(self.parentObjHandle.lgLinkSystemName.value)
        self.usrNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.linkNoSpinBox.setValue(self.parentObjHandle.lgLinkNo.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        try:
            self.parentObjHandle.lgLinkSystemName.value = self.sysNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.usrNameLineEdit.displayText()
            self.parentObjHandle.lgLinkNo.value = self.linkNoSpinBox.value()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if res:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(res))
                msg.setWindowTitle("Error")
                msg.exec_()
                return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_satLinkDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SATLINK_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.sysNameLineEdit.setEnabled(True)
        self.usrNameLineEdit.setEnabled(True)
        self.linkNoSpinBox.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(True)
        self.remCrcErrLineEdit.setEnabled(True)
        self.rxSymErrLineEdit.setEnabled(True)
        self.rxSizeErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.sysNameLineEdit.setEnabled(False)
        self.usrNameLineEdit.setEnabled(False)
        self.linkNoSpinBox.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.remCrcErrLineEdit.setEnabled(False)
        self.rxSymErrLineEdit.setEnabled(False)
        self.rxSizeErrLineEdit.setEnabled(False)
        self.wdErrLineEdit.setEnabled(False)
        self.clearStatsPushButton.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.sysNameLineEdit.setText(self.parentObjHandle.satLinkSystemName.value)
        self.usrNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.linkNoSpinBox.setValue(self.parentObjHandle.satLinkNo.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.rxCrcErr))
        self.remCrcErrLineEdit.setText(str(self.parentObjHandle.remCrcErr))
        self.rxSymErrLineEdit.setText(str(self.parentObjHandle.rxSymErr))
        self.rxSizeErrLineEdit.setText(str(self.parentObjHandle.rxSizeErr))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.wdErr))

    def setValues(self):
        try:
            self.parentObjHandle.satLinkSystemName.value = self.sysNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.usrNameLineEdit.displayText()
            self.parentObjHandle.satLinkNo.value = self.linkNoSpinBox.value()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if res:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(res))
                msg.setWindowTitle("Error")
                msg.exec_()
                return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

        # Update Stats widget
        self.updateStatsPushButton.clicked.connect(self.displayStats)

        # Clear Stats widget
        self.clearStatsPushButton.clicked.connect(self.clearStats)

    def clearStats(self):
        self.parentObjHandle.clearStats()
        self.displayStats()

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_lightGroupDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LIGHTGROUP_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(True)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.lgLinkAddressSpinBox.setEnabled(True)
        self.lgTypeComboBox.setEnabled(True)
        self.lgProp1ComboBox.setEnabled(True)
        self.lgProp2ComboBox.setEnabled(True)
        self.lgProp3ComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.lgShowingLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.lgLinkAddressSpinBox.setEnabled(False)
        self.lgTypeComboBox.setEnabled(False)
        self.lgProp1ComboBox.setEnabled(False)
        self.lgProp2ComboBox.setEnabled(False)
        self.lgProp3ComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.lgShowingLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(str(self.parentObjHandle.jmriLgSystemName.value))
        self.JMRIUserNameLineEdit.setText(str(self.parentObjHandle.userName.value))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.description.value))
        self.lgLinkAddressSpinBox.setValue(self.parentObjHandle.lgLinkAddr.value)
        self.lgTypeComboBox.setCurrentText(str(self.parentObjHandle.lgType.value))
        self.lgPropertyHandler()
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.lgShowingLineEdit.setText(str(self.parentObjHandle.lgShowing))

    def setValues(self):
        self.parentObjHandle.jmriLgSystemName.value = self.JMRISystemNameLineEdit.displayText()
        self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
        self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        self.parentObjHandle.lgLinkAddr.value = self.lgLinkAddressSpinBox.value()
        self.parentObjHandle.lgType.value = self.lgTypeComboBox.currentText()
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if res != rc.OK:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(res))
                msg.setWindowTitle("Error")
                msg.exec_()
                return res
        return rc.OK

    def lgPropertyHandler(self):
        pass # Define sub properties here

    def lgPropertySetHandler(self):
        pass # Define sub properties here

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_sateliteDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SATELITE_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.sysNameLineEdit.setEnabled(True)
        self.usrNameLineEdit.setEnabled(True)
        self.satAddrSpinBox.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(True)
        self.txCrcErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(True)
        self.updateStatsPushButton.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.sysNameLineEdit.setEnabled(False)
        self.usrNameLineEdit.setEnabled(False)
        self.satAddrSpinBox.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.txCrcErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(False)
        self.updateStatsPushButton.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.sysNameLineEdit.setText(self.parentObjHandle.satSystemName.value)
        self.usrNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.satAddrSpinBox.setValue(self.parentObjHandle.satLinkAddr.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.rxCrcErr))
        self.txCrcErrLineEdit.setText(str(self.parentObjHandle.txCrcErr))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.wdErr))

    def setValues(self):
        try:
            self.parentObjHandle.satSystemName.value = self.sysNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.usrNameLineEdit.displayText()
            self.parentObjHandle.satLinkAddr.value = self.satAddrSpinBox.value()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            res = self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
        if res:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not change Adm State")
            msg.setInformativeText('ReturnCode: ' + str(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

        # Update Stats widget
        self.updateStatsPushButton.clicked.connect(self.displayStats)

        # Clear Stast widget
        self.clearStatsPushButton.clicked.connect(self.clearStats)

    def clearStats(self):
        self.parentObjHandle.clearStats()
        self.displayStats()

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_sensorDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SENSOR_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(True)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.sensPortSpinBox.setEnabled(True)
        self.sensTypeComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.sensStateLineEdit.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.sensPortSpinBox.setEnabled(False)
        self.sensTypeComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.sensStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.jmriSensSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.sensPortSpinBox.setValue(self.parentObjHandle.sensPort.value)
        self.sensTypeComboBox.setCurrentText(self.parentObjHandle.sensType.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)
        self.sensStateLineEdit.setText(self.parentObjHandle.sensState)

    def setValues(self):
        try:
            self.parentObjHandle.jmriSensSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
            self.parentObjHandle.sensPort.value = self.sensPortSpinBox.value()
            self.parentObjHandle.sensType.value = self.sensTypeComboBox.currentText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
           res = self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
        if res:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not change Adm State")
            msg.setInformativeText('ReturnCode: ' + str(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()



class UI_actuatorDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(ACTUATOR_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.onTypeChanged()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(True)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.actPortSpinBox.setEnabled(True)
        self.actTypeComboBox.setEnabled(True)
        self.actSubTypeComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.actStateLineEdit.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.actPortSpinBox.setEnabled(False)
        self.actTypeComboBox.setEnabled(False)
        self.actSubTypeComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.actStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.jmriActSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.descriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.actPortSpinBox.setValue(self.parentObjHandle.actPort.value)
        self.actTypeComboBox.setCurrentText(self.parentObjHandle.actType.value)
        self.actSubTypeComboBox.setCurrentText(self.parentObjHandle.actSubType.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)
        self.actStateLineEdit.setText(self.parentObjHandle.actState)

    def setValues(self):
        try:
            self.parentObjHandle.jmriActSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
            self.parentObjHandle.actPort.value = self.actPortSpinBox.value()
            self.parentObjHandle.actType.value = self.actTypeComboBox.currentText()
            self.parentObjHandle.actSubType.value = self.actSubTypeComboBox.currentText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
        else:
            res = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if res:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(res))
                msg.setWindowTitle("Error")
                msg.exec_()
                return res
        return rc.OK

    def connectWidgetSignalsSlots(self):
        self.actTypeComboBox.currentTextChanged.connect(self.onTypeChanged)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def onTypeChanged(self):
        self.actSubTypeComboBox.clear()
        if self.actTypeComboBox.currentText() == "TURNOUT":
            self.actSubTypeComboBox.addItems(["SOLENOID", "SERVO"])
        elif self.actTypeComboBox.currentText() == "LIGHT":
            self.actSubTypeComboBox.addItems(["ONOFF"])
        elif self.actTypeComboBox.currentText() == "MEMORY":
            self.actSubTypeComboBox.addItems(["SOLENOID", "SERVO", "PWM", "ONOFF", "PULSE"])
        else:
            return rc.TYPE_VAL_ERR
        return rc.OK

    def accepted(self):
        res = self.setValues()
        if res == rc.OK:
            res = self.parentObjHandle.accepted()
            if res == rc.OK:
                self.close()
                return
            else:
                res = rc.getErrStr(res)
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Configuration validation failed")
        msg.setInformativeText('Info: ' + str(res))
        msg.setWindowTitle("Error")
        msg.exec_()
        return

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()
