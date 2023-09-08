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
	void down(void);
	static void onDiscoveredSateliteHelper(satelite* p_sateliteLibHandle, uint8_t p_satLink, uint8_t p_satAddr, bool p_exists, void* p_satLinkHandle);
	static void pmPollHelper(void* p_metaData);
	void onPmPoll(void);
	static void onSatLinkLibStateChangeHelper(sateliteLink* p_sateliteLinkLibHandler, uint8_t p_linkAddr, satOpState_t p_satOpState, void* p_satLinkHandler);
	void onSatLinkLibStateChange(satOpState_t p_satOpState);
	static void onSysStateChangeHelper(const void* p_satLinkHandle, sysState_t p_sysState);
	void onSysStateChange(const sysState_t p_sysState);
	void processSysState(void);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkObject);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t getOpStateStr(char* p_opStateStr);
	rc_t setSystemName(const char* p_systemName, const bool p_force = false);
	const char* getSystemName(bool p_force = false);
	rc_t setUsrName(const char* p_usrName, const bool p_force = false);
	const char* getUsrName(bool p_force = false);
	rc_t setDesc(const char* p_description, const bool p_force = false);
	const char* getDesc(bool p_force = false);
	rc_t setLink(uint8_t p_link);
	uint8_t getLink(void);
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
	uint8_t linkNo;
	char* xmlconfig[5];
	bool debug;
	bool pmPoll;
	sysState_t prevSysState;
	bool satLinkScanDisabled;
	SemaphoreHandle_t satLinkPmPollLock;
	wdt* satLinkWdt;
	sateliteLink* satLinkLibHandle;
	sat* sats[MAX_SATELITES];
	uint32_t txUnderunErr;
	uint32_t rxOverRunErr;
	uint32_t scanTimingViolationErr;
	uint32_t rxCrcErr;
	uint32_t remoteCrcErr;
	uint32_t rxSymbolErr;
	uint32_t rxDataSizeErr;
	uint32_t wdErr;
};
/*==============================================================================================================================================*/
/* END Class satLink                                                                                                                            */
/*==============================================================================================================================================*/
#endif /*SATLINK_H*/
