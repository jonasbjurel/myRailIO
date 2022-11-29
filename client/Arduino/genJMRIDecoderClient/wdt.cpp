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

wdtCbLists_t wdt::wdtCbLists;

wdt::wdt(uint16_t p_wdtTimeout, const char* p_wdtDescription, uint8_t p_wdtAction) {
    wdtData = new wdt_t;
    wdtData->wdtTimeout = p_wdtTimeout * 1000;
    wdtData->wdtDescription = createNcpystr(p_wdtDescription);
    wdtData->wdtAction = p_wdtAction;
    wdtTimer_args.arg = this;
    wdtTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&wdt::kickHelper);
    wdtTimer_args.dispatch_method = ESP_TIMER_TASK;
    wdtTimer_args.name = p_wdtDescription;
    esp_timer_create(&wdtTimer_args, &wdtData->timerHandle);
    esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
}

wdt::~wdt(void) {
    esp_timer_stop(wdtData->timerHandle);
    esp_timer_delete(wdtData->timerHandle);
    delete wdtData;
    return;
}

void wdt::wdtRegLgFailsafe(wdtCb_t* p_wdtLgFailsafeCb, void* p_wdtCbParms) {
    wdtCbParms_t* cbParms = new wdtCbParms_t;
    cbParms->wdtCb = p_wdtLgFailsafeCb;
    cbParms->wdtCbHandle = p_wdtCbParms;
    wdtCbLists.lgsWdtCbs.push_back(cbParms);
}

void wdt::wdtUnRegLgFailsafe(wdtCb_t* p_wdtLgFailsafeCb) {
    for (uint16_t cbIndex = 0; wdtCbLists.lgsWdtCbs.length(); cbIndex++) {
        if (wdtCbLists.lgsWdtCbs.at(cbIndex)->wdtCb == p_wdtLgFailsafeCb) {
            delete wdtCbLists.lgsWdtCbs.at(cbIndex);
            wdtCbLists.lgsWdtCbs.clear(cbIndex);
        }
    }
}

void wdt::wdtRegSensorFailsafe(wdtCb_t* p_wdtSensorFailsafeCb, void* p_wdtCbParms) {
    wdtCbParms_t* cbParms = new wdtCbParms_t;
    cbParms->wdtCb = p_wdtSensorFailsafeCb;
    cbParms->wdtCbHandle = p_wdtCbParms;
    wdtCbLists.sensorsWdtCbs.push_back(cbParms);
}

void wdt::wdtUnRegSensorFailsafe(wdtCb_t* p_wdtSensorFailsafeCb) {
    for (uint16_t cbIndex = 0; wdtCbLists.sensorsWdtCbs.length(); cbIndex++) {
        if (wdtCbLists.sensorsWdtCbs.at(cbIndex)->wdtCb == p_wdtSensorFailsafeCb) {
            delete wdtCbLists.sensorsWdtCbs.at(cbIndex);
            wdtCbLists.sensorsWdtCbs.clear(cbIndex);
        }
    }
}

void wdt::wdtRegActuatorFailsafe(wdtCb_t* p_wdtActuatorFailsafeCb, void* p_wdtCbParms) {
    wdtCbParms_t* cbParms = new wdtCbParms_t;
    cbParms->wdtCb = p_wdtActuatorFailsafeCb;
    cbParms->wdtCbHandle = p_wdtCbParms;
    wdtCbLists.actuatorsWdtCbs.push_back(cbParms);
}

void wdt::wdtUnRegActuatorFailsafe(wdtCb_t* p_wdtActuatorFailsafeCb) {
    for (uint16_t cbIndex = 0; wdtCbLists.actuatorsWdtCbs.length(); cbIndex++) {
        if (wdtCbLists.sensorsWdtCbs.at(cbIndex)->wdtCb == p_wdtActuatorFailsafeCb) {
            delete wdtCbLists.actuatorsWdtCbs.at(cbIndex);
            wdtCbLists.actuatorsWdtCbs.clear(cbIndex);
        }
    }
}

void wdt::wdtRegMqttDown(wdtCb_t* p_wdtMqttDownCb, void* p_wdtCbParms) {
    wdtCbParms_t* cbParms = new wdtCbParms_t;
    cbParms->wdtCb = p_wdtMqttDownCb;
    cbParms->wdtCbHandle = p_wdtCbParms;
    wdtCbLists.mqttDownWdtCbs.push_back(cbParms);
}

void wdt::wdtUnRegMqttDown(wdtCb_t* p_wdtMqttDownCb) {
    for (uint16_t cbIndex = 0; wdtCbLists.mqttDownWdtCbs.length(); cbIndex++) {
        if (wdtCbLists.mqttDownWdtCbs.at(cbIndex)->wdtCb == p_wdtMqttDownCb) {
            delete wdtCbLists.mqttDownWdtCbs.at(cbIndex);
            wdtCbLists.mqttDownWdtCbs.clear(cbIndex);
        }
    }
}

void wdt::feed(void) {
    esp_timer_stop(wdtData->timerHandle);
    esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
    return;
}

void wdt::kickHelper(wdt* p_wdtObject) {
    p_wdtObject->kick();
}

void wdt::kick(void) {
    if (wdtData->wdtAction & FAULTACTION_FAILSAFE_ACTUATORS)
        callFailsafeCbs(&(wdtCbLists.actuatorsWdtCbs));
    if (wdtData->wdtAction & FAULTACTION_FAILSAFE_LGS)
        callFailsafeCbs(&(wdtCbLists.lgsWdtCbs));
    if (wdtData->wdtAction & FAULTACTION_FAILSAFE_SENSORS)
        callFailsafeCbs(&(wdtCbLists.sensorsWdtCbs));
    if (wdtData->wdtAction & FAULTACTION_FREEZE) {
        //Not yet implemented
    }
    if (wdtData->wdtAction & FAULTACTION_MQTTDOWN)
        callFailsafeCbs(&(wdtCbLists.mqttDownWdtCbs));
    if (wdtData->wdtAction & FAULTACTION_DUMP_LOCAL) {
        //Not yet implemented
    }
    if (wdtData->wdtAction & FAULTACTION_DUMP_ALL) {
        //Not yet implemented
    }
    if (wdtData->wdtAction & FAULTACTION_REBOOT) {
        callFailsafeCbs(&(wdtCbLists.mqttDownWdtCbs));
        panic("wdt::kick: Watchdog triggered - rebooting...");
    }
}

void wdt::callFailsafeCbs(QList<wdtCbParms_t*>* p_wdtCbList) {
    for (uint16_t cbIndex = 0; cbIndex < p_wdtCbList->length(); cbIndex++)
        p_wdtCbList->at(cbIndex)->wdtCb(p_wdtCbList->at(cbIndex)->wdtCbHandle);
}

/*==============================================================================================================================================*/
/* END Class wdt																																*/
/*==============================================================================================================================================*/
