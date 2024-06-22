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



/*==============================================================================================================================================*/
/* .h Definitions                                                                                                                               */
/*==============================================================================================================================================*/
#ifndef TELNETCORE_H
#define TELNETCORE_H
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <Arduino.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <ESPTelnet.h>
#include "wdt.h"
#include "panic.h"
#include "rc.h"
#include "config.h"
#include "taskWrapper.h"
#include "logHelpers.h"

class telnetCore;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: telneCore                                                                                                                             */
/* Purpose: Provides a Telnet Server                                                                                                            */
/* Description:                                                                                                                                 */
/* Methods:                                                                                                                                     */
/* Data Structures*/
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
typedef void(telnetConnectCb_t)(const char* p_clientIp, bool p_connected,           //Call-back prototype for client connect
                                void* p_metaData);  
typedef void(telnetInputCb_t)(char* p_cmd, void* p_metaData);                       //Call-back prototype for client input

/* Methods                                                                                                                                      */
class telnetCore {
public:
    //Public methods
    static rc_t start(void);                                                            //Starts the Telnet server service
    static void reconnect(void);                                                        //Reconnects/restarts the telnet server
    static void regTelnetConnectCb(telnetConnectCb_t p_telnetConnectCb,                 //Register callback for client connect event, only one cb allowed
                                   void* p_telnetConnectCbMetaData);                    //   p_telnetConnectCbMetaData provides a method to pass arbitrary data to the cb function
    static void onTelnetConnect(String p_clientIp);                                     //Telnet client connected call-back - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL
    static void onTelnetDisconnect(String ip);                                          //Telnet client disconnected call-back - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL
    static void onTelnetReconnect(String ip);                                           //Telnet client re-connected call-back - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL
    static void onTelnetConnectionAttempt(String ip);                                   //Telnet client failed connection attempt call-back - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL
    static void regTelnetInputCb(telnetInputCb_t p_telnetInputCb,                       //Register Telnet client input call-back, p_telnetConnectCbMetaData provides a method to
                                 void* p_telnetInputCbMetaData);                        //   pass arbitrary data to the cb function, only one call-back function allowed
    static void onTelnetInput(String p_cmd);                                            //Telnet client input received call-back - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL
    static void print(const char* p_output);                                            //Prints a string to the Telnet client
    static void poll(void* dummy);                                                      //Polls the Telnet server for input - CAN WE MOVE THIS TO PRIVATE AS IT IS INTERNAL

    //Public data structures


private:
    //Private methods

    //Private data structures
    static ESPTelnet telnet;                                                            //ESP Telnet server instance
    static telnetConnectCb_t* telnetConnectCb;                                          //Telnet client connect Call-back function reference
    static void* telnetConnectCbMetaData;                                               //Telnet client connect Call-back metadata reference, as provided with the registering process
    static telnetInputCb_t* telnetInputCb;                                              //Telnet client input Call-back function reference
    static void* telnetInputCbMetaData;                                                 //Telnet client input Call-back metadata reference, as provided with the registering process
    static uint8_t connections;                                                         //Number of clients connected - NOTE THAT ONLY ONE IS SUPPORTED
    static char ip[20];                                                                 //Telnet client IP-address
    static wdt* telnetWdt;                                                              //Telnet watchdog timer handle
};
/*==============================================================================================================================================*/
/* END Class telnetCore                                                                                                                         */
/*==============================================================================================================================================*/
#endif //TELNETCORE_H
