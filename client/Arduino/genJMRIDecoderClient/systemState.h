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
#ifndef SYSSTATE_H
#define SYSSTATE_H
/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include <QList.h>
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "logHelpers.h"
#include "panic.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: systemState                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

//Operational state bitmap
typedef uint16_t sysState_t;
#define OP_WORKING                      0b0000000000   //Object working
#define OP_INIT                         0b0000000001   //Object initializing, not started
#define OP_DISCONNECTED                 0b0000000010   //Object disconnected to its peer
#define OP_NOIP                         0b0000000100   //Object has no IP address
#define OP_UNDISCOVERED                 0b0000001000   //Object not discovered
#define OP_UNCONFIGURED                 0b0000010000   //Object not configured
#define OP_DISABLED                     0b0000100000   //Object disbled from server
#define OP_UNAVAILABLE                  0b0001000000   //Object unavailable from server
#define OP_INTFAIL                      0b0010000000   //Object internal failure
#define OP_CBL                          0b0100000000   //Object control-block from any parent block reasons
#define OP_UNUSED                       0b1000000000   //Object not in use


//Call-back prototypes
typedef void (*sysStateCb_t)(const void* p_miscCbData, sysState_t p_sysState);

struct cb_t {
    sysStateCb_t cb;
    void* miscCbData;
};

class systemState {
public:
    //Public methods
    systemState(systemState* p_parent);
    ~systemState(void);
    rc_t regSysStateCb(void* p_miscCbData, sysStateCb_t p_cb);
    rc_t unRegSysStateCb(const sysStateCb_t p_cb);
    void addSysStateChild(systemState* p_child);
    void delSysStateChild(systemState* p_child);
    void setOpState(sysState_t p_opStateMap);
    void unSetOpState(sysState_t p_opStateMap);
    uint16_t getOpState(void);
    char* getOpStateStr(char* p_opStateStr, sysState_t p_opBitmap);
    char* getOpStateStr(char* p_opStateStr);
    void setSysStateObjName(const char* p_objName);
    void updateObjName(void);
    char* getSysStateObjName(char* p_objName);
    const char* getSysStateObjName(void);
    void updateObjOpStates(void);

    //Public data structures
    //--
private:
    //Private methods
    //--

    //Private data structures
    static uint16_t sysStateIndex;
    systemState* parent;
    char* objName;
    sysState_t opState;
    QList<cb_t*>* cbList;
    QList<systemState*>* childList;
};

#endif SYSSTATE_H
