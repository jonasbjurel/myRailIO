#!/bin/python
#################################################################################################################################################
# Copyright (c) 2022 Jonas Bjurel
# jonasbjurel@hotmail.com
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Apache License, Version 2.0
# which accompanies this distribution, and is available at
# http://www.apache.org/licenses/LICENSE-2.0
#################################################################################################################################################
# myRailIO MQTT client.
# A full description of the project can be found here: https://github.com/jonasbjurel/GenericJMRIdecoder/README.md
#################################################################################################################################################
# Todo - see https://github.com/jonasbjurel/GenericJMRIdecoder/issues
#################################################################################################################################################
import os
import sys
import time
import threading
import paho.mqtt.client as pahomqtt
from config import *
import imp
imp.load_source('myTrace', '..\\trace\\trace.py')
from myTrace import *
imp.load_source('rc', '..\\rc\\myRailIORc.py')
from rc import *

# ==============================================================================================================================================
# Class: mqttListener
# Purpose:  Listen and dispatches incomming mqtt mesages
#
# Data structures: None
# ==============================================================================================================================================
class mqtt(pahomqtt.Client):
    def __init__(self, URL, port=1883, keepalive=60, onConnectCb=None, onDisconnectCb=None, clientId="myRailIO"):
        trace.notify(DEBUG_INFO, "Starting and connecting MQTT client: " + clientId + " towards MQTT end-point " + URL + ":" + str(port))
        self.active = True
        self.retry = False
        self.retries = 0
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
        try:
            self.connect(self.URL, self.port, self.keepalive)
        except:
            trace.notify(DEBUG_ERROR, "MQTT client: " + clientId + " could not connect to broker")
        self.loop_start()
        trace.notify(DEBUG_INFO, "MQTT client: " + clientId + " started")

    def restart(self, URL, port=1883, keepalive=60, onConnectCb=None, onDisconnectCb=None, clientId=None):
        self.URL = URL
        self.port = port
        self.keepalive = keepalive
        if onConnectCb: self.onConnectCb = onConnectCb
        if onDisconnectCb: self.onDisconnectCb = onDisconnectCb
        if clientId == None: 
            self.clientId = clientId
            self.reinitialise(self.clientId)
        trace.notify(DEBUG_INFO, "Restarting MQTT client: " + self.clientId + " towards MQTT end-point " + URL + ":" + str(self.port))
        self.active = False
        self._stopRetryConnect()
        self.disconnect()
        while self.connected:
            time.sleep(0.1)
        self.loop_stop()
        self.active = True
        try:
            self.connect(self.URL, self.port, self.keepalive)
        except:
            trace.notify(DEBUG_ERROR, "MQTT client: " + self.clientId + " could not connect to broker")
            self.__on_connect(None, None, None, pahomqtt.MQTT_ERR_NO_CONN)
        self.loop_start()
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
            self._retryConnect()
        else:
            trace.notify(DEBUG_INFO, "MQTT client: " + self.clientId + " connected to: " + " MQTT end-point " + self.URL + ":" + str(self.port))
            self.connected = True
            for topic in self.subscriptions:
                self.unsubscribe(topic)
                self.subscribe(topic, qos=0)

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
            cb(message.topic, message.payload.decode("utf-8")) # EXCEPT FOR BAD PAYLOADS

    def _retryConnect(self):
        if not self.retry:
            self.retry = True
            threading.Timer(MQTT_RETRY_PERIOD_S, self._retryConnectLoop).start()

    def _stopRetryConnect(self):
        self.retry = False
        self.retries = 0

    def _retryConnectLoop(self):
        self.retries += 1
        if self.retry:
            if not self.connected:
                if self.retries < 2:
                    try:
                        self.connect(self.URL, self.port, self.keepalive)
                    except:
                        trace.notify(DEBUG_ERROR, "MQTT client: " + self.clientId + " could not connect to broker, retrying...")
                        self.__on_connect(None, None, None, pahomqtt.MQTT_ERR_NO_CONN)
                else:
                    trace.notify(DEBUG_INFO, "Waiting for MQTT client: " + self.clientId + " to connect...")
                threading.Timer(MQTT_RETRY_PERIOD_S, self._retryConnectLoop).start()
            else:
                self.retry = False
                self.retries = 0

    def __delete__(self):
        self.active = False
        self._stopRetryConnect()
        self.loop_stop()



#################################################################################################################################################
# Class: syncMqttRequest
# Purpose: syncMqttRequest is a synchronous MQTT Request/Response class that makes a blocking MQTT request and returns the MQTT response
#          a timeout can be provided which will cause the callback to return with None
# 
# Public Methods and objects to be used by the decoder producers:
# =============================================================
# Public data-structures:
# -----------------------
# -
#
# Public methods:
# ---------------
# -__init__() -> None                        Constructor - defining the request-
#                                            and response topics, the MQTT client object,
#                                            and the timeout. Multiple requests can be
#                                            made to the MQTT topic endpoint using this
#                                            class object
# -__delete__() -> None                      Destructor
# -sendRequest() -> Str | None               Send a request to the MQTT receiver of
#                                            the topic and return the resonse, in case 
#                                            timeout, or object distruction -None is returned.
# 
# Private Methods and objects only to be used internally or by the decoderHandler server:
# =====================================================================================
# Private data-structures:
# ------------------------
# -_requestTopic : str:                     Request topic
# -_requestPayload : str:                   Request payload
# -_responseTopic : str:                    Response topic
# -_mqttClient : mqtt:                      MQTT client object handle
# -_timeout : int:                          Timeout request timeout in ms, if set to 0 it will block indefenetly for an answer.
# -_responsePayload : str:                  Response payload
# -_syncRequestSemaphore : threading.Semaphore: Synchronizing semaphore between requests
#                                               and responses.
# -_syncRequestReentranceLock : threading.Lock: Reentrance lock for requests, only one can
#                                               be queued at a time, if lock cannot be taken
#                                               None will immediately be returned.
# -_delete : bool:                          Destructor flag, indicating that all operations 
#                                           should be ceased, and calls should be returned
#                                           with None.
#
# Private methods:
# ----------------
# -_getResponse() -> None                    Callback for response messages
#################################################################################################################################################
class syncMqttRequest():
    def __init__(self, requestTopic : str, requestPayload : str, responseTopic : str, mqttClient : mqtt, timeout : int):
        trace.notify(DEBUG_INFO, "An MQTT Sync request template is being created, requestTopic: " + requestTopic + ", requestPayload: " + requestPayload + ", responseTopic" + responseTopic + ", timeout: " + str(timeout))
        self._requestTopic : str = requestTopic
        self._requestPayload : str = requestPayload
        self._responseTopic : str = responseTopic
        self._mqttClient : mqtt = mqttClient
        if timeout <= 0:
            self._timeout = -1
        else:
            self._timeout : int = timeout
        self._responsePayload : str = None
        #self._syncRequestSemaphore : threading.Semaphore = threading.Semaphore(1)
        self._syncRequestSemaphore : threading.Lock = threading.Lock()
        self._syncRequestSemaphore.acquire()
        self._mqttClient.subscribeTopic(self._responseTopic, self._getResponse)
        self._syncRequestReentranceLock : threading.Lock = threading.Lock()
        self._delete : bool = False

    def __delete__(self):
        trace.notify(DEBUG_VERBOSE, "The MQTT Sync request template is being deleted for requestTopic: " + self._requestTopic + ", requestPayload: " + self._requestPayload + ", responseTopic" + self._responseTopic + ", timeout: " + str(self._timeout))
        self.mqttClient.unSubscribeTopic(self._responseTopic, self._getResponse)
        self._delete = True
        try:
            self._syncRequestSemaphore.release()
        except:
            pass

    def sendRequest(self) -> str | None:
        if not self._syncRequestReentranceLock.acquire(blocking = False):
            return None
        self._responsePayload = None
        self._mqttClient.publish(self._requestTopic, self._requestPayload)
        if not self._syncRequestSemaphore.acquire(timeout=self._timeout / 1000):
            trace.notify(DEBUG_ERROR, "MQTT sync request timed out for requestTopic: " + self._requestTopic + ", requestPayload: " + self._requestPayload + ", responseTopic" + self._responseTopic + ", timeout: " + str(self._timeout))
            self._syncRequestReentranceLock.release()
            return None
        if self._delete:
            return None
        self._syncRequestReentranceLock.release()
        trace.notify(DEBUG_ERROR, "MQTT sync request received a response: " + self._responsePayload + " for requestTopic: " + self._requestTopic + ", requestPayload: " + self._requestPayload + ", responseTopic" + self._responseTopic + ", timeout: " + str(self._timeout))
        return self._responsePayload

    def _getResponse(self, responseTopic : str, responsePayload : str) -> None :
        if not self._syncRequestReentranceLock.locked():
            return                                                                  # Ignore response if no one is still asking for it
        self._responsePayload = responsePayload
        self._syncRequestSemaphore.release()

#################################################################################################################################################
# End Class: syncMqttRequest
#################################################################################################################################################
