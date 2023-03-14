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

#ifndef RC_H
#define RC_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <Arduino.h>
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/

// Return Codes
typedef int8_t rc_t;
#define RC_OK						0														// Do not change, 0 should always be success
#define RC_GEN_ERR					-127													// Do not change, 255 should always be unspecified error 
#define RC_OUT_OF_MEM_ERR			-1														// No more memory
#define RC_PARSE_ERR				-2														// Could not the parse eg XML string
#define RC_NOT_FOUND_ERR			-3														// Object not found
#define RC_TIMEOUT_ERR				-4														// Timeout
#define RC_DEBUG_NOT_SET_ERR		-5														// Not in debug mode
#define RC_NOT_CONFIGURED_ERR		-6														// Not configured
#define RC_DISCOVERY_ERR			-7														// Discovery failed
#define RC_ALREADYEXISTS_ERR		-8														// Object already exists
#define RC_ALREADYRUNNING_ERR		-9														// Object already running
#define RC_NOTRUNNING_ERR			-10														// Object not running
#define RC_PARAMETERVALUE_ERR		-11														// Parameter value error
#define RC_MAX_REACHED_ERR			-12														// Maximum number of objects reached
#define RC_NOTIMPLEMENTED_ERR		-126													// Called method not implemented
#endif /*RC_H*/
