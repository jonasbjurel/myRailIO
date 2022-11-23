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
#define CPU_HISTORY_SIZE 61                                                           //Provides samples for maximum 60 second cpu load avarage

cpu CPU;

class cpu {
public:
    //methods
    static void init(void);
    static void startCpuLoadMeasurement(void);
    static void stopCpuLoadMeasurement(void);
    static float getAvgCpuLoadCore(uint8_t p_core, uint8_t p_period);
    static uint8_t getCpuMaxLoadCore(uint8_t p_core);
    static uint8_t clearCpuMaxLoadCore(uint8_t p_core);
    static void getTaskInfoAll(char* p_taskInfoTxt);
    static uint8_t getTaskInfoByTask(char* p_task, char* p_taskInfoTxt);
    static uint32_t getHeapMemInfoAll(void);
    static uint32_t getHeapMemInfoMaxAll(void);
    static uint32_t getHeapMemTrend10minAll(void);
    static uint32_t getStackByTask(char* p_task);
    static uint32_t getMaxStackByTask(char* p_task);

    //Data structures
    //--

private:
    //methods
    static void load(void* dummy);
    static void cpuMeasurment0(void* dummy);
    static void cpuMeasurment1(void* dummy);
    static void cpuPM(void* dummy);

    //Data structures
    static SemaphoreHandle_t cpuPMLock;
    static uint64_t accBusyTime0;
    static uint64_t accTime0;
    static uint64_t accBusyTime1;
    static uint64_t accTime1;
    static uint64_t totalUsHistory0[CPU_HISTORY_SIZE];
    static uint64_t totalUsHistory1[CPU_HISTORY_SIZE];
    static uint64_t busyUsHistory0[CPU_HISTORY_SIZE];
    static uint64_t busyUsHistory1[CPU_HISTORY_SIZE];
    static uint8_t index;
    static uint32_t totIndex;
    static float maxCpuLoad0;
    static float maxCpuLoad1;
};
/*==============================================================================================================================================*/
/* END Class cpu                                                                                                                                */
/*==============================================================================================================================================*/

#endif //CPU_H
