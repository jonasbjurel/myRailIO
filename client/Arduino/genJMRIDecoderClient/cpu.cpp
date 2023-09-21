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
#include "cpu.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose: See ntpTime.h                                                                                                                       */
/* Description: See ntpTime.h                                                                                                                   */
/* Methods: See ntpTime.h                                                                                                                       */
/* Data structures: See ntpTime.h                                                                                                               */
/*==============================================================================================================================================*/
cpu CPU;

EXT_RAM_ATTR bool cpu::cpuPmEnable = false;
EXT_RAM_ATTR bool cpu::cpuPmLogging = false;
EXT_RAM_ATTR uint16_t cpu::secondCount = 0;
EXT_RAM_ATTR uint cpu::maxCpuLoad;
EXT_RAM_ATTR uint* cpu::busyTicksHistory = NULL;
EXT_RAM_ATTR uint* cpu::idleTicksHistory = NULL;
EXT_RAM_ATTR uint* cpu::totHeapFreeHistory = NULL;
EXT_RAM_ATTR uint* cpu::intHeapFreeHistory = NULL;
EXT_RAM_ATTR QList<const char*> cpu::taskNameList;
EXT_RAM_ATTR QList<taskPmDesc_t*> cpu::taskPmDescList;
EXT_RAM_ATTR SemaphoreHandle_t cpu::cpuPMLock = xSemaphoreCreateMutex();

void cpu::startPm(void) {
	cpuPmEnable = true;
	busyTicksHistory = new (heap_caps_malloc(sizeof(uint[CPU_HISTORY_SIZE]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint[CPU_HISTORY_SIZE];
	idleTicksHistory = new (heap_caps_malloc(sizeof(uint[CPU_HISTORY_SIZE]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint[CPU_HISTORY_SIZE];
	totHeapFreeHistory = new (heap_caps_malloc(sizeof(uint[CPU_HISTORY_SIZE]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint[CPU_HISTORY_SIZE];
	intHeapFreeHistory = new (heap_caps_malloc(sizeof(uint[CPU_HISTORY_SIZE]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) uint[CPU_HISTORY_SIZE];
	if (!busyTicksHistory || !idleTicksHistory || !totHeapFreeHistory || !intHeapFreeHistory){
		panic("cpu::startPm: Could not allocate PM history vectors");
		return;
	}
	for (uint i = 0; i < CPU_HISTORY_SIZE; i++){
		*(busyTicksHistory + i) = 0;
		*(idleTicksHistory + i) = 0; //FILL IT WITH MAX TICKS FOR THE PERIOD
	}
	heapInfo_t heapInfo;
	getHeapMemInfo(&heapInfo, false);
	for (uint i = 0; i < CPU_HISTORY_SIZE; i++){
		*(totHeapFreeHistory + i) = heapInfo.freeSize;
	}
	getHeapMemInfo(&heapInfo, true);
	for (uint i = 0; i < CPU_HISTORY_SIZE; i++)
		*(intHeapFreeHistory + i) = heapInfo.freeSize;
	rc_t rc = xTaskCreatePinnedToCore(
				cpuPmCollect,																	// Task function
				CPU_PM_TASKNAME,																// Task function name reference
				CPU_PM_STACKSIZE_1K * 1024,														// Stack size
				NULL,																			// Parameter passing
				CPU_PM_PRIO,																	// Priority 0-24, higher is more
				NULL,																			// Task handle
				CPU_PM_CORE);																	// Core [CORE_0 | CORE_1]
	if (rc != pdPASS){
		panic("Could not start CPU PM collection task, return code %i", rc);
		return;
	}
	LOG_INFO_NOFMT("CPU PM collection started" CR);
}

void cpu::stopPm(void) {
	cpuPmEnable = false;
	delete busyTicksHistory;
	delete idleTicksHistory;
	delete totHeapFreeHistory;
	delete intHeapFreeHistory;
	LOG_VERBOSE_NOFMT("Stoping CPU PM collection" CR);
	while (cpuPmLogging)
		vTaskDelay(100 / portTICK_PERIOD_MS);
	LOG_INFO("CPU PM collection stopped" CR);
}

bool cpu::getPm(void) {
	return cpuPmEnable;
}
	
void cpu::cpuPmCollect(void* dummy) {
	heapInfo_t* heapInfo;
	UBaseType_t uxArraySize;
	UBaseType_t uxCurrentNumberOfTasks;
	TaskStatus_t* pxTaskStatusArray;
	int busyTicks = 0;
	int idleDelta1sTicks = 0;
	uint8_t delta1sStartIndex = 0;
	int tmpMaxCpuLoad = 0;
	cpuPmLogging = true;
	while(cpuPmEnable){
		//CPU load status/statistics collection
		/*
		busyTicks = 0;
		if (!secondCount)
			delta1sStartIndex = CPU_HISTORY_SIZE - 1;
		else
			delta1sStartIndex = secondCount - 1;
		idleTicksHistory[secondCount] = xTaskGetIdleRunTimeCounter();
		idleDelta1sTicks = idleTicksHistory[secondCount] - idleTicksHistory[delta1sStartIndex];
		uxCurrentNumberOfTasks = uxTaskGetNumberOfTasks();
		pxTaskStatusArray = (TaskStatus_t*)malloc(uxCurrentNumberOfTasks * sizeof(TaskStatus_t));
		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
		for (uint8_t task = 0; task < taskPmDescList.length(); task++)
			taskPmDescList.at(task)->scanned = true;
		for (uint8_t task = 0; task < uxCurrentNumberOfTasks; task++) {
			int8_t searchIndex = taskNameList.indexOf(pxTaskStatusArray[task].pcTaskName);
			if (searchIndex < 0) {
				taskNameList.push_back(pxTaskStatusArray[task].pcTaskName);
				taskPmDesc_t* newTaskDesc = new taskPmDesc_t;
				taskPmDescList.push_back(newTaskDesc);
				newTaskDesc->taskName = pxTaskStatusArray[task].pcTaskName;
				newTaskDesc->busyTaskTickHistory[secondCount] = pxTaskStatusArray[task].ulRunTimeCounter;
				newTaskDesc->maxCpuLoad = 0;
				newTaskDesc->scanned = true;
			}
			else {
				taskPmDescList.at(searchIndex)->busyTaskTickHistory[secondCount] = pxTaskStatusArray[task].ulRunTimeCounter;
				uint busyTaskDelta1sTicks;
				busyTaskDelta1sTicks = taskPmDescList.at(searchIndex)->busyTaskTickHistory[secondCount] -
									   taskPmDescList.at(searchIndex)->busyTaskTickHistory[delta1sStartIndex];
				uint tmpMaxCpuLoad = 100 * busyTaskDelta1sTicks / (busyTaskDelta1sTicks + idleDelta1sTicks);
				if (tmpMaxCpuLoad > taskPmDescList.at(searchIndex)->maxCpuLoad)
					taskPmDescList.at(searchIndex)->maxCpuLoad = tmpMaxCpuLoad;
				taskPmDescList.at(searchIndex)->scanned = true;
			}
			busyTicks += taskPmDescList.at(searchIndex)->busyTaskTickHistory[secondCount];
		}
		idleTicksHistory[secondCount] = xTaskGetIdleRunTimeCounter();
		busyTicksHistory[secondCount] = busyTicks;
		tmpMaxCpuLoad = 100 * busyTicksHistory[secondCount] / (busyTicksHistory[secondCount] + idleTicksHistory[secondCount]);
		if (tmpMaxCpuLoad > maxCpuLoad)
			maxCpuLoad = tmpMaxCpuLoad;
		*/
		//Heap memory status/statistics collection
		totHeapFreeHistory[secondCount] = xPortGetFreeHeapSize();
		intHeapFreeHistory[secondCount] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
		//Task status/statistics collection
		/*
		for (uint8_t task = 0; task < taskPmDescList.length(); task++) {
			if (taskPmDescList.at(task)->scanned == false) {
				delete taskPmDescList.at(task);
				taskPmDescList.clear(task);
				taskNameList.clear(task);
			}
		}
		*/
		if (secondCount == CPU_HISTORY_SIZE - 1)
			secondCount = 0;
		else
			secondCount++;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
	LOG_INFO_NOFMT("CPU PM ordered to be stopped, killing the pm collection task");
	cpuPmLogging = false;
	vTaskDelete(NULL);
}

uint cpu::getCpuAvgLoad(const char* p_task, uint8_t p_period_s) {
	if (p_period_s > CPU_HISTORY_SIZE) {
		LOG_ERROR_NOFMT("Averiging period out of bounds");
		return RC_PARAMETERVALUE_ERR;
	}
	int8_t startSecond;
	if (secondCount >= p_period_s - 1)
		startSecond = secondCount - p_period_s;
	else
		startSecond = (CPU_HISTORY_SIZE) + secondCount - p_period_s + 1;
	if (!p_task)
		return 100 * (busyTicksHistory[secondCount] - busyTicksHistory[startSecond]) / 
				((busyTicksHistory[secondCount] - busyTicksHistory[startSecond]) + 
				(idleTicksHistory[secondCount] - idleTicksHistory[startSecond]));
	else {
		taskPmDesc_t* taskDesc = taskPmDescList.at(taskNameList.indexOf(p_task));
		return 100 * (taskDesc->busyTaskTickHistory[secondCount] - taskDesc->busyTaskTickHistory[startSecond]) /
				((taskDesc->busyTaskTickHistory[secondCount] - taskDesc->busyTaskTickHistory[startSecond]) + 
				(idleTicksHistory[secondCount] - idleTicksHistory[startSecond]));
	}
}

uint cpu::getCpuMaxLoad(const char* p_task) {
	if (!p_task)
		return maxCpuLoad;
	else {
		if (taskNameList.indexOf(p_task) < 0){
			LOG_ERROR("task %s does not exist" CR, p_task);
			return 0;
		}
		return taskPmDescList.at(taskNameList.indexOf(p_task))->maxCpuLoad;
	}
}

rc_t cpu::clearCpuMaxLoad(const char* p_task) {
	if (!p_task) {
		maxCpuLoad = 0;
		LOG_VERBOSE_NOFMT("global cpuMaxLoad cleared" CR);
		return RC_OK;
	}
	else {
		if (taskNameList.indexOf(p_task) < 0) {
			LOG_ERROR("task %s does not exist" CR, p_task);
			return RC_NOT_FOUND_ERR;
		}
		taskPmDescList.at(taskNameList.indexOf(p_task))->maxCpuLoad = 0;
		LOG_VERBOSE("cpuMaxLoad cleared for task %s" CR, p_task);
		return RC_OK;
	}
	return RC_OK;
}

rc_t cpu::getTaskInfoAllTxt(char* p_taskInfoTxt, char* p_taskInfoHeadingTxt) {
	/* Not yet supported, ESP-IDF needs to be recompiled for uxTaskGetSystemState to work */
	*p_taskInfoTxt = 0x00;
	*p_taskInfoHeadingTxt = 0x00;
	getTaskInfoAllByTaskTxt(NULL, p_taskInfoTxt, p_taskInfoHeadingTxt);
	return RC_NOTIMPLEMENTED_ERR;
	/*
	UBaseType_t uxArraySize;
	UBaseType_t uxCurrentNumberOfTasks = uxTaskGetNumberOfTasks();
	TaskStatus_t* pxTaskStatusArray;
	pxTaskStatusArray = (TaskStatus_t*) malloc(uxCurrentNumberOfTasks * sizeof(TaskStatus_t));
	uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
	for (uint8_t task = 0; task < uxArraySize; task++) {
		getTaskInfoAllByTaskTxt(pxTaskStatusArray[task].pcTaskName, p_taskInfoTxt, p_taskInfoHeadingTxt);
		strcat(p_taskInfoTxt, "\n");
		p_taskInfoTxt += strlen(p_taskInfoTxt);
	}
	free(pxTaskStatusArray);
	return RC_OK;
	*/
}

rc_t cpu::getTaskInfoAllByTaskTxt(const char* p_task, char* p_taskInfoTxt, char* p_taskInfoHeadingTxt) {
	
	/* Not yet supported, ESP-IDF needs to be recompiled to support vTaskGetInfo*/
	/*
	TaskHandle_t xHandle;
	TaskStatus_t xTaskDetails;
	char taskState[10];
	*p_taskInfoTxt = 0x00;
	*p_taskInfoHeadingTxt = 0x00;
	if (!(xHandle = xTaskGetHandle(p_task))) {
		LOG_ERROR("cpu::getTaskInfoByTask: Task %s does not exist" CR, p_task);
		return RC_NOT_FOUND_ERR;
	}
	vTaskGetInfo(										// The handle of the task being queried.
		xHandle,										// The TaskStatus_t structure to complete with information on xTask
		&xTaskDetails,									// Include the stack high water mark value in the TaskStatus_t structure.
		pdTRUE,											// Include the task state in the TaskStatus_t structure.
		eInvalid);
	switch (xTaskDetails.eCurrentState) {
	case eReady:
		strcpy(taskState, "Ready");
		break;
	case eRunning:
		strcpy(taskState, "Running");
		break;
	case eBlocked:
		strcpy(taskState, "Blocked");
		break;
	case eSuspended:
		strcpy(taskState, "Suspended");
		break;
	case eDeleted:
		strcpy(taskState, "Deleted");
		break;
	}*/
	sprintf(p_taskInfoHeadingTxt, "Task No:\tTask Name:\tTask Status:\tTask Prio:\tCPU Load 1s:\tCPU Load 10s:\tCPU Load 60s:\tMaxCPU load\tStack Watermark:");
	/*
	sprintf(p_taskInfoTxt, "%i\t%s\t%s\t%i\t\t%s", xTaskDetails.xTaskNumber, xTaskDetails.pcTaskName, taskState, xTaskDetails.uxCurrentPriority,
			getCpuAvgLoad(p_task, 1), getCpuAvgLoad(p_task, 10), getCpuAvgLoad(p_task, 60), getCpuMaxLoad(p_task), xTaskDetails.usStackHighWaterMark);
	*/
	sprintf(p_taskInfoTxt, "Not yet supported");
	return RC_NOTIMPLEMENTED_ERR;
}

rc_t cpu::getHeapMemInfo(heapInfo_t* p_heapInfo, bool p_internal) {
	uint32_t caps = 0;
	caps =
		//MALLOC_CAP_EXEC |
		//MALLOC_CAP_32BIT |
		//MALLOC_CAP_8BIT |
		//MALLOC_CAP_DMA |
		//MALLOC_CAP_PID2 |
		//MALLOC_CAP_PID3 |
		//MALLOC_CAP_PID4 |
		//MALLOC_CAP_PID5 |
		//MALLOC_CAP_PID6 |
		//MALLOC_CAP_PID7 |
		//MALLOC_CAP_SPIRAM |
		MALLOC_CAP_INTERNAL;
		//MALLOC_CAP_DEFAULT |
		//MALLOC_CAP_IRAM_8BIT |
		//MALLOC_CAP_RETENTION |
		//MALLOC_CAP_RTCRAM;
	if (!p_internal)
		caps = caps | MALLOC_CAP_SPIRAM;
	p_heapInfo->totalSize = 0;
	p_heapInfo->freeSize = 0;
	p_heapInfo->highWatermark = 0;
	uint32_t capItterator = 1;
	for (uint8_t i=0; i<32; i++) {
		if (caps & capItterator){
			p_heapInfo->totalSize += heap_caps_get_total_size(caps & capItterator);
			p_heapInfo->freeSize += heap_caps_get_free_size(caps & capItterator);
			p_heapInfo->highWatermark += heap_caps_get_minimum_free_size(caps & capItterator);
		}
		capItterator = capItterator << 1;
	}
	return RC_OK;
}

uint cpu::getHeapMemFreeTime(uint16_t p_time_s, bool p_internal) {
	uint16_t timeSecond;
	if (secondCount <= p_time_s)
		timeSecond = CPU_HISTORY_SIZE - p_time_s + secondCount - 1;
	else
		timeSecond = secondCount - p_time_s - 1;
	if (p_internal)
		return intHeapFreeHistory[timeSecond];
	else
		return totHeapFreeHistory[timeSecond];
}

uint cpu::getAverageMemFreeTime(uint16_t p_time_s, bool p_internal) {
	unsigned long long int avgMem = 0;
	if (!p_time_s)
		return 0;
	if (p_time_s > CPU_HISTORY_SIZE)
		return 0;
	for (uint16_t i = 0; i < p_time_s; i++) {
		if (getHeapMemFreeTime(i, p_internal))
			avgMem += getHeapMemFreeTime(i, p_internal);
	}
	return avgMem / p_time_s;
}

uint cpu::getTrendMemFreeTime(uint16_t p_time, bool p_internal) {
	if (p_time > CPU_HISTORY_SIZE)
		return 0;
	return getHeapMemFreeTime(p_time, p_internal) - getHeapMemFreeTime(0, p_internal);
}

uint cpu::getHeapMemTrendTxt(char* p_heapMemTxt, char* p_heapHeadingTxt, bool p_internal) {
	heapInfo_t heapInfo;
	getHeapMemInfo(&heapInfo, p_internal);
	strcpy(p_heapMemTxt, "");
	strcpy(p_heapHeadingTxt, "");
	sprintf(p_heapHeadingTxt, "| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |",
							  -10, "FreeMem(B)",
							  -10, "UsedMem(B)",
							  -11, "TotalMem(B)",
							  -19, "HighMemWatermark(B)",
							  -11, "MemUsage(%)",
							  -19, "MemUsage10s(deltaB)",
							  -19, "MemUsage30s(deltaB)",
							  -18, "MemUsage1m(deltaB)",
							  -19, "MemUsage10m(deltaB)");
	if (cpuPmEnable) {
		char delta10S[10];
		if (!getHeapMemFreeTime(10, p_internal))
			strcpy(delta10S, "-");
		else
			itoa(getHeapMemFreeTime(10, p_internal) - getHeapMemFreeTime(0, p_internal), delta10S, 10);
		char delta30S[10];
		if (!getHeapMemFreeTime(30, p_internal))
			strcpy(delta30S, "-");
		else
			itoa(getHeapMemFreeTime(30, p_internal) - getHeapMemFreeTime(0, p_internal), delta30S, 10);
		char delta1M[10];
		if (!getHeapMemFreeTime(60, p_internal))
			strcpy(delta1M, "-");
		else
			itoa(getHeapMemFreeTime(60, p_internal) - getHeapMemFreeTime(0, p_internal), delta1M, 10);
		char delta10M[10];
		if (!getHeapMemFreeTime(10 * 60, p_internal))
			strcpy(delta10M, "-");
		else
			itoa(getHeapMemFreeTime(10 * 60, p_internal) - getHeapMemFreeTime(0, p_internal), delta10M, 10);
		sprintf(p_heapMemTxt, "| %*i | %*i | %*i | %*i | %*.2f | %*s | %*s | %*s | %*s |",
							  -10, heapInfo.freeSize,
							  -10, heapInfo.totalSize - heapInfo.freeSize,
							  -11, heapInfo.totalSize,
							  -19, heapInfo.highWatermark,
							  -11, (float)(100 - 100 * heapInfo.freeSize / heapInfo.totalSize),
							  -19, delta10S,
							  -19, delta30S,
							  -18, delta1M,
							  -19, delta10M);
	}
	else {
		sprintf(p_heapMemTxt, "| %*i | %*i | %*i | %*i | %*.2f | %*s | %*s | %*s | %*s |",
							  -10, heapInfo.freeSize,
							  -10, heapInfo.totalSize - heapInfo.freeSize,
							  -11, heapInfo.totalSize,
							  -19, heapInfo.highWatermark,
							  -11, (float)(100 - 100 * heapInfo.freeSize / heapInfo.totalSize),
							  -19, "-",
							  -19, "-",
							  -18, "-",
							  -19, "-");
	}
	return heapInfo.freeSize;
}

uint cpu::getMaxAllocMemBlockSize(bool p_internal) {
	if(p_internal)
		return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
	if (heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) > heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))
		return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
	else
		return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
}
/*==============================================================================================================================================*/
/* END class cpu                                                                                                                                */
/*==============================================================================================================================================*/
