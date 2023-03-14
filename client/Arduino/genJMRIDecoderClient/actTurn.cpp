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
    actBaseHandle = p_actBaseHandle;
    if (!strcmp(p_subType, MQTT_TURN_SERVO_PAYLOAD)) {
        turnType = TURN_TYPE_SERVO;
        throwtime = TURN_SERVO_DEFAULT_THROWTIME_MS;
        thrownTrim = TURN_SERVO_DEFAULT_THROW_TRIM;
        closedTrim = TURN_SERVO_DEFAULT_CLOSED_TRIM;
    }
    else if (!strcmp(p_subType, MQTT_TURN_SOLENOID_PAYLOAD)) {
        turnType = TURN_TYPE_SOLENOID;
        throwtime = TURN_SOLENOID_DEFAULT_THROWTIME_MS;
    }
    actBaseHandle->getPort(&actPort);
    actBaseHandle->satHandle->getAddr(&satAddr);
    actBaseHandle->satHandle->linkHandle->getLink(&satLinkNo);
    sysName = actBaseHandle->satHandle->getSystemName();
    satLibHandle = NULL;
    pendingStart = false;
    turnOutInvert = false;
    turnSolenoidPushPort = true;
    sysState = OP_INIT | OP_UNCONFIGURED;
    Log.notice("actTurn::actTurn: Creating turnout extention object for %s turnout for actuator port %d, on satelite adress %d, satLink %d" CR, turnType, actPort, satAddr, satLinkNo);
    if(!(actTurnLock = xSemaphoreCreateMutex()))
        panic("actTurn::actTurn: Could not create Lock objects - rebooting...");
    turnOutPos = TURN_DEFAULT_FAILSAFE;
    orderedTurnOutPos = TURN_DEFAULT_FAILSAFE;
    turnOutFailsafePos = TURN_DEFAULT_FAILSAFE;
}

actTurn::~actTurn(void) {
    panic("actTurn::~actTurn: actTurn destructor not supported - rebooting...");
}

rc_t actTurn::init(void) {
    Log.notice("actTurn::init: Initializing actTurn actuator extention object for turnout %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    return RC_OK;
}

void actTurn::onConfig(const tinyxml2::XMLElement* p_actExtentionXmlElement) {
    panic("actTurn::onConfig: Did not expect any configuration for turnout actuator extention object - rebooting...");
}

rc_t actTurn::start(void) {
    Log.notice("actTurn::start: Starting actTurn actuator extention object %s, on actuator port% d, on satelite adress% d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->systemState::getOpState() & OP_UNCONFIGURED) {
        Log.notice("actTurn::start: actTurn actuator extention object %s, on actuator port %d, on satelite adress %d, satLink %d not configured - will not start it" CR, sysName, actPort, satAddr, satLinkNo);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (actBaseHandle->systemState::getOpState() & OP_UNDISCOVERED) {
        Log.notice("actTurn::start: actTurn actuator extention class object %s, on actuator port %d, on satelite adress %d, satLink %d not yet discovered - waiting for discovery before starting it" CR, sysName, actPort, satAddr, satLinkNo);
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("actTurn::start: Configuring and startings actTurn extention class object %s, on actuator port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    if (turnType == TURN_TYPE_SOLENOID) {
        if (actPort % 2) {
            turnSolenoidPushPort = false;
            Log.notice("actTurn::start: Startings solenoid turn-out pull port %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        }
        else {
            Log.notice("actTurn::start: Startings solenoid turn-out push port %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
            turnSolenoidPushPort = true;
        }
        satLibHandle->setSatActMode(SATMODE_PULSE, actPort);
    }
    else if (turnType == TURN_TYPE_SERVO) {
        Log.notice("actTurn::start: Startings servo turn-out %s, on port %d, on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
        satLibHandle->setSatActMode(SATMODE_PWM100, actPort);
        pwmIncrements = abs((TURN_CLOSED_PWM_VAL + closedTrim) - (TURN_THROWN_PWM_VAL + thrownTrim)) * throwtime / TURN_PWM_UPDATE_TIME_MS;
        turnServoPwmTimerArgs.arg = this;
        turnServoPwmTimerArgs.callback = reinterpret_cast<esp_timer_cb_t>(&actTurn::turnServoMoveHelper);
        turnServoPwmTimerArgs.dispatch_method = ESP_TIMER_TASK;
        turnServoPwmTimerArgs.name = "TurnServo %s timer", sysName;
        esp_timer_create(&turnServoPwmTimerArgs, &turnServoPwmIncrementTimerHandle);
    }
    setFailSafe(true);
    Log.notice("actTurn::start: Subscribing to turn-out orders for turn-out %s,  on satelite adress %d, satLink %d" CR, sysName, actPort, satAddr, satLinkNo);
    const char* turnoutOrders[3] = { MQTT_TURN_TOPIC, "/", sysName };
    if (mqtt::subscribeTopic(concatStr(turnoutOrders, 3), &onActTurnChangeHelper, this))
        panic("actTurn::start: Failed to suscribe to turn-out order topic - rebooting...");
    return RC_OK;
}

void actTurn::onDiscovered(satelite* p_sateliteLibHandle) {
    satLibHandle = p_sateliteLibHandle;
    Log.notice("actTurn::onDiscovered: actTurn extention class object %s, on actuator port %d, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
    if (actBaseHandle->pendingStart) {
        Log.notice("actTurn::onDiscovered: Initiating pending start for actTurn extention class object %s on actuator port %d, on satelite adress %d, satLink %d discovered" CR, sysName, actPort, satAddr, satLinkNo);
        start();
    }
}

void actTurn::onSysStateChange(const uint16_t p_sysState) {
    sysState = p_sysState;
    Log.notice("actTurn::onSystateChange: Got a new systemState %d for actTurn extention class object for actuator port %d, on satelite adress %d, satLink %d" CR, sysState, actPort, satAddr, satLinkNo);
    if (sysState)
        setFailSafe(true);
    else
        setFailSafe(false);
}

void actTurn::onActTurnChangeHelper(const char* p_topic, const char* p_payload, const void* p_actTurnHandle) {
    ((actTurn*)p_actTurnHandle)->onActTurnChange(p_topic, p_payload);
}

void actTurn::onActTurnChange(const char* p_topic, const char* p_payload) {
    xSemaphoreTake(actTurnLock, portMAX_DELAY);
    if (strcmp(p_payload, MQTT_TURN_CLOSED_PAYLOAD)) {
        Log.notice("senseDigital::onTurnChange: Got a close turnout change order for turnout %s" CR, sysName);
        orderedTurnOutPos = TURN_CLOSED_POS;
        if (!failSafe)
            turnOutPos = TURN_CLOSED_POS;
    }
    else if (strcmp(p_payload, MQTT_TURN_THROWN_PAYLOAD)) {
        Log.notice("senseDigital::onTurnChange: Got a throw turnout change order for turnout %s" CR, sysName);
        orderedTurnOutPos = TURN_THROWN_POS;
        if (!failSafe)
            turnOutPos = TURN_THROWN_POS;
    }
    else
        Log.ERROR("senseDigital::onTurnChange: Got an invalid turnout change order for turnout %s" CR, sysName);
    xSemaphoreGive(actTurnLock);
    setTurn();
}

void actTurn::setTurn(void) {
    if (turnType == TURN_TYPE_SOLENOID) {
        if (turnOutPos ^ turnOutInvert ^ turnSolenoidPushPort) {
            satLibHandle->setSatActVal(throwtime, actPort);
            Log.notice("actTurn::turnServo: Turnout change order for turnout %s fininished" CR, sysName);
        }
    }
    else if (turnType == TURN_TYPE_SERVO) {
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
            currentPwmVal += TURN_CLOSED_PWM_VAL + closedTrim;
            if (currentPwmVal >= TURN_CLOSED_PWM_VAL + closedTrim) {
                currentPwmVal = TURN_CLOSED_PWM_VAL + closedTrim;
                continueTurnServo = false;
            }
        }
    }
    if (turnOutPos == TURN_THROWN_POS) {
        if (currentPwmVal > TURN_THROWN_PWM_VAL + thrownTrim) {
            currentPwmVal -= pwmIncrements;
            if (currentPwmVal <= TURN_THROWN_PWM_VAL + thrownTrim) {
                currentPwmVal = TURN_THROWN_PWM_VAL + thrownTrim;
                continueTurnServo = false;
            }
        }
        else {
            currentPwmVal += TURN_THROWN_PWM_VAL + thrownTrim;
            if (currentPwmVal >= TURN_THROWN_PWM_VAL + thrownTrim) {
                currentPwmVal = TURN_THROWN_PWM_VAL + thrownTrim;
                continueTurnServo = false;
            }
        }
    }
    satLibHandle->setSatActVal(currentPwmVal, actPort);
    if (continueTurnServo)
        esp_timer_start_once(turnServoPwmIncrementTimerHandle, TURN_PWM_UPDATE_TIME_MS * 1000);
}

void actTurn::setFailSafe(bool p_failSafe) {
    xSemaphoreTake(actTurnLock, portMAX_DELAY);
    failSafe = p_failSafe;
    if (failSafe) {
        Log.notice("actTurn::setFailSafe: Fail-safe set for turnout %s" CR, sysName);
        turnOutPos = turnOutFailsafePos;
    }
    else {
        Log.notice("actTurn::setFailSafe: Fail-safe un-set for turnout %s" CR, sysName);
        turnOutPos = orderedTurnOutPos;
    }
    setTurn();
    xSemaphoreGive(actTurnLock);
}

rc_t actTurn::setProperty(uint8_t p_propertyId, const char* p_propertyVal){
    Log.notice("actTurn::setProperty: Setting of Turn property not implemented" CR);
    return RC_NOTIMPLEMENTED_ERR;
}
rc_t actTurn::getProperty(uint8_t p_propertyId, char* p_propertyVal){
    Log.notice("actTurn::getProperty: Getting of Turn property not implemented" CR);
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
    if (turnOutPos)
        p_showing = (char*)MQTT_TURN_THROWN_PAYLOAD;
    else
        p_showing = (char*)MQTT_TURN_CLOSED_PAYLOAD;
    if (orderedTurnOutPos)
        p_orderedShowing = (char*)MQTT_TURN_THROWN_PAYLOAD;
    else
        p_orderedShowing = (char*)MQTT_TURN_CLOSED_PAYLOAD;
    xSemaphoreGive(actTurnLock);
    return RC_OK;
}

/*==============================================================================================================================================*/
/* END Class actTurn                                                                                                                            */
/*==============================================================================================================================================*/
