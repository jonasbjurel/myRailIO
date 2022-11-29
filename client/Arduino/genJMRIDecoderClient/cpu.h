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

#ifndef CPU_H
#define CPU_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include "rc.h"
#include "libraries/QList/src/QList.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "config.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
//Class fixed constants
#define CPU_HISTORY_SIZE 60 + 1                                                       //Provides samples for maximum 60 seconds cpu performance data
class cpu;

typedef struct taskPmDesc_t {
    const char* taskName;
    uint busyTaskTickHistory[CPU_HISTORY_SIZE];
    uint maxCpuLoad;
    bool scanned;
};

typedef struct heapInfo_t {
    int totalSize;
    int freeSize;
    int highWatermark;
};

class cpu {
public:
    //methods
    static void startPm(void);
    static void stopPm(void);
    static uint getCpuAvgLoad(const char* p_task, uint8_t p_period);
    static uint getCpuMaxLoad(const char* p_task);
    static rc_t clearCpuMaxLoad(const char* p_task);
    static rc_t getTaskInfoAllTxt(char* p_taskInfoTxt, char* p_taskInfoHeadingTxt);
    static rc_t getTaskInfoAllByTaskTxt(const char* p_task, char* p_taskInfoTxt, char* p_taskInfoHeadingTxt);
    static rc_t getHeapMemInfoAll(heapInfo_t* p_heapInfo);
    static float getHeapMemTime(uint8_t p_time);
    static uint getHeapMemTrendAllTxt(char* p_heapMemTxt, char* p_heapHeadingTxt);

    //Data structures
    //--

private:
    //methods
    static void cpuPmCollect(void* dummy);

    //Data structures
    static SemaphoreHandle_t cpuPMLock;
    static bool cpuPmEnable;
    static bool cpuPmLogging;
    static uint8_t secondCount;
    static uint busyTicksHistory[CPU_HISTORY_SIZE];
    static uint idleTicksHistory[CPU_HISTORY_SIZE];
    static uint heapHistory[CPU_HISTORY_SIZE];
    static uint maxCpuLoad;
    static QList<const char*> taskNameList;
    static QList<taskPmDesc_t*> taskPmDescList;
};
/*==============================================================================================================================================*/
/* END Class cpu                                                                                                                                */
/*==============================================================================================================================================*/

#endif //CPU_H
