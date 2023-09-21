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
#include "actBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actBase (Actor base/Stem-cell class)"                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

actBase::actBase(uint8_t p_actPort, sat* p_satHandle) : systemState(p_satHandle), globalCli(ACTUATOR_MO_NAME, ACTUATOR_MO_NAME, p_actPort, p_satHandle) {
    asprintf(&logContextName, "%s/%s-%i", p_satHandle->getLogContextName(), "actBase", p_actPort);
    LOG_INFO("%s: Creating actBase stem-object" CR, p_actPort, logContextName);
    satHandle = p_satHandle;
    actPort = p_actPort;
    satAddr = satHandle->getAddr();
    satLinkNo = satHandle->linkHandle->getLink();
    char sysStateObjName[20];
    sprintf(sysStateObjName, "act-%d", p_actPort);
    setSysStateObjName(sysStateObjName);
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNUSED);
    regSysStateCb(this, &onSysStateChangeHelper);
    if (!(actLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    xmlconfig[XML_ACT_SYSNAME] = NULL;
    xmlconfig[XML_ACT_USRNAME] = NULL;
    xmlconfig[XML_ACT_DESC] = NULL;
    xmlconfig[XML_ACT_PORT] = NULL;
    xmlconfig[XML_ACT_TYPE] = NULL;
    xmlconfig[XML_ACT_SUBTYPE] = NULL;
    xmlconfig[XML_ACT_ADMSTATE] = NULL;
    //xmlconfig[XML_ACT_PROPERTIES] = NULL;
    extentionActClassObj = NULL;
    satLibHandle = NULL;
    debug = false;
}

actBase::~actBase(void) {
    panic("%s: actBase destructior not supported", logContextName);
}

rc_t actBase::init(void) {
    LOG_INFO("%s Initializing actBase stem-object" CR, logContextName);
    /* CLI decoration methods */
    LOG_INFO("%s Registering actBase specific CLI methods" CR, logContextName);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set port
    regCmdMoArg(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPORT_SUB_MO_NAME, onCliGetPortHelper);
    regCmdHelp(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPORT_SUB_MO_NAME, ACT_GET_ACTPORT_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPORT_SUB_MO_NAME, onCliSetPortHelper);
    regCmdHelp(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPORT_SUB_MO_NAME, ACT_SET_ACTPORT_HELP_TXT);

    // get/set showing
    regCmdMoArg(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORSHOWING_SUB_MO_NAME, onCliGetShowingHelper);
    regCmdHelp(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORSHOWING_SUB_MO_NAME, ACT_GET_ACTSHOWING_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORSHOWING_SUB_MO_NAME, onCliSetShowingHelper);
    regCmdHelp(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORSHOWING_SUB_MO_NAME, ACT_SET_ACTSHOWING_HELP_TXT);

    // get/set property
    regCmdMoArg(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPROPERTY_SUB_MO_NAME, onCliGetPropertyHelper);
    regCmdHelp(GET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPROPERTY_SUB_MO_NAME, ACT_GET_ACTPROPERTY_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPROPERTY_SUB_MO_NAME, onCliSetPropertyHelper);
    regCmdHelp(SET_CLI_CMD, ACTUATOR_MO_NAME, ACTUATORPROPERTY_SUB_MO_NAME, ACT_SET_ACTPROPERTY_HELP_TXT);
    return RC_OK;
}

void actBase::onConfig(const tinyxml2::XMLElement* p_actXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)){
        panic("%s Received a configuration, while the it was already configured, dynamic re-configuration not supported", logContextName);
        return;
    }
    LOG_INFO("%s: Received an uverified configuration, parsing and validating it..." CR, logContextName);

    //PARSING CONFIGURATION
    const char* actSearchTags[7];
    actSearchTags[XML_ACT_SYSNAME] = "JMRISystemName";
    actSearchTags[XML_ACT_USRNAME] = "JMRIUserName";
    actSearchTags[XML_ACT_DESC] = "JMRIDescription";
    actSearchTags[XML_ACT_PORT] = "Port";
    actSearchTags[XML_ACT_TYPE] = "Type";
    actSearchTags[XML_ACT_SUBTYPE] = "SubType";
    actSearchTags[XML_ACT_ADMSTATE] = "AdminState";
    getTagTxt(p_actXmlElement->FirstChildElement(), actSearchTags, xmlconfig, sizeof(actSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_ACT_SYSNAME]) {
        panic("%s: SystemNane missing", logContextName);
        return;
    }
    if (!xmlconfig[XML_ACT_USRNAME]) {
        LOG_WARN("%s: User name was not provided - using \"%s-UserName\"" CR, logContextName, xmlconfig[XML_ACT_SYSNAME]);
        xmlconfig[XML_ACT_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_ACT_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_ACT_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_ACT_USRNAME], "%s-Username", xmlconfig[XML_ACT_SYSNAME]);
        return;
    }
    if (!xmlconfig[XML_ACT_DESC]) {
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_ACT_DESC] = createNcpystr("-");
        return;
    }
    if (!xmlconfig[XML_ACT_PORT]) {
        panic("%s: Port missing", logContextName);
        return;
    }
    if (!xmlconfig[XML_ACT_TYPE]) {
        panic("%s: Type missing", logContextName);
        return;
    }
    if (atoi((const char*)xmlconfig[XML_ACT_PORT]) != actPort) {
        panic("%s: Port No inconsistant", logContextName);
        return;
    }
    if (xmlconfig[XML_ACT_ADMSTATE] == NULL) {
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_ACT_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_ACT_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_ACT_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else{
        panic("%s: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", logContextName, xmlconfig[XML_ACT_ADMSTATE]);
        return;
    }

    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: System name: %s" CR, logContextName, xmlconfig[XML_ACT_SYSNAME]);
    LOG_INFO("%s: User name: %s" CR, logContextName, xmlconfig[XML_ACT_USRNAME]);
    LOG_INFO("%s: Description: %s" CR, logContextName, xmlconfig[XML_ACT_DESC]);
    LOG_INFO("%s: Port: %s" CR, logContextName, xmlconfig[XML_ACT_PORT]);
    LOG_INFO("%s: Type: %s" CR, logContextName, xmlconfig[XML_ACT_TYPE]);
    LOG_INFO("%s: act admin state: %s" CR, logContextName, xmlconfig[XML_ACT_ADMSTATE]);

    //CONFIFIGURING ACTUATORS
    //if (xmlconfig[XML_ACT_PROPERTIES])
    //    LOG_INFO("actBase::onConfig: Actuator type specific properties provided, will be passed to the actuator type sub-class object: %s" CR, xmlconfig[XML_ACT_PROPERTIES]);
    if (!strcmp((const char*)xmlconfig[XML_ACT_TYPE], "TURNOUT")) {
        LOG_INFO("%s: actuator type is turnout - programing act-stem object by creating an turnAct extention class object" CR, logContextName);
            extentionActClassObj = (void*) new (heap_caps_malloc(sizeof(actTurn), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) actTurn(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
        }
    else if (!strcmp((const char*)xmlconfig[XML_ACT_TYPE], "LIGHT")) {
        LOG_INFO("%s: actuator type is light - programing act-stem object by creating an lightAct extention class object" CR, logContextName);
        extentionActClassObj = (void*) new (heap_caps_malloc(sizeof(actLight), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) actLight(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
    }
    else if (!strcmp((const char*)xmlconfig[XML_ACT_TYPE], "MEMORY")) {
        LOG_INFO("%s: actuator type is memory - programing act-stem object by creating an memAct extention class object" CR, logContextName);
        extentionActClassObj = (void*) new (heap_caps_malloc(sizeof(actMem), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) actMem(this, xmlconfig[XML_ACT_TYPE], xmlconfig[XML_ACT_SUBTYPE]);
    }
    else{
        panic("%s: actuator type not supported", logContextName);
        return;
    }
    ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], init());
    //if (xmlconfig[XML_ACT_PROPERTIES]) {
    //    LOG_INFO("Configuring the actuator base stem-object with properties" CR);
    //    ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], onConfig(p_actXmlElement->FirstChildElement("Properties")));
    //}
    //else
    //    LOG_INFO("No properties provided for base stem-object" CR);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("%s: Configuration successfully finished" CR, logContextName);
}

rc_t actBase::start(void) {
    LOG_INFO("%s: Starting actuator" CR, logContextName);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_INFO("%s: actuator not configured - will not start it" CR, logContextName);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED) {
        LOG_INFO("%s: actuator  not yet discovered - starting it anyway" CR, logContextName);
    }
    LOG_INFO("%s: Starting extention class" CR, logContextName);
    ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], start());
    LOG_INFO("%s: Subscribing to adm- and op state topics" CR, logContextName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_ACT_ADMSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onAdmStateChangeHelper, this)){
        panic("%s: Failed to suscribe to admState topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_ACT_OPSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (mqtt::subscribeTopic(subscribeTopic, onOpStateChangeHelper, this)){
        panic("%s: Failed to suscribe to opState topic \"%s\"", logContextName, subscribeTopic);
        return RC_GEN_ERR;
    }
    //wdt::wdtRegActuatorFailsafe(wdtKickedHelper, this);
    unSetOpStateByBitmap(OP_INIT);
    LOG_INFO("%s: actuator has started" CR, logContextName);
    return RC_OK;
}

void actBase::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    LOG_INFO("%s: actuatorcdiscovered" CR, logContextName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        systemState::unSetOpStateByBitmap(OP_UNDISCOVERED);
    }
    else{
        satLibHandle = NULL;
        systemState::setOpStateByBitmap(OP_UNDISCOVERED);
    }
    if (!(getOpStateBitmap() & OP_UNCONFIGURED))
        ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], onDiscovered(p_sateliteLibHandle, p_exists));
    else 
        LOG_WARN("%s: actuator discovered, but was not configured" CR, logContextName);
}

void actBase::onSysStateChangeHelper(const void* p_actBaseHandle, uint16_t p_sysState) {
    ((actBase*)p_actBaseHandle)->onSysStateChange(p_sysState);
}

void actBase::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: actuator has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange)
        return;
    failsafe(newSysState != OP_WORKING);
    LOG_INFO("%s: actuator has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState, opStateStr));
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_ACT_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if ((newSysState & OP_INTFAIL)) {
        prevSysState = newSysState;
        panic("%s: actuator has experienced an internal error - informing server" CR, logContextName);
        return;
    }
    prevSysState = newSysState;
}

void actBase::failsafe(bool p_failsafe) {
    char OPSTATE[100];
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], failsafe(p_failsafe));
}

void actBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_actHandle) {
    ((actBase*)p_actHandle)->onOpStateChange(p_topic, p_payload);
}

void actBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: Got a new opState from server: %s" CR, logContextName, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void actBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_actHandle) {
    ((actBase*)p_actHandle)->onAdmStateChange(p_topic, p_payload);
}

void actBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: actuator got online message from server %s" CR, logContextName, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: actuator got off-line message from server %s" CR, actPort, satAddr, satLinkNo, p_payload);
    }
    else
        LOG_ERROR("%s: actuator got an invalid admstate message from server %s - doing nothing" CR, logContextName, p_payload);
}

void actBase::wdtKickedHelper(void* p_actuatorBaseHandle) {
    ((actBase*)p_actuatorBaseHandle)->wdtKicked();
}

void actBase::wdtKicked(void) {
    setOpStateByBitmap(OP_INTFAIL);
}

rc_t actBase::setSystemName(const char* p_systemName, bool p_force) {
    LOG_ERROR("%s: Cannot set System name - not suppoted" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* actBase::getSystemName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as actuator is not configured" CR, logContextName);
        return NULL;
    }
    return (const char*)xmlconfig[XML_ACT_SYSNAME];
}

rc_t actBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {                               //LETS MOVE THESE CHECKS TO GLOBALCLI
        LOG_ERROR("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set System name as actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting User name to %s" CR, logContextName, p_usrName);
        delete (char*)xmlconfig[XML_ACT_USRNAME];
        xmlconfig[XML_ACT_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* actBase::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as actuator is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_ACT_USRNAME];
}

rc_t actBase::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Description as actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting Description to %s" CR, logContextName, p_description);
        delete xmlconfig[XML_ACT_DESC];
        xmlconfig[XML_ACT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* actBase::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as actuator is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_ACT_DESC];
}

rc_t actBase::setPort(uint8_t p_port) {
    LOG_ERROR("%s: Cannot set port - not supported" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

int8_t actBase::getPort(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Port as actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    return actPort;
}

rc_t actBase::setProperty(uint8_t p_propertyId, const char* p_propertyVal, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Actuator property as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Actuator property as Actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], setProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t actBase::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot get Actuator property as Actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    ACT_CALL_EXT(extentionActClassObj, xmlconfig[XML_ACT_TYPE], getProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t actBase::getShowing(char* p_showing, char* p_orderedShowing) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot get Actuator showing as Actuator is not configured" CR, logContextName);
        p_showing = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    ACT_CALL_EXT_RC(extentionActClassObj, xmlconfig[XML_ACT_TYPE], getShowing(p_showing, p_orderedShowing));
    return EXT_RC;

}

rc_t actBase::setShowing(const char* p_showing, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Actuator showing as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Actuator showing as Actuator is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    ACT_CALL_EXT_RC(extentionActClassObj, xmlconfig[XML_ACT_TYPE], setShowing(p_showing));
    return EXT_RC;
}

const char* actBase::getLogLevel(void) {
    if (!Log.transformLogLevelInt2XmlStr(Log.getLogLevel())) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
}

void actBase::setDebug(bool p_debug) {
    debug = p_debug;
}

bool actBase::getDebug(void) {
    return debug;
}

const char* actBase::getLogContextName(void) {
    return logContextName;
}

/* CLI decoration methods */
void actBase::onCliGetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    uint8_t port;
    if ((port = static_cast<actBase*>(p_cliContext)->getPort()) < 0) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor port, return code: %i", port);
        return;
    }
    printCli("Actuator port: %i", port);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void actBase::onCliSetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if ((rc = static_cast<actBase*>(p_cliContext)->setPort(atoi(cmd.getArgument(1).getValue().c_str())))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Actuator port, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void actBase::onCliGetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    char* showing = NULL;
    char* orderedShowing = NULL;
    if ((rc = static_cast<actBase*>(p_cliContext)->getShowing(showing, orderedShowing))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Actuator showing, return code: %i", rc);
        return;
    }
    printCli("showing: %s, ordered-showing: %s", showing, orderedShowing);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void actBase::onCliSetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if ((rc = static_cast<actBase*>(p_cliContext)->setShowing(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Actuator showing, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void actBase::onCliGetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    char* property = NULL;
    if (cmd.getArgument(1)) {
        if ((rc = static_cast<actBase*>(p_cliContext)->getProperty(atoi(cmd.getArgument(1).getValue().c_str()), property))) {
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor properties, return code: %i", rc);
            return;
        }
        printCli("Actuator property %i: %s", atoi(cmd.getArgument(1).getValue().c_str()), property);
        acceptedCliCommand(CLI_TERM_QUIET);
    }
    else {
        printCli("Actuator property index:\t\t\Actuator property value:\n");
        for (uint8_t i = 0; i < 255; i++) {
            if ((rc = static_cast<actBase*>(p_cliContext)->getProperty(i, property))) {
                if (rc == RC_NOT_FOUND_ERR)
                    break;
                notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Actuator property %i, return code: %i", i, rc);
                return;
            }
            printCli("%i\t\t\t%s\n", i, property);
        }
        printCli("END");
        acceptedCliCommand(CLI_TERM_QUIET);
    }
}

void actBase::onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if ((rc = static_cast<actBase*>(p_cliContext)->setProperty(atoi(cmd.getArgument(1).getValue().c_str()), cmd.getArgument(2).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Actuator property %i, return code: %i", atoi(cmd.getArgument(1).getValue().c_str()), rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}
/*==============================================================================================================================================*/
/* END Class actBase                                                                                                                           */
/*==============================================================================================================================================*/
