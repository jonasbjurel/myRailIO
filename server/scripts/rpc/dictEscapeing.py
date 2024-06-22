#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# dictEscaping provides methods to escape- and unEscape special caracters in python dict characters which cannot otherwise be serialized
# by dicttoxml and xmltodict.
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
import sys
class dictEscapeing():
    TRANSLATION = {'___-:09':'\t',
    '\n':'___-:10:-___',
    "\r":'___-:13:-___',
    ' ':'___-:32:-___',
    '!':'___-:33:-___',
    '\"':'___-:34:-___',
    '#':'___-:35:-___',
    '$':'___-:36:-___',
    '%':'___-:37:-___',
    '&':'___-:38:-___',
    '\'':'___-:39:-___',
    '(':'___-:40:-___',
    ')':'___-:41:-___',
    '*':'___-:42:-___',
    '+':'___-:43:-___',
    ',':'___-:44:-___',
    '-':'___-:45:-___',
    '.':'___-:46:-___',
    '/':'___-:47:-___',
    ':':'___-:58:-___',
    ';':'___-:59:-___',
    '<':'___-:60:-___',
    '=':'___-:61:-___',
    '>':'___-:62:-___',
    '?':'___-:63:-___',
    '@':'___-:64:-___',
    '[':'___-:91:-___',
    '\\':'___-:92:-___',
    ']':'___-:93:-___',
    '^':'___-:94:-___',
    '_':'___-:95:-___',
    '`':'___-:96:-___',
    '{':'___-:123:-___',
    '|':'___-:124:-___',
    '}':'___-:125:-___',
    '~':'___-:126:-___'}

    @staticmethod
    def reverseLookup(code):
        for char in dictEscapeing.TRANSLATION:
            if dictEscapeing.TRANSLATION[char] == code:
                return char

    @staticmethod
    def escapeStr(string):
        newString = ""
        for i in range(0, len(string)):
            if string[i] in dictEscapeing.TRANSLATION:
                newString = newString + dictEscapeing.TRANSLATION[string[i]]
            else:
                newString = newString + string[i]
        return newString

    @staticmethod
    def unEscapeStr(string):
        newString = ""
        j = 0
        for i in range(0, len(string)):
            if i < j:
                continue
            if len(string) >= i+5 and string[i:i+5] == "___-:":
                code = ""
                for j in range(i, len(string)):
                    code = code + string[j]
                    if len(string) >= j+5 and string[j:j+5] == ":-___":
                        code = code + string[j+1:j+5]
                        j = j + 5
                        break
                newString = newString + dictEscapeing.reverseLookup(code)
            else:
                newString = newString + string[i]
        return newString

    @staticmethod
    def dictEscape(srcDict, escape=True):
        transformedDict = {}
        for key in srcDict:
            if escape:
                newKey = dictEscapeing.escapeStr(key)
            else:
                newKey = dictEscapeing.unEscapeStr(key)
            if sys.version_info.major == 2:
                if isinstance(srcDict[key], (str, unicode, int)) or srcDict[key] == None:
                    strOrNone = True
                else:
                    strOrNone = False
            if sys.version_info.major == 3:
                if isinstance(srcDict[key], (str, int)) or srcDict[key] == None:
                    strOrNone = True
                else:
                    strOrNone = False

            if strOrNone:
                transformedDict[newKey] = srcDict[key]
            elif isinstance(srcDict[key], dict):
                transformedDict[newKey] = dictEscapeing.dictEscape(srcDict[key], escape=escape)
            else:
                print("ERROR Instanse is neither str, unicode, none or dict")
                print(type(srcDict[key]))
                print(srcDict)


        return transformedDict

    @staticmethod
    def dictUnEscape(srcDict):
        return dictEscapeing.dictEscape(srcDict, escape=False)
