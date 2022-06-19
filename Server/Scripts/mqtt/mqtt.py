#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# genJMRI MQTT client.
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
import os
import sys
import time
import paho.mqtt.client as pahomqtt
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\genJMRIRc.py')
from rc import *

# ==============================================================================================================================================
# Class: mqttListener
# Purpose:  Listen and dispatches incomming mqtt mesages
#
# Data structures: None
# ==============================================================================================================================================
class mqtt(pahomqtt.Client):
    def __init__(self, URL, port=1883, keepalive=60, onConnectCb=None, onDisconnectCb=None, clientId="genJMRI"):
        trace.notify(DEBUG_INFO, "Starting and connecting MQTT client: " + clientId + " towards MQTT end-point " + URL + ":" + str(port))
        self.active = True
        self.subscriptions = {}
        self.onConnectCb = onConnectCb
        self.onDisconnectCb = onDisconnectCb
        self.clientId = clientId
        self.URL = URL
        self.port = port
        self.keepalive = keepalive
        self.topicPrefix = ""
        super().__init__(client_id=self.clientId)
        self.on_connect = self.__on_connect
        self.on_disconnect = self.__on_disconnect
        self.on_message = self.__on_message
        self.connected = False
        self.connect(self.URL, self.port, self.keepalive)
        self.loop_start()
        trace.notify(DEBUG_INFO, "MQTT client: " + clientId + " started")

    def restart(self, URL, port=1883, keepalive=60, onConnectCb=None, onDisconnectCb=None, clientId=None):
        self.URL = URL
        self.port = port
        self.keepalive = keepalive
        if onConnectCb: self.onConnectCb = onConnectCb
        if onDisconnectCb: self.onDisconnectCb = onDisconnectCb
        if clientId: self.clientId = clientId
        trace.notify(DEBUG_INFO, "Restarting and MQTT client: " + clientId + " towards MQTT end-point " + URL + ":" + str(port))
        self.active = False
        self.disconnect()
        while self.connected:
            time.sleep(0.1)
        self.loop_stop()
        self.active = True
        self.connect(self.URL, self.port, self.keepalive)
        self.loop_start()
        while not self.connected:
            time.sleep(0.1)
        for topic in self.subscriptions:
            self.subscribe(topic, qos=0)
        trace.notify(DEBUG_INFO, "MQTT client restarted")

    def setTopicPrefix(self, topicPrefix):
        self.topicPrefix = topicPrefix

    def subscribeTopic(self, topic, callback):
        try:
            self.subscriptions[topic]
        except:
            self.subscriptions[topic] = [callback]
            self.subscribe(topic, qos=0)
            trace.notify(DEBUG_INFO, "Adding new subscription for topic: " + topic + " with callback: " + str(callback) + " to MQTT client " + str(self.clientId))
        else:
            try:
                self.subscriptions[topic].index(callback)
            except:
                self.subscriptions[topic].append(callback)
                trace.notify(DEBUG_INFO, "Adding callback to existing subscription for topic: " + topic + " with callback: " + str(callback) + " to MQTT client " + str(self.clientId))
            else:
                trace.notify(DEBUG_INFO, "Callback already exists for topic: " + topic + " callback: " + str(callback) + " to MQTT client " + str(self.clientId))
        return rc.OK

    def unsubscribeTopic(self, topic, callback):
        trace.notify(DEBUG_INFO, "Deleting subscription for: " + topic + " with callback: " + str(callback) + " from MQTT client " + str(self.clientId))
        try:
            self.subscriptions[topic]
        except:
            trace.notify(DEBUG_ERROR, "Cannot delete subscription for: " + topic + " with callback: " + str(callback) + " from MQTT client " + str(self.clientId) + "Topic was never subscribed to" )
            return rc.DOES_NOT_EXIST
        removedItem = False
        try:
            while True:
                self.subscriptions[topic].remove(callback)
                removedItem = True
        except:
            pass
        if not removedItem:
            trace.notify(DEBUG_ERROR, "Cannot delete subscription for: " + topic + " with callback: " + str(callback) + " from MQTT client " + str(self.clientId) + ", callback was never registered to this topic" )
            return rc.DOES_NOT_EXIST
        trace.notify(DEBUG_INFO, "Successfully deleted call-back for: " + topic + " with callback: " + str(callback) + " from MQTT client " + str(self.clientId))
        if self.subscriptions[topic] == []:
            ret = self.unsubscribe(topic)
            if ret != pahomqtt.MQTT_ERR_SUCCESS:
                trace.notify(DEBUG_ERROR, "Could not delete subscription of topic: " + topic + " from MQTT client " + str(self.clientId) + ", paho MQTT client reported return code:" + str(rc))
                return rc.GEN_ERR
        return rc.OK

    def __on_connect(self, client, userdata, flags, rc):
        if self.onConnectCb != None:
            self.onConnectCb(self, self.clientId, rc)
        if rc != pahomqtt.MQTT_ERR_SUCCESS:
            trace.notify(DEBUG_ERROR, "MQTT client: " + self.clientId + " could not connect to: " + " MQTT end-point " + self.URL + ":" + str(self.port) + " Error code: " + str(rc))
        else:
            trace.notify(DEBUG_INFO, "MQTT client: " + self.clientId + " connected to: " + " MQTT end-point " + self.URL + ":" + str(self.port))
            self.connected = True

    def __on_disconnect(self, client, userdata, rc):
        self.connected = False
        if self.active:
            if self.onDisconnectCb != None:
                self.onDisconnectCb(self, self.clientId, rc)
            trace.notify(DEBUG_ERROR, "MQTT client: " + self.clientId + " was disconnected from: " + " MQTT end-point " + self.URL + ":" + str(self.port))
        else:
            trace.notify(DEBUG_INFO, "MQTT client: " + self.clientId + " was propperly disconnected from: " + " MQTT end-point " + self.URL + ":" + str(self.port))

    def __on_message(self, client, userdata, message):
        trace.notify(DEBUG_TERSE, "MQTT client: " + self.clientId + " got a message " + message.topic + ":" + str(message.payload))
        try:
            self.subscriptions[message.topic]
        except:
            trace.notify(DEBUG_ERROR, "MQTT client: " + self.clientId + " got a message topic " + message.topic + " despite it was not subscribed to")
            return
        for cb in self.subscriptions[message.topic]:
            trace.notify(DEBUG_VERBOSE, "MQTT client: " + self.clientId + " is calling " + str(cb) + "with MQTT message: " + message.topic + ":" + str(message.payload))
            cb(message.topic, message.payload)

    def __delete__(self):
        self.active = False
        self.loop_stop()
