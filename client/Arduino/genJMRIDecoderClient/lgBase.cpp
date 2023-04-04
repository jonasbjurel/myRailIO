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

//lgBase::lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle) : systemState(this), globalCli(LG_MO_NAME, LG_MO_NAME, lgIndex) {
lgBase::lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle) : systemState(this) {
    lgLinkHandle = (lgLink*)p_lgLinkHandle;
    lgAddress = p_lgAddress;
    lgLinkHandle->getLink(&lgLinkNo);
    Log.INFO("lgBase::lgBase: Creating lgBase stem-object for lg address %d, on lgLink %d" CR, p_lgAddress, lgLinkNo);
    regSysStateCb((void*)this, &onSysStateChangeHelper);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    lgBaseLock = xSemaphoreCreateMutex();
    if (lgBaseLock == NULL)
        panic("lgBase::lgBase: Could not create Lock objects - rebooting..." CR);
    xmlconfig[XML_LG_SYSNAME] = NULL;
    xmlconfig[XML_LG_USRNAME] = NULL;
    xmlconfig[XML_LG_DESC] = NULL;
    xmlconfig[XML_LG_LINKADDR] = NULL;
    xmlconfig[XML_LG_TYPE] = NULL;
    xmlconfig[XML_LG_PROPERTY1] = NULL;
    xmlconfig[XML_LG_PROPERTY2] = NULL;
    xmlconfig[XML_LG_PROPERTY3] = NULL;
    debug = false;
    stripOffset = 0;

    //CLI decoration definitions
    // get/set address
/*
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
    */
}

lgBase::~lgBase(void) {
    panic("lgBase::~lgBase: lgBase destructior not supported - rebooting..." CR);
}

rc_t lgBase::init(void) {
    Log.INFO("lgBase::init: Initializing stem-object for lg-address %d, on lgLink %d" CR, lgAddress, lgLinkNo);
    return RC_OK;
}

void lgBase::onConfig(const tinyxml2::XMLElement* p_lgXmlElement) {
    if (!(systemState::getOpState() & OP_UNCONFIGURED))
        panic("lgBase:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported rebooting..." CR);
    Log.INFO("lgBase::onConfig: lg-address %d, on lgLink %d received an unverified configuration, parsing and validating it..." CR, lgAddress, lgLinkNo);

    //PARSING CONFIGURATION
    const char* lgSearchTags[8];
    lgSearchTags[XML_LG_SYSNAME] = "JMRISystemName";
    lgSearchTags[XML_LG_USRNAME] = "JMRIUserName";
    lgSearchTags[XML_LG_DESC] = "JMRIDescription";
    lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
    lgSearchTags[XML_LG_TYPE] = "Type";
    lgSearchTags[XML_LG_PROPERTY1] = "Property1";
    lgSearchTags[XML_LG_PROPERTY2] = "Property2";
    lgSearchTags[XML_LG_PROPERTY3] = "Property3";
    getTagTxt(p_lgXmlElement->FirstChildElement(), lgSearchTags, xmlconfig, sizeof(lgSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_LG_SYSNAME])
        panic("lgBase::onConfig: System Name missing for lg-address %d, on lgLink %d - rebooting..." CR, lgAddress, lgLinkNo);
    if (!xmlconfig[XML_LG_USRNAME]){
        Log.WARN("lgBase::onConfig: User name was not provided - using \"%s-UserName\"" CR);
        xmlconfig[XML_LG_USRNAME] = new char[strlen(xmlconfig[XML_LG_SYSNAME]) + 10];
        const char* usrName[2] = { xmlconfig[XML_LG_SYSNAME], "- UserName" };
        strcpy(xmlconfig[XML_LG_USRNAME], concatStr(usrName, 2));
    }
    if (!xmlconfig[XML_LG_DESC]) {
        Log.WARN("lgBase::onConfig: Description was not provided - using \"-\"" CR);
        xmlconfig[XML_LG_DESC] = new char[2];
        strcpy(xmlconfig[XML_LG_DESC], "-");
        }
    if (!xmlconfig[XML_LG_LINKADDR])
        panic("lgBase::onConfig: Link address missing for lg-Systemname: %, lg-address %d, on lgLink %d - rebooting..." CR, xmlconfig[XML_LG_SYSNAME], lgAddress, lgLinkNo);
    if (atoi((const char*)xmlconfig[XML_LG_LINKADDR]) != lgAddress)
        panic("lgBase::onConfig: provided lgLinkAddress inconsistant with the constructor provided address - rebooting..." CR);
    if (!xmlconfig[XML_LG_TYPE])
        panic("lgBase::onConfig: lgType missing for lg-Systemname: %, lg-address %d, on lgLink %d - rebooting..." CR, xmlconfig[XML_LG_SYSNAME], lgAddress, lgLinkNo);

    //SHOW FINAL CONFIGURATION
    Log.INFO("lgBase::onConfig: Configuring lg %s with the follwing configuration" CR, xmlconfig[XML_LG_SYSNAME]);
    Log.INFO("lgBase::onConfig: System name: %s" CR, xmlconfig[XML_LG_SYSNAME]);
    Log.INFO("lgBase::onConfig: User name %s:" CR, xmlconfig[XML_LG_USRNAME]);
    Log.INFO("lgBase::onConfig: Description: %s" CR, xmlconfig[XML_LG_DESC]);
    Log.INFO("lgBase::onConfig: Type: %s" CR, xmlconfig[XML_LG_TYPE]);
     if (xmlconfig[XML_LG_PROPERTY1])
        Log.INFO("lgBase::onConfig: lg type specific \"property1\" provided with value %s, will be passed to the LG extention class object" CR, xmlconfig[XML_LG_PROPERTY1]);
    if (xmlconfig[XML_LG_PROPERTY2])
        Log.INFO("lgBase::onConfig: lg type specific \"property2\" provided with value %s, will be passed to the LG extention class object" CR, xmlconfig[XML_LG_PROPERTY2]);
    if (xmlconfig[XML_LG_PROPERTY3])
        Log.INFO("lgBase::onConfig: lg type specific \"property3\" provided with value %s, will be passed to the LG extention class object" CR, xmlconfig[XML_LG_PROPERTY3]);

    //CONFIFIGURING STEM OBJECT WITH PROPERTIES
    if (!strcmp((const char*)xmlconfig[XML_LG_TYPE], "SIGNAL MAST")) {
            Log.INFO("lgBase::onConfig: lg-type is Signal mast - programing act-stem object by creating an lgSignalMast extention class object" CR);
            extentionLgClassObj = (void*) new lgSignalMast(this);
        }
    else
        panic("lgBase::onConfig: lg type not supported" CR);
    LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], init());
    if (EXT_RC)
        panic("lgBase::onConfig: Failed to initialize Lg extention object - Rebooting..." CR);
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
        Log.INFO("lgBase::onConfig: Configuring the lg base stem-object - Lg Adress: %d, Lg Link %d, Lg System Name %s: with properties" CR, lgAddress, lgLinkNo, xmlconfig[XML_LG_SYSNAME]);
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onConfig(propertiesRoot));
    }
    else
        Log.INFO("lgBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.INFO("lgBase::onConfig: Configuration successfully finished" CR);
}

rc_t lgBase::start(void) {
    Log.INFO("lgBase::start: Starting lg address %d on lgLink %d" CR, lgAddress, lgLinkNo);
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.INFO("lgBase::start: lg address %d on lgLink %d not configured - will not start it" CR, lgAddress, lgLinkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.INFO("lgBase::start: lg address %d on lgLink %d  - starting extention class" CR, lgAddress, lgLinkNo);
    LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], start());
    if (EXT_RC) {
        panic("lgBase::onConfig: Failed to start Light group" CR);
        return EXT_RC;
    }
    Log.INFO("lgBase::start: Subscribing to adm- and op state topics" CR);
    char admopSubscribeTopic[300];
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LG_ADMSTATE_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LG_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, onAdmStateChangeHelper, this))
        panic("lgBase::start: Failed to suscribe to admState topic - rebooting..." CR);
    sprintf(admopSubscribeTopic, "%s/%s/%s", MQTT_LG_OPSTATE_TOPIC, mqtt::getDecoderUri(), xmlconfig[XML_LG_SYSNAME]);
    if (mqtt::subscribeTopic(admopSubscribeTopic, onOpStateChangeHelper, this))
        panic("lgBase::start: Failed to suscribe to opState topic - rebooting..." CR);
    wdt::wdtRegLgFailsafe(wdtKickedHelper, this);
    return RC_OK;
}

void lgBase::onSysStateChangeHelper(const void* p_lgBaseHandle, uint16_t p_sysState) {
    ((lgBase*)p_lgBaseHandle)->onSysStateChange(p_sysState);
}

void lgBase::onSysStateChange(uint16_t p_sysState) {
    if (!(p_sysState & OP_UNCONFIGURED)){
        char opStateStr[100];
        if (p_sysState)
            Log.INFO("lgBase::onSysStateChange: lg address %d on lgLink %d has a new OP-state: %s" CR, lgAddress, lgLinkNo, systemState::getOpStateStr(opStateStr, p_sysState));
        else
            Log.INFO("lgBase::onSysStateChange: lg address %d on lgLink %d has a \"WORKING\" OP-state" CR, lgAddress, lgLinkNo);
        if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
            Log.INFO("lgBase::onSysStateChange: lg address %d on lgLink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, lgAddress, lgLinkNo);
        else if (p_sysState & OP_INTFAIL)
            panic("lgBase::onSysStateChange: lg has experienced an internal error - rebooting..." CR);
        LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], onSysStateChange(p_sysState));
    }
}

void lgBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onOpStateChange(p_topic, p_payload);
}

void lgBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.INFO("lgBase::onOpStateChange: lg address %d on lgLink %d got available message from server: %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.INFO("lgBase::onOpStateChange: lg address %d on lgLink %d got unavailable message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else
        Log.ERROR("lgBase::onOpStateChange: lg address %d on lgLink %d got an invalid availability message from server %s - doing nothing" CR, lgAddress, lgLinkNo, p_payload);
}

void lgBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgHandle) {
    ((lgBase*)p_lgHandle)->onAdmStateChange(p_topic, p_payload);
}

void lgBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.INFO("lgBase::onAdmStateChange: lg address %d on lgLink %d got online message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.INFO("lgBase::onAdmStateChange: lg address %d on lgLink %d got off-line message from server %s" CR, lgAddress, lgLinkNo, p_payload);
    }
    else
        Log.ERROR("lgBase::onAdmStateChange: lg address %d on lgLink %d, satLink %d got an invalid admstate message from server %s - doing nothing" CR, lgAddress, lgLinkNo, p_payload);
}

void lgBase::wdtKickedHelper(void* lgBaseHandle) {
    ((lgBase*)lgBaseHandle)->wdtKicked();
}

void lgBase::wdtKicked(void) {
    setOpState(OP_INTFAIL);
}

rc_t lgBase::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t lgBase::setSystemName(const char* p_systemName, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setSystemName: cannot set System name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    Log.ERROR("lgBase::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getSystemName(char* p_systemName, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getSystemName: cannot get System name as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_systemName, xmlconfig[XML_LG_SYSNAME]);
    return RC_OK;
}

rc_t lgBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    Log.INFO("lgBase::setUsrName: Setting User name to %s" CR, p_usrName);
    delete (char*)xmlconfig[XML_LG_USRNAME];
    xmlconfig[XML_LG_USRNAME] = createNcpystr(p_usrName);
    return RC_OK;
}

rc_t lgBase::getUsrName(char* p_userName, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getUsrName: cannot get User name as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_userName, xmlconfig[XML_LG_USRNAME]);
    return RC_OK;
}

rc_t lgBase::setDesc(const char* p_description, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
        Log.INFO("lgBase::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LG_DESC];
        xmlconfig[XML_LG_DESC] = createNcpystr(p_description);
        return RC_OK;
}

rc_t lgBase::getDesc(char* p_desc, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getDesc: cannot get Description as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_desc, xmlconfig[XML_LG_DESC]);
    return RC_OK;
}

rc_t lgBase::setAddress(uint8_t p_address, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setAddress: cannot set Address as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    Log.ERROR("lgBase::setAddress: cannot set Address - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getAddress(uint8_t* p_address, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getAddress: cannot get Description as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    *p_address = lgAddress;
    return RC_OK;
}

rc_t lgBase::setNoOffLeds(uint8_t p_noOfLeds, bool p_force){
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setNoOffLeds: cannot set number of leds as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    Log.ERROR("lgBase::NoOfLeds: cannot set number of leds - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t lgBase::getNoOffLeds(uint8_t* p_noOfLeds, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getNoOffLeds: cannot get lg number of Leds as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getNoOffLeds(p_noOfLeds, p_force));
    return EXT_RC;
}

rc_t lgBase::setProperty(uint8_t p_propertyId, const char* p_propertyValue, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setProperty: cannot set lg property as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("lgBase::setProperty: cannot set lg property as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    LG_CALL_EXT_RC(extentionLgClassObj, xmlconfig[XML_LG_TYPE], setProperty(p_propertyId, p_propertyValue));
    if (EXT_RC)
        return EXT_RC;
    else {
        strcpy(xmlconfig[XML_LG_PROPERTY1 + p_propertyId - 1], p_propertyValue);
    }
    return RC_OK;
}

rc_t lgBase::getProperty(uint8_t p_propertyId, char* p_propertyValue, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getProperty: cannot get port as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    strcpy(p_propertyValue, xmlconfig[XML_LG_PROPERTY1 + p_propertyId - 1]);
    return RC_OK;
}

rc_t lgBase::setShowing(const char* p_showing, bool p_force) {
    if (!debug && !p_force) {
        Log.ERROR("lgBase::setShowing: cannot set showing as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], setShowing(p_showing));
    return RC_OK;
}

rc_t lgBase::getShowing(char* p_showing, bool p_force) {
    if ((systemState::getOpState() & OP_UNCONFIGURED) && !p_force) {
        Log.ERROR("lgBase::getShowing: cannot get showing as lg is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    LG_CALL_EXT(extentionLgClassObj, xmlconfig[XML_LG_TYPE], getShowing(p_showing));
    return RC_OK;
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

/* CLI decoration methods*/
/*
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
    printCli("Lightgroup-address: %i", address);
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
    printCli("Lightgroup-Ledcnt: %i", cnt);
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
    printCli("Lightgroup-offset: %i", static_cast<lgBase*>(p_cliContext)->getStripOffset());
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
    char* property = NULL;
    if (cmd.getArgument(1)) {
        if (rc = static_cast<lgBase*>(p_cliContext)->getProperty(atoi(cmd.getArgument(1).getValue().c_str()), property)) {
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg property, return code: %i", rc);
            return;
        }
        printCli("Lightgroup property %i: %s", atoi(cmd.getArgument(1).getValue().c_str()), property);
        acceptedCliCommand(CLI_TERM_QUIET);
    }
    else {
        printCli("Lightgroup property index:\t\t\tLightgroup property value:\n");
        for (uint8_t i = 0; i < 255; i++) {
            if (rc = static_cast<lgBase*>(p_cliContext)->getProperty(i, property)) {
                if (rc == RC_NOT_FOUND_ERR)
                    break;
                notAcceptedCliCommand(CLI_GEN_ERR, "Could not get lg property %i, return code: %i", i, rc);
                return;
            }
            printCli("%i\t\t\t%s\n", i, property);
        }
        printCli("END");
        acceptedCliCommand(CLI_TERM_QUIET);
    }
}

void lgBase::onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<lgBase*>(p_cliContext)->setProperty(atoi(cmd.getArgument(1).getValue().c_str()), cmd.getArgument(2).getValue().c_str())) {
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
    char* showing = NULL;
    static_cast<lgBase*>(p_cliContext)->getShowing(showing);
    printCli("Lightgroup-showing: %s", showing);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void lgBase::onCliSetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    static_cast<lgBase*>(p_cliContext)->getShowing(cmd.getArgument(1).getValue().c_str());
    acceptedCliCommand(CLI_TERM_ORDERED);
}
*/

/*==============================================================================================================================================*/
/* END Class lgBase                                                                                                                             */
/*==============================================================================================================================================*/
