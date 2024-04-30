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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "job.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: job																																	*/
/* Purpose: See job.h																															*/
/* Methods: See job.h																															*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR uint16_t job::jobCnt = 0;
EXT_RAM_ATTR QList<job*> job::jobList;
EXT_RAM_ATTR SemaphoreHandle_t job::jobListLock = NULL;
EXT_RAM_ATTR bool job::debug = false;

job::job(uint16_t p_jobQueueDepth, const char* p_processTaskName,
		 uint p_processTaskStackSize, uint8_t p_processTaskPrio,
		 bool p_taskSorting, uint32_t p_wdtTimeoutMs) {
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
	statCnt = 0;
	memset(jobQueueLatency, 0, JOB_STAT_CNT * sizeof(uint));
	maxJobQueueLatency = 0;
	memset(jobExecutionTime, 0, JOB_STAT_CNT * sizeof(uint));
	maxJobExecutionTime = 0;
	memset(jobSlotOccupancy, 0, JOB_STAT_CNT * sizeof(uint16_t));
	maxJobSlotOccupancy = 0;
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
	lapseDescList = new QList<lapseDesc_t*>;
	if (!lapseDescList) {
		panic("Could not create lapse list object");
		return;
	}
	jobWdt = new (heap_caps_malloc(sizeof(wdt),
						MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))
						wdt(wdtTimeoutMs, p_processTaskName,
							FAULTACTION_GLOBAL_FAILSAFE |
							FAULTACTION_GLOBAL_REBOOT |
							FAULTACTION_ESCALATE_INTERGAP);
	if (!jobWdt){
		panic("Could not create job watchdog");
		return;
	}
	processJobs = true;
	if (!(jobTaskHandle = eTaskCreate(jobProcessHelper,								// Task function
						p_processTaskName,											// Task function name reference
						p_processTaskStackSize,										// Stack size
						this,														// Parameter passing
						p_processTaskPrio,											// Priority 0-24, higher is more
						INTERNAL))){												// Task handle
		panic("Could not create job task %s", p_processTaskName);
		return;
	}
	jobWdt->activate();
	jobOverloadCb = NULL;
	jobOverloadCbMetaData = NULL;
	overloaded = false;
	unsetOverloadLevel = p_jobQueueDepth - 1;
	purgeAllJobs = false;
}

job::~job(void) {
	 panic("Destruction not supported");
	 jobWdt->inactivate();
	 processJobs = false;
	 vTaskDelay(500 / portTICK_PERIOD_MS);											// Wait for the job process task to complete
	 delete jobWdt;
	 uint16_t lapseDescListSize = lapseDescList->size();
	 for (uint16_t lapseDescListItter = 0; lapseDescListItter < lapseDescListSize; lapseDescListItter++) {
		 uint16_t taskDescListSize = lapseDescList->back()->jobDescList->size();
		 for (uint16_t taskDescListItter = 0; taskDescListItter < taskDescListSize; taskDescListItter++) {
			 delete lapseDescList->back()->jobDescList->back();
			 lapseDescList->back()->jobDescList->pop_back();
		 }
		 delete lapseDescList->back()->jobDescList;
		 delete lapseDescList->back();
		 lapseDescList->pop_back();
	 }
	 delete lapseDescList;
	 jobList.clear(jobList.indexOf(this));

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

void job::enqueue(jobCb_t p_jobCb, void* p_jobCbMetaData, bool p_purgeAllJobs,
				  bool p_supervisionJob) {
	if (p_purgeAllJobs)
		purgeAllJobs = p_purgeAllJobs;
	xSemaphoreTake(jobLock, portMAX_DELAY);
	jobdesc_t* jobDesc = new (heap_caps_malloc(sizeof(jobdesc_t),
		MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) jobdesc_t;
	if (!jobDesc){
		panic("Could not create a job descriptor");
		return;
	}
	jobDesc->jobCb = p_jobCb;
	jobDesc->jobCbMetaData = p_jobCbMetaData;
	jobDesc->taskHandle = xTaskGetCurrentTaskHandle();
	jobDesc->jobSupervision = p_supervisionJob;
	jobDesc->jobEnqueTime = esp_timer_get_time();
	if (lapseDescList->size() == 0) {
		lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t),
									  MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))
									  lapseDesc_t);
		if (!lapseDescList->back()) {
			panic("Could not create a lapse descriptor");
			return;
		}
		lapseDescList->back()->jobDescList = new QList<jobdesc_t*>;
		if (!lapseDescList->back()->jobDescList){
			panic("Could not create a job descriptor");
			return;
		}
		lapseDescList->back()->jobDescList->push_back(jobDesc);
		if(taskSorting)
			lapseDescList->back()->taskHandle = jobDesc->taskHandle;
		else
			lapseDescList->back()->taskHandle = NULL;
	}
	else if (!taskSorting) {														// If not taskSorting, queue all jobs under the same lapse
		lapseDescList->back()->jobDescList->push_back(jobDesc);
	}
	else{																			// If taskSorting collect jobs under respective lapse based on origin task
		for (uint16_t lapseItter = 0; lapseItter < lapseDescList->size();
			 lapseItter++) {
			if (lapseDescList->at(lapseItter)->taskHandle == jobDesc->taskHandle) {
				lapseDescList->at(lapseItter)->jobDescList->push_back(jobDesc);
				break;
			}
			if (lapseItter + 1 == lapseDescList->size()) {
				lapseDescList->push_back(new (heap_caps_malloc(sizeof(lapseDesc_t),
										 MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT))
										 lapseDesc_t);
				if (!lapseDescList->back()){
					panic("Could not create a lapse descriptor");
					return;
				}
				lapseDescList->back()->jobDescList = new QList<jobdesc_t*>;
				if(!lapseDescList->back()->jobDescList){
					panic("Could not create a job descriptor");
					return;
				}
				lapseDescList->back()->jobDescList->push_back(jobDesc);
				lapseDescList->back()->taskHandle = jobDesc->taskHandle;
				break;
			}
		}
	}
	if (!overloaded && uxSemaphoreGetCount(jobSleepSemaphore) + 1					// No more job slots, we are overloaded
		>= jobQueueDepth) {
		overloaded = true;
		overloadCnt++;
		if (jobOverloadCb)
			jobOverloadCb(jobOverloadCbMetaData, true);
	}
	xSemaphoreGive(jobLock);
	xSemaphoreGive(jobSleepSemaphore);												// Send the job to the scheduler backend
	if (purgeAllJobs) {																// If purgeAllJobs active, bump scheduler priority to above the originating enqueuer, effectively blocking the enqueuer her until the scheduler is empty
		if (uxSemaphoreGetCount(jobSleepSemaphore)){
			vTaskPrioritySet(jobTaskHandle, (uxTaskPriorityGet(NULL) >=
											ESP_TASK_PRIO_MAX - 1) ?
											  ESP_TASK_PRIO_MAX - 1 :
											  (uxTaskPriorityGet(NULL) >=
											  PURGE_JOB_PRIO_MAX ?
											    uxTaskPriorityGet(NULL) + 1 :
											    PURGE_JOB_PRIO_MAX));
		}
	}
	taskYIELD();
}

void job::purge(void) {
	purgeAllJobs = true;
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
	while (processJobs) {
		if ((xSemaphoreTake(jobSleepSemaphore, wdtTimeoutMs / (portTICK_PERIOD_MS * 3)) == pdFALSE)) {
			if (!processJobs)
				break;
			if (wdtTimeoutMs && !wdtSuperviseJobs && !overloaded) {
				enqueue(jobWdtSuperviseHelper, this, false, true);
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
		lapseDescList->front()->jobDescList->front()->jobStartTime =
			esp_timer_get_time();
		lapseDescList->front()->jobDescList->front()->jobCb(lapseDescList->
															front()->jobDescList->
															front()->jobCbMetaData);

		xSemaphoreTake(jobLock, portMAX_DELAY);
		updateJobStats(lapseDescList->front()->jobDescList->front(),
					   uxSemaphoreGetCount(jobSleepSemaphore) + 1);
		delete lapseDescList->front()->jobDescList->front();
		lapseDescList->front()->jobDescList->pop_front();
		if (lapseDescList->front()->jobDescList->size() == 0) {
			delete lapseDescList->front()->jobDescList;
			delete lapseDescList->front();
			lapseDescList->pop_front();
		}
		xSemaphoreGive(jobLock);
		if (overloaded && uxSemaphoreGetCount(jobSleepSemaphore) <= unsetOverloadLevel) {
			overloaded = false;
			jobOverloadCb(jobOverloadCbMetaData, false);
		}
		if (!uxSemaphoreGetCount(jobSleepSemaphore)) {
			vTaskPrioritySet(NULL, processTaskPrio);
			purgeAllJobs = false;
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
	for (uint16_t jobListItter  = 0; jobListItter <
		jobList.size(); jobListItter++) {
		if (jobList.at(jobListItter)->jobId > maxId)
			maxId = jobList.at(jobListItter)->jobId;
	}
	xSemaphoreGive(jobListLock);
	return maxId;
}

void job::updateJobStats(jobdesc_t* p_jobDesc, uint16_t p_jobslotOccupancy) {
	if (p_jobDesc->jobSupervision)
		return;
	jobSlotOccupancy[statCnt] = p_jobslotOccupancy;
	if (p_jobslotOccupancy > maxJobSlotOccupancy)
		maxJobSlotOccupancy = p_jobslotOccupancy;
	uint now = esp_timer_get_time();
	jobExecutionTime[statCnt] = now - p_jobDesc->jobStartTime;
	if (jobExecutionTime[statCnt] > maxJobExecutionTime)
		maxJobExecutionTime = jobExecutionTime[statCnt];
	jobQueueLatency[statCnt] = now - p_jobDesc->jobEnqueTime -
							   jobExecutionTime[statCnt];
	if (jobQueueLatency[statCnt] > maxJobQueueLatency)
		maxJobQueueLatency = jobQueueLatency[statCnt];
	statCnt++;
	if (statCnt >= JOB_STAT_CNT)
		statCnt = 0;
}

uint16_t job::getMaxJobSlotOccupancy(void) {
	return maxJobSlotOccupancy;
}

void job::clearMaxJobSlotOccupancyAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		jobList.at(jobListItter)->maxJobSlotOccupancy = 0;
}

rc_t job::clearMaxJobSlotOccupancy(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	jobHandle->maxJobSlotOccupancy = 0;
	return RC_OK;
}

void job::clearMaxJobSlotOccupancy(void) {
	maxJobSlotOccupancy = 0;
}

uint16_t job::getMeanJobSlotOccupancy(void) {
	uint sum = 0;
	uint16_t sampleCnt = 0;
	for (uint16_t jobSlotOccupancyItter = 0; jobSlotOccupancyItter < JOB_STAT_CNT;
		 jobSlotOccupancyItter++) {
		if(jobSlotOccupancy[jobSlotOccupancyItter]){
			sum += jobSlotOccupancy[jobSlotOccupancyItter];
			sampleCnt++;
		}
	}
	if (sampleCnt)
		return sum / sampleCnt;
	else 
		return 0;
}

void job::clearMeanJobSlotOccupancyAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		memset(jobList.at(jobListItter)->jobSlotOccupancy, 0,
			   JOB_STAT_CNT * sizeof(uint16_t));
}

rc_t job::clearMeanJobSlotOccupancy(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	memset(jobHandle->jobSlotOccupancy, 0, JOB_STAT_CNT * sizeof(uint16_t));
	return RC_OK;
}

void job::clearMeanJobSlotOccupancy(void) {
	memset(jobSlotOccupancy, 0, JOB_STAT_CNT * sizeof(uint16_t));
}

uint job::getMaxJobQueueLatency(void) {
	return maxJobQueueLatency;
}

void job::clearMaxJobQueueLatencyAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		jobList.at(jobListItter)->maxJobQueueLatency = 0;
}

rc_t job::clearMaxJobQueueLatency(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	jobHandle->maxJobQueueLatency = 0;
	return RC_OK;
}

void job::clearMaxJobQueueLatency(void) {
	maxJobQueueLatency = 0;
}
uint job::getMeanJobQueueLatency(void) {
	uint sum = 0;
	uint16_t sampleCnt = 0;
	for (uint16_t jobQueueLatencyItter = 0; jobQueueLatencyItter < JOB_STAT_CNT;
		 jobQueueLatencyItter++) {
		if (jobQueueLatency[jobQueueLatencyItter]){
			sum += jobQueueLatency[jobQueueLatencyItter];
			sampleCnt++;
		}
	}
	if (sampleCnt)
		return sum / sampleCnt;
	else
		return 0;
}

void job::clearMeanJobQueueLatencyAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		memset(jobList.at(jobListItter)->jobQueueLatency, 0,
			   JOB_STAT_CNT * sizeof(uint));
}

rc_t job::clearMeanJobQueueLatency(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	memset(jobHandle->jobQueueLatency, 0, JOB_STAT_CNT * sizeof(uint));
	return RC_OK;
}

void job::clearMeanJobQueueLatency(void) {
	memset(jobQueueLatency, 0, JOB_STAT_CNT * sizeof(uint));
}

uint job::getMaxJobExecutionTime(void) {
	return maxJobExecutionTime;
}

void job::clearMaxJobExecutionTimeAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		jobList.at(jobListItter)->maxJobExecutionTime = 0;
}

rc_t job::clearMaxJobExecutionTime(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	jobHandle->maxJobExecutionTime = 0;
	return RC_OK;
}

void job::clearMaxJobExecutionTime(void) {
	maxJobExecutionTime = 0;
}

uint job::getMeanJobExecutionTime(void) {
	uint sum = 0;
	uint16_t sampleCnt = 0;
	for (uint16_t jobExecutionTimeItter = 0; jobExecutionTimeItter < JOB_STAT_CNT;
		 jobExecutionTimeItter++) {
		if (jobExecutionTime[jobExecutionTimeItter]){
			sum += jobExecutionTime[jobExecutionTimeItter];
			sampleCnt++;
		}
	}
	if (sampleCnt)
		return sum / sampleCnt;
	else
		return 0;
}

void job::clearMeanJobExecutionTimeAll(void) {
	for (uint16_t jobListItter = 0; jobListItter < jobList.size(); jobListItter++)
		memset(jobList.at(jobListItter)->jobExecutionTime, 0,
			   JOB_STAT_CNT * sizeof(uint));
}

rc_t job::clearMeanJobExecutionTime(uint16_t p_id) {
	job* jobHandle;
	if (!(jobHandle = getJobHandleById(p_id)))
		return RC_NOT_FOUND_ERR;
	memset(jobHandle->jobExecutionTime, 0, JOB_STAT_CNT * sizeof(uint));
	return RC_OK;
}

void job::clearMeanJobExecutionTime(void) {
	memset(jobExecutionTime, 0, JOB_STAT_CNT * sizeof(uint));
}
