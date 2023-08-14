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
/*              These methods operates on genJMRI Managed objects (MOs) and genJMRI sub-managed objects (sub-MOs)								*/
/*				- MOs defines a category of sub-MOs:																							*/
/*				  > The global MO: defines a set of sub-MOs which arent related to any particular CLI context, but which is globally avaliable: */
/*					e.g. reboot, help, context, uptime. CLI examples for global MOs: "help help", "help cli", "get context",                    */
/*					set context {cli-conext path}", "get uptime", "show memory", etc.															*/
/*				  > The Common MO: Defines a set of sub-MOs that are commonly available for all MOs, but in difference to the global MO, the	*/
/*					sub-MOs defined as a common MO operates on particular MOs and their instances. CLI examples for common MO sub-MOs:			*/
/*					"{genJMRI-context-path}get opstate", "{genJMRI-context-path}set description {description string}",							*/
/*					"{genJMRI-context-path}set debug", "{genJMRI-context-path}unset debug"														*/
/*				  > Specific genJMRI MOs: These MOs are specific to certain genJMRI servicies, such as: "lglink", "lightgroup", "satelitelink", */
/*					"satelite", "sensor", "actuator, etc. Specific MOs may implent specific sub-MOs only rellevant for a particular specific MO.*/
/*					CLI examples for specific MOs sub-MOs: "get lglink -overruns", "get lglink -overruns", "get lightgroup -showing", ...		*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR const char ROOT_MO_NAME[] = "root";

EXT_RAM_ATTR const char GLOBAL_MO_NAME[] = "global";								//Practical usage of MO is vagely defined - other than that all global commands are allways available (no matter context) and served by the globalCli static class
EXT_RAM_ATTR const char		CLIHELP_SUB_MO_NAME[] = "cli";							//Only for help, we need to add a property that this is only for help, and not a real command
EXT_RAM_ATTR const char		HELP_SUB_MO_NAME[] = "help";							//Print help text, see help text definitions below
EXT_RAM_ATTR const char		CONTEXT_SUB_MO_NAME[] = "context";						//Global MO instance context sub-MO
EXT_RAM_ATTR const char		UPTIME_SUB_MO_NAME[] = "uptime";						//Global MO Decoder up-time sub-MO
EXT_RAM_ATTR const char		CPU_SUB_MO_NAME[] = "cpu";								//Global MO CPU sub-MO
EXT_RAM_ATTR const char		CPUMEM_SUB_MO_NAME[] = "memory";						//Global MO Memory sub-MO
EXT_RAM_ATTR const char		NETWORK_SUB_MO_NAME[] = "network";						//Global Network MO sub-MO
EXT_RAM_ATTR const char		TOPOLOGY_SUB_MO_NAME[] = "topology";					//Global MO topology sub-MO
EXT_RAM_ATTR const char		COMMANDS_SUB_MO_NAME[] = "commands";					//Global Context available commands sub-MO
EXT_RAM_ATTR const char		MQTT_SUB_MO_NAME[] = "mqtt";							//Global MO MQTT sub-MO
EXT_RAM_ATTR const char		PINGSUPERVISION_SUB_MO_NAME[] = "supervision";			//move to MQTT?????????
EXT_RAM_ATTR const char		TIME_SUB_MO_NAME[] = "time";							//Global MO time sub-MO
EXT_RAM_ATTR const char		LOG_SUB_MO_NAME[] = "log";								//Global MO log sub-MO
EXT_RAM_ATTR const char		FAILSAFE_SUB_MO_NAME[] = "failsafe";					//Global MO failsafe-MO


EXT_RAM_ATTR const char COMMON_MO_NAME[] = "common";								//Common MO
EXT_RAM_ATTR const char		OPSTATE_SUB_MO_NAME[] = "opstate";						//Common MO OP-state sub-MO
EXT_RAM_ATTR const char		SYSNAME_SUB_MO_NAME[] = "systemname";					//Common MO System name sub-MO
EXT_RAM_ATTR const char		USER_SUB_MO_NAME[] = "username";						//Common MO User ame sub-MO
EXT_RAM_ATTR const char		DESC_SUB_MO_NAME[] = "description";						//Common MO Description sub-MO
EXT_RAM_ATTR const char		DEBUG_SUB_MO_NAME[] = "debug";							//Common MO Debug sub-MO

EXT_RAM_ATTR const char	DECODER_MO_NAME[] = "decoder";							// All decoder MOs are mapped to the global and common context

EXT_RAM_ATTR const char LGLINK_MO_NAME[] = "lglink";
EXT_RAM_ATTR const char		LGLINKNO_SUB_MO_NAME[] = "link";
EXT_RAM_ATTR const char		LGLINKOVERRUNS_SUB_MO_NAME[] = "overruns";
EXT_RAM_ATTR const char		LGLINKMEANLATENCY_SUB_MO_NAME[] = "meanlatency";
EXT_RAM_ATTR const char		LGLINKMAXLATENCY_SUB_MO_NAME[] = "maxlatency";
EXT_RAM_ATTR const char		LGLINKMEANRUNTIME_SUB_MO_NAME[] = "meanruntime";
EXT_RAM_ATTR const char		LGLINKMAXRUNTIME_SUB_MO_NAME[] = "maxruntime";

EXT_RAM_ATTR const char LG_MO_NAME[] = "lightgroup";
EXT_RAM_ATTR const char		LGADDR_SUB_MO_NAME[] = "address";
EXT_RAM_ATTR const char		LGLEDCNT_SUB_MO_NAME[] = "ledcnt";
EXT_RAM_ATTR const char		LGLEDOFFSET_SUB_MO_NAME[] = "ledoffset";
EXT_RAM_ATTR const char		LGPROPERTY_SUB_MO_NAME[] = "property";
EXT_RAM_ATTR const char		LGSTATE_SUB_MO_NAME[] = "state";
EXT_RAM_ATTR const char		LGSHOWING_SUB_MO_NAME[] = "showing";

EXT_RAM_ATTR const char SATLINK_MO_NAME[] = "satelitelink";
EXT_RAM_ATTR const char		SATLINKNO_SUB_MO_NAME[] = "link";
EXT_RAM_ATTR const char		SATLINKTXUNDERRUN_SUB_MO_NAME[] = "txunderrun";
EXT_RAM_ATTR const char		SATLINKRXOVERRUN_SUB_MO_NAME[] = "rxoverrun";
EXT_RAM_ATTR const char		SATLINKTIMINGVIOLATION_SUB_MO_NAME[] = "timingviolation";
EXT_RAM_ATTR const char		SATLINKRXCRCERR_SUB_MO_NAME[] = "rxcrcerr";
EXT_RAM_ATTR const char		SATLINKREMOTECRCERR_SUB_MO_NAME[] = "remotecrcerr";
EXT_RAM_ATTR const char		SATLINKRXSYMERRS_SUB_MO_NAME[] = "rxsymbolerr";
EXT_RAM_ATTR const char		SATLINKRXSIZEERRS_SUB_MO_NAME[] = "rxsizeerr";
EXT_RAM_ATTR const char		SATLINKWDERRS_SUB_MO_NAME[] = "wderr";

EXT_RAM_ATTR const char SAT_MO_NAME[] = "satelite";
EXT_RAM_ATTR const char		SATADDR_SUB_MO_NAME[] = "address";
EXT_RAM_ATTR const char		SATRXCRCERR_SUB_MO_NAME[] = "rxcrcerr";
EXT_RAM_ATTR const char		SATTXCRCERR_SUB_MO_NAME[] = "txcrcerr";
EXT_RAM_ATTR const char		SATWDERR_SUB_MO_NAME[] = "wderr";

EXT_RAM_ATTR const char SENSOR_MO_NAME[] = "sensor";
EXT_RAM_ATTR const char		SENSPORT_SUB_MO_NAME[] = "port";
EXT_RAM_ATTR const char		SENSSENSING_SUB_MO_NAME[] = "sensing";
EXT_RAM_ATTR const char		SENSORPROPERTY_SUB_MO_NAME[] = "property";

EXT_RAM_ATTR const char ACTUATOR_MO_NAME[] = "actuator";
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
																		"	- Satelite-link\n\r" \
																		"		- Satelite\n\r" \
																		"			- Actuator\n\r" \
																		"			- Sensor\n\r" \
																		"The managed objects are not directly exposed in the CLI, but are indirectly referred " \
																		"to by the current- or temporary- CLI context. A CLI context is a hierarchical " \
																		"name-space which can only operate on the given managed-object - and instance sub-managed " \
																		"objects, with two exeptions:\n\r" \
																		"- Global management objects are available in any CLI context, and operates on system wide objects\n\r" \
																		"- Common management objects are available for all CLI contexts and operates on the current/or indicated" \
																		"CLI context.\n\r" \
																		"CLI context can be provided in two ways:\n\r" \
																		"- Current CLI context represents the CLI context path that is currently active, the current " \
																		"active CLI context tree path is shown at the CLI prompt: \"{Current active absolute CLI context-path}>> \"" \
																		"or you can retreive it from the command: \"set context {absolute CLI context-path}|{rellative context-path}\"\n\r" \
																		"- Temporary contexts represents a method to redirect a single command to a different CLI context than the current." \
																		"The temporary CLI context is limited to a single command and after the command has been executed the current "\
																		"CLI context remains as before." \
																		"The temporary CLI context is prepended the CLI command sub-MO and can be an absolute or a rellative CLI context path: " \
																		"Ie: \"command [{absolute - CLI - Context - Path} | {rellative - CLI - Context - Path}] sub-MO arguments...\"\n\r" \
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

EXT_RAM_ATTR const char GLOBAL_GET_CONTEXT_HELP_TXT[] =				"Prints current current CLI context\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_CONTEXT_HELP_TXT[] =				"Sets current context path. If {value} begins with \"/\" it is considered to be an absolute path, " \
																		"otherwise it is considered to be a rellative path. \"..\" is used to ascend one level in " \
																		"the CLI context topology. \".\" is a NULL separator which does not change (ascend or decend) " \
																		"the CLI context path, I.e. \"./././path represents the same path as \"path\"\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_TOPOLOGY_HELP_TXT[] =			"Shows the CLI topology tree.\n\r" \
																		"Current active context is highlighted with an \"<<<\" indication on the right side of the table. " \
																		"	- If the \"-childs\" flag is given, only the CLI topology from the curren CLI context junction and below is shown.\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_COMMANDS_HELP_TXT[] =			"Shows available commands and their MO belonging\n\r" \
																		"	- If the \"-all\"flag is provided, all possible CLI commands are shown, if it is not - only those commands\n\r" \
																		"	  only those commands available from the current CLI context are shown.\n\r" \
																		"	- \"-help\" Shows a truncated help text for each command\n\r";

EXT_RAM_ATTR const char GLOBAL_REBOOT_HELP_TXT[] =					"Reboots the decoder.\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_UPTIME_HELP_TXT[] =				"Prints the time in seconds sinse the previous boot.\n\r";

EXT_RAM_ATTR const char GLOBAL_START_CPU_HELP_TXT[] =				"Starts a CPU activity.\n\r" \
																		"	- If the \"-stats\" flag is provided, the collection of CPU- and Memory- statistics is started\n\r" \
																		"	  CPU- and memory- collection is a prerequisite for some of the statistics \n\r" \
																		"	  that can be provided by \"get cpu [-flags]\", \"show cpu\", \"get memory [-flags]\" \n\r" \
																		"	  and \"show memory\". CPU and memory collection of statistics can have a significant negative impact on the system performance\"\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_CPU_HELP_TXT[] =				"Stops an ongoing CPU activity.\n\r" \
																		"	If the \"-stats\" flag is provided - ongoing collection of CPU- and Memory- statistics is stoped\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_CPU_HELP_TXT[] =					"get cpu [-tasks] [-task {taskName}] [-cpuusage] [-watermark] [-stats]\n\r" \
																		"get cpu: Is identical to \"show cpu\"\n\r" \
																		"get cpu -tasks: Prints task information for all running tasks\n\r" \
																		"get cpu -task {taskName}: Prints information for tasks \"taskName\"\n\r" \
																		"get cpu -cpuusage: Prints current and trending CPU-usage information, the CPU and memory statistics collection function needs to be active (\"set cpu -stats\") \n\r" \
																		"get cpu -watermark: Prints the High Watermark CPU-usage since CPU statistics collection was started\n\r" \
																		"get cpu -stats: Prints the state of the CPU and memory statistics collection function\n\r";
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

EXT_RAM_ATTR const char GLOBAL_START_CPUMEM_HELP_TXT[] =			"Starts a heap memory activity.\n\r" \
																		"	- If the \"-allocation {value}\" flag is provided a test-buffer of \"{value}\" bytes is allocated.\n\r" \
																		"	- \"-internal\" Operates on internal memory segmants.\n\r" \
																		"	- \"-external\" Operates on external SPI-RAM memory segments.\n\r" \
																		"	- \"-default\" Operates on memory segments as defined by the OS memory allocation policy.\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_CPUMEM_HELP_TXT[] =				"Stops earlier started heap memory activities.\n\r" \
																	"	- If the \"-allocation\" flag is provided, memory earlier allocated by \"start memory -allocation {value}\" will be freed\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_CPUMEM_HELP_TXT[] =				"Shows a summary of the heap status and statistics, is identical to \"get memory\"";

EXT_RAM_ATTR const char GLOBAL_SET_NETWORK_HELP_TXT[] =				"Sets IP and WIFI networking parameters.\n\r" \
																		"	-hostname {hostName}: Sets the host name\n\r" \
																		"	-address {hostIpAddress}: Sets the host IPv4 network address\n\r" \
																		"	-mask {networkIpMask}: Sets the IPv4 network mask\n\r" \
																		"	-gw {networkIpAddress}: Sets the IPv4 network default gateway address\n\r" \
																		"	-dns {dnsIpAddress}: Sets the IPv4 DNS address\n\r" \
																		"	-persist: Persists the network configuration\n\r";

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
																		"	- \"opstate\": Prints current network operational state\n\r" \
																		"	- \"scanap\": Scans availabe APs and prints the information about them\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_NETWORK_HELP_TXT[] =			"Shows a summary of network information. Identical to \"get network\" without flags.\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_MQTT_HELP_TXT[] =				"Sets MQTT parameters.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-uri {mqttBrokerUri}\": Sets MQTT Broker URI to \"mqttBrokerUri\" URI or IPv4 address\n\r" \
																		"	- \"-port {mqttBrokerPort}\": Sets MQTT Broker port to \"mqttBrokerPort\"\n\r" \
																		"	- \"-clientid {mqttClientId}\": Sets MQTT client ID to \"mqttClientId\"\n\r" \
																		"	- \"-qos {mqttDefaultQos}\": Sets MQTT default QoS to \"mqttDefaultQos\"\n\r" \
																		"	- \"-keepalive {mqttKeepAlive_s}\": Sets MQTT keepalive period to \"mqttKeepAlive_s\" seconds\n\r" \
																		"	- \"-ping {ping_s}\": Sets MQTT overlay server-client MQTT ping period to \"ping_s\" seconds\n\r" \
																		"	- \"-persist\": Persists MQTT configuration of Broker URI and Broker port\n\r";

EXT_RAM_ATTR const char GLOBAL_CLEAR_MQTT_HELP_TXT[] =				"Clear MQTT statistics.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-maxlatency\": Clears MQTT max-latency statistics\n\r" \
																		"	- \"-overruns\n\r: Clears MQTT overrun statistics\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_MQTT_HELP_TXT[] =				"Prints MQTT parameters and statistics. \"get mqtt\" without flags is identical to \"show mqtt\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-uri\": Print MQTT broker URI or IPv4 address\n\r" \
																		"	- \"-port\": Prints the MQTT broker port\n\r" \
																		"	- \"-clientid\": Prints the MQTT client identifier\n\r" \
																		"	- \"-qos\": Prints the MQTT client default Quality of Service class\n\r" \
																		"	- \"-keepalive\": Prints the MQTT keep-alive period in seconds\n\r" \
																		"	- \"-ping\": Prints the server-client ping period in seconds\n\r" \
																		"	- \"-maxlatency\": Prints the MQTT poll loop max latency in uS\n\r" \
																		"	- \"-meanlatency\": Prints the MQTT poll loop mean latency in uS\n\r" \
																		"	- \"-overruns\": Prints the MQTT poll loop overrun counter\n\r" \
																		"	- \"-opstate\": Prints the MQTT client operational state\n\r" \
																		"	- \"-subscriptions\": Prints current subscriptions and call backs.";

EXT_RAM_ATTR const char GLOBAL_SHOW_MQTT_HELP_TXT[] =				"Shows a summary of the MQTT information. Is idetical to \"get mqtt\" without flags\n\r";

EXT_RAM_ATTR const char GLOBAL_ADD_TIME_HELP_TXT[] =				"Adds an NTP server.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-ntpserver {ntpServerURI | ntpServerIPv4Address} [-ntpport {ntpPort}]\": Adds a new NTP server with URI: \"ntpServerURI\" or IPv4 IP address: \"ntpServerIPv4Address\",\n\r" \
																		"	     the NTP port can be set with \"-ntpport {ntpPort}\", if the port is not given 123 will be used as the default port\n\r";

EXT_RAM_ATTR const char GLOBAL_DELETE_TIME_HELP_TXT[] =				"Deletes an NTP server.\n\r" \
																		"Available flags:\n\r" \
																		"- \"-ntpserver{ntpServerURI | ntpServerIPv4Address}\": Deletes a previously provisioned NTP server with URI : \"ntpServerURI\" or IPv4 IP address: \"ntpServerIPv4Address\"\n\r";

EXT_RAM_ATTR const char GLOBAL_START_TIME_HELP_TXT[] =				"Starts time services.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-ntpclient\": Starts the NTP client\n\r";

EXT_RAM_ATTR const char GLOBAL_STOP_TIME_HELP_TXT[] =				"Stops time services.\n\r" \
																	"Available flags:\n\r" \
																		"	- \"-ntpclient\": Stopps the NTP client\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_TIME_HELP_TXT[] =				"Sets the system time.\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-timeofday | -tod {timeOfDay}\": Sets time of day in UTC, \"timeOfDay\" format: \"YYYY-MM-DDTHH:MM:SS\"\n\r" \
																		"	- \"-epochtime {epochTime_s}\": Sets Epoch time, \"epochTime_s\" format: NNNNNN - seconds since Jan 1 1970 UTC\n\r" \
																		"	- \"-timezone {timeZone_h}\": Sets the timezone, \"timeZone_h\" format (-)NN houres, NN <= 12, E.g. \"CET+1\".\n\r";

EXT_RAM_ATTR const char GLOBAL_GET_TIME_HELP_TXT[] =				"Prints the system time. \"get time\" without flags is identical to \"show time\".\n\r" \
																		"Available flags:\n\r" \
																		"	- \"-timeofday | -tod [-utc]\": Prints the local or universal time\n\r" \
																		"	- \"-epochtime\": Prints the epoch time - seconds sinse Jan 1 1970 UTC\n\r" \
																		"	- \"-timezone\": Prints current time-zone\n\r" \
																		"	- \"-daylightsaving\": Prints daylight-saving status\n\r" \
																		"	- \"-ntpservers\": Prints the current status of all provisioned NTP servers\n\r" \
																		"	- \"-ntpsyncstatus\": Prints the current NTP client sync status\n\r" \
																		"	- \"-ntpsyncmode\": Prints the current NTP client sync mode\n\r" \
																		"	- \"-ntpopstate\": Prints the current NTP client operational state\n\r";

EXT_RAM_ATTR const char GLOBAL_SHOW_TIME_HELP_TXT[] =				"Shows a summary of the time services. Identical to \"get time\" without flags.\n\r";

EXT_RAM_ATTR const char GLOBAL_SET_LOG_HELP_TXT[] =					"set log [-loglevel {logLevel} [-logmo {logMo}]] [-logdestination {logDestination}]\n\r" \
																		"set log -loglevel {logLevel}[-logmo {logMo}]: Sets the loglevel \"logLevel\" to \"DEBUG-SILENT\"|\"DEBUG-PANIC\"|\"DEBUG-ERROR\"|\"DEBUG-WARN\"|\"DEBUG-INFO\"|\"DEBUG-TERSE\"|\"DEBUG-VERBOSE\"\n\r" \
																		"                                              if \"logmo {logMo}\" is provided, the given loglevel is only valid for the provided \"logMo\" (managed object) - NOT YET SUPPORTED\n\r" \
																		"set log -logdestination {logDestination}: Sets a remote log destination - NOT YET SUPPORTED\n\r";
EXT_RAM_ATTR const char GLOBAL_UNSET_LOG_HELP_TXT[] =				"unset log -logdestination: Terminates logging to the previously provisioned remote log destination - NOT YET SUPPORTED\n\r";
EXT_RAM_ATTR const char GLOBAL_GET_LOG_HELP_TXT[] =					"get log [-loglevel [-logmo {logMo}]] [-logdestination] [-tail {lines}]\n\r" \
																		"get log -loglevel [-logmo {logMo}] Prints the current loglevel, if \"logmo {logMo}\" is provided, the loglevel for the provided \"logMo\" (managed object) is given - NOT YET SUPPORTED\n\r" \
																		"get log - logdestination: Prints the remote loglevel destination - NOT YET SUPPORTED\n\r" \
																		"get log -tail {lines}: Prints the last \"lines\" of the log - NOT YET SUPPORTED\n\r";
EXT_RAM_ATTR const char GLOBAL_SHOW_LOG_HELP_TXT[] =				"show log: Prints a summary of log information\n\r";

EXT_RAM_ATTR const char COMMON_SET_DEBUG_HELP_TXT[] =				"Sets the debug state of the MO, this enables setting of MO/Sub-MO atributes which may lead to system inconsistensy.\n\r";

EXT_RAM_ATTR const char COMMON_UNSET_DEBUG_HELP_TXT[] =				"Un-sets the debug state of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_GET_DEBUG_HELP_TXT[] =				"Prints the debug state of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_GET_OPSTATE_HELP_TXT[] =				"Prints the Operational state of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_GET_SYSNAME_HELP_TXT[] =				"Prints Systemname of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_SET_SYSNAME_HELP_TXT[] =				"Sets the Systemname of the MO. MO Debugstate needs to be active - use \"set debug\"\n\r";

EXT_RAM_ATTR const char COMMON_GET_USRNAME_HELP_TXT[] =				"Prints the Username of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_SET_USRNAME_HELP_TXT[] =				"Sets the Username of the MO. MO Debugstate needs to be active - use \"set debug\"\n\r";

EXT_RAM_ATTR const char COMMON_GET_DESCRIPTION_HELP_TXT[] =			"Prints the Description of the MO.\n\r";

EXT_RAM_ATTR const char COMMON_SET_DESCRIPTION_HELP_TXT[] =			"Sets the description of the MO. MO Debugstate needs to be active - use \"set debug\"\n\r";

EXT_RAM_ATTR const char DECODER_GET_FAILSAFE_HELP_TXT[] =			"[{context-path}/]get failsafe: Prints the current fail-safe state\n\r";

EXT_RAM_ATTR const char DECODER_SET_FAILSAFE_HELP_TXT[] =			"[{context-path}/]set failsafe: Sets/activates fail-safe state. Debug state needs to be activated before fail-safe can be activated by CLI\n\r";

EXT_RAM_ATTR const char DECODER_UNSET_FAILSAFE_HELP_TXT[] =			"[{context-path}/]unset failsafe: Unsets/deactivates fail-safe state. Debug state needs to be activated before fail-safe can be de-activated by CLI\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINK_HELP_TXT[] =			"[{context-path}/]get link: Prints the lgLink instance number - the link\n\r";

EXT_RAM_ATTR const char LGLINKNO_SET_LGLINK_HELP_TXT[] =			"[{context-path}/]set link {lgKink-number}: Sets the lgLink instance number - the link, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKOVERRUNS_HELP_TXT[] =	"[{context-path}/]get overruns: Prints the accumulated number of lgLink over-runs, I.e. for which the lgLink scan had not finished before the next was due\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKOVERRUNS_HELP_TXT[] =	"[{context-path}/]clear overruns: Clears the counter for accumulated lgLink over-runs\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMEANLATENCY_HELP_TXT[] = "[{context-path}/]get meanlatency: Prints the mean latency of the lgLink scan, E.g. how much its start was delayed compared to schedule\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMAXLATENCY_HELP_TXT[] =	"[{context-path}/]get maxlatency: Prints the maximum latency of the lgLink scan, E.g. how much its start was delayed compared to schedule\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKMAXLATENCY_HELP_TXT[] = "[{context-path}/]clear maxlatency: Clears the maximum latency watermark of the lgLink scan\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMEANRUNTIME_HELP_TXT[] = "[{context-path}/]get meanruntime: Prints the mean run-time of the lgLink scan, E.g. how long the linkscan took\n\r";

EXT_RAM_ATTR const char LGLINKNO_GET_LGLINKMAXRUNTIME_HELP_TXT[] =	"[{context-path}/]get maxruntime: Prints the maximum run-time of the lgLink scan, E.g. how long the linkscan took\n\r";

EXT_RAM_ATTR const char LGLINKNO_CLEAR_LGLINKMAXRUNTIME_HELP_TXT[] = "[{context-path}/]clear maxruntime: Clears the maximum run-time watermark of the lgLink scan\n\r";

EXT_RAM_ATTR const char LG_GET_LGADDR_HELP_TXT[] =					"[{context-path}/]get address: Prints the Lg-link address of the Lightgroup\n\r";

EXT_RAM_ATTR const char LG_SET_LGADDR_HELP_TXT[] =					"[{context-path}/]set address: Sets the Lg-link address of the Lightgroup, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char LG_GET_LGLEDCNT_HELP_TXT[] =				"[{context-path}/]get ledcnt: Prints the number of Leds/pixels for the light group\n\r";

EXT_RAM_ATTR const char LG_SET_LGLEDCNT_HELP_TXT[] =				"[{context-path}/]set ledcnt {ledcnt}: Sets the number of Leds/pixels for the light group, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char LG_GET_LGLEDOFFSET_HELP_TXT[] =				"[{context-path}/]get ledoffset: Prints the Lightgrop LED offset on the Lg-link\n\r";

EXT_RAM_ATTR const char LG_SET_LGLEDOFFSET_HELP_TXT[] =				"[{context-path}/]set ledoffset {ledoffset}: Sets the Lightgrop LED offset on the Lg-link, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char LG_GET_LGPROPERTY_HELP_TXT[] =				"[{context-path}/]get property [{property-index}]: Prints the Lightgrop properties, if property index is provided the property corresponding to the index is provided, else all properties are provided\n\r";

EXT_RAM_ATTR const char LG_SET_LGPROPERTY_HELP_TXT[] =				"[{context-path}/]set property {property-index} {property}: Sets the Lightgrop property according to given property index, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char LG_GET_LGSHOWING_HELP_TXT[] =				"[{context-path}/]get showing: Prints the Light-group current aspect/show\n\r";

EXT_RAM_ATTR const char LG_SET_LGSHOWING_HELP_TXT[] =				"[{context-path}/]set showing {aspect/show}: Sets the Light-group current aspect/show, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char SATLINK_GET_SATLINKNO_HELP_TXT[] =			"[{context-path}/]get link: Prints the Satelite-link number\n\r";

EXT_RAM_ATTR const char SATLINK_SET_SATLINKNO_HELP_TXT[] =			"[{context-path}/]set link {satelite-link}: Sets the Satelite-link number, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char SATLINK_GET_TXUNDERRUNS_HELP_TXT[] =		"[{context-path}/]get txunderrun: Prints number of Satelite-link TX underruns\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_TXUNDERRUNS_HELP_TXT[] =		"[{context-path}/]clear txunderrun: Clears the Satelite-link TX underruns counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXOVERRUNS_HELP_TXT[] =			"[{context-path}/]get rxoverrun: Prints number of Satelite-link RX overruns\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXOVERRUNS_HELP_TXT[] =		"[{context-path}/]clear rxoverrun: Clears the Satelite-link RX overruns counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_TIMINGVIOLATION_HELP_TXT[] =	"[{context-path}/]get timingviolation: Prints number of Satelite-link timingviolations\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_TIMINGVIOLATION_HELP_TXT[] =	"[{context-path}/]clear timingviolation: Clears the Satelite-link timingviolation counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXCRCERR_HELP_TXT[] =			"[{context-path}/]get rxcrcerr: Prints number of Satelite-link RX CRC errors\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXCRCERR_HELP_TXT[] =			"[{context-path}/]clear rxcrcerr: Clears the Satelite-link RX CRC error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_REMOTECRCERR_HELP_TXT[] =		"[{context-path}/]get remotecrcerr: Prints number of Satelite-link RX CRC errors\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_REMOTECRCERR_HELP_TXT[] =		"[{context-path}/]clear remotecrcerr: Clears the Satelite-link RX CRC error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXSYMERRS_HELP_TXT[] =			"[{context-path}/]get symerr: Prints number of Satelite-link RX symbol errors\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXSYMERRS_HELP_TXT[] =		"[{context-path}/]clear symerr: Clears the Satelite-link RX symbol error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_RXSIZEERRS_HELP_TXT[] =			"[{context-path}/]get rxsizeerr: Prints number of Satelite-link RX size errors\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_RXSIZEERRS_HELP_TXT[] =		"[{context-path}/]clear rxsizeerr: Clears the Satelite-link RX size error counter\n\r";

EXT_RAM_ATTR const char SATLINK_GET_WDERRS_HELP_TXT[] =				"[{context-path}/]get wderr: Prints number of Satelite-link watchdog errors, I.e aggregated watchdog error reported by Satelites on the Satelite-link\n\r";

EXT_RAM_ATTR const char SATLINK_CLEAR_WDERRS_HELP_TXT[] =			"[{context-path}/]clear wderr: Clears the Satelite-link watchdog error counter\n\r";

EXT_RAM_ATTR const char SAT_GET_SATADDR_HELP_TXT[] =				"[{context-path}/]get address: Prints the Satelite address\n\r";

EXT_RAM_ATTR const char SAT_SET_SATADDR_HELP_TXT[] =				"[{context-path}/]set address {Satelite-address}: Sets the Satelite address, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char SAT_GET_SATRXCRCERR_HELP_TXT[] =			"[{context-path}/]get rxcrcerr: Prints Satelite RX CRC errors\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATRXCRCERR_HELP_TXT[] =			"[{context-path}/]clear rxcrcerr: Clears the Satelite RX CRC error counter\n\r";

EXT_RAM_ATTR const char SAT_GET_SATTXCRCERR_HELP_TXT[] =			"[{context-path}/]get txcrcerr: Prints Satelite TX CRC errors\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATTXCRCERR_HELP_TXT[] =			"[{context-path}/]clear txcrcerr: Clears the Satelite TX CRC error counter\n\r";

EXT_RAM_ATTR const char SAT_GET_SATWDERR_HELP_TXT[] =				"[{context-path}/]get wderr: Prints Satelite watchdog CRC errors\n\r";

EXT_RAM_ATTR const char SAT_CLEAR_SATWDERR_HELP_TXT[] =				"[{context-path}/]clear wderr: Clears the Satelite watchdog error counter\n\r";

EXT_RAM_ATTR const char SENS_GET_SENSPORT_HELP_TXT[] =				"[{context-path}/]get port: Prints the Sensor port number\n\r";

EXT_RAM_ATTR const char SENS_SET_SENSPORT_HELP_TXT[] =				"[{context-path}/]set port {sensor-port}: Sets the Sensor port number, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char SENS_GET_SENSING_HELP_TXT[] =				"[{context-path}/]get sensing: Prints current Sensor state\n\r";

EXT_RAM_ATTR const char SENS_GET_SENSORPROPERTY_HELP_TXT[] =		"[{context-path}/]get property [{property-index}] : Prints the Sensor properties, if property index is provided the property corresponding to the index is provided, else all properties are provided\n\r";

EXT_RAM_ATTR const char SENS_SET_SENSORPROPERTY_HELP_TXT[] =		"[{context-path}/]set property {property-index} : Sets the Sensor property according to given property index, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char ACT_GET_ACTPORT_HELP_TXT[] =				"[{context-path}/]get port: Prints the Actuator port\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTPORT_HELP_TXT[] =				"[{context-path}/]set port {actuator-port}: Sets the actuator port, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char ACT_GET_ACTSHOWING_HELP_TXT[] =				"[{context-path}/]get showing: Prints the actuator showing state\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTSHOWING_HELP_TXT[] =				"[{context-path}/]set showing {actuator-showing}: Sets the actuator showing state, debug mode needs to be activated to perform this action\n\r";

EXT_RAM_ATTR const char ACT_GET_ACTPROPERTY_HELP_TXT[] =			"[{context-path}/]get property [{property-index}] : Prints the Actuator properties, if property index is provided the property corresponding to the index is provided, else all properties are provided\n\r";

EXT_RAM_ATTR const char ACT_SET_ACTPROPERTY_HELP_TXT[] =			"[{context-path}/]set property {property-index} : Sets the Actuator property according to given property index, debug mode needs to be activated to perform this action\n\r";

/*==============================================================================================================================================*/
/* End MO CLI help texts                                                                                                                        */
/*==============================================================================================================================================*/
#endif CLIGLOBALDEFINITIONS_H
