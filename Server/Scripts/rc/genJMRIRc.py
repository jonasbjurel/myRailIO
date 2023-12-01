#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# genJMRI common return and error codes.
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
class rc():
    OK =                         0
    NOT_DISABLE =                1
    PARENT_NOT_ENABLE =          2
    CHILD_NOT_DISABLE =          3
    CHILDS_EXIST =               4
    DOES_NOT_EXIST =             5
    ALREADY_EXISTS =             6
    CANNOT_DELETE =              7
    KEEPALIVE_TIMEOUT =          8
    PARSE_ERR =                  9
    TYPE_VAL_ERR =               10
    RANGE_ERR =                  11
    PARAM_ERR =                  12
    SOCK_ERR =                   13
    GEN_COM_ERR =                14
    GEN_ERR =                    255

    ERROR_TEXT = [""] * 256
    ERROR_TEXT[0] =                 "Result OK"
    ERROR_TEXT[1] =                 "Object not disabled"
    ERROR_TEXT[2] =                 "Parent object not enabeled"
    ERROR_TEXT[3] =                 "Child objects not disabled"
    ERROR_TEXT[4] =                 "Child objects exsist"
    ERROR_TEXT[5] =                 "Object does not exist"
    ERROR_TEXT[6] =                 "Object already exists"
    ERROR_TEXT[7] =                 "Object cannot be deleted"
    ERROR_TEXT[8] =                 "Keep-alive timeout"
    ERROR_TEXT[9] =                 "Parsing error"
    ERROR_TEXT[10] =                 "Type value error"
    ERROR_TEXT[11] =                "Range error"
    ERROR_TEXT[12] =                "Parameter error"
    ERROR_TEXT[13] =                "Socket connect error"
    ERROR_TEXT[14] =                "General communication error"
    ERROR_TEXT[255] =               "General error"

    @staticmethod
    def getErrStr(errNo):
        try:
            return rc.ERROR_TEXT[errNo]
        except:
            return "Unexpected return code: " + str(errNo) + " received"