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
    Log.INFO("sat::sat: Creating Satelite adress %d" CR, p_satAddr);
    linkHandle = p_linkHandle;
    satLinkNo = linkHandle->getLink();
    satAddr = p_satAddr;
    memset(acts, NULL, sizeof(acts));
    memset(senses, NULL, sizeof(senses));
    satScanDisabled = true;
    char sysStateObjName[20];
    sprintf(sysStateObjName, "sat-%d", p_satAddr);
    setSysStateObjName(sysStateObjName);
    if (!(satLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
        panic("sat::sat: Could not create Lock objects - rebooting...");
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
    panic("sat::~sat: sat destructior not supported - rebooting..." CR);
}

rc_t sat::init(void) {
    uint8_t link;
    link = linkHandle->getLink();
    Log.INFO("sat::init: Initializing Satelite address %d" CR, satAddr);
    /* CLI decoration methods */
    Log.INFO("sat::init: Registering CLI methods for satelite %d, on satLink %d" CR, satAddr, link);
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
    Log.INFO("sat::init: CLI methods for satelite %d, on satLink %d registered" CR, satAddr, link);
    Log.INFO("sat::init: Creating actuators for satelite address %d on link %d" CR, satAddr, link);
    for (uint8_t actPort = 0; actPort < MAX_ACT; actPort++) {
        acts[actPort] = new (heap_caps_malloc(sizeof(actBase(actPort, this)), MALLOC_CAP_SPIRAM)) actBase(actPort, this);
        if (acts[actPort] == NULL)
            panic("sat::init: Could not create actuator object - rebooting...");
        addSysStateChild(acts[actPort]);
        acts[actPort]->init();
    }
    Log.INFO("sat::init: Creating sensors for satelite address %d on link %d" CR, satAddr, link);
    for (uint8_t sensPort = 0; sensPort < MAX_SENS; sensPort++) {
        senses[sensPort] = new (heap_caps_malloc(sizeof(senseBase(sensPort, this)), MALLOC_CAP_SPIRAM)) senseBase(sensPort, this);
        if (senses[sensPort] == NULL)
            panic("sat::init: Could not create sensor object - rebooting..." CR);
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
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        panic("sat:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    uint8_t link;
    link = linkHandle->getLink();
    Log.INFO("sat::onConfig: satAddress %d on link %d received an uverified configuration, parsing and validating it..." CR, satAddr, link);

    //PARSING CONFIGURATION
    const char* satSearchTags[5];
    satSearchTags[XML_SAT_SYSNAME] = "SystemName";
    satSearchTags[XML_SAT_USRNAME] = "UserName";
    satSearchTags[XML_SAT_DESC] = "Description";
    satSearchTags[XML_SAT_ADDR] = "LinkAddress";
    satSearchTags[XML_SAT_ADMSTATE] = "AdminState";
    getTagTxt(p_satXmlElement->FirstChildElement(), satSearchTags, xmlconfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_SAT_SYSNAME])
        panic("sat::onConfig: SystemNane missing - rebooting...");
    if (!xmlconfig[XML_SAT_USRNAME])
        panic("sat::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SAT_DESC])
        panic("sat::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SAT_ADDR])
        panic("sat::onConfig: Adrress missing - rebooting...");
    if (atoi(xmlconfig[XML_SAT_ADDR]) != satAddr)
        panic("sat::onConfig: Address no inconsistant - rebooting...");
    if (xmlconfig[XML_SAT_ADMSTATE] == NULL) {
        Log.WARN("sat::onConfig: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR);
        xmlconfig[XML_SAT_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_SAT_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_SAT_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else
        panic("satLink::onConfig: Admin state: %s is none of \"ENABLE\" or \"DISABLE\" - rebooting..." CR, xmlconfig[XML_SAT_ADMSTATE]);

    //SHOW FINAL CONFIGURATION
    Log.INFO("sat::onConfig: System name: %s" CR, xmlconfig[XML_SAT_SYSNAME]);
    Log.INFO("sat::onConfig: User name: %s" CR, xmlconfig[XML_SAT_USRNAME]);
    Log.INFO("sat::onConfig: Description: %s" CR, xmlconfig[XML_SAT_DESC]);
    Log.INFO("sat::onConfig: Address: %s" CR, xmlconfig[XML_SAT_ADDR]);
    Log.INFO("sat::onConfig: sat admin state: %s" CR, xmlconfig[XML_SAT_ADMSTATE]);

    //CONFIFIGURING ACTUATORS
    Log.INFO("sat::onConfig: Configuring actuators" CR);
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
                Log.TERSE("sat::onConfig: No more Actuators to configure" CR);
                break;
            }
            actXmlConfig[XML_ACT_PORT] = NULL;
            if (actItter >= MAX_ACT)
                panic("sat::onConfig: > than max actuators provided - not supported, rebooting..." CR);
            getTagTxt(actXmlElement->FirstChildElement(), actSearchTags, actXmlConfig, sizeof(actSearchTags) / 4); // Need to fix the addressing for portability
            if (!actXmlConfig[XML_ACT_PORT])
                panic("sat::onConfig:: Actuator port missing - rebooting...");
            if ((atoi(actXmlConfig[XML_ACT_PORT])) < 0 || atoi(actXmlConfig[XML_ACT_PORT]) >= MAX_ACT)
                panic("sat::onConfig:: Actuator port out of bounds - rebooting..." CR);
            Log.TERSE("sat::onConfig: Configuring Actuator %i" CR, atoi(actXmlConfig[XML_ACT_PORT]));
            acts[atoi(actXmlConfig[XML_ACT_PORT])]->onConfig(actXmlElement);
            actXmlElement = ((tinyxml2::XMLElement*)actXmlElement)->NextSiblingElement("Actuator");
        }
    }
    else
        Log.WARN("sat::onConfig: No Actuators provided, no Actuator will be configured" CR);

    //CONFIFIGURING SENSORS
    Log.INFO("sat::onConfig: Configuring sensors" CR);
    tinyxml2::XMLElement* sensXmlElement;
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
                Log.TERSE("sat::onConfig: No more Sensors to configure" CR);
                break;
            }
            if (sensItter >= MAX_SENS)
                panic("sat::onConfig: > than max sensors provided - not supported, rebooting..." CR);
            sensSearchTags[XML_SENS_PORT] = "Port";
            getTagTxt(sensXmlElement->FirstChildElement(), sensSearchTags, sensXmlConfig, sizeof(sensSearchTags) / 4); // Need to fix the addressing for portability
            if (!sensXmlConfig[XML_SENS_PORT])
                panic("sat::onConfig:: Sensor port missing - rebooting..." CR);
            if ((atoi(sensXmlConfig[XML_SENS_PORT])) < 0 || atoi(sensSearchTags[XML_SENS_PORT]) >= MAX_SENS)
                panic("sat::onConfig:: Sensor port out of bounds - rebooting...");
            Log.TERSE("sat::onConfig: Configuring Sensor %i" CR, atoi(sensXmlConfig[XML_SENS_PORT]));
            senses[atoi(sensXmlConfig[XML_SENS_PORT])]->onConfig(sensXmlElement);
            sensXmlElement = ((tinyxml2::XMLElement*)sensXmlElement)->NextSiblingElement("Sensor");
        }
    }
    else
        Log.WARN("sat::onConfig: No Sensors provided, no Sensor will be configured" CR);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    Log.INFO("sat::onConfig: Configuration successfully finished" CR);
}

rc_t sat::start(void) {
    uint8_t link;
    link = linkHandle->getLink();
    Log.INFO("sat::start: Starting Satelite address: %d on satlink %d" CR, satAddr, link);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.INFO("sat::start: Satelite address %d on satlink %d not configured - will not start it" CR, satAddr, link);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    Log.INFO("sat::start: Subscribing to adm- and op state topics" CR);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_SAT_ADMSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onAdmStateChangeHelper, this))
        panic("sat::start: Failed to suscribe to admState topic - rebooting..." CR);
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_DECODER_OPSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onOpStateChangeHelper, this))
        panic("sat::start: Failed to suscribe to opState topic - rebooting..." CR);
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++) {
        acts[actItter]->start();
    }
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++) {
        senses[sensItter]->start();
    }
    unSetOpStateByBitmap(OP_INIT);
    Log.INFO("sat::start: Satelite address: %d on satlink %d have started" CR, satAddr, link);
    return RC_OK;
}

void sat::up(void) {
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED){
        Log.INFO("sat::up: Could not enable sat-%d as it has not been discovered" CR, satAddr);
        return;
    }
    Log.info("sat::up: Enabling sat-%d" CR, satAddr);
    rc_t rc = satLibHandle->enableSat();
    if (rc)
        panic("sat::up: could not enable Satelite, return code: %i - rebooting..." CR, rc);
    satLibHandle->satRegSenseCb(onSenseChangeHelper, this);
}

void sat::down(void) {
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED) {
        Log.INFO("sat::down: Could not disable sat-%d as it has not been discovered" CR, satAddr);
        return;
    }
    Log.info("sat::down: Disabling sat-%d" CR, satAddr);
    rc_t rc = satLibHandle->disableSat();
    if (rc)
        panic("sat::down: could not disable Satelite - rebooting..." CR);
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

void sat::onDiscovered(satelite* p_sateliteLibHandle, uint8_t p_satAddr, bool p_exists) {
    uint8_t link;
    link = linkHandle->getLink();
    Log.INFO("sat::onDiscovered: Satelite address %d on satlink %d discovered" CR, satAddr, link);
    if (p_satAddr != satAddr)
        panic("sat::onDiscovered: Inconsistant satelite address provided - rebooting..." CR);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        satLibHandle->satRegStateCb(onSatLibStateChangeHelper, this);
        satLibHandle->setErrTresh(SAT_LINKERR_HIGHTRES, SAT_LINKERR_LOWTRES);
        unSetOpStateByBitmap(OP_UNDISCOVERED);
    }
    else{
        satLibHandle = NULL;
        setOpStateByBitmap(OP_UNDISCOVERED);
    }
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++)
        acts[actItter]->onDiscovered(satLibHandle, p_exists);
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++)
        senses[sensItter]->onDiscovered(satLibHandle, p_exists);
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
        Log.ERROR("sat::onPmPoll: Failed to send PM report" CR);
}

void sat::onSenseChangeHelper(satelite* p_satelite, uint8_t p_LinkAddr, uint8_t p_satAddr, uint8_t p_senseAddr, bool p_senseVal, void* p_metadata) {
    ((sat*)p_metadata)->onSenseChange(p_senseAddr, p_senseVal);
}

void sat::onSenseChange(uint8_t p_senseAddr, bool p_senseVal) {
    if (!getOpStateBitmap() && p_senseAddr >= 0 && p_senseAddr < MAX_SENS)
        senses[p_senseAddr]->onSenseChange(p_senseVal);
    else
        Log.TERSE("sat::onSenseChange: Sensor has changed to %i, but satelite is not OP_WORKING, doing nothing..." CR, p_senseVal);
}

void sat::onSatLibStateChangeHelper(satelite * p_sateliteLibHandle, uint8_t p_linkAddr, uint8_t p_satAddr, satOpState_t p_satOpState, void* p_satHandle) {
    ((sat*)p_satHandle)->onSatLibStateChange(p_satOpState);
}

void sat::onSatLibStateChange(satOpState_t p_satOpState) {
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
    Log.INFO("sat::onSysStateChange: sat-%d has a new OP-state: %s" CR, satAddr, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    bool satDisableScan = false;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange)
        return;
    failsafe(newSysState != OP_WORKING);
    Log.INFO("sat::onSysStateChange: sat-%d has a new OP-state: %s, Setting failsafe" CR, satAddr, systemState::getOpStateStrByBitmap(newSysState, opStateStr));
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
        panic("sat::onSysStateChange: sat-%d has experienced an internal error - informing server and rebooting..." CR, satAddr);
        return;
    }
    if (newSysState & OP_INIT) {
        Log.INFO("sat::onSysStateChange: sat-%d is initializing - informing server if not already done" CR, satAddr);
        satDisableScan = true;
    }
    if (newSysState & OP_UNUSED) {
        Log.INFO("satLink::onSysStateChange: sat-%d is unused - informing server if not already done" CR, satAddr);
        satDisableScan = true;
    }
    if (newSysState & OP_ERRSEC) {
        Log.INFO("sat::onSysStateChange: sat-%d has experienced excessive PM errors - informing server if not already done" CR, satAddr);
    }
    if (newSysState & OP_GENERR) {
        Log.INFO("sat::onSysStateChange: sat-%d has experienced an error - informing server if not already done" CR, satAddr);
    }
    if (newSysState & OP_DISABLED) {
        Log.INFO("sat::onSysStateChange: sat-%d is disabled by server - disabling satscanning if not already done" CR, satAddr);
        satDisableScan = true;
    }
    if (newSysState & OP_CBL) {
        Log.INFO("sat::onSysStateChange: sat-%d is control-blocked by decoder - informing server and disabling satscanning if not already done" CR, satAddr);
        satDisableScan = true;
    }
    if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_ERRSEC | OP_GENERR | OP_DISABLED | OP_CBL)) {
        Log.INFO("sat::onSysStateChange: satLink-%d has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, satAddr, systemState::getOpStateStrByBitmap(newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL), opStateStr));
    }
    if (satDisableScan && !satScanDisabled) {
        Log.INFO("sat::onSysStateChange: satLink-%d disabling sat scanning" CR, satAddr);
        down();
        satScanDisabled = true;
    }
    else if (satDisableScan && satScanDisabled) {
        Log.INFO("sat::onSysStateChange: sat-%d scaning already disabled - doing nothing..." CR, satAddr);
    }
    else if (!satDisableScan && satScanDisabled) {
        Log.INFO("sat::onSysStateChange: sat-%d enabling sat scaning" CR, satAddr);
        up();
        satScanDisabled = false;
    }
    else if (!satDisableScan && !satScanDisabled) {
        Log.INFO("sat::onSysStateChange: sat-%d sat scan already enabled - doing nothing..." CR, satAddr);
    }
    prevSysState = newSysState;
}

void sat::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle) {
    ((sat*)p_satHandle)->onOpStateChange(p_topic, p_payload);
}

void sat::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    Log.INFO("sat::onOpStateChange: got a new opState from server: %s" CR, p_payload);
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
        Log.INFO("sat::onAdmStateChange: satelite address %d on satlink %d got online message from server" CR, satAddr, link);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        Log.INFO("sat::onAdmStateChange: satelite address %d on satlink %d got off-line message from server" CR, satAddr, link);
    }
    else
        Log.ERROR("sat::onAdmStateChange: satelite address %d on satlink %d got an invalid admstate message from server - doing nothing" CR, satAddr, link);
}

rc_t sat::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t sat::setSystemName(const char* p_systemName, bool p_force) {
    Log.ERROR("sat::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* sat::getSystemName(bool  p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("sat::getSystemName: cannot get System name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_SYSNAME];
}

rc_t sat::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("sat::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.ERROR("sat::setUsrName: cannot set System name as satelite is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("sat::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_SAT_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* sat::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("sat::getUsrName: cannot get User name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_USRNAME];;
}

rc_t sat::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("sat::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.ERROR("sat::setDesc: cannot set Description as satelite is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("sat::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SAT_DESC];
        xmlconfig[XML_SAT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* sat::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("sat::getDesc: cannot get Description as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_DESC];
}

rc_t sat::setAddr(uint8_t p_addr) {
    Log.ERROR("sat::setAddr: cannot set Address - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

uint8_t sat::getAddr(void) {
    return satAddr;
}

const char* sat::getLogLevel(void) {
    if (!transformLogLevelInt2XmlStr(Log.getLevel())) {
        Log.ERROR("sat::satLink: Could not retrieve a valid Log-level" CR);
        return NULL;
    }
    else {
        return transformLogLevelInt2XmlStr(Log.getLevel());
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
