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
decoder::decoder(void) : systemState(NULL), globalCli(DECODER_MO_NAME, DECODER_MO_NAME, 0, true) {
    Log.INFO("decoder::decoder: Creating decoder" CR);
    setSysStateObjName("Decoder");
    Serial.printf("decoder: Free Heap: %i\n", esp_get_free_heap_size());
    Serial.printf("Decoder: Heap watermark: %i\n", esp_get_minimum_free_heap_size());
    debug = false;
    Log.INFO("decoder::init: Creating lgLinks" CR);
    for (uint8_t lgLinkNo = 0; lgLinkNo < MAX_LGLINKS; lgLinkNo++) {
        lgLinks[lgLinkNo] = new lgLink(lgLinkNo, this);
        if (lgLinks[lgLinkNo] == NULL)
            panic("decoder::init: Could not create lgLink objects - rebooting..." CR);
        addSysStateChild(lgLinks[lgLinkNo]);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    Log.INFO("decoder::init: lgLinks created" CR);
    Log.INFO("decoder::init: Creating satLinks" CR);
    for (uint8_t satLinkNo = 0; satLinkNo < MAX_SATLINKS; satLinkNo++) {
        satLinks[satLinkNo] = new satLink(satLinkNo, this);
        if (satLinks[satLinkNo] == NULL)
            panic("decoder::init: Could not create satLink objects - rebooting..." CR);
        Log.VERBOSE("Added Satlink index %d with object %d" CR, satLinkNo, satLinks[satLinkNo]);
        addSysStateChild(satLinks[satLinkNo]);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    Log.INFO("decoder::init: satLinks created" CR);
    mqtt::create();
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_UNDISCOVERED | OP_UNCONFIGURED | OP_DISABLED | OP_CBL);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    if (!(decoderLock = xSemaphoreCreateMutex()))
        panic("decoder::decoder: Could not create Lock objects - rebooting..." CR);
    xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(networking::getMqttUri());
    xmlconfig[XML_DECODER_MQTT_PORT] = new char[6];
    xmlconfig[XML_DECODER_MQTT_PORT] = itoa(networking::getMqttPort(), xmlconfig[XML_DECODER_MQTT_PORT], 10);
    xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(MQTT_PRE_TOPIC_DEFAULT_FRAGMENT);
    xmlconfig[XML_DECODER_MQTT_PINGPERIOD] = new char[6];
    xmlconfig[XML_DECODER_MQTT_PINGPERIOD] = itoa(MQTT_DEFAULT_PINGPERIOD_S, xmlconfig[XML_DECODER_MQTT_PINGPERIOD], 10);
    xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = new char[6];
    xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = itoa(MQTT_DEFAULT_KEEP_ALIVE_S, xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD], 10);
    xmlconfig[XML_DECODER_NTPURI] = createNcpystr(NTP_DEFAULT_URI); //SHOULD THIS ORIGINATE FROM networking
    xmlconfig[XML_DECODER_NTPPORT] = new char[6];
    xmlconfig[XML_DECODER_NTPPORT] = itoa(NTP_DEFAULT_PORT, xmlconfig[XML_DECODER_NTPPORT], 10); //SHOULD THIS ORIGINATE FROM networking
    xmlconfig[XML_DECODER_TZ_AREA] = createNcpystr(NTP_DEFAULT_TZ_AREA);
    xmlconfig[XML_DECODER_TZ_GMTOFFSET] = new char[4];
    xmlconfig[XML_DECODER_TZ_GMTOFFSET] = itoa(NTP_DEFAULT_TZ_GMTOFFSET, xmlconfig[XML_DECODER_TZ_GMTOFFSET], 10);
    xmlconfig[XML_DECODER_LOGLEVEL] = new char[2];
    xmlconfig[XML_DECODER_LOGLEVEL] = itoa(DEFAULT_LOGLEVEL, xmlconfig[XML_DECODER_LOGLEVEL], 10);
    xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(DEFAULT_FAILSAFE);
    xmlconfig[XML_DECODER_SYSNAME] = NULL;
    xmlconfig[XML_DECODER_USRNAME] = NULL;
    xmlconfig[XML_DECODER_DESC] = NULL;
    xmlconfig[XML_DECODER_MAC] = createNcpystr(networking::getMac());
    xmlconfig[XML_DECODER_URI] = NULL;
    xmlconfig[XML_DECODER_ADMSTATE] = NULL;

    Log.INFO("decoder::decoder: Starting CLI service" CR);
    //globalCli::start();
    Log.INFO("decoder::decoder: Decoder created" CR);
}

decoder::~decoder(void) {
    panic("decoder::~decoder: decoder destructor not supported - rebooting..." CR);
}

rc_t decoder::init(void){
    Log.INFO("decoder::init: Initializing decoder" CR);
    Log.INFO("decoder::init: Initializing MQTT " CR);
    strcpy(xmlconfig[XML_DECODER_MQTT_URI], networking::getMqttUri());
    itoa(networking::getMqttPort(), xmlconfig[XML_DECODER_MQTT_PORT], 10);
    mqtt::regOpStateCb(onMqttOpStateChangeHelper, this);
    mqtt::init((char*)xmlconfig[XML_DECODER_MQTT_URI],          // Broker URI
        atoi(xmlconfig[XML_DECODER_MQTT_PORT]),                 // Broker port
        (char*)"",                                              // User name
        (char*)"",                                              // Password
        (char*)MQTT_DEFAULT_CLIENT_ID,                          // Client ID
        MQTT_DEFAULT_QOS,                                       // QoS
        MQTT_DEFAULT_KEEP_ALIVE_S,                              // Keep alive time
        MQTT_DEFAULT_PINGPERIOD_S,                              // Ping period
        MQTT_RETAIN);                                           // Default retain
    unSetOpStateByBitmap(OP_UNDISCOVERED);
    Log.INFO("decoder::init: Initializing lgLinks" CR);
    for (uint8_t lgLinkNo = 0; lgLinkNo < MAX_LGLINKS; lgLinkNo++) {
        lgLinks[lgLinkNo]->init();
    }
    Log.INFO("decoder::init: lgLinks initialized" CR);
    Log.INFO("decoder::init: Initializing satLinks" CR);
    for (uint8_t satLinkNo = 0; satLinkNo < MAX_SATLINKS; satLinkNo++) {
        satLinks[satLinkNo]->init();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    Log.INFO("decoder::init: satLinks initialized" CR);
    Log.INFO("decoder::init: Subscribing to decoder configuration topic and sending configuration request" CR);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s", MQTT_CONFIG_RESP_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::subscribeTopic(subscribeTopic, &onConfigHelper, this))
        panic("decoder::init: Failed to suscribe to configuration response topic - rebooting..." CR);
    char publishTopic[300];
    sprintf(publishTopic, "%s%s%s", MQTT_CONFIG_REQ_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::sendMsg(publishTopic, MQTT_CONFIG_REQ_PAYLOAD, false))
        panic("decoder::init: Failed to send configuration request - rebooting..." CR);
    Log.INFO("decoder::init: Waiting for configuration ..." CR);
    uint16_t i = 0;
    while (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        if (i++ >= DECODER_CONFIG_TIMEOUT_S)
            panic("decoder::init: Did not receive configuration - rebooting..." CR);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    Log.INFO("decoder::init: Got valid configuration" CR);
    Log.INFO("decoder::init: Initialized" CR);
    return RC_OK;
}

void decoder::onConfigHelper(const char* p_topic, const char* p_payload, const void* p_decoderObj) {
    ((decoder*)p_decoderObj)->onConfig(p_topic, p_payload);
}

void decoder::onConfig(const char* p_topic, const char* p_payload) {
    //CONFIG PARSING
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        panic("decoder:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting..." CR);
    Log.INFO("decoder::onConfig: Received an uverified configuration, parsing and validating it..." CR);
    xmlConfigDoc = new tinyxml2::XMLDocument;
    if (xmlConfigDoc->Parse(p_payload))
        panic("decoder::onConfig: Configuration parsing failed - Rebooting..." CR);
    if (xmlConfigDoc->FirstChildElement("genJMRI") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("SystemName") == NULL)
        panic("decoder::onConfig: Failed to parse the configuration - xml is missformatted - rebooting..." CR);

    //PARSING CONFIGURATION
    const char* decoderSearchTags[17];
    decoderSearchTags[XML_DECODER_MQTT_URI] = "DecoderMqttURI";
    decoderSearchTags[XML_DECODER_MQTT_PORT] = "DecoderMqttPort";
    decoderSearchTags[XML_DECODER_MQTT_PREFIX] = "DecoderMqttTopicPrefix";
    decoderSearchTags[XML_DECODER_MQTT_PINGPERIOD] = "DecoderPingPeriod";
    decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = "DecoderKeepAlivePerid";
    decoderSearchTags[XML_DECODER_NTPURI] = "NTPServer";
    decoderSearchTags[XML_DECODER_NTPPORT] = "NTPPort";
    decoderSearchTags[XML_DECODER_TZ_AREA] = "TimeZoneArea"; //NEW FUNCTIONALITY FOR THE SERVER - we should fail back to CET
    decoderSearchTags[XML_DECODER_TZ_GMTOFFSET] = "TimeZoneGmtOffset";
    decoderSearchTags[XML_DECODER_LOGLEVEL] = "LogLevel";
    decoderSearchTags[XML_DECODER_FAILSAFE] = "DecodersFailSafe";
    decoderSearchTags[XML_DECODER_SYSNAME] = NULL;
    decoderSearchTags[XML_DECODER_USRNAME] = NULL;
    decoderSearchTags[XML_DECODER_DESC] = NULL;
    decoderSearchTags[XML_DECODER_MAC] = NULL;
    decoderSearchTags[XML_DECODER_URI] = NULL;
    decoderSearchTags[XML_DECODER_ADMSTATE] = NULL;

    Log.INFO("decoder::onConfig: Parsing decoder configuration:" CR);
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement(), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability
    decoderSearchTags[XML_DECODER_MQTT_URI] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PORT] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PREFIX] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PINGPERIOD] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = NULL;
    decoderSearchTags[XML_DECODER_NTPURI] = NULL;
    decoderSearchTags[XML_DECODER_NTPPORT] = NULL;
    decoderSearchTags[XML_DECODER_TZ_AREA] = NULL;
    decoderSearchTags[XML_DECODER_TZ_GMTOFFSET] = NULL;
    decoderSearchTags[XML_DECODER_LOGLEVEL] = NULL;
    decoderSearchTags[XML_DECODER_FAILSAFE] = NULL;
    decoderSearchTags[XML_DECODER_SYSNAME] = "SystemName";
    decoderSearchTags[XML_DECODER_USRNAME] = "UserName";
    decoderSearchTags[XML_DECODER_DESC] = "Description";
    decoderSearchTags[XML_DECODER_MAC] = "MAC";
    decoderSearchTags[XML_DECODER_URI] = "URI";
    decoderSearchTags[XML_DECODER_ADMSTATE] = "AdminState";
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement(), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    Log.INFO("decoder::onConfig: Validating and setting provided decoder configuration" CR);

    char tz[20];
    sprintf(tz, "%s%+.2i", xmlconfig[XML_DECODER_TZ_AREA], atoi(xmlconfig[XML_DECODER_TZ_AREA]));
    bool failSafe;
    if(!strcmp(xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_TRUE_PAYLOAD))
        failSafe=true;
    else if(!strcmp(xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_FALSE_PAYLOAD))
        failSafe = false;
    else{
        failSafe = true;
        Log.ERROR("decoder::onConfig: Provided Failsafe statement: %s is missformatted - only \"%s\" or \"%s\" allowed - activating Failsafe" CR, xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_TRUE_PAYLOAD, MQTT_BOOL_FALSE_PAYLOAD);
    }

/* NEED TO FIX CHANGES OF MQTT PARAMETERS
    if (setMqttBrokerURI(xmlconfig[XML_DECODER_MQTT_URI], true) ||
        setMqttPort(atoi(xmlconfig[XML_DECODER_MQTT_PORT]), true) ||
        setMqttPrefix(xmlconfig[XML_DECODER_MQTT_PREFIX], true) ||
        setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]), true) ||
        setNtpServer(xmlconfig[XML_DECODER_NTPURI], true) ||
        setNtpPort(atoi(xmlconfig[XML_DECODER_NTPPORT]), true) ||
        setLogLevel(xmlconfig[XML_DECODER_LOGLEVEL], true) ||
        setTz(tz, true) ||
        setFailSafe(failSafe, true))
        panic("decoder::onConfig: Could not validate and set provided configuration - rebooting..." CR);
*/
/*
    if (setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]), true) ||
        setNtpServer(xmlconfig[XML_DECODER_NTPURI], true) ||
        setNtpPort(atoi(xmlconfig[XML_DECODER_NTPPORT]), true) ||
        setLogLevel(xmlconfig[XML_DECODER_LOGLEVEL], true) ||
        setTz(tz, true) ||
        setFailSafe(failSafe, true))
            panic("decoder::onConfig: Could not validate and set provided configuration - rebooting..." CR);
*/
    if (xmlconfig[XML_DECODER_SYSNAME] == NULL)
        panic("decoder::onConfig: System name was not provided - rebooting..." CR);
    if (xmlconfig[XML_DECODER_USRNAME] == NULL){
        Log.WARN("decoder::onConfig: User name was not provided - using %s-UserName" CR, xmlconfig[XML_DECODER_SYSNAME]);
        xmlconfig[XML_DECODER_USRNAME] = new char[strlen(xmlconfig[XML_DECODER_SYSNAME]) + 10];
        const char* usrName[2] = { xmlconfig[XML_DECODER_SYSNAME], "-" };
        strcpy(xmlconfig[XML_DECODER_USRNAME], "-");
    }
    if (xmlconfig[XML_DECODER_DESC] == NULL){
        Log.WARN("decoder::onConfig: Description was not provided - using \"-\"" CR);
        xmlconfig[XML_DECODER_DESC] = new char[2];
        strcpy(xmlconfig[XML_DECODER_DESC], "-");
    }
    if (strcmp(xmlconfig[XML_DECODER_MAC], networking::getMac()))
        panic("decoder::onConfig: Provided MAC: %s is inconsistant with the decoders Physical MAC %s  - rebooting..." CR, xmlconfig[XML_DECODER_MAC], networking::getMac());
    if (xmlconfig[XML_DECODER_URI] == NULL)
        xmlconfig[XML_DECODER_URI] = createNcpystr(mqtt::getDecoderUri());
    if(strcmp(xmlconfig[XML_DECODER_URI], mqtt::getDecoderUri()))
        panic("decoder::onConfig: Configuration decoder URI is not the same as provided with the discovery response - rebooting ..." CR);
    //setSysStateObjName(xmlconfig[XML_DECODER_URI]); FIX
    if (xmlconfig[XML_DECODER_ADMSTATE] == NULL){
        Log.WARN("decoder::onConfig: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR);
        xmlconfig[XML_DECODER_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_DECODER_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_DECODER_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else
        panic("decoder::onConfig: Configuration decoder::onConfig: Admin state: %s is none of \"ENABLE\" or \"DISABLE\" - rebooting..." CR, xmlconfig[XML_DECODER_ADMSTATE]);

    //SHOW FINAL CONFIGURATION
    Log.INFO("decoder::onConfig: Successfully set the decoder top-configuration as follows:" CR);
    Log.INFO("decoder::onConfig: MQTT Server: \"%s:%s\"" CR, xmlconfig[XML_DECODER_MQTT_URI], xmlconfig[XML_DECODER_MQTT_PORT]);
    Log.INFO("decoder::onConfig: MQTT Prefix: %s - NOTE: CHANGING MQTT PREFIX CURRENTLY NOT SUPPORTED, will do nothing, default MQTT prefix: %s remains active" CR, xmlconfig[XML_DECODER_MQTT_PREFIX], MQTT_PRE_TOPIC_DEFAULT_FRAGMENT);
    Log.INFO("decoder::onConfig: MQTT Ping-period: %s" CR, xmlconfig[XML_DECODER_MQTT_PINGPERIOD]);
    Log.INFO("decoder::onConfig: NTP Server: \"%s:%s\"" CR, xmlconfig[XML_DECODER_NTPURI], xmlconfig[XML_DECODER_NTPPORT]);
    Log.INFO("decoder::onConfig: Time-zone: \"%s\"" CR, tz);
    Log.INFO("decoder::onConfig: Log-level: %s" CR, xmlconfig[XML_DECODER_LOGLEVEL]);
    Log.INFO("decoder::onConfig: Decoder fail-safe: %s" CR, xmlconfig[XML_DECODER_FAILSAFE]);
    Log.INFO("decoder::onConfig: Decoder MAC: %s" CR, xmlconfig[XML_DECODER_MAC]);
    Log.INFO("decoder::onConfig: Decoder URI: %s" CR, xmlconfig[XML_DECODER_URI]);
    Log.INFO("decoder::onConfig: Decoder System name: %s" CR, xmlconfig[XML_DECODER_SYSNAME]);
    Log.INFO("decoder::onConfig: Decoder User name: %s" CR, xmlconfig[XML_DECODER_USRNAME]);
    Log.INFO("decoder::onConfig: Decoder Description: %s" CR, xmlconfig[XML_DECODER_DESC]);
    Log.INFO("decoder::onConfig: Decoder Admin state: %s" CR, xmlconfig[XML_DECODER_ADMSTATE]);
    Log.INFO("decoder::onConfig: Successfully parsed and processed the decoder top-configuration:" CR);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    //CONFIFIGURING LIGHTGROUP LINKS
    Log.INFO("decoder::onConfig: Validating and configuring lightgroups links" CR);
    tinyxml2::XMLElement* lgLinkXmlElement;
    if (lgLinkXmlElement = ((tinyxml2::XMLElement*)xmlConfigDoc)->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("LightgroupsLink")){
        const char* lgLinkSearchTags[4];
        lgLinkSearchTags[XML_LGLINK_SYSNAME] = NULL;
        lgLinkSearchTags[XML_LGLINK_USRNAME] = NULL;
        lgLinkSearchTags[XML_LGLINK_DESC] = NULL;
        lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
        for (int lgLinkItter = 0; true; lgLinkItter++) {
            char* lgLinkXmlConfig[4] = { NULL };
            if (lgLinkXmlElement == NULL)
                break;
            if (lgLinkItter >= MAX_LGLINKS)
                panic("decoder::onConfig: > than maximum lgLinks provided (%i/%i) - not supported, rebooting..." CR, lgLinkItter, MAX_LGLINKS);
            getTagTxt(lgLinkXmlElement->FirstChildElement(), lgLinkSearchTags, lgLinkXmlConfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
            if (!lgLinkXmlConfig[XML_LGLINK_LINK])
                panic("decoder::onConfig:: lgLink missing - rebooting..." CR);
            if ((atoi(lgLinkSearchTags[XML_LGLINK_LINK])) < 0 || atoi(lgLinkSearchTags[XML_LGLINK_LINK]) >= MAX_LGLINKS)
                panic("decoder::onConfig:: lgLink number (%i) out of bounds - rebooting..." CR, atoi(lgLinkSearchTags[XML_LGLINK_LINK]));
            lgLinks[atoi(lgLinkXmlConfig[XML_LGLINK_LINK])]->onConfig(lgLinkXmlElement);
            lgLinkXmlElement = ((tinyxml2::XMLElement*)lgLinkXmlElement)->NextSiblingElement("LightgroupsLink");
        }
    }
    else
        Log.INFO("decoder::onConfig: No lgLinks provided, no lgLink will be configured" CR);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    //CONFIFIGURING SATELITE LINKS
    Log.INFO("decoder::onConfig: Validating and configuring satelite links" CR);
    tinyxml2::XMLElement* satLinkXmlElement;
    if (satLinkXmlElement = ((tinyxml2::XMLElement*)xmlConfigDoc)->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("SateliteLink")) {
        const char* satLinkSearchTags[4];
        satLinkSearchTags[XML_SATLINK_SYSNAME] = NULL;
        satLinkSearchTags[XML_SATLINK_USRNAME] = NULL;
        satLinkSearchTags[XML_SATLINK_DESC] = NULL;
        satLinkSearchTags[XML_SATLINK_LINK] = "Link";
        for (int satLinkItter = 0; true; satLinkItter++) {
            char* satLinkXmlconfig[4] = { NULL };
            if (satLinkXmlElement == NULL)
                break;
            if (satLinkItter >= MAX_SATLINKS)
                panic("decoder::onConfig: > than maximum satLinks provided (%i/%i) - not supported - rebooting..." CR, satLinkItter, MAX_SATLINKS);
            getTagTxt(satLinkXmlElement->FirstChildElement(), satLinkSearchTags, satLinkXmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
            if (!satLinkXmlconfig[XML_SATLINK_LINK])
                panic("decoder::onConfig:: satLink missing - rebooting..." CR);
            if ((atoi(satLinkXmlconfig[XML_SATLINK_LINK])) < 0 || atoi(satLinkXmlconfig[XML_SATLINK_LINK]) >= MAX_SATLINKS)
                panic("decoder::onConfig:: Satelite link number (%i) out of bounds - rebooting..." CR, atoi(satLinkXmlconfig[XML_SATLINK_LINK]));
            satLinks[atoi(satLinkXmlconfig[XML_SATLINK_LINK])]->onConfig(satLinkXmlElement);
            satLinkXmlElement = ((tinyxml2::XMLElement*)satLinkXmlElement)->NextSiblingElement("SateliteLink");
        }
    }
    else
        Log.INFO("decoder::onConfig: No satLinks provided, no satelites will be configured" CR);
    delete xmlConfigDoc;
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    Log.INFO("decoder::onConfig: Configuration successfully finished" CR);
}

rc_t decoder::start(void) {
    Log.INFO("decoder::start: Starting decoder" CR);
    uint16_t i = 0;
    while (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        if (i == 0)
            Log.INFO("decoder::start: Waiting for decoder to be configured before it can start" CR);
        if (i++ >= DECODER_CONFIG_TIMEOUT_S * 10)
            panic("decoder::start: Configuration process failed - rebooting..." CR);
        Serial.print('.');
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    Log.INFO("decoder::start: Subscribing to adm- and op state topics" CR);
    char admopSubscribeTopic[300];
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_DECODER_ADMSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), getSystemName());
    if (mqtt::subscribeTopic(admopSubscribeTopic, onAdmStateChangeHelper, this))
        panic("decoder::start: Failed to suscribe to admState topic - rebooting..." CR);
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_DECODER_OPSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), getSystemName());
    if (mqtt::subscribeTopic(admopSubscribeTopic, onOpStateChangeHelper, this))
        panic("decoder::start: Failed to suscribe to opState topic - rebooting..." CR);
    Log.INFO("decoder::start: Starting lightgroup link Decoders" CR);
    for (int lgLinkItter = 0; lgLinkItter < MAX_LGLINKS; lgLinkItter++) {
        Log.TERSE("decoder::start: Starting LgLink-%d" CR, lgLinkItter);
        if (lgLinks[lgLinkItter] == NULL)
            panic("decoder::start: lgLink - %d does not exist - rebooting..." CR, lgLinkItter);
        lgLinks[lgLinkItter]->start();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }

Log.INFO("decoder::start: Starting satelite link Decoders" CR);
for (int satLinkItter = 0; satLinkItter < MAX_SATLINKS; satLinkItter++) {
    Log.TERSE("decoder::start: Starting SatLink-%d" CR, satLinkItter);
    if (satLinks[satLinkItter] == NULL)
        panic("decoder::start: satLink - %d does not exist - rebooting..." CR, satLinkItter);
    satLinks[satLinkItter]->start();
    vTaskDelay(5 / portTICK_PERIOD_MS);
}
    unSetOpStateByBitmap(OP_INIT);
    Log.INFO("decoder::start: decoder started" CR);
    return RC_OK;
}

void decoder::onSysStateChangeHelper(const void* p_miscData, sysState_t p_sysState) {
    ((decoder*)p_miscData)->onSysStateChange(p_sysState);
}

void decoder::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    Log.INFO("decoder::onSysStateChange: decoder has a new OP-state: %s" CR, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    Serial.printf(">>>>>>>NewState 0x%x PrevState 0x%x Changed 0x%x" CR, newSysState, prevSysState, sysStateChange);
    if (!sysStateChange)
        return;
    if ((sysStateChange & ~(OP_CBL | OP_SERVUNAVAILABLE)) && mqtt::getDecoderUri() && !(newSysState & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_DECODER_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_DECODER_SYSNAME]);
        systemState::getOpStateStrByBitmap(newSysState, publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(newSysState & ~OP_CBL & ~OP_SERVUNAVAILABLE, publishPayload), false);
    }
    if ((sysStateChange & OP_INTFAIL) && (newSysState & OP_INTFAIL)) {
        panic("decoder::onSystateChange: decoder has experienced an internal error - rebooting..." CR);
        prevSysState = newSysState;
        return;
    }
    if (newSysState & OP_INIT) {
        Log.TERSE("decoder::onSystateChange: The decoder is initializing - doing nothing" CR);
        prevSysState = newSysState;
        return;
    }
    if ((sysStateChange & OP_DISCONNECTED) && (newSysState & OP_DISCONNECTED)) {
        Log.ERROR("decoder::onSystateChange: decoder has been disconnected" CR);
        prevSysState = newSysState;
    }
    if ((sysStateChange & OP_INIT) && !(newSysState & OP_DISABLED)) {
        Log.TERSE("decoder::onSystateChange: The decoder have been initialized and is enabled - enabling decoder supervision" CR);
        mqtt::up();
    }
    else if ((sysStateChange & OP_INIT) && (newSysState & OP_DISABLED)) {
        Log.TERSE("decoder::onSystateChange: The decoder have been initialized but is disabled - will not enable decoder supervision" CR);
        mqtt::down();
    }
    if ((sysStateChange & OP_DISABLED) && (newSysState & OP_DISABLED)) {
        Log.INFO("decoder::onSystateChange: decocoder has been disabled by server, disabling decoder supervision" CR);
        mqtt::down();
    }
    else if ((sysStateChange & OP_DISABLED) && !(newSysState & OP_DISABLED)) {
        Log.INFO("decoder::onSystateChange: decocoder has been enabled by server, enabling decoder supervision" CR);
        mqtt::up();
    }
    if ((sysStateChange & OP_SERVUNAVAILABLE) && !(newSysState & OP_SERVUNAVAILABLE)) {
        Log.INFO("decoder::onSystateChange: server is recieving MQTT Pings from the decoder" CR);
    }
    else if ((sysStateChange & OP_SERVUNAVAILABLE) && (newSysState & OP_SERVUNAVAILABLE)) {
        Log.INFO("decoder::onSystateChange: server failed to receive MQTT Pings from the decoder" CR);
    }
    if ((sysStateChange & OP_CLIEUNAVAILABLE) && !(newSysState & OP_CLIEUNAVAILABLE)) {
        Log.INFO("decoder::onSystateChange: decoder is recieving MQTT Pings from the server" CR);
    }
    else if ((sysStateChange & OP_CLIEUNAVAILABLE) && (newSysState & OP_CLIEUNAVAILABLE)) {
        Log.INFO("decoder::onSystateChange: decider failed to receive MQTT Pings from the server" CR);
    }
    prevSysState = newSysState;
}

void decoder::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onOpStateChange(p_topic, p_payload);
}

void decoder::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    Log.INFO("decoder::onOpStateChange: got a new opState from server: %s" CR, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & (OP_CBL | OP_SERVUNAVAILABLE));
    unSetOpStateByBitmap(~newServerOpState & (OP_CBL | OP_SERVUNAVAILABLE));
}

void decoder::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onAdmStateChange(p_topic, p_payload);
}

void decoder::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        Log.INFO("decoder::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        Log.INFO("decoder::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.ERROR("decoder::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

void decoder::onMqttOpStateChangeHelper(const void* p_miscCbData, sysState_t p_sysState) {
    ((decoder*)p_miscCbData)->onMqttOpStateChange(p_sysState);
}

void decoder::onMqttOpStateChange(sysState_t p_sysState) {
    if (p_sysState)
        setOpStateByBitmap(OP_CBL);
    else
        unSetOpStateByBitmap(OP_CBL);
    if (p_sysState & OP_DISCONNECTED)
        setOpStateByBitmap(OP_DISCONNECTED);
    else
        unSetOpStateByBitmap(OP_DISCONNECTED);
}

rc_t decoder::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t decoder::setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setMqttBrokerURI: cannot set MQTT URI as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        IPAddress mqttIp;
        if (!isUri(p_mqttBrokerURI) && !mqttIp.fromString(p_mqttBrokerURI)) {
            Log.ERROR("decoder::setMqttBrokerURI: Provided MQTT URI: %s missformatted, it is none of an URI or an IP address - rebooting..." CR, p_mqttBrokerURI);
            return RC_PARAMETERVALUE_ERR;
        }
        Log.INFO("decoder::setMqttBrokerURI: setting MQTT URI to %s" CR, p_mqttBrokerURI);
        if(xmlconfig[XML_DECODER_MQTT_URI])
            delete xmlconfig[XML_DECODER_MQTT_URI];
        xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(p_mqttBrokerURI);
        mqtt::setBrokerUri(xmlconfig[XML_DECODER_MQTT_URI]);
        return RC_OK;
    }
}

const char* decoder::getMqttBrokerURI(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getMqttBrokerURI: cannot get MQTT URI as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_URI];
}

rc_t decoder::setMqttPort(int32_t p_mqttPort, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setMqttPort: cannot set MQTT Port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (!((p_mqttPort) >= 0) && (p_mqttPort <= 65535)) {
            Log.ERROR("decoder::setMqttPort: Provided MQTT port: %i is out of range (0-65365): " CR, p_mqttPort);
            return RC_PARAMETERVALUE_ERR;
        }
        Log.INFO("decoder::setMqttPort: setting MQTT Port to %i" CR, p_mqttPort);
        if (xmlconfig[XML_DECODER_MQTT_PORT])
            delete xmlconfig[XML_DECODER_MQTT_PORT];
        char mqttPort[6];
        createNcpystr(itoa(p_mqttPort, mqttPort, 10));
        mqtt::setBrokerPort(atoi(xmlconfig[XML_DECODER_MQTT_PORT]));
        return RC_OK;
    }
}

uint16_t decoder::getMqttPort(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getMqttPort: cannot get MQTT port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_PORT]);
}

rc_t decoder::setMqttPrefix(const char* p_mqttPrefix, bool p_force) { //NEED TO FIX
    if (!debug && !p_force) {
        Log.ERROR("decoder::setMqttPrefix: cannot set MQTT Prefix as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        //Log.INFO("decoder::setMqttPrefix: setting MQTT Prefix to %s" CR, p_mqttPrefix);
        //if (xmlconfig[XML_DECODER_MQTT_PREFIX])
        //    delete xmlconfig[XML_DECODER_MQTT_PREFIX];
        //xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(p_mqttPrefix);
        Log.error("decoder::setMqttPrefix: Setting MQTT Prefix not supported" CR);
        return RC_OK;
    }
}

const char* decoder::getMqttPrefix(bool p_force) {  //NEED TO FIX
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getMqttPrefix: cannot get MQTT prefix as decoder is not configured" CR);
        return NULL;
    }
    return MQTT_PRE_TOPIC_DEFAULT_FRAGMENT;
    //return xmlconfig[XML_DECODER_MQTT_PREFIX];
}

rc_t decoder::setKeepAlivePeriod(uint8_t p_keepAlivePeriod, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setKeepAlivePeriod: cannot set keep-alive period as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD])
            delete xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD];
        char mqttKeepAlive[6];
        xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = createNcpystr(itoa(p_keepAlivePeriod, mqttKeepAlive, 10));
        mqtt::setKeepAlive(atoi(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]));
        return RC_OK;
    }
}

float decoder::getKeepAlivePeriod(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getKeepAlivePeriod: cannot get keep-alive period as decoder is not configured" CR);
        return RC_GEN_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
}

rc_t decoder::setPingPeriod(float p_pingPeriod, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setPingPeriod: cannot set ping supervision period as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        Log.INFO("decoder::setPingPeriod: setting ping supervision period to %f" CR, p_pingPeriod);
        if (xmlconfig[XML_DECODER_MQTT_PINGPERIOD])
            delete xmlconfig[XML_DECODER_MQTT_PINGPERIOD];
        char mqttPingPeriod[6];
        sprintf(mqttPingPeriod, "%f", p_pingPeriod);
        xmlconfig[XML_DECODER_MQTT_PINGPERIOD] = createNcpystr(mqttPingPeriod);
        mqtt::setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]));
        return RC_OK;
    }
}

float decoder::getPingPeriod(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force){
        Log.ERROR("decoder::getPingPeriod: cannot get keep-alive period as decoder is not configured" CR);
        return RC_GEN_ERR;
    }
    return atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]);
}

rc_t decoder::setNtpServer(const char* p_ntpServer, bool p_force) {
    IPAddress ntpAddress;
    if (!debug && !p_force) {
        Log.ERROR("decoder::setNtpServer: cannot set NTP server as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        IPAddress ntpIp;
        if (!(isUri(p_ntpServer) || ntpIp.fromString(p_ntpServer))) {
            Log.ERROR("decoder::setNtpServer: Provided NTP URI: %s missformatted, it is none of an URI or an IP address - rebooting..." CR, p_ntpServer);
        }
        if (isUri(p_ntpServer)) {
            if (xmlconfig[XML_DECODER_NTPURI])
                delete xmlconfig[XML_DECODER_NTPURI];
            xmlconfig[XML_DECODER_NTPURI] = createNcpystr(p_ntpServer);
            ntpTime::deleteNtpServer();
            ntpTime::addNtpServer(xmlconfig[XML_DECODER_NTPURI], atoi(xmlconfig[XML_DECODER_NTPPORT]));
            return RC_OK;
        }
        else if (ntpAddress.fromString(xmlconfig[XML_DECODER_NTPURI])){
            if (xmlconfig[XML_DECODER_NTPURI])
                delete xmlconfig[XML_DECODER_NTPURI];
            xmlconfig[XML_DECODER_NTPURI] = createNcpystr(p_ntpServer);
            ntpTime::deleteNtpServer();
            ntpTime::addNtpServer(ntpAddress, atoi(xmlconfig[XML_DECODER_NTPPORT]));
            return RC_OK;
        }
        else {
            Log.ERROR("decoder::setNtpServer: NTP configuration - NTP-server: %s is incorrect - doing nothing" CR, p_ntpServer);
            return RC_PARAMETERVALUE_ERR;
        }
    }
}

const char* decoder::getNtpServer(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && p_force) {
        Log.ERROR("decoder::getNtpServer: cannot get NTP server as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_NTPURI];
}

rc_t decoder::setNtpPort(int32_t p_ntpPort, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setNtpPort: cannot set NTP port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (!(p_ntpPort >= 0 && p_ntpPort <= 65535)) {
            panic("decoder::setNtpPort: Provided NTP port out of range (0-65365): " CR);
            return RC_PARAMETERVALUE_ERR;
        }
        Log.ERROR("decoder::setNtpPort: cannot set NTP port - not implemented" CR);
        return RC_OK;
    }
}

uint16_t decoder::getNtpPort(bool p_force) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.ERROR("decoder::getNtpPort: cannot get NTP port as decoder is not configured" CR);
        return 0;
    }
    return atoi(xmlconfig[XML_DECODER_NTPPORT]);
}

rc_t decoder::setTz(const char* p_tz, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setTz: cannot set Time zone as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        char tzTmp[20];
        strcpy(tzTmp, p_tz);
        char tzAreaTmp[20] = { '\0' };
        char offsetTmp[3] = { '\0' };
        char* token;
        char delim[2];
        bool posetive;
        bool found = false;
        if (tzTmp[0] == '-' || tzTmp[0] == '+' || tzTmp[strlen(tzTmp) - 1] == '-' || tzTmp[strlen(tzTmp) - 1] == '+') {
            Log.error("decoder::getNtpPort: TZ string \"%s\" missformatted" CR, p_tz);
            return RC_PARAMETERVALUE_ERR;
        }
        else {
            for (int i = 0; i < strlen(tzTmp); i++) {
                if (tzTmp[i] == '+') {
                    posetive = true;
                    if (found) {
                        Log.error("decoder::getNtpPort: TZ string \"%s\" missformatted" CR, p_tz);
                        return RC_PARAMETERVALUE_ERR;
                        break;
                    }
                    found = true;
                }
                if (tzTmp[i] == '-') {
                    posetive = false;
                    if (found) {
                        Log.error("decoder::getNtpPort: TZ string \"%s\" missformatted" CR, p_tz);
                        return RC_PARAMETERVALUE_ERR;
                        break;
                    }
                    found = true;
                }
            }
        }
        if (!found) {
            Log.WARN("decoder::getNtpPort: TZ string \"%s\" does not contain a GMT offset - using \"+00\"" CR, p_tz);
            strcpy(xmlconfig[XML_DECODER_TZ_AREA], p_tz);
            itoa(0, xmlconfig[XML_DECODER_TZ_GMTOFFSET], 10);
            return ntpTime::setTz(p_tz);
        }
        if (posetive)
            strcpy(delim, "+");
        else
            strcpy(delim, "-");
        token = strtok(tzTmp, delim);
        for (int i = 0; i < 2; i++) {
            if (i == 0)
                strcpy(tzAreaTmp, token);
            if (i == 1){
                if(isIntNumberStr(token)){
                    if (posetive)
                        sprintf(offsetTmp, "% +.2i" ,atoi(token));
                    else
                        sprintf(offsetTmp, "%+.2i", -atoi(token));
                }
                else {
                    return RC_PARAMETERVALUE_ERR;
                    break;
                }
            }
            token = strtok(NULL, delim);
        }
        strcpy(xmlconfig[XML_DECODER_TZ_AREA], tzAreaTmp);
        strcpy(xmlconfig[XML_DECODER_TZ_GMTOFFSET], offsetTmp);
        Log.INFO("decoder::getNtpPort: Setting TZ to \"%s\"" CR, p_tz);
        return ntpTime::setTz(p_tz, NULL);
    }
}

rc_t decoder::getTz(char* p_tz, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getTz: cannot get Time-zone as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }

    sprintf(p_tz, "%s%+.2i", xmlconfig[XML_DECODER_TZ_AREA], atoi(xmlconfig[XML_DECODER_TZ_AREA]));
    return RC_OK;
}

rc_t decoder::setLogLevel(const char* p_logLevel, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("decoder::setLogLevel: cannot set Log level as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (transformLogLevelXmlStr2Int(p_logLevel) == RC_GEN_ERR) {
            Log.ERROR("decoder::setLogLevel: cannot set Log-level %s, log-level not supported, using current log-level: %s" CR, p_logLevel, transformLogLevelInt2XmlStr(Log.getLevel()));
            return RC_PARAMETERVALUE_ERR;
        }
        Log.INFO("decoder::setLogLevel: Setting Log-level to %s" CR, p_logLevel);
        if(xmlconfig[XML_DECODER_LOGLEVEL])
            delete xmlconfig[XML_DECODER_LOGLEVEL];
        xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(p_logLevel);
        Log.setLevel(transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]));
        return RC_OK;
    }
}

const char* decoder::getLogLevel(void) {
    if (!transformLogLevelInt2XmlStr(Log.getLevel())) {
        Log.WARN("decoder::getLogLevel: Could not retrieve a valid Log-level" CR);
        return NULL;
    }
    else {
        return transformLogLevelInt2XmlStr(Log.getLevel());
    }
}

rc_t decoder::setFailSafe(const bool p_failsafe, bool p_force) {
    if (!debug && !p_force) {
        Log.INFO("decoder::setFailSafe: cannot set Fail-safe as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (xmlconfig[XML_DECODER_FAILSAFE])
            delete xmlconfig[XML_DECODER_FAILSAFE];
        if (p_failsafe) {
            Log.INFO("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_TRUE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_TRUE_PAYLOAD);
        }
        else {
            Log.INFO("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_FALSE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_FALSE_PAYLOAD);
        }
    }
    return RC_OK;
}

bool decoder::getFailSafe(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getFailSafe: cannot get fail-safe as decoder is not configured" CR);
        return false;
    }
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_TRUE_PAYLOAD))
        return true;
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_FALSE_PAYLOAD))
        return false;
}

rc_t decoder::setSystemName(const char* p_systemName, bool p_force) {
    Log.ERROR("decoder::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* decoder::getSystemName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getSystemName: cannot get System name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_SYSNAME];
}

rc_t decoder::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug && !p_force) {
        Log.INFO("decoder::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        Log.INFO("decoder::setUsrName: Setting User name to %s" CR, p_usrName);
        if (xmlconfig[XML_DECODER_USRNAME])
            delete xmlconfig[XML_DECODER_USRNAME];
        xmlconfig[XML_DECODER_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* decoder::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getUsrName: cannot get User name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_USRNAME];
}

rc_t decoder::setDesc(const char* p_description, bool p_force) {
    if (!debug && !p_force) {
        Log.INFO("decoder::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        Log.INFO("decoder::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_DECODER_DESC];
        xmlconfig[XML_DECODER_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* decoder::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getDesc: cannot get Description as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_DESC];
}

rc_t decoder::setMac(const char* p_mac, bool p_force) {
    if (!debug && p_force) {
        Log.INFO("decoder::setMac: cannot set MAC as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        Log.INFO("decoder::setMac: cannot set MAC - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getMac(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getMac: cannot get MAC as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MAC];
}

rc_t decoder::setDecoderUri(const char* p_decoderUri, bool p_force) {
    if (!debug && !p_force) {
        Log.INFO("decoder::setDecoderUri: cannot set decoder URI as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        Log.INFO("decoder::setDecoderUri: cannot set Decoder URI - not implemented" CR);          //FIX
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getDecoderUri(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("decoder::getDecoderUri: cannot get Decoder URI as decoder is not configured" CR);
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


/* CLI decoration methods */
// No CLI decorations for the decoder context - all decoder related MOs are available through the global CLI context.

/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/
