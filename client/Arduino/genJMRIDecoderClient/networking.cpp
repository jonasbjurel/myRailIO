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
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "networking.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: networking                                                                                                                            */
/* Purpose: See networking.h                                                                                                                    */
/* Description See networking.h                                                                                                                 */
/* Methods: See networking.h                                                                                                                    */
/* Data structures: See networking.h                                                                                                            */
/*==============================================================================================================================================*/
EXT_RAM_ATTR systemState* networking::sysState;
EXT_RAM_ATTR esp_timer_handle_t networking::WiFiWdTimerHandle;
EXT_RAM_ATTR esp_timer_create_args_t networking::WiFiWdTimerArgs;
EXT_RAM_ATTR bool networking::filtering = false;
EXT_RAM_ATTR WiFiManager networking::wifiManager;
EXT_RAM_ATTR netwStaConfig_t networking::networkConfig;
EXT_RAM_ATTR netwStaConfig_t networking::networkConfigBackup;
EXT_RAM_ATTR QList<wifiEventCallbackInstance_t*> networking::wifiEventCallbackList;
EXT_RAM_ATTR QList<wifiProvisionCallbackInstance_t*> networking::wifiProvisionCallbackList;
EXT_RAM_ATTR WiFiManagerParameter* networking::hostNameConfigParam;
EXT_RAM_ATTR WiFiManagerParameter* networking::mqttServerUriConfigParam;
EXT_RAM_ATTR WiFiManagerParameter* networking::mqttServerPortConfigParam;
EXT_RAM_ATTR wifiProvisioningAction_t networking::provisionAction = NO_PROVISION;

void networking::provisioningConfigTrigger(void) {
    LOG_INFO("Provisioning trigger capture started\n");
    pinMode(WIFI_CONFIG_PB_PIN, INPUT_PULLUP);
    uint8_t configPBSecs = 0;
    while (!digitalRead(WIFI_CONFIG_PB_PIN)) {
        configPBSecs++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (configPBSecs > 30) {
        provisionAction = provisionAction | PROVISIONING_DEFAULT_FROM_FACTORY_RESET;
        LOG_INFO("Factory reset and provisioning ordered\n");
    }
    else if (configPBSecs > 5) {
        provisionAction = provisionAction | PROVISION_DEFAULT_FROM_FILE;
        LOG_INFO("Provisioning with default settings from configuration file ordered\n");
    }
}

void networking::start(void) {
    LOG_INFO("Starting networking service" CR);
    sysState = new (heap_caps_malloc(sizeof(systemState), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) systemState(NULL);
    sysState->setSysStateObjName("networking");
    sysState->setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_NOIP | OP_UNCONFIGURED);
    WiFiWdTimerArgs.arg = NULL;
    WiFiWdTimerArgs.callback = WiFiWdTimerArgs.callback = static_cast<esp_timer_cb_t>
        (&networking::wifiWdTimeout);
    WiFiWdTimerArgs.dispatch_method = ESP_TIMER_TASK;
    WiFiWdTimerArgs.name = "WiFi WD timer";
    if (esp_timer_create(&WiFiWdTimerArgs, &WiFiWdTimerHandle))
        panic("Could not initialize WiFi watch-dog timer" CR);
    if (!getAps(NULL))
        panic("No WiFi networks found - cannot continue..." CR);
    sysState->regSysStateCb(NULL, wifiWd);
    WiFi.useStaticBuffers(true);
    WiFi.onEvent(&WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    if (!(provisionAction & PROVISIONING_DEFAULT_FROM_FACTORY_RESET)) {
        if (recoverPersistantConfig(&networkConfig) || setNetworkConfig(&networkConfig))
            provisionAction = provisionAction | PROVISIONING_DEFAULT_FROM_FACTORY_RESET;
    }
    //if (networkConfig.previousNetworkFail >= WIFI_WD_ESCALATION_CNT_THRES) NEEDS TO BE FIXED
    //    provisionAction = provisionAction | PROVISION_DEFAULT_FROM_FILE;
    if(provisionAction){
        WiFiManager wifiManager;
        wifiManager.setAPStaticIPConfig(IPAddress(WIFI_MGR_AP_IP),
            IPAddress(WIFI_MGR_AP_GW_IP),
            IPAddress(255, 255, 255, 0));
        wifiManager.setAPCallback(configModeCb);
        wifiManager.setPreSaveConfigCallback(preSaveConfigCb);
        wifiManager.setSaveConfigCallback(saveConfigCb);
        wifiManager.setTitle(WIFI_MGR_HTML_TITLE);
        wifiManager.setConnectTimeout(WIFI_MGR_STA_CONNECT_TIMEOUT_S);
        wifiManager.setConfigPortalTimeout(WIFI_MGR_AP_CONFIG_TIMEOUT_S);
        wifiManager.setShowStaticFields(true);
        wifiManager.setShowDnsFields(true);
        wifiManager.setShowInfoErase(true);
        wifiManager.setShowInfoUpdate(true);
        wifiManager.setBreakAfterConfig(true);
        wifiManager.setShowPassword(false);
        wifiManager.setCaptivePortalEnable(true);
        wifiManager.setConfigResetCallback(resetCb);
        wifiManager.setConfigPortalTimeoutCallback(configurePortalConnectTimeoutCb);

        if (provisionAction & PROVISIONING_DEFAULT_FROM_FACTORY_RESET) {
            LOG_INFO("Config button was hold down for more "
                       "than 30 seconds or file system was not available - "
                       "formatting filesystem, resetting to factory default settings "
                       "and starting provisioning AP, "
                       "connect to SSID: %s_%s and navigate to http://%s to "
                       "configure the device..." CR, WIFI_MGR_AP_NAME_PREFIX, getMac(),
                       IPAddress(WIFI_MGR_AP_IP).toString().c_str());
            fileSys::format();
            wifiManager.resetSettings();
            factoryResetSettings(&networkConfig);
            setNetworkConfig(&networkConfig);
            getNetworkConfig(&networkConfig);
        }
        else if (provisionAction | PROVISION_DEFAULT_FROM_FILE) {
            LOG_INFO("Config button was hold down for more than "
                        "5 seconds or restart escalation - starting provisioning AP, "
                        "connect to SSID: %s_%s and navigate to http://%s to configure "
                        "the device" CR, WIFI_MGR_AP_NAME_PREFIX, getMac(),
                        IPAddress(WIFI_MGR_AP_IP).toString().c_str());
            getNetworkConfig(&networkConfig);
        }
        hostNameConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("HostName", "HostName",
                                                       networkConfig.hostName, 31);
        wifiManager.addParameter(hostNameConfigParam);
        mqttServerUriConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("MQTTserverURI", "MQTTserverURI",
                                                            networkConfig.mqttUri, 100);
        wifiManager.addParameter(mqttServerUriConfigParam);
        char tmpMqttPort[6];
        mqttServerPortConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("MQTTserverPort", "MQTTserverPort",
                                                             itoa(networkConfig.mqttPort,
                                                             tmpMqttPort, 10), 5);
        wifiManager.addParameter(mqttServerPortConfigParam);
        char apSsid[50];
        sprintf(apSsid, "%s_%s", WIFI_MGR_AP_NAME_PREFIX, getMac());
        wifiManager.startConfigPortal(apSsid);
        wifiManager.stopConfigPortal();
        LOG_INFO("A networking config has been entered - starting the networking service, connecting to SSID: %s" CR,
            networkConfig.wifiConfig.ssid);
    }
    else {
        LOG_INFO("A valid networking configuration was found "
            "- starting the networking service, connecting to SSID: %s" CR,
            networkConfig.wifiConfig.ssid);
        setNetworkConfig(&networkConfig);
        getNetworkConfig(&networkConfig);
        sysState->unSetOpStateByBitmap(OP_UNCONFIGURED);
        WiFi.begin();
    }
}

void networking::regWifiEventCallback(const wifiEventCallback_t p_callback,
                                      const void* p_args) {
    wifiEventCallbackInstance_t* wifiEventCallbackInstance = 
                                 new (heap_caps_malloc(sizeof(wifiEventCallbackInstance_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wifiEventCallbackInstance_t;
    wifiEventCallbackInstance->cb = p_callback;
    wifiEventCallbackInstance->args = (void*)p_args;
    wifiEventCallbackList.push_back(wifiEventCallbackInstance);
}

void networking::unRegWifiEventCallback(const wifiEventCallback_t p_callback) {
    for (uint8_t i = 0; i < wifiEventCallbackList.size(); i++) {
        if (wifiEventCallbackList.at(i)->cb == p_callback) {
            delete wifiEventCallbackList.at(i);
            wifiEventCallbackList.clear(i);
        }
    }
}

void networking::regWifiOpStateCallback(sysStateCb_t p_callback,
                                        void* p_args) {
    sysState->regSysStateCb(p_args, p_callback);
}

void networking::unRegWifiOpStateCallback(sysStateCb_t p_callback) {
    sysState->unRegSysStateCb(p_callback);
}

void networking::regWifiProvisionCallback(const wifiProvisionCallback_t p_callback,
                                          const void* p_args) {
    wifiProvisionCallbackInstance_t* wifiProvisionCallbackInstance =
                                     new (heap_caps_malloc(sizeof(wifiProvisionCallbackInstance_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wifiProvisionCallbackInstance_t;
    wifiProvisionCallbackInstance->cb = p_callback;
    wifiProvisionCallbackInstance->args = (void*)p_args;
    wifiProvisionCallbackList.push_back(wifiProvisionCallbackInstance);
}

void networking::unRegWifiProvisionCallback(const wifiProvisionCallback_t p_callback) {
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        if (wifiProvisionCallbackList.at(i)->cb == p_callback) {
            delete wifiProvisionCallbackList.at(i);
            wifiProvisionCallbackList.clear(i);
        }
    }
}

uint16_t networking::getAps(QList<apStruct_t*>* p_apList, bool p_printOut) {
    uint16_t networks = 0;
    networks = WiFi.scanNetworks();
    if (networks)
        LOG_INFO("%i WiFi networks were found" CR, networks);
    else {
        LOG_INFO("No WiFi networks were found:" CR, networks);
        return 0;
    }
    if (p_printOut) {
        LOG_INFO("Following %i networks were found:" CR, networks);
        LOG_INFO("| %*s | %*s | %*s | %*s | %*s |",
                  -30, "SSID:", -20, "BSSID:",
                  -10, "RSSI:", -10, "Channel:", -20, "Encryption:");
    }
    for (int i = 0; i < networks; i++) {
        apStruct_t* apInfo = new (heap_caps_malloc(sizeof(apStruct_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) apStruct_t;
        strcpy(apInfo->ssid, WiFi.SSID(i).c_str());
        apInfo->rssi = WiFi.RSSI(i);
        apInfo->channel = WiFi.channel(i);
        sprintf(apInfo->bssid, "%s", WiFi.BSSIDstr(i).c_str());
        strcpy(apInfo->encryption, getEncryptionStr(WiFi.encryptionType(i)));
        if (p_printOut){
            LOG_INFO("| %*s | %*s | %*i | %*i | %*s |",
                      -30, apInfo->ssid, -20, apInfo->bssid,
                      -10, apInfo->rssi, -10, apInfo->channel, -20, apInfo->encryption);
        }
        if (p_apList)
            p_apList->push_back(apInfo);
        else
            delete apInfo;
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    WiFi.scanDelete();
    return networks;
}

void networking::unGetAps(QList<apStruct_t*>* p_apList) {
    while (p_apList->size()) {
        delete p_apList->back();
        p_apList->pop_back();
    }
}

char* networking::getSsid(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.ssid;
}

char* networking::getBssid(void) {
    return (char*)WiFi.BSSIDstr().c_str();
}

char* networking::getAuth() {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.encryption;
}

uint8_t networking::getChannel(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.channel;
}

long networking::getRssi(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.rssi;
}

char* networking::getMac(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.mac;
}

rc_t networking::setStaticIpAddr(IPAddress p_address, IPAddress p_mask, IPAddress p_gw, IPAddress p_dns, bool p_persist) {
    netwStaConfig_t nwConfig;
    getNetworkConfig(&networkConfig);
    nwConfig.ipAddr = p_address;
    nwConfig.ipMask = p_mask;
    nwConfig.gatewayIpAddr = p_gw;
    nwConfig.dnsIpAddr = p_dns;
    if (setNetworkConfig(&networkConfig)) {
        WiFi.persistent(true);
        return RC_PARAMETERVALUE_ERR;
    }
    if (p_persist) {
        persistentSaveConfig(&nwConfig);
        WiFi.persistent(true);
    }
    else
        WiFi.persistent(false);
    return RC_OK;
}

IPAddress networking::getIpAddr(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.ipAddr;
}

IPAddress networking::getIpMask(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.ipMask;
}

IPAddress networking::getGatewayIpAddr(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.gatewayIpAddr;
}

IPAddress networking::getDnsIpAddr(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.dnsIpAddr;
}

rc_t networking::setHostname(const char* p_hostname, bool p_persist) {
    if ((strlen(p_hostname) < 3) || strlen(p_hostname) > 31) {
        LOG_ERROR("Hostname: \"%s\" string size out "
                  "of bounds" CR, p_hostname);
        return RC_PARAMETERVALUE_ERR;
    }
    for (uint8_t i = 0; i < strlen(p_hostname); i++) {
        if (i == 0 && !isAlphaNumeric(p_hostname[i])) {
            LOG_ERROR("Hostname: \"%s\" string first "
                "character is not alphanumerical" CR, p_hostname);
            return RC_PARAMETERVALUE_ERR;
        }
        if (!(isAlphaNumeric(p_hostname[i]) || p_hostname[i] == '.' ||
                             p_hostname[i] == '-' || p_hostname[i] == '_' ||
                             p_hostname[i] == ':')) {
            LOG_ERROR("Hostname: \"%s\" string character "
                      "at position %i does not comply to host name conventions" CR,
                      p_hostname, i);
            return RC_PARAMETERVALUE_ERR;
        }
    }
    if (!p_persist)
        WiFi.persistent(false);
    getNetworkConfig(&networkConfig);
    strcpy(networkConfig.hostName, p_hostname);
    if (setNetworkConfig(&networkConfig)) {
        WiFi.persistent(true);
        return RC_PARAMETERVALUE_ERR;
    }
    if (p_persist)
        persistentSaveConfig(&networkConfig);
    WiFi.persistent(true);
    return RC_OK;
}

char* networking::getHostname(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.hostName;
}

rc_t networking::setMqttUri(const char* p_mqttUri, bool p_persist) {
    IPAddress isIpAddress = IPAddress();
    if (!(isUri(p_mqttUri) || isIpAddress.fromString(p_mqttUri))) {
        LOG_ERROR("MQTT URI: %s is not a valid URI" CR,
                  p_mqttUri);
        return RC_PARAMETERVALUE_ERR;
    }
    getNetworkConfig(&networkConfig);
    strcpy(networkConfig.mqttUri, p_mqttUri);
    if (p_persist)
        persistentSaveConfig(&networkConfig);
    return RC_OK;
}

char* networking::getMqttUri(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.mqttUri;
}

rc_t networking::setMqttPort(uint16_t p_mqttPort, bool p_persist) {
    if (p_mqttPort < 0 || p_mqttPort > 65535)
        return RC_PARAMETERVALUE_ERR;
    getNetworkConfig(&networkConfig);
    networkConfig.mqttPort = p_mqttPort;
    if (setNetworkConfig(&networkConfig))
            return RC_PARAMETERVALUE_ERR;
    if (p_persist)
        persistentSaveConfig(&networkConfig);
    return RC_OK;
}

uint16_t networking::getMqttPort(void) {
        getNetworkConfig(&networkConfig);
        return networkConfig.mqttPort;
}

sysState_t networking::getOpStateBitmap(void) {
    return sysState->getOpStateBitmap();
}

char* networking::getOpStateStr(char* p_opStateStr) {
    sysState->getOpStateStr(p_opStateStr);
    return p_opStateStr;
}

bool networking::getNetworkConfig(netwStaConfig_t* p_staConfig) {
    bool validConf = true;
    strcpy(p_staConfig->wifiConfig.ssid, WiFi.SSID().c_str());
    if (WiFi.SSID(true).length() == 0)
        validConf = false;
    sprintf(p_staConfig->wifiConfig.bssid, "%s", WiFi.BSSIDstr().c_str());
    p_staConfig->wifiConfig.channel = WiFi.channel();
    p_staConfig->wifiConfig.rssi = WiFi.RSSI();
    strcpy(p_staConfig->wifiConfig.encryption,
        getEncryptionStr(WiFi.encryptionType(0)));
    strcpy(p_staConfig->mac, WiFi.macAddress().c_str());
    p_staConfig->ipAddr = WiFi.localIP();
    p_staConfig->ipMask = WiFi.subnetMask();
    p_staConfig->gatewayIpAddr = WiFi.gatewayIP();
    p_staConfig->dnsIpAddr = WiFi.dnsIP();
    strcpy(p_staConfig->hostName, (WiFi.getHostname()));
    return validConf;
}

rc_t networking::setNetworkConfig(netwStaConfig_t* p_staConfig) {
    WiFi.config(p_staConfig->ipAddr, p_staConfig->gatewayIpAddr, p_staConfig->ipMask,  p_staConfig->dnsIpAddr);
    WiFi.setHostname(p_staConfig->hostName);
    return RC_OK;
}

rc_t networking::persistentSaveConfig(const netwStaConfig_t* p_staConfig) {
    LOG_INFO("Saving WiFi configuration to "
             "persistant store: %s" CR, WIFI_CONFIG_STORE_FILENAME);
    DynamicJsonDocument wifiConfigJsonDoc(WIFI_CONFIG_JSON_OBJ_SIZE);
    char wifiConfigJsonPrettySerialized[WIFI_CONFIG_JSON_SERIAL_SIZE];
    wifiConfigJsonDoc["wifiConfig"]["ssid"] = p_staConfig->wifiConfig.ssid;
    wifiConfigJsonDoc["wifiConfig"]["bssid"] = p_staConfig->wifiConfig.bssid;
    wifiConfigJsonDoc["wifiConfig"]["channel"] = p_staConfig->wifiConfig.channel;
    wifiConfigJsonDoc["wifiConfig"]["rssi"] = p_staConfig->wifiConfig.rssi;
    wifiConfigJsonDoc["wifiConfig"]["encryption"] = p_staConfig->wifiConfig.encryption;
    wifiConfigJsonDoc["mac"] = p_staConfig->mac;
    wifiConfigJsonDoc["ipAddr"] = p_staConfig->ipAddr;
    wifiConfigJsonDoc["ipMask"] = p_staConfig->ipMask;
    wifiConfigJsonDoc["gatewayIpAddr"] = p_staConfig->gatewayIpAddr;
    wifiConfigJsonDoc["dnsIpAddr"] = p_staConfig->dnsIpAddr;
    wifiConfigJsonDoc["hostName"] = p_staConfig->hostName;
    wifiConfigJsonDoc["mqttUri"] = p_staConfig->mqttUri;
    wifiConfigJsonDoc["mqttPort"] = p_staConfig->mqttPort;
    wifiConfigJsonDoc["previousNetworkFail"] = p_staConfig->previousNetworkFail;
    serializeJsonPretty(wifiConfigJsonDoc, wifiConfigJsonPrettySerialized);
    LOG_VERBOSE("Wifi pretty Json content "
                "to be stored: %s" CR, wifiConfigJsonPrettySerialized);
    uint storeSize;
    if (fileSys::putFile(WIFI_CONFIG_STORE_FILENAME,
                         wifiConfigJsonPrettySerialized,
                         WIFI_CONFIG_JSON_SERIAL_SIZE, &storeSize)) {
        LOG_ERROR("Could not save configuration "
                  "to file %s" CR, WIFI_CONFIG_STORE_FILENAME);
        return RC_GEN_ERR;
    }
    LOG_VERBOSE("Configuration saved to file %s" CR,
                WIFI_CONFIG_STORE_FILENAME);
    return RC_OK;
}

rc_t networking::recoverPersistantConfig(netwStaConfig_t* p_staConfig) {
    LOG_INFO("Recovering WiFi configuration from "
             "persistant store: %s" CR, WIFI_CONFIG_STORE_FILENAME);
    DynamicJsonDocument wifiConfigJsonDoc(WIFI_CONFIG_JSON_OBJ_SIZE);
    char wifiConfigJsonPrettySerialized[WIFI_CONFIG_JSON_SERIAL_SIZE];
    uint readSize;
    rc_t rc;
    if (fileSys::getFile(WIFI_CONFIG_STORE_FILENAME, wifiConfigJsonPrettySerialized,
                         WIFI_CONFIG_JSON_SERIAL_SIZE, &readSize)) {
        LOG_ERROR("Could not read configuration "
                  "from file %s" CR, WIFI_CONFIG_STORE_FILENAME);
        return RC_GEN_ERR;
    }
    LOG_VERBOSE("Wifi pretty Json content "
                "recovered: %s" CR, wifiConfigJsonPrettySerialized);
    deserializeJson(wifiConfigJsonDoc, wifiConfigJsonPrettySerialized); //CATCH ERROR
    strcpy(p_staConfig->wifiConfig.ssid, wifiConfigJsonDoc["wifiConfig"]["ssid"]);
    strcpy(p_staConfig->wifiConfig.bssid, wifiConfigJsonDoc["wifiConfig"]["bssid"]);
    p_staConfig->wifiConfig.channel = wifiConfigJsonDoc["wifiConfig"]["channel"];
    p_staConfig->wifiConfig.rssi = wifiConfigJsonDoc["wifiConfig"]["rssi"];
    strcpy(p_staConfig->wifiConfig.encryption,
           wifiConfigJsonDoc["wifiConfig"]["encryption"]);
    strcpy(p_staConfig->mac, wifiConfigJsonDoc["mac"]);
    p_staConfig->ipAddr.fromString(wifiConfigJsonDoc["ipAddr"].as<const char*>());
    p_staConfig->ipMask.fromString(wifiConfigJsonDoc["ipMask"].as<const char*>());
    p_staConfig->gatewayIpAddr.fromString(wifiConfigJsonDoc["gatewayIpAddr"].
                                          as<const char*>());
    p_staConfig->dnsIpAddr.fromString(wifiConfigJsonDoc["dnsIpAddr"].
                                      as<const char*>());
    strcpy(p_staConfig->hostName, wifiConfigJsonDoc["hostName"]);
    strcpy(p_staConfig->mqttUri, wifiConfigJsonDoc["mqttUri"]);
    p_staConfig->mqttPort = wifiConfigJsonDoc["mqttPort"];
    p_staConfig->previousNetworkFail = wifiConfigJsonDoc["previousNetworkFail"];
    return RC_OK;
}

void networking::factoryResetSettings(netwStaConfig_t* p_staConfig) {
    strcpy(p_staConfig->wifiConfig.ssid, "");
    sprintf(p_staConfig->hostName, "%s_%s", WIFI_ESP_HOSTNAME_PREFIX,
            WiFi.macAddress().c_str());
    strcpy(p_staConfig->mqttUri, MQTT_DEFAULT_URI);
    p_staConfig->mqttPort = MQTT_DEFAULT_PORT;
    p_staConfig->ipAddr = IPAddress(0, 0, 0, 0);
    p_staConfig->ipMask = IPAddress(0, 0, 0, 0);
    p_staConfig->gatewayIpAddr = IPAddress(0, 0, 0, 0);
    p_staConfig->dnsIpAddr = IPAddress(0, 0, 0, 0);
    p_staConfig->previousNetworkFail = 0;
}

char* networking::getEncryptionStr(wifi_auth_mode_t p_wifi_auth_mode) {
    switch (p_wifi_auth_mode) {
    case WIFI_AUTH_OPEN:
        return (char*)"OPEN";
        break;
    case WIFI_AUTH_WEP:
        return (char*)"WEP";
        break;
    case WIFI_AUTH_WPA_PSK:
        return (char*)"WPA_PSK";
        break;
    case WIFI_AUTH_WPA2_PSK:
        return (char*)"WPA2_PSK";
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        return (char*)"WPA_WPA2_PSK";
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        return (char*)"WPA2_ENTERPRISE";
        break;
    case WIFI_AUTH_MAX:
        return (char*)"MAX";
        break;
    default:
        return (char*)"UNKNOWN";
    }
}

void networking::WiFiEvent(WiFiEvent_t p_event, arduino_event_info_t p_info) { //This should be serialized into a job-queue
    switch (p_event) {
    case ARDUINO_EVENT_WIFI_READY:
        LOG_INFO("WiFi service ready" CR);
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        LOG_INFO("WiFi APs have been scanned" CR);
        break;
    case SYSTEM_EVENT_STA_START:
        LOG_INFO("Station Mode Started" CR);
        sysState->unSetOpStateByBitmap(OP_INIT);
        break;
    case SYSTEM_EVENT_STA_STOP:
        LOG_INFO("Station Mode Stoped" CR);
        sysState->setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_NOIP);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        sysState->unSetOpStateByBitmap(OP_INIT | OP_DISCONNECTED);                                  //Sometimes the start event is missing for unknown reasons
        LOG_INFO("Station connected to AP-SSID: %s, "
                   "channel: %d, RSSI: %d" CR, getSsid(), getChannel(), getRssi());
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        sysState->setOpStateByBitmap(OP_DISCONNECTED | OP_NOIP);
        LOG_INFO("Station disconnected from AP-SSID: %s, "
                   "attempting reconnection" CR, getSsid());
        WiFi.reconnect();
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        LOG_INFO("Authentication mode changed to: %s"
                   CR, getAuth());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        sysState->unSetOpStateByBitmap(OP_NOIP);
        LOG_INFO("Station got IP-address:%s, Mask: %s, \n"
                    "Gateway: %s, DNS: %s, Hostname: %s" CR,
                    getIpAddr().toString().c_str(), getIpMask().toString().c_str(),
                    getGatewayIpAddr().toString().c_str(),
                    getDnsIpAddr().toString().c_str(),
                    getHostname());
        break;
    case SYSTEM_EVENT_STA_LOST_IP:
        sysState->setOpStateByBitmap(OP_NOIP);
        LOG_ERROR("Station lost IP - reconnecting" CR);
        WiFi.reconnect();
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        LOG_INFO("Station WPS success" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        LOG_INFO("Station WPS failed" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        LOG_INFO("Station WPS timeout" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        LOG_INFO("Station WPS PIN failure" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        LOG_INFO("AP started with SSID %s, "
                   "IP address: %s" CR, WiFi.softAPSSID().c_str(),
                   WiFi.softAPIP().toString().c_str());
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        LOG_INFO("WiFiEvent: AP stoped" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        LOG_INFO("WiFiEvent: A Client connected to the AP" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        LOG_INFO("A Client disconnected from the AP" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        LOG_INFO("The AP asigned an IP address to "
                   "a client" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
        LOG_INFO("The AP received a probe request" CR);
        break;
    default:
        LOG_ERROR("Received a non recognized WIFI event: %i"
                  CR, p_event);
        break;
    }
    if (!wifiEventCallbackList.size())
        LOG_VERBOSE("No callback functions to call" CR);
    else {
        for (uint8_t i = 0; i < wifiEventCallbackList.size(); i++) {
            LOG_VERBOSE("Calling callback function: 0x%x"
                CR, wifiEventCallbackList.at(i)->cb);
            wifiEventCallbackList.at(i)->cb(p_event, p_info,
                wifiEventCallbackList.at(i)->args);
        }
    }
}

void networking::configModeCb(WiFiManager* p_WiFiManager) {
    LOG_INFO("genJMRIdecoder provisioning manager "
               "started - SSID: %s, IP address %s" CR,
               wifiManager.getConfigPortalSSID().c_str(), WiFi.softAPIP().toString().c_str());
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        LOG_VERBOSE("Calling callback function: 0x%x"
                    CR, wifiProvisionCallbackList.at(i)->cb);
        wifiProvisionCallbackList.at(i)->cb(WIFI_PROVISIONING_START,
                                            wifiProvisionCallbackList.at(i)->args);
    }
}

void networking::preSaveConfigCb() {
    LOG_INFO("genJMRIdecoder provisioning manager "
               "parameters are about to be saved, creating a backup of the configuration" CR);
    memcpy(&networkConfigBackup, &networkConfig, sizeof(networkConfig));
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        LOG_VERBOSE("Calling callback function: %p"
                    CR, wifiProvisionCallbackList.at(i)->cb);
        wifiProvisionCallbackList.at(i)->cb(WIFI_PROVISIONING_PRESAVE,
                                            wifiProvisionCallbackList.at(i)->args);
    }
}

void networking::saveConfigCb() {
    bool validConfig = true;
    getNetworkConfig(&networkConfig);

    if (setHostname(hostNameConfigParam->getValue())) {
        validConfig = false;
        LOG_ERROR("Host name validation error" CR);
    }
    if (setMqttUri(mqttServerUriConfigParam->getValue())) {
        validConfig = false;
        LOG_ERROR("MQTT server URI validation error" CR);
    }
    if (setMqttPort(atoi(mqttServerPortConfigParam->getValue()))) {
        validConfig = false;
        LOG_ERROR("MQTT server port validation error" CR);
    }
    if (!validConfig) {
        LOG_ERROR("New configuration have errors and will "
                  "not be applied, reverting to previous configuration or factory "
                  "default" CR);
        memcpy(&networkConfig, &networkConfigBackup, sizeof(networkConfigBackup));
        setNetworkConfig(&networkConfig);
    }
    if (persistentSaveConfig(&networkConfig))
        LOG_ERROR("Could not peristantly store "
                    "configuration" CR);
    else
        LOG_INFO("Configuration peristantly stored" CR);
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        LOG_VERBOSE("Calling callback function: 0x%x" CR,
                    wifiProvisionCallbackList.at(i)->cb);
        wifiProvisionCallbackList.at(i)->cb(WIFI_PROVISIONING_SAVE,
                                            wifiProvisionCallbackList.at(i)->args);
    }
    sysState->unSetOpStateByBitmap(OP_UNCONFIGURED);
}

void networking::resetCb(void) {
    factoryResetSettings(&networkConfig);
    setNetworkConfig(&networkConfig);
    getNetworkConfig(&networkConfig);
    if (persistentSaveConfig(&networkConfig)) {
        LOG_ERROR("Could not peristantly store configuration" CR);
        panic("Networking configuration has been reset to factory default - rebooting...");
    }
}

void networking::configurePortalConnectTimeoutCb(void) {
    if (provisionAction & PROVISIONING_DEFAULT_FROM_FACTORY_RESET)
        panic("No client connected to the WiFi provisioning manager despite factory reset request - rebooting...");
    else
        LOG_ERROR("No client connected to the WiFi provisioning manager, continuing with current configuration...");
}


void networking::wifiWd(const void* p_args, sysState_t p_wifiOpState) {
    esp_err_t errCode;
    if (sysState->getOpStateBitmap()) {
        if (!filtering) {
            filtering = true;
            if (errCode = esp_timer_start_once(WiFiWdTimerHandle, WIFI_WD_TIMEOUT_S * 1000000))
                panic("Could not start WiFi WD timer, reason: %s - "
                      "rebooting..." CR, esp_err_to_name(errCode));
            LOG_VERBOSE("Watchdog timer started" CR);
        }
    }
    else if (filtering){
        filtering = false;
        if (errCode = esp_timer_stop(WiFiWdTimerHandle))
            panic("Could not stop WiFi WD timer, reason: %s - "
                "rebooting..." CR, esp_err_to_name(errCode));
        networkConfig.previousNetworkFail = 0;
        //persistentSaveConfig(&networkConfig); NEEDS TO BE FIXED
        LOG_VERBOSE("Watchdog timer stoped" CR);
    }
}

void networking::wifiWdTimeout(void* p_dummy) {
    //networkConfig.previousNetworkFail++; NEEDS TO BE FIXED
    persistentSaveConfig(&networkConfig);
    panic("WiFi watchdog timed out, rebooting..." CR);
}
/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/
