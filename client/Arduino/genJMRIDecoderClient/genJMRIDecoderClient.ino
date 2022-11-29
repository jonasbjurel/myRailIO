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
#include <dummy.h>
#include "genJMRIDecoderClient.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    while (!Serial && !Serial.available()) {}
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    //  Log.setPrefix(printPrefix); // set prefix similar to NLog
    Log.notice("Logging started towards Serial" CR);
    //CPU.init();
    networking::start();
    uint8_t wifiWait = 0;
    while (networking::getOpState() != OP_WORKING) {
        if (wifiWait >= 60) {
            Log.fatal("Could not connect to wifi - rebooting..." CR);
            ESP.restart();
        }
        else {
            Log.notice("Waiting for WIFI to connect" CR);
        }
        wifiWait++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    Log.notice("WIFI connected" CR);
    //CLI.init();
    decoderHandle = new decoder();
    decoderHandle->init();
}

/*==============================================================================================================================================*/
/* END setup                                                                                                                                    */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: loop                                                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void loop() {
    // put your main code here, to run repeatedly:
    // Serial.print("Im in the background\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
