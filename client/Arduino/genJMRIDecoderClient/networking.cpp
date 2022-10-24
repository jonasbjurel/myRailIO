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



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "networking.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: networking                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
String networking::ssid = "";
uint8_t networking::channel = 255;
long  networking::rssi = 0;
String networking::mac = "";
IPAddress networking::ipaddr = IPAddress(0, 0, 0, 0);
IPAddress networking::ipmask = IPAddress(0, 0, 0, 0);
IPAddress networking::gateway = IPAddress(0, 0, 0, 0);
IPAddress networking::dns = IPAddress(0, 0, 0, 0);
IPAddress networking::ntp = IPAddress(0, 0, 0, 0);
String networking::hostname = WIFI_ESP_HOSTNAME_PREFIX;
esp_wps_config_t networking::config;
uint8_t networking::opState = WIFI_OP_INIT | WIFI_OP_DISCONNECTED | WIFI_OP_NOIP;
netwCallback_t networking::wifiStatusCallback;
const void* networking::wifiStatusCallbackArgs;

void networking::start(void) {
    pinMode(WPS_PIN, INPUT);
    WiFi.onEvent(&WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    mac = WiFi.macAddress();
    WiFi.setHostname((hostname + "/" + mac).c_str()); //define hostname
    Log.notice("networking::start: MAC Address: %s" CR, (char*)&mac[0]);
    uint8_t wpsPBT = 0;
    while (digitalRead(WPS_PIN)) {
        wpsPBT++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (wpsPBT == 0) {
        WiFi.begin();
    }
    else if (wpsPBT < 10) {
        Log.notice("networking::start: WPS button was hold down for less thann 10 seconds - connecting to a new WIFI network, push the WPS button on your WIFI router..." CR);
        opState = opState | WIFI_OP_WPS_UNCONFIG;
        wpsStart();
    }
    else {
        Log.notice("networking::start: WPS button was hold down for more that 10 seconds - forgetting previous learnd WIFI networks and rebooting..." CR);
        Serial.println("WPS button was hold down for more that 10 seconds - forgetting previous learnd WIFI networks and rebooting...");
        WiFi.disconnect(true);  //Erase SSID and pwd
        WiFi.disconnect();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        ESP.restart();
    }
}

void networking::regCallback(const netwCallback_t p_callback, const void* p_args) {
    wifiStatusCallback = p_callback;
    wifiStatusCallbackArgs = p_args;
}

void networking::wpsStart() {
    Log.notice("networking::wpsStart: Starting WPS" CR);
    wpsInitConfig();
    esp_wifi_wps_enable(&config);
    esp_wifi_wps_start(0);
}

void networking::wpsInitConfig() {
    Log.notice("networking::wpsInitConfig: Initializing WPS config" CR);
    config.wps_type = ESP_WPS_MODE;
    strcpy(config.factory_info.manufacturer, WIFI_ESP_MANUFACTURER);
    strcpy(config.factory_info.model_number, WIFI_ESP_MODEL_NUMBER);
    strcpy(config.factory_info.model_name, WIFI_ESP_MODEL_NAME);
    strcpy(config.factory_info.device_name, WIFI_ESP_DEVICE_NAME);
}

String networking::wpspin2string(uint8_t a[]) {
    char wps_pin[9];
    for (int i = 0; i < 8; i++) {
        wps_pin[i] = a[i];
    }
    wps_pin[8] = '\0';
    return (String)wps_pin;
}

void WiFiEventHelper(WiFiEvent_t p_event, arduino_event_info_t p_info) {
    networking::WiFiEvent(p_event, p_info);
}

void networking::WiFiEvent(WiFiEvent_t p_event, arduino_event_info_t p_info) {
    switch (p_event) {
    case SYSTEM_EVENT_STA_START:
        Log.notice("networking::WiFiEvent: Station Mode Started" CR);
        opState = opState & ~WIFI_OP_INIT;
        break;
    case SYSTEM_EVENT_STA_STOP:
        Log.notice("networking::WiFiEvent: Station Mode Stoped" CR);
        opState = opState | WIFI_OP_INIT;
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        opState = opState & ~WIFI_OP_DISCONNECTED;
        ssid = WiFi.SSID();
        channel = WiFi.channel();
        rssi = WiFi.RSSI();
        Log.notice("networking::WiFiEvent: Connected to: %s, channel: %d, RSSSI: %d" CR, (char*)&ssid[0], channel, rssi);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        opState = opState | WIFI_OP_DISCONNECTED;
        Log.notice("networking::WiFiEvent: Disconnected from station, attempting reconnection" CR);
        ssid = "";
        channel = 255;
        rssi = 0;
        ipaddr = IPAddress(0, 0, 0, 0);
        gateway = IPAddress(0, 0, 0, 0);
        dns = IPAddress(0, 0, 0, 0);
        ntp = IPAddress(0, 0, 0, 0);
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        opState = opState & ~WIFI_OP_NOIP;
        ipaddr = WiFi.localIP();
        ipmask = WiFi.subnetMask();
        gateway = WiFi.gatewayIP();
        dns = WiFi.dnsIP();
        ntp = WiFi.dnsIP(); //Need to look at DHCP options for NTP and broker names
        Log.notice("networking::WiFiEvent: Got IP-address: %i.%i.%i.%i, Mask: %i.%i.%i.%i, \nGateway: %i.%i.%i.%i, DNS: %i.%i.%i.%i, \nNTP: %i.%i.%i.%i, Hostname: %s" CR,
            ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], ipmask[0], ipmask[1], ipmask[2], ipmask[3], gateway[0], gateway[1], gateway[2], gateway[3],
            dns[0], dns[1], dns[2], dns[3], ntp[0], ntp[1], ntp[2], ntp[3], hostname);
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
        opState = opState | WIFI_OP_NOIP;
        Log.error("networking::WiFiEvent: Lost IP - reconnecting" CR);
        ssid = "";
        channel = 255;
        rssi = 0;
        ipaddr = IPAddress(0, 0, 0, 0);
        gateway = IPAddress(0, 0, 0, 0);
        dns = IPAddress(0, 0, 0, 0);
        ntp = IPAddress(0, 0, 0, 0);
        hostname = "";
        WiFi.reconnect();
        break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
        opState = opState & ~WIFI_OP_WPS_UNCONFIG;
        Log.notice("networking::WiFiEvent: WPS Successful, stopping WPS and connecting to: %s" CR, String(WiFi.SSID()));
        esp_wifi_wps_disable();
        delay(10);
        WiFi.begin();
        break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
        Log.error("networking::WiFiEvent: WPS Failed, retrying" CR);
        esp_wifi_wps_disable();
        esp_wifi_wps_enable(&config);
        esp_wifi_wps_start(0);
        break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
        Log.error("networking::WiFiEvent: WPS Timeout, retrying" CR);
        esp_wifi_wps_disable();
        esp_wifi_wps_enable(&config);
        esp_wifi_wps_start(0);
        break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
        Serial.println("WPS_PIN = " + networking::wpspin2string(p_info.wps_er_pin.pin_code));
        break;
    default:
        break;
    }
    if (wifiStatusCallback != NULL)
        wifiStatusCallback(p_event, p_info, wifiStatusCallbackArgs);
}

const char* networking::getSsid(void) {
    return ssid.c_str();
}

uint8_t networking::getChannel(void) {
    return channel;
}

long networking::getRssi(void) {
    return WiFi.RSSI();
}

const char* networking::getMac(void) {
    return mac.c_str();
}

IPAddress networking::getIpaddr(void) {
    return ipaddr;
}

IPAddress networking::getIpmask(void) {
    return ipmask;
}

IPAddress networking::getGateway(void) {
    return gateway;
}

IPAddress networking::getDns(void) {
    return dns;
}

IPAddress networking::getNtp(void) {
    return ntp;
}

const char* networking::getHostname(void) {
    return hostname.c_str();
}

uint8_t networking::getOpState(void) {
    return opState;
}

/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/
