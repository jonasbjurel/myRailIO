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
#include "job.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "job(job)"																															*/
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

EXT_RAM_ATTR uint16_t job::jobCnt = 0;
EXT_RAM_ATTR QList<job*> job::jobList;
EXT_RAM_ATTR SemaphoreHandle_t job::jobListLock = NULL;
EXT_RAM_ATTR bool job::debug = false;



job::job(uint16_t p_jobQueueDepth, const char* p_processTaskName, uint p_processTaskStackSize, uint8_t p_processTaskPrio, bool p_taskSorting, uint32_t p_wdtTimeoutMs) {
	if (!(jobCnt++)) {
		if (!(jobListLock = xSemaphoreCreateMutex())) {
			panic("Could not create Lock objects");
			return;
		}
	}
	jobId = jobCnt;
	jobQueueDepth = p_jobQueueDepth;
	processTaskName = p_processTaskName;
	Serial.printf("¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤ Starting job Id: %i, description %s\n", jobId, processTaskName);
	processTaskPrio = p_processTaskPrio;
	taskSorting = p_taskSorting;
	wdtTimeoutMs = p_wdtTimeoutMs;
	overloaded = false;
	overloadCnt = 0;
	if (!(jobLock = xSemaphoreCreateMutex())) {
		panic("Could not create Lock objects");
		return;
	}
	xSemaphoreTake(jobListLock, portMAX_DELAY);
	jobList.push_back(this);
	xSemaphoreGive(jobListLock);

	if (!(jobSleepSemaphore = xSemaphoreCreateCounting(p_jobQueueDepth, 0))){
		panic("Could not create jobProcess sleep semaphore  objects");
		return;
	}
	//if (!(lapseDescList = new (heap_caps_malloc(sizeof(QList<lapseDesc_t*>), MALLOC_CAP_SPIRAM)) QList<lapseDesc_t*>))
	if (!(lapseDescList = new QList<lapseDesc_t*>)){
		panic("Could not create lapse list object");
		return;
	}
	jobWdt = new (heap_caps_malloc(sizeof(wdt), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wdt(wdtTimeoutMs, p_processTaskName, FAULTACTION_GLOBAL_FAILSAFE | FAULTACTION_GLOBAL_REBOOT | FAULTACTION_ESCALATE_INTERGAP);
	if (!(jobTaskHandle = eTaskCreate(jobProcessHelper,					// Task function
						p_processTaskName,								// Task function name reference
						p_processTaskStackSize,							// Stack size
						this,											// Parameter passing
						p_processTaskPrio,								// Priority 0-24, higher is more
						INTERNAL))){									// Task handle
		panic("Could not create job task %s", p_processTaskName);
		return;
	}
	jobWdt->activate();
	jobOverloadCb = NULL;
	jobOverloadCbMetaData = NULL;
	overloaded = false;
	unsetOverloadLevel = p_jobQueueDepth - 1;
	bumpJobSlotPrio = 0;
	purgeAllJobs = false;
}

job::~job(void) {
	 panic("Destruction not supported");
}

void job::regOverloadCb(jobOverloadCb_t p_jobOverloadCb, void* p_metaData) {
	jobOverloadCb = p_jobOverloadCb;
	jobOverloadCbMetaData = p_metaData;
}

void job::unRegOverloadCb(void) {
	jobOverloadCb = NULL;
}

void job::setOverloadLevelCease(uint16_t p_unsetOverloadLevel) {
	unsetOverloadLevel = p_unsetOverloadLevel;
}

void job::enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, bool p_purgeAllJobs) {
	bool found = false;
	xSemaphoreTake(jobLock, portMAX_DELAY);
	jobdesc_t* jobDesc = new (heap_caps_malloc(sizeof(jobdesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) jobdesc_t;
	assert(jobDesc != NULL);
	jobDesc->jobCb = p_jobCb;
	jobDesc->jobCbMetaData = p_jobCbMetaData;
	jobDesc->taskHandle = xTaskGetCurrentTaskHandle();
	if (lapseDescList->size() == 0) {
		lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lapseDesc_t);
		assert(lapseDescList->back() != NULL);
		lapseDescList->back()->jobDescList = new QList<jobdesc_t*>;
		assert(lapseDescList->back()->jobDescList != NULL);
		lapseDescList->back()->jobDescList->push_back(jobDesc);
		if(taskSorting)
			lapseDescList->back()->taskHandle = jobDesc->taskHandle;
		else
			lapseDescList->back()->taskHandle = NULL;
		found = true;
	}
	else if (!taskSorting) {
		lapseDescList->back()->jobDescList->push_back(jobDesc);
		found = true;
	}
	else{
		for (uint16_t lapseItter = 0; lapseItter < lapseDescList->size(); lapseItter++) {
			if (lapseDescList->at(lapseItter)->taskHandle == jobDesc->taskHandle) {
				lapseDescList->at(lapseItter)->jobDescList->push_back(jobDesc);
				found = true;
				break;
			}
			if (lapseItter + 1 == lapseDescList->size()) {
				lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lapseDesc_t);
				assert(lapseDescList->back() != NULL);
				lapseDescList->back()->jobDescList = new QList<jobdesc_t*>;
				assert(lapseDescList->back()->jobDescList != NULL);
				lapseDescList->back()->jobDescList->push_back(jobDesc);
				lapseDescList->back()->taskHandle = jobDesc->taskHandle;
				found = true;
				break;
			}
		}
	}
	assert(found == true);
	if (p_purgeAllJobs) {
		purgeAllJobs = p_purgeAllJobs;
		bumpJobSlotPrio = 0;
		vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1? ESP_TASK_PRIO_MAX - 1 : (uxTaskPriorityGet(NULL) >= PURGE_JOB_PRIO? uxTaskPriorityGet(NULL) + 1 : PURGE_JOB_PRIO));
	}
	else if (uxSemaphoreGetCount(jobSleepSemaphore) + 1 >= jobQueueDepth && !purgeAllJobs) {
		bumpJobSlotPrio++;
		vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1? ESP_TASK_PRIO_MAX - 1 : uxTaskPriorityGet(NULL) + 1);
	}
	if (!overloaded && uxSemaphoreGetCount(jobSleepSemaphore) + 1 >= jobQueueDepth) {
		overloaded = true;
		overloadCnt++;
		if (jobOverloadCb)
			jobOverloadCb(jobOverloadCbMetaData, true);
	}
	xSemaphoreGive(jobLock);
	xSemaphoreGive(jobSleepSemaphore);
}

void job::purge(void) {
	purgeAllJobs = true;
	bumpJobSlotPrio = 0;
	vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1 ? ESP_TASK_PRIO_MAX - 1 : (uxTaskPriorityGet(NULL) >= PURGE_JOB_PRIO ? uxTaskPriorityGet(NULL) + 1 : PURGE_JOB_PRIO));
}

const char* job::getJobDescription(void) {
	return processTaskName;
}

uint16_t job::getId(void) {
	return jobId;
}

uint16_t job::getWdtId(void) {
	return jobWdt->getId();
}

bool job::getOverload(void) {
	return overloaded;
}

uint job::getOverloadCnt(void) {
	return overloadCnt;
}

void job::clearOverloadCntAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		jobList.at(jobListItter)->clearOverloadCnt();
}

rc_t job::clearOverloadCnt(uint16_t p_id) {
	if (!getJobHandleById(p_id))
		return RC_NOT_FOUND_ERR;
	getJobHandleById(p_id)->clearOverloadCnt();
	return RC_OK;
}

void job::clearOverloadCnt(void) {
	overloadCnt = 0;
}

uint16_t job::getJobSlots(void) {
	return jobQueueDepth;
}

uint16_t job::getPendingJobSlots(void) {
	return uxSemaphoreGetCount(jobSleepSemaphore);
}

rc_t job::setPriority(uint8_t p_priority) {
	if (!debug)
		return RC_DEBUG_NOT_SET_ERR;
	if (p_priority > 24)
		return RC_PARAMETERVALUE_ERR;
	processTaskPrio = p_priority;
	vTaskPrioritySet(jobTaskHandle, p_priority);
	return RC_OK;
}

uint8_t job::getPriority(void) {
	return processTaskPrio;
}

bool job::getTaskSorting(void) {
	return taskSorting;
}

job* job::getJobHandleById(uint16_t p_jobId) {
	xSemaphoreTake(jobListLock, portMAX_DELAY);
	for (uint16_t jobIdItter = 0; jobIdItter < jobList.size(); jobIdItter++) {
		if (jobList.at(jobIdItter)->jobId == p_jobId) {
			xSemaphoreGive(jobListLock);
			return jobList.at(jobIdItter);
		}
	}
	xSemaphoreGive(jobListLock);
	return NULL;
}

void job::setDebug(bool p_debug) {
	LOG_INFO("%s" CR, p_debug ? "Setting job debug mode" : "Unsetting job debug mode");
	debug = p_debug;
}

bool job::getDebug(void) {
	return debug;
}

void job::jobProcessHelper(void* p_objectHandle) {
	((job*)p_objectHandle)->jobProcess();
}

void job::jobProcess(void) {
	wdtSuperviseJobs = 0;
	while (true) {
		if ((xSemaphoreTake(jobSleepSemaphore, wdtTimeoutMs / (portTICK_PERIOD_MS * 3)) == pdFALSE)) {
			if (wdtTimeoutMs && !wdtSuperviseJobs && !overloaded) {
				enqueue(jobWdtSuperviseHelper, this, false);
				wdtSuperviseJobs++;
				Serial.printf("OOOOOOOOOO Starting a job WDT supervisor job\n");
			}
			else
				Serial.printf("OOOOOOOOOOO Could not start a job WDT supervisor job, wdtTimeoutMs: %i, wdtSuperviseJobs: %i, overloaded: %i\n", wdtTimeoutMs, wdtSuperviseJobs, overloaded);
			continue;
		}
		jobWdt->feed();
		if (lapseDescList->size() == 0)
			panic("Job buffer empty despite the jobSleepSemaphore was released" CR);
		lapseDescList->front()->jobDescList->front()->jobCb(lapseDescList->front()->jobDescList->front()->jobCbMetaData);
		xSemaphoreTake(jobLock, portMAX_DELAY);
		delete lapseDescList->front()->jobDescList->front();
		lapseDescList->front()->jobDescList->pop_front();
		if (lapseDescList->front()->jobDescList->size() == 0) {
			delete lapseDescList->front()->jobDescList;
			delete lapseDescList->front();
			lapseDescList->pop_front();
		}
		xSemaphoreGive(jobLock);
		if (purgeAllJobs && !uxSemaphoreGetCount(jobSleepSemaphore)) {
			bumpJobSlotPrio = 0;
			purgeAllJobs = false;
			vTaskPrioritySet(NULL, processTaskPrio);
		}
		else if (bumpJobSlotPrio) {
			if (!(--bumpJobSlotPrio))
				vTaskPrioritySet(NULL, processTaskPrio);
		}
		if (overloaded && uxSemaphoreGetCount(jobSleepSemaphore) <= unsetOverloadLevel) {
			overloaded = false;
			jobOverloadCb(jobOverloadCbMetaData, false);
		}
	}
	vTaskDelete(NULL);
}

void job::jobWdtSuperviseHelper(void* p_handle) {
	((job*)p_handle)->jobWdtSupervise();
}

void job::jobWdtSupervise(void) {
	Serial.printf("OOOOOOOOOO Ending a job WDT supervisor job\n");
	wdtSuperviseJobs--;
}

uint16_t job::maxId(void) {
	uint16_t maxId = 0;
	xSemaphoreTake(jobListLock, portMAX_DELAY);
	for (uint16_t jobListItter = 0; jobListItter <
		jobList.size(); jobListItter++) {
		if (jobList.at(jobListItter)->jobId > maxId)
			maxId = jobList.at(jobListItter)->jobId;
	}
	xSemaphoreGive(jobListLock);
	return maxId;
}
