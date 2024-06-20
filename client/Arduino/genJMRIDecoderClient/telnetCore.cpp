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
	LOG_INFO_NOFMT("Starting Telnet" CR);
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
		if (!(telnetWdt = new (heap_caps_malloc(sizeof(wdt), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wdt(WDT_TELNET_POLL_LOOP_TIMEOUT_MS, "Telnet/CLI",
			FAULTACTION_GLOBAL_FAILSAFE | FAULTACTION_GLOBAL_REBOOT | FAULTACTION_ESCALATE_INTERGAP))) {
			panic("Failed to start Telnet watchdog");
			return RC_GEN_ERR;
		}
		if (!eTaskCreate(
			poll,																		// Task function
			CPU_TELNET_TASKNAME,														// Task function name reference
			CPU_TELNET_STACKSIZE_1K * 1024,												// Stack size
			NULL,																		// Parameter passing
			CPU_TELNET_PRIO,															// Priority 0-24, higher is more
			CPU_TELNET_STACK_ATTR)) {													// Task stack attribute
			panic("Could not start Telnet polling task");
			delete telnetWdt;
			return RC_OUT_OF_MEM_ERR;
		}
		telnetWdt->activate();
		LOG_INFO_NOFMT("Telnet started" CR);
	}
	else {
		panic("Failed to start Telnet");
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
	LOG_INFO("CLI connected from: %s" CR, p_ip);
	connections++;
	strcpy(ip, p_ip.c_str());
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), true, telnetConnectCbMetaData);
}

void telnetCore::onTelnetDisconnect(String p_ip) {
	LOG_INFO("CLI disconnected from: %s" CR, p_ip);
	connections--;
	strcpy(ip, "");
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), false, telnetConnectCbMetaData);
}

void telnetCore::regTelnetDisConnectCb(telnetDisConnectCb_t p_telnetDisConnectCb,
	void* p_telnetDisConnectCbMetaData) {
	telnetDisConnectCb = p_telnetDisConnectCb;
	telnetDisConnectCbMetaData = p_telnetDisConnectCbMetaData;
}

void telnetCore::onTelnetReconnect(String p_ip) {
	LOG_INFO("dCLI reconnected from: %s" CR, p_ip);
	strcpy(ip, p_ip.c_str());
	if (telnetConnectCb)
		telnetConnectCb(p_ip.c_str(), true, telnetConnectCbMetaData);
}

void telnetCore::onTelnetConnectionAttempt(String p_ip) {
	LOG_INFO("CLI connection failed from: %s" CR, p_ip);
}

void telnetCore::regTelnetInputCb(telnetInputCb_t p_telnetInputCb,
								  void* p_telnetInputCbMetaData) {
	telnetInputCb = p_telnetInputCb;
	telnetInputCbMetaData = p_telnetInputCbMetaData;
}

void telnetCore::onTelnetInput(String p_cmd) {
	LOG_VERBOSE("Received input from CLI client: %s" CR, ip);
	if (telnetInputCb)
		telnetInputCb((char*)p_cmd.c_str(), telnetInputCbMetaData);
}

void telnetCore::print(const char* p_output) {
	telnet.print(p_output);
}

void telnetCore::poll(void* dummy) {
	LOG_INFO_NOFMT("Telnet polling started" CR);
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
	delete telnetWdt;
	vTaskDelete(NULL);
}
/*==============================================================================================================================================*/
/* END Class telnetCore                                                                                                                         */
/*==============================================================================================================================================*/
