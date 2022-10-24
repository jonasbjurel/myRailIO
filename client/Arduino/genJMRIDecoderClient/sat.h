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

#ifndef SAT_H
#define SAT_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "systemState.h"
#include "satLink.h"
#include "senseBase.h"
#include "actBase.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
class satLink;
class senseBase;
class actBase;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "sat(Satelite)"                                                                                                                       */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
#define XML_SAT_SYSNAME							0
#define XML_SAT_USRNAME							1
#define XML_SAT_DESC							2
#define XML_SAT_ADDR							3

class sat : public systemState {
public:
	//Public methods
	sat(uint8_t satAddr_p, satLink* linkHandle_p);
	~sat(void);
	rc_t init(void);
	void onConfig(tinyxml2::XMLElement* p_satXmlElement);
	rc_t start(void);
	void onDiscovered(satelite* p_sateliteLibHandle, uint8_t p_satAddr, bool p_exists);
	void onPmPoll(void);
	static void onSatLibStateChangeHelper(satelite* p_sateliteLibHandle, uint8_t p_linkAddr, uint8_t p_satAddr, satOpState_t p_satOpState, void* p_satHandle);
	void onSatLibStateChange(satOpState_t p_satOpState);
	static void onSysStateChangeHelper(const void* p_satHandle, uint16_t p_sysState);
	void onSysStateChange(const uint16_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t setSystemName(const char* p_systemName, const bool p_force = false);
	const char* getSystemName(void);
	rc_t setUsrName(const char* p_usrName, const bool p_force = false);
	const char* getUsrName(void);
	rc_t setDesc(const char* p_description, const bool p_force = false);
	const char* getDesc(void);
	rc_t setAddr(uint8_t p_addr);
	uint8_t getAddr(void);
	void setDebug(const bool p_debug);
	bool getDebug(void);
	uint32_t getRxCrcErrs(void);
	void clearRxCrcErrs(void);
	uint32_t getTxCrcErrs(void);
	void clearTxCrcErrs(void);
	uint32_t getWdErrs(void);
	void clearWdErrs(void);

	//Public data structures
	satLink* linkHandle;

private:
	//Private methods
	//-

	//Private data structures
	uint8_t satAddr;
	bool pendingStart;
	char* xmlconfig[4];
	bool debug;
	SemaphoreHandle_t satLock;
	satelite* satLibHandle;
	actBase* acts[MAX_ACT];
	senseBase* senses[MAX_SENS];
	uint32_t txUnderunErr;
	uint32_t rxOverRunErr;
	uint32_t scanTimingViolationErr;
	uint32_t rxCrcErr;
	uint32_t remoteCrcErr;
	uint32_t rxSymbolErr;
	uint32_t rxDataSizeErr;
	uint32_t wdErr;

	//Private data structures
};

/*==============================================================================================================================================*/
/* END Class sat                                                                                                                                */
/*==============================================================================================================================================*/

#endif /*SAT_H*/
