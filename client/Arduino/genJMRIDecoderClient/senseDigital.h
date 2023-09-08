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



#ifndef SENSDIGITAL_H
#define SENSDIGITAL_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "senseBase.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "logHelpers.h"

class senseBase;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: senseDigital                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
class senseDigital {
public:
    //Public methods
    senseDigital(senseBase* p_sensBaseHandle);
    ~senseDigital(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    rc_t init(void);
    void onConfig(const tinyxml2::XMLElement* p_sensExtentionXmlElement);
    rc_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    void onDiscovered(satelite* p_sateliteLibHandle, bool p_exists);
    void onSysStateChange(uint16_t p_sysState);
    void onSenseChange(bool p_senseVal);
    void failsafe(bool p_failSafe);
    void getSensing(char* p_sensing);
    rc_t setProperty(uint8_t p_propertyId, const char* p_propertyValue);
    rc_t getProperty(uint8_t p_propertyId, const char* p_propertyValue);
    void setDebug(bool p_debug);
    bool getDebug(void);
    bool getFilteredSense(void);
    bool getUnFilteredSense(void);

    //Public data structures
    //--

private:
    //Private methods
    //--

    //Private data structures
    senseBase* senseBaseHandle;
    uint8_t sensPort;
    uint8_t satAddr;
    uint8_t satLinkNo;
    satelite* satLibHandle;
    bool failSafe;
    const char* sensSysName;
    SemaphoreHandle_t senseDigitalLock;
    uint16_t sysState;
    bool filteredSenseVal;
    bool debug;
};

/*==============================================================================================================================================*/
/* END Class senseDigital                                                                                                                       */
/*==============================================================================================================================================*/
#endif /*SENSDIGITAL_H*/
