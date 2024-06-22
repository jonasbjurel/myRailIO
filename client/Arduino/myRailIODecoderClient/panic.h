/*============================================================================================================================================= */
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

#ifndef PANIC_H
#define PANIC_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <esp_debug_helpers.h>
#include <QList.h>
#include "rc.h"
#include "logHelpers.h"
#include "mbedtls/base64.h"
#include "fileSys.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/

#define PANIC_REBOOT_DELAY_MS							5000
/*==============================================================================================================================================*/
/* Functions: panic																																*/
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void(*panicCb_t)(void* p_metaData);
struct panicCbReg_t {
	panicCb_t panicCb;
	void* metaData;
};

void panic(const char* fmt, ...);
rc_t regPanicCb(panicCb_t p_panicCb, void* pMetaData);
rc_t unRegPanicCb(panicCb_t p_panicCb, void* pMetaData);
void sendPanicCb(void);
rc_t esp_backtrace_buff(int depth, char* p_stackBuff);
void reboot(void* p_dummy);

#endif /*PANIC_H*/
