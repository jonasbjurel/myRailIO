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
/* Purpose: The wdt class provides watch services to monitor certain other services liveness.                                                   */
/*          A consumer needs to register with a wdt class object, define the watchdog timeout as well as wanted actions should the watchdog get */
/*            kicked. Once the watchdog is set-up and activated, the consumer need to continously feed it with time intervals less than the     */
/*            configured timeout. If the consumer fails to feed the watchdog in a timely manner, the watchdog will be kicked and an             */
/*            escalation ladder will be started, running the defined actions in a series, starting from local actions, escalating into globala  */
/*            actions, eventually leading to a reboot of the device. If an action believs it has successfully fixed the issue, it can stop the  */
/*            escalation ladder - resuming the normal watchdog operation.                                                                       */
/*            If the FAULTACTION_ESCALATE_INTERGAP action is set, the escalation ladder runs with one watchdog tick gap inbetween the actions,  */
/*            otherwise there is no gap inbetween.                                                                                              */
/*          Watchdog kick actions can eather be used to try to remedy the failure which caused the watchdog to be kicked, or to set a failsafe  */
/*            operation.                                                                                                                        */
/*          Every time a watchdog is kicked - an expirey counter is incremented for that specific watchdog, and every hour, that expirey        */
/*            is decremented by one. Should the expiriey counter reach MAX_EXPIRIES defined below, the watchdog will start a global expiriry    */
/*            ladder starting from FAULTACTION_GLOBAL_FAILSAFE moving towards FAULTACTION_GLOBAL_REBOOT                                         */
/* Methods: See below.                                                                                                                          */
/* Data structures: See below                                                                                                                   */
/*==============================================================================================================================================*/
class wdt;

typedef uint8_t action_t ;                                                          // Watchdog action/escalation ladder bitmap, the escalations goes from FAULTACTION_LOCAL0 towards FAULTACTION_GLOBAL_REBOOT
#define FAULTACTION_GLOBAL_REBOOT           1<<7                                    // Reboots the entire decoder, first calls the globalRebootCb, and when it returns a divide by 0 exemption is created
#define FAULTACTION_GLOBAL_FAILSAFE         1<<6                                    // Failsafe the entire decoder, calls the globalFailsafeCb responsible for failsafing the decoder.
#define FAULTACTION_GLOBAL0                 1<<5                                    // 1st (and only) Global action intended for restoring operation, calls the globalCb responsible for trying to restore operation
#define FAULTACTION_LOCAL2                  1<<4                                    // Third local action callback in the escalation ladder, all local actions calls the wdtDescr->localCb.
#define FAULTACTION_LOCAL1                  1<<3                                    // Second local action callback in the escalation ladder, all local actions calls the wdtDescr->localCb.
#define FAULTACTION_LOCAL0                  1<<2                                    // First local action callback in the escalation ladder, all local actions calls the wdtDescr->localCb.
#define FAULTACTION_ESCALATE_INTERGAP       1<<0                                    // If set, the escalation ladder from FAULTACTION_LOCAL0 to FAULTACTION_REBOOT will be intergapped with one WDT tick

#define NO_OF_OUTSTANDING_WDT_TIMEOUT_JOBS  1                                       // Queue up to 1 outstanding jobs for the wdtHandlerBackend to keep up with

#define CPU_WDT_BACKEND_TASKNAME            "Watchdog"                              // ISR Backend task
#define CPU_WDT_BACKEND_STACKSIZE_1K        6                                       // ISR Backend task stack size, must be dimentioned to run the ISR backend + the action callbacks
#define CPU_WDT_BACKEND_PRIO                24                                      // The ISR backend runs with the highest priority on the system

#define WD_TICK_MS                          100                                     // Number of millis per wdt tick, highest resolution of the watchdog timeout.
#ifndef WDT_TIMER
#define WDT_TIMER                           2                                       // Hardware timer No
#endif WDT_TIMER

typedef uint8_t(wdtCb_t)(uint8_t escalationCnt,                                     // Watchdog kick action callback, carries the count of the escalation ladder,
                         const void* p_parms);                                      //    as well as params provided with the callback registration

                                                                                    // Return values from Watchdog kick action callbacks
#define DONT_ESCALATE                       0                                       //   Stop the escalation ladder
#define ESCALATE                            1                                       //   Continue the escalation ladder

struct wdt_t {                                                                      // Watchdog descriptor
    uint16_t id;                                                                    //   ID for the watchdog
    wdt* handle;                                                                    //   Handle for the watchdog class object
    char* wdtDescription;                                                           //   Textual description of the watchdog
    bool isActive;                                                                  //   Watchdog active state, if not active the watchdog will never be kicked
    bool isInhibited;                                                               //   Watchdog inhibit state, if inhibited - feeds will be ignored, starving the watchdog
    uint32_t wdtTimeoutTicks;                                                       //   Watchdog timeout ticks, defined by WD_TICK_MS
    uint32_t wdtUpcommingTimeoutTick;                                               //   Next global Watchdog tick to kick the watchdog
    uint8_t escalationCnt;                                                          //   Current Watchdog escalation cnt (stair cnt in the escalation ladder)
    wdtCb_t* localCb;                                                               //   Local callback function to call for FAULTACTION_LOCAL0 - FAULTACTION_LOCAL2 in case the watchdog is kicked
    void* localCbParams;                                                            //   Local callback parameters
    bool ongoingWdtTimeout;                                                         //   A watchdog kick escalation ladder is running for this watchdog
    uint8_t wdtAction;                                                              //   action_t watchdog action/escalation ladder bitmap for this watchdog
    uint16_t wdtExpiries;                                                           //   Number of watchdog kicks/expiries, decremented every hour.
    uint32_t closesedhit;                                                           //   The closesed the watchdog has been from a kick (ms)
};

typedef QList<wdt_t*> wdtDescrList_t;                                               // Type: List of watchdog descriptors



class wdt {
public:
    //Methods
    wdt(uint32_t p_wdtTimeoutMs, char* p_wdtDescription,                            // Watchdog class object constructor, defining: Watchdog timeout in ms, Watchdog description,
        action_t p_wdtAction);                                                      //   and Watchdog kick actions as an action_t bitmap
    ~wdt(void);                                                                     // Watchdog class object destructor
    static rc_t setActiveAll(bool p_active);                                        // Activate/Deactivate all watchdogs
    static rc_t setActive(uint16_t p_id, bool p_active);                            // Activate/Deactivate watchdog for watchdog id
    void activate(bool p_lock = true);                                              // Activate watchdog for current class object
    void inactivate(bool p_lock = true);                                            // In-Activate watchdog for current class object
    static rc_t setTimeout(uint16_t p_id,                                           // Set watchdog timeout (ms) for watchdog id
                           uint32_t p_timeoutMs);
    void setTimeout(uint32_t p_timeoutMs);                                          // Set watchdog timeout (ms) for current class object
    static rc_t setActionsFromStr(uint16_t p_id,                                    // Set action from string for watchdog id
                                  char* p_actionStr);                               //   Action|Action|Action
    rc_t setActionsFromStr(char* p_actionStr,                                       // Set action from string for current class object
                           bool p_lock = true);                                     //   Action|Action|Action
    void regLocalWdtCb(const wdtCb_t* p_localWdtCb,                                 // Register local action callback (FAULTACTION_LOCAL0|FAULTACTION_LOCAL1|FAULTACTION_LOCAL2)
                       const void* p_localWdtCbParms = NULL);
    void unRegLocalWdtCb(void);                                                     // Un-register local action callback
    static void regGlobalCb(const wdtCb_t* p_globalCb,                              // Register global action callback (FAULTACTION_GLOBAL0)
                            const void* p_globalCbParams = NULL);
    static void unRegGlobalCb(void);                                                // Un-register global action callback
    static void regGlobalFailsafeCb(const wdtCb_t* p_globalFailsafeCb,              // Register global failsafe action callback (FAULTACTION_GLOBAL_FAILSAFE)
                                    const void* p_globalFailsafeCbParams = NULL);
    static void unRegGlobalFailsafeCb(void);                                        // Un-register global failsafe action callback
    static void regGlobalRebootCb(const wdtCb_t* p_globalRebootCb,                  // Register global reboot action callback (FAULTACTION_GLOBAL_REBOOT)
                                  const void* p_globalRebootCbParams = NULL);
    static void unRegGlobalRebootCb(void);                                          // Un-register global reboot action callback
    static void setDebug(bool p_debug);                                             // Set/Un-set debug mode - enabling various set options
    static bool getDebug(void);                                                     // Get debug mode
    static rc_t getWdtDescById(uint16_t p_id,                                       // Get watchdog descriptor by watchdog Id
                               wdt_t** p_descr);
    static void clearExpiriesAll(void);                                             // Clear all watchdog expiries.
    static rc_t clearExpiries(uint16_t p_id);                                       // Clear watchdog expiries by watchdog Id
    void clearExpiries(void);                                                       // Clear watchdog expiries for current class object
    static void clearClosesedHitAll(void);                                          // Clear all watchdog closesed hit
    static rc_t clearClosesedHit(uint16_t p_id);                                    // Clear watchdog closesed hit by watchdog Id
    void clearClosesedHit(void);                                                    // Clear watchdog closesed hit for current class object
    static rc_t inhibitAllWdtFeeds(bool p_inhibit);                                 // Inhibit feed of all watchdogs, leaving them to starve
    static rc_t inhibitWdtFeeds(uint16_t p_id,                                      // Inhibit feed of watchdog by Id, leaving it to starve
                                bool p_inhibit);
    void inhibitWdtFeeds(bool p_inhibit);                                           // Inhibit feed of watchdog for current class object, leaving it to starve
    void feed(void);                                                                // Feed the watchdog, making it happy
    static char* actionToStr(char* p_actionStr,                                     // Create a string out of a action_t action bitmap
                             uint8_t size,
                             action_t p_action);
    static uint16_t maxId(void);                                                    // Return the highest watchdog Id (not the number of watchdogs)

    //Data structures
    //--

private:
    //Methods
    static bool isWdtTickAhead(uint32_t p_testWdtTick, 
                               uint32_t p_comparedWithWdtTick);
    static uint32_t nextTickToHandle(bool p_lock = true);                           // Itterates all watchdogs and returns the next watchdog tick that needs treatment
    static void wdtTimerIsr(void);                                                  // Watchdog timer interrupt handler
    static void wdtHandlerBackend(void* p_args);                                    // Watchdog timer interrupt backend handler

    //Data structures
    wdt_t* wdtDescr;                                                                // Watchdog descriptor
    static SemaphoreHandle_t wdtProcessSemaphore;                                   // Semaphore for request handling between the watchdog timer interrupt handler and the backend handler
    static SemaphoreHandle_t wdtDescrListLock;                                      // Lock protecting the watchdog descriptors and the linked list of those handlers
    static TaskHandle_t backendTaskHandle;                                          // Watchdog timer interrupt backend handler task handle
    static uint16_t wdtObjCnt;                                                      // Watchdog object count
    static hw_timer_t* wdtTimer;                                                    // Watchdog HW timer handle
    static const wdtCb_t* globalCb;                                                 // Global action callback
    static const void* globalCbParams;                                              // Global action callback params
    static const wdtCb_t* globalFailsafeCb;                                         // Global failsafe action callback
    static const void* globalFailsafeCbParams;                                      // Global failsafe action callback params
    static const wdtCb_t* globalRebootCb;                                           // Global reboot action callback
    static const void* globalRebootCbParams;                                        // Global reboot action callback params
    static wdtDescrList_t* wdtDescrList;                                            // List of all watchdog descriptors
    static uint32_t currentWdtTick;                                                 // Current watchdog tick
    static uint32_t nextWdtTimeoutTick;                                             // Next watchdog tick needing care
    static bool outstandingWdtTimeout;                                              // A watchdog timeout/kick is being handled
    static bool debug;                                                              // Watchdog debug flag
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/

#endif //WDT_H
