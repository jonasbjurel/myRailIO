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
//#include <dummy.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <Effortless_SPIFFS.h>
#include <FS.h>
#include <FSImpl.h>
#include <vfs_api.h>
#include <dummy.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <WiFiGeneric.h>
#include <WiFiMulti.h>
#include <WiFiScan.h>
#include <WiFiServer.h>
#include <WiFiSTA.h>
#include <WiFiType.h>
#include <WiFiUdp.h>
#include <wm_strings_en.h>
#include <wm_consts_en.h>
#include <WiFiManager.h>
#include <strings_en.h>
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
    setupRunning = true;
    Serial.begin(115200);
    //while (!Serial.available()) {}
    networking::provisioningConfigTrigger();
    xTaskCreatePinnedToCore(                    // Spinning up a setupTask task as wee need a bigger stack than set-up provides
        setupTask,                              // Task function
        SETUP_TASKNAME,                         // Task function name reference
        50 * 1024,                             // Stack size 6K VERIFIED FOR THE NETWORKING SERVICE, NEEDS TO BE EVALUATED FOR ALL OTHER SERVICES AND EVENTUALLY DEFINED BY SETUP_STACKSIZE_1K
        NULL,                                   // Parameter passing
        SETUP_PRIO,                             // Priority 0-24, higher is more
        NULL,                                   // Task handle
        SETUP_CORE);                            // Core [CORE_0 | CORE_1]
    while (setupRunning)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    Log.notice("setup: Starting the runtime web-portal" CR);
    wifiManager = new WiFiManager;
    wifiManager->setTitle(WIFI_MGR_HTML_TITLE);
    wifiManager->setShowStaticFields(true);
    wifiManager->setShowDnsFields(true);
    wifiManager->setShowInfoErase(true);
    wifiManager->setShowInfoUpdate(true);
    wifiManager->startWebPortal();
    Log.notice("genJMRIDecoderClient::setup: Initial setup has successfully concluded, handing over to \"Arduino loop\"" CR);
}

void setupTask(void* p_dummy) {
    networking::provisioningConfigTrigger();
    Log.notice("genJMRIDecoderClient::setupTask: setupTask started" CR);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    //  Log.setPrefix(printPrefix); // set prefix similar to NLog
    Log.notice("genJMRIDecoderClient::setupTask: Logging service started towards Serial" CR);
    fileSys::start();
    Log.notice("genJMRIDecoderClient::setupTask: File system service started" CR);
    networking::start();
    Log.notice("genJMRIDecoderClient::setupTask: WIFI Networking service started" CR);
    Log.notice("genJMRIDecoderClient::setupTask: Connecting to WIFI..." CR);
    while (networking::getOpState() != OP_WORKING) {
        Log.notice("Waiting for WIFI to connect, current WiFi OP state: %X" CR, networking::getOpState());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    Log.INFO("setupTask: Connected to WIFI and operational, got IP Address %s" CR, networking::getIpAddr().toString().c_str());
    Log.notice("genJMRIDecoderClient::setupTask: Initializing time- and NTP- service" CR);
    esp_timer_init();
    ntpTime::init();
    Log.INFO("setupTask: Time- and NTP- service initialized" CR);
    decoderHandle = new decoder();
    //decoderHandle->init();
    //decoderHandle->start();
    setupRunning = false;
    Log.notice("genJMRIDecoderClient::setupTask: Setup finished, killing setup task..." CR);
    vTaskDelete(NULL);
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
    wifiManager->process(); //WHY PROCESS WIFIMANAGER FROM HERE?
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
