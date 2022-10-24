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
#include "lgBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgBase (Light-group base/Stem-cell class)"                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
lgBase::lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle) : systemState(this) {
    lgLinkHandle = (lgLink*)p_lgLinkHandle;
    lgAddress = p_lgAddress;
    lgLinkHandle->getLink(&lgLinkNo);
    Log.notice("lgBase::lgBase: Creating lgBase stem-object for lg address %d, on lgLink %d" CR, p_lgAddress, lgLinkNo);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    lgBaseLock = xSemaphoreCreateMutex();
    if (lgBaseLock == NULL)
        panic("lgBase::lgBase: Could not create Lock objects - rebooting...");
    debug = false;
    stripOffset = 0;
}

lgBase::~lgBase(void) {
    panic("lgBase::~lgBase: lgBase destructior not supported - rebooting...");
}

rc_t lgBase::init(void) {
    Log.notice("lgBase::init: Initializing stem-object for lg-address %d, on lgLink %d" CR, lgAddress, lgLinkNo);
    return RC_OK;
}

void lgBase::onConfig(const tinyxml2::XMLElement* p_lgXmlElement) {
    if (!(getOpState() & OP_UNCONFIGURED))
        panic("lgBase:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported rebooting...");
    Log.notice("lgBase::onConfig: lg-address %d, on lgLink %d received an unverified configuration, parsing and validating it...", lgAddress, lgLinkNo);
    xmlconfig[XML_LG_SYSNAME] = NULL;
    xmlconfig[XML_LG_USRNAME] = NULL;
    xmlconfig[XML_LG_DESC] = NULL;
    xmlconfig[XML_LG_LINKADDR] = NULL;
    xmlconfig[XML_LG_TYPE] = NULL;
    xmlconfig[XML_LG_SUBTYPE] = NULL;
    xmlconfig[XML_LG_PROPERTIES] = NULL;
    const char* lgSearchTags[7];
    lgSearchTags[XML_LG_SYSNAME] = "SystemName";
    lgSearchTags[XML_LG_USRNAME] = "UserName";
    lgSearchTags[XML_LG_DESC] = "Description";
    lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
    lgSearchTags[XML_LG_TYPE] = "Type";
    lgSearchTags[XML_LG_SUBTYPE] = "SubType";
    lgSearchTags[XML_LG_PROPERTIES] = "Properties";
    getTagTxt(p_lgXmlElement, lgSearchTags, xmlconfig, sizeof(lgSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_LG_SYSNAME])
        panic("lgBase::onConfig: System Name missing for lg %s - rebooting...");
    if (!xmlconfig[XML_LG_USRNAME])
        panic("lgBase::onConfig: User name missing for lg %s - rebooting...");
    if (!xmlconfig[XML_LG_DESC])
        panic("lgBase::onConfig: Description missing for lg %s - rebooting...");
    if (!xmlconfig[XML_LG_LINKADDR])
        panic("lgBase::onConfig: Address missing for lg %s - rebooting...");
    if (!xmlconfig[XML_LG_TYPE])
        panic("lgBase::onConfig: Type missing for lg %s - rebooting...");
    if (atoi((const char*)xmlconfig[XML_LG_LINKADDR]) != lgAddress)
        panic("lgBase::onConfig: lg address inconsistant for lg %s- rebooting...");
    Log.notice("lgBase::onConfig: Configuring lg %s with the follwing configuration" CR, xmlconfig[XML_LG_SYSNAME]);
    Log.notice("lgBase::onConfig: System name: %s" CR, xmlconfig[XML_LG_SYSNAME]);
    Log.notice("lgBase::onConfig: User name:" CR, xmlconfig[XML_LG_USRNAME]);
    Log.notice("lgBase::onConfig: Description: %s" CR, xmlconfig[XML_LG_DESC]);
    Log.notice("lgBase::onConfig: Type: %s" CR, xmlconfig[XML_LG_TYPE]);
    Log.notice("lgBase::onConfig: Properties: %s" CR, xmlconfig[XML_LG_PROPERTIES]);
    if (xmlconfig[XML_LG_PROPERTIES])
        Log.notice("lgBase::onConfig: lg type specific properties provided, will be passed to the actuator type sub-class object: %s" CR, xmlconfig[XML_ACT_PROPERTIES]);
    if (!strcmp((const char*)xmlconfig[XML_LG_TYPE], "SIGNAL MAST")) {
            Log.notice("lgBase::onConfig: lg-type is Signal mast - programing act-stem object by creating an lgSignalMast extention class object" CR);
            extentionLgClassObj = (void*) new lgSignalMast(this);
        }
    else
        panic("lgBase::onConfig: lg type not supported");
    CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], init());
    if (EXT_RC)
        panic("lgBase::onConfig: Failed to initialize Lg extention object - Rebooting...");
    if (xmlconfig[XML_LG_PROPERTIES]) {
        Log.notice("lgBase::onConfig: Configuring the lg base stem-object - Lg Adress: %d, Lg Link %d, Lg System Name %s: with properties" CR, lgAddress, lgLinkNo, xmlconfig[XML_LG_SYSNAME]);
        CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onConfig(((tinyxml2::XMLElement*) p_lgXmlElement)->FirstChildElement("Properties")));
    }
    else
        Log.notice("lgBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("lgBase::onConfig: Configuration successfully finished" CR);
}

rc_t lgBase::start(void) {
    Log.notice("lgBase::start: Starting lg address %d on lgLink %d" CR, lgAddress, lgLinkNo);
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("lgBase::start: lg address %d on lgLink %d not configured - will not start it" CR, lgAddress, lgLinkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("lgBase::start: lg address %d on lgLink %d  - starting extention class" CR, lgAddress, lgLinkNo);
    CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], start());
    if (EXT_RC)
        panic("lgBase::onConfig: Failed to start Light group");
    Log.notice("lgBase::start: Subscribing to adm- and op state topics");
    const char* admSubscribeTopic[5] = { MQTT_LG_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_LG_SYSNAME]};
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), onAdmStateChangeHelper, this))
        panic("lgBase::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_LG_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_LG_SYSNAME]};
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), onOpStateChangeHelper, this))
        panic("lgBase::start: Failed to suscribe to opState topic - rebooting...");
}

void lgBase::onSysStateChangeHelper(const void* p_lgBaseHandle, uint16_t p_sysState) {
    ((lgBase*)p_lgBaseHandle)->onSysStateChange(p_sysState);
}

void lgBase::onSysStateChange(uint16_t p_sysState) {
    if (!(p_sysState & OP_UNCONFIGURED)){
        CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onSysStateChange(p_sysState));
        if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
            Log.notice("lgBase::onSysStateChange: lg address %d on lgLink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, lgAddress, lgLinkNo);
        else if (p_sysState & OP_INTFAIL)
            panic("lgBase::onSysStateChange: lg has experienced an internal error - rebooting...");
        if (p_sysState)
            Log.notice("lgBase::onSysStateChange: lg address %d on lgLink %d has received Opstate %b - doing nothing" CR, lgAddress, lgLinkNo);
        else
            Log.notice("lgBase::onSysStateChange: lg address %d on lgLink %d has received a cleared Opstate - doing nothing" CR, lgAddress, lgLinkNo);
    }
}

void lgBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onOpStateChange(p_topic, p_payload);
}

void lgBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("lgBase::onOpStateChange: lg address %d on lgLink %d got available message from server: %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("lgBase::onOpStateChange: lg address %d on lgLink %d got unavailable message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else
        Log.error("lgBase::onOpStateChange: lg address %d on lgLink %d got an invalid availability message from server %s - doing nothing" CR, lgAddress, lgLinkNo, p_payload);
}

void lgBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onAdmStateChange(p_topic, p_payload);
}

void lgBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("lgBase::onAdmStateChange: lg address %d on lgLink %d got online message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("lgBase::onAdmStateChange: actuator port %d, on satelite adress %d, satLink %d got off-line message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else
        Log.error("lgBase::onAdmStateChange: actuator port %d, on satelite adress %d, satLink %d got an invalid admstate message from server %s - doing nothing" CR, lgAddress, lgLinkNo, p_payload);
}

rc_t lgBase::setSystemName(char* p_systemName, bool p_force) {
    Log.error("lgBase::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getSystemName(const char* p_systemName) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getSystemName: cannot get System name as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_systemName = xmlconfig[XML_LG_SYSNAME];
    return RC_OK;
}

rc_t lgBase::setUsrName(char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.error("lgBase::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::setUsrName: cannot set System name as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("lgBase::setUsrName: Setting User name to %s" CR, p_usrName);
        delete (char*)xmlconfig[XML_LG_USRNAME];
        xmlconfig[XML_LG_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

rc_t lgBase::getUsrName(const char* p_userName) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getUsrName: cannot get User name as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    p_userName = xmlconfig[XML_LG_USRNAME];
    return RC_OK;
}

rc_t lgBase::setDesc(char* p_description, const bool p_force) {
    if (!debug || !p_force) {
        Log.error("lgBase::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::setDesc: cannot set Description as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("lgBase::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LG_DESC];
        xmlconfig[XML_LG_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t lgBase::getDesc(const char* p_desc) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getDesc: cannot get Description as lg is not configured" CR);
        p_desc = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    p_desc = xmlconfig[XML_LG_DESC];
    return RC_OK;
}

rc_t lgBase::setAddress(uint8_t p_address) {
    Log.error("lgBase::setPort: cannot set port - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getAddress(uint8_t* p_address) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getPort: cannot get port as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    *p_address = lgAddress;
    return RC_OK;
}

rc_t lgBase::setNoOffLeds(uint8_t p_noOfLeds){
    Log.error("lgBase::NoOfLeds: cannot set number of leds - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getNoOffLeds(uint8_t* p_noOfLeds) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getNoOffLeds: cannot get lg number of Leds as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getNoOffLeds(p_noOfLeds));
    return EXT_RC;
}

rc_t lgBase::setProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], setProperty(p_propertyId, p_propertyValue));
    return EXT_RC;
}

rc_t lgBase::getProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgBase::getProperties: cannot get port as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getProperty(p_propertyId, p_propertyValue));
    return EXT_RC;
}

void lgBase::setDebug(const bool p_debug) {
    debug = p_debug;
}

bool lgBase::getDebug(void) {
    return debug;
}

void lgBase::setStripOffset(uint16_t p_stripOffset) {
    stripOffset = p_stripOffset;
    return;
}

uint16_t lgBase::getStripOffset(void) {
    return stripOffset;
}

/*==============================================================================================================================================*/
/* END Class lgBase                                                                                                                             */
/*==============================================================================================================================================*/
