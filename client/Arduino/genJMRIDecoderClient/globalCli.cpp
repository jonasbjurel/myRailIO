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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "globalCli.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: globalCli                                                                                                                             */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
EXT_RAM_ATTR globalCli* globalCli::rootHandle;
char* globalCli::testBuff = NULL;

globalCli::globalCli(const char* p_moType, const char* p_moName, uint16_t p_moIndex, globalCli* p_parent_context, bool p_root) : cliCore(p_moType, p_moName, p_moIndex, p_parent_context, p_root) {
	if (p_root) {
		rootHandle = this;
	}
	moType = p_moType;
	moName = p_moName;
	moIndex = p_moIndex;
	parentContext = p_parent_context;
}

globalCli::~globalCli(void) {
}

void globalCli::regGlobalNCommonCliMOCmds(void) {
	regGlobalCliMOCmds();
	regCommonCliMOCmds();
}

void globalCli::regGlobalCliMOCmds(void) {
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: help																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
	regCmdMoArg(HELP_CLI_CMD, GLOBAL_MO_NAME, NULL, onCliHelp);
	regCmdHelp(HELP_CLI_CMD, GLOBAL_MO_NAME, NULL, GLOBAL_HELP_HELP_TXT);
	regCmdMoArg(HELP_CLI_CMD, GLOBAL_MO_NAME, HELP_SUB_MO_NAME, onCliHelp);
	regCmdHelp(HELP_CLI_CMD, GLOBAL_MO_NAME, HELP_SUB_MO_NAME, GLOBAL_HELP_HELP_TXT);
	regCmdMoArg(HELP_CLI_CMD, GLOBAL_MO_NAME, CLIHELP_SUB_MO_NAME, onCliHelp);
	regCmdHelp(HELP_CLI_CMD, GLOBAL_MO_NAME, CLIHELP_SUB_MO_NAME, GLOBAL_FULL_CLI_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: context																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	//Context SubMo - provides means for context navigation
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, onCliGetContextHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, GLOBAL_GET_CONTEXT_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, onCliSetContextHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, GLOBAL_SET_CONTEXT_HELP_TXT);

	//Topolgy SubMo - provides means for context topology visualization
	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, TOPOLOGY_SUB_MO_NAME, onCliShowTopology);
	regCmdFlagArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, TOPOLOGY_SUB_MO_NAME, "childs", 1, false);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, TOPOLOGY_SUB_MO_NAME, GLOBAL_SHOW_TOPOLOGY_HELP_TXT);

	//Command SubMo - provides means to show commands available for a speciffic context
	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, COMMANDS_SUB_MO_NAME, onCliShowCommands);
	regCmdFlagArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, COMMANDS_SUB_MO_NAME, "all", 1, false);
	regCmdFlagArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, COMMANDS_SUB_MO_NAME, "help", 1, false);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, COMMANDS_SUB_MO_NAME, GLOBAL_SHOW_COMMANDS_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: reboot																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	regCmdMoArg(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, onCliReboot);
	regCmdHelp(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, GLOBAL_REBOOT_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: uptime																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

//global uptime SubMo
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, onCliGetUptime);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, GLOBAL_GET_UPTIME_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: cpu																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
	regCmdMoArg(START_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, onCliStartCpu);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "stats", 1, false);
	regCmdHelp(START_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, GLOBAL_START_CPU_HELP_TXT);

	regCmdMoArg(STOP_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, onCliStopCpu);
	regCmdFlagArg(STOP_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "stats", 1, false);
	regCmdHelp(STOP_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, GLOBAL_STOP_CPU_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, onCliGetCpu);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "tasks", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "task", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "cpuusage", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "watermark", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, "stats", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, GLOBAL_GET_CPU_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, onCliShowCpu);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, CPU_SUB_MO_NAME, GLOBAL_SHOW_CPU_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: memory																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, onCliGetMem);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "internal", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "total", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "available", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "used", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "watermark", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "average", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "trend", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "maxblock", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, GLOBAL_GET_CPUMEM_HELP_TXT);
	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, onCliShowMem);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, GLOBAL_SHOW_CPUMEM_HELP_TXT);

	regCmdMoArg(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, onCliStartMem);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "allocate", 1, true);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "internal", 1, false);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "external", 1, false);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "default", 1, false);
	regCmdHelp(START_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, GLOBAL_START_CPUMEM_HELP_TXT);

	regCmdMoArg(STOP_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, onCliStopMem);
	regCmdFlagArg(STOP_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, "allocate", 1, false);
	regCmdHelp(STOP_CLI_CMD, GLOBAL_MO_NAME, CPUMEM_SUB_MO_NAME, GLOBAL_STOP_CPUMEM_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: network																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, onCliSetNetwork);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "hostname", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "address", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "mask", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "gw", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "dns", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "persist", 1, false);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, GLOBAL_SET_NETWORK_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, onCliGetNetwork);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "ssid", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "bssid", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "channel", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "auth", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "rssi", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "mac", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "hostname", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "address", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "mask", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "gw", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "dns", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "opstate", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "scanap", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, GLOBAL_GET_NETWORK_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, onCliShowNetwork);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, GLOBAL_SHOW_NETWORK_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: mqtt																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, onCliSetMqtt);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "uri", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "port", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "clientid", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "qos", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "keepalive", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "ping", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "persist", 1, false);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, GLOBAL_SET_MQTT_HELP_TXT);

	regCmdMoArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, onCliClearMqtt);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "maxlatency", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "overruns", 1, false);
	regCmdHelp(CLEAR_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, GLOBAL_CLEAR_MQTT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, onCliGetMqtt);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "uri", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "port", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "clientid", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "qos", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "keepalive", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "ping", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "maxlatency", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "meanlatency", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "overruns", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "opstate", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, "subscriptions", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, GLOBAL_GET_MQTT_HELP_TXT);
	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, onCliShowMqtt);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, MQTT_SUB_MO_NAME, GLOBAL_SHOW_MQTT_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: time																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	regCmdMoArg(ADD_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliAddTime);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpserver", 1, true);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpport", 1, true);
	regCmdHelp(ADD_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_ADD_TIME_HELP_TXT);

	regCmdMoArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliDeleteTime);
	regCmdFlagArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpserver", 1, true);
	regCmdHelp(DELETE_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_DELETE_TIME_HELP_TXT);

	regCmdMoArg(START_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliStartTime);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpclient", 1, false);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpdhcp", 1, false);
	regCmdHelp(START_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_START_TIME_HELP_TXT);

	regCmdMoArg(STOP_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliStopTime);
	regCmdFlagArg(STOP_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpclient", 1, false);
	regCmdHelp(STOP_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_STOP_TIME_HELP_TXT);

	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliSetTime);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "timeofday", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "tod", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "epochtime", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "timezone", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "daylightsaving", 1, true);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_SET_TIME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliGetTime);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "timeofday", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "tod", 1, false);							//Synonym for timeofday
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "utc", 2, false);							//Looking for UTC time
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "epochtime", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "timezone", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "daylightsaving", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpdhcp", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpservers", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpsyncstatus", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpsyncmode", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, "ntpopstate", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_GET_TIME_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, onCliShowTime);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, TIME_SUB_MO_NAME, GLOBAL_SHOW_TIME_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: log																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
//Common Log SubMo - needs work
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliSetLogHelper);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "loglevel", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logmo", 1, true); //which MO the order refers to, not yet implemented, if not specified global log properties are assuned
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, true); // Not yet implemented
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_SET_LOG_HELP_TXT);

	regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliUnSetLogHelper);
	regCmdFlagArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, false); // Not yet implemented
	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_UNSET_LOG_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliGetLogHelper);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "loglevel", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logmo", 1, false); //which MO the order refers to, not yet implemented, if not specified global log properties are assuned
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "tail", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "continous", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, false); // Not yet implemented
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_GET_LOG_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliShowLogHelper);
	regCmdFlagArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "tail", 1, true); // Not yet implemented
	regCmdFlagArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "continious", 1, true); // Not yet implemented
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_SHOW_LOG_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: debug																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	//Common Debug SubMo - needs work
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, onCliSetDebugHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, COMMON_SET_DEBUG_HELP_TXT);
	regCmdMoArg(UNSET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, onCliUnSetDebugHelper);
	regCmdHelp(UNSET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, COMMON_UNSET_DEBUG_HELP_TXT);
	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, onCliGetDebugHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, COMMON_GET_DEBUG_HELP_TXT);

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: failsafe																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, onCliSetFailsafeHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_SET_FAILSAFE_HELP_TXT);
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, onCliGetFailsafeHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_GET_FAILSAFE_HELP_TXT);
	//regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, onCliUnsetFailsafeHelper); Add
	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_UNSET_FAILSAFE_HELP_TXT);
}

void globalCli::regCommonCliMOCmds(void) {
	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, OPSTATE_SUB_MO_NAME, onCliGetOpStateHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, OPSTATE_SUB_MO_NAME, COMMON_GET_OPSTATE_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, onCliGetSysNameHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, COMMON_GET_SYSNAME_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, onCliSetSysNameHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, COMMON_SET_SYSNAME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, onCliGetUsrNameHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, COMMON_GET_USRNAME_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, onCliSetUsrNameHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, COMMON_SET_USRNAME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, onCliGetDescHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, COMMON_GET_DESCRIPTION_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, onCliSetDescHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, COMMON_SET_DESCRIPTION_HELP_TXT);
}

void globalCli::regContextCliMOCmds(void) {
	Log.INFO("globalCli::regContextCliMOCmds: No context unique" \
			 "MOs supported for CLI context" CR);
}

/* Global CLI decoration methods */
void globalCli::onCliHelp(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_dummy) {
	Command cmd(p_cmd);
	char helpCmd[50];
	char subMo[50];
	if (!cmd.getArg(0).getValue().c_str() ||
		!strcmp(cmd.getArg(0).getValue().c_str(), "")) {
		strcpy(helpCmd, cmd.getName().c_str());
		strcpy(subMo, "");
		Log.VERBOSE("globalCli::onCliHelp: Searching for help text for %s" CR, helpCmd);
	}
	else if (cmd.getArg(0).getValue().c_str() && (!cmd.getArg(1).getValue().c_str() || !strcmp(cmd.getArg(1).getValue().c_str(), ""))) {
		if (!strcmp(cmd.getArg(0).getValue().c_str(), "cli")) { //Exemption hack
			strcpy(helpCmd, "help");
			strcpy(subMo, "cli");
			Log.VERBOSE("globalCli::onCliHelp: Searching for help text for CLI" CR);
		}
		else {
			strcpy(helpCmd, cmd.getArg(0).getValue().c_str());
			strcpy(subMo, "");
			Log.VERBOSE("globalCli::onCliHelp: Searching for help text for %s" CR, helpCmd);
		}
	}
	else if (cmd.getArg(0).getValue().c_str() && cmd.getArg(1).getValue().c_str() && (!cmd.getArg(2).getValue().c_str() || !strcmp(cmd.getArg(2).getValue().c_str(),""))) {
		strcpy(helpCmd, cmd.getArg(0).getValue().c_str());
		strcpy(subMo, cmd.getArg(1).getValue().c_str());
		Log.VERBOSE("globalCli::onCliHelp: Searching for help text for %s %s" CR, helpCmd, subMo);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetContext: Bad number of arguments" CR);
		return;
	}
	char* helpStr = new(heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
	if (getHelpStr(helpStr, getCliTypeByName(helpCmd), NULL, subMo, true, true)) {
		printCliNoFormat(helpStr);
		acceptedCliCommand(CLI_TERM_QUIET);
		delete helpStr;
		return;
	}
	else {
		notAcceptedCliCommand(CLI_GEN_ERR, "No Help text available for %s %s", helpCmd, subMo);
		Log.INFO("globalCli::onCliHelp: No Help text available for %s %s" CR, helpCmd, subMo);
		delete helpStr;
		return;
	}
}

void globalCli::onCliGetContextHelper(cmd* p_cmd, cliCore* p_cliContext,
										cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	char fullCliContextPath[100];
	strcpy(fullCliContextPath, "");
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetContext: Bad number of arguments" CR);
		return;
	}
	else {
		static_cast<globalCli*>(p_cliContext)->onCliGetContext(fullCliContextPath);
		printCli("Current context: %s", fullCliContextPath);
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

rc_t globalCli::onCliGetContext(char* p_fullContextPath) {
	return getFullCliContextPath(p_fullContextPath);
}

void globalCli::onCliSetContextHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliSetContext(p_cmd);
}

void globalCli::onCliSetContext(cmd* p_cmd) {
	Command cmd(p_cmd);
	cliCore* targetContext;
	if (!cmd.getArgument(0) || !cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetContext: Bad number of arguments" CR);
	}
	else if (!(targetContext = getCliContextHandleByPath(cmd.getArg(1).getValue().			//TR LOW: WE SHOULD ONLY LOOK AT CHILD CANDIDATES RELATED TO THIS OBJECT, getCliContextHandleByPath IS STATIC AND GOES FROM CURRENT CONTEXT!
														 c_str()))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Context %s does not exist",
													  cmd.getArg(1).getValue().c_str());
		Log.VERBOSE("globalCli::onCliSetContext: Context %s does not exist" CR,
					cmd.getArg(1).getValue().c_str());
	}
	else {
		Log.VERBOSE("Setting context to: %s" CR, cmd.getArgument(1).getValue().c_str());
		setCurrentContext(targetContext);
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
}

void globalCli::onCliShowTopology(cmd* p_cmd, cliCore* p_cliContext,
								  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onClishowTopology: Bad number of arguments" CR);
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliShowTopology: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("childs"))
		static_cast<globalCli*>(p_cliContext)->printTopology();
	else
		static_cast<globalCli*>(getCliContextHandleByPath("/"))->printTopology();
	acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::printTopology(bool p_start) {
	uint8_t noOfChilds;
	{
		char contextPath[150];
		getFullCliContextPath(contextPath);
		char contextNameIndex[20];
		sprintf(contextNameIndex, "%s-%i", getCliContextDescriptor()->contextName, getCliContextDescriptor()->contextIndex);
		if (p_start)
			printCli("| %*s | %*s | %*s |", -50, "Context-path:", -20, "Context:",
				-20, "System-name");
		if(getCurrentContext() == this)
			printCli("| %*s | %*s | %*s | <<<",
				-50, contextPath,
				-20, contextNameIndex,
				-20, getCliContextDescriptor()->contextSysName ? getCliContextDescriptor()->contextSysName : "-");
		else
			printCli("| %*s | %*s | %*s |",
				-50, contextPath,
				-20, contextNameIndex,
				-20, getCliContextDescriptor()->contextSysName ? getCliContextDescriptor()->contextSysName : "-");
	}
	for (uint8_t i = 0; i < getChildContexts(NULL)->size(); i++) {
		static_cast<globalCli*>(getChildContexts(NULL)->at(i))->printTopology(false);
	}
}

void globalCli::onCliShowCommands(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(3)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onClishowCommands: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onClishowCommands: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	static_cast<globalCli*>(p_cliContext)->processAvailCommands(p_cmdTable->commandFlags->isPresent("all") ? true : false, p_cmdTable->commandFlags->isPresent("help")? true : false);
}
	
void globalCli::processAvailCommands(bool p_all, bool p_help) {
	QList<cliCmdTable_t*>* commandTable = getCliCommandTable();
	char flags[200];
	printCommand(NULL, NULL, NULL, NULL, NULL, true, p_all, p_help);
	char* helpStr = new (heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
	//First find all global MO sub-MOs
	for (uint16_t i = 0; i < commandTable->size(); i++) {
		if (!strcmp(commandTable->at(i)->mo, GLOBAL_MO_NAME)){
			getHelpStr(helpStr, commandTable->at(i)->cmdType, NULL, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "", false, false);
			printCommand(getCliNameByType(commandTable->at(i)->cmdType), commandTable->at(i)->mo, commandTable->at(i)->subMo? commandTable->at(i)->subMo : "-", commandTable->at(i)->commandFlags? commandTable->at(i)->commandFlags->getAllRegisteredStr(flags) : "", helpStr, false, p_all, p_help);
		}
	}
	//Second find common MO sub-MOs
	for (uint16_t i = 0; i < commandTable->size(); i++) {
		if (!strcmp(commandTable->at(i)->mo, COMMON_MO_NAME)){
			getHelpStr(helpStr, commandTable->at(i)->cmdType, NULL, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "", false, false);
			printCommand(getCliNameByType(commandTable->at(i)->cmdType), commandTable->at(i)->mo, commandTable->at(i)->subMo? commandTable->at(i)->subMo : "-", commandTable->at(i)->commandFlags? commandTable->at(i)->commandFlags->getAllRegisteredStr(flags) : "", helpStr, false, p_all , p_help);
		}
	}
	//Finally find uniqe MO sub-MOs
	for (uint16_t i = 0; i < commandTable->size(); i++) {
		if (p_all) {
			if (strcmp(commandTable->at(i)->mo, GLOBAL_MO_NAME) && strcmp(commandTable->at(i)->mo, COMMON_MO_NAME)) {
				getHelpStr(helpStr, commandTable->at(i)->cmdType, NULL, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "", false, false);
				printCommand(getCliNameByType(commandTable->at(i)->cmdType), commandTable->at(i)->mo, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "-", commandTable->at(i)->commandFlags ? commandTable->at(i)->commandFlags->getAllRegisteredStr(flags) : "", helpStr, false, p_all, p_help);
			}
		}
		else{
			if (!strcmp(commandTable->at(i)->mo, moType)) {
				getHelpStr(helpStr, commandTable->at(i)->cmdType, NULL, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "", false, false);
				printCommand(getCliNameByType(commandTable->at(i)->cmdType), commandTable->at(i)->mo, commandTable->at(i)->subMo ? commandTable->at(i)->subMo : "-", commandTable->at(i)->commandFlags ? commandTable->at(i)->commandFlags->getAllRegisteredStr(flags) : "", helpStr, false, p_all, p_help);
			}
		}
	}
	acceptedCliCommand(CLI_TERM_QUIET);
}
		
void globalCli::printCommand(const char* p_cmdType, const char* p_mo, const char* p_subMo, const char* p_flags, const char* p_helpStr, bool p_heading, bool p_all, bool p_showHelp) {
	if (p_heading) {
		if (p_showHelp)
			printCli("| %*s | %*s | %*s |", -60, "Command", -15, "MO", -60, "Help text:");
		else
			printCli("| %*s | %*s |", -60, "Command", -15, "MO");
		return;
	}
	char commandNFlags[61];
	strcpyTruncMaxLen(commandNFlags, p_cmdType, 60);
	strcatTruncMaxLen(commandNFlags, " ", 60);
	strcatTruncMaxLen(commandNFlags, p_subMo, 60);
	if (p_cmdType == getCliNameByType(SET_CLI_CMD)) {				//Have the argument syntax registered instead
		strcatTruncMaxLen(commandNFlags, " {value}", 60);
	}
	if (p_cmdType == getCliNameByType(HELP_CLI_CMD))				//Have the argument syntax registered instead
		strcatTruncMaxLen(commandNFlags, " [{command} [{sub-MO}]]", 60);
	strcatTruncMaxLen(commandNFlags, " ", 60);
	strcatTruncMaxLen(commandNFlags, p_flags, 60);
	char mo[16];
	strcpyTruncMaxLen(mo, p_mo, 15);
	if (!strcmp(p_mo, COMMON_MO_NAME) && !p_all) {
		strcatTruncMaxLen(mo, "-", 15);
		strcatTruncMaxLen(mo, moType, 15);
	}
	if (p_showHelp) {
		char help[61];
		strcpyTruncMaxLen(help, p_helpStr, 60);
		printCli("| %*s | %*s | %*s |", -60, commandNFlags, -15, mo, -60, help);
	}
	else
		printCli("| %*s | %*s |", -60, commandNFlags, -15, mo);
}

void globalCli::onCliReboot(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (cmd.getArgument(0))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("rebooting...");
		acceptedCliCommand(CLI_TERM_EXECUTED);
		panic("\"CLI reboot ordered\" - rebooting...");
	}
}

void globalCli::onCliGetUptime(cmd* p_cmd, cliCore* p_cliContext,
							   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("Uptime: %i seconds", (uint32_t)esp_timer_get_time()/1000000);
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

void globalCli::onCliStartCpu(cmd* p_cmd, cliCore* p_cliContext,
							  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliStartCpu: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliStartCpu: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliStartCpu: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("stats")) {
		if (cpu::getPm()) {
			notAcceptedCliCommand(CLI_GEN_ERR, "CPU statistics collection \
												is already active");
			Log.VERBOSE("globalCli::onCliStartCpu: Cannot start CPU statistics \
					     collection - already active" CR);
		}
		else {
			cpu::startPm();
			cmdHandled = true;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliStopCpu(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliStopPm: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliStopCpu: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliStopCpu: Bad number of arguments" CR);
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("stats"))) {
		if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_GEN_ERR, "CPU statistics collection \
												is already inactive");
			Log.VERBOSE("globalCli::onCliStopPm: Cannot stop CPU statistics collection \
						 - already inactive" CR);
			return;
		}
		else {
			cpu::stopPm();
			cmdHandled = true;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliGetCpu(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetCpu: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetCpu: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
	//Same as show cpu
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("tasks"))) {
		char* taskShowHeadingStr = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
		char* taskShowStr = new (heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
		Command cmd(p_cmd);
		rc_t rc;
		if (rc = cpu::getTaskInfoAllTxt(taskShowStr, taskShowHeadingStr)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not provide task information, \
												return code %i", rc);
			Log.WARN("globalCli::onCliGetCpu: Could not provide task information, \
					  return code %i" CR, rc);
			return;
		}
		else{
		printCli("%s\r\n%s", taskShowHeadingStr, taskShowStr);
		delete taskShowHeadingStr;
		delete taskShowStr;
		cmdHandled = true;
		}
	}
	if ((p_cmdTable->commandFlags->isPresent("task"))) {
		char* taskShowHeadingStr = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
		char* taskShowStr = new (heap_caps_malloc(sizeof(char[200]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[200];
		rc_t rc;
		if (rc = cpu::getTaskInfoAllByTaskTxt(cmd.getArgument(1).getValue().c_str(),
											  taskShowStr, taskShowHeadingStr)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not provide task information, \
								  return code %i", rc);
			Log.WARN("globalCli::onCliGetCpu: Could not provide task information, \
					  return code %i" CR, rc);
			return;
		}
		else {
			printCli("%s\n%s", taskShowHeadingStr, taskShowStr);
			delete taskShowHeadingStr;
			delete taskShowStr;
			cmdHandled = true;
		}
	}
	if ((p_cmdTable->commandFlags->isPresent("cpuusage"))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not implemented");
		Log.WARN("globalCli::onCliGetCpu: Not implemented" CR);
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("watermark"))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not implemented");
		Log.WARN("globalCli::onCliGetCpu: Not implemented" CR);
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("stats"))) {
		if (cpu::getPm())
			printCli("CPU statistics collection is active");
		else
			printCli("CPU statistics collection is inactive");
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowCpu(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliShowCpu: Bad number of arguments" CR);
		return;
	}
	notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not implemented");
	Log.WARN("globalCli::onCliShowCpu: Not implemented" CR);
}

void globalCli::onCliGetMem(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	heapInfo_t memInfo;
	bool internal = false;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetMem: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetMem: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		(p_cmdTable->commandFlags->getAllPresent()->size() == 1 && p_cmdTable->commandFlags->isPresent("internal"))) {							//WE NEED TO WORK ON THIS - SHOULD BE THE SAME AS show
		char* memShowHeadingStr = new (heap_caps_malloc(sizeof(char[300]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[300];
		char* memShowStr = new (heap_caps_malloc(sizeof(char[300]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[300];
		if (p_cmdTable->commandFlags->getAllPresent()->size() == 1 && p_cmdTable->commandFlags->isPresent("internal")) {
			internal = true;
			printCli("Internal memory statistics");
		}
		else
			printCli("Total memory statistics");
		cpu::getHeapMemTrendTxt(memShowStr, memShowHeadingStr, internal);
		printCli("%s\r\n%s", memShowHeadingStr, memShowStr);
		delete memShowHeadingStr;
		delete memShowStr;
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("internal"))
		internal = true;
	cpu::getHeapMemInfo(&memInfo, internal);
	if (p_cmdTable->commandFlags->isPresent("total")) {
		printCli("Total amount of memory: %i", memInfo.totalSize);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("used")) {
		printCli("Used memory: %i", memInfo.totalSize - memInfo.freeSize);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("available")) {
		printCli("Available memory: %i", memInfo.freeSize);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("watermark")) {
		printCli("Lowest ever available memory: %i", memInfo.highWatermark);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("average")) {
		if (atoi(p_cmdTable->commandFlags->isPresent("average")->getValue()) >=
			CPU_HISTORY_SIZE) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide average memory usage, given period " \
				"exceeds the maximum of %i seconds",
				CPU_HISTORY_SIZE - 1);
			Log.VERBOSE("globalCli::onCliGetMem: Could not provide average memory usage, " \
						"given period exceeds the maximum of %i seconds" CR,
						CPU_HISTORY_SIZE - 1);
			return;
		}
		else if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide average memory usage, performance monitoring not active, " \
				"use \"start cpu -stats\"");
			Log.VERBOSE("Could not provide average memory usage, performance monitoring not active" CR);
			return;
		}
		else{
			printCli("Average memory usage over a period of %i seconds: %i (out of: %i total)",
				atoi(p_cmdTable->commandFlags->isPresent("average")->getValue()),
				memInfo.totalSize - cpu::getAverageMemFreeTime(
					atoi(p_cmdTable->commandFlags->isPresent("average")->
						getValue()), internal), memInfo.totalSize);
			cmdHandled = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("trend")) {
		if (atoi(p_cmdTable->commandFlags->isPresent("trend")->getValue()) >=
			CPU_HISTORY_SIZE) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide memory usage trend, given period exceeds " \
				"the maximum %i seconds", CPU_HISTORY_SIZE - 1);
			Log.VERBOSE("globalCli::onCliGetMem: Could not provide memory usage trend, " \
						"given period exceeds the maximum of %i seconds" CR,
						CPU_HISTORY_SIZE - 1);
			return;
		}
		else if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide memory trend usage, performance monitoring not active, " \
				"use \"start cpu -stats\"");
			Log.VERBOSE("Could not provide memory trend usage, performance monitoring not active" CR);
			return;
		}
		else {
			printCli("Memory usage trend over a period of %i seconds: %i",
					atoi(p_cmdTable->commandFlags->isPresent("trend")->getValue()),
				cpu::getTrendMemFreeTime(0, internal) - cpu::getTrendMemFreeTime(atoi(p_cmdTable->commandFlags->
					isPresent("trend")->getValue()), internal));
			cmdHandled = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("maxblock")) {
		printCli("Max allocatable block-size is: %i", cpu::getMaxAllocMemBlockSize(internal));
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowMem(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliShowTasks: Bad number of arguments" CR);
		return;
	}
	else {
		char* memShowHeadingStr = new (heap_caps_malloc(sizeof(char[300]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[300];
		char* memShowStr = new (heap_caps_malloc(sizeof(char[300]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[300];
		cpu::getHeapMemTrendTxt(memShowStr, memShowHeadingStr);
		printCli("Total memory statistics");
		printCli("%s\r\n%s", memShowHeadingStr, memShowStr);
		delete memShowHeadingStr;
		delete memShowStr;
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

void globalCli::onCliStartMem(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool internalAlloc = false;
	bool externalAlloc = false;
	bool defaultAlloc = false;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliAddMem: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliAddMem: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("allocate")) {
		if (p_cmdTable->commandFlags->getAllPresent()->size() > 2) {
			char flags[100];
			p_cmdTable->commandFlags->getAllPresentStr(flags);
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed, incompatible permutation of flags: %s" CR, flags);
			Log.WARN("globalCli::onCliAddMem: Flag parsing failed, incompatible permutation of flags: %s" CR, flags);
			return;
		}
		if (testBuff) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Test buffer already allocated");
			Log.VERBOSE("globalCli::onCliAddMem: Test buffer already allocated" CR);
			return;
		}
		if (p_cmdTable->commandFlags->isPresent("internal"))
			internalAlloc = true;
		else if (p_cmdTable->commandFlags->isPresent("external"))
			externalAlloc = true;
		else if (p_cmdTable->commandFlags->isPresent("default"))
			defaultAlloc = true;
		else
			defaultAlloc = true;
		if (internalAlloc)
			testBuff = (char*)heap_caps_malloc(atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
		else if (externalAlloc)
			testBuff = (char*)heap_caps_malloc(atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
		else if(defaultAlloc)
			testBuff = (char*)heap_caps_malloc(atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()), MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT);
		if (!testBuff) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Failed to allocate %i bytes",
				atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()));
			Log.VERBOSE("globalCli::onCliGetMem: Failed to allocate %i bytes" CR,
				atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()));
			return;
		}
		else {
			printCli("Successfully allocated %i bytes to \"testBuffer\"",
				atoi(p_cmdTable->commandFlags->isPresent("allocate")->getValue()));
			cmdHandled = true;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliStopMem(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliDelMem: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliAddMem: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("allocate")) {
		if (p_cmdTable->commandFlags->getAllPresent()->size() > 1) {
			char flags[100];
			p_cmdTable->commandFlags->getAllPresentStr(flags);
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed, incompatible permutation of flags: %s", flags);
			Log.WARN("globalCli::onCliDelMem: Flag parsing failed, incompatible permutation of flags: %s" CR, flags);
			return;
		}
		else {
			if (!testBuff) {
				notAcceptedCliCommand(CLI_GEN_ERR, "\"testBuffer\" not previously allocated");
				Log.VERBOSE("globalCli::onCliGetMem: \"testBuffer\" not previously allocated" CR);
				return;
			}
			else {
				delete testBuff;
				testBuff = NULL;
				printCli("Successfully freed allocation of \"testBuffer\"");
				cmdHandled = true;
			}
		}
		if (cmdHandled)
			acceptedCliCommand(CLI_TERM_QUIET);
		else
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
	}
}

void globalCli::onCliSetNetwork(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	bool persist = false;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetNetwork: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, p_cmdTable->commandFlags->
			getParsErrs());
		Log.VERBOSE("globalCli::onCliSetNetwork: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetNetwork: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("persist")) {
		if (!(p_cmdTable->commandFlags->isPresent("hostname") ||
			p_cmdTable->commandFlags->isPresent("address") ||
			p_cmdTable->commandFlags->isPresent("mask") ||
			p_cmdTable->commandFlags->isPresent("gw") ||
			p_cmdTable->commandFlags->isPresent("dns"))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Can only persist Network \
								  Address-, Mask-, Gateway-, & DNS");
			Log.VERBOSE("globalCli::onCliSetNetwork: Can only persist MQTT URI and \
						MQTT Port" CR);
			return;
		}
		else{
			persist = true;
			cmdHandled = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("hostname")) {
		if (rc = networking::setHostname(p_cmdTable->commandFlags->
			isPresent("hostname")->getValue(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the Hostname, \
								  return code %i", rc);
			Log.WARN("globalCli::onCliSetNetwork: Could not set the Hostname, \
					  return code %i" CR, rc);
			return;
		}
		Log.INFO("globalCli::onCliSetNetwork: Hostname changed to %s" CR,
				 networking::getHostname());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("address")) {
		IPAddress ip;
		if (!ip.fromString(p_cmdTable->commandFlags->
			isPresent("address")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not a valid IP-address");
			Log.VERBOSE("globalCli::onCliSetNetwork: Not a valid IP-address" CR);
			return;
		}
		if (rc = networking::setStaticIpAddr(ip, networking::getIpMask(), 
											 networking::getGatewayIpAddr(),
											 networking::getDnsIpAddr(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the IP-Address, \
												return code %i", rc);
			Log.WARN("globalCli::onCliSetNetwork: Could not set the IP-Address, \
					  return code %i" CR, rc);
			return;
		}
		printCli("IP-address changed to % s", networking::getIpAddr().
				 toString().c_str());
		Log.INFO("globalCli::onCliSetNetwork: IP-address changed to %s" CR,
				 networking::getIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("mask")) {
		IPAddress mask;
		if (!mask.fromString(p_cmdTable->commandFlags->isPresent("mask")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not a valid IP-mask");
			Log.VERBOSE("globalCli::onCliSetNetwork: Not a valid IP-mask" CR);
			return;
		}
		if (rc = networking::setStaticIpAddr(networking::getIpAddr(), mask,
			networking::getGatewayIpAddr(),
			networking::getDnsIpAddr(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the IP-mask, \
								  return code %i", rc);
			Log.WARN("globalCli::onCliSetNetwork: Could not set the IP-mask, \
					 return code %i" CR, rc);
			return;
		}
		printCli("IP-mask changed to % s", networking::getIpMask().
				 toString().c_str());
		Log.INFO("globalCli::onCliSetNetwork: IP-mask changed to %s" CR,
				 networking::getIpMask().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("gw")) {
		IPAddress gw;
		if (!gw.fromString(p_cmdTable->commandFlags->isPresent("gw")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not a valid Gateway-address");
			Log.VERBOSE("globalCli::onCliSetNetwork: Not a valid Gateway-address" CR);
			return;
		}
		if (rc = networking::setStaticIpAddr(networking::getIpAddr(),
			networking::getIpMask(), gw,
			networking::getDnsIpAddr(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the Gateway-Address, \
												return code %i", rc);
			Log.WARN("globalCli::onCliSetNetwork: Could not set the Gateway-Address, \
					  return code %i" CR, rc);
			return;
		}
		printCli("Gateway IP address changed to % s",
				 networking::getGatewayIpAddr().toString().c_str());
		Log.INFO("globalCli::onCliSetNetwork: Gateway IP address changed to %s" CR,
				 networking::getGatewayIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("dns")){
			IPAddress dns;
		if (!dns.fromString(p_cmdTable->commandFlags->isPresent("dns")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not a valid DNS-address");
			Log.VERBOSE("globalCli::onCliSetNetwork: Not a valid DNS-address" CR);
			return;
		}
		if (rc = networking::setStaticIpAddr(networking::getIpAddr(),
											 networking::getIpMask(),
											 networking::getGatewayIpAddr(),
											 dns, persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the DNS-Address, \
												return code %i", rc);
			Log.WARN("globalCli::onCliSetNetwork: Could not set the DNS-Address, \
												return code %i" CR, rc);
			return;
		}
		printCli("DNS IP address changed to % s", networking::getDnsIpAddr().
				 toString().c_str());
		Log.INFO("globalCli::onCliSetNetwork: DNS IP address changed to %s" CR,
				 networking::getDnsIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliGetNetwork(cmd* p_cmd, cliCore* p_cliContext,
								cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetNetwork: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetNetwork: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		showNetwork();
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ssid")){
		printCli("SSID: %s", networking::getSsid());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("bssid")){
		printCli("BSSID: %s", networking::getBssid());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("channel")){
		printCli("Channel: %i", networking::getChannel());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("auth")){
		printCli("Encryption: %s", networking::getAuth());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("rssi")){
		printCli("RSSI: %i", networking::getRssi());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("mac")){
		printCli("MAC: %s", networking::getMac());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("hostname")){
		printCli("Hostname: %s", networking::getHostname());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("address")){
		printCli("IP address: %s", networking::getIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("mask")){
		printCli("Mask: %s", networking::getIpMask().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("gw")){
		printCli("Gateway: %s", networking::getGatewayIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("dns")){
		printCli("DNS: %s", networking::getDnsIpAddr().toString().c_str());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("opstate")) {
		char opState[30];
		printCli("Network Operational state: %s", networking::getOpStateStr(opState));
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("scanap")) {
		QList<apStruct_t*> aps;
		networking::getAps(&aps, false);
		char apsStr[200];
		sprintf(apsStr, "| %*s | %*s | %*s | %*s | %*s |",
						-30, "SSID:",
						-20, "BSSID:",
						-10, "Channel:",
						-10, "RSSI:",
						-20, "Encryption:");
		printCli(apsStr);
		for (uint8_t i = 0; i < aps.size(); i++) {
			sprintf(apsStr, "| %*s | %*s | %*i | %*i | %*s |", 
							-30, aps.at(i)->ssid,
							-20, aps.at(i)->bssid,
							-10, aps.at(i)->channel,
							-10, aps.at(i)->rssi,
							-20, aps.at(i)->encryption);
			printCli(apsStr);
		}
		networking::unGetAps(&aps);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowNetwork(cmd* p_cmd, cliCore* p_cliContext,
								 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliShowNetwork: Bad number of arguments" CR);
		return;
	}
	showNetwork();
}

void globalCli::showNetwork(void) {
	char ssid[20 + 1];																		//It apears that sprintf is multi-threaded, networking is not thread safe.
	strcpyTruncMaxLen(ssid, networking::getSsid(), 20);
	char bssid[18 + 1];
	strcpyTruncMaxLen(bssid, networking::getBssid(), 18);
	uint8_t channel = networking::getChannel();
	char auth[11 + 1];
	strcpyTruncMaxLen(auth, networking::getAuth(), 11);
	int rssi = networking::getRssi();
	char mac[18 + 1];
	strcpyTruncMaxLen(mac, networking::getMac(), 18);
	char hostname[35 + 1];
	strcpyTruncMaxLen(hostname, networking::getHostname(), 35);
	char ipaddr[16 + 1];
	strcpyTruncMaxLen(ipaddr, networking::getIpAddr().toString().c_str(), 35);
	char mask[16 + 1];
	strcpyTruncMaxLen(mask, networking::getIpMask().toString().c_str(), 16);
	char gw[16 + 1];
	strcpyTruncMaxLen(gw, networking::getGatewayIpAddr().toString().c_str(), 16);
	char dns[16 + 1];
	strcpyTruncMaxLen(dns, networking::getDnsIpAddr().toString().c_str(), 16);
	uint8_t ch = networking::getChannel();
	printCli("| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |", 
			-20, "SSID:",
			-18, "BSSID:",
			-8, "Channel:",
			-11, "Encryption:",
			-7, "RSSI:",
			-18, "MAC:",
			-30, "Host-name:",
			-16, "IP-Address:",
			-16, "IP-Mask",
			-16, "Gateway:",
			-16, "DNS");
	printCli("| %*s | %*s | %*i | %*s | %*i | %*s | %*s | %*s | %*s | %*s | %*s |",
			 -20, ssid,
			 -18, bssid,
			 -8, channel,
			 -11, auth,
			 -7, rssi,
			 -18, mac,
			 -30, hostname,
			 -16, ipaddr,
			 -16, mask,
			 -16, gw,
			 -16, dns);
	acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::onCliSetMqtt(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool persist = false;
	rc_t rc;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliSetMqttHelper: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliSetMqtt: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.VERBOSE("globalCli::onCliSetMqtt: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("persist")) {
		if (!(p_cmdTable->commandFlags->isPresent("uri") ||
			p_cmdTable->commandFlags->isPresent("port"))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "Can only persist MQTT URI and MQTT Port");
			Log.VERBOSE("globalCli::onCliSetMqtt: \
						 Can only persist MQTT URI and MQTT Port" CR);
			return;
		}
		else{
			persist = true;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("uri")) {
		if (rc = mqtt::setBrokerUri(p_cmdTable->commandFlags->
			isPresent("uri")->getValue(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT URI, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("port")) {
		if (rc = mqtt::setBrokerPort(atoi(p_cmdTable->commandFlags->
			isPresent("port")->getValue()), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Port, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("clientid")) {
		if (rc = mqtt::setClientId(p_cmdTable->commandFlags->
			isPresent("clientid")->getValue())) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Client Id, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("qos")) {
		if (rc = mqtt::setDefaultQoS(atoi(p_cmdTable->commandFlags->
			isPresent("qos")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Client Id, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("keepalive")) {
		if (rc = mqtt::setKeepAlive(atof(p_cmdTable->commandFlags->
			isPresent("keepalive")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT KeepAlive, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ping")) {
		if (rc = mqtt::setPingPeriod(atof(p_cmdTable->commandFlags->
			isPresent("ping")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Ping period, \
												return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliClearMqtt(cmd* p_cmd, cliCore* p_cliContext,
							   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliClearMqtt: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliClearMqtt: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliClearMqtt: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("maxlatency"))
		mqtt::clearMaxLatency();
	if (p_cmdTable->commandFlags->isPresent("overruns"))
		mqtt::clearOverRuns();
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliGetMqtt(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetMqtt: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetMqtt: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		showMqtt();
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("uri")) {
		if(mqtt::getBrokerUri())
			printCli("MQTT URI: %s", mqtt::getBrokerUri());
		else
			printCli("MQTT URI: -");
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("port")) {
		printCli("MQTT Port: %i", mqtt::getBrokerPort());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("clientid")){
		if(mqtt::getClientId())
			printCli("MQTT Client Id: %s", mqtt::getClientId());
		else
			printCli("MQTT Client Id: -");
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("qos")) {
		printCli("MQTT QoS: %i", mqtt::getDefaultQoS());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("keepalive")) {
		printCli("MQTT Keep-alive: %.2f", mqtt::getKeepAlive());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ping")) {
		printCli("MQTT Ping: %.2f", mqtt::getPingPeriod());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("maxlatency")){
		printCli("MQTT Max latency: %i", mqtt::getMaxLatency());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("meanlatency")) {
		printCli("MQTT Mean Latency: %i", mqtt::getMeanLatency());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("overruns")) {
		printCli("MQTT Overruns: %i", mqtt::getOverRuns());
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("opstate")) {
		char opState[100];
		mqtt::getOpStateStr(opState);
		printCli("MQTT Operational state: %s", opState);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("subscriptions")) {
		char cbInfo[200];
		sprintf(cbInfo, "| %*s | %*s |",
			-100, "Topic:",
			-50, "Callbacks:");
		printCli("%s", cbInfo);
		char topic[100];
		char cbs[50];
		uint16_t i = 0;
		while (mqtt::getSubs(i++, topic, 100, cbs, 50)) {
			sprintf(cbInfo, "| %*s | %*s |",
				-100, topic,
				-50, cbs);
			printCli("%s", cbInfo);
		}
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowMqtt(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliShowMqtt: Bad number of arguments" CR);
		return;
	}
	showMqtt();
	acceptedCliCommand(CLI_TERM_QUIET);
	return;
}

void globalCli::showMqtt(void) {
	char uri[20 + 1];
	if (mqtt::getBrokerUri())
		strcpyTruncMaxLen(uri, mqtt::getBrokerUri(), 20);
	else
		strcpy(uri, "-");
	uint16_t port = mqtt::getBrokerPort();
	char clientId[30 + 1];
	if (mqtt::getClientId())
		strcpyTruncMaxLen(clientId, mqtt::getClientId(), 30);
	else
		strcpy(clientId, "-");
	uint8_t qos = mqtt::getDefaultQoS();
	float keepAlive = mqtt::getKeepAlive();
	float ping = mqtt::getPingPeriod();
	uint32_t maxLatency = mqtt::getMaxLatency();
	uint32_t meanLatency = mqtt::getMeanLatency();
	uint32_t overRuns = mqtt::getOverRuns();
	char opState[100];
	mqtt::getOpStateStr(opState);
	char mqttInfo[200];
	sprintf(mqttInfo, "| %*s | %*s | %*s | %*s | %*s | %*s | " \
		              "%*s | %*s | %*s | %*s |",
		-20, "URI:",
		-5, "Port:",
		-30, "Client-Id:",
		-6, "QoS:",
		-13, "Keep-alive:",
		-7, "Ping:",
		-12, "Max latency:",
		-13, "Mean latency:",
		-9, "Overruns:",
		-40, "OP-State");
	printCli("%s", mqttInfo);
	sprintf(mqttInfo, "| %*s | %*i | %*s | %*i | %*.2f | %*.2f | " \
		"%*i | %*i | %*i | %*s |",
		-20, uri,
		-5, port,
		-30, clientId,
		-6, qos,
		-13, keepAlive,
		-7, ping,
		-12, maxLatency,
		-13, meanLatency,
		-9, overRuns,
		-40, opState);
	printCli("%s", mqttInfo);

}
void globalCli::onCliAddTime(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	IPAddress ipAddr;
	uint16_t ipPort;
	rc_t rc;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliAddTime: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliAddTime: Flag parsing failed: %s" CR, 
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		!p_cmdTable->commandFlags->isPresent("ntpserver")->getValue()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
							  mandatory flags missing");
		Log.WARN("globalCli::onCliAddTime: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpserver")) {
		Log.INFO("globalCli::onCliAddTime: Adding NTP server" CR);
		if (ipAddr.fromString(p_cmdTable->commandFlags->
			isPresent("ntpserver")->getValue())) {
			if (p_cmdTable->commandFlags->isPresent("ntpport") &&
				(ipPort = atoi(p_cmdTable->commandFlags->
					isPresent("ntpport")->getValue()))) {
				if (rc = ntpTime::addNtpServer(ipAddr, ipPort)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - \
														return code: %i", rc);
					Log.WARN("globalCli::onCliAddTime: Could not add NTP server - \
							  return code: %i" CR, rc);
					return;
				}
			}
			else{
				if (rc = ntpTime::addNtpServer(ipAddr)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - \
										  return code: %i", rc);
					Log.WARN("globalCli::onCliAddTime: Could not add NTP server - \
							  return code: %i" CR, rc);
					return;
				}
			}
		}
		else if(isUri(p_cmdTable->commandFlags->isPresent("ntpserver")->getValue())){
			if (p_cmdTable->commandFlags->isPresent("ntpport") &&
				(ipPort = atoi(p_cmdTable->commandFlags->
					isPresent("ntpport")->getValue()))){
				if (rc = ntpTime::addNtpServer(p_cmdTable->commandFlags->
					isPresent("ntpserver")->getValue(), ipPort)) {
						notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - \
															return code: %i", rc);
						Log.WARN("globalCli::onCliAddTime: Could not add NTP server - \
								  return code: %i" CR, rc);
						return;
				}
			}
			else {
				if (rc = ntpTime::addNtpServer(p_cmdTable->commandFlags->
					isPresent("ntpserver")->getValue())) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - \
														return code: %i", rc);
					Log.WARN("globalCli::onCliAddTime: Could not add NTP server - \
							  return code: %i" CR, rc);
					return;
				}
			}
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "%s is not a valid IP address or URI",
								  p_cmdTable->commandFlags->
										isPresent("ntpserver")->getValue());
			Log.WARN("globalCli::onCliAddTime: %s is not a valid IP address or URI" CR,
					 p_cmdTable->commandFlags->isPresent("ntpserver")->getValue());
			return;
		}
		Log.VERBOSE("globalCli::onCliAddTime: NTP server successfully added" CR);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliDeleteTime(cmd* p_cmd, cliCore* p_cliContext,
								cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	IPAddress ipAddr;
	rc_t rc;
	bool cmdHandled = false;

	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliDeleteTime:: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliDeleteTime:: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		!p_cmdTable->commandFlags->isPresent("ntpserver")->getValue()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
													  mandatory flags missing");
		Log.WARN("globalCli::onCliDeleteTime:: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpserver")) {
		Log.INFO("globalCli::onCliDeleteTime: Deleting NTP server" CR);
		if (ipAddr.fromString(p_cmdTable->commandFlags->
			isPresent("ntpserver")->getValue())) {
			if (rc = ntpTime::deleteNtpServer(ipAddr)) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not delete NTP server - \
													return code: %i", rc);
				Log.WARN("globalCli::onCliDeleteTime: Could not delete NTP server - \
						  return code: %i" CR, rc);
				return;
			}
		}
		else if (isUri(p_cmdTable->commandFlags->isPresent("ntpserver")->getValue())) {
			if (rc = ntpTime::deleteNtpServer(p_cmdTable->commandFlags->
				isPresent("ntpserver")->getValue())) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not delete NTP server - \
													return code: %i", rc);
				Log.WARN("globalCli::onCliDeleteTime: Could not delete NTP server - \
						  return code: %i" CR, rc);
				return;
			}
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "%s is not a valid IP address or URI",
								  p_cmdTable->commandFlags->
										isPresent("ntpserver")->getValue());
			Log.WARN("globalCli::onCliDeleteTime: %s is not a valid IP address \
					  or URI" CR,
					 p_cmdTable->commandFlags->isPresent("ntpserver")->getValue());
			return;
		}
		Log.VERBOSE("globalCli::onCliDeleteTime: NTP server successfully deleted" CR);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliStartTime(cmd* p_cmd, cliCore* p_cliContext,
							   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	ntpOpState_t ntpOpState;
	rc_t rc;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliStartTime: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliStartTime: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
													  mandatory flags missing");
		Log.WARN("globalCli::onCliStartTime: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpclient")) {
		Log.INFO("globalCli::onCliStartTime: Starting NTP server" CR);
		ntpTime::getNtpOpState(&ntpOpState);
		if (ntpOpState & NTP_CLIENT_DISABLED) {
			if(p_cmdTable->commandFlags->isPresent("ntpdhcp")){
				if (rc = ntpTime::start(true)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not start NTP-Client - \
														return code: %i", rc);
					Log.WARN("globalCli::onCliStartTime: Could not start NTP-Client - \
							  return code: %i" CR, rc);
					return;
				}
			}
			else {
				if (rc = ntpTime::start(false)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not start NTP-Client - \
														return code: %i", rc);
					Log.WARN("globalCli:onCliStartTime: Could not start NTP-Client - \
							  return code: %i" CR, rc);
					return;
				}
			}
		}
		else{
			notAcceptedCliCommand(CLI_GEN_ERR, "NTP client is already running");
			Log.WARN("globalCli:onCliStartTime: NTP client is already running" CR);
			return;
		}
		Log.VERBOSE("globalCli:onCliStartTime:: NTP server successfully started" CR);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliStopTime(cmd* p_cmd, cliCore* p_cliContext,
							  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	ntpOpState_t ntpOpState;
	rc_t rc;
	bool cmdHandled = false;

	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliStopTime: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliStopTime: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
													  mandatory flags missing");
		Log.WARN("globalCli::onCliStopTime: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpclient")) {
		Log.INFO("globalCli::onCliStopTime: Stoping NTP server" CR);
		ntpTime::getNtpOpState(&ntpOpState);
		if (!(ntpOpState & NTP_CLIENT_DISABLED)) {
			if (rc = ntpTime::stop()) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not stop NTP-Client - \
													return code: %i", rc);
				Log.WARN("globalCli::onCliStopTime: Could not stop NTP-Client - \
													return code: %i" CR, rc);
				return;
			}
		}
		else {
			notAcceptedCliCommand(CLI_GEN_ERR, "NTP client is not running");
			Log.WARN("globalCli::onCliStopTime: NTP client is not running" CR);
			return;
		}
		Log.VERBOSE("globalCli::onCliStopTime:: NTP server successfully stopped" CR);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliSetTime(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	char response[300];
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliSetTime: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliSetTime: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
													  mandatory flags missing");
		Log.WARN("globalCli::onCliSetTime: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("timeofday") ||
		p_cmdTable->commandFlags->isPresent("tod")) {
		Log.INFO("globalCli::onCliSetTime: Setting time of day" CR);
		if (p_cmdTable->commandFlags->isPresent("timeofday")){
			if (ntpTime::setTimeOfDay(p_cmdTable->commandFlags->
				isPresent("timeofday")->getValue(), response)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
				Log.WARN("globalCli::onCliSetTime: %s" CR, response);
				return;
			}
		}
		else {
			if (ntpTime::setTimeOfDay(p_cmdTable->commandFlags->
				isPresent("tod")->getValue(), response)) {
				notAcceptedCliCommand(CLI_GEN_ERR, "%s", response);
				Log.WARN("globalCli::onCliSetTime: %s" CR, response);
				return;
			}		
		}
		Log.VERBOSE("globalCli::onCliSetTime: Successfully set time of day" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("epochtime")) {
		Log.INFO("globalCli::onCliSetTime: Setting epoch time" CR);
		if (ntpTime::setEpochTime(p_cmdTable->commandFlags->
			isPresent("epochtime")->getValue(), response)) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
			Log.WARN("globalCli::onCliSetTime: %s" CR, response);
			return;
		}
		Log.VERBOSE("globalCli::onCliSetTime: Successfully set epoch time" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("timezone")) {
		Log.INFO("globalCli::onCliSetTime: Setting time zone" CR);
		if (ntpTime::setTz(p_cmdTable->commandFlags->
			isPresent("timezone")->getValue(), response)) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
			Log.WARN("globalCli::onCliSetTime: %s" CR, response);
			return;
		}
		Log.VERBOSE("globalCli::onCliSetTime: Successfully set time zone" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("daylightsaving")) {
		Log.INFO("globalCli::onCliSetTime: Setting dayligh saving" CR);
		if (!strcmp(p_cmdTable->commandFlags->isPresent("daylightsaving")->getValue(), "true")) {
			ntpTime::setDayLightSaving(true);
			Log.VERBOSE("globalCli::onCliSetTime: Successfully set time zone to \"true\"" CR);
			cmdHandled = true;
		}
		else if (!strcmp(p_cmdTable->commandFlags->isPresent("daylightsaving")->getValue(), "false")) {
			ntpTime::setDayLightSaving(false);
			Log.VERBOSE("globalCli::onCliSetTime: Successfully set time zone to \"false\"" CR);
			cmdHandled = true;
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s is not a value daylightsaving flag value, valid values are \"true\" or \false\"", response);
			Log.WARN("globalCli::onCliSetTime: %s" CR, response);
			return;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliGetTime(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	bool cmdHandled = true;
	char outputBuff[300];

	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetTime: Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetTime: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		showTime();
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("timeofday") ||
		p_cmdTable->commandFlags->isPresent("tod")) {
		if (p_cmdTable->commandFlags->isPresent("utc")) {
			ntpTime::getTimeOfDay(outputBuff, true);
			printCli("Universal time: %s", outputBuff);
		}
		else {
		ntpTime::getTimeOfDay(outputBuff);
		printCli("Local time: %s", outputBuff);
		}
		cmdHandled = true;
	}
	else if(p_cmdTable->commandFlags->isPresent("utc")) {
		ntpTime::getTimeOfDay(outputBuff, true);
		printCli("Universal time: %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("epochtime")) {
		ntpTime::getEpochTime(outputBuff);
		printCli("Epoch time: %s", outputBuff);
	}
	if (p_cmdTable->commandFlags->isPresent("timezone")) {
		ntpTime::getTz(outputBuff);
		printCli("Current time-zone: %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("daylightsaving")) {
		bool dls;
		ntpTime::getDayLightSaving(&dls);
		if (dls)
			sprintf(outputBuff, "true");
		else
			sprintf(outputBuff, "false");
		printCli("Daylight saving is %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpdhcp")) {
		if (ntpTime::getNtpDhcp())
			sprintf(outputBuff, "True");
		else
			sprintf(outputBuff, "False");
		printCli("NTP server is provided by DHCP: %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpservers")) {
		QList<ntpServerHost_t*>* ntpServers;
		if (ntpTime::getNtpServers(&ntpServers)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not get NTP servers");
			Log.ERROR("globalCli::onCliGetTime: Could not get NTP servers");
			return;
		}
		else {
			sprintf(outputBuff, "| %*s | %*s | %*s | %*s | %*s | %*s |",
								-6, "Index:",
								- 30, "NTP Server URI:",
								-22, "NTP Server IP Address:",
								-5, "Port:",
								-8, "Stratum:",
								-13, "Reachability:");
			printCli(outputBuff);
			for (uint8_t i = 0; i < ntpServers->size(); i++) {
				char stratum[10];
				if (!ntpServers->at(i)->stratum)
					strcpy(stratum, "Unknown");
				else
					strcpy(stratum, itoa(ntpServers->at(i)->stratum, stratum, 10));
				sprintf(outputBuff, "| %*i | %*s | %*s | %*i | %*s | 0x%*X |",
									-6, ntpServers->at(i)->index,
									-30, ntpServers->at(i)->ntpHostName,
									-22, ntpServers->at(i)->
											ntpHostAddress.toString().c_str(),
									-5, ntpServers->at(i)->ntpPort,
									-8, stratum,
									-11, ntpTime::getReachability(i));
				printCli(outputBuff);
			}
			cmdHandled = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("ntpsyncstatus")) {
		char syncStatusStr[50];
		ntpTime::getNtpSyncStatusStr(syncStatusStr);
		sprintf(outputBuff, "%s", syncStatusStr);
		printCli("NTP sync status: %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpsyncmode")) {
		char syncModeStr[50];
		ntpTime::getSyncModeStr(syncModeStr);
		sprintf(outputBuff, "%s", syncModeStr);
		printCli("NTP sync mode: %s", outputBuff);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpopstate")) {
		char opStateStr[50];
		ntpTime::getNtpOpStateStr(opStateStr);
		sprintf(outputBuff, "%s", opStateStr);
		printCli("NTP Operational state: %s", outputBuff);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowTime(cmd* p_cmd, cliCore* p_cliContext,
							  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	QList<ntpServerHost_t*>* ntpServers;
	char outputBuff[300];
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliShowTime: Bad number of arguments" CR);
		return;
	}
	showTime();
}

void globalCli::showTime(void) {
	rc_t rc;
	QList<ntpServerHost_t*>* ntpServers;
	char timeOfDayUTC[50];
	char timeOfDayLocal[50];
	timeval tv;
	char timeOfDayEpochTimeStr[15];
	char opState[30];
	char syncState[30];
	char syncMode[30];
	char tz[20];
	bool daylightSaving;
	uint8_t noOfNtpServers;
	char daylightSavingStr[10];
	ntpTime::getTimeOfDay(timeOfDayUTC, true);
	ntpTime::getTimeOfDay(timeOfDayLocal);
	ntpTime::getEpochTime(&tv);
	strcpy(timeOfDayEpochTimeStr, itoa(tv.tv_sec, timeOfDayEpochTimeStr, 10));
	printCli("Time:");
	printCli("| %*s | %*s | %*s |",
			 -40, "UTC:",
			 -40, "Local:",
			 -15, "Epoch time:");
	printCli("| %*s | %*s | %*s |",
			 -40, timeOfDayUTC,
			 -40, timeOfDayLocal,
			 -15, timeOfDayEpochTimeStr);
	ntpTime::getNtpOpStateStr(opState);
	ntpTime::getNtpSyncStatusStr(&syncState[0]);
	ntpTime::getSyncModeStr(syncMode);
	ntpTime::getTz(tz);
	ntpTime::getDayLightSaving(&daylightSaving);
	ntpTime::getNoOfNtpServers(&noOfNtpServers);
	if (daylightSaving)
		strcpy(daylightSavingStr, "True");
	else
		strcpy(daylightSavingStr, "False");
	printCli("\n\rNTP-Client:");
	printCli("| %*s | %*s | %*s | %*s | %*s | %*s |",
			 -30, "OpState:",
			 -30, "Sync status:",
			 -25, "Sync mode",
			 -14, "No of servers:",
			 -20, "Time zone:",
			 -16, "Daylight saving:");
	printCli("| %*s | %*s | %*s | %*i | %*s | %*s |",
			-30, opState,
			-30, syncState,
			-25, syncMode,
			-14, noOfNtpServers,
			-20, tz,
			-16, daylightSavingStr);
	if (ntpTime::getNtpServers(&ntpServers)) {
		notAcceptedCliCommand(CLI_GEN_ERR, "Could not get NTP servers");
		Log.ERROR("globalCli::onCliGetTime: Could not get NTP servers");
		return;
	}
	printCli("\n\rNTP-Servers:");
	printCli("| %*s | %*s | %*s | %*s | %*s | %*s |",
			-6, "Index:",
			-30, "NTP Server URI:",
			-22, "NTP Server IP Address:",
			-5, "Port:",
			-8, "Stratum:",
			-12, "Reachability:");
	for (uint8_t i = 0; i < ntpServers->size(); i++) {
		char stratum[10];
		if (!ntpServers->at(i)->stratum)
			strcpy(stratum, "Unknown");
		else
			strcpy(stratum, itoa(ntpServers->at(i)->stratum, stratum, 10));
		printCli("| %*i | %*s | %*s | %*i | %*s | 0x%*X |",
				 -6, ntpServers->at(i)->index,
				 -30, ntpServers->at(i)->ntpHostName,
				 -22, ntpServers->at(i)->ntpHostAddress.toString().c_str(),
				 -5, ntpServers->at(i)->ntpPort,
				 -8, stratum,
				 -11,ntpTime::getReachability(i));
	}
	acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::onCliSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,
								  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0) || !cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliSetLogHelper: Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliSetLogHelper: Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, 
							  "Bad number of arguments - mandatory flags missing");
		Log.WARN("globalCli::onCliSetLogHelper: \
				  Bad number of arguments - mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("loglevel")) {
		if (rc = static_cast<globalCli*>(rootHandle)->
			setLogLevel(p_cmdTable->commandFlags->
				isPresent("loglevel")->getValue())) {
			if (rc == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level \
				not accepted, debug flag not set");
				Log.WARN("globalCli::onCliSetLoglevelHelper: Setting of Log-level \
						  not accepted, debug flag not set" CR);
				return;
			}
			else {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level unsuccessfull, \
													unknown error - return code: %i", rc);
				Log.WARN("globalCli::onCliSetLoglevelHelper: Setting of Log-level \
						  unsuccessfull, unknown error - return code: %i" CR, rc);
				return;
			}
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("logmo")) {
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level for MO/SubMo \
											not accepted, not implemented", rc);
		Log.WARN("globalCli::onCliSetLogHelper: Setting of Log-level for MO/SubMo \
				  not accepted, not implemented" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of log-destination not accepted, \
											not implemented", rc);
		Log.WARN("globalCli::onCliSetLogHelper: Setting of log-destination \
				  not accepted, not implemented" CR);
		return;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

rc_t globalCli::setLogLevel(const char* p_logLevel, bool p_force){
	return RC_NOTIMPLEMENTED_ERR;
}


void globalCli::onCliUnSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0) || !cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliUnSetLogHelper: Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, 
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliUnSetLogHelper: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - \
													  mandatory flags missing");
		Log.WARN("globalCli::onCliUnSetLogHelper: Bad number of arguments - \
				  mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		Log.WARN("globalCli::onCliUnSetLogHelper: Un-Setting of \
				  log-destination not accepted, not implemented" CR);
		return;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliGetLogHelper(cmd* p_cmd, cliCore* p_cliContext,
								  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliGetLoglevelHelper: Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		Log.VERBOSE("globalCli::onCliGetLogHelper: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		onCliShowLog();
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("loglevel")) {
		if (p_cmdTable->commandFlags->isPresent("logmo")) {
			//Get logmo log preferences
			notAcceptedCliCommand(CLI_GEN_ERR, "Getting Log-level for MO/SubMo \
												not accepted, not implemented");
			Log.WARN("globalCli::onCliGetLogHelper: Getting Log-level for MO/SubMo \
					  not accepted, not implemented");
			return;
		}
		else {
			printCli("Log-level: %s",
					 static_cast<globalCli*>(rootHandle)->getLogLevel());
			cmdHandled = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		notAcceptedCliCommand(CLI_GEN_ERR, "Getting log-destination not accepted, \
											not implemented", rc);
		Log.WARN("globalCli::onCliSetLogHelper: Getting log-destination not accepted, \
				  not implemented");
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("tail")) {
		// Need to parse -tail n and -tail n -continous
		// If continous wee need to define a break sequence
		notAcceptedCliCommand(CLI_GEN_ERR, "Getting log -tail not accepted, \
											not implemented", rc);
		Log.WARN("globalCli::onCliSetLogHelper: Getting log -tail not accepted, \
				  not implemented");
		return;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

const char* globalCli::getLogLevel(void) {
	return NULL;
}

void globalCli::onCliShowLogHelper(cmd* p_cmd, cliCore* p_cliContext,
								   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	QList<cliCore*>* contexts;
	if (!cmd.getArgument(0) && cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		Log.WARN("globalCli::onCliShowLogHelper: Bad number of arguments");
		return;
	}
	onCliShowLog();
	acceptedCliCommand(CLI_TERM_QUIET);
	return;
}

void globalCli::onCliShowLog(void) {
	QList<cliCore*>* contexts;
	printCli("Log receiver: %s", "-");
	printCli("| %*s | %*s | %*s |",
			 -30, "Context:",
			 -15, "Debug-state:",
			 -15, "Log-level:");
	contexts = (QList<cliCore*>*)getAllContexts();
	for (uint16_t i = 0; i < contexts->size(); i++) {
		char contextStr[30];
		sprintf(contextStr, "%s-%i", contexts->at(i)->getContextName(),
				contexts->at(i)->getContextIndex());
		char debugStr[15];
		if (((globalCli*)contexts->at(i))->getDebug())
			strcpy(debugStr, "On");
		else
			strcpy(debugStr, "Off");

		printCli("| %*s | %*s | %*s |",
				 -30, contextStr,
				 -15, debugStr,
				 -15, ((globalCli*)contexts->at(i))->getLogLevel());
	}
}

void globalCli::onCliSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,
									   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	if (rc = rootHandle->setFailSafe(true))
		notAcceptedCliCommand(CLI_GEN_ERR, "Activating fail-safe not accepted, \
											unknown error - return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setFailSafe(const bool p_failsafe, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliUnSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	if (rc = rootHandle->setFailSafe(false))
		notAcceptedCliCommand(CLI_GEN_ERR, "In-activating fail-safe not accepted, \
											return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::unSetFailSafe(const bool p_failsafe, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliGetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext,
									   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	if (rootHandle->getFailSafe())
		printCli("Failsafe: Active");
	else
		printCli("Failsafe: Inactive");
	acceptedCliCommand(CLI_TERM_QUIET);
}

bool globalCli::getFailSafe(void){
	return false;
}

/* Common CLI decoration methods */
void globalCli::onCliSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->setDebug(true);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliUnSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->setDebug(false);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::setDebug(bool p_debug) {
}

void globalCli::onCliGetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	if (static_cast<globalCli*>(p_cliContext)->getDebug())
		printCli("Debug flag for context %s-%i is set",
				 p_cliContext->getContextName(),
				 p_cliContext->getContextIndex());
	else
		printCli("Debug flag for context %s-%i is not set",
				 p_cliContext->getContextName(),
				 p_cliContext->getContextIndex());
	acceptedCliCommand(CLI_TERM_QUIET);
}

bool globalCli::getDebug(void) {
	Log.warning("globalCli::getDebug: debug flag not implemented");
	return false;
}

void globalCli::onCliGetOpStateHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliGetOpState(p_cmd);
}

void globalCli::onCliGetOpState(cmd* p_cmd) {
	Command cmd(p_cmd);
	char opStateStr[100];
	strcpy(opStateStr, "");
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = getOpStateStr(opStateStr)) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR? "opStateStr not implemented for context %s-%i" : "Unknown error: could not get Opstate for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	printCli("OP-state: %s", opStateStr);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getOpStateStr(char* p_opStateStr){
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliGetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliGetSysName(p_cmd);
}

void globalCli::onCliGetSysName(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	char sysName[50];
	rc_t rc;
	if (rc = getSystemName(sysName)) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR? "getSystemName not implemented for context %s-%i" : rc == RC_NOT_CONFIGURED_ERR? "Could not get Systemname for context %s-%i, MO not configured" : "Unknown error: could not get Systemname for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	printCli("System name: %s", sysName);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getSystemName(char* p_systemName, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliSetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliSetSysName(p_cmd);
}
void globalCli::onCliSetSysName(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = setSystemName(cmd.getArgument(1).getValue().c_str())) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR? "setSystemName not implemented for context %s-%i" : rc == RC_DEBUG_NOT_SET_ERR? "Systemname could not be set for context %s-%i as debug is not set for this context - enable it by typing \"set debug\"" : "Unknown error: could not set Systemname for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setSystemName(const char* p_systemName) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliGetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliGetUsrName(p_cmd);
}

void globalCli::onCliGetUsrName(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	char usrName[50];
	rc_t rc;
	if (rc = getUsrName(usrName)) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR ? "getUsrName not implemented for context %s-%i" : rc == RC_NOT_CONFIGURED_ERR ? "Could not get Username for context %s-%i, MO not configured" : "Unknown error: could not get Username for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	printCli("User name: %s", usrName);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getUsrName(char* p_usrName, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliSetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliSetUsrName(p_cmd);
}

void globalCli::onCliSetUsrName(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = setUsrName(cmd.getArgument(1).getValue().c_str())) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR ? "setUsrName not implemented for context %s-%i" : rc == RC_DEBUG_NOT_SET_ERR ? "Username could not be set for context %s-%i as debug is not set for this context - enable it by typing \"set debug\"" : "Unknown error: could not set Username for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setUsrName(const char* p_usrName, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliGetDescHelper(cmd* p_cmd, cliCore* p_cliContext,
								   cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliGetDesc(p_cmd);
}

void globalCli::onCliGetDesc(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	char description[50];
	rc_t rc;
	if (rc = getDesc(description)) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR ? "getDesc not implemented for context %s-%i" : rc == RC_NOT_CONFIGURED_ERR ? "Could not get Description for context %s-%i, MO not configured" : "Unknown error: could not get Description for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	printCli("Description: %s", description);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getDesc(char* p_description, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliSetDescHelper(cmd* p_cmd, cliCore* p_cliContext,
								   cliCmdTable_t* p_cmdTable) {
	static_cast<globalCli*>(p_cliContext)->onCliSetDesc(p_cmd);
}

void globalCli::onCliSetDesc(cmd* p_cmd) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = setDesc(cmd.getArgument(1).getValue().c_str())) {
		notAcceptedCliCommand(CLI_GEN_ERR, rc == RC_NOTIMPLEMENTED_ERR ? "setDesc not implemented for context %s-%i" : rc == RC_DEBUG_NOT_SET_ERR ? "Description could not be set for context %s-%i as debug is not set for this context - enable it by typing \"set debug\"" : "Unknown error: could not set Description for context %s-%i",
			getContextName(), getContextIndex());
		return;
	}
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setDesc(const char* p_description, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}
/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/
