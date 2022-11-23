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

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "panic.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



void panic(const char* p_panicMessage) {
    Log.fatal(p_panicMessage);
//    decoderHandle->setOpState(OP_INTFAIL);
    Log.info("panic: Waiting 5 seconds before restaritng - enabling spool-out of syslog, fail-safe settings, etc");
    TimerHandle_t rebootTimer;
    rebootTimer = xTimerCreate("rebootTimer",                       // Just a text name, not used by the kernel.
        (5000 / portTICK_PERIOD_MS),                                // The timer period in ticks.
        pdFALSE,                                                    // The timer will not auto-reload when expired.
        NULL,                                                       // param passing.
        reboot                                                      // Each timer calls the same callback when it expires.
    );
    if (rebootTimer == NULL) {
        Log.fatal("panic: Could not create reboot timer - rebooting immediatly..." CR);
        ESP.restart();
    }
    if (xTimerStart(rebootTimer, 0) != pdPASS) {
        Log.fatal("panic: Could not start reboot timer - rebooting immediatly..." CR);
        ESP.restart();
    }
}

void reboot(void* p_args) {
    Log.fatal("panic: Delayed reboot..." CR);
    ESP.restart();
}
