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
#include "systemState.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: systemState - base class                                                                                                              */
/* Purpose: The "systemState" base class implements methods for handling system-state bit-maps representing the state of the objects inheriting */
/*          this class                                                                                                                          */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
systemState::systemState(const void* p_parent) {
    Log.INFO("systemState::systemState: Creating systemState object to parent object %d" CR, p_parent);
    parent = p_parent;
    opState = 0;
    childList = new QList<void*>;
}

systemState::~systemState(void) {
    Log.INFO("systemState::~systemState: Deleting systemState object for object %d" CR, parent);
    if (childList->size() > 0)
        Log.WARN("systemState::~systemState: SystemState object for object %d still hav registered childs, the reference to those will be deleted" CR, parent);
    childList->clear();
    delete childList;
}

void systemState::regSysStateCb(void* p_miscCbData, const sysStateCb_t p_cb) {
    Log.INFO("systemState::regSysStateCb: Registering systemState callback %d to parent object %d" CR, p_cb, parent);
    cb = p_cb;
    miscCbData = p_miscCbData;
}

void systemState::addSysStateChild(void* p_child) {
    if (childList->indexOf(p_child) >= 0) {
        Log.WARN("systemState::addSysStateChild: Child object %d already exists for object %d - doing nothing" CR, p_child, parent);
        return;
    }
    else {
        Log.INFO("systemState::addSysStateChild: adding child object %d to parent object %d" CR, p_child, parent);
        childList->push_back(p_child);
    }
}

void systemState::delSysStateChild(void* p_child) {
    int i;
    i = childList->indexOf(p_child);
    if (i < 0) {
        Log.WARN("systemState::delChild: Child does not exist - doing nothing" CR);
    }
    else {
        Log.INFO("systemState::delChild: deleting child object %d to parent object %d" CR, p_child, parent);
        childList->clear(i);
    }
}

void systemState::setOpState(const uint16_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState | p_opStateMap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStr(currentOpStr, opState);
        char previousOpStr[100];
        getOpStateStr(previousOpStr, prevOpState);
        Log.INFO("systemState::unSetOpState: opState has changed for object %d, previous opState: %s, current opState: %s" CR, parent, previousOpStr, currentOpStr);
        updateObjOpStates();
    }
}

void systemState::unSetOpState(const uint16_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState & ~p_opStateMap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStr(currentOpStr, opState);
        char previousOpStr[100];
        getOpStateStr(previousOpStr, prevOpState);
        Log.INFO("systemState::unSetOpState: opState has changed for object %d, previous opState: %s, current opState: %s" CR, parent, previousOpStr, currentOpStr);
        updateObjOpStates();
    }
}

uint16_t systemState::getOpState(void) {
    return opState;
}

char* systemState::getOpStateStr(char* p_opStateStr, uint16_t p_opBitmap) {
    strcpy(p_opStateStr, "");
    if (p_opBitmap & OP_INIT)
        strcat(p_opStateStr, "INIT|");
    if (p_opBitmap & OP_DISCONNECTED)
        strcat(p_opStateStr, "DISCONNECTED|");
    if (p_opBitmap & OP_UNDISCOVERED)
        strcat(p_opStateStr, "UNDISCOVERED|");
    if (p_opBitmap & OP_UNCONFIGURED)
        strcat(p_opStateStr, "UNCONFIGURED|");
    if (p_opBitmap & OP_DISABLED)
        strcat(p_opStateStr, "DISABLED|");
    if (p_opBitmap & OP_INTFAIL)
        strcat(p_opStateStr, "INTFAIL|");
    if (p_opBitmap & OP_CBL)
        strcat(p_opStateStr, "CBL|");
    if (p_opBitmap & OP_UNUSED)
        strcat(p_opStateStr, "UNUSED|");
    if (strlen(p_opStateStr))
        p_opStateStr[strlen(p_opStateStr) - 1] = '\0';
    else
        strcpy(p_opStateStr, "WORKING");
    if (!strlen(p_opStateStr))
        strcpy(p_opStateStr, "WORKING");
    return p_opStateStr;
}

char* systemState::getOpStateStr(char* p_opStateStr) {
    return getOpStateStr(p_opStateStr, opState);
}

void systemState::updateObjOpStates(void) {
    if (cb == NULL) {
        Log.INFO("systemState::updateObjOpStates: opState has changed for object %d, but no registered call-back to inform - doing nothing" CR, parent);
    }
    else
        cb(miscCbData, opState);
    for (uint16_t i = 0; i > childList->size(); i++) {
        if (opState)
            ((systemState*)(childList->get(i)))->setOpState(OP_CBL);
        else
            ((systemState*)(childList->get(i)))->unSetOpState(OP_CBL);
    }
}

/*==============================================================================================================================================*/
/* END Class systemState                                                                                                                        */
/*==============================================================================================================================================*/