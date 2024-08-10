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
#include "lgBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgBase (Light-group base/Stem-cell class)"                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/


lgBase::lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle) : systemState(p_lgLinkHandle), globalCli(LG_MO_NAME, LG_MO_NAME, p_lgAddress, p_lgLinkHandle) {
    asprintf(&logContextName, "%s/%s-%i", p_lgLinkHandle->getLogContextName(), "lgBase", p_lgAddress);
    LOG_INFO("%s: Creating lgBase stem-object" CR, logContextName);
    lgLinkHandle = (lgLink*)p_lgLinkHandle;
    lgAddress = p_lgAddress;
    lgLinkHandle->getLink(&lgLinkNo);
    extentionLgClassObj = NULL;
    char sysStateObjName[20];
    sprintf(sysStateObjName, "lg-%d", lgAddress);
    setSysStateObjName(sysStateObjName);
    prevSysState = OP_WORKING;
    systemState::setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNUSED);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    if (!(lgBaseLock = xSemaphoreCreateMutex())){
            panic("Could not create Lock objects");
            return;
    }
    xmlconfig[XML_LG_SYSNAME] = NULL;
    xmlconfig[XML_LG_USRNAME] = NULL;
    xmlconfig[XML_LG_DESC] = NULL;
    xmlconfig[XML_LG_LINKADDR] = NULL;
    xmlconfig[XML_LG_TYPE] = NULL;
    xmlconfig[XML_LG_PROPERTY1] = NULL;
    xmlconfig[XML_LG_PROPERTY2] = NULL;
    xmlconfig[XML_LG_PROPERTY3] = NULL;
    xmlconfig[XML_LG_ADMSTATE] = NULL;
    debug = false;
    stripOffset = 0;
}

lgBase::~lgBase(void) {
    panic("%s: lgBase destructior not supported", logContextName);
}

rc_t lgBase::init(void) {
    LOG_INFO("%s: Initializing lgBase stem-object" CR, logContextName);
    //CLI decoration definitions
    LOG_INFO("%s: Registering lg specific CLI methods" CR, logContextName);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set address
        regCmdMoArg(GET_CLI_CMD, LG_MO_NAME, LGADDR_SUB_MO_NAME, onCliGetAddressHelper);
        regCmdHelp(GET_CLI_CMD, LG_MO_NAME, LGADDR_SUB_MO_NAME, LG_GET_LGADDR_HELP_TXT);
        regCmdMoArg(SET_CLI_CMD, LG_MO_NAME, LGADDR_SUB_MO_NAME, onCliSetAddressHelper);
        regCmdHelp(SET_CLI_CMD, LG_MO_NAME, LGADDR_SUB_MO_NAME, LG_SET_LGADDR_HELP_TXT);

        // get/set ledcnt
        regCmdMoArg(GET_CLI_CMD, LG_MO_NAME, LGLEDCNT_SUB_MO_NAME, onCliGetLedCntHelper);
        regCmdHelp(GET_CLI_CMD, LG_MO_NAME, LGLEDCNT_SUB_MO_NAME, LG_GET_LGLEDCNT_HELP_TXT);
        regCmdMoArg(SET_CLI_CMD, LG_MO_NAME, LGLEDCNT_SUB_MO_NAME, onCliSetLedCntHelper);
        regCmdHelp(SET_CLI_CMD, LG_MO_NAME, LGLEDCNT_SUB_MO_NAME, LG_SET_LGLEDCNT_HELP_TXT);

        // get/set ledoffset
        regCmdMoArg(GET_CLI_CMD, LG_MO_NAME, LGLEDOFFSET_SUB_MO_NAME, onCliGetLedOffsetHelper);
        regCmdHelp(GET_CLI_CMD, LG_MO_NAME, LGLEDOFFSET_SUB_MO_NAME, LG_GET_LGLEDOFFSET_HELP_TXT);
        regCmdMoArg(SET_CLI_CMD, LG_MO_NAME, LGLEDOFFSET_SUB_MO_NAME, onCliSetLedOffsetHelper);
        regCmdHelp(SET_CLI_CMD, LG_MO_NAME, LGLEDOFFSET_SUB_MO_NAME, LG_SET_LGLEDOFFSET_HELP_TXT);

        // get/set properties
        regCmdMoArg(GET_CLI_CMD, LG_MO_NAME, LGPROPERTY_SUB_MO_NAME, onCliGetPropertyHelper);
        regCmdHelp(GET_CLI_CMD, LG_MO_NAME, LGPROPERTY_SUB_MO_NAME, LG_GET_LGPROPERTY_HELP_TXT);
        regCmdMoArg(SET_CLI_CMD, LG_MO_NAME, LGPROPERTY_SUB_MO_NAME, onCliSetPropertyHelper);
        regCmdHelp(SET_CLI_CMD, LG_MO_NAME, LGPROPERTY_SUB_MO_NAME, LG_SET_LGPROPERTY_HELP_TXT);

        // get/set showing
        regCmdMoArg(GET_CLI_CMD, LG_MO_NAME, LGSHOWING_SUB_MO_NAME, onCliGetShowingHelper);
        regCmdHelp(GET_CLI_CMD, LG_MO_NAME, LGSHOWING_SUB_MO_NAME, LG_GET_LGSHOWING_HELP_TXT);
        regCmdMoArg(SET_CLI_CMD, LG_MO_NAME, LGSHOWING_SUB_MO_NAME, onCliSetShowingHelper);
        regCmdHelp(SET_CLI_CMD, LG_MO_NAME, LGSHOWING_SUB_MO_NAME, LG_SET_LGSHOWING_HELP_TXT);
    LOG_INFO("%s: specific lg CLI methods registered" CR, logContextName);
    return RC_OK;
}

void lgBase::onConfig(const tinyxml2::XMLElement* p_lgXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)){
        panic("%s Received a configuration, while the it was already configured, dynamic re-configuration not supported", logContextName);
        return;
    }
    LOG_INFO("%s: Received an unverified configuration, parsing and validating it..." CR, logContextName);

    //PARSING CONFIGURATION
    const char* lgSearchTags[9];
    lgSearchTags[XML_LG_SYSNAME] = "JMRISystemName";
    lgSearchTags[XML_LG_USRNAME] = "JMRIUserName";
    lgSearchTags[XML_LG_DESC] = "JMRIDescription";
    lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
    lgSearchTags[XML_LG_TYPE] = "Type";
    lgSearchTags[XML_LG_PROPERTY1] = "Property1";
    lgSearchTags[XML_LG_PROPERTY2] = "Property2";
    lgSearchTags[XML_LG_PROPERTY3] = "Property3";
    lgSearchTags[XML_LG_ADMSTATE] = "AdminState";
    getTagTxt(p_lgXmlElement->FirstChildElement(), lgSearchTags, xmlconfig, sizeof(lgSearchTags) / 4); // Need to fix the addressing for portability
    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_LG_SYSNAME]){
        panic("%s: System Name missing", logContextName);
        return;
    }
	setContextSysName(xmlconfig[XML_LG_SYSNAME]);
    if (!xmlconfig[XML_LG_USRNAME]){
        LOG_WARN("%s: User name was not provided - using \"%s-UserName\"" CR, logContextName, xmlconfig[XML_LG_SYSNAME]);
        xmlconfig[XML_LG_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_LG_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_LG_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_LG_USRNAME], "%s-Username", xmlconfig[XML_LG_SYSNAME]);
    }
    if (!xmlconfig[XML_LG_DESC]) {
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_LG_DESC] = new (heap_caps_malloc(sizeof(char[2]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[2];
        strcpy(xmlconfig[XML_LG_DESC], "-");
    }
    if (!xmlconfig[XML_LG_LINKADDR]){
        panic("%s lgLinkAddress missing", logContextName);
        return;
    }
    if (atoi((const char*)xmlconfig[XML_LG_LINKADDR]) != lgAddress){
        panic("%s: Provided lgLinkAddress inconsistant with the constructor provided address", logContextName);
        return;
    }
    if (!xmlconfig[XML_LG_TYPE]){
        panic("%s: lgType missing", logContextName);
        return;
    }
    if (xmlconfig[XML_LG_ADMSTATE] == NULL) {
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_LG_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_LG_ADMSTATE], "ENABLE")) {
        systemState::unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_LG_ADMSTATE], "DISABLE")) {
        systemState::setOpStateByBitmap(OP_DISABLED);
    }
    else{
        panic("%s: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", logContextName, xmlconfig[XML_DECODER_ADMSTATE]);
        return;
    }
    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: Configuring lg %s with the follwing configuration" CR, logContextName, xmlconfig[XML_LG_SYSNAME]);
    LOG_INFO("%s: System name: %s" CR, logContextName, xmlconfig[XML_LG_SYSNAME]);
    LOG_INFO("%s: User name %s:" CR, logContextName, xmlconfig[XML_LG_USRNAME]);
    LOG_INFO("%s: Description: %s" CR, logContextName, xmlconfig[XML_LG_DESC]);
    LOG_INFO("%s: Type: %s" CR, logContextName, xmlconfig[XML_LG_TYPE]);
     if (xmlconfig[XML_LG_PROPERTY1])
        LOG_INFO("%s: lg-type specific \"property1\" provided with value %s, will be passed to the LG extention class object" CR, logContextName, xmlconfig[XML_LG_PROPERTY1]);
    if (xmlconfig[XML_LG_PROPERTY2])
        LOG_INFO("%s: lg-type specific \"property2\" provided with value %s, will be passed to the LG extention class object" CR, logContextName, xmlconfig[XML_LG_PROPERTY2]);
    if (xmlconfig[XML_LG_PROPERTY3])
        LOG_INFO("%s: lg-type specific \"property3\" provided with value %s, will be passed to the LG extention class object" CR, logContextName, xmlconfig[XML_LG_PROPERTY3]);
    LOG_INFO("%s: lg admin state: %s" CR, logContextName, xmlconfig[XML_LG_ADMSTATE]);
    //CONFIFIGURING STEM OBJECT WITH PROPERTIES
    if (!strcmp((const char*)xmlconfig[XML_LG_TYPE], "SIGNAL MAST")) {
            LOG_INFO("%s: lg-type is Signal mast (sm)- programing act-stem object by creating an lgSignalMast extention class object" CR, logContextName);
            extentionLgClassObj = (void*) new (heap_caps_malloc(sizeof(lgSignalMast), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lgSignalMast(this);
        }
    else{
        panic("lg-type not supported" CR);
        return;
    }
	if (!extentionLgClassObj) {
		panic("%s: Failed to create lg extention object", logContextName);
		return;
	}
    LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], init());
    if (EXT_RC){
        panic("%s: Failed to initialize lg extention object", logContextName);
        return;
    }
    if (xmlconfig[XML_LG_PROPERTY1] || xmlconfig[XML_LG_PROPERTY2] || xmlconfig[XML_LG_PROPERTY3]) {
        tinyxml2::XMLDocument propertiesXmlDoc;                 //TEMPORARY FIX, the XML API shall eventually be fixed
        tinyxml2::XMLElement* propertiesRoot = propertiesXmlDoc.NewElement("Properties");
        tinyxml2::XMLElement* property1 = propertiesXmlDoc.NewElement("Property1");
        property1->SetText(xmlconfig[XML_LG_PROPERTY1]);
        propertiesRoot->InsertEndChild(property1);
        tinyxml2::XMLElement* property2 = propertiesXmlDoc.NewElement("Property2");
        property2->SetText(xmlconfig[XML_LG_PROPERTY2]);
        propertiesRoot->InsertEndChild(property2);
        tinyxml2::XMLElement* property3 = propertiesXmlDoc.NewElement("Property3");
        property3->SetText(xmlconfig[XML_LG_PROPERTY3]);
        propertiesRoot->InsertEndChild(property3);
        LOG_INFO("%s: Configuring the lgBase stem-object - Lg Adress: %d, Lg Link %d, Lg System Name %s: with properties" CR, logContextName, lgAddress, lgLinkNo, xmlconfig[XML_LG_SYSNAME]);
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onConfig(propertiesRoot));
    }
    else{
        panic("%s: No properties provided for base stem-object", logContextName);
        return;
    }
    systemState::unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("Configuration successfully finished" CR);
}

rc_t lgBase::start(void) {
    LOG_INFO("%s Starting lg" CR, logContextName);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_INFO("%s: lg not configured - will not start it" CR, logContextName);
        systemState::setOpStateByBitmap(OP_UNUSED);
        systemState::unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    systemState::unSetOpStateByBitmap(OP_UNUSED);
    LOG_INFO("%s: - starting lg extention object" CR, logContextName);
    if (extentionLgClassObj) {
        LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], start());
        if (EXT_RC) {
            panic("%s Failed to start lg extention object" CR, logContextName);
            return EXT_RC;
        }
	}
	else {
		panic("%s: No extention object to start" CR, logContextName);
		return RC_NOT_CONFIGURED_ERR;
	}
    LOG_INFO("%s: Subscribing to adm- and op state topics" CR, logContextName);
    char admopSubscribeTopic[300];
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LG_ADMSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LG_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, onAdmStateChangeHelper, this)){
        panic("%s: Failed to suscribe to admState topic: \"%s\"", logContextName, admopSubscribeTopic);
        return RC_GEN_ERR;
    }
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LG_OPSTATE_DOWNSTREAM_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LG_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, onOpStateChangeHelper, this)) {
        panic("%s: Failed to suscribe to opState topic: \"%s\"", logContextName, admopSubscribeTopic);
        return RC_GEN_ERR;
    }
    systemState::unSetOpStateByBitmap(OP_INIT);
    return RC_OK;
}

void lgBase::onSysStateChangeHelper(const void* p_lgBaseHandle, sysState_t p_sysState) {
    ((lgBase*)p_lgBaseHandle)->onSysStateChange(p_sysState);
}

void lgBase::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: lg has a new OP-state: %s, Queueing it for processing" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange)
        return;
    LOG_INFO("%s: lg has a new OP-state: %s" CR, logContextName, systemState::getOpStateStr(opStateStr));
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_LG_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", xmlconfig[XML_LG_SYSNAME]);
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if (newSysState & OP_INTFAIL) {
        if (extentionLgClassObj)
            LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onSysStateChange(newSysState));
        panic("%s: lg has experienced an internal error", logContextName);
        prevSysState = newSysState;
        return;
    }
    if (newSysState & OP_INIT) {
        LOG_TERSE("%s: lg is initializing - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_UNUSED) {
        LOG_TERSE("%s: lg is unused - informing server if not already done" CR, logContextName);
    }
    if (newSysState & OP_DISABLED) {
        LOG_TERSE("%s: lg is disabled by server - doing nothing" CR, logContextName);
    }
    if (newSysState & OP_CBL) {
        LOG_TERSE("%s: lg is control blocked by decoder - doing nothing" CR, logContextName);
    }
    if (newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL)) {
        LOG_INFO("%s: lg has following additional failures in addition to what has been reported above (if any): %s - informing server if not already done" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState & ~(OP_INTFAIL | OP_INIT | OP_UNUSED | OP_DISABLED | OP_CBL), opStateStr));
    }
    if (extentionLgClassObj && !(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onSysStateChange(newSysState));
    prevSysState = newSysState;
}

void lgBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onOpStateChange(p_topic, p_payload);
}

void lgBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: lg got a new opState from server: %s" CR, logContextName, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void lgBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onAdmStateChange(p_topic, p_payload);
}

void lgBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        systemState::unSetOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: lg got online message from server %s" CR, logContextName, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        systemState::setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: lg got off-line message from server %s" CR, logContextName, p_payload);
    }
    else
        LOG_ERROR("%s: lf got an invalid admstate message from server %s - doing nothing" CR, logContextName, p_payload);
}

rc_t lgBase::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t lgBase::setSystemName(const char* p_systemName, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set System name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set System name as light group is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_ERROR("%s: Cannot set System name - not suppoted" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

rc_t lgBase::getSystemName(char* p_systemName, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_systemName, xmlconfig[XML_LG_SYSNAME]);
    return RC_OK;
}

rc_t lgBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set User name as light group is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting User name to %s" CR, logContextName, p_usrName);
        delete (char*)xmlconfig[XML_LG_USRNAME];
        xmlconfig[XML_LG_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

rc_t lgBase::getUsrName(char* p_userName, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_userName, xmlconfig[XML_LG_USRNAME]);
    return RC_OK;
}

rc_t lgBase::setDesc(const char* p_description, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Description as light group is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LG_DESC];
        xmlconfig[XML_LG_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t lgBase::getDesc(char* p_desc, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_desc, xmlconfig[XML_LG_DESC]);
    return RC_OK;
}

rc_t lgBase::setAddress(uint8_t p_address, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set Address as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Address as light group is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_ERROR("%s: Cannot set Address - not supported" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

rc_t lgBase::getAddress(uint8_t* p_address, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Address as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    *p_address = lgAddress;
    return RC_OK;
}

rc_t lgBase::setNoOffLeds(uint8_t p_noOfLeds, bool p_force){
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set number of leds as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set number of leds as light group is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_ERROR("%s: Cannot set number of leds - not supported" CR, logContextName);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

rc_t lgBase::getNoOffLeds(uint8_t* p_noOfLeds, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get lg number of Leds as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (extentionLgClassObj) {
        LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getNoOffLeds(p_noOfLeds, p_force));
        return EXT_RC;
    }
    else{
		LOG_WARN("%s: LG Extention class has not been configured/does not exist, cannot get number of leds" CR, logContextName);
		return RC_NOT_CONFIGURED_ERR;
    }
}

rc_t lgBase::setProperty(uint8_t p_propertyId, const char* p_propertyValue, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s Cannot set lg property as debug-flag is inactive, use \"set debug\" to activate debug" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set lg property as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (p_propertyId < 1 || p_propertyId > 3) {
        LOG_WARN("%s: Property id %i is out of range" CR, logContextName, p_propertyId);
        return RC_NOT_FOUND_ERR;
    }

    if (extentionLgClassObj) {
        LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], setProperty(p_propertyId, p_propertyValue));
        if (EXT_RC){
            LOG_WARN("%s: Could not set Property id %i, return code: %i" CR, logContextName, p_propertyId, EXT_RC);
            return EXT_RC;
        }
        else {
            strcpy(xmlconfig[XML_LG_PROPERTY1 + p_propertyId - 1], p_propertyValue);
            return RC_OK;
        }
    }
    else {
        LOG_WARN("%s: LG Extention class has not been configured/does not exist, cannot set property id %i" CR, logContextName, p_propertyId);
        return RC_NOT_CONFIGURED_ERR;
    }
}

rc_t lgBase::getProperty(uint8_t p_propertyId, char* p_propertyValue, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s Cannot get port as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
	if (p_propertyId < 1 || p_propertyId > 3) {
		LOG_WARN("%s: Property id %i is out of range" CR, logContextName, p_propertyId);
		return RC_NOT_FOUND_ERR;
	}
	if (!xmlconfig[XML_LG_PROPERTY1 + p_propertyId - 1]) {
		LOG_WARN("%s: Property %i is not set" CR, logContextName, p_propertyId);
		return RC_NOT_FOUND_ERR;
	}
    strcpy(p_propertyValue, xmlconfig[XML_LG_PROPERTY1 + p_propertyId - 1]);
    return RC_OK;
}

rc_t lgBase::setShowing(const char* p_showing, bool p_force) {
    if (!debug && !p_force) {
        LOG_ERROR("%s: Cannot set showing as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set showing as lg is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    if(extentionLgClassObj){
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], setShowing(p_showing));
        return RC_OK;
    }
    else {
		LOG_ERROR("%s: LG Extention class has not been configured/does not exist, cannot set showing" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
}

rc_t lgBase::getShowing(char* p_showing, bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get showing as lg is not configured" CR, logContextName);
        strcpy(p_showing, "-");
        return RC_NOT_CONFIGURED_ERR;
    }
    if(extentionLgClassObj){
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getShowing(p_showing));
        return RC_OK;
    }
	LOG_ERROR("%s: LG Extention class has not been configured/does not exist, cannot get showing" CR, logContextName);
    strcpy(p_showing, "-");
    return RC_NOT_CONFIGURED_ERR;
}

const char* lgBase::getLogLevel(void) {
    if (!Log.transformLogLevelInt2XmlStr(Log.getLogLevel())) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
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

const char* lgBase::getLogContextName(void) {
    return logContextName;
}

/* CLI decoration methods*/
void lgBase::onCliGetAddressHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    uint8_t address;
    if (rc = static_cast<lgBase*>(p_cliContext)->getAddress(&address)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg address, return code: %i", rc);
        return;
    }
    printCli("%i", address);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgBase::onCliSetAddressHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<lgBase*>(p_cliContext)->setAddress(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg address, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgBase::onCliGetLedCntHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    uint8_t cnt;
    if (rc = static_cast<lgBase*>(p_cliContext)->getNoOffLeds(&cnt)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg Led-cnt, return code: %i", rc);
        return;
    }
    printCli("%i", cnt);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgBase::onCliSetLedCntHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<lgBase*>(p_cliContext)->setNoOffLeds(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg Led-cnt, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgBase::onCliGetLedOffsetHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    printCli("%i", static_cast<lgBase*>(p_cliContext)->getStripOffset());
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgBase::onCliSetLedOffsetHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<lgBase*>(p_cliContext)->setStripOffset(atoi(cmd.getArgument(1).getValue().c_str()));
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgBase::onCliGetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    char property[50];
    if (cmd.getArgument(1)) {
        if (rc = static_cast<lgBase*>(p_cliContext)->getProperty(atoi(cmd.getArgument(1).getValue().c_str()), property)) {
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg property, return code: %i", rc);
            return;
        }
        printCli("Lightgroup property %i: %s", atoi(cmd.getArgument(1).getValue().c_str()), property);
        acceptedCliCommand(CLI_TERM_QUIET);
    }
    else {
        printCli("| %*s | %*s |", -26, "Lightgroup property index:", -30, "Lightgroup property value:");
        for (uint8_t i = 1; i < 255; i++) {
            if (rc = static_cast<lgBase*>(p_cliContext)->getProperty(i, property)) {
                if (rc == RC_NOT_FOUND_ERR)
                    break;
                notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg property %i, return code: %i", i, rc);
                return;
            }
            printCli("| %*i | %*s |", -26, i, -30, property);
        }
        printCli("END");
        acceptedCliCommand(CLI_TERM_QUIET);
    }
}

void lgBase::onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || !cmd.getArgument(2) || cmd.getArgument(3)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<lgBase*>(p_cliContext)->setProperty(atoi(cmd.getArgument(1).getValue().c_str()), cmd.getArgument(2).getValue().c_str())) {
        if (rc == RC_NOT_CONFIGURED_ERR)
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg property %i, lg not configured", atoi(cmd.getArgument(1).getValue().c_str()));
        else if (rc == RC_DEBUG_NOT_SET_ERR)
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg property %i, debug-flag not set, use\"set debug\"", atoi(cmd.getArgument(1).getValue().c_str()));
        else
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not set lg property %i, return code: %i", atoi(cmd.getArgument(1).getValue().c_str()), rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void lgBase::onCliGetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    char showing[50];
    static_cast<lgBase*>(p_cliContext)->getShowing(showing);
    printCli("%s", showing);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgBase::onCliSetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    rc = static_cast<lgBase*>(p_cliContext)->setShowing(cmd.getArgument(1).getValue().c_str());
    switch (rc){
    case RC_DEBUG_NOT_SET_ERR:
        notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set, see: \"set debug\"");
        return;
        break;
    case RC_NOT_CONFIGURED_ERR:
        notAcceptedCliCommand(CLI_GEN_ERR, "Lightgroup not configured");
        return;
        break;
    case RC_OK:
        acceptedCliCommand(CLI_TERM_ORDERED);
        return;
        break;
    }
    notAcceptedCliCommand(CLI_GEN_ERR, "A unknown error occurred");
}

/*==============================================================================================================================================*/
/* END Class lgBase                                                                                                                             */
/*==============================================================================================================================================*/
