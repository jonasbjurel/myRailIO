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
#include "actBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actBase (Actor base/Stem-cell class)"                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actBase::actBase(uint8_t p_actPort, sat* p_satHandle) : systemState(this) {
    Log.notice("actBase::actBase: Creating actBase stem-object for actuator port %d, on satelite adress %d, satLink %d" CR, p_actPort, p_satHandle->getAddr(), p_satHandle->linkHandle->getLink());
    satHandle = p_satHandle;
    actPort = p_actPort;
    satAddr = satHandle->getAddr();
    satLinkNo = satHandle->linkHandle->getLink();
    regSysStateCb(this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    actLock = xSemaphoreCreateMutex();
    pendingStart = false;
    satLibHandle = NULL;
    debug = false;
    if (actLock == NULL)
        panic("actBase::actBase: Could not create Lock objects - rebooting...");
}

actBase::~actBase(void) {
    panic("actBase::~actBase: actBase destructior not supported - rebooting...");
}

rc_t actBase::init(void) {
    Log.notice("actBase::init: Initializing stem-object for actuator port %d, on satelite adress %d, satLink %d" CR, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actBase::onConfig(const tinyxml2::XMLElement* p_actXmlElement) {
    if (!(getOpState() & OP_UNCONFIGURED))
        panic("actBase:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.notice("actBase::onConfig: actuator port %d, on satelite adress %d, satLink %d received an uverified configuration, parsing and validating it..." CR, actPort, satAddr, satLinkNo);
    xmlconfig[XML_ACT_SYSNAME] = NULL;
    xmlconfig[XML_ACT_USRNAME] = NULL;
    xmlconfig[XML_ACT_DESC] = NULL;
    xmlconfig[XML_ACT_PORT] = NULL;
    xmlconfig[XML_ACT_TYPE] = NULL;
    xmlconfig[XML_ACT_PROPERTIES] = NULL;
    const char* actSearchTags[7];
    actSearchTags[XML_ACT_SYSNAME] = "SystemName";
    actSearchTags[XML_ACT_USRNAME] = "UserName";
    actSearchTags[XML_ACT_DESC] = "Description";
    actSearchTags[XML_ACT_PORT] = "Port";
    actSearchTags[XML_ACT_TYPE] = "Type";
    actSearchTags[XML_ACT_SUBTYPE] = "SubType";
    actSearchTags[XML_ACT_PROPERTIES] = "Properties";
    getTagTxt(p_actXmlElement, actSearchTags, xmlconfig, sizeof(actSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_ACT_SYSNAME])
        panic("actBase::onConfig: SystemNane missing - rebooting...");
    if (!xmlconfig[XML_ACT_USRNAME])
        panic("actBase::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_ACT_DESC])
        panic("actBase::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_ACT_PORT])
        panic("actBase::onConfig: Port missing - rebooting...");
    if (!xmlconfig[XML_ACT_TYPE])
        panic("actBase::onConfig: Type missing - rebooting...");
    if (atoi((const char*)xmlconfig[XML_ACT_PORT]) != actPort)
        panic("actBase::onConfig: Port No inconsistant - rebooting...");
    Log.notice("actBase::onConfig: System name: %s" CR, xmlconfig[XML_ACT_SYSNAME]);
    Log.notice("actBase::onConfig: User name:" CR, xmlconfig[XML_ACT_USRNAME]);
    Log.notice("actBase::onConfig: Description: %s" CR, xmlconfig[XML_ACT_DESC]);
    Log.notice("actBase::onConfig: Port: %s" CR, xmlconfig[XML_ACT_PORT]);
    Log.notice("actBase::onConfig: Type: %s" CR, xmlconfig[XML_ACT_TYPE]);
    if (xmlconfig[XML_ACT_PROPERTIES])
        Log.notice("actBase::onConfig: Actuator type specific properties provided, will be passed to the actuator type sub-class object: %s" CR, xmlconfig[XML_ACT_PROPERTIES]);
    if (!strcmp((const char*)xmlconfig[XML_ACT_TYPE], "TURNOUT")) {
            Log.notice("actBase::onConfig: actuator type is turnout - programing act-stem object by creating an turnAct extention class object" CR);
            extentionActClassObj = (void*) new actTurn(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
        }
    else if (!strcmp((const char*)xmlconfig[XML_ACT_SUBTYPE], "LIGHT")) {
        Log.notice("actBase::onConfig: actuator type is light - programing act-stem object by creating an lightAct extention class object" CR);
        extentionActClassObj = (void*) new actLight(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
    }
    else if (!strcmp((const char*)xmlconfig[XML_ACT_SUBTYPE], "MEMORY")) {
        Log.notice("actBase::onConfig: actuator type is memory - programing act-stem object by creating an memAct extention class object" CR);
        extentionActClassObj = (void*) new actMem(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
    }
    else
        panic("actBase::onConfig: actuator type not supported");
    CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], init());
    if (xmlconfig[XML_ACT_PROPERTIES]) {
        Log.notice("actBase::onConfig: Configuring the actuator base stem-object with properties" CR);
        CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], onConfig(p_actXmlElement->FirstChildElement("Properties")));
    }
    else
        Log.notice("actBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("actBase::onConfig: Configuration successfully finished" CR);
}

rc_t actBase::start(void) {
    Log.notice("actBase::start: Starting actuator port %d, on satelite adress %d, satLink %d" CR, actPort, satAddr, satLinkNo);
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("actBase::start: actuator port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, actPort, satAddr, satLinkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (getOpState() & OP_UNDISCOVERED) {
        Log.notice("actBase::start: actuator port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, actPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("actBase::start: actuator port %d, on satelite adress %d, satLink %d - starting extention class" CR, actPort, satAddr, satLinkNo);
    CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], start());
    Log.notice("actBase::start: Subscribing to adm- and op state topics");
    const char* admSubscribeTopic[5] = { MQTT_ACT_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), onAdmStateChangeHelper, this))
        panic("actBase::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_ACT_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), onOpStateChangeHelper, this))
        panic("actBase::start: Failed to suscribe to opState topic - rebooting...");
}

void actBase::onDiscovered(satelite* p_sateliteLibHandle) {
    Log.notice("actBase::onDiscovered: actuator port %d, on satelite adress %d, satLink %d discovered" CR, actPort, satAddr, satLinkNo);
    satLibHandle = p_sateliteLibHandle;
}

void actBase::onSysStateChangeHelper(const void* p_actBaseHandle, uint16_t p_sysState) {
    ((actBase*)p_actBaseHandle)->onSysStateChange(p_sysState);
}

void actBase::onSysStateChange(uint16_t p_sysState) {
    if (!(p_sysState & OP_UNCONFIGURED)){
        CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], onSysStateChange(p_sysState));
        if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
            Log.notice("actBase::onSysStateChange: actuator port %d, on satelite adress %d, satLink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, actPort, satAddr, satLinkNo);
        else if (p_sysState & OP_INTFAIL)
            panic("actBase::onSysStateChange: actuator port has experienced an internal error - rebooting...");
        if (p_sysState)
            Log.notice("actBase::onSysStateChange: actuator port %d, on satelite adress %d, satLink %d has received Opstate %b - doing nothing" CR, actPort, satAddr, satLinkNo, p_sysState);
        else
            Log.notice("actBase::onSysStateChange: actuator port %d, on satelite adress %d, satLink %d has received a cleared Opstate - doing nothing" CR, actPort, satAddr, satLinkNo);
    }
}

void actBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_actHandle) {
    ((actBase*)p_actHandle)->onOpStateChange(p_topic, p_payload);
}

void actBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("actBase::onOpStateChange: actuator port %d, on satelite adress %d, satLink %d got available message from server: %s" CR, actPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("actBase::onOpStateChange: actuator port %d, on satelite adress %d, satLink %d got unavailable message from server %s" CR, actPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.error("actBase::onOpStateChange: actuator port %d, on satelite address %d on satlink %d got an invalid availability message from server %s - doing nothing" CR, actPort, satAddr, satLinkNo, p_payload);
}

void actBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_actHandle) {
    ((actBase*)p_actHandle)->onAdmStateChange(p_topic, p_payload);
}

void actBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("actBase::onAdmStateChange: actuator port %d, on satelite adress %d, satLink %d got online message from server %s" CR, actPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("actBase::onAdmStateChange: actuator port %d, on satelite adress %d, satLink %d got off-line message from server %s" CR, actPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.error("actBase::onAdmStateChange: actuator port %d, on satelite adress %d, satLink %d got an invalid admstate message from server %s - doing nothing" CR, actPort, satAddr, satLinkNo, p_payload);
}

rc_t actBase::setSystemName(const char* p_systemName, bool p_force) {
    Log.error("actBase::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* actBase::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::getSystemName: cannot get System name as actuator is not configured" CR);
        return NULL;
    }
    return (const char*)xmlconfig[XML_ACT_SYSNAME];
}

rc_t actBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.error("actBase::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::setUsrName: cannot set System name as actuator is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("actBase::setUsrName: Setting User name to %s" CR, p_usrName);
        delete (char*)xmlconfig[XML_ACT_USRNAME];
        xmlconfig[XML_ACT_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* actBase::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::getUsrName: cannot get User name as actuator is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_ACT_USRNAME];
}

rc_t actBase::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.error("actBase::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::setDesc: cannot set Description as actuator is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("actBase::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_ACT_DESC];
        xmlconfig[XML_ACT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* actBase::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::getDesc: cannot get Description as actuator is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_ACT_DESC];
}

rc_t actBase::setPort(uint8_t p_port) {
    Log.error("actBase::setPort: cannot set port - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

int8_t actBase::getPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("actBase::getPort: cannot get port as actuator is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_ACT_PORT]);
}

void actBase::setDebug(bool p_debug) {
    debug = p_debug;
}

bool actBase::getDebug(void) {
    return debug;
}

/*==============================================================================================================================================*/
/* END Class actBase                                                                                                                           */
/*==============================================================================================================================================*/
