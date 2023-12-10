/*============================================================================================================================================= =*/
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
#include "networking.h"
#include "libraries/tinyxml2/tinyxml2.h"
#include "logHelpers.h"
#include "rc.h"
#include "systemState.h"
#include "globalCli.h"
#include "satLink.h"
#include "lgLink.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "ntpTime.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
#include "cpu.h"

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
#define XML_DECODER_MQTT_PINGPERIOD					3
#define XML_DECODER_MQTT_KEEPALIVEPERIOD			4
#define XML_DECODER_NTPURI							5
#define XML_DECODER_NTPPORT							6
#define XML_DECODER_TZ_AREA							7
#define XML_DECODER_TZ_GMTOFFSET					8
#define XML_DECODER_LOGLEVEL						9
#define XML_DECODER_RSYSLOGSERVER					10
#define XML_DECODER_RSYSLOGPORT						11
#define XML_DECODER_FAILSAFE						12
#define XML_DECODER_SYSNAME							13
#define XML_DECODER_USRNAME							14
#define XML_DECODER_DESC							15
#define XML_DECODER_MAC								16
#define XML_DECODER_URI								17
#define XML_DECODER_ADMSTATE						18



class decoder : public systemState, public globalCli {
public:
	//Public methods
	decoder(void);
	~decoder(void);
	rc_t init(void);
	static void onConfigHelper(const char* p_topic, const char* p_payload, const void* p_decoderObj);
	void onConfig(const char* p_topic, const char* p_payload);
	rc_t start(void);
	static void onSysStateChangeHelper(const void* p_miscData, sysState_t p_sysState);
	void onSysStateChange(sysState_t p_sysState);
	static void onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject);
	void onOpStateChange(const char* p_topic, const char* p_payload);
	static void onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject);
	void onAdmStateChange(const char* p_topic, const char* p_payload);
	static void onMqttOpStateChangeHelper(const void* p_miscCbData, sysState_t p_sysState);
	void onMqttOpStateChange(sysState_t p_sysState);
	rc_t getOpStateStr(char* p_opStateStr);
	rc_t setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force = false);
	const char* getMqttBrokerURI(bool p_force = false);
	rc_t setMqttPort(int32_t p_mqttPort, bool p_force = false);
	uint16_t getMqttPort(bool p_force = false);
	rc_t setMqttPrefix(const char* p_mqttPrefix, bool p_force = false);
	const char* getMqttPrefix(bool p_force = false);
	rc_t setKeepAlivePeriod(uint8_t p_keepAlivePeriod, bool p_force = false);
	float getKeepAlivePeriod(bool p_force = false);
	rc_t setPingPeriod(float p_pingPeriod, bool p_force = false);
	float getPingPeriod(bool p_force = false);
	rc_t setNtpServer(const char* p_ntpServer, uint16_t p_port, bool p_force = false);
	rc_t setTz(const char* p_tz, bool p_force = false);
	rc_t getTz(char* p_tz, bool p_force = false);
	rc_t setLogLevel(const char* p_logLevel, bool p_force = false);
	const char* getLogLevel(void);
	rc_t setRSyslogServer(const char* p_uri, uint16_t p_port = RSYSLOG_DEFAULT_PORT, bool p_force = false);
	rc_t getRSyslogServer(char* p_uri, uint16_t* p_port, bool p_force = false);
	rc_t setFailSafe(const bool p_failsafe, bool p_force = false);
	bool getFailSafe(bool p_force = false);
	rc_t setSystemName(const char* p_systemName, bool p_force = false);
	rc_t getSystemName(char* p_systemName, bool p_force = false);
	rc_t setUsrName(const char* p_usrName, bool p_force = false);
	rc_t getUsrName(char* p_userName, bool p_force = false);
	rc_t setDesc(const char* p_description, bool p_force = false);
	rc_t getDesc(char* p_desc, bool p_force = false);
	rc_t setMac(const char* p_mac, bool p_force = false);
	const char* getMac(bool p_force = false);
	rc_t setDecoderUri(const char* p_decoderUri, bool p_force = false);
	const char* getDecoderUri(bool p_force = false);
	void setDebug(bool p_debug);
	bool getDebug(void);
	const char* getLogContextName(void);
	static void onRebootHelper(const char* p_topic, const char* p_payload, const void* p_decoderObject);
	void onReboot(void);

	/* CLI decoration methods */
	// No CLI decorations for the decoder context - all decoder related MOs are available through the global CLI context.

	//Public data structures
	char* xmlconfig[19];


private:
	//Private methods

	//Private data structures
	char* logContextName;
	satLink* satLinks[MAX_SATLINKS];
	lgLink* lgLinks[MAX_LGLINKS];
	SemaphoreHandle_t decoderLock;
	tinyxml2::XMLDocument* xmlConfigDoc;
	tinyxml2::XMLElement* satLinkXmlElement;
	tinyxml2::XMLElement* lgLinkXmlElement;
	sysState_t prevSysState;
	bool debug;
};

/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/
#endif //DECODER_H
