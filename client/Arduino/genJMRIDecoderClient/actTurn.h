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

#ifndef ACTTURN_H
#define ACTTURN_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "actBase.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "logHelpers.h"

class actBase;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actTurn (Turnout actuator DNA class)"                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define TURN_TYPE_SERVO                             0
#define TURN_TYPE_SOLENOID                          1

#define TURN_CLOSED_POS                             0
#define TURN_THROWN_POS                             1

#define TURN_CLOSED_PWM_VAL                         SERVO_LEFT_PWM_VAL
#define TURN_THROWN_PWM_VAL                         SERVO_RIGHT_PWM_VAL

typedef uint8_t turnType_t;
typedef uint16_t throwtime_t;
typedef uint8_t pwm_t; //To be moved to the satelite LIB

class actTurn {
public:
    //Public methods
    actTurn(actBase* p_senseBaseHandle, const char* p_type, char* p_subType);
    ~actTurn(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    rc_t init(void);
    void onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement);
    rc_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    void onDiscovered(satelite* p_sateliteLibHandle, bool p_exists);
    void onSysStateChange(uint16_t p_sysState);
    static void onActTurnChangeHelper(const char* p_topic, const char* p_payload, const void* p_actTurnHandle);
    static void turnServoMoveHelper(actTurn* p_actTurnHandle);
    rc_t setProperty(uint8_t p_propertyId, const char* p_propertyVal);
    rc_t getProperty(uint8_t p_propertyId, char* p_propertyVal);
    rc_t setShowing(const char* p_showing);
    rc_t getShowing(char* p_showing, char* p_orderedShowing);
    void failsafe(bool p_failSafe);


    //Public data structures
    //--

private:
    //Private methods
    void onActTurnChange(const char* p_topic, const char* p_payload);
    void setTurn(void);
    void turnServoMove(void);

    //Private data structures
    char* logContextName;
    actBase* actBaseHandle;
    esp_timer_handle_t turnServoPwmIncrementTimerHandle;
    esp_timer_create_args_t turnServoPwmTimerArgs;
    const char* sysName;
    uint8_t actPort;
    uint8_t satAddr;
    uint8_t satLinkNo;
    satelite* satLibHandle;
    SemaphoreHandle_t actTurnLock;
    turnType_t turnType;
    uint8_t turnOutPos;
    uint8_t orderedTurnOutPos;
    uint8_t turnOutFailsafePos;
    bool turnOutInvert;
    bool turnSolenoidPushPort;
    throwtime_t throwtime;
    uint8_t currentPwmVal;
    pwm_t thrownTrim;
    pwm_t closedTrim;
    pwm_t pwmIncrements;
    bool failSafe;
};

/*==============================================================================================================================================*/
/* END Class actTurn                                                                                                                            */
/*==============================================================================================================================================*/
#endif /*ACTTURN_H*/
