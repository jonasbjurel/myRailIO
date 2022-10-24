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
#include "sat.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "sat(Satelite)"                                                                                                                       */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
sat::sat(uint8_t p_satAddr, satLink* p_linkHandle) : systemState(this) {
    Log.notice("sat::sat: Creating Satelite adress %d" CR, p_satAddr);
    linkHandle = p_linkHandle;
    satAddr = p_satAddr;
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    satLock = xSemaphoreCreateMutex();
    if (satLock == NULL) {
        panic("sat::sat: Could not create Lock objects - rebooting...");
    }
}

sat::~sat(void) {
    panic("sat::~sat: sat destructior not supported - rebooting...");
}

rc_t sat::init(void) {
    Log.notice("sat::init: Initializing Satelite address %d" CR, satAddr);
    Log.notice("sat::init: Creating actuators for satelite address %d on link %d" CR, satAddr, linkHandle->getLink());
    for (uint8_t actPort = 0; actPort < MAX_ACT; actPort++) {
        acts[actPort] = new actBase(actPort, this);
        if (acts[actPort] == NULL)
            panic("sat::init: Could not create actuator object - rebooting...");
    }
    Log.notice("sat::init: Creating sensors for satelite address %d on link %d" CR, satAddr, linkHandle->getLink());
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
    if (~(getOpState() & OP_UNCONFIGURED))
        panic("sat:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("sat::onConfig: satAddress %d on link %d received an uverified configuration, parsing and validating it..." CR, satAddr, linkHandle->getLink());
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
    Log.notice("sat::onConfig: System name: %s" CR, xmlconfig[XML_SAT_SYSNAME]);
    Log.notice("sat::onConfig: User name:" CR, xmlconfig[XML_SAT_USRNAME]);
    Log.notice("sat::onConfig: Description: %s" CR, xmlconfig[XML_SAT_DESC]);
    Log.notice("sat::onConfig: Address: %s" CR, xmlconfig[XML_SAT_ADDR]);
    Log.notice("sat::onConfig: Configuring Actuators");
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
    Log.notice("sat::onConfig: Configuring sensors");
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
    Log.notice("sat::onConfig: Configuration successfully finished" CR);
}

rc_t sat::start(void) {
    Log.notice("sat::start: Starting Satelite address: %d on satlink %d" CR, satAddr, linkHandle->getLink());
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("sat::start: Satelite address %d on satlink %d not configured - will not start it" CR, satAddr, linkHandle->getLink());
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (getOpState() & OP_UNDISCOVERED) {
        Log.notice("sat::start: Satelite address %d on satlink %d not yet discovered - waiting for discovery before starting it" CR, satAddr, linkHandle->getLink());
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("sat::start: Subscribing to adm- and op state topics");
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
    Log.notice("sat::start: Satelite address: %d on satlink %d have started" CR, satAddr, linkHandle->getLink());
    return RC_OK;
}

void sat::onDiscovered(satelite* p_sateliteLibHandle, uint8_t p_satAddr, bool p_exists) {
    Log.notice("sat::onDiscovered: Satelite address %d on satlink %d discovered" CR, satAddr, linkHandle->getLink());
    if (p_satAddr != satAddr)
        panic("sat::onDiscovered: Inconsistant satelite address provided - rebooting...");
    satLibHandle = p_sateliteLibHandle;
    for (uint16_t actItter = 0; actItter < MAX_ACT; actItter++)
        acts[actItter]->onDiscovered(satLibHandle);
    for (uint16_t sensItter = 0; sensItter < MAX_SENS; sensItter++)
        senses[sensItter]->onDiscovered(satLibHandle);
    unSetOpState(OP_UNDISCOVERED);
    if (pendingStart) {
        Log.notice("sat::onDiscovered: Satelite address %d on satlink %d was waiting for discovery before it could be started - now starting it" CR, satAddr, linkHandle->getLink());
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
        Log.error("sat::onPmPoll: Failed to send PM report" CR);
}

void sat::onSatLibStateChangeHelper(satelite * p_sateliteLibHandle, uint8_t p_linkAddr, uint8_t p_satAddr, satOpState_t p_satOpState, void* p_satHandle) {
    ((sat*)p_satHandle)->onSatLibStateChange(p_satOpState);
}

void sat::onSatLibStateChange(satOpState_t p_satOpState) {
    if (!(getOpState() & OP_INIT)) {
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
    if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, satAddr, linkHandle->getLink());
    else if (p_sysState & OP_INTFAIL)
        panic("sat::onSystateChange: satelite has experienced an internal error - rebooting...");
    if (p_sysState)
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has received Opstate %b - doing nothing" CR, satAddr, linkHandle->getLink(), p_sysState);
    else
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has received a cleared Opstate - doing nothing" CR, satAddr, linkHandle->getLink());
}

void sat::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle) {
    ((sat*)p_satHandle)->onOpStateChange(p_topic, p_payload);
}

void sat::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("sat::onOpStateChange: satelite address %d on satlink %d got available message from server" CR, satAddr, linkHandle->getLink());
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("sat::onOpStateChange: satelite address %d on satlink %d got unavailable message from server" CR, satAddr, linkHandle->getLink());
    }
    else
        Log.error("sat::onOpStateChange: satelite address %d on satlink %d got an invalid availability message from server - doing nothing" CR, satAddr, linkHandle->getLink());
}

void sat::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkHandle) {
    ((satLink*)p_satLinkHandle)->onAdmStateChange(p_topic, p_payload);
}

void sat::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("sat::onAdmStateChange: satelite address %d on satlink %d got online message from server" CR, satAddr, linkHandle->getLink());
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("sat::onAdmStateChange: satelite address %d on satlink %d got off-line message from server" CR, satAddr, linkHandle->getLink());
    }
    else
        Log.error("sat::onAdmStateChange: satelite address %d on satlink %d got an invalid admstate message from server - doing nothing" CR, satAddr, linkHandle->getLink());
}

rc_t sat::setSystemName(const char* p_systemName, bool p_force) {
    Log.error("sat::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* sat::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getSystemName: cannot get System name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_SYSNAME];
}

rc_t sat::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.error("sat::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::setUsrName: cannot set System name as satelite is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("sat::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_SAT_USRNAME];
        xmlconfig[XML_SATLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* sat::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getUsrName: cannot get User name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_USRNAME];
}

rc_t sat::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.error("sat::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::setDesc: cannot set Description as satelite is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("sat::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SAT_DESC];
        xmlconfig[XML_SAT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* sat::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getDesc: cannot get Description as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_DESC];
}

rc_t sat::setAddr(uint8_t p_addr) {
    Log.error("sat::setAddr: cannot set Address - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

uint8_t sat::getAddr(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("satLink::getLink: cannot get Link No as satLink is not configured" CR);
        return -1; //WE NEED TO FIND AS STRATEGY AROUND RETURN CODES CODE WIDE
    }
    return atoi(xmlconfig[XML_SAT_ADDR]);
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

/*==============================================================================================================================================*/
/* END Class sat                                                                                                                                */
/*==============================================================================================================================================*/
