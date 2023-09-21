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
#ifndef LGBASE_H
#define LGBASE_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "wdt.h"
#include "globalCli.h"
#include "cliGlobalDefinitions.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "lgLink.h"
#include "lgSignalMast.h"
#include "logHelpers.h"

class lgLink;
class lgSignalMast;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lgBase                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_LG_SYSNAME					0
#define XML_LG_USRNAME					1
#define XML_LG_DESC						2
#define XML_LG_LINKADDR					3
#define XML_LG_TYPE						4
#define XML_LG_PROPERTY1				5
#define XML_LG_PROPERTY2				6
#define XML_LG_PROPERTY3				7
#define XML_LG_ADMSTATE					8

#define LG_CALL_EXT(ext_p, type, method)\
		if(!strcmp(type, "SIGNAL MAST"))\
			((lgSignalMast*)ext_p)->method;\
		else\
			panic("lgBase::CALL_EXT: Non supported type: \"%s\"", type)

#define LG_CALL_EXT_RC(ext_p, type, method)\
		rc_t EXT_RC;\
		if(!strcmp(type, "SIGNAL MAST")){\
			EXT_RC = ((lgSignalMast*)ext_p)->method;\
		}\
		else{\
			EXT_RC = RC_GEN_ERR;\
			panic("lgBase::CALL_EXT_RC: Non supported type: \"%s\"", type);\
		}

class lgBase : public systemState, public globalCli {
public:
	//Public methods
	lgBase(uint8_t p_lgAddress, lgLink* p_lgLinkHandle);
	~lgBase(void);
	rc_t init(void);
	void onConfig(const tinyxml2::XMLElement* p_sensXmlElement);
	rc_t start(void);
	static void onSysStateChangeHelper(const void* p_lgBaseHandle, sysState_t p_sysState);
	void onSysStateChange(sysState_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgBaseHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgBaseHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	static void wdtKickedHelper(void* lgBaseHandle);
	void wdtKicked(void);
	rc_t setSystemName(const char* p_systemName, bool p_force = false);
	rc_t getSystemName(char* p_systemName, bool p_force = false);
	rc_t setUsrName(const char* p_usrName, bool p_force = false);
	rc_t getUsrName(char* p_userName, bool p_force = false);
	rc_t setDesc(const char* p_description, bool p_force = false);
	rc_t getDesc(char* p_desc, bool p_force = false);
	rc_t setAddress(uint8_t p_address, bool p_force = false);
	rc_t getAddress(uint8_t* p_address, bool p_force = false);
	rc_t setNoOffLeds(uint8_t p_noOfLeds, bool p_force = false);
	rc_t getNoOffLeds(uint8_t* p_noOfLeds, bool p_force = false);
	rc_t setProperty(uint8_t p_propertyId, const char* p_propertyValue, bool p_force = false);
	rc_t getProperty(uint8_t p_propertyId, char* p_propertyValue, bool p_force = false);
	rc_t getShowing(char* p_showing, bool p_force = false);
	rc_t setShowing(const char* p_showing, bool p_force = false);
	const char* getLogLevel(void);
	void setDebug(bool p_debug);
	bool getDebug(void);
	void setStripOffset(const uint16_t p_stripOffset);
	uint16_t getStripOffset(void);
	const char* getLogContextName(void);

	/* CLI decoration methods */
	static void onCliGetAddressHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetAddressHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetLedCntHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetLedCntHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetLedOffsetHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetLedOffsetHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetShowingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);

	//Public data structures
	lgLink* lgLinkHandle;

private:
	//Private methods
	//--

	//Private data structures
	char* logContextName;
	uint8_t lgAddress;
	uint8_t lgLinkNo;
	char* xmlconfig[9];
	bool debug;
	uint16_t stripOffset;
	void* extentionLgClassObj;
	SemaphoreHandle_t lgBaseLock;
	sysState_t prevSysState;
};

#endif /*LGBASE_H*/
