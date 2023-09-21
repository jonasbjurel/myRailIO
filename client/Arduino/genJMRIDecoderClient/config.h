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

#ifndef CONFIG_H
#define CONFIG_H


/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "esp32SysConfig.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/

// Decoder configuration
#define DECODER_DISCOVERY_TIMEOUT_S					60
#define MAX_SATLINKS								1	//2
#define MAX_LGLINKS									1	//2
#define DECODER_CONFIG_TIMEOUT_S					60
#define MQTT_DEFAULT_KEEPALIVEPERIOD_S				10.0
#define NTP_DEFAULT_URI								"se.pool.ntp.org"
#define NTP_DEFAULT_PORT							123
#define NTP_DEFAULT_TZ_AREA							"CET"
#define NTP_DEFAULT_TZ_GMTOFFSET					+1
#define NTP_MAX_NTPSERVERS							3
#define NTP_POLL_PERIOD_S							60
#define NTP_DEFAULT_SYNCMODE						SNTP_SYNC_MODE_SMOOTH //SNTP_SYNC_MODE_SMOOTH | SNTP_SYNC_MODE_IMMED
#define DEFAULT_LOGLEVEL							GJMRI_DEBUG_VERBOSE
#define LOG_MSG_HISTORY_SIZE						30
#define RSYSLOG_DEFAULT_PORT						514
#define DEFAULT_FAILSAFE							"Yes"

// Flash Configuration
#define FLASH_0_5_HZ								0.5
#define FLASH_1_0_HZ								1.0
#define FLASH_1_5_HZ								1.5
#define FLASH_TYPES									[FLASH_0_5_HZ, FLASH_1_0_HZ, FLASH_1_5_HZ]
#define FLASH_TYPE_0_5_HZ							0
#define FLASH_TYPE_1_0_HZ							1
#define FLASH_TYPE_1_5_HZ							2

// Satelite link configuration
// ===========================
#define MAX_SATELITES								1 //8 Max satelites for each Satelite link
#define SATLINK_LINKERR_HIGHTRES					10 // Sum of all Link CRC- & Symbol errors over a second that will trigger ERRSEC
#define SATLINK_LINKERR_LOWTRES						0 // Sum of all Link CRC- & Symbol errors over a second that will trigger ERRSEC


// Satelite configuration
// ======================
#define SAT_LINKERR_HIGHTRES						2 // Sum of all Link CRC- & Symbol errors over a second that will trigger ERRSEC
#define SAT_LINKERR_LOWTRES							0 // Sum of all Link CRC- & Symbol errors over a second that will trigger ERRSEC

// Sensor configuration
// ====================
#define DEFAULT_SENS_FILTER_TIME					5	// Digital sensor filter (ms)
#define MAX_SENS									8	//8 Maximum sensors // Maximum satelites per satelite link
#define SATLINK_UPDATE_MS							5	// Satelite scan period (ms)
#define SENSDIGITAL_DEFAULT_FAILSAFE				true

// Actuator configuration
// ======================
#define MAX_ACT										4	//4 Maximum actuators per satelite

// General servo configuration
#define SERVO_LEFT_PWM_VAL							26
#define SERVO_RIGHT_PWM_VAL							50

// Memory actuators configuration
#define ACTMEM_DEFAULT_SOLENOID_ACTIVATION_TIME_MS	200
#define ACTMEM_DEFAULT_FAILSAFE						"0"

// Light actuators configuration
#define ACTLIGHT_DEFAULT_FAILSAFE					0

// Turnout actuators configuration
#define TURN_DEFAULT_FAILSAFE                       TURN_CLOSED_POS		//TURN_CLOSED_POS | TURN_THROWN_POS
#define TURN_SERVO_DEFAULT_THROWTIME_MS             1000
#define TURN_SOLENOID_DEFAULT_THROWTIME_MS          200
#define TURN_SERVO_DEFAULT_THROW_TRIM               0
#define TURN_SERVO_DEFAULT_CLOSED_TRIM              0
#define TURN_PWM_UPDATE_TIME_MS                     100

// Light group link configuration
// ==============================
#define MAX_LGSTRIPLEN								32					//32x3 Monochrome Pixels
#define STRIP_UPDATE_MS								5					//Lightgoups 2811 strip update periond [ms]

// Light group configuration
// =========================
#define MAX_LGS										1	//16

// Signal mast Light group configuration
#define SM_DIM_NORMAL_MS							75
#define SM_DIM_FAST_MS								40
#define SM_DIM_SLOW_MS								150
#define SM_BRIGHNESS_HIGH							80
#define SM_BRIGHNESS_NORMAL							40
#define SM_BRIGHNESS_LOW							20
#define SM_BRIGHNESS_FAIL							5
#define SM_FLASH_FAST								FLASH_TYPE_1_5_HZ
#define SM_FLASH_NORMAL								FLASH_TYPE_1_0_HZ
#define SM_FLASH_SLOW								FLASH_TYPE_0_5_HZ
#define SM_DUTY_HIGH								180									//NOT YET IMPLEMENTED
#define SM_DUTY_NORMAL								127									//NOT YET IMPLEMENTED
#define SM_DUTY_LOW									80									//NOT YET IMPLEMENTED

// MQTT Configuration
// ==================
#define MQTT_DEFAULT_URI							"JMRImqtt.org"
#define MQTT_DEFAULT_PORT							1883
#define MQTT_DEFAULT_CLIENT_ID						"genJMRIDecoder"
#define MQTT_DEFAULT_QOS							MQTT_QOS_0
#define MQTT_DEFAULT_KEEP_ALIVE_S					60
#define MQTT_DEFAULT_PINGPERIOD_S					10.0
#define MAX_MQTT_LOST_PINGS							3
#define MQTT_BUFF_SIZE								50000
#define MQTT_POLL_PERIOD_MS							50
#define MQTT_CONNECT_TIMEOUT_S						60
#define MAX_MQTT_CONNECT_ATTEMPTS					10

// CPU execution parameters
// ========================
//Task stack attributes
#define INTERNAL									true
#define EXTERNAL									false

// Log job task
#define LOGJOBSLOTS									10
#define LOG_PRIO									5
#define LOG_STACKSIZE_1K							6
#define LOG_TASKNAME								"log"
#define LOG_STACK_ATTR								EXTERNAL


// Setup task
#define SETUP_PRIO									4
#define SETUP_STACKSIZE_1K							10
#define SETUP_TASKNAME								"setup"
#define SETUP_STACK_ATTR							INTERNAL

// MQTT message polling
#define CPU_MQTT_POLL_PRIO							15
#define CPU_MQTT_POLL_STACKSIZE_1K					6
#define CPU_MQTT_POLL_TASKNAME						"mqttPoll"

// System state job task
#define CPU_SYSSTATE_JOB_PRIO						10
#define CPU_SYSSTATE_JOB_STACKSIZE_1K				6
#define CPU_SYSSTATE_JOB_TASKNAME					"sysStateJob"

// MQTT message supervision
#define CPU_MQTT_PING_PRIO							10
#define CPU_MQTT_PING_STACKSIZE_1K					6
#define CPU_MQTT_PING_TASKNAME						"mqttPing"
#define CPU_MQTT_PING_STACK_ATTR					INTERNAL

// Satelite link
const uint8_t CPU_SATLINK_CORE[] =					{ CORE_1, CORE_0 };
#define CPU_SATLINK_PRIO							20
#define CPU_SATLINK_STACKSIZE_1K					6
#define CPU_SATLINK_TASKNAME						"satLink %d"

#define CPU_SATLINK_PM_PRIO							10
#define CPU_SATLINK_PM_STACKSIZE_1K					6
#define CPU_SATLINK_PM_TASKNAME						"satLinkPmPoll %d"
#define CPU_SATLINK_PM_STACK_ATTR					INTERNAL

// Flash
#define FLASH_LOOP_PRIO								10
#define FLASH_LOOP_STACKSIZE_1K						6
#define FLASH_LOOP_TASKNAME							"FlashLoop %d"
#define FLASH_LOOP_STACK_ATTR						INTERNAL

// LgLink
#define CPU_UPDATE_STRIP_PRIO						20
#define CPU_UPDATE_STRIP_STACKSIZE_1K				3
#define CPU_UPDATE_STRIP_TASKNAME					"lgLinkStripHandler %d"
#define CPU_UPDATE_STRIP_SETUP_STACK_ATTR			INTERNAL

// Telnet
#define CPU_TELNET_PRIO								5
#define CPU_TELNET_STACKSIZE_1K						6
#define CPU_TELNET_TASKNAME							"telnetPoll"
#define CPU_TELNET_STACK_ATTR						INTERNAL

// CPU-PM
#define CPU_PM_CORE									CORE_1
#define CPU_PM_PRIO									10
#define CPU_PM_STACKSIZE_1K							1
#define CPU_PM_TASKNAME								"cpuPm"

// WIFI parameters
// ===============
#define WIFI_MGR_AP_NAME_PREFIX						"genJMRIconfAP"							// WiFi provisioning manager Access point name prefix, will be followed by "_<MAC address>"
#define WIFI_MGR_STA_CONNECT_TIMEOUT_S				120										// If WiFi provisioning manager hasn't been able to connect to the provisioned Access point within this time, it will deblock and the program will continue waiting for it to connect
#define WIFI_MGR_AP_CONFIG_TIMEOUT_S				120										// If no client has connected to the WiFi provisioning manager accesspoint within this time, it will deblock with curren current parameters (or factory default)
#define WIFI_WD_TIMEOUT_S							180										// If network connectivity has not been established within this time, reboot escalation counter will be incremented and decoder client will be restarted.
#define WIFI_WD_ESCALATION_CNT_THRES				3										// If the reboot escalation counter-, evaluated att reboot, is above this value, the Wifi manager provisioning access poit as well as the captive is activated to accept new configuration.
#define WIFI_MGR_HTML_TITLE							"genJMRI decoder configuration manager" // WiFi provisioning manager captive portal HTML title
#define WIFI_MGR_AP_IP								10,0,0,2								// WiFi provisioning manager captive portal address
#define WIFI_MGR_AP_GW_IP							10,0,0,1								// WiFi provisioning manager captive portal gateway address
#define WIFI_ESP_MANUFACTURER						"ESPRESSIF"								// Not yet supported
#define WIFI_ESP_MODEL_NUMBER						"ESP32"									// Not yet supported
#define WIFI_ESP_MODEL_NAME							"ESPRESSIF IOT"							// Not yet supported
#define WIFI_ESP_DEVICE_NAME						"ESP STATION"							// Not yet supported
#define WIFI_ESP_HOSTNAME_PREFIX					"genJmriDec"							// Factory default decoder client hostname prefix, will be followed by "_<MAC address>" and can be modified through 
#define WIFI_CONFIG_JSON_OBJ_SIZE					1024									// Config JSON document object size
#define WIFI_CONFIG_JSON_SERIAL_SIZE				1024									// Config JSON document serialized size
#define WIFI_CONFIG_STORE_FILENAME					FS_PATH "/" "WiFiConfig.json"			// Confiuration file path/name

// File system parameters
// ======================
#define FS_PATH										"/spiffs"								// SPIFFS filesystem path

// TELNET parameters
// =================

#endif /*CONFIG_H*/
