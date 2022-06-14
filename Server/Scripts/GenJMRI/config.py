#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# This module provides the genJMRI non alterable configurations, such as default parameters, tunable default parameters, and System hardcoded 
# parameters - not alterable without implementation design
#
# See readme.md and and architecture.md for installation-, configuration-, and architecture descriptions
# A full project description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
#################################################################################################################################################

# Default system parameters - defaults if nothing else is given in configuration
DEFAULT_DECODER_KEEPALIVE_PERIOD =      10.0
DEFAULT_DECODER_MQTT_URI =              "broker.emqx.io"
DEFAULT_DECODER_MQTT_PORT =             1883
DEFAULT_DECODER_MQTT_TOPIC_PREFIX =     "/trains/"
DEFAULT_JMRI_RPC_URI =                  "127.0.0.2"
DEFAULT_JMRI_RPC_PORT_BASE =            8000
DEFAULT_JMRI_RPC_KEEPALIVE_PERIOD =     10.0
DEFAULT_LOG_VERBOSITY =                 "DEBUG-INFO"
DEFAULT_NTP_SERVER =                    ["pool.ntp.org"]
DEFAULT_SNMP_SERVER =                   ["my.snmp.org"]
DEFAULT_RSYSLOG_SERVER =                "my.RsysLogServer.org"
DEFAULT_TRACK_FAILSAFE =                True
DEFAULT_GIT_BRANCH =                    "Main"
DEFAULT_GIT_TAG =                       "-"

# Tunable default parameters - can be changed - but not exposed as managed objects
SATLINK_CRC_ES_HIGH_TRESH =             10
SATLINK_SYMERR_ES_HIGH_TRESH =          10
SATLINK_SIZEERR_ES_HIGH_TRESH =         10
SATLINK_WD_ES_HIGH_TRESH =              1
SATLINK_CRC_ES_LOW_TRESH =              5
SATLINK_SYMERR_ES_LOW_TRESH =           5
SATLINK_SIZEERR_ES_LOW_TRESH =          5
SATLINK_WD_ES_LOW_TRESH =               0
SAT_CRC_ES_HIGH_TRESH =                 10
SAT_WD_ES_HIGH_TRESH =                  1
SAT_CRC_ES_LOW_TRESH =                  5
SAT_WD_ES_LOW_TRESH =                   0

# System hardcoded parameters - not alterable without implementation design, or verification
DECODER_MAX_LG_LINKS =                  2
DECODER_MAX_SAT_LINKS =                 2
SATLINK_MAX_SATS =                      16
SAT_MAX_SENS_PORTS =                    8
SAT_MAX_ACTS_PORTS =                    4
LG_LINK_MAX_LIGHTGROUPS =               16
