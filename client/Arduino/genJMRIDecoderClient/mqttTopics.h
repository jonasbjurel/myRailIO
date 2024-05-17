/*============================================================================================================================================= */
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

#ifndef MQTTTOPICS_H
#define MQTTTOPICS_H

//Mqtt topic prefix
#define MQTT_PRE0_TOPIC_DEFAULT_FRAGMENT		"/trains"
#define MQTT_PRE1_TOPIC_DEFAULT_FRAGMENT		"/track"
#define MQTT_PRE_TOPIC_DEFAULT_FRAGMENT			MQTT_PRE1_TOPIC_DEFAULT_FRAGMENT
//#define MQTT_PRE_TOPIC_DEFAULT_FRAGMENT			"/" MQTT_PRE0_TOPIC_DEFAULT_FRAGMENT "/" MQTT_PRE1_TOPIC_DEFAULT_FRAGMENT


//Mqtt discovery
#define MQTT_DISCOVERY_REQUEST_TOPIC			MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" "discoveryreq"
#define MQTT_DISCOVERY_RESPONSE_TOPIC			MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" "discoveryresp"
#define MQTT_DISCOVERY_REQUEST_PAYLOAD			"<DISCOVERY_REQUEST/>"

//Mqtt supervision
#define MQTT_PING_UPSTREAM_TOPIC				MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" "decodersupervision/upstream"
#define MQTT_PING_DOWNSTREAM_TOPIC				MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" "decodersupervision/downstream"

#define MQTT_PING_PAYLOAD						"<PING/>"


// MQTT object fragments
#define MQTT_DECODER_TOPIC_FRAGMENT				"decoder"
#define MQTT_LGLINK_TOPIC_FRAGMENT				"lglink"
#define MQTT_SATLINK_TOPIC_FRAGMENT				"satLink"
#define MQTT_LG_TOPIC_FRAGMENT					"lightgroup"
#define MQTT_SAT_TOPIC_FRAGMENT					"satelite"
#define MQTT_SENS_TOPIC_FRAGMENT				"sensor"
#define MQTT_ACT_TOPIC_FRAGMENT					"actuator"
#define MQTT_TURN_TOPIC_FRAGMENT				"turnout"
#define MQTT_LIGHT_TOPIC_FRAGMENT				"light"
#define MQTT_MEMORY_TOPIC_FRAGMENT				"memory"
//#define MQTT_SIGNALMAST_TOPIC_FRAGMENT			"signalmast" USING MQTT_LG_TOPIC_FRAGMENT instead


// MQTT System / object states - topics and payload 

// Configuration
#define MQTT_CONFIG_REQ_TOPIC_FRAGMENT			"configReq"
#define MQTT_CONFIG_RESP_TOPIC_FRAGMENT			"configResp"
#define MQTT_CONFIG_REQ_TOPIC					MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/" MQTT_CONFIG_REQ_TOPIC_FRAGMENT	//followed by /uri
#define MQTT_CONFIG_RESP_TOPIC					MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/" MQTT_CONFIG_RESP_TOPIC_FRAGMENT	//followed by /uri

#define MQTT_CONFIG_REQ_PAYLOAD					"<CONFIGURATION_REQUEST/>"

// #define MQTT_CONFIG_RESP_PAYLOAD				XML decoder configuration

// ADM state
#define MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	"admState/downstream"
#define MQTT_DECODER_ADMSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_LGLINK_ADMSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LGLINK_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_SATLINK_ADMSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SATLINK_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_LG_ADMSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LG_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SAT_ADMSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SAT_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SENS_ADMSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SENS_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_ACT_ADMSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_ACT_TOPIC_FRAGMENT "/" MQTT_ADMSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname

#define MQTT_ADM_ON_LINE_PAYLOAD				"<ADMSTATE>ONLINE</ADMSTATE>"
#define MQTT_ADM_OFF_LINE_PAYLOAD				"<ADMSTATE>OFFLINE</ADMSTATE>"

//OP state
#define MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT	"opState/downstream"
#define MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT	"opState/upstream"
#define MQTT_DECODER_OPSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_DECODER_OPSTATE_UPSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_LGLINK_OPSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LGLINK_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_LGLINK_OPSTATE_UPSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LGLINK_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SATLINK_OPSTATE_DOWNSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SATLINK_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_SATLINK_OPSTATE_UPSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SATLINK_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT	//followed by /uri/sysname
#define MQTT_LG_OPSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LG_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_LG_OPSTATE_UPSTREAM_TOPIC			MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LG_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT			//followed by /uri/sysname
#define MQTT_SAT_OPSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SAT_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SAT_OPSTATE_UPSTREAM_TOPIC			MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SAT_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SENS_OPSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SENS_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_SENS_OPSTATE_UPSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SENS_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_ACT_OPSTATE_DOWNSTREAM_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_ACT_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_DOWNSTREAM_FRAGMENT		//followed by /uri/sysname
#define MQTT_ACT_OPSTATE_UPSTREAM_TOPIC			MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_ACT_TOPIC_FRAGMENT "/" MQTT_OPSTATE_TOPIC_UPSTREAM_FRAGMENT		//followed by /uri/sysname

// Statistics
#define MQTT_STATS_TOPIC_FRAGMENT				"statistics"
#define MQTT_SATLINK_STATS_TOPIC				MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SATLINK_TOPIC_FRAGMENT "/" MQTT_STATS_TOPIC_FRAGMENT		//followed by /uri/sysname
#define MQTT_SAT_STATS_TOPIC					MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SAT_TOPIC_FRAGMENT "/" MQTT_STATS_TOPIC_FRAGMENT			//followed by /uri/sysname

// Payload examples:
// MQTT_STATS_TOPIC_FRAGMENT_PAYLOAD


// Decoder reboot:
#define MQTT_DECODER_REBOOT_TOPIC				MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/reboot"

// Payload examples:
#define REBOOT_PAYLOAD							"<REBOOT/>"

// Decoder fetch coredump
#define MQTT_DECODER_FETCHCOREDUMP_TOPIC		MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/coredumprequest"

// Payload examples:
#define FETCHCOREDUMP_PAYLOAD					"<FETCHCOREDUMP/>"

// Decoder deliver coredump
#define MQTT_DECODER_COREDUMP_UPSTREAM_TOPIC	MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_DECODER_TOPIC_FRAGMENT "/coredumpdelivery"

// Payload examples:
#define DELIVERCOREDUMP_XMLTAG_PAYLOAD		"COREDUMP"

// MQTT Object resource business logic topics and payloads
// Sensors
#define MQTT_SENS_TOPIC							MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_SENS_TOPIC_FRAGMENT "/state"										///followed by /sysname
#define MQTT_SENS_DIGITAL_ACTIVE_PAYLOAD		"ACTIVE"
#define MQTT_SENS_DIGITAL_INACTIVE_PAYLOAD		"INACTIVE"
#define MQTT_SENS_DIGITAL_FAILSAFE_PAYLOAD		"FAILSAFE"


// Turnouts
#define MQTT_TURN_TOPIC							MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_TURN_TOPIC_FRAGMENT "/state"										///followed by /sysname
#define MQTT_TURN_SOLENOID_PAYLOAD				"SOLENOID"
#define MQTT_TURN_SERVO_PAYLOAD					"SERVO"
#define MQTT_TURN_CLOSED_PAYLOAD				"CLOSED"
#define MQTT_TURN_THROWN_PAYLOAD				"THROWN"

// Memories
#define MQTT_MEM_TOPIC							MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_MEMORY_TOPIC_FRAGMENT "/state"
#define MQTT_MEM_SOLENOID_PAYLOAD				"SOLENOID"
#define MQTT_MEM_SERVO_PAYLOAD					"SERVO"
#define MQTT_MEM_PWM100_PAYLOAD					"PWM100"
#define MQTT_MEM_PWM1_25K_PAYLOAD				"PWM1_25K"
#define MQTT_MEM_ONOFF_PAYLOAD					"ONOFF"
#define MQTT_MEM_PULSE_PAYLOAD					"PULSE"
#define MQTT_MEM_ON_PAYLOAD						"ON"
#define MQTT_MEM_OFF_PAYLOAD					"OFF"

// Lights
#define MQTT_LIGHT_TOPIC						MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LIGHT_TOPIC_FRAGMENT "/state"

#define MQTT_LIGHT_ON_PAYLOAD					"ON"
#define MQTT_LIGHT_OFF_PAYLOAD					"OFF"


// Signal mast Light group
#define MQTT_ASPECT_TOPIC						MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LG_TOPIC_FRAGMENT "/state"
#define MQTT_ASPECT_REQUEST_TOPIC				MQTT_PRE_TOPIC_DEFAULT_FRAGMENT "/" MQTT_LG_TOPIC_FRAGMENT "/req"

#define MQTT_GETASPECT_PAYLOAD					"<REQUEST>GETLGASPECT</REQUEST>"

//Generic payloads
#define MQTT_BOOL_TRUE_PAYLOAD					"Yes"
#define MQTT_BOOL_FALSE_PAYLOAD					"No"

#endif /*MQTTTOPICS_H*/
