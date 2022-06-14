#!/bin/python
#################################################################################################################################################
# Copyright (c) 2021 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# A common gen JMRI server trace/log module test bench.
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################

#################################################################################################################################################
# Module/Library dependance
#################################################################################################################################################
from trace import *

#################################################################################################################################################
# Test bech code
#################################################################################################################################################
class foo():
    @staticmethod
    def bar():
        trace.notify(DEBUG_VERBOSE, "FooBar")

def myNestedPanicLogRoutine():
    trace.notify(DEBUG_VERBOSE, "This is a verbose debug message which shouldnt be seen")
    trace.notify(DEBUG_PANIC, "This is a PANIC message")

def myLogRoutine():
    trace.notify(DEBUG_VERBOSE, "This is a verbose debug message")
    trace.notify(DEBUG_TERSE, "This is a terse debug message")
    trace.notify(DEBUG_INFO, "This is a notification")
    trace.notify(DEBUG_ERROR, "This is an ERROR message")
    trace.setGlobalDebugLevel(DEBUG_INFO)
    myNestedPanicLogRoutine()

def myRSysLogEmitter(unifiedTime, callerFunFrame, severity, notification, defaultFormatNotification, termination):
    print("\n\nSending Syslog message with artifacts:\ntime: %s, funFrame: %s, severity: %s, notification: %s, \nDefault Notification: %s, Termination: %s\n\n" % (unifiedTime, str(callerFunFrame), trace.getSeverityStr(severity), notification, defaultFormatNotification, str(termination)))

trace.start()
trace.regNotificationCb(myRSysLogEmitter)
trace.setFunDebugLevel(foo.bar, DEBUG_VERBOSE)
foo.bar()
myLogRoutine()
