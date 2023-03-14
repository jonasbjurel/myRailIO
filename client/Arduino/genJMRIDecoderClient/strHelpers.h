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

#ifndef STRHLP_H
#define STRHLP_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include<ctype.h>
#include <string.h>
#include <cstddef>
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Helper: strHelpers                                                                                                                           */
/* Purpose: Provides helper functions for string handling                                                                                       */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
char* createNcpystr(const char* src);
char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings);
bool isUri(const char* p_uri);
bool isIntNumberStr(const char* p_numberStr);
bool isFloatNumberStr(const char* p_numberStr);

/*==============================================================================================================================================*/
/* END strHelpers                                                                                                                               */
/*==============================================================================================================================================*/

#endif /*STRHLP_H*/
