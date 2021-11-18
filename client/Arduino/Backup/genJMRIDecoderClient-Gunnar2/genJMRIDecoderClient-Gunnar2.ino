/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2021 Jonas Bjurel (jonas.bjurel@hotmail.com)
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
/* Description                                                                                                                                  */
/*==============================================================================================================================================*/
//
//
/*==============================================================================================================================================*/
/* END Description                                                                                                                              */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "genJMRIDecoderClient.h"

/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Functions: Helper functions                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
char* createNcpystr(const char* src) {
  int length = strlen(src);
  char* dst = new char[length + 1];
  if (dst == NULL) {
    Log.error("createNcpystr: Failed to allocate memory from heap - rebooting..." CR);
    ESP.restart();
  }
  strcpy(dst, src);
  dst[length] = '\0';
  return dst;
}

char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings) {
  int resLen = 0;
  char* dst;
  
  for(uint8_t i=0; i<noOfSrcStrings; i++){
    resLen += strlen(srcStrings[i]) - 1;
  }
  dst = new char[resLen + 1];
  char* stringptr = dst;
  for(uint8_t i=0; i<noOfSrcStrings; i++){
    strcpy(stringptr, srcStrings[i]);
    stringptr += strlen(srcStrings[i] - 1);
  }
  dst[resLen] = '\0';
  return dst;
}

uint8_t getTagTxt(tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len) {
  bool found;
  int i;
  while ((xmlNode = xmlNode->NextSiblingElement()) != NULL) {
    Serial.print("XML tag is: ");
    Serial.println(xmlNode->Name());
    found = false;
    for (i=0; i<len; i++){
      Serial.print("Itterating provided search tags: ");
      Serial.println((char*)tags[i]);
      if (!strcmp((char*)tags[i], xmlNode->Name())){
        Serial.print("Found, value is: ");
        Serial.println(xmlNode->GetText());
        if (xmlNode->GetText() != NULL) {
          xmlTxtBuff[i] = new char[strlen(xmlNode->GetText())];
          strcpy(xmlTxtBuff[i], xmlNode->GetText());
        }
        found = true;
        break;
      }
      //Serial.println("Tag adress: %x",(int)tags);
    }
    if (!found){
      xmlTxtBuff[i] = NULL;
    }
  }
}

/*==============================================================================================================================================*/
/* END Helper functions                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
wdt::wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction){
  wdtData = new wdt_t;
  wdtData->wdtTimeout = p_wdtTimeout*1000;
  strcpy(wdtData->wdtDescription, p_wdtDescription);
  wdtData->wdtAction = p_wdtAction;
  wdtTimer_args.arg =this;
  wdtTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&wdt::kickHelper);
  wdtTimer_args.dispatch_method = ESP_TIMER_TASK;
  wdtTimer_args.name = "FlashTimer";
  esp_timer_create(&wdtTimer_args, &wdtData->timerHandle);
  esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
}

wdt::~wdt(void){
  esp_timer_stop(wdtData->timerHandle);
  esp_timer_delete(wdtData->timerHandle);
  delete wdtData;
  return;
}

void wdt::feed(void){
  esp_timer_stop(wdtData->timerHandle);
  esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
  return;
}

void wdt::kickHelper(wdt* p_wdtObject){
  p_wdtObject->kick();
}
void wdt::kick(void){ //FIX
  return;
}
/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                         */
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
String networking::hostname = "MyHost";
esp_wps_config_t networking::config;
uint8_t networking::opState = OP_INIT;

void networking::start(void){
  pinMode(WPS_PIN, INPUT);
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  mac = WiFi.macAddress();
  Log.notice("networking::start: MAC Address: %s" CR, (char*)&mac[0]);
  Serial.println("MAC Address: " + mac);
  uint8_t wpsPBT = 0;
  while (digitalRead(WPS_PIN)){
    wpsPBT++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  if(wpsPBT == 0){
    WiFi.begin();
  } else if (wpsPBT < 10){
    Log.notice("networking::start: WPS button was hold down for less thann 10 seconds - connecting to a new WIFI network, push the WPS button on your WIFI router..." CR);
    Serial.println("WPS button was hold down for less thann 10 seconds - connecting to a new WIFI network, push the WPS button on your WIFI router..." CR);
    networking::wpsStart();
  } else {
  Log.notice("networking::start: WPS button was hold down for more that 30 seconds - forgetting previous learnd WIFI networks and rebooting..." CR);
  Serial.println("WPS button was hold down for more that 30 seconds - forgetting previous learnd WIFI networks and rebooting...");
  //WiFi.disconnect(eraseap = true);
  WiFi.disconnect();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP.restart();
  }
  return;
}

void networking::regCallback(const netwCallback_t p_callback){
  callback = p_callback;
  return;
}

void networking::wpsStart(){
  Log.notice("networking::wpsStart: Starting WPS" CR);
  Serial.println("Starting WPS");
  networking::wpsInitConfig();
  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
  return;
}

void networking::wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
  return;
}

String networking::wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0; i<8; i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void networking::WiFiEvent(WiFiEvent_t event, system_event_info_t info){
  switch(event){
    if(callback != NULL) {
      callback(event);
    }
    case SYSTEM_EVENT_STA_START:
      Log.notice("networking::WiFiEvent: Station Mode Started" CR);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      opState = OP_CONNECTED;
      ssid = WiFi.SSID();
      channel = WiFi.channel();
      rssi = WiFi.RSSI();
      Log.notice("networking::WiFiEvent: Connected to: %s, channel: %d, RSSSI: %d" CR, (char*)&ssid[0], channel, rssi);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      opState = OP_WORKING;
      ipaddr = WiFi.localIP();
      ipmask = WiFi.subnetMask();
      gateway = WiFi.gatewayIP();
      dns = WiFi.dnsIP();
      ntp = dns;
      hostname = "My host";
      Log.notice("networking::WiFiEvent: Got IP-address: %i.%i.%i.%i, Mask: %i.%i.%i.%i, \nGateway: %i.%i.%i.%i, DNS: %i.%i.%i.%i, \nNTP: %i.%i.%i.%i, Hostname: %s" CR, 
                  ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], ipmask[0], ipmask[1], ipmask[2], ipmask[3], gateway[0], gateway[1], gateway[2], gateway[3],
                  dns[0], dns[1], dns[2], dns[3], ntp[0], ntp[1], ntp[2], ntp[3], hostname);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      opState = OP_FAIL;
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
    case SYSTEM_EVENT_STA_DISCONNECTED:
      networking::opState = OP_FAIL;
      Log.notice("networking::WiFiEvent: Disconnected from station, attempting reconnection" CR);
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
      opState = OP_CONFIG;
      Log.notice("networking::WiFiEvent: WPS Successful, stopping WPS and connecting to: %s" CR, String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      networking::opState = OP_FAIL;
      Log.error("networking::WiFiEvent: WPS Failed, retrying" CR);
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      opState = OP_FAIL;
      Log.error("networking::WiFiEvent: WPS Timeout, retrying" CR);
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + networking::wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
  return;
}

const char* networking::getSsid(void){
  return ssid.c_str();
}

uint8_t networking::getChannel(void){
  return channel;
}

long networking::getRssi(void){
  return rssi;
}

const char* networking::getMac(void){
  return mac.c_str();
}

IPAddress networking::getIpaddr(void){
  return ipaddr;
}

IPAddress networking::getIpmask(void){
  return ipmask;
}

IPAddress networking::getGateway(void){
  return gateway;
}

IPAddress networking::getDns(void){
  return dns;
}

IPAddress networking::getNtp(void){
  return ntp;
}

const char* networking::getHostname(void){
  return hostname.c_str();
}

uint8_t networking::getOpState(void){
  return opState;
}


/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t mqtt::opState = OP_CREATED;

uint8_t mqtt::init(const char* p_broker, uint16_t p_port, const char* p_user, const char* p_pass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, bool p_defaultRetain, const char* p_lwTopic, const char* p_lwPayload, const char* p_upTopic, const char* p_upPayload){
  if(opState != OP_CREATED) {
      Log.fatal("mqtt::init: opState is not OP_CREATED - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
  }
  Log.notice("mqtt::init: Initializing and starting the MQTT client" CR);
  QList<mqttTopic_t*> mqttTopics;
  discovered = false;
  mqttLock = xSemaphoreCreateMutex();
  WiFiClient espClient;
  PubSubClient mqttClient(espClient);
  broker = createNcpystr(p_broker);
  port = p_port;
  user = createNcpystr(p_user);
  pass = createNcpystr(p_pass);
  clientId = createNcpystr(p_clientId);
  defaultQoS = p_defaultQoS;
  keepAlive = p_keepAlive;
  defaultRetain = p_defaultRetain;
  lwTopic = createNcpystr(p_lwTopic);
  lwPayload = createNcpystr(p_lwPayload);
  upTopic = createNcpystr(p_upTopic);
  upPayload = createNcpystr(p_upPayload);
  mqttClient.setServer(broker, port); //Shouldnt it be MQTT.setServer(broker, port);
  mqttClient.setKeepAlive(keepAlive);
  mqttStatus = mqttClient.state();
  if (!mqttClient.setBufferSize(MQTT_BUFF_SIZE)) {
    Log.fatal("mqtt::init: Could not allocate MQTT buffers, with buffer size: %d - rebooting..." CR, MQTT_BUFF_SIZE);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  mqttClient.setCallback(onMqttMsg);
  mqttClient.connect(clientId, 
                     user, 
                     pass, 
                     lwTopic, 
                     QOS_1, 
                     true, 
                     lwPayload);

  mqttStatus = mqttClient.state();
  if (statusCallback != NULL){
    statusCallback(mqttStatus);
  }
  Log.notice("mqtt::init: Spawning MQTT poll task" CR);
  xTaskCreatePinnedToCore(
                          poll,                       // Task function
                          "MQTT polling",             // Task function name reference
                          6*1024,                     // Stack size
                          NULL,                       // Parameter passing
                          10,                         // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          CORE_1);                    // Core [CORE_0 | CORE_1]

  while(true) {
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    if(opState == OP_WORKING) {
      xSemaphoreGive(mqttLock);
      break;
    }
    xSemaphoreGive(mqttLock);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  Log.notice("mqtt::init: Starting discovery process..." CR);
  discover();
  int tries = 0;
  while(true) {
    bool tmpDiscovered;
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    tmpDiscovered = discovered;
    xSemaphoreGive(mqttLock);
    if(tmpDiscovered){
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (tries++ >= 100){
      Log.fatal("mqtt::init: Discovery process failed, no discovery response was received - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  Log.notice("mqtt::init: Decoder successfully discovered; URI: %s" CR, uri);
  if(statusCallback != NULL){
    statusCallback(mqttStatus);
  }
  opState = OP_INIT;
  return RC_OK;
}

void mqtt::discover(void){
  const char* mqtt_discovery_response_topic = MQTT_DISCOVERY_RESPONSE_TOPIC;
  subscribeTopic(mqtt_discovery_response_topic, onDiscoverResponse, NULL);
  const char* mqtt_discovery_request_topic = MQTT_DISCOVERY_REQUEST_TOPIC;
  const char* discovery_req = DISCOVERY_REQ;

  sendMsg(mqtt_discovery_request_topic, discovery_req, false);
}

void mqtt::onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  if(discovered){
    xSemaphoreGive(mqttLock);
    Log.notice("mqtt::discover: Discovery response receied several times - doing nothing ..." CR);
    return;
  }
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::discover: Got a discover response, parsing and validating it..." CR);
  tinyxml2::XMLDocument* xmlDiscoveryDoc;
  xmlDiscoveryDoc = new tinyxml2::XMLDocument;
  tinyxml2::XMLElement* xmlDiscoveryElement;
  if (xmlDiscoveryDoc->Parse(p_payload) || xmlDiscoveryDoc->FirstChildElement("DiscoveryResponse") == NULL) {
    Log.fatal("mqtt::discover: Discovery response parsing or validation failed - Rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  
  if ((xmlDiscoveryElement=xmlDiscoveryElement->FirstChildElement("DiscoveryResponse")) == NULL) {
        Log.fatal("mqtt::discover: Failed to validate the configuration - xml is missformatted - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        delete xmlDiscoveryDoc;
        return;
  }
  xmlDiscoveryElement = xmlDiscoveryElement->FirstChildElement("DiscoveryResponse")->FirstChildElement("Decoder");
  bool found = false;
  while(xmlDiscoveryElement != NULL) {
    if(xmlDiscoveryElement->Value() == "Decoder" && xmlDiscoveryElement->FirstChildElement("MAC") != NULL && xmlDiscoveryElement->FirstChildElement("URI") != NULL && xmlDiscoveryElement->FirstChildElement("URI")->GetText() != NULL && !strcmp(xmlDiscoveryElement->FirstChildElement("MAC")->GetText(), NETWORK.getMac())){
      uri = new char[sizeof(xmlDiscoveryElement->FirstChildElement("URI")->GetText())];
      strcpy(uri, xmlDiscoveryElement->FirstChildElement("URI")->GetText());
      //uri = xmlDiscoveryElement->FirstChildElement("URI")->GetText();
      found = true;
      break;
    }
    xmlDiscoveryElement = xmlDiscoveryElement->NextSiblingElement();
  }
  if(!found){
    Log.fatal("mqtt::discover: Discovery response doesn't provide any information about this decoder (MAC) - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    delete xmlDiscoveryDoc;
    return;
  }
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  discovered = true;
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::discover: Discovery response successful, set URI to %s for this decoders MAC %s" CR, uri, NETWORK.getMac());
  opState = OP_CONFIG;
  delete xmlDiscoveryDoc;
  return;
}

uint8_t mqtt::regStatusCallback(const mqttStatusCallback_t p_statusCallback){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  statusCallback = p_statusCallback;
  xSemaphoreGive(mqttLock);
  return RC_OK;
}

void mqtt::setPingPeriod(float p_pingPeriod){
  pingPeriod = p_pingPeriod;
  return;
}

uint8_t mqtt::subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args){
  bool found = false;
  int i;
  int j;
  char* topic = createNcpystr(p_topic);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  if(opState != OP_WORKING){
    xSemaphoreGive(mqttLock);
    Log.fatal("mqtt::subscribeTopic: Could not subscribe to topic %s, MQTT is not running - rebooting..." CR, topic);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::subscribeTopic: Subscribing to topic %s" CR, topic);
  for (i=0; i < mqttTopics.size(); i++){
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    if (!strcmp(mqttTopics.at(i)->topic, topic)){
      found = true;
      break;
    }
  }
  if (!found){
    mqttTopics.push_back(new(mqttTopic_t));
    mqttTopics.back()->topic = topic;
    mqttTopics.back()->topicList = new QList<mqttSub_t*>;
    mqttTopics.back()->topicList->push_back(new(mqttSub_t));
    mqttTopics.back()->topicList->back()->topic = topic;
    mqttTopics.back()->topicList->back()->mqttSubCallback = p_callback;
    mqttTopics.back()->topicList->back()->mqttCallbackArgs = (void*)p_args;

    if (!mqttClient.subscribe(topic, defaultQoS)){
      Log.fatal("mqtt::subscribeTopic: Could not subscribe Topic: %s from broker - rebooting..." CR, topic);
      xSemaphoreGive(mqttLock);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Log.notice("mqtt::subscribeTopic: Subscribed to %s, QoS %d" CR, mqttTopics.back()->topicList->back()->topic, defaultQoS);
    logSubscribers();
    xSemaphoreGive(mqttLock);
    return RC_OK;
  }
  if (found){
    for (j=0; j<mqttTopics.at(i)->topicList->size(); j++){
      if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback){
        Log.warning("mqtt::subscribeTopic: MQTT-subscribeTopic: subscribeTopic was called, but the callback 0x%x for Topic %s already exists - doing nothing" CR, (int)p_callback, topic);
        xSemaphoreGive(mqttLock);
        return RC_OK;
      }
    }
    mqttTopics.at(i)->topicList->push_back(new(mqttSub_t));
    mqttTopics.at(i)->topicList->back()->topic = topic;
    mqttTopics.at(i)->topicList->back()->mqttSubCallback = p_callback;
    mqttTopics.at(i)->topicList->back()->mqttCallbackArgs = (void*)p_args;

    logSubscribers();
    xSemaphoreGive(mqttLock);
    return RC_OK;
  }
  xSemaphoreGive(mqttLock);
  return RC_GEN_ERR;   //Should never reach here
}

uint8_t mqtt::reSubscribe(void){
  Log.notice("mqtt::reSubscribe: Resubscribing all registered topics" CR);
  logSubscribers();
  for (int i=0; i < mqttTopics.size(); i++){
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    if (!mqttClient.subscribe(mqttTopics.at(i)->topic, defaultQoS)){
      Log.fatal("mqtt::reSubscribe: Failed to resubscribe topic: %s from broker" CR, mqttTopics.at(i)->topic);
      xSemaphoreGive(mqttLock);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    xSemaphoreGive(mqttLock);
  }
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::reSubscribe: Successfully resubscribed to all registered topics" CR);
  return RC_OK;
}

uint8_t mqtt::unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback){
  bool topicFound = false;
  bool cbFound = false;
  char* topic = createNcpystr(p_topic);
  Log.notice("MQTT-Unsubscribe, Un-subscribing to topic %s" CR, topic);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  for(int i=0; i < mqttTopics.size(); i++){
    if (!strcmp(mqttTopics.at(i)->topic, topic)){
      topicFound = true;
      for (int j=0; j<mqttTopics.at(i)->topicList->size(); j++){
        if(mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback){
          cbFound = true;
          mqttTopics.at(i)->topicList->clear(j);
          Log.notice("MQTT-Unsubscribe, Removed callback for %s" CR, topic);
          if (mqttTopics.at(i)->topicList->size() == 0){
            delete mqttTopics.at(i)->topicList;
            delete mqttTopics.at(i)->topic;
            mqttTopics.clear(i);
            if (!mqttClient.unsubscribe(topic)){
              Log.error("mqtt::unSubscribeTopic: could not unsubscribe Topic: %s from broker" CR, topic);
              logSubscribers();
              delete topic;
              xSemaphoreGive(mqttLock);
              return RC_GEN_ERR;
            }
            Log.notice("mqtt::unSubscribeTopic: Last callback for %s unsubscribed - unsubscribed Topic from broker" CR, topic);
            logSubscribers();
            delete topic;
            xSemaphoreGive(mqttLock);
            return RC_OK;
          }
        }
      }
    }
  }
  if(!topicFound) {
    Log.error("mqtt::unSubscribeTopic: Topic %s not found" CR, topic);
    logSubscribers();
    delete topic;
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;
  }
  if(!cbFound) {
    Log.error("MQTT-Unsubscribe, callback not found while unsubscribing topic %s" CR, topic);
    logSubscribers();
    xSemaphoreGive(mqttLock);
    return 1;
  }
  logSubscribers();
  delete topic;
  xSemaphoreGive(mqttLock);
  return RC_GEN_ERR;
}

void mqtt::logSubscribers(void){
  // if loglevel < verbose - return
  Log.verbose("mqtt::logSubscribers: ---CURRENT SUBSCRIBERS---" CR);
  for (int i=0; i < mqttTopics.size(); i++){
    Log.verbose("mqtt::logSubscribers: Topic: %s" CR, mqttTopics.at(i)->topic);
    for (int j=0; j < mqttTopics.at(i)->topicList->size(); j++){
      Log.verbose("mqtt::logSubscribers:    Callback: 0x%x" CR, (int)mqttTopics.at(i)->topicList->at(j)->mqttSubCallback);
    }
  }
  Log.verbose("mqtt::logSubscribers: ---END SUBSCRIBERS---" CR);
  return;
}

uint8_t mqtt::up(void){
  char* mqttPingTopic;
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(mqttLock);
  if (tmpOpState != OP_WORKING) {
    Log.notice("MQTT-up, Reconeccting ...");
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    opState = OP_CONFIG;              // While triger poll() to re-establish the connection
    xSemaphoreGive(mqttLock);
    while(true){
      xSemaphoreTake(mqttLock, portMAX_DELAY);
      if (opState == OP_WORKING){
        xSemaphoreGive(mqttLock);
        break;
      }
      xSemaphoreGive(mqttLock);
      vTaskDelay(100 / portTICK_PERIOD_MS);      
    }
    reSubscribe();
    mqttPingTimer_args.arg = 0;
    mqttPingTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&mqtt::mqttPing);
    mqttPingTimer_args.dispatch_method = ESP_TIMER_TASK;
    mqttPingTimer_args.name = "mqttPingTimerTimer";
    esp_timer_create(&mqttPingTimer_args, &mqttPingTimer);
  }
  Log.notice("mqtt::up: Client is up and working, sending up-message and starting MQTT ping" CR);
  sendMsg(upTopic, upPayload, true);
  if(uri == NULL){
    Log.fatal("mqtt::up: The URI for this decodet has not been defined, cannot bring up MQTT, rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  int len1 = strlen(MQTT_PING_UPSTREAM_TOPIC);
  int len2 = strlen(uri);
  mqttPingTopic = new char [len1 + len2 - 1];
  memcpy(mqttPingTopic, MQTT_PING_UPSTREAM_TOPIC, len1-1);
  memcpy(mqttPingTopic + len1, uri, len2-1);
  mqttPingTopic[len1+len2-2] = '\0';
  esp_timer_start_periodic(mqttPingTimer, pingPeriod*1000000);
  return RC_OK;
}

uint8_t mqtt::down(void){
  Log.notice("mqtt::down, Disconnecting Mqtt client" CR);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  opState = OP_DISABLE;
  xSemaphoreGive(mqttLock);
  esp_timer_stop(mqttPingTimer);
  sendMsg(lwTopic, lwPayload, true);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  mqttClient.disconnect();
  return RC_OK;
}

void mqtt::onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length) {
  Log.verbose("mqtt::onMqttMsg, Received an MQTT mesage, topic %s, payload: %s, length: %d" CR, p_topic, p_payload, p_length);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  bool subFound = false;
  char* payload = new char[p_length +1];
  memcpy(payload, p_payload, p_length);
  payload[p_length] = '\0';
  for (int i=0; i < mqttTopics.size(); i++){
    if (!strcmp(mqttTopics.at(i)->topic, p_topic)) {
      for (int j=0; j<mqttTopics.at(i)->topicList->size(); j++){
        subFound = true;
        xSemaphoreGive(mqttLock);
        mqttTopics.at(i)->topicList->at(j)->mqttSubCallback(p_topic, payload, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs);
        xSemaphoreTake(mqttLock, portMAX_DELAY);
      }
    } 
  }
  if(!subFound) {
    Log.error("mqtt::onMqttMsg, could not find any subscription for received message topic: %s" CR, p_topic);
    delete payload;
    xSemaphoreGive(mqttLock);
    return;
  }
  delete payload;
  xSemaphoreGive(mqttLock);
  return;
}

uint8_t mqtt::sendMsg(const char* p_topic, const char* p_payload, bool p_retain){
  if (!mqttClient.publish(p_topic, p_payload, p_retain)){
    Log.error("mqtt::sendMsg: could not send message, topic: %s, payload: %s" CR, p_topic, p_payload);
    return RC_GEN_ERR;
  } else {
    Log.verbose("mqtt::sendMsg: sent a message, topic: %s, payload: %s" CR, p_topic, p_payload);    
  }
  return RC_OK;
}

void mqtt::poll(void* dummy){
  uint8_t retryCnt = 0;
  int stat;
  uint8_t tmpOpState;

  //esp_task_wdt_init(1, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  while(true){
    //esp_task_wdt_reset();
    mqttClient.loop();
    stat = mqttClient.state();
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    tmpOpState = opState;
    xSemaphoreGive(mqttLock);
    switch (stat) {
      case MQTT_CONNECTED:
        xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_WORKING;
        xSemaphoreGive(mqttLock);
        if(mqttStatus != stat){
          Log.notice("mqtt::poll, MQTT connection established - opState set to OP_WORKING" CR);
        }
        retryCnt = 0;
        break;
      case MQTT_CONNECTION_TIMEOUT:
      case MQTT_CONNECTION_LOST:
      case MQTT_CONNECT_FAILED:
      case MQTT_DISCONNECTED:
        if (tmpOpState == OP_DISABLE){
          break;
        }
        xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_FAIL;
        xSemaphoreGive(mqttLock);
        if (retryCnt >= MAX_MQTT_CONNECT_ATTEMPTS_100MS){
          Log.fatal("mqtt::poll, Max number of MQTT connect/reconnect attempts reached, cause: %d - rebooting..." CR, stat);
          opState = OP_FAIL;
          //failsafe();
          ESP.restart();
          return;
        }
        mqttClient.connect(clientId, 
                           user, 
                           pass, 
                           lwTopic, 
                           QOS_1, 
                           true, 
                           lwPayload,
                           true);       //Clean session
        if(mqttStatus != stat){
          Log.error("mqtt::poll, MQTT connection not established or lost - opState set to OP_FAIL, cause: %d - retrying..." CR, stat);
        }
        retryCnt++;
        break;
      case MQTT_CONNECT_BAD_PROTOCOL:
      case MQTT_CONNECT_BAD_CLIENT_ID:
      case MQTT_CONNECT_UNAVAILABLE:
      case MQTT_CONNECT_BAD_CREDENTIALS:
      case MQTT_CONNECT_UNAUTHORIZED:
        xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_FAIL;
        xSemaphoreGive(mqttLock);
        Log.fatal("mqtt::poll, Fatal MQTT error, one of BAD_PROTOCOL, BAD_CLIENT_ID, UNAVAILABLE, BAD_CREDETIALS, UNOTHORIZED, cause: %d - rebooting..." CR, stat);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return;
    }
    if (mqttStatus != stat){
      mqttStatus = stat;
      if (statusCallback != NULL){
        statusCallback(mqttStatus);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  return;
}

void mqtt::mqttPing(void){
  if(opState == OP_WORKING && pingPeriod != 0) {
    sendMsg(mqttPingTopic, PING, false);
  }
  return;
}

uint8_t mqtt::getOpState(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(mqttLock);
  return tmpOpState;
}

char* mqtt::getUri(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  char* tmpUri = uri;
  xSemaphoreGive(mqttLock);
  return tmpUri;
}

/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: topDecoder                                                                                                                            */
/* Purpose: The "topDecoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,        */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroups [Signal Masts | general Lights | sequencedLights],       */
/*          Turnouts or sensors...                                                                                                              */
/*          The "topDecoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t topDecoder::opState = OP_CREATED;


uint8_t topDecoder::init(void){
  if(opState != OP_CREATED){
    Log.fatal("topDecoder::init: opState is not OP_CREATED - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  Log.notice("topDecoder::init: Initializing topDecoder" CR);
  topDecoderLock = xSemaphoreCreateMutex();
  Log.notice("topDecoder::init: Initializing MQTT " CR);
  MQTT.init( (char*)"test.mosquitto.org",  //Broker URI WE NEED TO GET THE BROKER FROM SOMEWHERE
              1883,                         //Broker port
              (char*)"",                    //User name
              (char*)"",                    //Password
              (char*)MQTT_CLIENT_ID,        //Client ID
              QOS_1,                        //QoS
              MQTT_KEEP_ALIVE,               //Keep alive time
              true,                         //Default retain
              (char*)MQTT_OPSTATE_TOPIC,    //LW Topic
              (char*)DECODER_DOWN,          //LW Payload
              (char*)MQTT_OPSTATE_TOPIC,    //Up topic
              (char*)DECODER_UP);           //Up payload
  int i = 0;
  while (MQTT.getOpState() != OP_WORKING) {
    if(i++ >= 120){
      Log.fatal("topDecoder::init: Could not connect to MQTT broker - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Serial.println("");
  Log.notice("topDecoder::init: Starting discovery process");
  i = 0;
  while(MQTT.getUri() == NULL) {
    if(i++ >= 120){
      Log.fatal("topDecoder::init: Discovery process failed - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Serial.println("");
  Log.notice("topdecoder-init: Subscribing to decoder configuration topic");
  const char* subscribeTopic[2] = {MQTT_CONFIG_TOPIC, MQTT.getUri()};
  if (MQTT.subscribeTopic(concatStr(subscribeTopic, 2), on_configUpdate, NULL)){
    Log.fatal("topDecoder::init: Failed to suscribe to configuration topic - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  opState = OP_INIT;
  return RC_OK;
}

void topDecoder::on_configUpdate(const char* p_topic, const char* p_payload, const void* p_dummy){
  if(opState != OP_INIT){
    Log.fatal("topdecoder-on_configUpdate: Received a configuration, while the topdecoder already had an earlier configuration, dynamic re-configuration not supported - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  Log.notice("topDecoder::on_configUpdate: Received an uverified configuration, parsing and validating it..." CR);
  xmlconfig = new char*[9];
  xmlconfig[XMLAUTHOR] = NULL;
  xmlconfig[XMLDESCRIPTION] = NULL;
  xmlconfig[XMLVERSION] = NULL;
  xmlconfig[XMLDATE] = NULL;
  const char* default_rsyslogreceiver = DEFAULT_RSYSLOGRECEIVER;
  xmlconfig[XMLRSYSLOGRECEIVER] = (char*)default_rsyslogreceiver;
  itoa(DEFAULT_LOGLEVEL, xmlconfig[XMLLOGLEVEL], 10);
  const char* default_ntpserver = DEFAULT_NTPSERVER;
  xmlconfig[XMLNTPSERVER] = (char*)default_ntpserver;
  itoa(DEFAULT_TIMEZONE, xmlconfig[XMLTIMEZONE], 10);
  itoa(DEFAULT_PINGPERIOD, xmlconfig[XMLPINGPERIOD], 10);
  mac = NULL;
  xmlVersion = NULL;
  xmlDate = NULL;
  ntpServer = NULL;
  timeZone = 0;
  xmlConfigDoc = new tinyxml2::XMLDocument;

  //Reset XMLDocument object???
  if (xmlConfigDoc->Parse(p_payload)) {
    Log.fatal("topDecoder::on_configUpdate: Configuration parsing failed - Rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  if (xmlConfigDoc->FirstChildElement("Decoder") == NULL || xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top") == NULL || xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top")->FirstChildElement() == NULL ) {
    Log.fatal("topDecoder::on_configUpdate: Failed to parse the configuration - xml is missformatted - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  getTagTxt(xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top")->FirstChildElement(), searchTags, xmlconfig, sizeof(searchTags)/4-1); // Need to fix the addressing for portability
  Log.notice("topDecoder::on_configUpdate: Successfully parsed the topdecoder configuration:" CR);
  if(xmlconfig[XMLAUTHOR] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Author: %s" CR, xmlconfig[XMLAUTHOR]);
  } else {
    Log.notice("topDecoder::on_configUpdate: XML Author not provided - skipping..." CR);
  }
  if(xmlconfig[XMLDATE] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Date: %s" CR, xmlconfig[XMLVERSION]);
  }
  else {
    Log.notice("topDecoder::on_configUpdate: XML Date not provided - skipping..." CR);
  }
  if(xmlconfig[XMLVERSION] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Version: %s" CR, xmlconfig[XMLDATE]);
  } else {
    Log.notice("topDecoder::on_configUpdate: XML Version not provided - skipping..." CR);
  }
  if(xmlconfig[XMLRSYSLOGRECEIVER] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Rsyslog receiver: %s - NOT IMPLEMENTED" CR, xmlconfig[XMLRSYSLOGRECEIVER]);
  }
  if(xmlconfig[XMLLOGLEVEL] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Log-level: %s" CR, xmlconfig[XMLLOGLEVEL]);
    switch (atoi(xmlconfig[XMLLOGLEVEL])){
      case DEBUG_VERBOSE:
        Log.setLevel(LOG_LEVEL_VERBOSE);
        break;
      case DEBUG_TERSE:
        Log.setLevel(LOG_LEVEL_TRACE);
        break;
      case INFO:
        Log.setLevel(LOG_LEVEL_NOTICE);
        break;
      case ERROR:
        Log.setLevel(LOG_LEVEL_ERROR);
        break;
      case PANIC:
        Log.setLevel(LOG_LEVEL_FATAL);
        break;
      default:
        Log.error("topDecoder::on_configUpdate: %s is not a valid log-level, will keep-on to the default log level %d" CR, xmlconfig[XMLLOGLEVEL], INFO);
        itoa(INFO, xmlconfig[XMLLOGLEVEL], 10);
    }
  } else {
    Log.notice("topDecoder::on_configUpdate: Loglevel not provided - using default log-level (Notice)" CR);
    Log.setLevel(LOG_LEVEL_NOTICE);
    itoa(INFO, xmlconfig[XMLLOGLEVEL], 10);
  }
  if(xmlconfig[XMLNTPSERVER] != NULL) {
    Log.notice("topDecoder::on_configUpdate: NTP-server: %s" CR, xmlconfig[XMLNTPSERVER]);
  } else {
    Log.notice("topDecoder::on_configUpdate: NTP server not provided - skipping..." CR);
  }
  if(xmlconfig[XMLTIMEZONE] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Time zone: %s" CR, xmlconfig[XMLTIMEZONE]);
  } else {
    Log.notice("topDecoder::on_configUpdate: Timezone not provided - using UTC..." CR);
    itoa(0, xmlconfig[XMLTIMEZONE], 10);
  }
  if(xmlconfig[XMLPINGPERIOD] != NULL) {
    Log.notice("topDecoder::on_configUpdate: MQTT Ping period: %s" CR, xmlconfig[XMLPINGPERIOD]);
    MQTT.setPingPeriod(strtof(xmlconfig[XMLPINGPERIOD], NULL)); //Should be moved to a supervision class
  } else {
    Log.notice("topDecoder::on_configUpdate: Ping-period not provided - using default %f" CR, DEFAULT_MQTT_PINGPERIOD); 
    MQTT.setPingPeriod(DEFAULT_MQTT_PINGPERIOD); //Should be moved to a supervision class
    xmlconfig[XMLPINGPERIOD] = new char[10];
    dtostrf(DEFAULT_MQTT_PINGPERIOD, 4, 3,xmlconfig[XMLPINGPERIOD]);
  }
  tinyxml2::XMLElement* lgXmlElement;
  if ((lgXmlElement = xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Lightgroups")) != NULL){
    lighGroupsDecoder_0 = new lgsDecoder(); //Should eventually support up to 4 channels
    lighGroupsDecoder_0->init(0);
    lighGroupsDecoder_0->on_configUpdate(lgXmlElement);
  }
  //Calls to other decoder types are placed here
  opState = OP_CONFIG;
  return;
}

uint8_t topDecoder::start(void){
  MQTT.up();
  if(lighGroupsDecoder_0 != NULL){
    if(lighGroupsDecoder_0->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 0 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_1 != NULL){
    if(lighGroupsDecoder_1->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 1 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_2 != NULL){
    if(lighGroupsDecoder_2->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 2 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_3 != NULL){
    if(lighGroupsDecoder_3->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 3 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(topDecoderLock);
}

char* topDecoder::getXmlAuthor(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlAuthor = xmlconfig[XMLAUTHOR];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlAuthor;
}

char* topDecoder::getXmlDate(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlDate = xmlconfig[XMLDATE];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlDate;
}

char* topDecoder::getXmlVersion(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlVersion = xmlconfig[XMLVERSION];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlVersion;
}

char* topDecoder::getRsyslogReceiver(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpRsyslogReceiver = xmlconfig[XMLRSYSLOGRECEIVER];
  xSemaphoreGive(topDecoderLock);
  return tmpRsyslogReceiver;
}

uint8_t topDecoder::getLogLevel(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpLoglevel = atoi(xmlconfig[XMLLOGLEVEL]);
  xSemaphoreGive(topDecoderLock);
  return tmpLoglevel;
}

char* topDecoder::getNtpServer(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpNtpServer = xmlconfig[XMLNTPSERVER];
  xSemaphoreGive(topDecoderLock);
  return tmpNtpServer;
}

uint8_t topDecoder::getTimezone(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpTimezone = atoi(xmlconfig[XMLTIMEZONE]);
  xSemaphoreGive(topDecoderLock);
  return tmpTimezone;
}

float topDecoder::getMqttPingperiod(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  float tmpXmlPingPeriod = atof(xmlconfig[XMLPINGPERIOD]);
  xSemaphoreGive(topDecoderLock);
  return tmpXmlPingPeriod;
}

uint8_t topDecoder::getopState(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(topDecoderLock);
  return tmpOpState;
}

/*==============================================================================================================================================*/
/* END Class topDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgsDecoder(lightgroupsDecoder)"                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
lgsDecoder::lgsDecoder(void){
  Log.notice("lgsDecoder::lgDecoder: Creating Lightgroups decoder channel" CR);
  opState = OP_CREATED;
}

lgsDecoder::~lgsDecoder(void){
  Log.fatal("lgsDecoder::~lgDecoder: Destruction not supported - rebooting..." CR);
  opState = OP_FAIL;
  //failsafe();
  ESP.restart();
  return;
}

uint8_t lgsDecoder::init(const uint8_t p_channel){
  if(opState != OP_CREATED) {
    Log.notice("lgsDecoder::init: opState is not OP_CREATED - Rebooting..." CR);
  }
  Log.notice("lgsDecoder::init: Initializing Lightgroups decoder channel %d" CR, p_channel);
  channel = p_channel;
  updateStripReentranceLock = xSemaphoreCreateMutex();
  lgsDecoderLock = xSemaphoreCreateMutex();
  switch(channel){
    case 0 :
      pin = LEDSTRIP_CH0_PIN;
      break;
    case 1 :
      pin = LEDSTRIP_CH1_PIN;
      break;
    case 2 :
      pin = LEDSTRIP_CH2_PIN;
      break;
    case 3 :
      pin = LEDSTRIP_CH3_PIN;
      break;
    default:
      Log.fatal("lgsDecoder::init: Lightgroup channel %d not supported - rebooting..." CR, channel);
      opState = OP_FAIL;
      //failSafe();
      ESP.restart();
      return RC_NOTIMPLEMENTED_ERR;
  }
/* In case we want to put SMASPECTS on the heap instead
 if(SMASPECTS == NULL){
    Log.fatal("lgsDecoder::init: Could not create an smAspect object - rebooting..." CR);
    //failSafe();
    opState = OP_FAILED;
    ESP.restat();
    return RC_OUT_OF_MEM_ERR;
  }
  */
  FLASHNORMAL = new flash((float)SM_FLASH_TIME_NORMAL, (uint8_t)50); // Move to top_decoder?
  FLASHSLOW = new flash(SM_FLASH_TIME_SLOW, 50); // Move to top_decoder?
  FLASHFAST = new flash(SM_FLASH_TIME_FAST, 50); // Move to top_decoder?
  if(FLASHNORMAL == NULL || FLASHSLOW == NULL || FLASHFAST == NULL){
    Log.fatal("lgsDecoder::init: Could not create flash objectst - rebooting..." CR);
    opState = OP_FAIL;
    //failSafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  //stripLed_t stripCtrlBuff[MAXSTRIPLEN];
  stripCtrlBuff = new stripLed_t[MAXSTRIPLEN];
  strip = new Adafruit_NeoPixel(MAXSTRIPLEN, pin, NEO_RGB + NEO_KHZ800);
  if(strip == NULL){
    Log.fatal("lgsDecoder::init: Could not create NeoPixel object - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  strip->begin();
  stripWritebuff = strip->getPixels();
  //failSafe();
  esp_timer_create_args_t stripUpdateTimer_args;
  stripUpdateTimer_args.arg=this;
  stripUpdateTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&lgsDecoder::updateStripHelper);
  stripUpdateTimer_args.dispatch_method = ESP_TIMER_TASK;
  stripUpdateTimer_args.name = "StripUpdateTimer";
  esp_timer_create(&stripUpdateTimer_args, &stripUpdateTimerHandle);
  if(stripUpdateTimerHandle == NULL){
    Log.fatal("lgsDecoder::init: Could not create stripUpdateTimer object - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  opState = OP_INIT;
  return RC_OK;
}

uint8_t lgsDecoder::on_configUpdate(tinyxml2::XMLElement* p_lightgroupsXmlElement){
  if(opState != OP_INIT){
    Log.fatal("lgsDecoder::on_configUpdate: opState is not OP_INIT - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  if(SMASPECTS.onConfigUpdate(p_lightgroupsXmlElement->FirstChildElement("SignalMastDesc"))){ // We should move this outside the lightgroups scope
    Log.error("lgsDecoder::on_configUpdate: Could not configure SMASPECTS - continuing..." CR);
  }
  lightGroupXmlElement = p_lightgroupsXmlElement->FirstChildElement("Lightgroup");
  const char* lgSearchTags[5];
  lgSearchTags[XMLLGADDR] = "LgAddr";
  lgSearchTags[XMLLGSEQ] = "LgSeq";
  lgSearchTags[XMLLGSYSTEMNAME] = "LgSystemName";
  lgSearchTags[XMLLGUSERNAME] = "LgUserName";
  lgSearchTags[XMLLGTYPE] = "LgType";
  char* lgConfigTxtBuff[5];
  while(lightGroupXmlElement != NULL){
    if(getTagTxt(lightGroupXmlElement, lgSearchTags, lgConfigTxtBuff, 5)){
      Log.fatal("lgsDecoder::on_configUpdate: No lightGroupXml provided - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
    }
    if(lgConfigTxtBuff[XMLLGADDR] == NULL || 
       lgConfigTxtBuff[XMLLGSEQ] == NULL ||
       lgConfigTxtBuff[XMLLGSYSTEMNAME] == NULL ||
       lgConfigTxtBuff[XMLLGUSERNAME] == NULL ||
       lgConfigTxtBuff[XMLLGTYPE] == NULL) {
        Log.fatal("lgsDecoder::on_configUpdate: Failed to parse lightGroupXml - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
    }
    lightGroup_t* lg = new lightGroup_t;
    lg->lightGroupsChannel = this;
    lg->lgAddr = atoi(lgConfigTxtBuff[XMLLGADDR]);
    lg->lgSeq = atoi(lgConfigTxtBuff[XMLLGSEQ]);
    lg->lgSystemName = lgConfigTxtBuff[XMLLGSYSTEMNAME];
    lg->lgUserName = lgConfigTxtBuff[XMLLGUSERNAME];
    lg->lgType = lgConfigTxtBuff[XMLLGTYPE];
    if(!strcmp(lg->lgType, "Signal Mast")){
      if(lightGroupXmlElement->FirstChildElement("LgDesc")==NULL){
        Log.fatal("lgsDecoder::on_configUpdate: LgDesc missing - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
      }
      if((lg->lightGroupObj = new mastDecoder()) == NULL){
        Log.fatal("lgsDecoder::on_configUpdate: Could not create mastDecoder object - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_OUT_OF_MEM_ERR;
      }
      if(lg->lightGroupObj->init()){
        Log.error("lgsDecoder::on_configUpdate: Could not initialize mastDecoder object - continuing..." CR);
      }
      if(lg->lightGroupObj->onConfigure(lg, lightGroupXmlElement->FirstChildElement("LgDesc"))){
        Log.error("lgsDecoder::on_configUpdate: Could not configure mastDecoder object - continuing..." CR);
      }
    }
    // else if OTHER LG TYPES....
    else {
      Log.error("lgsDecoder::on_configUpdate: LG Type %s does is not implemented - continuing..." CR);
    }
    lgList.push_back(lg);
    lightGroupXmlElement = lightGroupXmlElement->NextSiblingElement("Lightgroup");
  }
  opState = OP_CONFIG;
  return RC_OK;
}

uint8_t lgsDecoder::start(void){
  if(opState != OP_CONFIG){
    Log.fatal("lgsDecoder::start: opState is not OP_CONFIG - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  int prevSeq = -1;
  int foundSeq;
  int seqOffset = 0;
  for(int i=0; i<lgList.size(); i++){
    int nextSeq = MAXSTRIPLEN;
    int j = 0;
    for(j=0; j<lgList.size(); j++){
      if(lgList.get(j)->lgSeq <= prevSeq){
        continue;
      }
      if(lgList.get(j)->lgSeq < nextSeq){
        nextSeq = lgList.get(j)->lgSeq;
        foundSeq = j;
      }
    }
    prevSeq = nextSeq;
    lgList.get(j)->lgSeqOffset = seqOffset;
    seqOffset += lgList.get(j)->lgNoOfLed;
  }
  for(int i=0; i<lgList.size(); i++){
    lgList.get(i)->lightGroupObj->start();
  }
  esp_timer_start_periodic(stripUpdateTimerHandle, STRIP_UPDATE_MS*1000);
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(lgsDecoderLock);
  return RC_OK;
}

uint8_t lgsDecoder::updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, const uint8_t* p_wantedValueBuff, const uint16_t* p_transitionTimeBuff){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  for(int i=0; i<p_buffLen; i++){
    stripCtrlBuff[i+p_seqOffset].wantedValue = p_wantedValueBuff[i];
    stripCtrlBuff[i+p_seqOffset].incrementValue = floor(p_transitionTimeBuff[i]/STRIP_UPDATE_MS);
    stripCtrlBuff[i+p_seqOffset].dirty = true;
    bool alreadyDirty = false;
    for(int i=0; i<lgList.size(); i++){
      if(dirtyList.get(i) == &stripCtrlBuff[i+p_seqOffset]){
        alreadyDirty = true;
        break;
      }
    }
    if(!alreadyDirty){
      dirtyList.push_back(&stripCtrlBuff[i+p_seqOffset]);
    }
  }
  xSemaphoreGive(lgsDecoderLock);
  return RC_OK;
}

void lgsDecoder::updateStripHelper(lgsDecoder* p_lgsObject){
  p_lgsObject->updateStrip();
  return;
}

void lgsDecoder::updateStrip(void){
  if(xSemaphoreTake(updateStripReentranceLock, 0) == pdFALSE){
    overRuns++;
    return;
  }
  if(!strip->canShow()){
    xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
    overRuns++;
    xSemaphoreGive(lgsDecoderLock);
    xSemaphoreGive(updateStripReentranceLock);
    return;
  }
  int currentValue;
  int wantedValue;
  int incrementValue;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  for(int i=0; i<dirtyList.size(); i++){
    currentValue = (int)dirtyList.get(i)->currentValue;
    wantedValue = (int)dirtyList.get(i)->wantedValue;
    incrementValue = (int)dirtyList.get(i)->incrementValue;
    if(wantedValue > currentValue){
      currentValue += dirtyList.get(i)->incrementValue;
      if(currentValue > wantedValue){
        currentValue = wantedValue;
      }
    } else{
      currentValue -= dirtyList.get(i)->incrementValue;
      if(currentValue < wantedValue){
          currentValue = wantedValue;
      }
    }
    stripWritebuff[dirtyList.get(i)-stripCtrlBuff] = currentValue;
    dirtyList.get(i)->currentValue = (uint8_t)currentValue;
    if(wantedValue == currentValue){
      dirtyList.get(i)->dirty = false;
      dirtyList.clear(i);
    }
  }
  xSemaphoreGive(lgsDecoderLock);
  strip->show();
  xSemaphoreGive(updateStripReentranceLock);
}

unsigned long lgsDecoder::getOverRuns(void){
  unsigned long tmpOverRuns;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  tmpOverRuns = overRuns;
  xSemaphoreGive(lgsDecoderLock);
  return tmpOverRuns;
}

void lgsDecoder::clearOverRuns(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  overRuns = 0;
  xSemaphoreGive(lgsDecoderLock);
  return;
}

flash* lgsDecoder::getFlashObj(uint8_t p_flashType){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  if(opState == OP_CREATED || opState == OP_INIT || opState == OP_FAIL) {
    Log.error("lgsDecoder::getFlashObj: opState %d does not allow to provide flash objects - returning NULL - and continuing..." CR, opState);
    xSemaphoreGive(lgsDecoderLock);
    return NULL;
  }
  xSemaphoreGive(lgsDecoderLock);
  switch(p_flashType){
    case SM_FLASH_TYPE_SLOW:
    return FLASHFAST;
    break;
    
    case SM_FLASH_TYPE_NORMAL:
    return FLASHNORMAL;
    break;

    case SM_FLASH_TYPE_FAST:
    return FLASHFAST;
    break;
  }
}

uint8_t lgsDecoder::getOpState(void){
  uint8_t tmpOpState;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  tmpOpState = opState;
  xSemaphoreGive(lgsDecoderLock);
  return tmpOpState;
}

/*==============================================================================================================================================*/
/* END Class lgsDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
flash::flash(float p_freq, uint8_t p_duty){
  flashData = new flash_t;
  if(p_duty > 99){
    p_duty = 99;
  }
  flashData->onTime = (1/p_freq)*(p_duty/100)*1000000;
  flashData->offTime = (1/p_freq)*((100-p_duty)/100)*1000000;
  flashData->flashState = true;
  flashReentranceLock = xSemaphoreCreateMutex();
  flashTimer_args.arg = this;
  flashTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&flash::flashTimeoutHelper);
  flashTimer_args.dispatch_method = ESP_TIMER_TASK;
  flashTimer_args.name = "FlashTimer";
  esp_timer_create(&flashTimer_args, &flashData->timerHandle);
  esp_timer_start_once(flashData->timerHandle, flashData->onTime);
  overRuns = 0;
  opState = OP_WORKING;
}
  
flash::~flash(void){
  esp_timer_stop(flashData->timerHandle);
  esp_timer_delete(flashData->timerHandle);
  opState = OP_FAIL;
  delete flashData;
}

uint8_t flash::subscribe(flashCallback_t p_callback, void* p_args){
  Log.notice("flash::unSubcribe: Subscribing to flash object %d with callback %d" CR, this, p_callback);
  callbackSub_t* newCallbackSub = new callbackSub_t;
  flashData->callbackSubs.push_back(newCallbackSub);
  flashData->callbackSubs.back()->callback = p_callback;
  flashData->callbackSubs.back()->callbackArgs = p_args;
  return RC_OK;
}

uint8_t flash::unSubscribe(flashCallback_t p_callback){
  Log.verbose("flash::unSubcribe: Unsubscribing flash callback %d from flash object %d" CR, p_callback, this);
  uint16_t i = 0;
  for(i=0; true; i++){
    if(i >= flashData->callbackSubs.size()){
      break;
    }
    if(flashData->callbackSubs.get(i)->callback == p_callback)
      delete flashData->callbackSubs.get(i);
      flashData->callbackSubs.clear(i);
  }
  return RC_OK;
}

void flash::flashTimeoutHelper(flash* p_flashObject){
  p_flashObject->flashTimeout();
  return;
}

void flash::flashTimeout(void){
  if(xSemaphoreTake(flashReentranceLock, 0) == pdFALSE){
    overRuns++;
    xSemaphoreTake(flashReentranceLock, portMAX_DELAY);
  }
  if(flashData->flashState){
      esp_timer_start_once(flashData->timerHandle, flashData->offTime);
      flashData->flashState = false;
  }
  else{
    esp_timer_start_once(flashData->timerHandle, flashData->onTime);
    flashData->flashState = true;
  }
  for(uint16_t i=0; i<flashData->callbackSubs.size(); i++){
    flashData->callbackSubs.get(i)->callback(flashData->flashState, flashData->callbackSubs.get(i)->callbackArgs);
  }
  return;
}

int flash::getOverRuns(void){
  return overRuns;
}

void flash::clearOverRuns(void){
  overRuns = 0;
  return;
}

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
/*
# XML schema fragment:
#           <Aspects>
#        <Aspect>
#         <AspectName>Stopp</AspectName>
#         <Mast>
#           <Type>Sweden-3HMS:SL-5HL></Type>
#           <Head>UNLIT</Head>
#           <Head>LIT</Head>
#           <Head>UNLIT</Head>
#           <Head>UNLIT</Head>
#           <Head>UNLIT</Head>
#           <NoofPxl>6</NoofPxl>
#         </Mast>
#         <Mast>
#                    .
#                    .
#         </Mast>
#       </Aspect>
#           </Aspects>
*/

uint8_t signalMastAspects::opState = OP_INIT;

uint8_t signalMastAspects::onConfigUpdate(tinyxml2::XMLElement* p_smAspectsXmlElement){
  if(opState != OP_INIT){
    Log.fatal("signalMastAspects::onConfigure: opState is not OP_INIT - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  Log.notice("signalMastAspects::onConfigure: Configuring Mast aspects" CR);
  tinyxml2::XMLElement* smAspectsXmlElement = p_smAspectsXmlElement;
  if((smAspectsXmlElement = smAspectsXmlElement->FirstChildElement("Aspects")) == NULL || (smAspectsXmlElement = smAspectsXmlElement->FirstChildElement("Aspect"))==NULL || smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL){
    Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Aspects, Aspect, or AspectName - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_PARSE_ERR;
  }
  //Outer loop itterating all signal mast aspects
  for(uint8_t i=0; true; i++){
    Log.notice("signalMastAspects::onConfigure: Parsing Signal mast Aspect: %s" CR, smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
    char* newAspect = new char[sizeof(smAspectsXmlElement->FirstChildElement("Aspects")->GetText())];
    strcpy(newAspect, smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
    aspects.push_back(newAspect);
    
    //Mid loop itterating the XML mast types for the aspect
    tinyxml2::XMLElement* mastTypeAspectXmlElement = smAspectsXmlElement->FirstChildElement("Mast");
    if( mastTypeAspectXmlElement == NULL || mastTypeAspectXmlElement->FirstChildElement("Type") == NULL || mastTypeAspectXmlElement->FirstChildElement("Type")->GetText() == NULL){
      Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Mast or Type - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
    }
    for(uint16_t j=0; true; j++){
      if(mastTypeAspectXmlElement == NULL){
        break;
      }
      Log.notice("signalMastAspects::onConfigure: Parsing Mast type %s for Aspect %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());

      //Inner loop creating head aspects for a particular mast type
      uint8_t k = 0;
      bool mastTypeFound = false;
      for(k=0; k<mastTypes.size(); k++){
        if(!strcmp(mastTypes.at(k)->name, mastTypeAspectXmlElement->GetText())){
          mastTypeFound = true;
          break;
        }
      }
      if(!mastTypeFound){ //Creating a new signal mast type
        Log.notice("signalMastAspects::onConfigure: Creating Mast type %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
        mastTypes.push_back(new mastType_t);
        char* newMastTypeName = new char[sizeof(mastTypeAspectXmlElement->FirstChildElement("Type")->GetText())];
        strcpy(newMastTypeName, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
        mastTypes.back()->name = newMastTypeName;
        mastTypes.back()->noOfUsedHeads = atoi(mastTypeAspectXmlElement->FirstChildElement("NoofPxl")->GetText());
        mastTypes.back()->noOfHeads = ceil(mastTypes.back()->noOfUsedHeads/3)*3;
        for(uint8_t m=0; m<k; m++){ //Filling in previous head aspects which the mast is not part of
          for(uint8_t n=0; n<SM_MAXHEADS; n++){
            mastTypes.back()->headAspects.push_back(new uint8_t[SM_MAXHEADS]); //Should this really ever happen?
            Log.notice("signalMastAspects::onConfigure: MastType %s was not part of previous aspects, padding with UNUSED_APPEARANCE" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
            for(uint8_t r=0; r < SM_MAXHEADS; r++){
              mastTypes.back()->headAspects.back()[r] = UNUSED_APPEARANCE;
            }
          }
        }
      }
      tinyxml2::XMLElement* headXmlElement = mastTypeAspectXmlElement->FirstChildElement("Head");
      if(headXmlElement == NULL || headXmlElement->GetText() == NULL){
        Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Head - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
      }
      Log.notice("signalMastAspects::onConfigure: Adding Asspect % s to MastType %s" CR, smAspectsXmlElement->FirstChildElement("Aspects")->GetText(), mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
      mastTypes.back()->headAspects.push_back(new uint8_t[SM_MAXHEADS]);
      for(uint8_t p=0; p<SM_MAXHEADS; p++){
        if(headXmlElement == NULL){
          Log.notice("signalMastAspects::onConfigure: No more Head appearances, padding up with UNUSED_APPEARANCE from Head %d" CR, p);
          for(uint8_t r=p; r < SM_MAXHEADS; r++){
            mastTypes.back()->headAspects.back()[r] = UNUSED_APPEARANCE;
          }
          break;
        }
        if(!strcmp(headXmlElement->GetText(), "LIT")){
          Log.notice("signalMastAspects::onConfigure: Adding LIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
          mastTypes.back()->headAspects.back()[p] = LIT_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "UNLIT")){
          Log.notice("signalMastAspects::onConfigure: Adding UNLIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
          mastTypes.back()->headAspects.back()[p] = UNLIT_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "FLASH")){
          Log.notice("signalMastAspects::onConfigure: Adding FLASH_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
          mastTypes.back()->headAspects.back()[p] = FLASH_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "UNUSED")){
          Log.notice("signalMastAspects::onConfigure: Adding UNUSED_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());
          mastTypes.back()->headAspects.back()[p] = UNUSED_APPEARANCE;
        }
        headXmlElement = headXmlElement->NextSiblingElement("Head");
      }
      //End inner loop
      mastTypeAspectXmlElement = mastTypeAspectXmlElement->NextSiblingElement("Mast");
    }
    //End middle loop
    for(uint16_t q=0; q<mastTypes.size(); q++){ //Check that all mast types have i+1 aspects, otherwise pad them
      if(mastTypes.at(q)->headAspects.size() != i+1){
        Log.notice("signalMastAspects::onConfigure: MastType %s was not part of Appearance %s, padding it with UNUSED_APPEARANCE" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("Aspects")->GetText());

        mastTypes.at(q)->headAspects.push_back(new uint8_t[SM_MAXHEADS]);
        for(uint8_t r=0; r < SM_MAXHEADS; r++){
          mastTypes.at(q)->headAspects.back()[r] = UNUSED_APPEARANCE;
        }
      }
    }
    if((smAspectsXmlElement = smAspectsXmlElement->NextSiblingElement("Aspect")) == NULL){
      break;
    }
    if(smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL){
      Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing AspectName - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
    }
  }
  opState = OP_WORKING;
  return RC_OK;
}

uint8_t signalMastAspects::getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance){
  uint16_t i=0;
  if(opState != OP_WORKING){
    Log.error("signalMastAspects::getAppearance: OP_State is not OP_WORKING, doing nothing..." CR);
    *p_appearance = NULL;
    return RC_GEN_ERR;
  }
  for(i=0; true; i++){
    if(i>aspects.size()-1){
      Log.error("signalMastAspects::getAppearance: Aspect doesnt exist, doing nothing..." CR);
      *p_appearance = NULL;
      return RC_GEN_ERR;
    }
    if(!strcmp(p_aspect, aspects.at(i))){
      break;
    }
  }
  uint16_t j = 0;
  for(j=0; true; j++){
    if(j>mastTypes.size()-1){
      Log.error("signalMastAspects::getAppearance: Mast type doesnt exist, doing nothing..." CR);
      *p_appearance = NULL;
      return RC_GEN_ERR;
    }
    if(!strcmp(p_smType, mastTypes.at(j)->name)){
      *p_appearance = mastTypes.at(j)->headAspects.at(i);
      return RC_OK;
    }
  }
}

uint8_t signalMastAspects::getNoOfHeads(char* p_smType){
  if(opState != OP_WORKING){
    Log.error("signalMastAspects::getNoOfHeads: OP_State is not OP_WORKING, doing nothing..." CR);
    return RC_GEN_ERR;
  }
  for(uint8_t i=0; i<mastTypes.size(); i++){
    if(!strcmp(mastTypes.get(i)->name, p_smType)){
      return mastTypes.get(i)->noOfHeads;
    }
  }
  Log.error("signalMastAspects::getNoOfHeads: Mast type not found, doing nothing..." CR);
  return RC_GEN_ERR;
}

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mastDecoder                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
mastDecoder::mastDecoder(void){
  Log.notice("mastDecoder::mastDecoder: Creating mast decoder" CR);
  opState = OP_CREATED;
  uint8_t* tmpAppearance = new uint8_t[SM_MAXHEADS];

}

mastDecoder::~mastDecoder(void){
  Log.fatal("mastDecoder::~mastDecoder: Destructor not supported - rebooting..." CR);
  opState = OP_FAIL;
  //failsafe();
  ESP.restart();
}

uint8_t mastDecoder::init(void){
  Log.notice("mastDecoder::init: Initializing mast decoder" CR);
  mastDecoderLock = xSemaphoreCreateMutex();
  mastDecoderReentranceLock = xSemaphoreCreateMutex();
  if(mastDecoderLock == NULL || mastDecoderReentranceLock == NULL){
    Log.fatal("mastDecoder::init: Could not create Lock objects - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  strcpy(aspect, "FailSafe");
  opState = OP_INIT;
}

uint8_t mastDecoder::onConfigure(lightGroup_t* p_genLgDesc, tinyxml2::XMLElement* p_mastDescXmlElement){
  if(opState != OP_INIT) {
    Log.fatal("mastDecoder::onConfigure: Reconfigurarion not supported - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_NOTIMPLEMENTED_ERR;
  }
  if(genLgDesc == NULL){
    Log.fatal("mastDecoder::onConfigure: Could not allocate memory for genLgDesc - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  genLgDesc = p_genLgDesc; //Note SHARED DATA
  const char* searchMastTags[4];
  searchMastTags[SM_TYPE] = "smType";
  searchMastTags[SM_DIMTIME] = "smDimTime";
  searchMastTags[SM_FLASHFREQ] = "smFlashFreq";
  searchMastTags[SM_BRIGHTNESS] = "SmBrightness";
  if(p_mastDescXmlElement == NULL){
    Log.fatal("mastDecoder::onConfigure: No mastDescXml provided - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_PARSE_ERR;
  }
  char* mastDescBuff[4];
  getTagTxt(p_mastDescXmlElement, searchMastTags, mastDescBuff, sizeof(searchMastTags)/4-1); // Need to fix the addressing for portability
  if(mastDescBuff[SM_TYPE] == NULL ||
     mastDescBuff[SM_DIMTIME] == NULL ||
     mastDescBuff[SM_FLASHFREQ] == NULL ||
     mastDescBuff[SM_BRIGHTNESS] == NULL) {
      Log.fatal("mastDecoder::onConfigure: mastDescXml missformated - rebooting..." CR);
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
  }
  mastDesc = new mastDesc_t;
  mastDesc->smType = mastDescBuff[SM_TYPE];
  mastDesc->smDimTime = mastDescBuff[SM_DIMTIME];
  mastDesc->smFlashFreq = mastDescBuff[SM_FLASHFREQ];
  mastDesc->smBrightness = mastDescBuff[SM_BRIGHTNESS];
  genLgDesc->lgNoOfLed = SMASPECTS.getNoOfHeads(mastDesc->smType);
  appearance = new uint8_t[genLgDesc->lgNoOfLed];
  appearanceWriteBuff = new uint8_t[genLgDesc->lgNoOfLed];
  appearanceDimBuff = new uint16_t[genLgDesc->lgNoOfLed];
  if(!strcmp(mastDesc->smDimTime, "NORMAL")){
    smDimTime = SM_DIM_NORMAL;
  }
  else if(!strcmp(mastDesc->smDimTime, "FAST")){
    smDimTime = SM_DIM_FAST;
  }
  else if(!strcmp(mastDesc->smDimTime, "SLOW")){
    smDimTime = SM_DIM_SLOW;
  }
  else {
    Log.error("mastDecoder::onConfigure: smDimTime is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    smDimTime = SM_DIM_NORMAL;
  }
  if(!strcmp(mastDesc->smBrightness, "HIGH")){
    smBrightness = SM_BRIGHNESS_HIGH;
    
  }
  else if(!strcmp(mastDesc->smBrightness, "NORMAL")){
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  else if(!strcmp(mastDesc->smBrightness, "LOW")){
    smBrightness = SM_BRIGHNESS_LOW;
  }
  else{
    Log.error("mastDecoder::onConfigure: smBrighness is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  opState = OP_CONFIG;
  return RC_OK;
}

uint8_t mastDecoder::start(void){
  if(strcmp(mastDesc->smFlashFreq, "FAST")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_FAST)->subscribe(mastDecoder::onFlashHelper, this);
  } 
  else if(strcmp(mastDesc->smFlashFreq, "NORMAL")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else if(strcmp(mastDesc->smFlashFreq, "SLOW")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_SLOW)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else{
    Log.error("mastDecoder::start: smFlashFreq is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  char lgAddrTxtBuff[5];
  const char* subscribeTopic[4] = {MQTT_ASPECT_TOPIC, MQTT.getUri(), "/", itoa(genLgDesc->lgAddr, lgAddrTxtBuff, 10)};
  MQTT.subscribeTopic(concatStr(subscribeTopic, 4), mastDecoder::onAspectChangeHelper, this);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(mastDecoderLock);
  return RC_OK;
}

uint8_t mastDecoder::stop(void){
  Log.fatal("mastDecoder::stop: stop not supported - rebooting..." CR);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  opState = OP_FAIL;
  xSemaphoreGive(mastDecoderLock);
  //failsafe();
  ESP.restart();
  return RC_NOTIMPLEMENTED_ERR;
}

void mastDecoder::onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject){
  ((mastDecoder*)p_mastObject)->onAspectChange(p_topic, p_payload);
}

void mastDecoder::onAspectChange(const char* p_topic, const char* p_payload){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  if(opState != OP_WORKING){
    xSemaphoreGive(mastDecoderLock);
    Log.error("mastDecoder::onAspectChange: A new aspect received, but mast decoder opState is not OP_WORKING - continuing..." CR);
    return;
  }
  xSemaphoreGive(mastDecoderLock);
  if(parseXmlAppearance(p_payload, aspect)){
    return;
  }
  SMASPECTS.getAppearance(mastDesc->smType, aspect, &tmpAppearance);
  for(int i=0; i<genLgDesc->lgNoOfLed; i++){
    appearance[i] = tmpAppearance[i];
    appearanceDimBuff[i] = smDimTime;
    switch(appearance[i]){
      case LIT_APPEARANCE:
        appearanceWriteBuff[i] = smBrightness;
        break;
      case UNLIT_APPEARANCE:
        appearanceWriteBuff[i] = 0;
        break;
      case UNUSED_APPEARANCE:
        appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
        break;
      case FLASH_APPEARANCE:
        if(flashOn){
          appearanceWriteBuff[i] = smBrightness;
        } else{
          appearanceWriteBuff[i] = 0;
        }
        break;
      default:
        Log.error("mastDecoder::onAspectChange: The appearance is none of LIT, UNLIT, FLASH or UNUSED - setting head to SM_BRIGHNESS_FAIL and continuing..." CR);
        appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
        appearanceWriteBuff[i] = 0;
    }
  }
  genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset, genLgDesc->lgNoOfLed, appearanceWriteBuff, appearanceDimBuff);
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
}

uint8_t mastDecoder::parseXmlAppearance(const char* p_aspectXml, char* p_aspect){
  tinyxml2::XMLDocument aspectXmlDocument;
  if(aspectXmlDocument.Parse(p_aspectXml) || aspectXmlDocument.FirstChildElement("Aspect") == NULL || (strcpy(p_aspect, aspectXmlDocument.FirstChildElement("Aspect")->GetText()))){
    Log.error("mastDecoder::parseXmlAppearance: Failed to parse the new aspect - continuing..." CR);
    return RC_PARSE_ERR;
  }
  return RC_OK;
}

void mastDecoder::onFlashHelper(const bool p_flashState, void* p_flashObj){
  ((mastDecoder*)p_flashObj)->onFlash(p_flashState);
}

void mastDecoder::onFlash(const bool p_flashState){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  flashOn = p_flashState;
  for (uint16_t i=0; i<genLgDesc->lgNoOfLed; i++){
    if(appearance[i] == FLASH_APPEARANCE){
      if(flashOn){
        genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset + i, (uint8_t)1, &smBrightness, &smDimTime);
      }else{
        uint8_t zero = 0;
        genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset + i, (uint8_t)1, &zero, &smDimTime);
      }
    }
  }
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
}

uint8_t mastDecoder::getOpState(void){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(mastDecoderReentranceLock);
  return tmpOpState;
}

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial && !Serial.available()){}
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
//  Log.setPrefix(printPrefix); // set prefix similar to NLog
  Log.notice("Logging started towards Serial" CR);
  NETWORK.start();
  uint8_t wifiWait = 0;
  while(NETWORK.opState != OP_WORKING){
    if(wifiWait >= 60) {
      Log.fatal("Could not connect to wifi - rebooting..." CR);
      ESP.restart();
    } else {
      Log.notice("Waiting for WIFI to connect" CR);
    }
    wifiWait++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  Log.notice("WIFI connected" CR);
  topDecoder();
}

/*==============================================================================================================================================*/
/* END setup                                                                                                                                    */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: loop                                                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void loop() {
  // put your main code here, to run repeatedly:
  // Serial.print("Im in the background\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
