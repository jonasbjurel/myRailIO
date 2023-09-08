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
#ifndef CPU_H
#define CPU_H
/*==============================================================================================================================================*/
/* .h Definitions                                                                                                                               */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <Arduino.h>
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <QList.h>
#include "rc.h"
#include "panic.h"
#include "config.h"
#include "logHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose: Provides CPU, memory and task statistics                                                                                            */
/* Description:                                                                                                                                 */
/* Methods: See below                                                                                                                           */
/* Data structures: See below                                                                                                                   */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
#define CPU_HISTORY_SIZE (10 * 60 + 1)                                                  //Provides samples for maximum 10 minutes of CPU- and Memory performance data

class cpu;

typedef struct taskPmDesc_t {                                                           //Task desriptor
    const char* taskName;                                                               //Task name
    uint busyTaskTickHistory[CPU_HISTORY_SIZE];                                         //Busy ticks histogram
    uint maxCpuLoad;                                                                    //Maximum CPU-load consumed by the task
    bool scanned;
};

typedef struct heapInfo_t {                                                             //Heap memory statistics 
    uint totalSize;                                                                      //Currently used Heap memort (B)
    uint freeSize;                                                                       //Currently free heap memory(B)
    uint highWatermark;                                                                  //All time lowest free Heap memory (B)
};

/* Methods                                                                                                                                      */
class cpu {
public:
    //methods
    static void startPm(void);                                                          //Start CPU and memory performance monitoring
    static void stopPm(void);                                                           //Stop CPU and memory performance monitoring
    static bool getPm(void);                                                            //Provides the performance monitoring status
    static uint getCpuAvgLoad(const char* p_task, uint8_t p_period_s);                  //Provides the overall average CPU load (%) over p_period_s seconds
    static uint getCpuMaxLoad(const char* p_task);                                      //Provides the historic macimum CPU-load (%s)
    static rc_t clearCpuMaxLoad(const char* p_task);                                    //Clears the maximum CPU load measurement
    static rc_t getTaskInfoAllTxt(char* p_taskInfoTxt, char* p_taskInfoHeadingTxt);     //Get summary status and statistics of all running tasks - NOT YET SUPPORTED
    static rc_t getTaskInfoAllByTaskTxt(const char* p_task, char* p_taskInfoTxt,        //Get statistics for a particular task given by p_task
                                        char* p_taskInfoHeadingTxt);
    static rc_t getHeapMemInfo(heapInfo_t* p_heapInfo, bool p_internal = false);        //Get Heap memory information
    static uint getHeapMemFreeTime(uint16_t p_time_s, bool p_internal = false);         //Get free Heap memory (B) p_time_s seconds back in time
    static uint getAverageMemFreeTime(uint16_t p_time_s, bool p_internal = false);      //Get average free Heap memory (B) over p_time_s seconds
    static uint getTrendMemFreeTime(uint16_t p_time_s, bool p_internal = false);        //Get Heap memory trend (B) over p_time_s seconds, posetive value (B) indicates 
                                                                                        //   increased mem usage
    static uint getHeapMemTrendTxt(char* p_heapMemTxt, char* p_heapHeadingTxt,          //Get a pre-defined Heap memory trend report
                                   bool p_internal = false);
    static uint getMaxAllocMemBlockSize(bool p_internal = false);                       //Get the maximum memory block size that can be allocated from the Heap


    //Data structures
    //--

private:
    //methods
    static void cpuPmCollect(void* dummy);                                              //Performance monitoring collection task function

    //Data structures
    static SemaphoreHandle_t cpuPMLock;                                                 //Performance monitoring lock
    static bool cpuPmEnable;                                                            //Contains performance collection status
    static bool cpuPmLogging;                                                           //Contains actual performance collection task running status
    static uint16_t secondCount;                                                         //Performance monitoring collection histogram second counter
    static uint* busyTicksHistory;                                                      //CPU Busy ticks histogram
    static uint* idleTicksHistory;                                                      //CPU Idle ticks histogram
    static uint* totHeapFreeHistory;                                                    //Free totalHeap memory(B) histogram
    static uint* intHeapFreeHistory;                                                    //Free internal memory(B) histogram
    static uint maxCpuLoad;                                                             //Maximum overall CPU load (%)
    static QList<const char*> taskNameList;                                             //List of monitored tasks names
    static QList<taskPmDesc_t*> taskPmDescList;                                         //List of task descriptors
};
/*==============================================================================================================================================*/
/* END Class cpu                                                                                                                                */
/*==============================================================================================================================================*/
#endif //CPU_H
