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
    Log.info("systemState::systemState: Creating systemState object to parent object %d", p_parent);
    parent = p_parent;
    opState = 0;
    childList = new QList<void*>;
}

systemState::~systemState(void) {
    Log.info("systemState::~systemState: Deleting systemState object for object %d", parent);
    if (childList->size() > 0)
        Log.warning("systemState::~systemState: SystemState object for object %d still hav registered childs, the reference to those will be deleted", parent);
    childList->clear();
    delete childList;
}

void systemState::regSysStateCb(void* p_miscCbData, const sysStateCb_t p_cb) {
    Log.info("systemState::regSysStateCb: Registering systemState callback %d to parent object %d", p_cb, parent);
    cb = p_cb;
    miscCbData = p_miscCbData;
}

void systemState::addSysStateChild(void* p_child) {
    if (childList->indexOf(p_child) >= 0) {
        Log.warning("systemState::addChild: Child object %d already exists for object %d - doing nothing", p_child, parent);
        return;
    }
    else {
        Log.info("systemState::addChild: adding child object %d to parent object %d", p_child, parent);
        childList->push_back(p_child);
    }
}

void systemState::delSysStateChild(void* p_child) {
    int i;
    i = childList->indexOf(p_child);
    if (i < 0) {
        Log.warning("systemState::delChild: Child does not exist - doing nothing");
    }
    else {
        Log.info("systemState::delChild: deleting child object %d to parent object %d", p_child, parent);
        childList->clear(i);
    }
}

void systemState::setOpState(const uint16_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState | p_opStateMap;
    if (opState != prevOpState) {
        Log.info("systemState::opState has changed for object %d, previous opState bitmap: %b, current opState bitmap: %b", prevOpState, opState, parent);
        updateObjOpStates();
    }
}

void systemState::unSetOpState(const uint16_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState & ~p_opStateMap;
    if (opState != prevOpState) {
        Log.info("systemState::unSetOpState: opState has changed for object %d, previous opState bitmap: %b, current opState bitmap: %b", prevOpState, opState, parent);
        updateObjOpStates();
    }
}

uint16_t systemState::getOpState(void) {
    return opState;
}

void systemState::updateObjOpStates(void) {
    if (cb == NULL) {
        Log.info("systemState::updateObjOpStates: opState has changed for object %d, but no registered call-back to inform - doing nothing", parent);
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