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
#include "satLink.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "satLink(Satelite Link)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

satLink::satLink(uint8_t p_linkNo, decoder* p_decoderHandle) : systemState(p_decoderHandle), globalCli(SATLINK_MO_NAME, SATLINK_MO_NAME, p_linkNo, p_decoderHandle) {
    asprintf(&logContextName, "%s/%s-%i", p_decoderHandle->getLogContextName(), "satLink", p_linkNo);
    LOG_INFO("%s: Creating satLink channel %d" CR, logContextName);
    linkNo = p_linkNo;
    pmPoll = false;
    satLinkScanDisabled = true;
    debug = false;
    //We need to have this early since RMT requires internal RAM
    satLinkLibHandle = new (heap_caps_malloc(sizeof(sateliteLink), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) \
        sateliteLink(linkNo, (gpio_num_t)(SATLINK_TX_PINS[p_linkNo]),
        (gpio_num_t)(SATLINK_RX_PINS[p_linkNo]),
        (rmt_channel_t)(SATLINK_RMT_TX_CHAN[p_linkNo]),
        (rmt_channel_t)(SATLINK_RMT_RX_CHAN[p_linkNo]),
        SATLINK_RMT_TX_MEMBANK[p_linkNo],
        SATLINK_RMT_RX_MEMBANK[p_linkNo],
        CPU_SATLINK_PRIO,
        CPU_SATLINK_CORE[p_linkNo],
        SATLINK_UPDATE_MS);
    if (satLinkLibHandle == NULL) {
        panic("%s: Could not create satelite link library object for link channel", logContextName);
        return;
    }
    char sysStateObjName[20];
    sprintf(sysStateObjName, "satLink-%d", p_linkNo);
    setSysStateObjName(sysStateObjName);
    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    if (!(satLinkPmPollLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNUSED);
    regSysStateCb(this, &onSysStateChangeHelper);
    xmlconfig[XML_SATLINK_SYSNAME] = NULL;
    xmlconfig[XML_SATLINK_USRNAME] = NULL;
    xmlconfig[XML_SATLINK_DESC] = NULL;
    xmlconfig[XML_SATLINK_LINK] = NULL;
    xmlconfig[XML_SATLINK_ADMSTATE] = NULL;
    satLinkLibHandle->satLinkRegSatDiscoverCb(&onDiscoveredSateliteHelper, this);
    satLinkLibHandle->satLinkRegStateCb(&onSatLinkLibStateChangeHelper, this);
    satLinkLibHandle->setErrTresh(SATLINK_LINKERR_HIGHTRES, SATLINK_LINKERR_LOWTRES);
    txUnderunErr = 0;
    rxOverRunErr = 0;
    scanTimingViolationErr = 0;
    rxCrcErr = 0;
    remoteCrcErr = 0;
    rxSymbolErr = 0;
    rxDataSizeErr = 0;
    wdErr = 0;
}

satLink::~satLink(void) {
    panic("%s: satLink destructior not supported", logContextName);
}

rc_t satLink::init(void) {
    LOG_INFO("%s: Initializing satLink" CR, logContextName);
    /* CLI decoration methods */
    LOG_INFO("%s: Registering specific satLink CLI methods" CR, logContextName);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set satLinkNo
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKNO_SUB_MO_NAME, onCliGetLinkHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKNO_SUB_MO_NAME, SATLINK_GET_SATLINKNO_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, SATLINK_MO_NAME, SATLINKNO_SUB_MO_NAME, onCliSetLinkHelper);
    regCmdHelp(SET_CLI_CMD, SATLINK_MO_NAME, SATLINKNO_SUB_MO_NAME, SATLINK_SET_SATLINKNO_HELP_TXT);

    //get/clear Tx-underruns
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKTXUNDERRUN_SUB_MO_NAME, onCliGetTxUnderrunsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKTXUNDERRUN_SUB_MO_NAME, SATLINK_GET_TXUNDERRUNS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKTXUNDERRUN_SUB_MO_NAME, onCliClearTxUnderrunsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKTXUNDERRUN_SUB_MO_NAME, SATLINK_CLEAR_TXUNDERRUNS_HELP_TXT);

    //get/clear Tx-underruns
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXOVERRUN_SUB_MO_NAME, onCliGetRxOverrrunsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXOVERRUN_SUB_MO_NAME, SATLINK_GET_RXOVERRUNS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXOVERRUN_SUB_MO_NAME, onCliClearRxOverrunsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXOVERRUN_SUB_MO_NAME, SATLINK_CLEAR_RXOVERRUNS_HELP_TXT);

    //get/clear scan timing violations
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKTIMINGVIOLATION_SUB_MO_NAME, onCliGetScanTimingViolationsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKTIMINGVIOLATION_SUB_MO_NAME, SATLINK_GET_TIMINGVIOLATION_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKTIMINGVIOLATION_SUB_MO_NAME, onCliClearScanTimingViolationsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKTIMINGVIOLATION_SUB_MO_NAME, SATLINK_CLEAR_TIMINGVIOLATION_HELP_TXT);

    //get/clear rx crc errors
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXCRCERR_SUB_MO_NAME, onCliGetRxCrcErrsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXCRCERR_SUB_MO_NAME, SATLINK_GET_RXCRCERR_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXCRCERR_SUB_MO_NAME, onCliClearRxCrcErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXCRCERR_SUB_MO_NAME, SATLINK_CLEAR_RXCRCERR_HELP_TXT);

    //get/clear remote crc errors
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKREMOTECRCERR_SUB_MO_NAME, onCliGetRemoteCrcErrsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKREMOTECRCERR_SUB_MO_NAME, SATLINK_GET_REMOTECRCERR_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKREMOTECRCERR_SUB_MO_NAME, onCliClearRemoteCrcErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKREMOTECRCERR_SUB_MO_NAME, SATLINK_CLEAR_REMOTECRCERR_HELP_TXT);

    //get/clear rx symbol errors
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSYMERRS_SUB_MO_NAME, onCliGetRxSymbolErrsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSYMERRS_SUB_MO_NAME, SATLINK_GET_RXSYMERRS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSYMERRS_SUB_MO_NAME, onCliClearRxSymbolErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSYMERRS_SUB_MO_NAME, SATLINK_CLEAR_RXSYMERRS_HELP_TXT);

    //get/clear rx size errors
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSIZEERRS_SUB_MO_NAME, onCliGetRxDataSizeErrsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSIZEERRS_SUB_MO_NAME, SATLINK_GET_RXSIZEERRS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSIZEERRS_SUB_MO_NAME, onCliClearRxDataSizeErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKRXSIZEERRS_SUB_MO_NAME, SATLINK_CLEAR_RXSIZEERRS_HELP_TXT);

    //get/clear watchdog errors
    regCmdMoArg(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKWDERRS_SUB_MO_NAME, onCliGetWdErrsHelper);
    regCmdHelp(GET_CLI_CMD, SATLINK_MO_NAME, SATLINKWDERRS_SUB_MO_NAME, SATLINK_GET_WDERRS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKWDERRS_SUB_MO_NAME, onCliClearWdErrsHelper);
    regCmdHelp(CLEAR_CLI_CMD, SATLINK_MO_NAME, SATLINKWDERRS_SUB_MO_NAME, SATLINK_CLEAR_WDERRS_HELP_TXT);
    LOG_INFO("%s: specific satLink CLI methods registered" CR, logContextName);
    LOG_INFO("%s: Creating satelites for link channel %d" CR, logContextName);
    for (uint8_t satAddress = 0; satAddress < MAX_SATELITES; satAddress++) {
        sats[satAddress] = new (heap_caps_malloc(sizeof(sat), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) sat(satAddress, this);
        if (sats[satAddress] == NULL){
            panic("%s: Could not create satelite object", logContextName);
            return RC_OUT_OF_MEM_ERR;
        }
        addSysStateChild(sats[satAddress]);
        sats[satAddress]->init();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    return RC_OK;
}

void satLink::onConfig(tinyxml2::XMLElement* p_satLinkXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)) {
        panic("%s: satLink received a configuration, while the it was already configured, dynamic re-configuration not supported", logContextName);
        return;
    }
    LOG_INFO("%s: satLink received an unverified configuration, parsing and validating it..." CR, logContextName);
    //PARSING CONFIGURATION
    const char* satLinkSearchTags[5];
    satLinkSearchTags[XML_SATLINK_SYSNAME] = "SystemName";
    satLinkSearchTags[XML_SATLINK_USRNAME] = "UserName";
    satLinkSearchTags[XML_SATLINK_DESC] = "Description";
    satLinkSearchTags[XML_SATLINK_LINK] = "Link";
    satLinkSearchTags[XML_SATLINK_ADMSTATE] = "AdminState";
    getTagTxt(p_satLinkXmlElement->FirstChildElement(), satLinkSearchTags, xmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_SATLINK_SYSNAME]){
        panic("%s: SystemName missing", logContextName);
        return;
    }
    if (!xmlconfig[XML_SATLINK_USRNAME]){
        LOG_WARN("%s: User name was not provided - using \"%s-UserName\"" CR, logContextName, xmlconfig[XML_SATLINK_SYSNAME]);
        xmlconfig[XML_SATLINK_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_SATLINK_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_SATLINK_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_SATLINK_USRNAME], "%s-Username", xmlconfig[XML_SATLINK_SYSNAME]);
    }
    if (!xmlconfig[XML_SATLINK_DESC]){
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_SATLINK_DESC] = createNcpystr("-");
    }
    if (!xmlconfig[XML_SATLINK_LINK]){
        panic("%s: Link missing", logContextName);
        return;
    }
    if (atoi(xmlconfig[XML_SATLINK_LINK]) != linkNo){
        panic("%s: Link number inconsistant with what was provided in the object constructor", logContextName);
        return;
    }
    if (xmlconfig[XML_SATLINK_ADMSTATE] == NULL) {
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_SATLINK_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_SATLINK_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_SATLINK_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else{
        panic("%s: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", logContextName, xmlconfig[XML_SATLINK_ADMSTATE]);
        return;
    }
    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: System name: %s" CR, logContextName, xmlconfig[XML_SATLINK_SYSNAME]);
    LOG_INFO("%s: User name: %s" CR, logContextName, xmlconfig[XML_SATLINK_USRNAME]);
    LOG_INFO("%s: Description: %s" CR, logContextName, xmlconfig[XML_SATLINK_DESC]);
    LOG_INFO("%s: Link: %s" CR, logContextName, xmlconfig[XML_SATLINK_LINK]);
    LOG_INFO("%s: satLink admin state: %s" CR, logContextName, xmlconfig[XML_SATLINK_ADMSTATE]);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    //CONFIFIGURING SATELITES
    LOG_INFO("%s: Configuring Satelites" CR, logContextName);
    tinyxml2::XMLElement* satXmlElement;
    if (satXmlElement = ((tinyxml2::XMLElement*)p_satLinkXmlElement)->FirstChildElement("Satelite")) {
        const char* satSearchTags[4];
        satSearchTags[XML_SAT_SYSNAME] = NULL;
        satSearchTags[XML_SAT_USRNAME] = NULL;
        satSearchTags[XML_SAT_DESC] = NULL;
        satSearchTags[XML_SAT_ADDR] = "LinkAddress";
        for (uint16_t satItter = 0; true; satItter++) {
            char* satXmlConfig[4] = { NULL };
            if (satXmlElement == NULL)
                break;
            if (satItter >= MAX_SATELITES){
                panic("%s: More than maximum sats provided (%i/%i)", logContextName, satItter, MAX_SATELITES);
                return;
            }
            getTagTxt(satXmlElement->FirstChildElement(), satSearchTags, satXmlConfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability
            if (!satXmlConfig[XML_SAT_ADDR]){
                panic("%s: satLink address missing", logContextName);
                return;
            }
            if ((atoi(satXmlConfig[XML_SAT_ADDR])) < 0 || atoi(satXmlConfig[XML_SAT_ADDR]) >= MAX_SATELITES){
                panic("%s: satLink address (%i) out of bounds", logContextName, atoi(satXmlConfig[XML_SAT_ADDR]));
                return;
            }
            LOG_INFO("%s: Configuring satLink" CR, logContextName);
            sats[atoi(satXmlConfig[XML_SAT_ADDR])]->onConfig(satXmlElement);
            satXmlElement = ((tinyxml2::XMLElement*)satXmlElement)->NextSiblingElement("Satelite");
            vTaskDelay(5 / portTICK_PERIOD_MS);
        }
    }
    else
        LOG_WARN("%s: No Satelites provided, no Satelites will be configured" CR, logContextName);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("%s: satLink configuration successfully finished" CR, logContextName);
}

rc_t satLink::start(void) {
    LOG_INFO("%s: Starting Satelite link" CR, logContextName);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_INFO("%s: satLink not configured - will not start it" CR, logContextName);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    LOG_INFO("%s: Subscribing to adm- and op state topics" CR, logContextName);
    char suscribeTopic[300];
    sprintf(suscribeTopic, "%s%s%s%s%s", MQTT_SATLINK_ADMSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(suscribeTopic, onAdmStateChangeHelper, this)){
        panic("%s: Failed to suscribe to admState topic: \"%s\"", logContextName, suscribeTopic);
        return RC_GEN_ERR;
    }
    sprintf(suscribeTopic, "%s%s%s%s%s", MQTT_SATLINK_OPSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(suscribeTopic, onOpStateChangeHelper, this)){
        panic("%s: Failed to suscribe to opState topic: \"%s\"", logContextName, suscribeTopic);
        return RC_GEN_ERR;
    }
    for (uint16_t satItter = 0; satItter < MAX_SATELITES; satItter++) {
        sats[satItter]->start();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    unSetOpStateByBitmap(OP_INIT);
    LOG_INFO("%s: satLink and all its satelites have started" CR, logContextName);
    return RC_OK;
}

void satLink::onDiscoveredSateliteHelper(satelite* p_sateliteLibHandle, uint8_t p_satLink, uint8_t p_satAddr, bool p_exists, void* p_satLinkHandle) {
    if (p_satLink != ((satLink*)p_satLinkHandle)->linkNo){
        panic("Inconsistant link number, expected %i, got %i", ((satLink*)p_satLinkHandle)->linkNo, p_satLink);
        return;
    }
    if ((p_satAddr >= MAX_SATELITES) && p_exists) {
        panic("More than maximum (%i) allowed satelites discovered (%i) on the link", MAX_SATELITES, p_satAddr + 1);
        return;
    }
    if(p_satAddr < MAX_SATELITES)
        ((satLink*)p_satLinkHandle)->sats[p_satAddr]->onDiscovered(p_sateliteLibHandle, p_satAddr, p_exists);
}

void satLink::up(void) {
    if (!satLinkScanDisabled) {
        LOG_INFO("%s: satLink already enabled" CR, logContextName);
        return;
    }
    satLinkScanDisabled = false;
    LOG_INFO("%s: satLink link- and PM- scanning starting" CR, logContextName);
    satErr_t rc = satLinkLibHandle->enableSatLink();
    if (rc != 0){
        panic("%s: satLink scannig could not be started, return code: 0x%llx", logContextName, rc);
        return;
    }
    char satlinkPmTaskName[30];
    sprintf(satlinkPmTaskName, CPU_SATLINK_PM_TASKNAME, linkNo);
    pmPoll = true;
    if (!eTaskCreate(
        pmPollHelper,                                                                   // Task function
        satlinkPmTaskName,                                                              // Task function name reference
        CPU_SATLINK_PM_STACKSIZE_1K * 1024,                                             // Stack size
        this,                                                                           // Parameter passing
        CPU_SATLINK_PM_PRIO,                                                            // Priority 0-24, higher is more
        CPU_SATLINK_PM_STACK_ATTR)) {                                                     // Task stack atribute
        panic("%s: Could not start pm poll task", logContextName);
        return;
    }
}

void satLink::down(void) {
    if (satLinkScanDisabled) {
        LOG_INFO("%s: satLink already disabled" CR, logContextName);
        return;
    }
    LOG_INFO("%s: satLink link- and PM- scanning stopping" CR, logContextName);
    satLinkScanDisabled = true;
    pmPoll = false;
    satErr_t rc = satLinkLibHandle->disableSatLink();
    if (rc)
        LOG_ERROR("%s: Could not disable satLink link scanning, return code: %llx" CR, logContextName, rc);
}

void satLink::pmPollHelper(void* p_metaData) {
    ((satLink*)p_metaData)->onPmPoll();
}

void satLink::onPmPoll(void) {
    if (xSemaphoreTake(satLinkPmPollLock, 0) == pdFALSE) {
        LOG_VERBOSE("%s: Did not have time to exit the pmPoll loop from when pmPolling was ordered to be stop, no need to re-enter" CR, logContextName);
        vTaskDelete(NULL);
    }
    LOG_INFO("%s: Starting PM polling for satLink" CR, logContextName);
    int64_t  nextLoopTime = esp_timer_get_time();
    int64_t  thisLoopTime;
    TickType_t delay;
    while (pmPoll) {
        thisLoopTime = nextLoopTime;
        nextLoopTime += 1000000; //1E6 uS = 1S
        satPerformanceCounters_t pmData;
        satLinkLibHandle->getSatStats(&pmData, true);
        txUnderunErr += pmData.txUnderunErr;
        rxOverRunErr += pmData.rxOverRunErr;
        scanTimingViolationErr += pmData.scanTimingViolationErr;
        rxCrcErr += pmData.rxCrcErr;
        remoteCrcErr += pmData.remoteCrcErr;
        rxSymbolErr += pmData.rxSymbolErr;
        rxDataSizeErr += pmData.rxDataSizeErr;
        wdErr += pmData.wdErr;
        char publishPmTopic[300];
        sprintf(publishPmTopic, "%s%s%s%s%s", MQTT_SATLINK_STATS_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
        char publishPmPayload[150];
        sprintf(publishPmPayload, "<statReport>\n"
            "<rxCrcErr>%d</rxCrcErr>\n"
            "<remCrcErr> %d</remCrcErr>\n"
            "<rxSymErr> %d</rxSymErr>\n"
            "<rxSizeErr> %d</rxSizeErr>\n"
            "<wdErr> %d</wdErr>\n"
            "</statReport>",
            pmData.rxCrcErr,
            pmData.remoteCrcErr,
            pmData.rxSymbolErr,
            pmData.rxDataSizeErr,
            pmData.wdErr);
        if (mqtt::sendMsg(publishPmTopic, publishPmPayload, false)) {
            LOG_ERROR("%s: Failed to send PM report" CR, logContextName);
        }
        for (uint8_t i = 0; i < MAX_SATELITES; i++) {
            sats[i]->onPmPoll();
        }
        if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
        }
    }
    xSemaphoreGive(satLinkPmPollLock);                                                 //Here is a microscopic risk for a race-condition, probably a few ten cycles from when the while condition is false before we reach here
    LOG_INFO("%s: Stopping PM polling for satLink" CR, logContextName);
    vTaskDelete(NULL);
}

void satLink::onSatLinkLibStateChangeHelper(sateliteLink* p_sateliteLinkLibHandler, uint8_t p_linkAddr, satOpState_t p_satOpState, void* p_satLinkHandler) {
    ((satLink*)p_satLinkHandler)->onSatLinkLibStateChange(p_satOpState);
}

void satLink::onSatLinkLibStateChange(const satOpState_t p_satOpState) {
    if(p_satOpState & (SAT_OP_INIT | SAT_OP_FAIL | SAT_OP_CONTROLBOCK))
        systemState::setOpStateByBitmap(OP_GENERR);
    else
        systemState::unSetOpStateByBitmap(OP_GENERR);
    if (p_satOpState & SAT_OP_ERR_SEC)
        systemState::setOpStateByBitmap(OP_ERRSEC);
    else
        systemState::unSetOpStateByBitmap(OP_ERRSEC);
}

void satLink::onSysStateChangeHelper(const void* p_satLinkHandle, sysState_t p_sysState) {
    ((satLink*)p_satLinkHandle)->onSysStateChange(p_sysState);
}

void satLink::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: satLink has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    bool satLinkDisableScan = false;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange)
        return;
    LOG_INFO("%s: satLink has a new OP-state: %s" CR, logContextName, systemState::getOpStateStr(opStateStr));
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SATLINK_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if ((newSysState & OP_INTFAIL)) {
        satLinkDisableScan = true;
        down();
        prevSysState = newSysState;
        panic("%s: satLink has experienced an internal error - informing server", logContextName);
        return;
    }
    if (newSysState & OP_INIT) {
        LOG_INFO("%s: satLink is initializing - informing server if not already done" CR, logContextName);
        satLinkDisableScan = true;
    }
    if (newSysState & OP_UNUSED) {
        LOG_INFO("%s: satLink is unused - informing server if not already done" CR, logContextName);
        satLinkDisableScan = true;
    }
    if (newSysState & OP_ERRSEC) {
        LOG_INFO("%s: satLink has experienced excessive PM errors - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_GENERR) {
        LOG_INFO("%s: satLink has experienced an error - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_DISABLED) {
        LOG_INFO("%s: satLink is disabled by server - disabling linkscanning if not already done" CR, logContextName);
        satLinkDisableScan = true;
    }
    if (newSysState & OP_CBL) {
        LOG_INFO("%s: satLink is control-blocked by decoder - informing server and disabling scan if not already done" CR, logContextName);
        satLinkDisableScan = true;
    }
    if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_ERRSEC | OP_GENERR | OP_DISABLED | OP_CBL)) {
        LOG_INFO("%s: satLink has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL), opStateStr));
    }
    if (satLinkDisableScan && !satLinkScanDisabled) {
        LOG_INFO("%s: disabling satLink scaning" CR, logContextName);
        down();
    }
    else if (satLinkDisableScan && satLinkScanDisabled)
        LOG_INFO("%s: satLink link scaning already disabled - doing nothing..." CR, logContextName);
    else if (!satLinkDisableScan && satLinkScanDisabled) {
        LOG_INFO("%s: Enabling satLink scaning" CR, logContextName);
        up();
    }
    else if (!satLinkDisableScan && !satLinkScanDisabled)
        LOG_INFO("%s: satLink scan already enabled - doing nothing..." CR, logContextName);
    prevSysState = newSysState;
}

void satLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onOpStateChange(p_topic, p_payload);
}

void satLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: Got a new opState from server: %s" CR, logContextName, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void satLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void satLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: Got online message from server" CR, logContextName);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: Got off-line message from server" CR, logContextName);
    }
    else
        LOG_ERROR("%s: Got an invalid admstate message from server: \"%s\" - doing nothing" CR, logContextName, p_payload);
}

rc_t satLink::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t satLink::setSystemName(const char* p_systemName, const bool p_force) {
    LOG_ERROR("%s: Cannot set System name - not suppoted" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* satLink::getSystemName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as satLink is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_SYSNAME];
}

rc_t satLink::setUsrName(const char* p_usrName, const bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set System name as satLink is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting User name to %s" CR, logContextName, p_usrName);
        delete xmlconfig[XML_SATLINK_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* satLink::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as satLink is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_USRNAME];
}

rc_t satLink::setDesc(const char* p_description, const bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Description as satLink is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting Description to %s" CR, logContextName, p_description);
        delete xmlconfig[XML_SATLINK_DESC];
        xmlconfig[XML_SATLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* satLink::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as satLink is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_DESC];
}

rc_t satLink::setLink(uint8_t p_link, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Satelite link No as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Satelite link No as satLink is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    LOG_ERROR("%s: Cannot set Satelite link No - not supported" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

uint8_t satLink::getLink(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Satelite link No as satLink is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    return linkNo;
}

const char* satLink::getLogLevel(void) {
    if (!Log.transformLogLevelInt2XmlStr(Log.getLogLevel())) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
}

void satLink::setDebug(const bool p_debug) {
    debug = p_debug;
}

bool satLink::getDebug(void) {
    return debug;
}

uint32_t satLink::getTxUnderruns(void) {
    return txUnderunErr;
}

void satLink::clearTxUnderruns(void) {
    txUnderunErr = 0;
}

uint32_t satLink::getRxOverruns(void) {
    return rxOverRunErr;
}

void satLink::clearRxOverruns(void) {
    rxOverRunErr = 0;
}

uint32_t satLink::getScanTimingViolations(void) {
    return scanTimingViolationErr;
}

void satLink::clearScanTimingViolations(void) {
    scanTimingViolationErr = 0;
}

uint32_t satLink::getRxCrcErrs(void) {
    return rxCrcErr;
}

void satLink::clearRxCrcErrs(void) {
    rxCrcErr = 0;
}

uint32_t satLink::getRemoteCrcErrs(void) {
    return rxCrcErr;
}

void satLink::clearRemoteCrcErrs(void) {
    rxCrcErr = 0;
}

uint32_t satLink::getRxSymbolErrs(void) {
    return rxSymbolErr;
}

void satLink::clearRxSymbolErrs(void) {
    rxSymbolErr = 0;
}

uint32_t satLink::getRxDataSizeErrs(void) {
    return rxDataSizeErr;
}

void satLink::clearRxDataSizeErrs(void) {
    rxDataSizeErr = 0;
}

uint32_t satLink::getWdErrs(void) {
    return wdErr;
}

void satLink::clearWdErrs(void) {
    wdErr = 0;
}

//Statistics not yet existing from the Satelite library
//int64_t satLink::getMeanLatency(void) {}
//int64_t satLink::getMaxLatency(void) {}
//void satLink::clearMaxLatency(void) {}
// uint32_t satLink::getMeanRuntime(void) {}
//uint32_t satLink::getMaxRuntime(void) {}
//void satLink::clearMaxRuntime(void) {}

const char* satLink::getLogContextName(void) {
    return logContextName;
}

/* CLI decoration methods */
void satLink::onCliGetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link: %i", static_cast<satLink*>(p_cliContext)->getLink());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliSetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<satLink*>(p_cliContext)->setLink(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lgLink, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetTxUnderrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i TX-underruns: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getTxUnderruns());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearTxUnderrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearTxUnderruns();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetRxOverrrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i RX-overruns: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getTxUnderruns());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearRxOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearRxOverruns();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetScanTimingViolationsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i RX-Timing-violations: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getScanTimingViolations());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearScanTimingViolationsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearScanTimingViolations();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
        Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i RX-CRC-Errors: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getRxCrcErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearRxCrcErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetRemoteCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i Remote-CRC-Errors: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getRemoteCrcErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearRemoteCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearRemoteCrcErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetRxSymbolErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i RX-Symbol-Errors: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getRxSymbolErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearRxSymbolErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearRxSymbolErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetRxDataSizeErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i RX-Size-Errors: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getRxDataSizeErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearRxDataSizeErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->getRxDataSizeErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void satLink::onCliGetWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Satelite-link %i Watchdog-Errors: %i", static_cast<satLink*>(p_cliContext)->getLink(), static_cast<satLink*>(p_cliContext)->getWdErrs());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void satLink::onCliClearWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<satLink*>(p_cliContext)->clearWdErrs();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

/*==============================================================================================================================================*/
/* END Class satLink                                                                                                                            */
/*==============================================================================================================================================*/
