/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonas.bjurel@hotmail.com)
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

#ifndef ACTLIGHT_H
#define ACTLIGHT_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "systemState.h"
#include "actBase.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
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
class actLight {
public:
    //Public methods
    actLight(actBase* p_actBaseHandle, const char* p_type, char* p_subType);
    ~actLight(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    rc_t init(void);
    void onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement);
    rc_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    void onDiscovered(satelite* p_sateliteLibHandle);
    void onSysStateChange(uint16_t p_sysState);
    static void onActLightChangeHelper(const char* p_topic, const char* p_payload, const void* p_actLightHandle);
    rc_t setProperty(uint8_t p_propertyId, const char* p_propertyVal);
    rc_t getProperty(uint8_t p_propertyId, char* p_propertyVal);
    rc_t setShowing(const char* p_showing);
    rc_t getShowing(char* p_showing, char* p_orderedShowing);

    //Public data structures
    //--

private:
    //Private methods
    void onActLightChange(const char* p_topic, const char* p_payload);
    void setActLight(void);
    void setFailSafe(bool p_failSafe);

    //Private data structures
    actBase* actBaseHandle;
    uint16_t sysState;
    const char* sysName;
    uint8_t actPort;
    uint8_t satAddr;
    uint8_t satLinkNo;
    satelite* satLibHandle;
    SemaphoreHandle_t actLightLock;
    bool pendingStart;
    uint8_t actLightPos;
    uint8_t orderedActLightPos;
    uint8_t actLightFailsafePos;
    bool failSafe;
};

/*==============================================================================================================================================*/
/* END Class actLight                                                                                                                           */
/*==============================================================================================================================================*/
#endif /*ACTLIGHT_H*/