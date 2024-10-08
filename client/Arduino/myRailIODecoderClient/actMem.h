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

#ifndef ACTMEM_H
#define ACTMEM_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "actBase.h"
#include "libraries/genericIOSatellite/LIB/src/Satellite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "logHelpers.h"

class actBase;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "actMem (actuator memory DNA class)"                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define ACTMEM_TYPE_SOLENOID                        0
#define ACTMEM_TYPE_SERVO                           1
#define ACTMEM_TYPE_PWM100                          2
#define ACTMEM_TYPE_PWM125K                         3
#define ACTMEM_TYPE_ONOFF                           4
#define ACTMEM_TYPE_PULSE                           5

typedef uint8_t actMemType_t;

class actMem {
public:
    //Public methods
    actMem(actBase* p_actBaseHandle, const char* p_type, char* p_subType);
    ~actMem(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    rc_t init(void);
    void onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement);
    rc_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    void onDiscovered(satellite* p_satelliteLibHandle, bool p_exists);
    void onSysStateChange(sysState_t p_sysState);
    static void onActMemChangeHelper(const char* p_topic, const char* p_payload, const void* p_actMemHandle);
    rc_t setProperty(uint8_t p_propertyId, const char* p_propertyVal);
    rc_t setShowing(const char* p_showing);
    rc_t getShowing(char* p_showing, char* p_orderedShowing);
    void failsafe(bool p_failSafe);


    //Public data structures
    //--

private:
    //Private methods
    void onActMemChange(const char* p_topic, const char* p_payload);
    void setActMem(void);

    //Private data structures
    EXT_RAM_ATTR char* logContextName;
    EXT_RAM_ATTR actBase* actBaseHandle;
    EXT_RAM_ATTR char sysName[50];
    EXT_RAM_ATTR uint8_t actPort;
    EXT_RAM_ATTR uint8_t satAddr;
    EXT_RAM_ATTR uint8_t satLinkNo;
    EXT_RAM_ATTR satellite* satLibHandle;
    SemaphoreHandle_t actMemLock;
    EXT_RAM_ATTR actMemType_t actMemType;
    EXT_RAM_ATTR uint8_t actMemPos;
    EXT_RAM_ATTR uint8_t orderedActMemPos;
    EXT_RAM_ATTR uint8_t actMemFailsafePos;
    EXT_RAM_ATTR bool actMemSolenoidPushPort;
    EXT_RAM_ATTR uint8_t actMemSolenoidActivationTime;
    EXT_RAM_ATTR bool failSafe;
};

/*==============================================================================================================================================*/
/* END Class actMem                                                                                                                        */
/*==============================================================================================================================================*/
#endif /*ACTMEM_H*/
