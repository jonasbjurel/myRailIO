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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "telnetCore.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: telnetCore                                                                                                                            */
/* Purpose:  See telnetCore.h                                                                                                                   */
/* Description:  See telnetCore.h																												*/
/* Methods:  See telnetCore.h                                                                                                                   */
/* Data sructures: See telnetCore.h																												*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR ESPTelnet telnetCore::telnet;
EXT_RAM_ATTR telnetConnectCb_t* telnetCore::telnetConnectCb;
EXT_RAM_ATTR void* telnetCore::telnetConnectCbMetaData;
EXT_RAM_ATTR telnetInputCb_t* telnetCore::telnetInputCb;
EXT_RAM_ATTR void* telnetCore::telnetInputCbMetaData;
EXT_RAM_ATTR uint8_t telnetCore::connections = 0;
EXT_RAM_ATTR char telnetCore::ip[20];
EXT_RAM_ATTR wdt* telnetCore::telnetWdt;

rc_t telnetCore::start(void) {
	Log.INFO("telnetCore::start: Starting Telnet" CR);
	connections = 0;
	telnet.onConnect(onTelnetConnect);
	telnet.onConnectionAttempt(onTelnetConnectionAttempt);
	telnet.onReconnect(onTelnetReconnect);
	telnet.onDisconnect(onTelnetDisconnect);
	// passing a lambda function
	telnet.onInputReceived([](String str) {
		// checks for a certain command
		onTelnetInput(str);
		});
	if (telnet.begin()) {
		Log.INFO("decoderCli::init: CLI started" CR);
		if (!eTaskCreate(
				poll,																		// Task function
				CPU_TELNET_TASKNAME,														// Task function name reference
				CPU_TELNET_STACKSIZE_1K * 1024,												// Stack size
				NULL,																		// Parameter passing
				CPU_TELNET_PRIO,															// Priority 0-24, higher is more
				CPU_TELNET_STACK_ATTR))														// Task stack attribute
			panic("telnetCore::start: Could not start Telnet polling task - rebooting..." CR);
	}
	else {
		Log.ERROR("decoderCli::init: Failed to start CLI" CR);
		return RC_GEN_ERR;
	}
	return RC_OK;
}

void telnetCore::reconnect(void) {
	telnet.stop();
	telnet.begin();
}

void telnetCore::regTelnetConnectCb(telnetConnectCb_t p_telnetConnectCb,
									void* p_telnetConnectCbMetaData) {
	telnetConnectCb = p_telnetConnectCb;
	telnetConnectCbMetaData = p_telnetConnectCbMetaData;
}

void telnetCore::onTelnetConnect(String p_ip) {
	Log.INFO("telnetCore::onTelnetConnect: CLI connected from: %s" CR, p_ip);
	connections++;
	strcpy(ip, p_ip.c_str());
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), true, telnetConnectCbMetaData);
}

void telnetCore::onTelnetDisconnect(String p_ip) {
	Log.INFO("decoderCli::onTelnetDisconnect: CLI disconnected from: %s" CR, p_ip);
	connections--;
	strcpy(ip, "");
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), false, telnetConnectCbMetaData);
}

void telnetCore::onTelnetReconnect(String p_ip) {
	Log.INFO("decoderCli::onTelnetReconnect: CLI reconnected from: %s" CR, p_ip);
	strcpy(ip, p_ip.c_str());
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), true, telnetConnectCbMetaData);
}

void telnetCore::onTelnetConnectionAttempt(String p_ip) {
	Log.INFO("decoderCli::onTelnetConnectionAttempt: \
			    CLI connection failed from: %s" CR, p_ip);
}

void telnetCore::regTelnetInputCb(telnetInputCb_t p_telnetInputCb,
								  void* p_telnetInputCbMetaData) {
	telnetInputCb = p_telnetInputCb;
	telnetInputCbMetaData = p_telnetInputCbMetaData;
}

void telnetCore::onTelnetInput(String p_cmd) {
	Log.VERBOSE("telnetCore::onTelnetInput:" \
				"Received input from Telnet client: %s" CR, ip);
	if (telnetInputCb)
		telnetInputCb((char*)p_cmd.c_str(), telnetInputCbMetaData);
}

void telnetCore::print(const char* p_output) {
	telnet.print(p_output);
}

void telnetCore::poll(void* dummy) {
	Log.INFO("telnetServer::poll: telnet polling started" CR);
	telnetWdt = new (heap_caps_malloc(sizeof(wdt), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wdt(10 * 1000, "Telnet watchdog",
						FAULTACTION_FAILSAFE_ALL | FAULTACTION_REBOOT);
	while (true) {
		telnet.loop();
		if (connections) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}
		else {
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		telnetWdt->feed();
	}
}
/*==============================================================================================================================================*/
/* END Class telnetCore                                                                                                                         */
/*==============================================================================================================================================*/
