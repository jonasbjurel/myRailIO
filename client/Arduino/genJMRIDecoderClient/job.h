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

#ifndef JOB_H
#define JOB_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ArduinoLog.h>
#include <QList.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rc.h"
#include "panic.h"
#include "config.h"
#include "taskWrapper.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: job                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

typedef void(*jobCb_t)(void* jobCbMetaData);

struct jobdesc_t {
    TaskHandle_t taskHandle;
    jobCb_t jobCb;
    void* jobCbMetaData;
    uint8_t jobPrio;
};

struct lapseDesc_t {
    TaskHandle_t taskHandle;
    QList<jobdesc_t*>* jobDescList;
};

class job {
public:
    //Public methods
    job(uint16_t p_jobQueueDepth, const char* p_processTaskName, uint p_processTaskStackSize, uint8_t p_processTaskPrio);
    ~job(void);
    void enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, uint8_t p_prio);

    //Public data structures
    //--

private:
    //Private methods
    static void jobProcessHelper(void* p_objectHandle);
    void jobProcess(void);

    //Private data structures
    QList<lapseDesc_t*>* lapseDescList;
    SemaphoreHandle_t jobLock;
    SemaphoreHandle_t jobSleepSemaphore;
};

/*==============================================================================================================================================*/
/* END Class job                                                                                                                                */
/*==============================================================================================================================================*/
#endif /*JOB_H*/
