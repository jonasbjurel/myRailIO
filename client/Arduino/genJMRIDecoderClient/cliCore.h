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
#ifndef CLICORE_H
#define CLICORE_H
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <ESPTelnet.h>
#include <QList.h>
#include "libraries/SimpleCLI/src/SimpleCLI.h"
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
class cmdFlags;
class cmdFlag;
struct cliCmdTable_t;

typedef void (cliCmdCb_t)(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);//Command call-back function prototype

enum cliMainCmd_t {                                                                     //Command operator type enum
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

enum cmdErr_t {                                                                         //CLI command error enum
    CLI_PARSE_ERR,
    CLI_NOT_VALID_ARG_ERR,
    CLI_NOT_VALID_CMD_ERR,
    CLI_GEN_ERR,
};

enum successCmdTerm_t {                                                                 //CLI command success termination type enum
    CLI_TERM_QUIET,
    CLI_TERM_EXECUTED,
    CLI_TERM_ORDERED,
};

struct contextMap_t {                                                                   //CLI context struct
    cliCore* contextHandle;
    cliCmdCb_t* cb;
};

struct cliCmdTable_t {                                                                  //CLI command table struct
    cliMainCmd_t cmdType;
    char mo[20];
    char subMo[20];
    cmdFlags* commandFlags;
    char* help;
    QList< contextMap_t*>* contextMap;
};

struct cli_context_descriptor_t {                                                       //CLI context descriptor
    bool active;
    char* moType;
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
    cliCore(const char* p_moType, const char* p_moName, uint16_t p_moIndex,             //Constructor for a new CLI context instance
            bool p_root=false);
    ~cliCore(void);                                                                     //Destructor
    void regParentContext(const cliCore* p_parentContext);                              //Register parent CLI context
    void unRegParentContext(const cliCore* p_parentContext);                            //UnRegister parent CLI context
    void regChildContext(const cliCore* p_childContext, const char* contextName,        //Register a child CLI context
                         uint16_t p_contextIndex);
    void unRegChildContext(const cliCore* p_childContext);                              //Unregister a child CLI context
    QList<cliCore*>* getChildContexts(cliCore* p_cliContext);                           //Get all child contexts
    void setContextName(const char* p_contextName);                                     //Set CLI context name
    const char* getContextName(void);                                                   //Get CLI context name
    void setContextType(const char* p_contextType);                                     //Set CLI context type
    const char* getContextType(void);                                                   //Get CLI context type
    void setContextIndex(uint16_t p_contextIndex);                                      //Set CLI context type instance index
    uint16_t getContextIndex(void);                                                     //Get CLI context type instance index
    void setContextSysName(const char* p_contextSysName);                               //Set CLI context system name
    const char* getContextSysName(void);                                                //Get CLI context system name
    void setRoot(void);                                                                 //Assign this context as root CLI context
    void start(void);                                                                   //Start the CLI service
    static void onCliConnect(const char* p_clientIp, bool p_connected,                  //Telnet/CLI client connected - CAN WE MAKE THIS PRIVATE?
                             void* p_metaData);
    static void onRootIngressCmd(char* p_contextCmd, void* p_metaData);                 //A new CLI command with unknown/root context belonging has been
                                                                                        //   received - CAN WE MAKE THIS PRIVATE?
    rc_t onContextIngressCmd(char* p_contextCmd, bool p_setContext = false);            //A new CLI command to this CLI context (may need further context
                                                                                        //   routing) has been received - CAN WE MAKE THIS PRIVATE?
    static bool parseContextPath(char* p_cmd, char* p_nextHopContextPathRoute);         //Parse the CLI context path from the CLI command - CAN WE MAKE THIS PRIVATE?
    rc_t contextRoute(char* p_nextHop, char* p_contextCmd, bool p_setContext = false);  //Route the CLI command to next hop CLI context - CAN WE MAKE THIS PRIVATE?
    rc_t getFullCliContextPath(char* p_fullCliContextPath,                              //Get the full CLI context path
                               const cliCore* p_cliContextHandle = NULL);
    static cliCore* getCliContextHandleByPath(const char* p_path);                      //Get the CLI context handle from CLI context path
    static void printCli(const char* fmt, ...);                                         //Print a string with sprintf formatting to CLI client output
    void printCliNoFormat(char* p_msg);                                                 //Print a string without formatting to CLI client output
    rc_t regCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo,                      //Register a CLI command
                     const char* p_cmdSubMoArg, cliCmdCb_t* p_cliCmdCb);
    rc_t unRegCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo,                    //Un-register a CLI command
                       const char* p_cmdSubMoArg);
    rc_t regCmdFlagArg(cliMainCmd_t p_commandType, const char* p_mo,                    //Register command flags
                       const char* p_cmdSubMoArg, const char* p_flag,
                       uint8_t p_firstArgPos, bool p_hasValue);
    rc_t unRegCmdFlagArg(cliMainCmd_t p_commandType, const char* p_mo,                  //Un-register command flags
                         const char* p_cmdSubMoArg, const char* p_flag);
    rc_t regCmdHelp(cliMainCmd_t p_commandType, const char* p_mo,                       //Register command help text
                   const char* p_cmdSubMoArg, const char* p_helpText);
    rc_t getHelp(cmd* p_cmd);                                                           //Print help to CLI client
    static void notAcceptedCliCommand(cmdErr_t p_cmdErr, const char* errStr, ...);      //Method to call when a CLI command is not accepted
    static void acceptedCliCommand(successCmdTerm_t p_cmdTermType);                     //Method to call when a CLI command has been accepted
    static void onCliError(cmd_error* e);                                               //Call-back for simpleCLI command parse error - CAN WE MAKE THIS PRIVATE?
    static void onCliCmd(cmd* p_cmd);                                                   //Call-back for simpleCLI received command - CAN WE MAKE THIS PRIVATE?
    Command getCliCmdHandleByType(cliMainCmd_t p_commandType);                          //Get simpleCLI command handle from cliMainCmd_t
                                                                                        //   (eg.SHOW_CLI_CMD|GET_CLI_CMD|SET_CLI_CMD|UNSET_CLI_CMD|...)
    static cliMainCmd_t getCliTypeByName(const char* p_commandName);                    //Get simpleCLI command handle from CLI command type str
                                                                                        //   (eg."show"|"get"|"set"|"unset"|...)
    static const char* getCliNameByType(cliMainCmd_t p_commandType);                    //Get CLI command type str from cliMainCmd_t
    static cliCore* getCurrentContext(void);                                            //Get current CLI context handle
    static const QList<cliCore*>* getAllContexts(void);                                 //Get all CLI context handles
    static void setCurrentContext(cliCore* p_currentContext);                           //Set current CLI context
    cli_context_descriptor_t* getCliContextDescriptor(void);                            //Get CLI context descriptor
    //Public data structures

private:
    //Private methods

    //Private data structures
    static SemaphoreHandle_t cliCoreLock;                                               //CLI core lock handle
    static telnetCore telnetServer;                                                     //CLI Telnet server handle
    static SimpleCLI cliContextObjHandle;                                               //SimpleCLI handle
    cli_context_descriptor_t cliContextDescriptor;                                      //CLI context descriptor
    static cliCore* rootCliContext;                                                     //CLI root context handle
    static cliCore* currentContext;                                                     //Current CLI context handle
    static cliCore* currentParsingContext;                                              //Temporary CLI context for the CLI context path parsing process
    static QList<cliCmdTable_t*>* cliCmdTable;                                          //Global CLI command table
    static char* clientIp;                                                              //Telnet/CLI client IP-address
    static QList<cliCore*> allContexts;                                                 //List of all CLI contexts
    Command helpCliCmd;                                                                 //CLI main type/operator commands, CAN WE MOVE THESE DEFINITIONS TO cliGlobalDefinitions.h
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
/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cmdFlags                                                                                                                              */
/* Purpose: Provides means to define-, parse and otherhandle command flags                                                                      */
/* Description: Adds command flags to particular CLI command types- and command sub-MOs (cliCmdTable_t), keeps a list of cmdFlags objects       */
/*              available a particular CLI command type and sub MO. Also defines the offset into the command string the flag parsing should     */
/*              begin, and if the flag is to be followed by a value or not                                                                      */
/* Methods: See below                                                                                                                           */
/* Data structures: See below                                                                                                                   */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
// -

/* Methods                                                                                                                                      */
class cmdFlags {
public:
    //Public methods
    cmdFlags(uint8_t firstValidPos = 1);                                                //flags constructor - creates a set of flag object that 
                                                                                        //   defines the set of flags relevant for a particular CLI
                                                                                        //   command type and sub-MO defined by parent class:
                                                                                        //   cliCore::cliCmdTable.commandFlags
    ~cmdFlags(void);                                                                    //flags destructor
    rc_t add(const char* p_flag, bool p_needsValue);                                    //Add a flag
    rc_t remove(const char* p_flag);                                                    //Remove a flag
    uint8_t size(void);                                                                 //Get number of flags
    cmdFlag* get(const char* p_flag);                                                   //Get hadle to flag from flag name
    rc_t parse(Command p_command);                                                      //Parse a simpleCLI command for flags
    cmdFlag* isPresent(const char* p_flag);                                             //Check if a particular flag from name is present in a parsed
                                                                                        //   simpleCLI command and get the corresponding flag handle
                                                                                        //   from flag name (p_flag)
    QList<cmdFlag*>* getAllPresent(void);                                               //Get all all flags in a parsed simpleCLI command
    const char* getParsErrs(void);                                                      //Get flag parse errors from previous parse() method
    static const char* isFlag(const char* p_arg);                                       //Check if a command fragment is a flag - CAN WE MOVE THIS TO PRIVATE
    static const char* check(const char* p_arg);                                        //Check command flags - CAN WE MOVE THIS TO PRIVATE

    //Public data structures
    //-

private:
    //Private methods
    //-

    //Private data structures
    uint8_t firstValidPos;                                                              //First valid command flag postion
    QList<cmdFlag*>* flagList;                                                          //List of flags rellevant for CLI cmd type and sub-MO
    QList<cmdFlag*>* foundFlagsList;                                                    //List of found rellevant CLI flags from parsing
    char parseErrStr[200];                                                              //flag parse error sting
};
/*==============================================================================================================================================*/
/* END Class cmdFlags                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cmdFlag                                                                                                                               */
/* Purpose: Defines indidividual CLI command flags                                                                                              */
/* Description: Defines individual CLI flags, such as the name of the flag and it's properties, the flag's assosiation to a CLI command type    */
/*              and sub-MOs is not defined by the cmdFlag class, but byt the collection of command flags class: cmdFlags                         /
/* Methods: See below                                                                                                                           */
/* Data structures: See below                                                                                                                   */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
// -

/* Methods                                                                                                                                      */
class cmdFlag {
public:
    //Public methods
    cmdFlag(const char* p_flag, bool p_needsValue);                                     //flag constructur
    ~cmdFlag(void);                                                                     //flag destructor
    const char* getName(void);                                                          //Get flag name
    bool needsValue(void);                                                              //Get "need value" flag property
    void setValue(const char* p_value);                                                 //Set flag value
    const char* getValue(void);                                                         //Get flag value

    //Public data structures
    //-

private:
    //Private methods
    //-

    //Private data structures
    char name[20];                                                                      //flag name
    char value[20];                                                                     //flag value
    bool needsValueVar;                                                                 //flag "need value" property
};
/*==============================================================================================================================================*/
/* END Class cmdFlag                                                                                                                            */
/*==============================================================================================================================================*/
#endif //CLICORE_H
