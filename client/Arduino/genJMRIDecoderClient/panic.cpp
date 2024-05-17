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
#include "panic.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/
EXT_RAM_ATTR char stackTraceBuff[4096];
EXT_RAM_ATTR char panicLogMsg[512];
EXT_RAM_ATTR bool ongoingPanic = false;
QList<panicCbReg_t*>* panicCbList = new QList<panicCbReg_t*>;


void panic(const char* p_panicFmt, ...) {
    if (ongoingPanic) {
        Serial.printf(">>> A secondary panic occured, will not be handled..." CR);
        LOG_FATAL_NOFMT("A secondary panic occured, will not be handled..." CR);
        return; 
    }
    ongoingPanic = true;
    esp_backtrace_buff(20, stackTraceBuff);
    va_list args;
    va_start(args, p_panicFmt);
    int len = vsnprintf(NULL, 0, p_panicFmt, args);
    // format message
    vsnprintf(panicLogMsg, len + 1, p_panicFmt, args);
    Serial.printf(">>> PANIC!: %s - rebooting..." CR, panicLogMsg);
    LOG_FATAL("PANIC!: %s - rebooting..." CR, panicLogMsg);
    va_end(args);
    Serial.printf(">>> %s" CR, stackTraceBuff);
    LOG_FATAL("%s" CR, stackTraceBuff);
    Serial.printf(">>> Saving call-back stack to local %s/panic" CR, FS_PATH);
    LOG_FATAL("Saving call-back stack to local %s/panic" CR, FS_PATH);
    fileSys::putFile(FS_PATH "/" "panic", stackTraceBuff, strlen(stackTraceBuff) + 1, NULL);
    Serial.printf(">>> Sending panic callbacks to those who have subscribed to it" CR);
    LOG_FATAL("Sending panic callbacks to those who have subscribed to it" CR);
    sendPanicCb();
    Serial.printf(">>> Waiting 5 seconds before restarting - enabling spool-out of syslog, fail-safe settings, etc" CR);
    LOG_FATAL("Waiting %i seconds before restarting - enabling spool-out of syslog, fail-safe settings, etc" CR, PANIC_REBOOT_DELAY_MS / 1000);
    TimerHandle_t rebootTimer;
    rebootTimer = xTimerCreate("panicRebootTimer",                  // Just a text name, not used by the kernel.
        (PANIC_REBOOT_DELAY_MS / portTICK_PERIOD_MS),               // The timer period in ticks.
        pdFALSE,                                                    // The timer will not auto-reload when expired.
        NULL,                                                       // param passing.
        reboot                                                      // Each timer calls the same callback when it expires.
    );
    if (rebootTimer == NULL) {
        Serial.printf(">>> panic!: Could not create panic reboot timer - rebooting immediatly..." CR);
		LOG_FATAL_NOFMT("panic!: Could not create panic reboot timer - rebooting immediatly..." CR);
		reboot(NULL);
    }
    if (xTimerStart(rebootTimer, 0) != pdPASS) {
        Serial.printf(">>> panic!: Could not start panic reboot timer - rebooting immediatly..." CR);
        LOG_FATAL_NOFMT("panic!: Could not start panic reboot timer - rebooting immediatly..." CR);
        reboot(NULL);
    }
}

rc_t regPanicCb(panicCb_t p_panicCb, void* pMetaData){
    for (uint8_t cbListIndex = 0; cbListIndex < panicCbList->size(); cbListIndex++) {
        if (panicCbList->at(cbListIndex)->panicCb == p_panicCb && panicCbList->at(cbListIndex)->metaData == pMetaData)
            return RC_ALREADYEXISTS_ERR;
    }
    panicCbReg_t* panicCbReg = new (heap_caps_malloc(sizeof(panicCbReg_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) panicCbReg_t;
    panicCbReg->panicCb = p_panicCb;
    panicCbReg->metaData = pMetaData;
    panicCbList->push_back(panicCbReg);
    return RC_OK;
}

rc_t unRegPanicCb(panicCb_t p_panicCb, void* pMetaData) {
    for (uint8_t cbListIndex = 0; cbListIndex < panicCbList->size(); cbListIndex++) {
        if (panicCbList->at(cbListIndex)->panicCb == p_panicCb && panicCbList->at(cbListIndex)->metaData == pMetaData){
            delete panicCbList->at(cbListIndex);
            panicCbList->clear(cbListIndex);
            return RC_OK; 
        }
    }
    return RC_NOT_FOUND_ERR;
}

void sendPanicCb(void) {
    for (uint8_t cbListIndex = 0; cbListIndex < panicCbList->size(); cbListIndex++) {
        panicCbList->at(cbListIndex)->panicCb(panicCbList->at(cbListIndex)->metaData);
    }
}

rc_t IRAM_ATTR esp_backtrace_buff(int depth, char* p_stackBuff){
    //Check arguments
    int buffPtr = 0;
    //Initialize stk_frame with first frame of stack
    esp_backtrace_frame_t stk_frame;
    esp_backtrace_get_start(&(stk_frame.pc), &(stk_frame.sp), &(stk_frame.next_pc));
    buffPtr += sprintf(p_stackBuff + buffPtr, "Backtrace:0x%08X:0x%08X ", esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp);
    //Check if first frame is valid
    bool corrupted = (esp_stack_ptr_is_sane(stk_frame.sp) &&
        esp_ptr_executable((void*)esp_cpu_process_stack_pc(stk_frame.pc))) ?
        false : true;
    uint32_t i = (depth <= 0) ? INT32_MAX : depth;
    while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted) {
        if (!esp_backtrace_get_next_frame(&stk_frame)) {    //Get previous stack frame
            corrupted = true;
        }
        buffPtr += sprintf(p_stackBuff + buffPtr, "0x%08X:0x%08X ", esp_cpu_process_stack_pc(stk_frame.pc), stk_frame.sp);
    }
    //Print backtrace termination marker
    rc_t ret = RC_OK;
    if (corrupted) {
        buffPtr += sprintf(p_stackBuff + buffPtr, " |<-CORRUPTED");
        ret = RC_GEN_ERR;
    }
    else if (stk_frame.next_pc != 0) {    //Backtrace continues
        buffPtr += sprintf(p_stackBuff + buffPtr, " |<-CONTINUES");
    }
    return ret;
}

void reboot(void* p_dummy){
    Serial.printf("Panic reboot..." CR);
    ESP.restart();
}
