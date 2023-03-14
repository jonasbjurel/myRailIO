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
#define ROOT_MO_NAME								"root"

#define GLOBAL_MO_NAME								"global"							//Practical usage of MO is vagely defined - other than that all global commands are allways available (no matter context) and served by the globalCli static class
#define		CLIHELP_SUB_MO_NAME							"cli"							//Only for help, we need to add a property that this is only for help, and not a real command
#define		HELP_SUB_MO_NAME							"help"							//Print help text, see help text definitions below
#define		CONTEXT_SUB_MO_NAME							"context"						//Global MO instance context sub-MO
#define		UPTIME_SUB_MO_NAME							"uptime"						//Global MO Decoder up-time sub-MO
#define		CPU_SUB_MO_NAME								"cpu"							//Global MO CPU sub-MO
#define		CPUMEM_SUB_MO_NAME							"memory"						//Global MO Memory sub-MO
#define		NETWORK_SUB_MO_NAME							"network"						//Global Network MO sub-MO
#define		TOPOLOGY_SUB_MO_NAME						"topology"						//Global MO topology sub-MO
#define		MQTT_SUB_MO_NAME							"mqtt"							//Global MO MQTT sub-MO
#define		PINGSUPERVISION_SUB_MO_NAME					"supervision"					//move to MQTT?????????
#define		TIME_SUB_MO_NAME							"time"							//Global MO time sub-MO
#define		LOG_SUB_MO_NAME								"log"							//Global MO log sub-MO
#define		FAILSAFE_SUB_MO_NAME						"failsafe"						//Global MO failsafe-MO


#define COMMON_MO_NAME								"common"							//Common MO
#define		OPSTATE_SUB_MO_NAME							"opstate"						//Common MO OP-state sub-MO
#define		SYSNAME_SUB_MO_NAME							"systemname"					//Common MO System name sub-MO
#define		USER_SUB_MO_NAME							"username"						//Common MO User ame sub-MO
#define		DESC_SUB_MO_NAME							"description"					//Common MO Description sub-MO
#define		DEBUG_SUB_MO_NAME							"debug"							//Common MO Debug sub-MO

#define	DECODER_MO_NAME								"decoder"							// All decoder MOs are mapped to the global and common context

#define LGLINK_MO_NAME								"lglink"
#define		LGLINKNO_SUB_MO_NAME						"link"
#define		LGLINKOVERRUNS_SUB_MO_NAME					"overruns"
#define		LGLINKMEANLATENCY_SUB_MO_NAME				"meanlatency"
#define		LGLINKMAXLATENCY_SUB_MO_NAME				"maxlatency"
#define		LGLINKMEANRUNTIME_SUB_MO_NAME				"meanruntime"
#define		LGLINKMAXRUNTIME_SUB_MO_NAME				"maxruntime"

#define LG_MO_NAME									"lightgroup"
#define		LGADDR_SUB_MO_NAME							"address"
#define		LGLEDCNT_SUB_MO_NAME						"ledcnt"
#define		LGLEDOFFSET_SUB_MO_NAME						"ledoffset"
#define		LGPROPERTY_SUB_MO_NAME						"property"
#define		LGSTATE_SUB_MO_NAME							"state"
#define		LGSHOWING_SUB_MO_NAME						"showing"

#define SATLINK_MO_NAME								"satelitelink"
#define		SATLINKNO_SUB_MO_NAME						"link"
#define		SATLINKTXUNDERRUN_SUB_MO_NAME				"txunderrun"
#define		SATLINKRXOVERRUN_SUB_MO_NAME				"rxoverrun"
#define		SATLINKTIMINGVIOLATION_SUB_MO_NAME			"timingviolation"
#define		SATLINKRXCRCERR_SUB_MO_NAME					"rxcrcerr"
#define		SATLINKREMOTECRCERR_SUB_MO_NAME				"remotecrcerr"
#define		SATLINKRXSYMERRS_SUB_MO_NAME				"rxsymbolerr"
#define		SATLINKRXSIZEERRS_SUB_MO_NAME				"rxsizeerr"
#define		SATLINKWDERRS_SUB_MO_NAME					"wderr"

#define SAT_MO_NAME									"satelite"
#define		SATADDR_SUB_MO_NAME							"address"
#define		SATRXCRCERR_SUB_MO_NAME						"rxcrcerr"
#define		SATTXCRCERR_SUB_MO_NAME						"txcrcerr"
#define		SATWDERR_SUB_MO_NAME						"wderr"

#define SENSOR_MO_NAME								"sensor"
#define		SENSPORT_SUB_MO_NAME						"port"
#define		SENSSENSING_SUB_MO_NAME						"sensing"
#define		SENSORPROPERTY_SUB_MO_NAME					"property"

#define ACTUATOR_MO_NAME							"actuator"
#define		ACTUATORPORT_SUB_MO_NAME					"port"
#define		ACTUATORSHOWING_SUB_MO_NAME					"showing"
#define		ACTUATORPROPERTY_SUB_MO_NAME				"property"

/*==============================================================================================================================================*/
/* End MO definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Managed object (MO/Sub-MO) CLI help texts                                                                                                    */
/* Purpose: Defines the managed objects (MOs/sub-MOs) help texts																				*/
/* Description:																																	*/
/*==============================================================================================================================================*/
#define GLOBAL_HELP_HELP_TXT											"[{context-path}/]help [-m MO-object][{MO-subobject}]:\n\rProvides the full CLI help text, or only "\
																		"the help text for a particular managed sub-object related to the context/managed"\
																		"object. If [-m {MO-objecttype}] is provided, the help text for the MO-subobject "\
																		"is related to the given MO-object type rather than current context. Use \"show "\
																		" motypes\" to see available MO-types\n\r"

#define GLOBAL_FULL_CLI_HELP_TXT										"The genJMRIDecoder CLI provides interactive management capabilities throughout it\'s "\
																		"management object types:\n\r"\
																		"- Global\n\r"\
																		"- Common\n\r"\
																		"- Decoder\n\r"\
																		"	- Lightgroup-link\n\r"\
																		"		- Lightgroup\n\r"\
																		"	- Satelite-link\n\r"\
																		"		- Satelite\n\r"\
																		"			- Actuator\n\r"\
																		"			- Sensor\n\r"\
																		"The managed objects are not directly exposed in the CLI, but are indirectly referred"\
																		"to by the current- or temporary- CLI context. A CLI context is a hierarchical"\
																		"name-space which can only operate on the given managed-object - and instance sub-managed"\
																		"objects, with three exeptions:\n\r"\
																		"- Global management objects are available in any CLI context, and operates on system wide singelton objects\n\r"\
																		"- Common management objects are available for all CLI contexts and operates on the current/or indicated"\
																		"CLI context.\n\r"\
																		"CLI context can ne provided in two ways:\n\r"\
																		"- Current CLI context represents the CLI context pat that is currently active, the current"\
																		"active CLI context tree path is shown at the CLI prompt: \"{Current active absolute CLI context-path}>> \""\
																		"or you can retreive it by the command: \">> set context {absolute CLI context-path}|{rellative context-path}\"\n\r"\
																		"is shown\n\r"\
																		"- Temporary context represents a method to redirect a single command to a different CLI context than the current."\
																		"the temporary CLI context is limited to a single command and the current CLI context remains as before"\
																		"after the command has executed. The temporary CLI context is prepended the CLI command and can be an"\
																		"absolute or a rellative CLI context path: Ie: [{absolute-CLI-Context-Path}|{rellative-CLI-Context-Path}]command [parameters]\n\r"\
																		"\n\r"\
																		"Available genJMRI CLI commands:\n\r"\
																		"helpCliCmd [args]\n\r"\
																		"reboot\n\r"\
																		"show [args]\n\r"\
																		"get [args]\n\r"\
																		"set [args]\n\r"\
																		"unset [args]\n\r"\
																		"clear [args]\n\r"\
																		"add [args]\n\r"\
																		"delete [args]\n\r"\
																		"copy [args]\n\r"\
																		"paste [args]\n\r"\
																		"move [args]\n\r"\
																		"start [args]\n\r"\
																		"stop [args]\n\r"\
																		"restart [args]"

#define GLOBAL_GET_CONTEXT_HELP_TXT										"get context: Prints current Context/Managed object"

#define GLOBAL_SET_CONTEXT_HELP_TXT										"set context {context-path}: Sets current context/Managed object path. If context-path "\
																		"begins with \"/\" it is considered to be an absolute path, other wise it is considered "\
																		"to be a rellative path. \"..\" is used to ascend one level in the context/Managed object "\
																		"tree. \".\" indicates current level in the context/Managed object tree"

#define GLOBAL_SHOW_TOPOLOGY_HELP_TXT									"show topology [{root-path}]: Shows decoder context/Manage object topology-tree."\
																		"if \"{root-path}\" is given, the topology-tree shown will start from that "\
																		"path-junction and, otherwise the topology shown will start from the current "\
																		"context-path. Eg. if \"/\" is given, the printout will start from the decoder-root"\
																		"showing the full topology.\n\r"

#define GLOBAL_REBOOT_HELP_TXT											"reboot: Reboots the decoder"

#define GLOBAL_GET_UPTIME_HELP_TXT										"get uptime: Prints the time in seconds sinse the previous reboot"

#define GLOBAL_START_CPU_HELP_TXT										"start cpu -stats: Starts collection of CPU and Memory statistics"
#define GLOBAL_STOP_CPU_HELP_TXT										"stop cpu -stats: Stops collection of CPU and Memory statistics"
#define GLOBAL_GET_CPU_HELP_TXT											"get cpu [-tasks] [-task {taskName}] [-cpuusage] [-watermark] [-stats]\n\r"\
																		"get cpu: Is identical to \"show cpu\"\n\r"\
																		"get cpu -tasks: Prints task information for all running tasks\n\r"\
																		"get cpu -task {taskName}: Prints information for tasks \"taskName\"\n\r"\
																		"get cpu -cpuusage: Prints current and trending CPU-usage information, the CPU and memory statistics collection function needs to be active (\"set cpu -stats\") \n\r"\
																		"get cpu -watermark: Prints the High Watermark CPU-usage since CPU statistics collection was started\n\r"\
																		"get cpu -stats: Prints the state of the CPU and memory statistics collection function\n\r"
#define GLOBAL_SHOW_CPU_HELP_TXT										"show cpu: Shows a collection of CPU status- and statistics"

#define GLOBAL_GET_CPUMEM_HELP_TXT										"get memory [-total] [-used] [-available] [-watermark] [-average {averagePeriod_s}] [-trend {trendPeriod_s}] [-maxblock] [-allocation {aloccation_bytes] | -free]\n\r"\
																		"get memory Is identical to \"show memory\"\n\r"\
																		"get memory -total: Prints the total installed memory capacity in bytes\n\r"\
																		"get memory -used: Prints currently used memory capacity in bytes\n\r"\
																		"get memory -available: Prints currently available memory capacity in bytes\n\r"\
																		"get memory -watermark: Prints the lowest available memory watermark in bytes since CPU statistics collection was started\n\r"\
																		"get memory -average {averagePeriod_s}: Prints the average memory usage in bytes over \"averagePeriod_s\" seconds, the CPU and memory statistics collection function needs to be active - see \"set cpu -stats\"\n\r"\
																		"get memory -trend {trendPeriod_s}: Prints the memory usage trend in bytes over \"trendPeriod_s\" seconds, the CPU and memory statistics collection function needs to be active - see \"set cpu -stats\"\n\r"\
																		"get memory -maxblock: Prints the maximum block of heap memory in bytes that currently can be allocated\n\r"\
																		"get memory -allocate {size}: Allocates a memory chunk of \"size\" bytes for memory debug reasons - debug flag needs to be set - see \"set debug\"\n\r"\
																		"get memory -free: Frees/deallocates a previously allocated memory shunk from \"get memory -allocate {size}\"\n\r"
#define GLOBAL_SHOW_CPUMEM_HELP_TXT										"show memory: Prints a summary of memory status and statistics"

#define GLOBAL_SET_NETWORK_HELP_TXT										"set network [-hostname {hostName}] [-address {hostIpAddress}] [-mask {networkIpMask}] [-gw {networkIpAddress}] [-dns {dnsIpAddress}] [-persist]\n\r"\
																		"set network -hostname {hostName}: Sets the host name\n\r"\
																		"set network -address {hostIpAddress}: Sets the host IPv4 network address\n\r"\
																		"set network -mask {networkIpMask}: Sets the IPv4 network mask\n\r"\
																		"set network -gw {networkIpAddress}: Sets the IPv4 network default gateway address\n\r"\
																		"set network -dns {dnsIpAddress}: Sets the IPv4 DNS address\n\r"\
																		"set network -persist: Persists the network configuration\n\r"
#define GLOBAL_GET_NETWORK_HELP_TXT										"get network [-ssid] [-bssid] [-channel] [-auth] [-rssi] [-mac] [-hostname] [-address] [-mask] [-gw] [-dns] [-opstate] [-scanap]\n\r"\
																		"get network: Is identical to \"show network\"\n\r"\
																		"get network -ssid: Prints the AP SSID connected to\n\r"\
																		"get network -bssid: Prints the AP BSSID connected to\n\r"\
																		"get network -channel: Prints the AP WiFi channel connected to\n\r"\
																		"get network -auth: Prints current WiFi Autentication/Encryption method\n\r"\
																		"get network -rssi: Prints current SNR/RSSI WiFi signal quality\n\r"\
																		"get network -mac: Prints decoder WiFi MAC address\n\r"\
																		"get network -hostname: Prints decoder host name\n\r"\
																		"get network -address: Prints the host IPv4 network address\n\r"\
																		"get network -mask: Prints the IPv4 network mask\n\r"\
																		"get network -gw: Prints the IPv4 network default gateway address\n\r"\
																		"get network -dns: Prints the IPv4 DNS address\n\r"\
																		"get network -opstate Prints current network operational state\n\r"\
																		"get network -scanap: Scans availabe APs and prints the information about them\n\r"
#define GLOBAL_SHOW_NETWORK_HELP_TXT									"show network: Prints a summary of network information\n\r"

#define GLOBAL_SET_MQTT_HELP_TXT										"set mqtt [-uri {mqttBrokerUri}] [-port {mqttBrokerPort}] [-clientid {mqttClientId}] [-qos {mqttDefaultQos}] [-keepalive {mqttKeepAlive_s}] [-ping {ping_s}] [-persist]\n\r"\
																		"set mqtt -uri {mqttBrokerUri}: Sets MQTT Broker URI to \"mqttBrokerUri\" URI or IPv4 address\n\r"\
																		"set mqtt -port {mqttBrokerPort}: Sets MQTT Broker port to \"mqttBrokerPort\"\n\r"\
																		"set mqtt -clientid {mqttClientId}: Sets MQTT client ID to \"mqttClientId\"\n\r"\
																		"set mqtt -qos {mqttDefaultQos}: Sets MQTT default QoS to \"mqttDefaultQos\"\n\r"\
																		"set mqtt -keepalive {mqttKeepAlive_s}: Sets MQTT keepalive period to \"mqttKeepAlive_s\" seconds\n\r"\
																		"set mqtt -ping {ping_s}: Sets MQTT overlay server-client MQTT ping period to \"ping_s\" seconds\n\r"\
																		"set mqtt -persist: Persists MQTT configuration of Broker URI and Broker port\n\r"
#define GLOBAL_CLEAR_MQTT_HELP_TXT										"clear mqtt [-maxlatency] [-overruns]\n\r"\
																		"clear mqtt -maxlatency: Clears MQTT max-latency statistics\n\r"\
																		"clear mqtt -overruns: Clears MQTT overrun statistics\n\r"
#define GLOBAL_GET_MQTT_HELP_TXT										"get mqtt [-uri] [-port] [-clientid] [-qos] [-keepalive] [-ping] [-maxlatency] [-meanlatency] [-overruns] [-opstate]\n\r"\
																		"get mqtt: Identical to \"show mqtt\"\n\r"\
																		"get mqtt -uri: Print MQTT broker URI or IPv4 address\n\r"\
																		"get mqtt -port: Prints the MQTT broker port\n\r"\
																		"get mqtt -clientid: Prints the MQTT client identifier\n\r"\
																		"get mqtt -qos: Prints the MQTT client default Quality of Service class\n\r"\
																		"get mqtt -keepalive: Prints the MQTT keep-alive period in seconds\n\r"\
																		"get mqtt -ping: Prints the server-client ping period in seconds\n\r"\
																		"get mqtt -maxlatency: Prints the MQTT poll loop max latency in uS\n\r"\
																		"get mqtt -meanlatency: Prints the MQTT poll loop mean latency in uS\n\r"\
																		"get mqtt -overruns: Prints the MQTT poll loop overrun counter\n\r"\
																		"get mqtt -opstate: Prints the MQTT client operational state\n\r"
#define GLOBAL_SHOW_MQTT_HELP_TXT										"show mqtt: Prints a summary of the MQTT information\n\r" 

#define GLOBAL_ADD_TIME_HELP_TXT										"add time -ntpserver {ntpServerURI | ntpServerIPv4Address} [-ntpport {ntpPort}]: Adds a new NTP server with URI: \"ntpServerURI\" or IPv4 IP address: \"ntpServerIPv4Address\",\n\r"\
																		"the NTP port can be set with \"-ntpport {ntpPort}\", if the port is not given 123 will be used as the default port\n\r"
#define GLOBAL_DELETE_TIME_HELP_TXT										"delete time -ntpserver {ntpServerURI | ntpServerIPv4Address}: Deletes a previously provisioned NTP server with URI: \"ntpServerURI\" or IPv4 IP address: \"ntpServerIPv4Address\"\n\r"
#define GLOBAL_START_TIME_HELP_TXT										"start time -ntpclient: Starts the NTP client\n\r"
#define GLOBAL_STOP_TIME_HELP_TXT										"stop time -ntpclient: Stopps the NTP client\n\r"
#define GLOBAL_SET_TIME_HELP_TXT										"set time [-timeofday | -tod {timeOfDay}] [-epochtime {epochTime_s}] [-timezone {timeZone_h}] [-daylightsaving]\n\r"\
																		"set time -timeofday | -tod {timeOfDay}: Sets time of day in UTC, \"timeOfDay\" format: \"YYYY-MM-DDTHH:MM:SS\"\n\r"\
																		"set time -epochtime {epochTime_s}: Sets Epoch time, \"epochTime_s\" format: NNNNNN - seconds since Jan 1 1970 UTC\n\r"\
																		"set time -timezone {timeZone_h}: Sets the timezone, \"timeZone_h\" format (-)NN houres, NN <= 12\n\r"
#define GLOBAL_GET_TIME_HELP_TXT										"get time [-timeofday | -tod [-utc]] [-epochtime] [-timezone] [-daylightsaving] [-ntpservers] [-ntpsyncstatus] [-ntpsyncmode] [-ntpopstate]\n\r"\
																		"get time -timeofday | -tod [-utc]: Prints the local or universal time\n\r"\
																		"get time -epochtime: Prints the epoch time - seconds sinse Jan 1 1970 UTC\n\r"\
																		"get time -timezone: Prints current time-zone\n\r"\
																		"get time -daylightsaving: Prints daylight-saving status\n\r"\
																		"get time -ntpservers: Prints the current status of all provisioned NTP servers\n\r"\
																		"get time -ntpsyncstatus: Prints the current NTP client sync status\n\r"\
																		"get time -ntpsyncmode: Prints the current NTP client sync mode\n\r"\
																		"get time -ntpopstate: Prints the current NTP client operational state\n\r"
#define GLOBAL_SHOW_TIME_HELP_TXT										"show time: Prints a summary of time information\n\r"

#define GLOBAL_SET_LOG_HELP_TXT											"set log [-loglevel {logLevel} [-logmo {logMo}]] [-logdestination {logDestination}]\n\r"\
																		"set log -loglevel {logLevel}[-logmo {logMo}]: Sets the loglevel \"logLevel\" to \"DEBUG-SILENT\"|\"DEBUG-PANIC\"|\"DEBUG-ERROR\"|\"DEBUG-WARN\"|\"DEBUG-INFO\"|\"DEBUG-TERSE\"|\"DEBUG-VERBOSE\"\n\r"\
																		"                                              if \"logmo {logMo}\" is provided, the given loglevel is only valid for the provided \"logMo\" (managed object) - NOT YET SUPPORTED\n\r"\
																		"set log -logdestination {logDestination}: Sets a remote log destination - NOT YET SUPPORTED\n\r"
#define GLOBAL_UNSET_LOG_HELP_TXT										"unset log -logdestination: Terminates logging to the previously provisioned remote log destination - NOT YET SUPPORTED\n\r"
#define GLOBAL_GET_LOG_HELP_TXT											"get log [-loglevel [-logmo {logMo}]] [-logdestination] [-tail {lines}]\n\r"\
																		"get log -loglevel [-logmo {logMo}] Prints the current loglevel, if \"logmo {logMo}\" is provided, the loglevel for the provided \"logMo\" (managed object) is given - NOT YET SUPPORTED\n\r"\
																		"get log - logdestination: Prints the remote loglevel destination - NOT YET SUPPORTED\n\r"\
																		"get log -tail {lines}: Prints the last \"lines\" of the log - NOT YET SUPPORTED\n\r"
#define GLOBAL_SHOW_LOG_HELP_TXT										"show log: Prints a summary of log information\n\r"

#define COMMON_SET_DEBUG_HELP_TXT										"[{context-path}/]set debug: Sets the debug state of current-, or provided"\
																		"context-path/Managed object\n\r"
#define COMMON_UNSET_DEBUG_HELP_TXT										"[{context-path}/]unset debug: Un-sets the debug state of current-, or provided"\
																		"context-path/Managed object\n\r"
#define COMMON_GET_DEBUG_HELP_TXT										"[{context-path}/]get debug: Prints the debug state of current-, or provided"\
																		"context-path/Managed object\n\r"
#define COMMON_GET_OPSTATE_HELP_TXT										"[{context-path}/]get opstate: Prints operational state of current-, or provided"\
																		"context-path/Managed object\n\r"

#define COMMON_GET_SYSNAME_HELP_TXT										"[{context-path}/]get systemname: Prints system-name of current-, or provided"\
																		"context-path/Managed object\n\r"

#define COMMON_SET_SYSNAME_HELP_TXT										"[{context-path}/]set systemname {system-name}: Sets system-name of current-, "\
																		"or provided context-path/Managed object\n\r"

#define COMMON_GET_USRNAME_HELP_TXT										"[{context-path}/]get username: Prints user-name of current-, or provided"\
																		"context-path/Managed object\n\r"

#define COMMON_SET_USRNAME_HELP_TXT										"[{context-path}/]set username {user-name}: Sets user-name of current-, "\
																		"or provided context-path/Managed object\n\r"

#define COMMON_GET_DESCRIPTION_HELP_TXT									"[{context-path}/]get description: Prints description of current-, or provided"\
																		"context-path/Managed object\n\r"

#define COMMON_SET_DESCRIPTION_HELP_TXT									"[{context-path}/]set username {description}: Sets description of current-, "\
																		"or provided context-path/Managed object\n\r"

#define DECODER_GET_FAILSAFE_HELP_TXT									"[{context-path}/]get failsafe: Prints the current fail-safe state"

#define DECODER_SET_FAILSAFE_HELP_TXT									"[{context-path}/]set failsafe: Sets/activates fail-safe state. Debug state needs to be activated before fail-safe can be activated by CLI"

#define DECODER_UNSET_FAILSAFE_HELP_TXT									"[{context-path}/]unset failsafe: Unsets/deactivates fail-safe state. Debug state needs to be activated before fail-safe can be de-activated by CLI"

#define LGLINKNO_GET_LGLINK_HELP_TXT									"[{context-path}/]get link: Prints the lgLink instance number - the link"

#define LGLINKNO_SET_LGLINK_HELP_TXT									"[{context-path}/]set link {lgKink-number}: Sets the lgLink instance number - the link, debug mode needs to be activated to perform this action"

#define LGLINKNO_GET_LGLINKOVERRUNS_HELP_TXT							"[{context-path}/]get overruns: Prints the accumulated number of lgLink over-runs, I.e. for which the lgLink scan had not finished before the next was due"

#define LGLINKNO_CLEAR_LGLINKOVERRUNS_HELP_TXT							"[{context-path}/]clear overruns: Clears the counter for accumulated lgLink over-runs"

#define LGLINKNO_GET_LGLINKMEANLATENCY_HELP_TXT							"[{context-path}/]get meanlatency: Prints the mean latency of the lgLink scan, E.g. how much its start was delayed compared to schedule"

#define LGLINKNO_GET_LGLINKMAXLATENCY_HELP_TXT							"[{context-path}/]get maxlatency: Prints the maximum latency of the lgLink scan, E.g. how much its start was delayed compared to schedule"

#define LGLINKNO_CLEAR_LGLINKMAXLATENCY_HELP_TXT						"[{context-path}/]clear maxlatency: Clears the maximum latency watermark of the lgLink scan"

#define LGLINKNO_GET_LGLINKMEANRUNTIME_HELP_TXT							"[{context-path}/]get meanruntime: Prints the mean run-time of the lgLink scan, E.g. how long the linkscan took"

#define LGLINKNO_GET_LGLINKMAXRUNTIME_HELP_TXT							"[{context-path}/]get maxruntime: Prints the maximum run-time of the lgLink scan, E.g. how long the linkscan took"

#define LGLINKNO_CLEAR_LGLINKMAXRUNTIME_HELP_TXT						"[{context-path}/]clear maxruntime: Clears the maximum run-time watermark of the lgLink scan"

#define LG_GET_LGADDR_HELP_TXT											"[{context-path}/]get address: Prints the Lg-link adsress of the Lightgroup"

#define LG_SET_LGADDR_HELP_TXT											"[{context-path}/]set address: Sets the Lg-link adsress of the Lightgroup, debug mode needs to be activated to perform this action"

#define LG_GET_LGLEDCNT_HELP_TXT										"[{context-path}/]get ledcnt: Prints the number of Leds/pixels for the light group"

#define LG_SET_LGLEDCNT_HELP_TXT										"[{context-path}/]set ledcnt {ledcnt}: Sets the number of Leds/pixels for the light group, debug mode needs to be activated to perform this action"

#define LG_GET_LGLEDOFFSET_HELP_TXT										"[{context-path}/]get ledoffset: Prints the Lightgrop LED offset on the Lg-link"

#define LG_SET_LGLEDOFFSET_HELP_TXT										"[{context-path}/]set ledoffset {ledoffset}: Sets the Lightgrop LED offset on the Lg-link, debug mode needs to be activated to perform this action"

#define LG_GET_LGPROPERTY_HELP_TXT										"[{context-path}/]get property [{property-index}]: Prints the Lightgrop properties, if property index is provided the property corresponding to the index is provided, else all properties are provided"

#define LG_SET_LGPROPERTY_HELP_TXT										"[{context-path}/]set property {property-index} {property}: Sets the Lightgrop property according to given property index, debug mode needs to be activated to perform this action"

#define LG_GET_LGSHOWING_HELP_TXT										"[{context-path}/]get showing: Prints the Light-group current aspect/show"

#define LG_SET_LGSHOWING_HELP_TXT										"[{context-path}/]set showing {aspect/show}: Sets the Light-group current aspect/show, debug mode needs to be activated to perform this action"

#define SATLINK_GET_SATLINKNO_HELP_TXT									"[{context-path}/]get link: Prints the Satelite-link number"

#define SATLINK_SET_SATLINKNO_HELP_TXT									"[{context-path}/]set link {satelite-link}: Sets the Satelite-link number, debug mode needs to be activated to perform this action"

#define SATLINK_GET_TXUNDERRUNS_HELP_TXT								"[{context-path}/]get txunderrun: Prints number of Satelite-link TX underruns"

#define SATLINK_CLEAR_TXUNDERRUNS_HELP_TXT								"[{context-path}/]clear txunderrun: Clears the Satelite-link TX underruns counter"

#define SATLINK_GET_RXOVERRUNS_HELP_TXT									"[{context-path}/]get rxoverrun: Prints number of Satelite-link RX overruns"

#define SATLINK_CLEAR_RXOVERRUNS_HELP_TXT								"[{context-path}/]clear rxoverrun: Clears the Satelite-link RX overruns counter"

#define SATLINK_GET_TIMINGVIOLATION_HELP_TXT							"[{context-path}/]get timingviolation: Prints number of Satelite-link timingviolations"

#define SATLINK_CLEAR_TIMINGVIOLATION_HELP_TXT							"[{context-path}/]clear timingviolation: Clears the Satelite-link timingviolation counter"

#define SATLINK_GET_RXCRCERR_HELP_TXT									"[{context-path}/]get rxcrcerr: Prints number of Satelite-link RX CRC errors"

#define SATLINK_CLEAR_RXCRCERR_HELP_TXT									"[{context-path}/]clear rxcrcerr: Clears the Satelite-link RX CRC error counter"

#define SATLINK_GET_REMOTECRCERR_HELP_TXT								"[{context-path}/]get remotecrcerr: Prints number of Satelite-link RX CRC errors"

#define SATLINK_CLEAR_REMOTECRCERR_HELP_TXT								"[{context-path}/]clear remotecrcerr: Clears the Satelite-link RX CRC error counter"

#define SATLINK_GET_RXSYMERRS_HELP_TXT									"[{context-path}/]get symerr: Prints number of Satelite-link RX symbol errors"

#define SATLINK_CLEAR_RXSYMERRS_HELP_TXT								"[{context-path}/]clear symerr: Clears the Satelite-link RX symbol error counter"

#define SATLINK_GET_RXSIZEERRS_HELP_TXT									"[{context-path}/]get rxsizeerr: Prints number of Satelite-link RX size errors"

#define SATLINK_CLEAR_RXSIZEERRS_HELP_TXT								"[{context-path}/]clear rxsizeerr: Clears the Satelite-link RX size error counter"

#define SATLINK_GET_WDERRS_HELP_TXT										"[{context-path}/]get wderr: Prints number of Satelite-link watchdog errors, I.e aggregated watchdog error reported by Satelites on the Satelite-link"

#define SATLINK_CLEAR_WDERRS_HELP_TXT									"[{context-path}/]clear wderr: Clears the Satelite-link watchdog error counter"

#define SAT_GET_SATADDR_HELP_TXT										"[{context-path}/]get address: Prints the Satelite address"

#define SAT_SET_SATADDR_HELP_TXT										"[{context-path}/]set address {Satelite-address}: Sets the Satelite address, debug mode needs to be activated to perform this action"

#define SAT_GET_SATRXCRCERR_HELP_TXT									"[{context-path}/]get rxcrcerr: Prints Satelite RX CRC errors"

#define SAT_CLEAR_SATRXCRCERR_HELP_TXT									"[{context-path}/]clear rxcrcerr: Clears the Satelite RX CRC error counter"

#define SAT_GET_SATTXCRCERR_HELP_TXT									"[{context-path}/]get txcrcerr: Prints Satelite TX CRC errors"

#define SAT_CLEAR_SATTXCRCERR_HELP_TXT									"[{context-path}/]clear txcrcerr: Clears the Satelite TX CRC error counter"

#define SAT_GET_SATWDERR_HELP_TXT										"[{context-path}/]get wderr: Prints Satelite watchdog CRC errors"

#define SAT_CLEAR_SATWDERR_HELP_TXT										"[{context-path}/]clear wderr: Clears the Satelite watchdog error counter"

#define SENS_GET_SENSPORT_HELP_TXT										"[{context-path}/]get port: Prints the Sensor port number"

#define SENS_SET_SENSPORT_HELP_TXT										"[{context-path}/]set port {sensor-port}: Sets the Sensor port number, debug mode needs to be activated to perform this action"

#define SENS_GET_SENSING_HELP_TXT										"[{context-path}/]get sensing: Prints current Sensor state"

#define SENS_GET_SENSORPROPERTY_HELP_TXT								"[{context-path}/]get property [{property-index}] : Prints the Sensor properties, if property index is provided the property corresponding to the index is provided, else all properties are provided"

#define SENS_SET_SENSORPROPERTY_HELP_TXT								"[{context-path}/]set property {property-index} : Sets the Sensor property according to given property index, debug mode needs to be activated to perform this action"

#define ACT_GET_ACTPORT_HELP_TXT										"[{context-path}/]get port: Prints the Actuator port"

#define ACT_SET_ACTPORT_HELP_TXT										"[{context-path}/]set port {actuator-port}: Sets the actuator port, debug mode needs to be activated to perform this action"

#define ACT_GET_ACTSHOWING_HELP_TXT										"[{context-path}/]get showing: Prints the actuator showing state"

#define ACT_SET_ACTSHOWING_HELP_TXT										"[{context-path}/]set showing {actuator-showing}: Sets the actuator showing state, debug mode needs to be activated to perform this action"

#define ACT_GET_ACTPROPERTY_HELP_TXT									"[{context-path}/]get property [{property-index}] : Prints the Actuator properties, if property index is provided the property corresponding to the index is provided, else all properties are provided"

#define ACT_SET_ACTPROPERTY_HELP_TXT									"[{context-path}/]set property {property-index} : Sets the Actuator property according to given property index, debug mode needs to be activated to perform this action"

/*==============================================================================================================================================*/
/* End MO CLI help texts                                                                                                                        */
/*==============================================================================================================================================*/
#endif CLIGLOBALDEFINITIONS_H
