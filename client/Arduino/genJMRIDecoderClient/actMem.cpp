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
#include "actMem.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actMem (actuator memory DNA class)"                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actMem::actMem(actBase* p_actBaseHandle, const char* p_type, char* p_subType) {
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->getPort(true);
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->getSystemName(true);
    if (!(actMemLock = xSemaphoreCreateMutex()))
        panic("Could not create Lock objects - rebooting...");
    satLibHandle = NULL;
    actMemSolenoidPushPort = true;
    actMemSolenoidActivationTime = ACTMEM_DEFAULT_SOLENOID_ACTIVATION_TIME_MS;

    if (!strcmp(p_subType, MQTT_MEM_SOLENOID_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_SOLENOID;
    }
    if (!strcmp(p_subType, MQTT_MEM_SERVO_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_SERVO;
    }
    if (!strcmp(p_subType, MQTT_MEM_PWM100_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PWM100;
    }
    if (!strcmp(p_subType, MQTT_MEM_PWM1_25K_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PWM125K;
    }
    else if (!strcmp(p_subType, MQTT_MEM_ONOFF_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_ONOFF;
    }
    else if (!strcmp(p_subType, MQTT_MEM_PULSE_PAYLOAD)) {
        actMemType = ACTMEM_TYPE_PULSE;
    }
    LOG_INFO("Creating memory extention object for %s on actuator port %d, on satelite adress %d, satLink %d" CR, p_subType, actPort, satAddr, satLinkNo);
    actMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    orderedActMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    actMemFailsafePos = atoi(ACTMEM_DEFAULT_FAILSAFE);
}

actMem::~actMem(void) {
    panic("actMem destructor not supported - rebooting...");
}

rc_t actMem::init(void) {
    LOG_INFO("Initializing actMem actuator extention object for memory %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actMem::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("Did not expect any configuration for memory actuator extention object - rebooting...");
}

rc_t actMem::start(void) {
    LOG_INFO("Starting actMem actuator extention object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actMem::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_MEM_TOPIC, "/", mqtt::getDecoderUri(), "/", sysName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        LOG_INFO("Subscribing to memory orders for Memory actuator %s" CR, sysName);
        if (mqtt::subscribeTopic(subscribeTopic, onActMemChangeHelper, this))
            panic("Failed to suscribe to actMem order topic - rebooting..." CR);
        LOG_INFO("actMem extention class object %s, on actuator port %i, on satelite adress %i, satLink %i discovered" CR, sysName, actPort, satAddr, satLinkNo);
        if (actMemType == ACTMEM_TYPE_SOLENOID) {
            if (actPort % 2) {
                actMemSolenoidPushPort = false;
                LOG_INFO("Startings solenoid memory actuator pull port %s, on port %i, on satelite adress %i, satLink %i" CR, sysName, actPort, satAddr, satLinkNo);
            }
            else {
                LOG_INFO("Startings solenoid memory actuator push port %s, on port %i, on satelite adress %i, satLink %i" CR, sysName, actPort, satAddr, satLinkNo);
                actMemSolenoidPushPort = true;
            }
            satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_SERVO) {
            LOG_INFO("Startings servo memory  actuator %s, on port %i, on satelite adress %i, satLink %i" CR, sysName, actPort, satAddr, satLinkNo);
            satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_PWM100) {
            LOG_INFO("Startings PWM 100 Hz memory  actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
            satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_PWM125K) {
            LOG_INFO("Startings PWM 125 KHz memory actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
            satLibHandle->setSatActMode(SATMODE_PWM1_25K, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_ONOFF) {
            LOG_INFO("Startings ON/OFF memory actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        }
        else if (actMemType == ACTMEM_TYPE_PULSE) {
            LOG_INFO("Startings Pulse memory actuator %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
            satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
        }
    }
    else {
        satLibHandle = NULL;
        LOG_INFO("UnSubscribing to memory orders for Memory actuator %s" CR, sysName);
        mqtt::unSubscribeTopic(subscribeTopic, onActMemChangeHelper);
    }
}

void actMem::onSysStateChange(sysState_t p_sysState) {
    char opState[100];
    LOG_INFO("Got a new systemState %d for actMem extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, actBaseHandle->systemState::getOpStateStr(opState), actPort, satAddr, satLinkNo);
}

void actMem::onActMemChangeHelper(const char* p_topic, const char* p_payload, const void* p_actMemHandle) {
    ((actMem*)p_actMemHandle)->onActMemChange(p_topic, p_payload);
}

void actMem::onActMemChange(const char* p_topic, const char* p_payload) {
    LOG_INFO("Got a change order for memory actuator %s - new value %s" CR, sysName, p_payload);
    xSemaphoreTake(actMemLock, portMAX_DELAY);
    if (!strcmp(p_payload, "ON")) {
        orderedActMemPos = 255;
        if (!failSafe)
            actMemPos = 255;
    }
    else if (!strcmp(p_payload, "OFF")) {
        orderedActMemPos = 0;
        if (!failSafe)
            actMemPos = 0;
    }
    else {
        orderedActMemPos = (atoi(p_payload) > 255? 255 : (atoi(p_payload)));
        orderedActMemPos = (orderedActMemPos < 0 ? 0 : orderedActMemPos);
        if (!failSafe)
            actMemPos = orderedActMemPos;
    }
    setActMem();
    xSemaphoreGive(actMemLock);
}

void actMem::setActMem(void) {
    satErr_t libRc;
    if ((satLibHandle != NULL) && !failSafe) {
        if (actMemType == ACTMEM_TYPE_SOLENOID) {
            if ((actMemPos) ^ actMemSolenoidPushPort) {
                if (libRc = satLibHandle->setSatActVal(actMemSolenoidActivationTime, actPort))
                    LOG_ERROR("Failed to execute order for memory solenoid actuator %s" CR, sysName, libRc);
                else
                    LOG_INFO("Memory solenoid actuator change order for %s fininished" CR, sysName);
            }
        }
        else if (actMemType == ACTMEM_TYPE_SERVO) {
            uint8_t tmpServoPwmPos = SERVO_LEFT_PWM_VAL + (actMemPos * (SERVO_RIGHT_PWM_VAL - SERVO_LEFT_PWM_VAL) / 256);
            if (libRc = satLibHandle->setSatActVal(tmpServoPwmPos, actPort))
                LOG_ERROR("Failed to execute order for memory servo actuator %s, return code: 0x%llx" CR, sysName, libRc);
            else
                LOG_INFO("Memory servo actuator change order for %s fininished" CR, sysName);
        }
        else if (actMemType == ACTMEM_TYPE_PWM100) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("Failed to execute order for memory PWM 100 Hz actuator %s, return code: 0x%llx" CR, sysName, libRc);
            else
                LOG_INFO("Memory 100 Hz PWM actuator change order for %s fininished" CR, sysName);
        }
        else if (actMemType == ACTMEM_TYPE_PWM125K) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("Failed to execute order for memory PWM 125 KHz actuator %s, return code: 0x%llx" CR, sysName, libRc);
            else
                LOG_INFO("Memory 125 KHz PWM actuator change order for %s fininished" CR, sysName);
        }
        else if (actMemType == ACTMEM_TYPE_ONOFF) {
            if (actMemPos) {
                if (libRc = satLibHandle->setSatActMode(SATMODE_HIGH, actPort))
                    LOG_ERROR("Failed to execute order for memory ON/OFF actuator %s, return code: 0x%llx" CR, sysName, libRc);
                else
                    LOG_INFO("Memory ON actuator change order for %s fininished" CR, sysName);
            }
            else {
                if (libRc = satLibHandle->setSatActMode(SATMODE_LOW, actPort))
                    LOG_ERROR("Failed to execute order for memory ON/OFF actuator %s, return code: 0x%llx" CR, sysName, libRc);
                else
                    LOG_INFO("Memory OFF actuator change order for %s fininished" CR, sysName);
            }
        }
        else if (actMemType == ACTMEM_TYPE_PULSE) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("Failed to execute order for Pulse actuator %s, return code: 0x%llx" CR, sysName, libRc);
            else
                LOG_INFO("Memory Pulse actuator change order for %s fininished" CR, sysName);
        }
        else 
            LOG_ERROR("Configured memActuator type %i for actuator %s, not supported" CR, actMemType, sysName);
    }
}

void actMem::failsafe(bool p_failSafe) {
    xSemaphoreTake(actMemLock, portMAX_DELAY);
    if (failSafe) {
        LOG_INFO("Fail-safe set for memory actuator %s" CR, sysName);
        actMemPos = actMemFailsafePos;
        setActMem();
        failSafe = p_failSafe;
    }
    else {
        LOG_INFO("Fail-safe un-set for memory actuator %s" CR, sysName);
        failSafe = p_failSafe;
        actMemPos = orderedActMemPos;
        setActMem();
    }
    xSemaphoreGive(actMemLock);
}

rc_t actMem::setProperty(uint8_t p_propertyId, const char* p_propertyVal) {
    LOG_INFO("Setting of Memory property not implemented" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actMem::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    LOG_INFO("Getting of Memory property not implemented" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actMem::setShowing(const char* p_showing) {
    if (!strcmp(p_showing, MQTT_MEM_ON_PAYLOAD)) {
        onActMemChange(NULL, MQTT_LIGHT_ON_PAYLOAD);
        return RC_OK;
    }
    if (!strcmp(p_showing, MQTT_MEM_OFF_PAYLOAD)) {
        onActMemChange(NULL, MQTT_MEM_OFF_PAYLOAD);
        return RC_OK;
    }
    if (strtol(p_showing, NULL, 10) || !strcmp(p_showing, "0")) {
        onActMemChange(NULL, p_showing);
        return RC_OK;
        return RC_PARAMETERVALUE_ERR;
    }
}

rc_t actMem::getShowing(char* p_showing, char* p_orderedShowing) {
    xSemaphoreTake(actMemLock, portMAX_DELAY);
    if (actMemPos == 0)
        p_showing = (char*)MQTT_MEM_OFF_PAYLOAD;
    else if (actMemPos == 255)
        p_showing = (char*)MQTT_MEM_ON_PAYLOAD;
    else
        //ITOA NEEDS FIX - WILL CRASH
        p_showing = itoa(actMemPos, NULL, 10);
    if (orderedActMemPos == 0)
        p_orderedShowing = (char*)MQTT_MEM_OFF_PAYLOAD;
    else if (orderedActMemPos == 255)
        p_orderedShowing = (char*)MQTT_MEM_ON_PAYLOAD;
    else
        //ITOA NEEDS FIX - WILL CRASH
        p_orderedShowing = itoa(orderedActMemPos, NULL, 10);
    xSemaphoreGive(actMemLock);
    return RC_OK;
}

/*==============================================================================================================================================*/
/* END Class actMem                                                                                                                             */
/*==============================================================================================================================================*/
