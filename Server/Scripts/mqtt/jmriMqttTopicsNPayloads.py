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


MQTT_DECODER_DISCOVERY_REQUEST_TOPIC = "track/discoveryreq/"                            # Discovery request from decoders
MQTT_DECODER_DISCOVERY_RESPONSE_TOPIC = "track/discoveryres/"                           # Discovery response from server (aka topDecoder)
MQTT_DECODER_SUPERVISION_UPSTREAM_TOPIC = "track/decoderSupervision/upstream/"          # Ping/super
MQTT_DECODER_SUPERVISION_DOWNSTREAM_TOPIC = "track/decoderSupervision/downstream/"
MQTT_DECODER_CONFIG_TOPIC = "track/decoderMgmt/"                                        # Decoder configuration topic sent from  server (aka topDecoder)
MQTT_LOG_TOPIC = "track/log/"                                                           # Logs sent from decoders

MQTT_OPSTATE_TOPIC =    "opState/"                                                         # Decoder Operational state sent to decoders
MQTT_TOPDECODER_TOPIC =    "track/topdecoder/"
MQTT_DECODER_TOPIC =    "track/decoder/"
MQTT_LGLINK_TOPIC =     "track/lglink/"
MQTT_LG_TOPIC =         "track/lightgroup/"

MQTT_SATLINK_TOPIC =    "track/sateliteLink/"
MQTT_SAT_TOPIC =        "track/satelite/"
MQTT_ASPECT_TOPIC = "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC = "track/turnout/"
MQTT_LIGHT_TOPIC = "track/light/"
MQTT_SENS_TOPIC = "track/sensor/"
MQTT_MEMORY_TOPIC = "track/memory/"
MQTT_STATE_TOPIC = "state/"
MQTT_STATS_TOPIC = "statistics/"



# Southbound Decoder MQTT Payload
# -------------------------------
DISCOVERY = "<DISCOVERY_REQUEST/>"
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
FAULT_ASPECT = "<Aspect>FAULT</Aspect>"
NOFAULT_ASPECT = "<Aspect>NOFAULT</Aspect>"
PING = "<Ping/>"
ADM_BLOCK = "<AdmState>BLOCK</AdmState>"
ADM_DEBLOCK = "<AdmState>DEBLOCK</AdmState>"
PANIC_MSG = "<PANIC/>"
NOPANIC_MSG = "<NOPANIC/>"
ON_LINE = "<ONLINE/>"
OFF_LINE = "<OFFLINE/>"

# Northbound MQTT API topics
# --------------------------
MQTT_NB_API_REQ = "genJMRIDecoder/NbAPI/req/"
MQTT_NB_API_RESP = "genJMRIDecoder/NbAPI/resp/"
MQTT_NB_API_ALERT = "genJMRIDecoder/NbAPI/alert/"
