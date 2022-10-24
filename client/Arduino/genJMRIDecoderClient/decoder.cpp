/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonas.bjurel@hotmail.com)
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
#include "decoder.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: decoder                                                                                                                               */
/* Purpose: The "decoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,           */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroup links. Lightgroups [Signal Masts | general Lights |       */
/*          sequencedLights], Satelite Links, Satelites, Sensors, Actueators, etc.                                                              */
/*          Turnouts or sensors...                                                                                                              */
/*          The "decoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such    */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
decoder::decoder(void) : systemState(this) {
    Log.notice("decoder::decoder: Creating decoder" CR);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_DISCONNECTED | OP_UNDISCOVERED | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
    decoderLock = xSemaphoreCreateMutex();
    if (decoderLock == NULL)
        panic("decoder::decoder: Could not create Lock objects - rebooting...");
    xmlconfig[XML_DECODER_MQTT_URI] = new char[strlen(MQTT_DEFAULT_URI) + 1];
    strcpy(xmlconfig[XML_DECODER_MQTT_URI], MQTT_DEFAULT_URI);
    xmlconfig[XML_DECODER_MQTT_PORT] = new char[strlen(itoa(MQTT_DEFAULT_PORT, NULL, 10)) + 1];
    strcpy(xmlconfig[XML_DECODER_MQTT_PORT], itoa(MQTT_DEFAULT_PORT, NULL, 10));
    xmlconfig[XML_DECODER_MQTT_PREFIX] = new char[strlen(MQTT_PRE_TOPIC_DEFAULT_FRAGMENT) + 1];
    strcpy(xmlconfig[XML_DECODER_MQTT_PREFIX], MQTT_PRE_TOPIC_DEFAULT_FRAGMENT);
    xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = new char[10];
    sprintf(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD], "%f", MQTT_DEFAULT_PINGPERIOD_S);
    xmlconfig[XML_DECODER_NTPURI] = new char[strlen(NTP_DEFAULT_URI) + 1];
    strcpy(xmlconfig[XML_DECODER_NTPURI], NTP_DEFAULT_URI);
    xmlconfig[XML_DECODER_NTPPORT] = new char[strlen(itoa(NTP_DEFAULT_PORT, NULL, 10)) + 1];
    strcpy(xmlconfig[XML_DECODER_NTPPORT], itoa(NTP_DEFAULT_PORT, NULL, 10));
    xmlconfig[XML_DECODER_TZ] = new char[strlen(NTP_DEFAULT_TZ) + 1];
    strcpy(xmlconfig[XML_DECODER_TZ], NTP_DEFAULT_TZ);
    xmlconfig[XML_DECODER_LOGLEVEL] = new char[strlen(itoa(DEFAULT_LOGLEVEL, NULL, 10)) + 1];
    strcpy(xmlconfig[XML_DECODER_LOGLEVEL], itoa(DEFAULT_LOGLEVEL, NULL, 10));
    xmlconfig[XML_DECODER_FAILSAFE] = new char[strlen(DEFAULT_FAILSAFE) + 1];
    strcpy(xmlconfig[XML_DECODER_FAILSAFE], DEFAULT_FAILSAFE);
    xmlconfig[XML_DECODER_SYSNAME] = NULL;
    xmlconfig[XML_DECODER_USRNAME] = NULL;
    xmlconfig[XML_DECODER_DESC] = NULL;
    xmlconfig[XML_DECODER_MAC] = NULL;
    xmlconfig[XML_DECODER_URI] = NULL;
}

decoder::~decoder(void) {
    panic("decoder::~decoder: decoder destructor not supported - rebooting...");
}

rc_t decoder::init(void){
    Log.notice("decoder::init: Initializing decoder" CR);
    Log.notice("decoder::init: Initializing MQTT " CR);
    mqtt::init((char*)MQTT_DEFAULT_URI,         //Broker URI WE NEED TO GET THE BROKER FROM SOMEWHERE
        MQTT_DEFAULT_PORT,                      //Broker port
        (char*) "",                              //User name
        (char*)"",                              //Password
        (char*)MQTT_DEFAULT_CLIENT_ID,          //Client ID
        MQTT_DEFAULT_QOS,                       //QoS
        MQTT_DEFAULT_KEEP_ALIVE_S,              //Keep alive time
        MQTT_DEFAULT_PINGPERIOD_S,              //Ping period
        MQTT_RETAIN);                           //Default retain
    mqtt::regStatusCallback(&onMqttChangeHelper, (const void*)this);
    Log.notice("decoder::init: Waiting for MQTT connection" CR);
    uint8_t i = 0;
    while (mqtt::getOpState() != MQTT_CONNECTED) {
        if (i++ >= MQTT_CONNECT_TIMEOUT_S * 2)
            panic("decoder::init: Could not connect to MQTT broker - rebooting...");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    unSetOpState(OP_DISCONNECTED);
    Log.notice("decoder::init: MQTT connected");
    Log.notice("decoder::init: Waiting for discovery process" CR);
    i = 0;
    while (mqtt::getBrokerUri() == NULL) {
        if (i++ >= DECODER_DISCOVERY_TIMEOUT_S * 2)
            panic("decoder::init: Discovery process failed - rebooting...");
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    unSetOpState(OP_UNDISCOVERED);
    Log.notice("decoder::init: Discovered" CR);
    Log.notice("decoder::init: Creating lgLinks" CR);
    for (uint8_t lgLinkNo = 0; lgLinkNo < MAX_LGLINKS; lgLinkNo++) {
        lgLinks[lgLinkNo] = new lgLink(lgLinkNo);
        if (lgLinks[lgLinkNo] == NULL)
            panic("decoder::init: Could not create lgLink objects - rebooting...");
        lgLinks[lgLinkNo]->init();
    }
    Log.notice("decoder::init: Creating satLinks");
    for (uint8_t satLinkNo = 0; satLinkNo < MAX_SATLINKS; satLinkNo++) {
        satLinks[satLinkNo] = new satLink(satLinkNo);
        if (satLinks[satLinkNo] == NULL)
            panic("decoder::init: Could not create satLink objects - rebooting...");
        satLinks[satLinkNo]->init();
    }
    Log.notice("decoder::init: Subscribing to decoder configuration topic and sending configuration request");
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s", MQTT_CONFIG_RESP_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::subscribeTopic(subscribeTopic, &onConfigHelper, this))
        panic("decoder::init: Failed to suscribe to configuration response topic - rebooting...");
    char publishTopic[300];
    sprintf(publishTopic, "%s%s%s", MQTT_CONFIG_REQ_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::sendMsg(publishTopic, MQTT_CONFIG_REQ_PAYLOAD, false))
        panic("decoder::init: Failed to send configuration request - rebooting...");
    Log.notice("decoder::init: Waiting for configuration ");
    i = 0;
    while (getOpState() & OP_UNCONFIGURED) {
        if (i++ >= DECODER_CONFIG_TIMEOUT_S)
            panic("decoder::init: Did not receive configuration - rebooting...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        Log.notice("decoder::init: Still no decoder configuration, waiting another % d s" CR, DECODER_CONFIG_TIMEOUT_S - i);
    }
    Log.notice("decoder::init: Got valid configuration");
    Log.notice("decoder::init: Initialized");
    return RC_OK;
}

void decoder::onConfigHelper(const char* p_topic, const char* p_payload, const void* p_decoderObj) {
    ((decoder*)p_decoderObj)->onConfig(p_topic, p_payload);
}

void decoder::onConfig(const char* p_topic, const char* p_payload) {
    if (!(getOpState() & OP_UNCONFIGURED))
        panic("decoder:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("decoder::onConfig: Received an uverified configuration, parsing and validating it..." CR);
    xmlConfigDoc = new tinyxml2::XMLDocument;
    if (xmlConfigDoc->Parse(p_payload))
        panic("decoder::onConfig: Configuration parsing failed - Rebooting...");
    if (xmlConfigDoc->FirstChildElement("genJMRI") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("SystemName") == NULL)
        panic("decoder::onConfig: Failed to parse the configuration - xml is missformatted - rebooting...");
    const char* decoderSearchTags[14];
    decoderSearchTags[XML_DECODER_MQTT_URI] = "DecoderMqttURI";
    decoderSearchTags[XML_DECODER_MQTT_PORT] = "DecoderMqttPort";
    decoderSearchTags[XML_DECODER_MQTT_PREFIX] = "DecoderMqttTopicPrefix";
    decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = "DecoderKeepAlivePeriod";
    decoderSearchTags[XML_DECODER_NTPURI] = "NTPServer";
    decoderSearchTags[XML_DECODER_NTPPORT] = "NTPPort";
    decoderSearchTags[XML_DECODER_TZ] = "TIMEZONE";
    decoderSearchTags[XML_DECODER_LOGLEVEL] = "LogLevel";
    decoderSearchTags[XML_DECODER_FAILSAFE] = "DecodersFailSafe";
    decoderSearchTags[XML_DECODER_SYSNAME] = "SystemName";
    decoderSearchTags[XML_DECODER_USRNAME] = "UserName";
    decoderSearchTags[XML_DECODER_DESC] = "Description";
    decoderSearchTags[XML_DECODER_MAC] = "MAC";
    decoderSearchTags[XML_DECODER_URI] = "URI";
    Log.notice("decoder::onConfig: Parsing decoder configuration:" CR);
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI"), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder"), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability
    if (xmlconfig[XML_DECODER_SYSNAME] == NULL)
        panic("decoder::onConfig: System name was not provided - rebooting...");
        Log.notice("decoder::onConfig: Decoder System name: %s" CR, xmlconfig[XML_DECODER_SYSNAME]);
    if (xmlconfig[XML_DECODER_USRNAME] == NULL)
        Log.notice("decoder::onConfig: User name was not provided - using \"\"");
    else
        Log.notice("decoder::onConfig: User name: %s" CR, xmlconfig[XML_DECODER_USRNAME]);
    if (xmlconfig[XML_DECODER_DESC] == NULL)
        Log.notice("decoder::onConfig: Description was not provided - using \"\"");
    else
        Log.notice("decoder::onConfig: Description: %s" CR, xmlconfig[XML_DECODER_DESC]);
    if (!(xmlconfig[XML_DECODER_URI] != NULL) && strcmp(xmlconfig[XML_DECODER_URI], mqtt::getDecoderUri()))
        panic("Configuration decoder URI not the same as provided with the discovery response - rebooting ...");
        Log.notice("decoder::onConfig: Decoder URI: %s" CR, xmlconfig[XML_DECODER_URI]);
    if (!(xmlconfig[XML_DECODER_MAC] != NULL) && strcmp(xmlconfig[XML_DECODER_URI], networking::getMac()))
        panic("Configuration decoder MAC not the same as the physical MAC for this decoder - rebooting ...");
    Log.notice("decoder::onConfig: MQTT URI: %s" CR, xmlconfig[XML_DECODER_MQTT_URI]);
    Log.notice("decoder::onConfig: MQTT Port: %s" CR, xmlconfig[XML_DECODER_MQTT_PORT]);
    Log.notice("decoder::onConfig: MQTT Prefix: %s" CR, xmlconfig[XML_DECODER_MQTT_PREFIX]);
    Log.notice("decoder::onConfig: MQTT Keep-alive period: %s" CR, xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
    Log.notice("decoder::onConfig: NTP URI: %s - NOT IMPLEMENTED" CR, xmlconfig[XML_DECODER_NTPURI]);
    Log.notice("decoder::onConfig: NTP Port: %s:" CR, xmlconfig[XML_DECODER_NTPPORT]);
    Log.notice("decoder::onConfig: TimeZone: %s" CR, xmlconfig[XML_DECODER_TZ]);
    Log.notice("decoder::onConfig: Log-level: %s" CR, xmlconfig[XML_DECODER_LOGLEVEL]);
    Log.notice("decoder::onConfig: Decoder fail-safe: %s" CR, xmlconfig[XML_DECODER_FAILSAFE]);
    Log.notice("decoder::onConfig: Decoder MAC: %s" CR, xmlconfig[XML_DECODER_MAC]);
    Log.notice("decoder::onConfig: Successfully parsed the decoder top-configuration:" CR);
    mqtt::setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]));
    mqtt::reConnect();
    // RESET NTP CONFIGURATION
    // RESET TZ CONFIGURATION
    if (transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]) == RC_GEN_ERR) {
        Log.error("decoder::onConfig: Log-level %s as provided in the decoder configuration is not supported, using default: %s" CR, xmlconfig[XML_DECODER_LOGLEVEL], transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
        delete xmlconfig[XML_DECODER_LOGLEVEL];
        xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
    }
    Log.setLevel(transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]));
    Log.notice("decoder::onConfig: Successfully set the decoder top-configuration:" CR);
    tinyxml2::XMLElement* lgLinkXmlElement;
    lgLinkXmlElement = xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("LightgroupsLink");
    const char* lgLinkSearchTags[4];
    char* lgLinkXmlConfig[4];
    for (int lgLinkItter = 0; lgLinkItter <= MAX_LGLINKS; lgLinkItter++) {
        if (lgLinkXmlElement == NULL)
            break;
        if (lgLinkItter >= MAX_LGLINKS)
            panic("decoder::onConfig: > than maximum lgLinks provided - not supported, rebooting...");
            lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
        getTagTxt(lgLinkXmlElement, lgLinkSearchTags, lgLinkXmlConfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
        if (!lgLinkXmlConfig[XML_LGLINK_LINK])
            panic("decoder::onConfig:: lgLink missing - rebooting..." CR);
        lgLinks[atoi(lgLinkXmlConfig[XML_LGLINK_LINK])]->onConfig(lgLinkXmlElement);
        addSysStateChild(lgLinks[atoi(lgLinkXmlConfig[XML_LGLINK_LINK])]);
        lgLinkXmlElement = xmlConfigDoc->NextSiblingElement("LightgroupsLink");
    }
    tinyxml2::XMLElement* satLinkXmlElement;
    satLinkXmlElement = xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("SateliteLink");
    const char* satLinkSearchTags[4];
    char* satLinkXmlconfig[4];
    for (int satLinkItter = 0; satLinkItter <= MAX_SATLINKS; satLinkItter++) {
        if (satLinkXmlElement == NULL)
            break;
        if (satLinkItter >= MAX_SATLINKS)
            panic("decoder::onConfig: > than maximum satLinks provided - not supported - rebooting...");
        satLinkSearchTags[XML_SATLINK_LINK] = "Link";
        getTagTxt(satLinkXmlElement, satLinkSearchTags, xmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
        if (!satLinkXmlconfig[XML_SATLINK_LINK])
            panic("decoder::onConfig:: satLink missing - rebooting..." CR);
        satLinks[atoi(xmlconfig[XML_SATLINK_LINK])]->onConfig(satLinkXmlElement);
        addSysStateChild(satLinks[atoi(xmlconfig[XML_SATLINK_LINK])]);
        satLinkXmlElement = xmlConfigDoc->NextSiblingElement("SateliteLink");
    }
    delete xmlConfigDoc;
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("decoder::onConfig: Configuration successfully finished" CR);
}

rc_t decoder::start(void) {
Log.notice("decoder::start: Starting decoder" CR);
uint8_t i = 0;
while (getOpState() & OP_UNCONFIGURED) {
    if (i == 0)
        Log.notice("decoder::start: Waiting for decoder to be configured before it can start" CR);
    if (i++ >= 120)
        panic("decoder::start: Discovery process failed - rebooting...");
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
}
Log.notice("decoder::start: Subscribing to adm- and op state topics");
const char* admSubscribeTopic[4] = { MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getDecoderUri(), "/", getSystemName() };
if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 4), onAdmStateChangeHelper, NULL))
    panic("decoder::start: Failed to suscribe to admState topic - rebooting...");
const char* opSubscribeTopic[4] = { MQTT_DECODER_OPSTATE_TOPIC, mqtt::getDecoderUri(), "/", getSystemName() };
if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 4), onOpStateChangeHelper, NULL))
    panic("decoder::start: Failed to suscribe to opState topic - rebooting...");
Log.notice("decoder::start: Starting lightgroup link Decoders" CR);
for (int lgLinkItter = 0; lgLinkItter < MAX_LGLINKS; lgLinkItter++) {
    if (lgLinks[lgLinkItter] == NULL)
        break;
    lgLinks[lgLinkItter]->start();
}
Log.notice("decoder::start: Starting satelite link Decoders" CR);
for (int satLinkItter = 0; satLinkItter < MAX_LGLINKS; satLinkItter++) {
    if (satLinks[satLinkItter] == NULL)
        break;
    satLinks[satLinkItter]->start();
}
unSetOpState(OP_INIT);
Log.notice("decoder::start: decoder started" CR);
}

void decoder::onSysStateChangeHelper(const void* p_miscData, uint16_t p_sysState) {
    ((decoder*)p_miscData)->onSysStateChange(p_sysState);
}

void decoder::onSysStateChange(uint16_t p_sysState) {
    if (p_sysState & OP_INTFAIL)
        panic("decoder::onSystateChange: decoder has experienced an internal error - rebooting...");
    if ((p_sysState & OP_DISCONNECTED) && !(p_sysState & OP_INIT))
        panic("decoder::onSystateChange: decoder has been disconnected after initialization phase- rebooting...");
    if (p_sysState & ~OP_DISABLED) {
        Log.notice("decoder::onSystateChange: decocoder is marked fault free by server (OP_DISABLED not concidered), opState bitmap: %b - enabling mqtt supervision" CR, p_sysState);
        if (mqtt::up())
            panic("decoder::onSystateChange: Could not enable mqtt supervision - rebooting");
    }
    else {
        Log.notice("decoder::onSystateChange: decocoder is marked faulty by server (OP_DISABLED not concidered), opState bitmap: %b - enabling mqtt supervision" CR, p_sysState);
        mqtt::down();
    }
}

void decoder::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onOpStateChange(p_topic, p_payload);
}

void decoder::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("decoder::onOpStateChange: got available message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("decoder::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.error("decoder::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void decoder::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onAdmStateChange(p_topic, p_payload);
}

void decoder::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("decoder::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("decoder::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.error("decoder::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

void decoder::onMqttChangeHelper(uint8_t p_mqttState, const void* p_decoderObj) {
    ((decoder*)p_decoderObj)->onMqttChange(p_mqttState);
}

void decoder::onMqttChange(uint8_t p_mqttStatus) {
    if (p_mqttStatus == MQTT_CONNECTED)
        unSetOpState(OP_DISCONNECTED);
    else
        setOpState(OP_DISCONNECTED);
}

rc_t decoder::setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force) {
    if (!debug || !p_force)
        Log.error("decoder::setMqttBrokerURI: cannot set MQTT URI as debug is inactive" CR);
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttURI: cannot set MQTT URI as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMqttBrokerURI: setting MQTT URI to %s" CR, p_mqttBrokerURI);
        delete xmlconfig[XML_DECODER_MQTT_URI];
        xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(p_mqttBrokerURI);
        mqtt::setBrokerUri(xmlconfig[XML_DECODER_MQTT_URI]);
        mqtt::reConnect();
        return RC_OK;
    }
}

const char* decoder::getMqttBrokerURI(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttBrokerURI: cannot get MQTT URI as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_URI];
}

rc_t decoder::setMqttPort(const uint16_t p_mqttPort, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setMqttPort: cannot set MQTT Port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttPort: cannot set MQTT port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMqttPort: setting MQTT Port to %s" CR, p_mqttPort);
        delete xmlconfig[XML_DECODER_MQTT_PORT];
        xmlconfig[XML_DECODER_MQTT_PORT] = createNcpystr(itoa(p_mqttPort, NULL,  10));
        mqtt::setBrokerPort(atoi(xmlconfig[XML_DECODER_MQTT_PORT]));
        mqtt::reConnect();
        return RC_OK;
    }
}

uint16_t decoder::getMqttPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttPort: cannot get MQTT port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_PORT]);
}

rc_t decoder::setMqttPrefix(const char* p_mqttPrefix, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setMqttPrefix: cannot set MQTT Prefix as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttPrefix: cannot set MQTT prefix as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMqttPrefix: setting MQTT Prefix to %s" CR, p_mqttPrefix);
        delete xmlconfig[XML_DECODER_MQTT_PREFIX];
        xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(p_mqttPrefix);
        return RC_OK;
    }
}

const char* decoder::getMqttPrefix(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttPrefix: cannot get MQTT prefix as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_PREFIX];
}

rc_t decoder::setKeepAlivePeriod(const float p_keepAlivePeriod, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setKeepAlivePeriod: cannot set keep-alive period as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setKeepAlivePeriod: cannot set keep-alive period as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setKeepAlivePeriod: setting keep-alive period to %f" CR, p_keepAlivePeriod);
        sprintf(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD], "%f", p_keepAlivePeriod);
        mqtt::setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]));
        return RC_OK;
    }
}

float decoder::getKeepAlivePeriod(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getKeepAlivePeriod: cannot get keep-alive period as decoder is not configured" CR);
        return RC_GEN_ERR;
    }
    return atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
}

rc_t decoder::setNtpServer(const char* p_ntpServer, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setNtpServer: cannot set NTP server as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setNtpServer: cannot set NTP server as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setNtpServer: cannot set NTP server - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getNtpServer(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getNtpServer: cannot get NTP server as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_NTPURI];
}

rc_t decoder::setNtpPort(const uint16_t p_ntpPort, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setNtpPort: cannot set NTP port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setNtpPort: cannot set NTP port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setNtpPort: cannot set NTP port - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

uint16_t decoder::getNtpPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getNtpPort: cannot get NTP port as decoder is not configured" CR);
        return 0;
    }
    return atoi(xmlconfig[XML_DECODER_NTPPORT]);
}

rc_t decoder::setTz(const uint8_t p_tz, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setTz: cannot set Time zone as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setTz: cannot set time-zone as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setTz: cannot set Time zone - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

uint8_t decoder::getTz(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getTz: cannot get Time-zone as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_TZ]);
}

rc_t decoder::setLogLevel(const char* p_logLevel, bool p_force) {
    if (!debug || !p_force) {
        Log.error("decoder::setLogLevel: cannot set Log level as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setLogLevel: cannot set log-level as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        if (transformLogLevelXmlStr2Int(p_logLevel) == RC_GEN_ERR) {
            Log.error("decoder::setLogLevel: cannot set Log-level %s, log-level not supported, using current log-level: %s" CR, p_logLevel, xmlconfig[XML_DECODER_LOGLEVEL]);
            return RC_GEN_ERR;
        }
        Log.notice("decoder::setLogLevel: Setting Log-level to %s" CR, p_logLevel);
        delete xmlconfig[XML_DECODER_LOGLEVEL];
        xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(p_logLevel);
    }
    Log.setLevel(transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]));
    return RC_OK;
}

const char* decoder::getLogLevel(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getLogLevel: cannot get log-level as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_LOGLEVEL];
}

rc_t decoder::setFailSafe(const bool p_failsafe, bool p_force) {
    if (!debug || !p_force) {
        Log.notice("decoder::setFailSafe: cannot set Fail-safe as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setFailSafe: cannot set fail-safe as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        delete xmlconfig[XML_DECODER_FAILSAFE];
        if (p_failsafe) {
            Log.notice("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_TRUE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_TRUE_PAYLOAD);
        }
        else {
            Log.notice("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_FALSE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_FALSE_PAYLOAD);
        }
    }
    return RC_OK;
}

bool decoder::getFailSafe(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getFailSafe: cannot get fail-safe as decoder is not configured" CR);
        return false;
    }
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_TRUE_PAYLOAD))
        return true;
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_FALSE_PAYLOAD))
        return false;
}

rc_t decoder::setSystemName(const char* p_systemName, bool p_force) {
    Log.error("decoder::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* decoder::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getSystemName: cannot get System name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_SYSNAME];
}

rc_t decoder::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.notice("decoder::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setUsrName: cannot set User name as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_DECODER_USRNAME];
        xmlconfig[XML_DECODER_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* decoder::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getUsrName: cannot get User name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_USRNAME];
}

rc_t decoder::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.notice("decoder::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setDesc: cannot set Description as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_DECODER_DESC];
        xmlconfig[XML_DECODER_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* decoder::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getDesc: cannot get Description as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_DESC];
}

rc_t decoder::setMac(const char* p_mac, bool p_force) {
    if (!debug || !p_force) {
        Log.notice("decoder::setMac: cannot set MAC as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMac: cannot set MAC as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMac: cannot set MAC - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getMac(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMac: cannot get MAC as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MAC];
}

rc_t decoder::setDecoderUri(const char* p_decoderUri, bool p_force) {
    if (!debug || !p_force) {
        Log.notice("decoder::setDecoderUri: cannot set decoder URI as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setDecoderUri: cannot set Decoder URI as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setDecoderUri: cannot set Decoder URI - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getDecoderUri(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getDecoderUri: cannot get Decoder URI as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_URI];
}

void decoder::setDebug(bool p_debug) {
    debug = p_debug;
}

bool decoder::getDebug(void) {
    return debug;
}

/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/
