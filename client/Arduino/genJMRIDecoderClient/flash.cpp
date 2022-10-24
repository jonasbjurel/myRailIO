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
#include "flash.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t flash::flashInstanses = 0;

flash::flash(float p_freq, uint8_t p_duty) {
    flashInstanse = flashInstanses++;
    flashData = new flash_t;
    if (p_duty > 99) {
        p_duty = 99;
    }
    flashData->onTime = uint32_t((float)(1 / (float)p_freq) * (float)((float)p_duty / 100) * 1000000);
    flashData->offTime = uint32_t((float)(1 / (float)p_freq) * (float)((100 - (float)p_duty) / 100) * 1000000);
    Log.notice("flash::flash: Creating flash object %d with flash frequency %d Hz and dutycycle %d" CR, this, p_freq, p_duty);
    flashData->flashState = true;
    flashLock = xSemaphoreCreateMutex();
    overRuns = 0;
    maxLatency = 0;
    maxAvgSamples = p_freq * FLASH_LATENCY_AVG_TIME;
    latencyVect = new uint32_t[maxAvgSamples];

    xTaskCreatePinnedToCore(
        flashLoopStartHelper,                   // Task function
        "FlashLoop",                            // Task function name reference
        6 * 1024,                               // Stack size
        this,                                   // Parameter passing
        FLASH_LOOP_PRIO,                        // Priority 0-24, higher is more
        NULL,                                   // Task handle
        FLASH_LOOP_CORE[flashInstanse % 2]);    // Core [CORE_0 | CORE_1]
}

flash::~flash(void) {
    // need to stop the timer task
    delete flashData;                           //Much more to do/delete
    delete latencyVect;
}

rc_t flash::subscribe(flashCallback_t p_callback, void* p_args) {
    Log.notice("flash::subcribe: Subscribing to flash object %d with callback %d" CR, this, p_callback);
    callbackSub_t* newCallbackSub = new callbackSub_t;
    xSemaphoreTake(flashLock, portMAX_DELAY);
    flashData->callbackSubs.push_back(newCallbackSub);
    flashData->callbackSubs.back()->callback = p_callback;
    flashData->callbackSubs.back()->callbackArgs = p_args;
    xSemaphoreGive(flashLock);
    return RC_OK;
}

rc_t flash::unSubscribe(flashCallback_t p_callback) {
    Log.info("flash::unSubcribe: Unsubscribing flash callback %d from flash object %d" CR, p_callback, this);
    uint16_t i = 0;
    bool found = false;
    xSemaphoreTake(flashLock, portMAX_DELAY);
    for (i = 0; true; i++) {
        if (i >= flashData->callbackSubs.size()) {
            break;
        }
        if (flashData->callbackSubs.get(i)->callback == p_callback)
            found = true;
        Log.info("flash::unSubcribe: Deleting flash subscription %d from flash object %d" CR, p_callback, this);
        delete flashData->callbackSubs.get(i);
        flashData->callbackSubs.clear(i);
    }
    if (found) {
        xSemaphoreGive(flashLock);
        return RC_OK;
    }
    else {
        Log.error("flash::unSubcribe: Could not find flash subscription %d from flash object %d to delete" CR, p_callback, this);
        xSemaphoreGive(flashLock);
        return RC_NOT_FOUND_ERR;
    }
}

void flash::flashLoopStartHelper(void* p_flashObject) {
    ((flash*)p_flashObject)->flashLoop();
    return;
}

void flash::flashLoop(void) {
    int64_t  nextLoopTime = esp_timer_get_time();
    int64_t  thisLoopTime;
    uint16_t latencyIndex = 0;
    uint32_t latency = 0;
    Log.notice("flash::flashLoop: Starting flash object %d" CR, this);
    while (true) {
        thisLoopTime = nextLoopTime;
        if (flashData->flashState) {
            nextLoopTime += flashData->offTime;
            flashData->flashState = false;
        }
        else {
            nextLoopTime += flashData->onTime;
            flashData->flashState = true;
        }
        for (uint16_t i = 0; i < flashData->callbackSubs.size(); i++) {
            flashData->callbackSubs.get(i)->callback(flashData->flashState, flashData->callbackSubs.get(i)->callbackArgs);
        }
        if (latencyIndex >= maxAvgSamples) {
            latencyIndex = 0;
        }
        xSemaphoreTake(flashLock, portMAX_DELAY);
        latency = esp_timer_get_time() - thisLoopTime;
        latencyVect[latencyIndex++] = latency;
        if (latency > maxLatency) {
            maxLatency = latency;
        }
        xSemaphoreGive(flashLock);
        TickType_t delay;
        if ((int)(delay = nextLoopTime - esp_timer_get_time()) > 0) {
            vTaskDelay((delay / 1000) / portTICK_PERIOD_MS);
        }
        else {
            Log.verbose("flash::flashLoop: Flash object %d overrun" CR, this);
            xSemaphoreTake(flashLock, portMAX_DELAY);
            overRuns++;
            xSemaphoreGive(flashLock);
            nextLoopTime = esp_timer_get_time();
        }
    }
}

int flash::getOverRuns(void) {
    xSemaphoreTake(flashLock, portMAX_DELAY);
    uint32_t tmpOverRuns = overRuns;
    xSemaphoreGive(flashLock);
    return tmpOverRuns;
}

void flash::clearOverRuns(void) {
    xSemaphoreTake(flashLock, portMAX_DELAY);
    overRuns = 0;
    xSemaphoreGive(flashLock);
    return;
}

uint32_t flash::getMeanLatency(void) {
    uint32_t* tmpLatencyVect = new uint32_t[maxAvgSamples];
    uint32_t accLatency;
    uint32_t meanLatency;
    xSemaphoreTake(flashLock, portMAX_DELAY);
    memcpy(tmpLatencyVect, latencyVect, maxAvgSamples);
    xSemaphoreGive(flashLock);
    for (uint16_t latencyIndex = 0; latencyIndex < maxAvgSamples; latencyIndex++) {
        accLatency += tmpLatencyVect[latencyIndex];
    }
    meanLatency = accLatency / maxAvgSamples;
    delete tmpLatencyVect;
    return meanLatency;
}

uint32_t flash::getMaxLatency(void) {
    xSemaphoreTake(flashLock, portMAX_DELAY);
    uint32_t tmpMaxLatency = maxLatency;
    xSemaphoreGive(flashLock);
    return tmpMaxLatency;
}

void flash::clearMaxLatency(void) {
    xSemaphoreTake(flashLock, portMAX_DELAY);
    maxLatency = 0;
    xSemaphoreGive(flashLock);
    return;
}

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/
