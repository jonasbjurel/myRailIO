/*============================================================================================================================================= =*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonas.bjurel@hotmail.com)
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

#ifndef TELNETCORE_H
#define TELNETCORE_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "libraries/ESPTelnet/src/ESPTelnet.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "panic.h"
#include "rc.h"
#include "config.h"
class telnetCore;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: telneCore                                                                                                                             */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

/* Definitions                                                                                                                                  */
typedef void(telnetConnectCb_t)(const char* p_clientIp, bool p_connected, void* p_metaData);
typedef void(telnetInputCb_t)(char* p_cmd, void* p_metaData);

/* Methods                                                                                                                                      */
class telnetCore {
public:
    //Public methods
    static rc_t start(void);
    static void regTelnetConnectCb(telnetConnectCb_t p_telnetConnectCb, void* p_telnetConnectCbMetaData);
    static void onTelnetConnect(String p_clientIp);
    static void onTelnetDisconnect(String ip);
    static void onTelnetReconnect(String ip);
    static void onTelnetConnectionAttempt(String ip);
    static void regTelnetInputCb(telnetInputCb_t p_telnetInputCb, void* p_telnetInputCbMetaData);
    static void onTelnetInput(String p_cmd);
    static void print(const char* p_output);
    static void poll(void* dummy);

    //Public data structures


private:
    //Private methods

    //Private data structures
    static ESPTelnet telnet;
    static telnetConnectCb_t* telnetConnectCb;
    static void* telnetConnectCbMetaData;
    static telnetInputCb_t* telnetInputCb;
    static void* telnetInputCbMetaData;
    static uint8_t connections;
    static char ip[20];

};

/*==============================================================================================================================================*/
/* END Class telnetCore                                                                                                                         */
/*==============================================================================================================================================*/
#endif //TELNETCORE_H

