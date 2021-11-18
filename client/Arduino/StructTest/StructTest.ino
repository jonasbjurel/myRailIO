//#include <MqttClient.h> After recovery
#include <ArduinoLog.h>
//#include <C:\Users\jonas\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WiFi\src\WiFi.h>
//#include "WiFi.h"
//#include "esp_wps.h"
#include "esp_timer.h"
//#include <PubSubClient.h>
//#include <QList.h>
//#include <tinyxml2.h>
//#include <esp_task_wdt.h>
#include <Adafruit_NeoPixel.h>
//#include <cmath>
//#include "ESPTelnet.h"
//#include <SimpleCLI.h>
#include <limits>
//#include <cstddef>
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "soc/rmt_reg.h"

struct satLinkWire_t {
    uint8_t sensorVal;
    uint8_t actVal[4];
    uint8_t actMode[4];
    uint8_t cmdReserv;
    uint8_t invCrc;
    uint8_t fbReserv;
    uint8_t wdErr;
    uint8_t remoteCrcErr;
    uint8_t startMark;
    uint8_t crc;
};

satLinkWire_t satWire;

uint8_t* buff;


void setup() {
  Serial.begin(115200);
  buff = (uint8_t*)malloc(8);
  satWire.sensorVal=0x01;
  satWire.actVal[3]=0x23;
  satWire.actVal[2]=0x45;
  satWire.actVal[1]=0x67;
  satWire.actVal[0]=0x89;
  satWire.actMode[3]=0b111;
  satWire.actMode[2]=0b110;
  satWire.actMode[1]=0b100;
  satWire.actMode[0]=0b000; 
  satWire.cmdReserv=0b00;
  satWire.invCrc=0b1;
  satWire.startMark=0b1;
  satWire.fbReserv=0b00;
  satWire.wdErr=0b1;
  satWire.remoteCrcErr=0b1;
  satWire.crc=0x5;

  buff[0] = satWire.sensorVal;
  buff[1] = satWire.actVal[3];
  buff[2] = satWire.actVal[2];
  buff[3] = satWire.actVal[1];
  buff[4] = satWire.actVal[0];
  buff[5] = 0x00;
  buff[5] = buff[5] | (satWire.actMode[3] & 0x7) << 5;
  buff[5] = buff[5] | (satWire.actMode[2] & 0x7) << 2;
  buff[5] = buff[5] | (satWire.actMode[1] & 0x7) >> 1;
  buff[6] = 0x00;
  buff[6] = buff[6] | (satWire.actMode[1] & 0x7) << 7;
  buff[6] = buff[6] | (satWire.actMode[0] & 0x7) << 4;
  buff[6] = buff[6] | (satWire.cmdReserv & 0x3) << 2;
  buff[6] = buff[6] | (satWire.invCrc & 0x1) << 1;
  buff[6] = buff[6] | (satWire.startMark & 0x1) << 1;

  buff[7] = 0x00;
  buff[7] = buff[7] | (satWire.fbReserv & 0x3) << 6;
  buff[7] = buff[7] | (satWire.wdErr & 0x1) << 5;
  buff[7] = buff[7] | (satWire.remoteCrcErr & 0x1) << 4;
  buff[7] = buff[7] | (satWire.crc & 0xF);

  Serial.printf("Size of struct %i\n", sizeof(satWire));
  Serial.printf("Content: ");
  for(uint8_t i=0; i<8; i++)
    Serial.printf("%x:", buff[i]); 
}

void loop() {

  delay(1);
}
