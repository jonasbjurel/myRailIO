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

#ifndef SATLINK_H
#define SATLINK_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "Arduino.h"
#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <QList.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "wdt.h"
#include "globalCli.h"
#include "cliGlobalDefinitions.h"
#include "decoder.h"
#include "sat.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "pinout.h"
#include "esp32SysConfig.h"
#include "taskWrapper.h"
#include "logHelpers.h"

class decoder;
class sat;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "satLink(Satelite Link)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
#define XML_SATLINK_SYSNAME							0
#define XML_SATLINK_USRNAME							1
#define XML_SATLINK_DESC							2
#define XML_SATLINK_LINK							3
#define XML_SATLINK_ADMSTATE						4

class satLink : public systemState, public globalCli {
public:
	//Public methods
	satLink(uint8_t p_linkNo, decoder* p_decoderHandle);
	~satLink(void);
	rc_t init(void);
	void onConfig(tinyxml2::XMLElement* p_satLinkXmlElement);
	rc_t start(void);
	void up(void);
	static void onDiscoveredSateliteHelper(satelite* p_sateliteLibHandle, uint8_t p_satLink, uint8_t p_satAddr, bool p_exists, void* p_satLinkHandle);
	void down(void);
	static void onSatLinkScanHelper(void* p_metaData);
	void onSatLinkScan(void);
	static void pmPollHelper(void* p_metaData);
	void onPmPoll(void);
	static void onSatLinkLibStateChangeHelper(sateliteLink* p_sateliteLinkLibHandler, uint8_t p_linkAddr, satOpState_t p_satOpState, void* p_satLinkHandler);
	void onSatLinkLibStateChange(satOpState_t p_satOpState);
	static void onSysStateChangeHelper(const void* p_satLinkHandle, sysState_t p_sysState);
	void onSysStateChange(const sysState_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t getOpStateStr(char* p_opStateStr);
	rc_t setSystemName(const char* p_systemName, bool p_force = false);
	rc_t getSystemName(char* p_systemName, bool p_force = false);
	rc_t setUsrName(const char* p_usrName, const bool p_force = false);
	rc_t getUsrName(char* p_userName, bool p_force = false);
	rc_t setDesc(const char* p_description, const bool p_force = false);
	rc_t getDesc(char* p_desc, bool p_force = false);
	rc_t setLink(uint8_t p_link, bool p_force = false);
	uint8_t getLink(bool p_force = false);
	const char* getLogLevel(void);
	void setDebug(const bool p_debug);
	bool getDebug(void);
	uint32_t getTxUnderruns(void);
	void clearTxUnderruns(void);
	uint32_t getRxOverruns(void);
	void clearRxOverruns(void);
	uint32_t getScanTimingViolations(void);
	void clearScanTimingViolations(void);
	uint32_t getRxCrcErrs(void);
	void clearRxCrcErrs(void);
	uint32_t getRemoteCrcErrs(void);
	void clearRemoteCrcErrs(void);
	uint32_t getRxSymbolErrs(void);
	void clearRxSymbolErrs(void);
	uint32_t getRxDataSizeErrs(void);
	void clearRxDataSizeErrs(void);
	uint32_t getWdErrs(void);
	void clearWdErrs(void);
	//int64_t lgLink::getMeanLatency(void) {}
	//int64_t lgLink::getMaxLatency(void) {}
	//void lgLink::clearMaxLatency(void) {}
	//uint32_t lgLink::getMeanRuntime(void) {}
	//uint32_t lgLink::getMaxRuntime(void) {}
	//void lgLink::clearMaxRuntime(void) {}
	const char* getLogContextName(void);

	/* CLI decoration methods */
	static void onCliGetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetLinkHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetTxUnderrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearTxUnderrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetRxOverrrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearRxOverrunsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetScanTimingViolationsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearScanTimingViolationsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearRxCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetRemoteCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearRemoteCrcErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetRxSymbolErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearRxSymbolErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetRxDataSizeErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearRxDataSizeErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliClearWdErrsHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);

	//Public data structures

private:
	//Private methods
	//-

	//Private data structures
	EXT_RAM_ATTR char* logContextName;
	EXT_RAM_ATTR uint8_t linkNo;
	EXT_RAM_ATTR char* xmlconfig[5];
	EXT_RAM_ATTR bool debug;
	EXT_RAM_ATTR bool pmPoll;
	EXT_RAM_ATTR sysState_t prevSysState;
	EXT_RAM_ATTR bool satLinkScanDisabled;
	EXT_RAM_ATTR SemaphoreHandle_t satLinkPmPollLock;
	EXT_RAM_ATTR SemaphoreHandle_t upDownLock;
	EXT_RAM_ATTR sateliteLink* satLinkLibHandle;
	EXT_RAM_ATTR sat* sats[MAX_SATELITES];
	EXT_RAM_ATTR uint32_t txUnderunErr;
	EXT_RAM_ATTR uint32_t rxOverRunErr;
	EXT_RAM_ATTR uint32_t scanTimingViolationErr;
	EXT_RAM_ATTR uint32_t rxCrcErr;
	EXT_RAM_ATTR uint32_t remoteCrcErr;
	EXT_RAM_ATTR uint32_t rxSymbolErr;
	EXT_RAM_ATTR uint32_t rxDataSizeErr;
	EXT_RAM_ATTR uint32_t wdErr;
	wdt* linkScanWdt;
};
/*==============================================================================================================================================*/
/* END Class satLink                                                                                                                            */
/*==============================================================================================================================================*/
#endif /*SATLINK_H*/
