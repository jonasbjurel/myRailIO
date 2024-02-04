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
#ifndef SENSBASE_H
#define SENSBASE_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "rc.h"
#include "systemState.h"
#include "wdt.h"
#include "globalCli.h"
#include "sat.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "senseDigital.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "logHelpers.h"

class sat;
class senseDigital;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: senseBase                                                                                                                             */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_SENS_SYSNAME				0
#define XML_SENS_USRNAME				1
#define XML_SENS_DESC					2
#define XML_SENS_PORT					3
#define XML_SENS_TYPE					4
#define XML_SENS_ADMSTATE				5

#define SENSE_CALL_EXT(ext_p, type, method)\
		if(!strcmp(type, "DIGITAL"))\
			((senseDigital*)ext_p)->method;\
		else\
			panic("senseBase::CALL_EXT: Non supported type %s", type)

#define SENSE_CALL_EXT_RC(ext_p, type, method)\
		rc_t EXT_RC;\
		if(!strcmp(type, "DIGITAL"))\
			EXT_RC = ((senseDigital*)ext_p)->method;\
		else{\
			EXT_RC = RC_GEN_ERR;\
			panic("senseBase::CALL_EXT: Non supported type: %s", type);\
		}

class senseBase : public systemState, globalCli {
public:
	//Public methods
	senseBase(uint8_t p_sensPort, sat* p_satHandle);
	~senseBase(void);
	rc_t init(void);
	void onConfig(const tinyxml2::XMLElement* p_sensXmlElement);
	rc_t start(void);
	void onDiscovered(satelite* p_sateliteLibHandle, bool p_exists);
	void onSenseChange(bool p_senseVal);
	static void onSystateChangeHelper(const void* p_senseBaseHandle, sysState_t p_sysState);
	void onSysStateChange(sysState_t p_sysState);
	void failsafe(bool p_failsafe);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	static void wdtKickedHelper(void* senseBaseHandle);
	void wdtKicked(void);
	rc_t getOpStateStr(char* p_opStateStr);
	rc_t setSystemName(char* p_sysName, bool p_force = false);
	const char* getSystemName(bool p_force = false);
	rc_t setUsrName(const char* p_usrName, bool p_force = false);
	const char* getUsrName(bool p_force = false);
	rc_t setDesc(const char* p_description, bool p_force = false);
	const char* getDesc(bool p_force = false);
	rc_t setPort(uint8_t p_port, bool p_force = false);
	uint8_t getPort(bool p_force = false);
	rc_t setProperty(uint8_t p_propertyId, const char* p_propertyVal, bool p_force = false);
	rc_t getProperty(uint8_t p_propertyId, char* p_propertyVal, bool p_force = false);
	rc_t getSensing(const char* p_sensing);
	const char* getLogLevel(void);
	void setDebug(bool p_debug);
	bool getDebug(void);
	const char* getLogContextName(void);
	/* CLI decoration methods */
	static void onCliGetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetPortHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetSensingHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliGetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);
	static void onCliSetPropertyHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable);

	//Public data structures
	sat* satHandle;


private:
	//Private methods
	//--

	//Private data structures
	char* logContextName;
	uint8_t sensPort;
	uint8_t satAddr;
	uint8_t satLinkNo;
	sysState_t prevSysState;
	char* xmlconfig[6];
	bool debug;
	satelite* satLibHandle;
	void* extentionSensClassObj;
	SemaphoreHandle_t sensLock;
};

#endif /*SENSBASE_H*/
