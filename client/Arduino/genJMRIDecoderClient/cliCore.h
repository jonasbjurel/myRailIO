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

#ifndef CLICORE_H
#define CLICORE_H

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
#include "libraries/SimpleCLI/src/SimpleCLI.h"
#include "libraries/QList/src/QList.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "telnetCore.h"
#include "panic.h"
#include "version.h"
#include "cliGlobalDefinitions.h"
#include "strHelpers.h"
class telnetCore;
class cliCore;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cliCore                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/

/* Definitions                                                                                                                                  */
typedef void (cliCmdCb_t)(cmd* p_cmd, cliCore* p_cliContext);

enum cliMainCmd_t {
    HELP_CLI_CMD,
    REBOOT_CLI_CMD,
    SHOW_CLI_CMD,
    GET_CLI_CMD,
    SET_CLI_CMD,
    UNSET_CLI_CMD,
    CLEAR_CLI_CMD,
    ADD_CLI_CMD,
    DELETE_CLI_CMD,
    MOVE_CLI_CMD,
    START_CLI_CMD,
    STOP_CLI_CMD,
    RESTART_CLI_CMD,
    ILLEGAL_CLI_CMD
};

enum cmdErr_t {
    CLI_PARSE_ERR,
    CLI_NOT_VALID_ARG_ERR,
    CLI_NOT_VALID_CMD_ERR,
    CLI_GEN_ERR,
};

enum successCmdTerm_t {
    CLI_TERM_QUIET,
    CLI_TERM_EXECUTED,
    CLI_TERM_ORDERED,
};

struct contextMap_t {
    Command commandHandle;
    cliCore* contextHandle;
};

struct cliCmdTable_t {
    cliMainCmd_t cmdType;
    char mo[20];
    char subMo[20];
    const char* help;
    cliCmdCb_t* cb;
    QList< contextMap_t*>* contextMap;
};

struct cli_context_descriptor_t {
    bool active;
    char moType[20];
    char* contextName;
    uint16_t contextIndex;
    char* contextSysName;
    cliCore* parentContext;
    QList<cliCore*>* childContexts;
};

/* Methods                                                                                                                                      */
class cliCore{
public:
    //Public methods
    cliCore(const char* p_moType);
    ~cliCore(void);
    void regParentContext(const cliCore* p_parentContext);
    void unRegParentContext(const cliCore* p_parentContext);
    void regChildContext(const cliCore* p_childContext, const char* contextName, uint16_t p_contextIndex);
    void unRegChildContext(const cliCore* p_childContext);
    QList<cliCore*>* getChildContexts(cliCore* p_cliContext);
    void setContextName(const char* p_contextName);
    const char* getContextName(void);
    void setContextIndex(uint16_t p_contextIndex);
    uint16_t getContextIndex(void);
    void setContextSysName(const char* p_contextSysName);
    const char* getContextSysName(void);
    void start(void);
    static void onCliConnect(const char* p_clientIp, bool p_connected, void* p_metaData);
    static void onRootIngressCmd(char* p_contextCmd, void* p_metaData);
    rc_t onContextIngressCmd(char* p_contextCmd, bool p_setContext = false);
    static bool parseContextPath(char* p_cmd, char* p_nextHopContextPathRoute);
    rc_t contextRoute(char* p_nextHop, char* p_contextCmd, bool p_setContext = false);
    rc_t getFullCliContextPath(char* p_fullCliContextPath, const cliCore* p_cliContextHandle = NULL);
    static cliCore* getCliContextHandleByPath(const char* p_path);
    static void printCli(const char* fmt, ...);
    rc_t regCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg, cliCore* p_cliContext, cliCmdCb_t* p_cliCmdCb);
    rc_t unRegCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg);
    rc_t regCmdHelp(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg, const char* p_helpText);
    rc_t getHelp(const char* p_cmd = NULL, const char* p_cmdSubMoArg = NULL);
    static void notAcceptedCliCommand(cmdErr_t p_cmdErr, const char* errStr, ...);
    static void acceptedCliCommand(successCmdTerm_t p_cmdTermType);
    static void onCliError(cmd_error* e);
    static void onCliCmd(cmd* p_cmd);
    Command getCliCmdHandleByType(cliMainCmd_t p_commandType);
    static cliMainCmd_t getCliTypeByName(const char* p_commandName);
    static const char* getCliNameByType(cliMainCmd_t p_commandType);
    static cliCore* getCurrentContext(void);
    static void setCurrentContext(cliCore* p_currentContext);
    cli_context_descriptor_t* getCliContextDescriptor(void);

    //Public data structures

private:
    //Private methods

    //Private data structures
    static SemaphoreHandle_t cliCoreLock;
    static telnetCore telnetServer;
    static SimpleCLI cliContextObjHandle;
    static char* clientIp;
    static cliCore* rootCliContext;
    cli_context_descriptor_t cliContextDescriptor;
    static cliCore* currentContext;
    static QList<cliCmdTable_t*>* cliCmdTable;
    Command helpCliCmd;
    Command rebootCliCmd;
    Command showCliCmd;
    Command getCliCmd;
    Command setCliCmd;
    Command unsetCliCmd;
    Command clearCliCmd;
    Command addCliCmd;
    Command deleteCliCmd;
    Command copyCliCmd;
    Command pasteCliCmd;
    Command moveCliCmd;
    Command startCliCmd;
    Command stopCliCmd;
    Command restartCliCmd;
};

void getHeapMemTrendAll(char* p_heapMemTxt, char* p_heapHeadingTxt);

/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/
#endif //CLICORE_H
