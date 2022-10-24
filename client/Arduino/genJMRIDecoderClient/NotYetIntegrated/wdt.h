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
#define FAULTACTION_MQTTDOWN                1<<6        // Brings down MQTT
#define FAULTACTION_FREEZE                  1<<5        // Freezes the state of the decoder, inhibits MQTT bring down and reboot 
#define FAULTACTION_FAILSAFE_LGS            1<<4        // Fail safes the Light groups channels decoders
#define FAULTACTION_FAILSAFE_SENSORS        1<<3        // Fail safes the sensor decoders
#define FAULTACTION_FAILSAFE_TURNOUTS       1<<2        // Fail safes the turnout decoders
#define FAULTACTION_FAILSAFE_ALL            FAULTACTION_FAILSAFE_LGS|FAULTACTION_FAILSAFE_SENSORS|FAULTACTION_FAILSAFE_TURNOUTS // Fail safe alias for all of the decoders
#define FAULTACTION_DUMP_ALL                1<<1        // Tries to dump as much decoder information as possible
#define FAULTACTION_DUMP_LOCAL              1           // Dumps information from the object at fault

struct wdt_t {
    long unsigned int wdtTimeout;
    esp_timer_handle_t timerHandle;
    char* wdtDescription;
    uint8_t wdtAction;
};

class wdt {
public:
    //methods
    wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction);
    ~wdt(void);
    void feed(void);

    //Data structures
    //--

private:
    //methods
    static void kickHelper(wdt* p_wdtObject);
    void kick(void);

    //Data structures
    wdt_t* wdtData;
    esp_timer_create_args_t wdtTimer_args;
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/

#endif //WDT_H
