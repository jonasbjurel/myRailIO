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
#include "actTurn.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actTurn (Turnout actuator DNA class)"                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
actTurn::actTurn(actBase* p_actBaseHandle, const char* p_type, char* p_subType) {
    asprintf(&logContextName, "%s/%s-%s", p_actBaseHandle->getLogContextName(), "turn", p_subType);
    actBaseHandle = p_actBaseHandle;
    actPort = actBaseHandle->getPort(true);
    satAddr = actBaseHandle->satHandle->getAddr();
    satLinkNo = actBaseHandle->satHandle->linkHandle->getLink();
    actBaseHandle->getSystemName(sysName, true);
    satLibHandle = NULL;
    turnOutInvert = false;
    turnSolenoidPushPort = true;
    if (!strcmp(p_subType, MQTT_TURN_SERVO_PAYLOAD)) {
        LOG_INFO("%s: Creating servo turnout extention object" CR, logContextName);
        turnType = TURN_TYPE_SERVO;
        throwtime = TURN_SERVO_DEFAULT_THROWTIME_MS;
        thrownTrim = TURN_SERVO_DEFAULT_THROW_TRIM;
        closedTrim = TURN_SERVO_DEFAULT_CLOSED_TRIM;
    }
    else if (!strcmp(p_subType, MQTT_TURN_SOLENOID_PAYLOAD)) {
        LOG_INFO("%s: Creating solenoid turnout extention object" CR, logContextName);
        turnType = TURN_TYPE_SOLENOID;
        throwtime = TURN_SOLENOID_DEFAULT_THROWTIME_MS;
    }
    if(!(actTurnLock = xSemaphoreCreateMutex())){
        panic("%s: Could not create Lock objects", logContextName);
        return;
    }
    TURN_DEFAULT_FAILSAFE == TURN_CLOSED_POS ? currentPwmVal = TURN_CLOSED_PWM_VAL : currentPwmVal = TURN_THROWN_PWM_VAL;
    orderedTurnOutPos = TURN_DEFAULT_FAILSAFE;
    turnOutFailsafePos = TURN_DEFAULT_FAILSAFE;
    failsafe(true);
}

actTurn::~actTurn(void) {
    panic("%s: actTurn destructor not supported", logContextName);
}

rc_t actTurn::init(void) {
    LOG_INFO("%s: Initializing actTurn actuator extention object" CR, logContextName);
    return RC_OK;
}

void actTurn::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("%s: Did not expect any configuration for turnout actuator extention object", logContextName);
}

rc_t actTurn::start(void) {
    LOG_INFO("%s: Starting actTurn extention object" CR, logContextName);
    return RC_OK;
}

void actTurn::onDiscovered(satellite* p_satelliteLibHandle, bool p_exists) {
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_TURN_TOPIC, "/", mqtt::getDecoderUri(), "/", sysName);
    if(p_exists){
        satLibHandle = p_satelliteLibHandle;
        LOG_INFO("%s: Subscribing to turn-out orders" CR, logContextName);
        if (mqtt::subscribeTopic(subscribeTopic, onActTurnChangeHelper, this)){
            panic("%s: Failed to suscribe to turn-out order topic \"%s\"", logContextName, subscribeTopic);
            return;
        }
        LOG_INFO("%s: actTurn extention object discovered" CR, logContextName);
        if (turnType == TURN_TYPE_SOLENOID) {
            if (actPort % 2) {
                turnSolenoidPushPort = false;
                LOG_INFO("%s: Startings solenoid turn-out pull port" CR, logContextName);
            }
            else {
                LOG_INFO("%s: Startings solenoid turn-out push port" CR, logContextName);
                turnSolenoidPushPort = true;
            }
            satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
        }
        else if (turnType == TURN_TYPE_SERVO) {
            LOG_INFO("%s: Startings servo turn-out" CR, logContextName);
            satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
            (TURN_CLOSED_PWM_VAL + closedTrim) > (TURN_THROWN_PWM_VAL + thrownTrim) ?
                pwmIncrements = ((TURN_CLOSED_PWM_VAL + closedTrim) - (TURN_THROWN_PWM_VAL + thrownTrim)) * TURN_PWM_UPDATE_TIME_MS / throwtime :
                pwmIncrements = ((TURN_THROWN_PWM_VAL + thrownTrim) - (TURN_CLOSED_PWM_VAL + closedTrim)) * TURN_PWM_UPDATE_TIME_MS / throwtime;
            turnServoPwmTimerArgs.arg = this;
            turnServoPwmTimerArgs.callback = reinterpret_cast<esp_timer_cb_t>(&actTurn::turnServoMoveHelper);
            turnServoPwmTimerArgs.dispatch_method = ESP_TIMER_TASK;
            turnServoPwmTimerArgs.name = "TurnServo %s timer", sysName;
            esp_timer_create(&turnServoPwmTimerArgs, &turnServoPwmIncrementTimerHandle);
        }
        else {
            satLibHandle = NULL;
            LOG_INFO("%s: UnSubscribing to turn-out orders" CR, logContextName);
            mqtt::unSubscribeTopic(subscribeTopic, onActTurnChangeHelper);
        }
    }
}

void actTurn::onSysStateChange(const uint16_t p_sysState) {
    char opState[100];
    LOG_INFO("%s: Got a new systemState %s" CR, logContextName, actBaseHandle->systemState::getOpStateStr(opState));
}

void actTurn::onActTurnChangeHelper(const char* p_topic, const char* p_payload, const void* p_actTurnHandle) {
    ((actTurn*)p_actTurnHandle)->onActTurnChange(p_topic, p_payload);
}

void actTurn::onActTurnChange(const char* p_topic, const char* p_payload) {
    xSemaphoreTake(actTurnLock, portMAX_DELAY);
    if (!strcmp(p_payload, MQTT_TURN_CLOSED_PAYLOAD)) {
        LOG_INFO("%s: Got a close turnout change order" CR, logContextName);
        orderedTurnOutPos = TURN_CLOSED_POS;
        if (!failSafe && !moving){
            turnOutPos = TURN_CLOSED_POS;
            moving = true;
        }
    }
    else if (!strcmp(p_payload, MQTT_TURN_THROWN_PAYLOAD)) {
        LOG_INFO("%s: Got a throw turnout change order" CR, logContextName);
        orderedTurnOutPos = TURN_THROWN_POS;
        if (!failSafe && !moving) {
            turnOutPos = TURN_THROWN_POS;
            moving = true;
        }
    }
    else
        LOG_ERROR("%s: Got an invalid turnout change order" CR, logContextName);
    xSemaphoreGive(actTurnLock);
    setTurn();
}

void actTurn::setTurn(void) {
    if((satLibHandle != NULL) && !failSafe){
        if (turnType == TURN_TYPE_SOLENOID) {
            if (turnOutPos ^ turnOutInvert ^ turnSolenoidPushPort) {
                satLibHandle->setSatActVal(throwtime, actPort);
                if (turnOutPos != orderedTurnOutPos) {
                    turnOutPos = orderedTurnOutPos;
                    setTurn();
                    moving = false;
                    return;
                }
            }
            moving = false;
            LOG_INFO("%s: Turnout change order fininished" CR, logContextName);
        }
        else if (turnType == TURN_TYPE_SERVO)
            turnServoMove();
    }
}

void actTurn::turnServoMoveHelper(actTurn* p_actTurnHandle) {
    p_actTurnHandle->turnServoMove();
}

void actTurn::turnServoMove(void) {
    bool continueTurnServo = true;
    if (turnOutPos == TURN_CLOSED_POS) {
        if (currentPwmVal > TURN_CLOSED_PWM_VAL + closedTrim) {
            currentPwmVal -= pwmIncrements;
            if (currentPwmVal <= TURN_CLOSED_PWM_VAL + closedTrim) {
                currentPwmVal = TURN_CLOSED_PWM_VAL + closedTrim;
                continueTurnServo = false;
            }
        }
        else {
            currentPwmVal += pwmIncrements;
            if (currentPwmVal >= TURN_CLOSED_PWM_VAL + closedTrim) {
                currentPwmVal = TURN_CLOSED_PWM_VAL + closedTrim;
                continueTurnServo = false;
            }
        }
    }
    if (turnOutPos == TURN_THROWN_POS) {
        if (currentPwmVal < TURN_THROWN_PWM_VAL + thrownTrim) {
            currentPwmVal += pwmIncrements;
            if (currentPwmVal >= TURN_THROWN_PWM_VAL + thrownTrim) {
                currentPwmVal = TURN_THROWN_PWM_VAL + thrownTrim;
                continueTurnServo = false;
            }
        }
        else {
            currentPwmVal -= pwmIncrements;
            if (currentPwmVal <= TURN_THROWN_PWM_VAL + thrownTrim) {
                currentPwmVal = TURN_THROWN_PWM_VAL + thrownTrim;
                continueTurnServo = false;
            }
        }
    }
    satLibHandle->setSatActVal(currentPwmVal, actPort);
    if (continueTurnServo)
        esp_timer_start_once(turnServoPwmIncrementTimerHandle, TURN_PWM_UPDATE_TIME_MS * 1000);
    else{
        if (turnOutPos != orderedTurnOutPos) {
            turnOutPos = orderedTurnOutPos;
            turnServoMove();
            moving = false;
            return;
        }
    moving = false;
    }
}

void actTurn::failsafe(bool p_failSafe) {
    xSemaphoreTake(actTurnLock, portMAX_DELAY);
    if (p_failSafe) {
        LOG_INFO("Fail-safe set" CR, logContextName);
        turnOutPos = turnOutFailsafePos;
        setTurn();
        failSafe = p_failSafe;
    }
    else {
        LOG_INFO("%s: Fail-safe un-set" CR, logContextName);
        turnOutPos = orderedTurnOutPos;
        failSafe = p_failSafe;
        setTurn();
    }
    xSemaphoreGive(actTurnLock);
}

rc_t actTurn::setProperty(uint8_t p_propertyId, const char* p_propertyVal){
    LOG_INFO("%s: Setting of Turn property not implemented" CR, logContextName);
    return RC_NOTIMPLEMENTED_ERR;
}

rc_t actTurn::setShowing(const char* p_showing) {
    if (!strcmp(p_showing, MQTT_TURN_THROWN_PAYLOAD)) {
        onActTurnChange(NULL, MQTT_TURN_THROWN_PAYLOAD);
        return RC_OK;
    }
    if (!strcmp(p_showing, MQTT_TURN_CLOSED_PAYLOAD)) {
        onActTurnChange(NULL, MQTT_TURN_CLOSED_PAYLOAD);
        return RC_OK;
    }
    return RC_PARAMETERVALUE_ERR;
}

rc_t actTurn::getShowing(char* p_showing, char* p_orderedShowing) {
    xSemaphoreTake(actTurnLock, portMAX_DELAY);
    strcpy(p_showing, turnOutPos? MQTT_TURN_THROWN_PAYLOAD : MQTT_TURN_CLOSED_PAYLOAD);
    strcpy(p_orderedShowing, turnOutPos ? MQTT_TURN_THROWN_PAYLOAD : MQTT_TURN_CLOSED_PAYLOAD);
    xSemaphoreGive(actTurnLock);
    return RC_OK;
}

/*==============================================================================================================================================*/
/* END Class actTurn                                                                                                                            */
/*==============================================================================================================================================*/
