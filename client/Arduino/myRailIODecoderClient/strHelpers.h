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
char* createNcpystr(const char* src, bool internal = false);
//char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings);
bool isUri(const char* p_uri);
bool isIpPort(const char* p_port);
bool isIpPort(int32_t p_port);
bool isIpAddress(const char* p_ipAddr);
bool isHostName(const char* p_hostName);
bool isIntNumberStr(const char* p_numberStr);
bool isFloatNumberStr(const char* p_numberStr);
char* trimSpace(char* p_s);
const char* trimNlCr(char* p_str);
const char* strTruncMaxLen(char* p_src, uint p_maxStrLen);
const char* strcpyTruncMaxLen(char* p_dest, const char* p_src, uint p_maxStrLen);
const char* strcatTruncMaxLen(char* p_src, const char* p_cat, uint p_maxStrLen);
const char* fileBaseName(const char* p_filePath);

/*==============================================================================================================================================*/
/* END strHelpers                                                                                                                               */
/*==============================================================================================================================================*/

#endif /*STRHLP_H*/
