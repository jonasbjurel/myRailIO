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
#include "genJMRIDecoderClient.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*=============================================================================================================================================*/
void setup() {
    setupRunning = true;
    Serial.begin(115200);
    Serial.printf("setup: Free Heap: %i\n", esp_get_free_heap_size());
    Serial.printf("setup: Heap watermark: %i\n", esp_get_minimum_free_heap_size());
    networking::provisioningConfigTrigger();
    xTaskCreatePinnedToCore(                    // Spinning up a setupTask task as wee need a bigger stack than set-up provides
        setupTask,                              // Task function
        SETUP_TASKNAME,                         // Task function name reference
        6 * 1024,                              // Stack size 6K VERIFIED FOR THE NETWORKING SERVICE, NEEDS TO BE EVALUATED FOR ALL OTHER SERVICES AND EVENTUALLY DEFINED BY SETUP_STACKSIZE_1K
        NULL,                                   // Parameter passing
        SETUP_PRIO,                             // Priority 0-24, higher is more
        NULL,                                   // Task handle
        SETUP_CORE);                            // Core [CORE_0 | CORE_1]
    while (setupRunning)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    Log.INFO("genJMRIDecoderClient::setup: Initial setup has successfully concluded, handing over to \"Arduino loop\"" CR);
}

void setupTask(void* p_dummy) {
    Log.INFO("genJMRIDecoderClient::setupTask: setupTask started" CR);
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);
    //  Log.setPrefix(printPrefix); // set prefix similar to NLog
    Log.INFO("genJMRIDecoderClient::setupTask: Logging service started towards Serial" CR);
    fileSys::start();
    Log.INFO("genJMRIDecoderClient::setupTask: File system service started" CR);
    networking::start();
    Log.INFO("genJMRIDecoderClient::setupTask: WIFI Networking service started" CR);
    Log.INFO("genJMRIDecoderClient::setupTask: Connecting to WIFI..." CR);
    while (networking::getOpState() != OP_WORKING) {
        Log.INFO("Waiting for WIFI to connect, current WiFi OP state: %X" CR, networking::getOpState());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    Log.INFO("setupTask: Connected to WIFI and operational, got IP Address %s" CR, networking::getIpAddr().toString().c_str());
    Log.INFO("genJMRIDecoderClient::setupTask: Initializing time- and NTP- service" CR);
    esp_timer_init();
    ntpTime::init();
    Log.INFO("genJMRIDecoderClient::setupTask: Time- and NTP- service initialized" CR);
    Log.INFO("genJMRIDecoderClient::setupTask: Starting the runtime web-portal service" CR);
    wifiManager = new WiFiManager;
    wifiManager->setTitle(WIFI_MGR_HTML_TITLE);
    wifiManager->setShowStaticFields(true);
    wifiManager->setShowDnsFields(true);
    wifiManager->setShowInfoErase(true);
    wifiManager->setShowInfoUpdate(true);
    wifiManager->startWebPortal();
    Log.INFO("genJMRIDecoderClient::setupTask: Runtime web-portal service started" CR);
    Log.INFO("genJMRIDecoderClient::setupTask: Starting the decoder service" CR);
    decoderHandle = new decoder();
    decoderHandle->init();
    decoderHandle->start();
    Log.INFO("genJMRIDecoderClient::setupTask: Decoder service started" CR);
    setupRunning = false;
    Log.INFO("genJMRIDecoderClient::setupTask: Setup finished, killing setup task..." CR);
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
