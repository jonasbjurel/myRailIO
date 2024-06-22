/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2023 Jonas Bjurel (jonasbjurel@hotmail.com)
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
class wdt;
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
/* Purpose: The job class provides a scheduler service for generic jobs, creating a boundary between stimulis and their respective back-end     */
/*            job tasks.                                                                                                                        */
/*          A job class object is created for each needed scheduler back-end which runs as an own task. The creation involves parameters        */
/*            such as the depth of the scheduler queue, the scheduler's task name, the scheduler's task stack size, the scheduler priority,     */
/*            if task sorting should be applied (more on this later), and the scheduler's watchdog timeout (more on this later).                */
/*          Tasks are scheduled through the enqueue method, and the scheduling principle is mainly FIFO, the enqueue methods specifies the      */
/*            receiving callback, any metadata to the callback, and a scheduler purge option (more on this later).                              */
/*          As jobs gets enqueued to a scheduler, they are asynchronously handled and scheduled to the respective receiving callback backend.   */
/*            The enqueuing is non-blocking, with the task to add a new job to the scheduler backend. Should the scheduler queue get exhausted  */
/*            consumers can register a callback to be informed about the congestion and apply necessary back-pressure. At congestion, newly     */
/*            queued jobs are discarded.                                                                                                        */
/*          There is an taskSort option which compromises the FIFO property of the scheduler. When taskSort is enabled order is ensured within  */
/*            each task context posting jobs to the scheduler, if multiple tasks are posting jobs to the scheduler it is not always going to    */
/*            result in a strict orderly scheduling, but the order from a given task posting jobs is assured. This can be useful in certain     */
/*            cases where you want to keep a forlop from a producer together.                                                                   */
/*          There is also a purge option for the enqueue method, as well as a dedicated purge method. If the purge option is applied with the   */
/*            enqueue method, all scheduled jobs including the enqued job requesting job queue purge will be expedited before any new jobs are  */
/*            added to the scheduler - similarilly with the dedicated purge method.                                                             */
/*            Scheduler purge should be used with causiousness as the scheduler priority will bump with every enqueue request steming from      */
/*            tasks with higher priority than the current job scheduler priority, and will only revert back when the scheduler queue is empty.  */
/*            This will result in enqueue calls blocking waiting for the scheduler queue to empty.                                              */
/*          The job scheduler provides various runtime statistics such as:                                                                      */
/*            - Job queue occupancy(Max/Current/Peak/Average/Overload/OverloadCnt): The utilization statistics of the job/scheduler queue.      */
/*            - Latency (Peak/Mean): The latency from when the job was first schedueled/enqueued until it started.                              */
/*            - Execution time (Peak/Mean): The execution time from when the job was scheduled/dequeued from the job scheduler until it has     */
/*                finished.                                                                                                                     */
/*            The job statistics is not time based, but based on the invocation of the scheduler, and uses JOB_STAT_CNT samples for its         */
/*              for it's averaging.                                                                                                             */
/*          The job scheduler is supervised by a watchdog with a time-out value provided at the creation of the job class object. The watchdog  */
/*            is fed everytime the job scheduler backend is run, should ther not be enough jobs scheduled to feed the watchdog - small          */
/*            small supervisor jobs are injected, which however is not counted for in the job statistics                                        */
/* Methods: See below                                                                                                                           */
/* Data structures:  See below                                                                                                                  */
/*==============================================================================================================================================*/
//Definitions
#define PURGE_JOB_PRIO_MAX                  (ESP_TASK_PRIO_MAX - 3)                 // Maximum job purge prio despite triggering task prio
#define JOB_STAT_CNT_STR                    "10"                                    // Number of job statistic samples (JOB_STAT_CNT needs to change accordingly)
#define JOB_STAT_CNT                        10                                      // Number of job statistic samples (JOB_STAT_CNT_STR needs to change accordingly)

//Call-back prototypes
typedef void (*jobOverloadCb_t)(void* jobOverloadCbMetaData, bool p_overload);      // Overload callback
typedef void(*jobCb_t)(void* jobCbMetaData);                                        // Job scheduler dequeue callback

struct jobdesc_t {                                                                  // Job descriptor
    TaskHandle_t taskHandle;                                                        //   Invoking job task handle
    jobCb_t jobCb;                                                                  //   Job scheduler dequeue cb
    void* jobCbMetaData;                                                            //   Job scheduler dequeue cb meta data
    bool jobSupervision;                                                            //   Is a WDT supervision job
    uint jobEnqueTime;                                                              //   System time enqueued
    uint jobStartTime;                                                              //   System time job started
};

struct lapseDesc_t {                                                                // Lapse descriptor, a lapse is all triggering events from a given source task
    TaskHandle_t taskHandle;                                                        //   Invoking job task handle - only applicable if taskSorting
    QList<jobdesc_t*>* jobDescList;                                                 //   List of jobdescriptors queued for this lapse
};



class job {
public:
    //Public methods
    job(uint16_t p_jobQueueDepth, const char* p_processTaskName,                    // job class object constructor. p_jobQueueDepth: maximum number of jobs to be schedueld, p_processTaskName: scheduler backend task name,
        uint p_processTaskStackSize, uint8_t p_processTaskPrio,                     //   p_processTaskStackSize: scheduler backend task stack size, p_processTaskPrio: scheduler backend task priority,
        bool p_taskSorting, uint32_t p_wdtTimeoutMs);                               //   p_taskSorting: see description above, p_wdtTimeoutMs: job scheduler watchdog timeout
    ~job(void);                                                                     // job class object destructor
    void regOverloadCb(jobOverloadCb_t p_jobOverloadCb, void* p_metaData);          // Register job scheduler overload cb - see jobOverloadCb_t
    void unRegOverloadCb(void);                                                     // Un register job scheduler overload cb
    void setOverloadLevelCease(uint16_t p_unsetOverloadLevel);                      // Set the overload cease level
    void enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, bool p_purge = false,      // Enqueue a new job. p_jobCb: job callback - see jobCb_t, p_jobCbMetaData: job callback metadata, p_purge: purge/empty the scheduler after this,
                 bool p_supervisionJob = false);                                    //   p_supervisionJob: internally used for WDT supervision jobs.
    void purge(void);                                                               // Purge the job schedulerr
    const char* getJobDescription(void);                                            // Get the job description, same as the scheduler backend task name.
    uint16_t getId(void);                                                           // Get job Id
    uint16_t getWdtId(void);                                                        // Get WDT id supervising this job
    bool getOverload(void);                                                         // Get overload condition
    uint getOverloadCnt(void);                                                      // Get overload condition count
    static void clearOverloadCntAll(void);                                          // Clear all job class objects overload condition count
    static rc_t clearOverloadCnt(uint16_t p_id);                                    // Clear overload condition count for a particular job Id
    void clearOverloadCnt(void);                                                    // Clear overload condition count
    uint16_t getJobSlots(void);                                                     // Get number of jobslots/scheduler depth
    uint16_t getPendingJobSlots(void);                                              // Get number of pending jobs sitting in the scheduler queue
    rc_t setPriority(uint8_t p_priority);                                           // Set the base task priority for the job scheduler back-end
    uint8_t getPriority(void);                                                      // Get the base task priority for the job scheduler back-end, the actual current priority may be differen in case of an ongoing purge operation
    bool getTaskSorting(void);                                                      // Get taskSorting option
    static job* getJobHandleById(uint16_t p_jobId);                                 // Get job class object handle by job id
    static void setDebug(bool p_debug);                                             // Set debug option
    static bool getDebug(void);                                                     // Get debug option
    static uint16_t maxId(void);                                                    // Get maximum current job class oobject id
    uint16_t getMaxJobSlotOccupancy(void);                                          // Get maximum registered jobslot/queue utilization.
    static void clearMaxJobSlotOccupancyAll(void);                                  // Clear all job class objects maximum registered jobslot/queue utilization.
    static rc_t clearMaxJobSlotOccupancy(uint16_t p_id);                            // Clear maximum registered jobslot/queue utilization for a particular job class object id
    void clearMaxJobSlotOccupancy(void);                                            // Clear maximum registered jobslot/queue utilization
    uint16_t getMeanJobSlotOccupancy(void);                                         // Get mean registered jobslot/queue utilization.
    static void clearMeanJobSlotOccupancyAll(void);                                 // Clear all job class objects mean registered jobslot/queue utilization.
    static rc_t clearMeanJobSlotOccupancy(uint16_t p_id);                           // Clear mean registered jobslot/queue utilization for a particular job class object id
    void clearMeanJobSlotOccupancy(void);                                           // Clear mean registered jobslot/queue utilization
    uint getMaxJobQueueLatency(void);                                               // Get maximum registered jobslot/queue latency - from enqueueing to decueueing
    static void clearMaxJobQueueLatencyAll(void);                                   // Clear all job class objects maximum registered jobslot/queue latency
    static rc_t clearMaxJobQueueLatency(uint16_t p_id);                             // Clear maximum registered jobslot/queue latency for a particular job class object id
    void clearMaxJobQueueLatency(void);                                             // Clear maximum registered jobslot/queue latency
    uint getMeanJobQueueLatency(void);                                              // Get mean registered jobslot/queue latency - from enqueueing to decueueing
    static void clearMeanJobQueueLatencyAll(void);                                  // Clear all job class objects mean registered jobslot/queue latency
    static rc_t clearMeanJobQueueLatency(uint16_t p_id);                            // Clear mean registered jobslot/queue latency for a particular job class object id
    void clearMeanJobQueueLatency(void);                                            // Clear mean registered jobslot/queue latency
    uint getMaxJobExecutionTime(void);                                              // Get maximum registered jobexecution time - scheduler decueueing to finish
    static void clearMaxJobExecutionTimeAll(void);                                  // Clear all job class objects maximum registered jobexecution time
    static rc_t clearMaxJobExecutionTime(uint16_t p_id);                            // Clear maximum registered jobexecution time for a particular job class object id
    void clearMaxJobExecutionTime(void);                                            // Clear maximum registered jobexecution time
    uint getMeanJobExecutionTime(void);                                             // Get mean registered jobexecution time - scheduler decueueing to finish
    static void clearMeanJobExecutionTimeAll(void);                                 // Clear all job class objects mean registered jobexecution time
    static rc_t clearMeanJobExecutionTime(uint16_t p_id);                           // Clear mean registered jobexecution time for a particular job class object id
    void clearMeanJobExecutionTime(void);                                           // Clear mean registered jobexecution time

    //Public data structures
    //--

private:
    //Private methods
    static void jobProcessHelper(void* p_objectHandle);                             // Job scheduler backend start helper
    void jobProcess(void);                                                          // Job scheduler backend
    static void jobWdtSuperviseHelper(void* p_dummy);                               // Job scheduler WDT supervisor target helper
    void jobWdtSupervise(void);                                                     // Job scheduler WDT supervisor target
    void updateJobStats(jobdesc_t* p_jobDesc, uint16_t p_jobslotOccupancy);         // Statistics update method

    //Private data structures
    static uint16_t jobCnt;                                                         // Job class object count
    static QList<job*> jobList;                                                     // List of all jobs
    uint16_t jobId;                                                                 // Job class object id
    TaskHandle_t jobTaskHandle;                                                     // Job scheduler backend task handle
    bool processJobs;                                                               // while true - process jobs, if false terminate processing of jobs
    const char* processTaskName;                                                    // Job scheduler backend task name
    uint8_t processTaskPrio;                                                        // Job scheduler backend task base prio
    bool taskSorting;                                                               // Job scheduler backend task sorting
    uint32_t wdtTimeoutMs;                                                          // Job scheduler backend WDT timeout
    uint8_t wdtSuperviseJobs;                                                       // Number of ongoing WDT supervisor jobs
    uint16_t jobQueueDepth;                                                         // Number of job class object scheduler slots/queue depth
    jobOverloadCb_t jobOverloadCb;                                                  // job class object overload cb
    void* jobOverloadCbMetaData;                                                    // job class object overload cb metadata
    bool overloaded;                                                                // job class object overload
    uint overloadCnt;                                                               // job class object overload event count
    uint16_t unsetOverloadLevel;                                                    // job class object overload unset threshold count
    bool purgeAllJobs;                                                              // job class object purge indication
    QList<lapseDesc_t*>* lapseDescList;                                             // job class object lapse list
    static SemaphoreHandle_t jobListLock;                                           // Overall job list lock protection
    SemaphoreHandle_t jobLock;                                                      // job class object job data lock protection
    SemaphoreHandle_t jobSleepSemaphore;                                            // job class object scheduling semaphore
    wdt* jobWdt;                                                                    // job class object watchdog
    static bool debug;                                                              // Overall job debug flag
    uint16_t statCnt;                                                               // Statistics sample counter
    uint jobQueueLatency[JOB_STAT_CNT];                                             // job class object latency samples
    uint maxJobQueueLatency;                                                        // job class object max latency
    uint jobExecutionTime[JOB_STAT_CNT];                                            // job class object execution time samples
    uint maxJobExecutionTime;                                                       // job class object max execution time
    uint16_t jobSlotOccupancy[JOB_STAT_CNT];                                        // job class object job slot occupancy samples
    uint16_t maxJobSlotOccupancy;                                                   // job class object max job slot occupancy
};

/*==============================================================================================================================================*/
/* END Class job                                                                                                                                */
/*==============================================================================================================================================*/
#endif /*JOB_H*/
