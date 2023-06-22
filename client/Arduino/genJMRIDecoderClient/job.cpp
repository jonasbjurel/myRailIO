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
uint16_t job::jobId = 0;
job::job(uint16_t p_jobQueueDepth, const char* p_processTaskName, uint p_processTaskStackSize, uint8_t p_processTaskPrio, uint8_t p_coreMap) {
	//if (!(jobLock = xSemaphoreCreateMutexStatic((StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
	if (!(jobLock = xSemaphoreCreateMutex()))
		panic("job::job: Could not create Lock objects - rebooting..." CR);
	//if (!(jobSleepSemaphore = xSemaphoreCreateCountingStatic(p_jobQueueDepth, 0, (StaticQueue_t*)heap_caps_malloc(sizeof(StaticQueue_t), MALLOC_CAP_SPIRAM))))
	if (!(jobSleepSemaphore = xSemaphoreCreateCounting(p_jobQueueDepth, 0)))
		panic("job::job: Could not create jobProcess sleep semaphore  objects - rebooting..." CR);
	if (!(lapseDescList = new QList<lapseDesc_t*>))
		panic("job::job: Could not create lapse list object - rebooting..." CR);
	//uint8_t jobProcessCore = ....
	BaseType_t core;
	if (!p_coreMap < 2)
		core = CORE_0;
	else if (p_coreMap == 2)
		core = CORE_1;
	else
		core = jobId % 2;
	xTaskCreatePinnedToCore(jobProcessHelper,		// Task function
		p_processTaskName,							// Task function name reference
		p_processTaskStackSize,						// Stack size
		this,										// Parameter passing
		p_processTaskPrio,							// Priority 0-24, higher is more
		NULL,										// Task handle
		core);										// Core [CORE_0 | CORE_1]
	jobId++;
}

job::~job(void) {
	panic("job::~job: Destruction not supported - rebooting..." CR);
}

void job::enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, uint8_t p_prio){
	jobdesc_t* jobDesc = new jobdesc_t;
	if (!jobDesc)
		panic("job::enqueue: Failed to create a new jobdescriptor, rebooting..." CR);
	jobDesc->jobCb = p_jobCb;
	jobDesc->jobCbMetaData = p_jobCbMetaData;
	jobDesc->taskHandle = xTaskGetCurrentTaskHandle();
	xSemaphoreTake(jobLock, portMAX_DELAY);
	if (lapseDescList->size() == 0) {
		lapseDescList->push_back(new lapseDesc_t);
		if (!lapseDescList->back())
			panic("job::enqueue: Could not create lapse descriptor object - rebooting..." CR);
		if (!(lapseDescList->back()->jobDescList = new QList<jobdesc_t*>))
			panic("job::enqueue: Could not create job descriptor list object - rebooting..." CR);
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
				lapseDescList->push_back(new lapseDesc_t);
				if (!lapseDescList->back())
					panic("job::enqueue: Could not create lapse descriptor object - rebooting..." CR);
				if (!(lapseDescList->back()->jobDescList = new QList<jobdesc_t*>))
					panic("job::enqueue: Could not create job descriptor list object - rebooting..." CR);
				lapseDescList->back()->jobDescList->push_back(jobDesc);
				lapseDescList->back()->taskHandle = jobDesc->taskHandle;
				break;
			}
		}
	}
	xSemaphoreGive(jobLock);
	xSemaphoreGive(jobSleepSemaphore);

}

void job::jobProcessHelper(void* p_objectHandle) {
	((job*)p_objectHandle)->jobProcess();
}

void job::jobProcess(void) {
	while (true) {
		xSemaphoreTake(jobSleepSemaphore, portMAX_DELAY);
		if (lapseDescList->size() == 0) {
			panic("job::jobProcess: Job buffer empty despite the jobSleepSemaphore was released" CR);
		}
		lapseDescList->front()->jobDescList->front()->jobCb(lapseDescList->front()->jobDescList->front()->jobCbMetaData);

		xSemaphoreTake(jobLock, portMAX_DELAY);
		delete lapseDescList->front()->jobDescList->front();
		lapseDescList->front()->jobDescList->pop_front();
		if (lapseDescList->front()->jobDescList->size() == 0) {
			delete lapseDescList->front()->jobDescList;
			lapseDescList->pop_front();
		}
		xSemaphoreGive(jobLock);
	}
}
