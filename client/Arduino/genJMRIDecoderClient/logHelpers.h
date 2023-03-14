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



#ifndef LOGHELPERS_H
#define LOGHELPERS_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "rc.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



//Debug level
#define GJMRI_DEBUG_VERBOSE             6
#define GJMRI_DEBUG_TERSE               5
#define GJMRI_DEBUG_INFO                4
#define GJMRI_DEBUG_WARN                3
#define GJMRI_DEBUG_ERROR               2
#define GJMRI_DEBUG_PANIC               1
#define GJMRI_DEBUG_SILENT              0

#define VERBOSE							verbose
#define TERSE							trace
#define INFO							info
#define WARN							warning
#define ERROR							error
#define PANIC							fatal



/*==============================================================================================================================================*/
/* Functions: Log Helper functions                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
// NEED TO FIX, destroys uint8_t definition
uint8_t transformLogLevelXmlStr2Int(const char* p_loglevelXmlTxt);
const char* transformLogLevelInt2XmlStr(uint8_t p_loglevelInt);

#endif //LOGHELPERS_H
