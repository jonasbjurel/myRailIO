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

bool cpu::cpuPmEnable = false;
bool cpu::cpuPmLogging = false;
uint8_t cpu::secondCount = 0;
uint cpu::maxCpuLoad;
uint cpu::busyTicksHistory[CPU_HISTORY_SIZE];
uint cpu::idleTicksHistory[CPU_HISTORY_SIZE];
uint cpu::heapHistory[CPU_HISTORY_SIZE] = { 0 };
QList<const char*> cpu::taskNameList;
QList<taskPmDesc_t*> cpu::taskPmDescList;
SemaphoreHandle_t cpu::cpuPMLock = xSemaphoreCreateMutex();
	

void cpu::startPm(void) {
	cpuPmEnable = true;
	xTaskCreatePinnedToCore(
		cpuPmCollect,																	// Task function
		CPU_PM_TASKNAME,																// Task function name reference
		CPU_PM_STACKSIZE_1K * 1024,														// Stack size
		NULL,																			// Parameter passing
		CPU_PM_PRIO,																	// Priority 0-24, higher is more
		NULL,																			// Task handle
		CPU_PM_CORE);																	// Core [CORE_0 | CORE_1]
	Log.INFO("cpu::startPm: CPU PM collection started");
}

void cpu::stopPm(void) {
	cpuPmEnable = false;
	Log.VERBOSE("cpu::stopPm: Stoping CPU PM collection");
	while (cpuPmLogging)
		vTaskDelay(100 / portTICK_PERIOD_MS);
	Log.INFO("cpu::stopPm: CPU PM collection stopped");
}

bool cpu::getPm(void) {
	return cpuPmEnable;
}
	
void cpu::cpuPmCollect(void* dummy) {
	heapInfo_t* heapInfo;
	UBaseType_t uxArraySize;
	UBaseType_t uxCurrentNumberOfTasks;
	TaskStatus_t* pxTaskStatusArray;
	int busyTicks;
	int idleDelta1sTicks;
	uint8_t delta1sStartIndex;
	int tmpMaxCpuLoad;
	memset(heapHistory, 0, sizeof heapHistory);
	cpuPmLogging = true;
	while(cpuPmEnable){
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
		heapHistory[secondCount] = xPortGetFreeHeapSize();
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
	Log.INFO("cpu::cpuPmCollect: CPU PM ordered to be stopped, killing the pm collection task");
	cpuPmLogging = false;
	vTaskDelete(NULL);
}

uint cpu::getCpuAvgLoad(const char* p_task, uint8_t p_period_s) {
	if (p_period_s > CPU_HISTORY_SIZE) {
		Log.ERROR("cpu::getCpuAvgLoad: averiging period out of bounds");
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
			Log.ERROR("cpu::getCpuMaxLoad: task %s does not exist" CR, p_task);
			return 0;
		}
		return taskPmDescList.at(taskNameList.indexOf(p_task))->maxCpuLoad;
	}
}

rc_t cpu::clearCpuMaxLoad(const char* p_task) {
	if (!p_task) {
		maxCpuLoad = 0;
		Log.VERBOSE("cpu::clearCpuMaxLoad: global cpuMaxLoad cleared" CR);
		return RC_OK;
	}
	else {
		if (taskNameList.indexOf(p_task) < 0) {
			Log.ERROR("cpu::clearCpuMaxLoad: task %s does not exist" CR, p_task);
			return RC_NOT_FOUND_ERR;
		}
		taskPmDescList.at(taskNameList.indexOf(p_task))->maxCpuLoad = 0;
		Log.VERBOSE("cpu::clearCpuMaxLoad: cpuMaxLoad cleared for task %s" CR, p_task);
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
		Log.ERROR("cpu::getTaskInfoByTask: Task %s does not exist" CR, p_task);
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

rc_t cpu::getHeapMemInfoAll(heapInfo_t* p_heapInfo) {
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
		MALLOC_CAP_SPIRAM |
		MALLOC_CAP_INTERNAL;
		//MALLOC_CAP_DEFAULT |
		//MALLOC_CAP_IRAM_8BIT |
		//MALLOC_CAP_RETENTION |
		//MALLOC_CAP_RTCRAM;
	uint32_t totMem = 0;
	uint32_t capItterator = 1;
	for (uint8_t i=0; i<32; i++) {
		if (caps & capItterator)
			totMem += heap_caps_get_total_size(caps & capItterator);
		capItterator = capItterator << 1;
	}
	p_heapInfo->totalSize = totMem;
	p_heapInfo->freeSize = xPortGetFreeHeapSize();
	p_heapInfo->highWatermark = xPortGetMinimumEverFreeHeapSize();
	return RC_OK;
}

uint cpu::getHeapMemTime(uint16_t p_time_s) {
	uint8_t timeSecond;
	if (secondCount -1 >= p_time_s - 1)
		timeSecond = secondCount -1 - p_time_s;
	else
		timeSecond = (CPU_HISTORY_SIZE) + secondCount - 1 - p_time_s + 1;
	return heapHistory[timeSecond];
}

uint cpu::getAverageMemTime(uint16_t p_time_s) {
	int avgMem = 0;
	if (!p_time_s)
		return 0;
	if (p_time_s > CPU_HISTORY_SIZE)
		return 0;
	for (uint16_t i = 0; i < p_time_s; i++) {
		if (getHeapMemTime(i))
			avgMem += getHeapMemTime(i);
	}
	return avgMem / p_time_s;
}

uint cpu::getTrendMemTime(uint16_t p_time) {
	if (p_time > CPU_HISTORY_SIZE)
		return 0;
	return getHeapMemTime(p_time) - getHeapMemTime(0);
}

uint cpu::getHeapMemTrendAllTxt(char* p_heapMemTxt, char* p_heapHeadingTxt) {
	heapInfo_t heapInfo;
	getHeapMemInfoAll(&heapInfo);
	strcpy(p_heapMemTxt, "");
	strcpy(p_heapHeadingTxt, "");
	sprintf(p_heapHeadingTxt, "| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |",
							  -20, "FreeMem(B)",
							  -20, "UsedMem(B)",
							  -20, "TotalMem(B)",
							  -20, "HighMemWatermark(B)",
							  -20, "MemUsage(%)",
							  -20, "MemUsage10s(deltaB)",
							  -20, "MemUsage30s(deltaB)",
							  -20, "MemUsage1m(deltaB)",
							  -20, "MemUsage10m(deltaB)");
	if (cpuPmEnable) {
		char delta10S[10];
		if (!getHeapMemTime(10))
			strcpy(delta10S, "-");
		else
			itoa(getHeapMemTime(10) - getHeapMemTime(0), delta10S, 10);
		char delta30S[10];
		if (!getHeapMemTime(30))
			strcpy(delta30S, "-");
		else
			itoa(getHeapMemTime(30) - getHeapMemTime(0), delta30S, 10);
		char delta1M[10];
		if (!getHeapMemTime(60))
			strcpy(delta1M, "-");
		else
			itoa(getHeapMemTime(60) - getHeapMemTime(0), delta1M, 10);
		char delta10M[10];
		if (!getHeapMemTime(10*60))
			strcpy(delta10M, "-");
		else
			itoa(getHeapMemTime(10*60) - getHeapMemTime(0), delta10M, 10);
		sprintf(p_heapMemTxt, "| %*i | %*i | %*i | %*i | %*.2f | %*s | %*s | %*s | %*s |",
							  -20, heapInfo.freeSize,
							  -20, heapInfo.totalSize - heapInfo.freeSize,
							  -20, heapInfo.totalSize,
							  -20, heapInfo.highWatermark,
							  -20, (float)(100 - 100 * heapInfo.freeSize / heapInfo.totalSize),
							  -20, delta10S,
							  -20, delta30S,
							  -20, delta1M,
							  -20, delta10M);
	}
	else {
		sprintf(p_heapMemTxt, "| %*i | %*i | %*i | %*i | %*.2f | %*s | %*s | %*s | %*s |",
							  -20, heapInfo.freeSize,
							  -20, heapInfo.totalSize - heapInfo.freeSize,
							  -20, heapInfo.totalSize,
							  -20, heapInfo.highWatermark,
							  -20, (float)(100 - 100 * heapInfo.freeSize / heapInfo.totalSize),
							  -20, "-",
							  -20, "-",
							  -20, "-",
							  -20, "-");
	}
	return heapInfo.freeSize;
}

uint cpu::getMaxAllocMemBlockSize(void) {
	if (heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) > heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL))
		return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
	else
		return heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
}
/*==============================================================================================================================================*/
/* END class cpu                                                                                                                                */
/*==============================================================================================================================================*/
