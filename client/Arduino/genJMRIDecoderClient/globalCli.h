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

#ifndef GLOBALCLI_H
#define GLOBALCLI_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "cliCore.h"
#include "cliGlobalDefinitions.h"
#include "panic.h"
#include "rc.h"
#include "config.h"
#include "networking.h"
#include "mqtt.h"
#include "cpu.h"

//class globalCli;
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
    globalCli(const char* p_moType, bool p_root=false);
    ~globalCli(void);

    //Public data structures

private:
    //Private methods
    void regCliMOCmds(void);
    void regGlobalCliMOCmds(void);
    void regCommonCliMOCmds(void);
    virtual void regContextCliMOCmds(void);
    /* Global CLI decoration methods */
    static void onCliHelp(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliReboot(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliGetContextHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliGetContext(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliSetContextHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliSetContext(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliGetUptime(cmd* p_cmd, cliCore* p_cliContext);

    static void onCliShowTasks(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliShowTask(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliShowMem(cmd* p_cmd, cliCore* p_cliContext);

    static void onCliGetWifi(cmd* p_cmd, cliCore* p_cliContext);
    static void onClishowTopology(cmd* p_cmd, cliCore* p_cliContext);
    rc_t printTopology(bool p_begining = true);
    static void onCliGetMqttBrokerURIHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual const char* getMqttBrokerURI(void);
    static void onCliSetMqttBrokerURIHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force = false);
    static void onCliGetMqttPortHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual uint16_t getMqttPort(void);
    static void onCliSetMqttPortHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setMqttPort(const uint16_t p_mqttPort, bool p_force = false);
    static void onCliGetKeepaliveHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual float getKeepAlivePeriod(void);
    static void onCliSetKeepaliveHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setKeepAlivePeriod(const float p_keepAlivePeriod, bool p_force = false);
    static void onCliGetPingHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual float getPingPeriod(void);
    static void onCliSetPingHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setPingPeriod(const float p_pingPeriod, bool p_force = false);
    static void onCliGetNtpserverHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual const char* getNtpServer(void);
    static void onCliSetNtpserverHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setNtpServer(const char* p_ntpServer, bool p_force = false);
    static void onCliGetNtpportHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual uint16_t getNtpPort(void);
    static void onCliSetNtpportHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setNtpPort(const uint16_t p_ntpPort, bool p_force = false);
    static void onCliGetTzHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual uint8_t getTz(void);
    static void onCliSetTzHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setTz(const uint8_t p_tz, bool p_force = false);
    static void onCliGetLoglevelHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual const char* getLogLevel(void);
    static void onCliSetLoglevelHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setLogLevel(const char* p_logLevel, bool p_force = false);
    static void onCliGetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual bool getFailSafe(void);
    static void onCliSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliUnsetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setFailSafe(const bool p_failsafe, bool p_force = false);

    /* Common CLI decoration methods */
    static void onCliGetOpStateHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliGetOpState(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t getOpStateStr(char* p_opStateStr);
    static void onCliGetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliGetSysName(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t getSystemName(const char* p_systemName);
    static void onCliSetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliSetSysName(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setSystemName(const char* p_systemName);
    static void onCliGetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliGetUsrName(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t getUsrName(const char* p_usrName);
    static void onCliSetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliSetUsrName(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setUsrName(const char* p_usrName);
    static void onCliGetDescHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliGetDesc(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t getDesc(const char* p_description);
    static void onCliSetDescHelper(cmd* p_cmd, cliCore* p_cliContext);
    void onCliSetDesc(cmd* p_cmd, cliCore* p_cliContext);
    virtual rc_t setDesc(const char* p_description);
    static void onCliGetDebugHelper(cmd* p_cmd, cliCore* p_cliContext);
    bool getDebug(void);
    static void onCliSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext);
    static void onCliUnsetDebugHelper(cmd* p_cmd, cliCore* p_cliContext);
    virtual void setDebug(bool p_debug);

    //Private data structures
    static globalCli* rootHandle;
};
/*==============================================================================================================================================*/
/* END Class globalCli                                                                                                                          */
/*==============================================================================================================================================*/
#endif //GLOBALCLI_H
