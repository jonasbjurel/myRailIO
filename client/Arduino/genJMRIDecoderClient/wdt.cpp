/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2024 Jonas Bjurel (jonasbjurel@hotmail.com)
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
#include "wdt.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose: See wdt.h                                                                                                                           */
/* Methods: See wdt.h                                                                                                                           */
/* Data structures: See wdt.h                                                                                                                   */
/*==============================================================================================================================================*/
DRAM_ATTR  SemaphoreHandle_t wdt::wdtProcessSemaphore = NULL;
EXT_RAM_ATTR SemaphoreHandle_t wdt::wdtDescrListLock = NULL;
EXT_RAM_ATTR TaskHandle_t wdt::backendTaskHandle = NULL;
EXT_RAM_ATTR uint16_t wdt::wdtObjCnt = 0;
DRAM_ATTR hw_timer_t* wdt::wdtTimer = NULL;
EXT_RAM_ATTR const wdtCb_t* wdt::globalCb = NULL;
EXT_RAM_ATTR const void* wdt::globalCbParams = NULL;
EXT_RAM_ATTR const wdtCb_t* wdt::globalFailsafeCb = NULL;
EXT_RAM_ATTR const void* wdt::globalFailsafeCbParams = NULL;
EXT_RAM_ATTR const wdtCb_t* wdt::globalRebootCb = NULL;
EXT_RAM_ATTR const void* wdt::globalRebootCbParams = NULL;
EXT_RAM_ATTR wdtDescrList_t* wdt::wdtDescrList = NULL;
DRAM_ATTR  uint32_t wdt::currentWdtTick = 0;
DRAM_ATTR  uint32_t wdt::nextWdtTimeoutTick = UINT32_MAX - 1;
DRAM_ATTR  bool wdt::outstandingWdtTimeout = false;
DRAM_ATTR  bool wdt::ageExpiries = false;
DRAM_ATTR  bool wdt::debug = false;



wdt::wdt(uint32_t p_wdtTimeoutMs, const char* p_wdtDescription, action_t p_wdtAction) {
	if (wdtObjCnt++ == 0) {															//First Watchdog instance - set up all needed global/static functions and structures
		LOG_INFO_NOFMT("Starting the watchdog base object" CR);
		if (!(wdtProcessSemaphore = xSemaphoreCreateCounting(NO_OF_OUTSTANDING_WDT_TIMEOUT_JOBS, 0))) {
			panic("Could not create Watchdog semaphore object" CR);
			return;
		}
		if (!(wdtDescrListLock = xSemaphoreCreateMutex())) {
			panic("Could not create Watchdog descriptor lock object" CR);
			return;
		}
		if (!(wdtDescrList = new wdtDescrList_t)) {
			panic("Could not create Watchdog descriptor list" CR);
			return;
		}
		currentWdtTick = 0;
		nextWdtTimeoutTick = UINT32_MAX - 1;
		outstandingWdtTimeout = false;
		if (!(backendTaskHandle = eTaskCreate(wdtHandlerBackend,					// Task function
			CPU_WDT_BACKEND_TASKNAME,												// Task function name reference
			CPU_WDT_BACKEND_STACKSIZE_1K * 1024,									// Stack size
			NULL,																	// Parameter passing
			CPU_WDT_BACKEND_PRIO,													// Priority 0-24, higher is more
			INTERNAL))) {															// Task stack attribute
			panic("Failed to start the watchdog" CR);
		}
		if (!(wdtTimer = timerBegin(WDT_TIMER, (TIMER_BASE_CLK / 1000000), TIMER_COUNT_UP))) {
			panic("Could not initialize the watchdog timer");
			return;
		}
		timerAttachInterrupt(wdtTimer, &wdtTimerIsr, true);
		timerAlarmWrite(wdtTimer, WD_TICK_MS * 1000, TIMER_AUTORELOAD_EN);
		timerAlarmEnable(wdtTimer);
	}
	LOG_INFO("Starting watchdog instance for %s" CR, p_wdtDescription);
	if(!(wdtDescr = new wdt_t))
		panic("Could not create the watchdog timer descriptor");
	wdtDescr->id = wdtObjCnt;
	wdtDescr->handle = this;
	wdtDescr->isActive = false;
	wdtDescr->isInhibited = false;
	if (!(wdtDescr->wdtTimeoutTicks = p_wdtTimeoutMs / WD_TICK_MS))
		wdtDescr->wdtTimeoutTicks = 1;
	if (!(wdtDescr->wdtDescription = new (heap_caps_malloc(sizeof(char) * (strlen(p_wdtDescription) + 1),
		MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(p_wdtDescription) + 1])) {
		panic("Could not create the watchdog timer description buffer");
	}
	strcpy(wdtDescr->wdtDescription, p_wdtDescription);
	wdtDescr->wdtAction = p_wdtAction;
	wdtDescr->escalationCnt = 0;
	wdtDescr->wdtExpiries = 0;
	wdtDescr->closesedhit = wdtDescr->wdtTimeoutTicks * WD_TICK_MS;
	wdtDescr->localCb = NULL;
	wdtDescr->localCbParams = NULL;
	wdtDescr->ongoingWdtTimeout = false;
	wdtDescr->wdtUpcommingTimeoutTick = currentWdtTick + wdtDescr->wdtTimeoutTicks;
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescrList->push_back(wdtDescr);
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("Watchdog instance started for %s" CR, p_wdtDescription);
}

wdt::~wdt(void) {
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescrList->clear(wdtDescrList->indexOf(wdtDescr));
	nextWdtTimeoutTick = nextTickToHandle(false);
	delete wdtDescr->wdtDescription;
	delete wdtDescr;
	if (!--wdtObjCnt) {																// If last watchdog object/instance - stop the watch dog timer
		timerAlarmDisable(wdtTimer);												//   and stop the watchdog backend handler
		timerDetachInterrupt(wdtTimer);
		timerEnd(wdtTimer);
		delete wdtTimer;
		vTaskDelete(backendTaskHandle);
		backendTaskHandle = NULL;
		vSemaphoreDelete(wdtProcessSemaphore);
		wdtProcessSemaphore = NULL;
		delete wdtDescrList;
		vSemaphoreDelete(wdtDescrListLock);
		return;
	}
	xSemaphoreGive(wdtDescrListLock);
}

rc_t wdt::setActiveAll(bool p_active) {
	if (!debug) {
		return RC_DEBUG_NOT_SET_ERR;
	}
	LOG_INFO("%s" CR, p_active ? "Activating all watchdogs" : "De-activating all watchdogs");
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (p_active)
			wdtDescrList->at(wdtDescrListItter)->handle->activate(false);
		else
			wdtDescrList->at(wdtDescrListItter)->handle->inactivate(false);
	}
	xSemaphoreGive(wdtDescrListLock);
	return RC_OK;
}

rc_t wdt::setActive(uint16_t p_id, bool p_active) {
	if (!debug) {
		return RC_DEBUG_NOT_SET_ERR;
	}
	LOG_INFO("%s id: %i" CR, p_active ? "Activating watchdog" : "De-activating watchdog", p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			if (p_active)
				wdtDescrList->at(wdtDescrListItter)->handle->activate(false);
			else
				wdtDescrList->at(wdtDescrListItter)->handle->inactivate(false);
			xSemaphoreGive(wdtDescrListLock);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("%s id: %i" CR, p_active ? "Could not activate watchdog" : "Could not de-activating watchdog", p_id);
	return RC_NOT_FOUND_ERR;
}

void wdt::activate(bool p_lock) {
	LOG_INFO("Activating watchdog instance for %s" CR, wdtDescr->wdtDescription);
	if (p_lock)
		xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	clearClosesedHit();
	clearExpiries();
	wdtDescr->wdtUpcommingTimeoutTick = currentWdtTick + wdtDescr->wdtTimeoutTicks;
	nextWdtTimeoutTick = nextTickToHandle(false);
	wdtDescr->isActive = true;
	if (p_lock)
		xSemaphoreGive(wdtDescrListLock);
}

void wdt::inactivate(bool p_lock) {
	LOG_INFO("In-activating watchdog instance for %s" CR, wdtDescr->wdtDescription);
	if (p_lock)
		xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescr->isActive = false;
	nextWdtTimeoutTick = nextTickToHandle(false);
	if (p_lock)
		xSemaphoreGive(wdtDescrListLock);
}

rc_t wdt::setTimeout(uint16_t p_id, uint32_t p_wdtTimeoutMs) {
	if (!debug)
		return RC_DEBUG_NOT_SET_ERR;
	LOG_INFO("Setting watchdog timeout for wdt id %i" CR, p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			wdtDescrList->at(wdtDescrListItter)->wdtTimeoutTicks = 
				p_wdtTimeoutMs / WD_TICK_MS;
			nextWdtTimeoutTick = nextTickToHandle(false);
			xSemaphoreGive(wdtDescrListLock);
			clearClosesedHit(p_id);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_ERROR("Could not Set watchdog timeout for wdt id %i, wdt id does not exist" CR, p_id);
	return RC_NOT_FOUND_ERR;
}

void wdt::setTimeout(uint32_t p_wdtTimeoutMs) {
	LOG_INFO("Setting watchdog timeout for wdt %s" CR, wdtDescr->wdtDescription);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescr->wdtTimeoutTicks = p_wdtTimeoutMs / WD_TICK_MS;
	nextWdtTimeoutTick = nextTickToHandle(false);
	xSemaphoreGive(wdtDescrListLock);
}

rc_t wdt::setActionsFromStr(uint16_t p_id, char* p_actionStr) {
	if (!debug) {
		return RC_DEBUG_NOT_SET_ERR;
	}
	LOG_INFO("Setting action for wdt id %i" CR, p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			rc_t rc = wdtDescrList->at(wdtDescrListItter)->handle->
				setActionsFromStr(p_actionStr, false);
			xSemaphoreGive(wdtDescrListLock);
			return rc;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("Could not set action for wdt id %i as it does not exist" CR, p_id);
	return RC_NOT_FOUND_ERR;
}
rc_t wdt::setActionsFromStr(char* p_actionStr, bool p_lock) {
	if (!debug)
		return RC_DEBUG_NOT_SET_ERR;
	LOG_INFO("Setting action for wdt %s" CR, wdtDescr->wdtDescription);
	if(p_lock)
		xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescr->wdtAction = 0;
	char* actionStr;
	actionStr = strtok(p_actionStr, "|");
	while (actionStr != NULL) {
		if (!strcmp(actionStr, "LOC0"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_LOCAL0;
		else if (!strcmp(actionStr, "LOC1"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_LOCAL1;
		else if (!strcmp(actionStr, "LOC2"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_LOCAL2;
		else if (!strcmp(actionStr, "GLB1"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_GLOBAL0;
		else if (!strcmp(actionStr, "GLBFSF"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_GLOBAL_FAILSAFE;
		else if (!strcmp(actionStr, "REBOOT"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_GLOBAL_REBOOT;
		else if (!strcmp(actionStr, "ESC_GAP"))
			wdtDescr->wdtAction = wdtDescr->wdtAction | FAULTACTION_ESCALATE_INTERGAP;
		else {
			if (p_lock)
				xSemaphoreGive(wdtDescrListLock);
			LOG_INFO("Could not set action for wdt %s, invalid action" CR, wdtDescr->wdtDescription);
			return RC_PARAMETERVALUE_ERR;
		}
		actionStr = strtok(NULL, "|");
	}
	if (p_lock)
		xSemaphoreGive(wdtDescrListLock);
	return RC_OK;
}

void wdt::setDebug(bool p_debug) {
	LOG_INFO("%s" CR, p_debug? "Setting watchdog debug mode" : "Unsetting watchdog debug mode");
	debug = p_debug;
}

bool wdt::getDebug(void) {
	return debug;
}

rc_t wdt::getWdtDescById(uint16_t p_id, wdt_t** p_descr) {
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			*p_descr = wdtDescrList->at(wdtDescrListItter);
			xSemaphoreGive(wdtDescrListLock);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	return RC_NOT_FOUND_ERR;
}

void wdt::clearExpiriesAll(void) {
	LOG_INFO_NOFMT("Clearing all watchdog expireies counters" CR);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		wdtDescrList->at(wdtDescrListItter)->wdtExpiries = 0;
	}
	xSemaphoreGive(wdtDescrListLock);
}

rc_t wdt::clearExpiries(uint16_t p_id) {
	LOG_INFO("Clearing watchdog expireies counter for id: %i" CR, p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			wdtDescrList->at(wdtDescrListItter)->wdtExpiries = 0;
			xSemaphoreGive(wdtDescrListLock);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("Could not clear watchdog expireies counter for id: %i - does not exist" CR, p_id);
	return RC_NOT_FOUND_ERR;
}

void wdt::clearExpiries(void) {
	wdtDescr->wdtExpiries = 0;
}

void wdt::clearClosesedHitAll(void) {
	LOG_INFO_NOFMT("Clearing all watchdog closesed hit counters" CR);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		wdtDescrList->at(wdtDescrListItter)->closesedhit =
			wdtDescrList->at(wdtDescrListItter)->wdtTimeoutTicks;
	}
	xSemaphoreGive(wdtDescrListLock);
}

rc_t wdt::clearClosesedHit(uint16_t p_id) {
	LOG_INFO("Clearing watchdog closesed hit counter for id: %i" CR, p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			wdtDescrList->at(wdtDescrListItter)->closesedhit = 
				wdtDescrList->at(wdtDescrListItter)->wdtTimeoutTicks;
			xSemaphoreGive(wdtDescrListLock);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("Could not clear watchdog closesed hit counter for id: %i - does not exist" CR, p_id);
	return RC_NOT_FOUND_ERR;
}

void wdt::clearClosesedHit() {
	wdtDescr->closesedhit = wdtDescr->wdtTimeoutTicks;
}

rc_t wdt::inhibitAllWdtFeeds(bool p_inhibit) {
	if (!debug)
		return RC_DEBUG_NOT_SET_ERR;
	LOG_INFO("%s" CR, p_inhibit? "Inhibiting all watchdog feeds" : "Enabling all watchdog feeds");
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		wdtDescrList->at(wdtDescrListItter)->isInhibited = p_inhibit;
	}
	xSemaphoreGive(wdtDescrListLock);
	return RC_OK;
}

rc_t wdt::inhibitWdtFeeds(uint16_t p_id, bool p_inhibit) {
	if (!debug)
		return RC_DEBUG_NOT_SET_ERR;
	LOG_INFO("%s for id: %i" CR, p_inhibit ? "Inhibiting watchdog feed" : "Enabling watchdog feed", p_id);
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter < wdtDescrList->size();
		 wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id == p_id) {
			wdtDescrList->at(wdtDescrListItter)->isInhibited = p_inhibit;
			xSemaphoreGive(wdtDescrListLock);
			return RC_OK;
		}
	}
	xSemaphoreGive(wdtDescrListLock);
	LOG_INFO("%s for id: %i - does not exist" CR, p_inhibit ? "Could not inhibit watchdog feed" : "Could not enable watchdog feed", p_id);
	return RC_NOT_FOUND_ERR;
}

void wdt::inhibitWdtFeeds(bool p_inhibit) {
	wdtDescr->isInhibited = p_inhibit;
}

void wdt::feed(void) {
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	if ((uint32_t)(wdtDescr->wdtUpcommingTimeoutTick - currentWdtTick) <
		wdtDescr->closesedhit)
		wdtDescr->closesedhit = (uint32_t)(wdtDescr->wdtUpcommingTimeoutTick - currentWdtTick);
	if (!wdtDescr->ongoingWdtTimeout && wdtDescr->isActive &&
		!wdtDescr->isInhibited){
		wdtDescr->wdtUpcommingTimeoutTick = currentWdtTick + wdtDescr->wdtTimeoutTicks;
		nextWdtTimeoutTick = nextTickToHandle(false);
	}
	xSemaphoreGive(wdtDescrListLock);
}

void wdt::regLocalWdtCb(const wdtCb_t* p_localWdtCb,
						const void* p_localWdtCbParms) {
	xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
	wdtDescr->localCb = p_localWdtCb;
	wdtDescr->localCbParams = (void*)p_localWdtCbParms;
}

void wdt::unRegLocalWdtCb(void) {
	LOG_INFO_NOFMT("Un-registering local watchdog" CR);
	wdtDescr->localCb = NULL;
	wdtDescr->localCbParams = NULL;
}

void wdt::regGlobalCb(const wdtCb_t* p_globalCb,
	const void* p_globalParams) {
	LOG_INFO("Registering global watchdog callback: 0x%x" CR, p_globalCb);
	globalCb = p_globalCb;
	globalCbParams = p_globalParams;
}

void wdt::unRegGlobalCb(void) {
	LOG_INFO_NOFMT("Un-registering global watchdog callback" CR);
	globalCb = NULL;
	globalCbParams = NULL;
}

void wdt::regGlobalFailsafeCb(const wdtCb_t* p_globalFailsafeCb,
							  const void* p_globalFailsafeCbParams) {
	LOG_INFO("Registering global failsafe watchdog callback: 0x%x" CR, p_globalFailsafeCb);
	globalFailsafeCb = p_globalFailsafeCb;
	globalFailsafeCbParams = p_globalFailsafeCbParams;
}
 
void wdt::unRegGlobalFailsafeCb(void) {
	LOG_INFO_NOFMT("Un-registering global failsafe watchdog callback" CR);
	globalFailsafeCb = NULL;
	globalFailsafeCbParams = NULL;
}

void wdt::regGlobalRebootCb(const wdtCb_t* p_globalRebootCb,
							const void* p_globalRebootCbParams) {
	LOG_INFO("Registering global reboot watchdog callback: 0x%x" CR, p_globalRebootCb);
	globalRebootCb = p_globalRebootCb;
	globalRebootCbParams = p_globalRebootCbParams;
}

void wdt::unRegGlobalRebootCb(void) {
	LOG_INFO_NOFMT("Un-registering global reboot watchdog callback" CR);
	globalRebootCb = NULL;
	globalRebootCbParams = NULL;
}

void IRAM_ATTR wdt::wdtTimerIsr(void) {												// Watchdog tick as defined by WD_TICK_MS elapsed.
	bool handleBackend = false;														//   Running in ISR context - keep it short and do not use system calls not meant for ISR
	if (++currentWdtTick == nextWdtTimeoutTick)										// We have a Watchdog timeout, needs to be handled by the backend task
		handleBackend = true;
	if (outstandingWdtTimeout)														// A watchdog timeout escalation ladder with escalation intergaps is ongoing,
		handleBackend = true;														//   needs to be handled by the backend task
	if (currentWdtTick % WDT_HOUR_TICK == 0) {										// Age/decrease expiry counters each hour
		ageExpiries = true;
		handleBackend = true;
	}
	if (handleBackend){
		BaseType_t xHigherPriorityTaskWoken = pdTRUE;
		if (xSemaphoreGiveFromISR(wdtProcessSemaphore, &xHigherPriorityTaskWoken) == pdTRUE) {
		}
		else {																		// Backend still running/stuck - no other alternatives left than reboot
			esp_restart();															// Hope for esp_restart system call to reboot the decoder
			while (true);															// As a last resort - spin, hoping that the idle loop WD catches it
		}
	}
	portYIELD_FROM_ISR();
}

void wdt::wdtHandlerBackend(void* p_args) {											// Watchdog back-end ever running task, when this loop runs we potentially
	while (true) {																	//   have a compromized system. Avoid system calls and logs up till the end
		xSemaphoreTake(wdtProcessSemaphore, portMAX_DELAY);							// Wait for a Watchdog timeout detected by the Interrupt handler
		xSemaphoreTake(wdtDescrListLock, portMAX_DELAY);
		for (uint16_t wdtDescrListItter = 0; wdtDescrListItter <					// Itterate all registered watch dogs
			 wdtDescrList->size(); wdtDescrListItter++) {
			if (ageExpiries && wdtDescrList->at(wdtDescrListItter)->wdtExpiries)	// Age the expiry counter
				wdtDescrList->at(wdtDescrListItter)->wdtExpiries--;
			if ((wdtDescrList->at(wdtDescrListItter)->wdtUpcommingTimeoutTick == 
				currentWdtTick ||
				wdtDescrList->at(wdtDescrListItter)->ongoingWdtTimeout) &&
				wdtDescrList->at(wdtDescrListItter)->isActive){						// A Watch dog timeout has occured
				if (!wdtDescrList->at(wdtDescrListItter)->ongoingWdtTimeout)
					wdtDescrList->at(wdtDescrListItter)->wdtExpiries++;				// Increase the history of watchdog expireies
				outstandingWdtTimeout = true;										// Mark that we are currently treating an ongoing Watch dog timeout escalation
				wdtDescrList->at(wdtDescrListItter)->ongoingWdtTimeout = true;
				if (wdtDescrList->at(wdtDescrListItter)->wdtExpiries >=				// Maximum wdt expiries occurred, unconditional reboot
					WDT_MAX_EXPIRIES) {
					panic("Maximum number of watchdog expiries reached for %s", wdtDescrList->at(wdtDescrListItter)->wdtDescription);
					currentWdtTick = currentWdtTick / 0;
					return;
				}
				bool escalationIntergapEnabled = wdtDescrList->						// If FAULTACTION_ESCALATE_INTERGAP is set there will be one escalatin step per
					at(wdtDescrListItter)->											//   tick, otherwise all escalation callbacks will happen at this Watch dog tick
					wdtAction & FAULTACTION_ESCALATE_INTERGAP;
				for (uint8_t i = 0; escalationIntergapEnabled? i < 1 : i < 6; i++){
					uint8_t wdtAction = wdtDescrList->at(wdtDescrListItter)->wdtAction;
					wdtAction = wdtAction >> 2;
					uint8_t escalationActionCnt = 0;
					uint8_t escalationCnt = wdtDescrList->at(wdtDescrListItter)->escalationCnt;
					for (escalationActionCnt = 0; escalationActionCnt < 6;			// Check what is the next requested escalation action for this watchdog
						 escalationActionCnt++) {
						if ((wdtAction & 1) && !escalationCnt--) {
							break;
						}
						wdtAction = wdtAction >> 1;
					}
					escalationCnt++;
					uint8_t nextAction = DONT_ESCALATE;
					switch (escalationActionCnt) {
						case 0:														// FAULTACTION_LOCAL0 callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"calling FAULTACTION_LOCAL0" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
									  "calling FAULTACTION_LOCAL0" CR, 
								      wdtDescrList->at(wdtDescrListItter)->wdtDescription);

							if (!wdtDescrList->at(wdtDescrListItter)->localCb)		// No callback registered - escalate!
								nextAction = ESCALATE;
							else													// Make the call, and let the receiver decide if further escalation is needed
								nextAction = wdtDescrList->at(wdtDescrListItter)->
											 localCb(0, wdtDescrList->at(wdtDescrListItter)->localCbParams);
							break;
						case 1:														// FAULTACTION_LOCAL1 callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_LOCAL1" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
									  "Escalation, calling FAULTACTION_LOCAL1" CR,
									  wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							if (!wdtDescrList->at(wdtDescrListItter)->localCb)		// No callback registered - escalate!
								nextAction = ESCALATE;
							else													// Make the call, and let the receiver decide if further escalation is needed
								nextAction = wdtDescrList->at(wdtDescrListItter)->
											 localCb(1, wdtDescrList->at(wdtDescrListItter)->localCbParams);
							break;
						case 2:														// FAULTACTION_LOCAL2 callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_LOCAL2" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_LOCAL2" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							if (!wdtDescrList->at(wdtDescrListItter)->localCb)		// No callback registered - escalate!
								nextAction = ESCALATE;
							else													// Make the call, and let the receiver decide if further escalation is needed
								nextAction = wdtDescrList->at(wdtDescrListItter)->
											 localCb(2, wdtDescrList->at(wdtDescrListItter)->localCbParams);
							break;
						case 3:														// FAULTACTION_GLOBAL_FAILSAFE callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_GLOBAL0" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_GLOBAL0" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							if (!globalCb)											// No callback registered - escalate!
								nextAction = ESCALATE;
							else {													// Make the call, and let the receiver decide if further escalation is needed
								nextAction = globalCb(3, globalFailsafeCbParams);
							}
							break;
						case 4:														// FAULTACTION_GLOBAL_FAILSAFE callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_GLOBAL_FAILSAFE" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_GLOBAL_FAILSAFE" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							if (!globalFailsafeCb)									// No callback registered - escalate!
								nextAction = ESCALATE;
							else{													// Make the call, and let the receiver decide if further escalation is needed
								nextAction = globalFailsafeCb(4, globalFailsafeCbParams);
							}
							break;
						case 5:														// FAULTACTION_GLOBAL_REBOOT callback requested and is scheduled for this escalation step
							Serial.printf(">>> Watchdog timer has expired for %s, " \
								"Escalation, calling FAULTACTION_GLOBAL_REBOOT" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							LOG_ERROR("Watchdog timer has expired for %s, " \
									  "Escalation, calling FAULTACTION_GLOBAL_REBOOT" CR,
								wdtDescrList->at(wdtDescrListItter)->wdtDescription);
							if (!globalRebootCb) {									// No callback/reboot handler registered - reboot immediatly - no way out from here
								Serial.printf(">>> Watchdog expired for %s but no global "		// Try an ordinary panic
									"watchdog reboot handler registered, "
									"rebooting..." CR,
									wdtDescrList->at(wdtDescrListItter)->
									wdtDescription);
								panic("Watchdog expired for %s but no global "		// Try an ordinary panic
									  "watchdog reboot handler registered, "		
									  "rebooting..." CR,
									  wdtDescrList->at(wdtDescrListItter)->
									  wdtDescription);
								vTaskDelay((PANIC_REBOOT_DELAY_MS + 1000) /			// FIX PANIC as a static class with methods for this ...GetPanicDelay from panic
											portTICK_PERIOD_MS);		
								esp_restart();										// Next - hope for esp_restart system call to reboot the decoder
								currentWdtTick = currentWdtTick / 0;				// If fail, try division by zero
								while (true);										// As a last resort - spin, hoping that the idle loop WD catches it
							}
							else{													// Call the registered callback/reboot handler - no way out from here
								nextAction = globalRebootCb(5, globalRebootCbParams);
								currentWdtTick = currentWdtTick / 0;				// If fail, try division by zero
								while (true);										// As a last resort - spin, hoping that the idle loop WD catches it
							}
							break;
					}
					if (nextAction == DONT_ESCALATE) {								// The Watchdog callback handler for this escalation step has indicated that the problem is solved
						outstandingWdtTimeout = false;								// Reseting the watchdog
						wdtDescrList->at(wdtDescrListItter)->escalationCnt = 0;
						wdtDescrList->at(wdtDescrListItter)->ongoingWdtTimeout = false;
						wdtDescrList->at(wdtDescrListItter)->wdtUpcommingTimeoutTick =
							currentWdtTick + wdtDescrList->at(wdtDescrListItter)->wdtTimeoutTicks;
						break;
					}
					else {
						if (escalationActionCnt < 4) {
							wdtDescrList->at(wdtDescrListItter)->escalationCnt++;	// Move to the next step on the escalation ladder
						}
						else {
							outstandingWdtTimeout = false;							//WHAT DO WE DO HERE?
						}
					}
				}
			}
		}
		ageExpiries = false;
		nextWdtTimeoutTick = nextTickToHandle(false);
		xSemaphoreGive(wdtDescrListLock);
	}
}

bool wdt::isWdtTickAhead(uint32_t p_testWdtTick, uint32_t p_comparedWithWdtTick) {
	bool curentWdtTickInbetween = ((p_testWdtTick >= currentWdtTick) &&
								   (p_comparedWithWdtTick < p_comparedWithWdtTick)) ||
								  ((p_testWdtTick <= p_comparedWithWdtTick) &&
								   (p_comparedWithWdtTick > p_comparedWithWdtTick));
	bool isAhead = ((p_testWdtTick > p_comparedWithWdtTick) && !curentWdtTickInbetween) ||
				   ((p_testWdtTick < p_comparedWithWdtTick) && curentWdtTickInbetween);
	return isAhead;
}

uint32_t wdt::nextTickToHandle(bool p_lock) {
	if (p_lock)
		xSemaphoreTake(wdtProcessSemaphore, portMAX_DELAY);
	uint32_t nextTickToHandle = UINT32_MAX;
	for (uint16_t wdtDescrListItter; wdtDescrListItter <
		wdtDescrList->size(); wdtDescrListItter++) {
		if (!wdtDescrList->at(wdtDescrListItter)->isActive)
			continue;
		if (wdtDescrList->at(wdtDescrListItter)->wdtUpcommingTimeoutTick < nextTickToHandle)
			nextTickToHandle = wdtDescrList->at(wdtDescrListItter)->wdtUpcommingTimeoutTick;
	}
	if (p_lock)
		xSemaphoreGive(wdtProcessSemaphore);
	return nextTickToHandle;
}

char* wdt::actionToStr(char* p_actionStr, uint8_t p_size, action_t p_action) {
	if (p_action)
		strcpy(p_actionStr, "");
	else {
		strcpy(p_actionStr, "-");
		return p_actionStr;
	}
	if (p_action & FAULTACTION_ESCALATE_INTERGAP)
		strcatTruncMaxLen(p_actionStr, "ESC_GAP|", p_size - 4);
	if (p_action & FAULTACTION_LOCAL0)
		strcatTruncMaxLen(p_actionStr, "LOC0|", p_size - 4);
	if (p_action & FAULTACTION_LOCAL1)
		strcatTruncMaxLen(p_actionStr, "LOC1|", p_size - 4);
	if (p_action & FAULTACTION_LOCAL2)
		strcatTruncMaxLen(p_actionStr, "LOC2|", p_size - 4);
	if (p_action & FAULTACTION_GLOBAL0)
		strcatTruncMaxLen(p_actionStr, "GLB0|", p_size - 4);
	if (p_action & FAULTACTION_GLOBAL_FAILSAFE)
		strcatTruncMaxLen(p_actionStr, "GLBFSF|", p_size - 4);
	if (p_action & FAULTACTION_GLOBAL_REBOOT)
		strcatTruncMaxLen(p_actionStr, "REBOOT|", p_size - 4);
	if (strlen(p_actionStr) >= p_size - 4)
		strcatTruncMaxLen(p_actionStr, "...", p_size - 1);
	if (p_actionStr[strlen(p_actionStr) - 1] == '|')
		p_actionStr[strlen(p_actionStr) - 1] = '\0';
	return p_actionStr;
}

uint16_t wdt::maxId(void) {
	uint16_t maxId = 0;
	for (uint16_t wdtDescrListItter = 0; wdtDescrListItter <
		wdtDescrList->size(); wdtDescrListItter++) {
		if (wdtDescrList->at(wdtDescrListItter)->id > maxId)
			maxId = wdtDescrList->at(wdtDescrListItter)->id;
	}
	return wdtObjCnt;
}
/*==============================================================================================================================================*/
/* END Class wdt																																*/
/*==============================================================================================================================================*/
