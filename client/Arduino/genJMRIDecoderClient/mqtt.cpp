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
TaskHandle_t* mqtt::supervisionTaskHandle = NULL;
systemState* mqtt::sysState = NULL;
SemaphoreHandle_t mqtt::mqttLock = NULL;
WiFiClient mqtt::espClient;
PubSubClient* mqtt::mqttClient;
uint32_t mqtt::overRuns = 0;
uint32_t mqtt::maxLatency = 0;
uint16_t mqtt::avgSamples = 0;
uint32_t* mqtt::latencyVect = NULL;
char* mqtt::decoderUri = NULL;
char* mqtt::brokerUri = NULL;
uint16_t mqtt::brokerPort = 0;
char* mqtt::brokerUser = NULL;
char* mqtt::brokerPass = NULL;
char* mqtt::clientId = NULL;
uint8_t mqtt::defaultQoS = 0;
uint8_t mqtt::keepAlive = 0;
TaskHandle_t mqtt::mqttPingHandle = NULL;
bool mqtt::opStateTopicSet = false;
char* mqtt::opStateTopic = NULL;
char* mqtt::upPayload = NULL;
char* mqtt::downPayload = NULL;
char* mqtt::mqttPingUpstreamTopic = NULL;
uint8_t mqtt::missedPings = 0;
int mqtt::mqttStatus;
uint8_t mqtt::qos = 0;
bool mqtt::defaultRetain = false;
float mqtt::pingPeriod = 0 ;
bool mqtt::discovered = false;
QList<mqttTopic_t*> mqtt::mqttTopics;
mqttStatusCallback_t mqtt::statusCallback = NULL;
void* mqtt::statusCallbackArgs = NULL;
wdt* mqtt::mqttWdt = NULL;
bool mqtt::supervision = false;

// Public methods
void mqtt::create(void) {
    Log.INFO("mqtt::create: Creating MQTT client" CR);
    sysState = new systemState(NULL);
    sysState->setSysStateObjName("mqtt");
    sysState->setOpStateByBitmap(OP_INIT | OP_DISABLED | OP_DISCONNECTED | OP_UNDISCOVERED | OP_CLIEUNAVAILABLE);
    mqttLock = xSemaphoreCreateMutex();
}

rc_t mqtt::init(const char* p_brokerUri, uint16_t p_brokerPort, const char* p_brokerUser, const char* p_brokerPass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, float p_pingPeriod, bool p_defaultRetain) {
    Log.INFO("mqtt::init: Initializing and starting MQTT client, BrokerURI: %s, BrokerPort: %i, User: %s, Password: %s, ClientId: %s" CR, p_brokerUri, p_brokerPort, p_brokerUser, p_brokerPass, p_clientId);
    mqttWdt = new wdt(MQTT_POLL_PERIOD_MS * 3 * 1000, "MQTT watchdog", FAULTACTION_FAILSAFE_ALL | FAULTACTION_REBOOT);
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
    avgSamples = int(MQTT_POLL_LATENCY_AVG_TIME_MS * 1000 / MQTT_POLL_PERIOD_MS);
    latencyVect = new uint32_t[avgSamples];
    mqttClient = new PubSubClient(espClient);
    mqttClient->setServer(brokerUri, brokerPort);
    mqttClient->setKeepAlive(keepAlive);
    mqttStatus = mqttClient->state();
    if (!mqttClient->setBufferSize(MQTT_BUFF_SIZE)) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("mqtt::init: Could not allocate MQTT buffers - rebooting...");
        return RC_OUT_OF_MEM_ERR;
    }
    mqttClient->setCallback(&onMqttMsg);
    Log.INFO("mqtt::init: Spawning MQTT poll task" CR);
    xTaskCreatePinnedToCore(poll,                               // Task function
                            CPU_MQTT_POLL_TASKNAME,             // Task function name reference
                            CPU_MQTT_POLL_STACKSIZE_1K * 1024,  // Stack size
                            NULL,                               // Parameter passing
                            CPU_MQTT_POLL_PRIO,                 // Priority 0-24, higher is more
                            NULL,                               // Task handle
                            CPU_MQTT_POLL_CORE);                // Core [CORE_0 | CORE_1]

    Log.INFO("mqtt::init: Waiting for MQTT client to connect (I.e. opState ~OP_DISCONNECTED)..." CR);
    while (true) {
        if (!(getOpStateBitmap() & OP_DISCONNECTED)) {
            break;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Log.INFO("mqtt::init: MQTT client successfully connected (I.e. opState ~OP_DISCONNECTED)..." CR);
    Log.INFO("mqtt::init: Starting discovery process..." CR);
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
            panic("mqtt::init: Discovery process failed, no discovery response was received - rebooting...");
            return RC_DISCOVERY_ERR;
        }
    }
    Log.INFO("mqtt::init: Decoder successfully discovered; URI: %s" CR, decoderUri);
    if (statusCallback != NULL) {
        statusCallback(mqttStatus, statusCallbackArgs);
    }
    sysState->unSetOpStateByBitmap(OP_INIT);
    wdt::wdtRegMqttDown(wdtKicked, NULL);
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
    Log.INFO("mqtt::reConnect: Re-connecting MQTT Client" CR);
    mqttClient->disconnect();
    mqttClient->setServer(brokerUri, brokerPort);
    mqttClient->setKeepAlive(keepAlive);
    mqttClient->connect(clientId,
                        brokerUser,
                        brokerPass,
                        opStateTopic,
                        MQTT_DEFAULT_QOS,
                        true,
                        downPayload);
    uint8_t tries = 0;
    while (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        if (tries++ >= 10) {
            Log.ERROR("mqtt::reConnect: Failed to reconnect MQTT client, will continue to try in the background...");
            return RC_GEN_ERR;
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Log.INFO("mqtt::reConnect: MQTT Client reconnected" CR);
    return RC_OK;
}

void mqtt::disConnect(void) {
    mqttClient->disconnect();
}

rc_t mqtt::up(void) {
    if (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        Log.WARN("mqtt::up, could not declare MQTT up as opState is OP_DISCONNECTED");
        return RC_GEN_ERR;
    }
    Log.INFO("mqtt::up: Starting MQTT ping supervision" CR);
    sysState->unSetOpStateByBitmap(OP_DISABLED);
    sysState->unSetOpStateByBitmap(OP_CLIEUNAVAILABLE);
    mqttPingUpstreamTopic = new char[strlen(MQTT_PING_UPSTREAM_TOPIC) + strlen("/") + strlen(decoderUri) + 1];
    sprintf(mqttPingUpstreamTopic, "%s%s%s", MQTT_PING_UPSTREAM_TOPIC, "/", decoderUri);
    char mqttPingDownstreamTopic[300];
    sprintf(mqttPingDownstreamTopic, "%s%s%s", MQTT_PING_DOWNSTREAM_TOPIC, "/", decoderUri);
    if (subscribeTopic(mqttPingDownstreamTopic, onMqttPing, NULL)) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("mqtt::up: Failed to to subscribe to MQTT ping topic - rebooting..." CR);
        return RC_GEN_ERR;
    }
    if(!supervision){
        supervision = true;
        xTaskCreatePinnedToCore(
            mqttPingTimer,                      // Task function
            CPU_MQTT_PING_TASKNAME,             // Task function name reference
            CPU_MQTT_PING_STACKSIZE_1K * 1024,  // Stack size
            NULL,                               // Parameter passing
            CPU_MQTT_PING_PRIO,                 // Priority 0-24, higher is more
            &mqttPingHandle,                    // Task handle
            CPU_MQTT_PING_CORE);                // Core [CORE_0 | CORE_1]
    }
    return RC_OK;
}

void mqtt::down(void) {
    Log.INFO("mqtt::down, declaring Mqtt client down" CR);
    sysState->setOpStateByBitmap(OP_DISABLED);
    supervision = false;
}

void mqtt::wdtKicked(void* p_dummy) {
    panic("mqtt::wdtKicked: Watch dog kicked - rebooting...", CR);
    down();
}

rc_t mqtt::subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args) {
    bool found = false;
    int i;
    int j;
    char* topic = createNcpystr(p_topic);
    uint16_t state = sysState->getOpStateBitmap();
    if (sysState->getOpStateBitmap() & OP_DISCONNECTED) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("mqtt::subscribeTopic: Could not subscribe to topic as the MQTT client is not running - rebooting...");
        return RC_GEN_ERR;
    }
    Log.INFO("mqtt::subscribeTopic: Subscribing to topic %s" CR, topic);
    for (i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, topic)) {
            found = true;
            break;
        }
    }
    if (!found) {
        Log.VERBOSE("mqtt::subscribeTopic: Adding new subscription topic %s with related callback" CR, topic);
        mqttTopics.push_back(new(mqttTopic_t));
        mqttTopics.back()->topic = topic;
        mqttTopics.back()->topicList = new QList<mqttSub_t*>;
        mqttTopics.back()->topicList->push_back(new(mqttSub_t));
        mqttTopics.back()->topicList->back()->topic = topic;
        mqttTopics.back()->topicList->back()->mqttSubCallback = p_callback;
        mqttTopics.back()->topicList->back()->mqttCallbackArgs = (void*)p_args;
        if (!mqttClient->subscribe(topic, defaultQoS)) {
            sysState->setOpStateByBitmap(OP_INTFAIL);
            int conStat;
            if (conStat = mqttClient->state()){
                panic("mqtt::subscribeTopic: Could not subscribe to topic from broker as client is not connected, status: %d - rebooting...", conStat);
                return RC_GEN_ERR;
            }
            else {
                panic("mqtt::subscribeTopic: Could not subscribe to topic from broker - rebooting...", stat);
                return RC_GEN_ERR;
            }

        }
        return RC_OK;
    }
    else {
        for (j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
            if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback) {
                Log.WARN("mqtt::subscribeTopic: MQTT-subscribeTopic: subscribeTopic was called, but the callback 0x%x for Topic %s already exists - doing nothing" CR, (int)p_callback, topic);
                return RC_OK;
            }
        }
        Log.VERBOSE("mqtt::subscribeTopic: Adding new callback to existing topic %s" CR, topic);
        mqttTopics.at(i)->topicList->push_back(new(mqttSub_t));
        mqttTopics.at(i)->topicList->back()->topic = topic;
        mqttTopics.at(i)->topicList->back()->mqttSubCallback = p_callback;
        mqttTopics.at(i)->topicList->back()->mqttCallbackArgs = (void*)p_args;
        return RC_OK;
    }
    return RC_GEN_ERR;   //Should never reach here
}

rc_t mqtt::unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback) {
    bool topicFound = false;
    bool cbFound = false;
    char* topic = createNcpystr(p_topic);
    Log.INFO("MQTT-Unsubscribe, Un-subscribing to topic %s" CR, topic);
    for (int i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, topic)) {
            topicFound = true;
            for (int j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
                if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback) {
                    cbFound = true;
                    mqttTopics.at(i)->topicList->clear(j);
                    Log.INFO("MQTT-Unsubscribe, Removed callback for %s" CR, topic);
                    if (mqttTopics.at(i)->topicList->size() == 0) {
                        delete mqttTopics.at(i)->topicList;
                        delete mqttTopics.at(i)->topic;
                        mqttTopics.clear(i);
                        if (!mqttClient->unsubscribe(topic)) {
                            Log.ERROR("mqtt::unSubscribeTopic: could not unsubscribe Topic: %s from broker" CR, topic);
                            delete topic;
                            return RC_GEN_ERR;
                        }
                        Log.INFO("mqtt::unSubscribeTopic: Last callback for %s unsubscribed - unsubscribed Topic from broker" CR, topic);
                        delete topic;
                        return RC_OK;
                    }
                }
            }
        }
    }
    if (!topicFound) {
        Log.ERROR("mqtt::unSubscribeTopic: Topic %s not found" CR, topic);
        delete topic;
        return RC_GEN_ERR;
    }
    if (!cbFound) {
        Log.ERROR("MQTT-Unsubscribe, callback not found while unsubscribing topic %s" CR, topic);
        return 1;
    }
    delete topic;
    return RC_GEN_ERR;
}

rc_t mqtt::sendMsg(const char* p_topic, const char* p_payload, bool p_retain) {
    Log.verbose("mqtt::sendMsg: Sending MQTT message, Topic: %s, Payload: %s" CR, p_topic, p_payload);
    char sysStateStr[200];
    if ((sysState->getOpStateBitmap() & OP_DISCONNECTED) || !mqttClient->publish(p_topic, p_payload, p_retain)) {
        Log.ERROR("mqtt::sendMsg: could not send message, topic: %s, payload: %s , either the MQTT OP-state: %s doesnt allow it, or there was an internal MQTT error" CR, p_topic, p_payload, sysState->getOpStateStr(sysStateStr));
        return RC_GEN_ERR;
    }
    else {
        Log.VERBOSE("mqtt::sendMsg: Sent a message, topic: %s, payload: %s" CR, p_topic, p_payload);
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

float mqtt::getKeepAlive(void) {
    return keepAlive;
}

rc_t mqtt::setPingPeriod(float p_pingPeriod) {
    if (p_pingPeriod < 0 || p_pingPeriod > 600)
        return RC_PARAMETERVALUE_ERR;
    pingPeriod = p_pingPeriod;
    if (!(sysState->getOpStateBitmap() & OP_INIT)){
        down();
        up();
    }
    return RC_OK;
}

float mqtt::getPingPeriod(void) {
    return pingPeriod;
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
        Log.INFO("mqtt::discover: Discovery response receied several times - doing nothing ..." CR);
        return;
    }
    Log.INFO("mqtt::discover: Got a discover response, parsing and validating it..." CR);
    tinyxml2::XMLDocument* xmlDiscoveryDoc;
    xmlDiscoveryDoc = new tinyxml2::XMLDocument;
    tinyxml2::XMLElement* xmlDiscoveryElement;
    Log.INFO("mqtt::onDiscoverResponse: Parsing discovery response: %s" CR, p_payload);
    if (xmlDiscoveryDoc->Parse(p_payload) || (xmlDiscoveryElement = xmlDiscoveryDoc->FirstChildElement("DiscoveryResponse")) == NULL) {
        sysState->setOpStateByBitmap(OP_INTFAIL);
        panic("mqtt::discover: Discovery response parsing or validation failed - Rebooting...");
    }
    xmlDiscoveryElement = xmlDiscoveryElement->FirstChildElement("Decoder");
    bool found = false;
    while (xmlDiscoveryElement != NULL) {
        if (!strcmp(xmlDiscoveryElement->Value(), "Decoder") && xmlDiscoveryElement->FirstChildElement("MAC") != NULL && xmlDiscoveryElement->FirstChildElement("URI") != NULL && xmlDiscoveryElement->FirstChildElement("URI")->GetText() != NULL && !strcmp(xmlDiscoveryElement->FirstChildElement("MAC")->GetText(), networking::getMac())) {
            decoderUri = new char[strlen(xmlDiscoveryElement->FirstChildElement("URI")->GetText()) + 1];
            strcpy(decoderUri, xmlDiscoveryElement->FirstChildElement("URI")->GetText());
            found = true;
            break;
        }
        xmlDiscoveryElement = xmlDiscoveryElement->NextSiblingElement();
    }
    if (!found) {
        panic("mqtt::discover: Discovery response doesn't provide any information about this decoder (MAC) - rebooting...");
    }
    discovered = true;
    Log.INFO("mqtt::discover: Discovery response successful, set URI to %s for this decoders MAC %s" CR, decoderUri, networking::getMac());
    sysState->unSetOpStateByBitmap(OP_UNDISCOVERED);
}

rc_t mqtt::reSubscribe(void) {
    Log.INFO("mqtt::reSubscribe: Resubscribing all registered topics" CR);
    for (int i = 0; i < mqttTopics.size(); i++) {
        if (!mqttClient->subscribe(mqttTopics.at(i)->topic, defaultQoS)) {
            sysState->setOpStateByBitmap(OP_INTFAIL);
            panic("mqtt::reSubscribe: Failed to resubscribe topic from broker - rebooting...");
            return RC_GEN_ERR;
        }
    }
    Log.INFO("mqtt::reSubscribe: Successfully resubscribed to all registered topics" CR);
    return RC_OK;
}

void mqtt::onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length) {
    bool subFound = false;
    char* payload = new char[p_length + 1];
    if (!payload)
        panic("mqtt::onMqttMsg: Failed to allocate a MQTT receive buffer, rebooting...");
    memcpy(payload, p_payload, p_length);
    payload[p_length] = '\0';
    Log.VERBOSE("mqtt::onMqttMsg, Received an MQTT mesage, topic: %s, payload: %s, length: %d" CR, p_topic, payload, p_length);
    for (int i = 0; i < mqttTopics.size(); i++) {
        if (!strcmp(mqttTopics.at(i)->topic, p_topic)) {
            for (int j = 0; j < mqttTopics.at(i)->topicList->size(); j++) {
                subFound = true;
                mqttTopics.at(i)->topicList->at(j)->mqttSubCallback(p_topic, payload, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs);
            }
        }
    }
    if (!subFound) {
        Log.ERROR("mqtt::onMqttMsg, could not find any subscription for received message topic: %s" CR, p_topic);
        delete payload;
        return;
    }
    delete payload;
    return;
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
    uint32_t* tmpLatencyVect = new uint32_t[avgSamples]; //Wee need to fix all latencies to type int32_t, and all other performance metrics to int....
    uint32_t accLatency;
    uint32_t meanLatency;
    memcpy(tmpLatencyVect, latencyVect, avgSamples);
    for (uint16_t latencyIndex = 0; latencyIndex < avgSamples; latencyIndex++) {
        accLatency += latencyVect[latencyIndex];
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

void mqtt::poll(void* dummy) {
    int64_t  nextLoopTime = esp_timer_get_time();
    int64_t thisLoopTime;
    uint16_t latencyIndex = 0;
    int32_t latency = 0;
    uint8_t retryCnt = 0;
    int stat;
    //esp_task_wdt_init(1, true); //enable panic so ESP32 restarts
    //esp_task_wdt_add(NULL); //add current thread to WDT watch
    while (true) {
        char op[200];
        //Serial.println("POLL");
        thisLoopTime = nextLoopTime;
        nextLoopTime += MQTT_POLL_PERIOD_MS * 1000;
        mqttWdt->feed();
        //esp_task_wdt_reset();
        mqttClient->loop();
        stat = mqttClient->state();
        switch (stat) {
        case MQTT_CONNECTED:
            if (mqttStatus != stat) {
                sysState->unSetOpStateByBitmap(OP_DISCONNECTED);
                //mqttClient->unsubscribe("#");
                //resubscribe
                Log.INFO("mqtt::poll, MQTT connection established - unsetting opstate OP_DISCONNECTED" CR);
            }
            retryCnt = 0;
            break;
        case MQTT_CONNECTION_TIMEOUT:
        case MQTT_CONNECTION_LOST:
        case MQTT_CONNECT_FAILED:
        case MQTT_DISCONNECTED:
            if (mqttStatus != stat)
                Log.WARN("mqtt::poll: MQTT Client disconnected, trying to re-connect" CR);
            sysState->setOpStateByBitmap(OP_DISCONNECTED);
            if (retryCnt++ >= MAX_MQTT_CONNECT_ATTEMPTS) {
                sysState->setOpStateByBitmap(OP_INTFAIL);
                panic("mqtt::poll, Max number of MQTT connect/reconnect attempts reached - rebooting...");
            }
            Log.INFO("mqtt::poll: Connecting to Mqtt" CR);
            if (opStateTopicSet) {
                Log.INFO("mqtt::poll: reconnecting with Last will" CR);
                mqttClient->connect(clientId,
                    brokerUser,
                    brokerPass,
                    opStateTopic,
                    MQTT_DEFAULT_QOS,
                    true,
                    downPayload);
            }
            else {
                mqttClient->connect(clientId,
                    brokerUser,
                    brokerPass);
            }
            if (mqttStatus != stat) {
                Log.ERROR("mqtt::poll, MQTT connection not established or lost - opState set to OP_FAIL, cause: %d - retrying..." CR, stat); //This never times out but freezes
            }
            break;
        case MQTT_CONNECT_BAD_PROTOCOL:
        case MQTT_CONNECT_BAD_CLIENT_ID:
        case MQTT_CONNECT_UNAVAILABLE:
        case MQTT_CONNECT_BAD_CREDENTIALS:
        case MQTT_CONNECT_UNAUTHORIZED:
            sysState->setOpStateByBitmap(OP_INTFAIL);
            panic("mqtt::poll, Fatal MQTT error, one of BAD_PROTOCOL, BAD_CLIENT_ID, UNAVAILABLE, BAD_CREDETIALS, UNOTHORIZED - rebooting...");
            break;
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
            Log.VERBOSE("mqtt::poll: MQTT Overrun" CR);
            overRuns++;
            nextLoopTime = esp_timer_get_time();
        }
    }
}

void mqtt::mqttPingTimer(void* dummy) {
    Log.INFO("mqtt::mqttPingTimer: MQTT Ping timer started, ping period: %d" CR, pingPeriod);
    missedPings = 0;
    while (supervision) {
        //Serial.println("Ping");
        if (!(sysState->getOpStateBitmap() & OP_DISABLED) && (pingPeriod != 0)) {
            if (++missedPings >= MAX_MQTT_LOST_PINGS) {
                sysState->setOpStateByBitmap(OP_CLIEUNAVAILABLE);
                panic("mqtt::mqttPingTimer, Lost maximum ping responses - bringing down MQTT and rebooting...");
            }
            sendMsg(mqttPingUpstreamTopic, MQTT_PING_PAYLOAD, false);
        }
        vTaskDelay(pingPeriod * 1000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void mqtt::onMqttPing(const char* p_topic, const char* p_payload, const void* p_dummy) { //Relying on the topic, not parsing the payload for performance
    Log.VERBOSE("mqtt::onMqttPing: Received a Ping response" CR);
    if (sysState->getOpStateBitmap() & OP_CLIEUNAVAILABLE)
            sysState->unSetOpStateByBitmap(OP_CLIEUNAVAILABLE);
    missedPings = 0;
}
/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/
