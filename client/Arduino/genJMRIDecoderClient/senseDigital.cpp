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

    LOG_INFO("Creating senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    senseDigitalLock = xSemaphoreCreateMutex();
    if (senseDigitalLock == NULL)
        panic("Could not create Lock objects - rebooting...");
}

senseDigital::~senseDigital(void) {
    panic("SenseDigital destructior not supported - rebooting...");
}

rc_t senseDigital::init(void) {
    LOG_INFO("Initializing senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseDigital::onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement) {
    panic("Did not expect any configuration for senseDigital sensor extention object for sensor port");
    //Potential extention properties: sensor filtering time,...
}

rc_t senseDigital::start(void) {
    LOG_INFO("Starting senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseDigital::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    if (p_exists) {
        LOG_INFO("sensor discovered, extention class object for digital sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
        satLibHandle = p_sateliteLibHandle;
        satLibHandle->setSenseFilter(DEFAULT_SENS_FILTER_TIME, sensPort);
    }
    else {
        LOG_INFO("sensor removed, extention class object for digital sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
        satLibHandle = NULL;
    }
}

void senseDigital::onSysStateChange(uint16_t p_sysState) {
    sysState = p_sysState;
    LOG_INFO("Got a new systemState %d for senseDigital extention class object %s, on sensor port %d, on satelite adress %d, satLink %d" CR, sysState, sensPort, satAddr, satLinkNo);

    if (!sysState)
        onSenseChange(filteredSenseVal);
}

void senseDigital::onSenseChange(bool p_filteredSensorVal) {
    LOG_VERBOSE("sensorvalue have changed to: %i" CR, p_filteredSensorVal);
    filteredSenseVal = p_filteredSensorVal;
    if (!failSafe) {
        char publishTopic[300];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SENS_TOPIC, "/", mqtt::getDecoderUri(), "/", sensSysName);
        if (mqtt::sendMsg(publishTopic, ("%s", filteredSenseVal ? MQTT_SENS_DIGITAL_ACTIVE_PAYLOAD : MQTT_SENS_DIGITAL_INACTIVE_PAYLOAD), false))
            LOG_ERROR("Failed to send sensor value" CR);
    }
}

void senseDigital::failsafe(bool p_failSafe) {
    if (p_failSafe) {
        failSafe = p_failSafe;
        char publishTopic[300];
        sprintf(publishTopic, "%s%s%s%s%s", MQTT_SENS_TOPIC, "/", mqtt::getDecoderUri(), "/", sensSysName);
        if (mqtt::sendMsg(publishTopic, MQTT_SENS_DIGITAL_FAILSAFE_PAYLOAD, false))
            LOG_ERROR("Failed to send sensor value" CR);
    }
    else {
        LOG_INFO("Fail-safe un-set for light actuator %s" CR, sensSysName);
        failSafe = p_failSafe;
        onSenseChange(filteredSenseVal);
    }
}

rc_t senseDigital::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    LOG_INFO("Setting Digital sensor property for %s, property Id %d, property value %s" CR, sensSysName, p_propertyId, p_propertyValue);
    return RC_NOTIMPLEMENTED_ERR;
    //......
}

rc_t senseDigital::getProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    LOG_INFO("Not supported" CR);
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
