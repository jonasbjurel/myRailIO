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
MQTT_DECODER_CONFIGREQ_TOPIC =              "track/decoder/configReq/"
MQTT_DECODER_CONFIG_TOPIC =                 "track/decoder/configResp/"

MQTT_OPSTATE_TOPIC =                        "opState/"                                          # Decoder Operational state sent to decoders
MQTT_ADMSTATE_TOPIC =                       "admState/"                                         # Decoder Operational state sent to decoders

MQTT_TOPDECODER_TOPIC =                     "track/topdecoder/"
MQTT_DECODER_TOPIC =                        "track/decoder/"
MQTT_LGLINK_TOPIC =                         "track/lglink/"
MQTT_LG_TOPIC =                             "track/lightgroup/"

MQTT_SATLINK_TOPIC =                        "track/satLink/"
MQTT_SAT_TOPIC =                            "track/satelite/"
MQTT_ASPECT_TOPIC =                         "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC =                        "track/turnout/"
MQTT_LIGHT_TOPIC =                          "track/light/"
MQTT_LIGHTGROUP_TOPIC =                     "track/lightgroup/"
MQTT_LIGHTGROUPREQ_TOPIC =                  "track/lightgroup/req/"
MQTT_SENS_TOPIC =                           "track/sensor/"
MQTT_MEMORY_TOPIC =                         "track/memory/"
MQTT_STATE_TOPIC =                          "state/" #DUPLICATE?
MQTT_STATS_TOPIC =                          "statistics/"
MQTT_DISCOVERY_REQ =                        "track/discoveryreq"
MQTT_DISCOVERY_RSEP =                       "track/discoveryresp"
MQTT_SUPERVISION_UPSTREAM =                 "track/decodersupervision/upstream/"
MQTT_SUPERVISION_DOWNSTREAM =               "track/decodersupervision/downstream/"

# Northbound Decoder MQTT Payload           //Need to fix what is NB and SB
# -------------------------------
DISCOVERY =                                 "<DISCOVERY_REQUEST/>"
MQTT_DECODER_CONFIGREQ_PAYLOAD =            "<CONFIGURATION_REQUEST/>"
DECODER_UP =                                "<OPSTATE>ONLINE</OPSTATE>" #DUPLICATE?
DECODER_DOWN =                              "<OPSTATE>OFFLINE</OPSTATE>" #DUPLICATE?
FAULT_ASPECT =                              "<ASPECT>FAULT</ASPECT>"
NOFAULT_ASPECT =                            "<ASPECT>NOFAULT</ASPECT>"
PING =                                      "<PING/>"
GET_LG_ASPECT =                             "<REQUEST>GETLGASPECT</REQUEST>"
ADM_ON_LINE_PAYLOAD =                       "<ADMSTATE>ONLINE</ADMSTATE>"
ADM_OFF_LINE_PAYLOAD =                      "<ADMSTATE>OFFLINE</ADMSTATE>"
OP_AVAIL_PAYLOAD =                          "<OPSTATE>AVAILABLE</OPSTATE>"
OP_UNAVAIL_PAYLOAD =                        "<OPSTATE>UNAVAILABLE</OPSTATE>"



# Northbound MQTT API topics
# --------------------------

