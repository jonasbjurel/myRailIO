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

#ifndef TASKWRP_H
#define TASKWRP_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include<ctype.h>
#include <string.h>
#include <cstddef>
#include <ArduinoLog.h>
#include "rc.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Helper: taskWrappers                                                                                                                         */
/* Purpose: Provides helper functions for task handling																							*/
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
TaskHandle_t eTaskCreate(TaskFunction_t pvTaskCode, const char* const pcName, uint32_t ulStackDepth, void* pvParameters, UBaseType_t uxPriority, bool internal = true);
/*==============================================================================================================================================*/
/* END taskWrappers                                                                                                                               */
/*==============================================================================================================================================*/
#endif /*TASKWRP_H*/
