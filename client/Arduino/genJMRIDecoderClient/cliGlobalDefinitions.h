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
#ifndef CLIGLOBALDEFINITIONS_H
#define CLIGLOBALDEFINITIONS_H



/*==============================================================================================================================================*/
/* Managed object (MO) definitions                                                                                                              */
/*==============================================================================================================================================*/
#define ROOT_MO_NAME													"root"

#define GLOBAL_MO_NAME													"global"				// Help text definition:
#define		CONTEXT_SUB_MO_NAME												"context"
#define		UPTIME_SUB_MO_NAME												"uptime"
#define		WIFI_SUB_MO_NAME												"wifi"
#define		TOPOLOGY_SUB_MO_NAME											"topology"
#define		MQTTBROKER_SUB_MO_NAME											"mqttbroker"
#define		MQTTPORT_SUB_MO_NAME											"mqttport"
#define		KEEPALIVE_SUB_MO_NAME											"mqttkeepalive"
#define		PINGSUPERVISION_SUB_MO_NAME										"supervision"
#define		NTPSERVER_SUB_MO_NAME											"ntpserver"
#define		NTPPORT_SUB_MO_NAME												"ntpport"
#define		TZ_SUB_MO_NAME													"tz"
#define		LOGLEVEL_SUB_MO_NAME											"loglevel"
#define		FAILSAFE_SUB_MO_NAME											"failsafe"

#define COMMON_MO_NAME													"common"
#define		OPSTATE_SUB_MO_NAME												"opstate"
#define		SYSNAME_SUB_MO_NAME												"systemname"
#define		USER_SUB_MO_NAME												"username"
#define		DESC_SUB_MO_NAME												"description"
#define		DEBUG_SUB_MO_NAME												"debug"

#define	DECODER_MO_NAME													"decoder"
// All decoder MOs are mapped to the global context

#define LGLINK_MO_NAME													"lglink"
#define		LGLINKNO_SUB_MO_NAME											"link"
#define		LGLINKOVERRUNS_SUB_MO_NAME										"overruns"
#define		LGLINKMEANLATENCY_SUB_MO_NAME									"meanlatency"
#define		LGLINKMAXLATENCY_SUB_MO_NAME									"maxlatency"
#define		LGLINKMEANRUNTIME_SUB_MO_NAME									"meanruntime"
#define		LGLINKMAXRUNTIME_SUB_MO_NAME									"maxruntime"

#define LG_MO_NAME														"lightgroup"
#define		LGADDR_SUB_MO_NAME												"address"
#define		LGLEDCNT_SUB_MO_NAME											"ledcnt"
#define		LGLEDOFFSET_SUB_MO_NAME											"ledoffset"
#define		LGPROPERTY_SUB_MO_NAME											"property"
#define		LGSTATE_SUB_MO_NAME												"state"
#define		LGSHOWING_SUB_MO_NAME											"showing"

#define SATLINK_MO_NAME													"satelitelink"
#define		SATLINKNO_SUB_MO_NAME											"link"
#define		SATLINKTXUNDERRUN_SUB_MO_NAME									"txunderrun"
#define		SATLINKRXOVERRUN_SUB_MO_NAME									"rxoverrun"
#define		SATLINKTIMINGVIOLATION_SUB_MO_NAME								"timingviolation"
#define		SATLINKRXCRCERR_SUB_MO_NAME										"rxcrcerr"
#define		SATLINKREMOTECRCERR_SUB_MO_NAME									"remotecrcerr"
#define		SATLINKRXSYMERRS_SUB_MO_NAME									"rxsymbolerr"
#define		SATLINKRXSIZEERRS_SUB_MO_NAME									"rxsizeerr"
#define		SATLINKWDERRS_SUB_MO_NAME										"wderr"

#define SAT_MO_NAME														"satelite"
#define		SATADDR_SUB_MO_NAME												"address"
#define		SATRXCRCERR_SUB_MO_NAME											"rxcrcerr"
#define		SATTXCRCERR_SUB_MO_NAME											"txcrcerr"
#define		SATWDERR_SUB_MO_NAME											"wderr"

#define SENSOR_MO_NAME													"sensor"
#define		SENSPORT_SUB_MO_NAME											"port"
#define		SENSSENSING_SUB_MO_NAME											"sensing"
#define		SENSORPROPERTY_SUB_MO_NAME										"property"

#define ACTUATOR_MO_NAME												"actuator"
#define		ACTUATORPORT_SUB_MO_NAME										"port"
#define		ACTUATORSHOWING_SUB_MO_NAME										"showing"
#define		ACTUATORPROPERTY_SUB_MO_NAME									"property"

/*==============================================================================================================================================*/
/* End MO definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Managed object (MO) CLI help texts                                                                                                           */
/*==============================================================================================================================================*/
#define GLOBAL_HELP_HELP_TXT											"[{context-path}/]help [-m MO-object][{MO-subobject}]:\nProvides the full CLI help text, or only "\
																		"the help text for a particular managed sub-object related to the context/managed"\
																		"object. If [-m {MO-objecttype}] is provided, the help text for the MO-subobject "\
																		"is related to the given MO-object type rather than current context. Use \"show "\
																		" motypes\" to see available MO-types\n"

#define GLOBAL_FULL_CLI_HELP_TXT										"The genJMRIDecoder CLI provides interactive management capabilities throughout its"\
																		"management object types:\n"\
																		"- Global\n"\
																		"- Common\n"\
																		"- Decoder\n"\
																		"	- Lightgroup-link\n"\
																		"		- Lightgroup\n"\
																		"	- Satelite-link\n"\
																		"		- Satelite\n"\
																		"			- Actuator\n"\
																		"			- Sensor\n"\
																		"The managed objects are not directly exposed in the CLI, but are indirectly referred"\
																		"to by the current- or temporary- CLI context. A CLI context is a hierarchical"\
																		"name-space which can only operate on the given managed-object - and instance sub-managed"\
																		"objects, with three exeptions:\n"\
																		"- Global management objects are available in any CLI context, and operates on system wide singelton objects\n"\
																		"- Common management objects are available for all CLI contexts and operates on the current/or indicated"\
																		"CLI context.\n"\
																		"CLI context can ne provided in two ways:\n"\
																		"- Current CLI context represents the CLI context pat that is currently active, the current"\
																		"active CLI context tree path is shown at the CLI prompt: \"{Current active absolute CLI context-path}>> \""\
																		"or you can retreive it by the command: \">> set context {absolute CLI context-path}|{rellative context-path}\"\n"\
																		"is shown\n"\
																		"- Temporary context represents a method to redirect a single command to a different CLI context than the current."\
																		"the temporary CLI context is limited to a single command and the current CLI context remains as before"\
																		"after the command has executed. The temporary CLI context is prepended the CLI command and can be an"\
																		"absolute or a rellative CLI context path: Ie: [{absolute-CLI-Context-Path}|{rellative-CLI-Context-Path}]command [parameters]\n"\
																		"\n"\
																		"Available genJMRI CLI commands:\n"\
																		"helpCliCmd [args]\n"\
																		"reboot\n"\
																		"show [args]\n"\
																		"get [args]\n"\
																		"set [args]\n"\
																		"unset [args]\n"\
																		"clear [args]\n"\
																		"add [args]\n"\
																		"delete [args]\n"\
																		"copy [args]\n"\
																		"paste [args]\n"\
																		"move [args]\n"\
																		"start [args]\n"\
																		"stop [args]\n"\
																		"restart [args]\n"

#define GLOBAL_MOTYPES_HELP_TXT											"show motypes: Prints all available Manage object types"

#define GLOBAL_REBOOT_HELP_TXT											"reboot:\nReboots the decoder\n"

#define GLOBAL_GET_CONTEXT_HELP_TXT										"get context:\nPrints current context/Managed object\n"

#define GLOBAL_SET_CONTEXT_HELP_TXT										"set context {context-path}:\nSets current context/Managed object path. If context-path"\
																		"begins with \"/\" it is considered to be an absolute path, other wise it is considered"\
																		"to be a rellative path. \"..\" is used to decend one level in the context/Managed object"\
																		"tree.\n"

#define GLOBAL_GET_UPTIME_HELP_TXT										"get uptime:\nPrints the time in seconds sinse the previous reboot.\n"

#define GLOBAL_GET_WIFI_HELP_TXT										"get (wifi,ip) [-addr] [-mask] [-gw] [-dns] [-ntp] [-host/name] [-broker]\n"\
																		"Prints current wifi/ip configuration, if no argument is given - all wifi/ip "\
																		"configurations will be printed, otherwise only those selected by provided "\
																		"arguments will be printed. Available arguments:\n"\
																		"-addr: IPv4 address\n"\
																		"-mask: IPv4 mask\n"\
																		"-gw: Default gateway\n"\
																		"-dns: DNS server\n"\
																		"-ntp: NTP server\n"\
																		"-host/name: Host name\n"\
																		"-broker: MQTT broker\n"

#define GLOBAL_SHOW_TOPOLOGY_HELP_TXT									"show topology [{root-path}]: Shows decoder context/Manage object topology-tree."\
																		"if \"{root-path}\" is given, the topology-tree shown will start from that "\
																		"path-junction and, otherwise the topology shown will start from the current "\
																		"context-path. Eg. if \"/\" is given, the printout will start from the decoder-root"\
																		"showing the full topology.\n"

#define COMMON_GET_OPSTATE_HELP_TXT										"[{context-path}/]get opstate: Prints operational state of current-, or provided"\
																		"context-path/Managed object\n"

#define COMMON_GET_SYSNAME_HELP_TXT										"[{context-path}/]get systemname: Prints system-name of current-, or provided"\
																		"context-path/Managed object\n"

#define COMMON_GET_SYSNAME_HELP_TXT										"[{context-path}/]set systemname {system-name}: Sets system-name of current-, "\
																		""or provided context-path/Managed object\n"

#define COMMON_GET_SYSNAME_HELP_TXT										"[{context-path}/]get systemname: Prints system-name of current-, or provided"\
																		"context-path/Managed object\n"

#define COMMON_SET_SYSNAME_HELP_TXT										"[{context-path}/]set systemname {system-name}: Sets system-name of current-, "\
																		"or provided context-path/Managed object\n"

#define COMMON_GET_USRNAME_HELP_TXT										"[{context-path}/]get username: Prints user-name of current-, or provided"\
																		"context-path/Managed object\n"

#define COMMON_SET_USRNAME_HELP_TXT										"[{context-path}/]set username {user-name}: Sets user-name of current-, "\
																		"or provided context-path/Managed object\n"

#define COMMON_GET_DESCRIPTION_HELP_TXT									"[{context-path}/]get description: Prints description of current-, or provided"\
																		"context-path/Managed object\n"

#define COMMON_SET_DESCRIPTION_HELP_TXT									"[{context-path}/]set username {description}: Sets description of current-, "\
																		"or provided context-path/Managed object\n"

#define DECODER_GET_MQTTBROKER_HELP_TXT									"[{context-path}/]get mqttbroker: Prints the MQTT broker URI"

#define DECODER_SET_MQTTBROKER_HELP_TXT									"[{context-path}/]set mqttbroker {broker-url}: Sets the MQTT broker URI (DNS-name or IPv4 address)"

#define DECODER_GET_MQTTPORT_HELP_TXT									"[{context-path}/]get mqttport: Prints the MQTT IP-port"

#define DECODER_SET_MQTTPORT_HELP_TXT									"[{context-path}/]set mqttport {broker-url}: Sets the MQTT IP-port"

#define DECODER_GET_KEEPALIVE_HELP_TXT									"[{context-path}/]get mqttkeepalive: Prints the MQTT keep-alive period in seconds"

#define DECODER_SET_KEEPALIVE_HELP_TXT									"[{context-path}/]set mqttkeepalive {mqtt-kepalive}: Sets the MQTT keep-alive period"

#define DECODER_GET_PINGSUPERVISION_HELP_TXT							"[{context-path}/]get supervision: Prints the server supervision period"

#define DECODER_SET_PINGSUPERVISION_HELP_TXT							"[{context-path}/]set supervision {supervision}: Sets the server supervision period"

#define DECODER_GET_NTPSERVER_HELP_TXT									"[{context-path}/]get ntpserver: Prints the NTP server URI"

#define DECODER_SET_NTPSERVER_HELP_TXT									"[{context-path}/]set ntpserver {ntp-server}: Sets the NTP server URI (DNS-name or IPv4 address)"

#define DECODER_GET_NTPPORT_HELP_TXT									"[{context-path}/]get ntpport: Prints the NTP port"

#define DECODER_SET_NTPPORT_HELP_TXT									"[{context-path}/]set ntpport {ntp-port}: Sets the NTP port"

#define DECODER_GET_TZ_HELP_TXT											"[{context-path}/]get tz: Prints the time-zone port"

#define DECODER_SET_TZ_HELP_TXT											"[{context-path}/]set ntpport {tz}: Sets the time-zone"

#define DECODER_GET_LOGLEVEL_HELP_TXT									"[{context-path}/]get loglevel: Prints the current loglevel"

#define DECODER_SET_LOGLEVEL_HELP_TXT									"[{context-path}/]set loglevel {fatal|error|notice|verbose}: Sets current loglevel"

#define DECODER_GET_FAILSAFE_HELP_TXT									"[{context-path}/]get failsafe: Prints the current fail-safe state"

#define DECODER_SET_FAILSAFE_HELP_TXT									"[{context-path}/]set failsafe: Sets/activates fail-safe state. Debug state needs to be activated before fail-safe can be activated by CLI"

#define DECODER_UNSET_FAILSAFE_HELP_TXT									"[{context-path}/]unset failsafe: Unsets/deactivates fail-safe state. Debug state needs to be activated before fail-safe can be de-activated by CLI"

#define DECODER_GET_DEBUG_HELP_TXT										"[{context-path}/]get failsafe: Prints the current fail-safe state"

#define DECODER_SET_DEBUG_HELP_TXT										"[{context-path}/]set failsafe: Sets/activates fail-safe state. Debug state needs to be activated before fail-safe can be activated by CLI"

#define DECODER_UNSET_DEBUG_HELP_TXT									"[{context-path}/]unset failsafe: Unsets/deactivates fail-safe state. Debug state needs to be activated before fail-safe can be de-activated by CLI"

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
