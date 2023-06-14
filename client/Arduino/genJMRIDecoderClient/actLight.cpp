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
#include "actLight.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actLight (actuator light DNA class)"                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actLight::actLight(actBase* p_actBaseHandle, const char* p_type, char* p_subType) {
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->getPort(true);
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->getSystemName(true);
    satLibHandle = NULL;
    Log.INFO("actLight::actLight: Creating light extention object on actuator port %d, on satelite adress %d, satLink %d" CR, actPort, satAddr, satLinkNo);
    //if (!(actLightLock = xSemaphoreCreateMutex()));
    if (!(actLightLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
        panic("actLight::actLight: Could not create Lock objects - rebooting..." CR);
    actLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    orderedActLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    actLightFailsafePos = ACTLIGHT_DEFAULT_FAILSAFE;
}

actLight::~actLight(void) {
    panic("actLight::~actLight: actLight destructor not supported - rebooting..." CR);
}

rc_t actLight::init(void) {
    Log.INFO("actLight::init: Initializing actLight actuator extention object for light %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actLight::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("actLight::onConfig: Did not expect any configuration for light actuator extention object - rebooting..." CR);
}

rc_t actLight::start(void) {
    Log.INFO("actLight::start: Starting actLight actuator extention object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actLight::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    Log.INFO("actLight::onDiscovered: actLight extention class object %s, on actuator port %s, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_LIGHT_TOPIC, "/", mqtt::getDecoderUri(), "/", sysName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        Log.INFO("actLight::onDiscovered: Subscribing to light orders for light actuator %s, topic: %s" CR, sysName, subscribeTopic);
        if (mqtt::subscribeTopic(subscribeTopic, onActLightChangeHelper, this))
            panic("actLight::onDiscovered: Failed to suscribe to light actuator order topic - rebooting..." CR);
    }
    else{
        satLibHandle = NULL;
        Log.INFO("actLight::onDiscovered: UnSubscribing to light orders for light actuator %s, topic: %s" CR, sysName, subscribeTopic);
        mqtt::unSubscribeTopic(subscribeTopic, onActLightChangeHelper);
    }
}

void actLight::onSysStateChange(uint16_t p_sysState) {
    char opState[100];
    Log.INFO("actLight::onSystateChange: Got a new systemState %s for actLight extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, actBaseHandle->getOpStateStr(opState), actPort, satAddr, satLinkNo);
}

void actLight::onActLightChangeHelper(const char* p_topic, const char* p_payload, const void* p_actLightHandle) {
    ((actLight*)p_actLightHandle)->onActLightChange(p_topic, p_payload);
}

void actLight::onActLightChange(const char* p_topic, const char* p_payload) {
    Log.INFO("actLight::onMemActChange: Got a change order for light actuator %s - new value %d" CR, sysName, p_payload);
    xSemaphoreTake(actLightLock, portMAX_DELAY);
    if (strcmp(p_payload, MQTT_LIGHT_ON_PAYLOAD)) {
        orderedActLightPos = 255;
        if (!failSafe)
            actLightPos = 255;
    }
    else if (strcmp(p_payload, MQTT_LIGHT_OFF_PAYLOAD)) {
        orderedActLightPos = 0;
        if (!failSafe)
            actLightPos = 0;
    }
    setActLight();
    xSemaphoreGive(actLightLock);
}

void actLight::setActLight(void) {
    if ((satLibHandle != NULL) && !failSafe){
        if (actLightPos) {
            if (satLibHandle->setSatActMode(SATMODE_HIGH, actPort))
                Log.ERROR("actLight::setActLight: Failed to execute order for light actuator %s" CR, sysName);
        }
        else {
            if (satLibHandle->setSatActMode(SATMODE_LOW, actPort))
                Log.ERROR("actLight::setActLight: Failed to execute order for light actuator %s" CR, sysName);
        }
        Log.INFO("actLight::setActLight: Light actuator change order for %s fininished" CR, sysName);
    }
}

void actLight::failsafe(bool p_failSafe) {
    xSemaphoreTake(actLightLock, portMAX_DELAY);
    if (p_failSafe) {
        Log.INFO("actLight::setFailSafe: Fail-safe set for light actuator %s" CR, sysName);
        actLightPos = actLightFailsafePos;
        setActLight();
        failSafe = p_failSafe;

    }
    else {
        Log.INFO("actLight::setFailSafe: Fail-safe un-set for light actuator %s" CR, sysName);
        actLightPos = orderedActLightPos;
        failSafe = p_failSafe;
        setActLight();
    }
    xSemaphoreGive(actLightLock);
}

rc_t actLight::setProperty(uint8_t p_propertyId, const char* p_propertyVal) {
    Log.INFO("actLight::setProperty: Setting of Light property not implemented" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actLight::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    Log.INFO("actLight::getProperty: Getting of Light property not implemented" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actLight::setShowing(const char* p_showing) {
    if (!strcmp(p_showing, MQTT_LIGHT_ON_PAYLOAD)) {
        onActLightChange(NULL, MQTT_LIGHT_ON_PAYLOAD);
        return RC_OK;
    }
    if (!strcmp(p_showing, MQTT_LIGHT_OFF_PAYLOAD)) {
        onActLightChange(NULL, MQTT_LIGHT_OFF_PAYLOAD);
        return RC_OK;
    }
    return RC_PARAMETERVALUE_ERR;
}

rc_t actLight::getShowing(char* p_showing, char* p_orderedShowing) {
    xSemaphoreTake(actLightLock, portMAX_DELAY);
    if (actLightPos)
        p_showing = (char*)MQTT_LIGHT_ON_PAYLOAD;
    else
        p_showing = (char*)MQTT_LIGHT_OFF_PAYLOAD;
    if (orderedActLightPos)
        p_orderedShowing = (char*)MQTT_LIGHT_ON_PAYLOAD;
    else
        p_orderedShowing = (char*)MQTT_LIGHT_OFF_PAYLOAD;
    xSemaphoreGive(actLightLock);
    return RC_OK;
}

/*==============================================================================================================================================*/
/* END Class actLight                                                                                                                           */
/*==============================================================================================================================================*/
