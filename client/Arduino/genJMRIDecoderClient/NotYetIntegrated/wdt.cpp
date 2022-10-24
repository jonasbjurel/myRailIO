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
#include "wdt.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
wdt::wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction) {
	wdtData = new wdt_t;
	wdtData->wdtTimeout = p_wdtTimeout * 1000;
	strcpy(wdtData->wdtDescription, p_wdtDescription);
	wdtData->wdtAction = p_wdtAction;
	wdtTimer_args.arg = this;
	wdtTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&wdt::kickHelper);
	wdtTimer_args.dispatch_method = ESP_TIMER_TASK;
	wdtTimer_args.name = "FlashTimer";
	esp_timer_create(&wdtTimer_args, &wdtData->timerHandle);
	esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
}

wdt::~wdt(void) {
	esp_timer_stop(wdtData->timerHandle);
	esp_timer_delete(wdtData->timerHandle);
	delete wdtData;
	return;
}

void wdt::feed(void) {
	esp_timer_stop(wdtData->timerHandle);
	esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
	return;
}

void wdt::kickHelper(wdt* p_wdtObject) {
	p_wdtObject->kick();
}
void wdt::kick(void) { //FIX
	return;
}
/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                         */
/*==============================================================================================================================================*/