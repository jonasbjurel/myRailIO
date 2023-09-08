/*============================================================================================================================================= =*/
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
#ifndef FLASH_H
#define FLASH_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries\tinyxml2\tinyxml2.h"
#include <QList.h>
#include "rc.h"
#include "config.h"
#include "panic.h"
#include "taskWrapper.h"
#include "logHelpers.h"

class flash;

/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void(*flashCallback_t)(bool p_flashState, void* args);
#define FLASH_LATENCY_AVG_TIME        10

struct callbackSub_t {
    flashCallback_t callback;
    void* callbackArgs;
};

struct flash_t {
    QList<callbackSub_t*> callbackSubs;
    uint32_t onTime;
    uint32_t offTime;
    bool flashState;
};

class flash {
public:
    //Public methods
    flash(float p_freq, uint8_t p_duty);
    ~flash(void);
    rc_t subscribe(flashCallback_t p_callback, void* p_args);
    rc_t unSubscribe(flashCallback_t cb);
    static void flashLoopStartHelper(void* p_flashObject);
    void flashLoop(void);
    int getOverRuns(void);
    void clearOverRuns(void);
    uint32_t getMeanLatency(void);
    uint32_t getMaxLatency(void);
    void clearMaxLatency(void);

    //Public data structures

private:
    //Privat methods
    //--

    //Private data structures
    static uint8_t flashInstanses;
    uint8_t flashInstanse;
    uint32_t overRuns;
    uint32_t maxLatency;
    uint16_t maxAvgSamples;
    uint32_t* latencyVect;
    flash_t* flashData;
    SemaphoreHandle_t flashLock;
};

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/
#endif /*FLASH_H*/
