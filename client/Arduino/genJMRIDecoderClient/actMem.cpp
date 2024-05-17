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
    asprintf(&logContextName, "%s/%s-%s", p_actBaseHandle->getLogContextName(), "mem", p_subType);
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->getPort(true);
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    sysName = actBaseHandle->getSystemName(true);
    if (!(actMemLock = xSemaphoreCreateMutex())) {
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
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
    LOG_INFO("%s: Creating memory extention object" CR, logContextName);
    actMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    orderedActMemPos = atoi(ACTMEM_DEFAULT_FAILSAFE);
    actMemFailsafePos = atoi(ACTMEM_DEFAULT_FAILSAFE);
}

actMem::~actMem(void) {
    panic("%s: actMem destructor not supported", logContextName);
}

rc_t actMem::init(void) {
    LOG_INFO("%s: Initializing actMem extention object" CR, logContextName);
    return RC_OK;
}

void actMem::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("%s: Did not expect any configuration for memory actuator extention object", logContextName);
}

rc_t actMem::start(void) {
    LOG_INFO("%s: Starting actMem extention object" CR, logContextName);
    return RC_OK;
}

void actMem::onDiscovered(satelite* p_sateliteLibHandle, bool p_exists) {
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_MEM_TOPIC, "/", mqtt::getDecoderUri(), "/", sysName);
    if (p_exists) {
        satLibHandle = p_sateliteLibHandle;
        LOG_INFO("%s: Subscribing to memory orders" CR, logContextName);
        if (mqtt::subscribeTopic(subscribeTopic, onActMemChangeHelper, this)) {
            panic("%s: Failed to suscribe to actMem order topic \"%s\"" CR, logContextName, subscribeTopic);
            return;
        }
        LOG_INFO("%s: actMem extention class object discovered" CR, logContextName);
        if (actMemType == ACTMEM_TYPE_SOLENOID) {
            if (actPort % 2) {
                actMemSolenoidPushPort = false;
                LOG_INFO("%s: Startings solenoid memory actuator pull port" CR, logContextName);
            }
            else {
                LOG_INFO("%s: Startings solenoid memory actuator push port" CR, logContextName);
                actMemSolenoidPushPort = true;
            }
            satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_SERVO) {
            LOG_INFO("%s: Startings servo memory actuator %s" CR, logContextName);
            satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_PWM100) {
            LOG_INFO("%s: Startings PWM 100 Hz memory  actuator" CR, logContextName);
            satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_PWM125K) {
            LOG_INFO("%s: Startings PWM 125 KHz memory actuator" CR, logContextName);
            satLibHandle->setSatActMode(SATMODE_PWM1_25K, actPort);
        }

        else if (actMemType == ACTMEM_TYPE_ONOFF) {
            LOG_INFO("%s: Startings ON/OFF memory actuator" CR, logContextName);
        }
        else if (actMemType == ACTMEM_TYPE_PULSE) {
            LOG_INFO("%s: Startings Pulse memory actuator" CR, logContextName);
            satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
        }
    }
    else {
        satLibHandle = NULL;
        LOG_INFO("%s: UnSubscribing to memory orders" CR, logContextName);
        mqtt::unSubscribeTopic(subscribeTopic, onActMemChangeHelper);
    }
}

void actMem::onSysStateChange(sysState_t p_sysState) {
    char opState[100];
    LOG_INFO("%s: Got a new systemState %s " CR, logContextName, actBaseHandle->systemState::getOpStateStr(opState));
}

void actMem::onActMemChangeHelper(const char* p_topic, const char* p_payload, const void* p_actMemHandle) {
    ((actMem*)p_actMemHandle)->onActMemChange(p_topic, p_payload);
}

void actMem::onActMemChange(const char* p_topic, const char* p_payload) {
    LOG_INFO("%s: Got a change order - new value %s" CR, logContextName, p_payload);
    xSemaphoreTake(actMemLock, portMAX_DELAY);
    if (!strcmp(p_payload, "ON")) {
        if (actMemType == ACTMEM_TYPE_SOLENOID) {
            orderedActMemPos = 1;
            if (!failSafe)
                actMemPos = 1;
        }
		else {
			orderedActMemPos = 255;
			if (!failSafe)
				actMemPos = 255;
		}
    }
    else if (!strcmp(p_payload, "OFF")) {
        orderedActMemPos = 0;
        if (!failSafe)
            actMemPos = 0;
    }
    else {
        if (actMemType == ACTMEM_TYPE_SOLENOID) {
            orderedActMemPos = 0;
        }
        else {
            orderedActMemPos = (atoi(p_payload) > 255 ? 255 : (atoi(p_payload)));
            orderedActMemPos = (orderedActMemPos < 0 ? 0 : orderedActMemPos);
        }
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
                    LOG_ERROR("%s: Failed to execute order for memory solenoid actuator, return code 0x%llx" CR, logContextName, libRc);
                else
                    LOG_INFO("%s: Memory solenoid actuator change order fininished" CR, logContextName);
            }
        }
        else if (actMemType == ACTMEM_TYPE_SERVO) {
            uint8_t tmpServoPwmPos = SERVO_LEFT_PWM_VAL + (actMemPos * (SERVO_RIGHT_PWM_VAL - SERVO_LEFT_PWM_VAL) / 256);
            if (libRc = satLibHandle->setSatActVal(tmpServoPwmPos, actPort))
                LOG_ERROR("%s: Failed to execute order for memory servo actuator, return code: 0x%llx" CR, logContextName, libRc);
            else
                LOG_INFO("%s: Memory servo actuator change order fininished" CR, logContextName);
        }
        else if (actMemType == ACTMEM_TYPE_PWM100) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("%s: Failed to execute order for memory PWM 100 Hz actuator, return code: 0x%llx" CR, logContextName, libRc);
            else
                LOG_INFO("%s: Memory 100 Hz PWM actuator change order fininished" CR, logContextName);
        }
        else if (actMemType == ACTMEM_TYPE_PWM125K) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("%s: Failed to execute order for memory PWM 125 KHz actuator, return code: 0x%llx" CR, logContextName, libRc);
            else
                LOG_INFO("%s: Memory 125 KHz PWM actuator change order fininished" CR, logContextName);
        }
        else if (actMemType == ACTMEM_TYPE_ONOFF) {
            if (actMemPos) {
                if (libRc = satLibHandle->setSatActMode(SATMODE_HIGH, actPort))
                    LOG_ERROR("%s: Failed to execute order for memory ON/OFF actuator, return code: 0x%llx" CR, logContextName, libRc);
                else
                    LOG_INFO("%s: Memory ONF actuator change order fininished" CR, logContextName);
            }
            else {
                if (libRc = satLibHandle->setSatActMode(SATMODE_LOW, actPort))
                    LOG_ERROR("%s: Failed to execute order for memory ON/OFF actuator, return code: 0x%llx" CR, logContextName, libRc);
                else
                    LOG_INFO("%s: Memory OFF actuator change order fininished" CR, logContextName);
            }
        }
        else if (actMemType == ACTMEM_TYPE_PULSE) {
            if (libRc = satLibHandle->setSatActVal(actMemPos, actPort))
                LOG_ERROR("%s: Failed to execute order for Pulse actuator, return code: 0x%llx" CR, logContextName, libRc);
            else
                LOG_INFO("%s: Memory Pulse actuator change order fininished" CR, logContextName);
        }
        else 
            LOG_ERROR("%s: Configured memActuator type %i, not supported" CR, logContextName, actMemType);
    }
}

void actMem::failsafe(bool p_failSafe) {
    xSemaphoreTake(actMemLock, portMAX_DELAY);
    if (failSafe) {
        LOG_INFO("%s: Fail-safe set for memory actuator" CR, logContextName);
        actMemPos = actMemFailsafePos;
        setActMem();
        failSafe = p_failSafe;
    }
    else {
        LOG_INFO("%s: Fail-safe un-set for memory actuator" CR, logContextName);
        failSafe = p_failSafe;
        actMemPos = orderedActMemPos;
        setActMem();
    }
    xSemaphoreGive(actMemLock);
}

rc_t actMem::setProperty(uint8_t p_propertyId, const char* p_propertyVal) {
    LOG_INFO("%s: Setting of Memory property not implemented" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actMem::getProperty(uint8_t p_propertyId, char* p_propertyVal) {
    LOG_INFO("%s: Getting of Memory property not implemented" CR, logContextName);
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
