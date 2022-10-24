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
#ifndef ACTBASE_H
#define ACTBASE_H



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
#include "sat.h"
#include "actTurn.h"
#include "actLight.h"
#include "actMem.h"
#include "libraries/genericIOSatellite/LIB/src/Satelite.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
class sat;
class actTurn;
class actLight;
class actMem;
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: actBase                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XML_ACT_SYSNAME					0
#define XML_ACT_USRNAME					1
#define XML_ACT_DESC					2
#define XML_ACT_PORT					3
#define XML_ACT_TYPE					4
#define XML_ACT_SUBTYPE					5
#define XML_ACT_PROPERTIES				6

#define CALL_EXT(ext_p, type, method)\
		if(!strcmp(type, "TURNOUT"))\
			((actTurn*)ext_p)->method;\
		else if(!strcmp(type, "LIGHT"))\
			((actLight*)ext_p)->method;\
		else if(!strcmp(type, "MEMORY"))\
			((actMem*)ext_p)->method;\
		else\
			panic("actBase::CALL_EXT: Non supported type - rebooting")


class actBase : public systemState {
public:
	//Public methods
	actBase(uint8_t p_actPort, sat* p_satHandle);
	~actBase(void);
	rc_t init(void);
	void onConfig(const tinyxml2::XMLElement* p_sensXmlElement);
	rc_t start(void);
	void onDiscovered(satelite* p_sateliteLibHandle);
	static void onSysStateChangeHelper(const void* p_actBaseHandle, uint16_t p_sysState);
	void onSysStateChange(uint16_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_sensHandle);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	rc_t setSystemName(const char* p_systemName, const bool p_force = false);
	const char* getSystemName(void);
	rc_t setUsrName(const char* p_usrName, bool p_force = false);
	const char* getUsrName(void);
	rc_t setDesc(const char* p_description, bool p_force = false);
	const char* getDesc(void);
	rc_t setPort(uint8_t p_port);
	int8_t getPort(void);
	void setDebug(bool p_debug);
	bool getDebug(void);

	//Public data structures
	sat* satHandle;
	uint8_t actPort;
	uint8_t satAddr;
	uint8_t satLinkNo;
	bool pendingStart;
	char* xmlconfig[6];
	bool debug;

private:
	//Private methods
	//--

	//Private data structures
	satelite* satLibHandle;
	void* extentionActClassObj;
	SemaphoreHandle_t actLock;
};

#endif /*ACTBASE_H*/
