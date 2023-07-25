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
#include "lgLink.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgLink(lightgroupLink)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

lgLink::lgLink(uint8_t p_linkNo, decoder* p_decoderHandle) : systemState(p_decoderHandle), globalCli(LGLINK_MO_NAME, LGLINK_MO_NAME, p_linkNo, p_decoderHandle) {
    Log.INFO("lgLink::lgLink: Creating Lightgroup link channel %d" CR, p_linkNo);
    char sysStateObjName[20];
    sprintf(sysStateObjName, "lgLink-%d", p_linkNo);
    setSysStateObjName(sysStateObjName);
    linkNo = p_linkNo;
    linkScan = false;
    lgLinkScanDisabled = true;
    debug = false;
    heap_caps_print_heap_info(MALLOC_CAP_SPIRAM);
    failsafeSet = false;
    //if (!(lgLinkLock = xSemaphoreCreateMutex()))                       //Why do we have this
    if (!(lgLinkLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
        panic("lgLink::lgLink: Could not create Lock objects - rebooting..." CR);
    dirtyPixelLock = xSemaphoreCreateMutex();
    if (dirtyPixelLock == NULL)
        panic("lgLink::satLink: Could not create \"dirtyPixelLock\" object - rebooting..." CR);
    Log.INFO("lgLink::lgLink: Creating stripled objects for lgLink Channel" CR, linkNo);
    strip = new (heap_caps_malloc(sizeof(Adafruit_NeoPixel(MAX_LGSTRIPLEN, (LGLINK_PINS[linkNo]), NEO_RGB + NEO_KHZ800)), MALLOC_CAP_SPIRAM)) Adafruit_NeoPixel(MAX_LGSTRIPLEN, (LGLINK_PINS[linkNo]), NEO_RGB + NEO_KHZ800);
    strip->begin();
    stripWritebuff = strip->getPixels();
    Log.INFO("lgLink::lgLink: stripled for lgLink Channel started" CR, linkNo);
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNUSED);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    xmlconfig[XML_LGLINK_SYSNAME] = NULL;
    xmlconfig[XML_LGLINK_USRNAME] = NULL;
    xmlconfig[XML_LGLINK_DESC] = NULL;
    xmlconfig[XML_LGLINK_LINK] = NULL;
    xmlconfig[XML_LGLINK_ADMSTATE] = NULL;
    avgSamples = UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS;
    latencyVect = new (heap_caps_malloc(sizeof(int64_t) * avgSamples, MALLOC_CAP_SPIRAM)) int64_t[avgSamples];
    runtimeVect = new (heap_caps_malloc(sizeof(uint32_t) * avgSamples, MALLOC_CAP_SPIRAM)) uint32_t[avgSamples];
    if (latencyVect == NULL || runtimeVect == NULL)
        panic("lgLink::satLink: Could not create PM vectors - rebooting..." CR);
}

lgLink::~lgLink(void) {
    panic("lgLink::~lgLink: lgLink destructior not supported - rebooting..." CR);
}

rc_t lgLink::init(void) {
    Log.INFO("lgLink::init: Initializing Lightgroup link channel %d" CR, linkNo);

    /* CLI decoration methods */
    Log.INFO("lgLink::init: Registering CLI methods for Lightgroup link channel %d" CR, linkNo);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set lgLinkNo
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKNO_SUB_MO_NAME, onCliGetLinkHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKNO_SUB_MO_NAME, LGLINKNO_GET_LGLINK_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, LGLINK_MO_NAME, LGLINKNO_SUB_MO_NAME, onCliSetLinkHelper);
    regCmdHelp(SET_CLI_CMD, LGLINK_MO_NAME, LGLINKNO_SUB_MO_NAME, LGLINKNO_SET_LGLINK_HELP_TXT);

    // get/clear lgLink overrun stats
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKOVERRUNS_SUB_MO_NAME, onCliGetLinkOverrunsHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKOVERRUNS_SUB_MO_NAME, LGLINKNO_GET_LGLINKOVERRUNS_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKOVERRUNS_SUB_MO_NAME, onCliClearLinkOverrunsHelper);
    regCmdHelp(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKOVERRUNS_SUB_MO_NAME, LGLINKNO_CLEAR_LGLINKOVERRUNS_HELP_TXT);

    // get/clear lgLink latency stats
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMEANLATENCY_SUB_MO_NAME, onCliGetMeanLatencyHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMEANLATENCY_SUB_MO_NAME, LGLINKNO_GET_LGLINKMEANLATENCY_HELP_TXT);
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXLATENCY_SUB_MO_NAME, onCliGetMaxLatencyHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXLATENCY_SUB_MO_NAME, LGLINKNO_GET_LGLINKMAXLATENCY_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXLATENCY_SUB_MO_NAME, onCliClearMaxLatencyHelper);
    regCmdHelp(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXLATENCY_SUB_MO_NAME, LGLINKNO_CLEAR_LGLINKMAXLATENCY_HELP_TXT);

    // get/clear lgLink runtime stats
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMEANRUNTIME_SUB_MO_NAME, onCliGetMeanRuntimeHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMEANRUNTIME_SUB_MO_NAME, LGLINKNO_GET_LGLINKMEANRUNTIME_HELP_TXT);
    regCmdMoArg(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXRUNTIME_SUB_MO_NAME, onCliGetMaxRuntimeHelper);
    regCmdHelp(GET_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXRUNTIME_SUB_MO_NAME, LGLINKNO_GET_LGLINKMAXRUNTIME_HELP_TXT);
    regCmdMoArg(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXRUNTIME_SUB_MO_NAME, onCliClearMaxRuntimeHelper);
    regCmdHelp(CLEAR_CLI_CMD, LGLINK_MO_NAME, LGLINKMAXRUNTIME_SUB_MO_NAME, LGLINKNO_CLEAR_LGLINKMAXRUNTIME_HELP_TXT);
    Log.INFO("lgLink::init: CLI methods for Lightgroup link channel %d registered" CR, linkNo);

    Log.INFO("lgLink::init: Creating lighGrups for link channel %d" CR, linkNo);
    for (uint8_t lgAddress = 0; lgAddress < MAX_LGS; lgAddress++) {
        lgs[lgAddress] = new (heap_caps_malloc(sizeof(lgBase(lgAddress, this)), MALLOC_CAP_SPIRAM)) lgBase(lgAddress, this);
        if (lgs[lgAddress] == NULL)
            panic("lgLink::init: Could not create light-group object - rebooting...");
        addSysStateChild(lgs[lgAddress]);
        lgs[lgAddress]->init();
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    Log.VERBOSE("lgLink::init: lighGrups for link channel %d created" CR, linkNo);

    Log.INFO("lgLink::init: Creating flash objects for lgLink Channel" CR, linkNo);
    FLASHNORMAL = new (heap_caps_malloc(sizeof(flash(FLASH_1_0_HZ, 50)), MALLOC_CAP_SPIRAM)) flash(FLASH_1_0_HZ, 50);
    FLASHSLOW = new (heap_caps_malloc(sizeof(flash(FLASH_0_5_HZ, 50)), MALLOC_CAP_SPIRAM)) flash(FLASH_0_5_HZ, 50);
    FLASHFAST = new (heap_caps_malloc(sizeof(flash(FLASH_1_5_HZ, 50)), MALLOC_CAP_SPIRAM)) flash(FLASH_1_5_HZ, 50);
    if (FLASHNORMAL == NULL || FLASHSLOW == NULL || FLASHFAST == NULL) {
        panic("lgLink::init: Could not create flash objects - rebooting..." CR);
    }
    Log.VERBOSE("lgLink::init: flash objects for lgLink Channel created" CR, linkNo);
    //Log.INFO("lgLink::init: Creating stripled objects for lgLink Channel" CR, linkNo);
    //strip = new Adafruit_NeoPixel(MAX_LGSTRIPLEN, (LGLINK_PINS[linkNo]), NEO_RGB + NEO_KHZ800);
    Log.VERBOSE("lgLink::init: stripled objects for lgLink Channel created" CR, linkNo);
    Log.INFO("lgLink::init: Starting stripled objects for lgLink Channel" CR, linkNo);
    return RC_OK;
}

void lgLink::onConfig(const tinyxml2::XMLElement* p_lightgroupLinkXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        panic("lgLink:onConfig: lgLink received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting..." CR);
    Log.INFO("lgLink::onConfig: lgLink channel %d received an unverified configuration, parsing and validating it..." CR, linkNo);

    //PARSING CONFIGURATION
    const char* lgLinkSearchTags[5];
    lgLinkSearchTags[XML_LGLINK_SYSNAME] = "SystemName";
    lgLinkSearchTags[XML_LGLINK_USRNAME] = "UserName";
    lgLinkSearchTags[XML_LGLINK_DESC] = "Description";
    lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
    lgLinkSearchTags[XML_LGLINK_ADMSTATE] = "AdminState";

    getTagTxt(p_lightgroupLinkXmlElement->FirstChildElement(), lgLinkSearchTags, xmlconfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_LGLINK_SYSNAME])
        panic("lgLink::onConfig: System name was not provided - rebooting..." CR);
    if (!xmlconfig[XML_LGLINK_USRNAME]){
        Log.WARN("lgLink::onConfig: User name was not provided - using \"%s-UserName\"" CR);
        xmlconfig[XML_LGLINK_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_LGLINK_SYSNAME]) + 10), MALLOC_CAP_SPIRAM)) char[strlen(xmlconfig[XML_LGLINK_SYSNAME]) + 10];
        const char* usrName[2] = { xmlconfig[XML_LGLINK_SYSNAME], "-" };
        strcpy(xmlconfig[XML_LGLINK_USRNAME], "-");
    }
    if (!xmlconfig[XML_LGLINK_DESC]){
        Log.WARN("lgLink::onConfig: Description was not provided - using \"-\"" CR);
        xmlconfig[XML_LGLINK_DESC] = new (heap_caps_malloc(sizeof(char) * 2, MALLOC_CAP_SPIRAM)) char[2];
        strcpy(xmlconfig[XML_LGLINK_DESC], "-");
    }
    if (!xmlconfig[XML_LGLINK_LINK])
        panic("lgLink::onConfig: Link was not provided - rebooting..." CR);
    if (atoi(xmlconfig[XML_LGLINK_LINK]) != linkNo)
        panic("lgLink::onConfig: Link no inconsistant with what was provided in the object constructor - rebooting..." CR);

    if (xmlconfig[XML_LGLINK_ADMSTATE] == NULL) {
        Log.WARN("lgLink::onConfig: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR);
        xmlconfig[XML_LGLINK_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_LGLINK_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_LGLINK_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else
        panic("lgLink::onConfig: Configuration decoder::onConfig: Admin state: %s is none of \"ENABLE\" or \"DISABLE\" - rebooting..." CR, xmlconfig[XML_DECODER_ADMSTATE]);

    //SHOW FINAL CONFIGURATION
    Log.INFO("lgLink::onConfig: Successfully set the Lightgroup-link configuration as follows:" CR);
    Log.INFO("lgLink::onConfig: System name: %s" CR, xmlconfig[XML_LGLINK_SYSNAME]);
    Log.INFO("lgLink::onConfig: User name: %s" CR, xmlconfig[XML_LGLINK_USRNAME]);
    Log.INFO("lgLink::onConfig: Description: %s" CR, xmlconfig[XML_LGLINK_DESC]);
    Log.INFO("lgLink::onConfig: LG-Link #: %s" CR, xmlconfig[XML_LGLINK_LINK]);
    Log.INFO("lgLink::onConfig: LG-Link admin state: %s" CR, xmlconfig[XML_LGLINK_ADMSTATE]);

    //CONFIFIGURING LIGHTGROUP ASPECTS
    Log.INFO("lgLink::onConfig: Creating and configuring signal mast aspect description object" CR);
    signalMastAspectsObject = new (heap_caps_malloc(sizeof(signalMastAspects(this)), MALLOC_CAP_SPIRAM)) signalMastAspects(this);
    if (signalMastAspectsObject == NULL)
        panic("lgLink:onConfig: Could not start signalMastAspect object - rebooting..." CR);
    if (!(p_lightgroupLinkXmlElement)->FirstChildElement("SignalMastDesc"))
        panic("lgLink::onConfig: Signal mast aspect description missing - rebooting..." CR);
    else if (signalMastAspectsObject->onConfig(p_lightgroupLinkXmlElement->FirstChildElement("SignalMastDesc")))
        panic("lgLink::onConfig: Could not configure Signal mast aspect description - rebooting..." CR);
    if (((tinyxml2::XMLElement*) p_lightgroupLinkXmlElement)->FirstChildElement("SignalMastDesc")->NextSiblingElement("SignalMastDesc"))
        panic("lgLink::onConfig: Multiple signal mast aspect descriptions provided - rebooting..." CR);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    //CONFIFIGURING LIGHTGROUPS
    Log.INFO("lgLink::onConfig: Configuring Light groups" CR);
    tinyxml2::XMLElement* lgXmlElement;
    if (lgXmlElement = ((tinyxml2::XMLElement*)p_lightgroupLinkXmlElement)->FirstChildElement("LightGroup")) {
        const char* lgSearchTags[8];
        lgSearchTags[XML_LG_SYSNAME] = NULL;
        lgSearchTags[XML_LG_USRNAME] = NULL;
        lgSearchTags[XML_LG_DESC] = NULL;
        lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
        lgSearchTags[XML_LG_TYPE] = NULL;
        lgSearchTags[XML_LG_PROPERTY1] = NULL;
        lgSearchTags[XML_LG_PROPERTY2] = NULL;
        lgSearchTags[XML_LG_PROPERTY3] = NULL;
        for (uint16_t lgItter = 0; true; lgItter++) {
            char* lgXmlconfig[8] = { NULL };
            if (!lgXmlElement){
                break;
            }
            if (lgItter >= MAX_LGS){
                panic("lgLink::onConfig: More than maximum lightgroups provided - not supported, rebooting..." CR);
                return;
            }
            getTagTxt(lgXmlElement->FirstChildElement(), lgSearchTags, lgXmlconfig, sizeof(lgSearchTags) / 4); // Need to fix the addressing for portability
            if (!lgXmlconfig[XML_LG_LINKADDR])
                panic("lgLink::onConfig:: lightgroups Linkaddress not provided/missing - rebooting..." CR);
            if ((atoi(lgXmlconfig[XML_LG_LINKADDR])) < 0 || atoi(lgXmlconfig[XML_LG_LINKADDR]) >= MAX_LGS)
                panic("lgLink::onConfig:: Light group link address (%i) out of bounds - rebooting..." CR, atoi(lgXmlconfig[XML_LG_LINKADDR]));
            lgs[atoi(lgXmlconfig[XML_LG_LINKADDR])]->onConfig(lgXmlElement);
            lgXmlElement = ((tinyxml2::XMLElement*)lgXmlElement)->NextSiblingElement("LightGroup");
        }
        uint16_t stripOffset = 0;
        for (uint16_t lgItter = 0; lgItter < MAX_LGS; lgItter++) {
            if (lgs[lgItter]->getOpStateBitmap() & OP_UNUSED)
                break;
            lgs[lgItter]->setStripOffset(stripOffset);
            uint8_t noOfLeds;
            lgs[lgItter]->getNoOffLeds(&noOfLeds, true);
            stripOffset += noOfLeds;
            if (stripOffset > MAX_LGSTRIPLEN * 3)
                panic("lgLink::onConfig: Number of used strip pixels exceeds MAX_LGSTRIPLEN*3 - rebooting...");
        }
    }
    else
        Log.WARN("lgLink::onConfig: No lightgroups provided, no lightgroup will be configured" CR);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    Log.INFO("lgLink::onConfig: Configuration successfully finished" CR);
}

rc_t lgLink::start(void) {
    Log.INFO("lgLink::start: Starting lightgroup link: %d" CR, linkNo);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.INFO("lgLink::start: LG Link %d not configured - will not start it" CR, linkNo);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    Log.INFO("lgLink::start: Subscribing to adm- and op state topics" CR);
    char admopSubscribeTopic[300];
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LGLINK_ADMSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LGLINK_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, &onAdmStateChangeHelper, this))
        panic("lgLink::start: Failed to suscribe to admState topic - rebooting..." CR);
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LGLINK_OPSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LGLINK_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, &onOpStateChangeHelper, this))
        panic("lgLink::start: Failed to suscribe to opState topic - rebooting..." CR);
    for (uint16_t lgItter = 0; lgItter < MAX_LGS; lgItter++)
        lgs[lgItter]->start();
    unSetOpStateByBitmap(OP_INIT);
    Log.INFO("lgLink::start: lightgroups link %d and all its lightgroupDecoders have started" CR, linkNo);
    return RC_OK;
}

void lgLink::up(void) {
    BaseType_t rc;
    Log.TERSE("lgLink::up: Starting link scanning for lgLink %d" CR, linkNo);
    char taskName[30];
    sprintf(taskName, CPU_UPDATE_STRIP_TASKNAME, linkNo);
    linkScan = true;
    rc = xTaskCreatePinnedToCore(
        updateStripHelper,                                  // Task function
        CPU_UPDATE_STRIP_TASKNAME,                          // Task function name reference
        CPU_UPDATE_STRIP_STACKSIZE_1K * 1024,               // Stack size
        this,                                               // Parameter passing
        CPU_UPDATE_STRIP_PRIO,                              // Priority 0-24, higher is more
        NULL,                                               // Task handle
        CPU_UPDATE_STRIP_CORE[linkNo % 2]) ;                 // Core [CORE_0 | CORE_1]
    if (rc != pdPASS)
        panic("lgLink::up: Could not start lglink scanning, return code %i- rebooting..." CR, rc);
}

void lgLink::down(void) {
    Log.TERSE("lgLink::up: Stoping link scanning for lgLink %d" CR, linkNo);
    linkScan = false;
}

void lgLink::onSysStateChangeHelper(const void* p_miscData, sysState_t p_sysState) {
    ((lgLink*)p_miscData)->onSysStateChange(p_sysState);
}

void lgLink::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    Log.INFO("lgLink::onSysStateChange: lgLink-%d has a new OP-state: %s" CR, linkNo, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    bool lgLinkDisableScan = false;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange) {
        return;
    }
    Log.INFO("lgLink::onSysStateChange: lgLink-%d has a new OP-state: %s" CR, linkNo, systemState::getOpStateStr(opStateStr));
    failsafe(newSysState != OP_WORKING);
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_LGLINK_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_LGLINK_SYSNAME]);
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if ((newSysState & OP_INTFAIL)) {
        lgLinkDisableScan = true;
        down();
        lgLinkScanDisabled = true;
        prevSysState = newSysState;
        //failsafe
        panic("lgLink::onSysStateChange: lgLink-%d has experienced an internal error - informing server and rebooting..." CR, linkNo);
        return;
    }
    if (newSysState & OP_INIT) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d is initializing - informing server if not already done" CR, linkNo);
        lgLinkDisableScan = true;
    }
    if (newSysState & OP_UNUSED) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d is unused - informing server if not already done" CR, linkNo);
        lgLinkDisableScan = true;
    }
    if (newSysState & OP_DISABLED) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d is disabled by server - disabling linkscanning if not already done" CR, linkNo);
        lgLinkDisableScan = true;
    }
    if (newSysState & OP_CBL) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d is control-blocked by decoder - informing server and disabling scan if not already done" CR, linkNo);
        lgLinkDisableScan = true;
    }
    if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL)) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, linkNo, systemState::getOpStateStrByBitmap(newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL), opStateStr));
    }
    if (lgLinkDisableScan && !lgLinkScanDisabled) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d disabling lgLink scaning" CR, linkNo);
        down();
        lgLinkScanDisabled = true;
    }
    else if (lgLinkDisableScan && lgLinkScanDisabled) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d Link scan already disabled - doing nothing..." CR, linkNo);
    }
    else if (!lgLinkDisableScan && lgLinkScanDisabled) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d enabling lgLink scaning" CR, linkNo);
        up();
        lgLinkScanDisabled = false;
    }
    else if (!lgLinkDisableScan && !lgLinkScanDisabled) {
        Log.INFO("lgLink::onSysStateChange: lgLink-%d Link scan already enabled - doing nothing..." CR, linkNo);
    }
    prevSysState = newSysState;
}

void lgLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onOpStateChange(p_topic, p_payload);
}

void lgLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    Log.INFO("lgLink::onOpStateChange: got a new opState from server: %s" CR, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void lgLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void lgLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        Log.INFO("lgLink::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        Log.INFO("lgLink::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.ERROR("lgLink::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

rc_t lgLink::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t lgLink::setSystemName(const char* p_systemName, bool p_force) {
    Log.ERROR("lgLink::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgLink::getSystemName(char* p_systemName, bool p_force){
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgLink::getSystemName: cannot get System name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_systemName = xmlconfig[XML_LGLINK_SYSNAME];
    return RC_OK;
}

rc_t lgLink::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgLink::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.ERROR("lgLink::setUsrName: cannot set System name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("lgLink::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_LGLINK_USRNAME];
        xmlconfig[XML_LGLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

rc_t  lgLink::getUsrName(char* p_userName, bool p_force){
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgLink::getUsrName: cannot get User name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_userName = xmlconfig[XML_LGLINK_USRNAME];
    return RC_OK;
}

rc_t lgLink::setDesc(const char* p_description, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgLink::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        Log.ERROR("lgLink::setDesc: cannot set Description as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("lgLink::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LGLINK_DESC];
        xmlconfig[XML_LGLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t lgLink::getDesc(char* p_desc, bool p_force){
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgLink::getDesc: cannot get Description as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_desc = xmlconfig[XML_LGLINK_DESC];
    return RC_OK;
}

rc_t lgLink::setLink(uint8_t p_link) {
    Log.ERROR("lgLink::setLink: cannot set Link No - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgLink::getLink(uint8_t* p_link) {
    *p_link = linkNo;
    return RC_OK;
}

const char* lgLink::getLogLevel(void) {
    if (!transformLogLevelInt2XmlStr(Log.getLevel())) {
        Log.ERROR("decoder::lgLink: Could not retrieve a valid Log-level" CR);
        return NULL;
    }
    else {
        return transformLogLevelInt2XmlStr(Log.getLevel());
    }
}

void lgLink::setDebug(bool p_debug) {
    debug = p_debug;
}

bool lgLink::getDebug(void) {
    return debug;
}

rc_t lgLink::updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, uint8_t* p_wantedValueBuff, uint8_t p_transitionTime) {
    uint8_t transitionTime = floor((float)(SM_BRIGHNESS_NORMAL / (p_transitionTime / STRIP_UPDATE_MS)));
    if (systemState::getOpStateBitmap()) {
        Log.error("lgLink::updateLg: Could not update LG as LgLink is not in OP_WORKING State");
        return RC_OPSTATE_ERR;
    }
    xSemaphoreTake(dirtyPixelLock, portMAX_DELAY);
    bool alreadyDirty = false;
    for (uint16_t i = 0; i < p_buffLen; i++) {
        for (uint16_t j = 0; j < dirtyPixelList.size(); j++) {
            if (dirtyPixelList.at(j)->index == p_seqOffset + i) {
                alreadyDirty == true;
                dirtyPixelList.at(j)->wantedValue = p_wantedValueBuff[i];
                break;
            }
        }
        if (!alreadyDirty) {
            if (p_wantedValueBuff[i] != stripWritebuff[p_seqOffset + i]) {
                dirtyPixel_t* dirtyPixel = new (heap_caps_malloc(sizeof(dirtyPixel_t), MALLOC_CAP_SPIRAM)) dirtyPixel_t;
                if (!dirtyPixel)
                    panic("lgLink::updateLg: Could not allocate an dirtyPixel object - rebooting..." CR);
                dirtyPixel->index = p_seqOffset + i;
                dirtyPixel->currentValue = stripWritebuff[p_seqOffset + i];
                dirtyPixel->wantedValue = p_wantedValueBuff[i];
                dirtyPixel->incrementValue = transitionTime;
                dirtyPixelList.push_back(dirtyPixel);
            }
        }
    }
    xSemaphoreGive(dirtyPixelLock);
    return RC_OK;
}

void lgLink::updateStripHelper(void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->updateStrip();
}

void lgLink::updateStrip(void) {
    int currentValue;
    int wantedValue;
    int incrementValue;
    int64_t nextLoopTime = esp_timer_get_time();
    int64_t thisLoopTime;
    uint32_t startTime;
    uint16_t avgIndex = 0;
    int64_t latency = 0;
    uint32_t runtime = 0;
    maxLatency = 0;
    maxRuntime = 0;
    overRuns = 0;
    uint32_t maxAvgIndex = floor((float)(UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS));
    uint32_t loopTime = STRIP_UPDATE_MS * 1000;
    Log.VERBOSE("lgLink::updateStrip: Starting sriphandler channel %d" CR, linkNo);
    uint32_t wdtFeeed_cnt = 3000 / (STRIP_UPDATE_MS * 2);
    while (linkScan) {
        startTime = esp_timer_get_time();
        thisLoopTime = nextLoopTime;
        nextLoopTime += STRIP_UPDATE_MS * 1000;
        if (!wdtFeeed_cnt--) {
            lgLinkWdt->feed();
            wdtFeeed_cnt = 3000 / (STRIP_UPDATE_MS * 2);
            //Serial.printf("Kick\n");
        }
        if (avgIndex >= maxAvgIndex) {
            avgIndex = 0;
        }
        latency = startTime - thisLoopTime;
        latencyVect[avgIndex] = latency;
        if (latency > maxLatency) {
            maxLatency = latency;
        }
        if (!strip->canShow()) {
            overRuns++;
            Log.VERBOSE("lgLink::updateStrip: Couldnt update strip channel %d, continuing..." CR, linkNo);
        }
        else {
            xSemaphoreTake(dirtyPixelLock, portMAX_DELAY);
            for (uint16_t i = 0; i < dirtyPixelList.size(); i++) {
                currentValue = (int)dirtyPixelList.at(i)->currentValue;
                wantedValue = (int)dirtyPixelList.at(i)->wantedValue;
                incrementValue = (int)dirtyPixelList.at(i)->incrementValue;
                if (wantedValue > currentValue) {
                    currentValue += dirtyPixelList.at(i)->incrementValue;
                    if (currentValue > wantedValue) {
                        currentValue = wantedValue;
                    }
                }
                else if (wantedValue < currentValue){
                    currentValue -= dirtyPixelList.at(i)->incrementValue;
                    if (currentValue < wantedValue) {
                        currentValue = wantedValue;
                    }
                }
                stripWritebuff[dirtyPixelList.at(i)->index] = currentValue;
                dirtyPixelList.at(i)->currentValue = (uint8_t)currentValue;
                if (wantedValue == currentValue) {
                    delete dirtyPixelList.at(i);
                    dirtyPixelList.clear(i);
                }
            }
            xSemaphoreGive(dirtyPixelLock);
            strip->show();
        }
        runtime = esp_timer_get_time() - startTime;
        runtimeVect[avgIndex] = runtime;
        if (runtime > maxRuntime) {
            maxRuntime = runtime;
        }
        TickType_t delay;
        if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0) {
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
        }
        else {
            Log.VERBOSE("lgLink::updateStrip: Strip channel %d overrun" CR, linkNo);
            overRuns++;
            nextLoopTime = esp_timer_get_time();
        }
        avgIndex++;
    }
    vTaskDelete(NULL);
}

uint32_t lgLink::getOverRuns(void) {
    return overRuns;
}

void lgLink::clearOverRuns(void) {
    overRuns = 0;
}

int64_t lgLink::getMeanLatency(void) {
    int64_t accLatency = 0;
    int64_t meanLatency;
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
    for (uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++)
        accLatency += latencyVect[avgIndex];
    xSemaphoreGive(lgLinkLock);
    meanLatency = accLatency / avgSamples;
    return meanLatency;
}

int64_t lgLink::getMaxLatency(void) {
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
    int64_t tmpMaxLatency = maxLatency;
    xSemaphoreGive(lgLinkLock);
    return tmpMaxLatency;
}

void lgLink::clearMaxLatency(void) {
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
    maxLatency = -1000000000; //NEEDS FIX
    xSemaphoreGive(lgLinkLock);
}

uint32_t lgLink::getMeanRuntime(void) {
    uint32_t accRuntime = 0;
    uint32_t meanRuntime;
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
    for (uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++)
        accRuntime += runtimeVect[avgIndex];
    xSemaphoreGive(lgLinkLock);
    meanRuntime = accRuntime / avgSamples;
    return meanRuntime;
}

uint32_t lgLink::getMaxRuntime(void) {
    return maxRuntime;
}

void lgLink::clearMaxRuntime(void) {
    maxRuntime = 0;
}

flash* lgLink::getFlashObj(uint8_t p_flashType) {
    if (systemState::getOpStateBitmap() == OP_INIT) {
        Log.ERROR("lgLink::getFlashObj: opState %d does not allow to provide flash objects - returning NULL - and continuing..." CR, systemState::getOpStateBitmap());
        return NULL;
    }
    switch (p_flashType) {
    case SM_FLASH_SLOW:
        return FLASHSLOW;
        break;
    case SM_FLASH_NORMAL:
        return FLASHNORMAL;
        break;
    case SM_FLASH_FAST:
        return FLASHFAST;
        break;
    }
}
signalMastAspects* lgLink::getSignalMastAspectObj(void){
return signalMastAspectsObject;
}

void lgLink::failsafe(bool p_set) {
    if (!failsafeSet && p_set) {
        Log.info("lgLink::failsafe: Setting failsafe");
        xSemaphoreTake(dirtyPixelLock, portMAX_DELAY);
        uint8_t i = 0;
        while (!strip->canShow()) {
            vTaskDelay(10 / portTICK_PERIOD_MS);
            if (i == 10) {
                Log.WARN("lgLink::failsafe: Could not set failsafe" CR);
                return;
            }
        }
        for (uint16_t i = 0; i < dirtyPixelList.size(); i++) {
            delete dirtyPixelList.at(i);
            dirtyPixelList.clear(i);
        }
        failsafeSet = true;
        for (uint16_t i = 0; i < MAX_LGSTRIPLEN * 3; i++)
            stripWritebuff[i] = SM_BRIGHNESS_FAIL;
        strip->show();
        xSemaphoreGive(dirtyPixelLock);
    }
    else if (failsafeSet && !p_set){
        Log.info("lgLink::failsafe: Unsetting failsafe");
        failsafeSet = false;
    }
}

/* CLI decoration methods */
void lgLink::onCliGetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    uint8_t link;
    if (rc = static_cast<lgLink*>(p_cliContext)->getLink(&link)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg link, return code: %i", rc);
        return;
    }
    printCli("Lightgroup link: %i", link);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliSetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<lgLink*>(p_cliContext)->setLink(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg link, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgLink::onCliGetLinkOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Lightgroup-link overruns: %i", static_cast<lgLink*>(p_cliContext)->getOverRuns());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliClearLinkOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<lgLink*>(p_cliContext)->clearOverRuns();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgLink::onCliGetMeanLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Lightgroup-link mean latency: %i", static_cast<lgLink*>(p_cliContext)->getMeanLatency());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliGetMaxLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Lightgroup-link max latency: %i", static_cast<lgLink*>(p_cliContext)->getMaxLatency());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliClearMaxLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<lgLink*>(p_cliContext)->clearMaxLatency();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgLink::onCliGetMeanRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Lightgroup-link mean run-time: %i", static_cast<lgLink*>(p_cliContext)->getMeanRuntime());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliGetMaxRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("Lightgroup-link max run-time: %i", static_cast<lgLink*>(p_cliContext)->getMaxRuntime());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgLink::onCliClearMaxRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<lgLink*>(p_cliContext)->clearMaxRuntime();
    acceptedCliCommand(CLI_TERM_EXECUTED);
}
/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                             */
/*==============================================================================================================================================*/
