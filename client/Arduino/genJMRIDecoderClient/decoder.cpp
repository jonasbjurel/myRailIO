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
EXT_RAM_ATTR char decoder::newRssiNotification[50];
EXT_RAM_ATTR char decoder::previousRssiNotification[50];
EXT_RAM_ATTR char decoder::newIntMemNotification[50];
EXT_RAM_ATTR char decoder::previousIntMemNotification[50];
EXT_RAM_ATTR char decoder::newLogNotification[50];
EXT_RAM_ATTR char decoder::previousLogNotification[50] = "FALSE";
EXT_RAM_ATTR uint32_t decoder::previousMissedLogs = 0;
EXT_RAM_ATTR char decoder::newCliNotification[50];
EXT_RAM_ATTR char decoder::previousCliNotification[50];
EXT_RAM_ATTR char decoder::newDebugNotification[50];
EXT_RAM_ATTR char decoder::previousDebugNotification[50];
EXT_RAM_ATTR char decoder::newNtpNotification[50];
EXT_RAM_ATTR char decoder::previousNtpNotification[50];

decoder::decoder(void) : systemState(NULL), globalCli(DECODER_MO_NAME, DECODER_MO_NAME, 0, NULL, true) {
    regPanicCb(onPanicHelper, this);
    wdt::regGlobalFailsafeCb(onWdtFailsafeHelper, this);
    wdt::regGlobalRebootCb(onWdtReboot);
    asprintf(&logContextName, "%s", "decoder-0");
    LOG_INFO("%s: Creating decoder" CR, logContextName);
    setSysStateObjName(logContextName);
    debug = false;
    LOG_INFO("%s: Creating lgLinks" CR, logContextName);
    for (uint8_t lgLinkNo = 0; lgLinkNo < MAX_LGLINKS; lgLinkNo++) {
        lgLinks[lgLinkNo] = new (heap_caps_malloc(sizeof(lgLink), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lgLink(lgLinkNo, this);
        if (lgLinks[lgLinkNo] == NULL) {
            panic("Could not create lgLink objects");
            return;
        }
        addSysStateChild(lgLinks[lgLinkNo]);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: lgLinks created" CR, logContextName);
    LOG_INFO("%s: Creating satLinks" CR, logContextName);
    for (uint8_t satLinkNo = 0; satLinkNo < MAX_SATLINKS; satLinkNo++) {
        satLinks[satLinkNo] = new (heap_caps_malloc(sizeof(satLink), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) satLink(satLinkNo, this);
        if (satLinks[satLinkNo] == NULL) {
            panic("Could not create satLink objects");
            return;
        }
        LOG_VERBOSE("%s: Added Satlink index %d with object %d" CR, logContextName, satLinkNo, satLinks[satLinkNo]);
        addSysStateChild(satLinks[satLinkNo]);
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: satLinks created" CR, logContextName);
    mqtt::create();
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_UNDISCOVERED | OP_UNCONFIGURED | OP_DISABLED | OP_CBL);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    if (!(decoderLock = xSemaphoreCreateMutex())) {
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(networking::getMqttUri());
    xmlconfig[XML_DECODER_MQTT_PORT] = new (heap_caps_malloc(sizeof(char[6]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[6];
    xmlconfig[XML_DECODER_MQTT_PORT] = itoa(networking::getMqttPort(), xmlconfig[XML_DECODER_MQTT_PORT], 10);
    xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(MQTT_PRE_TOPIC_DEFAULT_FRAGMENT);
    xmlconfig[XML_DECODER_MQTT_PINGPERIOD] = new (heap_caps_malloc(sizeof(char[6]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[6];
    xmlconfig[XML_DECODER_MQTT_PINGPERIOD] = itoa(MQTT_DEFAULT_PINGPERIOD_S, xmlconfig[XML_DECODER_MQTT_PINGPERIOD], 10);
    xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = new (heap_caps_malloc(sizeof(char[6]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[6];
    xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = itoa(MQTT_DEFAULT_KEEP_ALIVE_S, xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD], 10);
    xmlconfig[XML_DECODER_NTPURI] = createNcpystr(NTP_DEFAULT_URI); //SHOULD THIS ORIGINATE FROM networking
    xmlconfig[XML_DECODER_NTPPORT] = new (heap_caps_malloc(sizeof(char[6]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[6];
    xmlconfig[XML_DECODER_NTPPORT] = itoa(NTP_DEFAULT_PORT, xmlconfig[XML_DECODER_NTPPORT], 10); //SHOULD THIS ORIGINATE FROM networking
    xmlconfig[XML_DECODER_TZ_CLEAR_TEXT] = createNcpystr(NTP_DEFAULT_TZ_AREA_CLEAR_TEXT);
    xmlconfig[XML_DECODER_TZ_ENCODED_TEXT] = createNcpystr(NTP_DEFAULT_TZ_AREA_ENCODED_TEXT);
    xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(Log.transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
    xmlconfig[XML_DECODER_RSYSLOGSERVER] = NULL;
    xmlconfig[XML_DECODER_RSYSLOGPORT] = new (heap_caps_malloc(sizeof(char[4]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[6];
    itoa(RSYSLOG_DEFAULT_PORT, xmlconfig[XML_DECODER_RSYSLOGPORT], 10);
    xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(DEFAULT_FAILSAFE);
    xmlconfig[XML_DECODER_SYSNAME] = NULL;
    xmlconfig[XML_DECODER_USRNAME] = NULL;
    xmlconfig[XML_DECODER_DESC] = NULL;
    xmlconfig[XML_DECODER_MAC] = createNcpystr(networking::getMac());
    xmlconfig[XML_DECODER_URI] = NULL;
    xmlconfig[XML_DECODER_ADMSTATE] = NULL;
}

decoder::~decoder(void) {
    panic("%s: decoder destructor not supported", logContextName);
}

rc_t decoder::init(void){
    LOG_INFO("%s: Initializing decoder" CR, logContextName);
    LOG_INFO("%s: Initializing MQTT " CR, logContextName);
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
    LOG_INFO("%s: Initializing lgLinks" CR, logContextName);
    for (uint8_t lgLinkNo = 0; lgLinkNo < MAX_LGLINKS; lgLinkNo++) {
        lgLinks[lgLinkNo]->init();
    }
    LOG_INFO("%s: lgLinks initialized" CR, logContextName);
    LOG_INFO("%s: Initializing satLinks" CR, logContextName);
    for (uint8_t satLinkNo = 0; satLinkNo < MAX_SATLINKS; satLinkNo++) {
        satLinks[satLinkNo]->init();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: satLinks initialized" CR, logContextName);
    LOG_INFO("%s: Subscribing to decoder configuration topic and sending configuration request" CR, logContextName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s", MQTT_CONFIG_RESP_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::subscribeTopic(subscribeTopic, &onConfigHelper, this)){
        panic("%s: Failed to suscribe to configuration response topic \"%s\"" CR, logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    char publishTopic[300];
    sprintf(publishTopic, "%s%s%s", MQTT_CONFIG_REQ_TOPIC, "/", mqtt::getDecoderUri());
    if (mqtt::sendMsg(publishTopic, MQTT_CONFIG_REQ_PAYLOAD, false)) {
        panic("%s: Failed to send configuration request" CR, logContextName);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Waiting for configuration ..." CR, logContextName);
    uint16_t i = 0;
    while (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        if (i++ >= DECODER_CONFIG_TIMEOUT_S) {
            panic("%s: Did not receive configuration", logContextName);
            return RC_GEN_ERR;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: Got valid configuration" CR, logContextName);
    LOG_INFO("%s: Starting CLI service" CR, logContextName);
    regGlobalNCommonCliMOCmds();
    globalCli::start();
    LOG_INFO("%s: CLI service started" CR, logContextName);
    LOG_INFO("%s: Initialized" CR, logContextName);
    return RC_OK;
}

void decoder::onConfigHelper(const char* p_topic, const char* p_payload, const void* p_decoderObj) {
    ((decoder*)p_decoderObj)->onConfig(p_topic, p_payload);
}

void decoder::onConfig(const char* p_topic, const char* p_payload) {
    //CONFIG PARSING
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)) {
        panic("%s: Received a configuration, while the it was already configured, dynamic re - configuration not supported", logContextName);
        return;
    }
    LOG_INFO("%s: Received an uverified configuration, parsing and validating it..." CR, logContextName);
    xmlConfigDoc = new (heap_caps_malloc(sizeof(tinyxml2::XMLDocument), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) tinyxml2::XMLDocument;
    if (xmlConfigDoc->Parse(p_payload)) {
        panic("%s: Configuration parsing failed", logContextName);
        return;
    }
    if (xmlConfigDoc->FirstChildElement("genJMRI") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder") == NULL || xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement("SystemName") == NULL) {
        panic("%s: Failed to parse the configuration - xml is missformatted", logContextName);
        return;
    }
    //PARSING CONFIGURATION
    const char* decoderSearchTags[19];
    decoderSearchTags[XML_DECODER_MQTT_URI] = "DecoderMqttURI";
    decoderSearchTags[XML_DECODER_MQTT_PORT] = "DecoderMqttPort";
    decoderSearchTags[XML_DECODER_MQTT_PREFIX] = "DecoderMqttTopicPrefix";
    decoderSearchTags[XML_DECODER_MQTT_PINGPERIOD] = "DecoderPingPeriod";
    decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = "DecoderKeepAlivePerid";
    decoderSearchTags[XML_DECODER_NTPURI] = "NTPServer";
    decoderSearchTags[XML_DECODER_NTPPORT] = "NTPPort";
    decoderSearchTags[XML_DECODER_TZ_CLEAR_TEXT] = "TimeZoneClearText";
    decoderSearchTags[XML_DECODER_TZ_ENCODED_TEXT] = "TimeZoneEncodedText";
    decoderSearchTags[XML_DECODER_LOGLEVEL] = "LogLevel";
    decoderSearchTags[XML_DECODER_RSYSLOGSERVER] = "RSyslogServer";
    decoderSearchTags[XML_DECODER_RSYSLOGPORT] = "RSyslogPort";
    decoderSearchTags[XML_DECODER_FAILSAFE] = "DecodersFailSafe";
    decoderSearchTags[XML_DECODER_SYSNAME] = NULL;
    decoderSearchTags[XML_DECODER_USRNAME] = NULL;
    decoderSearchTags[XML_DECODER_DESC] = NULL;
    decoderSearchTags[XML_DECODER_MAC] = NULL;
    decoderSearchTags[XML_DECODER_URI] = NULL;
    decoderSearchTags[XML_DECODER_ADMSTATE] = NULL;
    LOG_INFO("Parsing decoder configuration:" CR);
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement(), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability E.g. sizeof(decoderSearchTags)/sizeof(size_t)
    decoderSearchTags[XML_DECODER_MQTT_URI] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PORT] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PREFIX] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_PINGPERIOD] = NULL;
    decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = NULL;
    decoderSearchTags[XML_DECODER_NTPURI] = NULL;
    decoderSearchTags[XML_DECODER_NTPPORT] = NULL;
    decoderSearchTags[XML_DECODER_TZ_CLEAR_TEXT] = NULL;
    decoderSearchTags[XML_DECODER_TZ_ENCODED_TEXT] = NULL;
    decoderSearchTags[XML_DECODER_LOGLEVEL] = NULL;
    decoderSearchTags[XML_DECODER_RSYSLOGSERVER] = NULL;
    decoderSearchTags[XML_DECODER_RSYSLOGPORT] = NULL;
    decoderSearchTags[XML_DECODER_FAILSAFE] = NULL;
    decoderSearchTags[XML_DECODER_SYSNAME] = "SystemName";
    decoderSearchTags[XML_DECODER_USRNAME] = "UserName";
    decoderSearchTags[XML_DECODER_DESC] = "Description";
    decoderSearchTags[XML_DECODER_MAC] = "MAC";
    decoderSearchTags[XML_DECODER_URI] = "URI";
    decoderSearchTags[XML_DECODER_ADMSTATE] = "AdminState";
    getTagTxt(xmlConfigDoc->FirstChildElement("genJMRI")->FirstChildElement("Decoder")->FirstChildElement(), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    LOG_INFO("%s: Validating and setting provided decoder configuration" CR, logContextName);
    bool failSafe;
    if(!strcmp(xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_TRUE_PAYLOAD))
        failSafe = true;
    else if(!strcmp(xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_FALSE_PAYLOAD))
        failSafe = false;
    else{
        failSafe = true;
        LOG_ERROR("%s;Provided Failsafe statement: %s is missformatted - only \"%s\" or \"%s\" allowed - activating Failsafe" CR, logContextName, xmlconfig[XML_DECODER_FAILSAFE], MQTT_BOOL_TRUE_PAYLOAD, MQTT_BOOL_FALSE_PAYLOAD);
    }
    if (setLogLevel(xmlconfig[XML_DECODER_LOGLEVEL], true)) {
        LOG_ERROR("%s: Could not set log-level to %s, leaving it to default %s" CR, logContextName, xmlconfig[XML_DECODER_LOGLEVEL], Log.transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
        delete xmlconfig[XML_DECODER_LOGLEVEL];
        createNcpystr(xmlconfig[XML_DECODER_LOGLEVEL], Log.transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
    }
    if (xmlconfig[XML_DECODER_RSYSLOGSERVER]) {
        if (!isUri(xmlconfig[XML_DECODER_RSYSLOGSERVER])) {
            LOG_ERROR("%s: RSyslog server URI: %s is not a valid URI, will not start RSyslog" CR, logContextName, xmlconfig[XML_DECODER_RSYSLOGSERVER]);
        }
        else if (!xmlconfig[XML_DECODER_RSYSLOGPORT]){
            LOG_WARN("%s: RSyslog server port not provided, will use the default port: %i" CR, logContextName, RSYSLOG_DEFAULT_PORT);
            itoa(RSYSLOG_DEFAULT_PORT, xmlconfig[XML_DECODER_RSYSLOGPORT], 10);
        }
        else if (atoi(xmlconfig[XML_DECODER_RSYSLOGPORT]) < 0 || atoi(xmlconfig[XML_DECODER_RSYSLOGPORT]) > 65535) {
            LOG_WARN("%s: Provided RSyslog server port: %s is not a valid port, will use default RSyslog port %i" CR, logContextName, xmlconfig[XML_DECODER_RSYSLOGPORT], RSYSLOG_DEFAULT_PORT);
            itoa(RSYSLOG_DEFAULT_PORT, xmlconfig[XML_DECODER_RSYSLOGPORT], 10);
        }
        setRSyslogServer(xmlconfig[XML_DECODER_RSYSLOGSERVER], atoi(xmlconfig[XML_DECODER_RSYSLOGPORT]), true);
    }
    else {
        LOG_INFO("%s: RSyslog server URI not provided, will not start RSyslog" CR, logContextName);
    }
    LOG_TERSE("Setting up MQTT endpoints and properties" CR);
    if (strcmp(xmlconfig[XML_DECODER_MQTT_URI], networking::getMqttUri())){
        setMqttBrokerURI(xmlconfig[XML_DECODER_MQTT_URI], true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (atoi(xmlconfig[XML_DECODER_MQTT_PORT]) != networking::getMqttPort()) {
        setMqttPort(atoi(xmlconfig[XML_DECODER_MQTT_PORT]), true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]) != MQTT_DEFAULT_PINGPERIOD_S) {
        setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]), true);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (atoi(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]) != MQTT_DEFAULT_KEEP_ALIVE_S) {
        setKeepAlivePeriod(atoi(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]), true);
    }
    setMqttPrefix(xmlconfig[XML_DECODER_MQTT_PREFIX], true);
    LOG_INFO("Setting up NTP time server" CR);
    if (xmlconfig[XML_DECODER_NTPURI] && xmlconfig[XML_DECODER_NTPPORT]) {
        LOG_TERSE("Setting up NTP server URI and Port from configuration - URI: %s Port: %s" CR, xmlconfig[XML_DECODER_NTPURI], xmlconfig[XML_DECODER_NTPPORT]);
        if (uint8_t rc = setNtpServer(xmlconfig[XML_DECODER_NTPURI], atoi(xmlconfig[XML_DECODER_NTPPORT]), true) != RC_OK)
            LOG_WARN("Could not set-up NTP server - NTP client will not be started, rc: %i" CR, rc);
        else
            ntpTime::start();
    }
    else if (xmlconfig[XML_DECODER_NTPURI]) {
        LOG_TERSE("Setting up NTP server URI from configuration with default port settings - URI: %s Port: %s" CR, xmlconfig[XML_DECODER_NTPURI], NTP_DEFAULT_PORT);
        if (uint8_t rc = setNtpServer(xmlconfig[XML_DECODER_NTPURI], NTP_DEFAULT_PORT, true) != RC_OK)
            LOG_WARN("Could not set-up NTP server - NTP client will not be started, rc: %i" CR, rc);
        else
            ntpTime::start();
    }
    else {
        LOG_TERSE("No NTP server configuration was provided, setting up default NTP server and port settings - URI: %s Port: %s" CR, NTP_DEFAULT_URI, NTP_DEFAULT_PORT);
        if (uint8_t rc = setNtpServer(NTP_DEFAULT_URI, NTP_DEFAULT_PORT, true) != RC_OK)
            LOG_WARN("Could not set-up NTP server - NTP client will not be started, rc: %i" CR, rc);
        else
            ntpTime::start();
    }
    LOG_TERSE("Seting timezone to %s (%s)" CR, xmlconfig[XML_DECODER_TZ_CLEAR_TEXT], xmlconfig[XML_DECODER_TZ_ENCODED_TEXT]);
	if(uint8_t rc = setTz(xmlconfig[XML_DECODER_TZ_ENCODED_TEXT], true))
        LOG_WARN("Could not set time-zone, rc: %i" CR, rc);
    if (xmlconfig[XML_DECODER_SYSNAME] == NULL) {
        panic("%s: System name was not provided", logContextName);
        return;
    }
    if (xmlconfig[XML_DECODER_USRNAME] == NULL){
        LOG_WARN("%s: User name was not provided - using %s-UserName" CR, logContextName, xmlconfig[XML_DECODER_SYSNAME]);
        xmlconfig[XML_DECODER_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_DECODER_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_DECODER_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_DECODER_USRNAME], "%s-UserName", xmlconfig[XML_DECODER_SYSNAME]);
    }
    if (xmlconfig[XML_DECODER_DESC] == NULL){
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_DECODER_DESC] = createNcpystr("-");
    }
    if (strcmp(xmlconfig[XML_DECODER_MAC], networking::getMac())){
        panic("%s: Provided MAC: %s is inconsistant with the decoders Physical MAC %s", logContextName, xmlconfig[XML_DECODER_MAC], networking::getMac());
        return;
    }
    if (xmlconfig[XML_DECODER_URI] == NULL)
        xmlconfig[XML_DECODER_URI] = createNcpystr(mqtt::getDecoderUri());
    if (strcmp(xmlconfig[XML_DECODER_URI], mqtt::getDecoderUri())) {
        panic("%s: Configuration decoder URI is not the same as provided with the discovery response", logContextName);
        return;
    }
    //setSysStateObjName(xmlconfig[XML_DECODER_URI]); FIX
    if (xmlconfig[XML_DECODER_ADMSTATE] == NULL){
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_DECODER_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_DECODER_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_DECODER_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else {
        panic("%s: Configuration decoder::onConfig: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", logContextName, xmlconfig[XML_DECODER_ADMSTATE]);
        return;
    }
    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: Successfully set the decoder top-configuration as follows:" CR, logContextName);
    LOG_INFO("%s: MQTT Server: \"%s:%s\"" CR, logContextName, xmlconfig[XML_DECODER_MQTT_URI], xmlconfig[XML_DECODER_MQTT_PORT]);
    LOG_INFO("%s: MQTT Prefix: \"%s\"" CR, logContextName, xmlconfig[XML_DECODER_MQTT_PREFIX], MQTT_PRE_TOPIC_DEFAULT_FRAGMENT);
    LOG_INFO("%s: MQTT Ping-period: %s" CR, logContextName, xmlconfig[XML_DECODER_MQTT_PINGPERIOD]);
    LOG_INFO("%s: NTP Server: \"%s:%s\"" CR, logContextName, xmlconfig[XML_DECODER_NTPURI], xmlconfig[XML_DECODER_NTPPORT]);
    LOG_INFO("%s: Time-zone: \"%s\" (\"%s\")" CR, logContextName, xmlconfig[XML_DECODER_TZ_CLEAR_TEXT], xmlconfig[XML_DECODER_TZ_ENCODED_TEXT]);
    LOG_INFO("%s: Log-level: %s" CR, logContextName, xmlconfig[XML_DECODER_LOGLEVEL]);
    LOG_INFO("%s: Decoder fail-safe: %s" CR, logContextName, xmlconfig[XML_DECODER_FAILSAFE]);
    LOG_INFO("%s: Decoder MAC: %s" CR, logContextName, xmlconfig[XML_DECODER_MAC]);
    LOG_INFO("%s: Decoder URI: %s" CR, logContextName, xmlconfig[XML_DECODER_URI]);
    LOG_INFO("%s: Decoder System name: %s" CR, logContextName, xmlconfig[XML_DECODER_SYSNAME]);
    LOG_INFO("%s: Decoder User name: %s" CR, logContextName, xmlconfig[XML_DECODER_USRNAME]);
    LOG_INFO("%s: Decoder Description: %s" CR, logContextName, xmlconfig[XML_DECODER_DESC]);
    LOG_INFO("%s: Decoder Admin state: %s" CR, logContextName,  xmlconfig[XML_DECODER_ADMSTATE]);
    LOG_INFO("%s: Successfully parsed and processed the decoder top-configuration:" CR, logContextName);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    //CONFIFIGURING LIGHTGROUP LINKS
    LOG_INFO("%s: Validating and configuring lightgroups links" CR, logContextName);
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
            if (lgLinkItter >= MAX_LGLINKS) {
                panic("%s: More than maximum lgLinks provided (%i/%i) - not supported", logContextName, lgLinkItter, MAX_LGLINKS);
                return;
            }
            getTagTxt(lgLinkXmlElement->FirstChildElement(), lgLinkSearchTags, lgLinkXmlConfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
            if (!lgLinkXmlConfig[XML_LGLINK_LINK]) {
                panic("%s: lgLink missing", logContextName);
                return;
            }
            if ((atoi(lgLinkSearchTags[XML_LGLINK_LINK])) < 0 || atoi(lgLinkSearchTags[XML_LGLINK_LINK]) >= MAX_LGLINKS) {
                panic("%s: lgLink number (%i) out of bounds", logContextName, atoi(lgLinkSearchTags[XML_LGLINK_LINK]));
                return;
            }
            lgLinks[atoi(lgLinkXmlConfig[XML_LGLINK_LINK])]->onConfig(lgLinkXmlElement);
            lgLinkXmlElement = ((tinyxml2::XMLElement*)lgLinkXmlElement)->NextSiblingElement("LightgroupsLink");
        }
    }
    else
        LOG_INFO("%s No lgLinks provided, no lgLinks will be configured" CR, logContextName);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    //CONFIFIGURING SATELITE LINKS
    LOG_INFO("%s: Validating and configuring satelite links" CR, logContextName);
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
            if (satLinkItter >= MAX_SATLINKS) {
                panic("%s: More than maximum satLinks provided (%i/%i) - not supported", logContextName, satLinkItter, MAX_SATLINKS);
                return;
            }
            getTagTxt(satLinkXmlElement->FirstChildElement(), satLinkSearchTags, satLinkXmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
            if (!satLinkXmlconfig[XML_SATLINK_LINK]) {
                panic("%s: satLink missing", logContextName);
                return;
            }
            if ((atoi(satLinkXmlconfig[XML_SATLINK_LINK])) < 0 || atoi(satLinkXmlconfig[XML_SATLINK_LINK]) >= MAX_SATLINKS) {
                panic("%s:satLink number (%i) out of bounds", logContextName, atoi(satLinkXmlconfig[XML_SATLINK_LINK]));
                return;
            }
            satLinks[atoi(satLinkXmlconfig[XML_SATLINK_LINK])]->onConfig(satLinkXmlElement);
            satLinkXmlElement = ((tinyxml2::XMLElement*)satLinkXmlElement)->NextSiblingElement("SateliteLink");
        }
    }
    else
        LOG_INFO("%s: No satLinks provided, no satelites will be configured" CR, logContextName);
    delete xmlConfigDoc;
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("%s: Configuration successfully finished" CR, logContextName);
}

rc_t decoder::start(void) {
    LOG_INFO("%s: Starting decoder" CR, logContextName);
    uint16_t i = 0;
    while (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        if (i == 0)
            LOG_INFO("%s: Waiting for decoder to be configured before it can start" CR, logContextName);
        if (i++ >= DECODER_CONFIG_TIMEOUT_S * 10) {
            panic("%s: Configuration process failed", logContextName);
            return RC_GEN_ERR;
        }
        Serial.print('.');
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: Subscribing to adm- and op- state topics" CR, logContextName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_ADMSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onAdmStateChangeHelper, this)) {
        panic("%s: Failed to suscribe to admState topic\"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_OPSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onOpStateChangeHelper, this)) {
        panic("%s: Failed to suscribe to opState topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }

    LOG_INFO("%s: Subscribing to reboot topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_REBOOT_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onRebootHelper, this)) {
        panic("%s: Failed to suscribe to reboot topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to coredump request topic" CR, logContextName);
	sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_COREDUMP_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetCoreDumpHelper, this)) {
		panic("%s: Failed to suscribe to coredump topic \"%s\"", logContextName, subscribeTopic);
	    return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to SSID request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_SSID_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetSsidHelper, this)) {
        panic("%s: Failed to suscribe to SSID topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to WIFI SNR request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_SNR_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetSnrHelper, this)) {
        panic("%s: Failed to suscribe to WIFI SNR topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to IP Address request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_IPADDR_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetIpAddrHelper, this)) {
        panic("%s: Failed to suscribe to IP Address topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to Memory statistics request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_MEMSTAT_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetMemStatHelper, this)) {
        panic("%s: Failed to suscribe to Memory statistics \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to CPU statistics request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_CPUSTAT_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetCpuStatHelper, this)) {
        panic("%s: Failed to suscribe to CPU statistics \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    
    LOG_INFO("%s: Subscribing to Broker URI request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_BROKERURI_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetBrokerUriHelper, this)) {
        panic("%s: Failed to suscribe to Broker URI \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to HW Version request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_HWVER_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetHwVerHelper, this)) {
        panic("%s: Failed to suscribe to HW Version \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to SW Version request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_SWVER_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetSwVerHelper, this)) {
        panic("%s: Failed to suscribe to SW Version \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to Debug level request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_LOGLVL_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetLogLvlHelper, this)) {
        panic("%s: Failed to suscribe to Debug level \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to Decoder WWW UI request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_WWWUI_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetWwwUiHelper, this)) {
        panic("%s: Failed to suscribe to WWW UI request \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    LOG_INFO("%s: Subscribing to Decoder OP State request topic" CR, logContextName);
    sprintf(subscribeTopic, "%s/%s/%s", MQTT_DECODER_OPSTATE_REQ_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    if (mqtt::subscribeTopic(subscribeTopic, onMqttGetOpStateHelper, this)) {
        panic("%s: Failed to suscribe to OP State request \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }

    LOG_INFO("Starting lightgroup link Decoders" CR);
    for (int lgLinkItter = 0; lgLinkItter < MAX_LGLINKS; lgLinkItter++) {
        LOG_TERSE("%s: Starting LgLink-%d" CR, logContextName, lgLinkItter);
        if (lgLinks[lgLinkItter] == NULL){
            panic("%s: lgLink-%d does not exist", logContextName, lgLinkItter);
            return RC_GEN_ERR;
        }
        lgLinks[lgLinkItter]->start();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    LOG_INFO("%s: Starting satelite link Decoders" CR, logContextName);
    for (int satLinkItter = 0; satLinkItter < MAX_SATLINKS; satLinkItter++) {
        LOG_TERSE("Starting SatLink-%d" CR, satLinkItter);
        if (satLinks[satLinkItter] == NULL) {
            panic("%s: satLink-%d does not exist", logContextName, satLinkItter);
            return RC_GEN_ERR;
        }
        satLinks[satLinkItter]->start();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    unSetOpStateByBitmap(OP_INIT);
	startDecoderNotifications();
    LOG_INFO("%s: decoder started" CR, logContextName);
    return RC_OK;
}

void decoder::onSysStateChangeHelper(const void* p_miscData, sysState_t p_sysState) {
    ((decoder*)p_miscData)->onSysStateChange(p_sysState);
}

void decoder::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: decoder has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    sysState_t sysStateChange = newSysState ^ prevSysState;
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
        panic("%s: decoder has experienced an internal error.", logContextName);
        prevSysState = newSysState;
        return;
    }
    if (newSysState & OP_INIT) {
        LOG_INFO("%s: The decoder is initializing..." CR, logContextName);
        prevSysState = newSysState;
        return;
    }
    if ((sysStateChange & OP_DISCONNECTED) && (newSysState & OP_DISCONNECTED)) {
        LOG_ERROR("decoder has been disconnected" CR);
        prevSysState = newSysState;
    }
    if ((sysStateChange & OP_INIT) && !(newSysState & OP_DISABLED)) {
        LOG_INFO("%s: The decoder have been initialized and is enabled - enabling decoder supervision" CR, logContextName);
        if (mqtt::up())
            panic("Could not start MQTT supervision");
    }
    else if ((sysStateChange & OP_INIT) && (newSysState & OP_DISABLED)) {
        LOG_INFO("%s: The decoder have been initialized but is disabled - will not enable decoder supervision" CR, logContextName);
        mqtt::down();
    }
    if ((sysStateChange & OP_DISABLED) && (newSysState & OP_DISABLED)) {
        LOG_INFO("%s: decocoder has been disabled by server, disabling decoder supervision" CR, logContextName);
        mqtt::down();
    }
    else if ((sysStateChange & OP_DISABLED) && !(newSysState & OP_DISABLED)) {
        LOG_INFO("%s: decocoder has been enabled by server, enabling decoder supervision" CR, logContextName);
        if (mqtt::up())
            panic("Could not start MQTT supervision");
    }
    if ((sysStateChange & OP_SERVUNAVAILABLE) && !(newSysState & OP_SERVUNAVAILABLE)) {
        LOG_INFO("%s: Server is reporting recieved MQTT Pings from the decoder" CR, logContextName);
    }
    else if ((sysStateChange & OP_SERVUNAVAILABLE) && (newSysState & OP_SERVUNAVAILABLE)) {
        LOG_INFO("%s: server failed to receive MQTT Pings from the decoder" CR, logContextName);
    }
    if ((sysStateChange & OP_CLIEUNAVAILABLE) && !(newSysState & OP_CLIEUNAVAILABLE)) {
        LOG_INFO("%s: decoder is recieving MQTT Pings from the server" CR, logContextName);
    }
    else if ((sysStateChange & OP_CLIEUNAVAILABLE) && (newSysState & OP_CLIEUNAVAILABLE)) {
        LOG_INFO("%s: decoder failed to receive MQTT Pings from the server" CR, logContextName);
    }
    prevSysState = newSysState;
}

void decoder::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onOpStateChange(p_topic, p_payload);
}

void decoder::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: Got a new opState from server: %s" CR, logContextName, p_payload);
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
        LOG_INFO("%s: Got online message from server" CR, logContextName);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: Got off-line message from server" CR, logContextName);
    }
    else
        LOG_ERROR("%s: Got an invalid admstate message from server - doing nothing" CR, logContextName);
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
        LOG_ERROR("%s: Cannot set MQTT URI as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        IPAddress mqttIp;
        if (!isUri(p_mqttBrokerURI) && !mqttIp.fromString(p_mqttBrokerURI)) {
            LOG_ERROR("%s: Provided MQTT URI: %s missformatted, it is none of an URI or an IP address - rebooting..." CR, logContextName, p_mqttBrokerURI);
            return RC_PARAMETERVALUE_ERR;
        }
        LOG_INFO("%s: Setting MQTT URI to %s" CR, logContextName, p_mqttBrokerURI);
        if(xmlconfig[XML_DECODER_MQTT_URI])
            delete xmlconfig[XML_DECODER_MQTT_URI];
        xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(p_mqttBrokerURI);
        mqtt::setBrokerUri(xmlconfig[XML_DECODER_MQTT_URI]);
        return RC_OK;
    }
}

const char* decoder::getMqttBrokerURI(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get MQTT URI as decoder is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_URI];
}

rc_t decoder::setMqttPort(int32_t p_mqttPort, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set MQTT Port as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (!((p_mqttPort) >= 0) && (p_mqttPort <= 65535)) {
            LOG_ERROR("%s: Provided MQTT port: %i is out of range (0-65365): " CR, logContextName, p_mqttPort);
            return RC_PARAMETERVALUE_ERR;
        }
        LOG_INFO("%s: Setting MQTT Port to %i" CR, logContextName, p_mqttPort);
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
        LOG_ERROR("%s: Cannot get MQTT port as decoder is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_PORT]);
}

rc_t decoder::setMqttPrefix(const char* p_mqttPrefix, bool p_force) { //NEED TO FIX
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set MQTT Prefix as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        LOG_INFO("decoder::setMqttPrefix: setting MQTT Prefix to %s" CR, p_mqttPrefix);
        if (xmlconfig[XML_DECODER_MQTT_PREFIX])
            delete xmlconfig[XML_DECODER_MQTT_PREFIX];
        xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(p_mqttPrefix);
        mqtt::setMqttTopicPrefix(p_mqttPrefix);
        return RC_OK;
    }
}

const char* decoder::getMqttPrefix(bool p_force) {  //NEED TO FIX
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get MQTT prefix as decoder is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_PREFIX];
}

rc_t decoder::setKeepAlivePeriod(uint8_t p_keepAlivePeriod, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set keep-alive period as debug is inactive" CR, logContextName);
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

uint8_t decoder::getKeepAlivePeriod(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get keep-alive period as decoder is not configured" CR, logContextName);
        return RC_GEN_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
}

rc_t decoder::setPingPeriod(float p_pingPeriod, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set ping supervision period as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        LOG_INFO("%s: Setting ping supervision period to %f" CR, logContextName, p_pingPeriod);
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
        LOG_ERROR("%s: Cannot get keep-alive period as decoder is not configured" CR, logContextName);
        return RC_GEN_ERR;
    }
    return atof(xmlconfig[XML_DECODER_MQTT_PINGPERIOD]);
}

rc_t decoder::setNtpServer(const char* p_ntpServer, uint16_t p_port, bool p_force) {
    IPAddress ntpAddress;
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set NTP server as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        IPAddress ntpIp;
        if (!(isUri(p_ntpServer) || ntpIp.fromString(p_ntpServer))) {
            LOG_ERROR("%s: Provided NTP URI: %s missformatted, it is none of an URI or an IP address - rebooting..." CR, logContextName, p_ntpServer);
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
            LOG_ERROR("%s: NTP configuration - NTP-server: %s is incorrect - doing nothing" CR, logContextName, p_ntpServer);
            return RC_PARAMETERVALUE_ERR;
        }
    }
}

rc_t decoder::setTz(const char* p_tz, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set Time zone as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
		delete xmlconfig[XML_DECODER_TZ_ENCODED_TEXT];
        xmlconfig[XML_DECODER_TZ_ENCODED_TEXT] = createNcpystr(p_tz);
        LOG_INFO("Setting TZ to \"%s\"" CR, p_tz);
        return ntpTime::setTz(p_tz, NULL);
    }
}

rc_t decoder::getTz(char* p_tz, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Time-zone as decoder is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_tz, xmlconfig[XML_DECODER_TZ_ENCODED_TEXT]);
    return RC_OK;
}

rc_t decoder::setLogLevel(const char* p_logLevel, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set Log level as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (Log.transformLogLevelXmlStr2Int(p_logLevel) == RC_GEN_ERR) {
            LOG_ERROR("%s: Cannot set Log-level %s, log-level not supported, using current log-level: %s" CR, logContextName, p_logLevel, Log.transformLogLevelInt2XmlStr(Log.getLogLevel()));
            return RC_PARAMETERVALUE_ERR;
        }
        LOG_INFO("%s: Setting Log-level to %s" CR, logContextName, p_logLevel);
        if(xmlconfig[XML_DECODER_LOGLEVEL])
            delete xmlconfig[XML_DECODER_LOGLEVEL];
        xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(p_logLevel);
        if (Log.setLogLevel(Log.transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]))) {
            LOG_ERROR("%s: Cannot set Log-level %s, log-level not supported, using current log-level: %s" CR, logContextName, p_logLevel, Log.transformLogLevelInt2XmlStr(Log.getLogLevel()));
            return RC_PARAMETERVALUE_ERR;
        }
        return RC_OK;
    }
}

const char* decoder::getLogLevel(void) {
    if (Log.getLogLevel() < 0) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
}

rc_t decoder::setRSyslogServer(const char* p_uri, uint16_t p_port, bool p_force) {
    if (!debug && !p_force)
        return RC_DEBUG_NOT_SET_ERR;
    if (Log.getLogServer(0, NULL, NULL) == RC_ALREADYEXISTS_ERR)
        return RC_ALREADYEXISTS_ERR;
    return Log.addLogServer(xmlconfig[XML_DECODER_SYSNAME], p_uri, p_port);
}

rc_t decoder::getRSyslogServer(char* p_uri, uint16_t* p_port, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force)
        return RC_NOT_CONFIGURED_ERR;
    else
        return Log.getLogServer(0, p_uri, p_port);
}

rc_t decoder::setFailSafe(const bool p_failsafe, bool p_force) {
    if (!debug && !p_force) {
        LOG_INFO("%s: Cannot set Fail-safe as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        if (xmlconfig[XML_DECODER_FAILSAFE])
            delete xmlconfig[XML_DECODER_FAILSAFE];
        if (p_failsafe) {
            LOG_INFO("%s: Setting Fail-safe to %s" CR, logContextName, MQTT_BOOL_TRUE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_TRUE_PAYLOAD);
        }
        else {
            LOG_INFO("%s: Setting Fail-safe to %s" CR, logContextName, MQTT_BOOL_FALSE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_FALSE_PAYLOAD);
        }
    }
    return RC_OK;
}

bool decoder::getFailSafe(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get fail-safe as decoder is not configured" CR, logContextName);
        return false;
    }
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_TRUE_PAYLOAD))
        return true;
    if (strcmp(xmlconfig[XML_DECODER_LOGLEVEL], MQTT_BOOL_FALSE_PAYLOAD))
        return false;
}

rc_t decoder::setSystemName(const char* p_systemName, bool p_force) {
    LOG_ERROR("%s: Cannot set System name - not supported" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t decoder::getSystemName(char* p_systemName, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as decoder is not configured" CR, logContextName);
        strcpy(p_systemName, "-");
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_systemName, xmlconfig[XML_DECODER_SYSNAME]);
    return RC_OK;
}

rc_t decoder::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug && !p_force) {
        LOG_INFO("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        LOG_INFO("Setting User name to %s" CR, p_usrName);
        if (xmlconfig[XML_DECODER_USRNAME])
            delete xmlconfig[XML_DECODER_USRNAME];
        xmlconfig[XML_DECODER_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

rc_t decoder::getUsrName(char* p_userName, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as decoder is not configured" CR, logContextName);
        strcpy(p_userName, "-");
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_userName, xmlconfig[XML_DECODER_USRNAME]);
    return RC_OK;
}

rc_t decoder::setDesc(const char* p_description, bool p_force) {
    if (!debug && !p_force) {
        LOG_INFO("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        LOG_INFO("%s: Setting Description to %s" CR, logContextName, p_description);
        delete xmlconfig[XML_DECODER_DESC];
        xmlconfig[XML_DECODER_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t decoder::getDesc(char* p_desc, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as decoder is not configured" CR, logContextName);
        strcpy(p_desc, "-");
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_desc, xmlconfig[XML_DECODER_DESC]);
    return RC_OK;
}

rc_t decoder::setMac(const char* p_mac, bool p_force) {
    if (!debug && p_force) {
        LOG_INFO("%s: Cannot set MAC as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else {
        LOG_INFO("%s: Cannot set MAC - not implemented" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getMac(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get MAC as decoder is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MAC];
}

rc_t decoder::setDecoderUri(const char* p_decoderUri, bool p_force) {
    if (!debug && !p_force) {
        LOG_INFO("%s: Cannot set decoder URI as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set decoder URI as Decoder is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Cannot set Decoder URI - not implemented" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getDecoderUri(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Decoder URI as decoder is not configured" CR, logContextName);
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

const char* decoder::getLogContextName(void) {
    return logContextName;
}

void decoder::onPanicHelper(void* p_metaData) {
    ((decoder*)p_metaData)->onPanic();
}

void decoder::onPanic(void) {
    char opStateFail[50];
    Serial.printf(">>> %s: Panic callback received - setting OP-state to: \"%s\"" CR, logContextName, getOpStateStrByBitmap(OP_INTFAIL, opStateFail));
    LOG_FATAL("%s: Panic callback received - setting OP-state to: \"%s\"" CR, logContextName, getOpStateStrByBitmap(OP_INTFAIL, opStateFail));
    setOpStateByBitmap(OP_INTFAIL);
}

uint8_t decoder::onWdtFailsafeHelper(uint8_t escalationCnt, const void* p_metaData) {
    return ((decoder*)p_metaData)->onWdtFailsafe();
}

uint8_t decoder::onWdtFailsafe(void) {
    char opStateFail[50];
    Serial.printf(">>> %s: Watchdog timer expired, requesting global failsafe" CR, logContextName);
    LOG_FATAL("%s: Watchdog timer expired, requesting global failsafe" CR, logContextName);
    setOpStateByBitmap(OP_INTFAIL);
    return DONT_ESCALATE;
}

uint8_t decoder::onWdtReboot(uint8_t escalationCnt, const void* p_metaData) {
    Serial.printf(">>> Watchdog timer expired, requesting reboot" CR);
    panic("Watchdog timer expired, requesting reboot");
    return ESCALATE;
}

void decoder::onRebootHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onReboot();
}

void decoder::onReboot(void) {
    Serial.printf(">>> Ordered reboot");
    panic("Ordered reboot");
}

void decoder::onMqttGetCoreDumpHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetCoreDump();
}

void decoder::onMqttGetCoreDump(void) {
	char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
	sprintf(topic, "%s/%s/%s", MQTT_DECODER_COREDUMP_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
    char* coreDumpBuff = new (heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
    uint readBytes;
	if (fileSys::getFile(FS_PATH "/" "panic", coreDumpBuff, 10000, &readBytes)) {
        sprintf(payload, "<%s>\n\r%s\n\r</%s>", DELIVERCOREDUMP_XMLTAG_PAYLOAD, "No Coredump stored", DELIVERCOREDUMP_XMLTAG_PAYLOAD);
		mqtt::sendMsg(topic, payload, false);
	}
	else {
		coreDumpBuff[readBytes] = '\0';
		sprintf(payload, "<%s>\n\r%s\n\r</%s>", DELIVERCOREDUMP_XMLTAG_PAYLOAD, coreDumpBuff, DELIVERCOREDUMP_XMLTAG_PAYLOAD);
        mqtt::sendMsg(topic, payload, false);
	}
    delete coreDumpBuff;
    delete payload;
    delete topic;
}

void decoder::onMqttGetSsidHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetSsid();
}

void decoder::onMqttGetSsid(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_SSID_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>%s</%s>", DELIVERSSID_XMLTAG_PAYLOAD, networking::getSsid(), DELIVERSSID_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetSnrHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
	((decoder*)p_decoderObject)->onMqttGetSnr();
}

void decoder::onMqttGetSnr(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_SNR_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>%i</%s>", DELIVERSNR_XMLTAG_PAYLOAD, networking::getRssi(), DELIVERSNR_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetIpAddrHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetIpAddr();
}

void decoder::onMqttGetIpAddr(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_IPADDR_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>%s</%s>", DELIVERIPADDR_XMLTAG_PAYLOAD, networking::getIpAddr().toString().c_str(), DELIVERIPADDR_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetMemStatHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetMemStat();
}

void decoder::onMqttGetMemStat(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_MEMSTAT_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    heapInfo_t intMemInfo;
    cpu::getHeapMemInfo(&intMemInfo, true);
    heapInfo_t extMemInfo;
	cpu::getHeapMemInfo(&extMemInfo, false);
    sprintf(payload, "<%s>%i/%i</%s>", DELIVERMEMSTAT_XMLTAG_PAYLOAD, intMemInfo.freeSize/1000, extMemInfo.freeSize/1000, DELIVERMEMSTAT_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetCpuStatHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetCpuStat();
}

void decoder::onMqttGetCpuStat(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_CPUSTAT_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>-</%s>", DELIVERCPUSTAT_XMLTAG_PAYLOAD, DELIVERCPUSTAT_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetBrokerUriHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetBrokerUri();
}

void decoder::onMqttGetBrokerUri(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_BROKERURI_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>%s</%s>", DELIVERBROKERURI_XMLTAG_PAYLOAD, xmlconfig[XML_DECODER_MQTT_URI], DELIVERBROKERURI_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetHwVerHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetHwVer();
}

void decoder::onMqttGetHwVer(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_HWVER_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];

    sprintf(payload, "<%s>%s-V%i</%s>", DELIVERHWVER_XMLTAG_PAYLOAD, ESP.getChipModel(), ESP.getChipRevision(), DELIVERHWVER_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetSwVerHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetSwVer();
}

void decoder::onMqttGetSwVer(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_SWVER_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];

    sprintf(payload, "<%s>V%s-%s</%s>", DELIVERSWVER_XMLTAG_PAYLOAD, GENJMRI_VERSION, GENJMRI_STATUS, DELIVERSWVER_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
    }

void decoder::onMqttGetLogLvlHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetLogLvl();
}

void decoder::onMqttGetLogLvl(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_LOGLVL_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];

    sprintf(payload, "<%s>%s</%s>", DELIVERLOGLVL_XMLTAG_PAYLOAD, xmlconfig[XML_DECODER_LOGLEVEL], DELIVERLOGLVL_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetWwwUiHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetWwwUi();
}

void decoder::onMqttGetWwwUi(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_WWWUI_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>http://%s:80</%s>", DELIVERWWWUI_XMLTAG_PAYLOAD, networking::getIpAddr().toString().c_str(), DELIVERWWWUI_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

void decoder::onMqttGetOpStateHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject) {
    ((decoder*)p_decoderObject)->onMqttGetOpState();
}

void decoder::onMqttGetOpState(void) {
    char* topic = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(topic, "%s/%s/%s", MQTT_DECODER_OPSTATE_RESP_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_DECODER_SYSNAME]);
    char opStateStr[50];
    getOpStateStr(opStateStr);
    char* payload = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
    sprintf(payload, "<%s>%s</%s>", DELIVEROPSTATE_XMLTAG_PAYLOAD, opStateStr, DELIVEROPSTATE_XMLTAG_PAYLOAD);
    mqtt::sendMsg(topic, payload, false);
    delete payload;
    delete topic;
}

rc_t decoder::startDecoderNotifications(void) {
    LOG_INFO_NOFMT("Starting Decoder notification service" CR);
    if (!eTaskCreate(                               // Spinning up a notification loop task
        decoderNotificationLoop,                    // Task function
        CPU_NOTIF_TASKNAME,                         // Task function name reference
        CPU_NOTIF_STACKSIZE_1K * 1024,              // Stack size
        this,                                       // Parameter passing
        CPU_NOTIF_PRIO,                             // Priority 0-24, higher is more
        CPU_NOTIF_STACK_ATTR)) {                    // Stack attibute
        panic("Could not start the Decoder notificationservice");
        return RC_OUT_OF_MEM_ERR;
    }
    return RC_OK;
}

void decoder::decoderNotificationLoop(void* p_handle) {
    while (true) {  
        ((decoder*)p_handle)->checkWifiRssi();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        //((decoder*)p_handle)->checkCpu();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkMem();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkLog();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkCli();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkNtp();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkCli();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
        ((decoder*)p_handle)->checkDebug();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void decoder::checkWifiRssi(void) {
	newRssiNotification[0] = '\0';
	long rssi = networking::getRssi();
    if (rssi <= RSSI_POOR && strcmp(previousRssiNotification, "POOR")) {
        strcpy(newRssiNotification, "POOR");
        LOG_ERROR("%s: POOR WIFI Signal detected, RSSI: %i" CR, logContextName, rssi);
        }
	else if (rssi > RSSI_POOR && rssi <= RSSI_FAIR && strcmp(previousRssiNotification, "FAIR")) {
		strcpy(newRssiNotification, "FAIR");
		LOG_WARN("%s: FAIR WIFI Signal detected, RSSI: %i" CR, logContextName, rssi);
	}
	else if (rssi > RSSI_FAIR && strcmp(previousRssiNotification, "GOOD")) {
		strcpy(newRssiNotification, "GOOD");
		LOG_INFO("%s: GOOD WIFI Signal detected, RSSI: %i" CR, logContextName, rssi);
	}
	if (strlen(newRssiNotification)){
        char publishTopic[200];
        char publishPayload[50];
        sprintf(publishTopic, "%s%s%s", MQTT_DECODER_WIFISTATUS_TOPIC, "/", mqtt::getDecoderUri());
        sprintf(publishPayload, "<%s>%s</%s>", DELIVERWIFISTATUS_XMLTAG_PAYLOAD, newRssiNotification, DELIVERWIFISTATUS_XMLTAG_PAYLOAD);
		mqtt::sendMsg(publishTopic, publishPayload, false);
		strcpy(previousRssiNotification, newRssiNotification);
    }
}

void decoder::checkMem(void) {
	heapInfo_t intMemInfo;
	newIntMemNotification[0] = '\0';
	cpu::getHeapMemInfo(&intMemInfo, true);
	uint8_t intMemUsage = (1 - (float)((float)intMemInfo.freeSize/(float)intMemInfo.totalSize)) * 100;
    if (intMemUsage >= MEM_CRITICAL && strcmp(previousIntMemNotification, "CRITICAL")) {
		strcpy(newIntMemNotification, "CRITICAL");
        LOG_ERROR("%s: Critical memory utilization: %i\%" CR, logContextName, intMemUsage);
	}
	else if (intMemUsage < MEM_CRITICAL && intMemUsage >= MEM_HIGH && strcmp(previousIntMemNotification, "HIGH")) {
        strcpy(newIntMemNotification, "HIGH");
		LOG_WARN("%s: High memory utilization: %i\%" CR, logContextName, intMemUsage);
	}
	else if (intMemUsage < MEM_HIGH && intMemUsage >= MEM_WARN && strcmp(previousIntMemNotification, "WARN")) {
        strcpy(newIntMemNotification, "WARN");
		LOG_INFO("%s: Warning for memory utilization: %i\%" CR, logContextName, intMemUsage);
	}
	else if (intMemUsage < MEM_WARN && strcmp(previousIntMemNotification, "NORMAL")) {
        strcpy(newIntMemNotification, "NORMAL");
		LOG_INFO("%s: Normal memory utilization: %i\%" CR, logContextName, intMemUsage);
	}
    if (strlen(newIntMemNotification)) {
        char publishTopic[200];
        char publishPayload[50];
        sprintf(publishTopic, "%s%s%s", MQTT_DECODER_MEMSTATUS_TOPIC, "/", mqtt::getDecoderUri());
        sprintf(publishPayload, "<%s>%s</%s>", DELIVERMEMSTATUS_XMLTAG_PAYLOAD, newIntMemNotification, DELIVERMEMSTATUS_XMLTAG_PAYLOAD);
        mqtt::sendMsg(publishTopic, publishPayload, false);
		strcpy(previousIntMemNotification, newIntMemNotification);
    }
}

void decoder::checkLog(void) {
    newLogNotification[0] = '\0';
	uint32_t missedLogs = Log.getMissedLogs();
	if (missedLogs > previousMissedLogs && strcmp(previousLogNotification, "TRUE")) {
		strcpy(newLogNotification, "TRUE");
    }
	else if (missedLogs == previousMissedLogs && strcmp(previousLogNotification, "FALSE")) {
		strcpy(newLogNotification, "FALSE");
	}
	else if (missedLogs < previousMissedLogs && strcmp(previousLogNotification, "FALSE")) {
		strcpy(newLogNotification, "FALSE");
	}
	previousMissedLogs = missedLogs;
	if (strlen(newLogNotification)) {
        char publishTopic[200];
        char publishPayload[50];
        sprintf(publishTopic, "%s%s%s", MQTT_DECODER_LOGOVERLOAD_TOPIC, "/", mqtt::getDecoderUri());
		sprintf(publishPayload, "<%s>%s</%s>", DELIVERLOGOVERLOAD_XMLTAG_PAYLOAD, newLogNotification, DELIVERLOGOVERLOAD_XMLTAG_PAYLOAD);
		mqtt::sendMsg(publishTopic, publishPayload, false);
		strcpy(previousLogNotification, newLogNotification);
	}
}

void decoder::checkNtp(void) {
	newNtpNotification[0] = '\0';
    ntpOpState_t ntpOpState;
	ntpTime::getNtpOpState(&ntpOpState);
	Serial.printf("NTP state: 0x%x" CR, ntpOpState);
	if (!ntpOpState && strcmp(previousNtpNotification, "SYNCED")) {
		strcpy(newNtpNotification, "SYNCED");
	}
    else if (ntpOpState == NTP_CLIENT_SYNCHRONIZING && strcmp(previousNtpNotification, "SYNCING")) {
        strcpy(newNtpNotification, "SYNCING");
    }
	else if (ntpOpState == NTP_CLIENT_DISABLED && strcmp(previousNtpNotification, "DISABLED")) {
		strcpy(newNtpNotification, "DISABLED");
	}
	else if (ntpOpState == NTP_CLIENT_NOT_SYNCHRONIZED && strcmp(previousNtpNotification, "NOTSYNCING")) {
		strcpy(newNtpNotification, "NOTSYNCING");
	}
	if (strlen(newNtpNotification)) {
		char publishTopic[200];
		char publishPayload[50];
		sprintf(publishTopic, "%s%s%s", MQTT_DECODER_NTPSTATUS_TOPIC, "/", mqtt::getDecoderUri());
		sprintf(publishPayload, "<%s>%s</%s>", DELIVERNTPSTATUS_XMLTAG_PAYLOAD, newNtpNotification, DELIVERNTPSTATUS_XMLTAG_PAYLOAD);
		mqtt::sendMsg(publishTopic, publishPayload, false);
		strcpy(previousNtpNotification, newNtpNotification);
	}
}
void decoder::checkCli(void) {
    newCliNotification[0] = '\0';
    const char* cliClient = getConnectedClient();
    if (cliClient && strcmp(previousCliNotification, cliClient)) {
        strcpy(newCliNotification, cliClient);
    }
    else if (!cliClient && strcmp(previousCliNotification, "NONE")) {
        strcpy(newCliNotification, "NONE");
    }
    if (strlen(newCliNotification)) {
        char publishTopic[200];
        char publishPayload[50];
        sprintf(publishTopic, "%s%s%s", MQTT_DECODER_CLIACCESS_TOPIC, "/", mqtt::getDecoderUri());
        sprintf(publishPayload, "<%s>%s</%s>", DELIVERCLIACCESS_XMLTAG_PAYLOAD, newCliNotification, DELIVERCLIACCESS_XMLTAG_PAYLOAD);
        mqtt::sendMsg(publishTopic, publishPayload, false);
        strcpy(previousCliNotification, newCliNotification);
    }
}

void decoder::checkDebug(void) {
	newDebugNotification[0] = '\0';
	uint8_t debugCnt = getDebugCnt();
    if (debugCnt && strcmp(previousDebugNotification, "ACTIVE")) {
		strcpy(newDebugNotification, "ACTIVE");
		LOG_INFO("%s: Debug flag(s) is active" CR, logContextName);
    }
    else if (!debugCnt && strcmp(previousDebugNotification, "INACTIVE")) {
		strcpy(newDebugNotification, "INACTIVE");
		LOG_INFO("%s: Debug flag(s) is inactive" CR, logContextName);
    }
	if (strlen(newDebugNotification)) {
        char publishTopic[200];
        char publishPayload[50];
        sprintf(publishTopic, "%s%s%s", MQTT_DECODER_DEBUG_TOPIC, "/", mqtt::getDecoderUri());
		sprintf(publishPayload, "<%s>%s</%s>", DELIVERDEBUG_XMLTAG_PAYLOAD, newDebugNotification, DELIVERDEBUG_XMLTAG_PAYLOAD);
		mqtt::sendMsg(publishTopic, publishPayload, false);
		strcpy(previousDebugNotification, newDebugNotification);
	}
}

/* CLI decoration methods */
// No CLI decorations for the decoder context - all decoder related MOs are available through the global CLI context.

/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/
