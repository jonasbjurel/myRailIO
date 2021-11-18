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
#include "SateliteTest.h"

/*==============================================================================================================================================*/
/* END Include files*/
/*==============================================================================================================================================*/
#define NO_OF_SAT 8

#define SAT_RX_CH 1
#define SAT_RX_PIN  14
#define SAT_RX_MEM_BLCK 1
#define SAT_TX_CH 2
#define SAT_TX_PIN  25
#define SAT_TX_MEM_BLCK 2

#define SATBUF_LEN 8
#define SATBUF_SENSE_BYTE_OFFSET 0
#define SATBUF_ACT3_BYTE_OFFSET 1
#define SATBUF_ACT2_BYTE_OFFSET 2
#define SATBUF_ACT1_BYTE_OFFSET 3
#define SATBUF_ACT0_BYTE_OFFSET 4
#define SATBUF_MODE3_BYTE_OFFSET 5
#define SATBUF_MODE3_BIT_OFFSET 5
#define SATBUF_MODE2_BYTE_OFFSET 5
#define SATBUF_MODE2_BIT_OFFSET 2
#define SATBUF_MODE1H_BYTE_OFFSET 5
#define SATBUF_MODE1H_BIT_OFFSET 1
#define SATBUF_MODE1L_BYTE_OFFSET 6
#define SATBUF_MODE1L_BIT_OFFSET 7
#define SATBUF_MODE0_BYTE_OFFSET 6
#define SATBUF_MODE0_BIT_OFFSET 4
#define SATBUF_RESERVED_BYTE_OFFSET 6
#define SATBUF_RESERVED_BIT_OFFSET 0
#define SATBUF_FB_BYTE_OFFSET 7
#define SATBUF_FB_BIT_OFFSET 4
#define SATBUF_CRC_BYTE_OFFSET 7
#define SATBUF_CRC_BIT_OFFSET 0

#define SATMODE_PULSE 4
#define SATMODE_PULSE_INV 5
#define SATMODE_PWM100 3
#define SATMODE_PWM1_25K 2
#define SATMODE_HIGH 1
#define SATMODE_LOW 0
/*
#define CPUFREQ           80000000
#define WS28XX_T0H_NS     400
#define WS28XX_T0L_NS     850
#define WS28XX_T1H_NS     800
#define WS28XX_T1L_NS     450
#define WS28XX_CYC_NS     1250
#define WS28XX_GUARD_NS   10000

#define WS28XX_T0H_CYC  32            //WS28XX_T0H_NS*CPUFREQ/1000000000
#define WS28XX_T0L_CYC  68            //WS28XX_T0L_NS*CPUFREQ/1000000000
#define WS28XX_T1H_CYC  64            //WS28XX_T1H_NS*CPUFREQ/1000000000
#define WS28XX_T1L_CYC  36            //WS28XX_T1L_NS*CPUFREQ/1000000000
#define WS28XX_CYC_CYC   100          //WS28XX_CYC_NS*CPUFREQ/1000000000
#define WS28XX_GUARD_CYC 800          //WS28XX_GUARD_NS*CPUFREQ/1000000000

*/

#define CPUFREQ                     80000000
#define WS28XX_T0H_NS               250 //400
#define WS28XX_T1H_NS               600 //800
#define WS28XX_CYC_NS               1250
#define WS28XX_GUARD_NS             10000

#define WS28XX_T0H_CYC              20              //WS28XX_T0H_NS*CPUFREQ/1000000000
#define WS28XX_T0H_CYC_MIN          18              //WS28XX_T0H_CYC * 0.9
#define WS28XX_T0H_CYC_MAX          22              //WS28XX_T0H_CYC * 1.1
#define WS28XX_T0L_CYC              (WS28XX_CYC_CYC - WS28XX_T0H_CYC)
#define WS28XX_T1H_CYC              48              //WS28XX_T1H_NS*CPUFREQ/1000000000
#define WS28XX_T1H_CYC_MIN          43              //WS28XX_T1H_CYC * .9
#define WS28XX_T1H_CYC_MAX          53              //WS28XX_T1H_CYC * 1.1
#define WS28XX_T1L_CYC              (WS28XX_CYC_CYC - WS28XX_T1H_CYC)
#define WS28XX_CYC_CYC              100             //WS28XX_CYC_NS*CPUFREQ/1000000000
#define WS28XX_CYC_CYC_MIN          90              //WS28XX_CYC_CYC*0.9
#define WS28XX_CYC_CYC_MAX          110             //WS28XX_CYC_CYC*1.1
#define WS28XX_GUARD_CYC            800             //WS28XX_GUARD_NS*CPUFREQ/1000000000

/*
struct satelite_t{
  uint8_t adress;                     // Satelite local adress on the Satelite link
  uint8_t actMode[3];                 // Satelite Actuators mode
  uint8_t actVal[3];                  // Satelite Actuators value
  bool sense[8];                      // Sensor current values
  uint8_t senseTres[8];               // Sensor change threshold in 5 ms intervals
  sense_callback_t callback;          // Callback function for sensor changes
  uint32_t crcErr;                    // Cumulative CRC errors
  uint32_t remoteCrcErr;              // Cumulative remote CRC errors
  uint8_t opState;                    // Satelite operational status - a bitmap showing the cause
  uint8_t admState;                   // Satelite operational status - a bitmap showing origin of the operational state
  satLink_t* satLink;                 // A handle to the parent Satelite link
};

struct satLink_t {
  rmt_channel_t txCh;                 // Satelite link TX channel
  rmt_channel_t rxCh;                 // Satelite link RX channel
  gpio_num_t txPin;                   // Satelite link TX Pin
  gpio_num_t rxPin;                   // Satelite link RX Pin
  uint8_t txMemblck                   // Satelite link TX RMT Memory block
  uint8_t rxMemblck                   // Satelite link RX RMT Memory block
  list?** satelites;                  // Linked list to all Satelites discovered on this satelite link
  uint32_t crcErr;                    // Cumulative counter for all CRC errors reported on this satelite link
  uint32_t remoteCrcErr;              // Cumulative counter for all remote CRC errors reported on this satelite link
  uint16_t crcThres;                  // Cumulative CRC error threshold (local and remote) for bringing the link down.
  uint8_t opState;                    // Satelite link operational status - a bitmap showing the cause
  uint8_t admState;                   // Satelite link operational status - a bitmap showing the origin of the operational state
 }
  
//Constructor

uint8_t satLink(rmt_channel_t txCh, rmt_channel_t rxCh, uint8_t txRmtMemBank, uint8_t RxRmtMemBank, uint8_t* txBuff, uint8_t* rxBuff, satLink_t* satLink){
}

uint8_t satLinkDiscover(rmt_channel_t txCh, rmt_channel_t rxCh, uint8_t* txBuff, uint8_t* rxBuff, satLinkHandle_t* satLink){
}

uint8_t satLinkSenseCbReg(satLink_t* satLink, uint8_t satAdr, uint8_t sensorBitMap, sensCbFn_t fn){
}

uint8_t satLinkNoOfSats(satLinkHandle_t* satLink){
}

uint8_t satLinkSatInfo(satLinkHandle_t* satLink, uint8_t satAdress, satelite_t* sateliteInfo){
}

uint8_t satLinkCrcErr(satLinkHandle_t* satLink, uint32_t crcErrs){
}

uint8_t satLinkRemoteCrcErr(satLinkHandle_t* satLink, uint32_t RemoteCrcErrs){
}

uint8_t setSatActMode(satLinkHandle_t* satLink, uint8_t satAdr, uint8_t actBitMap, uint8_t mode){
}

uint8_t getSatActMode(satLinkHandle_t* satLink, uint8_t satAdr, uint8_t act, uint8_t* mode){
}

uint8_t setSatActValue(satLinkHandle_t* satLink, uint8_t satAdr, uint8_t act, uint8_t value){
}

uint8_t getSatActValue(satLinkHandle_t* satLink, uint8_t satAdr, uint8_t act, uint8_t* value){
}

uint8_t injectSatActCrcErr(satLinkHandle_t* satLink, uint8_t satAdr){
}

uint8_t injectSatActRemoteCrcErr(satLinkHandle_t* satLink, uint8_t satAdr){
}
*/

//Adafruit_NeoPixel* sateliteLink;
uint8_t value;
rmt_obj_t* rmt_recv = NULL;
size_t rx_size = 0;
rmt_item32_t* items = NULL;
// define ringbuffer handle
RingbufHandle_t rb;
uint8_t* satRxBuff;
uint8_t* satTxBuff;
rmt_item32_t* txItems;
size_t txItemNum;
size_t txTranslatedSize;



// initialize an RMT receive channel
void rx_channel_init(int channel, int pin, int mem_blck) {
  rmt_config_t config;
    config.rmt_mode = RMT_MODE_RX;
    config.channel = (rmt_channel_t)channel;
    config.gpio_num = (gpio_num_t)pin;
    config.clk_div = 1;
    config.mem_block_num = mem_blck;
        config.rx_config.filter_en = false;
        config.rx_config.filter_ticks_thresh = 0;
        config.rx_config.idle_threshold = 800;

  rmt_config(&config);
  rmt_driver_install(config.channel, 10000, 0);
  rmt_get_ringbuf_handle(config.channel, &rb);
  rmt_rx_start(config.channel, 1);
}

void tx_channel_init(int channel, int pin, int mem_blck) {
  rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = (rmt_channel_t)channel;
    config.gpio_num = (gpio_num_t)pin;
    config.clk_div = 1;
    config.mem_block_num = mem_blck;
      config.tx_config.carrier_freq_hz = 38000;
      config.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
      config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
      config.tx_config.carrier_duty_percent = 33;
      config.tx_config.carrier_en = false;
      config.tx_config.loop_en = false;
      config.tx_config.idle_output_en = true;

  rmt_config(&config);
  rmt_driver_install(config.channel, 0, 0);
  txItems = (rmt_item32_t*)malloc(NO_OF_SAT*8*8*4);
  rmt_translator_init(config.channel, ws28xx_rmt_tx_adapter);
}

static void IRAM_ATTR ws28xx_rmt_tx_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num){
  if (src == NULL || dest == NULL) {
    *translated_size = 0;
    *item_num = 0;
    return;
  }
  const rmt_item32_t bit0 = {{{ WS28XX_T0H_CYC, 1, WS28XX_T0L_CYC, 0 }}}; //Logical 0
  const rmt_item32_t bit1 = {{{ WS28XX_T1H_CYC, 1, WS28XX_T1L_CYC, 0 }}}; //Logical 1
  size_t size = 0;
  size_t num = 0;
  uint8_t *psrc = (uint8_t *)src;
  rmt_item32_t *pdest = dest;
  while (size < src_size && num < wanted_num) {
      for (int i = 0; i < 8; i++) {
          // MSB first
          if (*psrc & (1 << (7 - i))) {
              pdest->val =  bit1.val;
          } else {
              pdest->val =  bit0.val;
          }
          num++;
          pdest++;
      }
      size++;
      psrc++;
  }
  *translated_size = size;
  *item_num = num;
}


void itob(int x, char *buf){
  unsigned char *ptr = (unsigned char *)&x;
  int pos = 0;
  for (int i = sizeof(int) - 1; i >= 0; i--)
    for (int j = CHAR_BIT - 1; j >= 0; j--)
      buf[pos++] = '0' + !!(ptr[i] & 1U << j);
  buf[pos] = '\0';
}

void crc(unsigned char* p_crc, unsigned char* p_buff, unsigned int p_buffSize, bool p_invalidate){
  uint8_t crc;
  //char* crcStr;
  //crcStr = (char*)malloc(128);
  crc = 0x00;
  for(unsigned int buffIndex=0; buffIndex<p_buffSize; buffIndex++){
    for(unsigned int bitIndex=8; bitIndex>0; bitIndex--){
      //itob(crc, crcStr);
      //Serial.print(crcStr);
      //Serial.print(" ");

      if(buffIndex != p_buffSize-1 || bitIndex-1 > 3){
        crc = crc << 1;
        if(p_buff[buffIndex] & (1 << (bitIndex-1)))
          crc = crc ^ 0b00010000;
        if(crc & 0b00010000)
          crc = crc ^ 0b00000011;
      }
    }
  }
  *p_crc = *p_crc & 0xF0;
  if(p_invalidate)
    *p_crc = *p_crc | (~crc & 0x0F);
  else
    *p_crc = *p_crc | (crc & 0x0F);
}


uint8_t IRAM_ATTR parseWs2811(const rmt_item32_t* src, uint8_t* dest, uint16_t offset, uint16_t len) {
  //Serial.println("Parsing");
  if (src->level0 != 1){
    Serial.println("Symbol ERR-1");
    return 0;
  }
  for (uint16_t i = 0; i < len / 8; i++) {
    dest[i] = 0;
    for (uint8_t j = 8; j > 0; j--) {
      //Serial.printf("Duration 0: %d\n", src[i * 8 + 8 - j].duration0);
      if (src[i * 8 + 8 - j].duration0 >= WS28XX_T1H_CYC_MIN && src[i * 8 + 8 - j].duration0 <= WS28XX_T1H_CYC_MAX)
        dest[i] = dest[i] | (0x01 << j - 1);
      else if(src[i * 8 + 8 - j].duration0 < WS28XX_T0H_CYC_MIN || src[i * 8 + 8 - j].duration0 > WS28XX_T0H_CYC_MAX) {
        Serial.println("Symbol ERR-2");
        return 0;
      }
      else if ((i != 7 && j != 1) && (src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 < WS28XX_CYC_CYC_MIN ||
           src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 > WS28XX_CYC_CYC_MAX)){
        Serial.printf("Symbol ERR-3, cycle: %d, i=%d j=%d\n", src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1,i,j);
        return 0;
      }
    }
  }
  return 0;
}


/*
#define WS28TRESH 34
uint8_t parseWs2811(rmt_item32_t *items, uint8_t* outBuff, uint16_t offset, uint16_t len){
  int8_t c;
  
  if(items->level0 != 1)
    return 1;
  for(uint16_t i=offset; i<offset+len/8; i++){
    outBuff[i] = 0;
    for(uint8_t j=8; j>0; j--){
      //Serial.printf("level0:%i Duration0:%i level1:%i Duration1:%i ", items[i*8+8-j].level0, items[i*8+8-j].duration0, items[i*8+8-j].level1, items[i*8+8-j].duration1);

      if(items[i*8+8-j].duration0 > WS28TRESH){
        
        //Serial.printf("1");
        c = 0b00000001;
        outBuff[i] = outBuff[i] | (c << j-1);
      }
//      else
        //Serial.printf("0");
    }
  }
  return 0;
}

*/

/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
       // Initialize the channel to capture up to 192 items
  //if ((rmt_recv = rmtInit(33, false, RMT_MEM_64)) == NULL){
 //   Serial.println("init receiver failed\n");
 // }
  //float realTick = rmtSetTick(rmt_recv, 100);
  //Serial.printf("real tick set to: %fns\n", realTick);

  // Ask to start reading
  //rmtRead(rmt_recv, receive_data, NULL);
  //Serial.println(rmtDeinit(rmt_recv));

  satTxBuff = (uint8_t*)malloc(8*NO_OF_SAT);
  satRxBuff = (uint8_t*)malloc(8*NO_OF_SAT);
  for(uint16_t l=0; l<8*NO_OF_SAT; l++){
    satTxBuff[l] = 0;
    satRxBuff[l] = 0;
  }
  rx_channel_init(SAT_RX_CH, SAT_RX_PIN, SAT_RX_MEM_BLCK);
  tx_channel_init(SAT_TX_CH, SAT_TX_PIN, SAT_TX_MEM_BLCK);

  //sateliteLink = new Adafruit_NeoPixel(32, 25, NEO_RGB + NEO_KHZ800); //96 bytes (8), PIN = 25, 12 Satelites
  //sateliteLink->begin();
  //satTxBuff = sateliteLink->getPixels();
  for(uint8_t m=0;m <NO_OF_SAT*8; m++)
    satTxBuff[m] = m;
    
  satTxBuff[0] = 0x00;
  satTxBuff[SATBUF_SENSE_BYTE_OFFSET+NO_OF_SAT*8-8] = 0x00;
  satTxBuff[SATBUF_ACT3_BYTE_OFFSET+NO_OF_SAT*8-8] = 0x00;
  satTxBuff[SATBUF_ACT2_BYTE_OFFSET+NO_OF_SAT*8-8] = 0x00;
  satTxBuff[SATBUF_ACT1_BYTE_OFFSET+NO_OF_SAT*8-8] = 0x00;
  satTxBuff[SATBUF_ACT0_BYTE_OFFSET+NO_OF_SAT*8-8] = 0x00;
  satTxBuff[SATBUF_MODE3_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE3_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE3_BIT_OFFSET); 
  satTxBuff[SATBUF_MODE3_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE3_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_PWM100 << SATBUF_MODE3_BIT_OFFSET);
  satTxBuff[SATBUF_MODE2_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE2_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE2_BIT_OFFSET);
  satTxBuff[SATBUF_MODE2_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE2_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_PWM1_25K << SATBUF_MODE2_BIT_OFFSET);
  
  satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 >> SATBUF_MODE1H_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_PULSE >> SATBUF_MODE1H_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE1L_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_PULSE << SATBUF_MODE1L_BIT_OFFSET);
  satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE0_BIT_OFFSET);
  satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_PULSE_INV << SATBUF_MODE0_BIT_OFFSET);
/*
//Test code Low/HIGH
  satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 >> SATBUF_MODE1H_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1H_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_LOW >> SATBUF_MODE1H_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE1L_BIT_OFFSET);
  satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE1L_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_LOW << SATBUF_MODE1L_BIT_OFFSET);
  satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b111 << SATBUF_MODE0_BIT_OFFSET);
  satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_MODE0_BYTE_OFFSET+NO_OF_SAT*8-8] | (SATMODE_HIGH << SATBUF_MODE0_BIT_OFFSET);
//End test code High/Low
  */
  satTxBuff[SATBUF_RESERVED_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_RESERVED_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b1111 << SATBUF_RESERVED_BIT_OFFSET);
  satTxBuff[SATBUF_RESERVED_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_RESERVED_BYTE_OFFSET+NO_OF_SAT*8-8] | (0b0000 << SATBUF_RESERVED_BIT_OFFSET);
  satTxBuff[SATBUF_FB_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_FB_BYTE_OFFSET+NO_OF_SAT*8-8] & ~(0b1111 << SATBUF_FB_BIT_OFFSET);
  satTxBuff[SATBUF_FB_BYTE_OFFSET+NO_OF_SAT*8-8] = satTxBuff[SATBUF_FB_BYTE_OFFSET+NO_OF_SAT*8-8] | (0b0000 << SATBUF_FB_BIT_OFFSET);

  crc(&satTxBuff[SATBUF_CRC_BYTE_OFFSET+NO_OF_SAT*8-8], &satTxBuff[+NO_OF_SAT*8-8], 8, false);
  //sateliteLink->show();
  rmt_write_sample((rmt_channel_t)SAT_TX_CH, satTxBuff, (size_t)NO_OF_SAT*8, true);
  //rmt_wait_tx_done(config.channel, pdMS_TO_TICKS(100))
  value = 0;
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
  vTaskDelay(300 / portTICK_PERIOD_MS);
  
 /* for(uint8_t index=0; index<96; index++) {
    Serial.print(" 0x");
    Serial.print(satTxBuff[index], HEX);
  }*/
  //Serial.println("");
  value++;
  //value=3;
  satTxBuff[SATBUF_ACT3_BYTE_OFFSET+NO_OF_SAT*8-8] = value;
  satTxBuff[SATBUF_ACT2_BYTE_OFFSET+NO_OF_SAT*8-8] = value;
  satTxBuff[SATBUF_ACT1_BYTE_OFFSET+NO_OF_SAT*8-8] = value;
  satTxBuff[SATBUF_ACT0_BYTE_OFFSET+NO_OF_SAT*8-8] = value;
  crc(&satTxBuff[SATBUF_CRC_BYTE_OFFSET+NO_OF_SAT*8-8], &satTxBuff[NO_OF_SAT*8-8], 8, false);
  //sateliteLink->show();

  ws28xx_rmt_tx_adapter(satTxBuff, txItems, NO_OF_SAT*8, NO_OF_SAT*8*8, &txTranslatedSize, &txItemNum);
  /*Serial.printf("Translated size: %i, itemNum: %i \n", txTranslatedSize, txItemNum);
  parseWs2811(txItems, satRxBuff, 0, 64*8);
  Serial.printf("SendBuff: ");

  for(uint16_t j=0; j<64; j++){
    Serial.printf("%x: ", satRxBuff[j]);
  }
  Serial.printf("\n");

  
  Serial.printf("Ringuff size: %i", xRingbufferGetCurFreeSize(rb));
*/

  for(uint8_t i=0; i<NO_OF_SAT; i++){
    //rmt_write_sample((rmt_channel_t) SAT_TX_CH, &satTxBuff[i*8], (size_t)8, true);
    rmt_write_items((rmt_channel_t) SAT_TX_CH, &txItems[i*8*8], 64, false);
    //delay(1);
  }



  uint8_t i=0;
  while (items = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, 10)){
    parseWs2811(items, satRxBuff+i*8, 0, rx_size/4); //Try to parse afterwards, make a quick memCpy instead
    vRingbufferReturnItem(rb, (void*) items);
    i++;
  }
  if(i != 8)
    Serial.printf("Expected 8 bytes, got %i bytes", i);

  Serial.printf("Sent buffer: ");
  for(uint16_t l=0; l<NO_OF_SAT*8; l++)
    Serial.printf("%x:", satTxBuff[l]);
  Serial.printf("\n");
  Serial.printf("Received buffer: ");
  for(uint16_t l=0; l<NO_OF_SAT*8; l++)
    Serial.printf("%x:", satRxBuff[l]);
  Serial.printf("\n");
  
  for(uint16_t l=0; l<NO_OF_SAT; l++){
    uint8_t crcSum;
    crcSum = 0;
    crc(&crcSum, &satRxBuff[l*8], 8, false);
    if((crcSum&0x0F) != (satRxBuff[l*8+7]&0x0F))
        Serial.printf("Satelite %i data have CRC errors, expected CRC: %x, detected CRC: %x\n", l, crcSum&0x0F, satRxBuff[l*8+7]&0x0F);
    else
        Serial.printf("Satelite %i data were free from CRC errors, expected CRC: %x, detected CRC: %x\n", l, crcSum&0x0F, satRxBuff[l*8+7]&0x0F );

    if(satRxBuff[l*8+7]&0x10)
        Serial.printf("Satelite %i reported remote CRC errors\n", l);
    else
        Serial.printf("Satelite %i did not report remote CRC errors\n", l);
  }
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
