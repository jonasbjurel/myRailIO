/*==============================================================================================================================================*/
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

#ifndef CLI_H
#define CLI_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: decoderCli                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define CLI_PARSE_CONTINUE  0
#define CLI_PARSE_STOP  1

typedef void (*cliInitCb_t)(void);
typedef uint8_t(*cliCb_t)(cmd* c);

struct cliContext_t {
    char* cliContext;
    cliInitCb_t cliContextInitCb;
    cliCb_t cliContextCb;
};

class decoderCli {
public:
    //methods
    static void init(void);
    static void registerCliContext(cliContext_t* p_context);
    static void printToCli(String p_output, bool partial = false);

    //Data structures
    static Command help;
    static Command set;
    static Command get;
    static Command exec;

private:
    //methods
    static void onTelnetConnect(String ip);
    static void onTelnetDisconnect(String ip);
    static void onTelnetReconnect(String ip);
    static void onTelnetConnectionAttempt(String ip);
    static void onTelnetString(String p_cmd);
    static void onCliCommand(cmd* p_cmd);
    static cliContext_t* getCliContext(char* p_context);
    static void setCliContext(cliContext_t* p_context);
    static void resetCliContext(void);
    static void commonCliContextInit(void);
    static uint8_t contextCommonCliCmd(cmd* p_cmd);
    static void rootCliContextInit(void);
    static uint8_t onRootCliCmd(cmd* p_cmd);
    static void cliPoll(void* dummy);

    //Data structures
    static QList<cliContext_t*>* cliContextList;
    static cliContext_t* currentCliContext;
    static cliContext_t* newCliContext;
    static SimpleCLI* cli;
    static ESPTelnet telnet;
    static cliContext_t* rootCliContext;
    static uint8_t connections;
};

/*==============================================================================================================================================*/
/* End class decoderCli                                                                                                                         */
/*==============================================================================================================================================*/

#endif //CLI_H
