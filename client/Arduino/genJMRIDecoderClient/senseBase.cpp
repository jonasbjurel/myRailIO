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

uint16_t senseBase::sensIndex = 0;

senseBase::senseBase(uint8_t p_sensPort, sat* p_satHandle) : systemState(p_satHandle), globalCli(SENSOR_MO_NAME, SENSOR_MO_NAME, sensIndex) {
    satHandle = p_satHandle;
    sensPort = p_sensPort;
    satHandle->linkHandle->getLink(&satLinkNo);
    satHandle->getAddr(&satAddr);
    Log.INFO("senseBase::senseBase: Creating senseBase stem-object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    if (++sensIndex > MAX_SENS)
        panic("senseBase::senseBase:Number of configured sensors exceeds maximum configured by [MAX_SENS: %d] - rebooting ..." CR, MAX_SENS);
    char sysStateObjName[20];
    sprintf(sysStateObjName, "sens-%d", p_sensPort);
    setSysStateObjName(sysStateObjName);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    regSysStateCb(this, &onSystateChangeHelper);
    sensLock = xSemaphoreCreateMutex();
    pendingStart = false;
    satLibHandle = NULL;
    debug = false;
    if (sensLock == NULL)
        panic("senseBase::senseBase: Could not create Lock objects - rebooting...");

/* CLI decoration methods */
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
}

senseBase::~senseBase(void) {
    panic("senseBase::~senseBase: senseBase destructior not supported - rebooting...");
}

rc_t senseBase::init(void) {
    Log.INFO("senseBase::init: Initializing stem-object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseBase::onConfig(const tinyxml2::XMLElement* p_sensXmlElement) {
    if (!(systemState::getOpState() & OP_UNCONFIGURED))
        panic("senseBase:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    Log.INFO("senseBase::onConfig: sensor port %d, on satelite adress %d, satLink %d received an uverified configuration, parsing and validating it..." CR, sensPort, satAddr, satLinkNo);
    xmlconfig[XML_SENS_SYSNAME] = NULL;
    xmlconfig[XML_SENS_USRNAME] = NULL;
    xmlconfig[XML_SENS_DESC] = NULL;
    xmlconfig[XML_SENS_PORT] = NULL;
    xmlconfig[XML_SENS_TYPE] = NULL;
    xmlconfig[XML_SENS_PROPERTIES] = NULL;
    const char* sensSearchTags[6];
    sensSearchTags[XML_SENS_SYSNAME] = "SystemName";
    sensSearchTags[XML_SENS_USRNAME] = "UserName";
    sensSearchTags[XML_SENS_DESC] = "Description";
    sensSearchTags[XML_SENS_PORT] = "Port";
    sensSearchTags[XML_SENS_TYPE] = "Type";
    sensSearchTags[XML_SENS_PROPERTIES] = "Properties";
    getTagTxt(p_sensXmlElement, sensSearchTags, xmlconfig, sizeof(sensSearchTags) / 4); // Need to fix the addressing for portability
    if (!xmlconfig[XML_SENS_SYSNAME])
        panic("senseBase::onConfig: SystemNane missing - rebooting...");
    if (!xmlconfig[XML_SENS_USRNAME])
        panic("senseBase::onConfig: User name missing - rebooting...");
    if (!xmlconfig[XML_SENS_DESC])
        panic("senseBase::onConfig: Description missing - rebooting...");
    if (!xmlconfig[XML_SENS_PORT])
        panic("senseBase::onConfig: Port missing - rebooting...");
    if (!xmlconfig[XML_SENS_TYPE])
        panic("senseBase::onConfig: Type missing - rebooting...");
    if (atoi((const char*)xmlconfig[XML_SENS_PORT]) != sensPort)
        panic("senseBase::onConfig: Port No inconsistant - rebooting...");
    Log.INFO("senseBase::onConfig: System name: %s" CR, xmlconfig[XML_SENS_SYSNAME]);
    Log.INFO("senseBase::onConfig: User name:" CR, xmlconfig[XML_SENS_USRNAME]);
    Log.INFO("senseBase::onConfig: Description: %s" CR, xmlconfig[XML_SENS_DESC]);
    Log.INFO("senseBase::onConfig: Port: %s" CR, xmlconfig[XML_SENS_PORT]);
    Log.INFO("senseBase::onConfig: Type: %s" CR, xmlconfig[XML_SENS_TYPE]);
    if (xmlconfig[XML_SENS_PROPERTIES])
        Log.INFO("senseBase::onConfig: Sensor type specific properties provided, will be passed to the sensor type sub-class object: %s" CR, xmlconfig[XML_SENS_PROPERTIES]);
    if (!strcmp((const char*)xmlconfig[XML_SENS_TYPE], "DIGITAL")) {
        Log.INFO("senseBase::onConfig: Sensor type is digital - programing sens-stem object by creating a senseDigital extention class object" CR);
        extentionSensClassObj = (void*) new senseDigital(this);
    }
    // else if (other sensor types) {...}
    else
        panic("senseBase::onConfig: sensor type not supported");
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], init());
    if (xmlconfig[XML_SENS_PROPERTIES]) {
        Log.INFO("senseBase::onConfig: Configuring the sensor base stem-object with properties" CR);
        SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onConfig(p_sensXmlElement->FirstChildElement("Properties")));
    }
    else
        Log.INFO("senseBase::onConfig: No properties provided for base stem-object" CR);
    unSetOpState(OP_UNCONFIGURED);
    Log.INFO("senseBase::onConfig: Configuration successfully finished" CR);
}

rc_t senseBase::start(void) {
    Log.INFO("senseBase::start: Starting sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.INFO("senseBase::start: sensor port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sensPort, satAddr, satLinkNo);
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (systemState::getOpState() & OP_UNDISCOVERED) {
        Log.INFO("senseBase::start: sensor port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sensPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.INFO("senseBase::start: sensor port %d, on satelite adress %d, satLink %d - starting extention class" CR, sensPort, satAddr, satLinkNo);
   SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], start());
    Log.INFO("senseBase::start: Subscribing to adm- and op state topics");
    char* tmpSysName;
    getSystemName(tmpSysName);
    const char* admSubscribeTopic[5] = { MQTT_SENS_ADMSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", tmpSysName };
    if (mqtt::subscribeTopic(concatStr(admSubscribeTopic, 5), onAdmStateChangeHelper, this))
        panic("senseBase::start: Failed to suscribe to admState topic - rebooting...");
    const char* opSubscribeTopic[5] = { MQTT_SENS_OPSTATE_TOPIC, "/", mqtt::getDecoderUri(), "/", tmpSysName };
    if (mqtt::subscribeTopic(concatStr(opSubscribeTopic, 5), onOpStateChangeHelper, this))
        panic("senseBase::start: Failed to suscribe to opState topic - rebooting...");
    wdt::wdtRegSensorFailsafe(wdtKickedHelper, this);
}

void senseBase::onDiscovered(const satelite* p_sateliteLibHandle) {
    Log.INFO("senseBase::onDiscovered: sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
    satLibHandle = p_sateliteLibHandle;
}

void senseBase::onSystateChangeHelper(const void* p_senseBaseHandle, uint16_t p_sysState) {
    ((senseBase*)p_senseBaseHandle)->onSystateChange(p_sysState);
}

void senseBase::onSystateChange(uint16_t p_sysState) {
    if (!(p_sysState & OP_UNCONFIGURED)){
        SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], onSysStateChange(p_sysState));
        if (p_sysState & OP_INTFAIL && p_sysState & OP_INIT)
            Log.INFO("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, sensPort, satAddr, satLinkNo);
        else if (p_sysState & OP_INTFAIL)
            panic("senseBase::onSystateChange: sensor port on satelite has experienced an internal error - rebooting...");
        if (p_sysState)
            Log.INFO("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has received Opstate %b - doing nothing" CR, sensPort, satAddr, satLinkNo, p_sysState);
        else
            Log.INFO("senseBase::onSystateChange: sensor port %d, on satelite adress %d, satLink %d has received a cleared Opstate - doing nothing" CR, sensPort, satAddr, satLinkNo);
    }
}

void senseBase::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onOpStateChange(p_topic, p_payload);
}

void senseBase::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_OP_AVAIL_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.INFO("senseBase::onOpStateChange: sensor port %d, on satelite adress %d, satLink %d got available message from server: %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_OP_UNAVAIL_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.INFO("senseBase::onOpStateChange: sensor port %d, on satelite adress %d, satLink %d got unavailable message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.ERROR("senseBase::onOpStateChange: sensor port %d, on satelite address %d on satlink %d got an invalid availability message from server %s - doing nothing" CR, sensPort, satAddr, satLinkNo, p_payload);
}

void senseBase::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle) {
    ((senseBase*)p_sensHandle)->onAdmStateChange(p_topic, p_payload);
}

void senseBase::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!strcmp(p_payload, MQTT_ADM_ON_LINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.INFO("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got online message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else if (!strcmp(p_payload, MQTT_ADM_OFF_LINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.INFO("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got off-line message from server %s" CR, sensPort, satAddr, satLinkNo, p_payload);
    }
    else
        Log.ERROR("senseBase::onAdmStateChange: sensor port %d, on satelite adress %d, satLink %d got an invalid admstate message from server %s - doing nothing" CR, sensPort, satAddr, satLinkNo, p_payload);
}

void senseBase::wdtKickedHelper(void* senseBaseHandle) {
    ((senseBase*)senseBaseHandle)->wdtKicked();
}

void senseBase::wdtKicked(void) {
    setOpState(OP_INTFAIL);
}

rc_t senseBase::getOpStateStr(char* p_opStateStr) {
    systemState::getOpStateStr(p_opStateStr);
    return RC_OK;
}

rc_t senseBase::setSystemName(char* p_sysName, bool p_force) {
    Log.ERROR("senseBase::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t senseBase::getSystemName(const char* p_sysName) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getSystemName: cannot get System name as sensor is not configured" CR);
        p_sysName = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    p_sysName = xmlconfig[XML_SENS_SYSNAME];
    return RC_OK;
}

rc_t senseBase::setUsrName(const char* p_usrName, bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("senseBase::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::setUsrName: cannot set System name as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("senseBase::setUsrName: Setting User name to %s" CR, p_usrName);
        delete (char*)xmlconfig[XML_SENS_USRNAME];
        xmlconfig[XML_SENS_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

 rc_t senseBase::getUsrName(const char* p_usrName) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getUsrName: cannot get User name as sensor is not configured" CR);
        p_usrName = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    p_usrName = xmlconfig[XML_SENS_USRNAME];
    return RC_OK;
}

rc_t senseBase::setDesc(const char* p_description, bool p_force) {
    if (!debug || !p_force) {
        Log.ERROR("senseBase::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::setDesc: cannot set Description as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.INFO("senseBase::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SENS_DESC];
        xmlconfig[XML_SENS_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t senseBase::getDesc(char* p_desc) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getDesc: cannot get Description as sensor is not configured" CR);
        p_desc = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    p_desc = xmlconfig[XML_SENS_DESC];
    return RC_OK;
}

rc_t senseBase::setPort(const uint8_t p_port) {
    Log.ERROR("senseBase::setPort: cannot set port - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t senseBase::getPort(uint8_t* p_port) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getPort: cannot get port as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    *p_port = atoi(xmlconfig[XML_SENS_PORT]);
    return RC_OK;
}

rc_t senseBase::setProperty(uint8_t p_propertyId, const char* p_propertyVal, bool p_force){
    if (!debug || !p_force) {
        Log.ERROR("senseBase::setProperty: cannot set Sensor property as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::setProperty: cannot set Sensor property as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], setProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t senseBase::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getProperty: cannot set Sensor property as sensor is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], getProperty(p_propertyId, p_propertyVal));
    return RC_OK;
}

rc_t senseBase::getSensing(const char* p_sensing) {
    if (systemState::getOpState() & OP_UNCONFIGURED) {
        Log.ERROR("senseBase::getSensing: cannot get sensing as sensor is not configured" CR);
        p_sensing = NULL;
        return RC_NOT_CONFIGURED_ERR;
    }
    SENSE_CALL_EXT(extentionSensClassObj, xmlconfig[XML_SENS_TYPE], getSensing((char*)p_sensing));
    return RC_OK;
}

void senseBase::setDebug(bool p_debug) {
    debug = p_debug;
}

bool senseBase::getDebug(void) {
    return debug;
}

/* CLI decoration methods */
void senseBase::onCliGetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
    Command cmd(p_cmd);
    if (cmd.getArgument(1)) {
        notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
        return;
    }
    rc_t rc;
    uint8_t port;
    if (rc = static_cast<senseBase*>(p_cliContext)->getPort(&port)) {
        notAcceptedCliCommand(CLI_GEN_ERR, "Could not get Sensor port, return code: %i", rc);
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
