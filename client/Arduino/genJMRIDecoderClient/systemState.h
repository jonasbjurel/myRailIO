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
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/


//Operational state bitmap
#define OP_WORKING                      0b000000000   //Object working
#define OP_INIT                         0b000000001   //Object initializing, not started
#define OP_DISCONNECTED                 0b000000010   //Object disconnected from MQTT
#define OP_UNDISCOVERED                 0b000000100   //Object not discovered
#define OP_UNCONFIGURED                 0b000001000   //Object not configured
#define OP_DISABLED                     0b000010000   //Object disbled from server
#define OP_UNAVAILABLE                  0b000100000   //Object unavailable from server
#define OP_INTFAIL                      0b001000000   //Object internal failure
#define OP_CBL                          0b010000000   //Object control-block from any parent block reasons
#define OP_UNUSED                       0b100000000



/*==============================================================================================================================================*/
/* Class: systemState                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

//Call-back prototypes
typedef void (*sysStateCb_t)(const void* p_miscCbData, uint16_t p_sysState);

class systemState {
public:
    //Public methods
    systemState(const void* p_parent);
    ~systemState(void);
    void regSysStateCb(void* p_miscCbData, const sysStateCb_t p_cb);
    void addSysStateChild(void* p_child);
    void delSysStateChild(void* p_child);
    void setOpState(uint16_t p_opStateMap);
    void unSetOpState(uint16_t p_opStateMap);
    uint16_t getOpState(void);
    char* getOpStateStr(char* p_opStateStr, uint16_t p_opBitmap);
    char* getOpStateStr(char* p_opStateStr);

    void updateObjOpStates(void);

    //Public data structures
    //--
private:
    //Private methods
    //--

    //Private data structures
    const void* parent;
    sysStateCb_t cb;
    void* miscCbData;
    uint16_t opState;
    QList<void*>* childList;
};

#endif SYSSTATE_H
