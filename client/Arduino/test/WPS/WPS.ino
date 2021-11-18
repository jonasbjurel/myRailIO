/*
Example Code To Get ESP32 To Connect To A Router Using WPS
===========================================================
This example code provides both Push Button method and Pin
based WPS entry to get your ESP connected to your WiFi router.

Hardware Requirements
========================
ESP32 and a Router having WPS functionality

This code is under Public Domain License.

Author:
Pranav Cherukupalli <cherukupallip@gmail.com>
*/

//#include <genJMRIDecoderClient.h>
#include <ArduinoLog.h>
//#include <C:\Users\jonas\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WiFi\src\WiFi.h>
#include "WiFi.h"
#include "esp_wps.h"

#define OP_INIT         0
#define OP_CONFIG       1
#define OP_WORKING      2
#define OP_CONNECTED    2
#define OP_FAIL         3


static const BaseType_t CORE_0 = 0;
static const BaseType_t CORE_1 = 1;

static const byte WPS_PIN =34;


/*
Change the definition of the WPS mode
from WPS_TYPE_PBC to WPS_TYPE_PIN in
the case that you are using pin type
WPS
*/
#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"

static esp_wps_config_t config;

void wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
}

String wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0;i<8;i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void WiFiEvent(WiFiEvent_t event, system_event_info_t info){
  switch(event){
    case SYSTEM_EVENT_STA_START:
      Serial.println("Station Mode Started");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("Connected to :" + String(WiFi.SSID()));
      Serial.print("Got IP: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("Disconnected from station, attempting reconnection");
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WPS Successful, stopping WPS and connecting to: " + String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WPS Failed, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WPS Timeout, retrying");
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
}

void setup(){
  Serial.begin(115200);
  delay(10);

  Serial.println();

  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);

  Serial.println("Starting WPS");

  wpsInitConfig();
  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
  /*  
  xTaskCreatePinnedToCore(
    wifi,                   // Task function
    "MQTT polling",             // Task function name reference
    1024,                       // Stack size
    NULL,                       // Parameter passing
    10,                         // Priority 0-24, higher is more
    NULL,                       // Task handle
    CORE_1);                    // Core [core_0 | core_1]
 

  xTaskCreatePinnedToCore(
    updateOP,                   // Task function
    "update OP",                // Task function name reference
    1024,                       // Stack size
    NULL,                       // Parameter passing
    15,                         // Priority 0-24, higher is more
    NULL,                       // Task handle
    core_1);                    // Core [core_0 | core_1]
}

void mqttpoll(void *dummy) {
    while(1){
      Log.notice("Polling MQTT" CR);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void updateOP(void *dummy) {
    while(1){
      Log.notice("Updating OP" CR);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
*/
}

void loop(){
  // put your main code here, to run repeatedly:
  Serial.print("Im in the background\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
