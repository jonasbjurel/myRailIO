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

#ifndef LGLINK_H
#define LGLINK_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "systemState.h"
#include "wdt.h"
#include "globalCli.h"
#include "cliGlobalDefinitions.h"
#include "decoder.h"
#include "lgBase.h"
#include "signalMastAspects.h"
#include "flash.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "pinout.h"
#include "libraries/Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include "esp32SysConfig.h"

class flash;
class signalMastAspects;
class lgBase;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lgLink                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define UPDATE_STRIP_LATENCY_AVG_TIME           10

#define XML_LGLINK_SYSNAME                      0
#define XML_LGLINK_USRNAME                      1
#define XML_LGLINK_DESC                         2
#define XML_LGLINK_LINK                         3

struct stripLed_t {
    uint8_t currentValue = 0;
    uint8_t wantedValue = SM_BRIGHNESS_FAIL;
    uint8_t incrementValue = 0;
    bool dirty = false;
};



class lgLink : public systemState, public globalCli {
public:
    //Public methods
    lgLink(uint8_t p_linkNo);
    ~lgLink(void);
    rc_t init(void);
    void onConfig(const tinyxml2::XMLElement* p_lightgroupsXmlElement);
    rc_t start(void);
    static void onSysStateChangeHelper(const void* p_miscData, uint16_t p_sysState);
    void onSysStateChange(uint16_t p_sysState);
    static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject);
    void onOpStateChange(const char* p_topic, const char* p_payload);
    static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject);
    void onAdmStateChange(const char* p_topic, const char* p_payload);
    rc_t getOpStateStr(char* p_opStateStr);
    rc_t setSystemName(char* p_systemName, bool p_force = false);
    rc_t getSystemName(const char* p_systemName);
    rc_t setUsrName(char* p_usrName, bool p_force = false);
    rc_t getUsrName(const char* p_userName);
    rc_t setDesc(char* p_description, bool p_force = false);
    rc_t getDesc(const char* p_desc);
    rc_t setLink(uint8_t p_link);
    rc_t getLink(uint8_t* p_link);
    void setDebug(bool p_debug);
    bool getDebug(void);
    flash* getFlashObj(uint8_t p_flashType);
    signalMastAspects* getSignalMastAspectObj(void);
    rc_t updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, uint8_t* p_wantedValueBuff, uint16_t* p_transitionTimeBuff);
    uint32_t getOverRuns(void);
    void clearOverRuns(void);
    int64_t getMeanLatency(void);
    int64_t getMaxLatency(void);
    void clearMaxLatency(void);
    uint32_t getMeanRuntime(void);
    uint32_t getMaxRuntime(void);
    void clearMaxRuntime(void);
    /* CLI decoration methods */
    static void onCliGetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliSetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliGetLinkOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliClearLinkOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliGetMeanLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliGetMaxLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliClearMaxLatencyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliGetMeanRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliGetMaxRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
    static void onCliClearMaxRuntimeHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);


    //Public data structures
    //--

private:
    //Private methods
    static void updateStripHelper(void* p_lgsObject);
    void updateStrip(void);

    //Private data structures
    uint8_t linkNo;
    char* xmlconfig[4];
    SemaphoreHandle_t lgLinkLock;
    wdt* lgLinkWdt;
    char* sysName;
    char* usrName;
    char* desc;
    uint32_t overRuns;
    int64_t maxLatency;
    uint32_t maxRuntime;
    uint16_t avgSamples;
    int64_t* latencyVect;
    uint32_t* runtimeVect;
    uint8_t pin;
    QList<stripLed_t*> dirtyList;
    tinyxml2::XMLElement* lightGroupXmlElement;
    stripLed_t* stripCtrlBuff;
    Adafruit_NeoPixel* strip;
    uint8_t* stripWritebuff;
    flash* FLASHSLOW;
    flash* FLASHNORMAL;
    flash* FLASHFAST;
    signalMastAspects* signalMastAspectsObject;
    lgBase* lgs[MAX_LGSTRIPLEN];
    bool debug;
    static uint16_t lgLinkIndex;
};

/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                             */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                           */
/*==============================================================================================================================================*/
#endif /*LGLINK_H*/
