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
#include "senseBase.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "senseBase (Sensor base/Stem-cell class)"                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

senseBase::senseBase(uint8_t p_sensPort, sat* p_satHandle) : systemState(p_satHandle), globalCli(SENSOR_MO_NAME, SENSOR_MO_NAME, p_sensPort, p_satHandle) {
    asprintf(&logContextName, "%s/%s-%i", p_satHandle->getLogContextName(), "senseBase", p_sensPort);
    LOG_INFO("%s: Creating senseBase stem-object" CR, logContextName);
    satHandle = p_satHandle;
    sensPort = p_sensPort;
    satLinkNo = satHandle->linkHandle->getLink();
    satAddr = satHandle->getAddr();
    char sysStateObjName[20];
    sprintf(sysStateObjName, "sens-%d", p_sensPort);
    setSysStateObjName(sysStateObjName);
    if (!(sensLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    prevSysState = OP_WORKING;
    setOpStateByBitmap(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNUSED);
    regSysStateCb(this, &onSystateChangeHelper);
    xmlconfig[XML_SENS_SYSNAME] = NULL;
    xmlconfig[XML_SENS_USRNAME] = NULL;
    xmlconfig[XML_SENS_DESC] = NULL;
    xmlconfig[XML_SENS_PORT] = NULL;
    xmlconfig[XML_SENS_TYPE] = NULL;
    xmlconfig[XML_SENS_ADMSTATE] = NULL;
    //xmlconfig[XML_SENS_PROPERTIES] = NULL;
    extentionSensClassObj = NULL;
    satLibHandle = NULL;
    debug = false;
}

senseBase::~senseBase(void) {
    panic("%s: senseBase destructor not supported", logContextName);
}

rc_t senseBase::init(void) {
    LOG_INFO("%s: Initializing senseBase stem-object" CR, logContextName);

    /* CLI decoration methods */
    LOG_INFO("%s: Registering senseBase specific CLI methods" CR, logContextName);
    //Global and common MO Commands
    regGlobalNCommonCliMOCmds();
    // get/set port
    regCmdMoArg(GET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, onCliGetPortHelper);
    regCmdHelp(GET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, SENS_GET_SENSPORT_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, onCliSetPortHelper);
    regCmdHelp(SET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, SENS_SET_SENSPORT_HELP_TXT);
    // get sensing
    regCmdMoArg(GET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, onCliGetSensingHelper);
    regCmdHelp(GET_CLI_CMD, SENSOR_MO_NAME, SENSPORT_SUB_MO_NAME, SENS_GET_SENSING_HELP_TXT);
    // get/set property
    regCmdMoArg(GET_CLI_CMD, SENSOR_MO_NAME, SENSORPROPERTY_SUB_MO_NAME, onCliGetPropertyHelper);
    regCmdHelp(GET_CLI_CMD, SENSOR_MO_NAME, SENSORPROPERTY_SUB_MO_NAME, SENS_GET_SENSORPROPERTY_HELP_TXT);
    regCmdMoArg(SET_CLI_CMD, SENSOR_MO_NAME, SENSORPROPERTY_SUB_MO_NAME, onCliSetPropertyHelper);
    regCmdHelp(SET_CLI_CMD, SENSOR_MO_NAME, SENSORPROPERTY_SUB_MO_NAME, SENS_SET_SENSORPROPERTY_HELP_TXT);
    LOG_INFO("%s: senseBase specific CLI methods reistered" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseBase::onConfig(const tinyxml2::XMLElement* p_sensXmlElement) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED)){
        panic("%s: Received a configuration, while the it was already configured, dynamic re-configuration not supported", logContextName);
        return;
    }
    LOG_INFO("%s: Received an uverified configuration, parsing and validating it..." CR, logContextName);

    //PARSING CONFIGURATION
    const char* sensSearchTags[6];
    sensSearchTags[XML_SENS_SYSNAME] = "JMRISystemName";
    sensSearchTags[XML_SENS_USRNAME] = "JMRIUserName";
    sensSearchTags[XML_SENS_DESC] = "JMRIDescription";
    sensSearchTags[XML_SENS_PORT] = "Port";
    sensSearchTags[XML_SENS_TYPE] = "Type";
    sensSearchTags[XML_SENS_ADMSTATE] = "AdminState";
    //sensSearchTags[XML_SENS_PROPERTIES] = "Properties";
    getTagTxt(p_sensXmlElement->FirstChildElement(), sensSearchTags, xmlconfig, sizeof(sensSearchTags) / 4); // Need to fix the addressing for portability

    //VALIDATING AND SETTING OF CONFIGURATION
    if (!xmlconfig[XML_SENS_SYSNAME]){
        panic("%s: JMRISystemName missing", logContextName);
        return;
    }
    if (!xmlconfig[XML_SENS_USRNAME]){
        LOG_WARN("%s: User name was not provided - using \"%s-UserName\"" CR, logContextName, xmlconfig[XML_SENS_SYSNAME]);
        xmlconfig[XML_SENS_USRNAME] = new (heap_caps_malloc(sizeof(char) * (strlen(xmlconfig[XML_SENS_SYSNAME]) + 15), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(xmlconfig[XML_SENS_SYSNAME]) + 15];
        sprintf(xmlconfig[XML_SENS_USRNAME], "%s-Username", xmlconfig[XML_SENS_SYSNAME]);
        return;
    }
    if (!xmlconfig[XML_SENS_DESC]){
        LOG_WARN("%s: Description was not provided - using \"-\"" CR, logContextName);
        xmlconfig[XML_SENS_DESC] = createNcpystr("-");
        return;
    }
    if (!xmlconfig[XML_SENS_PORT]){
        panic("%s: Port missing");
        return;
    }
    if (!xmlconfig[XML_SENS_TYPE]){
        panic("%s: Type missing", logContextName);
        return;
    }
    if (atoi((const char*)xmlconfig[XML_SENS_PORT]) != sensPort){
        panic("%s:Port No inconsistant", logContextName);
        return;
    }
    if (xmlconfig[XML_SENS_ADMSTATE] == NULL) {
        LOG_WARN("%s: Admin state not provided in the configuration, setting it to \"DISABLE\"" CR, logContextName);
        xmlconfig[XML_SENS_ADMSTATE] = createNcpystr("DISABLE");
    }
    if (!strcmp(xmlconfig[XML_SENS_ADMSTATE], "ENABLE")) {
        unSetOpStateByBitmap(OP_DISABLED);
    }
    else if (!strcmp(xmlconfig[XML_SENS_ADMSTATE], "DISABLE")) {
        setOpStateByBitmap(OP_DISABLED);
    }
    else{
        panic("%s: Admin state: %s is none of \"ENABLE\" or \"DISABLE\"", xmlconfig[XML_SENS_ADMSTATE], logContextName);
        return;
    }

    //SHOW FINAL CONFIGURATION
    LOG_INFO("%s: Sense system name: %s" CR, logContextName, xmlconfig[XML_SENS_SYSNAME]);
    LOG_INFO("%s: Sense user name: %s" CR, logContextName, xmlconfig[XML_SENS_USRNAME]);
    LOG_INFO("%s: Sense description: %s" CR, logContextName, xmlconfig[XML_SENS_DESC]);
    LOG_INFO("%s: Sense port: %s" CR, logContextName, xmlconfig[XML_SENS_PORT]);
    LOG_INFO("%s: Sense type: %s" CR, logContextName, xmlconfig[XML_SENS_TYPE]);
    LOG_INFO("%s: Sense admin state: %s" CR, logContextName, xmlconfig[XML_SENS_ADMSTATE]);

    //CONFIFIGURING SENSORS
    if (!strcmp((const char*)xmlconfig[XML_SENS_TYPE], "DIGITAL")) {
        LOG_INFO("%s: Sensor-type is digital - programing sens-stem object by creating a senseDigital extention class object" CR, logContextName);
        extentionSensClassObj = (void*) new (heap_caps_malloc(sizeof(senseDigital), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) senseDigital(this);
    }
    // else if (other sensor types) {...}
    else{
        panic("Sensor type not supported");
        return;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], init());
    //if (xmlconfig[XML_SENS_PROPERTIES]) {
    //    LOG_INFO("senseBase::onConfig: Configuring the sensor base stem-object with properties" CR);
    //    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onConfig(p_sensXmlElement->FirstChildElement("Properties")));
    //}
    //else
    //    LOG_INFO("senseBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpStateByBitmap(OP_UNCONFIGURED);
    LOG_INFO("%s: Configuration successfully finished" CR, logContextName);
}

rc_t senseBase::start(void) {
    LOG_INFO("%s: Starting sensor" CR, logContextName);
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_INFO("%s: Sensor port not configured - will not start it" CR, logContextName);
        setOpStateByBitmap(OP_UNUSED);
        unSetOpStateByBitmap(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    unSetOpStateByBitmap(OP_UNUSED);
    if (systemState::getOpStateBitmap() & OP_UNDISCOVERED) {
        LOG_INFO("%s: Sensor port not yet discovered - starting it anyway" CR, logContextName);
    }
    LOG_INFO("%s: Starting extention class" CR, logContextName);
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], start());
    LOG_INFO("%s: Subscribing to adm- and op state topics" CR, logContextName);
    rc_t rc;
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_SENS_ADMSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (rc = mqtt::subscribeTopic(subscribeTopic, onAdmStateChangeHelper, this)){
        panic("%s: Failed to suscribe to admState topic \"%s\", return code: %i", logContextName, subscribeTopic, rc);
        return RC_GEN_ERR;
    }
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_SENS_OPSTATE_DOWNSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
    if (rc = mqtt::subscribeTopic(subscribeTopic, onOpStateChangeHelper, this)){
        panic("%s: Failed to suscribe to opState topic \"%s\", return code: %i", logContextName, subscribeTopic, rc);
        return RC_GEN_ERR;
    }
    unSetOpStateByBitmap(OP_INIT);
    LOG_INFO("%s: Sensor port has started" CR, logContextName);
    return RC_OK;
}

void senseBase::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    LOG_INFO("%s: Sensor port discovered" CR, logContextName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        systemState::unSetOpStateByBitmap(OP_UNDISCOVERED);
    }
    else {
        satLibHandle = NULL;
        systemState::setOpStateByBitmap(OP_UNDISCOVERED);
    }
    if (!(getOpStateBitmap() & OP_UNCONFIGURED))
        SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onDiscovered(p_sateliteLibHandle, p_exists));
    else
        LOG_INFO("%s: Sensor port discovered, but was not configured" CR, logContextName);
}

void senseBase::onSenseChange(bool p_senseVal) {
    if (!getOpStateBitmap())
        SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onSenseChange(p_senseVal));
    else {
        char opStateStr[100];
        getOpStateStr(opStateStr);
        LOG_TERSE("%s: Sensor has changed to %i, but senseBase has opState: %s and is not OP_WORKING, doing nothing..." CR, logContextName, p_senseVal, opStateStr);
    }
}

void senseBase::onSystateChangeHelper(const void* p_senseBaseHandle, sysState_t p_sysState) {
    ((senseBase*)p_senseBaseHandle)->onSysStateChange(p_sysState);
}

void senseBase::onSysStateChange(sysState_t p_sysState) {
    char opStateStr[100];
    LOG_INFO("%s: sensBase has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(p_sysState, opStateStr));
    sysState_t newSysState;
    newSysState = p_sysState;
    sysState_t sysStateChange = newSysState ^ prevSysState;
    if (!sysStateChange) {
        return;
    }
    failsafe(newSysState != OP_WORKING);
    LOG_INFO("%s: sensBase has a new OP-state: %s" CR, logContextName, systemState::getOpStateStrByBitmap(newSysState, opStateStr));
    if ((sysStateChange & ~OP_CBL) && mqtt::getDecoderUri() && !(getOpStateBitmap() & OP_UNCONFIGURED)) {
        char publishTopic[200];
        char publishPayload[100];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SENS_OPSTATE_UPSTREAM_TOPIC, "/", mqtt::getDecoderUri(), "/", getSystemName());
        systemState::getOpStateStr(publishPayload);
        mqtt::sendMsg(publishTopic, getOpStateStrByBitmap(getOpStateBitmap() & ~OP_CBL, publishPayload), false);
    }
    if ((newSysState & OP_INTFAIL)) {
        prevSysState = newSysState;
        panic("%s: sensBase has experienced an internal error - informing server", logContextName);
        return;
    }
    prevSysState = newSysState;
}

void senseBase::failsafe(bool p_failsafe) {
    if (!(systemState::getOpStateBitmap() & OP_UNCONFIGURED))
        SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_ACT_TYPE], failsafe(p_failsafe));
}

void senseBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onOpStateChange(p_topic, p_payload);
}

void senseBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    sysState_t newServerOpState;
    LOG_INFO("%s: Got a new opState from server: %s" CR, logContextName, p_payload);
    newServerOpState = getOpStateBitmapByStr(p_payload);
    setOpStateByBitmap(newServerOpState & OP_CBL);
    unSetOpStateByBitmap(~newServerOpState & OP_CBL);
}

void senseBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onAdmStateChange(p_topic, p_payload);
}

void senseBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: senseBase got online message from server %s" CR, logContextName, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpStateByBitmap(OP_DISABLED);
        LOG_INFO("%s: senseBase got an off-line message from server %s" CR, logContextName, p_payload);
    }
    else
        LOG_ERROR("%s: senseBase got an invalid admstate message from server %s - doing nothing" CR, logContextName, p_payload);
}

void senseBase::wdtKickedHelper(void* senseBaseHandle) {
    ((senseBase*)senseBaseHandle)->wdtKicked();
}

void senseBase::wdtKicked(void) {
    setOpStateByBitmap(OP_INTFAIL);
}

rc_t senseBase::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t senseBase::setSystemName(char* p_sysName, bool p_force) {
    LOG_ERROR("Cannot set System name - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* senseBase::getSystemName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get System name as sensor is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SENS_SYSNAME];
}

rc_t senseBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set User name as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set System name as sensor is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting User name to %s" CR, logContextName, p_usrName);
        delete (char*)xmlconfig[XML_SENS_USRNAME];
        xmlconfig[XML_SENS_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* senseBase::getUsrName(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get User name as sensor is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SENS_USRNAME];
}

rc_t senseBase::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Description as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Description as sensor is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        LOG_INFO("%s: Setting Description to %s" CR, logContextName, p_description);
        delete xmlconfig[XML_SENS_DESC];
        xmlconfig[XML_SENS_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* senseBase::getDesc(bool p_force) {
    if ((systemState::getOpStateBitmap() & OP_UNCONFIGURED) && !p_force) {
        LOG_ERROR("%s: Cannot get Description as sensor is not configured" CR, logContextName);
        return NULL;
    }
    return xmlconfig[XML_SENS_DESC];
}

rc_t senseBase::setPort(const uint8_t p_port) {
    LOG_ERROR("%s: Cannot set port - not supported" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

uint8_t senseBase::getPort(void) {
    return sensPort;
}

rc_t senseBase::setProperty(uint8_t p_propertyId, const char* p_propertyVal, bool p_force){
    if (!debug || !p_force) {
        LOG_ERROR("%s: Cannot set Sensor property as debug is inactive" CR, logContextName);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Sensor property as sensor is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], setProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t senseBase::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot set Sensor property as sensor is not configured" CR, logContextName);
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], getProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t senseBase::getSensing(const char* p_sensing) {
    if (systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR("%s: Cannot get sensing as sensor is not configured" CR, logContextName);
        p_sensing = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], getSensing((char*)p_sensing));
    return RC_OK;
}

const char* senseBase::getLogLevel(void) {
    if (!Log.transformLogLevelInt2XmlStr(Log.getLogLevel())) {
        LOG_ERROR("%s: Could not retrieve a valid Log-level" CR, logContextName);
        return NULL;
    }
    else {
        return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
    }
}

void senseBase::setDebug(bool p_debug) {
    debug = p_debug;
}

bool senseBase::getDebug(void) {
    return debug;
}

const char* senseBase::getLogContextName(void) {
    return logContextName;
}

/* CLI decoration methods */
void senseBase::onCliGetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    uint8_t port;
    if ((port = static_cast<senseBase*>(p_cliContext)->getPort()) < 0) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor port, return code: %i", port);
        return;
    }
    printCli("Sensor port: %i", port);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void senseBase::onCliSetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<senseBase*>(p_cliContext)->setPort(atoi(cmd.getArgument(1).getValue().c_str()))) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Sensor port, return code: %i", rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}

void senseBase::onCliGetSensingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    char* sensing;
    if (rc = static_cast<senseBase*>(p_cliContext)->getSensing(sensing)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor sensing, return code: %i", rc);
        return;
    }
    printCli("Sensing: %s", sensing);
    acceptedCliCommand(CLI_TERM_QUIET);
}

void senseBase::onCliGetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    char* property;
    if (cmd.getArgument(1)) {
        if (rc = static_cast<senseBase*>(p_cliContext)->getProperty(atoi(cmd.getArgument(1).getValue().c_str()), property)) {
            notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor properties, return code: %i", rc);
            return;
        }
        printCli("Sensor property %i: %s", atoi(cmd.getArgument(1).getValue().c_str()), property);
        acceptedCliCommand(CLI_TERM_QUIET);
    }
    else {
        printCli("Sensor property index:\t\t\Sensor property value:\n");
        for (uint8_t i = 0; i < 255; i++) {
            if (rc = static_cast<senseBase*>(p_cliContext)->getProperty(i, property)) {
                if (rc == RC_NOT_FOUND_ERR)
                    break;
                notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor property %i, return code: %i", i, rc);
                return;
            }
            printCli("%i\t\t\t%s\n", i, property);
        }
        printCli("END");
        acceptedCliCommand(CLI_TERM_QUIET);
    }
}
void senseBase::onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
    Command cmd(p_cmd);
    if (!cmd.getArgument(1) || cmd.getArgument(2)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    if (rc = static_cast<senseBase*>(p_cliContext)->setProperty(atoi(cmd.getArgument(1).getValue().c_str()), cmd.getArgument(2).getValue().c_str())) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Sensor property %i, return code: %i", atoi(cmd.getArgument(1).getValue().c_str()), rc);
        return;
    }
    acceptedCliCommand(CLI_TERM_EXECUTED);
}
/*==============================================================================================================================================*/
/* END Class senseBase                                                                                                                           */
/*==============================================================================================================================================*/
