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

uint16_t satLink::satLinkIndex = 0;

satLink::satLink(uint8_t p_linkNo, decoder* p_decoderHandle) : systemState(p_decoderHandle), globalCli(SATLINK_MO_NAME, SATLINK_MO_NAME, satLinkIndex) {
    Log.INFO("satLink::satLink: Creating Satelite link channel %d" CR, p_linkNo);
    if (satLinkIndex++ > MAX_SATLINKS)
        panic("satLink::satLink:Number of configured satLinks exceeds maximum configured by [MAX_SATLINKS: %d] - rebooting ..." CR, MAX_SATLINKS);
    //We need to have this early since RMT requires internal RAM
    satLinkLibHandle = new sateliteLink(linkNo, (gpio_num_t)(SATLINK_TX_PINS[p_linkNo]),
        (gpio_num_t)(SATLINK_RX_PINS[p_linkNo]),
        (rmt_channel_t)(SATLINK_RMT_TX_CHAN[p_linkNo]),
        (rmt_channel_t)(SATLINK_RMT_RX_CHAN[p_linkNo]),
        SATLINK_RMT_TX_MEMBANK[p_linkNo],
        SATLINK_RMT_RX_MEMBANK[p_linkNo],
        CPU_SATLINK_PRIO,
        CPU_SATLINK_CORE[p_linkNo],
        SATLINK_UPDATE_MS);
    if (satLinkLibHandle == NULL)
        panic("satLink::satLink: Could not create satelite link library object for link channel - rebooting..." CR);
    char sysStateObjName[20];
    sprintf(sysStateObjName, "satLink-%d", p_linkNo);
    setSysStateObjName(sysStateObjName);
    linkNo = p_linkNo;
    pmPoll = false;
    satLinkDownDeclared = false;
    satLinkScanDisabled = true;
    processingSysState = false;
    sysStateQ = new QList<sysState_t*>;
    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    //if (!(satLinkSysStateLock = xSemaphoreCreateMutex()))
    if (!(satLinkSysStateLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
        panic("satLink::satLink: Could not create Lock objects - rebooting..." CR);
    //if (!(satLinkPmPollLock = xSemaphoreCreateMutex()))
    if (!(satLinkPmPollLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
        panic("satLink::satLink: Could not create Lock objects - rebooting..." CR);
    prevSysState = OP_WORKING;
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_ERRSEC | OP_GENERR);
    regSysStateCb(this, &onSysStateChangeHelper);
    xmlconfig[XML_SATLINK_SYSNAME] = NULL;
    xmlconfig[XML_SATLINK_USRNAME] = NULL;
    xmlconfig[XML_SATLINK_DESC] = NULL;
    xmlconfig[XML_SATLINK_LINK] = NULL;
    xmlconfig[XML_SATLINK_ADMSTATE] = NULL;

    //TODO CLI Initialization needs to move to a separate method to not eat all internal heap
    /* CLI decoration methods */
    // get/set satLinkNo
    /*
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
    */
}

satLink::~satLink(void) {
    panic("satLink::~satLink: satLink destructior not supported - rebooting..." CR);
}

rc_t satLink::init(void) {
    Log.INFO("satLink::init: Initializing Satelite link channel %d" CR, linkNo);
    Log.INFO("satLink::init: Creating satelites for link channel %d" CR, linkNo);
    for (uint8_t satAddress = 0; satAddress < MAX_SATELITES; satAddress++) {
        sats[satAddress] = new sat(satAddress, this);
        if (sats[satAddress] == NULL)
            panic("satLink::init: Could not create satelite object for link channel - rebooting...");
        addSysStateChild(sats[satAddress]);
        sats[satAddress]->init();
    }
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
    return RC_OK;
}

void satLink::onConfig(tinyxml2::XMLElement* p_satLinkXmlElement) {
    if (!(systemState::getOpState() & OP_UNCONFIGURED))
        panic("satLink:onConfig: satLink channel received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.INFO("satLink::onConfig: satLink channel %d received an uverified configuration, parsing and validating it..." CR, linkNo);

    //PARSING CONFIGURATION
    const char* satLinkSearchTags[5];
    satLinkSearchTags[XML_SATLINK_SYSNAME] = "SystemName";
    satLinkSearchTags[XML_SATLINK_USRNAME] = "UserName";
    satLinkSearchTags[XML_SATLINK_DESC] = "Description";
    satLinkSearchTags[XML_SATLINK_LINK] = "Link";
    satLinkSearchTags[XML_SATLINK_ADMSTATE] = "AdminState";
    getTagTxt(p_satLinkXmlElement->FirstChildElement(), satLinkSearchTags, xmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_SATLINK_SYSNAME])
        panic("satLink::onConfig: SystemName missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_USRNAME])
        panic("satLink::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_DESC])
        panic("satLink::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SATLINK_LINK])
        panic("satLink::onConfig: Link missing - rebooting...");
    if (atoi(xmlconfig[XML_SATLINK_LINK]) != linkNo)
        panic("satLink::onConfig: Link no inconsistant - rebooting...");
    if (xmlconfig[XML_SATLINK_ADMSTATE] == NULL) {
        Log.WARN("satLink::onConfig: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR);
        xmlconfig[XML_SATLINK_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_SATLINK_ADMSTATE], "ENABLE")) {
        unSetOpState(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_SATLINK_ADMSTATE], "DISABLE")) {
        setOpState(OP_DISABLED);
    }
    else
        panic("satLink::onConfig: Admin state: %s is none of \"ENABLE\" or \"DISABLE\" - rebooting..." CR, xmlconfig[XML_SATLINK_ADMSTATE]);

    //SHOW FINAL CONFIGURATION
    Log.INFO("satLink::onConfig: System name: %s" CR, xmlconfig[XML_SATLINK_SYSNAME]);
    Log.INFO("satLink::onConfig: User name: %s" CR, xmlconfig[XML_SATLINK_USRNAME]);
    Log.INFO("satLink::onConfig: Description: %s" CR, xmlconfig[XML_SATLINK_DESC]);
    Log.INFO("satLink::onConfig: Link: %s" CR, xmlconfig[XML_SATLINK_LINK]);
    Log.INFO("satLink::onConfig: satLink admin state: %s" CR, xmlconfig[XML_SATLINK_ADMSTATE]);

    //CONFIFIGURING SATELITES
    Log.INFO("satLink::onConfig: Configuring Satelites" CR);
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
            if (satItter >= MAX_SATELITES)
                panic("satLink::onConfig: More than maximum sats provided - not supported, rebooting...");
            getTagTxt(satXmlElement->FirstChildElement(), satSearchTags, satXmlConfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability
            if (!satXmlConfig[XML_SAT_ADDR])
                panic("satLink::onConfig:: Satelite Linkaddr missing - rebooting..." CR);
            if ((atoi(satXmlConfig[XML_SAT_ADDR])) < 0 || atoi(satXmlConfig[XML_SAT_ADDR]) >= MAX_SATELITES)
                panic("sat::onConfig:: Satelite link address out of bounds - rebooting...");
            Log.INFO("satLink::onConfig: Configuring satLink-%i:sat-%i" CR, link, atoi(satXmlConfig[XML_SAT_ADDR]));
            sats[atoi(satXmlConfig[XML_SAT_ADDR])]->onConfig(satXmlElement);
            satXmlElement = ((tinyxml2::XMLElement*)satXmlElement)->NextSiblingElement("Satelite");
        }
    }
    else
        Log.WARN("satLink::onConfig: No Satelites provided, no Satelite will be configured" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.INFO("satLink::onConfig: Configuration successfully finished" CR);
}

rc_t satLink::start(void) {
    Log.INFO("satLink::start: Starting Satelite link: %d" CR, linkNo);
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.INFO("satLink::start: Satelite Link %d not configured - will not start it" CR, linkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpState(OP_UNUSED);
    Log.INFO("satLink::start: Subscribing to adm- and op state topics");
    const char* admSubscribeTopic[5] = { MQTT_SATLINK_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), &onAdmStateChangeHelper, this))
        panic("satLink::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_SATLINK_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), &onOpStateChangeHelper, this))
        panic("satLink::start: Failed to suscribe to opState topic - rebooting...");
    for (uint16_t satItter = 0; satItter < MAX_SATELITES; satItter++) {
        if (sats[satItter]->start())
            panic("satLink::start: Could not start satelite satlink-%i:sat-%i - rebooting..." CR, link, satItter);
    }
    unSetOpState(OP_INIT);
    Log.INFO("lgLink::start: lightgroups link %d and all its lightgroupDecoders have started" CR, linkNo);
    return RC_OK;
}

void satLink::onDiscoveredSateliteHelper(satelite* p_sateliteLibHandle, uint8_t p_satLink, uint8_t p_satAddr, bool p_exists, void* p_satLinkHandle) {
    if (p_satLink != ((satLink*)p_satLinkHandle)->linkNo){
        panic("satLink::onDiscoveredSateliteHelper: Inconsistant link number - Rebooting...");
    }
    if (p_satAddr >= MAX_SATELITES) {
        panic("satLink::onDiscoveredSateliteHelper: More than maximum allowed(% i) satelites discovered on the link - Rebooting..., p_satAddr");
        return;
    }
    ((satLink*)p_satLinkHandle)->sats[p_satAddr]->onDiscovered(p_sateliteLibHandle, p_satAddr, p_exists);
}

void satLink::up(void) {
    if (!satLinkScanDisabled) {
        Log.INFO("satLink::up: satLink-%d link already enabled" CR, linkNo);
        return;
    }
    satLinkScanDisabled = false;
    Log.INFO("satLink::up: satLink-%d link and PM scanning starting" CR, linkNo);
    satErr_t rc = satLinkLibHandle->enableSatLink();
    if (rc != 0){
        Serial.printf("satLink::up: Satelite link scannig could not be started, return code: 0x%llx - rebooting..." CR, rc); //HOW TO DEAL WITH A NON FUNCTIONAL SATLINK, SHALL WE REALLY REBOOT?
        //panic("satLink::up: Satelite link scannig could not be started, return code: 0x%llx - rebooting..." CR, rc);
    }
    char satlinkPmTaskName[30];
    sprintf(satlinkPmTaskName, CPU_SATLINK_PM_TASKNAME, linkNo);
    pmPoll = true;
    BaseType_t taskRc = xTaskCreatePinnedToCore(
        pmPollHelper,                                                                   // Task function
        satlinkPmTaskName,                                                              // Task function name reference
        CPU_SATLINK_PM_STACKSIZE_1K * 1024,                                             // Stack size
        this,                                                                           // Parameter passing
        CPU_SATLINK_PM_PRIO,                                                            // Priority 0-24, higher is more
        NULL,                                                                           // Task handle
        ((linkNo % MAX_SATLINKS) ? CPU_SATLINK_PM_CORE[0] : CPU_SATLINK_PM_CORE[1]));    // Core [CORE_0 | CORE_1]
    if (taskRc != pdPASS)
        panic("satLink::up: could not start pm poll task  - rebooting..." CR);
}

void satLink::down(void) {
    if (satLinkScanDisabled) {
        Log.INFO("satLink::down: satLink-%d link already disabled" CR, linkNo);
        return;
    }
    Log.INFO("satLink::down: satLink-%d link and PM scanning stopping" CR, linkNo);
    satLinkScanDisabled = true;
    pmPoll = false;
    satErr_t rc = satLinkLibHandle->disableSatLink();
    if (rc)
        panic("satLink::down: could not disable link scanning, return code: %llx  - rebooting..." CR, rc);
}

void satLink::pmPollHelper(void* p_metaData) {
    ((satLink*)p_metaData)->onPmPoll();
}

void satLink::onPmPoll(void) {
    if (xSemaphoreTake(satLinkPmPollLock, 0) == pdFALSE) {
        Log.VERBOSE("satLink::onPmPoll: Did not have time to exit the pmPoll loop from when pmPolling was ordered to be stop, no need to re-enter" CR);
        return;
    }
    Log.INFO("satLink::onPmPoll: Starting PM polling for satLink %d" CR, link);
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
        const char* publishPmTopic[5] = { MQTT_SATLINK_STATS_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName() };
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
        if (mqtt::sendMsg(concatStr(publishPmTopic, 5), publishPmPayload, false)) {
            Log.ERROR("satLink::onPmPoll: Failed to send PM report" CR);
        }
        for (uint8_t i = 0; i < MAX_SATELITES; i++) {
            sats[i]->onPmPoll();
        }
        if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
        }
    }
    xSemaphoreGive(satLinkPmPollLock);                                                 //Here is a microscopic risk for a race-condition, probably a few ten cycles from when the while condition is false before we reach here
    Log.INFO("satLink::onPmPoll: Stopping PM polling for satLink %d" CR, link);
    vTaskDelete(NULL);
}

void satLink::onSatLinkLibStateChangeHelper(sateliteLink* p_sateliteLinkLibHandler, uint8_t p_linkAddr, satOpState_t p_satOpState, void* p_satLinkHandler) {
    ((satLink*)p_satLinkHandler)->onSatLinkLibStateChange(p_satOpState);
}

void satLink::onSatLinkLibStateChange(const satOpState_t p_satOpState) {
    if(p_satOpState & (SAT_OP_INIT | SAT_OP_FAIL | SAT_OP_CONTROLBOCK))
        systemState::setOpState(OP_GENERR);
    else
        systemState::unSetOpState(OP_GENERR);
    if (p_satOpState & SAT_OP_ERR_SEC)
        systemState::setOpState(OP_ERRSEC);
    else
        systemState::unSetOpState(OP_ERRSEC);
}

void satLink::onSysStateChangeHelper(const void* p_satLinkHandle, sysState_t p_sysState) {
    ((satLink*)p_satLinkHandle)->onSysStateChange(p_sysState);
}

void satLink::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    Log.INFO("satLink::onSysStateChange: satLink-%d has a new OP-state: %s, Queueing it for processing" CR, linkNo, systemState::getOpStateStr(opStateStr, p_sysState));
    xSemaphoreTake(satLinkSysStateLock, portMAX_DELAY);
    sysState_t* sysStateToQ = new sysState_t;
    *sysStateToQ = p_sysState;
    sysStateQ->push_back(sysStateToQ);
    xSemaphoreGive(satLinkSysStateLock);
    if (!processingSysState) {
        processingSysState = true;
        processSysState();
    }
}

void satLink::processSysState(void) {
    sysState_t newSysState;
    bool entering = true;
    xSemaphoreTake(satLinkSysStateLock, portMAX_DELAY);
    while (sysStateQ->size()){
        if (!entering) {
            xSemaphoreTake(satLinkSysStateLock, portMAX_DELAY);
            entering = false;
        }
        newSysState = *(sysStateQ->front());
        delete sysStateQ->front();
        sysStateQ->pop_front();
        xSemaphoreGive(satLinkSysStateLock);
        bool satLinkDeclareDown = false;
        bool satLinkDisableScan = false;
        char opStateStr[100];
        char opStateStr1[100];
        sysState_t sysStateChange = newSysState ^ prevSysState;
        if (!sysStateChange) {
            Log.VERBOSE("satLink::processSysState: No system state change - previous system state: %s, current system state %s" CR, systemState::getOpStateStr(opStateStr1, prevSysState), systemState::getOpStateStr(opStateStr, prevSysState));
            continue;
        }
        if (newSysState) {
            // failsafe(true); NOT NEEDED, on link-level, disabling LG-scan will cause the satelite HW to set failsafe by HW WDs
            Log.INFO("satLink::processSysState: satLink-%d has a new OP-state: %s, Setting failsafe" CR, linkNo, systemState::getOpStateStr(opStateStr, newSysState));
            Log.TERSE("satLink::processSysState: Following satLink-%d OP-states have changed: %s" CR, linkNo, systemState::getOpStateStr(opStateStr, sysStateChange));
        }
        else {
            Log.TERSE("satLink::processSysState: satLink-%d has a new OP-state: \"WORKING\", Unsetting failsafe" CR, linkNo);
            // failsafe(false); NOT NEEDED, on link-level enabling LG-scan will cause the satelite HW to unset failsafe
        }
        if ((newSysState & OP_INTFAIL)) {
            satLinkDisableScan = true;
            down();
            if (!(newSysState & OP_UNCONFIGURED)) {
                char publishTopic[300];
                sprintf(publishTopic, "%s%s%s%s%s", MQTT_SATLINK_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_SATLINK_SYSNAME]);
                satLinkDeclareDown = true;
                mqtt::sendMsg(publishTopic, MQTT_OP_UNAVAIL_PAYLOAD, false);
                satLinkDownDeclared = true;
            }
            prevSysState = newSysState;
            //failsafe
            panic("satLink::processSysState: satLink-%d has experienced an internal error - informing server and rebooting..." CR, linkNo);
            continue;
        }
        if (newSysState & OP_INIT) {
            Log.INFO("satLink::processSysState: satLink-%d is initializing - informing server if not already done" CR, linkNo);
            satLinkDisableScan = true;
            satLinkDeclareDown = true;
        }
        if (newSysState & OP_UNUSED) {
            Log.INFO("satLink::processSysState: satLink-%d is unused - informing server if not already done" CR, linkNo);
            satLinkDisableScan = true;
            satLinkDeclareDown = true;
        }
        if (newSysState & OP_ERRSEC) {
            Log.INFO("satLink::processSysState: satLink-%d has experienced excessive PM errors - informing server if not already done" CR, linkNo);
            satLinkDeclareDown = true;
        }
        if (newSysState & OP_GENERR) {
            Log.INFO("satLink::processSysState: satLink-%d has experienced an error - informing server if not already done" CR, linkNo);
            satLinkDeclareDown = true;
        }
        if (newSysState & OP_DISABLED) {
            Log.INFO("satLink::processSysState: satLink-%d is disabled by server - disabling linkscanning if not already done" CR, linkNo);
            satLinkDisableScan = true;
        }
        if (newSysState & OP_UNAVAILABLE) {
            Log.INFO("satLink::processSysState: satLink-%d is control-blocked by server - disabling linkscanning if not already done" CR, linkNo);
            satLinkDisableScan = true;
        }
        if (newSysState & OP_CBL) {
            Log.INFO("satLink::processSysState: satLink-%d is control-blocked by decoder - informing server and disabling scan if not already done" CR, linkNo);
            satLinkDeclareDown = true;
            satLinkDisableScan = true;
        }
        if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_ERRSEC | OP_GENERR | OP_DISABLED | OP_UNAVAILABLE | OP_CBL)) {
            Log.INFO("satLink::processSysState: satLink-%d has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, linkNo, systemState::getOpStateStr(opStateStr, newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_UNAVAILABLE | OP_CBL)));
            satLinkDeclareDown = true;
        }
        if (satLinkDisableScan && !satLinkScanDisabled) {
            Log.INFO("satLink::processSysState: satLink-%d disabling satLink scaning" CR, linkNo);
            down();
        }
        else if (satLinkDisableScan && satLinkScanDisabled)
            Log.INFO("satLink::processSysState: satLink-%d Link scan already disabled - doing nothing..." CR, linkNo);
        else if (!satLinkDisableScan && satLinkScanDisabled) {
            Log.INFO("satLink::processSysState: satLink-%d enabling satLink scaning" CR, linkNo);
            up();
        }
        else if (!satLinkDisableScan && !satLinkScanDisabled)
            Log.INFO("satLink::processSysState: satLink-%d Link scan already enabled - doing nothing..." CR, linkNo);
        if (satLinkDeclareDown && !satLinkDownDeclared && !(newSysState & OP_INIT) && !(newSysState & OP_UNUSED) && !(newSysState & OP_UNCONFIGURED)) {
            Log.INFO("satLink::processSysState: satLink-%d Declaring link down to server" CR, linkNo);
            char publishTopic[300];
            sprintf(publishTopic, "%s%s%s%s%s", MQTT_SATLINK_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_SATLINK_SYSNAME]);
            mqtt::sendMsg(publishTopic, MQTT_OP_UNAVAIL_PAYLOAD, false);
            satLinkDownDeclared = true;
        }
        else if (satLinkDeclareDown && satLinkDownDeclared)
            Log.INFO("satLink::processSysState: satLink-%d already declared down to server - doing nothing..." CR, linkNo);
        else if (!satLinkDeclareDown && satLinkDownDeclared && !(newSysState & OP_INIT) && !(newSysState & OP_UNUSED) && !(newSysState & OP_UNCONFIGURED)) {
            Log.INFO("satLink::processSysState: satLink-%d Declaring link up to server" CR, linkNo);
            char publishTopic[300];
            sprintf(publishTopic, "%s%s%s%s%s", MQTT_SATLINK_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_SATLINK_SYSNAME]);
            mqtt::sendMsg(publishTopic, MQTT_OP_AVAIL_PAYLOAD, false);
            satLinkDownDeclared = false;
        }
        else if (!satLinkDeclareDown && !satLinkDownDeclared) {
            Log.INFO("satLink::processSysState: satLink-%d already declared up to server - doing nothing..." CR, linkNo);
        }
        prevSysState = newSysState;
    }
    processingSysState = false;
}

void satLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onOpStateChange(p_topic, p_payload);
}

void satLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.INFO("satLink::onOpStateChange: got available message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.INFO("satLink::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.ERROR("satLink::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void satLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject) {
    ((satLink*)p_satLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void satLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.INFO("satLink::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.INFO("satLink::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.ERROR("satLink::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

rc_t satLink::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t satLink::setSystemName(const char* p_systemName, const bool p_force) {
    Log.ERROR("satLink::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* satLink::getSystemName(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::getSystemName: cannot get System name as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_SYSNAME];
}

rc_t satLink::setUsrName(const char* p_usrName, const bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("satLink::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::setUsrName: cannot set System name as satLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("satLink::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_SATLINK_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* satLink::getUsrName(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::getUsrName: cannot get User name as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_USRNAME];
}

rc_t satLink::setDesc(const char* p_description, const bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("satLink::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::setDesc: cannot set Description as satLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("satLink::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SATLINK_DESC];
        xmlconfig[XML_SATLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* satLink::getDesc(void) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("satLink::getDesc: cannot get Description as satLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SATLINK_DESC];
}

rc_t satLink::setLink(uint8_t p_link) {
    Log.ERROR("satLink::setLink: cannot set Link No - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t satLink::getLink(uint8_t* p_link) {
    *p_link = linkNo;
    return RC_OK;
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

/* CLI decoration methods */
void satLink::onCliGetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink, return code: %i", rc);
        return;
    }
    printCli("Satelite-link: %i", link);
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or txunderruns , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i TX-underruns: %i", link, static_cast<satLink*>(p_cliContext)->getTxUnderruns());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or rxoverruns , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i RX-overruns: %i", link, static_cast<satLink*>(p_cliContext)->getTxUnderruns());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or timingviolations , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i RX-Timing-violations: %i", link, static_cast<satLink*>(p_cliContext)->getScanTimingViolations());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or rxcrcerr , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i RX-CRC-Errors: %i", link, static_cast<satLink*>(p_cliContext)->getRxCrcErrs());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or remotecrcerr , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i Remote-CRC-Errors: %i", link, static_cast<satLink*>(p_cliContext)->getRemoteCrcErrs());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or rxsymerr , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i RX-Symbol-Errors: %i", link, static_cast<satLink*>(p_cliContext)->getRxSymbolErrs());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or rxsizeerr , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i RX-Size-Errors: %i", link, static_cast<satLink*>(p_cliContext)->getRxDataSizeErrs());
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
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<satLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get satLink or wderr , return code: %i", rc);
        return;
    }
    printCli("Satelite-link %i Watchdog-Errors: %i", link, static_cast<satLink*>(p_cliContext)->getWdErrs());
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
