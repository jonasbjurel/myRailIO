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
    asprintf(&logContextName, "%s/%s-%s", p_actBaseHandle->getLogContextName(), "mem", p_subType);
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->getPort(true);
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->getSystemName(true);
    satLibHandle = NULL;
    LOG_INFO("%s: Creating light extention object" CR, logContextName);
    if (!(actLightLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    actLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    orderedActLightPos = ACTLIGHT_DEFAULT_FAILSAFE;
    actLightFailsafePos = ACTLIGHT_DEFAULT_FAILSAFE;
}

actLight::~actLight(void) {
    panic("%s: actLight destructor not supported", logContextName);
}

rc_t actLight::init(void) {
    LOG_INFO("%s Initializing actLight actuator extention object" CR, logContextName);
    return RC_OK;
}

void actLight::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("%s: Did not expect any configuration for light actuator extention object", logContextName);
}

rc_t actLight::start(void) {
    LOG_INFO("%s: Starting actLight actuator extention object" CR, logContextName);
    return RC_OK;
}

void actLight::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    LOG_INFO("%s: actLight extention class object discovered" CR, logContextName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_LIGHT_TOPIC, "/", mqtt::getDecoderUri(), "/", sysName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        LOG_INFO("%s: Subscribing to light orders \"%s\"" CR, logContextName, subscribeTopic);
        if (mqtt::subscribeTopic(subscribeTopic, onActLightChangeHelper, this)){
            panic("%s: Failed to suscribe to light actuator order topic", logContextName);
            return;
        }
    }
    else{
        satLibHandle = NULL;
        LOG_INFO("%s: UnSubscribing to light orders, topic: \"%s\"" CR, sysName, subscribeTopic);
        mqtt::unSubscribeTopic(subscribeTopic, onActLightChangeHelper);
    }
}

void actLight::onSysStateChange(uint16_t p_sysState) {
    char opState[100];
    LOG_INFO("%s: Got a new systemState %s" CR, logContextName, actBaseHandle->systemState::getOpStateStr(opState));
}

void actLight::onActLightChangeHelper(const char* p_topic, const char* p_payload, const void* p_actLightHandle) {
    ((actLight*)p_actLightHandle)->onActLightChange(p_topic, p_payload);
}

void actLight::onActLightChange(const char* p_topic, const char* p_payload) {
    LOG_INFO("%s: Got a change order - new value %s" CR, logContextName, p_payload);
    xSemaphoreTake(actLightLock, portMAX_DELAY);
    if (!strcmp(p_payload, MQTT_LIGHT_ON_PAYLOAD)) {
        orderedActLightPos = 255;
        if (!failSafe)
            actLightPos = 255;
    }
    else if (!strcmp(p_payload, MQTT_LIGHT_OFF_PAYLOAD)) {
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
                LOG_ERROR("%s: Failed to execute light order" CR, logContextName);
        }
        else {
            if (satLibHandle->setSatActMode(SATMODE_LOW, actPort))
                LOG_ERROR("%s: Failed to execute order for light actuator %s" CR, logContextName);
        }
        LOG_INFO("%s: Light actuator change order fininished" CR, logContextName);
    }
}

void actLight::failsafe(bool p_failSafe) {
    xSemaphoreTake(actLightLock, portMAX_DELAY);
    if (p_failSafe) {
        LOG_INFO("%s Fail-safe set" CR, logContextName);
        actLightPos = actLightFailsafePos;
        setActLight();
        failSafe = p_failSafe;

    }
    else {
        LOG_INFO("%s: Fail-safe un-set" CR, logContextName);
        actLightPos = orderedActLightPos;
        failSafe = p_failSafe;
        setActLight();
    }
    xSemaphoreGive(actLightLock);
}

rc_t actLight::setProperty(uint8_t p_propertyId, const char* p_propertyVal) {
    LOG_INFO("%s: Setting of Light property not implemented" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actLight::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    LOG_INFO("%s: Getting of Light property not implemented" CR, logContextName);
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
