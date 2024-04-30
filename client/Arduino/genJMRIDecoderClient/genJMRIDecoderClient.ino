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
//#include <wm_strings_es.h>
//#include <wm_strings_en.h>
//#include <wm_consts_en.h>
//#include <WiFiManager.h>
//#include <strings_en.h>
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
    heap_caps_malloc_extmem_enable(0);
    setupRunning = true;
    Serial.begin(115200);
    Serial.printf("setup: Free Heap: %i\n", esp_get_free_heap_size());
    Serial.printf("setup: Heap watermark: %i\n", esp_get_minimum_free_heap_size());
    cpu::startHeapSupervision(true);
    logg::start();
    Serial.printf("Starting provisioning network trigger\n");
    networking::provisioningConfigTrigger();
    Serial.printf("Starting Setup task\n");
    if (!eTaskCreate(                               // Spinning up a temporary setup task as wee need a bigger stack than set-up provides
            setupTask,                              // Task function
            CPU_SETUP_TASKNAME,                     // Task function name reference
            CPU_SETUP_STACKSIZE_1K * 1024,          // Stack size
            NULL,                                   // Parameter passing
            CPU_SETUP_PRIO,                         // Priority 0-24, higher is more
            CPU_SETUP_STACK_ATTR)) {                // Stack attibute
        panic("Could not start setup task");
        return;
    }
    while (setupRunning)
        vTaskDelay(100 / portTICK_PERIOD_MS);
    LOG_INFO_NOFMT("Initial setup has successfully concluded, handing over to \"Arduino loop\"" CR);
}

void setupTask(void* p_dummy) {
    Log.setLogLevel(DEFAULT_LOGLEVEL);
    LOG_INFO_NOFMT("setupTask started" CR);
    LOG_INFO_NOFMT("Logging service started towards Serial" CR);
    fileSys::start();
    LOG_INFO_NOFMT("File system service started" CR);
    networking::start();
    LOG_INFO_NOFMT("WIFI Networking service started" CR);
    LOG_INFO_NOFMT("Connecting to WIFI..." CR);
    char nwOpStateStr[100];
    while (networking::getOpStateBitmap() != OP_WORKING) {
        LOG_INFO("Waiting for WIFI to connect, current Network OP state: %s" CR, networking::getOpStateStr(nwOpStateStr));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    LOG_INFO("Connected to WIFI and operational, got IP Address %s" CR, networking::getIpAddr().toString().c_str());
    LOG_INFO_NOFMT("Initializing time- and NTP- service" CR);
    esp_timer_init();
    ntpTime::init();
    LOG_INFO_NOFMT("Time- and NTP- service initialized" CR);
    LOG_INFO_NOFMT("Starting the runtime web-portal service" CR);
    //wifiManager = new(heap_caps_malloc(sizeof(WiFiManager), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManager;
    //wifiManager = new WiFiManager;

    /*wifiManager->setTitle(WIFI_MGR_HTML_TITLE);
    wifiManager->setShowStaticFields(true);
    wifiManager->setShowDnsFields(true);
    wifiManager->setShowInfoErase(true);
    wifiManager->setShowInfoUpdate(true);
    */
    //wifiManager->startWebPortal();
    LOG_INFO_NOFMT("Runtime web-portal service started" CR);
    LOG_INFO_NOFMT("Starting the decoder service" CR);
    decoderHandle = new (heap_caps_malloc(sizeof(decoder), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) decoder();
    decoderHandle->init();
    decoderHandle->start();
    LOG_INFO_NOFMT("Decoder service started" CR);
    setupRunning = false;
    LOG_INFO_NOFMT("Setup finished, killing setup task..." CR);
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
    //wifiManager->process(); //WHY PROCESS WIFIMANAGER FROM HERE?
    vTaskDelay(100 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
