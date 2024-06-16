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
EXT_RAM_ATTR char globalCli::decoderSysName[50];


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
	regCmdFlagArg(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, "panic", 0, true);
	regCmdFlagArg(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, "exception", 0, false);
	regCmdHelp(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, GLOBAL_REBOOT_HELP_TXT);



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: uptime																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

//global uptime SubMo
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, onCliGetUptime);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, GLOBAL_GET_UPTIME_HELP_TXT);


	
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: coredump																												*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

//global uptime SubMo
	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, COREDUMP_SUB_MO_NAME, onCliShowCoreDump);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, COREDUMP_SUB_MO_NAME, GLOBAL_SHOW_COREDUMP_HELP_TXT);



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
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "ssid", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "pass", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "hostname", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "address", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "mask", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "gw", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "dns", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, NETWORK_SUB_MO_NAME, "dhcp", 1, false);
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
/* CLI Sub-Managed object: wdt																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliSetWdt);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "debug", 1, false);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "timeout", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "action", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "active", 1, true);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_SET_WDT_HELP_TXT);

	regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliUnsetWdt);
	regCmdFlagArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "debug", 1, false);
	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_UNSET_WDT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliGetWdt);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "description", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "active", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "inhibited", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "timeout", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "actions", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "expiries", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "closesedhit", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_GET_WDT_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliShowWdt);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_SHOW_WDT_HELP_TXT);

	regCmdMoArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliClearWdt);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "allstats", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "expiries", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "closesedhit", 1, false);
	regCmdHelp(CLEAR_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_CLEAR_WDT_HELP_TXT);

	regCmdMoArg(STOP_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliStopWdt);
	regCmdFlagArg(STOP_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "id", 1, true);
	regCmdHelp(STOP_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_STOP_WDT_HELP_TXT);

	regCmdMoArg(START_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, onCliStartWdt);
	regCmdFlagArg(START_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, "id", 1, true);
	regCmdHelp(START_CLI_CMD, GLOBAL_MO_NAME, WDT_SUB_MO_NAME, GLOBAL_START_WDT_HELP_TXT);



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* CLI Sub-Managed object: job																													*/
/* Description: See cliGlobalDefinitions.h																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, onCliSetJob);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "debug", 1, false);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "priority", 1, true);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, GLOBAL_SET_JOB_HELP_TXT);

	regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, onCliUnsetJob);
	regCmdFlagArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "debug", 1, false);
	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, GLOBAL_UNSET_JOB_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, onCliGetJob);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "description", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "maxjobs", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "currentjobs", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peakjobs", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averagejobs", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peaklatency", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averagelatency", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peakexecution", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averageexecution", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "priority", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "overloaded", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "overloadcnt", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "wdtid", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "tasksort", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, GLOBAL_GET_JOB_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, onCliShowJob);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, GLOBAL_SHOW_JOB_HELP_TXT);

	regCmdMoArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, onCliClearJob);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "id", 1, true);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "allstats", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peakjobs", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averagejobs", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peaklatency", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averagelatency", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "peakexecution", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "averageexecution", 1, false);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, "overloadcnt", 1, false);
	regCmdHelp(CLEAR_CLI_CMD, GLOBAL_MO_NAME, JOB_SUB_MO_NAME, GLOBAL_CLEAR_JOB_HELP_TXT);



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
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliSetLogHelper);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "loglevel", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, true);
	regCmdFlagArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logconsole", 1, false);

	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_SET_LOG_HELP_TXT);

	regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliUnSetLogHelper);
	regCmdFlagArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, false);
	regCmdFlagArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logconsole", 1, false);

	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_UNSET_LOG_HELP_TXT);

	regCmdMoArg(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliAddLogHelper);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogitem", 1, false);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customloglevel", 1, true);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogfile", 1, true);
	regCmdFlagArg(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogfunc", 1, true);

	regCmdHelp(ADD_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_ADD_LOG_HELP_TXT);

	regCmdMoArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliDeleteLogHelper);
	regCmdFlagArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogitem", 1, false);
	regCmdFlagArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "all", 1, false);
	regCmdFlagArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogfile", 1, true);
	regCmdFlagArg(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogfunc", 1, true);
	regCmdHelp(DELETE_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_DELETE_LOG_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliGetLogHelper);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "loglevel", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "logdestination", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "customlogitems", 1, false);
	regCmdFlagArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "missedlogs", 1, false);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_GET_LOG_HELP_TXT);

	regCmdMoArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliClearLogHelper);
	regCmdFlagArg(CLEAR_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, "missedlogs", 1, false);
	regCmdHelp(CLEAR_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, GLOBAL_CLEAR_LOG_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, LOG_SUB_MO_NAME, onCliShowLogHelper);
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
	LOG_INFO_NOFMT("No context unique MOs supported for CLI context" CR);
}

/* Global CLI decoration methods */

//-------------------- Commands and contexts --------------------
void globalCli::onCliHelp(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_dummy) {
	Command cmd(p_cmd);
	char helpCmd[50];
	char subMo[50];
	if (!cmd.getArg(0).getValue().c_str() ||
		!strcmp(cmd.getArg(0).getValue().c_str(), "")) {
		strcpy(helpCmd, cmd.getName().c_str());
		strcpy(subMo, "");
		LOG_VERBOSE("Searching for help text for %s" CR, helpCmd);
	}
	else if (cmd.getArg(0).getValue().c_str() && (!cmd.getArg(1).getValue().c_str() || !strcmp(cmd.getArg(1).getValue().c_str(), ""))) {
		if (!strcmp(cmd.getArg(0).getValue().c_str(), "cli")) { //Exemption hack
			strcpy(helpCmd, "help");
			strcpy(subMo, "cli");
			LOG_VERBOSE_NOFMT("Searching for help text for CLI" CR);
		}
		else {
			strcpy(helpCmd, cmd.getArg(0).getValue().c_str());
			strcpy(subMo, "");
			LOG_VERBOSE("Searching for help text for %s" CR, helpCmd);
		}
	}
	else if (cmd.getArg(0).getValue().c_str() && cmd.getArg(1).getValue().c_str() && (!cmd.getArg(2).getValue().c_str() || !strcmp(cmd.getArg(2).getValue().c_str(),""))) {
		strcpy(helpCmd, cmd.getArg(0).getValue().c_str());
		strcpy(subMo, cmd.getArg(1).getValue().c_str());
		LOG_VERBOSE("Searching for help text for %s %s" CR, helpCmd, subMo);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
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
		LOG_INFO("No Help text available for %s %s" CR, helpCmd, subMo);
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
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
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
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
	}
	else if (!(targetContext = getCliContextHandleByPath(cmd.getArg(1).getValue().			//TR LOW: WE SHOULD ONLY LOOK AT CHILD CANDIDATES RELATED TO THIS OBJECT, getCliContextHandleByPath IS STATIC AND GOES FROM CURRENT CONTEXT!
														 c_str()))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Context %s does not exist",
													  cmd.getArg(1).getValue().c_str());
		LOG_VERBOSE("Context %s does not exist" CR,
					cmd.getArg(1).getValue().c_str());
	}
	else {
		LOG_VERBOSE("Setting context to: %s" CR, cmd.getArgument(1).getValue().c_str());
		setCurrentContext(targetContext);
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
}

void globalCli::onCliShowTopology(cmd* p_cmd, cliCore* p_cliContext,
								  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
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
				-20, "System-name:");
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
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	static_cast<globalCli*>(p_cliContext)->processAvailCommands(p_cmdTable->commandFlags->isPresent("all") ? true : false, p_cmdTable->commandFlags->isPresent("help")? true : false);
}
	
void globalCli::processAvailCommands(bool p_all, bool p_help) {
	QList<cliCmdTable_t*>* commandTable = getCliCommandTable();
	char flags[300];
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

//-------------------- Reboot --------------------
void globalCli::onCliReboot(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (cmd.getArgument(2)){
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		printCli("rebooting...");
		acceptedCliCommand(CLI_TERM_EXECUTED);
		reboot(NULL);
		cmdHandled = true;
		return;
	}
	if (p_cmdTable->commandFlags->getAllPresent()->size() > 1) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No more than one flag accepted");
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("panic")) {
		printCli("Creating a panic reboot with panic message \"%s\"", p_cmdTable->commandFlags->get("panic")->getValue());
		acceptedCliCommand(CLI_TERM_EXECUTED);
		acceptedCliCommand(CLI_TERM_EXECUTED);
		panic(p_cmdTable->commandFlags->get("panic")->getValue());
		cmdHandled = true;
		return;
	}
	else if (p_cmdTable->commandFlags->isPresent("exception")) {
		printCli("Creating an exeption reboot by \"dividing by zero\"");
		acceptedCliCommand(CLI_TERM_EXECUTED);
		int exception = 1 / 0;
		Serial.print(exception);
		cmdHandled = true;
		return;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

//-------------------- Uptime --------------------
void globalCli::onCliGetUptime(cmd* p_cmd, cliCore* p_cliContext,
							   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("Uptime: %i seconds", (uint32_t)(esp_timer_get_time()/1000000));
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

//-------------------- CoreDump --------------------
void globalCli::onCliShowCoreDump(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		char* stackTraceBuff = new (heap_caps_malloc(sizeof(char[10000]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[10000];
		uint readBytes;
		if (fileSys::getFile(FS_PATH "/" "panic", stackTraceBuff, 10000, &readBytes)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not read core dump file");
			LOG_WARN_NOFMT("Could not read core dump file" CR);
			delete stackTraceBuff;
			return;
		}
		stackTraceBuff[readBytes] = '\0';
		printCli("%s", stackTraceBuff);
		delete stackTraceBuff;
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

//-------------------- CPU --------------------
void globalCli::onCliStartCpu(cmd* p_cmd, cliCore* p_cliContext,
							  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("stats")) {
		if (cpu::getPm()) {
			notAcceptedCliCommand(CLI_GEN_ERR, "CPU statistics collection " \
											"is already active");
			LOG_VERBOSE_NOFMT("Cannot start CPU statistics " \
					     "collection - already active" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("globalCli::onCliStopCpu: Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("stats"))) {
		if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_GEN_ERR, "CPU statistics collection \
												is already inactive");
			LOG_VERBOSE_NOFMT("Cannot stop CPU statistics collection " \
						 "- already inactive" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
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
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not provide task information, " \
												"return code %i", rc);
			LOG_WARN("Could not provide task information, " \
					  "return code %i" CR, rc);
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
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not provide task information, " \
								  "return code %i", rc);
			LOG_WARN("Could not provide task information, " \
					  "return code %i" CR, rc);
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
		LOG_WARN_NOFMT("Not implemented" CR);
		return;
	}
	if ((p_cmdTable->commandFlags->isPresent("watermark"))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not implemented");
		LOG_WARN_NOFMT("Not implemented" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Not implemented");
	LOG_WARN_NOFMT("Not implemented" CR);
}

//-------------------- MEMORY --------------------
void globalCli::onCliGetMem(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	heapInfo_t memInfo;
	bool internal = false;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		(p_cmdTable->commandFlags->getAllPresent()->size() == 1 && p_cmdTable->commandFlags->isPresent("internal"))) {
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
			LOG_VERBOSE("Could not provide average memory usage, " \
						"given period exceeds the maximum of %i seconds" CR,
						CPU_HISTORY_SIZE - 1);
			return;
		}
		else if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide average memory usage, performance monitoring not active, " \
				"use \"start cpu -stats\"");
			LOG_VERBOSE_NOFMT("Could not provide average memory usage, performance monitoring not active" CR);
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
			LOG_VERBOSE("Could not provide memory usage trend, " \
						"given period exceeds the maximum of %i seconds" CR,
						CPU_HISTORY_SIZE - 1);
			return;
		}
		else if (!cpu::getPm()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Could not provide memory trend usage, performance monitoring not active, " \
				"use \"start cpu -stats\"");
			LOG_VERBOSE_NOFMT("Could not provide memory trend usage, performance monitoring not active" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("allocate")) {
		if (p_cmdTable->commandFlags->getAllPresent()->size() > 2) {
			char flags[100];
			p_cmdTable->commandFlags->getAllPresentStr(flags);
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed, incompatible permutation of flags: %s" CR, flags);
			LOG_WARN("Flag parsing failed, incompatible permutation of flags: %s" CR, flags);
			return;
		}
		if (testBuff) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Test buffer already allocated");
			LOG_VERBOSE_NOFMT("Test buffer already allocated" CR);
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
			LOG_VERBOSE("Failed to allocate %i bytes" CR,
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("allocate")) {
		if (p_cmdTable->commandFlags->getAllPresent()->size() > 1) {
			char flags[100];
			p_cmdTable->commandFlags->getAllPresentStr(flags);
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Flag parsing failed, incompatible permutation of flags: %s", flags);
			LOG_WARN("Flag parsing failed, incompatible permutation of flags : %s" CR, flags);
			return;
		}
		else {
			if (!testBuff) {
				notAcceptedCliCommand(CLI_GEN_ERR, "\"testBuffer\" not previously allocated");
				LOG_VERBOSE_NOFMT("\"testBuffer\" not previously allocated" CR);
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

//-------------------- NETWORK --------------------
void globalCli::onCliSetNetwork(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	rc_t rc;
	bool persist = false;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, p_cmdTable->commandFlags->
			getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	printCli("Changing network parameters will cause disruption in the network connectivity, potentially" \
			 "leading to service failures, including disconnection of this CLI Telnet session." CR);
	printCli("Do you want to continue [\"Y\"/\"N\"]?:");
	// Create acknowledgement
	printCli("Please re-establish your CLI-session...");
	if (p_cmdTable->commandFlags->isPresent("persist")) {
		if (!(p_cmdTable->commandFlags->isPresent("hostname") ||
			p_cmdTable->commandFlags->isPresent("address") ||
			p_cmdTable->commandFlags->isPresent("mask") ||
			p_cmdTable->commandFlags->isPresent("gw") ||
			p_cmdTable->commandFlags->isPresent("dns") ||
			p_cmdTable->commandFlags->isPresent("dhcp") ||
			p_cmdTable->commandFlags->isPresent("ssid") ||
			p_cmdTable->commandFlags->isPresent("pass"))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Can only persist Network configurations such as " \
														 "\"hostname\"-, \"address\"-, \"mask\"-, \"gw\"-, " \
														 "\"dns\"- , \"dhcp\", \"ssid\" and \"pass\"");
			LOG_VERBOSE_NOFMT("Can only persist Network configurations such as " \
							  "\"hostname\"-, \"address\"-, \"mask\"-, \"gw\"-, \"dns\"-, \"dhcp\", \"ssid\" and \"pass\"");
			return;
		}
		else{
			persist = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("ssid") || p_cmdTable->commandFlags->isPresent("pass")) {
		if (rc = networking::setSsidNPass(p_cmdTable->commandFlags->isPresent("ssid") ? p_cmdTable->commandFlags->isPresent("ssid")->getValue() : networking::getSsid(),
										  p_cmdTable->commandFlags->isPresent("pass") ? p_cmdTable->commandFlags->isPresent("pass")->getValue() : networking::getPass(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the WiFi SSID or WiFi Password, " \
											   "return code %i", rc);
			LOG_WARN("Could not set the WiFi SSID or WiFi Password, " \
					 "return code % i" CR, rc);
		}
		else {
			printCli("Wifi SSID and Password set - \"ssid\": %s, \"pass\": %s" CR,
					 p_cmdTable->commandFlags->isPresent("ssid") ? p_cmdTable->commandFlags->isPresent("ssid")->getValue() : networking::getSsid(),
 					 p_cmdTable->commandFlags->isPresent("pass") ? p_cmdTable->commandFlags->isPresent("pass")->getValue() : networking::getPass());
			LOG_INFO("Wifi SSID and Password set - \"ssid\": %s, \"pass\": %s" CR,
					 p_cmdTable->commandFlags->isPresent("ssid") ? p_cmdTable->commandFlags->isPresent("ssid")->getValue() : networking::getSsid(),
					 p_cmdTable->commandFlags->isPresent("pass") ? p_cmdTable->commandFlags->isPresent("pass")->getValue() : networking::getPass());
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("hostname")) {
		if (rc = networking::setHostname(p_cmdTable->commandFlags->
			isPresent("hostname")->getValue(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set the Hostname, " \
								  "return code %i", rc);
			LOG_WARN("Could not set the Hostname, \
					  return code %i" CR, rc);
			return;
		}
		LOG_INFO("Hostname changed to %s" CR,
				 networking::getHostname());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("dhcp")) {
		if (rc = networking::setDHCP(persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not enable DHCP, " \
				"return code %i", rc);
			LOG_WARN("Could not enable DHCP, \
					  return code %i" CR, rc);
			return;
		}
		LOG_INFO_NOFMT("DHCP Enabled" CR);
		cmdHandled = true;
		return;
	}
	else {
		IPAddress ip;
		bool ipProvided = false;
		if (p_cmdTable->commandFlags->isPresent("address")) {
			if (!ip.fromString(p_cmdTable->commandFlags->
				isPresent("address")->getValue())) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "\"Address\" does not provide a valid IP-address");
				LOG_VERBOSE_NOFMT("\"Address\" does not provide a valid IP-address" CR);
				return;
			}
			ipProvided = true;
		}
		IPAddress mask;
		bool maskProvided = false;
		if (p_cmdTable->commandFlags->isPresent("mask")) {
			if (!mask.fromString(p_cmdTable->commandFlags->isPresent("mask")->getValue())) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "\"mask\" does not provide a valid IP-address");
				LOG_VERBOSE_NOFMT("\"mask\" does not provide a valid IP-address" CR);
				return;
			}
			maskProvided = true;
		}
		IPAddress gw;
		bool gwProvided = false;
		if (p_cmdTable->commandFlags->isPresent("gw")) {
			if (!gw.fromString(p_cmdTable->commandFlags->isPresent("gw")->getValue())) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "\"gw\" does not provide a valid IP-address");
				LOG_VERBOSE_NOFMT("Not a valid Gateway-address" CR);
				return;
			}
			gwProvided = true;
		}
		IPAddress dns;
		bool dnsProvided = false;
		if (p_cmdTable->commandFlags->isPresent("dns")) {
			if (!dns.fromString(p_cmdTable->commandFlags->isPresent("dns")->getValue())) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "\"dns\" does not provide a valid IP-address");
				LOG_VERBOSE_NOFMT("\"dns\" does not provide a valid IP-address" CR);
				return;
			}
			dnsProvided = true;
		}
		if (ipProvided || maskProvided || gwProvided || dnsProvided) {
			if (rc = networking::setStaticIpAddr(ipProvided ? ip : networking::getIpAddr(),
				maskProvided ? mask : networking::getIpMask(),
				gwProvided ? gw : networking::getGatewayIpAddr(),
				dnsProvided ? dns : networking::getDnsIpAddr(),
				persist)) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Static addresses, " \
					"return code %i", rc);
				LOG_WARN("Could not set Static addresses, " \
					"return code %i" CR, rc);
				return;
			}
			printCli("Static IP addresses set - \"ip\": %s, \"mask\": %s, \"gw\": %s, \"dns\": %s" CR,
				ipProvided ? ip.toString().c_str() : networking::getIpAddr().toString().c_str(),
				maskProvided ? mask.toString().c_str() : networking::getIpMask().toString().c_str(),
				gwProvided ? gw.toString().c_str() : networking::getGatewayIpAddr().toString().c_str(),
				dnsProvided ? dns.toString().c_str() : networking::getDnsIpAddr().toString().c_str());
			LOG_INFO("Static IP addresses set - \"ip\": %s, \"mask\": %s, \"gw\": %s, \"dns\": %s" CR,
				ipProvided ? ip.toString().c_str() : networking::getIpAddr().toString().c_str(),
				maskProvided ? mask.toString().c_str() : networking::getIpMask().toString().c_str(),
				gwProvided ? gw.toString().c_str() : networking::getGatewayIpAddr().toString().c_str(),
				dnsProvided ? dns.toString().c_str() : networking::getDnsIpAddr().toString().c_str());
			cmdHandled = true;
		}
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
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
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
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
	char hostname[20 + 1];
	strcpyTruncMaxLen(hostname, networking::getHostname(), 20);
	char dhcp[5 + 1];
	strcpyTruncMaxLen(dhcp, networking::isStatic()? "False" : "True", 5);
	char ipaddr[16 + 1];
	strcpyTruncMaxLen(ipaddr, networking::getIpAddr().toString().c_str(), 16);
	char mask[16 + 1];
	strcpyTruncMaxLen(mask, networking::getIpMask().toString().c_str(), 16);
	char gw[16 + 1];
	strcpyTruncMaxLen(gw, networking::getGatewayIpAddr().toString().c_str(), 16);
	char dns[16 + 1];
	strcpyTruncMaxLen(dns, networking::getDnsIpAddr().toString().c_str(), 16);
	uint8_t ch = networking::getChannel();
	printCli("| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |", 
			-20, "SSID:",
			-18, "BSSID:",
			-8, "Channel:",
			-11, "Encryption:",
			-7, "RSSI:",
			-18, "MAC:",
			-20, "Host-name:",
			-5, "DHCP:",
			-16, "IP-Address:",
			-16, "IP-Mask",
			-16, "Gateway:",
			-16, "DNS");
	printCli("| %*s | %*s | %*i | %*s | %*i | %*s | %*s | %*s | %*s | %*s | %*s | %*s |",
			 -20, ssid,
			 -18, bssid,
			 -8, channel,
			 -11, auth,
			 -7, rssi,
			 -18, mac,
			 -20, hostname,
			 -5, dhcp,
			 -16, ipaddr,
			 -16, mask,
			 -16, gw,
			 -16, dns);
	acceptedCliCommand(CLI_TERM_QUIET);
}



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/wdt																										*/
/* Purpose: Defines global Watchdog managed objects																								*/
/* Description:	Provides means to monitor and manage registered Watchdog objects																*/
/*              such as:																														*/
/*				- set wdt...																													*/
/*				- unset wdt...																													*/
/*				- get wdt...																													*/
/*				- show wdt...																													*/
/*				- clear wdt...																													*/
/*				- stop wdt...																													*/
/*				- start wdt...																													*/
/*==============================================================================================================================================*/
void globalCli::onCliSetWdt(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("debug")) {
		if (wdt::getDebug()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Debug already set");
			LOG_VERBOSE_NOFMT("Debug already set" CR);
			return;
		}
		wdt::setDebug(true);
		cmdHandled = true;
	}
	if (!p_cmdTable->commandFlags->isPresent("id")) {
		if (p_cmdTable->commandFlags->isPresent("active")) {
			bool active;
			if (!strcmp(p_cmdTable->commandFlags->isPresent("active")->getValue(), "True"))
				active = true;
			else if (!strcmp(p_cmdTable->commandFlags->isPresent("active")->getValue(), "False"))
				active = false;
			else {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Invalid \"-active\" flag value: %s, is none of \"True\" or \"False\"", p_cmdTable->commandFlags->isPresent("active")->getValue());
				LOG_VERBOSE_NOFMT("Invalid \"-active\" flag value" CR);
				return;
			}
			if (wdt::setActiveAll(active) == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
				LOG_VERBOSE_NOFMT("debug flag not set" CR);
				return;
			}
			cmdHandled = true;
		}
	}
	else {
		uint16_t id = atoi(p_cmdTable->commandFlags->isPresent("id")->getValue());
		if (id == 0) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog id not valid");
			LOG_VERBOSE_NOFMT("Watchdog id not valid" CR);
			return;
		}
		if (!(p_cmdTable->commandFlags->isPresent("timeout") || p_cmdTable->commandFlags->isPresent("action") || p_cmdTable->commandFlags->isPresent("active"))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog \"-id\" flag must be followed by any of \"-timeout\", \"-action\" \"-active\ flags");
			LOG_VERBOSE_NOFMT("Watchdog \"-id\" flag must be followed by any of \"-timeout\", \"-action\" \"-active\ flags" CR);
			return;
		}
		if (p_cmdTable->commandFlags->isPresent("timeout")) {
			uint32_t timeout = atoi(p_cmdTable->commandFlags->isPresent("timeout")->getValue());
			if (timeout == 0) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog timeout not valid");
				LOG_VERBOSE_NOFMT("Watchdog timeout not valid" CR);
				return;
			}
			if (rc_t rc = wdt::setTimeout(id, timeout)) {
				if (rc == RC_NOT_FOUND_ERR) {
					notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog id does not exist");
					LOG_VERBOSE_NOFMT("Watchdog id does not exist" CR);
					return;
				}
				else if (rc == RC_DEBUG_NOT_SET_ERR) {
					notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
					LOG_VERBOSE_NOFMT("debug flag not set" CR);
					return;
				}
				else {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Watchdog timeout");
					LOG_VERBOSE_NOFMT("Could not set Watchdog timeout" CR);
					return;
				}
			}
			cmdHandled = true;
		}
		if (p_cmdTable->commandFlags->isPresent("action")) {
			char action[100];
			strcpy(action, p_cmdTable->commandFlags->isPresent("action")->getValue());
			if (rc_t rc = wdt::setActionsFromStr(id, action)) {
				if (rc == RC_NOT_FOUND_ERR) {
					notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog id does not exist");
					LOG_VERBOSE_NOFMT("Watchdog id does not exist" CR);
					return;
				}
				else if (rc == RC_PARAMETERVALUE_ERR) {
					notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog escalation actions invalid");
					LOG_VERBOSE_NOFMT("Watchdog escalation actions invalid" CR);
					return;
				}
				else if (rc == RC_DEBUG_NOT_SET_ERR) {
					notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
					LOG_VERBOSE_NOFMT("debug flag not set" CR);
					return;
				}
				else {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Watchdog escalation actions");
					LOG_VERBOSE_NOFMT("Could not set Watchdog escalation actions" CR);
					return;
				}
			}
			cmdHandled = true;
		}
		if (p_cmdTable->commandFlags->isPresent("active")) {
			bool active;
			if (!strcmp(p_cmdTable->commandFlags->isPresent("active")->getValue(), "True"))
				active = true;
			else if (!strcmp(p_cmdTable->commandFlags->isPresent("active")->getValue(), "False"))
				active = false;
			else {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Invalid \"-active\" flag value: %s, is none of \"True\" or \"False\"", p_cmdTable->commandFlags->isPresent("active")->getValue());
				LOG_VERBOSE_NOFMT("Invalid \"-active\" flag value" CR);
				return;
			}
			if (rc_t rc = wdt::setActive(id, active)) {
				if (rc == RC_NOT_FOUND_ERR) {
					notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Watchdog id does not exist");
					LOG_VERBOSE_NOFMT("Watchdog id does not exist" CR);
					return;
				}
				else if (rc == RC_DEBUG_NOT_SET_ERR) {
					notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
					LOG_VERBOSE_NOFMT("debug flag not set" CR);
					return;
				}
				else {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not set Watchdog escalation actions");
					LOG_VERBOSE_NOFMT("Could not set Watchdog escalation actions" CR);
					return;
				}
			}
			cmdHandled = true;
		}
	}
	if (cmdHandled) {
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliUnsetWdt(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("debug")) {
		if (!wdt::getDebug()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Debug already unset");
			LOG_VERBOSE_NOFMT("Debug already unset" CR);
			return;
		}
		wdt::setDebug(false);
		cmdHandled = true;
	}
	if (cmdHandled) {
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliGetWdt(cmd* p_cmd, cliCore* p_cliContext,
							cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		showWdt();
		cmdHandled = true;
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id")) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "Watchdog Id flag missing");
		LOG_VERBOSE_NOFMT("Watchdog Id flag missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "watchdog Id value missing");
		LOG_VERBOSE_NOFMT("Watchdog Id value missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->getAllPresent()->size() == 1) {
		if (!atoi(p_cmdTable->commandFlags->isPresent("id")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Invalid Watchdog Id");
			LOG_VERBOSE_NOFMT("Invalid Watchdog Id" CR);
			return;
		}
		showWdt(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()));
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->getAllPresent()->size() > 2) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "Only one WDT parameter can be showed at a time, " \
							  "use \"show wdt\" or \"get wdt\" to get a summary " \
							  "of all parameters");
		LOG_VERBOSE_NOFMT("Only one WDT parameter can be showed at a time, " \
							  "use \"show wdt\" or \"get wdt\" to get a summary " \
							  "of all parameters" CR);
		return;
	}
	wdt_t* wdtDescr;
	if (wdt::getWdtDescById( atoi(p_cmdTable->commandFlags->get("id")->getValue()), &wdtDescr)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "Watchdog ID does not exsist");
		LOG_VERBOSE_NOFMT("Watchdog ID does not exist" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("description")) {
		printCli(wdtDescr->wdtDescription);
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("active")) {
		printCli(wdtDescr->isActive? "True" : "False");
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("inhibited")) {
		printCli(wdtDescr->isInhibited ? "True" : "False");
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("timeout")) {
		printCli("%i", wdtDescr->wdtTimeoutTicks * WD_TICK_MS);
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("actions")) {
		char actionStr[31];
		printCli(wdt::actionToStr(actionStr, 30, wdtDescr->wdtAction));
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("expiries")) {
		printCli("%i", wdtDescr->wdtExpiries);
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("closesedhit")) {
		printCli("%i", wdtDescr->closesedhit * WD_TICK_MS);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliShowWdt(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	showWdt();
	acceptedCliCommand(CLI_TERM_QUIET);
	return;
}

void globalCli::showWdt(uint16_t p_wdtId) {
	char wdtInfo[200];
	sprintf(wdtInfo, "| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |",
		-3, "id:",
		-20, "Description:",
		-7, "Active:",
		-10, "Inhibited:",
		-13, "Timeout [ms]:",
		-30, "Actions: ",
		-9, "Expiries:",
		-17, "Closesedhit [ms]:");
	printCli("%s", wdtInfo);
	if (p_wdtId != 0) {
		char actionStr[31];
		wdt_t* wdtDescr;
		if (wdt::getWdtDescById(p_wdtId, &wdtDescr))
			return;
		sprintf(wdtInfo, "| %*i | %*s | %*s | %*s | %*i | %*s | %*i | %*i |",
			-3, wdtDescr->id,
			-20, wdtDescr->wdtDescription,
			-7, wdtDescr->isActive ? "True" : "False",
			-10, wdtDescr->isInhibited ? "True" : "False",
			-13, wdtDescr->wdtTimeoutTicks * WD_TICK_MS,
			-30, wdt::actionToStr(actionStr, 30, wdtDescr->wdtAction),
			-9, wdtDescr->wdtExpiries,
			-17, wdtDescr->closesedhit * WD_TICK_MS);
		printCli("%s", wdtInfo);
		return;
	}
	for (uint16_t wdtItter = 0; wdtItter <= wdt::maxId(); wdtItter++) {
		char actionStr[31];
		wdt_t* wdtDescr;
		if (wdt::getWdtDescById(wdtItter, &wdtDescr))
			continue;
		sprintf(wdtInfo, "| %*i | %*s | %*s | %*s | %*i | %*s | %*i | %*i |",
			-3, wdtDescr->id,
			-20, wdtDescr->wdtDescription,
			-7, wdtDescr->isActive? "True" : "False",
			-10, wdtDescr->isInhibited? "True" : "False",
			-13, wdtDescr->wdtTimeoutTicks * WD_TICK_MS,
			-30, wdt::actionToStr(actionStr, 30, wdtDescr->wdtAction),
			-9, wdtDescr->wdtExpiries,
			-17, wdtDescr->closesedhit * WD_TICK_MS);
		printCli("%s", wdtInfo);
	}
}

void globalCli::onCliClearWdt(cmd* p_cmd, cliCore* p_cliContext,
							  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	bool allIds = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id"))
		allIds = true;
	else if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "watchdog Id value missing");
		LOG_VERBOSE_NOFMT("Watchdog Id value missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("allstats")) {
		wdt::clearExpiriesAll();
		wdt::clearClosesedHitAll();
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("expiries")) {
		if (allIds) {
			wdt::clearExpiriesAll();
			cmdHandled = true;
		}
		else if (wdt::clearExpiries(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "Watchdog Id: %s does not exist", p_cmdTable->commandFlags->isPresent("id")->getValue());
			LOG_VERBOSE("Watchdog Id: %s does not exist" CR, p_cmdTable->commandFlags->isPresent("id")->getValue());
			return;
		}
		else
			cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("closesedhit")) {
		if (allIds) {
			wdt::clearClosesedHitAll();
			cmdHandled = true;
		}
		else if (wdt::clearClosesedHit(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Watchdog Id: %s does not exist", p_cmdTable->commandFlags->isPresent("id")->getValue());
			LOG_VERBOSE("Watchdog Id: %s does not exist" CR, p_cmdTable->commandFlags->isPresent("id")->getValue());
			return;
		}
		else
			cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliStartWdt(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable){
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		if (rc_t rc = wdt::inhibitAllWdtFeeds(false)) {
			if (rc == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
				LOG_VERBOSE_NOFMT("debug flag not set" CR);
				return;
			}
			else {
				notAcceptedCliCommand(CLI_GEN_ERR, "Watchdog could not be started");
				LOG_VERBOSE_NOFMT("Watchdog could not be started" CR);
				return;
			}
		}
		acceptedCliCommand(CLI_TERM_EXECUTED);
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id")){ 
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Watchdog id flag was expected but is missing");
		LOG_VERBOSE_NOFMT("Watchdog id flag was expected but is missing" CR);
		return;
	}
	else if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "watchdog id value missing");
		LOG_VERBOSE_NOFMT("Watchdog id value missing" CR);
		return;
	}
	if (rc_t rc = wdt::inhibitWdtFeeds(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()), false)) {
		if (rc == RC_NOT_FOUND_ERR) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"watchdog id does not exist");
			LOG_VERBOSE_NOFMT("watchdog id does not exist" CR);
			return;
		}
		else if (rc == RC_DEBUG_NOT_SET_ERR) {
			notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
			LOG_VERBOSE_NOFMT("debug flag not set" CR);
			return;
		}
		else {
			LOG_VERBOSE_NOFMT("watchdog could not be started" CR);
			return;
		}
	}
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliStopWdt(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		if (rc_t rc = wdt::inhibitAllWdtFeeds(true)){
			if (rc == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
					LOG_VERBOSE_NOFMT("debug flag not set" CR);
					return;
			}
			else {
				notAcceptedCliCommand(CLI_GEN_ERR, "Watchdog could not be started");
					LOG_VERBOSE_NOFMT("Watchdog could not be started" CR);
					return;
			}
		}
		acceptedCliCommand(CLI_TERM_EXECUTED);
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id")) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Watchdog id was expected but is missing");
		LOG_VERBOSE_NOFMT("Watchdog id was expected but is missing" CR);
		return;
	}
	else if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  "watchdog id value missing");
		LOG_VERBOSE_NOFMT("Watchdog id value missing" CR);
		return;
	}
	if (rc_t rc = wdt::inhibitWdtFeeds(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()), true)) {
		if (rc == RC_NOT_FOUND_ERR) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "watchdog id does not exist");
			LOG_VERBOSE_NOFMT("watchdog id does not exist" CR);
		}
		else if (rc == RC_DEBUG_NOT_SET_ERR) {
			notAcceptedCliCommand(CLI_GEN_ERR, 
								  "debug flag not set - see set wdt -debug");
			LOG_VERBOSE_NOFMT("debug flag not set" CR);
			return;
		}
		else {
			LOG_VERBOSE_NOFMT("watchdog could not be stoped" CR);
			return;
		}
	}
	acceptedCliCommand(CLI_TERM_EXECUTED);
}
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/wdt																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/job																										*/
/* Purpose: Defines global Job managed objects																									*/
/* Description:	Provides means to monitor and manage registered Job objects																		*/
/*              such as:																														*/
/*				- set job...																													*/
/*				- unset job...																													*/
/*				- get job...																													*/
/*				- show job...																													*/
/*				- clear job...																													*/
/*==============================================================================================================================================*/

void globalCli::onCliSetJob(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("debug")) {
		if (job::getDebug()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Debug already set");
			LOG_VERBOSE_NOFMT("Debug already set" CR);
			return;
		}
		job::setDebug(true);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("id")) {
		uint16_t id = atoi(p_cmdTable->commandFlags->isPresent("id")->getValue());
		if (id == 0) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Job id not valid");
			LOG_VERBOSE_NOFMT("Job id not valid" CR);
			return;
		}
		job* jobHandle = job::getJobHandleById(id);
		if (!jobHandle) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Job id does not exist");
			LOG_VERBOSE_NOFMT("Job id does not exist" CR);
			return;
		}
		if (p_cmdTable->commandFlags->isPresent("priority")) {
			rc_t rc = jobHandle->setPriority(atoi(p_cmdTable->commandFlags->isPresent("priority")->getValue()));
			if (rc == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "debug flag not set - see set wdt -debug");
				LOG_VERBOSE_NOFMT("debug flag not set" CR);
				return;
			}
			else if (rc) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Invalid Priority");
				LOG_VERBOSE_NOFMT("Invalid Priority" CR);
				return;
			}
			else
				cmdHandled = true;
		}
	}
	if (cmdHandled) {
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliUnsetJob(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("debug")) {
		if (!job::getDebug()) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Debug already unset");
			LOG_VERBOSE_NOFMT("Debug already unset" CR);
			return;
		}
		job::setDebug(false);
		cmdHandled = true;
	}
	if (cmdHandled) {
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliGetJob(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		showJob();
		cmdHandled = true;
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id")) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Job Id flag missing");
		LOG_VERBOSE_NOFMT("Job Id flag missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Job Id value missing");
		LOG_VERBOSE_NOFMT("Job Id value missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->getAllPresent()->size() == 1) {
		if (!atoi(p_cmdTable->commandFlags->isPresent("id")->getValue())) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Invalid Job Id");
			LOG_VERBOSE_NOFMT("Invalid Job Id" CR);
			return;
		}
		showJob(atoi(p_cmdTable->commandFlags->isPresent("id")->getValue()));
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->getAllPresent()->size() > 2) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Only one Job parameter can be showed at a time, " \
			"use \"show job\" or \"get job\" to get a summary " \
			"of all parameters");
		LOG_VERBOSE_NOFMT("Only one Job parameter can be showed at a time, " \
			"use \"show job\" or \"get job\" to get a summary " \
			"of all parameters" CR);
		return;
	}
	job* jobHandle;
	if (!(jobHandle = job::getJobHandleById(atoi(p_cmdTable->commandFlags->get("id")->getValue())))) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Job ID does not exsist");
		LOG_VERBOSE_NOFMT("Job ID does not exist" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("description")) {
		printCli(jobHandle->getJobDescription());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("maxjobs")) {
		printCli("%i", jobHandle->getJobSlots());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("currentjobs")) {
		printCli("%i", jobHandle->getPendingJobSlots());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("peakjobs")) {
		printCli("%i", jobHandle->getMaxJobSlotOccupancy());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("averagejobs")) {
		printCli("%i", jobHandle->getMeanJobSlotOccupancy());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("peaklatency")) {
		printCli("%i", jobHandle->getMaxJobQueueLatency());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("averagelatency")) {
		printCli("%i", jobHandle->getMeanJobQueueLatency());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("peakexecution")) {
		printCli("%i", jobHandle->getMaxJobExecutionTime());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("averageexecution")) {
		printCli("%i", jobHandle->getMeanJobExecutionTime());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("averageexecution")) {
		printCli("%i", jobHandle->getMeanJobExecutionTime());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("priority")) {
		printCli("%i", jobHandle->getPriority());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("overloaded")) {
		printCli("%s", jobHandle->getOverload()? "True" : "False");
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("overloadcnt")) {
		printCli("%i", jobHandle->getOverloadCnt());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("wdtid")) {
		printCli("%i", jobHandle->getWdtId());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("tasksort")) {
		printCli("%s", jobHandle->getTaskSorting() ? "True" : "False");
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}

void globalCli::onCliShowJob(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(0) || cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	showJob();
	acceptedCliCommand(CLI_TERM_QUIET);
	return;
}

void globalCli::showJob(uint16_t p_jobId) {
	char jobInfo[250];
	sprintf(jobInfo, "| %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |",
		-5, "id:",
		-20, "Desc:",
		-9, "Max Jobs:",
		-10, "Curr Jobs:",
		-10, "Peak Jobs:",
		-9, "Avr Jobs:",
		-13, "Peak lat(us):",
		-12, "Avr lat(us):",
		-12, "Peak ex(us):",
		-11, "Avr ex(us):",
		-5,  "Prio:",
		-9, "O-loaded:",
		-11, "O-load Cnt:",
		-6, "WdtId:",
		-10, "Task sort:");
	printCli("%s", jobInfo);
	if (p_jobId != 0) {
		if (!job::getJobHandleById(p_jobId)) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
				"Job ID does not exsist");
			LOG_VERBOSE_NOFMT("Job ID does not exist" CR);
			return;
		}
		sprintf(jobInfo, "| %*i | %*s | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*s | %*i | %*i | %*s |",
			-5, p_jobId,
			-20, job::getJobHandleById(p_jobId)->getJobDescription(),
			-9,  job::getJobHandleById(p_jobId)->getJobSlots(),
			-10, job::getJobHandleById(p_jobId)->getPendingJobSlots(),
			-10, job::getJobHandleById(p_jobId)->getMaxJobSlotOccupancy(),
			-9, job::getJobHandleById(p_jobId)->getMeanJobSlotOccupancy(),
			-13, job::getJobHandleById(p_jobId)->getMaxJobQueueLatency(),
			-12, job::getJobHandleById(p_jobId)->getMeanJobQueueLatency(),
			-12, job::getJobHandleById(p_jobId)->getMaxJobExecutionTime(),
			-11, job::getJobHandleById(p_jobId)->getMeanJobExecutionTime(),
			-5,  job::getJobHandleById(p_jobId)->getPriority(),
			-9, job::getJobHandleById(p_jobId)->getOverload()? "Yes" : "No",
			-11, job::getJobHandleById(p_jobId)->getOverloadCnt(),
			-6,  job::getJobHandleById(p_jobId)->getWdtId(),
			-10, job::getJobHandleById(p_jobId)->getTaskSorting()? "True" : "False");
		printCli("%s", jobInfo);
		return;
	}
	for (uint16_t jobItter = 0; jobItter <= job::maxId(); jobItter++) {
		if (!job::getJobHandleById(jobItter))
			continue;
		sprintf(jobInfo, "| %*i | %*s | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*i | %*s | %*i | %*i | %*s |",
			-5, jobItter,
			-20, job::getJobHandleById(jobItter)->getJobDescription(),
			-9, job::getJobHandleById(jobItter)->getJobSlots(),
			-10, job::getJobHandleById(jobItter)->getPendingJobSlots(),
			-10, job::getJobHandleById(jobItter)->getMaxJobSlotOccupancy(),
			-9, job::getJobHandleById(jobItter)->getMeanJobSlotOccupancy(),
			-13, job::getJobHandleById(jobItter)->getMaxJobQueueLatency(),
			-12, job::getJobHandleById(jobItter)->getMeanJobQueueLatency(),
			-12, job::getJobHandleById(jobItter)->getMaxJobExecutionTime(),
			-11, job::getJobHandleById(jobItter)->getMeanJobExecutionTime(),
			-5, job::getJobHandleById(jobItter)->getPriority(),
			-9, job::getJobHandleById(jobItter)->getOverload() ? "Yes" : "No",
			-11, job::getJobHandleById(jobItter)->getOverloadCnt(),
			-6, job::getJobHandleById(jobItter)->getWdtId(),
			-10, job::getJobHandleById(jobItter)->getTaskSorting() ? "True" : "False");
		printCli("%s", jobInfo);
	}
}

void globalCli::onCliClearJob(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	bool allIds = false;
	uint16_t id;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->isPresent("id"))
		allIds = true;
	else if (p_cmdTable->commandFlags->isPresent("id")->getValue() == NULL) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			"Job Id value missing");
		LOG_VERBOSE_NOFMT("Job Id value missing" CR);
		return;
	}
	else
		id = atoi(p_cmdTable->commandFlags->isPresent("id")->getValue());
	if (p_cmdTable->commandFlags->isPresent("allstats")) {
		if (allIds) {
			job::clearMaxJobSlotOccupancyAll();
			job::clearMeanJobSlotOccupancyAll();
			job::clearMaxJobQueueLatencyAll();
			job::clearMeanJobQueueLatencyAll();
			job::clearMaxJobExecutionTimeAll();
			job::clearMeanJobExecutionTimeAll();
			job::clearOverloadCntAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMaxJobSlotOccupancy(id) ||
				job::clearMeanJobSlotOccupancy(id) ||
				job::clearMaxJobQueueLatency(id) ||
				job::clearMeanJobQueueLatency(id) ||
				job::clearMaxJobExecutionTime(id) ||
				job::clearMeanJobExecutionTime(id) ||
				job::clearOverloadCnt(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("peakjobs")) {
		if (allIds) {
			job::clearMaxJobSlotOccupancyAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMaxJobSlotOccupancy(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("averagejobs")) {
		if (allIds) {
			job::clearMeanJobSlotOccupancyAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMeanJobSlotOccupancy(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("peaklatency")) {
		if (allIds) {
			job::clearMaxJobQueueLatencyAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMaxJobQueueLatency(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("averagelatency")) {
		if (allIds) {
			job::clearMeanJobQueueLatencyAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMeanJobQueueLatency(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("peakexecution")) {
		if (allIds) {
			job::clearMaxJobExecutionTimeAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMaxJobExecutionTime(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("averageexecution")) {
		if (allIds) {
			job::clearMaxJobExecutionTimeAll();
			cmdHandled = true;
		}
		else {
			if (job::clearMaxJobExecutionTime(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("overloadcnt")) {
		if (allIds) {
			job::clearOverloadCntAll();
			cmdHandled = true;
		}
		else {
			if (job::clearOverloadCnt(id)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
					"Job Id: %s does not exist", id);
				LOG_VERBOSE("Job Id: %s does not exist" CR, id);
				return;
			}
			cmdHandled = true;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
		LOG_VERBOSE_NOFMT("No valid arguments" CR);
	}
}
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/job																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



//-------------------- MQTT --------------------
void globalCli::onCliSetMqtt(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool persist = false;
	rc_t rc;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_VERBOSE_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("persist")) {
		if (!(p_cmdTable->commandFlags->isPresent("uri") ||
			p_cmdTable->commandFlags->isPresent("port"))) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "Can only persist MQTT URI and MQTT Port");
			LOG_VERBOSE_NOFMT("Can only persist MQTT URI and MQTT Port" CR);
			return;
		}
		else{
			persist = true;
		}
	}
	if (p_cmdTable->commandFlags->isPresent("uri") || p_cmdTable->commandFlags->isPresent("port")){
		printCli("Changing network parameters will cause disruption in the network connectivity, potentially" \
				 "leading to service failures, including disconnection of this CLI Telnet session." CR);
		printCli("Do you want to continue [\"Y\"/\"N\"]?:");
		// Create acknowledgement
		printCli("Please re-establish your CLI-session...");
	}
	if (p_cmdTable->commandFlags->isPresent("uri")) {
		if (rc = mqtt::setBrokerUri(p_cmdTable->commandFlags->
			isPresent("uri")->getValue(), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT URI, " \
												"return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("port")) {
		if (rc = mqtt::setBrokerPort(atoi(p_cmdTable->commandFlags->
			isPresent("port")->getValue()), persist)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Port, " \
												"return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("clientid")) {
		if (rc = mqtt::setClientId(p_cmdTable->commandFlags->
			isPresent("clientid")->getValue())) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Client Id, " \
												"return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("qos")) {
		if (rc = mqtt::setDefaultQoS(atoi(p_cmdTable->commandFlags->
			isPresent("qos")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Client Id, " \
												"return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("keepalive")) {
		if (rc = mqtt::setKeepAlive(atof(p_cmdTable->commandFlags->
			isPresent("keepalive")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT KeepAlive, " \
											   "return code %i", rc);
			return;
		}
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("ping")) {
		if (rc = mqtt::setPingPeriod(atof(p_cmdTable->commandFlags->
			isPresent("ping")->getValue()))) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not set MQTT Ping period, " \
												"return code %i", rc);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
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

//-------------------- TIME --------------------
void globalCli::onCliAddTime(cmd* p_cmd, cliCore* p_cliContext,
							 cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	IPAddress ipAddr;
	uint16_t ipPort;
	rc_t rc;
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR, 
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		!p_cmdTable->commandFlags->isPresent("ntpserver")->getValue()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - "\
							  "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - "\
				  "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpserver")) {
		LOG_INFO_NOFMT("Adding NTP server" CR);
		if (ipAddr.fromString(p_cmdTable->commandFlags->
			isPresent("ntpserver")->getValue())) {
			if (p_cmdTable->commandFlags->isPresent("ntpport") &&
				(ipPort = atoi(p_cmdTable->commandFlags->
					isPresent("ntpport")->getValue()))) {
				if (rc = ntpTime::addNtpServer(ipAddr, ipPort)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - " \
														"return code: %i", rc);
					LOG_WARN("Could not add NTP server - " \
							  "return code: %i" CR, rc);
					return;
				}
			}
			else{
				if (rc = ntpTime::addNtpServer(ipAddr)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - " \
										  "return code: %i", rc);
					LOG_WARN("Could not add NTP server - " \
							  "return code: %i" CR, rc);
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
						notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - " \
															"return code: %i", rc);
						LOG_WARN("Could not add NTP server - " \
								  "return code: %i" CR, rc);
						return;
				}
			}
			else {
				if (rc = ntpTime::addNtpServer(p_cmdTable->commandFlags->
					isPresent("ntpserver")->getValue())) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not add NTP server - " \
														"return code: %i", rc);
					LOG_WARN("Could not add NTP server - " \
							  "return code: %i" CR, rc);
					return;
				}
			}
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "%s is not a valid IP address or URI",
								  p_cmdTable->commandFlags->
										isPresent("ntpserver")->getValue());
			LOG_WARN("%s is not a valid IP address or URI" CR,
					 p_cmdTable->commandFlags->isPresent("ntpserver")->getValue());
			return;
		}
		LOG_VERBOSE_NOFMT("NTP server successfully added" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size() ||
		!p_cmdTable->commandFlags->isPresent("ntpserver")->getValue()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - " \
													  "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - " \
				  "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpserver")) {
		LOG_INFO_NOFMT("Deleting NTP server" CR);
		if (ipAddr.fromString(p_cmdTable->commandFlags->
			isPresent("ntpserver")->getValue())) {
			if (rc = ntpTime::deleteNtpServer(ipAddr)) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not delete NTP server - " \
													"return code: %i", rc);
				LOG_WARN("Could not delete NTP server - "\
						  "return code: %i" CR, rc);
				return;
			}
		}
		else if (isUri(p_cmdTable->commandFlags->isPresent("ntpserver")->getValue())) {
			if (rc = ntpTime::deleteNtpServer(p_cmdTable->commandFlags->
				isPresent("ntpserver")->getValue())) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not delete NTP server - "\
													"return code: %i", rc);
				LOG_WARN("Could not delete NTP server - " \
						  "return code: %i" CR, rc);
				return;
			}
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
								  "%s is not a valid IP address or URI",
								  p_cmdTable->commandFlags->
										isPresent("ntpserver")->getValue());
			LOG_WARN("%s is not a valid IP address " \
					  "or URI" CR,
					 p_cmdTable->commandFlags->isPresent("ntpserver")->getValue());
			return;
		}
		LOG_VERBOSE_NOFMT("NTP server successfully deleted" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - " \
													  "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - " \
				  "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpclient")) {
		LOG_INFO_NOFMT("Starting NTP server" CR);
		ntpTime::getNtpOpState(&ntpOpState);
		if (ntpOpState & NTP_CLIENT_DISABLED) {
			if(p_cmdTable->commandFlags->isPresent("ntpdhcp")){
				if (rc = ntpTime::start(true)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not start NTP-Client - " \
														"return code: %i", rc);
					LOG_WARN("Could not start NTP-Client - " \
							  "return code: %i" CR, rc);
					return;
				}
			}
			else {
				if (rc = ntpTime::start(false)) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Could not start NTP-Client - "\
														"return code: %i", rc);
					LOG_WARN("Could not start NTP-Client - "\
							  "return code: %i" CR, rc);
					return;
				}
			}
		}
		else{
			notAcceptedCliCommand(CLI_GEN_ERR, "NTP client is already running");
			LOG_WARN_NOFMT("NTP client is already running" CR);
			return;
		}
		LOG_VERBOSE_NOFMT("NTP server successfully started" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - " \
													  "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - " \
				  "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("ntpclient")) {
		LOG_INFO("Stoping NTP server" CR);
		ntpTime::getNtpOpState(&ntpOpState);
		if (!(ntpOpState & NTP_CLIENT_DISABLED)) {
			if (rc = ntpTime::stop()) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Could not stop NTP-Client - " \
													"return code: %i", rc);
				LOG_WARN("Could not stop NTP-Client - " \
						"return code: %i" CR, rc);
				return;
			}
		}
		else {
			notAcceptedCliCommand(CLI_GEN_ERR, "NTP client is not running");
			LOG_WARN_NOFMT("NTP client is not running" CR);
			return;
		}
		LOG_VERBOSE_NOFMT("NTP server successfully stopped" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - " \
													  "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - " \
				  "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("timeofday") ||
		p_cmdTable->commandFlags->isPresent("tod")) {
		LOG_INFO_NOFMT("Setting time of day" CR);
		if (p_cmdTable->commandFlags->isPresent("timeofday")){
			if (ntpTime::setTimeOfDay(p_cmdTable->commandFlags->
				isPresent("timeofday")->getValue(), response)) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
				LOG_WARN("Setting time of day not accepted, %s" CR, response);
				return;
			}
		}
		else {
			if (ntpTime::setTimeOfDay(p_cmdTable->commandFlags->
				isPresent("tod")->getValue(), response)) {
				notAcceptedCliCommand(CLI_GEN_ERR, "%s", response);
				LOG_WARN("globalCli::onCliSetTime: %s" CR, response);
				return;
			}		
		}
		LOG_VERBOSE_NOFMT("Successfully set time of day" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("epochtime")) {
		LOG_INFO_NOFMT("Setting epoch time" CR);
		if (ntpTime::setEpochTime(p_cmdTable->commandFlags->
			isPresent("epochtime")->getValue(), response)) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
			LOG_WARN("Setting epoch time not accepted, %s" CR, response);
			return;
		}
		LOG_VERBOSE("Successfully set epoch time" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("timezone")) {
		LOG_INFO_NOFMT("Setting time zone" CR);
		if (ntpTime::setTz(p_cmdTable->commandFlags->
			isPresent("timezone")->getValue(), response)) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s", response);
			LOG_WARN("Setting time zone not accepted, %s" CR, response);
			return;
		}
		LOG_VERBOSE_NOFMT("Successfully set time zone" CR);
		cmdHandled = true;
	}
	if (p_cmdTable->commandFlags->isPresent("daylightsaving")) {
		LOG_INFO_NOFMT("Setting dayligh saving" CR);
		if (!strcmp(p_cmdTable->commandFlags->isPresent("daylightsaving")->getValue(), "true")) {
			ntpTime::setDayLightSaving(true);
			LOG_VERBOSE_NOFMT("Successfully set time zone to \"true\"" CR);
			cmdHandled = true;
		}
		else if (!strcmp(p_cmdTable->commandFlags->isPresent("daylightsaving")->getValue(), "false")) {
			ntpTime::setDayLightSaving(false);
			LOG_VERBOSE_NOFMT("Successfully set time zone to \"false\"" CR);
			cmdHandled = true;
		}
		else {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "%s is not a value daylightsaving flag value, valid values are \"true\" or \false\"", response);
			LOG_WARN("%s is not a value daylightsaving flag value, valid values are \"true\" or \false\"" CR, response);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
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
			LOG_ERROR_NOFMT("Could not get NTP servers" CR);
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
		LOG_WARN_NOFMT("Bad number of arguments" CR);
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
		LOG_ERROR_NOFMT("Could not get NTP servers" CR);
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

//-------------------- LOG --------------------
void globalCli::onCliSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,
								  cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0) || !cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, 
							  "Bad number of arguments - mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("loglevel")) {
		if (rc = static_cast<globalCli*>(rootHandle)->
			setLogLevel(p_cmdTable->commandFlags->
				get("loglevel")->getValue())) {
			if (rc == RC_DEBUG_NOT_SET_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level " \
				"not accepted, debug flag not set");
				LOG_WARN_NOFMT("Setting of Log-level " \
						  "not accepted, debug flag not set" CR);
				return;
			}
			else if (rc == RC_PARAMETERVALUE_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level " \
					"not accepted, provided Log-level: %s is not valid", 
					p_cmdTable->commandFlags->isPresent("loglevel")->getValue());
				LOG_WARN("Setting of Log-level " \
					"not accepted, provided Log-level: %s is not valid" CR,
					p_cmdTable->commandFlags->isPresent("loglevel")->getValue());
				return;
			}
			else {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Log-level unsuccessfull, "\
													"unknown error - return code: %i", rc);
				LOG_WARN("Setting of Log-level " \
						 "unsuccessfull, unknown error - return code: %i" CR, rc);
				return;
			}
		}
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		if (!Log.getLogServer(0, NULL, NULL)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Cannot set log destination, log destination is already configured, use \"unset log -logdestination\" to remove current log destination first");
			LOG_WARN_NOFMT("Cannot set log destination, log destination is already configured" CR);
			return;
		}
		if (!isUri(p_cmdTable->commandFlags->get("logdestination")->getValue())) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Provided Log-destination: %s " \
				"is not valid URI", p_cmdTable->commandFlags->get("logdestination")->getValue());
			LOG_WARN("Provided Log-destination %s " \
				"is not a valid URI" CR, p_cmdTable->commandFlags->get("logdestination")->getValue());
			return;
		}
		rootHandle->getSystemName(decoderSysName, true);
		Log.addLogServer(decoderSysName, p_cmdTable->commandFlags->get("logdestination")->getValue());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("logconsole")) {
		Log.setConsoleLog(true);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

rc_t globalCli::setLogLevel(const char* p_logLevel, bool p_force) {
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliUnSetLogHelper(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0) || !cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, 
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments - " \
													 "mandatory flags missing");
		LOG_WARN_NOFMT("Bad number of arguments - " \
				 "mandatory flags missing" CR);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		if (Log.getLogServer(0, NULL, NULL)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Can not unset log destination, no log destination has been configured");
			LOG_WARN_NOFMT("Can not unset log destination, no log destination has been configured" CR);
			return;
		}
		Log.deleteAllLogServers();;
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("logconsole")) {
		Log.setConsoleLog(false);
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliAddLogHelper(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0) || !cmd.getArgument(3)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("customlogitem")) {
		if (!p_cmdTable->commandFlags->isPresent("customloglevel")) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments, \"-customloglevel\" flag is required for \"customlogitem\" flag");
			LOG_WARN_NOFMT("Bad number of arguments, \"-customloglevel\" flag is required for \"customlogitem\" flag" CR);
			return;
		}
		if (!p_cmdTable->commandFlags->isPresent("customlogfile")) {
			notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments, \"-file\" flag is required for \"customlogitem\" flag");
			LOG_WARN_NOFMT("Bad number of arguments, \"-customlogfile\" flag is required for \"customlogitem\" flag" CR);
			return;
		}
		rc_t rc;
		if (rc = Log.addCustomLogItem(p_cmdTable->commandFlags->get("customlogfile")->getValue(), p_cmdTable->commandFlags->isPresent("customlogfunc") ? p_cmdTable->commandFlags->get("customlogfunc")->getValue() : NULL, Log.transformLogLevelXmlStr2Int(p_cmdTable->commandFlags->get("customloglevel")->getValue()))) {
			if (rc == RC_PARAMETERVALUE_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of custom Log-level " \
					"not accepted, Custom log-level: %s is not a valid log-level",
					p_cmdTable->commandFlags->get("customloglevel")->getValue());
				LOG_WARN("Setting of Log-level " \
					"not accepted, Custom log-level: %s is not a valid log-level" CR,
					p_cmdTable->commandFlags->get("customloglevel")->getValue());
				return;
			}
			else if (rc == RC_ALREADYEXISTS_ERR) {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of custom Log-level " \
					"not accepted, Custom log-level item already exist, remove it before setting a new custom log policy to this item");
				LOG_WARN("Setting of Log-level " \
					"not accepted, Custom log-level item already exist" CR);
				return;
			}
			else {
				notAcceptedCliCommand(CLI_GEN_ERR, "Setting of custom Log-level unsuccessful, " \
					"unknown error - return code: %i", rc);
				LOG_WARN("Setting of custom Log-level " \
					"unsuccessfull, unknown error - return code: %i" CR, rc);
				return;
			}
		}
		else
			cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliDeleteLogHelper(cmd* p_cmd, cliCore* p_cliContext,
	cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0) || !cmd.getArgument(1) || cmd.getArgument(6)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments" CR);
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("customlogitem")) {
		if (p_cmdTable->commandFlags->isPresent("all")) {
			Log.deleteAllCustomLogItems();
			cmdHandled = true;
		}
		else {
			if (!p_cmdTable->commandFlags->isPresent("customlogfile")) {
				notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments, \"-customlogfile\" flag is required for \"-customlogitem\" flag");
				LOG_WARN_NOFMT("Bad number of arguments, \"-customlogfile\" flag is required for \"-customlogitem\" flag" CR);
				return;
			}
			rc_t rc;
			if (rc = Log.deleteCustomLogItem(p_cmdTable->commandFlags->get("customlogfile")->getValue(), p_cmdTable->commandFlags->isPresent("customlogfunc")? p_cmdTable->commandFlags->get("customlogfunc")->getValue() : NULL)) {
				if (rc == RC_NOT_FOUND_ERR) {
					notAcceptedCliCommand(CLI_GEN_ERR, "Deletion of custom Log-level " \
						"not accepted, Custom log item not found");
					LOG_WARN("Deletion of custom Log-level " \
						"not accepted, Custom log item: %s - %s not found" CR,
						p_cmdTable->commandFlags->get("customlogfile")->getValue(), p_cmdTable->commandFlags->get("customlogfunc")? p_cmdTable->commandFlags->get("customlogfunc")->getValue() : "-");
					return;
				}
				else {
					notAcceptedCliCommand(CLI_GEN_ERR, "Deletion of custom Log item unsuccessful, " \
						"unknown error - return code: %i", rc);
					LOG_WARN("Deletion of custom Log-level " \
						"unsuccessfull, unknown error - return code: %i" CR, rc);
					return;
				}
			}
			else
				cmdHandled = true;
		}
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_EXECUTED);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliGetLogHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	rc_t rc;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
							  p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
					p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (!p_cmdTable->commandFlags->getAllPresent()->size()) {
		onCliShowLog();
		acceptedCliCommand(CLI_TERM_QUIET);
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("loglevel")) {
		if (!static_cast<globalCli*>(rootHandle)->getLogLevel()) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not retrieve current global log-level");
			LOG_VERBOSE_NOFMT("Could not retrieve current global log-level" CR);
			return;
		}
		printCli("Current global log-level: %s", static_cast<globalCli*>(rootHandle)->getLogLevel());
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("logdestination")) {
		char rSysLogServer[100];
		uint16_t rSysLogServerPort;
		if (static_cast<globalCli*>(rootHandle)->getRSyslogServer(rSysLogServer, &rSysLogServerPort)) {
			notAcceptedCliCommand(CLI_GEN_ERR, "Could not retrieve Rsyslog destination");
			LOG_VERBOSE_NOFMT("Could not retrieve Rsyslog destination" CR);
			return;
		}
		else{
			printCli("RSyslog destination: %s, RSyslog port %i", rSysLogServer, rSysLogServerPort);
			cmdHandled = true;
		}
	}
	else if (p_cmdTable->commandFlags->isPresent("customlogitems")) {
		printCustomLogItems();
		cmdHandled = true;
	}
	else if (p_cmdTable->commandFlags->isPresent("missedlogs")) {
		printCli("Missed log items: %i", Log.getMissedLogs());
		cmdHandled = true;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

const char* globalCli::getLogLevel(void) {
	return Log.transformLogLevelInt2XmlStr(Log.getLogLevel());
}

rc_t globalCli::getRSyslogServer(char* p_uri, uint16_t* p_port, bool p_force) {
	return Log.getLogServer(0, p_uri, p_port);
}

void globalCli::onCliClearLogHelper(cmd* p_cmd, cliCore* p_cliContext, cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0) || !cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	if (p_cmdTable->commandFlags->parse(cmd)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR,
			p_cmdTable->commandFlags->getParsErrs());
		LOG_VERBOSE("Flag parsing failed: %s" CR,
			p_cmdTable->commandFlags->getParsErrs());
		return;
	}
	if (p_cmdTable->commandFlags->isPresent("missedlogs")) {
		Log.clearMissedLogs();
		acceptedCliCommand(CLI_TERM_EXECUTED);
		return;
	}
	if (cmdHandled)
		acceptedCliCommand(CLI_TERM_QUIET);
	else
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "No valid arguments");
}

void globalCli::onCliShowLogHelper(cmd* p_cmd, cliCore* p_cliContext,
								   cliCmdTable_t* p_cmdTable) {
	Command cmd(p_cmd);
	bool cmdHandled = false;
	if (!cmd.getArgument(0)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		LOG_WARN_NOFMT("Bad number of arguments");
		return;
	}
	onCliShowLog();
	acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::onCliShowLog(void) {
	printLogInfo();
	printCustomLogItems();
}
void globalCli::printLogInfo(void) {
	printCli("Global log information");
	printCli("| %*s | %*s | %*s | %*s |",
		-20, "Log-level:",
		-30, "Log-receiver:",
		-10, "Log-port",
		-21, "Missed log entries:");
	{
		char rSyslogServerUri[50];
		uint16_t rsysLogServerPort;
		if (!static_cast<globalCli*>(rootHandle)->getRSyslogServer(rSyslogServerUri, &rsysLogServerPort)) {
			printCli("| %*s | %*s | %*i | %*i |",
				-20, static_cast<globalCli*>(rootHandle)->getLogLevel(),
				-30, strTruncMaxLen(rSyslogServerUri, 28),
				-10, rsysLogServerPort,
				-21, Log.getMissedLogs());
		}
		else {
			printCli("| %*s | %*s | %*s | %*i |",
				-20, static_cast<globalCli*>(rootHandle)->getLogLevel(),
				-30, "-",
				-10, "-",
				-21, Log.getMissedLogs());
		}
	}
}

void globalCli::printCustomLogItems(void) {
	printCli("\nCustom log information");
	printCli("| %*s | %*s | %*s |",
		-40, "Custom log function:",
		-40, "Custom log function:",
		-20, "Custom Log-level:");
	for (uint16_t customLogItemItter = 0; customLogItemItter < Log.getNoOffCustomLogItem(); customLogItemItter++) {
		const char* customLogFile;
		const char* customLogFunction;
		logSeverity_t customLogLevel;
		if (!Log.getCustomLogItem(customLogItemItter, &customLogFile, &customLogFunction, &customLogLevel)) {
			printCli("| %*s | %*s | %*s |",
				-40, customLogFile,
				-40, customLogFunction ? customLogFunction : "-",
				-20, Log.transformLogLevelInt2XmlStr(customLogLevel));
		}
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
		notAcceptedCliCommand(CLI_GEN_ERR, "Activating fail-safe not accepted, " \
											"unknown error - return code: %i", rc);
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
		notAcceptedCliCommand(CLI_GEN_ERR, "In-activating fail-safe not accepted, " \
											"return code: %i", rc);
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
//-------------------- DEBUG --------------------
void globalCli::onCliSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,
									cliCmdTable_t* p_cmdTable) {
	if (static_cast<globalCli*>(p_cliContext)->getDebug()) {
		notAcceptedCliCommand(CLI_NOT_VALID_CMD_ERR, "Debug already set");
		return;
	}
	static_cast<globalCli*>(p_cliContext)->setDebug(true);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliUnSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext,
									  cliCmdTable_t* p_cmdTable) {
	if (!static_cast<globalCli*>(p_cliContext)->getDebug()) {
		notAcceptedCliCommand(CLI_NOT_VALID_CMD_ERR, "Debug already unset");
		return;
	}
	static_cast<globalCli*>(p_cliContext)->setDebug(false);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::setDebug(bool p_debug) {
	notAcceptedCliCommand(CLI_GEN_ERR, "Setting/Unsetting debug flag is not implemented for this context");
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
	LOG_WARN_NOFMT("debug flag not implemented" CR);
	return false;
}

//-------------------- OPSTATE --------------------
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

//-------------------- MOM Sys, Usr, and Desc handling --------------------
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

rc_t globalCli::setSystemName(const char* p_systemName, bool p_force) {
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
/* END Class globalCli                                                                                                                            */
/*==============================================================================================================================================*/
