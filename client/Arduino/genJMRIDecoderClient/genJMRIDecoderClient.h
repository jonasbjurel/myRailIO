/*============================================================================================================================================= =*/
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



#ifndef GENJMRICLIENT_INO
#define GENJMRICLIENT_INO



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <dummy.h>
#include <ArduinoLog.h>
#include "globalCli.h"
#include "networking.h"
#include "systemState.h"
#include "decoder.h"
#include "fileSys.h"
class decoder;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO:                                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
decoder* decoderHandle = NULL;
WiFiManager* wifiManager = NULL;
bool setupRunning;
void setup(void);
void setupTask(void* p_dummy);
void loop(void);
/*==============================================================================================================================================*/
/* End ARDUINO:                                                                                                                                 */
/*==============================================================================================================================================*/

#endif //GENJMRICLIENT_INO
