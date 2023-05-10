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
uint16_t systemState::sysStateIndex = 0;
systemState::systemState(systemState* p_parent) {
    parent = p_parent;
    if (parent) {
        Log.TERSE("systemState::systemState: Creating systemState object %s:sysStateObjIndex-%d to parent object %s" CR, parent->getSysStateObjName(), sysStateIndex, parent->getSysStateObjName());
        objName = new char[strlen(parent->getSysStateObjName()) + 25];
        sprintf(objName, "%s:sysStateObjIndex-%d", parent->getSysStateObjName(), sysStateIndex);
    }
    else{
        Log.TERSE("systemState::systemState: Creating systemState top object sysStateObjIndex-%d" CR, sysStateIndex);
        objName = new char[25];
        sprintf(objName, "sysStateObjIndex-%d", sysStateIndex);
    }
    if (parent && parent->getOpState())
        opState = OP_CBL;
    else
        opState = OP_WORKING;
    cbList = new QList<cb_t*>;
    childList = new QList<systemState*>;
    sysStateIndex++;
}

systemState::~systemState(void) {
    if(parent)
        Log.INFO("systemState::~systemState: Deleting systemState object %s belonging to parent %s" CR, getSysStateObjName(), parent->getSysStateObjName());
    else
        Log.INFO("systemState::~systemState: Deleting top parent systemState object %s" CR, getSysStateObjName());
    if (childList->size() > 0)
        Log.WARN("systemState::~systemState: SystemState object %s still have registered childs, the reference to those will be deleted" CR, getSysStateObjName());
    if (objName)
        delete objName;

    childList->clear();
    delete childList;
}

rc_t systemState::regSysStateCb(void* p_miscCbData, sysStateCb_t p_cb) {
    Log.TERSE("systemState::regSysStateCb: Registering systemState callback %d for system state object %s" CR, p_cb, getSysStateObjName());
    for (uint8_t i = 0; i < cbList->size(); i++) {
        if (cbList->at(i)->cb == p_cb) {
            Log.warning("systemState::regSysStateCb: Callback function already exists, over-writing it" CR);
            cbList->at(i)->miscCbData = p_miscCbData;
            updateObjOpStates();
            return RC_ALREADYEXISTS_ERR;
        }
    }
    cb_t* cbObj = new cb_t;
    cbObj->cb = p_cb;
    cbObj->miscCbData = p_miscCbData;
    cbList->push_back(cbObj);
    updateObjOpStates();
    return RC_OK;
}

rc_t systemState::unRegSysStateCb(const sysStateCb_t p_cb) {
    Log.TERSE("systemState::unRegSysStateCb: Un - registering systemState callback %d for system state object %s" CR, p_cb, getSysStateObjName());
    for (uint8_t i = 0; i < cbList->size(); i++) {
        if (cbList->at(i)->cb == p_cb) {
            delete cbList->at(i);
            cbList->clear(i);
            return RC_OK;
        }
    }
    Log.WARN("systemState::unRegSysStateCb: Un - registering systemState callback %d for system state object %s failed, not found" CR, p_cb, getSysStateObjName());
    return RC_NOT_FOUND_ERR;
}

void systemState::addSysStateChild(systemState* p_child) {
    if (childList->indexOf(p_child) >= 0) {
        Log.WARN("systemState::addSysStateChild: Child object %s is already a member of object %s - doing nothing" CR, p_child->getSysStateObjName(), getSysStateObjName());
        return;
    }
    else {
        Log.TERSE("systemState::addSysStateChild: adding member child object %s to parent object %s" CR, p_child->getSysStateObjName(), getSysStateObjName());
        childList->push_back(p_child);
    }
}

void systemState::delSysStateChild(systemState* p_child) {
    int i;
    i = childList->indexOf(p_child);
    if (i < 0) {
        Log.WARN("systemState::delChild: Child %s is not a member of parent %s - doing nothing" CR, p_child->getSysStateObjName(), getSysStateObjName());
    }
    else {
        Log.TERSE("systemState::delChild: deleting child object %s to parent object %s" CR, p_child->getSysStateObjName(), getSysStateObjName());
        childList->clear(i);
    }
}

void systemState::setOpState(const sysState_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState | p_opStateMap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStr(currentOpStr, opState);
        char previousOpStr[100];
        getOpStateStr(previousOpStr, prevOpState);
        Log.INFO("systemState::setOpState: opState has changed for object %s, previous opState: %s, current opState: %s" CR, getSysStateObjName(), previousOpStr, currentOpStr);
        updateObjOpStates();
    }
}

void systemState::unSetOpState(const sysState_t p_opStateMap) {
    uint16_t prevOpState = opState;
    opState = opState & ~p_opStateMap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStr(currentOpStr, opState);
        char previousOpStr[100];
        getOpStateStr(previousOpStr, prevOpState);
        Log.INFO("systemState::unSetOpState: opState has changed for object %s, previous opState: %s, current opState: %s" CR, getSysStateObjName(), previousOpStr, currentOpStr);
        updateObjOpStates();
    }
}

uint16_t systemState::getOpState(void) {
    return opState;
}

char* systemState::getOpStateStr(char* p_opStateStr, sysState_t p_opBitmap) {
    strcpy(p_opStateStr, "");
    if (p_opBitmap & OP_INIT)
        strcat(p_opStateStr, "INIT|");
    if (p_opBitmap & OP_DISCONNECTED)
        strcat(p_opStateStr, "DISCONNECTED|");
    if (p_opBitmap & OP_NOIP)
        strcat(p_opStateStr, "NOIP|");
    if (p_opBitmap & OP_UNDISCOVERED)
        strcat(p_opStateStr, "UNDISCOVERED|");
    if (p_opBitmap & OP_UNCONFIGURED)
        strcat(p_opStateStr, "UNCONFIGURED|");
    if (p_opBitmap & OP_DISABLED)
        strcat(p_opStateStr, "DISABLED|");
    if (p_opBitmap & OP_UNAVAILABLE)
        strcat(p_opStateStr, "UNAVAILABLE|");
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

void systemState::setSysStateObjName(const char* p_objName) {
    if (parent){
        if (objName) {
            Log.TERSE("systemState::setObjName: Setting child object name: %s:%s, previous object name: %s" CR, parent->getSysStateObjName(), p_objName, getSysStateObjName());
            delete objName;
        }
        else
            Log.TERSE("systemState::setObjName: Setting child object name: %s:%s, previous object name: -" CR, parent->getSysStateObjName(), p_objName);
        objName = new char[strlen(parent->getSysStateObjName()) + strlen(p_objName) + 1];
        sprintf(objName, "%s:%s", parent->getSysStateObjName(), p_objName);
    }
    else {
        if (objName){
            Log.TERSE("systemState::setObjName: Setting top object name: %s, previous object name: %s" CR, p_objName, getSysStateObjName());
            delete objName;
        }
        else
            Log.TERSE("systemState::setObjName: Setting top object name: %s, previous object name: -" CR, p_objName);
        objName = new char[strlen(p_objName)];
        sprintf(objName, "%s", p_objName);
    }
    for (uint16_t i = 9; i < childList->size(); i++)
        childList->at(i)->updateObjName();
}

void systemState::updateObjName(void) {
    if (objName) {
        char instanceObjName[30];
        int16_t lastObjNameSeparator = -1;
        for (uint8_t i = 0; i < strlen(objName); i++) {
            if (*(objName + i) == ':')
                lastObjNameSeparator = i;
        }
        if (lastObjNameSeparator >= 0 && strlen(objName) - lastObjNameSeparator < 30)
            strcpy(instanceObjName, objName + lastObjNameSeparator + 1);
        else
            panic("systemState::updateObjName: Local System state object not found or not well-formatted - rebooting..." CR);
        setSysStateObjName(instanceObjName);
    }
    else
        panic("systemState::updateObjName: Local System state object not set - rebooting..." CR);
}

char* systemState::getSysStateObjName(char* p_objName) {
    if (objName)
        strcpy(p_objName, objName);
    else
        sprintf(p_objName, "Object %d", this);
    return p_objName;
}

const char* systemState::getSysStateObjName(void) {
    return objName;
}

void systemState::updateObjOpStates(void) {
    if (!cbList->size()) {
        Log.VERBOSE("systemState::updateObjOpStates: opState has changed for object %s, but no call-backs registered - doing nothing" CR, getSysStateObjName());
    }
    for (uint16_t i = 0; i < cbList->size(); i++){
        Log.VERBOSE("systemState::updateObjOpStates: Sending call-back to %i" CR, cbList->at(i)->cb);
        cbList->at(i)->cb(cbList->at(i)->miscCbData, opState);
    }
    char objName[100];
    char childObjName1[100];

    for (uint16_t i = 0; i < childList->size(); i++) {
        if (opState){
            Log.VERBOSE("systemState::updateObjOpStates: %s setting child %s to CBL" CR, getSysStateObjName(objName), childList->get(i)->getSysStateObjName(childObjName1));
            childList->get(i)->setOpState(OP_CBL);
        }
        else {
            Log.VERBOSE("systemState::updateObjOpStates: %s unsetting child %s from CBL" CR, getSysStateObjName(objName), childList->get(i)->getSysStateObjName(childObjName1));
            childList->get(i)->unSetOpState(OP_CBL);
        }
    }
}

/*==============================================================================================================================================*/
/* END Class systemState                                                                                                                        */
/*==============================================================================================================================================*/