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
job::job(uint16_t p_jobQueueDepth, const char* p_processTaskName, uint p_processTaskStackSize, uint8_t p_processTaskPrio) {
	jobQueueDepth = p_jobQueueDepth;
	processTaskPrio = p_processTaskPrio;
	if (!(jobLock = xSemaphoreCreateMutex()))
		panic("Could not create Lock objects - rebooting..." CR);
	if (!(jobSleepSemaphore = xSemaphoreCreateCounting(p_jobQueueDepth, 0)))
		panic("Could not create jobProcess sleep semaphore  objects - rebooting..." CR);
	//if (!(lapseDescList = new (heap_caps_malloc(sizeof(QList<lapseDesc_t*>), MALLOC_CAP_SPIRAM)) QList<lapseDesc_t*>))
	if (!(lapseDescList = new QList<lapseDesc_t*>))
		panic("Could not create lapse list object - rebooting..." CR);
	//uint8_t jobProcessCore = ....
	if (!(jobTaskHandle = eTaskCreate(jobProcessHelper,					// Task function
						p_processTaskName,								// Task function name reference
						p_processTaskStackSize,							// Stack size
						this,											// Parameter passing
						p_processTaskPrio,								// Priority 0-24, higher is more
						INTERNAL)))										// Task handle
		panic("Could not create job task - rebooting..." CR);
	jobOverloadCb = NULL;
	jobOverloadCbMetaData = NULL;
	overloaded = false;
	unsetOverloadLevel = 0;
	bumpJobSlotPrio = 0;
	purgeAllJobs = false;
}

job::~job(void) {
	panic("Destruction not supported - rebooting..." CR);
}

void job::regOverloadCb(jobOverloadCb_t p_jobOverloadCb, void* p_metaData) {
	jobOverloadCb = p_jobOverloadCb;
	jobOverloadCbMetaData = p_metaData;
}

void job::unRegOverloadCb(void) {
	jobOverloadCb = NULL;
}

void job::setjobQueueUnsetOverloadLevel(uint16_t p_unsetOverloadLevel) {
	unsetOverloadLevel = p_unsetOverloadLevel;
}

void job::enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, uint8_t p_prio, bool p_purgeAllJobs) {
	jobdesc_t* jobDesc = new (heap_caps_malloc(sizeof(jobdesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) jobdesc_t;
	if (!jobDesc)
		panic("Failed to create a new jobdescriptor, rebooting..." CR);
	jobDesc->jobCb = p_jobCb;
	jobDesc->jobCbMetaData = p_jobCbMetaData;
	jobDesc->taskHandle = xTaskGetCurrentTaskHandle();
	xSemaphoreTake(jobLock, portMAX_DELAY);
	if (lapseDescList->size() == 0) {
		lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lapseDesc_t);
		if (!lapseDescList->back())
			panic("Could not create lapse descriptor object - rebooting..." CR);
		if (!(lapseDescList->back()->jobDescList = new QList<jobdesc_t*>))
			panic("Could not create job descriptor list object - rebooting..." CR);
		lapseDescList->back()->jobDescList->push_back(jobDesc);
		lapseDescList->back()->taskHandle = jobDesc->taskHandle;
	}
	else{
		for (uint16_t lapseItter = 0; lapseItter < lapseDescList->size(); lapseItter++) {
			if (lapseDescList->at(lapseItter)->taskHandle == jobDesc->taskHandle) {
				lapseDescList->at(lapseItter)->jobDescList->push_back(jobDesc);
				break;
			}
			if (lapseItter + 1 == lapseDescList->size()) {
				lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) lapseDesc_t);
				if (!lapseDescList->back())
					panic("Could not create lapse descriptor object - rebooting..." CR);
				if (!(lapseDescList->back()->jobDescList = new QList<jobdesc_t*>))
					panic("Could not create job descriptor list object - rebooting..." CR);
				lapseDescList->back()->jobDescList->push_back(jobDesc);
				lapseDescList->back()->taskHandle = jobDesc->taskHandle;
				break;
			}
		}
	}
	xSemaphoreGive(jobLock);
	if (p_purgeAllJobs) {
		purgeAllJobs = p_purgeAllJobs;
		bumpJobSlotPrio = 0;
		vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1? ESP_TASK_PRIO_MAX - 1 : (uxTaskPriorityGet(NULL) >= PURGE_JOB_PRIO? uxTaskPriorityGet(NULL) + 1 : PURGE_JOB_PRIO));
	}
	else if (uxSemaphoreGetCount(jobSleepSemaphore) >= jobQueueDepth && !purgeAllJobs) {
		bumpJobSlotPrio++;
		vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1? ESP_TASK_PRIO_MAX - 1 : uxTaskPriorityGet(NULL) + 1);
	}
	xSemaphoreGive(jobSleepSemaphore);
	if (!overloaded && uxSemaphoreGetCount(jobSleepSemaphore) >= jobQueueDepth) {
		overloaded = true;
		if (jobOverloadCb)
			jobOverloadCb(true);
	}
	else if(overloaded && uxSemaphoreGetCount(jobSleepSemaphore) <= unsetOverloadLevel)
		jobOverloadCb(true);
}

void job::purge(void) {
	purgeAllJobs = true;
	bumpJobSlotPrio = 0;
	vTaskPrioritySet(jobTaskHandle, uxTaskPriorityGet(NULL) >= ESP_TASK_PRIO_MAX - 1 ? ESP_TASK_PRIO_MAX - 1 : (uxTaskPriorityGet(NULL) >= PURGE_JOB_PRIO ? uxTaskPriorityGet(NULL) + 1 : PURGE_JOB_PRIO));
}

bool job::getOverload(void) {
	return overloaded;
}

uint16_t job::getPendingJobSlots(void) {
	return uxSemaphoreGetCount(jobSleepSemaphore);
}

void job::jobProcessHelper(void* p_objectHandle) {
	((job*)p_objectHandle)->jobProcess();
}

void job::jobProcess(void) {
	while (true) {
		xSemaphoreTake(jobSleepSemaphore, portMAX_DELAY);
		if (lapseDescList->size() == 0)
			panic("Job buffer empty despite the jobSleepSemaphore was released" CR);
		lapseDescList->front()->jobDescList->front()->jobCb(lapseDescList->front()->jobDescList->front()->jobCbMetaData);
		xSemaphoreTake(jobLock, portMAX_DELAY);
		delete lapseDescList->front()->jobDescList->front();
		lapseDescList->front()->jobDescList->pop_front();
		if (lapseDescList->front()->jobDescList->size() == 0) {
			delete lapseDescList->front()->jobDescList;
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
	}
}
