#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# TestBench for the genJMRIRpc framework.
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################

#################################################################################################################################################
# Module/Library dependance
#################################################################################################################################################
import os
import sys
sys.path.append(os.path.realpath('..'))
from JMRIObjects import jmriObj
from genJMRIRpcClient import *
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
# END <Module/Library dependance> ---------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Parameters
#################################################################################################################################################
interactive = True
result = 0
MAST_FAIL =     0b00000001
TURNOUT_FAIL =  0b00000010
LIGHT_FAIL =    0b00000100
SENSOR_FAIL =   0b00001000
CB_FAIL =       0b00010000
MEMORY_FAIL =   0b00100000
MQTT_FAIL =     0b01000000
OTHER_FAIL =    0b10000000

MAST_STATE_CHANGES = 0
MAST_STATE_CHANGES = 0
TURNOUT_STATE_CHANGES = 0
LIGHT_STATE_CHANGES = 0
SENSOR_STATE_CHANGES = 0
# END <Parameters> ------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Event callbacks
#################################################################################################################################################
def errHandler(errNo):
    print("Client error handler reports error: " + str (errNo))

def mastListener(event):
    print("Listener - mast changed aspect from " + event.oldState + " to " + event.newState)
    global MAST_STATE_CHANGES
    MAST_STATE_CHANGES += 1

def turnoutListener(event):
    print("Listener - turnout changed position from " + event.oldState + " to " + event.newState)
    global TURNOUT_STATE_CHANGES
    TURNOUT_STATE_CHANGES += 1

def lightListener(event):
    print("Listener - light changed lit from " + event.oldState + " to " + event.newState)
    global LIGHT_STATE_CHANGES
    LIGHT_STATE_CHANGES += 1

def sensorListener(event):
    print("Listener - sensor changed sense from " + event.oldState + " to " + event.newState)
    global SENSOR_STATE_CHANGES
    SENSOR_STATE_CHANGES += 1
# END <Event callbacks> ------------------------------------------------------------------------------------------------------------------------------



#################################################################################################################################################
# Test bench code
#################################################################################################################################################
## Creating RPC backend
trace.start()
print("Starting genJMRI RPC Client")
RPC_CLIENT = jmriRpcClient()
RPC_CLIENT.start(uri = "127.0.0.2", errCb=errHandler)

# Excersising log level
print("\n\nRetreiving and setting global logLevel")
debugLevelStr = RPC_CLIENT.getGlobalDebugLevelStr()
print("Current global logLevel is " + RPC_CLIENT.getGlobalDebugLevelStr())
print("Setting global LogLevel to DEBUG-PANIC")
RPC_CLIENT.setGlobalDebugLevelStr("DEBUG-PANIC")
trace.setGlobalDebugLevel(trace.getSeverityFromSeverityStr("DEBUG-PANIC"))

print("Current global logLevel is: " + RPC_CLIENT.getGlobalDebugLevelStr())

if RPC_CLIENT.getGlobalDebugLevelStr() != "DEBUG-PANIC":
    print("ERROR - Could not set global logLevel")
    result = result | OTHER_FAIL
else:
    print("PASS - global logLevel was successfully set")
print("Resetting Debug level to " + debugLevelStr)
RPC_CLIENT.setGlobalDebugLevelStr(debugLevelStr)
trace.setGlobalDebugLevel(trace.getSeverityFromSeverityStr(debugLevelStr))

if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break

# Excersising keep-alive 
print("\n\nRetreiving and setting keep-alive interval")
print("Current keep-alive interval is " + str(RPC_CLIENT.getKeepaliveInterval()))
print("Setting keep-alive interval to 1 second and waiting 30 seconds")
RPC_CLIENT.setKeepaliveInterval(1)
if RPC_CLIENT.getKeepaliveInterval() != 1 or int(RPC_CLIENT.getStateBySysName(jmriObj.MEMORIES, "IM_GENJMRI_RPC_KEEPALIVE_INTERVAL")) != 1:
    print("ERROR - Could not set keep-alive interval")
    result = result | OTHER_FAIL
else:
    print("PASS - keep-alive interval was successfully set to one second")
    time.sleep(30)
print("Resetting keep-alive interval to 30 seconds")
RPC_CLIENT.setKeepaliveInterval(30)
if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break

# Requesting configuration
print("\n\nRetreiving configuration:")
print("\nMast configuration:....")
print(RPC_CLIENT.getConfigsByType(jmriObj.MASTS))

if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break
print("\nTurnout configuration:....")
print(RPC_CLIENT.getConfigsByType(jmriObj.TURNOUTS))

if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break
print("\nLights configuration:....")
print(RPC_CLIENT.getConfigsByType(jmriObj.LIGHTS))

if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break
print("\nSensors configuration:....")
print(RPC_CLIENT.getConfigsByType(jmriObj.SENSORS))

if interactive:
    print("Press a key...")
    while True:
        if keyboard.read_key():
            time.sleep(0.5)
            break

# Creating a signal mast
print("\n\nCreating and configuring JMRI objects")
print("\nCreate a signal mast - SysName: IF$vsm:Sweden-3HMS:SL-5HL($0100), UsrName: My SignalMast, Comment: My SignalMast comment")
RPC_CLIENT.deleteObject(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
RPC_CLIENT.createObject(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
RPC_CLIENT.setUserNameBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", "My SignalMast")
RPC_CLIENT.setCommentBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", "My SignalMast comment")
RPC_CLIENT.regEventCb(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", mastListener)

if RPC_CLIENT.getObjectConfig(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)") == None or\
                    RPC_CLIENT.getUserNameBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)") != "My SignalMast" or\
                    RPC_CLIENT.getCommentBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)") != "My SignalMast comment":
    print("ERROR - Mast was not propperly created - either it was not created at all, or its properties were not correctly set")
    result = result | MAST_FAIL
aspects = RPC_CLIENT.getValidStatesBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
for aspect in aspects:
    print("Changing aspect to: " + aspect)
    RPC_CLIENT.setStateBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", aspect)
    resultState = RPC_CLIENT.getStateBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
    if resultState != aspect:
        print("ERROR - aspect did not change as expected, expected apect: " + aspect + " detected aspect: " + resultState)
        result = result | MAST_FAIL
    else:
        print("PASS - aspect changed as expected, expected apect: " + aspect + " detected aspect: " + resultState)

# Creating a turnout
print("\nCreate a turnout - SysName: MT100, UsrName: My Turnout, Comment: My TurnOut comment")
RPC_CLIENT.deleteObject(jmriObj.TURNOUTS, "MT100")
RPC_CLIENT.createObject(jmriObj.TURNOUTS , "MT100")
RPC_CLIENT.setUserNameBySysName(jmriObj.TURNOUTS, "MT100", "My Turnout")
RPC_CLIENT.setCommentBySysName(jmriObj.TURNOUTS, "MT100", "My Turnout comment")
RPC_CLIENT.regEventCb(jmriObj.TURNOUTS, "MT100", turnoutListener)
if RPC_CLIENT.getObjectConfig(jmriObj.TURNOUTS, "MT100") == None or\
                    RPC_CLIENT.getUserNameBySysName(jmriObj.TURNOUTS, "MT100") != "My Turnout" or\
                    RPC_CLIENT.getCommentBySysName(jmriObj.TURNOUTS, "MT100") != "My Turnout comment":
    print("ERROR - Turnout was not propperly created - either it was not created at all, or its properties were not correctly set")
    result = result | TURNOUT_FAIL
positions = RPC_CLIENT.getValidStatesBySysName(jmriObj.TURNOUTS, "MT100")
for position in positions:
    print("Changing position to: " + position)
    RPC_CLIENT.setStateBySysName(jmriObj.TURNOUTS, "MT100", position)
    resultState = RPC_CLIENT.getStateBySysName(jmriObj.TURNOUTS, "MT100")
    if resultState != position:
        print("ERROR - turnout did not move as expected, expected position: " + position + " detected position: " + resultState)
        result = result | TURNOUT_FAIL
    else:
        print("PASS - turnout moved as expected, expected position: " + position + " detected position: " + resultState)

# Creating a light
print("\nCreate a light - SysName: ML100, UsrName: My Light, Comment: My Light comment")
RPC_CLIENT.deleteObject(jmriObj.LIGHTS, "ML100")
RPC_CLIENT.createObject(jmriObj.LIGHTS , "ML100")
RPC_CLIENT.setUserNameBySysName(jmriObj.LIGHTS, "ML100", "My Light")
RPC_CLIENT.setCommentBySysName(jmriObj.LIGHTS, "ML100", "My Light comment")
RPC_CLIENT.regEventCb(jmriObj.LIGHTS, "ML100", lightListener)
if RPC_CLIENT.getObjectConfig(jmriObj.LIGHTS, "ML100") == None or\
                    RPC_CLIENT.getUserNameBySysName(jmriObj.LIGHTS, "ML100") != "My Light" or\
                    RPC_CLIENT.getCommentBySysName(jmriObj.LIGHTS, "ML100") != "My Light comment":
    print("ERROR - Light was not propperly created - either it was not created at all, or its properties were not correctly set")
    result = result | LIGHT_FAIL
lightStates = RPC_CLIENT.getValidStatesBySysName(jmriObj.LIGHTS, "ML100")
for lightState in lightStates:
    print("Changing light to: " + lightState)
    RPC_CLIENT.setStateBySysName(jmriObj.LIGHTS, "ML100", lightState)
    resultState = RPC_CLIENT.getStateBySysName(jmriObj.LIGHTS, "ML100")
    if resultState != lightState:
        print("ERROR - light did not lit/unlit as expected, expected state: " + lightState + " detected state: " + resultState)
        result = result | LIGHT_FAIL
    else:
        print("PASS - light lit/unlit as expected, expected state: " + lightState + " detected state: " + resultState)

# Creating a sensor
print("\nCreate a sensor - SysName: MSMS100, UsrName: My Sensor, Comment: My Sensor comment")
RPC_CLIENT.deleteObject(jmriObj.SENSORS, "MSMS100")
RPC_CLIENT.createObject(jmriObj.SENSORS , "MSMS100")
RPC_CLIENT.setUserNameBySysName(jmriObj.SENSORS, "MSMS100", "My Sensor")
RPC_CLIENT.setCommentBySysName(jmriObj.SENSORS, "MSMS100", "My Sensor comment")
RPC_CLIENT.regEventCb(jmriObj.SENSORS, "MSMS100", sensorListener)
if RPC_CLIENT.getObjectConfig(jmriObj.SENSORS, "MSMS100") == None or\
                    RPC_CLIENT.getUserNameBySysName(jmriObj.SENSORS, "MSMS100") != "My Sensor" or\
                    RPC_CLIENT.getCommentBySysName(jmriObj.SENSORS, "MSMS100") != "My Sensor comment":
    print("ERROR - Sensor was not propperly created - either it was not created at all, or its properties were not correctly set")
    result = result | SENSOR_FAIL
sensorStates = RPC_CLIENT.getValidStatesBySysName(jmriObj.SENSORS, "MSMS100")
for sensorState in sensorStates:
    print("Changing sensor to: " + sensorState)
    RPC_CLIENT.setStateBySysName(jmriObj.SENSORS, "MSMS100", sensorState)
    resultState = RPC_CLIENT.getStateBySysName(jmriObj.SENSORS, "MSMS100")
    if resultState != sensorState:
        print("ERROR - sensor did not activate/de-activate as expected, expected state: " + sensorState + " detected state: " + resultState)
        result = result | SENSOR_FAIL
    else:
        print("PASS - sensor did activate/de-activate as expected, expected state: " + sensorState + " detected state: " + resultState)

# Creating a memory
print("\nCreate a memory - SysName: IM100, UsrName: My Memory, Comment: My Memory comment")
RPC_CLIENT.deleteObject(jmriObj.MEMORIES, "IM100")
RPC_CLIENT.createObject(jmriObj.MEMORIES , "IM100")
RPC_CLIENT.setUserNameBySysName(jmriObj.MEMORIES, "IM100", "My Memory")
RPC_CLIENT.setCommentBySysName(jmriObj.MEMORIES, "IM100", "My Memory comment")
if RPC_CLIENT.getObjectConfig(jmriObj.MEMORIES, "IM100") == None or\
                    RPC_CLIENT.getUserNameBySysName(jmriObj.MEMORIES, "IM100") != "My Memory" or\
                    RPC_CLIENT.getCommentBySysName(jmriObj.MEMORIES, "IM100") != "My Memory comment":
    print("ERROR - Memory was not propperly created - either it was not created at all, or its properties were not correctly set")
    result = result | MEMORY_FAIL
RPC_CLIENT.setStateBySysName(jmriObj.MEMORIES, "IM100", "My Memory content")
value = RPC_CLIENT.getStateBySysName(jmriObj.MEMORIES, "IM100")
if value != "My Memory content":
        print("ERROR - memory did not store value as expected, expected value: \"My Memory content\" detected value: " + value)
        result = result | MEMORY_FAIL
else:
    print("PASS - memory stored value as expected, expected value: \"My Memory content\" detected value: " + value)

# Creating MQTT emmiters
print("\nCreate remote MQTT emmitters")
if RPC_CLIENT.regMqttPub(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", "MyMastTopic", {"*":"*"}):
    print("Signal mast MQTT emmit registration failed")
    result = result | MQTT_FAIL
if RPC_CLIENT.regMqttPub(jmriObj.LIGHTS, "ML100", "MyLightTopic", {"ON":"LIGHT ON", "OFF":"LIGHT OFF"}):
    print("Light MQTT emmit registration failed")
    result = result | MQTT_FAIL
if RPC_CLIENT.regMqttPub(jmriObj.TURNOUTS, "MT100", "MyTurnoutTopic", {"THROWN":"MT100 IS THROWN", "CLOSED":"MT100 IS CLOSED"}):
    print("Light MQTT turnout registration failed")
    result = result | MQTT_FAIL
if RPC_CLIENT.regMqttPub(jmriObj.SENSORS , "MSMS100", "MySensorTopic", {"ACTIVE":"MSMS100 IS ACTIVE", "INACTIVE":"MSMS100 IS INACTIVE"}):
    print("Sensor MQTT emmit registration failed")
    result = result | MQTT_FAIL
if RPC_CLIENT.regMqttPub(jmriObj.MEMORIES , "IM100", "MyMemoryTopic", {"*":"*"}):
    print("Memory MQTT emmit registration failed")
    result = result | MQTT_FAIL
if result & MQTT_FAIL:
    print("FAIL - MQTT emmit registration failed")
else:
    print("PASS - MQTT emmit registration passed")


# Call back statechange verification
print("\n\nChecking JMRI state change call backs")
print("\nChecking signal mast state change call backs")
aspects = RPC_CLIENT.getValidStatesBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
MAST_STATE_CHANGES = 0
for aspect in aspects:
    print("Changing aspect to: " + aspect)
    RPC_CLIENT.setStateBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", aspect)
time.sleep(2)
if MAST_STATE_CHANGES != len(aspects):
    print("ERROR - Expected " + str(len(aspects)) + " mast call backs, but got " + str(MAST_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got " + str(len(aspects)) + " mast call backs as expected")

# Checking turnout state change call backs
print("\nChecking turnout state change call backs")
positions = RPC_CLIENT.getValidStatesBySysName(jmriObj.TURNOUTS, "MT100")
TURNOUT_STATE_CHANGES = 0
for position in positions:
    print("Changing position to: " + position)
    RPC_CLIENT.setStateBySysName(jmriObj.TURNOUTS, "MT100", position)
time.sleep(2)
if TURNOUT_STATE_CHANGES != len(positions)*2: #Note, for some reason JMRI generates two callbacks for each state change?
    print("ERROR - Expected " + str(len(positions)) + " turnout call backs, but got " + str(TURNOUT_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got " + str(len(positions)) + " turnout call backs as expected")

# Checking lights state change call backs
print("\nChecking lights state change call backs")
states = RPC_CLIENT.getValidStatesBySysName(jmriObj.LIGHTS, "ML100")
LIGHT_STATE_CHANGES = 0
for state in states:
    print("Changing light state to: " + state)
    RPC_CLIENT.setStateBySysName(jmriObj.LIGHTS, "ML100", state)
time.sleep(2)
if LIGHT_STATE_CHANGES != len(states):
    print("ERROR - Expected " + str(len(states)) + " light call backs, but got " + str(LIGHT_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got " + str(len(states)) + " light call backs as expected")

# Checking sensors state change call backs
print("\nChecking sensors state change call backs")
states = RPC_CLIENT.getValidStatesBySysName(jmriObj.SENSORS, "MSMS100")
SENSOR_STATE_CHANGES = 0
for state in states:
    print("Changing sensor state to: " + state)
    RPC_CLIENT.setStateBySysName(jmriObj.SENSORS, "MSMS100", state)
time.sleep(2)
if SENSOR_STATE_CHANGES != len(states):
    print("ERROR - Expected " + str(len(states)) + " sensor call backs, but got " + str(SENSOR_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got " + str(len(states)) + " sensor call backs as expected")

#time.sleep(150)

#Unregistering events
print("\n\nUnregistering event callbacks")
print("\nUnregestering Mast")
RPC_CLIENT.unRegEventCb(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", mastListener)
aspects = RPC_CLIENT.getValidStatesBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
MAST_STATE_CHANGES = 0
for aspect in aspects:
    print("Changing aspect to: " + aspect)
    RPC_CLIENT.setStateBySysName(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)", aspect)
time.sleep(2)
if MAST_STATE_CHANGES != 0:
    print("ERROR - Expected 0 mast call backs, but got " + str(MAST_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got no mast callbacks")

print("\nUnregestering Turnout")
RPC_CLIENT.unRegEventCb(jmriObj.TURNOUTS, "MT100", turnoutListener)
positions = RPC_CLIENT.getValidStatesBySysName(jmriObj.TURNOUTS, "MT100")
TURNOUT_STATE_CHANGES = 0
for position in positions:
    print("Changing position to: " + position)
    RPC_CLIENT.setStateBySysName(jmriObj.TURNOUTS, "MT100", position)
time.sleep(2)
if TURNOUT_STATE_CHANGES != 0:
    print("ERROR - Expected 0  turnout call backs, but got " + str(TURNOUT_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got no turnout call backs")

print("\nUnregestering Light")
RPC_CLIENT.unRegEventCb(jmriObj.LIGHTS, "ML100", lightListener)
states = RPC_CLIENT.getValidStatesBySysName(jmriObj.LIGHTS, "ML100")
LIGHT_STATE_CHANGES = 0
for state in states:
    print("Changing light state to: " + state)
    RPC_CLIENT.setStateBySysName(jmriObj.LIGHTS, "ML100", state)
time.sleep(2)
if LIGHT_STATE_CHANGES != 0:
    print("ERROR - Expected 0 light call backs, but got " + str(LIGHT_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got no light call backs")

print("\nUnregestering Sensor")
RPC_CLIENT.unRegEventCb(jmriObj.SENSORS, "MSMS100", sensorListener)
states = RPC_CLIENT.getValidStatesBySysName(jmriObj.SENSORS, "MSMS100")
SENSOR_STATE_CHANGES = 0
for state in states:
    print("Changing sensor state to: " + state)
    RPC_CLIENT.setStateBySysName(jmriObj.SENSORS, "MSMS100", state)
time.sleep(2)
if SENSOR_STATE_CHANGES != 0:
    print("ERROR - Expected 0 sensor call backs, but got " + str(SENSOR_STATE_CHANGES))
    result = result | CB_FAIL
else:
    print("PASS - Got no sensor call backs")

# Deleting objects
print("\n\nDeleting objects")
print("\nDelete signal mast - SysName: IF$vsm:Sweden-3HMS:SL-5HL($0100)")
RPC_CLIENT.deleteObject(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)")
if RPC_CLIENT.getObjectConfig(jmriObj.MASTS, "IF$vsm:Sweden-3HMS:SL-5HL($0100)") != None:
    print("Error - mast was not deleted")
    result = result | MAST_FAIL
else:
    print("PASS: Successfully deleted")

print("\nDelete turnout - SysName: MT100")
RPC_CLIENT.deleteObject(jmriObj.TURNOUTS, "MT100")
if RPC_CLIENT.getObjectConfig(jmriObj.TURNOUTS, "MT100") != None:
    print("Error - turnout was not deleted")
    result = result | TURNOUT_FAIL
else:
    print("PASS: Successfully deleted")

print("\nDelete light - SysName: ML100")
RPC_CLIENT.deleteObject(jmriObj.LIGHTS, "ML100")
if RPC_CLIENT.getObjectConfig(jmriObj.LIGHTS, "ML100") != None:
    print("Error - light was not deleted")
    result = result | LIGHT_FAIL
else:
    print("PASS: Successfully deleted")

print("\nDelete sensor - SysName: MSMS100")
RPC_CLIENT.deleteObject(jmriObj.SENSORS, "MSMS100")
if RPC_CLIENT.getObjectConfig(jmriObj.SENSORS, "MSMS100") != None:
    print("Error - sensor was not deleted")
    result = result | SENSOR_FAIL
else:
    print("PASS: Successfully deleted")

print("\nDelete memory - SysName: IM100")
RPC_CLIENT.deleteObject(jmriObj.MEMORIES, "IM100")
if RPC_CLIENT.getObjectConfig(jmriObj.MEMORIES, "IM100") != None:
    print("Error - memory was not deleted")
    result = result | MEMORY_FAIL
else:
    print("PASS: Successfully deleted")

print("\n\nStopping....")
RPC_CLIENT.stop()
print("Stopped")

print("\n\nResult:")
print("=======")
print("Mast: %s" %("FAILED" if result & MAST_FAIL else "PASS"))
print("Turnout: %s" %("FAILED" if result & TURNOUT_FAIL else "PASS"))
print("Light: %s" %("FAILED" if result & LIGHT_FAIL else "PASS"))
print("Sensor: %s" %("FAILED" if result & SENSOR_FAIL else "PASS"))
print("Memory: %s" %("FAILED" if result & MEMORY_FAIL else "PASS"))
print("Calback: %s" %("FAILED" if result & CB_FAIL else "PASS"))
print("MQTT: %s" %("FAILED" if result & MQTT_FAIL else "PASS"))
print("Other: %s" %("FAILED" if result & OTHER_FAIL else "PASS"))
# END <Test bench code> ------------------------------------------------------------------------------------------------------------------------------
