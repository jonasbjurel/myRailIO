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

#ifndef CONFIG_H
#define CONFIG_H


/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "logHelpers.h"
#include "esp32SysConfig.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



// Decoder configuration
#define DECODER_DISCOVERY_TIMEOUT_S					60
#define MAX_SATLINKS								2
#define MAX_LGLINKS									2
#define DECODER_CONFIG_TIMEOUT_S					60
#define MQTT_DEFAULT_URI							"myJmri.mqtt.org" //*** Needs to be aligned across the project
#define MQTT_DEFAULT_PORT							"1883"
#define MQTT_DEFAULT_KEEPALIVEPERIOD_S				10.0
#define NTP_DEFAULT_URI								"se.pool.ntp.org"
#define NTP_DEFAULT_PORT							123
#define NTP_DEFAULT_TZ								0
#define DEFAULT_LOGLEVEL							DEBUG_INFO
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
#define MAX_SATELITES								8	

// Satelite configuration
// ======================

// Sensor configuration
// ====================
#define DEFAULT_SENS_FILTER_TIME					5	// Digital sensor filter (ms)
#define MAX_SENS									8	// Maximum sensors // Maximum satelites per satelite link
#define SATLINK_UPDATE_MS							5	// Satelite scan period (ms)

// Actuator configuration
// ======================
#define MAX_ACT										4	// Maximum actuators per satelite

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
#define MAX_LGSTRIPLEN								32
#define STRIP_UPDATE_MS								5					//Lightgoups 2811 strip update periond [ms]

// Light group configuration
// =========================

// Signal mast Light group configuration
#define SM_DIM_NORMAL_MS							75
#define SM_DIM_FAST_MS								40
#define SM_DIM_SLOW_MS								150
#define SM_BRIGHNESS_HIGH							80
#define SM_BRIGHNESS_NORMAL							40
#define SM_BRIGHNESS_LOW							20
#define SM_BRIGHNESS_FAIL							120
#define SM_FLASH_FAST								FLASH_TYPE_1_5_HZ
#define SM_FLASH_NORMAL								FLASH_TYPE_1_0_HZ
#define SM_FLASH_SLOW								FLASH_TYPE_0_5_HZ

// MQTT Configuration
// ==================
#define MQTT_DEFAULT_URI							"genJMRImqtt.org"
#define MQTT_DEFAULT_PORT							1883
#define MQTT_DEFAULT_CLIENT_ID						"Generic JMRI decoder"
#define MQTT_DEFAULT_QOS							MQTT_QOS_0
#define MQTT_DEFAULT_KEEP_ALIVE_S					10.0
#define MQTT_DEFAULT_PINGPERIOD_S					10.0
#define MAX_MQTT_LOST_PINGS							3
#define MQTT_BUFF_SIZE								16384
#define MQTT_POLL_PERIOD_MS							100
#define MQTT_CONNECT_TIMEOUT_S						60
#define MAX_MQTT_CONNECT_ATTEMPTS					10

// CPU execution parameters
// ========================
// MQTT message polling
#define CPU_MQTT_POLL_CORE							CORE_1
#define CPU_MQTT_POLL_PRIO							10
#define CPU_MQTT_POLL_STACKSIZE_1K					6
#define CPU_MQTT_POLL_TASKNAME						"mqttPoll"

// MQTT message supervision
#define CPU_MQTT_PING_CORE							CORE_0
#define CPU_MQTT_PING_PRIO							10
#define CPU_MQTT_PING_STACKSIZE_1K					6
#define CPU_MQTT_PING_TASKNAME						"mqttPing"

// Satelit link
const uint8_t CPU_SATLINK_CORE[] = { CORE_1, CORE_0 };
#define CPU_SATLINK_PRIO							10
#define CPU_SATLINK_STACKSIZE_1K					6
#define CPU_SATLINK_TASKNAME						"satLink %d"

const uint8_t CPU_SATLINK_PM_CORE[] =				{CORE_0, CORE_1};
#define CPU_SATLINK_PM_PRIO							10
#define CPU_SATLINK_PM_STACKSIZE_1K					6
#define CPU_SATLINK_PM_TASKNAME						"satLinkPmPoll %d"

// Flash
const uint8_t FLASH_LOOP_CORE[] =					{CORE_1, CORE_1};
#define FLASH_LOOP_PRIO								10

// LgLink
const uint8_t  CPU_UPDATE_STRIP_CORE[] =			{CORE_0, CORE_1};
#define CPU_UPDATE_STRIP_PRIO						10
#define CPU_UPDATE_STRIP_STACKSIZE_1K				6
#define CPU_UPDATE_STRIP_TASKNAME					"lgLinkStripHandler"

// WIFI parameters
// ===============
#define WIFI_ESP_MANUFACTURER						"ESPRESSIF" 
#define WIFI_ESP_MODEL_NUMBER						"ESP32"
#define WIFI_ESP_MODEL_NAME							"ESPRESSIF IOT"
#define WIFI_ESP_DEVICE_NAME						"ESP STATION"
#define WIFI_ESP_HOSTNAME_PREFIX					"genJmriDecoder"

/*
#define CLI_POLL_PRIO                 5
#define CPU_PM_POLL_PRIO              10
#define CLI_POLL_CORE                 CORE_1
#define CPU_PM_CORE                   CORE_1
*/
#endif /*CONFIG_H*/
