/*============================================================================================================================================= */
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonasbjurel@hotmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law and agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*==============================================================================================================================================*/
/* END License                                                                                                                                  */
/*==============================================================================================================================================*/

#ifndef MQTT_H
#define MQTT_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <WiFi.h>
#include <QList.h>
#include <PubSubClient.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "systemState.h"
#include "wdt.h"
#include "networking.h"
#include "strHelpers.h"
#include "panic.h"
#include "rc.h"
#include "config.h"
#include "mqttTopics.h"
#include "wdt.h"
#include "taskWrapper.h"
#include "logHelpers.h"

//DEBUG
#include "cpu.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define MQTT_POLL_LATENCY_AVG_TIME_MS       10
#define MQTT_QOS_0                          0
#define MQTT_QOS_1                          1
#define MQTT_RETAIN                         true

typedef void(*mqttSubCallback_t)(const char* topic, const char* payload, const void* args);
typedef void(*mqttStatusCallback_t)(uint8_t p_mqttStatus, const void* args);


struct mqttSub_t {
    char* topic;
    mqttSubCallback_t mqttSubCallback;
    void* mqttCallbackArgs;
};

struct mqttTopic_t {
    char* topic;
    QList<mqttSub_t*>* topicList;
};



class mqtt : public wdt{
public:
    //Public methods
    static void create(void);
    static rc_t init(const char* p_broker, uint16_t p_port, const char* p_user, const char* p_pass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, float p_pingPeriod, bool p_defaultRetain);
    static void regOpStateCb(sysStateCb_t p_systemStateCb, void* p_systemStateCbArgs);
    static void unRegOpStateCb(sysStateCb_t p_systemStateCb);
    static rc_t regStatusCallback(const mqttStatusCallback_t p_statusCallback, const void* args);
    static rc_t reConnect(void);
    static void disConnect(void);
    static rc_t up(void);
    static void down(void);
    static void wdtKicked(void* p_dummy);
    static rc_t subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args);
    static rc_t unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback);
    static rc_t sendMsg(const char* p_topic, const char* p_payload, bool p_retain = defaultRetain);
    static rc_t setDecoderUri(const char* p_decoderUri);
    static const char* getDecoderUri(void);
    static rc_t setBrokerUri(const char* p_brokerUri, bool p_persist = false);
    static const char* getBrokerUri(void);
    static rc_t setBrokerPort(uint16_t p_brokerPort, bool p_persist = false);
    static uint16_t getBrokerPort(void);
    static rc_t setBrokerUser(const char* p_brokerUser);
    static const char* getBrokerUser(void);
    static rc_t setBrokerPass(const char* p_brokerPass);
    static const char* getBrokerPass(void);
    static rc_t setClientId(const char* p_clientId);
    static const char* getClientId(void);
    static rc_t setDefaultQoS(uint8_t p_defaultQoS);
    static uint8_t getDefaultQoS(void);
    static rc_t setKeepAlive(uint8_t p_keepAlive);
    static uint8_t getKeepAlive(void);
    static rc_t setPingPeriod(float p_pingPeriod);
    static float getPingPeriod(void);
    static void setMqttTopicPrefix(const char* p_mqttTopicPrefix);
    static const char* getMqttTopicPrefix(void);
    static uint16_t getOpStateBitmap(void);
    static rc_t getOpStateStr(char* p_opState);
    static uint32_t getOverRuns(void);
    static void clearOverRuns(void);
    static uint32_t getMeanLatency(void);
    static uint32_t getMaxLatency(void);
    static void clearMaxLatency(void);
    static bool getSubs(uint16_t p_index, char* p_topic, uint16_t p_topicSize, char* p_cbs, uint16_t p_cbSize);


    //Public data structures
    //--

private:
    //Private methods
    static void discover(void);
    static void onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy);
    static void poll(void* dummy);
    static void onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length);
    static rc_t reSubscribe(void);
    static void mqttPingTimer(void* dummy);
    static void onMqttPing(const char* p_topic, const char* p_payload, const void* p_dummy);

    //Private data structures
    static TaskHandle_t* supervisionTaskHandle;
    static systemState* sysState;
    static SemaphoreHandle_t mqttLock;
    static WiFiClient espClient;
    static PubSubClient* mqttClient;
    static uint32_t overRuns;
    static uint32_t maxLatency;
    static uint16_t avgSamples;
    static uint32_t* latencyVect;
    static char* decoderUri;
    static char* brokerUri;
    static uint16_t brokerPort;
    static char* brokerUser;
    static char* brokerPass;
    static char* clientId;
    static uint8_t defaultQoS;
    static uint8_t keepAlive;
    static bool opStateTopicSet;
    static char* opStateTopic;
    static char mqttTopicPrefix[50];
    static char* mqttPingUpstreamTopic;
    static char* upPayload;
    static char* downPayload;
    static uint8_t missedPings;
    static int mqttStatus;
    static uint8_t qos;
    static bool defaultRetain;
    static float pingPeriod;
    static bool discovered;
    static bool reSubscribeReq;
    static QList<mqttTopic_t*> mqttTopics;
    static mqttStatusCallback_t statusCallback;
    static void* statusCallbackArgs;
    static wdt* mqttWdt;
    static bool supervision;
    static uint16_t retryCnt;
};

/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/
#endif /*MQTT_H*/
