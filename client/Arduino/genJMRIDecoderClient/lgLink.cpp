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
#include "lgLink.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgLink(lightgroupLink)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
lgLink::lgLink(uint8_t p_linkNo) : systemState(this) {
    Log.notice("lgLink::lgLink: Creating Lightgroup link channel %d" CR, p_linkNo);
    linkNo = p_linkNo;
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
    lgLinkLock = xSemaphoreCreateMutex();
    if (lgLinkLock == NULL)
        panic("lgLink::satLink: Could not create Lock objects - rebooting...");
    avgSamples = UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS;
    latencyVect = new int64_t[avgSamples];
    runtimeVect = new uint32_t[avgSamples];
    if (latencyVect == NULL || runtimeVect == NULL)
        panic("lgLink::satLink: Could not create PM vectors - rebooting...");
}

lgLink::~lgLink(void) {
    panic("lgLink::~lgLink: lgLink destructior not supported - rebooting...");
}

rc_t lgLink::init(void) {
    Log.notice("lgLink::init: Initializing Lightgroup link channel %d" CR, linkNo);
    Log.notice("lgLink::init: Creating lighGrups for link channel %d" CR, linkNo);
    for (uint8_t lgAddress = 0; lgAddress < MAX_LGSTRIPLEN; lgAddress++) {
        lgs[lgAddress] = new lgBase(lgAddress, this);
        if (lgs[lgAddress] == NULL)
            panic("lgLink::init: Could not create light-group object - rebooting...");
    }
    Log.notice("lgLink::init: Creating flash objects for lgLink Channel" CR, linkNo);
    FLASHNORMAL = new flash(SM_FLASH_NORMAL, 50);
    FLASHSLOW = new flash(SM_FLASH_SLOW, 50);
    FLASHFAST = new flash(SM_FLASH_FAST, 50);
    if (FLASHNORMAL == NULL || FLASHSLOW == NULL || FLASHFAST == NULL) {
        panic("lgLink::init: Could not create flash objects - rebooting...");
    }
    stripCtrlBuff = new stripLed_t[MAX_LGSTRIPLEN];
    strip = new Adafruit_NeoPixel(MAX_LGSTRIPLEN, (LGLINK_PINS[linkNo]), NEO_RGB + NEO_KHZ800);
    if (strip == NULL)
        panic("lgLink::init: Could not create NeoPixel object - rebooting...");
    strip->begin();
    stripWritebuff = strip->getPixels();
    return RC_OK;
}

void lgLink::onConfig(const tinyxml2::XMLElement* p_lightgroupLinkXmlElement) {
    if (~(getOpState() & OP_UNCONFIGURED))
        panic("lgLink:onConfig: lgLink received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("lgLink::onConfig: lgLink channel %d received an uverified configuration, parsing and validating it..." CR, linkNo);
    xmlconfig[XML_LGLINK_SYSNAME] = NULL;
    xmlconfig[XML_LGLINK_USRNAME] = NULL;
    xmlconfig[XML_LGLINK_DESC] = NULL;
    xmlconfig[XML_LGLINK_LINK] = NULL;
    const char* lgLinkSearchTags[4];
    lgLinkSearchTags[XML_LGLINK_SYSNAME] = "SystemName";
    lgLinkSearchTags[XML_LGLINK_USRNAME] = "UserName";
    lgLinkSearchTags[XML_LGLINK_DESC] = "Description";
    lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
    getTagTxt(p_lightgroupLinkXmlElement, lgLinkSearchTags, xmlconfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_LGLINK_SYSNAME])
        panic("lgLink::onConfig: SystemName missing - rebooting...");
    sysName = xmlconfig[XML_LGLINK_SYSNAME];
    if (!xmlconfig[XML_LGLINK_USRNAME])
        panic("lgLink::onConfig: User name missing - rebooting...");
    usrName = xmlconfig[XML_LGLINK_USRNAME];
    if (!xmlconfig[XML_LGLINK_DESC])
        panic("lgLink::onConfig: Description missing - rebooting...");
    desc = xmlconfig[XML_LGLINK_DESC];
    if (!xmlconfig[XML_LGLINK_LINK])
        panic("lgLink::onConfig: Link missing - rebooting...");
    linkNo = atoi(xmlconfig[XML_LGLINK_LINK]);
    if (atoi(xmlconfig[XML_LGLINK_LINK]) != linkNo)
        panic("lgLink::onConfig: Link no inconsistant - rebooting...");
    Log.notice("lgLink::onConfig: System name: %s" CR, xmlconfig[XML_LGLINK_SYSNAME]);
    Log.notice("lgLink::onConfig: User name:" CR, xmlconfig[XML_LGLINK_USRNAME]);
    Log.notice("lgLink::onConfig: Description: %s" CR, xmlconfig[XML_LGLINK_DESC]);
    Log.notice("lgLink::onConfig: Link: %s" CR, xmlconfig[XML_LGLINK_LINK]);
    Log.notice("lgLink::onConfig: Creating and configuring signal mast aspect description object");
    signalMastAspectsObject = new signalMastAspects(this);
    if (signalMastAspectsObject == NULL)
        panic("lgLink:onConfig: Could not start signalMastAspect object - rebooting...");
    if (!((tinyxml2::XMLElement*) p_lightgroupLinkXmlElement)->FirstChildElement("SignalMastDesc"))
        panic("lgLink::onConfig: Signal mast aspect description missing - rebooting...");
    else if (signalMastAspectsObject->onConfig(((tinyxml2::XMLElement*)p_lightgroupLinkXmlElement)->FirstChildElement("SignalMastDesc")))
        panic("lgLink::onConfig: Could not configure Signal mast aspect description - rebooting...");
    if (((tinyxml2::XMLElement*) p_lightgroupLinkXmlElement)->NextSiblingElement("SignalMastDesc"))
        panic("lgLink::onConfig: Multiple signal mast aspect descriptions provided - rebooting...");
    Log.notice("lgLink::onConfig: Creating and configuring Light groups");
    tinyxml2::XMLElement* lgXmlElement;
    lgXmlElement = ((tinyxml2::XMLElement*)p_lightgroupLinkXmlElement)->FirstChildElement("LightGroup");
    const char* lgSearchTags[8];
    char* lgXmlconfig[8];
    for (uint16_t lgItter = 0; false; lgItter++) {
        if (lgXmlElement == NULL)
            break;
        if (lgItter > MAX_LGSTRIPLEN)
            panic("lgLink::onConfig: More than maximum lgs provided - not supported, rebooting...");
        lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
        getTagTxt(lgXmlElement, lgSearchTags, lgXmlconfig, sizeof(lgXmlElement) / 4); // Need to fix the addressing for portability
        if (!lgXmlconfig[XML_LG_LINKADDR])
            panic("lgLink::onConfig:: LG Linkaddr missing - rebooting...");
        lgs[atoi(lgXmlconfig[XML_LG_LINKADDR])]->onConfig(lgXmlElement);
        addSysStateChild(lgs[atoi(lgXmlconfig[XML_LG_LINKADDR])]);
        lgXmlElement = ((tinyxml2::XMLElement*) p_lightgroupLinkXmlElement)->NextSiblingElement("LightGroup");
    }
    uint16_t stripOffset = 0;
    for (uint16_t lgItter = 0; lgItter < MAX_LGSTRIPLEN; lgItter++) {
        lgs[lgItter]->setStripOffset(stripOffset);
        uint8_t noOfLeds;
        lgs[lgItter]->getNoOffLeds(&noOfLeds);
        stripOffset += noOfLeds;
    }
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("lgLink::onConfig: Configuration successfully finished" CR);
}

rc_t lgLink::start(void) {
    Log.notice("lgLink::start: Starting lightgroup link: %d" CR, linkNo);
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("lgLink::start: LG Link %d not configured - will not start it" CR, linkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("lgLink::start: Subscribing to adm- and op state topics");
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s,%s,%s,%s", MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getDecoderUri(), "/", sysName);
    if (mqtt::subscribeTopic(subscribeTopic, &onAdmStateChangeHelper, this))
        panic("lgLink::start: Failed to suscribe to admState topic - rebooting...");
    sprintf(subscribeTopic, "%s,%s,%s,%s", MQTT_DECODER_OPSTATE_TOPIC, mqtt::getDecoderUri(), "/", sysName);
    if (mqtt::subscribeTopic(subscribeTopic, &onOpStateChangeHelper, this))
        panic("lgLink::start: Failed to suscribe to opState topic - rebooting...");
    for (uint16_t lgItter = 0; lgItter < MAX_LGSTRIPLEN; lgItter++)
        lgs[lgItter]->start();
    char taskName[20];
    sprintf(taskName, "%s-%d", CPU_UPDATE_STRIP_TASKNAME, linkNo);
    xTaskCreatePinnedToCore(
        updateStripHelper,                                  // Task function
        CPU_UPDATE_STRIP_TASKNAME,                          // Task function name reference
        CPU_UPDATE_STRIP_STACKSIZE_1K * 1024,               // Stack size
        this,                                               // Parameter passing
        CPU_UPDATE_STRIP_PRIO,                              // Priority 0-24, higher is more
        NULL,                                               // Task handle
        CPU_UPDATE_STRIP_CORE[linkNo % 2]);                 // Core [CORE_0 | CORE_1]
    unSetOpState(OP_INIT);
    Log.notice("lgLink::start: lightgroups link %d and all its lightgroupDecoders have started" CR, linkNo);
    return RC_OK;
}

void lgLink::onSysStateChangeHelper(const void* p_miscData, uint16_t p_sysState) {
    ((lgLink*)p_miscData)->onSysStateChange(p_sysState);
}

void lgLink::onSysStateChange(uint16_t p_sysState) {
    if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
        Log.notice("lgLink::onSystateChange:  lglink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, linkNo);
    else if (p_sysState & OP_INTFAIL)
        panic("lgLink::onSystateChange: lg link has experienced an internal error - rebooting...");
    if (p_sysState)
        Log.notice("lgLink::onSystateChange: Link %d has received Opstate %b - doing nothing" CR, linkNo, p_sysState);
    else
        Log.notice("lgLink::onSystateChange: Link %d has received a cleared Opstate - doing nothing" CR, linkNo);
}

void lgLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onOpStateChange(p_topic, p_payload);
}

void lgLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("lgLink::onOpStateChange: got available message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("lgLink::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.error("lgLink::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void lgLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void lgLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("lgLink::onAdmStateChange: got online message from server" CR);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("lgLink::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.error("lgLink::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

rc_t lgLink::setSystemName(char* p_systemName, bool p_force) {
    Log.error("lgLink::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgLink::getSystemName(const char* p_systemName){
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getSystemName: cannot get System name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_systemName = xmlconfig[XML_LGLINK_SYSNAME];
    return RC_OK;
}

rc_t lgLink::setUsrName(char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.error("lgLink::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::setUsrName: cannot set System name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("lgLink::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_LGLINK_USRNAME];
        xmlconfig[XML_LGLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

rc_t  lgLink::getUsrName(const char* p_userName){
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getUsrName: cannot get User name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_userName = xmlconfig[XML_LGLINK_USRNAME];
    return RC_OK;
}

rc_t lgLink::setDesc(char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.error("lgLink::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::setDesc: cannot set Description as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("lgLink::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LGLINK_DESC];
        xmlconfig[XML_LGLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t lgLink::getDesc(const char* p_desc){
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getDesc: cannot get Description as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_desc = xmlconfig[XML_LGLINK_DESC];
    return RC_OK;
}

rc_t lgLink::setLink(uint8_t p_link) {
    Log.error("lgLink::setLink: cannot set Link No - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgLink::getLink(uint8_t* p_link) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getLink: cannot get Link No as lgLink is not configured" CR);
        return RC_GEN_ERR;
    }
    *p_link = atoi(xmlconfig[XML_LGLINK_LINK]);
    return RC_OK;
}

void lgLink::setDebug(bool p_debug) {
    debug = p_debug;
}

bool lgLink::getDebug(void) {
    return debug;
}

rc_t lgLink::updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, uint8_t* p_wantedValueBuff, uint16_t* p_transitionTimeBuff) {
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
    for (uint16_t i = 0; i < p_buffLen; i++) {
        stripCtrlBuff[i + p_seqOffset].incrementValue = floor(abs(p_wantedValueBuff[i] - stripCtrlBuff[i + p_seqOffset].currentValue) / (p_transitionTimeBuff[i] / STRIP_UPDATE_MS));
        stripCtrlBuff[i + p_seqOffset].wantedValue = p_wantedValueBuff[i];
        stripCtrlBuff[i + p_seqOffset].dirty = true;
        bool alreadyDirty = false;
        for (uint16_t j = 0; j < dirtyList.size(); j++) {
            if (dirtyList.get(j) == &stripCtrlBuff[i + p_seqOffset]) {
                alreadyDirty = true;
                break;
            }
        }
        if (!alreadyDirty) {
            dirtyList.push_back(&stripCtrlBuff[i + p_seqOffset]);
        }
    }
    xSemaphoreGive(lgLinkLock);
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
    uint32_t maxAvgIndex = floor(UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS);
    uint32_t loopTime = STRIP_UPDATE_MS * 1000;
    Log.verbose("gLink::updateStrip: Starting sriphandler channel %d" CR, linkNo);
    while (true) {
        xSemaphoreTake(lgLinkLock, portMAX_DELAY);
        startTime = esp_timer_get_time();
        thisLoopTime = nextLoopTime;
        nextLoopTime += loopTime;
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
            Log.verbose("Couldnt update strip channel %d, continuing..." CR, linkNo);
        }
        else {
            for (int i = 0; i < dirtyList.size(); i++) {
                currentValue = (int)dirtyList.get(i)->currentValue;
                wantedValue = (int)dirtyList.get(i)->wantedValue;
                incrementValue = (int)dirtyList.get(i)->incrementValue;
                if (wantedValue > currentValue) {
                    currentValue += dirtyList.get(i)->incrementValue;
                    if (currentValue > wantedValue) {
                        currentValue = wantedValue;
                    }
                }
                else {
                    currentValue -= dirtyList.get(i)->incrementValue;
                    if (currentValue < wantedValue) {
                        currentValue = wantedValue;
                    }
                }
                stripWritebuff[dirtyList.get(i) - stripCtrlBuff] = currentValue;
                dirtyList.get(i)->currentValue = (uint8_t)currentValue;
                if (wantedValue == currentValue) {
                    dirtyList.get(i)->dirty = false;
                    dirtyList.clear(i);
                }
            }
            strip->show();
        }
    }
    runtime = esp_timer_get_time() - startTime;
    runtimeVect[avgIndex] = runtime;
    if (runtime > maxRuntime) {
        maxRuntime = runtime;
    }
    TickType_t delay;
    if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0) {
        xSemaphoreGive(lgLinkLock);
        vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
    }
    else {
        Log.verbose("Strip channel %d overrun" CR, linkNo);
        overRuns++;
        xSemaphoreGive(lgLinkLock);
        nextLoopTime = esp_timer_get_time();
    }
    avgIndex++;
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
    if (getOpState() == OP_INIT) {
        Log.error("lgLink::getFlashObj: opState %d does not allow to provide flash objects - returning NULL - and continuing..." CR, getOpState());
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

/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                             */
/*==============================================================================================================================================*/
