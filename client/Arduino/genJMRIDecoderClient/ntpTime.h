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
#ifndef NTPTIME_H
#define NTPTIME_H
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <Arduino.h>
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <cctype>
#include <stdint.h>
#include <esp_sntp.h>
#include <time.h>
#include <QList.h>
#include "config.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "rc.h"
#include "strHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: ntpTime                                                                                                                               */
/* Purpose: The ntpTime class is overall responsible for the real-time clock.                                                                   */
/* Description: ntpTime provides a realtime clock, providing time of day, it provides means to set and get time of day, time-zone,              */
/*              daylight - saving, as well as synchronizing the time with a set of remote NTP servers                                           */
/* Methods: See below                                                                                                                           */
/* Data structures: See below                                                                                                                   */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
typedef uint8_t ntpOpState_t;
#define NTP_WORKING                             0b00000000                              //NTP Client is synchronized and working
#define NTP_CLIENT_DISABLED                     0b00000001                              //NTP Client is not started, or stopped
#define NTP_CLIENT_NOT_SYNCHRONIZED             0b00000010                              //NTP Client is started but is not synchronized
#define NTP_CLIENT_SYNCHRONIZING                0b00000100                              //NTP Client is about to sychronize

struct ntpServerHost_t {                                                                //NTP server reccord
    uint8_t index;                                                                      //NTP Server index
    IPAddress ntpHostAddress;                                                           //NTP server IP-address
    char ntpHostName[50];                                                               //NTP server host name
    uint16_t ntpPort;                                                                   //NTP server port
    uint8_t reachability;                                                               //NTP server reachability
    uint8_t stratum;                                                                    //NTP server clock stratum - not yet implemented
};

struct ntpDescriptor_t {                                                                //NTP client descriptor
    uint8_t ntpServerIndexMap;                                                          //NTP server bitmap
    ntpOpState_t opState;                                                               //NTP Client Operational state
    bool dhcp;                                                                          //NTP Servers DHCP state
    char timeZone[100];                                                                 //NTP Client time-zone
    uint8_t syncMode;                                                                   //NTP Client synchronization mode - See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
    uint8_t noOfServers;                                                                //Number of defined NTP servers
    uint8_t syncStatus;                                                                 //NTP Client synchronization status: SNTP_SYNC_STATUS_RESET|SNTP_SYNC_STATUS_IN_PROGRESS|SNTP_SYNC_STATUS_COMPLETED - See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
    uint32_t pollPeriodMs;                                                              //NTP server poll period[ms]
    char opStateStr[50];                                                                //NTP Client operatonal state string
    char syncMode_Str[50];                                                              //NTP Client synchronization mode string
    char syncStatusStr[50];                                                             //NTP Client synchronization status string
    QList<ntpServerHost_t*>* ntpServerHosts;                                            //NTP Servers list
};

/* Methods                                                                                                                                      */
class ntpTime {                                                                         //ntpTime static class
    public:
    //Public methods
    static void init(void);                                                             //Initialize the class
    static rc_t start(bool p_dhcp=true);                                                //Start NTP synchronization with defined NTP servers, if DHCP is true,
                                                                                        //   use NTP servers provided by the DHCP server
    static rc_t stop(void);                                                             //Stop NTP synchroniation
    static rc_t restart(void);                                                          //Restart NTP synchronization
    static bool getNtpDhcp(void);                                                       //Get NTP server DHCP option
    static rc_t addNtpServer(IPAddress p_ntpServerAddr, uint16_t p_port=123);           //Add an NTP server with an IP-Address to the list of NTP servers to synchronize with
    static rc_t addNtpServer(const char* p_ntpServerName, uint16_t p_port = 123);       //Add an NTP server with an URL to the list of NTP servers to synchronize with
    static rc_t deleteNtpServer(IPAddress p_ntpServerAddr);                             //Delete an NTP server with an IP-Address from the list of NTP servers to synchronize with
    static rc_t deleteNtpServer(const char* p_ntpServerName);                           //Delete an NTP server with an URL from the list of NTP servers to synchronize with
    static rc_t deleteNtpServer(void);                                                  //Delete an NTP server with an URL from the list of NTP servers to synchronize with
    static rc_t getNtpServer(const IPAddress* p_ntpServerAddr, ntpServerHost_t* p_ntpServerHost); // Get the NTP server reccord for the NTP server with the given IP-Address
    static rc_t getNtpServer(const char* p_ntpServerName, ntpServerHost_t* p_ntpServerHost);//Get the NTP server reccord for the NTP server with the given URL
    static rc_t getNtpServers(QList<ntpServerHost_t*>** p_ntpServerHosts);              //Get the list of all defined NTP server reccords
    static uint8_t getReachability(uint8_t p_idx);                                      //Get reachability for a given NTP server index (RFC 5905)
    static rc_t getNoOfNtpServers(uint8_t* p_ntpNoOfServerHosts);                       //Get current number of defined NTP servers
    static rc_t getNtpOpState(ntpOpState_t* p_ntpOpState);                              //Get current NTP client operational state
    static rc_t getNtpOpStateStr(char* p_ntpOpStateStr);                                //Get current NTP client operational state string
    static rc_t setSyncMode(const uint8_t p_syncMode);                                  //Set current NTP client synchronization mode - See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
    static rc_t setSyncModeStr(const char* p_syncModeStr);                              //Set current NTP client synchronization mode from string: "SYNC_MODE_SMOOTH"|"SYNC_MODE_IMMED"
    static rc_t getSyncMode(uint8_t* p_syncMode);                                       //Get current NTP client synchronization mode - See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
    static rc_t getSyncModeStr(char* p_syncModeStr);                                    //Get current NTP client synchronization mode string:"SYNC_MODE_SMOOTH"|"SYNC_MODE_IMMED"
    static rc_t getNtpSyncStatus(uint8_t* p_ntpSyncStatus);                             //Get current NTP client synchronization status: SYNC_STATUS_RESET|SYNC_STATUS_IN_PROGRESS|SYNC_STATUS_COMPLETED - See https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/system_time.html
    static rc_t getNtpSyncStatusStr(char* p_ntpSyncStatusStr);                          //Get current NTP client synchronization status string: "SYNC_STATUS_RESET"|"SYNC_STATUS_IN_PROGRESS"|"SYNC_STATUS_COMPLETED"
    static rc_t setTimeOfDay(const char* p_timeOfDayStr, char* p_resultStr = NULL);     //Set TimeOfDay from sting - format: YYYY-MM-DDTHH:MM.SS
    static rc_t setTimeOfDay(tm* p_tm, char* p_resultStr = NULL);                       //Set TimeOfDay from tm struct
    static rc_t getTimeOfDay(char* p_timeOfDayStr, bool p_utc = false);                 //Get timeOfDay string UTC or local - format: E.g.Sat Jul  1 2023 - 11:03:39 - UTC time
    static rc_t getTimeOfDay(tm** p_timeOfDayTm, bool p_utc = false);                   //Get timeOfDay tm struct UTC or local
    static rc_t setEpochTime(const char* p_epochTimeStr, char* p_resultStr = NULL);     //Set Epoch-time from string
    static rc_t setEpochTime(const timeval* p_tv, char* p_resultStr = NULL);            //Set Epoch-time from timeval
    static rc_t getEpochTime(char* p_epochTimeStr);                                     //Get Epoch-time string
    static rc_t getEpochTime(timeval* p_tv);                                            //Get Epoch-time timeval
    static rc_t setTz(const char* p_tz, char* p_resultStr = NULL);                      //Set Timezone from string
    static rc_t getTz(char* p_tz);                                                      //Get Timezone to string
    static rc_t getTz(timezone** p_tz);                                                 //Get Timezone to timezone struct
    static rc_t getDayLightSaving(bool* p_daylightSaving);                              //Get current day-light saving status

    //Public data structures
    //-

private:
    //Private methods
    static rc_t setNtpOpState(ntpDescriptor_t* p_ntpDescriptor, ntpOpState_t p_opState); //Set NTP client Operational status
    static rc_t unSetNtpOpState(ntpDescriptor_t* p_ntpDescriptor, ntpOpState_t p_opState); //Un-set NTP client Operational status
    static rc_t getNtpOpState(const ntpDescriptor_t* p_ntpDescriptor, ntpOpState_t* p_opState); //Get NTP client Operational status
    static rc_t updateNtpDescriptorStr(ntpDescriptor_t* p_ntpDescriptor);               //Update NTP client descriptor strings
    static 	rc_t syncModeToInt(const char* p_syncModeStr, uint8_t* p_syncMode);         //Get Syncmode Int from string
    static 	rc_t syncModeToStr(char* p_syncModeStr, uint8_t p_syncMode);                //Get Syncmode String from Int
    static void sntpCb(timeval* p_tv);                                                  //NTP server response call-back


    //Private data structures
    static ntpDescriptor_t ntpDescriptor;                                               //NTP Client descriptor
    };
/*==============================================================================================================================================*/
/* END Class ntpTime                                                                                                                            */
/*==============================================================================================================================================*/
#endif //NTPTIME_H
