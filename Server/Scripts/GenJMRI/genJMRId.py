#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# This is the MAIN genJMRI deamon program
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################



#################################################################################################################################################
# Dependencies
import os
import sys
from topDecoderLogic import *



#################################################################################################################################################
# MAIN
#################################################################################################################################################
if __name__ == '__main__':
    print("0")
    app = QApplication(sys.argv)
    print("1")
    win = UI_mainWindow()
    print("2")
    win.setParentObjHandle(topDecoder(win))
    print("3")
    win.show()
    print("4")
    sys.exit(app.exec())
