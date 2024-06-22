#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# This is the MAIN myRailIO deamon program
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
import os
import sys
import psutil
from topDecoderLogic import *
from pathlib import Path
from PyQt5.QtGui import QIcon
import ctypes


#################################################################################################################################################
# MAIN
#################################################################################################################################################
if __name__ == '__main__':
    app = QApplication(sys.argv)
    win = UI_mainWindow()
    myappid = 'myRailIO.Server.GUI.version' 
    ctypes.windll.shell32.SetCurrentProcessExplicitAppUserModelID(myappid)
    win.setWindowIcon(QIcon('icons\\myRailIO.png'))
    win.setParentObjHandle(topDecoder(win))
    win.show()
    app.exec()
    current_system_pid = os.getpid()
    ThisSystem = psutil.Process(current_system_pid)
    ThisSystem.terminate()
    exit()
