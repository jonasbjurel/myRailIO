#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# Provides a set of commonly used and pre-defined genJMRI MQTT topics and payloads
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
################################################################################################################################################## Southbound Decoder MQTT Topics
# Decoder channel topics
# -----------------------
MQTT_JMRI_PRE_TOPIC =                       "/trains/"


MQTT_DECODER_DISCOVERY_REQUEST_TOPIC =      "track/discoveryreq"                                # Discovery request from decoders
MQTT_DECODER_DISCOVERY_RESPONSE_TOPIC =     "track/discoveryresp"                               # Discovery response from server (aka topDecoder)
MQTT_LOG_TOPIC =                            "track/log/"                                        # Logs sent from decoders
MQTT_DECODER_CONFIGREQ_TOPIC =              "track/decoderconfigreq/"
MQTT_DECODER_CONFIG_TOPIC =                 "track/decoderconfig/"

MQTT_OPSTATE_TOPIC =                        "opState/"                                          # Decoder Operational state sent to decoders
MQTT_ADMSTATE_TOPIC =                       "admState/"                                         # Decoder Operational state sent to decoders

MQTT_TOPDECODER_TOPIC =                     "track/topdecoder/"
MQTT_DECODER_TOPIC =                        "track/decoder/"
MQTT_LGLINK_TOPIC =                         "track/lglink/"
MQTT_LG_TOPIC =                             "track/lightgroup/"

MQTT_SATLINK_TOPIC =                        "track/sateliteLink/"
MQTT_SAT_TOPIC =                            "track/satelite/"
MQTT_ASPECT_TOPIC =                         "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC =                        "track/turnout/"
MQTT_LIGHT_TOPIC =                          "track/light/"
MQTT_LIGHTGROUP_TOPIC =                     "track/lightgroup/"
MQTT_SENS_TOPIC =                           "track/sensor/"
MQTT_MEMORY_TOPIC =                         "track/memory/"
MQTT_STATE_TOPIC =                          "state/"
MQTT_STATS_TOPIC =                          "statistics/"
MQTT_DISCOVERY_REQ =                        "track/discoveryreq"
MQTT_DISCOVERY_RSEP =                       "track/discoveryresp"
MQTT_SUPERVISION_UPSTREAM =                 "track/decodersupervision/upstream/"
MQTT_SUPERVISION_DOWNSTREAM =               "track/decodersupervision/downstream/"

# Southbound Decoder MQTT Payload
# -------------------------------
DISCOVERY = "<DISCOVERY_REQUEST/>"
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
FAULT_ASPECT = "<Aspect>FAULT</Aspect>"
NOFAULT_ASPECT = "<Aspect>NOFAULT</Aspect>"
PING =                                      "<PING/>"

ADM_ON_LINE_PAYLOAD =                               "<ONLINE/>"
ADM_OFF_LINE_PAYLOAD =                              "<OFFLINE/>"
OP_AVAIL_PAYLOAD =                                  "<AVAILABLE/>"
OP_UNAVAIL_PAYLOAD =                                "<UNAVAILABLE/>"

# Northbound MQTT API topics
# --------------------------

