import dataclasses
import os
import sys
from PyQt5.QtWidgets import QApplication, QDialog, QWidget, QMainWindow, QMessageBox, QMenu, QFileDialog, QComboBox, QLineEdit
from PyQt5.uic import loadUi
from PyQt5.Qt import QStandardItemModel, QStandardItem
from PyQt5.QtGui import QPalette, QFont, QColor, QTextCursor, QIcon
from PyQt5 import QtCore
import xml.etree.ElementTree as ET
import xml.dom.minidom as minidom
import time
from datetime import datetime, timedelta
import re
import threading
import traceback
from momResources import *
import webbrowser
import markdown
import imp
imp.load_source('rpc', '..\\rpc\\JMRIObjects.py')
from rpc import jmriObj
imp.load_source('alarmHandler', '..\\alarmHandler\\alarmHandler.py')
from alarmHandler import *
sys.path.append(os.path.dirname(os.path.realpath(__file__))+"\\..\\trace\\")
import trace
import imp
imp.load_source('sysState', '..\\sysState\\sysState.py')
from sysState import *
imp.load_source('rc', '..\\rc\\myRailIORc.py')
from rc import rc
imp.load_source('parseXml', '..\\xml\\parseXml.py')
from parseXml import *
imp.load_source('tz', '..\\tz\\tz.py')
from tz import *
imp.load_source('jmriObj', '..\\rpc\\JMRIObjects.py')
from jmriObj import *


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
CRASHDUMPOUTPUT_DIALOG_UI = "ui/Crash_Dump_Dialog.ui"
LIGHTGROUPSLINK_DIALOG_UI = "ui/LightGroupsLink_Dialog.ui"
SATLINK_DIALOG_UI = "ui/SatLink_Dialog.ui"
LIGHTGROUP__LINK_DIALOG_UI = "ui/LightGroupsLink_Dialog.ui"
LIGHTGROUP_DIALOG_UI = "ui/LightGroup_Dialog.ui"
LIGHTGROUP_INVENTORY_DIALOG_UI = "ui/LightGroups_Inventory_Dialog.ui"
SATELLITE_DIALOG_UI = "ui/Sat_Dialog.ui"
SENSOR_DIALOG_UI = "ui/Sensor_Dialog.ui"
SENSOR_INVENTORY_DIALOG_UI = "ui/Sensors_Inventory_Dialog.ui"
ACTUATOR_DIALOG_UI = "ui/Actuator_Dialog.ui"
ACTUATOR_INVENTORY_DIALOG_UI = "ui/Actuators_Inventory_Dialog.ui"
LOGOUTPUT_DIALOG_UI = "ui/Log_Output_Dialog.ui"
LOGSETTING_DIALOG_UI = "ui/Log_Setting_Dialog.ui"
SHOWALARMS_DIALOG_UI = "ui/Alarms_Show_Dialog.ui"
SHOWALARMSINVENTORY_DIALOG_UI = "ui/Alarms_Inventory_Dialog.ui"
SHOWS_SELECTED_ALARM_DIALOG_UI = "ui/Individual_Alarm_Show_Dialog.ui"
SHOWDECODERINVENTORY_DIALOG_UI = "ui/Decoders_Inventory_Dialog.ui"
CONFIGOUTPUT_DIALOG_UI = "ui/Config_Output_Dialog.ui"
AUTOLOAD_PREF_DIALOG_UI = "ui/AutoLoad_Pref_Dialog.ui"

# UI Icon resources
SERVER_ICON = "icons/server.png"
DECODER_ICON = "icons/decoder.png"
LINK_ICON = "icons/link.png"
SATELLITE_ICON = "icons/satellite.png"
TRAFFICLIGHT_ICON = "icons/traffic-light.png"
SENSOR_ICON = "icons/sensor.png"
ACTUATOR_ICON = "icons/servo.png"
ALARM_A_ICON = "icons/redBell.jpg"
ALARM_B_ICON = "icons/orangeBell.jpg"
ALARM_C_ICON = "icons/orangeBell.jpg"
NO_ALARM_ICON = "icons/greyBell.jpg"

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
        self.fnt = QFont('Open Sans', font_size)
        self.fnt.setBold(set_bold)
        self.setEditable(False)
        self.setForeground(color)
        self.obj = obj
        self.setFont(self.fnt)
        self.setText(txt)
        if icon != None:
            self.setIcon(QIcon(icon))

    def getObj(self):
        return self.obj

    def setColor(self, color):
        try:
            self.setForeground(color)
            self.setFont(self.fnt)
        except:
            pass

    def __delete__(self):
        super().__delete__()

class UI_mainWindow(QMainWindow):
    faultBlockMarkMoMObjSignal = QtCore.pyqtSignal(StandardItem, bool, name='faultBlockMo')
    inactivateMoMObjSignal = QtCore.pyqtSignal(StandardItem, name='inactivateMo')
    controlBlockMarkMoMObjSignal = QtCore.pyqtSignal(StandardItem, name='controlBlockMo')

    def __init__(self, parent = None):
        super().__init__(parent)
        self.faultBlockMarkMoMObjSignal.connect(self.__faultBlockMarkMoMObj)
        self.inactivateMoMObjSignal.connect(self.__inactivateMoMObj)
        self.controlBlockMarkMoMObjSignal.connect(self.__controlBlockMarkMoMObj)
        self.configFileDialog = UI_fileDialog("myRailIO main configuration", self)
        loadUi(MAIN_FRAME_UI, self)
        self.actionOpenConfig.setEnabled(True)
        self.actionSaveConfig.setEnabled(True)
        self.actionSaveConfigAs.setEnabled(True)
        self.autoLoadPreferences.setEnabled(True)
        self.connectActionSignalsSlots()
        self.connectWidgetSignalsSlots()
        self.MoMTreeModel = QStandardItemModel()
        self.MoMroot = self.MoMTreeModel.invisibleRootItem()
        self.topMoMTree.setModel(self.MoMTreeModel)
        self.topMoMTree.expandAll()

    def setParentObjHandle(self, parentObjHandle):
        self.parentObjHandle = parentObjHandle
        self.configFileDialog.regFileOpenCb(self.parentObjHandle.onXmlConfig)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_A, self._onAlarm, p_metaData = self)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_B, self._onAlarm, p_metaData = self)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_C, self._onAlarm, p_metaData = self)

    def registerMoMObj(self, objHandle, parentItem, string, type, displayIcon = None):
        if type == TOP_DECODER:
            fontSize = 14 
            setBold = True
            color = QColor(0, 0, 0)
            item = StandardItem(objHandle, txt=string, font_size=fontSize, set_bold = setBold, color = color, icon = displayIcon)
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
        elif type == SATELLITE_LINK:
            fontSize = 12
            setBold = True
            color = QColor(0, 0, 0)
        elif type == SATELLITE:
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
            item = StandardItem(objHandle, txt = string, font_size = fontSize, set_bold = setBold, color = color, icon = displayIcon)
            parentItem.appendRow(item)
            return item
        except:
            return None

    def reSetMoMObjStr(self, item, string):
        item.setText(string)

    def unRegisterMoMObj(self, item):
        self.MoMTreeModel.removeRow(item.row(), parent=self.MoMTreeModel.indexFromItem(item).parent())

    def faultBlockMarkMoMObj(self, item, faultState):
        print("faultBlockMarkMoMObj" + str(faultState))
        self.faultBlockMarkMoMObjSignal.emit(item, faultState)

    def __faultBlockMarkMoMObj(self, item, faultState):
        print("__faultBlockMarkMoMObj" + str(faultState))
        if faultState:
            item.setColor(QColor(255, 0, 0))
        else:
            item.setColor(QColor(119,150,56))
            
    def inactivateMoMObj(self, item):
        self.inactivateMoMObjSignal.emit(item)
        
    def __inactivateMoMObj(self, item):
        item.setColor(QColor(200, 200, 200))

    def controlBlockMarkMoMObj(self, item):
        print("controlBlockMarkMoMObj")
        self.controlBlockMarkMoMObjSignal.emit(item)

    def __controlBlockMarkMoMObj(self, item):
        print("__controlBlockMarkMoMObj")
        item.setColor(QColor(255, 200, 0))

    def _onAlarm(self, p_criticality, p_noOfAlarms, p_object):
        if alarmHandler.getNoOfActiveAlarms(ALARM_CRITICALITY_A):
            self.alarmPushButton.setIcon(QIcon(ALARM_A_ICON))
            return
        if alarmHandler.getNoOfActiveAlarms(ALARM_CRITICALITY_B):
            self.alarmPushButton.setIcon(QIcon(ALARM_B_ICON))
            return
        if alarmHandler.getNoOfActiveAlarms(ALARM_CRITICALITY_C):
            self.alarmPushButton.setIcon(QIcon(ALARM_C_ICON))
            return
        self.alarmPushButton.setIcon(QIcon(NO_ALARM_ICON))

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
        addAction = menu.addAction("Add")
        addAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_ADD else addAction.setEnabled(False)
        deleteAction = menu.addAction("Delete")
        deleteAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_DELETE else deleteAction.setEnabled(False)
        editAction = menu.addAction("Edit")
        editAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_EDIT else editAction.setEnabled(False)
        viewAction = menu.addAction("View") if stdItemObj.getObj().getMethods() & METHOD_VIEW else None
        viewAction.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_VIEW else viewAction.setEnabled(False)
        menu.addSeparator()
        copyAction = menu.addAction("Copy")
        copyAction.setEnabled(False)
        pasteAction = menu.addAction("Paste")
        pasteAction.setEnabled(False)
        FindAction = menu.addAction("Find")
        FindAction.setEnabled(False)
        menu.addSeparator()
        enableAction = menu.addAction("Enable")
        enableAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE else enableAction.setEnabled(False)
        enableRecursAction = menu.addAction("Enable - recursive")
        enableRecursAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE_RECURSIVE else enableRecursAction.setEnabled(False)
        disableAction = menu.addAction("Disable")
        disableAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE else disableAction.setEnabled(False)
        disableRecursAction = menu.addAction("Disable - recursive")
        disableRecursAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE_RECURSIVE else disableRecursAction.setEnabled(False)
        menu.addSeparator()
        restartAction = menu.addAction("Restart")
        restartAction.setEnabled(True)  if stdItemObj.getObj().getActivMethods() & METHOD_RESTART else restartAction.setEnabled(False)
        action = menu.exec_(self.topMoMTree.mapToGlobal(point))
        res = 0
        try:
            if action == addAction: res = stdItemObj.getObj().add()
            if action == deleteAction: res = stdItemObj.getObj().delete(top=True)
            if action == editAction: res = stdItemObj.getObj().edit()
            if action == viewAction: res = stdItemObj.getObj().view()
            #if action == copyAction: res = stdItemObj.getObj().copy()
            #if action == pasteAction: res = stdItemObj.getObj().paste()
            #if action == findAction: res = stdItemObj.getObj().copy()
            if action == enableAction: res = stdItemObj.getObj().enable()
            if action == enableRecursAction: res = stdItemObj.getObj().enableRecurse()
            if action == disableAction: res = stdItemObj.getObj().disable()
            if action == disableRecursAction: res = stdItemObj.getObj().disableRecurse()
            if action == restartAction: res = stdItemObj.getObj().decoderRestart()
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
        # ------------
        self.actionAdd.triggered.connect(self.add)
        self.actionDelete.triggered.connect(self.delete)
        self.actionEdit.triggered.connect(self.edit)
        self.actionView.triggered.connect(self.view)
        self.actionEnable.triggered.connect(self.enable)
        self.actionEnable_recursive.triggered.connect(self.enableRecursive)
        self.actionDisable.triggered.connect(self.disable)
        self.actionDisable_recursive.triggered.connect(self.disableRecursive)
        self.actionRestart_3.triggered.connect(self.restart)
        self.actionmyRailIO_preferences.triggered.connect(self.editMyRailIOPreferences)

        # View actions
        # ------------
        self.actionConfiguration.triggered.connect(self.showConfig)
        self.actionAlarms.triggered.connect(self.showAlarms)

        # Tools actions
        # -------------

        # Inventory actions
        # -----------------
        self.actionShow_AlarmInventory.triggered.connect(self.showAlarmInventory)
        self.actionShow_DecoderInventory.triggered.connect(self.showDecoderInventory)
        self.actionShow_LightGroupInventory.triggered.connect(self.showLightGroupInventory)
        self.actionShow_SensorInventory.triggered.connect(self.showSensorInventory)
        self.actionShow_ActuatorInventory.triggered.connect(self.showActuatorInventory)

        # Debug actions
        # -----------------
        self.actionOpenLogProp.triggered.connect(self.setLogProperty)
        self.actionOpen_Log_window.triggered.connect(self.log)

        # Help actions
        # ------------
        self.actionServer_version.triggered.connect(self.version)
        self.actionAbout_myRailIO.triggered.connect(self.about)
        self.actionSearch_for_help.triggered.connect(self.searchHelp)
        self.actionLicense.triggered.connect(self.license)
        self.actionHow_to_contribute.triggered.connect(self.contribute)
        self.actionSource.triggered.connect(self.source)

    def connectWidgetSignalsSlots(self):
        # MoM tree widget
        self.topMoMTree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.topMoMTree.customContextMenuRequested.connect(self.MoMMenuContextTree)
        self.menuEdit.aboutToShow.connect(self.updateEditMenu)

        # Alarm widget
        #self.alarmPushButton.clicked.connect(self.showAlarms)

    def openConfigFile(self):
        self.configFileDialog.openFileDialog()

    def saveConfigFile(self):
        self.configFileDialog.saveFileDialog(self.parentObjHandle.getXmlConfigTree(text=True))

    def saveConfigFileAs(self):
        self.configFileDialog.saveFileAsDialog(self.parentObjHandle.getXmlConfigTree(text=True))
        
    def showConfig(self):
        self.configOutputDialog = UI_getConfig(self, self.parentObjHandle.getXmlConfigTree(text=True))
        self.configOutputDialog.show()

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
        
    def updateEditMenu(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            self.actionAdd.setEnabled(False)
            self.actionDelete.setEnabled(False)
            self.actionEdit.setEnabled(False)
            self.actionView.setEnabled(False)
            self.actionEnable.setEnabled(False)
            self.actionEnable_recursive.setEnabled(False)
            self.actionDisable.setEnabled(False)
            self.actionDisable_recursive.setEnabled(False)
            self.actionRestart_3.setEnabled(False)
            return
        stdItemObj = item.model().itemFromIndex(item)
        self.actionAdd.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_ADD else self.actionAdd.setEnabled(False)
        self.actionDelete.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_DELETE else self.actionDelete.setEnabled(False)
        self.actionEdit.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_EDIT else self.actionEdit.setEnabled(False)
        self.actionView.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_VIEW else self.actionView.setEnabled(False)
        self.actionEnable.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE else self.actionEnable.setEnabled(False)
        self.actionEnable_recursive.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_ENABLE_RECURSIVE else self.actionEnable_recursive.setEnabled(False)
        self.actionDisable.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE else self.actionDisable.setEnabled(False)
        self.actionDisable_recursive.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_DISABLE_RECURSIVE else self.actionDisable_recursive.setEnabled(False)
        self.actionRestart_3.setEnabled(True) if stdItemObj.getObj().getActivMethods() & METHOD_RESTART else self.actionRestart_3.setEnabled(False)

    def add(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().add()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not add object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def delete(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().delete()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not delete object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()

    def edit(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().edit()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not edit object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def view(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().view()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not view object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def enable(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().enable()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not enable object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def enableRecursive(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().enableRecurse()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not enable object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def disable(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().disable()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not disable object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def disableRecursive(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().disableRecurse()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not disable object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()
            
    def restart(self):
        try:
            item = self.topMoMTree.selectedIndexes()[0]
        except:
            return
        stdItemObj = item.model().itemFromIndex(item)
        res = stdItemObj.getObj().decoderRestart()
        if res != rc.OK and res != None:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error could not restart object")
            msg.setInformativeText(rc.getErrStr(res))
            msg.setWindowTitle("Error")
            msg.exec_()

    def log(self):
        self.log = UI_logDialog(self.parentObjHandle)
        self.log.show()

    def setLogProperty(self):
        self.logsetting = UI_logSettingDialog(self.parentObjHandle)
        self.logsetting.show()

    def editMyRailIOPreferences(self):
        self.dialog = UI_topDialog(self.parentObjHandle, edit=True)
        self.dialog.show()

    def showAlarms(self):
        self.alarmWidget = UI_alarmShowDialog(self.parentObjHandle)
        self.alarmWidget.show()

    def showAlarmInventory(self):
        self.alarmInventoryWidget = UI_alarmInventoryShowDialog(self.parentObjHandle)
        self.alarmInventoryWidget.show()
        
    def showDecoderInventory(self):
        self.decoderInventoryWidget = UI_decoderInventoryShowDialog(self.parentObjHandle)
        self.decoderInventoryWidget.show()
        
    def showLightGroupInventory(self):
        self.lightGroupInventoryWidget = UI_lightGroupInventoryShowDialog(self.parentObjHandle)
        self.lightGroupInventoryWidget.show()
        
    def showSensorInventory(self):
        self.sensorInventoryWidget = UI_sensorInventoryShowDialog(self.parentObjHandle)
        self.sensorInventoryWidget.show()
        
    def showActuatorInventory(self):
        self.actuatorInventoryWidget = UI_actuatorInventoryShowDialog(self.parentObjHandle)
        self.actuatorInventoryWidget.show()

    def version(self):
        with open('VERSION.md', 'r') as file:
            versionMd = file.read()
            QMessageBox.about(self, "myRailIO server version", markdown.markdown(versionMd))

    def about(self):
        with open('../../../ABOUT_SHORT.md', 'r') as file:
            aboutMd = file.read()
            QMessageBox.about(self, "About myRailIO", markdown.markdown(aboutMd))

    def license(self):
        with open('../../../LICENSE_SHORT.md', 'r') as file:
            licenseMd = file.read()
            QMessageBox.about(self, "myRailIO Lisense", markdown.markdown(licenseMd))
            
    def searchHelp(self):
        QMessageBox.about(self, "myRailIO Help", markdown.markdown("help can me found at <https://www.myrail.io/resources>"))

    def contribute(self):
        QMessageBox.about(self, "myRailIO How to contribute", markdown.markdown("Sign up for your contribution <https://myrail.io/#Join>"))

    def source(self):
        QMessageBox.about(self, "myRailIO Source", markdown.markdown("All sources can be found at <https://github.com/jonasbjurel/myRailIO>"))


class UI_fileDialog(QWidget):
    def __init__(self, fileContext, parentObjHandle, autoLoad = None, autoLoadDelay = None, path = None, fileName = None):
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
            filePrefsXml = ET.Element("myRailIOFilePreference")
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
    def __init__(self, parentObjHandle, autoLoad, autoLoadDelay, parent = None):
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
        self.parentObjHandle.setAutoLoad(True if self.autoLoadConfigComboBox.currentText() == "Yes" else False, self.autoLoadConfigDelaySpinBox.value() if self.autoLoadConfigComboBox.currentText() == "Yes" else 0)
        self.close()

    def rejected(self):
        self.close()



class UI_logDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
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
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LOGSETTING_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.logSettingVerbosityComboBox.setCurrentText(long2shortVerbosity(self.parentObjHandle.logVerbosity.value))

    def connectWidgetSignalsSlots(self):
        self.logSettingConfirmButtonBox.accepted.connect(self.accepted)
        self.logSettingConfirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.parentObjHandle.setLogVerbosity(short2longVerbosity(self.logSettingVerbosityComboBox.currentText()), commit = True)
        self.close()

    def rejected(self):
        self.close()



class UI_alarmShowDialogUpdateWorker(QtCore.QObject):
    updateAlarms = QtCore.pyqtSignal()
    def __init__(self, alarmShowHandle = None):
        super(self.__class__, self).__init__(alarmShowHandle)

    @QtCore.pyqtSlot()
    def start(self):
        self.run = True
        while self.run:
            self.updateAlarms.emit()
            QtCore.QThread.sleep(1)

    def stop(self):
        self.run = False



class UI_alarmShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SHOWALARMS_DIALOG_UI, self)
        self.alarmHistoryComboBox.setCurrentText("Active")
        self.severityFilterComboBox.setCurrentText("*")
        self.freeSearchTextEdit.setText("*")
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.alarmTableModel = alarmTableModel(self)
        self.proxymodel.setSourceModel(self.alarmTableModel)
        self.alarmsTableView.setModel(self.proxymodel)
        self.alarmsTableView.setSortingEnabled(True)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_A , self._onAlarm)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_B , self._onAlarm)
        alarmHandler.regAlarmCb(ALARM_CRITICALITY_C , self._onAlarm)
        self._rePopulateSrcFilters()
        self.updateTableWorker = UI_alarmShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateAlarms.connect(self.updateAlarmTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
            self.updateTableWorker.stop()
            self.updateTableWorkerThread.quit()
            self.updateTableWorkerThread.wait()

    def updateAlarmTable(self):
        self.proxymodel.beginResetModel()
        self.alarmTableModel.beginResetModel()
        self.alarmTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self._rePopulateSrcFilters()
        self.alarmsTableView.resizeColumnsToContents()
        self.alarmsTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.alarmHistoryComboBox.currentTextChanged.connect(self.alarmHistoryHandler)
        self.severityFilterComboBox.currentTextChanged.connect(self.severityFilterHandler)
        self.sourceFilterComboBox.currentTextChanged.connect(self.sourceFilterHandler)
        self.freeSearchTextEdit.textChanged.connect(self.freeSearchHandler)
        self.alarmExportSelectedPushButton.clicked.connect(self.saveSelectedAlarms)
        self.alarmExportAllPushButton.clicked.connect(self.saveAllAlarms)
        self.alarmsTableView.clicked.connect(self.showSelectedAlarm)

    def alarmHistoryHandler(self):
        if self.alarmHistoryComboBox.currentText() == "Active":
            self.alarmTableModel.setAlarmHistory(False)
        elif self.alarmHistoryComboBox.currentText() == "History":
            self.alarmTableModel.setAlarmHistory(True)
        self._rePopulateSrcFilters()

    def severityFilterHandler(self):
        if self.severityFilterComboBox.currentText() == "*":
            self.alarmTableModel.setAlarmCriticality(ALARM_ALL_CRITICALITY)
        elif self.severityFilterComboBox.currentText() == "A-Level":
            self.alarmTableModel.setAlarmCriticality(ALARM_CRITICALITY_A)
        elif self.severityFilterComboBox.currentText() == "B-Level":
            self.alarmTableModel.setAlarmCriticality(ALARM_CRITICALITY_B)
        elif self.severityFilterComboBox.currentText() == "C-Level":
            self.alarmTableModel.setAlarmCriticality(ALARM_CRITICALITY_C)

    def sourceFilterHandler(self):
        self.alarmTableModel.setSrcFilter(self.sourceFilterComboBox.currentText())

    def freeSearchHandler(self):
        palette = self.freeSearchTextEdit.palette()
        try:
            self.filterString.setPattern("")
            self.proxymodel.setFilterRegExp(self.filterString)
        except:
            pass
        if self.freeSearchTextEdit.toPlainText() == "" or self.freeSearchTextEdit.toPlainText() == "*":
            palette.setColor(QPalette.Text, QColor("black"))
            self.freeSearchTextEdit.setPalette(palette)
            return
        columnMatch = re.search("(?<=\@)(.*)(?=\@)", self.freeSearchTextEdit.toPlainText())
        if columnMatch == None:
            palette.setColor(QPalette.Text, QColor("red"))
            self.freeSearchTextEdit.setPalette(palette)
            return
        try:
            regex = re.split("@" + columnMatch.group() + "@", self.freeSearchTextEdit.toPlainText(), maxsplit=0, flags=0)[1]
        except:
            palette.setColor(QPalette.Text, QColor("red"))
            self.freeSearchTextEdit.setPalette(palette)
            return
        column = None
        for headingColumnItter in range(self.proxymodel.columnCount()):
            if self.proxymodel.headerData(headingColumnItter, QtCore.Qt.Horizontal, QtCore.Qt.DisplayRole) == columnMatch.group():
                column = headingColumnItter
                break
        if column == None:
            palette.setColor(QPalette.Text, QColor("red"))
            self.freeSearchTextEdit.setPalette(palette)
            return
        self.filterString = QtCore.QRegExp(  regex,
                                            QtCore.Qt.CaseInsensitive,
                                            QtCore.QRegExp.RegExp
                                            )
        if not self.filterString.isValid():
            palette.setColor(QPalette.Text, QColor("red"))
            self.freeSearchTextEdit.setPalette(palette)
        else:
            palette.setColor(QPalette.Text, QColor("black"))
            self.freeSearchTextEdit.setPalette(palette)
        self.proxymodel.setFilterRegExp(self.filterString)
        self.proxymodel.setFilterKeyColumn(column)

    def _rePopulateSrcFilters(self):
        self.sourceFilterComboBox.clear()
        if self.alarmHistoryComboBox.currentText() == "Active":
            srcFilterList = ["*"]
            srcFilterList.extend(alarmHandler.getSrcs())
            self.sourceFilterComboBox.addItems(srcFilterList)
        else:
            srcFilterList = ["*"]
            srcFilterList.extend(alarmHandler.getSrcs(True))
            self.sourceFilterComboBox.addItems(srcFilterList)
        self.sourceFilterComboBox.setCurrentText("*")

    def showSelectedAlarm(self, p_clickedIndex):
        self.individualAlarmWidget = UI_individualAlarmShowDialog(p_clickedIndex, self.parentObjHandle, parent = self)
        self.individualAlarmWidget.show()

    def saveAllAlarms(self):
        self.saveAlarms(self.alarmTableModel.formatAllAlarmsCsv())

    def saveSelectedAlarms(self):
        self.saveAlarms(self.alarmTableModel.formatSelectedAlarmsCsv())

    def saveAlarms(self, p_alarmsCsv):
        options = QFileDialog.Options()
        #options |= QFileDialog.DontUseNativeDialog
        try:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a file name for saving", self.path,"CSV Files (*.csv);;All Files (*)", options=options)
        except:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a file name for saving", "","CSV Files (*.csv);;All Files (*)", options=options)
        if fileName:
            try:
                ext = extTxt.split("(")[1].split(")")[0].split(".")[1]
                fileName = fileName + "." + ext
            except:
                pass
            f = open(fileName, "w")
            f.write(p_alarmsCsv)
            f.close()
            self.path = fileName.rsplit("/", 1)[0]
            self.fileName = fileName

    def _onAlarm(self, p_criticality, p_noOfAlarms, p_metaData):
        pass



class UI_alarmInventoryShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SHOWALARMSINVENTORY_DIALOG_UI, self)
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.alarmInventoryTableModel = alarmInventoryTableModel(self)
        self.proxymodel.setSourceModel(self.alarmInventoryTableModel)
        self.alarmInventoryTableView.setModel(self.proxymodel)
        self.alarmInventoryTableView.setSortingEnabled(True)
        self.updateTableWorker = UI_alarmShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateAlarms.connect(self.updateAlarmInventoryTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
            self.updateTableWorker.stop()
            self.updateTableWorkerThread.quit()
            self.updateTableWorkerThread.wait()

    def updateAlarmInventoryTable(self):
        self.proxymodel.beginResetModel()
        self.alarmInventoryTableModel.beginResetModel()
        self.alarmInventoryTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self.alarmInventoryTableView.resizeColumnsToContents()
        self.alarmInventoryTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.alarmInventoryTableView.clicked.connect(self.showSelectedAlarm)

    def showSelectedAlarm(self, p_clickedIndex):
        self.individualAlarmWidget = UI_individualAlarmShowDialog(p_clickedIndex, self.parentObjHandle, parent = self)
        self.individualAlarmWidget.show()



class UI_individualAlarmShowDialog(QDialog):
    def __init__(self, p_clickedIndex, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.parent = parent
        self.clickedIndex = p_clickedIndex
        loadUi(SHOWS_SELECTED_ALARM_DIALOG_UI, self)
        self._populate()

    def _populate(self):
        alarm = None
        try:
            tableModel = self.parent.alarmInventoryTableModel
        except:
            tableModel = self.parent.alarmTableModel

        if tableModel.isFirstColumnObjectId():
            alarm = tableModel.getAlarmObjFromObjId(int(self.parent.alarmInventoryTableView.model().index(self.clickedIndex.row(), 0).data()))
        elif tableModel.isFirstColumnInstanceId():
            alarm = tableModel.getAlarmObjFromInstanceId(int(self.parent.alarmsTableView.model().index(self.clickedIndex.row(), 0).data()))
        else:
            #PANIC 
            pass
        if not alarm:
            return

        self.objIdTextEdit.setText(str(alarm.getGlobalAlarmClassObjUid()))
        if alarm.getGlobalAlarmInstanceUid() != None:
            self.instanceIdTextEdit.setText(str(alarm.getGlobalAlarmInstanceUid()))
        else:
            self.instanceIdTextEdit.setText("-")
        self.typeTextEdit.setText(alarm.getType())
        self.sourceTextEdit.setText(alarm.getSource())
        self.sloganTextEdit.setText(alarm.getSloganDescription())
        self.severityTextEdit.setText(alarm.getSeverity())
        self.activeTextEdit.setText(str(alarm.getIsActive()))
        if alarm.getCurrentOriginActiveInstance() != 0:
            self.parentAlarmIdTextEdit.setText(str(alarm.getCurrentOriginActiveInstance()))
        elif alarm.getCurrentOriginActiveInstance(True) != 0:
            self.parentAlarmIdTextEdit.setText(str(alarm.getCurrentOriginActiveInstance(True)))
        else:
            self.parentAlarmIdTextEdit.setText("-")
        if alarm.getRaiseUtcTime() != None:
            self.raiseTimeTextEdit.setText(alarm.getRaiseUtcTime())
        else:
            self.raiseTimeTextEdit.setText("-")
        if alarm.getRaiseEpochTime() != None:
            self.raiseTimeEpochTextEdit.setText(str(alarm.getRaiseEpochTime()))
        else:
            self.raiseTimeEpochTextEdit.setText("-")
        if alarm.getRaiseReason() != None:
            self.raiseReasonTextEdit.setText(alarm.getRaiseReason())
        else:
            self.raiseReasonTextEdit.setText("-")
        if alarm.getCeaseUtcTime() != None:
            self.ceaseTimeTextEdit.setText(alarm.getCeaseUtcTime())
        else:
            self.ceaseTimeTextEdit.setText("-")
        if alarm.getCeaseEpochTime() != None:
            self.ceaseTimeEpochTextEdit.setText(str(alarm.getCeaseEpochTime()))
        else:
            self.ceaseTimeEpochTextEdit.setText("-")
        if alarm.getCeaseReason() != None:
            self.ceaseReasonTextEdit.setText(alarm.getCeaseReason())
        else:
            self.ceaseReasonTextEdit.setText("-")
        if alarm.getLastingTime() != None:
            self.durationTextEdit.setText(alarm.getLastingTime())
        else:
            self.durationTextEdit.setText("-")
        if alarm.getLastingTimeS() != None:
            self.durationSTextEdit.setText(str(alarm.getLastingTimeS()))
        else:
            self.durationSTextEdit.setText("-")
        if len(alarmHandler.getAlarmObjHistory()[alarm.getGlobalAlarmClassObjUid() - 1]) > 0:
            self.occuranceTextEdit.setText(str(len(alarmHandler.getAlarmObjHistory()[alarm.getGlobalAlarmClassObjUid() - 1]))) # I THINK IT SHOULD BE -1 sinse AlarmObjID is 1 enumerated
        else:
            self.occuranceTextEdit.setText("-")
        index = 0
        for alarmItter in alarmHandler.getAlarmObjHistory()[alarm.getGlobalAlarmClassObjUid() - 1]: # I THINK IT SHOULD BE -1 sinse AlarmObjID is 1 enumerated
            if alarmItter.getGlobalAlarmInstanceUid() == alarm.getGlobalAlarmInstanceUid():
                break
            index += 1
        if index > 0:
            self.prevOccuranceTextEdit.setText(alarmHandler.getAlarmObjHistory()[alarm.getGlobalAlarmClassObjUid() - 1][index - 1].getRaiseUtcTime()) # I THINK IT SHOULD BE -1 sinse AlarmObjID is 1 enumerated
        else:
            self.prevOccuranceTextEdit.setText("-")
        oneHBack = (datetime.utcnow() - timedelta(hours = 1)).strftime('UTC: %Y-%m-%d %H:%M:%S.%f')
        occurances = 0
        for alarmItter in alarmHandler.getAlarmObjHistory()[alarm.getGlobalAlarmClassObjUid() - 1]: # I THINK IT SHOULD BE -1 sinse AlarmObjID is 1 enumerated
            if alarmItter.getRaiseUtcTime() < oneHBack:
                break
            occurances += 1
        if occurances > 0:
            self.intencityTextEdit.setText(str(occurances))
        else:
            self.intencityTextEdit.setText("-")
        if alarm.getIsActive() == False:
            self.alarmPushButton.setIcon(QIcon(NO_ALARM_ICON))
            return
        if alarm.getSeverity() == "A":
            self.alarmPushButton.setIcon(QIcon(ALARM_A_ICON))
            return
        elif alarm.getSeverity() == "B":
            self.alarmPushButton.setIcon(QIcon(ALARM_B_ICON))
            return
        elif alarm.getSeverity() == "C":
            self.alarmPushButton.setIcon(QIcon(ALARM_C_ICON))
            return



class UI_getConfig(QDialog):
    def __init__(self, parentObjHandle, configuration, parent = None):
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

        # Save as widget
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
    def __init__(self, parentObjHandle, resourceTypes, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(ADD_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        resourceTypeList = []
        if resourceTypes & TOP_DECODER: resourceTypeList.append("Top decoder")
        if resourceTypes & DECODER: resourceTypeList.append("Decoder")
        if resourceTypes & LIGHT_GROUP_LINK: resourceTypeList.append("Light group link")
        if resourceTypes & LIGHT_GROUP: resourceTypeList.append("Light group")
        if resourceTypes & SATELLITE_LINK: resourceTypeList.append("Satellite link")
        if resourceTypes & SATELLITE: resourceTypeList.append("Satellite")
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
        elif resourceTypeStr == "Satellite link": resourceType = SATELLITE_LINK
        elif resourceTypeStr == "Satellite": resourceType = SATELLITE
        elif resourceTypeStr == "Sensor": resourceType = SENSOR
        elif resourceTypeStr == "Actuator": resourceType = ACTUATOR
        print("ResourceTypeStr: " + resourceTypeStr)
        self.parentObjHandle.addChild(resourceType) #Check return code and handle error
        self.close()

    def rejected(self):
        self.close()



class UI_topDialog(QDialog):
    topDecoderHandle = None
    def __init__(self, parentObjHandle, edit = False, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(TOP_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.timeZoneComboBox.addItems(tz.getClearTextTimeZones())
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        # Git configuration/operation section
        self.gitUrlLineEdit.setEnabled(False) #Missing functionality
        self.gitBranchComboBox.setEnabled(False) #Missing functionality
        self.gitTagComboBox.setEnabled(False) #Missing functionality
        self.gitCiPushButton.setEnabled(False) #Missing functionality
        self.gitCoPushButton.setEnabled(False) #Missing functionality
        # General myRailIO Meta-data
        self.authorLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.versionLineEdit.setEnabled(True)
        self.dateLineEdit.setEnabled(True)
        # General myRailIO services configuration
        self.ntpLineEdit.setEnabled(True)
        self.timeZoneComboBox.setEnabled(True)
        self.rsyslogUrlLineEdit.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.rsyslogFileLineEdit.setEnabled(True)
        self.rsysLogFileBrowsePushButton.setEnabled(True)
        self.logRotateNoFilesSpinBox.setEnabled(True)
        self.logRotateFileSizeLineEdit.setEnabled(True)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setEnabled(True)
        self.RPC_Port_LineEdit.setEnabled(True)
        self.JMRIRpcKeepalivePeriodDoubleSpinBox.setEnabled(True)
        # MQTT Southbound API Configuration
        self.DecoderPingPeriodDoubleSpinBox.setEnabled(True)
        self.DecoderKeepAlivePeriodDoubleSpinBox.setEnabled(True)
        self.decoderFailSafeCheckBox.setEnabled(True)
        self.trackFailSafeCheckBox.setEnabled(True)
        self.MQTT_URI_LineEdit.setEnabled(True)
        self.MQTT_Port_LineEdit.setEnabled(True)
        self.MQTT_TOPIC_PREFIX_LineEdit.setEnabled(True)
        # myRailIO states
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        # Top-decoder/myRailIO preference Confirm or cancel
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        # Git configuration/operation section
        self.gitUrlLineEdit.setEnabled(False) #Missing functionality
        self.gitBranchComboBox.setEnabled(False) #Missing functionality
        self.gitTagComboBox.setEnabled(False) #Missing functionality
        self.gitCiPushButton.setEnabled(False) #Missing functionality
        self.gitCoPushButton.setEnabled(False) #Missing functionality
        # General myRailIO Meta-data
        self.authorLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.versionLineEdit.setEnabled(False)
        self.dateLineEdit.setEnabled(False)
        # General myRailIO services configuration
        self.ntpLineEdit.setEnabled(False)
        self.timeZoneComboBox.setEnabled(False)
        self.rsyslogUrlLineEdit.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.rsyslogFileLineEdit.setEnabled(False)
        self.rsysLogFileBrowsePushButton.setEnabled(False)
        self.logRotateNoFilesSpinBox.setEnabled(False)
        self.logRotateFileSizeLineEdit.setEnabled(False)
        # MQTT Southbound API Configuration
        self.DecoderPingPeriodDoubleSpinBox.setEnabled(False)
        self.DecoderKeepAlivePeriodDoubleSpinBox.setEnabled(False)
        self.decoderFailSafeCheckBox.setEnabled(False)
        self.trackFailSafeCheckBox.setEnabled(False)
        self.MQTT_URI_LineEdit.setEnabled(False)
        self.MQTT_Port_LineEdit.setEnabled(False)
        self.MQTT_TOPIC_PREFIX_LineEdit.setEnabled(False)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setEnabled(False)
        self.RPC_Port_LineEdit.setEnabled(False)
        self.JMRIRpcKeepalivePeriodDoubleSpinBox.setEnabled(False)
        # General myRailIO states
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
        self.gitUrlLineEdit.setText(str(self.parentObjHandle.gitUrl.value))
        self.gitBranchComboBox.setCurrentText(str(self.parentObjHandle.gitBranch.value))
        self.gitTagComboBox.setCurrentText(str(self.parentObjHandle.gitTag.value))
        # General myRailIO Meta-data
        self.authorLineEdit.setText(str(self.parentObjHandle.author.value))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.description.value))
        self.versionLineEdit.setText(str(self.parentObjHandle.version.value))
        self.dateLineEdit.setText(str(self.parentObjHandle.date.value))
        # General myRailIO services configuration
        self.ntpLineEdit.setText(str(self.parentObjHandle.ntpUri.value))
        self.timeZoneComboBox.setCurrentText(self.parentObjHandle.tzClearText.value)
        self.rsyslogUrlLineEdit.setText(self.parentObjHandle.rsyslogUrl.value)
        self.logVerbosityComboBox.setCurrentText(str(long2shortVerbosity(self.parentObjHandle.logVerbosity.value)))
        self.rsyslogFileLineEdit.setText(self.parentObjHandle.logFile.value)
        self.logRotateNoFilesSpinBox.setValue(self.parentObjHandle.logRotateNoFiles.value)
        self.logRotateFileSizeLineEdit.setText(str(self.parentObjHandle.logFileSize.value))
        # MQTT Southbound API Configuration
        self.DecoderPingPeriodDoubleSpinBox.setValue(self.parentObjHandle.decoderMqttPingPeriod.value)
        self.DecoderKeepAlivePeriodDoubleSpinBox.setValue(self.parentObjHandle.decoderMqttKeepAlivePeriod.value)
        self.decoderFailSafeCheckBox.setChecked(self.parentObjHandle.decoderFailSafe.value)
        self.trackFailSafeCheckBox.setChecked(self.parentObjHandle.trackFailSafe.value)
        self.MQTT_URI_LineEdit.setText(self.parentObjHandle.decoderMqttURI.value)
        self.MQTT_Port_LineEdit.setText(str(self.parentObjHandle.decoderMqttPort.value))
        self.MQTT_TOPIC_PREFIX_LineEdit.setText(self.parentObjHandle.decoderMqttTopicPrefix.value)
        # JMRI RPC Northbound Configuration
        self.RPC_URI_LineEdit.setText(self.parentObjHandle.jmriRpcURI.value)
        self.RPC_Port_LineEdit.setText(str(self.parentObjHandle.jmriRpcPortBase.value))
        self.JMRIRpcKeepalivePeriodDoubleSpinBox.setValue(self.parentObjHandle.JMRIRpcKeepAlivePeriod.value)
        # General myRailIO states
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        # Git configuration/operation section
        self.parentObjHandle.gitUrl.value = self.gitUrlLineEdit.displayText()
        self.parentObjHandle.gitBranch.value = self.gitBranchComboBox.currentText()
        self.parentObjHandle.gitTag.value = self.gitTagComboBox.currentText()
        # General myRailIO Meta-data
        self.parentObjHandle.author.value = self.authorLineEdit.displayText()
        self.parentObjHandle.description.value = self.descriptionLineEdit.displayText()
        self.parentObjHandle.version.value = self.versionLineEdit.displayText()
        self.parentObjHandle.date.value = self.dateLineEdit.displayText()
        # General myRailIO services configuration
        self.parentObjHandle.ntpUri.value = self.ntpLineEdit.displayText()
        self.parentObjHandle.tzClearText.value = self.timeZoneComboBox.currentText()
        self.parentObjHandle.tzEncodedText.value = tz.getEncodedTimeZones(self.parentObjHandle.tzClearText.candidateValue)
        self.parentObjHandle.rsyslogUrl.value = self.rsyslogUrlLineEdit.displayText()
        self.parentObjHandle.logVerbosity.value = short2longVerbosity(self.logVerbosityComboBox.currentText())
        self.parentObjHandle.logFile.value = self.rsyslogFileLineEdit.displayText()
        self.parentObjHandle.logRotateNoFiles.value = self.logRotateNoFilesSpinBox.value()
        self.parentObjHandle.logFileSize.value = int(self.logRotateFileSizeLineEdit.displayText())
        # MQTT Southbound API Configuration
        self.parentObjHandle.decoderMqttPingPeriod.value = self.DecoderPingPeriodDoubleSpinBox.value()
        self.parentObjHandle.decoderMqttKeepAlivePeriod.value = int(self.DecoderKeepAlivePeriodDoubleSpinBox.value())
        self.parentObjHandle.decoderFailSafe.value = self.decoderFailSafeCheckBox.isChecked()
        self.parentObjHandle.trackFailSafe.value = self.trackFailSafeCheckBox.isChecked()
        self.parentObjHandle.decoderMqttURI.value = self.MQTT_URI_LineEdit.displayText()
        self.parentObjHandle.decoderMqttPort.value = int(self.MQTT_Port_LineEdit.displayText())
        self.parentObjHandle.decoderMqttTopicPrefix.value = self.MQTT_TOPIC_PREFIX_LineEdit.displayText()
        # MQTT Southbound API Configuration
        self.parentObjHandle.jmriRpcURI.value = self.RPC_URI_LineEdit.displayText()
        self.parentObjHandle.jmriRpcPortBase.value = int(self.RPC_Port_LineEdit.displayText())
        self.parentObjHandle.JMRIRpcKeepAlivePeriod.value = self.JMRIRpcKeepalivePeriodDoubleSpinBox.value()
        # General myRailIO states
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

    def connectWidgetSignalsSlots(self):
        self.gitCiPushButton.clicked.connect(self.parentObjHandle.gitCi)
        self.gitCoPushButton.clicked.connect(self.parentObjHandle.gitCo)
        self.genConfigPushButton.clicked.connect(self.genConfig)
        self.rsysLogFileBrowsePushButton.clicked.connect(self.rsysLogFileDialog)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def rsysLogFileDialog(self):
        options = QFileDialog.Options()
        #options |= QFileDialog.DontUseNativeDialog
        try:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a base path-/file- name for RSyslog files", self.path,"log Files (*.log);;All Files (*)", options=options)
        except:
            fileName, ext = QFileDialog.getSaveFileName(self,"Select a base path-/file- name for RSyslog files", "","log Files (*.log);;All Files (*)", options=options)
        if fileName:
            try:
                ext = extTxt.split("(")[1].split(")")[0].split(".")[1]
                fileName = fileName + "." + ext
            except:
                pass
            self.rsyslogFileLineEdit.setText(fileName)

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



#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# DECODER DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#################################################################################################################################################
# Class: UI_decoderDialog
# Purpose: UI_decoderDialog provides configuration GUI dialogs for the decoder objects.
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: decoder:                Parent decoder object handle
# -rpcClient: rpc:                          rpcClient handle
# -newConfig: bool                          New configuration flag
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -setEditable() -> bool:                   Set the dialog to editable/configurable
# -unSetEditable() -> bool:                 Set the dialog to uneditable/unconfigurable
# -displayValues() -> None:                 Display the decoder object values in the dialog
# -setValues() -> int:                      Set the decoder object values from the dialog
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -accepted() -> None:                      Slot for the dialog accept button
# -rejected() -> None:                      Slot for the dialog reject button
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_decoderDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(DECODER_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameLineEdit.setEnabled(True)
        else:
            self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.uriLineEdit.setEnabled(True)
        self.macLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.hwVerLineEdit.setEnabled(False)
        self.fwVerLineEdit.setEnabled(False)
        self.ipAddressLineEdit.setEnabled(False)
        self.launchUiPushButton.setEnabled(True)
        self.launchTelnetPushButton.setEnabled(False)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
        self.uriLineEdit.setEnabled(False)
        self.macLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.hwVerLineEdit.setEnabled(False)
        self.fwVerLineEdit.setEnabled(False)
        self.ipAddressLineEdit.setEnabled(False)
        self.launchUiPushButton.setEnabled(True)
        self.launchTelnetPushButton.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.decoderSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.uriLineEdit.setText(self.parentObjHandle.decoderMqttURI.value)
        self.macLineEdit.setText(self.parentObjHandle.mac.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        try:
            self.hwVerLineEdit.setText(self.parentObjHandle.getHardwareVersionFromClient())
        except:
            self.hwVerLineEdit.setText("-")
        try:
            self.fwVerLineEdit.setText(self.parentObjHandle.getFirmwareVersionFromClient())
        except:
            self.fwVerLineEdit.setText("-")
        try:
            self.ipAddressLineEdit.setText(self.parentObjHandle.getIpAddressFromClient())
        except:
            self.ipAddressLineEdit.setText("-")
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        try:
            self.parentObjHandle.decoderSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
            self.parentObjHandle.decoderMqttURI.value = self.uriLineEdit.displayText()
            self.parentObjHandle.mac.value = self.macLineEdit.displayText()
        except AssertionError as configError:
            return configError
        if self.adminStateForceCheckBox.isChecked():
            res = self.parentObjHandle.setAdmStateRecurse(self.adminStateComboBox.currentText())
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
        self.launchUiPushButton.clicked.connect(self.launchUi)
        self.launchTelnetPushButton.clicked.connect(self.launchTelnet)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def launchUi(self):
        try:
            webbrowser.open_new_tab(self.parentObjHandle.getDecoderWwwUiFromClient())
        except:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("Error: Could not launch the decoder UI")
            msg.setInformativeText('Decoder UI not available')
            msg.setWindowTitle("Error")
            msg.exec_()

    def launchTelnet(self):
        msg = QMessageBox()
        msg.setIcon(QMessageBox.Critical)
        msg.setText("Error: Telnet not yet implemented")
        msg.setInformativeText("Telnet not yet implemented")
        msg.setWindowTitle("Error")
        msg.exec_()

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
#################################################################################################################################################
# End Class: UI_decoderDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_decoderInventoryShowDialog
# Purpose: UI_decoderInventoryShowDialog provides decoder inventory GUI dialogs
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: topDecoder:             Top decoder object handle
# -proxymodel: QSortFilterProxyModel        Filter model for the decoder inventory table
# -decoderInventoryTableModel: decoderInventoryTableModel: Decoder inventory table model
# -updateTableWorker: UI_decoderShowDialogUpdateWorker: Table update worker
# -updateTableWorkerThread: QThread         Table update worker thread
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -stopUpdate() -> None:                    Stop the table update worker
# -updateDecoderInventoryTable() -> None:   Update the decoder inventory table
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -showSelectedDecoder() -> None:           Show the selected decoder
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_decoderInventoryShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SHOWDECODERINVENTORY_DIALOG_UI, self)
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.decoderInventoryTableModel = decoderInventoryTableModel(self, self.parentObjHandle)
        self.proxymodel.setSourceModel(self.decoderInventoryTableModel)
        self.decoderInventoryTableView.setModel(self.proxymodel)
        self.decoderInventoryTableView.setSortingEnabled(True)
        self.updateTableWorker = UI_decoderShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateDecoders.connect(self.updateDecoderInventoryTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
             self.updateTableWorker.stop()
             self.updateTableWorkerThread.quit()
             self.updateTableWorkerThread.wait()
        pass

    def updateDecoderInventoryTable(self):
        self.decoderInventoryTableModel.reLoadData()
        self.proxymodel.beginResetModel()
        self.decoderInventoryTableModel.beginResetModel()
        self.decoderInventoryTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self.decoderInventoryTableView.resizeColumnsToContents()
        self.decoderInventoryTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.decoderInventoryTableView.clicked.connect(self.showSelectedDecoder)

    def showSelectedDecoder(self, p_clickedIndex):
        if p_clickedIndex.column() == decoderInventoryTableModel.provUriCol():
            decoder = self.decoderInventoryTableModel.getDecoderObjFromSysId(self.decoderInventoryTableView.model().index(p_clickedIndex.row(), decoderInventoryTableModel.sysNameCol()).data())
            if decoder:
                webbrowser.open_new_tab(decoder.getDecoderWwwUiFromClient())
        elif p_clickedIndex.column() == decoderInventoryTableModel.coreDumpCol():
            decoder = self.decoderInventoryTableModel.getDecoderObjFromSysId(self.decoderInventoryTableView.model().index(p_clickedIndex.row(), decoderInventoryTableModel.sysNameCol()).data())
            if decoder:
                UI_crashDumpDialog(decoder).show()
        else:
            decoder = self.decoderInventoryTableModel.getDecoderObjFromSysId(self.decoderInventoryTableView.model().index(p_clickedIndex.row(), 0).data())
            if decoder:
                self.individualDecoderWidget = UI_decoderDialog(decoder, decoder.rpcClient)
                self.individualDecoderWidget.show()

            else:
                self.parentObjHandle.add(mac = self.decoderInventoryTableView.model().index(p_clickedIndex.row(), decoderInventoryTableModel.macCol()).data(), showResourceTypeSelector = False)

#################################################################################################################################################
# End Class: UI_decoderInventoryShowDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_decoderShowDialogUpdateWorker
# Purpose: A worker class for the decoder inventory table update
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -updateDecoders: QtCore.pyqtSignal:       Signal for decoder inventory table update
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -start() -> None:                         Start the worker
# -stop() -> None:                          Stop the worker
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_decoderShowDialogUpdateWorker(QtCore.QObject):
    updateDecoders = QtCore.pyqtSignal()
    
    def __init__(self, decoderShowHandle = None):
        super(self.__class__, self).__init__(decoderShowHandle)

    @QtCore.pyqtSlot()
    def start(self):
        self.run = True
        cnt = 0
        while self.run:
            if cnt == 10:
                self.updateDecoders.emit()
                cnt = 0
            QtCore.QThread.sleep(1)
            cnt += 1

    def stop(self):
        self.run = False
#################################################################################################################################################
# End Class: UI_decoderShowDialogUpdateWorker
#################################################################################################################################################



#################################################################################################################################################
# Class: decoderInventoryTableModel
# Purpose: decoderInventoryTableModel is a QAbstractTable model for registered decoder inventory table model. It provides the capabilities to
# represent a registered decoders inventory in a QTableView table.
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent>)
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId() -> bool        Used to identify column 0 key information
# -getDecoderObjFromSysId() -> decoder:     Used to get the decoder object from the decoder system ID
# -formatdecoderInventoryTable() -> List[List[str]]: Populate the decoder inventory table
# -rowCount() -> int:                       Returns the current table view row count
# -columnCount() -> int:                    Returns the current table view column count
# -headerData() -> List[str] | None:        Returns the decoder table header columns
# -data() -> str | Any:                     Returns decoder table cell data
# -<*>Col() -> int                          Provides the coresponding column index as provided with formatdecoderInventoryTable()
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_decoderInventoryTableReloadLock : threading.Lock:
#                                           decoder inventory Re-load lock
# -_parent : Any:                           Calling UI object
# -_decoderInventoryTable : List[List[str]]:   Inventory list of lists [row][column]
# -_colCnt                                  Decoder table column count
# -_rowCnt                                  Decoder table row count
#
# Private methods:
# ----------------
# -_reLoadData() -> None                    Reload decoder inventory content
#################################################################################################################################################
class decoderInventoryTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent, parentObjHandle):
        self._decoderInventoryTableReloadLock  = threading.Lock()
        self._parent : Any = p_parent
        self.parentObjHandle = parentObjHandle
        self._decoderInventoryTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        self.reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return True

    def isFirstColumnInstanceId(self) -> bool: 
        return False

    def getDecoderObjFromSysId(self, decoderSysId : str) -> ...:
        for decoderItter in self.parentObjHandle.decoders.value:
            if decoderItter.decoderSystemName.value == decoderSysId:
                return decoderItter
        return None

    def reLoadData(self) -> None:
        with self._decoderInventoryTableReloadLock:
            self._decoderInventoryTable = self.formatDecoderInventoryTable()
            try:
                self._colCnt = len(self._decoderInventoryTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._decoderInventoryTable)
        self._parent.decoderInventoryTableView.resizeColumnsToContents()
        self._parent.decoderInventoryTableView.resizeRowsToContents()

    def formatDecoderInventoryTable(self) -> List[List[str]]:
        self._decoderInventoryTable : List[decoder] = []
        decoderInventoryList : List[decoder] = self.parentObjHandle.decoders.value
        for decoderInventoryItter in decoderInventoryList:
            decoderInventoryRow : List[str] = []
            decoderInventoryRow.append(decoderInventoryItter.decoderSystemName.value)
            decoderInventoryRow.append(decoderInventoryItter.userName.value)
            decoderInventoryRow.append(decoderInventoryItter.description.value)
            decoderInventoryRow.append(decoderInventoryItter.getOpStateDetailStr() + "/" + decoderInventoryItter.getOpStateFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getFirmwareVersionFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getHardwareVersionFromClient())
            decoderInventoryRow.append(decoderInventoryItter.mac.value)
            decoderInventoryRow.append(decoderInventoryItter.getIpAddressFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getBrokerUriFromClient())
            decoderInventoryRow.append(decoderInventoryItter.decoderMqttURI.value)
            decoderInventoryRow.append(decoderInventoryItter.getUptime())
            decoderInventoryRow.append(decoderInventoryItter.getWifiSsidFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getWifiSsidSnrFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getLoglevelFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getMemUsageFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getCpuUsageFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getCoreDumpIdFromClient())
            decoderInventoryRow.append(decoderInventoryItter.getDecoderWwwUiFromClient())
            self._decoderInventoryTable.append(decoderInventoryRow)
        for decoderInventoryItter in self.parentObjHandle.discoveredDecoders:
            decoderInventoryRow : List[str] = []
            if not self.parentObjHandle.discoveredDecoders[decoderInventoryItter]["CONFIGURED"]:
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append(self.parentObjHandle.discoveredDecoders[decoderInventoryItter]["HWVER"])
                decoderInventoryRow.append(self.parentObjHandle.discoveredDecoders[decoderInventoryItter]["FWVER"])
                decoderInventoryRow.append(decoderInventoryItter)
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                decoderInventoryRow.append("-")
                self._decoderInventoryTable.append(decoderInventoryRow)
            
            

        return self._decoderInventoryTable

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self._decoderInventoryTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()):
        with self._decoderInventoryTableReloadLock:
            return self._colCnt

    def headerData(self, section, orientation, role):
        with self._decoderInventoryTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                return ("SysName:","UsrName:", "Desc:", "OpState (Server/Client):", "FWVersion:", "HWVersion:", "MACAddress:", "IPAddress:", "MQTTBroker:", "MQTTClient:", "UpTime[M]:", "WifiSSID:", "WiFiRSSI[dBm]:", "LogLevel:", "MemFree INT/EXT [kB]:", "CPUUsage[%]:", "CoreDump", "DecoderUI:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole)-> str | Any:
        with self._decoderInventoryTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._decoderInventoryTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if not self.getDecoderObjFromSysId(self._decoderInventoryTable[row][self.sysNameCol()]):
                    return QtGui.QBrush(QtGui.QColor('#0000FF'))
                if self.getDecoderObjFromSysId(self._decoderInventoryTable[row][self.sysNameCol()]).getOpStateDetail() == OP_WORKING[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#779638'))
                if self.getDecoderObjFromSysId(self._decoderInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_DISABLED[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                if self.getDecoderObjFromSysId(self._decoderInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & ~OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                if self.getDecoderObjFromSysId(self._decoderInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                return QtGui.QBrush(QtGui.QColor('#0000FF'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignCenter
    @staticmethod
    def sysNameCol() -> int: return 0
    @staticmethod
    def usrNameCol() -> int: return 1
    @staticmethod
    def descCol() -> int: return 2
    @staticmethod
    def opStateCol() -> int: return 3
    @staticmethod
    def fwVer() -> int: return 4
    @staticmethod
    def swVer() -> int: return 5
    @staticmethod
    def macCol() -> int: return 6
    @staticmethod
    def ipCol() -> int: return 7
    @staticmethod
    def mqttBrokerCol() -> int: return 8
    @staticmethod
    def mqttClientCol() -> int: return 9
    @staticmethod
    def upTimeCol() -> int: return 10
    @staticmethod
    def ssidCol() -> int: return 11
    @staticmethod
    def snrCol() -> int: return 12
    @staticmethod
    def logLevelCol() -> int: return 13
    @staticmethod
    def memUsageCol() -> int: return 14
    @staticmethod
    def cpuUsageCol() -> int: return 15
    @staticmethod
    def coreDumpCol() -> int: return 16
    @staticmethod
    def provUriCol() -> int: return 17
#################################################################################################################################################
# End Class: decoderInventoryTableModel
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_crashDumpDialog
# Purpose: UI_crashDumpDialog provides GUI dialogs for the decoder core dump output
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__() -> None:                      Constructor
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -writeCrashDump() -> None:                Write the crash dump to the dialog
# -copyDisplay() -> None:                   Copy the crash dump to the clipboard
# -close_() -> None:                        Close the dialog
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_crashDumpDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(CRASHDUMPOUTPUT_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.writeCrashDump()
        self.closeEvent = self.close_

    def connectWidgetSignalsSlots(self):
        # Close button
        self.crashDumpOutputClosePushButton.clicked.connect(self.close_)
        
        # Copy widget
        self.crashDumpOutputCopyPushButton.clicked.connect(self.copyDisplay)

    def writeCrashDump(self):
        self.crashDumpOutputTextBrowser.append(self.parentObjHandle.getCoreDumpFromClient())

    def copyDisplay(self):
        self.crashDumpOutputTextBrowser.selectAll()
        self.crashDumpOutputTextBrowser.copy()

    def close_(self, unknown):
        self.close()
#################################################################################################################################################
# End Class: UI_crashDumpDialog
#################################################################################################################################################

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# END - DECODER DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



class UI_lightgroupsLinkDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(LIGHTGROUP__LINK_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameLineEdit.setEnabled(True)
        else:
            self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.linkNoSpinBox.setEnabled(True)
        self.mastDefinitionPathLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
        self.linkNoSpinBox.setEnabled(False)
        self.mastDefinitionPathLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.lgLinkSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.linkNoSpinBox.setValue(self.parentObjHandle.lgLinkNo.value)
        self.mastDefinitionPathLineEdit.setText(self.parentObjHandle.mastDefinitionPath.value)
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        try:
            self.parentObjHandle.lgLinkSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
            self.parentObjHandle.lgLinkNo.value = self.linkNoSpinBox.value()
            self.parentObjHandle.mastDefinitionPath.value = self.mastDefinitionPathLineEdit.displayText()
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



class UI_satLinkDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(SATLINK_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameLineEdit.setEnabled(True)
        else:
            self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.linkNoSpinBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.remCrcErrLineEdit.setEnabled(False)
        self.rxSymErrLineEdit.setEnabled(False)
        self.rxSizeErrLineEdit.setEnabled(False)
        self.wdErrLineEdit.setEnabled(False)
        self.clearStatsPushButton.setEnabled(True)
        self.updateStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.linkNoSpinBox.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
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
        self.clearStatsPushButton.setEnabled(True)
        self.updateStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.satLinkSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.linkNoSpinBox.setValue(self.parentObjHandle.satLinkNo.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def setValues(self):
        try:
            self.parentObjHandle.satLinkSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.satLinkNo.value = self.linkNoSpinBox.value()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
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
        self.updateStatsPushButton.clicked.connect(self.displayStats)
        self.clearStatsPushButton.clicked.connect(self.clearStats)

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.rxCrcErr))
        self.remCrcErrLineEdit.setText(str(self.parentObjHandle.remCrcErr))
        self.rxSymErrLineEdit.setText(str(self.parentObjHandle.rxSymErr))
        self.rxSizeErrLineEdit.setText(str(self.parentObjHandle.rxSizeErr))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.wdErr))

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

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# LIGHT GROUP DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#################################################################################################################################################
# Class: UI_lightGroupDialog
# Purpose: UI_lightGroupDialog provides configuration GUI dialogs for the light group objects.
#
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: decoder:                Parent decoder object handle
# -rpcClient: rpc:                          rpcClient handle
# -newConfig: bool                          New configuration flag
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -setEditable() -> bool:                   Set the dialog to editable/configurable
# -unSetEditable() -> bool:                 Set the dialog to uneditable/unconfigurable
# -displayValues() -> None:                 Display the decoder object values in the dialog
# -setValues() -> int:                      Set the decoder object values from the dialog
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -onSysNameChanged                         System name changed during initial configuration
# -lgPropertyHandler                        Property configuration handler
# -onMastTypeChanged                        Mast type changed during initial configuration
# -accepted() -> None:                      Slot for the dialog accept button
# -rejected() -> None:                      Slot for the dialog reject button
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################

class UI_lightGroupDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(LIGHTGROUP_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.property1Geometry = self.lgProp1Box.geometry()
        self.property1Font = self.lgProp1Box.font()
        self.property2Geometry = self.lgProp2Box.geometry()
        self.property2Font = self.lgProp2Box.font()
        self.property3Geometry = self.lgProp3Box.geometry()
        self.property3Font = self.lgProp3Box.font()
        self.lgPropertyHandler()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameComboBox.setEnabled(True)
        else:
            self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.lgLinkAddressSpinBox.setEnabled(True)
        if self.newConfig:
            self.lgTypeComboBox.setEnabled(True)
            self.lgProp1Box.setEnabled(True)
        else:
            self.lgTypeComboBox.setEnabled(False)
            self.lgProp1Box.setEnabled(False)
        self.lgProp2Box.setEnabled(True)
        self.lgProp3Box.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.lgShowingLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
        self.lgLinkAddressSpinBox.setEnabled(False)
        self.lgTypeComboBox.setEnabled(False)
        self.lgProp1Box.setEnabled(False)
        self.lgProp2Box.setEnabled(False)
        self.lgProp3Box.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.lgShowingLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRISystemNameComboBox.setCurrentText(str(self.parentObjHandle.jmriLgSystemName.value))
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.lgLinkAddressSpinBox.setValue(self.parentObjHandle.lgLinkAddr.value)
        self.lgTypeComboBox.setCurrentText(str(self.parentObjHandle.lgType.value))
        self.lgProp1Box.setCurrentText(str(self.parentObjHandle.lgProperty1.value))
        self.lgProp2Box.setCurrentText(str(self.parentObjHandle.lgProperty2.value))
        self.lgProp3Box.setCurrentText(str(self.parentObjHandle.lgProperty3.value))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.lgShowingLineEdit.setText(str(self.parentObjHandle.lgShowing))

    def setValues(self):
        try:
            self.parentObjHandle.jmriLgSystemName.value = self.JMRISystemNameComboBox.currentText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
            self.parentObjHandle.lgLinkAddr.value = self.lgLinkAddressSpinBox.value()
            self.parentObjHandle.lgType.value = self.lgTypeComboBox.currentText()
            self.parentObjHandle.lgProperty1.value = self.lgProp1Box.currentText()
            self.parentObjHandle.lgProperty2.value = self.lgProp2Box.currentText()
            self.parentObjHandle.lgProperty3.value = self.lgProp3Box.currentText()
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
        self.JMRISystemNameComboBox.currentTextChanged.connect(self.onSysNameChanged)
        self.lgTypeComboBox.currentTextChanged.connect(self.lgPropertyHandler)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def onSysNameChanged(self):
        try:
            self.JMRIUserNameLineEdit.setText(self.rpcClient.getUserNameBySysName(jmriObj.MASTS, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIUserNameLineEdit.setText("")
            if self.newConfig:
                self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        try:
            self.JMRIDescriptionLineEdit.setText(self.rpcClient.getCommentBySysName(jmriObj.MASTS, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIDescriptionLineEdit.setText("")
            if self.newConfig:
                self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
            
    def lgPropertyHandler(self):
        try:
            self.lgProp1Box.currentTextChanged.disconnect();
        except:
            pass
        self.lgProp1Box.deleteLater()
        self.lgProp2Box.deleteLater()
        self.lgProp3Box.deleteLater()
        if self.lgTypeComboBox.currentText() == "SIGNAL MAST":
            self.lgProperty1Label.setText("Mast type:")
            self.lgProperty2Label.setText("Diming time:")
            self.lgProperty3Label.setText("Flash freq.:")
            self.lgProp1Box = QComboBox(self)
            self.lgProp1Box.addItems(self.parentObjHandle.parent.getMastTypes())
            self.lgProp2Box = QComboBox(self)
            self.lgProp2Box.addItems(["NORMAL", "FAST", "SLOW"])
            self.lgProp3Box = QComboBox(self)
            self.lgProp3Box.addItems(["NORMAL", "FAST", "SLOW"])
            self.lgProp1Box.setGeometry(self.property1Geometry)
            self.lgProp2Box.setGeometry(self.property2Geometry)
            self.lgProp3Box.setGeometry(self.property3Geometry)
            self.lgProp1Box.setFont(self.property1Font)
            self.lgProp2Box.setFont(self.property2Font)
            self.lgProp3Box.setFont(self.property3Font)
            self.lgProp1Box.show()
            self.lgProp2Box.show()
            self.lgProp3Box.show()
            self.lgProp1Box.currentTextChanged.connect(self.onMastTypeChanged)
            self.onMastTypeChanged()
        else:
            self.lgProperty1Label.setText("Property 1:")
            self.lgProperty2Label.setText("Property 2:")
            self.lgProperty3Label.setText("Property 3:")
            self.lgProp1Box = QLineEdit(self)
            self.lgProp2Box = QLineEdit(self)
            self.lgProp3Box = QLineEdit(self)
            self.lgProp1Box.setGeometry(self.property1Geometry)
            self.lgProp2Box.setGeometry(self.property2Geometry)
            self.lgProp3Box.setGeometry(self.property3Geometry)
            self.lgProp1Box.setFont(self.property1Font)
            self.lgProp2Box.setFont(self.property2Font)
            self.lgProp3Box.setFont(self.property3Font)
            self.lgProp1Box.show()
            self.lgProp2Box.show()
            self.lgProp3Box.show()
        
    def onMastTypeChanged(self):
        self.JMRISystemNameComboBox.clear()
        for mastItter in self.rpcClient.getObjects(jmriObj.MASTS):
            if mastItter.startswith("IF$vsm:" + self.lgProp1Box.currentText()):
                self.JMRISystemNameComboBox.addItem(mastItter)
        if self.newConfig:
            self.JMRISystemNameComboBox.setCurrentText("IF$vsm:" + self.lgProp1Box.currentText() + "($0001)")
        self.onSysNameChanged()

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
#################################################################################################################################################
# End Class: UI_lightGroupDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_lightGroupInventoryShowDialog
# Purpose: UI_lightGroupInventoryShowDialog provides Light group inventory GUI dialogs
#
# Public Methods and objects to be used by the lightGroup producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: lglink   :              Parent lgLink object handle
# -proxymodel: QSortFilterProxyModel        Filter model for the Light group inventory table
# -lightGroupInventoryTableModel: lightGroupInventoryTableModel: Light group inventory table model
# -updateTableWorker: UI_lightGroupShowDialogUpdateWorker: Table update worker
# -updateTableWorkerThread: QThread         Table update worker thread
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -stopUpdate() -> None:                    Stop the table update worker
# -updatelightGroupInventoryTable() -> None:Update the Light group inventory table
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -showSelectedLighGroup() -> None:           Show the selected light group in the dialog
# 
# Private Methods and objects only to be used internally or by the lightGroupHandler server:
# ==========================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_lightGroupInventoryShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LIGHTGROUP_INVENTORY_DIALOG_UI, self)
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.lightGroupInventoryTableModel = lightGroupInventoryTableModel(self, self.parentObjHandle)
        self.proxymodel.setSourceModel(self.lightGroupInventoryTableModel)
        self.lightGroupInventoryTableView.setModel(self.proxymodel)
        self.lightGroupInventoryTableView.setSortingEnabled(True)
        self.updateTableWorker = UI_lightGroupShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateLightGroups.connect(self.updatelightGroupInventoryTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
             self.updateTableWorker.stop()
             self.updateTableWorkerThread.quit()
             self.updateTableWorkerThread.wait()
        pass

    def updatelightGroupInventoryTable(self):
        self.lightGroupInventoryTableModel.reLoadData()
        self.proxymodel.beginResetModel()
        self.lightGroupInventoryTableModel.beginResetModel()
        self.lightGroupInventoryTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self.lightGroupInventoryTableView.resizeColumnsToContents()
        self.lightGroupInventoryTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.lightGroupInventoryTableView.clicked.connect(self.showSelectedLightGroup)

    def showSelectedLightGroup(self, p_clickedIndex):
        lightGroup = self.lightGroupInventoryTableModel.getLightGroupObjFromSysId(self.lightGroupInventoryTableView.model().index(p_clickedIndex.row(), 0).data())
        self.individualLightGroupWidget = UI_lightGroupDialog(lightGroup, lightGroup.rpcClient)
        self.individualLightGroupWidget.show()
#################################################################################################################################################
# End Class: UI_lightGroupInventoryShowDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_lightGroupShowDialogUpdateWorker
# Purpose: A worker class for the lightGroup inventory table update
#
# Public Methods and objects to be used by the lightGroup producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -updatelightGroups: QtCore.pyqtSignal:    Signal for lightGroup inventory table update
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -start() -> None:                         Start the worker
# -stop() -> None:                          Stop the worker
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# ======================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_lightGroupShowDialogUpdateWorker(QtCore.QObject):
    updateLightGroups = QtCore.pyqtSignal()
    
    def __init__(self, lightGroupShowHandle = None):
        super(self.__class__, self).__init__(lightGroupShowHandle)

    @QtCore.pyqtSlot()
    def start(self):
        self.run = True
        while self.run:
            self.updateLightGroups.emit()
            QtCore.QThread.sleep(5)

    def stop(self):
        self.run = False
#################################################################################################################################################
# End Class: UI_lightGroupShowDialogUpdateWorker
#################################################################################################################################################



#################################################################################################################################################
# Class: lightGroupInventoryTableModel
# Purpose: lightGroupInventoryTableModel is a QAbstractTable model for registered lightGroup inventory table model. It provides the capabilities to
# represent a registered lightGroup inventory in a QTableView table.
#
# Public Methods and objects to be used by the lightGroup producers:
# =================================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent>)
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId() -> bool        Used to identify column 0 key information
# -getLightGroupFromSysId() -> lightGroup:  Used to get the lightGroup object from the lightGroup system ID
# -formatLightGroupInventoryTable() -> List[List[str]]: Populate the lightGroup inventory table
# -rowCount() -> int:                       Returns the current table view row count
# -columnCount() -> int:                    Returns the current table view column count
# -headerData() -> List[str] | None:        Returns the lightGroup table header columns
# -data() -> str | Any:                     Returns lightGroup table cell data
# -<*>Col() -> int                          Provides the coresponding column index as provided with formatLightGroupInventoryTable()
# 
# Private Methods and objects only to be used internally or by the lightGroupHandler server:
# ==========================================================================================
# Private data-structures:
# ------------------------
# -_lightGroupInventoryTableReloadLock : threading.Lock:
#                                           lightGroup inventory Re-load lock
# -_parent : Any:                           Calling UI object
# -_lightGroupInventoryTable : List[List[str]]:   Inventory list of lists [row][column]
# -_colCnt                                  lightGroup table column count
# -_rowCnt                                  lightGroup table row count
#
# Private methods:
# ----------------
# -_reLoadData() -> None                    Reload lightGroup inventory content
#################################################################################################################################################
class lightGroupInventoryTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent, parentObjHandle):
        self._lightGroupInventoryTableReloadLock  = threading.Lock()
        self._parent : Any = p_parent
        self.parentObjHandle = parentObjHandle
        self._lightGroupInventoryTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        self.reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return True

    def isFirstColumnInstanceId(self) -> bool: 
        return False

    def getLightGroupObjFromSysId(self, lightGroupSysId : str) -> ...:
        lightGroupInventoryList : List[lightGroup] = self.getAllLightGroups()
        for lightGroupItter in lightGroupInventoryList:
            if lightGroupItter.jmriLgSystemName.value == lightGroupSysId:
                return lightGroupItter
        return None

    def reLoadData(self) -> None:
        with self._lightGroupInventoryTableReloadLock:
            self._lightGroupInventoryTable = self.formatLightGroupInventoryTable()
            try:
                self._colCnt = len(self._lightGroupInventoryTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._lightGroupInventoryTable)
        self._parent.lightGroupInventoryTableView.resizeColumnsToContents()
        self._parent.lightGroupInventoryTableView.resizeRowsToContents()

    def formatLightGroupInventoryTable(self) -> List[List[str]]:
        self._lightGroupInventoryTable : List[lightGroup] = []
        lightGroupInventoryList : List[lightGroup] = self.getAllLightGroups()
        for lightGroupInventoryItter in lightGroupInventoryList:
            lightGroupInventoryRow : List[str] = []
            lightGroupInventoryRow.append(lightGroupInventoryItter.jmriLgSystemName.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.userName.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.description.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.lgType.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.lgProperty1.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.lgLinkAddr.value)
            lightGroupInventoryRow.append(lightGroupInventoryItter.getOpStateDetailStr())
            lightGroupInventoryRow.append(lightGroupInventoryItter.getShowing())
            lightGroupInventoryRow.append(lightGroupInventoryItter.getUptime())
            lightGroupInventoryRow.append(lightGroupInventoryItter.getTopology())
            self._lightGroupInventoryTable.append(lightGroupInventoryRow)
        return self._lightGroupInventoryTable

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self._lightGroupInventoryTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()):
        with self._lightGroupInventoryTableReloadLock:
            return self._colCnt

    def headerData(self, section, orientation, role):
        with self._lightGroupInventoryTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                return ("SysName:","UsrName:", "Desc:", "Type:", "Subtype:", "LinkAddr:", "OpState:", "Showing:", "UpTime[M]:", "Topology:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole)-> str | Any:
        with self._lightGroupInventoryTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._lightGroupInventoryTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if self.getLightGroupObjFromSysId(self._lightGroupInventoryTable[row][self.sysNameCol()]).getOpStateDetail() == OP_WORKING[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#779638'))
                if self.getLightGroupObjFromSysId(self._lightGroupInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_DISABLED[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                if self.getLightGroupObjFromSysId(self._lightGroupInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & ~OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                if self.getLightGroupObjFromSysId(self._lightGroupInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                return QtGui.QBrush(QtGui.QColor('#0000FF'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignCenter
            
    def getAllLightGroups(self):
        lightGroups : List[lightGroup] = []
        for decoderItter in self.parentObjHandle.decoders.value:
            for lightGroupLinkItter in decoderItter.lgLinks.value:
                lightGroups.extend(lightGroupLinkItter.lightGroups.value)
        return lightGroups
        
    @staticmethod
    def sysNameCol() -> int: return 0
    @staticmethod
    def usrNameCol() -> int: return 1
    @staticmethod
    def descCol() -> int: return 2
    @staticmethod
    def typeCol() -> int: return 3
    @staticmethod
    def subTypeCol() -> int: return 4
    @staticmethod
    def linkAddrCol() -> int: return 5
    @staticmethod
    def opStateCol() -> int: return 6
    @staticmethod
    def showing() -> int: return 7
    @staticmethod
    def upTimeCol() -> int: return 8
    @staticmethod
    def topologyCol() -> int: return 9
#################################################################################################################################################
# End Class: lightGroupInventoryTableModel
#################################################################################################################################################

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# END - LIGHT GROUP DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%



class UI_satelliteDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(SATELLITE_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameLineEdit.setEnabled(True)
        else:
            self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.satAddrSpinBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.txCrcErrLineEdit.setEnabled(False)
        self.wdErrLineEdit.setEnabled(False)
        self.updateStatsPushButton.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
        self.satAddrSpinBox.setEnabled(False)
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
        self.JMRISystemNameLineEdit.setText(self.parentObjHandle.satSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.satAddrSpinBox.setValue(self.parentObjHandle.satLinkAddr.value)
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def setValues(self):
        try:
            self.parentObjHandle.satSystemName.value = self.JMRISystemNameLineEdit.displayText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
            self.parentObjHandle.satLinkAddr.value = self.satAddrSpinBox.value()
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
        self.updateStatsPushButton.clicked.connect(self.displayStats)
        self.clearStatsPushButton.clicked.connect(self.clearStats)

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.rxCrcErr))
        self.txCrcErrLineEdit.setText(str(self.parentObjHandle.txCrcErr))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.wdErr))

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



#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# SENSOR DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#################################################################################################################################################
# Class: UI_sensorDialog
# Purpose: UI_sensorDialog provides configuration GUI dialogs for the sensor objects.
#
# Public Methods and objects to be used by the sensor consumers:
# ==============================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: satellite                Parent satellite object handle
# -rpcClient: rpc:                          rpcClient handle
# -newConfig: bool                          New configuration flag
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -setEditable() -> bool:                   Set the dialog to editable/configurable
# -unSetEditable() -> bool:                 Set the dialog to uneditable/unconfigurable
# -displayValues() -> None:                 Display the sensor object values in the dialog
# -setValues() -> int:                      Set the sensor object values from the dialog
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -onSysNameChanged                         System name changed during initial configuration
# -accepted() -> None:                      Slot for the dialog accept button
# -rejected() -> None:                      Slot for the dialog reject button
# 
# Private Methods and objects only to be used internally or by the sensorHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################

class UI_sensorDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(SENSOR_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.JMRISystemNameComboBox.addItems(self.rpcClient.getObjects(jmriObj.SENSORS))
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameComboBox.setEnabled(True)
        else:
            self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.sensPortSpinBox.setEnabled(True)
        self.sensTypeComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.sensStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
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
        self.JMRISystemNameComboBox.setCurrentText(self.parentObjHandle.jmriSensSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
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
            self.parentObjHandle.jmriSensSystemName.value = self.JMRISystemNameComboBox.currentText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
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
        self.JMRISystemNameComboBox.currentTextChanged.connect(self.onSysNameChanged)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def onSysNameChanged(self):
        try:
            self.JMRIUserNameLineEdit.setText(self.rpcClient.getUserNameBySysName(jmriObj.SENSORS, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIUserNameLineEdit.setText("")
        try:
            self.JMRIDescriptionLineEdit.setText(self.rpcClient.getCommentBySysName(jmriObj.SENSORS, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIDescriptionLineEdit.setText("")

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

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()

    def closeEvent(self, event):
        self.parentObjHandle.rejected()
        self.close()
#################################################################################################################################################
# End Class: UI_sensorDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_sensorInventoryShowDialog
# Purpose: UI_sensorInventoryShowDialog provides Light group inventory GUI dialogs
#
# Public Methods and objects to be used by the sensor producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: lglink   :              Parent lgLink object handle
# -proxymodel: QSortFilterProxyModel        Filter model for the Light group inventory table
# -sensorInventoryTableModel: sensorInventoryTableModel: Light group inventory table model
# -updateTableWorker: UI_sensorShowDialogUpdateWorker: Table update worker
# -updateTableWorkerThread: QThread         Table update worker thread
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -stopUpdate() -> None:                    Stop the table update worker
# -updateSensorInventoryTable() -> None:Update the Light group inventory table
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -showSelectedLighGroup() -> None:           Show the selected light group in the dialog
# 
# Private Methods and objects only to be used internally or by the sensorHandler server:
# ==========================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_sensorInventoryShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(SENSOR_INVENTORY_DIALOG_UI, self)
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.sensorInventoryTableModel = sensorInventoryTableModel(self, self.parentObjHandle)
        self.proxymodel.setSourceModel(self.sensorInventoryTableModel)
        self.sensorInventoryTableView.setModel(self.proxymodel)
        self.sensorInventoryTableView.setSortingEnabled(True)
        self.updateTableWorker = UI_sensorShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateSensors.connect(self.updateSensorInventoryTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
            self.updateTableWorker.stop()
            self.updateTableWorkerThread.quit()
            self.updateTableWorkerThread.wait()
        pass

    def updateSensorInventoryTable(self):
        self.sensorInventoryTableModel.reLoadData()
        self.proxymodel.beginResetModel()
        self.sensorInventoryTableModel.beginResetModel()
        self.sensorInventoryTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self.sensorInventoryTableView.resizeColumnsToContents()
        self.sensorInventoryTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.sensorInventoryTableView.clicked.connect(self.showSelectedSensor)

    def showSelectedSensor(self, p_clickedIndex):
        sensor = self.sensorInventoryTableModel.getSensorObjFromSysId(self.sensorInventoryTableView.model().index(p_clickedIndex.row(), 0).data())
        self.individualSensorWidget = UI_sensorDialog(sensor, sensor.rpcClient)
        self.individualSensorWidget.show()
#################################################################################################################################################
# End Class: UI_sensorInventoryShowDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_sensorShowDialogUpdateWorker
# Purpose: A worker class for the sensor inventory table update
#
# Public Methods and objects to be used by the sensor producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -updateSensors: QtCore.pyqtSignal:    Signal for sensor inventory table update
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -start() -> None:                         Start the worker
# -stop() -> None:                          Stop the worker
# 
# Private Methods and objects only to be used internally or by the sensorHandler server:
# ======================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_sensorShowDialogUpdateWorker(QtCore.QObject):
    updateSensors = QtCore.pyqtSignal()
    
    def __init__(self, sensorShowHandle = None):
        super(self.__class__, self).__init__(sensorShowHandle)

    @QtCore.pyqtSlot()
    def start(self):
        self.run = True
        while self.run:
            self.updateSensors.emit()
            QtCore.QThread.sleep(5)

    def stop(self):
        self.run = False
#################################################################################################################################################
# End Class: UI_sensorShowDialogUpdateWorker
#################################################################################################################################################



#################################################################################################################################################
# Class: sensorInventoryTableModel
# Purpose: sensorInventoryTableModel is a QAbstractTable model for registered sensor inventory table model. It provides the capabilities to
# represent a registered sensor inventory in a QTableView table.
#
# Public Methods and objects to be used by the sensor producers:
# =================================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent>)
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId() -> bool        Used to identify column 0 key information
# -getSensorFromSysId() -> sensor:  Used to get the sensor object from the sensor system ID
# -formatSensorInventoryTable() -> List[List[str]]: Populate the sensor inventory table
# -rowCount() -> int:                       Returns the current table view row count
# -columnCount() -> int:                    Returns the current table view column count
# -headerData() -> List[str] | None:        Returns the sensor table header columns
# -data() -> str | Any:                     Returns sensor table cell data
# -<*>Col() -> int                          Provides the coresponding column index as provided with formatSensorInventoryTable()
# 
# Private Methods and objects only to be used internally or by the sensorHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_sensorInventoryTableReloadLock : threading.Lock:
#                                           sensor inventory Re-load lock
# -_parent : Any:                           Calling UI object
# -_sensorInventoryTable : List[List[str]]: Inventory list of lists [row][column]
# -_colCnt                                  sensor table column count
# -_rowCnt                                  sensor table row count
#
# Private methods:
# ----------------
# -_reLoadData() -> None                    Reload sensor inventory content
#################################################################################################################################################
class sensorInventoryTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent, parentObjHandle):
        self._sensorInventoryTableReloadLock  = threading.Lock()
        self._parent : Any = p_parent
        self.parentObjHandle = parentObjHandle
        self._sensorInventoryTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        self.reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return True

    def isFirstColumnInstanceId(self) -> bool: 
        return False

    def getSensorObjFromSysId(self, sensorSysId : str) -> ...:
        sensorInventoryList : List[sensor] = self.getAllSensors()
        for sensorItter in sensorInventoryList:
            if sensorItter.jmriSensSystemName.value == sensorSysId:
                return sensorItter
        return None

    def reLoadData(self) -> None:
        with self._sensorInventoryTableReloadLock:
            self._sensorInventoryTable = self.formatSensorInventoryTable()
            try:
                self._colCnt = len(self._sensorInventoryTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._sensorInventoryTable)
        self._parent.sensorInventoryTableView.resizeColumnsToContents()
        self._parent.sensorInventoryTableView.resizeRowsToContents()

    def formatSensorInventoryTable(self) -> List[List[str]]:
        self._sensorInventoryTable : List[sensor] = []
        sensorInventoryList : List[sensor] = self.getAllSensors()
        for sensorInventoryItter in sensorInventoryList:
            sensorInventoryRow : List[str] = []
            sensorInventoryRow.append(sensorInventoryItter.jmriSensSystemName.value)
            sensorInventoryRow.append(sensorInventoryItter.userName.value)
            sensorInventoryRow.append(sensorInventoryItter.description.value)
            sensorInventoryRow.append(sensorInventoryItter.sensType.value)
            sensorInventoryRow.append(sensorInventoryItter.sensPort.value)
            sensorInventoryRow.append(sensorInventoryItter.getOpStateDetailStr())
            sensorInventoryRow.append(sensorInventoryItter.sensState)
            sensorInventoryRow.append(sensorInventoryItter.getUptime())
            sensorInventoryRow.append(sensorInventoryItter.getTopology())
            self._sensorInventoryTable.append(sensorInventoryRow)
        return self._sensorInventoryTable

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self._sensorInventoryTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()):
        with self._sensorInventoryTableReloadLock:
            return self._colCnt

    def headerData(self, section, orientation, role):
        with self._sensorInventoryTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                return ("SysName:","UsrName:", "Desc:", "Type:", "Port:", "OpState:", "Sensing:", "UpTime[s]:", "Topology:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole)-> str | Any:
        with self._sensorInventoryTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._sensorInventoryTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if self.getSensorObjFromSysId(self._sensorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() == OP_WORKING[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#779638'))
                if self.getSensorObjFromSysId(self._sensorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_DISABLED[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                if self.getSensorObjFromSysId(self._sensorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & ~OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                if self.getSensorObjFromSysId(self._sensorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                return QtGui.QBrush(QtGui.QColor('#0000FF'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignCenter
            
    def getAllSensors(self):
        sensors : List[sensor] = []
        for decoderItter in self.parentObjHandle.decoders.value:
            for satLinkLinkItter in decoderItter.satLinks.value:
                for satItter in satLinkLinkItter.satellites.value:
                    sensors.extend(satItter.sensors.value)
        return sensors
        
    @staticmethod
    def sysNameCol() -> int: return 0
    @staticmethod
    def usrNameCol() -> int: return 1
    @staticmethod
    def descCol() -> int: return 2
    @staticmethod
    def typeCol() -> int: return 3
    @staticmethod
    def portCol() -> int: return 4
    @staticmethod
    def opStateCol() -> int: return 5
    @staticmethod
    def sensingCol() -> int: return 6
    @staticmethod
    def upTimeCol() -> int: return 7
    @staticmethod
    def topologyCol() -> int: return 8
#################################################################################################################################################
# End Class: sensorInventoryTableModel
#################################################################################################################################################

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# END - SENSOR DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%













#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# ACTUATOR DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#################################################################################################################################################
# Class: UI_actuatorDialog
# Purpose: UI_actuatorDialog provides configuration GUI dialogs for the actuator objects.
#
# Public Methods and objects to be used by the actuator consumers:
# ===============================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: satellite                Parent satellite object handle
# -rpcClient: rpc:                          rpcClient handle
# -newConfig: bool                          New configuration flag
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -setEditable() -> bool:                   Set the dialog to editable/configurable
# -unSetEditable() -> bool:                 Set the dialog to uneditable/unconfigurable
# -displayValues() -> None:                 Display the actuator object values in the dialog
# -setValues() -> int:                      Set the actuator object values from the dialog
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -onSysNameChanged                         System name changed during initial configuration
# -onTypeChanged                            Actuator type changed during initial configuration
# -accepted() -> None:                      Slot for the dialog accept button
# -rejected() -> None:                      Slot for the dialog reject button
# 
# Private Methods and objects only to be used internally or by the actuatorHandler server:
# ========================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################

class UI_actuatorDialog(QDialog):
    def __init__(self, parentObjHandle, rpcClient, edit = False, parent = None, newConfig = False):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        self.rpcClient = rpcClient
        self.newConfig = newConfig
        loadUi(ACTUATOR_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        if self.actTypeComboBox.currentText() == "TURNOUT":
            type = jmriObj.TURNOUTS
        elif(self.actTypeComboBox.currentText() == "LIGHT"):
            type = jmriObj.LIGHTS
        elif(self.actTypeComboBox.currentText() == "MEMORY"):
            type = jmriObj.MEMORIES
        self.JMRISystemNameComboBox.addItems(self.rpcClient.getObjects(type))
        self.onTypeChanged()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        if self.newConfig:
            self.JMRISystemNameComboBox.setEnabled(True)
        else:
            self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.JMRIDescriptionLineEdit.setEnabled(True)
        self.actPortSpinBox.setEnabled(True)
        if self.newConfig:
            self.actTypeComboBox.setEnabled(True)
        else:
            self.actTypeComboBox.setEnabled(False)
        self.actSubTypeComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.actStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRISystemNameComboBox.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.JMRIDescriptionLineEdit.setEnabled(False)
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
        self.JMRISystemNameComboBox.setCurrentText(self.parentObjHandle.jmriActSystemName.value)
        self.JMRIUserNameLineEdit.setText(self.parentObjHandle.userName.value)
        self.JMRIDescriptionLineEdit.setText(self.parentObjHandle.description.value)
        self.actTypeComboBox.setCurrentText(self.parentObjHandle.actType.value)
        self.actSubTypeComboBox.setCurrentText(self.parentObjHandle.actSubType.value)
        self.actPortSpinBox.setValue(self.parentObjHandle.actPort.value)
        self.opStateSummaryLineEdit.setText(self.parentObjHandle.getOpStateSummary()[STATE_STR])
        self.opStateDetailLineEdit.setText(self.parentObjHandle.getOpStateDetailStr())
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(self.parentObjHandle.getAdmState()[STATE_STR])
        self.adminStateForceCheckBox.setChecked(False)
        self.actStateLineEdit.setText(self.parentObjHandle.actState)

    def setValues(self):
        try:
            self.parentObjHandle.jmriActSystemName.value = self.JMRISystemNameComboBox.currentText()
            self.parentObjHandle.userName.value = self.JMRIUserNameLineEdit.displayText()
            self.parentObjHandle.description.value = self.JMRIDescriptionLineEdit.displayText()
            self.parentObjHandle.actType.value = self.actTypeComboBox.currentText()
            self.parentObjHandle.actSubType.value = self.actSubTypeComboBox.currentText()
            self.parentObjHandle.actPort.value = self.actPortSpinBox.value()
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
        self.JMRISystemNameComboBox.currentTextChanged.connect(self.onSysNameChanged)
        self.actTypeComboBox.currentTextChanged.connect(self.onTypeChanged)
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def onSysNameChanged(self):
        if self.actTypeComboBox.currentText() == "TURNOUT":
            type = jmriObj.TURNOUTS
        elif(self.actTypeComboBox.currentText() == "LIGHT"):
            type = jmriObj.LIGHTS
        elif(self.actTypeComboBox.currentText() == "MEMORY"):
            type = jmriObj.MEMORIES
        else:
            type = None
        try:
            self.JMRIUserNameLineEdit.setText(self.rpcClient.getUserNameBySysName(type, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIUserNameLineEdit.setText("")
        try:
            self.JMRIDescriptionLineEdit.setText(self.rpcClient.getCommentBySysName(type, self.JMRISystemNameComboBox.currentText()))
        except:
            self.JMRIDescriptionLineEdit.setText("")

    def onTypeChanged(self):
        self.actSubTypeComboBox.clear()
        self.JMRISystemNameComboBox.clear()
        if self.actTypeComboBox.currentText() == "TURNOUT":
            self.JMRISystemNameComboBox.addItems(self.rpcClient.getObjects(jmriObj.TURNOUTS))
            self.JMRISystemNameComboBox.setCurrentText("MT-MyNewTurnoutSysName")
            self.JMRIUserNameLineEdit.setText("MyNewTurnoutUserName")
            self.JMRIDescriptionLineEdit.setText("MyNewTurnoutDescription")
            self.actSubTypeComboBox.addItems(["SOLENOID", "SERVO"])
        elif self.actTypeComboBox.currentText() == "LIGHT":
            self.JMRISystemNameComboBox.addItems(self.rpcClient.getObjects(jmriObj.LIGHTS))
            self.JMRISystemNameComboBox.setCurrentText("ML-MyNewLightSysName")
            self.JMRIUserNameLineEdit.setText("MyNewLightUserName")
            self.JMRIDescriptionLineEdit.setText("MyNewLightDescription")
            self.actSubTypeComboBox.addItems(["ONOFF"])
        elif self.actTypeComboBox.currentText() == "MEMORY":
            self.JMRISystemNameComboBox.addItems(self.rpcClient.getObjects(jmriObj.MEMORIES))
            self.JMRISystemNameComboBox.setCurrentText("IM-MyNewMemorySysName")
            self.JMRIUserNameLineEdit.setText("MyNewMemoryUserName")
            self.JMRIDescriptionLineEdit.setText("MyNewMemoryDescription")
            self.actSubTypeComboBox.addItems(["SOLENOID", "SERVO", "PWM100", "PWM1_25K", "ONOFF", "PULSE"])

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

    def rejected(self):
        self.parentObjHandle.rejected()
        self.close()
        
    def closeEvent(self, event):
        self.parentObjHandle.rejected()
        self.close()
#################################################################################################################################################
# End Class: UI_actuatorDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_actuatorInventoryShowDialog
# Purpose: UI_actuatorInventoryShowDialog provides Light group inventory GUI dialogs
#
# Public Methods and objects to be used by the actuator producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -parentObjHandle: lglink   :              Parent lgLink object handle
# -proxymodel: QSortFilterProxyModel        Filter model for the Light group inventory table
# -actuatorInventoryTableModel: actuatorInventoryTableModel: Light group inventory table model
# -updateTableWorker: UI_actuatorShowDialogUpdateWorker: Table update worker
# -updateTableWorkerThread: QThread         Table update worker thread
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -stopUpdate() -> None:                    Stop the table update worker
# -updateActuatorInventoryTable() -> None:Update the Light group inventory table
# -connectWidgetSignalsSlots() -> None:     Connect the dialog widget signals to slots
# -showSelectedLighGroup() -> None:           Show the selected light group in the dialog
# 
# Private Methods and objects only to be used internally or by the actuatorHandler server:
# ==========================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_actuatorInventoryShowDialog(QDialog):
    def __init__(self, parentObjHandle, parent = None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(ACTUATOR_INVENTORY_DIALOG_UI, self)
        self.proxymodel = QtCore.QSortFilterProxyModel()
        self.actuatorInventoryTableModel = actuatorInventoryTableModel(self, self.parentObjHandle)
        self.proxymodel.setSourceModel(self.actuatorInventoryTableModel)
        self.actuatorInventoryTableView.setModel(self.proxymodel)
        self.actuatorInventoryTableView.setSortingEnabled(True)
        self.updateTableWorker = UI_actuatorShowDialogUpdateWorker(self)
        self.updateTableWorker.setParent(None)
        self.updateTableWorkerThread = QtCore.QThread()
        self.updateTableWorker.moveToThread(self.updateTableWorkerThread)
        self.updateTableWorkerThread.start()
        self.updateTableWorkerThread.started.connect(self.updateTableWorker.start)
        self.updateTableWorker.updateActuators.connect(self.updateActuatorInventoryTable)
        self.connectWidgetSignalsSlots()
        self.closeEvent = self.stopUpdate

    def stopUpdate(self, event):
        if self.updateTableWorkerThread.isRunning():
             self.updateTableWorker.stop()
             self.updateTableWorkerThread.quit()
             self.updateTableWorkerThread.wait()
        pass

    def updateActuatorInventoryTable(self):
        self.actuatorInventoryTableModel.reLoadData()
        self.proxymodel.beginResetModel()
        self.actuatorInventoryTableModel.beginResetModel()
        self.actuatorInventoryTableModel.endResetModel()
        self.proxymodel.endResetModel()
        self.actuatorInventoryTableView.resizeColumnsToContents()
        self.actuatorInventoryTableView.resizeRowsToContents()

    def connectWidgetSignalsSlots(self):
        self.actuatorInventoryTableView.clicked.connect(self.showSelectedActuator)

    def showSelectedActuator(self, p_clickedIndex):
        actuator = self.actuatorInventoryTableModel.getActuatorObjFromSysId(self.actuatorInventoryTableView.model().index(p_clickedIndex.row(), 0).data())
        self.individualActuatorWidget = UI_actuatorDialog(actuator, actuator.rpcClient)
        self.individualActuatorWidget.show()
#################################################################################################################################################
# End Class: UI_actuatorInventoryShowDialog
#################################################################################################################################################



#################################################################################################################################################
# Class: UI_actuatorShowDialogUpdateWorker
# Purpose: A worker class for the actuator inventory table update
#
# Public Methods and objects to be used by the actuator producers:
# ==================================================================
# Public data-structures:
# -----------------------
# -updateActuators: QtCore.pyqtSignal:    Signal for actuator inventory table update
#
# Public methods:
# ---------------
# -__init__(<parent>) -> None:              Constructor
# -start() -> None:                         Start the worker
# -stop() -> None:                          Stop the worker
# 
# Private Methods and objects only to be used internally or by the actuatorHandler server:
# ======================================================================================
# Private data-structures:
# ------------------------
# -
#
# Private methods:
# ----------------
# -
#################################################################################################################################################
class UI_actuatorShowDialogUpdateWorker(QtCore.QObject):
    updateActuators = QtCore.pyqtSignal()
    
    def __init__(self, actuatorShowHandle = None):
        super(self.__class__, self).__init__(actuatorShowHandle)

    @QtCore.pyqtSlot()
    def start(self):
        self.run = True
        while self.run:
            self.updateActuators.emit()
            QtCore.QThread.sleep(5)

    def stop(self):
        self.run = False
#################################################################################################################################################
# End Class: UI_actuatorShowDialogUpdateWorker
#################################################################################################################################################



#################################################################################################################################################
# Class: actuatorInventoryTableModel
# Purpose: actuatorInventoryTableModel is a QAbstractTable model for registered actuator inventory table model. It provides the capabilities to
# represent a registered actuator inventory in a QTableView table.
#
# Public Methods and objects to be used by the actuator producers:
# =================================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__(<parent>)
# -isFirstColumnObjectId() -> bool:         Used to identify column 0 key information
# -isFirstColumnInstanceId() -> bool        Used to identify column 0 key information
# -getActuatorFromSysId() -> actuator:  Used to get the actuator object from the actuator system ID
# -formatActuatorInventoryTable() -> List[List[str]]: Populate the actuator inventory table
# -rowCount() -> int:                       Returns the current table view row count
# -columnCount() -> int:                    Returns the current table view column count
# -headerData() -> List[str] | None:        Returns the actuator table header columns
# -data() -> str | Any:                     Returns actuator table cell data
# -<*>Col() -> int                          Provides the coresponding column index as provided with formatActuatorInventoryTable()
# 
# Private Methods and objects only to be used internally or by the actuatorHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_actuatorInventoryTableReloadLock : threading.Lock:
#                                           actuator inventory Re-load lock
# -_parent : Any:                           Calling UI object
# -_actuatorInventoryTable : List[List[str]]: Inventory list of lists [row][column]
# -_colCnt                                  actuator table column count
# -_rowCnt                                  actuator table row count
#
# Private methods:
# ----------------
# -_reLoadData() -> None                    Reload actuator inventory content
#################################################################################################################################################
class actuatorInventoryTableModel(QtCore.QAbstractTableModel):
    def __init__(self, p_parent, parentObjHandle):
        self._actuatorInventoryTableReloadLock  = threading.Lock()
        self._parent : Any = p_parent
        self.parentObjHandle = parentObjHandle
        self._actuatorInventoryTable : List[List[str]] = []
        self._colCnt : int = 0
        self._rowCnt : int = 0
        QtCore.QAbstractTableModel.__init__(self)
        self.reLoadData()

    def isFirstColumnObjectId(self) -> bool:
        return True

    def isFirstColumnInstanceId(self) -> bool: 
        return False

    def getActuatorObjFromSysId(self, actuatorSysId : str) -> ...:
        actuatorInventoryList : List[actuator] = self.getAllActuators()
        for actuatorItter in actuatorInventoryList:
            if actuatorItter.jmriActSystemName.value == actuatorSysId:
                return actuatorItter
        return None

    def reLoadData(self) -> None:
        with self._actuatorInventoryTableReloadLock:
            self._actuatorInventoryTable = self.formatActuatorInventoryTable()
            try:
                self._colCnt = len(self._actuatorInventoryTable[0])
            except:
                self._colCnt = 0
            self._rowCnt = len(self._actuatorInventoryTable)
        self._parent.actuatorInventoryTableView.resizeColumnsToContents()
        self._parent.actuatorInventoryTableView.resizeRowsToContents()

    def formatActuatorInventoryTable(self) -> List[List[str]]:
        self._actuatorInventoryTable : List[actuator] = []
        actuatorInventoryList : List[actuator] = self.getAllActuators()
        for actuatorInventoryItter in actuatorInventoryList:
            actuatorInventoryRow : List[str] = []
            actuatorInventoryRow.append(actuatorInventoryItter.jmriActSystemName.value)
            actuatorInventoryRow.append(actuatorInventoryItter.userName.value)
            actuatorInventoryRow.append(actuatorInventoryItter.description.value)
            actuatorInventoryRow.append(actuatorInventoryItter.actType.value)
            actuatorInventoryRow.append(actuatorInventoryItter.actSubType.value)
            actuatorInventoryRow.append(actuatorInventoryItter.actPort.value)
            actuatorInventoryRow.append(actuatorInventoryItter.getOpStateDetailStr())
            actuatorInventoryRow.append(actuatorInventoryItter.actState)
            actuatorInventoryRow.append(actuatorInventoryItter.getUptime())
            actuatorInventoryRow.append(actuatorInventoryItter.getTopology())
            self._actuatorInventoryTable.append(actuatorInventoryRow)
        return self._actuatorInventoryTable

    def rowCount(self, p_parent : Any = Qt.QModelIndex()) -> int:
        with self._actuatorInventoryTableReloadLock:
            return self._rowCnt

    def columnCount(self, p_parent : Any = Qt.QModelIndex()):
        with self._actuatorInventoryTableReloadLock:
            return self._colCnt

    def headerData(self, section, orientation, role):
        with self._actuatorInventoryTableReloadLock:
            if role != QtCore.Qt.DisplayRole:
                return None
            if orientation == QtCore.Qt.Horizontal:
                return ("SysName:","UsrName:", "Desc:", "Type:", "SubType:", "Port:", "OpState:", "Position:", "UpTime[s]:", "Topology:")[section]
            else:
                return f"{section}"

    def data(self, index : Any, role : Any = QtCore.Qt.DisplayRole)-> str | Any:
        with self._actuatorInventoryTableReloadLock:
            column = index.column()
            row = index.row()
            if role == QtCore.Qt.DisplayRole:
                return self._actuatorInventoryTable[row][column]
            if role == QtCore.Qt.ForegroundRole:
                if self.getActuatorObjFromSysId(self._actuatorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() == OP_WORKING[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#779638'))
                if self.getActuatorObjFromSysId(self._actuatorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_DISABLED[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#505050'))
                if self.getActuatorObjFromSysId(self._actuatorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & ~OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF0000'))
                if self.getActuatorObjFromSysId(self._actuatorInventoryTable[row][self.sysNameCol()]).getOpStateDetail() & OP_CBL[STATE]:
                    return QtGui.QBrush(QtGui.QColor('#FF8000'))
                return QtGui.QBrush(QtGui.QColor('#0000FF'))
            if role == QtCore.Qt.TextAlignmentRole:
                return QtCore.Qt.AlignCenter

    def getAllActuators(self):
        actuators : List[actuator] = []
        for decoderItter in self.parentObjHandle.decoders.value:
            for satLinkLinkItter in decoderItter.satLinks.value:
                for satItter in satLinkLinkItter.satellites.value:
                    actuators.extend(satItter.actuators.value)
        return actuators

    @staticmethod
    def sysNameCol() -> int: return 0
    @staticmethod
    def usrNameCol() -> int: return 1
    @staticmethod
    def descCol() -> int: return 2
    @staticmethod
    def typeCol() -> int: return 3
    @staticmethod
    def subTypeCol() -> int: return 4
    @staticmethod
    def portCol() -> int: return 5
    @staticmethod
    def opStateCol() -> int: return 6
    @staticmethod
    def positionCol() -> int: return 7
    @staticmethod
    def upTimeCol() -> int: return 8
    @staticmethod
    def topologyCol() -> int: return 9
#################################################################################################################################################
# End Class: actuatorInventoryTableModel
#################################################################################################################################################

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# END - ACTUATOR DIALOG CLASSES
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
