import sys
from PyQt5.QtWidgets import QApplication, QDialog, QMainWindow, QMessageBox, QMenu
from PyQt5.uic import loadUi
from PyQt5.Qt import QStandardItemModel, QStandardItem
from PyQt5.QtGui import QFont, QColor, QTextCursor, QIcon
from PyQt5 import QtCore
import time
import threading
import traceback
from sysState import *
from rc import *
from dataModel import *
from momResources import *



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

# UI definitions
MAIN_FRAME_UI = "ui/Main_Frame.ui"
TOP_DIALOG_UI = "ui/Top_Dialog.ui"
ADD_DIALOG_UI = "ui/Add_Dialog.ui"
DECODER_DIALOG_UI = "ui/Decoder_Dialog.ui"
SATLINK_DIALOG_UI = "ui/SatLink_Dialog.ui"
LIGHTGROUP_DIALOG_UI = "ui/LightGroup_Dialog.ui"
SATELITE_DIALOG_UI = "ui/Sat_Dialog.ui"
SENSOR_DIALOG_UI = "ui/Sensor_Dialog.ui"
ACTUATOR_DIALOG_UI = "ui/Actuator_Dialog.ui"
LOGOUTPUT_DIALOG_UI = "ui/Log_Output_Dialog.ui"
LOGSETTING_DIALOG_UI = "ui/Log_Setting_Dialog.ui"

# UI Icon resources
SERVER_ICON = "icons/server.png"
DECODER_ICON = "icons/decoder.png"
LINK_ICON = "icons/link.png"
SATELITE_ICON = "icons/satelite.png"
TRAFFICLIGHT_ICON = "icons/traffic-light.png"
SENSOR_ICON = "icons/sensor.png"
ACTUATOR_ICON = "icons/servo.png"


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
            #self.setIcon = icon

    def getObj(self):
        return self.obj

    def __delete__(self):
        super().__delete__()



class UI_mainWindow(QMainWindow):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.lock = threading.Lock()
        loadUi(MAIN_FRAME_UI, self)
        self.connectActionSignalsSlots()
        self.connectWidgetSignalsSlots()
        self.MoMTreeModel = QStandardItemModel()
        self.MoMroot = self.MoMTreeModel.invisibleRootItem()
        self.topMoMTree.setModel(self.MoMTreeModel)
        self.topMoMTree.expandAll()

    def setTop(self, top):
        self.top = top

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
        MoMName = str(self.topMoMTree.selectedIndexes()[0].data())
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
        rc = 0
        try:
            if action == viewAction: rc = stdItemObj.getObj().view()
            if action == addAction: rc = stdItemObj.getObj().add()
            if action == editAction: rc = stdItemObj.getObj().edit()
            if action == copyAction: rc = stdItemObj.getObj().copy()
            if action == deleteAction: rc = stdItemObj.getObj().delete()
            if action == enableAction: rc = stdItemObj.getObj().enable()
            if action == enableRecursAction: rc = stdItemObj.getObj().enableRecurs()
            if action == disableAction: rc = stdItemObj.getObj().disable()
            if action == disableRecursAction: rc = stdItemObj.getObj().disableRecurs()
            if action == logAction: 
                self.logDialog = UI_logDialog(stdItemObj.getObj())
                self.logDialog.show()
            if action == restartAction: rc = stdItemObj.getObj().restart()
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not execute action: " + str(action.text()))
                msg.setInformativeText(ERROR_TEXT[rc])
                msg.setWindowTitle("Error")
                msg.exec_()
        except Exception:
            traceback.print_exc()

    def connectActionSignalsSlots(self):
        #self.action_Exit.triggered.connect(self.close)
        self.actionAbout_genJMRI.triggered.connect(self.about)

    def connectWidgetSignalsSlots(self):
        # MoM tree widget
        self.topMoMTree.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.topMoMTree.customContextMenuRequested.connect(self.MoMMenuContextTree)

        # Log widget
        self.topLogPushButton.clicked.connect(self.log)

        # Restart widget
        self.restartPushButton.clicked.connect(self.restart)

    def log(self):
        self.log = UI_logDialog(self.top)
        self.log.show()

    def restart(self):
        self.top.restart()

    def about(self):
        QMessageBox.about(
            self,
            "About genJMRI",
            "<p>genJMRI implements a generic signal masts-, actuators- and sensor decoder framework for JMRI, </p>"
            "<p>For more information see:</p>"
            "<p>www.svd.se</p>",
        )



class UI_logDialog(QDialog):
    def __init__(self, parentObjHandle, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(LOGOUTPUT_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.setWindowTitle(self.parentObjHandle.getNameKey() + " stdout TTY")
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
        self.logOutputTextBrowser.append(self.parentObjHandle.getNameKey() + " stdout TTY - log level: " + self.parentObjHandle.getLogVerbosity() + ">")

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
        self.logSettingVerbosityComboBox.setCurrentText(self.parentObjHandle.getLogVerbosity())

    def connectWidgetSignalsSlots(self):
        self.logSettingConfirmButtonBox.accepted.connect(self.accepted)
        self.logSettingConfirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.parentObjHandle.setLogVerbosity(self.logSettingVerbosityComboBox.currentText())
        self.close()

    def rejected(self):
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
        elif resourceTypeStr == "Light group": resourceType = LIGHT_GROUP
        elif resourceTypeStr == "Satelite link": resourceType = SATELITE_LINK
        elif resourceTypeStr == "Satelite": resourceType = SATELITE
        elif resourceTypeStr == "Sensor": resourceType = SENSOR
        elif resourceTypeStr == "Actuator": resourceType = ACTUATOR
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
        self.gitTagComboBox.setEnabled(True)
        self.authorLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.versionLineEdit.setEnabled(True)
        self.dateLineEdit.setEnabled(True)
        self.ntpLineEdit.setEnabled(True)
        self.timeZoneSpinBox.setEnabled(True)
        self.rsyslogLineEdit.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.supervisionPeriodSpinBox.setEnabled(True)
        self.decoderFailSafeCheckBox.setEnabled(True)
        self.trackFailSafeCheckBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.gitCiPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.gitTagComboBox.setEnabled(False)
        self.authorLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.versionLineEdit.setEnabled(False)
        self.dateLineEdit.setEnabled(False)
        self.ntpLineEdit.setEnabled(False)
        self.timeZoneSpinBox.setEnabled(False)
        self.rsyslogLineEdit.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.supervisionPeriodSpinBox.setEnabled(False)
        self.decoderFailSafeCheckBox.setEnabled(False)
        self.trackFailSafeCheckBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.gitCiPushButton.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.gitTagComboBox.setCurrentText(str(self.parentObjHandle.getGitTag()))
        self.authorLineEdit.setText(str(self.parentObjHandle.getAuthor()))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.versionLineEdit.setText(str(self.parentObjHandle.getVersion()))
        self.dateLineEdit.setText(str(self.parentObjHandle.getDate()))
        self.ntpLineEdit.setText(str(self.parentObjHandle.getNtp()))
        self.timeZoneSpinBox.setValue(self.parentObjHandle.getTz())
        self.rsyslogLineEdit.setText(str(self.parentObjHandle.getRsysLog()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.supervisionPeriodSpinBox.setValue(self.parentObjHandle.getSupervisionPeriod())
        self.decoderFailSafeCheckBox.setChecked(self.parentObjHandle.getDecoderFailSafe())
        self.trackFailSafeCheckBox.setChecked(self.parentObjHandle.getTrackFailSafe())
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        self.parentObjHandle.setGitTag(self.gitTagComboBox.currentText())
        self.parentObjHandle.setAuthor(self.authorLineEdit.displayText())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setVersion(self.versionLineEdit.displayText())
        self.parentObjHandle.setDate(self.dateLineEdit.displayText())
        self.parentObjHandle.setNtp(self.ntpLineEdit.displayText())
        self.parentObjHandle.setTz(self.timeZoneSpinBox.value())
        self.parentObjHandle.setRsysLog(self.rsyslogLineEdit.displayText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        self.parentObjHandle.setSupervisionPeriod(self.supervisionPeriodSpinBox.value())
        self.parentObjHandle.setDecoderFailSafe(self.decoderFailSafeCheckBox.isChecked())
        self.parentObjHandle.setTrackFailSafe(self.trackFailSafeCheckBox.isChecked())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)
        self.gitCiPushButton.clicked.connect(self.parentObjHandle.gitCi)

    def accepted(self):
        self.setValues()
        self.close()

    def rejected(self):
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
        self.uriLineEdit.setEnabled(True)
        self.macLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.uriLineEdit.setEnabled(False)
        self.macLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.uriLineEdit.setText(str(self.parentObjHandle.getUri()))
        self.macLineEdit.setText(str(self.parentObjHandle.getMac()))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)

    def setValues(self):
        self.parentObjHandle.setUri(self.uriLineEdit.displayText())
        self.parentObjHandle.setMac(self.macLineEdit.displayText())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)
        pass

    def accepted(self):
        self.setValues()
        self.close()

    def rejected(self):
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
        self.linkNoSpinBox.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(True)
        self.remCrcErrLineEdit.setEnabled(True)
        self.rxSymErrLineEdit.setEnabled(True)
        self.rxSizeErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.linkNoSpinBox.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.remCrcErrLineEdit.setEnabled(False)
        self.rxSymErrLineEdit.setEnabled(False)
        self.rxSizeErrLineEdit.setEnabled(False)
        self.wdErrLineEdit.setEnabled(False)
        self.clearStatsPushButton.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.linkNoSpinBox.setValue(self.parentObjHandle.getSatLinkNo())
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.getRxCrcErr()))
        self.remCrcErrLineEdit.setText(str(self.parentObjHandle.getRemCrcErr()))
        self.rxSymErrLineEdit.setText(str(self.parentObjHandle.getRxSymErr()))
        self.rxSizeErrLineEdit.setText(str(self.parentObjHandle.getRxSizeErr()))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.getWdErr()))

    def setValues(self):
        self.parentObjHandle.setSatLinkNo(self.linkNoSpinBox.value())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

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
        self.setValues()
        self.close()

    def rejected(self):
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
        self.JMRIAddrLineEdit.setEnabled(True)
        self.lgLinkSpinBox.setEnabled(True)
        self.lgSpinBox.setEnabled(True)
        self.lgSeqSpinBox.setEnabled(True)
        self.JMRISystemNameLineEdit.setEnabled(True)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.lgTypeComboBox.setEnabled(True)
        self.lgProp1ComboBox.setEnabled(False)
        self.lgProp2ComboBox.setEnabled(False)
        self.lgProp3ComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.lgShowingLineEdit.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRIAddrLineEdit.setEnabled(False)
        self.lgLinkSpinBox.setEnabled(False)
        self.lgSpinBox.setEnabled(False)
        self.lgSeqSpinBox.setEnabled(False)
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.lgTypeComboBox.setEnabled(False)
        self.lgProp1ComboBox.setEnabled(False)
        self.lgProp2ComboBox.setEnabled(False)
        self.lgProp3ComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.lgShowingLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRIAddrLineEdit.setText(str(self.parentObjHandle.getJMRIAddr()))
        self.lgLinkSpinBox.setValue(self.parentObjHandle.getLgLink())
        self.lgSpinBox.setValue(self.parentObjHandle.getLg())
        self.lgSeqSpinBox.setValue(self.parentObjHandle.getLgSeq())
        self.JMRISystemNameLineEdit.setText(str(self.parentObjHandle.getJMRISystemName()))
        self.JMRIUserNameLineEdit.setText(str(self.parentObjHandle.getJMRIUserName()))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.lgTypeComboBox.setCurrentText(str(self.parentObjHandle.getLgType()))
        self.lgPropertyHandler()
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.adminStateForceCheckBox.setChecked(False)
        self.lgShowingLineEdit.setText(str(self.parentObjHandle.getLgShowing()))

    def setValues(self):
        self.parentObjHandle.setLgLink(self.lgLinkSpinBox.value())
        self.parentObjHandle.setLg(self.lgSpinBox.value())
        self.parentObjHandle.setLgSeq(self.lgSeqSpinBox.value())
        self.parentObjHandle.setJMRISystemName(self.JMRISystemNameLineEdit.displayText())
        self.parentObjHandle.setJMRIUserName(self.JMRIUserNameLineEdit.displayText())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setLgType(self.lgTypeComboBox.currentText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

    def lgPropertyHandler(self):
        pass # Define sub properties here

    def lgPropertySetHandler(self):
        pass # Define sub properties here

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.setValues()
        self.close()

    def rejected(self):
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
        self.satAddrSpinBox.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.rxCrcErrLineEdit.setEnabled(True)
        self.txCrcErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(True)
        self.updateStatsPushButton.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.satAddrSpinBox.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.rxCrcErrLineEdit.setEnabled(False)
        self.txCrcErrLineEdit.setEnabled(True)
        self.wdErrLineEdit.setEnabled(False)
        self.updateStatsPushButton.setEnabled(True)
        self.clearStatsPushButton.setEnabled(True)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.satAddrSpinBox.setValue(self.parentObjHandle.getSatAddr())
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.displayStats()

    def displayStats(self):
        self.rxCrcErrLineEdit.setText(str(self.parentObjHandle.getRxCrcErr()))
        self.txCrcErrLineEdit.setText(str(self.parentObjHandle.getTxCrcErr()))
        self.wdErrLineEdit.setText(str(self.parentObjHandle.getWdErr()))

    def setValues(self):
        self.parentObjHandle.setSatAddr(self.satAddrSpinBox.value())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

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
        self.setValues()
        self.close()

    def rejected(self):
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
        self.JMRIAddrLineEdit.setEnabled(True)
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
        self.logVerbosityComboBox.setEnabled(True)
        self.sensStateLineEdit.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRIAddrLineEdit.setEnabled(False)
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
        self.logVerbosityComboBox.setEnabled(False)
        self.sensStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRIAddrLineEdit.setText(str(self.parentObjHandle.getJMRIAddr()))
        self.JMRISystemNameLineEdit.setText(str(self.parentObjHandle.getJMRISystemName()))
        self.JMRIUserNameLineEdit.setText(str(self.parentObjHandle.getJMRIUserName()))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.sensPortSpinBox.setValue(self.parentObjHandle.getSensPort())
        self.sensTypeComboBox.setCurrentText(str(self.parentObjHandle.getSensType()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.sensStateLineEdit.setText(str(self.parentObjHandle.getSensState()))

    def setValues(self):
        self.parentObjHandle.setSensPort(self.sensPortSpinBox.value())
        self.parentObjHandle.setJMRISystemName(self.JMRISystemNameLineEdit.displayText())
        self.parentObjHandle.setJMRIUserName(self.JMRIUserNameLineEdit.displayText())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setSensType(self.sensTypeComboBox.currentText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.setValues()
        self.close()

    def rejected(self):
        self.close()



class UI_actuatorDialog(QDialog):
    def __init__(self, parentObjHandle, edit=False, parent=None):
        super().__init__(parent)
        self.parentObjHandle = parentObjHandle
        loadUi(ACTUATOR_DIALOG_UI, self)
        self.connectWidgetSignalsSlots()
        self.displayValues()
        if edit:
            self.setEditable()
        else:
            self.unSetEditable()

    def setEditable(self):
        self.JMRIAddrLineEdit.setEnabled(True)
        self.JMRISystemNameLineEdit.setEnabled(True)
        self.JMRIUserNameLineEdit.setEnabled(True)
        self.descriptionLineEdit.setEnabled(True)
        self.actPortSpinBox.setEnabled(True)
        self.actTypeComboBox.setEnabled(True)
        self.opStateSummaryLineEdit.setEnabled(True)
        self.opStateDetailLineEdit.setEnabled(True)
        self.upTimeLineEdit.setEnabled(True)
        self.adminStateComboBox.setEnabled(True)
        self.adminStateForceCheckBox.setEnabled(True)
        self.logVerbosityComboBox.setEnabled(True)
        self.actStateLineEdit.setEnabled(True)
        self.confirmButtonBox.setEnabled(True)

    def unSetEditable(self):
        self.JMRIAddrLineEdit.setEnabled(False)
        self.JMRISystemNameLineEdit.setEnabled(False)
        self.JMRIUserNameLineEdit.setEnabled(False)
        self.descriptionLineEdit.setEnabled(False)
        self.actPortSpinBox.setEnabled(False)
        self.actTypeComboBox.setEnabled(False)
        self.opStateSummaryLineEdit.setEnabled(False)
        self.opStateDetailLineEdit.setEnabled(False)
        self.upTimeLineEdit.setEnabled(False)
        self.adminStateComboBox.setEnabled(False)
        self.adminStateForceCheckBox.setEnabled(False)
        self.logVerbosityComboBox.setEnabled(False)
        self.actStateLineEdit.setEnabled(False)
        self.confirmButtonBox.setEnabled(False)

    def displayValues(self):
        self.JMRIAddrLineEdit.setText(str(self.parentObjHandle.getJMRIAddr()))
        self.JMRISystemNameLineEdit.setText(str(self.parentObjHandle.getJMRISystemName()))
        self.JMRIUserNameLineEdit.setText(str(self.parentObjHandle.getJMRIUserName()))
        self.descriptionLineEdit.setText(str(self.parentObjHandle.getDescription()))
        self.actPortSpinBox.setValue(self.parentObjHandle.getActPort())
        self.actTypeComboBox.setCurrentText(str(self.parentObjHandle.getActType()))
        self.logVerbosityComboBox.setCurrentText(str(self.parentObjHandle.getLogVerbosity()))
        self.opStateSummaryLineEdit.setText(str(self.parentObjHandle.getOpStateSummary()[STATE_STR]))
        self.opStateDetailLineEdit.setText(str(self.parentObjHandle.getOpStateDetailStr()))
        self.upTimeLineEdit.setText(str(self.parentObjHandle.getUptime()))
        self.adminStateComboBox.setCurrentText(str(self.parentObjHandle.getAdmState()[STATE_STR]))
        self.adminStateForceCheckBox.setChecked(False)
        self.actStateLineEdit.setText(str(self.parentObjHandle.getActState()))

    def setValues(self):
        self.parentObjHandle.setActPort(self.actPortSpinBox.value())
        self.parentObjHandle.setJMRISystemName(self.JMRISystemNameLineEdit.displayText())
        self.parentObjHandle.setJMRIUserName(self.JMRIUserNameLineEdit.displayText())
        self.parentObjHandle.setDescription(self.descriptionLineEdit.displayText())
        self.parentObjHandle.setActType(self.actTypeComboBox.currentText())
        self.parentObjHandle.setLogVerbosity(self.logVerbosityComboBox.currentText())
        if self.adminStateForceCheckBox.isChecked():
            rc = self.parentObjHandle.setAdmStateRecurs(self.adminStateComboBox.currentText())
        else:
            rc = self.parentObjHandle.setAdmState(self.adminStateComboBox.currentText())
            if rc:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("Error could not change Adm State")
                msg.setInformativeText('ReturnCode: ' + str(rc))
                msg.setWindowTitle("Error")
                msg.exec_()

    def connectWidgetSignalsSlots(self):
        self.confirmButtonBox.accepted.connect(self.accepted)
        self.confirmButtonBox.rejected.connect(self.rejected)

    def accepted(self):
        self.setValues()
        self.close()

    def rejected(self):
        self.close()
