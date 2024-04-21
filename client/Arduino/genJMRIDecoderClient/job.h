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
class job;

#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <QList.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "rc.h"
#include "panic.h"
#include "wdt.h"
#include "config.h"
#include "taskWrapper.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: job                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
//Call-back prototypes
typedef void (*jobOverloadCb_t)(void* jobOverloadCbMetaData, bool p_overload);
typedef void(*jobCb_t)(void* jobCbMetaData);

//Definitions
#define PURGE_JOB_PRIO                              ESP_TASK_PRIO_MAX - 3

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


class wdt;

class job {
public:
    //Public methods
    job(uint16_t p_jobQueueDepth, const char* p_processTaskName, uint p_processTaskStackSize, uint8_t p_processTaskPrio, bool p_taskSorting, uint32_t p_wdtTimeoutMs);
    ~job(void);
    void regOverloadCb(jobOverloadCb_t p_jobOverloadCb, void* p_metaData);
    void unRegOverloadCb(void);
    void setOverloadLevelCease(uint16_t p_unsetOverloadLevel);
    void enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, bool p_purge = false);
    void purge(void);
    const char* getJobDescription(void);
    uint16_t getId(void);
    uint16_t getWdtId(void);
    bool getOverload(void);
    uint getOverloadCnt(void);
    static void clearOverloadCntAll(void);
    static rc_t clearOverloadCnt(uint16_t p_id);
    void clearOverloadCnt(void);
    uint16_t getJobSlots(void);
    uint16_t getPendingJobSlots(void);
    rc_t setPriority(uint8_t p_priority);
    uint8_t getPriority(void);
    bool getTaskSorting(void);
    static job* getJobHandleById(uint16_t p_jobId);
    static void setDebug(bool p_debug);
    static bool getDebug(void);
    static uint16_t maxId(void);


    //Public data structures
    //--

private:
    //Private methods
    static void jobProcessHelper(void* p_objectHandle);
    void jobProcess(void);
    static void jobWdtSuperviseHelper(void* p_dummy);
    void jobWdtSupervise(void);

    //Private data structures
    static uint16_t jobCnt;
    static QList<job*> jobList;
    uint16_t jobId;
    TaskHandle_t jobTaskHandle;
    const char* processTaskName;
    uint8_t processTaskPrio;
    bool taskSorting;
    uint32_t wdtTimeoutMs;
    uint8_t wdtSuperviseJobs;
    uint16_t jobQueueDepth;
    jobOverloadCb_t jobOverloadCb;
    void* jobOverloadCbMetaData;
    bool overloaded;
    uint overloadCnt;
    uint16_t unsetOverloadLevel;
    uint16_t bumpJobSlotPrio;
    bool purgeAllJobs;
    QList<lapseDesc_t*>* lapseDescList;
    static SemaphoreHandle_t jobListLock;
    SemaphoreHandle_t jobLock;
    SemaphoreHandle_t jobSleepSemaphore;
    wdt* jobWdt;
    static bool debug;
};

/*==============================================================================================================================================*/
/* END Class job                                                                                                                                */
/*==============================================================================================================================================*/
#endif /*JOB_H*/
