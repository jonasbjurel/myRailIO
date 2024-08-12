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
/* .h Definitions                                                                                                                               */
/*==============================================================================================================================================*/
#ifndef CLIGLOBALDEFINITIONS_H
#define CLIGLOBALDEFINITIONS_H
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Managed object (MO) definitions                                                                                                              */
/* Purpose: Defines the managed objects (MOs)																									*/
/* Description: The CLI defines a number of operations/methods defined in found in cliCore.h, E.g. help, set, get, show, start, stop, ...		*/
/*              These methods operates on myRailIO Managed objects (MOs) and myRailIO sub-managed objects (sub-MOs)								*/
/*				- MOs defines a category of sub-MOs:																							*/
/*				  > The global MO: defines a set of sub-MOs which arent related to any particular CLI context, but which is globally avaliable: */
/*					e.g. reboot, help, context, uptime. CLI examples for global MOs: "help help", "help cli", "get context",                    */
/*					set context {cli-conext path}", "get uptime", "show memory", etc.															*/
/*				  > The Common MO: Defines a set of sub-MOs that are commonly available for all MOs, but in difference to the global MO, the	*/
/*					sub-MOs defined as a common MO operates on particular MOs and their instances. CLI examples for common MO sub-MOs:			*/
/*					"{myRailIO-context-path}get opstate", "{myRailIO-context-path}set description {description string}",							*/
/*					"{myRailIO-context-path}set debug", "{myRailIO-context-path}unset debug"														*/
/*				  > Specific myRailIO MOs: These MOs are specific to certain myRailIO servicies, such as: "lglink", "lightgroup", "satellitelink", */
/*					"satellite", "sensor", "actuator, etc. Specific MOs may implent specific sub-MOs only rellevant for a particular specific MO.*/
/*					CLI examples for specific MOs sub-MOs: "get lglink -overruns", "get lglink -overruns", "get lightgroup -showing", ...		*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR const char	ROOT_MO_NAME[] = "root";

EXT_RAM_ATTR const char	GLOBAL_MO_NAME[] = "global";								//Practical usage of MO is vagely defined - other than that all global commands are allways available (no matter context) and served by the globalCli static class
EXT_RAM_ATTR const char		CLIHELP_SUB_MO_NAME[] = "cli";							//Only for help, we need to add a property that this is only for help, and not a real command
EXT_RAM_ATTR const char		HELP_SUB_MO_NAME[] = "help";							//Print help text, see help text definitions below
EXT_RAM_ATTR const char		CONTEXT_SUB_MO_NAME[] = "context";						//Global MO instance context sub-MO
EXT_RAM_ATTR const char		UPTIME_SUB_MO_NAME[] = "uptime";						//Global MO Decoder up-time sub-MO
EXT_RAM_ATTR const char		COREDUMP_SUB_MO_NAME[] = "coredump";					//Global MO Decoder coredump sub-MO
EXT_RAM_ATTR const char		CPU_SUB_MO_NAME[] = "cpu";								//Global MO CPU sub-MO
EXT_RAM_ATTR const char		CPUMEM_SUB_MO_NAME[] = "memory";						//Global MO Memory sub-MO
EXT_RAM_ATTR const char		NETWORK_SUB_MO_NAME[] = "network";						//Global Network MO sub-MO
EXT_RAM_ATTR const char		TOPOLOGY_SUB_MO_NAME[] = "topology";					//Global MO topology sub-MO
EXT_RAM_ATTR const char		COMMANDS_SUB_MO_NAME[] = "commands";					//Global Context available commands sub-MO
EXT_RAM_ATTR const char		WDT_SUB_MO_NAME[] = "wdt";								//Global MO WDT sub-MO
EXT_RAM_ATTR const char		JOB_SUB_MO_NAME[] = "job";								//Global MO JOB sub-MO
EXT_RAM_ATTR const char		MQTT_SUB_MO_NAME[] = "mqtt";							//Global MO MQTT sub-MO
EXT_RAM_ATTR const char		PINGSUPERVISION_SUB_MO_NAME[] = "supervision";			//move to MQTT?????????
EXT_RAM_ATTR const char		TIME_SUB_MO_NAME[] = "time";							//Global MO time sub-MO
EXT_RAM_ATTR const char		LOG_SUB_MO_NAME[] = "log";								//Global MO log sub-MO
EXT_RAM_ATTR const char		FAILSAFE_SUB_MO_NAME[] = "failsafe";					//Global MO failsafe-MO


EXT_RAM_ATTR const char	COMMON_MO_NAME[] = "common";								//Common MO
EXT_RAM_ATTR const char		OPSTATE_SUB_MO_NAME[] = "opstate";						//Common MO OP-state sub-MO
EXT_RAM_ATTR const char		SYSNAME_SUB_MO_NAME[] = "systemname";					//Common MO System name sub-MO
EXT_RAM_ATTR const char		USER_SUB_MO_NAME[] = "username";						//Common MO User ame sub-MO
EXT_RAM_ATTR const char		DESC_SUB_MO_NAME[] = "description";						//Common MO Description sub-MO
EXT_RAM_ATTR const char		DEBUG_SUB_MO_NAME[] = "debug";							//Common MO Debug sub-MO

EXT_RAM_ATTR const char	DECODER_MO_NAME[] = "decoder";							// All decoder MOs are mapped to the global and common context

EXT_RAM_ATTR const char	LGLINK_MO_NAME[] = "lglink";
EXT_RAM_ATTR const char		LGLINKNO_SUB_MO_NAME[] = "link";
EXT_RAM_ATTR const char		LGLINKOVERRUNS_SUB_MO_NAME[] = "overruns";
EXT_RAM_ATTR const char		LGLINKMEANLATENCY_SUB_MO_NAME[] = "meanlatency";
EXT_RAM_ATTR const char		LGLINKMAXLATENCY_SUB_MO_NAME[] = "maxlatency";
EXT_RAM_ATTR const char		LGLINKMEANRUNTIME_SUB_MO_NAME[] = "meanruntime";
EXT_RAM_ATTR const char		LGLINKMAXRUNTIME_SUB_MO_NAME[] = "maxruntime";

EXT_RAM_ATTR const char	LG_MO_NAME[] = "lightgroup";
EXT_RAM_ATTR const char		LGADDR_SUB_MO_NAME[] = "address";
EXT_RAM_ATTR const char		LGLEDCNT_SUB_MO_NAME[] = "ledcnt";
EXT_RAM_ATTR const char		LGLEDOFFSET_SUB_MO_NAME[] = "ledoffset";
EXT_RAM_ATTR const char		LGPROPERTY_SUB_MO_NAME[] = "property";
EXT_RAM_ATTR const char		LGSTATE_SUB_MO_NAME[] = "state";
EXT_RAM_ATTR const char		LGSHOWING_SUB_MO_NAME[] = "showing";

EXT_RAM_ATTR const char	SATLINK_MO_NAME[] = "satellitelink";
EXT_RAM_ATTR const char		SATLINKNO_SUB_MO_NAME[] = "link";
EXT_RAM_ATTR const char		SATLINKTXUNDERRUN_SUB_MO_NAME[] = "txunderrun";
EXT_RAM_ATTR const char		SATLINKRXOVERRUN_SUB_MO_NAME[] = "rxoverrun";
EXT_RAM_ATTR const char		SATLINKTIMINGVIOLATION_SUB_MO_NAME[] = "timingviolation";
EXT_RAM_ATTR const char		SATLINKRXCRCERR_SUB_MO_NAME[] = "rxcrcerr";
EXT_RAM_ATTR const char		SATLINKREMOTECRCERR_SUB_MO_NAME[] = "remotecrcerr";
EXT_RAM_ATTR const char		SATLINKRXSYMERRS_SUB_MO_NAME[] = "rxsymbolerr";
EXT_RAM_ATTR const char		SATLINKRXSIZEERRS_SUB_MO_NAME[] = "rxsizeerr";
EXT_RAM_ATTR const char		SATLINKWDERRS_SUB_MO_NAME[] = "wderr";

EXT_RAM_ATTR const char	SAT_MO_NAME[] = "satellite";
EXT_RAM_ATTR const char		SATADDR_SUB_MO_NAME[] = "address";
EXT_RAM_ATTR const char		SATRXCRCERR_SUB_MO_NAME[] = "rxcrcerr";
EXT_RAM_ATTR const char		SATTXCRCERR_SUB_MO_NAME[] = "txcrcerr";
EXT_RAM_ATTR const char		SATWDERR_SUB_MO_NAME[] = "wderr";

EXT_RAM_ATTR const char	SENSOR_MO_NAME[] = "sensor";
EXT_RAM_ATTR const char		SENSPORT_SUB_MO_NAME[] = "port";
EXT_RAM_ATTR const char		SENSSENSING_SUB_MO_NAME[] = "sensing";
EXT_RAM_ATTR const char		SENSORPROPERTY_SUB_MO_NAME[] = "property";

EXT_RAM_ATTR const char	ACTUATOR_MO_NAME[] = "actuator";
EXT_RAM_ATTR const char		ACTUATORPORT_SUB_MO_NAME[] = "port";
EXT_RAM_ATTR const char		ACTUATORSHOWING_SUB_MO_NAME[] = "showing";
EXT_RAM_ATTR const char		ACTUATORPROPERTY_SUB_MO_NAME[] = "property";

/*==============================================================================================================================================*/
/* End MO definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO) CLI help texts                                                                                                    */
/* Purpose: Defines the managed objects (MOs/sub-MOs) help texts																				*/
/* Description:																																	*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR const char GLOBAL_HELP_HELP_TXT[] =					"Provides help text for a command or a particular managed sub-object (sub-MO)\n\r" \
																		"Type \"help cli\" to find out about the general CLI principles and concepts.\n\r" \
																		"Type \"show commands [-all] [-help] to see available commands\n\r";

EXT_RAM_ATTR const char GLOBAL_FULL_CLI_HELP_TXT[] =				"The CLI provides interactive management capabilities throughout it\'s " \
																		"management object types:\n\r" \
																		"- Global\n\r" \
																		"- Common\n\r" \
																		"- Decoder\n\r" \
																		"	- Lightgroup-link\n\r" \
																		"		- Lightgroup\n\r" \
																		"	- Satellite-link\n\r" \
																		"		- Satellite\n\r" \
																		"			- Actuator\n\r" \
																		"			- Sensor\n\r" \
																		"The managed objects are not directly exposed in the CLI, but are indirectly referred " \
																		"to by the current- or temporary- CLI context. A CLI context is a hierarchical " \
																		"name-space which can only operate on the given managed-object - and instance sub-managed " \
																		"objects, with two exeptions:\n\r" \
																		"- Global management objects are available in any CLI context, and operates on system wide objects\n\r" \
																		"- Common management objects are available for all CLI contexts and operates on the current/or indicated " \
																		"CLI context.\n\r" \
																		"Context paths can be absolute or rellative and follows the same pattern and semantics as th UNIX file system, " \
																		"I.e.: \"..\" to decend one level, .\".\" stay on the same level, \"\/\" separate sub-paths\n\r" \
																		"Active CLI context can be examined in two ways:\n\r" \
																		"- The current active CLI context path is shown at the CLI prompt: \"{absolute_context_path}>> \"\n\r" \
																		"- or you can retreive it from the command: \"get context\"\n\r" \
																		"Setting a new active CLI context is achieve by the command \"set context {absolute_context_path} | {rellative_context_path}\"\n\r"
																		"Temporary contexts represents a method to redirect a single command to a different CLI context than the current." \
																		"The temporary CLI context is limited to a single command and after the command has been executed the current "\
																		"CLI context remains as before. " \
																		"The temporary CLI context is prepended the CLI command sub-MO and can be an absolute or a rellative CLI context path:\n\r" \
																		"Ie: \"command [{absolute_context_path} | {rellative_context_path}] sub-MO arguments...\"\n\r" \
																		"To show the CLI context topology-, available CLI contexts- as well as current context, type \"show topology\"\n\r" \
																		"Available CLI commands:\n\r" \
																		"	help [{command} [{sub-MO}\n\r" \
																		"	reboot\n\r" \
																		"	show {sub-MO} [-flags...]\n\r" \
																		"	get {sub-MO} [-flags...]\n\r" \
																		"	set {sub-MO} {value} [-flags...]\n\r" \
																		"	unset {sub-MO} [-flags...]\n\r" \
																		"	clear {sub-MO} [-flags...]\n\r" \
																		"	add {sub-MO} [-flags...]\n\r" \
																		"	delete {sub-MO} [-flags...]\n\r" \
																		"	copy {sub-MO} [-flags...]\n\r" \
																		"	paste {sub-MO}\n\r" \
																		"	move {sub-MO} {dest} [-flags...]\n\r" \
																		"	start {sub-MO} [-flags...]\n\r" \
																		"	stop {sub-MO} [-flags...]\n\r" \
																		"	restart {sub-MO} [-flags...]\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_COMMANDS_HELP_TXT[] =				"Shows available commands and their MO belonging\n\r" \
																		"	- If the \"-all\" flag is provided, all possible CLI commands are shown, if it is not -\n\r" \
																		"	  only those commands available from the current CLI context are shown.\n\r" \
																		"	- \"-help\" Shows a truncated help text for each command\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/help																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/context																									*/
/* Purpose: Defines global CLI-context operations																								*/
/* Description:	CLI-context can be changed or viewed through following commands:																*/
/*				- set context...																												*/
/*				- get context...																												*/
/*				- show topology...																												*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR const char GLOBAL_GET_CONTEXT_HELP_TXT[] =				"Prints current CLI context\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_CONTEXT_HELP_TXT[] =				"Sets current context path. If {value} begins with \"/\" it is considered to be an absolute path, " \
																		"otherwise it is considered to be a relative path. \"..\" is used to ascend one level in " \
																		"the CLI context topology. \".\" is a NULL separator which does not change (ascend or descend) " \
																		"the CLI context path, i.e. \"./././path represents the same path as \"path\"\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_TOPOLOGY_HELP_TXT[] =			"Shows the CLI context topology tree.\n\r" \
																		"Current active context is highlighted with an \"<<<\" indication on the right side of the table.\n\r" \
																		"	- If the \"-childs\" flag is given, only the CLI context topology starting from the current CLI context junction and below is shown.\n\r";


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/context																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/runtime Sub-MOs																							*/
/* Purpose: Defines global run-time operations																									*/
/* Description:	Following runtime commands exists:																								*/
/*				- reboot...																														*/
/*				- get uptime...																													*/
/*				- show coredump...																												*/
/*				- start cpu																														*/
/*				- stop cpu																														*/
/*				- get cpu																														*/
/*				- show cpu																														*/
/*				- get memory																													*/
/*				- show memory																													*/
/*				- start memory																													*/
/*				- stop memory																													*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char GLOBAL_REBOOT_HELP_TXT[] =					"Reboots the decoder.\n\r" \
																		"	- If none of \"-panic\" or \"-exception\" is provided, the decoder will be rebooted through a normal software reset.\n\r" \
																		"	- If \"-panic {panic_message}\" is provided, an application panic with a panic message \"panic_message\" will cause the \n\r" \
																		"	  decoder to reboot.\n\r" \
																		"	- If \"-exception\" is provided, the decoder will be rebooted as a consequence of an illegal \"division-by-zero\" \n\r" \
																		"	  machine exception.\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_UPTIME_HELP_TXT[] =				"Prints the time in seconds since the previous boot.\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_COREDUMP_HELP_TXT[] =			"Prints the the previously stored core-dump.\n\r";

EXT_RAM_ATTR const char GLOBAL_START_CPU_HELP_TXT[] =				"Starts a CPU activity.\n\r" \
																		"	- If the \"-stats\" flag is provided, the collection of CPU- and Memory- statistics is started\n\r" \
																		"	  CPU- and memory- collection is a prerequisite for some of the statistics \n\r" \
																		"	  that can be provided by \"get cpu [-flags]\", \"show cpu\", \"get memory [-flags]\"\n\r" \
																		"	  and \"show memory\".\n\r" \
																		"	  CPU and memory collection of statistics can have a significant negative impact on the system performance\"\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_CPU_HELP_TXT[] =				"Stops an ongoing CPU activity.\n\r" \
																		"	- If the \"-stats\" flag is provided - ongoing collection of CPU- and Memory- statistics is stoped\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_CPU_HELP_TXT[] =					"get cpu [-tasks] [-task {taskName}] [-cpuusage] [-watermark] [-stats]\n\r" \
																		"	- get cpu: Is identical to \"show cpu\"\n\r" \
																		"	- get cpu -tasks: Prints task information for all running tasks\n\r" \
																		"	- get cpu -task {taskName}: Prints information for tasks \"taskName\"\n\r" \
																		"	- get cpu -cpuusage: Prints current and trending CPU-usage information, the CPU and memory statistics collection function needs to be active (\"set cpu -stats\") \n\r" \
																		"	- get cpu -watermark: Prints the High Watermark CPU-usage since CPU statistics collection was started\n\r" \
																		"	- get cpu -stats: Prints the state of the CPU and memory statistics collection function\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_CPU_HELP_TXT[] =				"show cpu: Shows a collection of CPU status- and statistics";

EXT_RAM_ATTR const char GLOBAL_GET_CPUMEM_HELP_TXT[] =				"Provides heap memory status and statistics\n\r" \
																		"	- If no flags are provided-, a summary of the global (Internal + SPIRAM) memory status is provided\n\r" \
																		"	- If the \"-internal\" flag is provided, the status of the internal on-chip RAM status is provided\n\r" \
																		"	  else the total combined memory statistics is shown\n\r" \
																		"	- \"-total\" : Provides the total installed memory capacity in bytes\n\r" \
																		"	- \"-available\": Prints currently available memory capacity in bytes\n\r" \
																		"	- \"-used\": Provides currently used memory capacity in bytes\n\r" \
																		"	- \"-watermark\": Prints the lowest available memory watermark in bytes\n\r" \
																		"	- \"-average {seconds}\": Prints the average memory usage in bytes over \"{value}\" seconds, the CPU and memory statistics collection function needs to be active - see \"start cpu -stats\"\n\r" \
																		"	- \"-trend {seconds}\": Prints the memory usage trend in bytes over \"{value}\" seconds, the CPU and memory statistics collection function needs to be active - see \"set cpu -stats\"\n\r" \
																		"	- \"-maxblock\" : Prints the maximum block of heap memory in bytes that can be allocated\n\r";

EXT_RAM_ATTR const char GLOBAL_START_CPUMEM_HELP_TXT[] =			"Starts a heap memory allocation activity.\n\r" \
																		"	- The \"-allocate {bytes}\" flag determines the test-buffer size to be allocated\n\r" \
																		"	- \"-internal\" Operates on internal memory segments.\n\r" \
																		"	- \"-external\" Operates on external SPI-RAM memory segments.\n\r" \
																		"	- \"-default\" Operates on memory segments as defined by the default OS memory allocation policy.\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_CPUMEM_HELP_TXT[] =				"Stops earlier started heap memory activities.\n\r" \
																	"	- If the \"-allocation\" flag is provided, memory earlier allocated by \"start memory -allocation {value}\" will be freed\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_CPUMEM_HELP_TXT[] =				"Shows a summary of the heap status and statistics, it is identical to \"get memory\"\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/runtime																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/network																									*/
/* Purpose: Defines global Network operations																									*/
/* Description:	Networking can be viewed and provisioned through the following commands:														*/
/*				- set network...																												*/
/*				- get network...																												*/
/*				- show network...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char GLOBAL_SET_NETWORK_HELP_TXT[] =				"Sets IP and WIFI networking parameters:\n\r" \
																	"Available flags:\n\r"
																	"	- \"-ssid {ssid}\": Sets the WiFi SSID\n\r" \
																	"	- \"-pass {pass}\": Sets the WiFi password\n\r" \
																	"	- \"-hostname {hostName}\": Sets the host name\n\r" \
																	"	- \"-address {address}\": Sets the host IPv4 network address\n\r" \
																	"	- \"-mask {networkIpMask}\": Sets the IPv4 network mask\n\r" \
																	"	- \"-gw {networkIpAddress}\": Sets the IPv4 network default gateway address\n\r" \
																	"	- \"-dns {dnsIpAddress}\": Sets the IPv4 DNS address\n\r" \
																	"	- \"-dhcp\": Sets DHCP operation where the network parameters are assigned from a DHCP server\n\r" \
																	"	- \"-persist\": Persists the network configuration\n\r" \
																	"If any of \"ssid\"-, or \"pass\"- are provided, The WiFi SSID and password will be statically reprovisioned, \n\r" \
																	"if one of the two parameters is not provided it will assume the same value as in the previous configuration\n\r" \
																	"If any of \"address\"-, \"mask\"-, \"gw\"-, or \"dns\"- are provided, a static IP address configuration will be set for all\n\r" \
																	"of those parameters, if any of those are not provided, that parameter will assume a static value from what it previously\n\r" \
																	"was set to be \(from previously set static configuration, or from previously DHCP assigned configuration\)\n\r" \
																	"If \"dhcp\" is set, all of \"address\"-, \"mask\"-, \"gw\"-, and \"dns\" will be assigned from the DHCP server.\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_NETWORK_HELP_TXT[] =				"Prints IP and WIFI networking parameters and statistics. \"get network\" without flags is identical to \"show network\"\n\r" \
																		"Available flags:\n\r"
																		"	- \"-ssid\": Prints the AP SSID connected to\n\r" \
																		"	- \"-bssid\": Prints the AP BSSID connected to\n\r" \
																		"	- \"-channel\": Prints the AP WiFi channel connected to\n\r" \
																		"	- \"-auth\": Prints current WiFi Autentication/Encryption method\n\r" \
																		"	- \"-rssi\": Prints current SNR/RSSI WiFi signal quality\n\r" \
																		"	- \"-mac\": Prints decoder WiFi MAC address\n\r" \
																		"	- \"-hostname\": Prints decoder host name\n\r" \
																		"	- \"-address\": Prints the host IPv4 network address\n\r" \
																		"	- \"-mask\": Prints the IPv4 network mask\n\r" \
																		"	- \"-gw\": Prints the IPv4 network default gateway address\n\r" \
																		"	- \"-dns\": Prints the IPv4 DNS address\n\r" \
																		"	- \"-opstate\": Prints current network operational state\n\r" \
																		"	- \"-scanap\": Scans available APs and prints the information about them\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_NETWORK_HELP_TXT[] =			"Shows a summary of network information. Identical to \"get network\" without flags.\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/network																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



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
EXT_RAM_ATTR const char GLOBAL_SET_WDT_HELP_TXT[] =					"set wdt [-debug] | [-id [-timeout {timeout_ms}] | [-action {escalation_ladder_actions}] | [-active {\"true\"|\"false\"}]]\n\r" \
																	"Sets various parameters of WDT given by the provided flags: \n\r" \
																	"Available flags:\n\r" \
																	"	- \"-debug\": Sets the debug flag, allowing intrusive Watchdog CLI commands\n\r" \
																	"	- \"-id {watchdog_id}\": The watchdog identity to be altered\n\r" \
																	"	- \"-timeout {timeout_ms}\": Sets the watchdog timeout in ms\n\r" \
																	"	- \"-action {LOC0|LOC1|LOC2|GLB0|GLBFSF|REBOOT|ESC_GAP}\": Sets the Watchdog escalation ladder actions\n\r" \
																	"				 \"LOC0\": Watchdog owner first local action\n\r" \
																	"				 \"LOC1\": Watchdog owner second local action\n\r" \
																	"				 \"LOC2\" Watchdog owner third local action\n\r" \
																	"				 \"GLB1\": Watchdog global action\n\r" \
																	"				 \"GLBFSF\": Watchdog global fail-safe action\n\r" \
																	"				 \"REBOOT\": Watchdog global reboot action\n\r" \
																	"				 \"ESC_GAP\": Escalation gap, each escalation action is intergaped by one watchdog timer tick\n\r" \
																	"				 The escalation ladder actions can be combined in any way by separating them with \"|\"\n\r" \
																	"				 but the escalation ladder order always remains the same - from \"LOC0\" towards \"REBOOT\" \n\r" \
																	"	- \"-active {\"True\"|\"False\"}\": activates or inactivates the Watchdog\n\r";

EXT_RAM_ATTR const char GLOBAL_UNSET_WDT_HELP_TXT[] =				"unset wdt -debug \n\r" \
																	"Un-sets various parameters of WDT given by the provided flags: \n\r" \
																	"Available flags:\n\r" \
																		"	- \"-debug\": Un-sets the debug flag, dis-allowing intrusive Watchdog CLI commands \n\r";

EXT_RAM_ATTR const char GLOBAL_GET_WDT_HELP_TXT[] =					"get wdt [-id {wd-id}[-description]|[-active]|[-inhibited]|[-timeout]|[-action]|[-expires]|[-closesthit]]\n\r" \
																	"Prints Watchdog parameters and statistics. \"get wdt\" without flags is identical to \"show wdt\n\r" \
																	"\"get wdt -id {wd_id}\" without any other flags shows all parameters and statistics for that specific Watchdog id\n\r" \
																	"Available flags:\n\r" \
																	"	- \"-id\": Specifies the WDT id for which information is requested\n\r" \
																	"	- \"-description\": Prints the WDT description\n\r" \
																	"	- \"-active\": Shows if the WDT is active or not\n\r" \
																	"	- \"-inhibited\": Shows if the WDT is inhibited or not - I.e. from a \"stop wdt\" command\n\r" \
																	"	- \"-timeout\": Prints the WDT expiration value [ms]\n\r" \
																	"	- \"-actions\": Shows requested and existing expiration actions {FAULTACTION_ESCALATE_INTERGAP | FAULTACTION_LOCAL0 | FAULTACTION_LOCAL1 | FAULTACTION_LOCAL2 | FAULTACTION_GLOBAL_FAILSAFE | FAULTACTION_GLOBAL_REBOOT\n\r" \
																	"	- \"-expires\": Number of WDT expires\n\r" \
																	"	- \"-closesthit\": Closest time to a WDT expiry [ms]\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_WDT_HELP_TXT[] =				"show wdt\n\r" \
																	"Shows a summary of the WDT information. Is identical to \"get wdt\" without flags\n\r";

EXT_RAM_ATTR const char GLOBAL_CLEAR_WDT_HELP_TXT[] =				"clear wdt [-id {wd_id}]|-allstats|-expires|-closesthit\n\r" \
																	"Clears certain WDT statistics\n\r" \
																	"	- \"-id\": Specifies the WDT id for which a statistics object should be cleared, if omitted the below given statistics object for all WDTs will be cleared\n\r" \
																	"	- \"-allstats\": Clears all statistics\n\r" \
																	"	- \"-expires\": The WDT expires counter will be cleared\n\r" \
																	"	- \"-closesthit\": The WDT closest hit statistics will be cleared\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_WDT_HELP_TXT[] =				"stop wdt [-id {wd_id}]\n\r" \
																	"Stops the reception of WDT feeds, ultimately resulting in a WDT expiry and escalation ladder, \n\r" \
																	"this command requires the WDT debug flag to be set - see \"set wdt -debug\"\n\r" \
																	"Available flags:\n\r" \
																	"	- \"-id\": Specifies the WDT id for which the feeding should be stopped, if id is omitted - \n\r" \
																	"	           all WDTs will be starved\n\r";

EXT_RAM_ATTR const char GLOBAL_START_WDT_HELP_TXT[] =				"start wdt [-id {wd_id}]\n\r" \
																	"Starts the reception of WDT feeds which was earlier stopped \n\r" \
																	"this command requires the WDT debug flag to be set - see \"set wdt -debug\"\n\r" \
																	"Available flags:\n\r" \
																	"	- \"-id\": Specifies the WDT id for which the feeding should start, if id is omitted - \n\r" \
																	"	           all WDTs will be started";
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/wdt																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/job																										*/
/* Purpose: Defines global Job managed objects																									*/
/* Description:	Provides means to monitor and manage registered Job queue objects																*/
/*              such as:																														*/
/*				- set job...																													*/
/*				- unset job...																													*/
/*				- get job...																													*/
/*				- show job...																													*/
/*				- clear job...																													*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR const char GLOBAL_SET_JOB_HELP_TXT[] = "set job [-debug] | [-id {job_Id} [-priority {priority 1-24}]]\n\r" \
"Sets various parameters of a Job queue given by the provided flags: \n\r" \
"Available flags:\n\r" \
"	- \"-debug\": Sets the debug flag, allowing intrusive Job CLI commands\n\r" \
"	- \"-id {job_Id}\": The Job identity to be altered\n\r" \
"	- \"-priority {jobTaskPriority 0-24}\": Sets the job task priority\n\r";

EXT_RAM_ATTR const char GLOBAL_UNSET_JOB_HELP_TXT[] = "unset job [-debug] \n\r" \
"Un-sets various parameters of Job given by the provided flags: \n\r" \
"Available flags:\n\r" \
"	- \"-debug\": Un-sets the debug flag, dis-allowing intrusive Job CLI commands\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_JOB_HELP_TXT[] = "get job [-id {job_Id} [-description]|[-maxjobs]|[-currentjobs]|[-peakjobs]|[-averagejobs]|[-peaklatency]|[-averagelatency]|[-peakexecution]|[-averageexecution]|[-priority]|[-priority]|[-overloaded]|[-overloadcnt]|[-wdtid]|[-tasksort]]\n\r" \
"Prints Job queue parameters and statistics. \"get job\" without flags is identical to \"show job\"\n\r" \
"\"get job -id {job_Id}\" without any other flags shows all parameters and statistics for that specific Job id\n\r" \
"Available flags:\n\r" \
"	- \"-id\": Specifies the Job id for which information is requested\n\r" \
"	- \"-description\": Prints the Job description, same as the job task name\n\r" \
"	- \"-maxjobs\": Shows the total maximum of Job slots/queue depth available\n\r" \
"	- \"-currentjobs\": Shows currently queued/pending Jobs\n\r" \
"	- \"-peakjobs\": Shows the peak/maximum number of jobs that have ever been pending in the job queue\n\r" \
"	- \"-averagejobs\": Shows the average number of jobs that have been pending in the job queue for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-peaklatency\": Shows the peak/maximum latency(uS) that a job has been in queue until start of execution\n\r" \
"	- \"-averagelatency\": Shows the average latency time(uS) that jobs have been in queue until start of execution for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-peakexecution\": Shows the peak/maximum job execution time(uS) from when a job was dequeued from the job buffer until finished\n\r" \
"	- \"-averageexecution\": Shows the average job execution time(uS) from when a job was dequeued from the job buffer until finished for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-priority\": Shows the Job task base priority\n\r" \
"	- \"-overloaded\": Shows the current job queue overload status\n\r" \
"	- \"-overloadcnt\": Shows the number of job queue overload instances\n\r" \
"	- \"-wdtid\": Shows related Watchdog Id for the job supervision\n\r" \
"	- \"-tasksort\": Tasksorting - when task sorting is disabled (\"False\") strict first-in first-served global policy applies\n\r" \
"					 if not, a sorting policy keeping jobs enqueued from same tasks is applied: I.e. strict first-in first-served per\n\r" \
"					 job enqueueing origin task\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_JOB_HELP_TXT[] = "show job\n\r" \
"Shows a summary of the Job queue information. Is identical to \"get job\" without flags\n\r";

EXT_RAM_ATTR const char GLOBAL_CLEAR_JOB_HELP_TXT[] = "clear job [-id {job_Id}] [-allStats]|[-peakjobs]|[-averagejobs]|[-peaklatency]|[-averagelatency]|[-peakexecution]|[-averageexecution]|[-overloadcnt]]\n\r" \
"Clears certain Job statistics\n\r" \
"Available flags:\n\r" \
"	- \"-id\": Specifies the Job id for which a statistics object should be cleared, if omitted the statistics object will be cleared for all Jobs\n\r" \
"	- \"-allstats\": Clears all statistics\n\r" \
"	- \"-peakjobs\": Clears stats of the peak/maximum number of jobs that has ever been pending in the job queue\n\r" \
"	- \"-averagejobs\": Clears stats of the average number of jobs that has been pending in the job queue for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-peaklatency\": Clears stats of the peak/maximum latency(uS) that a job has been in queue until start of execution\n\r" \
"	- \"-averagelatency\": Clears stats of the average latency time(uS) that jobs have been in queue until start of execution for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-peakexecution\": Clears stats of the peak/maximum job execution time(uS) from when a job was dequeued from the job buffer until finished\n\r" \
"	- \"-averageexecution\": Clears stats of the average job execution time(uS) from when a job was dequeued from the job buffer until finished for the past " JOB_STAT_CNT_STR " job invocations\n\r" \
"	- \"-overloadcnt\": Clears stats of the number of job queue overload instances\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/job																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/mqtt																										*/
/* Purpose: Provides means to provision and view MQTT																							*/
/* Description:	Following MQTT CLI commands exist: 																								*/
/*				- set mqtt...																													*/
/*				- get mqtt...																													*/
/*				- show mqtt...																													*/
/*				- clear job...																													*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char GLOBAL_SET_MQTT_HELP_TXT[] =				"Sets MQTT parameters.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-uri {mqtt_broker_uri | mqtt_broker_IPv4Address}\": Sets MQTT Broker URI to \"mqtt_broker_uri\"URI or \"mqtt_broker_IPv4Address\"\n\r" \
																		"	- \"-port {mqtt_broker_port}\": Sets MQTT Broker port to \"mqtt_broker_port\"\n\r" \
																		"	- \"-clientid {mqtt_client_id}\": Sets MQTT client ID to \"mqtt_client_id\"\n\r" \
																		"	- \"-qos {mqtt_qos}\": Sets MQTT default QoS to \"mqtt_qos\", i.e. 0,1 or 2.\n\r" \
																		"	- \"-keepalive {mqttKeepAlive_s}\": Sets MQTT keepalive period to \"mqttKeepAlive_s\" seconds\n\r" \
																		"	- \"-ping {mqtt_e2e_ping_period_s}\": Sets MQTT overlay server-client MQTT ping period to \"mqtt_e2e_ping_period_s\" seconds, \n\r" \
																		"	  This represents the supervision period between the server and the decoders.\n\r" \
																		"	- \"-persist\": Persists MQTT configuration of Broker URI and Broker port\n\r";

EXT_RAM_ATTR const char GLOBAL_CLEAR_MQTT_HELP_TXT[] =				"Clear MQTT statistics.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-maxlatency\": Clears MQTT max-latency statistics\n\r" \
																		"	- \"-overruns\": Clears MQTT overrun statistics\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_MQTT_HELP_TXT[] =				"Prints MQTT parameters and statistics. \"get mqtt\" without flags is identical to \"show mqtt\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-uri\": Print MQTT broker URI or IPv4 address\n\r" \
																		"	- \"-port\": Prints the MQTT broker port\n\r" \
																		"	- \"-clientid\": Prints the MQTT client identifier\n\r" \
																		"	- \"-qos\": Prints the MQTT client default Quality of Service class\n\r" \
																		"	- \"-keepalive\": Prints the MQTT keep-alive period in seconds\n\r" \
																		"	- \"-ping\": Prints the server-decoder ping period in seconds\n\r" \
																		"	- \"-maxlatency\": Prints the MQTT poll loop max latency in uS\n\r" \
																		"	- \"-meanlatency\": Prints the MQTT poll loop mean latency in uS\n\r" \
																		"	- \"-overruns\": Prints the MQTT poll loop overrun counter\n\r" \
																		"	- \"-opstate\": Prints the MQTT client operational state\n\r" \
																		"	- \"-subscriptions\": Prints current subscriptions and call backs.";

EXT_RAM_ATTR const char GLOBAL_SHOW_MQTT_HELP_TXT[] =				"Shows a summary of the MQTT information. Is identical to \"get mqtt\" without flags\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/mqtt																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/time																										*/
/* Purpose: Provides means to provision and view time																							*/
/* Description:	Following time CLI commands exist: 																								*/
/*				- add time...																													*/
/*				- delete time...																												*/
/*				- start time...																													*/
/*				- stop time...																													*/
/*				- set time...																													*/
/*				- get time...																													*/
/*				- show time...																													*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char GLOBAL_ADD_TIME_HELP_TXT[] =				"Adds an NTP server.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-ntpserver {ntp_server_uri | ntp_server_IPv4Address}\": NTP server URI or IPv4 address\n\r" \
																		"	- \"-ntpport{ntp_server_port}\": NTP server port, if a port is not given, the default NTP port : 123 will be used\n\r";

EXT_RAM_ATTR const char GLOBAL_DELETE_TIME_HELP_TXT[] =				"Deletes an NTP server.\n\r" \
																		"Available flags:\n\r" \
																		"- \"-ntpserver{ntp_server_uri | ntp_server_IPv4Address}\": Deletes a previously provisioned NTP server with URI : \"ntp_server_uri\" or IPv4 IP address: \"ntp_server_IPv4Address\"\n\r";

EXT_RAM_ATTR const char GLOBAL_START_TIME_HELP_TXT[] =				"Starts time services.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"{-ntpclient [-ntpdhcp]}\": Starts the NTP client, if \"-ntpdhcp\" is provided, the ntp server information is taken from DHCP option 042\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_TIME_HELP_TXT[] =				"Stops time services.\n\r" \
																	"Available flags:\n\r" \
																		"	- \"-ntpclient\": Stops the NTP client\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_TIME_HELP_TXT[] =				"Sets the system time.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-timeofday {timeOfDay} | -tod {timeOfDay}\": Sets time of day in UTC, \"timeOfDay\" format: \"YYYY-MM-DDTHH:MM:SS\"\n\r" \
																		"	- \"-epochtime {epochTime_s}\": Sets Epoch time, \"epochTime_s\" format: NNNNNN - seconds since Jan 1 1970 UTC\n\r" \
																		"	- \"-timezone {timeZone_h}\": Sets the timezone, \"timeZone_h\" format (-)NN houres, NN <= 12, E.g. \"CET+1\".\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_TIME_HELP_TXT[] =				"Prints the system time. \"get time\" without flags is identical to \"show time\".\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-timeofday | -tod [-utc]\": Prints the local or universal time\n\r" \
																		"	- \"-epochtime\": Prints the epoch time - seconds since Jan 1 1970 UTC\n\r" \
																		"	- \"-timezone\": Prints current time-zone\n\r" \
																		"	- \"-daylightsaving\": Prints daylight-saving status\n\r" \
																		"	- \"-ntpsdhcp\": Prints the NTP DHCP Option 042 status\n\r" \
																		"	- \"-ntpservers\": Prints the current status of all provisioned NTP servers\n\r" \
																		"	- \"-ntpsyncstatus\": Prints the current NTP client sync status\n\r" \
																		"	- \"-ntpsyncmode\": Prints the current NTP client sync mode\n\r" \
																		"	- \"-ntpopstate\": Prints the current NTP client operational state\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_TIME_HELP_TXT[] =				"Shows a summary of the time services. Identical to \"get time\" without flags.\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/time																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): global/log																										*/
/* Purpose: Provides means to provision logging																									*/
/* Description:	Following log CLI commands exist: 																								*/
/*				- set log...																													*/
/*				- unset log...																													*/
/*				- clear log...																													*/
/*				- get log...																													*/
/*				- add log...																													*/
/*				- delete log...																													*/
/*				- show log...																													*/
/*				- show log...																													*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char GLOBAL_SET_LOG_HELP_TXT[] =					"Sets logg properties\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-loglevel {log_level}\" Sets the global log level, the global log level should be one of: \n\r" \
																		"			should be one of \"DEBUG-SILENT\"|\"DEBUG-PANIC\"|\"DEBUG-ERROR\"|\"DEBUG-WARN\"|\"DEBUG-INFO\"|\"DEBUG-TERSE\"|\"DEBUG-VERBOSE\"\n\r" \
																		"	- \"-logdestination {rsys_log_uri}\" Sets the RSyslog destination URI and starts logging to it\n\r" \
																		"	- \"-logconsole\" Enables console logging\r\n";

EXT_RAM_ATTR const char GLOBAL_UNSET_LOG_HELP_TXT[] =				"Unsets log properties\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-logdestination\" Removes the RSyslog destination(s) and stops logging to it\n\r" \
																		"	- \"-logconsole\" Stops logging to the console\n\r";

EXT_RAM_ATTR const char GLOBAL_CLEAR_LOG_HELP_TXT[] =				"Clears various log metrics\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-missedlogs\" Clears the missed log entries counter\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_LOG_HELP_TXT[] =					"Prints various log parameters and metrics, \"get log\" without flags is identical to \"show log\"\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-loglevel\" Prints the current global log level\n\r" \
																		"	- \"-logdestination\" Prints the current Rsyslog log destination\n\r" \
																		"	- \"-customlogitems\" Prints custom log items with their respective log levels\n\r" \
																		"	- \"-missedlogs\" Prints the aggregated sum of missed log entries that have been pruned due to \n\r" \
																		"			overload or lack of resources, this counter can be reset by \"clear log -missedlogs\"\n\r";

EXT_RAM_ATTR const char GLOBAL_ADD_LOG_HELP_TXT[] =					"Adds logg items\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-customlogitem -customloglevel {custom_log_level} -customlogfile {custom_src_file_name} [-customlogfunc {custom_src_fun_name}]\"\n\r" \
																		"	  Adds a custom log item for a specific source file name - \n\r" \
																		"	  and an optional function name, with a custom log level, the log level value\n\r" \
																		"	  should be one of \"DEBUG-SILENT\"|\"DEBUG-PANIC\"|\"DEBUG-ERROR\"|\"DEBUG-WARN\"|\n\r" \
																		"	  \"DEBUG-INFO\"|\"DEBUG-TERSE\"|\"DEBUG-VERBOSE\"\n\r" \
																		"	  A custom log item allows to alter the log verbosity for certain parts of the source code,\n\r" \
																		"	  enabling fine grained debugging for that specific part without having to increase the global\n\r" \
																		"	  log verbosity, potentially increasing system load, or loosing log items\n\r";

EXT_RAM_ATTR const char GLOBAL_DELETE_LOG_HELP_TXT[] =				"Deletes logg items\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-customlogitem -all\" Deletes all custom log items\n\r" \
																		"	- \"-customlogitem -customlogfile {custom_src_file_name} [-customlogfunc {custom_src_fun_name}]\"\n\r" \
																		"	  Deletes the custom log item: “custom_src_file_name”::”custom_src_fun_name”\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_LOG_HELP_TXT[] =				"Shows a summary of log information. Identical to \"get log\" without flags\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): global/log																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): common/debug																										*/
/* Purpose: Provides means to provision the debug-flags																							*/
/* Description:	Following debug CLI commands exist: 																							*/
/*				- set debug...																													*/
/*				- unset debug...																												*/
/*				- get debug...																													*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char COMMON_SET_DEBUG_HELP_TXT[] =				"Sets the debug state of the MO, this enables setting of MO/Sub-MO attributes which may lead to system inconsistency.\n\r";

EXT_RAM_ATTR const char COMMON_UNSET_DEBUG_HELP_TXT[] =				"Un-sets the debug state of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_GET_DEBUG_HELP_TXT[] =				"Prints the debug state of the MO.\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): common/debug																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): common/opstate																									*/
/* Purpose: Provides means to view opstates																										*/
/* Description:	Following opstate CLI commands exist: 																							*/
/*				- get opstate...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char COMMON_GET_OPSTATE_HELP_TXT[] =				"Prints the Operational state of the MO.\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): common/opstate																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): common/sysname, username, description																			*/
/* Purpose: Provides means to view opstates																										*/
/* Description:	Following opstate CLI commands exist: 																							*/
/*				- set sysname...																												*/
/*				- get sysname...																												*/
/*				- set username...																												*/
/*				- get username...																												*/
/*				- set description...																											*/
/*				- get description...																											*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char COMMON_GET_SYSNAME_HELP_TXT[] =				"Prints \"systemname\" of the current Managed Object(MO)/CLI-context.\n\r";

EXT_RAM_ATTR const char COMMON_SET_SYSNAME_HELP_TXT[] =				"Sets the \"systemname\" of the current Managed Object(MO)/CLI-context.\n\r" \
																	"The\"debug-flag\" of the current Managed Object/CLI-context needs to be active to set the \"systemname\" -\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\"\n\r" \
																	"Attention: The System name of a Managed Object is normally immutable, changing it from the CLI\n\r" \
																	"           may result in unpredictive consequences and side effects.\n\r";

EXT_RAM_ATTR const char COMMON_GET_USRNAME_HELP_TXT[] =				"Prints the \"username\" of the current Managed Object(MO)/CLI-context.\n\r";

EXT_RAM_ATTR const char COMMON_SET_USRNAME_HELP_TXT[] =				"Sets the \"username\" of the current Managed Object(MO)/CLI-context.\n\r" \
																	"The \"debug-flag\" of the current Managed Object / CLI - context needs to be active to set the \"username\" -\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r" \
																	"Attention: The User name of a Managed Object is normally mutable, changing it from the CLI will not however\n\r" \
																	"           synchronize the new User name with the MyRailIO server and many cause inconsistencies\n\r";

EXT_RAM_ATTR const char COMMON_GET_DESCRIPTION_HELP_TXT[] =			"Prints the \"description\" of the current Managed Object(MO)/CLI-context.\n\r";

EXT_RAM_ATTR const char COMMON_SET_DESCRIPTION_HELP_TXT[] =			"Sets the \"description\" of the current Managed Object(MO)/CLI-context.\n\r" \
																	"The \"debug-flag\" of the current Managed Object / CLI - context needs to be active to set the \"description\" -\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r" \
																	"Attention: The Description of a Managed Object is normally mutable, changing it from the CLI will not however\n\r" \
																	"           synchronize the new Description with the MyRailIO server and many cause inconsistencies\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): common/sysname, username, description																		*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): common/failsafe																									*/
/* Purpose: Provides means to set failsafe																										*/
/* Description:	Following failsafe CLI commands exist: 																							*/
/*				- set failsafe...																												*/
/*				- get failsafe...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char DECODER_GET_FAILSAFE_HELP_TXT[] =			"get failsafe: Prints the current fail-safe state\n\r";

EXT_RAM_ATTR const char DECODER_SET_FAILSAFE_HELP_TXT[] =			"set failsafe: Sets/activates fail-safe state, the \"debug-flag\" needs to be activated before fail-safe can be activated by CLI,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char DECODER_UNSET_FAILSAFE_HELP_TXT[] =			"unset failsafe: Unsets/deactivates fail-safe state. the \"debug-flag\" needs to be activated before fail-safe can be de-activated by CLI,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): common/failsafe																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/


/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): lglink/*																											*/
/* Purpose: Provides means to provision and view lglinks																						*/
/* Description:	Following lglink CLI commands exist: 																							*/
/*				- set link...																													*/
/*				- get link...																													*/
/*				- get overruns...																												*/
/*				- clear overruns...																												*/
/*				- get meanlatency...																											*/
/*				- get maxlatency...																												*/
/*				- clear maxlatency...																											*/
/*				- get meanruntime...																											*/
/*				- get maxruntime...																												*/
/*				- clear maxruntime...																											*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINK_HELP_TXT[] =			"get link: Prints the lgLink number\n\r";

EXT_RAM_ATTR const char LGLINKNO_SET_LGLINK_HELP_TXT[] =			"set link {lgLink_number}: Sets the lgLink number - the \"debug-flag\" needs to be activated to perform this action, \n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKOVERRUNS_HELP_TXT[] =	"get overruns: Prints the accumulated number of lgLink over-runs, I.e. for which the lgLink scan had not finished before the next was due\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKOVERRUNS_HELP_TXT[] =	"clear overruns: Clears the counter for accumulated lgLink over-runs\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMEANLATENCY_HELP_TXT[] = "get meanlatency: Prints the mean latency of the lgLink scan, E.g. how much its start was delayed compared to schedule\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMAXLATENCY_HELP_TXT[] =	"get maxlatency: Prints the maximum latency watermark of the lgLink scan, E.g. how much its start was delayed compared to schedule\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKMAXLATENCY_HELP_TXT[] = "clear maxlatency: Clears the maximum latency watermark of the lgLink scan\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMEANRUNTIME_HELP_TXT[] = "get meanruntime: Prints the mean run-time of the lgLink scan, E.g. how long the lgLink scan took\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMAXRUNTIME_HELP_TXT[] =	"get maxruntime: Prints the maximum run-time of the lgLink scan, E.g. how long the lgLink scan took\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKMAXRUNTIME_HELP_TXT[] = "clear maxruntime: Clears the maximum run-time watermark of the lgLink scan\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): lglink/*																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): lightgroup/*																										*/
/* Purpose: Provides means to provision and view lightgroups																					*/
/* Description:	Following lightgroup CLI commands exist: 																						*/
/*				- set address...																												*/
/*				- get address...																												*/
/*				- set ledcnt...																													*/
/*				- get ledcnt...																													*/
/*				- set ledoffset...																												*/
/*				- get ledoffset...																												*/
/*				- set propert...																												*/
/*				- get propert...																												*/
/*				- set showing...																												*/
/*==============================================================================================================================================*/


EXT_RAM_ATTR const char LG_GET_LGADDR_HELP_TXT[] =					"get address: Prints the LgLink address of the LightGroup\n\r";

EXT_RAM_ATTR const char LG_SET_LGADDR_HELP_TXT[] =					"set address: Sets the LgLink address of the LightGroup, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char LG_GET_LGLEDCNT_HELP_TXT[] =				"get ledcnt: Prints the number of LEDs/pixels for the LightGroup\n\r";

EXT_RAM_ATTR const char LG_SET_LGLEDCNT_HELP_TXT[] =				"set ledcnt {ledcnt}: Sets the number of LEDs/pixels for the LightGroup, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char LG_GET_LGLEDOFFSET_HELP_TXT[] =				"get ledoffset: Prints the LightGroup LED offset on the LgLink\n\r";

EXT_RAM_ATTR const char LG_SET_LGLEDOFFSET_HELP_TXT[] =				"set ledoffset {led_offset}: Sets the LightGroup LED offset on the LgLink, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char LG_GET_LGPROPERTY_HELP_TXT[] =				"get property [{property_index}]: Prints the LightGroup properties, if \"property_index\" is provided, the property corresponding to this index is printed, else all properties are printed\n\r";

EXT_RAM_ATTR const char LG_SET_LGPROPERTY_HELP_TXT[] =				"set property {property_index} {property_value}: Sets the LightGroup property according to given property index and value, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char LG_GET_LGSHOWING_HELP_TXT[] =				"get showing: Prints the LightGroup's current showing\n\r";

EXT_RAM_ATTR const char LG_SET_LGSHOWING_HELP_TXT[] =				"set showing {aspect}: Sets the LightGroup's current aspect to be shown, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r" \
																	"	Valid aspects depend on the LightGroup type/sub-type.\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): lightgroup/*																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): satellitelink/*																									*/
/* Purpose: Provides means to provision and view satellitelinks																					*/
/* Description:	Following satellitelink CLI commands exist: 																					*/
/*				- set link...																													*/
/*				- get link...																													*/
/*				- get txunderruns...																											*/
/*				- clear txunderruns...																											*/
/*				- get rxoverruns...																												*/
/*				- clear rxoverruns...																											*/
/*				- get timingviolation...																										*/
/*				- clear timingviolation...																										*/
/*				- get rxcrcerr...																												*/
/*				- clear rxcrcerr...																												*/
/*				- get remotecrcerr...																											*/
/*				- clear remotecrcerr...																											*/
/*				- get rxsymbolerr...																											*/
/*				- clear rxsymbolerr...																											*/
/*				- get rxsizeerr...																												*/
/*				- clear rxsizeerr...																											*/
/*				- get wderr...																													*/
/*				- clear wderr...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char SATLINK_GET_SATLINKNO_HELP_TXT[] =			"get link: Prints the Satellite-link number\n\r";

EXT_RAM_ATTR const char SATLINK_SET_SATLINKNO_HELP_TXT[] =			"set link {satellite_link_no}: Sets the Satellite-link number, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char SATLINK_GET_TXUNDERRUNS_HELP_TXT[] =		"get txunderrun: Prints number of Satellite-link TX underruns,\n\r" \
																	"	I.e. number of occurrences the TX link scan could not be served in a timely manner.\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_TXUNDERRUNS_HELP_TXT[] =		"clear txunderrun: Clears the Satellite-link TX underruns counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXOVERRUNS_HELP_TXT[] =			"get rxoverrun: Prints number of Satellite-link RX overruns,\n\r"\
																	"	I.e. number of occurrences the RX link scan could not be served in a timely manner.\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXOVERRUNS_HELP_TXT[] =		"clear rxoverrun: Clears the Satellite-link RX overruns counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_TIMINGVIOLATION_HELP_TXT[] =	"get timingviolation: Prints number of Satellite-link timingviolations,\n\r" \
																	"	I.e. link scans which did not finish in time \n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_TIMINGVIOLATION_HELP_TXT[] =	"clear timingviolation: Clears the Satellite-link timingviolation counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXCRCERR_HELP_TXT[] =			"get rxcrcerr: Prints number of Satellite-link RX CRC errors,\n\r" \
																	"	I.e. number of CRC errors detected at the RX far end of the link.\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXCRCERR_HELP_TXT[] =			"clear rxcrcerr: Clears the Satellite-link RX CRC error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_REMOTECRCERR_HELP_TXT[] =		"get remotecrcerr: Prints the number of Satellite-link remote CRC errors,\n\r"
																	"	I.e. the aggregate number of CRC errors reported from each Satellite's RX interface.\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_REMOTECRCERR_HELP_TXT[] =		"clear remotecrcerr: Clears the Satellite-link RX CRC error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXSYMERRS_HELP_TXT[] =			"get rxsymbolerr: Prints number of Satellite-link RX symbol errors,\n\r"
																	"	I.e. layer-1 symbol errors detected at the RX far end of the link.\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXSYMERRS_HELP_TXT[] =		"clear rxsymbolerr: Clears the Satellite-link RX symbol error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXSIZEERRS_HELP_TXT[] =			"get rxsizeerr: Prints number of Satellite-link RX size errors,\n\r" \
																	"	I.e. occasions where the datagram size has been detected invalid at the RX far end of the link\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXSIZEERRS_HELP_TXT[] =		"clear rxsizeerr: Clears the Satellite-link RX size error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_WDERRS_HELP_TXT[] =				"get wderr: Prints number of Satellite-link watchdog errors, I.e aggregated watchdog error reported by Satellites on the Satellite-link\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_WDERRS_HELP_TXT[] =			"clear wderr: Clears the Satellite-link watchdog error counter\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): satellitelink/*																								*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): satellite/*																										*/
/* Purpose: Provides means to provision and view satellites																						*/
/* Description:	Following satellite CLI commands exist: 																						*/
/*				- set address...																												*/
/*				- get address...																												*/
/*				- get rxcrcerr...																												*/
/*				- clear rxcrcerr...																												*/
/*				- get txcrcerr...																												*/
/*				- clear txcrcerr...																												*/
/*				- get wderr...																													*/
/*				- clear wderr...																												*/
/*==============================================================================================================================================*/


EXT_RAM_ATTR const char SAT_GET_SATADDR_HELP_TXT[] =				"get address: Prints the Satellite address\n\r";

EXT_RAM_ATTR const char SAT_SET_SATADDR_HELP_TXT[] =				"set address: Sets the Satellite address, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r" \
																	"Note that the Satellite address is counted from the RX far end of the Satellite link - towards the TX side,\n\r" \
																	"	and it is 0 - numbered.\n\r";

EXT_RAM_ATTR const char SAT_GET_SATRXCRCERR_HELP_TXT[] =			"get rxcrcerr: Prints Satellite RX CRC errors,\n\r" \
																	"	I.e. CRC errors encountered for the datagram bound for the Satellite.\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATRXCRCERR_HELP_TXT[] =			"clear rxcrcerr: Clears the Satellite RX CRC error counter\n\r";

EXT_RAM_ATTR const char SAT_GET_SATTXCRCERR_HELP_TXT[] =			"get txcrcerr: Prints Satellite TX CRC errors\n\r" \
																	"	I.e. CRC errors detected on the satelite-link RX far-end,\n\r" \
																	"	belonging to datagrams supposedly sent from this Satellite.\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATTXCRCERR_HELP_TXT[] =			"clear txcrcerr: Clears the Satellite TX CRC error counter\n\r";

EXT_RAM_ATTR const char SAT_GET_SATWDERR_HELP_TXT[] =				"get wderr: Prints Satellite watchdog errors, \n\r" \
																	"	I.e. occasions when a satellite scan update hasn’t been received in due time.\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATWDERR_HELP_TXT[] =				"clear wderr: Clears the Satellite watchdog error counter\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): satellite/*																									*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): sensor/*																											*/
/* Purpose: Provides means to provision and view sensors																						*/
/* Description:	Following sensor CLI commands exist: 																							*/
/*				- set port...																													*/
/*				- get port...																													*/
/*				- get sensing...																												*/
/*				- set property...																												*/
/*				- get property...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char SENS_GET_SENSPORT_HELP_TXT[] =				"get port: Prints the Sensor port number\n\r";

EXT_RAM_ATTR const char SENS_SET_SENSPORT_HELP_TXT[] =				"set port {sensor_port}: Sets the Sensor port number, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char SENS_GET_SENSING_HELP_TXT[] =				"get sensing: Prints current Sensor state\n\r";

EXT_RAM_ATTR const char SENS_GET_SENSORPROPERTY_HELP_TXT[] =		"get property [{property_index}] : Prints the Sensor properties, if property index is provided the property corresponding to the index is printed, else all properties are printed\n\r";

EXT_RAM_ATTR const char SENS_SET_SENSORPROPERTY_HELP_TXT[] =		"set property {property_index} {property_value}: Sets the Sensor property according to given property index and value, the \"debug-flag\" needs to be activated to perform this action\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): sensor/*																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO): actuator/*																										*/
/* Purpose: Provides means to provision and view actuators																						*/
/* Description:	Following actuator CLI commands exist: 																							*/
/*				- set port...																													*/
/*				- get port...																													*/
/*				- get sensing...																												*/
/*				- set property...																												*/
/*				- get property...																												*/
/*==============================================================================================================================================*/

EXT_RAM_ATTR const char ACT_GET_ACTPORT_HELP_TXT[] =				"get port: Prints the Actuator port\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTPORT_HELP_TXT[] =				"set port {actuator_port}: Sets the actuator port, the \"debug-flag\" needs to be activated to perform this action,\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";

EXT_RAM_ATTR const char ACT_GET_ACTSHOWING_HELP_TXT[] =				"get showing: Prints the actuator showing state\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTSHOWING_HELP_TXT[] =				"set showing {actuator_showing}: Sets the actuator showing state, the \"debug-flag\" needs to be activated to perform this action\n\r" \
																	"	Showing state is a string that depends on the type of actuator.\n\r";

EXT_RAM_ATTR const char ACT_GET_ACTPROPERTY_HELP_TXT[] =			"get property [{property_index}] : Prints the Actuator properties, if property index is provided the property corresponding to the index is provided, else all properties are provided\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTPROPERTY_HELP_TXT[] =			"set property {property_index} {property_value}: Sets the Actuator property according to given property index and value, the \"debug-flag\" needs to be activated to perform this action\n\r" \
																	"use \"set debug\" to enable the \"debug-flag\".\n\r";
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* END Managed object (MO/Sub-MO): actuator/*																										*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/*==============================================================================================================================================*/
/* End MO CLI help texts                                                                                                                        */
/*==============================================================================================================================================*/
#endif CLIGLOBALDEFINITIONS_H
