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
#include "sat.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "sat(Satelite)"                                                                                                                       */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

sat::sat(uint8_t p_satAddr, satLink* p_linkHandle) : systemState(p_linkHandle), globalCli(SAT_MO_NAME, SAT_MO_NAME, p_satAddr, p_linkHandle) {
    asprintf(&logContextName, "%s/%s-%i", p_linkHandle->getLogContextName(), "sat", p_satAddr);
    LOG_INFO("%s: Creating Satelite (sat)" CR, logContextName);
    linkHandle = p_linkHandle;
    satLinkNo = linkHandle->getLink();
    satAddr = p_satAddr;
    memset(acts, NULL, sizeof(acts));
    memset(senses, NULL, sizeof(senses));
    satScanDisabled = true;
    char sysStateObjName[20];
    sprintf(sysStateObjName, "sat-%d", p_satAddr);
    setSysStateObjName(sysStateObjName);
    if (!(satLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNDISCOVERED | OP_UNUSED);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    xmlconfig[XML_SAT_SYSNAME] = NULL;
    xmlconfig[XML_SAT_USRNAME] = NULL;
    xmlconfig[XML_SAT_DESC] = NULL;
    xmlconfig[XML_SAT_ADDR] = NULL;
    xmlconfig[XML_SAT_ADMSTATE] = NULL;
}

sat::~sat(void) {
    panic("%s: sat destructor not supported" CR, logContextName);
}

rc_t sat::init(void) {
    uint8_t link;
    link = linkHandle->getLink();
    LOG_INFO("%s: Initializing sat" CR, logContextName);
    /* CLI decoration methods */
    LOG_INFO("%s: Registering sat specific CLI methods" CR, logContextName);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set address
    regCmdMoArg(GET_CLI_CMD, SAT_MO_NAME, SATADDR_SUB_MO_NAME, onCliGetAddrHelper);
    regCmdHelp(GET_CLI_CMD, SAT_MO_NAME, SATADDR_SUB_MO_NAME, SAT_GET_SATADDR_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, SAT_MO_NAME, SATADDR_SUB_MO_NAME, onCliSetAddrHelper);
    regCmdHelp(SET_CLI_CMD, SAT_MO_NAME, SATADDR_SUB_MO_NAME, SAT_SET_SATADDR_HELP_TXT);
    // get/clear rxcrcerr
    regCmdMoArg(GET_CLI_CMD, SAT_MO_NAME, SATRXCRCERR_SUB_MO_NAME, onCliGetRxCrcErrsHelper);
    regCmdHelp(GET_CLI_CMD, SAT_MO_NAME, SATRXCRCERR_SUB_MO_NAME, SAT_GET_SATRXCRCERR_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SAT_MO_NAME, SATRXCRCERR_SUB_MO_NAME, onCliClearRxCrcErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SAT_MO_NAME, SATRXCRCERR_SUB_MO_NAME, SAT_CLEAR_SATRXCRCERR_HELP_TXT);
    // get/clear txcrcerr
    regCmdMoArg(GET_CLI_CMD, SAT_MO_NAME, SATTXCRCERR_SUB_MO_NAME, onCliGetTxCrcErrsHelper);
    regCmdHelp(GET_CLI_CMD, SAT_MO_NAME, SATTXCRCERR_SUB_MO_NAME, SAT_GET_SATTXCRCERR_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SAT_MO_NAME, SATTXCRCERR_SUB_MO_NAME, onCliClearTxCrcErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SAT_MO_NAME, SATTXCRCERR_SUB_MO_NAME, SAT_CLEAR_SATTXCRCERR_HELP_TXT);
    // get/clear wderr
    regCmdMoArg(GET_CLI_CMD, SAT_MO_NAME, SATWDERR_SUB_MO_NAME, onCliGetWdErrsHelper);
    regCmdHelp(GET_CLI_CMD, SAT_MO_NAME, SATWDERR_SUB_MO_NAME, SAT_GET_SATWDERR_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SAT_MO_NAME, SATWDERR_SUB_MO_NAME, onCliClearWdErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SAT_MO_NAME, SATWDERR_SUB_MO_NAME, SAT_CLEAR_SATWDERR_HELP_TXT);
    LOG_INFO("%s: specific sat CLI methods registered" CR, logContextName);
    LOG_INFO("%s: Creating actuators for sat" CR, logContextName);
    for (uint8_t actPort = 0; actPort < MAX_ACT; actPort++) {
        acts[actPort] = new (heap_caps_malloc(sizeof(actBase), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) actBase(actPort, this);
        if (acts[actPort] == NULL){
            panic("%s Could not create actuator object", logContextName);
            return RC_OUT_OF_MEM_ERR;
        }
        addSysStateChild(acts[actPort]);
        acts[actPort]->init();
    }
    LOG_INFO("%s: Creating sensors for sat" CR, logContextName);
    for (uint8_t sensPort = 0; sensPort < MAX_SENS; sensPort++) {
        senses[sensPort] = new (heap_caps_malloc(sizeof(senseBase), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) senseBase(sensPort, this);
        if (senses[sensPort] == NULL){
            panic("%s: Could not create sensor object", logContextName);
            return RC_OUT_OF_MEM_ERR;
        }
        addSysStateChild(senses[sensPort]);
        senses[sensPort]->init();
    }
    txUnderunErr = 0;
    rxOverRunErr = 0;
    scanTimingViolationErr = 0;
    rxCrcErr = 0;
    remoteCrcErr = 0;
    rxSymbolErr = 0;
    rxDataSizeErr = 0;
    wdErr = 0;
    return RC_OK;
}

void sat::onConfig(tinyxml2::XMLElement* p_satXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)){
        panic("%s: Received a configuration, while the it was already configured, dynamic re-configuration not supported", logContextName);
        return;
    }
    uint8_t link;
    link = linkHandle->getLink();
    LOG_INFO("%s: Received an uverified configuration, parsing and validating it..." CR, logContextName);

    //PARSING CONFIGURATION
    const char* satSearchTags[5];
    satSearchTags[XML_SAT_SYSNAME] = "SystemName";
    satSearchTags[XML_SAT_USRNAME] = "UserName";
    satSearchTags[XML_SAT_DESC] = "Description";
    satSearchTags[XML_SAT_ADDR] = "LinkAddress";
    satSearchTags[XML_SAT_ADMSTATE] = "AdminState";
    getTagTxt(p_satXmlElement->FirstChildElement(), satSearchTags, xmlconfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_SAT_SYSNAME]) {
        panic("%s: SystemNane missing", logContextName);
        return;
    }
    if (!xmlconfig[XML_SAT_USRNAME]) {
        LOG_WARN("%s: User name was not provided - using \"%s-UserName\"" CR, logContextName, xmlconfig[XML_SAT_SYSNAME]);
        xmlconfig[XML_SAT_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_SAT_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_SAT_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_SAT_USRNAME], "%s-Username", xmlconfig[XML_SAT_SYSNAME]);
    }
    if (!xmlconfig[XML_SAT_DESC]) {
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_SAT_DESC] = createNcpystr("-");
    }
    if (!xmlconfig[XML_SAT_ADDR]) {
        panic("%s: Adrress missing", logContextName);
        return;
    }
    if (atoi(xmlconfig[XML_SAT_ADDR]) != satAddr) {
        panic("%s: sat Address inconsistant with what was provided in the object constructor", logContextName);
        return;
    }
    if (xmlconfig[XML_SAT_ADMSTATE] == NULL) {
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_SAT_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_SAT_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_SAT_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else{
        panic("%s: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", logContextName, xmlconfig[XML_SAT_ADMSTATE]);
        return;
    }

    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: System name: %s" CR, logContextName, xmlconfig[XML_SAT_SYSNAME]);
    LOG_INFO("%s: User name: %s" CR, logContextName, xmlconfig[XML_SAT_USRNAME]);
    LOG_INFO("%s: Description: %s" CR, logContextName, xmlconfig[XML_SAT_DESC]);
    LOG_INFO("%s: Address: %s" CR, logContextName, xmlconfig[XML_SAT_ADDR]);
    LOG_INFO("%s: sat admin state: %s" CR, logContextName, xmlconfig[XML_SAT_ADMSTATE]);

    //CONFIFIGURING ACTUATORS
    LOG_INFO("%s: Configuring actuators" CR, logContextName);
    tinyxml2::XMLElement* actXmlElement;
    if (actXmlElement = ((tinyxml2::XMLElement*)p_satXmlElement)->FirstChildElement("Actuator")) {
        const char* actSearchTags[6];
        actSearchTags[XML_ACT_SYSNAME] = NULL;
        actSearchTags[XML_ACT_USRNAME] = NULL;
        actSearchTags[XML_ACT_DESC] = NULL;
        actSearchTags[XML_ACT_PORT] = "Port";
        actSearchTags[XML_ACT_TYPE] = NULL;
        actSearchTags[XML_ACT_SUBTYPE] = NULL;
        actSearchTags[XML_SAT_ADMSTATE] = NULL;
        for (uint8_t actItter = 0; true; actItter++) {
            char* actXmlConfig[6] = { NULL };
            if (actXmlElement == NULL){
                break;
            }
            actXmlConfig[XML_ACT_PORT] = NULL;
            if (actItter >= MAX_ACT){
                panic("%s: More than max actuators provided (%i/%i)", logContextName, actItter, MAX_ACT);
                return;
            }
            getTagTxt(actXmlElement->FirstChildElement(), actSearchTags, actXmlConfig, sizeof(actSearchTags) / 4); // Need to fix the addressing for portability
            if (!actXmlConfig[XML_ACT_PORT]){
                panic("%s: Actuator port missing", logContextName);
                return;
            }
            if ((atoi(actXmlConfig[XML_ACT_PORT])) < 0 || atoi(actXmlConfig[XML_ACT_PORT]) >= MAX_ACT){
                panic("%s: Actuator port out of bounds: %i", logContextName, atoi(actXmlConfig[XML_ACT_PORT]));
                return;
            }
            LOG_TERSE("%s: Configuring Actuator %i" CR, logContextName, atoi(actXmlConfig[XML_ACT_PORT]));
            acts[atoi(actXmlConfig[XML_ACT_PORT])]->onConfig(actXmlElement);
            actXmlElement = ((tinyxml2::XMLElement*)actXmlElement)->NextSiblingElement("Actuator");
        }
    }
    else
        LOG_WARN("%s: No Actuators provided, no Actuator will be configured" CR, logContextName);

    //CONFIFIGURING SENSORS
    LOG_INFO("%s: Configuring sensors" CR, logContextName);
    tinyxml2::XMLElement* sensXmlElement = NULL;
    if (sensXmlElement = ((tinyxml2::XMLElement*)p_satXmlElement)->FirstChildElement("Sensor")) {
        const char* sensSearchTags[5];
        sensSearchTags[XML_SENS_SYSNAME] = NULL;
        sensSearchTags[XML_SENS_USRNAME] = NULL;
        sensSearchTags[XML_SENS_DESC] = NULL;
        sensSearchTags[XML_SENS_PORT] = "Port";
        sensSearchTags[XML_SENS_TYPE] = NULL;
        for (uint8_t sensItter = 0; true; sensItter++) {
            char* sensXmlConfig[5] = { NULL };
            if (sensXmlElement == NULL){
                break;
            }
            if (sensItter >= MAX_SENS){
                panic("%s: More than max sensors provided (%i/%i)", logContextName, sensItter, MAX_SENS);
                return;
            }
            sensSearchTags[XML_SENS_PORT] = "Port";
            getTagTxt(sensXmlElement->FirstChildElement(), sensSearchTags, sensXmlConfig, sizeof(sensSearchTags) / 4); // Need to fix the addressing for portability
            if (!sensXmlConfig[XML_SENS_PORT]){
                panic("%s: Sensor port missing", logContextName);
                return;
            }
            if ((atoi(sensXmlConfig[XML_SENS_PORT])) < 0 || atoi(sensSearchTags[XML_SENS_PORT]) >= MAX_SENS){
                panic("%s: Sensor port out of bounds: %i", logContextName, atoi(sensXmlConfig[XML_SENS_PORT]));
                return;
            }
            LOG_TERSE("%s: Configuring Sensor %i" CR, logContextName, atoi(sensXmlConfig[XML_SENS_PORT]));
            senses[atoi(sensXmlConfig[XML_SENS_PORT])]->onConfig(sensXmlElement);
            sensXmlElement = ((tinyxml2::XMLElement*)sensXmlElement)->NextSiblingElement("Sensor");
        }
    }
    else
        LOG_WARN("%s: No Sensors provided, no Sensor will be configured" CR, logContextName);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("%s: Configuration successfully finished" CR, logContextName);
}

rc_t sat::start(void) {
    uint8_t link;
    link = linkHandle->getLink();
    LOG_INFO("%s: Starting sat" CR, logContextName);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_INFO("%s: sat not configured - will not start it" CR, logContextName);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    LOG_INFO("%s: Subscribing to adm- and op state topics" CR, logContextName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_SAT_ADMSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onAdmStateChangeHelper, this)){
        panic("%s: Failed to suscribe to admState topic:\"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_DECODER_OPSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onOpStateChangeHelper, this)){
        panic("%s: Failed to suscribe to opState topic: \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++) {
        acts[actItter]->start();
    }
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++) {
        senses[sensItter]->start();
    }
    unSetOpStateByBitmap(OP_INIT);
    LOG_INFO("%s: sat has started" CR, logContextName);
    return RC_OK;
}

void sat::up(void) {
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED){
        LOG_WARN("%s: Could not enable sats it has not been discovered" CR, logContextName);
        return;
    }
    LOG_INFO("%s: Enabling sat" CR, logContextName);
    rc_t rc = satLibHandle->enableSat();
    if (rc){
        panic("%s: Could not enable sat, return code: %i", logContextName, rc);
        return;
    }
    satLibHandle->satRegSenseCb(onSenseChangeHelper, this);
}

void sat::onDiscovered(satelite* p_sateliteLibHandle, uint8_t p_satAddr, bool p_exists) {
    uint8_t link;
    link = linkHandle->getLink();
    LOG_INFO("%s: sat discovered" CR, logContextName);
    if (p_satAddr != satAddr) {
        panic("%s: Inconsistent satelite address provided", logContextName);
        return;
    }
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        satLibHandle->satRegStateCb(onSatLibStateChangeHelper, this);
        satLibHandle->setErrTresh(SAT_LINKERR_HIGHTRES, SAT_LINKERR_LOWTRES);
        unSetOpStateByBitmap(OP_UNDISCOVERED);
    }
    else {
        satLibHandle = NULL;
        setOpStateByBitmap(OP_UNDISCOVERED);
    }
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++)
        acts[actItter]->onDiscovered(satLibHandle, p_exists);
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++)
        senses[sensItter]->onDiscovered(satLibHandle, p_exists);
}

void sat::down(void) {
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED) {
        LOG_INFO("%s: Could not disable sat as it has not been discovered" CR, logContextName);
        return;
    }
    LOG_INFO("%s: Disabling sat" CR, logContextName);
    rc_t rc = satLibHandle->disableSat();
    if (rc){
        panic("%s: Could not disable Satelite - return code %X", logContextName, rc);
        return;
    }
    satLibHandle->satUnRegSenseCb();
}

void sat::failsafe(bool p_failsafe) {
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++) {
        if (acts[actItter])
            acts[actItter]->failsafe(p_failsafe);
    }
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++){
        if (senses[sensItter])
            senses[sensItter]->failsafe(p_failsafe);
    }
}

void sat::onPmPoll(void) {
    if (getOpStateBitmap())
        return;
    satPerformanceCounters_t pmData;
    satLibHandle->getSatStats(&pmData, true);
    rxCrcErr += pmData.rxCrcErr;
    remoteCrcErr += pmData.remoteCrcErr;
    wdErr += pmData.wdErr;
    char pmPublishTopic[300];
    sprintf(pmPublishTopic, "%s%s%s%s%s", MQTT_SAT_STATS_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    char pmPublishPayload[100];
    sprintf(pmPublishPayload,
        ("<statReport>\n"
            "<rxCrcErr>%d</rxCrcErr>\n"
            "<txCrcErr> %d</txCrcErr>\n"
            "<wdErr> %d</wdErr>\n"
         "</statReport>"), 
        pmData.remoteCrcErr,
        pmData.rxCrcErr,
        pmData.wdErr);
    if (mqtt::sendMsg(pmPublishTopic, pmPublishPayload, false))
        LOG_ERROR("%s: Failed to send PM report" CR, logContextName);
}

void sat::onSenseChangeHelper(satelite* p_satelite, uint8_t p_LinkAddr, uint8_t p_satAddr, uint8_t p_senseAddr, bool p_senseVal, void* p_metadata) {
    ((sat*)p_metadata)->onSenseChange(p_senseAddr, p_senseVal);
}

void sat::onSenseChange(uint8_t p_senseAddr, bool p_senseVal) {
    if (!getOpStateBitmap() && p_senseAddr >= 0 && p_senseAddr < MAX_SENS)
        senses[p_senseAddr]->onSenseChange(p_senseVal);
    else
        LOG_TERSE("%s: sat: Sensor has changed value to %i, but satelite is not OP_WORKING, doing nothing..." CR, logContextName, p_senseVal);
}

void sat::onSatLibStateChangeHelper(satelite * p_sateliteLibHandle, uint8_t p_linkAddr, uint8_t p_satAddr, satOpState_t p_satOpState, void* p_satHandle) {
    ((sat*)p_satHandle)->onSatLibStateChange(p_satOpState);
}

void sat::onSatLibStateChange(satOpState_t p_satOpState) {
    Serial.printf("EEEEEEEEEEEEEEEEEEE Got a new OP-state from the satelite lib: 0x%X\n", p_satOpState);
    if (p_satOpState & (SAT_OP_INIT | SAT_OP_FAIL | SAT_OP_CONTROLBOCK))
        systemState::setOpStateByBitmap(OP_GENERR);
    else
        systemState::unSetOpStateByBitmap(OP_GENERR);
    if (p_satOpState & SAT_OP_ERR_SEC)
        systemState::setOpStateByBitmap(OP_ERRSEC);
    else
        systemState::unSetOpStateByBitmap(OP_ERRSEC);
}

void sat::onSysStateChangeHelper(const void* p_satHandle, sysState_t p_sysState) {
    ((sat*)p_satHandle)->onSysStateChange(p_sysState);
}

void sat::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: sat has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    bool satDisableScan = false;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange)
        return;
    failsafe(newSysState != OP_WORKING);
    LOG_INFO("%s sat has a new OP-state: %s, Setting failsafe" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState, opStateStr));
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SAT_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if ((newSysState & OP_INTFAIL)) {
        satDisableScan = true;
        down();
        satScanDisabled = true;
        prevSysState = newSysState;
        panic("%s: sat has experienced an internal error - informing server", logContextName);
        return;
    }
    if (newSysState & OP_INIT) {
        LOG_INFO("%s: sat is initializing - informing server if not already done" CR, logContextName);
        satDisableScan = true;
    }
    if (newSysState & OP_UNUSED) {
        LOG_INFO("%s: sat is unused - informing server if not already done" CR, logContextName);
        satDisableScan = true;
    }
    if (newSysState & OP_ERRSEC) {
        LOG_INFO("%s: sat has experienced excessive PM errors - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_GENERR) {
        LOG_INFO("%s: sat has experienced an error - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_DISABLED) {
        LOG_INFO("%s: sat is disabled by server - disabling sat scanning if not already done" CR, logContextName);
        satDisableScan = true;
    }
    if (newSysState & OP_CBL) {
        LOG_INFO("%s: sat is control-blocked by decoder - informing server and disabling satscanning if not already done" CR, logContextName);
        satDisableScan = true;
    }
    if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_ERRSEC | OP_GENERR | OP_DISABLED | OP_CBL)) {
        LOG_INFO("%s: sat has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL), opStateStr));
    }
    if (satDisableScan && !satScanDisabled) {
        LOG_INFO("%s: Disabling sat scanning" CR, logContextName);
        down();
        satScanDisabled = true;
    }
    else if (satDisableScan && satScanDisabled) {
        LOG_INFO("%s: sat scaning already disabled - doing nothing..." CR, logContextName);
    }
    else if (!satDisableScan && satScanDisabled) {
        LOG_INFO("%s: Enabling sat scaning" CR, logContextName);
        up();
        satScanDisabled = false;
    }
    else if (!satDisableScan && !satScanDisabled) {
        LOG_INFO("%s: sat scaning already enabled - doing nothing..." CR, logContextName);
    }
    prevSysState = newSysState;
}

void sat::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle) {
    ((sat*)p_satHandle)->onOpStateChange(p_topic, p_payload);
}

void sat::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: sat got a new opState from server: %s" CR, logContextName);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void sat::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkHandle) {
    ((satLink*)p_satLinkHandle)->onAdmStateChange(p_topic, p_payload);
}

void sat::onAdmStateChange(const char* p_topic, const char* p_payload) {
    uint8_t link;
    link = linkHandle->getLink();
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: sat got online message from server" CR, logContextName);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: sat got off-line message from server" CR, logContextName);
    }
    else
        LOG_ERROR("%s sat got an invalid admstate message from server: \"%s\" - doing nothing" CR, logContextName, p_payload);
} 


actBase* sat::getActHandleByPort(uint8_t p_port) {
    if (p_port >= MAX_ACT) {
        LOG_INFO("%s: Port does not exist - outside of maximum actuators limit", logContextName);
        return NULL;
    }
    for (uint8_t actItter = 0; actItter < MAX_ACT; actItter++) {
        //Serial.printf("YYYYYYYYYYYYYYY Itterating port: %i belonging to object: %i\n", acts[actItter]->getPort(true), acts[actItter]);
        if (acts[actItter]->getPort(true) == p_port)
            return acts[actItter];
    }
    LOG_INFO("%s: Port does not exist - not found", logContextName);
    return NULL;
}

senseBase* sat::getSenseHandleByPort(uint8_t p_port) {
    if (p_port >= MAX_SENS) {
        LOG_INFO("%s: Port does not exist - outside of maximum sense limit", logContextName);
        return NULL;
    }
    for (uint8_t senseItter = 0; senseItter < MAX_SENS; senseItter++) {
        if (senses[senseItter]->getPort(true) == p_port)
            return senses[senseItter];
    }
    LOG_INFO("%s: Port does not exist - not found", logContextName);
    return NULL;
}

rc_t sat::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t sat::setSystemName(const char* p_systemName, bool p_force) {
    LOG_ERROR("%s: Cannot set System name - not suppoted" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* sat::getSystemName(bool  p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as sat is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SAT_SYSNAME];
}

rc_t sat::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set System name as satelite is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting User name to %s" CR, logContextName, p_usrName);
        delete xmlconfig[XML_SAT_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* sat::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as satelite is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SAT_USRNAME];;
}

rc_t sat::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Description as satelite is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting Description to %s" CR, logContextName, p_description);
        delete xmlconfig[XML_SAT_DESC];
        xmlconfig[XML_SAT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* sat::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as satelite is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SAT_DESC];
}

rc_t sat::setAddr(uint8_t p_addr, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Satelite address as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Satelite address as Actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_ERROR("%s: Cannot set Satelite Address - not supported" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

uint8_t sat::getAddr(bool p_force) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED && !p_force) {
        LOG_ERROR("%s: Cannot get Satelite adress as Satelite is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    return satAddr;
}

const char* sat::getLogLevel(void) { //IS THIS METHOD REALLY NEEDED HERE?
    if (!Log.transformLogLevelInt2XmlStr(Log.getLogLevel())) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
}

void sat::setDebug(bool p_debug) {
    debug = p_debug;
}

bool sat::getDebug(void) {
    return debug;
}

uint32_t sat::getRxCrcErrs(void) {
    return remoteCrcErr;
}

void sat::clearRxCrcErrs(void) {
    remoteCrcErr = 0;
}

uint32_t sat::getTxCrcErrs(void) {
    return remoteCrcErr;
}

void sat::clearTxCrcErrs(void) {
    remoteCrcErr = 0;
}

uint32_t sat::getWdErrs(void) {
    return wdErr;
}

void sat::clearWdErrs(void) {
    wdErr = 0;
}

const char* sat::getLogContextName(void) {
    return logContextName;
}

/* CLI decoration methods */
void sat::onCliGetAddrHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite address: %i", static_cast<sat*>(p_cliContext)->getAddr());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void sat::onCliSetAddrHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<sat*>(p_cliContext)->setAddr(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Satelite address, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void sat::onCliGetRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite RX CRC errors: %i", static_cast<sat*>(p_cliContext)->getRxCrcErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void sat::onCliClearRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<sat*>(p_cliContext)->clearRxCrcErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void sat::onCliGetTxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite TX CRC errors: %i", static_cast<sat*>(p_cliContext)->getTxCrcErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void sat::onCliClearTxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    static_cast<sat*>(p_cliContext)->clearTxCrcErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void sat::onCliGetWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite watchdog errors: %i", static_cast<sat*>(p_cliContext)->getWdErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void sat::onCliClearWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<sat*>(p_cliContext)->clearWdErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

/*==============================================================================================================================================*/
/* END Class sat                                                                                                                                */
/*==============================================================================================================================================*/
