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

#ifndef WDT_H
#define WDT_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/timer.h>
#include <esp_clk.h>
#include <QList.h>
#include "panic.h"
#include "strHelpers.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
class wdt;
typedef uint8_t action_t ;
#define FAULTACTION_GLOBAL_REBOOT           1<<7        // Reboots the entire decoder
#define FAULTACTION_GLOBAL_FAILSAFE         1<<6        // Failsafe the entire decoder
#define FAULTACTION_GLOBAL0                 1<<5        // 1st Global action callback in the escalation ladder
#define FAULTACTION_LOCAL2                  1<<4        // Third local action callback in the escalation ladder
#define FAULTACTION_LOCAL1                  1<<3        // Second local action callback in the escalation ladder
#define FAULTACTION_LOCAL0                  1<<2        // First local action callback in the escalation ladder
#define FAULTACTION_ESCALATE_INTERGAP       1<<0        // The escalation ladder from FAULTACTION_LOCAL0 to FAULTACTION_REBOOT will be intergapped with one WDT tick

#define NO_OUTSTANDING_WDT_TIMEOUT_JOBS     1           // Queue up to 1 outstanding jobs for the wdtHandlerBackend to keep up with

#define CPU_WDT_BACKEND_TASKNAME            "Watchdog"
#define CPU_WDT_BACKEND_STACKSIZE_1K        6
#define CPU_WDT_BACKEND_PRIO                24          // Highest priority on the system

#define WD_TICK_MS                          100
#define TIMER                               2

typedef uint8_t(wdtCb_t)(uint8_t escalationCnt, const void* p_parms);

//returntypes from CB
#define DONT_ESCALATE                       0
#define ESCALATE                            1

struct wdt_t {
    uint16_t id;
    wdt* handle;
    char* wdtDescription;
    bool isActive;
    bool isInhibited;
    uint32_t wdtTimeoutTicks;
    uint32_t wdtUpcommingTimeoutTick;
    uint8_t escalationCnt;
    wdtCb_t* localCb;
    void* localCbParams;
    bool ongoingWdtTimeout;
    uint8_t wdtAction;
    uint16_t wdtExpieries;
    uint32_t closesedhit;
};

typedef QList<wdt_t*> wdtDescrList_t;



class wdt {
public:
    //Methods
    wdt(uint32_t p_wdtTimeoutMs, char* p_wdtDescription, action_t p_wdtAction);
    ~wdt(void);
    static rc_t setActiveAll(bool p_active);
    static rc_t setActive(uint16_t p_id, bool p_active);
    void activate(bool p_lock = true);
    void inactivate(bool p_lock = true);
    static rc_t setTimeout(uint16_t p_id, uint32_t p_timeoutMs);
    void setTimeout(uint32_t p_timeoutMs);
    static rc_t setActionsFromStr(uint16_t p_id, char* p_actionStr);
    rc_t setActionsFromStr(char* p_actionStr, bool p_lock = true);
    void regLocalWdtCb(const wdtCb_t* p_localWdtCb, const void* p_localWdtCbParms = NULL);
    void unRegLocalWdtCb(void);
    static void regGlobalFailsafeCb(const wdtCb_t* p_globalFailsafeCb, const void* p_globalFailsafeCbParams = NULL);
    static void unRegGlobalFailsafeCb(void);
    static void regGlobalRebootCb(const wdtCb_t* p_globalRebootCb, const void* p_globalRebootCbParams = NULL);
    static void unRegGlobalRebootCb(void);
    static void setDebug(bool p_debug);
    static bool getDebug(void);
    static rc_t getWdtDescById(uint16_t p_id, wdt_t** p_descr);
    static void clearExpiriesAll(void);
    static rc_t clearExpiries(uint16_t p_id);
    void clearExpiries(void);
    static void clearClosesedHitAll(void);
    static rc_t clearClosesedHit(uint16_t p_id);
    void clearClosesedHit(void);
    static rc_t inhibitAllWdtFeeds(bool p_inhibit);
    static rc_t inhibitWdtFeeds(uint16_t p_id, bool p_inhibit);
    void inhibitWdtFeeds(bool p_inhibit);
    void feed(void);
    static char* actionToStr(char* p_actionStr, uint8_t size, action_t p_action);
    static uint16_t maxId(void);

    //Data structures
    //--

private:
    //Methods
    static bool isWdtTickAhead(uint32_t p_testWdtTick, uint32_t p_comparedWithWdtTick);
    static uint32_t nextTickToHandle(bool p_lock = true);
    static void wdtTimerIsrCb(void);
    static void wdtHandlerBackend(void* p_args);

    //Data structures
    wdt_t* wdtDescr;
    static SemaphoreHandle_t wdtProcessSemaphore;
    static SemaphoreHandle_t wdtDescrListLock;
    static TaskHandle_t backendTaskHandle;
    static uint16_t wdtObjCnt;
    //static timer_config_t* wdtTimerConfig;
    static hw_timer_t* wdtTimer;
    static const wdtCb_t* globalFailsafeCb;
    static const void* globalFailsafeCbParams;
    static const wdtCb_t* globalRebootCb;
    static const void* globalRebootCbParams;
    static wdtDescrList_t* wdtDescrList;
    static uint32_t currentWdtTick;
    static uint32_t nextWdtTimeoutTick;
    static bool outstandingWdtTimeout;
    static bool debug;
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/

#endif //WDT_H
