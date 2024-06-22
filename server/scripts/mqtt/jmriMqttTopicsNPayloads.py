#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# Provides a set of commonly used and pre-defined myRailIO MQTT topics and payloads
# A full description can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/blob/main/README.md
################################################################################################################################################## Southbound Decoder MQTT Topics
# Decoder channel topics
# -----------------------

MQTT_JMRI_PRE_TOPIC =                       "/trains/"

MQTT_RPC_SERVER_MQTT_SUPERVISION_TOPIC =    "track/rpcservermqttsupervision/"                   # Supervision of the RPC server MQTT connectivity
MQTT_DECODER_DISCOVERY_REQUEST_TOPIC =      "track/discoveryreq"                                # Discovery request from decoders
MQTT_DECODER_DISCOVERY_RESPONSE_TOPIC =     "track/discoveryresp"                               # Discovery response from server (aka topDecoder)
MQTT_LOG_TOPIC =                            "track/log/"                                        # Logs sent from decoders
MQTT_DECODER_CONFIGREQ_TOPIC =              "track/decoder/configReq/"                          # Decoder ask for configuration
MQTT_DECODER_CONFIG_TOPIC =                 "track/decoder/configResp/"                         # Configuration response from server

# Southbound Decoder requests
# ---------------------------
MQTT_DECODER_COREDUMPREQ_TOPIC =            "track/decoder/coredumprequest/"
MQTT_DECODER_COREDUMPREQ_PAYLOAD =          "<COREDUMPREQ/>"
MQTT_DECODER_COREDUMPRESP_TOPIC =           "track/decoder/coredumpresponse/"
MQTT_DECODER_COREDUMPRESP_PAYLOAD_TAG =     "COREDUMP"

MQTT_DECODER_SSIDREQ_TOPIC =                "track/decoder/ssidrequest/"
MQTT_DECODER_SSIDREQ_PAYLOAD =              "<SSIDREQ/>"
MQTT_DECODER_SSIDRESP_TOPIC =               "track/decoder/ssidresponse/"
MQTT_DECODER_SSIDRESP_PAYLOAD_TAG =         "SSID"

MQTT_DECODER_SNRREQ_TOPIC =                 "track/decoder/snrrequest/"
MQTT_DECODER_SNRREQ_PAYLOAD =               "<SNRREQ/>"
MQTT_DECODER_SNRRESP_TOPIC =                "track/decoder/snrresponse/"
MQTT_DECODER_SNRRESP_PAYLOAD_TAG =          "SNR"

MQTT_DECODER_IPADDRREQ_TOPIC =              "track/decoder/ipaddrrequest/"
MQTT_DECODER_IPADDRREQ_PAYLOAD =            "<IPADDRREQ/>"
MQTT_DECODER_IPADDRRESP_TOPIC =             "track/decoder/ipaddrresponse/"
MQTT_DECODER_IPADDRRESP_PAYLOAD_TAG =       "IPADDR"

MQTT_DECODER_MEMSTATREQ_TOPIC =             "track/decoder/memstatrequest/"
MQTT_DECODER_MEMSTATREQ_PAYLOAD =           "<MEMSTATREQ/>"
MQTT_DECODER_MEMSTATRESP_TOPIC =            "track/decoder/memstatresponse/"
MQTT_DECODER_MEMSTATRESP_PAYLOAD_TAG =      "MEMSTAT"

MQTT_DECODER_CPUSTATREQ_TOPIC =             "track/decoder/cpustatrequest/"
MQTT_DECODER_CPUSTATREQ_PAYLOAD =           "<CPUSTATREQ/>"
MQTT_DECODER_CPUSTATRESP_TOPIC =            "track/decoder/cpustatresponse/"
MQTT_DECODER_CPUSTATRESP_PAYLOAD_TAG =      "CPUSTAT"

MQTT_DECODER_BROKERURIREQ_TOPIC =           "track/decoder/brokerurirequest/"
MQTT_DECODER_BROKERURIREQ_PAYLOAD =         "<BROKERURIREQ/>"
MQTT_DECODER_BROKERURIRESP_TOPIC =          "track/decoder/brokeruriresponse/"
MQTT_DECODER_BROKERURIRESP_PAYLOAD_TAG =    "BROKERURI"

MQTT_DECODER_HWVERREQ_TOPIC =               "track/decoder/hwverrequest/"
MQTT_DECODER_HWVERREQ_PAYLOAD =             "<HWVERREQ/>"
MQTT_DECODER_HWVERRESP_TOPIC =              "track/decoder/hwverresponse/"
MQTT_DECODER_HWVERRESP_PAYLOAD_TAG =        "HWVER"

MQTT_DECODER_SWVERREQ_TOPIC =               "track/decoder/swverrequest/"
MQTT_DECODER_SWVERREQ_PAYLOAD =             "<SWVERREQ/>"
MQTT_DECODER_SWVERRESP_TOPIC =              "track/decoder/swverresponse/"
MQTT_DECODER_SWVERRESP_PAYLOAD_TAG =        "SWVER"

MQTT_DECODER_LOGLVLREQ_TOPIC =              "track/decoder/loglvlrequest/"
MQTT_DECODER_LOGLVLREQ_PAYLOAD =            "<LOGLVL/>"
MQTT_DECODER_LOGLVLRESP_TOPIC =             "track/decoder/loglvlresponse/"
MQTT_DECODER_LOGLVLRESP_PAYLOAD_TAG =       "LOGLVL"

MQTT_DECODER_WWWUIREQ_TOPIC =               "track/decoder/wwwuirequest/"
MQTT_DECODER_WWWUIREQ_PAYLOAD =             "<WWWUI/>"
MQTT_DECODER_WWWUIRESP_TOPIC =              "track/decoder/wwwuiresponse/"
MQTT_DECODER_WWWUIRESP_PAYLOAD_TAG =        "WWWUI"


MQTT_DECODER_OPSTATEREQ_TOPIC =             "track/decoder/opstaterequest/"
MQTT_DECODER_OPSTATEREQ_PAYLOAD =           "<OPSTATE/>"
MQTT_DECODER_OPSTATERESP_TOPIC =            "track/decoder/opstateresponse/"
MQTT_DECODER_OPSTATERESP_PAYLOAD_TAG =      "OPSTATE"

# Northbound decoder notifications
# --------------------------------
MQTT_DECODER_WIFISTATUS_TOPIC =             "track/decoder/wifistatus/"
MQTT_DECODER_WIFISTATUS_PAYLOAD_TAG =       "WIFISTATUS"

MQTT_DECODER_MEMSTATUS_TOPIC =              "track/decoder/memstatus/"
MQTT_DECODER_MEMSTATUS_PAYLOAD_TAG =        "MEMSTATUS"

MQTT_DECODER_LOGOVERLOAD_TOPIC =            "track/decoder/logoverload/"
MQTT_DECODER_LOGOVERLOAD_PAYLOAD_TAG =      "LOGOVERLOAD"

MQTT_DECODER_CLIACCESS_TOPIC =              "track/decoder/cliaccess/"
MQTT_DECODER_CLIACCESS_PAYLOAD_TAG =        "CLIENT"

MQTT_DECODER_DEBUG_TOPIC =                  "track/decoder/debug/"
MQTT_DECODER_DEBUG_PAYLOAD_TAG =            "DEBUG"

MQTT_DECODER_NTPSTATUS_TOPIC =              "track/decoder/ntpstatus/"
MQTT_DECODER_NTPSTATUS_PAYLOAD_TAG =            "NTPSTATUS"

# bothway opStates
# ----------------
MQTT_OPSTATE_TOPIC_DOWNSTREAM =             "opState/downstream/"                               # Decoder Operational state sent to decoders
MQTT_OPSTATE_TOPIC_UPSTREAM =               "opState/upstream/"                                 # Decoder Operational state received from decoders
MQTT_ADMSTATE_TOPIC_DOWNSTREAM =            "admState/downstream/"                              # Decoder Operational state sent to decoders

MQTT_TOPDECODER_TOPIC =                     "track/topdecoder/"                                 # Server top decode object topic
MQTT_DECODER_TOPIC =                        "track/decoder/"                                    # Server/Client decoder object topic
MQTT_LGLINK_TOPIC =                         "track/lglink/"                                     # Server/Client lgLink object topic
MQTT_LG_TOPIC =                             "track/lightgroup/"                                 # Server/Client lg object topic
MQTT_SATLINK_TOPIC =                        "track/satLink/"
MQTT_SAT_TOPIC =                            "track/satelite/"
MQTT_ACT_TOPIC =                            "track/actuator/"
MQTT_SENS_TOPIC =                           "track/sensor/"
MQTT_ASPECT_TOPIC =                         "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC =                        "track/turnout/"
MQTT_LIGHT_TOPIC =                          "track/light/"
MQTT_LIGHTGROUP_TOPIC =                     "track/lightgroup/" #DUPLICATE?
MQTT_LIGHTGROUPREQ_TOPIC =                  "track/lightgroup/req/" #?
MQTT_SENS_TOPIC =                           "track/sensor/"
MQTT_MEMORY_TOPIC =                         "track/memory/"
MQTT_STATE_TOPIC =                          "state/"
MQTT_STATS_TOPIC =                          "statistics/"
MQTT_SUPERVISION_UPSTREAM =                 "track/decodersupervision/upstream/"
MQTT_SUPERVISION_DOWNSTREAM =               "track/decodersupervision/downstream/"
MQTT_REBOOT_TOPIC =                         "reboot/"

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
REBOOT_PAYLOAD =                            "<REBOOT/>"



# Northbound MQTT API topics
# --------------------------

