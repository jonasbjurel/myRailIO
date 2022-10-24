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
#include "cpu.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
cpu CPU;

SemaphoreHandle_t cpu::cpuPMLock;
uint64_t cpu::accBusyTime0;
uint64_t cpu::accTime0;
uint64_t cpu::accBusyTime1;
uint64_t cpu::accTime1;
uint64_t cpu::totalUsHistory0[CPU_HISTORY_SIZE];
uint64_t cpu::totalUsHistory1[CPU_HISTORY_SIZE];
uint64_t cpu::busyUsHistory0[CPU_HISTORY_SIZE];
uint64_t cpu::busyUsHistory1[CPU_HISTORY_SIZE];
uint8_t cpu::index;
uint32_t cpu::totIndex;
float cpu::maxCpuLoad0;
float cpu::maxCpuLoad1;

void cpu::init(void) {
	cpuPMLock = xSemaphoreCreateMutex();
	xTaskCreatePinnedToCore(
		cpuMeasurment0,												// Task function
		"CPU-MEAS-0",												// Task function name reference
		1024,														// Stack size
		NULL,														// Parameter passing
		tskIDLE_PRIORITY + 1,										// Priority 0-24, higher is more
		NULL,														// Task handle
		CORE_0);													// Core [CORE_0 | CORE_1]

	xTaskCreatePinnedToCore(
		cpuMeasurment1,												// Task function
		"CPU-MEAS-1",												// Task function name reference
		1024,														// Stack size
		NULL,														// Parameter passing
		tskIDLE_PRIORITY + 1,										// Priority 0-24, higher is more
		NULL,														// Task handle
		CORE_1);													// Core [CORE_0 | CORE_1]

/*	xTaskCreatePinnedToCore(										// *** Debug: Load calibration task ***
						load,										// Task function
						"LOAD",										// Task function name reference
						1024,										// Stack size
						NULL,										// Parameter passing
						4,											// Priority 0-24, higher is more
						NULL,										// Task handle
						CORE_1);									// Core [CORE_0 | CORE_1] */

	xTaskCreatePinnedToCore(
		cpuPM,														// Task function
		"CPU-PM",													// Task function name reference
		6 * 1024,													// Stack size
		NULL,														// Parameter passing
		CPU_PM_POLL_PRIO,											// Priority 0-24, higher is more
		NULL,														// Task handle
		CPU_PM_CORE);												// Core [CORE_0 | CORE_1]
}

void cpu::load(void* dummy) {										//Calibrating load function - steps the system core load every 5 seconds
	while (true) {
		for (uint16_t load = 0; load < 10000; load += 1000) {
			Serial.print("Generating ");
			Serial.print((float)load / (1000 + load) * 100);
			Serial.println("% load");
			uint64_t _5Sec = esp_timer_get_time();
			while (esp_timer_get_time() - _5Sec < 5000000) {
				uint64_t time = esp_timer_get_time();
				while (esp_timer_get_time() - time < load) {
				}
				vTaskDelay(1);
			}
		}
	}
}

void cpu::cpuMeasurment0(void* dummy) {								// Core CPU load measuring function - disables built in idle function, and thus legacy watchdog and heap housekeeping.
	int64_t measureBusyTimeStart;
	int64_t measureTimeStart;
	accBusyTime0 = 0;
	accTime0 = 0;
	vTaskSuspendAll();												// Suspend the task scheduling for this core - to take full controll of the scheduling.
	while (true) {
		measureTimeStart = esp_timer_get_time();					// Start a > 100 mS time measuring loop, we cannot measure the time for each idividual loop for truncation error reasons
		while (esp_timer_get_time() - measureTimeStart < 100000) {	// Wait 100 mS before granting Idle task runtime
			measureBusyTimeStart = esp_timer_get_time();			// Start of busy run-time measurement
			xTaskResumeAll();										// By shortly enabling scheduling we will trigger scheduling of queued higher priority tasks
			vTaskSuspendAll();										// After completion of the higher priority tasks we will disable scheduling again
			if (esp_timer_get_time() - measureBusyTimeStart > 10) {	// If the time duration for enabled scheduling was more than 10 uS we assume that the time was spent for busy workloaads and account the time as busy
				accBusyTime0 += esp_timer_get_time() - measureBusyTimeStart;
			}
		}
		accTime0 += esp_timer_get_time() - measureTimeStart;		// Exclude idle task runtime or potentially highrér priority tasks runtime from the measurement samples
		xTaskResumeAll();
		vTaskDelay(1);												// The hope here is that the task queue is empty - Give IDLE task time to run
		vTaskSuspendAll();
	}
}

void cpu::cpuMeasurment1(void* dummy) {								// Core CPU load measuring function - disables built in idle function, and thus legacy watchdog and heap housekeeping.
	int64_t measureBusyTimeStart;
	int64_t measureTimeStart;
	accBusyTime1 = 0;
	accTime1 = 0;
	vTaskSuspendAll();												// Suspend the task scheduling for this core - to take full controll of the scheduling.
	while (true) {
		measureTimeStart = esp_timer_get_time();					// Start a > 100 mS time measuring loop, we cannot measure the time for each idividual loop for truncation error reasons
		while (esp_timer_get_time() - measureTimeStart < 100000) {	// Wait 100 mS before granting Idle task runtime
			measureBusyTimeStart = esp_timer_get_time();			// Start of busy run-time measurement
			xTaskResumeAll();										// By shortly enabling scheduling we will trigger scheduling of queued higher priority tasks
			vTaskSuspendAll();										// After completion of the higher priority tasks we will disable scheduling again
			if (esp_timer_get_time() - measureBusyTimeStart > 10) { // If the time duration for enabled scheduling was more than 10 uS we assume that the time was spent for busy workloaads and account the time as busy
				accBusyTime1 += esp_timer_get_time() - measureBusyTimeStart;
			}
		}
		accTime1 += esp_timer_get_time() - measureTimeStart;		// Exclude idle task runtime or potentially highrér priority tasks runtime from the measurement samples
		xTaskResumeAll();
		vTaskDelay(1);												// The hope here is that the task queue is empty - Give IDLE task time to run
		vTaskSuspendAll();
	}
}

void cpu::cpuPM(void* dummy) {
	float cpuLoad0 = 0;
	float cpuLoad1 = 0;
	uint64_t prevAccBusyTime0 = 0;
	uint64_t prevAccTime0 = 0;
	uint64_t prevAccBusyTime1 = 0;
	uint64_t prevAccTime1 = 0;
	xSemaphoreTake(cpuPMLock, portMAX_DELAY);
	index = 0;
	totIndex = 0;
	while (true) {
		busyUsHistory0[index] = accBusyTime0;
		totalUsHistory0[index] = accTime0;							//FIX Should be total acc time
		busyUsHistory1[index] = accBusyTime1;
		totalUsHistory1[index] = accTime1;							//FIX Should be total acc time
		xSemaphoreGive(cpuPMLock);
		cpuLoad0 = getAvgCpuLoadCore(CORE_0, 1);
		cpuLoad1 = getAvgCpuLoadCore(CORE_1, 1);
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		if (cpuLoad0 > maxCpuLoad0)
			maxCpuLoad0 = cpuLoad0;
		if (cpuLoad1 > maxCpuLoad1)
			maxCpuLoad1 = cpuLoad1;
		xSemaphoreGive(cpuPMLock);
		Serial.print("--- CPU load report for index: ");
		Serial.print(index);
		Serial.println(" ---");
		Serial.println("CPU,  1 Sec, 10 Sec, 1 Min, Max");
		Serial.print("0,    ");
		Serial.print(getAvgCpuLoadCore(CORE_0, 1));
		Serial.print(",   ");
		Serial.print(getAvgCpuLoadCore(CORE_0, 10));
		Serial.print(",   ");
		Serial.print(getAvgCpuLoadCore(CORE_0, 60));
		Serial.print(", ");
		Serial.println(maxCpuLoad0);
		Serial.print("1,    ");
		Serial.print(getAvgCpuLoadCore(CORE_1, 1));
		Serial.print(",   ");
		Serial.print(getAvgCpuLoadCore(CORE_1, 10));
		Serial.print(",   ");
		Serial.print(getAvgCpuLoadCore(CORE_1, 60));
		Serial.print(",   ");
		Serial.println(maxCpuLoad1);
		Serial.println("--- END CPU load report ---");
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		if (++index == CPU_HISTORY_SIZE)
			index = 0;
		totIndex++;
	}
}

float cpu::getAvgCpuLoadCore(uint8_t p_core, uint8_t p_period) {
	int8_t tmp_startIndex;
	if (p_period > CPU_HISTORY_SIZE) {
		Log.error("cpu::getAvgCpuLoadCore: invalid average period provided" CR);
		return RC_GEN_ERR;
	}
	xSemaphoreTake(cpuPMLock, portMAX_DELAY);
	if (totIndex < p_period) {
		xSemaphoreGive(cpuPMLock);
		//Log.verbose("cpu::getAvgCpuLoadCore: Not enough samples to provide CPU load average" CR);
		return 0;
	}
	if (p_core == CORE_0) {
		tmp_startIndex = index - p_period;
		if (tmp_startIndex < 0) {
			tmp_startIndex = CPU_HISTORY_SIZE + tmp_startIndex;
		}
		if (totalUsHistory0[index] - totalUsHistory0[tmp_startIndex] == 0) {
			xSemaphoreGive(cpuPMLock);
			return 100;
		}
		float tmp_AvgLoad0 = ((float)(busyUsHistory0[index] - busyUsHistory0[tmp_startIndex]) / (totalUsHistory0[index] - totalUsHistory0[tmp_startIndex])) * 100;
		xSemaphoreGive(cpuPMLock);
		return tmp_AvgLoad0;
	}
	else if (p_core == CORE_1) {
		tmp_startIndex = index - p_period;
		if (tmp_startIndex < 0) {
			tmp_startIndex = CPU_HISTORY_SIZE + tmp_startIndex;
		}
		if (totalUsHistory1[index] - totalUsHistory1[tmp_startIndex] == 0) {
			xSemaphoreGive(cpuPMLock);
			return 100;
		}
		float tmp_AvgLoad1 = ((float)(busyUsHistory1[index] - busyUsHistory1[tmp_startIndex]) / (totalUsHistory1[index] - totalUsHistory1[tmp_startIndex])) * 100;
		xSemaphoreGive(cpuPMLock);
		return tmp_AvgLoad1;
	}
	else {
		xSemaphoreGive(cpuPMLock);
		Log.error("cpu::get1SecCpuLoadCore: Invalid core provided, continuing ..." CR);
		return RC_GEN_ERR;
	}
}

uint8_t cpu::getCpuMaxLoadCore(uint8_t p_core) {
	uint8_t tmp_maxCpuLoad;
	if (p_core == CORE_0) {
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		tmp_maxCpuLoad = maxCpuLoad0;
		xSemaphoreGive(cpuPMLock);
		return tmp_maxCpuLoad;
	}
	else if (p_core == CORE_1) {
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		tmp_maxCpuLoad = maxCpuLoad1;
		xSemaphoreGive(cpuPMLock);
		return tmp_maxCpuLoad;
	}
	else
		return RC_GEN_ERR;
}

uint8_t cpu::clearCpuMaxLoadCore(uint8_t p_core) {
	if (p_core == CORE_0) {
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		maxCpuLoad0 = 0;
		xSemaphoreGive(cpuPMLock);
		return RC_OK;
	}
	else if (p_core == CORE_1) {
		xSemaphoreTake(cpuPMLock, portMAX_DELAY);
		maxCpuLoad1 = 0;
		xSemaphoreGive(cpuPMLock);
		return RC_OK;
	}
	else
		return RC_GEN_ERR;
}

void cpu::getTaskInfoAll(char* p_taskInfoTxt) {							//TODO
}

uint8_t cpu::getTaskInfoByTask(char* p_task, char* p_taskInfoTxt) {		//TODO
}

uint32_t cpu::getHeapMemInfoAll(void) {									//Migrate function from CLI class to here
}

uint32_t cpu::getHeapMemInfoMaxAll(void) {								//Migrate function from CLI class to here
}

uint32_t cpu::getHeapMemTrend10minAll(void) {							//TODO
}

uint32_t cpu::getStackByTask(char* p_task) {							//TODO

}

uint32_t cpu::getMaxStackByTask(char* p_task) {							//TODO
//  return uxTaskGetStackHighWaterMark(xTaskGetHandle(p_task));
}
/*==============================================================================================================================================*/
/* END class cpu                                                                                                                                */
/*==============================================================================================================================================*/
