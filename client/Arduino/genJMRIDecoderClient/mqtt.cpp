/*==============================================================================================================================================*/
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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "mqtt.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
EXT_RAM_ATTR systemState* mqtt::sysState = NULL;
EXT_RAM_ATTR job* mqtt::mqttJobHandle = NULL;
EXT_RAM_ATTR SemaphoreHandle_t mqtt::mqttLock = NULL;
EXT_RAM_ATTR SemaphoreHandle_t mqtt::pubSubLock = NULL;
EXT_RAM_ATTR WiFiClient mqtt::espClient;
EXT_RAM_ATTR PubSubClient* mqtt::mqttClient;
EXT_RAM_ATTR uint32_t mqtt::overRuns = 0;
EXT_RAM_ATTR uint32_t mqtt::maxLatency = 0;
EXT_RAM_ATTR uint16_t mqtt::avgSamples = 0;
EXT_RAM_ATTR uint32_t* mqtt::latencyVect = NULL;
EXT_RAM_ATTR char* mqtt::decoderUri = NULL;
EXT_RAM_ATTR char* mqtt::brokerUri = NULL;
EXT_RAM_ATTR uint16_t mqtt::brokerPort = 0;
EXT_RAM_ATTR char* mqtt::brokerUser = NULL;
EXT_RAM_ATTR char* mqtt::brokerPass = NULL;
EXT_RAM_ATTR char* mqtt::clientId = NULL;
EXT_RAM_ATTR uint8_t mqtt::defaultQoS = 0;
EXT_RAM_ATTR uint8_t mqtt::keepAlive = 0;
EXT_RAM_ATTR bool mqtt::opStateTopicSet = false;
EXT_RAM_ATTR char* mqtt::opStateTopic = NULL;
EXT_RAM_ATTR char mqtt::mqttTopicPrefix[50];
EXT_RAM_ATTR char* mqtt::upPayload = NULL;
EXT_RAM_ATTR char* mqtt::downPayload = NULL;
EXT_RAM_ATTR char* mqtt::mqttPingUpstreamTopic = NULL;
EXT_RAM_ATTR uint8_t mqtt::missedPings = 0;
EXT_RAM_ATTR int mqtt::mqttStatus;
EXT_RAM_ATTR uint8_t mqtt::qos = 0;
EXT_RAM_ATTR bool mqtt::defaultRetain = false;
EXT_RAM_ATTR float mqtt::pingPeriod = 0 ;
EXT_RAM_ATTR bool mqtt::discovered = false;
EXT_RAM_ATTR bool mqtt::reSubscribeReq = false;
EXT_RAM_ATTR QList<mqttTopic_t*> mqtt::mqttTopics;
EXT_RAM_ATTR mqttStatusCallback_t mqtt::statusCallback = NULL;
EXT_RAM_ATTR void* mqtt::statusCallbackArgs = NULL;
EXT_RAM_ATTR wdt* mqtt::mqttWdt = NULL;
EXT_RAM_ATTR bool mqtt::supervision = false;
EXT_RAM_ATTR uint16_t mqtt::retryCnt = 0;



// Public methods
void mqtt::create(void) {
    LOG_INFO_NOFMT("Creating MQTT client" CR);
    sysState = new (heap_caps_malloc(sizeof(systemState), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) systemState(NULL);
    if (!sysState)
        panic("systemState could not be created");
    sysState->setSysStateObjName("mqtt");
    sysState->setOpStateByBitmap(OP_INIT | OP_DISABLED | OP_DISCONNECTED | OP_UNDISCOVERED | OP_CLIEUNAVAILABLE);
    mqttJobHandle = new (heap_caps_malloc(sizeof(job), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) job(MQTT_JOB_SLOTS, CPU_JOB_MQTT_TASKNAME, CPU_JOB_MQTT_STACKSIZE_1K * 1024, CPU_JOB_MQTT_PRIO, false, WDT_JOB_MQTT_TIMEOUT_MS);
    if (!mqttJobHandle)
        panic("MQTT job could not be created");
    mqttJobHandle->setOverloadLevelCease(MQTT_JOB_SLOTS / 2);
    mqttJobHandle->regOverloadCb(onMqttJobOverload, NULL);
    mqttLock = xSemaphoreCreateMutex();
    pubSubLock = xSemaphoreCreateMutex();
    if (!mqttLock || !pubSubLock)
        panic("MQTT Lock could not be created");
}

rc_t mqtt::init(const char* p_brokerUri, uint16_t p_brokerPort, const char* p_brokerUser, const char* p_brokerPass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, float p_pingPeriod, bool p_defaultRetain) {
    LOG_INFO("Initializing and starting MQTT client, BrokerURI: %s, BrokerPort: %i, User: %s, Password: %s, ClientId: %s" CR, p_brokerUri, p_brokerPort, p_brokerUser, p_brokerPass, p_clientId);
    mqttWdt = new (heap_caps_malloc(sizeof(wdt), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wdt(WDT_MQTT_POLL_LOOP_TIMEOUT_MS, "mqttPollLoop", FAULTACTION_GLOBAL_FAILSAFE | FAULTACTION_GLOBAL_REBOOT | FAULTACTION_ESCALATE_INTERGAP);
    if (!mqttWdt)
        panic("Failed to create MQTT Watchdog");
    missedPings = 0;
    opStateTopicSet = false;
    discovered = false;
    brokerUri = createNcpystr(p_brokerUri);
    brokerPort = p_brokerPort;
    if (p_brokerUser != NULL)
        brokerUser = createNcpystr(p_brokerUser);
    if (p_brokerPass != NULL)
        brokerPass = createNcpystr(p_brokerPass);
    clientId = createNcpystr(p_clientId);
    defaultQoS = p_defaultQoS;
    keepAlive = p_keepAlive;
    pingPeriod = p_pingPeriod;
    defaultRetain = p_defaultRetain;
    strcpy(mqttTopicPrefix, MQTT_PRE0_TOPIC_DEFAULT_FRAGMENT);
    avgSamples = int(MQTT_POLL_LATENCY_AVG_TIME_MS * 1000 / MQTT_POLL_PERIOD_MS);
    latencyVect = new (heap_caps_malloc(sizeof(uint32_t) * avgSamples, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint32_t[avgSamples];
    for (uint16_t i = 0; i < avgSamples; i++)
        latencyVect[i] = 0;
    mqttClient = new (heap_caps_malloc(sizeof(PubSubClient) * avgSamples, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) PubSubClient(espClient);
    mqttClient->setServer(brokerUri, brokerPort);
    mqttClient->setKeepAlive(keepAlive);
    mqttStatus = mqttClient->state();
    if (!mqttClient->setBufferSize(MQTT_BUFF_SIZE)) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("Could not allocate MQTT buffers");
        return RC_OUT_OF_MEM_ERR;
    }
    mqttClient->setCallback(&onMqttMsg);
    LOG_INFO("Spawning MQTT poll task" CR);
    if (!eTaskCreate(poll,                                          // Task function
        CPU_MQTT_POLL_TASKNAME,                                     // Task function name reference
        CPU_MQTT_POLL_STACKSIZE_1K * 1024,                          // Stack size
        NULL,                                                       // Parameter passing
        CPU_MQTT_POLL_PRIO,                                         // Priority 0-24, higher is more
        INTERNAL)) {                                                // Task stack attribute
        panic("Could not start the MQTT poll task");
        return RC_OUT_OF_MEM_ERR;
    }
    mqttWdt->activate();
    LOG_INFO_NOFMT("Waiting for MQTT client to connect (I.e. opState ~OP_DISCONNECTED)..." CR);
    while (true) {
        if (!(getOpStateBitmap() & OP_DISCONNECTED)) {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    LOG_INFO_NOFMT("MQTT client successfully connected (I.e. opState ~OP_DISCONNECTED)..." CR);
    networking::concludeRestart();
    LOG_INFO_NOFMT("Starting discovery process..." CR);
    discover();
    uint8_t tries = 0;
    while (true) {
        bool tmpDiscovered;
        tmpDiscovered = discovered;
        if (tmpDiscovered) {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (tries++ >= 100) {
            panic("Discovery process failed, no discovery response was received");
            return RC_DISCOVERY_ERR;
        }
    }
    LOG_INFO("Decoder successfully discovered; URI: %s" CR, decoderUri);
    if (statusCallback != NULL) {
        statusCallback(mqttStatus, statusCallbackArgs);
    }
    sysState->unSetOpStateByBitmap(OP_INIT);
    return RC_OK;
}

void mqtt::regOpStateCb(sysStateCb_t p_systemStateCb, void* p_systemStateCbArgs) {
    sysState->regSysStateCb(p_systemStateCbArgs, p_systemStateCb);
}

void mqtt::unRegOpStateCb(sysStateCb_t p_systemStateCb) {
    sysState->unRegSysStateCb(p_systemStateCb);
}

rc_t mqtt::regStatusCallback(const mqttStatusCallback_t p_statusCallback, const void* p_args) {
    statusCallback = p_statusCallback;
    statusCallbackArgs = (void*)p_args;
    return RC_OK;
}

rc_t mqtt::reConnect(void){
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    LOG_INFO("Re-connecting MQTT Client" CR);
    retryCnt = 0;
    xSemaphoreTake(pubSubLock, portMAX_DELAY);
    mqttClient->disconnect();
    xSemaphoreGive(pubSubLock);
    mqttClient->setServer(brokerUri, brokerPort);
    mqttClient->setKeepAlive(keepAlive);
    xSemaphoreTake(pubSubLock, portMAX_DELAY);
    mqttClient->connect(clientId,
                        brokerUser,
                        brokerPass,
                        opStateTopic,
                        MQTT_DEFAULT_QOS,
                        true,
                        downPayload);
    xSemaphoreGive(pubSubLock);
    xSemaphoreGive(mqttLock);
    uint8_t tries = 0;
    while (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        if (tries++ >= 50) {
            LOG_ERROR_NOFMT("Failed to reconnect MQTT client, will continue to try in the background...");
            return RC_GEN_ERR;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    LOG_INFO_NOFMT("MQTT Client reconnected" CR);
    return RC_OK;
}

void mqtt::disConnect(void) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    LOG_INFO("Disconnecting MQTT Client" CR);
    xSemaphoreTake(pubSubLock, portMAX_DELAY);
    mqttClient->disconnect();
    xSemaphoreGive(pubSubLock);
    xSemaphoreGive(mqttLock);
}

rc_t mqtt::up(void) {
    if (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        LOG_WARN_NOFMT("could not declare MQTT up as opState is OP_DISCONNECTED");
        return RC_GEN_ERR;
    }
    LOG_INFO_NOFMT("Starting MQTT ping supervision" CR);
    sysState->unSetOpStateByBitmap(OP_DISABLED);
    sysState->unSetOpStateByBitmap(OP_CLIEUNAVAILABLE);
    mqttPingUpstreamTopic = new (heap_caps_malloc(sizeof(char) * (strlen(MQTT_PING_UPSTREAM_TOPIC) + strlen("/") + strlen(decoderUri) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(MQTT_PING_UPSTREAM_TOPIC) + strlen("/") + strlen(decoderUri) + 1];
    sprintf(mqttPingUpstreamTopic, "%s%s%s", MQTT_PING_UPSTREAM_TOPIC, "/", decoderUri);
    char mqttPingDownstreamTopic[300];
    sprintf(mqttPingDownstreamTopic, "%s%s%s", MQTT_PING_DOWNSTREAM_TOPIC, "/", decoderUri);
    if (subscribeTopic(mqttPingDownstreamTopic, onMqttPing, NULL)) {
        sysState->setOpStateByBitmap(OP_INTFAIL); 
        panic("Failed to to subscribe to MQTT ping topic");
        return RC_GEN_ERR;
    }
    if(!supervision){
        supervision = true;
        if (!eTaskCreate(
            mqttPingTimer,                      // Task function
            CPU_MQTT_PING_TASKNAME,             // Task function name reference
            CPU_MQTT_PING_STACKSIZE_1K * 1024,  // Stack size
            NULL,                               // Parameter passing
            CPU_MQTT_PING_PRIO,                 // Priority 0-24, higher is more
            CPU_MQTT_PING_STACK_ATTR)) {        // Task stack attribute
            panic("MQTT supervision task could not be started");
            return RC_OUT_OF_MEM_ERR;
        }
    }
    return RC_OK;
}

void mqtt::down(void) {
    LOG_INFO_NOFMT("Declaring Mqtt client down" CR);
    sysState->setOpStateByBitmap(OP_DISABLED);
    supervision = false;
}

rc_t mqtt::subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    bool found = false;
    int i;
    int j;
    char* topic = new (heap_caps_malloc(strlen(p_topic) + strlen(mqttTopicPrefix) + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char;
    strcpy(topic, mqttTopicPrefix);
    strcat(topic, p_topic);
    Serial.printf("XXXXXXXXXXXX Subscribing to topic: %s, with callback: %i, and cbArgs* %i\n", topic, p_callback, p_args);
    //uint16_t state = sysState->getOpStateBitmap();
    /*if (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("Could not subscribe to topic as the MQTT client is not running");
        xSemaphoreGive(mqttLock);
        return RC_GEN_ERR;
    }*/
    LOG_INFO("Subscribing to topic %s" CR, topic);
    for (i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, topic)) {
            Serial.printf("XXXXXXXXXXXX Topic %s has already been subscribed to\n", topic);
            found = true;
            break;
        }
    }
    if (!found) {
        LOG_VERBOSE("Adding new subscription topic %s with related callback" CR, topic);
        Serial.printf("XXXXXXXXXXXX Topic %s has not been subscribed to, subscribing to it\n", topic);
        mqttTopics.push_back(new (heap_caps_malloc(sizeof(mqttTopic_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) mqttTopic_t);
        mqttTopics.back()->topic = topic;
        mqttTopics.back()->topicList = new QList<mqttSub_t*>;
        mqttTopics.back()->topicList->push_back(new(heap_caps_malloc(sizeof(mqttSub_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) mqttSub_t);
        mqttTopics.back()->topicList->back()->topic = topic;
        mqttTopics.back()->topicList->back()->mqttSubCallback = p_callback;
        mqttTopics.back()->topicList->back()->mqttCallbackArgs = (void*)p_args;
        xSemaphoreTake(pubSubLock, portMAX_DELAY);
        bool rc = mqttClient->subscribe(topic, defaultQoS);
        xSemaphoreGive(pubSubLock);
/*      if (!rc) {//IS THIS LOGIC REALLY CORRECT WOULDNT RESUBSCRIBE AND CONNECTIVITY MONITORING SOLVE THIS
            Serial.printf("XXXXXXXXXXXX Could not subscribe to Topic %s\n", topic);
            sysState->setOpStateByBitmap(OP_INTFAIL);
            int conStat;
            if (conStat = mqttClient->state()){ 
                //panic("Could not subscribe to topic from broker as client is not connected, status: %d", conStat);
                LOG_WARN("Could not subscribe to topic from broker as client is not connected, status: %d - continuing...", conStat);
                xSemaphoreGive(mqttLock);
                return RC_OK;
            }
            else {
                panic("Could not subscribe to topic from broker", stat);
                xSemaphoreGive(mqttLock);
                return RC_GEN_ERR;
            }
        }
*/

        xSemaphoreGive(mqttLock);
        return RC_OK;
    }
    else {
        Serial.printf("XXXXXXXXXXXX Topic %s has already been subscribed to, adding new cb subscription\n", topic);
        for (j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
            if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback && mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs == p_args) {
                Serial.printf("XXXXXXXXXXXX subscriber CB: %i and CBArgs %i already registered - skipping\n", p_callback, p_args);
                LOG_WARN("MQTT-subscribeTopic: subscribeTopic was called, but the callback 0x%x and the args 0x%x for Topic %s already exists - doing nothing" CR, (int)p_callback, p_args, topic);
                xSemaphoreGive(mqttLock);
                return RC_OK; //SHOULD WE ISSUE AN ALLREADY EXIST ERROR?
            }
        }
        LOG_VERBOSE("Adding new callback to existing topic %s" CR, topic);
        mqttTopics.at(i)->topicList->push_back(new (heap_caps_malloc(sizeof(mqttSub_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) mqttSub_t);
        mqttTopics.at(i)->topicList->back()->topic = topic;
        mqttTopics.at(i)->topicList->back()->mqttSubCallback = p_callback;
        mqttTopics.at(i)->topicList->back()->mqttCallbackArgs = (void*)p_args;
        xSemaphoreGive(mqttLock);
        return RC_OK;
    }
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;   //Should never reach here
}

rc_t mqtt::unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    bool topicFound = false;
    bool cbFound = false;
    char* topic = new (heap_caps_malloc(strlen(p_topic) + strlen(mqttTopicPrefix) + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char;
    strcpy(topic, mqttTopicPrefix);
    strcat(topic, p_topic);
    LOG_INFO("MQTT-Unsubscribe, Un-subscribing to topic %s" CR, topic);
    for (int i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, topic)) {
            topicFound = true;
            for (int j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
                if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback) {
                    cbFound = true;
                    mqttTopics.at(i)->topicList->clear(j);
                    LOG_INFO("MQTT-Unsubscribe, Removed callback for %s" CR, topic);
                    if (mqttTopics.at(i)->topicList->size() == 0) {
                        delete mqttTopics.at(i)->topicList;
                        delete mqttTopics.at(i)->topic;
                        mqttTopics.clear(i);
                        xSemaphoreTake(pubSubLock, portMAX_DELAY);
                        bool rc = mqttClient->unsubscribe(topic);
                        xSemaphoreGive(pubSubLock);
                        if (!rc) {
                            LOG_ERROR("Could not unsubscribe Topic: %s from broker" CR, topic);
                            delete topic;
                            xSemaphoreGive(mqttLock);
                            return RC_GEN_ERR;
                        }
                        LOG_INFO("Last callback for %s unsubscribed - unsubscribed Topic from broker" CR, topic);
                        delete topic;
                        xSemaphoreGive(mqttLock);
                        return RC_OK;
                    }
                }
            }
        }
    }
    if (!topicFound) {
        LOG_ERROR("Topic %s not found" CR, topic);
        delete topic;
        xSemaphoreGive(mqttLock);
        return RC_GEN_ERR;
    }
    if (!cbFound) {
        LOG_ERROR("MQTT-Unsubscribe, callback not found while unsubscribing topic %s" CR, topic);
        xSemaphoreGive(mqttLock);
        return RC_GEN_ERR;
    }
    delete topic;
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;
}

rc_t mqtt::sendMsg(const char* p_topic, const char* p_payload, bool p_retain) {
    LOG_VERBOSE("Sending MQTT message, Topic: %s, Payload: %s" CR, p_topic, p_payload);
    char topic[300];
    char sysStateStr[200];
    strcpy(topic, mqttTopicPrefix);
    strcat(topic, p_topic);
    xSemaphoreTake(pubSubLock, portMAX_DELAY);
    bool rc = mqttClient->publish(topic, p_payload, p_retain);
    xSemaphoreGive(pubSubLock);
    if ((sysState->getOpStateBitmap() & OP_DISCONNECTED) || !rc) {
        LOG_ERROR("Could not send message, topic: %s, payload: %s , either the MQTT OP-state: %s doesnt allow it, or there was an internal MQTT error" CR, topic, p_payload, sysState->getOpStateStr(sysStateStr));
        return RC_GEN_ERR;
    }
    else {
        LOG_VERBOSE("Sent a message, topic: %s, payload: %s" CR, p_topic, p_payload);
        return RC_OK;
    }
    return RC_GEN_ERR;                                      // We should never come here
}

rc_t mqtt::setDecoderUri(const char* p_decoderUri) {        //Lock btw set and get needed
    if (decoderUri)
        delete decoderUri;
    decoderUri = createNcpystr(p_decoderUri);
    if(!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

const char* mqtt::getDecoderUri(void) {
    return decoderUri;
}

rc_t mqtt::setBrokerUri(const char* p_brokerUri, bool p_persist) {
    if (brokerUri)
        delete brokerUri;
    brokerUri = createNcpystr(p_brokerUri);
    networking::setMqttUri(p_brokerUri, p_persist);
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

const char* mqtt::getBrokerUri(void) {
    return brokerUri;
}

rc_t mqtt::setBrokerPort(uint16_t p_brokerPort, bool p_persist) {
    if (p_brokerPort < 1 || p_brokerPort > 65535)
        return RC_PARAMETERVALUE_ERR;
    brokerPort = p_brokerPort;
    networking::setMqttPort(p_brokerPort, p_persist);
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

uint16_t mqtt::getBrokerPort(void) {
    return brokerPort;
}

rc_t mqtt::setBrokerUser(const char* p_brokerUser) {
    if (brokerUser)
        delete brokerUser;
    brokerUser = createNcpystr(p_brokerUser);
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

const char* mqtt::getBrokerUser(void) {
    return brokerUser;
}

rc_t mqtt::setBrokerPass(const char* p_brokerPass) {
    if (brokerPass)
        delete brokerPass;
    brokerPass = createNcpystr(p_brokerPass);
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

const char* mqtt::getBrokerPass(void) {
    return brokerPass;
}

rc_t mqtt::setClientId(const char* p_clientId) {
    if (clientId)
        delete clientId;
    clientId = createNcpystr(p_clientId);
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

const char* mqtt::getClientId(void) {
    return clientId;
}

rc_t mqtt::setDefaultQoS(uint8_t p_defaultQoS) {
    if (p_defaultQoS < 0 || p_defaultQoS > 2)
        return RC_PARAMETERVALUE_ERR;
    defaultQoS = p_defaultQoS;
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

uint8_t mqtt::getDefaultQoS(void) {
    return defaultQoS;
}

rc_t mqtt::setKeepAlive(uint8_t p_keepAlive) {
    if (p_keepAlive < 0 || p_keepAlive > 600)
        return RC_PARAMETERVALUE_ERR;
    keepAlive = p_keepAlive;
    if (!(sysState->getOpStateBitmap() & OP_INIT))
        reConnect();
    return RC_OK;
}

uint8_t mqtt::getKeepAlive(void) {
    return keepAlive;
}

rc_t mqtt::setPingPeriod(float p_pingPeriod) {
    if (p_pingPeriod < 0 || p_pingPeriod > 600)
        return RC_PARAMETERVALUE_ERR;
    pingPeriod = p_pingPeriod;
    if (!(sysState->getOpStateBitmap() & OP_INIT)){
        reConnect();
    }
    return RC_OK;
}

float mqtt::getPingPeriod(void) {
    return pingPeriod;
}

void mqtt::setMqttTopicPrefix(const char* p_mqttTopicPrefix) {
    strcpy(mqttTopicPrefix, p_mqttTopicPrefix);
}

const char* mqtt::getMqttTopicPrefix(void) {
    return mqttTopicPrefix;
}

uint16_t mqtt::getOpStateBitmap(void) {
    return sysState->getOpStateBitmap();
}

rc_t mqtt::getOpStateStr(char* p_opState) {
    sysState->getOpStateStr(p_opState);
    return RC_OK;
}

// Private methods
void mqtt::discover(void) {
    const char* mqtt_discovery_response_topic = MQTT_DISCOVERY_RESPONSE_TOPIC;
    subscribeTopic(mqtt_discovery_response_topic, &onDiscoverResponse, NULL);
    sendMsg(MQTT_DISCOVERY_REQUEST_TOPIC, MQTT_DISCOVERY_REQUEST_PAYLOAD, false);
}

void mqtt::onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy) {
    if (discovered) {
        LOG_INFO("Discovery response received several times - doing nothing ..." CR);
        return;
    }
    LOG_INFO_NOFMT("Got a discover response, parsing and validating it..." CR);
    tinyxml2::XMLDocument* xmlDiscoveryDoc;
    xmlDiscoveryDoc = new (heap_caps_malloc(sizeof(tinyxml2::XMLDocument), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) tinyxml2::XMLDocument;
    tinyxml2::XMLElement* xmlDiscoveryElement;
    LOG_INFO("Parsing discovery response: %s" CR, p_payload);
    if (xmlDiscoveryDoc->Parse(p_payload) || (xmlDiscoveryElement = xmlDiscoveryDoc->FirstChildElement("DiscoveryResponse")) == NULL) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("Discovery response parsing or validation failed");
        return;
    }
    xmlDiscoveryElement = xmlDiscoveryElement->FirstChildElement("Decoder");
    bool found = false;
    while (xmlDiscoveryElement != NULL) {
        if (!strcmp(xmlDiscoveryElement->Value(), "Decoder") && xmlDiscoveryElement->FirstChildElement("MAC") != NULL && xmlDiscoveryElement->FirstChildElement("URI") != NULL && xmlDiscoveryElement->FirstChildElement("URI")->GetText() != NULL && !strcmp(xmlDiscoveryElement->FirstChildElement("MAC")->GetText(), networking::getMac())) {
            decoderUri = new (heap_caps_malloc(sizeof(char) * (strlen(xmlDiscoveryElement->FirstChildElement("URI")->GetText()) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlDiscoveryElement->FirstChildElement("URI")->GetText()) + 1];
            strcpy(decoderUri, xmlDiscoveryElement->FirstChildElement("URI")->GetText());
            found = true;
            break;
        }
        xmlDiscoveryElement = xmlDiscoveryElement->NextSiblingElement();
    }
    if (!found) {
        panic("Discovery response doesn't provide any information about this decoder (MAC)");
        return;
    }
    discovered = true;
    LOG_INFO("Discovery response successful, set URI to %s for this decoders MAC %s" CR, decoderUri, networking::getMac());
    sysState->unSetOpStateByBitmap(OP_UNDISCOVERED);
}

rc_t mqtt::reSubscribe(void) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    LOG_INFO_NOFMT("Resubscribing all registered topics" CR);
    xSemaphoreTake(pubSubLock, portMAX_DELAY);
    mqttClient->unsubscribe("#");
    xSemaphoreGive(pubSubLock);
    for (int i = 0; i < mqttTopics.size(); i++) {
        xSemaphoreTake(pubSubLock, portMAX_DELAY);
        bool rc = mqttClient->subscribe(mqttTopics.at(i)->topic, defaultQoS);
        xSemaphoreGive(pubSubLock);
        if (!rc) {
            sysState->setOpStateByBitmap(OP_INTFAIL);
            xSemaphoreGive(mqttLock);
            panic("Failed to resubscribe topic from broker");
            return RC_GEN_ERR;
        }
    }
    xSemaphoreGive(mqttLock);
    LOG_INFO_NOFMT("Successfully resubscribed to all registered topics" CR);
    return RC_OK;
}

void mqtt::onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length) {
    mqttJobDesc_t* mqttJobDesc;
    mqttJobDesc = new (heap_caps_malloc(sizeof(mqttJobDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) mqttJobDesc_t;
    if (!mqttJobDesc) {
        panic("Failed to allocate a MQTT receive job descriptor");
        return;
    }
    mqttJobDesc->topic = new (heap_caps_malloc(sizeof(char) * (strlen(p_topic) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(p_topic) + 1];
    if (!mqttJobDesc->topic) {
        panic("Failed to allocate a MQTT topic job descriptor");
        return;
    }
    strcpy(mqttJobDesc->topic, p_topic);
    mqttJobDesc->payload = new (heap_caps_malloc(sizeof(char) * (p_length + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[p_length + 1];
    if (!mqttJobDesc->payload) {
        panic("Failed to allocate a MQTT payload job descriptor");
        return;
    }
    memcpy(mqttJobDesc->payload, p_payload, p_length);
    mqttJobDesc->payload[p_length] = '\0';
    mqttJobDesc->length = p_length;
    LOG_VERBOSE("Received an MQTT mesage, topic: %s, payload: %s, length: %d, enquing it to the job queue" CR, mqttJobDesc->topic, mqttJobDesc->payload, p_length);
    mqttJobHandle->enqueue(dequeueMqttRxMsg, mqttJobDesc, false);
}

void mqtt::dequeueMqttRxMsg(void* p_mqttJobDesc){
    bool subFound = false;
    for (int i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, ((mqttJobDesc_t*)p_mqttJobDesc)->topic)) {
            Serial.printf("YYYYYYYYYYYY Received an MQTT message topic %s with payload %s, There Are %i subscribers to this topic\n", mqttTopics.at(i)->topic, ((mqttJobDesc_t*)p_mqttJobDesc)->payload, mqttTopics.at(i)->topicList->size());
            for (int j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
                subFound = true;
                Serial.printf("YYYYYYYYYYYY Sending MQTT Callback to helper %i, with callback Arg* %i\n", mqttTopics.at(i)->topicList->at(j)->mqttSubCallback, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs);
                mqttTopics.at(i)->topicList->at(j)->mqttSubCallback(((mqttJobDesc_t*)p_mqttJobDesc)->topic, ((mqttJobDesc_t*)p_mqttJobDesc)->payload, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs);
                Serial.printf("YYYYYYYYYYYY Sent MQTT Callback to helper %i\n", mqttTopics.at(i)->topicList->at(j)->mqttSubCallback);
            }
        }
    }
    if (!subFound)
        LOG_ERROR("Could not find any subscription for received message topic: %s" CR, ((mqttJobDesc_t*)p_mqttJobDesc)->topic);
    delete ((mqttJobDesc_t*)p_mqttJobDesc)->topic;
    delete ((mqttJobDesc_t*)p_mqttJobDesc)->payload;
    delete p_mqttJobDesc;
}


uint32_t mqtt::getOverRuns(void) {
    return overRuns;
}

void mqtt::clearOverRuns(void) {
    overRuns = 0;
}

uint32_t mqtt::getMeanLatency(void) {
    if (avgSamples == 0)
        return 0;
    uint32_t* tmpLatencyVect = new (heap_caps_malloc(sizeof(uint32_t) * avgSamples, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint32_t[avgSamples]; //Wee need to fix all latencies to type int32_t, and all other performance metrics to int....
    uint32_t accLatency = 0;
    uint32_t meanLatency = 0;
    //TAKE A LOCK BLOCKING latencyVect access
    memcpy(tmpLatencyVect, latencyVect, avgSamples);
    //RELEASE THE LOCK
    for (uint16_t latencyIndex = 0; latencyIndex < avgSamples; latencyIndex++) {
        accLatency += tmpLatencyVect[latencyIndex];
    }
    meanLatency = accLatency / avgSamples;
    delete tmpLatencyVect;
    return meanLatency;
}

uint32_t mqtt::getMaxLatency(void) {
    uint32_t tmpMaxLatency = maxLatency;
    return tmpMaxLatency;
}

void mqtt::clearMaxLatency(void) {
    maxLatency = 0;
}

bool mqtt::getSubs(uint16_t p_index, char* p_topic, uint16_t p_topicSize, char* p_cbs, uint16_t p_cbSize) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    if (p_index >= mqttTopics.size()) {
        xSemaphoreGive(mqttLock);
        return false;
    }
    else {
        strcpyTruncMaxLen(p_topic, mqttTopics.at(p_index)->topic, p_topicSize);
        strcpyTruncMaxLen(p_cbs, itoa((int)(void*)mqttTopics.at(p_index)->topicList->at(0)->mqttSubCallback, p_cbs, 10), p_cbSize);
        for (uint16_t i = 1; i < mqttTopics.at(p_index)->topicList->size(); i++) {
            strcatTruncMaxLen(p_cbs, ", ", p_cbSize);
            strcatTruncMaxLen(p_cbs, itoa((int)(void*)mqttTopics.at(p_index)->topicList->at(0)->mqttSubCallback, p_cbs, 10), p_cbSize);
        }
    }
    xSemaphoreGive(mqttLock);
    return true;
}

void mqtt::poll(void* dummy) {
    int64_t  nextLoopTime = esp_timer_get_time();
    int64_t thisLoopTime;
    uint16_t latencyIndex = 0;
    int32_t latency = 0;
    for (uint16_t i = 0; i < avgSamples; i++)
        latencyVect[i] = 0;
    int stat;
    int printPoll = 0;
    while (true) {
        //if (!(printPoll++ % 20))
            //Serial.printf("XXXXXXX Poll\n");
        if (sysState->getOpStateBitmap() & OP_INTFAIL) {
            delete mqttWdt;
            vTaskDelete(NULL);
            return;
        }
        char op[200];
        thisLoopTime = nextLoopTime;
        nextLoopTime += MQTT_POLL_PERIOD_MS * 1000;
        mqttWdt->feed();
        xSemaphoreTake(pubSubLock, portMAX_DELAY);
        mqttClient->loop();
        xSemaphoreGive(pubSubLock);
        stat = mqttClient->state();
        switch (stat) {
        case MQTT_CONNECTED:
            if (mqttStatus != stat) {
                LOG_INFO("MQTT connection established - unsetting opstate OP_DISCONNECTED" CR);
                sysState->unSetOpStateByBitmap(OP_DISCONNECTED);
                reSubscribeReq = true;
            }
            retryCnt = 0;
            break;
        case MQTT_CONNECTION_TIMEOUT:
        case MQTT_CONNECTION_LOST:
        case MQTT_CONNECT_FAILED:
        case MQTT_DISCONNECTED:
            if (mqttStatus != stat)
                LOG_WARN_NOFMT("MQTT Client disconnected, trying to re-connect" CR);
            sysState->setOpStateByBitmap(OP_DISCONNECTED);
            if (retryCnt++ >= MAX_MQTT_CONNECT_ATTEMPTS) {
                sysState->setOpStateByBitmap(OP_INTFAIL);
                networking::reportNwFail();
                panic("Max number of MQTT connect/reconnect attempts reached - stopping MQTT poll loop");
                Serial.printf("XXXXXXX MQTT Exit 1\n");
                delete mqttWdt;
                vTaskDelete(NULL);
                Serial.printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX Why did it return\n");
                return;
            }
            if (xSemaphoreTake(mqttLock, (TickType_t)0) == pdTRUE){
                LOG_INFO("Re-connecting MQTT Client" CR);
                retryCnt = 0;
                xSemaphoreTake(pubSubLock, portMAX_DELAY);
                mqttClient->connect(clientId,
                    brokerUser,
                    brokerPass,
                    opStateTopic,
                    MQTT_DEFAULT_QOS,
                    true,
                    downPayload);
                xSemaphoreGive(pubSubLock);
                xSemaphoreGive(mqttLock);
            }
            if (mqttStatus != stat) {
                LOG_ERROR("MQTT connection not established or lost - opState set to OP_FAIL, cause: %d - retrying..." CR, stat); //This never times out but freezes
            }
            break;
        case MQTT_CONNECT_BAD_PROTOCOL:
        case MQTT_CONNECT_BAD_CLIENT_ID:
        case MQTT_CONNECT_UNAVAILABLE:
        case MQTT_CONNECT_BAD_CREDENTIALS:
        case MQTT_CONNECT_UNAUTHORIZED:
            sysState->setOpStateByBitmap(OP_INTFAIL);
            networking::reportNwFail();
            panic("Fatal MQTT error, one of BAD_PROTOCOL, BAD_CLIENT_ID, UNAVAILABLE, BAD_CREDETIALS, UNOTHORIZED - stopping MQTT poll loop" CR);
            Serial.printf("XXXXXXX MQTT Exit 2\n");
            delete mqttWdt;
            vTaskDelete(NULL);
            return;
        }
        if (mqttStatus != stat) {
            mqttStatus = stat;
            if (statusCallback != NULL) {
                statusCallback(mqttStatus, statusCallbackArgs);
            }
        }
        if (latencyIndex >= avgSamples) {
            latencyIndex = 0;
        }
        latency = esp_timer_get_time() - thisLoopTime;
        latencyVect[latencyIndex++] = latency;
        if (latency > maxLatency) {
            maxLatency = latency;
        }
        int64_t delay = nextLoopTime - esp_timer_get_time();
        int64_t now = esp_timer_get_time();
        if ((int)delay > 0) {
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
        }
        else {
            LOG_VERBOSE_NOFMT("MQTT Overrun" CR);
            overRuns++;
            nextLoopTime = esp_timer_get_time();
        }
    }
    Serial.printf("XXXXXXX MQTT Exit 3\n");
    delete mqttWdt;
    vTaskDelete(NULL);
}

void mqtt::mqttPingTimer(void* dummy) {
    LOG_INFO("MQTT Ping timer started, ping period: %d" CR, pingPeriod);
    missedPings = 0;
    while (supervision) {
        if (reSubscribeReq && mqttClient->state() == MQTT_CONNECTED) {
            reSubscribe();
            reSubscribeReq = false;
        }
        else if (!(sysState->getOpStateBitmap() & OP_DISABLED) && (pingPeriod != 0)) {
            if (++missedPings >= MAX_MQTT_LOST_PINGS) {
                sysState->setOpStateByBitmap(OP_CLIEUNAVAILABLE);
                panic("Lost maximum ping responses - bringing down MQTT");
            }
            sendMsg(mqttPingUpstreamTopic, MQTT_PING_PAYLOAD, false);
        }
        vTaskDelay(pingPeriod * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void mqtt::onMqttPing(const char* p_topic, const char* p_payload, const void* p_dummy) { //Relying on the topic, not parsing the payload for performance
    LOG_VERBOSE_NOFMT("Received a Ping response" CR);
    if (sysState->getOpStateBitmap() & OP_CLIEUNAVAILABLE)
            sysState->unSetOpStateByBitmap(OP_CLIEUNAVAILABLE);
    missedPings = 0;
}

void mqtt::onMqttJobOverload(void* p_dummy, bool p_overload) {
    if (p_overload)
    panic("Mqtt job buffer overloaded");
}

/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/
