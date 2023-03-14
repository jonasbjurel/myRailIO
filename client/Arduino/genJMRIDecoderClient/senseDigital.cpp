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
    sensPort = senseBaseHandle->sensPort;
    senseBaseHandle->satHandle->getAddr(&satAddr);
    senseBaseHandle->satHandle->linkHandle->getLink(&satLinkNo);
    senseBaseHandle->getSystemName(sensSysName);
    satLibHandle = NULL;
    pendingStart = false;
    sysState = 0;
    filteredSenseVal = false;
    debug = false;

    Log.notice("senseDigital::senseDigital: Creating senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    senseDigitalLock = xSemaphoreCreateMutex();
    if (senseDigitalLock == NULL)
        panic("senseDigital::senseDigital: Could not create Lock objects - rebooting...");
}

senseDigital::~senseDigital(void) {
    panic("senseDigital::~senseDigital: senseDigital destructior not supported - rebooting...");
}

rc_t senseDigital::init(void) {
    Log.notice("senseDigital::init: Initializing senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    return RC_OK;
}

void senseDigital::onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement) {
    panic("senseDigital::onConfig: Did not expect any configuration for senseDigital sensor extention object for sensor port");
    //Potential extention properties: sensor filtering time,...
}

rc_t senseDigital::start(void) {
    Log.notice("senseDigital::start: Starting senseDigital sensor extention object for sensor port% d, on satelite adress% d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    if (senseBaseHandle->systemState::getOpState() & OP_UNCONFIGURED) {
        Log.notice("senseDigital::start: senseDigital sensor extention object for sensor port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sensPort, satAddr, satLinkNo);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (senseBaseHandle->systemState::getOpState() & OP_UNDISCOVERED) {
        Log.notice("senseDigital::start: senseDigital sensor extention class object for sensor port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sensPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("senseDigital::start: Configuring and startings senseDigital extention class object for sensor port %d, on satelite adress %d, satLink %d" CR, sensPort, satAddr, satLinkNo);
    satLibHandle->setSenseFilter(DEFAULT_SENS_FILTER_TIME, sensPort);
    return RC_OK;
}

void senseDigital::onDiscovered(satelite* p_sateliteLibHandle) {
    satLibHandle = p_sateliteLibHandle;
    Log.notice("senseDigital::onDiscovered: sensor extention class object for digital sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
    if (senseBaseHandle->pendingStart) {
        Log.notice("senseDigital::onDiscovered: Initiating pending start for digital sensor extention class object for sensor port %d, on satelite adress %d, satLink %d discovered" CR, sensPort, satAddr, satLinkNo);
        start();
    }
}

void senseDigital::onSysStateChange(uint16_t p_sysState) {
    sysState = p_sysState;
    if (!sysState)
        onSensChange(filteredSenseVal);
}

void senseDigital::onSensChange(bool p_filteredSensorVal) {
    filteredSenseVal = p_filteredSensorVal;
    if (!sysState) {
        const char* publishTopic[2] = { MQTT_SENS_TOPIC, sensSysName };
        if (mqtt::sendMsg(concatStr(publishTopic, 2), ("%s", filteredSenseVal ? MQTT_SENS_DIGITAL_ACTIVE_PAYLOAD : MQTT_SENS_DIGITAL_INACTIVE_PAYLOAD), false))
            Log.ERROR("senseDigital::onSensChange: Failed to send sensor value" CR);
    }
}

rc_t senseDigital::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    Log.notice("senseDigital::setProperty: Setting Digital sensor property for %s, property Id %d, property value %s" CR, sensSysName, p_propertyId, p_propertyValue);
    return RC_NOTIMPLEMENTED_ERR;
    //......
}

rc_t senseDigital::getProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    Log.notice("senseDigital::getProperty: Not supported" CR);
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
