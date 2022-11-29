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

#ifndef WDT_H
#define WDT_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "panic.h" 
#include "libraries/QList/src/QList.h"
#include "strHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define FAULTACTION_REBOOT                  1<<7        // Reboots the entire decoder
#define FAULTACTION_DUMP_ALL                1<<6        // Tries to dump as much decoder information as possible
#define FAULTACTION_DUMP_LOCAL              1<<5        // Dumps information from the object at fault
#define FAULTACTION_MQTTDOWN                1<<4        // Brings down MQTT
#define FAULTACTION_FREEZE                  1<<3        // Freezes the state of the decoder, inhibits MQTT bring down and reboot 
#define FAULTACTION_FAILSAFE_SENSORS        1<<2        // Fail safes all sensors
#define FAULTACTION_FAILSAFE_LGS            1<<1        // Fail safes all Light groups
#define FAULTACTION_FAILSAFE_ACTUATORS      1<<0        // Fail safes all actuators
#define FAULTACTION_FAILSAFE_ALL            FAULTACTION_FAILSAFE_LGS|FAULTACTION_FAILSAFE_SENSORS|FAULTACTION_FAILSAFE_ACTUATORS // Fail safe alias for all of the decoders
class wdt;

typedef void(wdtCb_t)(void* p_parms);

struct wdt_t {
    long unsigned int wdtTimeout;
    esp_timer_handle_t timerHandle;
    char* wdtDescription;
    uint8_t wdtAction;
};

struct wdtCbParms_t {
    wdtCb_t* wdtCb;
    void* wdtCbHandle;
};

struct wdtCbLists_t {
    QList<wdtCbParms_t*> lgsWdtCbs;
    QList<wdtCbParms_t*> sensorsWdtCbs;
    QList<wdtCbParms_t*> actuatorsWdtCbs;
    QList<wdtCbParms_t*> mqttDownWdtCbs;
};

class wdt {
public:
    //methods
    wdt(uint16_t p_wdtTimeout, const char* p_wdtDescription, uint8_t p_wdtAction);
    ~wdt(void);
    static void wdtRegLgFailsafe(wdtCb_t* p_wdtLgFailsaveCb, void* p_wdtCbParms);
    static void wdtUnRegLgFailsafe(wdtCb_t* p_wdtLgFailsaveCb);
    static void wdtRegSensorFailsafe(wdtCb_t* p_wdtSensorFailsaveCb, void* p_wdtCbParms);
    static void wdtUnRegSensorFailsafe(wdtCb_t* p_wdtSensorFailsaveCb);
    static void wdtRegActuatorFailsafe(wdtCb_t* p_wdtActuatorFailsaveCb, void* p_wdtCbParms);
    static void wdtUnRegActuatorFailsafe(wdtCb_t* p_wdtActuatorFailsaveCb);
    static void wdtRegMqttDown(wdtCb_t* p_wdtMqttDownCb, void* p_cbHandle);
    static void wdtUnRegMqttDown(wdtCb_t* p_wdtMqttDownCb);
    void feed(void);
    static void kickHelper(wdt* p_wdtObject);

    //Data structures
    //--

private:
    //methods
    void kick(void);
    static void callFailsafeCbs(QList<wdtCbParms_t*>* p_wdtCbList);

    //Data structures
    wdt_t* wdtData;
    esp_timer_create_args_t wdtTimer_args;
    static wdtCbLists_t wdtCbLists;
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/

#endif //WDT_H
