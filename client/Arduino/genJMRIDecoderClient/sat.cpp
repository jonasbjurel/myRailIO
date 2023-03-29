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

uint16_t sat::satIndex = 0;

sat::sat(uint8_t p_satAddr, satLink* p_linkHandle) : systemState(this), globalCli(SAT_MO_NAME, SAT_MO_NAME, satIndex) {
    Log.INFO("sat::sat: Creating Satelite adress %d" CR, p_satAddr);
    satIndex++;
    linkHandle = p_linkHandle;
    satAddr = p_satAddr;
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    satLock = xSemaphoreCreateMutex();
    if (satLock == NULL) {
        panic("sat::sat: Could not create Lock objects - rebooting...");
    }
    /* CLI decoration methods */
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
}


sat::~sat(void) {
    panic("sat::~sat: sat destructior not supported - rebooting...");
}

rc_t sat::init(void) {
    uint8_t link;
    linkHandle->getLink(&link);
    Log.INFO("sat::init: Initializing Satelite address %d" CR, satAddr);
    Log.INFO("sat::init: Creating actuators for satelite address %d on link %d" CR, satAddr, link);
    for (uint8_t actPort = 0; actPort < MAX_ACT; actPort++) {
        acts[actPort] = new actBase(actPort, this);
        if (acts[actPort] == NULL)
            panic("sat::init: Could not create actuator object - rebooting...");
    }
    Log.INFO("sat::init: Creating sensors for satelite address %d on link %d" CR, satAddr, link);
    for (uint8_t sensPort = 0; sensPort < MAX_SENS; sensPort++) {
        senses[sensPort] = new senseBase(sensPort, this);
        if (senses[sensPort] == NULL)
            panic("sat::init: Could not create sensor object - rebooting..." CR);
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
    if (~(systemState::getOpState() & OP_UNCONFIGURED))
        panic("sat:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    uint8_t link;
    linkHandle->getLink(&link);
    Log.INFO("sat::onConfig: satAddress %d on link %d received an uverified configuration, parsing and validating it..." CR, satAddr, link);
    xmlconfig[XML_SAT_SYSNAME] = NULL;
    xmlconfig[XML_SAT_USRNAME] = NULL;
    xmlconfig[XML_SAT_DESC] = NULL;
    xmlconfig[XML_SAT_ADDR] = NULL;
    const char* satSearchTags[4];
    satSearchTags[XML_SAT_SYSNAME] = "SystemName";
    satSearchTags[XML_SAT_USRNAME] = "UserName";
    satSearchTags[XML_SAT_DESC] = "Description";
    satSearchTags[XML_SAT_ADDR] = "Address";
    getTagTxt(p_satXmlElement, satSearchTags, xmlconfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_SAT_USRNAME])
        panic("sat::onConfig: SystemNane missing - rebooting...");
    if (!xmlconfig[XML_SAT_USRNAME])
        panic("sat::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SAT_DESC])
        panic("sat::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SAT_ADDR])
        panic("sat::onConfig: Adrress missing - rebooting...");
    if (atoi(xmlconfig[XML_SAT_ADDR]) != satAddr)
        panic("sat::onConfig: Address no inconsistant - rebooting...");
    Log.INFO("sat::onConfig: System name: %s" CR, xmlconfig[XML_SAT_SYSNAME]);
    Log.INFO("sat::onConfig: User name:" CR, xmlconfig[XML_SAT_USRNAME]);
    Log.INFO("sat::onConfig: Description: %s" CR, xmlconfig[XML_SAT_DESC]);
    Log.INFO("sat::onConfig: Address: %s" CR, xmlconfig[XML_SAT_ADDR]);
    Log.INFO("sat::onConfig: Configuring Actuators");
    tinyxml2::XMLElement* actXmlElement;
    actXmlElement = p_satXmlElement->FirstChildElement("Actuator");
    const char* actSearchTags[6];
    char* actXmlConfig[6];
    for (uint8_t actItter = 0; false; actItter++) {
        if (actXmlElement == NULL)
            break;
        if (actItter >= MAX_ACT)
            panic("sat::onConfig: > than max actuators provided - not supported, rebooting...");
        actSearchTags[XML_ACT_PORT] = "Port";
        getTagTxt(actXmlElement, actSearchTags, actXmlConfig, sizeof(actXmlElement) / 4); // Need to fix the addressing for portability
        if (!actXmlConfig[XML_ACT_PORT])
            panic("sat::onConfig:: Actuator port missing - rebooting...");
        acts[atoi(actXmlConfig[XML_ACT_PORT])]->onConfig(actXmlElement);
        addSysStateChild(acts[atoi(actXmlConfig[XML_ACT_PORT])]);
        actXmlElement = p_satXmlElement->NextSiblingElement("Actuator");
    }
    Log.INFO("sat::onConfig: Configuring sensors");
    tinyxml2::XMLElement* sensXmlElement;
    sensXmlElement = p_satXmlElement->FirstChildElement("Sensor");
    const char* sensSearchTags[5];
    char* sensXmlConfig[5];
    for (uint8_t sensItter = 0; false; sensItter++) {
        if (actXmlElement == NULL)
            break;
        if (sensItter >= MAX_SENS)
            panic("sat::onConfig: > than max sensors provided - not supported, rebooting...");
        sensSearchTags[XML_SENS_PORT] = "Port";
        getTagTxt(sensXmlElement, sensSearchTags, sensXmlConfig, sizeof(sensXmlElement) / 4); // Need to fix the addressing for portability
        if (!sensXmlConfig[XML_SENS_PORT])
            panic("sat::onConfig:: Sensor port missing - rebooting..." CR);
        senses[atoi(sensXmlConfig[XML_SENS_PORT])]->onConfig(sensXmlElement);
        addSysStateChild(senses[atoi(sensXmlConfig[XML_SENS_PORT])]);
        sensXmlElement = p_satXmlElement->NextSiblingElement("Sensor");
    }
    unSetOpState(OP_UNCONFIGURED);
    Log.INFO("sat::onConfig: Configuration successfully finished" CR);
}

rc_t sat::start(void) {
    uint8_t link;
    linkHandle->getLink(&link);
    Log.INFO("sat::start: Starting Satelite address: %d on satlink %d" CR, satAddr, link);
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.INFO("sat::start: Satelite address %d on satlink %d not configured - will not start it" CR, satAddr, link);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (systemState::getOpState() & OP_UNDISCOVERED) {
        Log.INFO("sat::start: Satelite address %d on satlink %d not yet discovered - waiting for discovery before starting it" CR, satAddr, link);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.INFO("sat::start: Subscribing to adm- and op state topics");
    const char* admSubscribeTopic[5] = { MQTT_SAT_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), onAdmStateChangeHelper, this))
        panic("sat::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_DECODER_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName() };
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), onOpStateChangeHelper, this))
        panic("sat::start: Failed to suscribe to opState topic - rebooting...");
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++) {
        acts[actItter]->start();
    }
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++) {
        senses[sensItter]->start();
    }
    
    satLibHandle->satRegStateCb(&onSatLibStateChangeHelper, this);
    satLibHandle->setErrTresh(0, 0);
    rc_t rc = satLibHandle->enableSat();
    if (rc)
        panic("sat::start: could not enable Satelite - rebooting...");
    unSetOpState(OP_INIT);
    Log.INFO("sat::start: Satelite address: %d on satlink %d have started" CR, satAddr, link);
    return RC_OK;
}

void sat::onDiscovered(satelite* p_sateliteLibHandle, uint8_t p_satAddr, bool p_exists) {
    uint8_t link;
    linkHandle->getLink(&link);
    Log.INFO("sat::onDiscovered: Satelite address %d on satlink %d discovered" CR, satAddr, link);
    if (p_satAddr != satAddr)
        panic("sat::onDiscovered: Inconsistant satelite address provided - rebooting...");
    satLibHandle = p_sateliteLibHandle;
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++)
        acts[actItter]->onDiscovered(satLibHandle);
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++)
        senses[sensItter]->onDiscovered(satLibHandle);
    unSetOpState(OP_UNDISCOVERED);
    if (pendingStart) {
        Log.INFO("sat::onDiscovered: Satelite address %d on satlink %d was waiting for discovery before it could be started - now starting it" CR, satAddr, link);
        rc_t rc = start();
        if (rc)
            panic("sat::onDiscovered: Could not start Satelite - rebooting...");
    }
}

void sat::onPmPoll(void) {
    satPerformanceCounters_t pmData;
    satLibHandle->getSatStats(&pmData, true);
    rxCrcErr += pmData.rxCrcErr;
    remoteCrcErr += pmData.remoteCrcErr;
    wdErr += pmData.wdErr;
    const char* pmPublishTopic[5] = { MQTT_SAT_STATS_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
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
    if (mqtt::sendMsg(concatStr(pmPublishTopic, 4), 
                                pmPublishPayload,
                                false))
        Log.ERROR("sat::onPmPoll: Failed to send PM report" CR);
}

void sat::onSatLibStateChangeHelper(satelite * p_sateliteLibHandle, uint8_t p_linkAddr, uint8_t p_satAddr, satOpState_t p_satOpState, void* p_satHandle) {
    ((sat*)p_satHandle)->onSatLibStateChange(p_satOpState);
}

void sat::onSatLibStateChange(satOpState_t p_satOpState) {
    if (!(systemState::getOpState() & OP_INIT)) {
        if (p_satOpState)
            setOpState(OP_INTFAIL);
        else
            unSetOpState(OP_INTFAIL);
    }
}

void sat::onSysStateChangeHelper(const void* p_satHandle, uint16_t p_sysState) {
    ((sat*)p_satHandle)->onSysStateChange(p_sysState);
}

void sat::onSysStateChange(uint16_t p_sysState) {
    uint8_t link;
    linkHandle->getLink(&link);
    if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
        Log.INFO("sat::onSystateChange: satelite address %d on satlink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, satAddr, link);
    else if (p_sysState & OP_INTFAIL)
        panic("sat::onSystateChange: satelite has experienced an internal error - rebooting...");
    if (p_sysState)
        Log.INFO("sat::onSystateChange: satelite address %d on satlink %d has received Opstate %b - doing nothing" CR, satAddr, link, p_sysState);
    else
        Log.INFO("sat::onSystateChange: satelite address %d on satlink %d has received a cleared Opstate - doing nothing" CR, satAddr, link);
}

void sat::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle) {
    ((sat*)p_satHandle)->onOpStateChange(p_topic, p_payload);
}

void sat::onOpStateChange(const char* p_topic, const char* p_payload) {
    uint8_t link;
    linkHandle->getLink(&link);
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.INFO("sat::onOpStateChange: satelite address %d on satlink %d got available message from server" CR, satAddr, link);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.INFO("sat::onOpStateChange: satelite address %d on satlink %d got unavailable message from server" CR, satAddr, link);
    }
    else
        Log.ERROR("sat::onOpStateChange: satelite address %d on satlink %d got an invalid availability message from server - doing nothing" CR, satAddr, link);
}

void sat::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkHandle) {
    ((satLink*)p_satLinkHandle)->onAdmStateChange(p_topic, p_payload);
}

void sat::onAdmStateChange(const char* p_topic, const char* p_payload) {
    uint8_t link;
    linkHandle->getLink(&link);
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.INFO("sat::onAdmStateChange: satelite address %d on satlink %d got online message from server" CR, satAddr, link);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.INFO("sat::onAdmStateChange: satelite address %d on satlink %d got off-line message from server" CR, satAddr, link);
    }
    else
        Log.ERROR("sat::onAdmStateChange: satelite address %d on satlink %d got an invalid admstate message from server - doing nothing" CR, satAddr, link);
}

rc_t sat::getOpStateStr(char* p_opStateStr) {
    return systemState::getOpStateStr(p_opStateStr);
}

rc_t sat::setSystemName(const char* p_systemName, bool p_force) {
    Log.ERROR("sat::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* sat::getSystemName(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
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
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
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

const char* sat::getUsrName(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("sat::getUsrName: cannot get User name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_USRNAME];
}

rc_t sat::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("sat::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
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

const char* sat::getDesc(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("sat::getDesc: cannot get Description as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_DESC];
}

rc_t sat::setAddr(uint8_t p_addr) {
    Log.ERROR("sat::setAddr: cannot set Address - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t sat::getAddr(uint8_t* p_addr) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::getLink: cannot get Link No as satLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    *p_addr = atoi(xmlconfig[XML_SAT_ADDR]);
    return RC_OK;
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
    rc_t rc;
    uint8_t addr;
    if (rc = static_cast<sat*>(p_cliContext)->getAddr(&addr)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Satelit address, return code: %i", rc);
        return;
    }
    printCli("Satelite address: %i", addr);
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
