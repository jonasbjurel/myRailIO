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



/*==============================================================================================================================================*/
/* .h Definitions                                                                                                                               */
/*==============================================================================================================================================*/
#ifndef GLOBALCLI_H
#define GLOBALCLI_H
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
#include <IPAddress.h>
//#include "decoder.h"
#include "logHelpers.h"
#include "cliCore.h"
#include "cliGlobalDefinitions.h"
#include "panic.h"
#include "rc.h"
#include "config.h"
#include "networking.h"
#include "mqtt.h"
#include "cpu.h"
#include "ntpTime.h"
#include "wdt.h"
#include "strHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: globalCli                                                                                                                             */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */

/* Methods                                                                                                                                      */
class globalCli : public cliCore {
public:
    //Public methods
    globalCli(const char* p_moType, const char* p_moName, uint16_t p_moIndex,           //globalCLI instance constructor, one for each MO instance
        globalCli* p_parent_context, bool p_root=false);                                //set root if top MO instance
    ~globalCli(void);                                                                   //globalCLI instance destructor

    void regGlobalNCommonCliMOCmds(void);                                               //Register global and common MOs and sub-MOs


    //Public data structures
    // -

private:
    //Private methods
    void regGlobalCliMOCmds(void);                                                      //Internal class procedure, register global MOs and sub-MOs
    void regCommonCliMOCmds(void);                                                      //Internal class procedure, register common MOs and sub-MOs
    virtual void regContextCliMOCmds(void);

    /* Global CLI decoration methods */
    static void onCliHelp(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);//Prints help-text
    static void onCliGetContextHelper(cmd* p_cmd, cliCore* p_cliContext,                //Get CLI-context helper
                                      cliCmdTable_t* p_cmdTable);
    rc_t onCliGetContext(char* p_fullContextPath);                                      //Get CLI-context
    static void onCliSetContextHelper(cmd* p_cmd, cliCore* p_cliContext,                //Set CLI-context helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliSetContext(cmd* p_cmd);                                                   //Set CLI-context
    static void onCliShowTopology(cmd* p_cmd, cliCore* p_cliContext,                    //Show CLI-context topology
                                  cliCmdTable_t* p_cmdTable);
    void printTopology(bool p_start = true);                                            //Print CLI topology
    static void onCliShowCommands(cmd* p_cmd, cliCore* p_cliContext,                    //Show CLI-context available commands
                           cliCmdTable_t* p_cmdTable);
    void processAvailCommands(bool p_all, bool p_help);                                 //Prints commands available from all or current context
    void printCommand(const char* p_cmdType, const char* p_mo, const char* p_subMo,
                      const char* p_flags, const char* p_helpStr,
                      bool p_heading = false, bool p_all = false,
                      bool p_showHelp = false);
    static void onCliReboot(cmd* p_cmd, cliCore* p_cliContext,                          //Reboot sub-MO command
                            cliCmdTable_t* p_cmdTable);
    static void onCliGetCoreDump(cmd* p_cmd, cliCore* p_cliContext,                     //Fetch Coredump sub-MO command
                                cliCmdTable_t* p_cmdTable);
    static void onCliGetUptime(cmd* p_cmd, cliCore* p_cliContext,                       //Get decoder uptime sub-MO command
                               cliCmdTable_t* p_cmdTable);
    static void onCliStartCpu(cmd* p_cmd, cliCore* p_cliContext,                        //Start CPU sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliStopCpu(cmd* p_cmd, cliCore* p_cliContext,                         //Stop CPU sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliGetCpu(cmd* p_cmd, cliCore* p_cliContext,                          //Get CPU sub-MO
                            cliCmdTable_t* p_cmdTable);
    static void onCliShowCpu(cmd* p_cmd, cliCore* p_cliContext,                         //Show CPU sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliGetMem(cmd* p_cmd, cliCore* p_cliContext,                          //Get Mem sub-MO
                            cliCmdTable_t* p_cmdTable);
    static void onCliShowMem(cmd* p_cmd, cliCore* p_cliContext,                         //Show Mem sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliStartMem(cmd* p_cmd, cliCore* p_cliContext,                        //Stop Mem sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliStopMem(cmd* p_cmd, cliCore* p_cliContext,                         //Start Mem sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliSetNetwork(cmd* p_cmd, cliCore* p_cliContext,                      //Set Network sub-MO
                                cliCmdTable_t* p_cmdTable);
    static void onCliGetNetwork(cmd* p_cmd, cliCore* p_cliContext,                      //Get Network sub-MO
                                cliCmdTable_t* p_cmdTable);
    static void onCliShowNetwork(cmd* p_cmd, cliCore* p_cliContext,                     //Show Network sub-MO
                                 cliCmdTable_t* p_cmdTable);
    static void showNetwork(void);                                                      //Show Network sub-MO
    static void onCliSetWdt(cmd* p_cmd, cliCore* p_cliContext,                          //Set WDT sub-MO
                            cliCmdTable_t* p_cmdTable);
    static void onCliUnsetWdt(cmd* p_cmd, cliCore* p_cliContext,                        //UnSet WDT sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliGetWdt(cmd* p_cmd, cliCore* p_cliContext,                          //Get WDT sub-MO
                            cliCmdTable_t* p_cmdTable);
    static void onCliShowWdt(cmd* p_cmd, cliCore* p_cliContext,                         //Show WDT sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void showWdt(uint16_t p_wdtId = 0);
    static void onCliClearWdt(cmd* p_cmd, cliCore* p_cliContext,                        //Clear WDT sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliStopWdt(cmd* p_cmd, cliCore* p_cliContext,                         //Stop WDT sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliStartWdt(cmd* p_cmd, cliCore* p_cliContext,                        //Start WDT sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliSetMqtt(cmd* p_cmd, cliCore* p_cliContext,                         //Set MQTT sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliClearMqtt(cmd* p_cmd, cliCore* p_cliContext,                       //Clear MQTT sub-MO
                               cliCmdTable_t* p_cmdTable);
    static void onCliGetMqtt(cmd* p_cmd, cliCore* p_cliContext,                         //Get MQTT sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliShowMqtt(cmd* p_cmd, cliCore* p_cliContext,                        //Show MQTT sub-MO, same as get mqtt
                              cliCmdTable_t* p_cmdTable);
    static void showMqtt(void);
    static void onCliAddTime(cmd* p_cmd, cliCore* p_cliContext,                         //Add time sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliDeleteTime(cmd* p_cmd, cliCore* p_cliContext,                      //Delete time sub-MO
                                cliCmdTable_t* p_cmdTable);
    static void onCliStartTime(cmd* p_cmd, cliCore* p_cliContext,                       //Start time sub-MO
                               cliCmdTable_t* p_cmdTable);
    static void onCliStopTime(cmd* p_cmd, cliCore* p_cliContext,                        //Stop time sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void onCliSetTime(cmd* p_cmd, cliCore* p_cliContext,                         //Set time sub-MO
                             cliCmdTable_t * p_cmdTable);
    static void onCliGetTime(cmd* p_cmd, cliCore* p_cliContext,                         //Get time sub-MO
                             cliCmdTable_t* p_cmdTable);
    static void onCliShowTime(cmd* p_cmd, cliCore* p_cliContext,                        //Show time sub-MO
                              cliCmdTable_t* p_cmdTable);
    static void showTime(void);                                                         //Show time sub-MO

    /* Common CLI decoration methods */
    static void onCliSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,                    //Set log level helper sub-MO
                                  cliCmdTable_t* p_cmdTable);
    virtual rc_t setLogLevel(const char* p_logLevel, bool p_force = false);
    static void onCliUnSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,                  //Un-set log level helper sub-MO
        cliCmdTable_t* p_cmdTable);
    static void onCliAddLogHelper(cmd* p_cmd, cliCore* p_cliContext,                    //Add log level helper sub-MO
        cliCmdTable_t* p_cmdTable);
    static void onCliDeleteLogHelper(cmd* p_cmd, cliCore* p_cliContext,                 //Delete log level helper sub-MO
        cliCmdTable_t* p_cmdTable);
    static void onCliGetLogHelper(cmd* p_cmd, cliCore* p_cliContext,                    //Get log level helper
                                  cliCmdTable_t* p_cmdTable);
    virtual const char* getLogLevel(void);
    virtual rc_t getRSyslogServer(char* p_uri, uint16_t* p_port, bool p_force = false);
    static void onCliClearLogHelper(cmd* p_cmd, cliCore* p_cliContext,
        cliCmdTable_t* p_cmdTable);
    static void onCliShowLogHelper(cmd* p_cmd, cliCore* p_cliContext,
                                   cliCmdTable_t* p_cmdTable);
    static void onCliShowLog(void);                                                     //Show extensive log provisioning information
    static void printLogInfo(void);                                                     //Prints log provisioning to CLI output
    static void printCustomLogItems(void);                                              //Prints provisioned custom log items to CLI output
    static void onCliSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,               //Set failsafe helper
                                       cliCmdTable_t* p_cmdTable);
    virtual rc_t setFailSafe(const bool p_failsafe, bool p_force = false);              //Set failsafe for context - not supported
    static void onCliUnSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,             //Un-set failsafe helper
                                       cliCmdTable_t* p_cmdTable);
    virtual rc_t unSetFailSafe(const bool p_failsafe, bool p_force = false);            //Un-set failsafe for context - not supported
    static void onCliGetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,               //Get failsafe property helper
                                       cliCmdTable_t* p_cmdTable);
    virtual bool getFailSafe(void);                                                     //Get failsafe for context - not supported
    static void onCliSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,                  //Set Debugflag helper
                                    cliCmdTable_t* p_cmdTable);
    static void onCliUnSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,                //unSet Debugflag helper
                                      cliCmdTable_t* p_cmdTable);
    virtual void setDebug(bool p_debug);                                                //Set Debugflag for context - not supported
    static void onCliGetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,                  //Get Debugflag helper
                                    cliCmdTable_t* p_cmdTable);
    virtual bool getDebug(void);                                                        //Get Debugflag for context - not supported
    static void onCliGetOpStateHelper(cmd* p_cmd, cliCore* p_cliContext,                //Get OP-state (Operational-state) helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliGetOpState(cmd* p_cmd);                                                   //Get context OP-state (Operational-state)
    virtual rc_t getOpStateStr(char* p_opStateStr);                                     //Get context OP-state string (Operational-state)
    static void onCliGetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext,                //Get System name helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliGetSysName(cmd* p_cmd);                                                   //Get System name for context parser
    virtual rc_t getSystemName(char* p_systemName, bool p_force = false);               //Get System name for context
    static void onCliSetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext,                //Set System name helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliSetSysName(cmd* p_cmd);                                                   //Set System name for context parser
    virtual rc_t setSystemName(const char* p_systemName);                               //Set System name for context
    static void onCliGetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext,                //Get User name helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliGetUsrName(cmd* p_cmd);                                                   //Get User name for context parser
    virtual rc_t getUsrName(char* p_usrName, bool p_force = false);                     //Get User name for context
    static void onCliSetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext,                //Set User name helper
                                      cliCmdTable_t* p_cmdTable);
    void onCliSetUsrName(cmd* p_cmd);                                                   //Set User name for context parser
    virtual rc_t setUsrName(const char* p_usrName, bool p_force = false);               //Set User name for context
    static void onCliGetDescHelper(cmd* p_cmd, cliCore* p_cliContext,                   //Get Description helper
                                   cliCmdTable_t* p_cmdTable);
    void onCliGetDesc(cmd* p_cmd);                                                      //Get Description for context parser
    virtual rc_t getDesc(char* p_description, bool p_force = false);                    //Get Description for context
    static void onCliSetDescHelper(cmd* p_cmd, cliCore* p_cliContext,                   //Set Description helper
                                   cliCmdTable_t* p_cmdTable);
    void onCliSetDesc(cmd* p_cmd);                                                      //Set Description for context parser
    virtual rc_t setDesc(const char* p_description, bool p_force = false);              //Set Description for context

    //Private data structures
    static char decoderSysName[50];
    static globalCli* rootHandle;                                                       //root context handle
    const char* moType;
    const char* moName;
    uint16_t moIndex;;
    globalCli* parentContext;
    static char* testBuff;                                                              //memory allocation test buffer
};
/*==============================================================================================================================================*/
/* END Class globalCli                                                                                                                          */
/*==============================================================================================================================================*/
#endif //GLOBALCLI_H
