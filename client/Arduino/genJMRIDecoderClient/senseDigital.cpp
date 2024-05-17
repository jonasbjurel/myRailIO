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
#include "senseDigital.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "senseDigital (Digital sensor DNA class)"                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
senseDigital::senseDigital(senseBase* p_senseBaseHandle) {
    asprintf(&logContextName, "%s/%s", p_senseBaseHandle->getLogContextName(), "digital");
    LOG_INFO("%s: Creating senseDigital extention object" CR, logContextName);
    senseBaseHandle = p_senseBaseHandle;
    sensPort = senseBaseHandle->getPort();
    satAddr = senseBaseHandle->satHandle->getAddr();
    satLinkNo = senseBaseHandle->satHandle->linkHandle->getLink();
    sensSysName = senseBaseHandle->getSystemName(true);
    satLibHandle = NULL;
    sysState = OP_WORKING;
    failSafe = false;
    filteredSenseVal = SENSDIGITAL_DEFAULT_FAILSAFE;
    debug = false;
}

senseDigital::~senseDigital(void) {
    panic("%s: SenseDigital destructior not supported", logContextName);
}

rc_t senseDigital::init(void) {
    LOG_INFO("%s: Initializing senseDigital extention object" CR, logContextName);
    return RC_OK;
}

void senseDigital::onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement) {
    panic("%s: Did not expect any configuration for senseDigital extention object", logContextName);
    //Potential extention properties: sensor filtering time,...
}

rc_t senseDigital::start(void) {
    LOG_INFO("%s: Starting senseDigital extention object" CR, logContextName);
    return RC_OK;
}

void senseDigital::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    if (p_exists) {
        LOG_INFO("%s: sensor discovered" CR, logContextName);
        satLibHandle = p_sateliteLibHandle;
        satLibHandle->setSenseFilter(DEFAULT_SENS_FILTER_TIME, sensPort);
    }
    else {
        LOG_INFO("%s: sensor removed" CR, sensPort, satAddr, satLinkNo);
        satLibHandle = NULL;
    }
}

void senseDigital::onSysStateChange(uint16_t p_sysState) {
    sysState = p_sysState;
    char opState[100];
    LOG_INFO("%s: Got a new systemState %x" CR, logContextName, senseBaseHandle->systemState::getOpStateStr(opState));

    if (!sysState)
        onSenseChange(filteredSenseVal);
}

void senseDigital::onSenseChange(bool p_filteredSensorVal) {
    LOG_VERBOSE("%s: sensor value has changed to: %i" CR, logContextName, p_filteredSensorVal);
    filteredSenseVal = p_filteredSensorVal;
    if (!failSafe) {
        char publishTopic[300];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SENS_TOPIC, "/", mqtt::getDecoderUri(), "/", sensSysName);
        if (mqtt::sendMsg(publishTopic, ("%s", filteredSenseVal ? MQTT_SENS_DIGITAL_ACTIVE_PAYLOAD : MQTT_SENS_DIGITAL_INACTIVE_PAYLOAD), false))
            LOG_ERROR("%s: Failed to send sensor value" CR, logContextName);
    }
}

void senseDigital::failsafe(bool p_failSafe) {
    if (p_failSafe) {
        failSafe = p_failSafe;
        char publishTopic[300];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SENS_TOPIC, "/", mqtt::getDecoderUri(), "/", sensSysName);
        if (mqtt::sendMsg(publishTopic, MQTT_SENS_DIGITAL_FAILSAFE_PAYLOAD, false))
            LOG_ERROR("%s: Failed to send sensor value" CR, logContextName);
    }
    else {
        LOG_INFO("%s: Fail-safe un-set" CR, logContextName);
        failSafe = p_failSafe;
        onSenseChange(filteredSenseVal);
    }
}

rc_t senseDigital::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    LOG_INFO("%s: Setting Digital sensor property Id %d, property value %s" CR, logContextName, p_propertyId, p_propertyValue);
    return RC_NOTIMPLEMENTED_ERR;
    //......
}

rc_t senseDigital::getProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    return RC_NOTIMPLEMENTED_ERR;
    //......
}

void senseDigital::getSensing(char* p_sensing) {
    (filteredSenseVal ? p_sensing = (char*)MQTT_SENS_DIGITAL_ACTIVE_PAYLOAD : p_sensing = (char*)MQTT_SENS_DIGITAL_INACTIVE_PAYLOAD);
}

void senseDigital::setDebug(bool p_debug) {
    debug = p_debug;
}

bool senseDigital::getDebug(void) {
    return debug;
}

bool senseDigital::getFilteredSense(void) {
    return filteredSenseVal;
}

bool senseDigital::getUnFilteredSense(void) {
    return satLibHandle->getSenseVal(sensPort);
}

/*==============================================================================================================================================*/
/* END Class senseDigital                                                                                                                        */
/*==============================================================================================================================================*/
