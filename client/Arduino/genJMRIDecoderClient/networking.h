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
#ifndef NETWORKING_H
#define NETWORKING_H
#define ESP32
/*==============================================================================================================================================*/
/* END .h Definitions                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <cctype>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_pm.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <WiFiManager.h>
#include <QList.h>
#include <string.h>
#include "systemState.h"
#include "strHelpers.h"
#include "config.h"
#include "pinout.h"
#include "panic.h"
#include "rc.h"
#include "fileSys.h"
#include "telnetCore.h"
#include "logHelpers.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: networking                                                                                                                            */
/* Purpose: The networking class is responsible for the WiFi network connectivity and the provisioning of networking parameters such as:        */
/*          WiFi SSID, WiFi password, IP addresses, MQTT brocker and MQTT brocker port.                                                         */
/* Description: Calling networking::init() will start the WiFi networking service, if pushbutton <WIFI_CONFIG_PB_PIN> is pushed for more than   */
/*              5 seconds after networking::provisioningConfigTrigger() has been called a provisioning Access point is started with SSID:       */
/*              <WIFI_MGR_AP_NAME_PREFIX>_<MAC-address> and if supported, an captive provisioning WEB portal is automatically started when a    */
/*              client connects to the Access point, if captive portals are not supported by the client, navigate to http://<WIFI_MGR_AP_IP>.   */
/*              In the captive portal WiFi- and MQTT- parameters can be configured after which the genJMRIdecoder will connect to the given     */
/*              Access-point. If the pushbutton <WIFI_CONFIG_PB_PIN> is pushed for more than 30 seconds after                                   */
/*              networking::provisioningConfigTrigger() has been called, the genJMRIdecoder will be reset to factory default, the filesystem    */
/*              for the configuration file(s) will be re-formatted, and the provisioning Access point is started.                               */
/*              If "Erase WiFi config" is activated inside the captive provisioning portal the genJMRIdecoder will be reset to factory default, */
/*              the filesystem for the configuration file(s) will be re-formatteand and the decoder will be restarted.                          */
/*              If a persisted configuration file cannot be found the filesystem will be re-formatted, and the provisioning Access point is     */
/*              started. When ever "Save" is pressed in the captive provisioning portal, the configuration will be persisted in the confuration */
/*              filesystem/file(s).                                                                                                             */
/*              There is a time-out for connection to the provisioning Access point, if a client has not connected to the Accesspoint within    */
/*              <WIFI_MGR_AP_CONFIG_TIMEOUT_S> seconds, the provisioning Access point is terminated and the WiFi connection process proceeds    */
/*              with the WiFi connection process with the persisted-, or manufacturing default parameters.                                      */
/*              If WiFi connectivity cannot be achieved within <WIFI_WD_TIMEOUT_S> seconds from when networking::init() was called, the decoder */
/*              is rebooted, and an escallation counter is incremented, when the escalation counter has reached <WIFI_WD_ESCALATION_CNT_THRES>  */
/*              the provisioning Access point will be started.                                                                                  */
/*              Over the air software updates: TBD                                                                                              */
/*              If pushbutton <WIFI_CONFIG_PB_PIN> is NOT pushed and there exists persisted configuration on the file system,                   */
/*              the genJMRIdecoder will not start the provisioning Access point, but will try to connect to the Access point defined by the     */
/*              persisted configuration                                                                                                         */
/*              All modifyable configuration parameters are defined in config.h                                                                 */
/* Methods: See below                                                                                                                           */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
/* Definitions                                                                                                                                  */
typedef struct apStruct_t {                                                             // WIFI parameters struct
    char ssid[50];                                                                      // WIFI AP SSID
    char bssid[18];                                                                     // WIFI AP BSSID
    int32_t channel;                                                                    // WIFI AP Channel
    int8_t rssi;                                                                        // WIFI AP RSSI (E.g. S/N ratio [dBm] as measured from the
                                                                                        //   station/client)
    char encryption[20];                                                                // WIFI Encryption method
    char pass[30];
};

typedef struct netwStaConfig_t {                                                        // Station network configuration struct
    apStruct_t wifiConfig;                                                              // Wifi configuration
    char mac[18];                                                                       // Station WIFI MAC address
    bool staticIp;                                                                      // Static IP address
    IPAddress ipAddr;                                                                   // Station Static host WIFI IP address
    IPAddress ipMask;                                                                   // Station Static WIFI network mask
    IPAddress gatewayIpAddr;                                                            // Station Static Gateway IP address
    IPAddress dnsIpAddr;                                                                // Station Static DNS IP address
    char hostName[100];                                                                 // Station host name
    char mqttUri[100];                                                                  // MQTT broker URI or IP-address
    uint16_t mqttPort;                                                                  // MQTT broker port
    uint8_t previousNetworkFail;                                                        // Number of brevious reboot that was due to failure to
                                                                                        //   successfully establish network connectivity
};

typedef uint8_t wifiProvisioningAction_t;                                               // WiFi provisioning action based on the push-button
                                                                                        //   <WIFI_CONFIG_PB_PIN> and the status of the persisted
                                                                                        //   config file(s)
#define NO_PROVISION                                    0b00000000                      // No provisioning - directly proceed to WiFi connection
                                                                                        //   based on the persisted configuration
#define PROVISION_DEFAULT_FROM_FILE                     0b00000001                      // Start provisioning Access point and captive portal with
                                                                                        //   default settings from persisted configuration file(s)
#define PROVISIONING_DEFAULT_FROM_FACTORY_RESET         0b00000010                      // Start provisioning Access point and captive portal with
                                                                                        //   factory default settings

typedef uint8_t networkProvisioningValidationResult_t;                                  // WiFi manager Network configuration propriatary parameter
                                                                                        //   validation errors
#define NETWORK_VALIDATION_NO_ERR                       0b00000000                      // No validation error
#define NETWORK_VALIDATION_MQTTURI_ERR                  0b00000001                      // MQTT URI did not pass the validation
#define NETWORK_VALIDATION_MQTTPORT_ERR                 0b00000010                      // MQTT URI Port did not pass the validation
#define NETWORK_VALIDATION_HOSTNAME_ERR                 0b00000100                      // MQTT URI did not pass the validation


typedef uint8_t wifiProvisioningEvent_t;
#define WIFI_PROVISIONING_START                         1
#define WIFI_PROVISIONING_PRESAVE                       2
#define WIFI_PROVISIONING_SAVE                          3

typedef void (*wifiEventCallback_t)(WiFiEvent_t p_event, arduino_event_info_t p_info,   // WiFi event call-back prototype
              const void* p_args);

typedef void (*wifiProvisionCallback_t)(wifiProvisioningEvent_t p_wifiProvisioningEvent,// WiFi Provisioning event call-back prototype
              const void* p_args);

typedef struct wifiEventCallbackInstance_t {                                            // WiFi event call-back parameter struct
    wifiEventCallback_t cb;
    void* args;
};

typedef struct wifiProvisionCallbackInstance_t {                                        // WiFi Provisioning call-back parameter struct
    wifiProvisionCallback_t cb;
    void* args;
};

/* Methods                                                                                                                                      */
class networking {
public:
    //Public methods
    static void provisioningConfigTrigger(void);                                        // Start sense of WiFi provisioning button 
    static void start(void);                                                            // Start WiFi networking service
    static rc_t regWifiEventCallback(const wifiEventCallback_t p_callback,
                                     const void* p_args);                               // Register callback for WiFi events
    static rc_t unRegWifiEventCallback(const wifiEventCallback_t p_callback,            // Un-Register callback for WiFi events
                                       const void* p_args);
    static rc_t regWifiOpStateCallback(sysStateCb_t p_callback,
                                       void* p_args);                                   // Register callback for WiFi operational status
    static rc_t unRegWifiOpStateCallback(sysStateCb_t p_callback,                       // Un-Register callback for WiFi operational status
                                         void* p_args);
    static rc_t regWifiProvisionCallback(const wifiProvisionCallback_t p_callback,
                                            const void* p_args);                        // Register callback for WiFi provisioning events
    static rc_t unRegWifiProvisionCallback(const wifiProvisionCallback_t p_callback,    // Un-Register callback for WiFi provisioning
        const void* p_args);
    static uint16_t getAps(QList<apStruct_t*>* p_apList, bool p_printOut=true);         // Scan all APs, print out all AP details, and provide a list of
                                                                                        //   apStruct_t* unless p_apList is NULL
    static void unGetAps(QList<apStruct_t*>* p_apList);                                 // Frees the p_aplist structure created by getAps
    static rc_t setSsidNPass(const char* p_ssid, const char* p_pass,                    // Sets both of Accsess point SSID and password
                             bool p_persist = true);
    static rc_t setSsid(const char* p_ssid, bool p_persist = true);                     // Sets the Accsess point SSID

    static char* getSsid(void);                                                         // Get WIFI SSID
    static char* getBssid(void);                                                        // Get connected AP BSSID
    static char* getAuth();                                                             // Get WIFI Authentication method
    static rc_t setPass(const char* p_pass, bool p_persist = true);
    static char* getPass(void);
    static uint8_t getChannel(void);                                                    // Get provisioned WIFI Channel
    static long getRssi(void);                                                          // Get connected WIFI signal SNR
    static char* getMac(void);                                                          // Get MAC adress
    static rc_t setDHCP(bool p_persist = true);                                         // Sets DHCP address assignment operation, if p_persist 
                                                                                        //   is true the configuration will persit across reboots
    static rc_t setStaticIpAddr(const char* p_address, const char* p_mask,              // Sets static IP adresses operation, if p_persist 
        const char* p_gw, const char* p_dns, bool p_persist = true);                    //   is true the configuration will persit across reboots
    static rc_t setStaticIpAddr(IPAddress p_ipAddr, IPAddress p_ipMask,                 // Sets static IP adresses operation, if p_persist 
        IPAddress p_gatewayIpAddr, IPAddress p_dnsIpAddr, bool p_persist = true);       //   is true the configuration will persit across reboots
    static bool isStatic(void);                                                         // Reports if static IP is set or not
    static IPAddress getIpAddr(void);                                                   // Get host IP address received by DHCP or provisioned 
                                                                                        //   by WiFi manager
    static IPAddress getIpMask(void);                                                   // Get network IP mask received by DHCP or provisioned
                                                                                        //   by WiFi manager
    static IPAddress getGatewayIpAddr(void);                                            // Get gateway router address received by DHCP or provisioned
                                                                                        //   by WiFi manager
    static IPAddress getDnsIpAddr(void);                                                // Get DNS server address received by DHCP or provisioned
                                                                                        //   by WiFi manager
    static rc_t setHostname(const char* p_hostname, bool p_persist = true);             // Set hostname
    static char* getHostname(void);                                                     // Get hostname received - either factory defined-, or defined
                                                                                        //   by WiFi manager- or defined by setHostname()
    static rc_t setMqttUri(IPAddress p_mqttIP, bool p_persist = true);                  // Set Mqtt IP Address or URL
    static rc_t setMqttUri(const char* p_mqttUri, bool p_persist = true);               // Set Mqtt URI, IP Address or URL
    static char* getMqttUri(void);                                                      // Get Mqtt URI, IP Address or URL
    static rc_t setMqttPort(const char* p_mqttPort, bool p_persist = true);             // Set MQTT broker port
    static rc_t setMqttPort(int32_t p_mqttPort, bool p_persist = true);                 // Set MQTT broker port
    static uint16_t getMqttPort(void);                                                  // Get MQTT broker port
    static uint8_t getNwFail(void);                                                     // Get number of consequtive network fails
    static void reportNwFail(void);                                                     // Report a network dependent service start fail
    static void concludeRestart(void);                                                  // Conclude the start of all network dependent services 
    static sysState_t getOpStateBitmap(void);                                           // Get current Networking Operational state
    static char* getOpStateStr(char* p_opStateStr);                                     // Get current Networking Operational state as string
	static rc_t startRuntimePortal(void);                                                // Start runtime portal
	static void wwwPoll(void* p_dummy);                                                 // Polling function for the WiFi manager

    //Public data structures
    //-

private:
    //Private methods
    static void sendWifiEventCallback(WiFiEvent_t p_event, arduino_event_info_t p_info);// Sends WiFi event notification callbacks to
                                                                                        //   those subscribing to it
    static void sendNetworkProvisionCallback(void);                                     // Sends WiFi provisioning notification callbacks to
                                                                                        //   those subscribing to it
    static rc_t getNetworkConfig(netwStaConfig_t* p_staConfig);                         // Retreive network config from persistant store,
                                                                                        //   returns true if all network config was found,
                                                                                        //   otherwise returns false
    static rc_t setNetworkConfig(netwStaConfig_t* p_staConfig,                          // Validates and sets the current network configuration.
                                 bool p_persist = true);
    static void cpyNetworkConfig(netwStaConfig_t* dst, const netwStaConfig_t* src);     // Copies a network configuratrion
    static void setNetworkUiDefault(netwStaConfig_t* p_staConfig);                      // Pre fills the UI with default data
    static void factoryResetSettings(netwStaConfig_t* p_staConfig);                     // Set factory default network settings to *p_staConfig
    static rc_t persistentSaveConfig(const netwStaConfig_t* p_staConfig);               // Persistantly save network settings from *p_staConfig
    static rc_t recoverPersistantConfig(netwStaConfig_t* p_staConfig);                  // Recover persistantly stored network settings to *p_staConfig
    static char* getEncryptionStr(wifi_auth_mode_t p_wifi_auth_mode);                   // Get current encryption mode 
    static void WiFiEvent(WiFiEvent_t p_event, arduino_event_info_t p_info);            // WiFi event call-back
    static void configModeCb(WiFiManager* p_WiFiManager);                               // Provisionig Access point/captive portal activation call-back
    static void preSaveConfigCb();                                                      // Call-back just before saving WiFi parameters
    static void saveConfigCb();                                                         // Call-back just after WiFi parameters are saved
    static void resetCb(void);                                                          // Call-back from when captive portal "Erase WiFi config" is pushed
    static void configurePortalConnectTimeoutCb(void);                                  // Call-back from provisioning portal time-out
    static void wifiWd(const void* p_miscCbData, sysState_t p_sysState);                // Call-back from WiFi operational state change
    static void wifiWdTimeout(void* p_dummy);                                           // WiFi unoperational timeout <WIFI_WD_TIMEOUT_S>

    //Private data structures
    static systemState* sysState;                                                       // Operational/system state object
    static WiFiManager wifiManager;
    static esp_timer_handle_t WiFiWdTimerHandle;
    static esp_timer_create_args_t WiFiWdTimerArgs;
    static wifiProvisioningAction_t provisionAction;                                    // WiFi Provisioning action <wifiProvisioningAction_t>
    static bool filtering;                                                              // WiFi Wd filtering ongoing
    static netwStaConfig_t networkConfig;                                               // Current network station configuration
    static netwStaConfig_t networkConfigBackup;                                         // Backup network station configuration - used while reconfiguration
    static QList<wifiEventCallbackInstance_t*> wifiEventCallbackList;                   // List of registered Wifi event callbacks
    static QList<wifiProvisionCallbackInstance_t*> wifiProvisionCallbackList;           // List of registered Wifi provisioning callbacks
    static DynamicJsonDocument* configDoc;                                              // Configuration Json object
    static WiFiManagerParameter* hostNameConfigParam;                                   // Captive portal hostName configuration object
    static WiFiManagerParameter* mqttServerUriConfigParam;                              // Captive portal MQTT brocker URI configuration object
    static WiFiManagerParameter* mqttServerPortConfigParam;                             // Captive portal MQTT brocker port configuration object
    static bool nwFailCounted;                                                          // Indicates if a network dependent start fail has already
                                                                                        //  been accounted for
    static networkProvisioningValidationResult_t provisionValidationErr;                // Validation error of propriatary WiFi manager network parameters
};
/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/
#endif //NETWORKING_H
