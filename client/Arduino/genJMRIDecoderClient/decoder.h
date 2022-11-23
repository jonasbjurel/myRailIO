/*============================================================================================================================================= =*/
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

#ifndef DECODER_H
#define DECODER_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "globalCli.h"
#include "networking.h"
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "logHelpers.h"
#include "rc.h"
#include "systemState.h"
#include "satLink.h"
#include "lgLink.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"

class lgLink;
class satLink;

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/


/*==============================================================================================================================================*/
/* Class: decoder                                                                                                                               */
/* Purpose: The "decoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,           */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroup links. Lightgroups [Signal Masts | general Lights |       */
/*          sequencedLights], Satelite Links, Satelites, Sensors, Actueators, etc.                                                              */
/*          Turnouts or sensors...                                                                                                              */
/*          The "decoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such    */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/*     public:                                                                                                                                  */
/*          const uint8_t decoder::init(void): Initializes the decoder and all it's data-structures, initializes common services, performs a    */
/*                                       discovery process, and requests the decoder configuration from the server                              */
/*                                       The method blocks until it successfully got a valid configuration.                                     */
/*                                       Params: -                                                                                              */
/*                                       Returns: const ERR Return Codes                                                                        */
/*                                                                                                                                              */
/*          void decoder::onConfig(const char* p_topic, const char* p_payload, const void* p_dummy): Config callback                      */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: const char* p_topic: MQTT config topic                                                        */
/*                                                const char* p_payload: Configuration payload                                                  */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          const uint8_t decoder::start(void): Starts the decoder services, if called before the decoder configuration successfully have been  */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: -                                                                                             */
/*                                        Returns: const ERR Return Codes                                                                       */
/*                                                                                                                                              */
/*          void decoder::onSystateChange(const uint16_t p_sysState): system/opstate change callback ...................                        */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: const uint16_t p_sysState: New system state                                                   */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          void decoder::onOpStateChange(const char* p_topic, const char* p_payload, const void* DUMMY): opstatechange callback from server    */
/*                                        Params: const char* p_topic: MQTT opstate change topic                                                */
/*                                                const char* p_payload: MQTT opstate payload                                                   */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          void decoder::onAdmStateChange(const char* p_topic, const char* p_payload, const void* DUMMY): admstatechange callback from server  */
/*                                        Params: const char* p_topic: MQTT admstate change topic                                               */
/*                                                const char* p_payload: MQTT admstate payload                                                  */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          uint8_t decoder::getSysState(void): Get system state of decoder                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const Operational state bitmap                                                               */
/*                                                                                                                                              */
/*          const char* decoder::getMqttURI(void): Get the MQTT brooker URI                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT URI string pointer reference                                                      */
/*                                                                                                                                              */
/*          const uint16_t decoder::getMqttPort(void): get the MQTT brooker port                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT port                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getMqttPrefix(void): get the MQTT topic prefix                                                                 */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT topic string pointer reference                                                    */
/*                                                                                                                                              */
/*          const float decoder::getKeepAlivePeriod(void): get the MQTT keep-alive period                                                       */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT keep-alive period                                                                 */
/*                                                                                                                                              */
/*          const char* decoder::getNtpServer(void): get the NTP server URI                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const NTP server URI string pointer reference                                                */
/*                                                                                                                                              */
/*          const uint16_t decoder::getNtpPort(void): get the NTP Port                                                                          */
/*                                        Params: -                                                                                             */
/*                                        Returns: NTP Port                                                                                     */
/*                                                                                                                                              */
/*          const uint8_t decoder::getTz(void): get time zone                                                                                   */
/*                                        Params: -                                                                                             */
/*                                        Returns: const time zone                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getLogLevel(void): get loglevel                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const log level string pointer reference                                                     */
/*                                                                                                                                              */
/*          const bool decoder::getFailSafe(void): get fail-safe                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const fail-safe                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getSystemName(void): get system name                                                                           */
/*                                        Params: -                                                                                             */
/*                                        Returns: const system name string pointer reference                                                   */
/*                                                                                                                                              */
/*          const char* decoder::getUsrName(void): get user name                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const user name string pointer reference                                                     */
/*                                                                                                                                              */
/*          const char* decoder::getDesc(void): get description                                                                                 */
/*                                        Params: -                                                                                             */
/*                                        Returns: const description string pointer reference                                                   */
/*                                                                                                                                              */
/*          const char* decoder::getMac(void): get decoder MAC address                                                                          */
/*                                        Params: -                                                                                             */
/*                                        Returns: const decoder MAC address string pointer reference                                           */
/*                                                                                                                                              */
/*          const char* decoder::getUri(void): get decoder URI                                                                                  */
/*                                        Params: -                                                                                             */
/*                                        Returns: const decoder URI string pointer reference                                                   */
/*                                                                                                                                              */
/*          void setDebug(const bool p_debug): set object debug status                                                                          */
/*                                        Params: p_debug set debug status                                                                      */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*     private:                                                                                                                                 */
/*                                                                                                                                              */
/* Data structures:                                                                                                                             */
/*     public: sysState: Holds the systemState object, wich needs to be reachable by other objects systemState objects                          */
/*     private: UPDATE NEEDED                                                                                                                   */
/*==============================================================================================================================================*/
#define XML_DECODER_MQTT_URI						0
#define XML_DECODER_MQTT_PORT						1
#define XML_DECODER_MQTT_PREFIX						2
#define XML_DECODER_MQTT_KEEPALIVEPERIOD			3
#define XML_DECODER_NTPURI							4
#define XML_DECODER_NTPPORT							5
#define XML_DECODER_TZ								6
#define XML_DECODER_LOGLEVEL						7
#define XML_DECODER_FAILSAFE						8
#define XML_DECODER_SYSNAME							9
#define XML_DECODER_USRNAME							10
#define XML_DECODER_DESC							11
#define XML_DECODER_MAC								12
#define XML_DECODER_URI								13

class decoder : public systemState, public globalCli {
public:
	//Public methods
	decoder(void);
	~decoder(void);
	rc_t init(void);
	static void onConfigHelper(const char* p_topic, const char* p_payload, const void* p_decoderObj);
	void onConfig(const char* p_topic, const char* p_payload);
	rc_t start(void);
	static void onSysStateChangeHelper(const void* p_miscData, uint16_t p_sysState);
	void onSysStateChange(uint16_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	static void onMqttChangeHelper(uint8_t p_mqttState, const void* p_decoderObj);
	void onMqttChange(uint8_t p_mqttState);
	rc_t getOpStateStr(char* p_opStateStr);
	rc_t setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force = false);
	const char* getMqttBrokerURI(void);
	rc_t setMqttPort(const uint16_t p_mqttPort, bool p_force = false);
	uint16_t getMqttPort(void);
	rc_t setMqttPrefix(const char* p_mqttPrefix, bool p_force = false);
	const char* getMqttPrefix(void);
	rc_t setKeepAlivePeriod(const float p_keepAlivePeriod, bool p_force = false);
	float getKeepAlivePeriod(void);
	rc_t setPingPeriod(const float p_pingPeriod, bool p_force = false);
	float getPingPeriod(void);
	rc_t setNtpServer(const char* p_ntpServer, bool p_force = false);
	const char* getNtpServer(void);
	rc_t setNtpPort(const uint16_t p_ntpPort, bool p_force = false);
	uint16_t getNtpPort(void);
	rc_t setTz(const uint8_t p_tz, bool p_force = false);
	uint8_t getTz(void);
	rc_t setLogLevel(const char* p_logLevel, bool p_force = false);
	const char* getLogLevel(void);
	rc_t setFailSafe(const bool p_failsafe, bool p_force = false);
	bool getFailSafe(void);
	rc_t setSystemName(const char* p_systemName, bool p_force = false);
	const char* getSystemName(void);
	rc_t setUsrName(const char* p_usrName, bool p_force = false);
	const char* getUsrName(void);
	rc_t setDesc(const char* p_description, bool p_force = false);
	const char* getDesc(void);
	rc_t setMac(const char* p_mac, bool p_force = false);
	const char* getMac(void);
	rc_t setDecoderUri(const char* p_decoderUri, bool p_force = false);
	const char* getDecoderUri(void);
	void setDebug(bool p_debug);
	bool getDebug(void);
	/* CLI decoration methods */
	// No CLI decorations for the decoder context - all decoder related MOs are available through the global CLI context.

	//Public data structures

private:
	//Private methods

	//Private data structures
	char* xmlconfig[14];
	satLink* satLinks[MAX_SATLINKS];
	lgLink* lgLinks[MAX_LGLINKS];
	SemaphoreHandle_t decoderLock;
	tinyxml2::XMLDocument* xmlConfigDoc;
	tinyxml2::XMLElement* satLinkXmlElement;
	tinyxml2::XMLElement* lgLinkXmlElement;
	bool debug;
};

/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/
#endif //DECODER_H
