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

#ifndef NETWORKING_H
#define NETWORKING_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <WiFi.h>
#include <esp_wps.h>
#include "config.h"
#include "pinout.h"
#include "libraries/ArduinoLog/ArduinoLog.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: networking                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void (*netwCallback_t)(WiFiEvent_t p_event, arduino_event_info_t p_info, const void* p_args);

#define ESP_WPS_MODE                        WPS_TYPE_PBC                                //Wifi WPS mode set to "push-button
#define WIFI_OP_INIT                        0b00000001
#define WIFI_OP_DISCONNECTED                0b00000010
#define WIFI_OP_NOIP                        0b00000100
#define WIFI_OP_WPS_UNCONFIG                0b00001000

void WiFiEventHelper(WiFiEvent_t p_event, arduino_event_info_t p_info);

class networking {
public:
    //Public methods
    static void start(void);                                                            //Start ZTP WIFI
    static void regCallback(const netwCallback_t p_callback, const void* p_args);       //Register callback for networking status changes
    static const char* getSsid(void);                                                   //Get connected WIFI SSID
    static uint8_t getChannel(void);                                                    //Get connected WIFI Channel
    static long getRssi(void);                                                          //Get connected WIFI signal SNR
    static const char* getMac(void);                                                    //Get connected WIFI MAC adress
    static IPAddress getIpaddr(void);                                                   //Get host IP address received by DHCP
    static IPAddress getIpmask(void);                                                   //Get network IP mask received by DHCP
    static IPAddress getGateway(void);                                                  //Get gateway router address received by DHCP
    static IPAddress getDns(void);                                                      //Get DNS server address received by DHCP
    static IPAddress getNtp(void);                                                      //Get NTP server address received by DHCP
    static const char* getHostname(void);                                               //Get hostname received by DHCP
    static uint8_t getOpState(void);                                                    //Get current Networking Operational state
    static void WiFiEvent(WiFiEvent_t p_event, arduino_event_info_t p_info);

    //Public data structures
    static uint8_t opState;                                                             //Holds current Operational state

private:
    //Private methods
    static void wpsStart(void);
    static void wpsInitConfig(void);
    static String wpspin2string(uint8_t a[]);

    //Private data structures
    static String ssid;                                                                 //Connected WIFI SSID
    static uint8_t channel;                                                             //Connected WIFI channel
    static long rssi;                                                                   //RSSI WIFI signal SNR
    static String mac;                                                                  //WIFI MAC address
    static IPAddress ipaddr;                                                            //Host IP address received by DHCP
    static IPAddress ipmask;                                                            //Network IP mask received by DHCP
    static IPAddress gateway;                                                           //Gateway router received by DHCP
    static IPAddress dns;                                                               //DNS server received by DHCP
    static IPAddress ntp;                                                               //NTP server received by DHCP
    static String hostname;                                                             //Hostname received by DHCP
    static netwCallback_t wifiStatusCallback;                                           //Reference to callback for network status change
    static const void* wifiStatusCallbackArgs;                                          //Reference to callback for network status change
    static esp_wps_config_t config;                                                     //Internal WIFI configuration object
};
/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/

#endif //NETWORKING_H
