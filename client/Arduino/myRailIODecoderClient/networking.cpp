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
EXT_RAM_ATTR bool networking::nwFailCounted = false;
EXT_RAM_ATTR networkProvisioningValidationResult_t networking::provisionValidationErr = NETWORK_VALIDATION_NO_ERR;



void networking::provisioningConfigTrigger(void) {
    LOG_INFO_NOFMT("Provisioning trigger capture started" CR);
    pinMode(WIFI_CONFIG_PB_PIN, INPUT_PULLUP);
    uint8_t configPBSecs = 0;
    while (!digitalRead(WIFI_CONFIG_PB_PIN)) {
        configPBSecs++;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    if (configPBSecs > FACTORY_RESET_PROVISIONING_TIMEOUT_S) {
        provisionAction = provisionAction | PROVISIONING_DEFAULT_FROM_FACTORY_RESET;
        LOG_INFO_NOFMT("Factory reset and provisioning ordered" CR);
    }
    else if (configPBSecs > PROVISIONING_TIMEOUT_S) {
        provisionAction = provisionAction | PROVISION_DEFAULT_FROM_FILE;
        LOG_INFO_NOFMT("Provisioning with default settings from configuration file ordered" CR);
    }
}

void networking::start(void) {
    LOG_INFO_NOFMT("Starting networking service" CR);
    sysState = new (heap_caps_malloc(sizeof(systemState), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) systemState(NULL);
    sysState->setSysStateObjName("networking");
    sysState->setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_NOIP | OP_UNCONFIGURED);
    WiFiWdTimerArgs.arg = NULL;
    WiFiWdTimerArgs.callback = WiFiWdTimerArgs.callback = static_cast<esp_timer_cb_t>
        (&networking::wifiWdTimeout);
    WiFiWdTimerArgs.dispatch_method = ESP_TIMER_TASK;
    WiFiWdTimerArgs.name = "WiFi WD timer";
    if (esp_timer_create(&WiFiWdTimerArgs, &WiFiWdTimerHandle)) {
        panic("Could not initialize WiFi watch-dog timer" CR);
        return;
    }
    if (!getAps(NULL)){
        panic("No WiFi networks found" CR);
        return;
    }
    sysState->regSysStateCb(NULL, wifiWd);
    WiFi.useStaticBuffers(true);
    WiFi.onEvent(&WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);
    LOG_TERSE_NOFMT("Setting up the WifI provisioning manager service" CR);
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
    wifiManager.setRestorePersistent(true);
    //wifiManager.setAPClientCheck(false); // Inhibits timeout when client connects to WiFi manager AP
    wifiManager.setWiFiAutoReconnect(true);
    wifiManager.setConfigResetCallback(resetCb);
    wifiManager.setConfigPortalTimeoutCallback(configurePortalConnectTimeoutCb);
    hostNameConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("HostName", "HostName",
                               networkConfig.hostName, 31);
    wifiManager.addParameter(hostNameConfigParam);
    mqttServerUriConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("MQTTserverURI", "MQTTserverURI",
                                    networkConfig.mqttUri, 100);
    wifiManager.addParameter(mqttServerUriConfigParam);
    char tmpMqttPort[6];
    mqttServerPortConfigParam = new (heap_caps_malloc(sizeof(WiFiManagerParameter), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) WiFiManagerParameter("MQTTserverPort", "MQTTserverPort",
                                     itoa(networkConfig.mqttPort, tmpMqttPort, 10), 5);
    wifiManager.addParameter(mqttServerPortConfigParam);
    char apSsid[50];
    sprintf(apSsid, "%s_%s", WIFI_MGR_AP_NAME_PREFIX, getMac());
    if (recoverPersistantConfig(&networkConfig)) {
        LOG_INFO("No valid persistant configuration found, "
                 "formatting filesystem, resetting to factory default settings "
                 "and starting provisioning AP, "
                 "connect to SSID: %s_%s and navigate to http://%s to "
                 "configure the device..." CR,
                 WIFI_MGR_AP_NAME_PREFIX, getMac(),
                 IPAddress(WIFI_MGR_AP_IP).toString().c_str());
        fileSys::format();
        wifiManager.resetSettings();
        factoryResetSettings(&networkConfig);
        setNetworkUiDefault(&networkConfig);
        wifiManager.startConfigPortal(apSsid);
        wifiManager.stopConfigPortal();
        setNetworkConfig(&networkConfig);
        LOG_INFO_NOFMT("Wifi provisiong service stopped, networking service started" CR);
    }
    else if (provisionAction & PROVISIONING_DEFAULT_FROM_FACTORY_RESET) {
        LOG_INFO("Config button was hold down for more than %i seconds: "
                 "formatting filesystem, resetting to factory default settings "
                 "and starting provisioning AP, "
                 "connect to SSID: %s_%s and navigate to http://%s to "
                 "configure the device..." CR, FACTORY_RESET_PROVISIONING_TIMEOUT_S,
                 WIFI_MGR_AP_NAME_PREFIX, getMac(),
                 IPAddress(WIFI_MGR_AP_IP).toString().c_str());
        fileSys::format();
        wifiManager.resetSettings();
        factoryResetSettings(&networkConfig);
        setNetworkUiDefault(&networkConfig);
        wifiManager.startConfigPortal(apSsid);
        wifiManager.stopConfigPortal();
        setNetworkConfig(&networkConfig);
        LOG_INFO_NOFMT("Wifi provisiong service stopped, networking service started" CR);

    }
    else if (networkConfig.previousNetworkFail >= WIFI_WD_ESCALATION_CNT_THRES) {
        LOG_INFO("WiFi connect escalationConfig, could not connect to WiFi for %i consecutive reboots - "
                 "starting provisioning AP, "
                 "connect to SSID: %s_%s and navigate to http://%s to configure "
                 "the device" CR, WIFI_WD_ESCALATION_CNT_THRES, WIFI_MGR_AP_NAME_PREFIX, getMac(),
                 IPAddress(WIFI_MGR_AP_IP).toString().c_str());
        getNetworkConfig(&networkConfig);
        setNetworkUiDefault(&networkConfig);
        wifiManager.startConfigPortal(apSsid);
        wifiManager.stopConfigPortal();
        setNetworkConfig(&networkConfig);
        LOG_INFO_NOFMT("Wifi provisiong service stopped, networking service started" CR);

    }
    else if (provisionAction & PROVISION_DEFAULT_FROM_FILE) {
        LOG_INFO("Config button was hold down for more than "
                 "%i seconds and less than %i seconds - starting provisioning AP, "
                 "connect to SSID: %s_%s and navigate to http://,%s to configure "
                 "the device" CR, PROVISIONING_TIMEOUT_S, FACTORY_RESET_PROVISIONING_TIMEOUT_S,
                 WIFI_MGR_AP_NAME_PREFIX, getMac(),
                 IPAddress(WIFI_MGR_AP_IP).toString().c_str());
        getNetworkConfig(&networkConfig);
        setNetworkUiDefault(&networkConfig);
        wifiManager.startConfigPortal(apSsid);
        wifiManager.stopConfigPortal();
        setNetworkConfig(&networkConfig);
        LOG_INFO_NOFMT("Wifi provisiong service stopped, networking service started" CR);
    }
    else {
        LOG_INFO("A valid networking configuration was found - "
                 "starting the networking service, connecting to SSID: %s" CR,
                 networkConfig.wifiConfig.ssid);
        getNetworkConfig(&networkConfig);
        sysState->unSetOpStateByBitmap(OP_UNCONFIGURED);
        setNetworkConfig(&networkConfig);
        LOG_INFO_NOFMT("Networking service started" CR);
    }
}

rc_t networking::regWifiEventCallback(const wifiEventCallback_t p_callback,
                                      const void* p_args) {
    LOG_INFO("Regestring WiFi Event call-back function at: 0x%X with arg 0x%X" CR, p_callback, p_args);
    for (uint8_t i = 0; i < wifiEventCallbackList.size(); i++) {
        if ((wifiEventCallbackList.at(i)->cb == p_callback) && wifiEventCallbackList.at(i)->args == p_args) {
            LOG_WARN("Could not register WiFi Provision callback 0x%X, already exists" CR, p_callback);
            return RC_ALREADYEXISTS_ERR;
        }
    }
    wifiEventCallbackInstance_t* wifiEventCallbackInstance = 
        new (heap_caps_malloc(sizeof(wifiEventCallbackInstance_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wifiEventCallbackInstance_t;
    wifiEventCallbackInstance->cb = p_callback;
    wifiEventCallbackInstance->args = (void*)p_args;
    wifiEventCallbackList.push_back(wifiEventCallbackInstance);
    return RC_OK;
}

void networking::sendWifiEventCallback(WiFiEvent_t p_event, arduino_event_info_t p_info){
    LOG_TERSE_NOFMT("Sendig WiFi event notifications to those that have subscribed to it" CR);
    for (uint8_t i = 0; i < wifiEventCallbackList.size(); i++) {
        LOG_TERSE("Calling WIFI event callback function: 0x%x" CR,
            wifiEventCallbackList.at(i)->cb);
        wifiEventCallbackList.at(i)->cb(p_event, p_info,
            wifiEventCallbackList.at(i)->args);
    }
}

rc_t networking::unRegWifiEventCallback(const wifiEventCallback_t p_callback,
                                        const void* p_args) {
    LOG_INFO("UnRegestring WiFi Event callback function at: 0x%X with arg 0x%X" CR, p_callback, p_args);
    for (uint8_t i = 0; i < wifiEventCallbackList.size(); i++) {
        if (wifiEventCallbackList.at(i)->cb == p_callback && wifiEventCallbackList.at(i)->args == p_args) {
            delete wifiEventCallbackList.at(i);
            wifiEventCallbackList.clear(i);
            return RC_OK;
        }
    }
    LOG_WARN("Could not remove WiFi event callback 0x%X, not found" CR, p_callback);
    return RC_NOT_FOUND_ERR;
}

rc_t networking::regWifiOpStateCallback(sysStateCb_t p_callback, void* p_args) {
    return sysState->regSysStateCb(p_args, p_callback);
}

rc_t networking::unRegWifiOpStateCallback(sysStateCb_t p_callback, void* p_args) {
    return sysState->unRegSysStateCb(p_callback);
}

rc_t networking::regWifiProvisionCallback(const wifiProvisionCallback_t p_callback,
                                          const void* p_args) {
    LOG_INFO("Regestring WiFi provisioning call-back function at: 0x%X with arg 0x%X" CR, p_callback, p_args);
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        if ((wifiProvisionCallbackList.at(i)->cb == p_callback) && wifiProvisionCallbackList.at(i)->args == p_args) {
            LOG_WARN("Could not register WiFi Provision callback 0x%X, already exists" CR, p_callback);
            return RC_ALREADYEXISTS_ERR;
        }
    }
    wifiProvisionCallbackInstance_t* wifiProvisionCallbackInstance =
        new (heap_caps_malloc(sizeof(wifiProvisionCallbackInstance_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) wifiProvisionCallbackInstance_t;
    wifiProvisionCallbackInstance->cb = p_callback;
    wifiProvisionCallbackInstance->args = (void*)p_args;
    wifiProvisionCallbackList.push_back(wifiProvisionCallbackInstance);
    return RC_OK;
}

void networking::sendNetworkProvisionCallback(void) {
    LOG_TERSE_NOFMT("Sendig WiFi provisioning event notifications to those that have subscribed to it" CR);
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        LOG_VERBOSE("Calling WIFI provisioning event callback function: 0x%x" CR,
            wifiProvisionCallbackList.at(i)->cb);
        wifiProvisionCallbackList.at(i)->cb(WIFI_PROVISIONING_SAVE,
            wifiProvisionCallbackList.at(i)->args);
    }
}

rc_t networking::unRegWifiProvisionCallback(const wifiProvisionCallback_t p_callback,
                                            const void* p_args) {
    LOG_TERSE("Removing WiFi Provision callback 0x%X" CR, p_callback);
    for (uint8_t i = 0; i < wifiProvisionCallbackList.size(); i++) {
        if ((wifiProvisionCallbackList.at(i)->cb == p_callback) && wifiProvisionCallbackList.at(i)->args == p_args ) {
            delete wifiProvisionCallbackList.at(i);
            wifiProvisionCallbackList.clear(i);
            return RC_OK;
        }
    }
    LOG_WARN("Could not remove WiFi Provision callback 0x%X, not found" CR, p_callback);
    return RC_NOT_FOUND_ERR;
}

uint16_t networking::getAps(QList<apStruct_t*>* p_apList, bool p_printOut) {
    uint16_t networks = 0;
    LOG_INFO_NOFMT("Scanning APs" CR);
    networks = WiFi.scanNetworks();
    if (networks)
        LOG_INFO("%i WiFi networks were found" CR, networks);
    else {
        LOG_INFO("No WiFi networks were found:" CR, networks);
        return 0;
    }
    if (p_printOut) {
        LOG_INFO("Following %i networks were found:" CR, networks);
        LOG_INFO("| %*s | %*s | %*s | %*s | %*s |" CR,
                 -30, "SSID:", -17, "BSSID:",
                 -5, "RSSI:", -8, "Channel:", -20, "Encryption:");
    }
    for (int i = 0; i < networks; i++) {
        apStruct_t* apInfo = new (heap_caps_malloc(sizeof(apStruct_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) apStruct_t;
        strcpy(apInfo->ssid, WiFi.SSID(i).c_str());
        apInfo->rssi = WiFi.RSSI(i);
        apInfo->channel = WiFi.channel(i);
        sprintf(apInfo->bssid, "%s", WiFi.BSSIDstr(i).c_str());
        strcpy(apInfo->encryption, getEncryptionStr(WiFi.encryptionType(i)));
        if (p_printOut){
            LOG_INFO("| %*s | %*s | %*i | %*i | %*s |" CR,
                      -30, apInfo->ssid, -17, apInfo->bssid,
                      -5, apInfo->rssi, -8, apInfo->channel,
                      -20, apInfo->encryption);
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
    LOG_INFO_NOFMT("unScanning / Forgetting APs" CR);
    while (p_apList->size()) {
        delete p_apList->back();
        p_apList->pop_back();
    }
}

rc_t networking::setSsidNPass(const char* p_ssid, const char* p_pass, bool p_persist) {
    LOG_INFO("Setting %s SSID: %s and Pass: %s" CR, p_persist? "persistant" : "non persistant", p_ssid, p_pass);
    getNetworkConfig(&networkConfig);
    char ssidBackup[50];
    strcpy(ssidBackup, networkConfig.wifiConfig.ssid);
    strcpy(networkConfig.wifiConfig.ssid, p_ssid);
    char passBackup[50];
    strcpy(passBackup, networkConfig.wifiConfig.pass);
    strcpy(networkConfig.wifiConfig.pass, p_pass);

    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR_NOFMT("Could not set SSID and Pass, reverting to previous SSID and Pass configuration" CR);
        strcpy(networkConfig.wifiConfig.ssid, ssidBackup);
        strcpy(networkConfig.wifiConfig.pass, passBackup);
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

rc_t networking::setSsid(const char* p_ssid, bool p_persist) {
    LOG_INFO("Setting SSID: %s" CR, p_ssid);
    getNetworkConfig(&networkConfig);
    char ssidBackup[50];
    strcpy(ssidBackup, networkConfig.wifiConfig.ssid);
    strcpy(networkConfig.wifiConfig.ssid, p_ssid);
    if (setNetworkConfig(&networkConfig)) {
        LOG_ERROR_NOFMT("Could not set SSID, reverting to previous SSID configuration" CR);
        strcpy(networkConfig.wifiConfig.ssid, ssidBackup);
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

char* networking::getSsid(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.ssid;
}

char* networking::getBssid(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.bssid;
}

char* networking::getAuth(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.encryption;
}

rc_t networking::setPass(const char* p_pass, bool p_persist) {
    LOG_INFO("Setting WiFi password: %s" CR, p_pass);
    getNetworkConfig(&networkConfig);
    char passBackup[30];
    strcpy(passBackup, networkConfig.wifiConfig.pass);
    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR_NOFMT("Could not set WiFi password, reverting to previous configuration" CR);
        strcpy(networkConfig.wifiConfig.pass, passBackup);
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

char* networking::getPass(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.wifiConfig.pass;
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

rc_t networking::setDHCP(bool p_persist) {
    LOG_INFO_NOFMT("Setting DHCP network operation" CR);
    getNetworkConfig(&networkConfig);
    bool dhcpBackup = networkConfig.staticIp;
    networkConfig.staticIp = false;
    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR_NOFMT("Could not set DHCP network operation, reverting network configuration" CR);
        networkConfig.staticIp = dhcpBackup;
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

rc_t networking::setStaticIpAddr(const char* p_address, const char* p_mask, const char* p_gw, const char* p_dns, bool p_persist) {
    if (!(isIpAddress(p_address) && isIpAddress(p_mask) && isIpAddress(p_gw) && isIpAddress(p_dns))) {
        LOG_ERROR_NOFMT("One or more of provided Static IP-Addresses is not a valid IP-Addresses " CR);
        return RC_PARAMETERVALUE_ERR;
    }
    else{
        IPAddress ipAddr;
        ipAddr.fromString(p_address);
        IPAddress ipMask;
        ipMask.fromString(p_mask);
        IPAddress ipGw;
        ipGw.fromString(p_gw);
        IPAddress ipDns;
        ipGw.fromString(p_dns);
        return setStaticIpAddr(ipAddr, ipMask, ipGw, ipDns, p_persist);
    }
}
rc_t networking::setStaticIpAddr(IPAddress p_address, IPAddress p_mask, IPAddress p_gw, IPAddress p_dns, bool p_persist) {
    LOG_INFO("Setting Static IP address network operation - IP: %s, MASK: %s, GW: %s, DNS: %s" CR,
                    p_address.toString().c_str(),
                    p_mask.toString().c_str(),
                    p_gw.toString().c_str(),
                    p_dns.toString().c_str());
    getNetworkConfig(&networkConfig);
    IPAddress ipAddrBackup = networkConfig.ipAddr;
    IPAddress ipMaskBackup = networkConfig.ipMask;
    IPAddress ipGwBackup = networkConfig.gatewayIpAddr;
    IPAddress ipDnsBackup = networkConfig.dnsIpAddr;
    bool staticIpBackup = networkConfig.staticIp;
    networkConfig.ipAddr = p_address;
    networkConfig.ipMask = p_mask;
    networkConfig.gatewayIpAddr = p_gw;
    networkConfig.dnsIpAddr = p_dns;
    networkConfig.staticIp = true;
    if (setNetworkConfig(&networkConfig, p_persist)){
        LOG_ERROR_NOFMT("Could not set Static IP address network operation, reverting to previous configuration" CR);
        networkConfig.ipAddr = ipAddrBackup;
        networkConfig.ipMask = ipMaskBackup;
        networkConfig.gatewayIpAddr = ipGwBackup;
        networkConfig.dnsIpAddr = ipDnsBackup;
        networkConfig.staticIp = staticIpBackup;
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

bool networking::isStatic(void) {
    return networkConfig.staticIp;
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
    LOG_INFO("Setting Hostname to: %s" CR, p_hostname);
    if(!isHostName(p_hostname)){
        LOG_ERROR("Hostname: \"%s\" is not a valid Hostname " CR, p_hostname);
        return RC_PARAMETERVALUE_ERR;
    }
    getNetworkConfig(&networkConfig);
    char hostNameBackup[100];
    strcpy(hostNameBackup, networkConfig.hostName);
    strcpy(networkConfig.hostName, p_hostname);
    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR("Could not set Hostname: \"%s\" reverting to previous configuration " CR, p_hostname);
        strcpy(networkConfig.hostName, hostNameBackup);
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

char* networking::getHostname(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.hostName;
}

rc_t networking::setMqttUri(IPAddress p_mqttIP, bool p_persist) {
    return setMqttUri(p_mqttIP.toString().c_str(), p_persist);
}

rc_t networking::setMqttUri(const char* p_mqttUri, bool p_persist) {
    LOG_INFO("Setting MQTT-URI to: %s" CR, p_mqttUri);
    if (!(isUri(p_mqttUri) || isIpAddress(p_mqttUri))) {
        LOG_ERROR("MQTT URI: %s is not a valid URI or IP-Address" CR,
                  p_mqttUri);
        return RC_PARAMETERVALUE_ERR;
    }
    char mqttUriBackup[100];
    strcpy(mqttUriBackup, networkConfig.mqttUri);
    strcpy(networkConfig.mqttUri, p_mqttUri);
    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR_NOFMT("Could not set MQTT URI, reverting to previous configuration" CR);
        strcpy(networkConfig.mqttUri, mqttUriBackup);
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

char* networking::getMqttUri(void) {
    getNetworkConfig(&networkConfig);
    return networkConfig.mqttUri;
}

rc_t networking::setMqttPort(const char* p_mqttPort, bool p_persist) {
    LOG_INFO("Setting MQTT-Port to: %s" CR, p_mqttPort);
    if (!atoi(p_mqttPort)) {
        LOG_ERROR_NOFMT("MQTT-Port is not a valid port" CR);
        return RC_PARAMETERVALUE_ERR;
    }
    else
        return setMqttPort(atoi(p_mqttPort), p_persist);
}

rc_t networking::setMqttPort(int32_t p_mqttPort, bool p_persist) {
    LOG_INFO("Setting MQTT-Port to: %i" CR, p_mqttPort);
    if (!isIpPort(p_mqttPort)) {
        LOG_ERROR_NOFMT("MQTT-Port is not a valid port" CR);
        return RC_PARAMETERVALUE_ERR;
    }
    uint16_t mqttPortBackup = networkConfig.mqttPort;
    networkConfig.mqttPort = p_mqttPort;
    if (setNetworkConfig(&networkConfig, p_persist)) {
        LOG_ERROR_NOFMT("Could not set MQTT-Port, reverting to previous configuration" CR);
        networkConfig.mqttPort = mqttPortBackup;
        setNetworkConfig(&networkConfig, p_persist);
        return RC_PARAMETERVALUE_ERR;
    }
    return RC_OK;
}

uint16_t networking::getMqttPort(void) {
        getNetworkConfig(&networkConfig);
        return networkConfig.mqttPort;
}

uint8_t networking::getNwFail(void) {
    return networkConfig.previousNetworkFail;
}

void networking::reportNwFail(void) {
    if (!nwFailCounted){
        networkConfig.previousNetworkFail++;
        persistentSaveConfig(&networkConfig);
        nwFailCounted = true;
    }
}

void networking::concludeRestart(void) {
    LOG_INFO_NOFMT("Restart of services that directly depend on "
                   "Networking, including any reporting of errors "
                   "from those services concluded" CR);
    if (!nwFailCounted) {
        networkConfig.previousNetworkFail = 0;
        persistentSaveConfig(&networkConfig);
    }
}

sysState_t networking::getOpStateBitmap(void) {
    return sysState->getOpStateBitmap();
}

char* networking::getOpStateStr(char* p_opStateStr) {
    sysState->getOpStateStr(p_opStateStr);
    return p_opStateStr;
}

rc_t networking::getNetworkConfig(netwStaConfig_t* p_staConfig) {
    bool inValidConf = false;
    if (wifiManager.getWiFiSSID(true).length() == 0)
        inValidConf = true;
    else
        strcpy(p_staConfig->wifiConfig.ssid, wifiManager.getWiFiSSID(true).c_str());
    strcpy(p_staConfig->wifiConfig.bssid, WiFi.BSSIDstr().c_str());
    p_staConfig->wifiConfig.channel = WiFi.channel();
    p_staConfig->wifiConfig.rssi = WiFi.RSSI();
    strcpy(p_staConfig->mac, WiFi.macAddress().c_str());
    if (!networkConfig.staticIp) {
        LOG_INFO_NOFMT("DHCP address allocation configured, getting asigned addresses" CR);
        p_staConfig->ipAddr = WiFi.localIP();
        p_staConfig->ipMask = WiFi.subnetMask();
        p_staConfig->gatewayIpAddr = WiFi.gatewayIP();
        p_staConfig->dnsIpAddr = WiFi.dnsIP();
    }
    else {
        LOG_INFO_NOFMT("Static IP configured, static addresses already known" CR);
    }
    strcpy(p_staConfig->hostName, (WiFi.getHostname()));
    if (inValidConf) {
        LOG_ERROR_NOFMT("Retreived configuration not valid - reverting to previously known configuration" CR);
        return RC_GEN_ERR;
    }
    return RC_OK;
}

rc_t networking::setNetworkConfig(netwStaConfig_t* p_staConfig, bool p_persist) {
    LOG_INFO("Setting %s networking configuration" CR, p_persist? "persistant" : "non persistant"); // BETTER SHOW THE CONFIG
    WiFi.disconnect();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    WiFi.persistent(p_persist); //Broken
    if (p_staConfig->staticIp){
        LOG_INFO("Setting WiFi static address configuration, address: %s, gateway: %s, mask: %s, DNS: %s" CR,
                 p_staConfig->ipAddr.toString(), p_staConfig->gatewayIpAddr.toString(),
                 p_staConfig->ipMask.toString(), p_staConfig->dnsIpAddr.toString());
        WiFi.config(p_staConfig->ipAddr, p_staConfig->gatewayIpAddr, p_staConfig->ipMask, p_staConfig->dnsIpAddr);
        WiFi.setHostname(p_staConfig->hostName);
        WiFi.begin(p_staConfig->wifiConfig.ssid, getPass());
    }
    else {
        LOG_INFO("Setting WiFi DHCP configuration" CR);
        WiFi.setHostname(p_staConfig->hostName);
        WiFi.begin(p_staConfig->wifiConfig.ssid, p_staConfig->wifiConfig.pass);
    }
    if (p_persist) {
        persistentSaveConfig(p_staConfig);
        WiFi.persistent(false);
    }
    sendNetworkProvisionCallback();
    return RC_OK;
}

void networking::cpyNetworkConfig(netwStaConfig_t* dst, const netwStaConfig_t* src) {
    memcpy(dst->wifiConfig.ssid, src->wifiConfig.ssid, sizeof(*dst->wifiConfig.ssid));
    memcpy(dst->wifiConfig.bssid, src->wifiConfig.bssid, sizeof(*dst->wifiConfig.bssid));
    dst->wifiConfig.channel = src->wifiConfig.channel;
    dst->wifiConfig.rssi = src->wifiConfig.rssi;
    memcpy(dst->wifiConfig.encryption, src->wifiConfig.encryption, sizeof(*dst->wifiConfig.encryption));
    memcpy(dst->wifiConfig.pass, src->wifiConfig.pass, sizeof(*dst->wifiConfig.pass));
    memcpy(dst->mac, src->mac, sizeof(*dst->mac));
    dst->staticIp = src->staticIp;
    dst->ipAddr = src->ipAddr;
    dst->ipMask = src->ipMask;
    dst->gatewayIpAddr = src->gatewayIpAddr;
    dst->dnsIpAddr = src->dnsIpAddr;
    memcpy(dst->hostName, src->hostName, sizeof(*dst->hostName));
    memcpy(dst->mqttUri, src->mqttUri, sizeof(*dst->mqttUri));
    dst->mqttPort = src->mqttPort;
    dst->previousNetworkFail = src->previousNetworkFail;
}

rc_t networking::persistentSaveConfig(const netwStaConfig_t* p_staConfig) {
    LOG_INFO("Saving Network configuration to "
             "persistant store: %s" CR, WIFI_CONFIG_STORE_FILENAME);
    DynamicJsonDocument wifiConfigJsonDoc(WIFI_CONFIG_JSON_OBJ_SIZE);
    char wifiConfigJsonPrettySerialized[WIFI_CONFIG_JSON_SERIAL_SIZE];
    wifiConfigJsonDoc["wifiConfig"]["ssid"] = p_staConfig->wifiConfig.ssid;
    wifiConfigJsonDoc["wifiConfig"]["bssid"] = p_staConfig->wifiConfig.bssid;
    wifiConfigJsonDoc["wifiConfig"]["channel"] = p_staConfig->wifiConfig.channel;
    wifiConfigJsonDoc["wifiConfig"]["rssi"] = p_staConfig->wifiConfig.rssi;
    wifiConfigJsonDoc["wifiConfig"]["encryption"] = p_staConfig->wifiConfig.encryption;
    wifiConfigJsonDoc["wifiConfig"]["pass"] = p_staConfig->wifiConfig.pass;
    wifiConfigJsonDoc["mac"] = p_staConfig->mac;
    wifiConfigJsonDoc["staticIp"] = p_staConfig->staticIp;
    wifiConfigJsonDoc["ipAddr"] = p_staConfig->ipAddr;
    wifiConfigJsonDoc["ipMask"] = p_staConfig->ipMask;
    wifiConfigJsonDoc["gatewayIpAddr"] = p_staConfig->gatewayIpAddr;
    wifiConfigJsonDoc["dnsIpAddr"] = p_staConfig->dnsIpAddr;
    wifiConfigJsonDoc["hostName"] = p_staConfig->hostName;
    wifiConfigJsonDoc["mqttUri"] = p_staConfig->mqttUri;
    wifiConfigJsonDoc["mqttPort"] = p_staConfig->mqttPort;
    wifiConfigJsonDoc["previousNetworkFail"] = p_staConfig->previousNetworkFail;
    serializeJsonPretty(wifiConfigJsonDoc, wifiConfigJsonPrettySerialized);
    LOG_VERBOSE("Wifi pretty Json content " \
                "to be stored: %s" CR, wifiConfigJsonPrettySerialized);
    uint storeSize;
    if (fileSys::putFile(WIFI_CONFIG_STORE_FILENAME,
                         wifiConfigJsonPrettySerialized,
                         strlen(wifiConfigJsonPrettySerialized) + 1, &storeSize)) {
        LOG_ERROR("Could not save configuration " \
                  "to file %s" CR, WIFI_CONFIG_STORE_FILENAME);
        return RC_GEN_ERR;
    }
    LOG_VERBOSE("Configuration saved to file %s" CR,
                WIFI_CONFIG_STORE_FILENAME);
    return RC_OK;
}

rc_t networking::recoverPersistantConfig(netwStaConfig_t* p_staConfig) {
    LOG_INFO("Recovering Network configuration from "
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
	wifiConfigJsonPrettySerialized[readSize] = '\0';
    LOG_VERBOSE("Wifi pretty Json content "
                "recovered: %s" CR, wifiConfigJsonPrettySerialized);
    if (deserializeJson(wifiConfigJsonDoc, wifiConfigJsonPrettySerialized)) {
        LOG_ERROR_NOFMT("Could not parse/deserialize persistant Wifi configuration" CR);
        return RC_GEN_ERR;
    }
    if (wifiConfigJsonDoc["wifiConfig"]["ssid"].isNull() || wifiConfigJsonDoc["wifiConfig"]["bssid"].isNull() ||
        wifiConfigJsonDoc["wifiConfig"]["channel"].isNull() || wifiConfigJsonDoc["wifiConfig"]["rssi"].isNull() ||
        wifiConfigJsonDoc["wifiConfig"]["encryption"].isNull() || wifiConfigJsonDoc["wifiConfig"]["pass"].isNull() ||
        wifiConfigJsonDoc["mac"].isNull() || wifiConfigJsonDoc["staticIp"].isNull() ||
        wifiConfigJsonDoc["ipAddr"].isNull() || wifiConfigJsonDoc["ipMask"].isNull() ||
        wifiConfigJsonDoc["gatewayIpAddr"].isNull() || wifiConfigJsonDoc["dnsIpAddr"].isNull() ||
        wifiConfigJsonDoc["hostName"].isNull() || wifiConfigJsonDoc["mqttUri"].isNull() ||
        wifiConfigJsonDoc["mqttPort"].isNull() || wifiConfigJsonDoc["previousNetworkFail"].isNull()) {
        LOG_ERROR_NOFMT("Could not recover persistant Wifi configuration, schema entries missing..." CR);
        return RC_NOT_FOUND_ERR;
    }
    strcpy(p_staConfig->wifiConfig.ssid, wifiConfigJsonDoc["wifiConfig"]["ssid"]);
    strcpy(p_staConfig->wifiConfig.bssid, wifiConfigJsonDoc["wifiConfig"]["bssid"]);
    p_staConfig->wifiConfig.channel = wifiConfigJsonDoc["wifiConfig"]["channel"];
    p_staConfig->wifiConfig.rssi = wifiConfigJsonDoc["wifiConfig"]["rssi"];
    strcpy(p_staConfig->wifiConfig.encryption,
           wifiConfigJsonDoc["wifiConfig"]["encryption"]);
    strcpy(p_staConfig->wifiConfig.pass,
        wifiConfigJsonDoc["wifiConfig"]["pass"]);
    strcpy(p_staConfig->mac, wifiConfigJsonDoc["mac"]);
    p_staConfig->mqttPort = wifiConfigJsonDoc["mqttPort"];
    p_staConfig->staticIp = wifiConfigJsonDoc["staticIp"];
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
    LOG_TERSE_NOFMT("Setting Factory Network default settings" CR);
    strcpy(p_staConfig->wifiConfig.ssid, "");
    strcpy(p_staConfig->wifiConfig.bssid, "");
    strcpy(p_staConfig->wifiConfig.encryption, getEncryptionStr(WiFi.encryptionType(0)));
    strcpy(p_staConfig->wifiConfig.pass, "");
    p_staConfig->wifiConfig.channel = WiFi.channel();
    p_staConfig->wifiConfig.rssi = WiFi.RSSI();
    strcpy(p_staConfig->mac, WiFi.macAddress().c_str());
    p_staConfig->staticIp = false;
    p_staConfig->ipAddr = IPAddress(0, 0, 0, 0);
    p_staConfig->ipMask = IPAddress(0, 0, 0, 0);
    p_staConfig->gatewayIpAddr = IPAddress(0, 0, 0, 0);
    p_staConfig->dnsIpAddr = IPAddress(0, 0, 0, 0);
    sprintf(p_staConfig->hostName, "%s_%s", WIFI_ESP_HOSTNAME_PREFIX,
            WiFi.macAddress().c_str());
    strcpy(p_staConfig->mqttUri, MQTT_DEFAULT_URI);
    p_staConfig->mqttPort = MQTT_DEFAULT_PORT;
    p_staConfig->previousNetworkFail = 0;
}

void networking::setNetworkUiDefault(netwStaConfig_t* p_staConfig) {
    if (p_staConfig->staticIp) {
        //wifiManager.setShowSTAStaticIPFields(p_staConfig->ipAddr, p_staConfig->gatewayIpAddr, p_staConfig->ipMask, p_staConfig->dnsIpAddr); // Needs a pullrequest proposal to the WiFiManager project
    }
    hostNameConfigParam->setValue(p_staConfig->hostName, 31);
    mqttServerUriConfigParam->setValue(p_staConfig->mqttUri, 100);
    char mqttPortStr[6];
    itoa(p_staConfig->mqttPort, mqttPortStr, 10);
    mqttServerPortConfigParam->setValue(mqttPortStr, 5);
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

void networking::WiFiEvent(WiFiEvent_t p_event, arduino_event_info_t p_info) { //Should this be serialized into a job-queue?
    switch (p_event) {
    case ARDUINO_EVENT_WIFI_READY:
        LOG_INFO_NOFMT("WiFi service ready" CR);
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        LOG_INFO_NOFMT("WiFi APs have been scanned" CR);
        break;
    case SYSTEM_EVENT_STA_START:
        LOG_INFO_NOFMT("Station Mode Started" CR);
        sysState->unSetOpStateByBitmap(OP_INIT);
        break;
    case SYSTEM_EVENT_STA_STOP:
        LOG_INFO_NOFMT("Station Mode Stoped" CR);
        sysState->setOpStateByBitmap(OP_INIT | OP_DISCONNECTED | OP_NOIP);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        sysState->unSetOpStateByBitmap(OP_INIT | OP_DISCONNECTED);
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
        LOG_INFO("Authentication mode changed to: %s" CR, getAuth());
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
        LOG_ERROR_NOFMT("Station lost IP - reconnecting" CR);
        WiFi.reconnect();
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        LOG_INFO_NOFMT("Station WPS success" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        LOG_INFO_NOFMT("Station WPS failed" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        LOG_INFO_NOFMT("Station WPS timeout" CR);
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        LOG_INFO_NOFMT("Station WPS PIN failure" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_START:
        LOG_INFO("AP started with SSID %s, "
                 "IP address: %s" CR, WiFi.softAPSSID().c_str(),
                 WiFi.softAPIP().toString().c_str());
        break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
        LOG_INFO_NOFMT("WiFiEvent: AP stoped" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
        LOG_INFO_NOFMT("WiFiEvent: A Client connected to the AP" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
        LOG_INFO_NOFMT("A Client disconnected from the AP" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
        LOG_INFO_NOFMT("The AP asigned an IP address to "
                   "a client" CR);
        break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
        LOG_INFO_NOFMT("The AP received a probe request" CR);
        break;
    default:
        LOG_ERROR("Received a non recognized WIFI event: %i" CR,
                  p_event);
        break;
    }
    sendWifiEventCallback(p_event, p_info);
}

void networking::configModeCb(WiFiManager* p_WiFiManager) {
    LOG_INFO("myRailIOdecoder provisioning manager "
             "started - AP SSID: %s, IP address %s" CR,
             wifiManager.getConfigPortalSSID().c_str(),
             WiFi.softAPIP().toString().c_str());
    cpyNetworkConfig(&networkConfigBackup, &networkConfig);
}

void networking::preSaveConfigCb() {
    LOG_INFO("myRailIOdecoder provisioning manager "
        "Network parameters are about to be saved, validating Network parameters" CR);
    provisionValidationErr = NETWORK_VALIDATION_NO_ERR;
    if (!(isUri(mqttServerUriConfigParam->getValue()) || isIpAddress(mqttServerUriConfigParam->getValue()))) {
        LOG_ERROR("Configured/provided MQTT URI %s does not qualify as a legimite URI, nor a legimite IP-address" CR, mqttServerUriConfigParam->getValue());
        provisionValidationErr = provisionValidationErr | NETWORK_VALIDATION_MQTTURI_ERR;
    }
    if (!isIpPort(mqttServerPortConfigParam->getValue())) {
        LOG_ERROR("Configured/provided MQTT Port %s does not qualify as a legimite MQTT port" CR, mqttServerPortConfigParam->getValue());
        provisionValidationErr = provisionValidationErr | NETWORK_VALIDATION_MQTTPORT_ERR;
    }
    if (!isHostName(hostNameConfigParam->getValue())){
        LOG_ERROR("Configured/provided Hostname %s does not qualify as a legimite Hostname" CR, hostNameConfigParam->getValue());
        provisionValidationErr = provisionValidationErr | NETWORK_VALIDATION_HOSTNAME_ERR;
    }
    if (!provisionValidationErr) {
        LOG_INFO_NOFMT("Validation of Network parameters succeded, setting hostname early as is needed" CR);
        WiFi.setHostname(hostNameConfigParam->getValue());
    }
    //LOG_ERROR("Validation of Network parameters failed: %s" CR, provisionValidationErrToStr(provisionValidationErr));
}

void networking::saveConfigCb() {
    LOG_INFO_NOFMT("A networking config has been saved or is about to be saved" CR);
    if (provisionValidationErr) {
        //HERE WE HAVE PRESAVE ERRORS, SKIP SAVE, AND RESTART THE UI OR USE BACKUP TO RESTORE?
    }
    strcpy(networkConfig.wifiConfig.ssid, wifiManager.getWiFiSSID().c_str());
    strcpy(networkConfig.wifiConfig.bssid, WiFi.BSSIDstr().c_str());
    networkConfig.wifiConfig.channel = WiFi.channel();
    networkConfig.wifiConfig.rssi = WiFi.RSSI();
    strcpy(networkConfig.wifiConfig.encryption,
           getEncryptionStr(WiFi.encryptionType(0)));
    strcpy(networkConfig.wifiConfig.pass, 
           wifiManager.getWiFiPass().c_str());
    strcpy(networkConfig.mac,
           WiFi.macAddress().c_str());
    if (networkConfig.staticIp = wifiManager.isStaticIp()) {
        wifiManager.getStaticIp(&networkConfig.ipAddr);
        wifiManager.getStaticSn(&networkConfig.ipMask);
        wifiManager.getStaticGw(&networkConfig.gatewayIpAddr);
        wifiManager.getStaticDns(&networkConfig.dnsIpAddr);
    }
    else {
        networkConfig.ipAddr = WiFi.localIP();
        networkConfig.ipMask = WiFi.subnetMask();
        networkConfig.gatewayIpAddr = WiFi.gatewayIP();
        networkConfig.dnsIpAddr = WiFi.dnsIP();
    }
    strcpy(networkConfig.hostName, hostNameConfigParam->getValue());
    WiFi.setHostname(hostNameConfigParam->getValue()); //Maybe needs to go to pre-save
    strcpy(networkConfig.mqttUri, mqttServerUriConfigParam->getValue());
    networkConfig.mqttPort = atoi(mqttServerPortConfigParam->getValue());
    networkConfig.previousNetworkFail = 0;
    persistentSaveConfig(&networkConfig);
    sendNetworkProvisionCallback();
    sysState->unSetOpStateByBitmap(OP_UNCONFIGURED);
}

void networking::resetCb(void) {
    LOG_INFO_NOFMT("Reseting configuration to factory default ordered from UI" CR);
    factoryResetSettings(&networkConfig);
}

void networking::configurePortalConnectTimeoutCb(void) {
    if (provisionAction & PROVISIONING_DEFAULT_FROM_FACTORY_RESET){
        panic("No client connected to the WiFi provisioning manager despite factory reset request" CR);
        return;
    }
    else {
        LOG_ERROR_NOFMT("No client connected to the WiFi provisioning manager, continuing with current configuration..." CR);
        getNetworkConfig(&networkConfig);
        sysState->unSetOpStateByBitmap(OP_UNCONFIGURED);
    }
}

rc_t networking::startRuntimePortal(void) {
    LOG_INFO_NOFMT("Starting WiFi runtime portal" CR);
    wifiManager.setTitle(WIFI_MGR_HTML_TITLE);
    wifiManager.setShowStaticFields(true);
    wifiManager.setShowDnsFields(true);
    wifiManager.setShowInfoErase(true);
    wifiManager.setShowInfoUpdate(true);
    wifiManager.startWebPortal();
    if (!eTaskCreate(                               // Spinning up a www polling task
        wwwPoll,                                    // Task function
        CPU_WWWPOLL_TASKNAME,                       // Task function name reference
        CPU_WWWPOLL_STACKSIZE_1K * 1024,            // Stack size
        NULL,                                       // Parameter passing
        CPU_WWWPOLL_PRIO,                           // Priority 0-24, higher is more
        CPU_WWWPOLL_STACK_ATTR)) {                  // Stack attibute
        panic("Could not start the WWW poll task");
        return RC_OUT_OF_MEM_ERR;
    }
    return RC_OK;
}
void networking::wwwPoll(void* p_dummy) {
    while (true) {
		wifiManager.process();
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void networking::wifiWd(const void* p_args, sysState_t p_wifiOpState) {
    esp_err_t errCode;
    if (sysState->getOpStateBitmap()) {
        if (!filtering) {
            filtering = true;
            if (errCode = esp_timer_start_once(WiFiWdTimerHandle, WIFI_WD_TIMEOUT_S * 1000000)) {
                panic("Could not start WiFi WD timer, reason: %s" CR, esp_err_to_name(errCode));
                return;
            }
            LOG_VERBOSE_NOFMT("WIFI watchdog timer started" CR);
        }
    }
    else if (filtering){
        filtering = false;
        if (errCode = esp_timer_stop(WiFiWdTimerHandle)) {
            panic("WIFI up and connected, but could not stop WiFi WD timer, reason: %s" CR, esp_err_to_name(errCode));
            return;
        }
        LOG_VERBOSE_NOFMT("WIFI up and connected - Watchdog timer stoped" CR);
    }
}

void networking::wifiWdTimeout(void* p_dummy) {
    reportNwFail();
    panic("Could not establish WiFi connectivity - watchdog timed out" CR);
}
/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/
