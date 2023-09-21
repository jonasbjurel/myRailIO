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
EXT_RAM_ATTR uint16_t systemState::sysStateIndex = 0;
EXT_RAM_ATTR const char* systemState::OP_STR[14] = OP_ARR;
//WHY IS NOT THIS WORKING: EXT_RAM_ATTR job* systemState::jobHandler = new (heap_caps_malloc(sizeof(job), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) job(JOB_QUEUE_SIZE, CPU_SYSSTATE_JOB_TASKNAME, CPU_SYSSTATE_JOB_STACKSIZE_1K * 1024, CPU_SYSSTATE_JOB_PRIO, CPU_SYSSTATE_JOB_CORE);
EXT_RAM_ATTR job* systemState::jobHandler = new job(JOB_QUEUE_SIZE, CPU_SYSSTATE_JOB_TASKNAME, CPU_SYSSTATE_JOB_STACKSIZE_1K * 1024, CPU_SYSSTATE_JOB_PRIO, true);

systemState::systemState(systemState* p_parent) {
    parent = p_parent;
    if (parent) {
        LOG_TERSE("Creating systemState object %s:sysStateObjIndex-%d to parent object %s" CR, parent->getSysStateObjName(), sysStateIndex, parent->getSysStateObjName());
        objName = new (heap_caps_malloc(sizeof(char) * (strlen(parent->getSysStateObjName()) + 25), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(parent->getSysStateObjName()) + 25];
        sprintf(objName, "%s:sysStateObjIndex-%d", parent->getSysStateObjName(), sysStateIndex);
    }
    else {
        LOG_TERSE("Creating systemState top object sysStateObjIndex-%d" CR, sysStateIndex);
        objName = new (heap_caps_malloc(sizeof(char[25]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[25];
        sprintf(objName, "sysStateObjIndex-%d", sysStateIndex);
    }
    if (parent && parent->getOpStateBitmap())
        opState = OP_CBL;
    else
        opState = OP_WORKING;
    cbList = new QList<cb_t*>;
    childList = new QList<systemState*>;
    sysStateIndex++;
}

systemState::~systemState(void) {
    if (parent)
        LOG_INFO("Deleting systemState object %s belonging to parent %s" CR, getSysStateObjName(), parent->getSysStateObjName());
    else
        LOG_INFO("Deleting top parent systemState object %s" CR, getSysStateObjName());
    if (childList->size() > 0)
        LOG_WARN("SystemState object %s still have registered childs, the reference to those will be deleted" CR, getSysStateObjName());
    if (objName)
        delete objName;

    childList->clear();
    delete childList;
}

rc_t systemState::regSysStateCb(void* p_miscCbData, sysStateCb_t p_cb) {
    LOG_TERSE("Registering systemState callback %i for system state object %s with miscDataPointer %i" CR, p_cb, getSysStateObjName(), p_miscCbData);
    for (uint8_t i = 0; i < cbList->size(); i++) {
        if (cbList->at(i)->cb == p_cb) {
            LOG_WARN("Callback function already exists, over-writing it" CR);
            cbList->at(i)->miscCbData = p_miscCbData;
            updateObjOpStates();
            return RC_ALREADYEXISTS_ERR;
        }
    }
    cb_t* cbObj = new (heap_caps_malloc(sizeof(cb_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) cb_t;
    cbObj->cb = p_cb;
    cbObj->miscCbData = p_miscCbData;
    cbList->push_back(cbObj);
    updateObjOpStates();
    return RC_OK;
}

rc_t systemState::unRegSysStateCb(const sysStateCb_t p_cb) {
    LOG_TERSE("Un-registering systemState callback %d for system state object %s" CR, p_cb, getSysStateObjName());
    for (uint8_t i = 0; i < cbList->size(); i++) {
        if (cbList->at(i)->cb == p_cb) {
            delete cbList->at(i);
            cbList->clear(i);
            return RC_OK;
        }
    }
    LOG_WARN("Un-registering systemState callback %d for system state object %s failed, not found" CR, p_cb, getSysStateObjName());
    return RC_NOT_FOUND_ERR;
}

void systemState::addSysStateChild(systemState* p_child) {
    if (childList->indexOf(p_child) >= 0) {
        LOG_WARN("Child object %s is already a member of object %s - doing nothing" CR, p_child->getSysStateObjName(), getSysStateObjName());
        return;
    }
    else {
        LOG_TERSE("Adding member child object %s to parent object %s" CR, p_child->getSysStateObjName(), getSysStateObjName());
        childList->push_back(p_child);
    }
}

void systemState::delSysStateChild(systemState* p_child) {
    int i;
    i = childList->indexOf(p_child);
    if (i < 0) {
        LOG_WARN("Child %s is not a member of parent %s - doing nothing" CR, p_child->getSysStateObjName(), getSysStateObjName());
    }
    else {
        LOG_TERSE("Deleting child object %s to parent object %s" CR, p_child->getSysStateObjName(), getSysStateObjName());
        childList->clear(i);
    }
}

char* systemState::getOpStateStrByBitmap(sysState_t p_opStateBitmap, char* p_opStateStrBuff) {
    if (p_opStateBitmap == OP_WORKING) {
        strcpy(p_opStateStrBuff, OP_STR[0]);
        return p_opStateStrBuff;
    }
    else {
        strcpy(p_opStateStrBuff, "");
        sysState_t sysStateItter = p_opStateBitmap & OP_ALL;
        bool found = false;
        for (uint8_t i = 0; i < NO_OPSTATES - 1; i++) {
            if (sysStateItter & 0b0000000000001) {
                found = true;
                strcat(p_opStateStrBuff, OP_STR[i + 1]);
                sysStateItter >> 1;
                if (sysStateItter)
                    strcat(p_opStateStrBuff, "|");
                else
                    return p_opStateStrBuff;
            }
            sysStateItter = sysStateItter >> 1;
        }
        if (!found) {
            LOG_ERROR("opStateBitmap 0x%X is invalid\n", p_opStateBitmap);
            return NULL;
        }

    }
    p_opStateStrBuff[strlen(p_opStateStrBuff) - 1] = '\0';
    return p_opStateStrBuff;
}

sysState_t systemState::getOpStateBitmapByStr(const char* p_opStateStrBuff) {
    char opStr[20];
    char* opStateStrBuff;
    opStateStrBuff = new (heap_caps_malloc(sizeof(char) * (strlen(p_opStateStrBuff) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(p_opStateStrBuff) + 1];
    strcpy(opStateStrBuff, p_opStateStrBuff);
    sysState_t opStateBitmap = 0;
    trimSpace(opStateStrBuff);
    uint16_t j = 0;
    if (!strcmp(opStateStrBuff, OP_STR[0])){
        delete opStateStrBuff;
        return OP_WORKING;
    }
    else {
        for (uint16_t i = 0; i < strlen(opStateStrBuff) + 1; i++) {
            if ((opStateStrBuff[i] == '|') || (opStateStrBuff[i] == '\0')) {
                memcpy(opStr, opStateStrBuff + j, i - j);
                opStr[i - j] = '\0';
                trimSpace(opStr);
                if (opStateStrBuff[i] == '|')
                    j = i + 1;
                else
                    j = i;
                sysState_t opStateBitmapItter = 0b1;
                bool found = false;
                for (uint8_t k = 0; k < NO_OPSTATES - 1; k++) {
                    if (!strcmp(opStr, OP_STR[k + 1])) {
                        opStateBitmap = opStateBitmap | opStateBitmapItter;
                        found = true;
                        break;
                    }
                    opStateBitmapItter = opStateBitmapItter << 1;
                }
                if (!found) {
                    LOG_ERROR("opStateStr missformated, %s opState is not a valid one\n", opStr);
                    delete opStateStrBuff;
                    return -1;
                }
            }
        }
    }
    delete opStateStrBuff;
    return opStateBitmap;
}

rc_t systemState::setOpStateByBitmap(sysState_t p_opStateBitmap) {
    uint16_t prevOpState = opState;
    if (p_opStateBitmap > OP_ALL) {
        LOG_ERROR("opStateBitmap: 0x%X out of boundaries\n", p_opStateBitmap);
        return RC_OPSTATE_ERR;
    }
    opState = opState | p_opStateBitmap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStrByBitmap(opState, currentOpStr);
        char previousOpStr[100];
        getOpStateStrByBitmap(prevOpState, previousOpStr);
        LOG_INFO("opState has changed for object %s, previous opState: %s, current opState: %s" CR, getSysStateObjName(), previousOpStr, currentOpStr);
        updateObjOpStates();
    }
    return RC_OK;
}

rc_t systemState::setOpStateByStr(const char* p_opStateStrBuff) {
    return setOpStateByBitmap(getOpStateBitmapByStr(p_opStateStrBuff));
}

rc_t systemState::unSetOpStateByBitmap(sysState_t p_opStateBitmap) {
    uint16_t prevOpState = opState;
    if (p_opStateBitmap > OP_ALL) {
        LOG_ERROR("opStateBitmap: 0x%X out of boundaries\n", p_opStateBitmap);
        return RC_OPSTATE_ERR;
    }
     opState = opState & ~p_opStateBitmap;
    if (opState != prevOpState) {
        char currentOpStr[100];
        getOpStateStrByBitmap(opState, currentOpStr);
        char previousOpStr[100];
        getOpStateStrByBitmap(prevOpState, previousOpStr);
        LOG_INFO("opState has changed for object %s, previous opState: %s, current opState: %s" CR, getSysStateObjName(), previousOpStr, currentOpStr);
        updateObjOpStates();
    }
    return RC_OK;
}

rc_t systemState::unSetOpStateByStr(const char* p_opStateStrBuff) {
    return unSetOpStateByBitmap(getOpStateBitmapByStr(p_opStateStrBuff));
}

rc_t systemState::setAbsoluteOpStateByBitmap(sysState_t p_opStateBitmap) {
    if (p_opStateBitmap > OP_ALL) {
        LOG_ERROR("opStateBitmap: 0x%X out of boundaries\n", p_opStateBitmap);
        return RC_OPSTATE_ERR;
    }
    rc_t rc;
    if ((rc = setOpStateByBitmap(p_opStateBitmap)) || (rc = unSetOpStateByBitmap(~p_opStateBitmap)))
        return rc;
    return RC_OK;
}

rc_t systemState::setAbsoluteOpStateByStr(const char* p_opStateStrBuff) {
    return setAbsoluteOpStateByBitmap(getOpStateBitmapByStr(p_opStateStrBuff));
}

sysState_t systemState::getOpStateBitmap(void) {
    return opState;
}

char* systemState::getOpStateStr(char* p_opStateStrBuff) {
    return getOpStateStrByBitmap(opState, p_opStateStrBuff);
}


void systemState::setSysStateObjName(const char* p_objName) {
    if (parent){
        if (objName) {
            LOG_TERSE("Setting child object name: %s:%s, previous object name: %s" CR, parent->getSysStateObjName(), p_objName, getSysStateObjName());
            delete objName;
        }
        else
            LOG_TERSE("Setting child object name: %s:%s, previous object name: -" CR, parent->getSysStateObjName(), p_objName);
        objName = new (heap_caps_malloc(sizeof(char) * (strlen(parent->getSysStateObjName()) + strlen(p_objName) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(parent->getSysStateObjName()) + strlen(p_objName) + 1];
        sprintf(objName, "%s:%s", parent->getSysStateObjName(), p_objName);
    }
    else {
        if (objName){
            LOG_TERSE("Setting top object name: %s, previous object name: %s" CR, p_objName, getSysStateObjName());
            delete objName;
        }
        else
            LOG_TERSE("Setting top object name: %s, previous object name: -" CR, p_objName);
        objName = new (heap_caps_malloc(sizeof(char) * (strlen(p_objName) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(p_objName) + 1];
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
        else {
            panic("Local System state object not found or not well-formatted");
            return;
        }
        setSysStateObjName(instanceObjName);
    }
    else {
        panic("Local System state object not set");
        return;
    }
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
        LOG_VERBOSE("opState has changed for object %s, but no call-backs registered - doing nothing" CR, getSysStateObjName());
    }
    for (uint16_t i = 0; i < cbList->size(); i++){
        LOG_VERBOSE("Sending call-back to %i" CR, cbList->at(i)->cb);
        sysStateJobDesc_t* sysStateJobDesc = new (heap_caps_malloc(sizeof(sysStateJobDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) sysStateJobDesc_t;
        sysStateJobDesc->cb = cbList->at(i)->cb;
        sysStateJobDesc->miscCbData = cbList->at(i)->miscCbData;
        sysStateJobDesc->opState = opState;
        jobHandler->enqueue(sysCbHelper, (void*)sysStateJobDesc, 0);
    }
    char objName[100];
    char childObjName1[100];

    for (uint16_t i = 0; i < childList->size(); i++) {
        if (opState){
            LOG_VERBOSE("%s setting child %s to CBL" CR, getSysStateObjName(objName), childList->get(i)->getSysStateObjName(childObjName1));
            childList->get(i)->setOpStateByBitmap(OP_CBL);
        }
        else {
            LOG_VERBOSE("%s unsetting child %s from CBL" CR, getSysStateObjName(objName), childList->get(i)->getSysStateObjName(childObjName1));
            childList->get(i)->unSetOpStateByBitmap(OP_CBL);
        }
    }
}

void systemState::sysCbHelper(void* p_jobCbDesc) {

    //DET SMÄLLER HÄR
    ((sysStateJobDesc_t*)p_jobCbDesc)->cb(((sysStateJobDesc_t*)p_jobCbDesc)->miscCbData, ((sysStateJobDesc_t*)p_jobCbDesc)->opState);
    //if (!heap_caps_check_integrity_all(true))
    delete (sysStateJobDesc_t*)p_jobCbDesc;
}


/*==============================================================================================================================================*/
/* END Class systemState                                                                                                                        */
/*==============================================================================================================================================*/