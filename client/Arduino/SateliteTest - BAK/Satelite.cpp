/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2021 Jonas Bjurel (jonasbjurel@hotmail.com)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law and agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either expressed or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*= END License ================================================================================================================================*/

/*==============================================================================================================================================*/
/* Description                                                                                                                                  */
/*==============================================================================================================================================*/
//
//
/*= END Description ============================================================================================================================*/

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "Satelite.h"
/*= END Include files ==========================================================================================================================*/

/*==============================================================================================================================================*/
/* Function and Class implementation                                                                                                            */
/*==============================================================================================================================================*/
// Commn variables

//locks
SemaphoreHandle_t rxSatStructLock;
SemaphoreHandle_t txSatStructLock;
SemaphoreHandle_t satStatusLock;


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: ws28xx_rmt_tx_translator & ws28xx_rmt_rx_translator                                                                                */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
static void IRAM_ATTR ws28xx_rmt_tx_translator(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num, 
            size_t* translated_size, size_t* item_num) {
              
	if (src == NULL || dest == NULL) {
		*translated_size = 0;
		*item_num = 0;
		return;
	}
	const rmt_item32_t bit0 = { {{ WS28XX_T0H_CYC, 1, WS28XX_T0L_CYC, 0 }} }; //Logical 0
	const rmt_item32_t bit1 = { {{ WS28XX_T1H_CYC, 1, WS28XX_T1L_CYC, 0 }} }; //Logical 1
	size_t size = 0;
	size_t num = 0;
	uint8_t* psrc = (uint8_t*)src;
	rmt_item32_t* pdest = dest;
	while (size < src_size && num < wanted_num) {
		for (int i = 0; i < 8; i++) {
			// MSB first
			if (*psrc & (1 << (7 - i))) {
				pdest->val = bit1.val;
			}
			else {
				pdest->val = bit0.val;
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


satErr_t IRAM_ATTR ws28xx_rmt_rx_translator(const rmt_item32_t* src, uint8_t* dest, uint16_t len) {
  //Serial.println("Parsing");
	if (src->level0 != 1){
    Serial.println("Symbol ERR-1");
		return SAT_ERR_SYMBOL_ERR;
	}
	for (uint16_t i = 0; i < len / 8; i++) {
		dest[i] = 0;
		for (uint8_t j = 8; j > 0; j--) {
			if (src[i * 8 + 8 - j].duration0 >= WS28XX_T1H_CYC_MIN && src[i * 8 + 8 - j].duration0 <= WS28XX_T1H_CYC_MAX)
				dest[i] = dest[i] | (0x01 << j - 1);
			else if(src[i * 8 + 8 - j].duration0 < WS28XX_T0H_CYC_MIN || src[i * 8 + 8 - j].duration0 > WS28XX_T0H_CYC_MAX){
        Serial.println("Symbol ERR-2");
				return SAT_ERR_SYMBOL_ERR;
			}
			else if (src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 < WS28XX_CYC_CYC_MIN ||
					 src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 > WS28XX_CYC_CYC_MAX){
				Serial.println("Symbol ERR-3");
				return SAT_ERR_SYMBOL_ERR;
			}
		}
	}
	return SAT_OK;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: crc												                                                                                */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void crc(uint8_t* p_crc, uint8_t* p_buff, uint16_t p_buffSize, bool p_invalidate) {
	uint8_t crc;
	crc = 0x00;
	for (unsigned int buffIndex = 0; buffIndex < p_buffSize; buffIndex++) {
		for (unsigned int bitIndex = 8; bitIndex > 0; bitIndex--) {
			//itob(crc, crcStr);
			//Serial.print(crcStr);
			//Serial.print(" ");

			if (buffIndex != p_buffSize - 1 || bitIndex - 1 > 3) {
				crc = crc << 1;
				if (p_buff[buffIndex] & (1 << (bitIndex - 1)))
					crc = crc ^ 0b00010000;
				if (crc & 0b00010000)
					crc = crc ^ 0b00000011;
			}
		}
	}
	*p_crc = *p_crc & 0xF0;
	if (p_invalidate)
		*p_crc = *p_crc | (~crc & 0x0F);
	else
		*p_crc = *p_crc | (crc & 0x0F);
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: returnCode												                                                                            */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satErr_t returnCode(uint8_t localErr, uint32_t callFnErr) {
  satErr_t rc;
  rc = callFnErr;
  rc = (rc << 32) | localErr;
	return rc;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: populateSatLinkBuff & populateSatWireStruct                                                                                        */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void populateSatLinkBuff(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p) {
	satWireBuff_p[0] = satWireStruct_p->sensorVal;
	satWireBuff_p[1] = satWireStruct_p->actVal[3];
	satWireBuff_p[2] = satWireStruct_p->actVal[2];
	satWireBuff_p[3] = satWireStruct_p->actVal[1];
	satWireBuff_p[4] = satWireStruct_p->actVal[0];
	satWireBuff_p[5] = 0x00;
	satWireBuff_p[5] = satWireBuff_p[5] | (satWireStruct_p->actMode[3] & 0x07) << 5;
	satWireBuff_p[5] = satWireBuff_p[5] | (satWireStruct_p->actMode[2] & 0x07) << 2;
	satWireBuff_p[5] = satWireBuff_p[5] | (satWireStruct_p->actMode[1] & 0x07) >> 1;
	satWireBuff_p[6] = 0x00;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->actMode[1] & 0x07) << 7;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->actMode[0] & 0x07) << 4;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->cmdReserv & 0x01) << 3;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->cmdEnable & 0x01) << 2;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->cmdInvCrc & 0x1) << 1;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->startMark & 0x01);
	satWireBuff_p[7] = 0x00;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->fbReserv & 0x3) << 6;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->fbWdErr & 0x1) << 5;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->fbRemoteCrcErr & 0x1) << 4;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->crc & 0xF);
}


bool populateSatWireStruct(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p) {
  bool sensChange = false;
  if (satWireStruct_p->sensorVal != satWireBuff_p[0])
    sensChange = true;
	satWireStruct_p->sensorVal = satWireBuff_p[0];
	satWireStruct_p->actVal[3] = satWireBuff_p[1];
	satWireStruct_p->actVal[2] = satWireBuff_p[2];
	satWireStruct_p->actVal[1] = satWireBuff_p[3];
	satWireStruct_p->actVal[0] = satWireBuff_p[4];
	satWireStruct_p->actMode[3] = 0x00 | (satWireBuff_p[5] & 0xE0) >> 5;
	satWireStruct_p->actMode[2] = 0x00 | (satWireBuff_p[5] & 0x1C) >> 2;
	satWireStruct_p->actMode[1] = 0x00 | (satWireBuff_p[5] & 0x03) << 1 | (satWireBuff_p[6] & 0x80) >> 7;
	satWireStruct_p->actMode[0] = 0x00 | (satWireBuff_p[6] & 0x70) >> 4;
	satWireStruct_p->cmdReserv = 0x00 | (satWireBuff_p[6] & 0x08) >> 3;
	satWireStruct_p->cmdEnable = 0x00 | (satWireBuff_p[6] & 0x04) >> 2;
	satWireStruct_p->cmdInvCrc = 0x00 | (satWireBuff_p[6] & 0x02) >> 1;
	satWireStruct_p->startMark = 0x00 | (satWireBuff_p[6] & 0x01);
	satWireStruct_p->fbReserv = 0x00 | (satWireBuff_p[7] & 0xC0) >> 6;
	satWireStruct_p->fbWdErr = 0x00 | (satWireBuff_p[7] & 0x20) >> 5;
	satWireStruct_p->fbRemoteCrcErr = 0x00 | (satWireBuff_p[7] & 0x10) >> 4;
	satWireStruct_p->crc = 0x00 | (satWireBuff_p[7] & 0x0F);
  return sensChange;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: clearPerformanceCounters                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void clearPerformanceCounters(satPerformanceCounters_t performanceCounters_p) {
  performanceCounters_p.txUnderunErr = 0;
  performanceCounters_p.txUnderunErrSec = 0;
  performanceCounters_p.rxOverRunErr = 0;
  performanceCounters_p.rxOverRunErrSec = 0;
  performanceCounters_p.scanTimingViolationErr = 0;
  performanceCounters_p.scanTimingViolationErrSec = 0;
  performanceCounters_p.rxDataSizeErr = 0;
  performanceCounters_p.rxDataSizeErrSec = 0;
  performanceCounters_p.rxSymbolErr = 0;
  performanceCounters_p.rxSymbolErrSec = 0;
  performanceCounters_p.wdErr = 0;
  performanceCounters_p.wdErrSec = 0;
  performanceCounters_p.rxCrcErr = 0;
  performanceCounters_p.rxCrcErrSec = 0;
  performanceCounters_p.remoteCrcErr = 0;
  performanceCounters_p.remoteCrcErrSec = 0;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: sateliteLink                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/***** PUBLIC MEMBERS *****/
/*sateliteLink Constructur*/
sateliteLink::sateliteLink(uint8_t address_p, gpio_num_t txPin_p, gpio_num_t rxPin_p, rmt_channel_t txCh_p, rmt_channel_t rxCh_p, uint8_t txRmtMemBank_p, uint8_t rxRmtMemBank_p, UBaseType_t pollTaskPrio_p, UBaseType_t pollTaskCore_p, uint8_t scanInterval_p) {

	satLinkInfo = new satLinkInfo_t;
	rxSatStructLock = xSemaphoreCreateMutex();
  txSatStructLock = xSemaphoreCreateMutex();
  satStatusLock = xSemaphoreCreateMutex();
  satLinkInfo->performanceCounterLock = xSemaphoreCreateMutex();
	rmt_config_t rmtTxConfig;
	rmt_config_t rmtRxConfig;
	satLinkInfo->address = address_p;
	satLinkInfo->txPin = txPin_p;
	satLinkInfo->rxPin = rxPin_p;
	satLinkInfo->txCh = txCh_p;
	satLinkInfo->rxCh = rxCh_p;
	satLinkInfo->txMemblck = txRmtMemBank_p;
	satLinkInfo->rxMemblck = rxRmtMemBank_p;
	satLinkInfo->pollTaskPrio = pollTaskPrio_p;
	satLinkInfo->pollTaskCore = pollTaskCore_p;
	satLinkInfo->scanInterval = scanInterval_p;
	satLinkInfo->errThresHigh = 0;
	satLinkInfo->errThresLow = 0;
	satLinkInfo->noOfSats = 0;
	satLinkInfo->opState = SAT_OP_DISABLE;
	satLinkInfo->admState = SAT_ADM_DISABLE;
	satLinkInfo->satLinkStateCb = NULL;
	satLinkInfo->serverCrcTst = false;
  satLinkInfo->clientCrcTst = false;
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++){
		satLinkInfo->sateliteHandle[i] = NULL;
    satLinkInfo->txSatStruct[i].enable = false;
    satLinkInfo->txSatStruct[i].dirty = true;
	}
	satLinkInfo->scan = false;
	clearPerformanceCounters(satLinkInfo->performanceCounters);
	satLinkInfo->oneSecTimer = esp_timer_get_time();
	rmtTxConfig.rmt_mode = RMT_MODE_TX;
	rmtTxConfig.channel = satLinkInfo->txCh;
	rmtTxConfig.gpio_num = satLinkInfo->txPin;
	rmtTxConfig.clk_div = 1;
	rmtTxConfig.mem_block_num = satLinkInfo->txMemblck;
	rmtTxConfig.tx_config.carrier_freq_hz = 38000;
	rmtTxConfig.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
	rmtTxConfig.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	rmtTxConfig.tx_config.carrier_duty_percent = 33;
	rmtTxConfig.tx_config.carrier_en = false;
	rmtTxConfig.tx_config.loop_en = false;
	rmtTxConfig.tx_config.idle_output_en = true;
	assert(rmt_config(&rmtTxConfig) == ESP_OK);
	assert(rmt_driver_install(satLinkInfo->txCh, 0, 0) == ESP_OK);
	rmt_translator_init(satLinkInfo->txCh, ws28xx_rmt_tx_translator);
	rmtRxConfig.rmt_mode = RMT_MODE_RX;
	rmtRxConfig.channel = satLinkInfo->rxCh;
	rmtRxConfig.gpio_num = satLinkInfo->rxPin;
	rmtRxConfig.clk_div = 1;
	rmtRxConfig.mem_block_num = satLinkInfo->rxMemblck;
	rmtRxConfig.rx_config.filter_en = false;
	rmtRxConfig.rx_config.filter_ticks_thresh = 0;
	rmtRxConfig.rx_config.idle_threshold = WS28XX_GUARD_CYC;
	rmt_config(&rmtRxConfig);
	rmt_driver_install(satLinkInfo->rxCh, 10000, 0);
	rmt_get_ringbuf_handle(satLinkInfo->rxCh, &satLinkInfo->rb);
}


/*sateliteLink Destructur*/
sateliteLink::~sateliteLink(void) {
	satLinkStopScan();
	assert(satLinkInfo->opState == SAT_OP_DISABLE);
	assert(rmt_driver_uninstall(satLinkInfo->txCh) == ESP_OK);
	assert(rmt_driver_uninstall(satLinkInfo->rxCh) == ESP_OK);
	delete satLinkInfo;
}


/*sateliteLink enableSatLink*/
satErr_t sateliteLink::enableSatLink(void){
	esp_err_t rmtRc;
	satErr_t rc;
  
	if(satLinkInfo->admState != SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	if(rmtRc = rmt_rx_start(satLinkInfo->rxCh, 1) != ESP_OK)
		return (returnCode(SAT_ERR_RMT_ERR, rmtRc));
	if (rc = admDeBlock())
		return (returnCode(rc, 0));
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		satLinkInfo->txSatStruct[i].invServerCrc = false;
		satLinkInfo->txSatStruct[i].invClientCrc = false;
		satLinkInfo->txSatStruct[i].enable = false;
		satLinkInfo->txSatStruct[i].dirty = true;
    xSemaphoreGive(txSatStructLock);
	}
	if(rc = satLinkStartScan())
		return (returnCode(rc, 0));
  if(rc = satLinkDiscover())
    return (returnCode(rc, 0));
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink disableSatLink*/
satErr_t sateliteLink::disableSatLink(void) {
	esp_err_t rmtRc;
	satErr_t rc;
	if (satLinkInfo->admState != SAT_ADM_ENABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	if (rc = admBlock())
		return (returnCode(rc, 0));
	if (rmtRc = rmt_rx_stop(satLinkInfo->rxCh) != ESP_OK)
		return (returnCode(SAT_ERR_RMT_ERR, rmtRc));
	if (rc = satLinkStopScan())
		return (returnCode(rc, 0));
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++)
		if (satLinkInfo->sateliteHandle[i] != NULL)
			delete satLinkInfo->sateliteHandle[i];
}


/*sateliteLink setErrTresh*/
void sateliteLink::setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow) {
	satLinkInfo->errThresHigh = p_errThresHigh;
	satLinkInfo->errThresLow = p_errThresLow;
}


/*sateliteLink satLinkRegStateCb*/
void sateliteLink::satLinkRegStateCb(satLinkStateCb_t satLinkStateCb_p) {
	satLinkInfo->satLinkStateCb = satLinkStateCb_p;
}

/*sateliteLink satLinkUnRegStateCb*/
void sateliteLink::satLinkUnRegStateCb(void) {
	satLinkInfo->satLinkStateCb = NULL;
}


/*sateliteLink getSatLinkNoOfSats*/
uint8_t sateliteLink::getSatLinkNoOfSats(void) {
	return satLinkInfo->noOfSats;
}


void sateliteLink::getSatLinkStats(satPerformanceCounters_t* satStats_p, bool resetStats) {
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
  memcpy((void*)satStats_p, (void*)&(satLinkInfo->performanceCounters), sizeof(satPerformanceCounters_t));
  if (resetStats)
    clearPerformanceCounters(satLinkInfo->performanceCounters);
  xSemaphoreGive(satLinkInfo->performanceCounterLock);
}


void sateliteLink::clearSatLinkStats(void) {
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
  clearPerformanceCounters(satLinkInfo->performanceCounters);
  xSemaphoreGive(satLinkInfo->performanceCounterLock);
}


/*sateliteLink getsatRef*/
satelite* sateliteLink::getsatHandle(uint8_t satAddr_p) {
  if (satAddr_p >= satLinkInfo->noOfSats)
    return NULL;
	return satLinkInfo->sateliteHandle[satAddr_p];
}


/***** PRIVATE MEMBERS *****/

/*sateliteLink satLinkDiscover*/
satErr_t sateliteLink::satLinkDiscover(void) {
  satelite* satTmp;

  Serial.printf("Running discovery\n");

	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
		if (satLinkInfo->sateliteHandle[i] != NULL) {
      satTmp = satLinkInfo->sateliteHandle[i];
      satLinkInfo->sateliteHandle[i] = NULL;
			delete satTmp;
		}
	}
  satLinkInfo->noOfSats = MAX_NO_OF_SAT_PER_CH + 1;       //Scan one more sat than max

	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    Serial.printf("Ask for inverting CRC for sat %d\n", i);
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		satLinkInfo->txSatStruct[i].invServerCrc = true;
		satLinkInfo->txSatStruct[i].dirty = true;
    xSemaphoreGive(txSatStructLock);
	}
	vTaskDelay(50 / portTICK_PERIOD_MS);

  xSemaphoreTake(satStatusLock, portMAX_DELAY);
  if (satLinkInfo->satStatus[MAX_NO_OF_SAT_PER_CH + 1].remoteCrcErr) {
    xSemaphoreGive(satStatusLock);
		return (returnCode(SAT_ERR_EXESSIVE_SATS, 0));
	}
  xSemaphoreGive(satStatusLock);

  xSemaphoreTake(satStatusLock, portMAX_DELAY);
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		bool endOfSat = false;
		if (!satLinkInfo->satStatus[i].remoteCrcErr)
			endOfSat = true;
		if (satLinkInfo->satStatus[i].remoteCrcErr && endOfSat) {
			return (returnCode(SAT_ERR_GEN_SATLINK_ERR, 0));
		}
	}
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    satLinkInfo->noOfSats = i;
		if (!satLinkInfo->satStatus[i].remoteCrcErr)
      break;
    else{
      Serial.printf("Detected remote CRC for sat %d\n", i);
			satLinkInfo->sateliteHandle[i] = new satelite(this, i);
    }
	}

	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		satLinkInfo->txSatStruct[i].invServerCrc = false;
		satLinkInfo->txSatStruct[i].dirty = true;
    xSemaphoreGive(txSatStructLock);
	}
  xSemaphoreGive(satStatusLock);

	vTaskDelay(50 / portTICK_PERIOD_MS);

  xSemaphoreTake(satStatusLock, portMAX_DELAY);
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		satLinkInfo->satStatus[i].dirty = false;
	}
  xSemaphoreGive(satStatusLock);
  Serial.printf("End discovery\n");

	return (returnCode(SAT_OK, 0));
}


/*sateliteLink satLinkStartScan*/
satErr_t sateliteLink::satLinkStartScan(void) {
	BaseType_t taskRc;
	if(satLinkInfo->scan == true)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));

	taskRc = xTaskCreatePinnedToCore(
		satLinkScan,											      // Task function
		"satLinkScan",											    // Task function name reference
		6 * 1024,												        // Stack size
		this,													          // Parameter passing
		satLinkInfo->pollTaskPrio,							// Priority 0-24, higher is more
		&satLinkInfo->scanTaskHandle,					  // Task handle
		satLinkInfo->pollTaskCore);							//

	if (taskRc != pdPASS)
		return (returnCode(SAT_ERR_SCANTASK_ERR, 0));

	satLinkInfo->scan = true;
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink satLinkStopScan*/
satErr_t sateliteLink::satLinkStopScan(void) {
	if (satLinkInfo->scan == false)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	vTaskDelete(satLinkInfo->scanTaskHandle);
	satLinkInfo->scan = false;
	return (returnCode(SAT_OK, 0));
}


void sateliteLink::satLinkScan(void* satLinkObj) {
  int64_t t0;

	while (true) {
		t0 = esp_timer_get_time();
		satLinkInfo_t* satLinkInfo = ((sateliteLink*)satLinkObj)->satLinkInfo;
    //Serial.printf("Scaning - time:%d\n", t0);
    //Serial.printf("Number of satelites:%d\n", satLinkInfo->noOfSats);
        
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
      Serial.println("looping-1");

      //Serial.printf("Im in");
			if (i == 0)
				satLinkInfo->txSatStruct[i].startMark = 0x01;
			else
				satLinkInfo->txSatStruct[i].startMark = 0x00;

			if (satLinkInfo->txSatStruct[i].dirty == true) {
				if (satLinkInfo->txSatStruct[i].invServerCrc){
					satLinkInfo->serverCrcTst = SAT_CRC_TEST_ACTIVE;
				}
        else if (!satLinkInfo->txSatStruct[i].invServerCrc && satLinkInfo->serverCrcTst == SAT_CRC_TEST_ACTIVE)
          satLinkInfo->serverCrcTst = SAT_CRC_TEST_DEACTIVATING;       
				if (satLinkInfo->txSatStruct[i].invClientCrc) {
					satLinkInfo->txSatStruct[i].cmdInvCrc = 0x01;
					satLinkInfo->clientCrcTst = SAT_CRC_TEST_ACTIVE;
				}
        else if (!satLinkInfo->txSatStruct[i].invClientCrc && satLinkInfo->clientCrcTst == SAT_CRC_TEST_ACTIVE){
          satLinkInfo->txSatStruct[i].cmdInvCrc = 0x01;
          satLinkInfo->clientCrcTst = SAT_CRC_TEST_DEACTIVATING;
        }
				else {
					satLinkInfo->txSatStruct[i].cmdInvCrc = 0x00;
				}
				if (satLinkInfo->txSatStruct[i].enable)
					satLinkInfo->txSatStruct[i].enable = 0x01;
				else
					satLinkInfo->txSatStruct[i].enable = 0x00;

				populateSatLinkBuff(&satLinkInfo->txSatStruct[i], &satLinkInfo->txSatBuff[i * 8]);
				crc(&satLinkInfo->txSatBuff[i * 8 + SATBUF_CRC_BYTE_OFFSET], &satLinkInfo->txSatBuff[i * 8],
				    8, (satLinkInfo->txSatStruct[i].invServerCrc == 0x01));
        if (satLinkInfo->txSatStruct[i].invServerCrc == 0x01)
          //Serial.printf("Invalidating outgoing server CRC for Satelite %d\n", i);
				satLinkInfo->txSatStruct[i].dirty = false;
			}

			if (satLinkInfo->serverCrcTst != SAT_CRC_TEST_ACTIVE && satLinkInfo->serverCrcTst != SAT_CRC_TEST_INACTIVE)
				satLinkInfo->serverCrcTst--;
			if (satLinkInfo->clientCrcTst != SAT_CRC_TEST_INACTIVE && satLinkInfo->clientCrcTst != SAT_CRC_TEST_INACTIVE)
				satLinkInfo->clientCrcTst--;
		}
    xSemaphoreGive(txSatStructLock);

		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
			rmt_write_sample(satLinkInfo->txCh, &satLinkInfo->txSatBuff[i * 8], (size_t)8, true);
      Serial.println("looping-2");
		}
		bool rxSymbolErr, rxDataSizeErr;
		rxSymbolErr = false;
		rxDataSizeErr = false;
    
		while (true) {
      Serial.println("looping-3");
			rmt_item32_t* items = NULL;
			size_t rx_size = 0;
			uint16_t tot_rx_size = 0;
      uint8_t index = 0;

			items = (rmt_item32_t*)xRingbufferReceive(satLinkInfo->rb, &rx_size, 10);
      items = NULL;
			tot_rx_size += rx_size / 4;
			if (!items) {
				if (tot_rx_size != satLinkInfo->noOfSats * 8 * 8) {
				  rxDataSizeErr = true;
				  satLinkInfo->performanceCounters.rxDataSizeErr++;
				  satLinkInfo->performanceCounters.rxDataSizeErrSec++;
          Serial.printf("Size ERR\n");
				  break;
				}
				else
					break;
			}
			else {
				if (ws28xx_rmt_rx_translator(items, &satLinkInfo->txSatBuff[index * 8], rx_size / 4)) {
					rxSymbolErr = true;
					satLinkInfo->performanceCounters.rxSymbolErr++;
					satLinkInfo->performanceCounters.rxSymbolErrSec++;
					while (items = (rmt_item32_t*)xRingbufferReceive(satLinkInfo->rb, &rx_size, 10))
						vRingbufferReturnItem(satLinkInfo->rb, (void*)items);
					break;
				}
			}
      index++;
		}

		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {

			bool change;
			uint8_t crcCalc;
      bool err;
      bool dirtyTmp;

      Serial.println("looping-4");

      
      xSemaphoreTake(rxSatStructLock, portMAX_DELAY);
			change = populateSatWireStruct(&satLinkInfo->rxSatStruct[i], &satLinkInfo->rxSatBuff[i * 8]);
			crc(&crcCalc, &satLinkInfo->txSatBuff[i*8], 8, false);
      xSemaphoreGive(rxSatStructLock);
      
      err = false;
      xSemaphoreTake(satStatusLock, portMAX_DELAY);
      if (!(dirtyTmp = satLinkInfo->satStatus[i].dirty)) {
        satLinkInfo->satStatus[i].rxDataSizeErr = false;
        satLinkInfo->satStatus[i].rxSymbolErr = false;
        satLinkInfo->satStatus[i].rxCrcErr = false;
        satLinkInfo->satStatus[i].remoteCrcErr = false;
        satLinkInfo->satStatus[i].wdErr = false;
      }

      xSemaphoreTake(rxSatStructLock, portMAX_DELAY);
			if (crcCalc != satLinkInfo->rxSatStruct[i].crc || satLinkInfo->rxSatStruct[i].fbRemoteCrcErr || satLinkInfo->rxSatStruct[i].fbRemoteCrcErr || rxDataSizeErr || rxSymbolErr) {
				if (rxDataSizeErr && !dirtyTmp){
				  satLinkInfo->satStatus[i].rxDataSizeErr = true;
          err = true;
				}
				else if (rxSymbolErr && !dirtyTmp) {
			    satLinkInfo->satStatus[i].rxSymbolErr = true;
          err = true;
				}
				else {
					if (satLinkInfo->satStatus[i].rxCrcErr = (crcCalc != satLinkInfo->rxSatStruct[i].crc)) {
						if (!satLinkInfo->clientCrcTst) {
							satLinkInfo->performanceCounters.rxCrcErr++;
							satLinkInfo->performanceCounters.rxCrcErrSec++;
              if (!dirtyTmp){
                satLinkInfo->satStatus[i].rxCrcErr = true;
                err = true;
              }
						}
					}
					if (satLinkInfo->satStatus[i].remoteCrcErr = (satLinkInfo->rxSatStruct[i].fbRemoteCrcErr != 0)) {
						if (!satLinkInfo->serverCrcTst) {
              //Serial.printf("Remote CRC ERR for Sat: %d\n", i);
							satLinkInfo->performanceCounters.remoteCrcErr++;
							satLinkInfo->performanceCounters.remoteCrcErrSec++;
              if (!dirtyTmp){
                satLinkInfo->satStatus[i].remoteCrcErr = true;
                err = true;
              }
						}
					}
					if (satLinkInfo->satStatus[i].wdErr = (satLinkInfo->rxSatStruct[i].fbWdErr != 0)) {
						satLinkInfo->performanceCounters.wdErr++;
						satLinkInfo->performanceCounters.wdErrSec++;
            if (!dirtyTmp){
              satLinkInfo->satStatus[i].wdErr = true;
              err = true;
            }
					}
				}
        if (err && satLinkInfo->sateliteHandle[i] != NULL) {
				  satLinkInfo->satStatus[i].dirty = true;
				  satLinkInfo->sateliteHandle[i]->statusUpdate(&satLinkInfo->satStatus[i]);
        }
			}
			else if (change && satLinkInfo->sateliteHandle[i] != NULL) {
				satLinkInfo->rxSatStruct[i].dirty == true;
				satLinkInfo->sateliteHandle[i]->senseUpdate(&satLinkInfo->rxSatStruct[i]);
			}
      xSemaphoreGive(rxSatStructLock);
      xSemaphoreGive(satStatusLock);

		}
		((sateliteLink*)satLinkObj)->chkErrSec();
		int64_t nextTime;

		if ((nextTime = satLinkInfo->scanInterval - ((esp_timer_get_time() - t0) / 1000)) <= 0) {
			satLinkInfo->performanceCounters.scanTimingViolationErr++;
			satLinkInfo->performanceCounters.scanTimingViolationErrSec++;
      Serial.printf("Overrun next time %d ms\n", nextTime);
      Serial.printf("Scan ended-1 time %d us\n", esp_timer_get_time());
		}
		else{
      //Serial.printf("Scan ended-2 time %d us\n", esp_timer_get_time());
      Serial.printf("Delaying next scan %d ms\n", nextTime);
			vTaskDelay(nextTime / (portTICK_PERIOD_MS));
		}
	}
}


/*sateliteLink opBlock*/
void sateliteLink::opBlock(satOpState_t p_opState) {
	if (!satLinkInfo->opState)
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++)
			satLinkInfo->sateliteHandle[i]->opBlock(SAT_OP_CONTROLBOCK);
	satLinkInfo->opState = satLinkInfo->opState | p_opState;
	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(satLinkInfo->opState);
}


/*sateliteLink opDeBlock*/
void sateliteLink::opDeBlock(satOpState_t p_opState) {
	if (!(satLinkInfo->opState = satLinkInfo->opState & ~p_opState))
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++)
			satLinkInfo->sateliteHandle[i]->opDeBlock(SAT_OP_CONTROLBOCK);
      
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
	clearPerformanceCounters(satLinkInfo->performanceCounters);
  xSemaphoreGive(satLinkInfo->performanceCounterLock);

	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(satLinkInfo->opState);
}


/*sateliteLink admBlock*/
satErr_t sateliteLink::admBlock(void) {
	if (satLinkInfo->admState == SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
		if (satLinkInfo->sateliteHandle[i]->satInfo->admState != SAT_ADM_DISABLE)
			return (returnCode(SAT_ERR_SAT_NOTBLOCKED_ERR, 0));
	}
	satLinkInfo->admState = SAT_ADM_DISABLE;
	opBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink admDeBlock*/
satErr_t sateliteLink::admDeBlock(void) {
	if (satLinkInfo->admState == SAT_ADM_ENABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	satLinkInfo->admState = SAT_ADM_ENABLE;
	opDeBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink chkErrSec*/
void sateliteLink::chkErrSec(void) {
	uint16_t ErrSum;
	int64_t now;

	if (!satLinkInfo->errThresHigh)
		return;

  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
	ErrSum = satLinkInfo->performanceCounters.rxDataSizeErrSec +
		satLinkInfo->performanceCounters.rxSymbolErrSec +
		satLinkInfo->performanceCounters.rxCrcErrSec +
		satLinkInfo->performanceCounters.remoteCrcErrSec +
		satLinkInfo->performanceCounters.scanTimingViolationErrSec +
		satLinkInfo->performanceCounters.wdErrSec;

	if (!(satLinkInfo->opState & SAT_OP_ERR_SEC) && ErrSum >= satLinkInfo->errThresHigh)
		opBlock(SAT_OP_ERR_SEC);

	if ((now = esp_timer_get_time()) - satLinkInfo->oneSecTimer >= ONE_SEC_US) {
		satLinkInfo->oneSecTimer = now;
		if ((satLinkInfo->opState & SAT_OP_ERR_SEC) && (ErrSum <= satLinkInfo->errThresLow))
			opDeBlock(SAT_OP_ERR_SEC);
		satLinkInfo->performanceCounters.rxDataSizeErrSec = 0;
		satLinkInfo->performanceCounters.rxSymbolErrSec = 0;
		satLinkInfo->performanceCounters.rxCrcErrSec = 0;
		satLinkInfo->performanceCounters.remoteCrcErrSec = 0;
		satLinkInfo->performanceCounters.scanTimingViolationErrSec = 0;
		satLinkInfo->performanceCounters.wdErrSec = 0;
	}
  xSemaphoreGive(satLinkInfo->performanceCounterLock);
}
/*----------------------------------------------------- END Class sateliteLink -----------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: satelite                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satelite::satelite(sateliteLink* satLink_p, uint8_t satAddr_p) {
  Serial.printf("Creating Satelite %d\n", satAddr_p);
	satInfo = new satInfo_t;
  satInfo->performanceCounterLock = xSemaphoreCreateMutex();
	satInfo->address = satAddr_p;
	satInfo->satLinkParent = satLink_p;
	for (uint8_t i = 0; i < 4; i++) {
		satInfo->actMode[i] = SATMODE_LOW;
		satInfo->actVal[i] = 0;
	}
	for (uint8_t i = 0; i < 8; i++) {
		satInfo->sensors[i].satObj = this;
		satInfo->sensors[i].address = i;
		satInfo->sensors[i].filterTime = 0;
		satInfo->sensors[i].timerHandle = xTimerCreate("SenseTimer", pdMS_TO_TICKS(1), pdFALSE, (void*)&(this->satInfo->sensors[i]), &filterExp);
		satInfo->sensors[i].timerActive = false;
		satInfo->sensors[i].currentSensorVal = false;
		satInfo->sensors[i].filteredSensorVal = false;
	}
  satInfo->oneSecTimerHandle = xTimerCreate("oneSecTimer", pdMS_TO_TICKS(1000), pdTRUE, this, &chkErrSec);
	satInfo->stateCb = NULL;
	satInfo->senseCb = NULL;
  clearPerformanceCounters(satInfo->performanceCounters);
	satInfo->errThresHigh = 0;
	satInfo->errThresLow = 0;
	satInfo->admState = SAT_ADM_DISABLE;
	satInfo->opState = SAT_OP_DISABLE;
}


satelite::~satelite(void) {
	delete satInfo;
}


satErr_t satelite::enableSat(void) {
	return(returnCode(admDeBlock(), 0));
}


satErr_t satelite::disableSat(void) {
	return(returnCode(admBlock(), 0));
}


void satelite::setErrTresh(uint16_t errThresHigh_p, uint16_t errThresLow_p) {
	satInfo->errThresHigh = errThresHigh_p;
	satInfo->errThresLow = errThresLow_p;
}


satErr_t satelite::setSatActMode(actMode_t actMode_p, uint8_t actIndex_p) {
	if(actIndex_p > 3 || actMode_p > 5)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->actMode[actIndex_p] = actMode_p;
  xSemaphoreTake(txSatStructLock, portMAX_DELAY);
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].actMode[actIndex_p] = actMode_p;
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
	return(returnCode(SAT_OK, 0));
  xSemaphoreGive(txSatStructLock);
}


satErr_t satelite::setSatActVal(uint8_t actVal_p, uint8_t actIndex_p) {
	if (actIndex_p > 3)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->actVal[actIndex_p] = actVal_p;
  xSemaphoreTake(txSatStructLock, portMAX_DELAY);
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].actVal[actIndex_p] = actVal_p;
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  xSemaphoreGive(txSatStructLock);
	return(returnCode(SAT_OK, 0));
}


satErr_t satelite::setSenseFilter(uint8_t senseFilter_p, uint8_t actIndex_p) {
	if (actIndex_p > 7)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->sensors[actIndex_p].filterTime = pdMS_TO_TICKS(senseFilter_p);
	return(returnCode(SAT_OK, 0));
}


void satelite::satRegSenseCb(satSenseCb_t fn) {
	satInfo->senseCb = fn;
}


void satelite::satUnRegSenseCb(void) {
	satInfo->senseCb = NULL;
}


void satelite::satRegStateCb(satStateCb_t fn) {
	satInfo->stateCb = fn;
}


void satelite::satUnRegStateCb(void) {
	satInfo->stateCb = NULL;
}


void satelite::senseUpdate(satWire_t* rxData_p) {
	if (satInfo->senseCb == NULL)
		return;
	uint8_t bitTest = 0b00000001;
	for (uint8_t i = 0; i < 8; i++) {
		if (rxData_p->sensorVal & (bitTest << i))
			satInfo->sensors[i].currentSensorVal = true;
		else
			satInfo->sensors[i].currentSensorVal = false;

		if (satInfo->sensors[i].timerActive) {
			assert(xTimerStop(satInfo->sensors[i].timerHandle, 10) != pdFAIL);
			satInfo->sensors[i].timerActive = false;
		}

		if (satInfo->sensors[i].currentSensorVal != satInfo->sensors[i].currentSensorVal) {
      assert(xTimerChangePeriod( satInfo->sensors[i].timerHandle, satInfo->sensors[i].filterTime, 10) != pdFAIL); 
			assert(xTimerStart(satInfo->sensors[i].timerHandle, 10) != pdFAIL);
			satInfo->sensors[i].timerActive = true;
		}
	}
}


void satelite::statusUpdate(satStatus_t* status_p) {
	if (status_p->dirty != true)
		return;

  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
	if (status_p->wdErr) {
		satInfo->performanceCounters.wdErr++;
		satInfo->performanceCounters.wdErrSec++;
	}
	if (status_p->remoteCrcErr) {
		satInfo->performanceCounters.remoteCrcErr++;
		satInfo->performanceCounters.remoteCrcErrSec++;
	}
	if (status_p->rxCrcErr) {
		satInfo->performanceCounters.rxCrcErr++;
		satInfo->performanceCounters.rxCrcErrSec++;
	}
	if (status_p->rxSymbolErr) {
		satInfo->performanceCounters.rxSymbolErr++;
		satInfo->performanceCounters.rxSymbolErrSec++;
	}
	if (status_p->rxDataSizeErr) {
		satInfo->performanceCounters.rxDataSizeErr++;
		satInfo->performanceCounters.rxDataSizeErrSec++;
	}
  xSemaphoreGive(satInfo->performanceCounterLock);
}


/*sateliteLink admBlock*/
satErr_t satelite::admBlock(void) {
  if(satInfo->admState == SAT_ADM_DISABLE)
    return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
  assert(xTimerStop(satInfo->oneSecTimerHandle, 10) != pdFAIL);
  satInfo->admState = SAT_ADM_DISABLE;
  opBlock(SAT_OP_DISABLE);
  return (returnCode(SAT_OK, 0));
}


/*sateliteLink admDeBlock*/
satErr_t satelite::admDeBlock(void) {
  if (satInfo->admState == SAT_ADM_ENABLE)
    return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
  assert(xTimerStart(satInfo->oneSecTimerHandle, 10) != pdFAIL); 
  satInfo->admState = SAT_ADM_ENABLE;
  opDeBlock(SAT_OP_DISABLE);
  return (returnCode(SAT_OK, 0));
}


/*satelite opBlock*/
void satelite::opBlock(satOpState_t opState_p) {
	if (!satInfo->opState) {
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].enable = false;
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
    xSemaphoreGive(txSatStructLock);
	}
		satInfo->opState = satInfo->opState | opState_p;
	if (satInfo->stateCb != NULL)
		satInfo->stateCb(satInfo->satLinkParent->satLinkInfo->address, satInfo->address, satInfo->opState);
}


/*sateliteLink opDeBlock*/
void satelite::opDeBlock(satOpState_t opState_p) {
	if (!(satInfo->opState = satInfo->opState & ~opState_p)){
    xSemaphoreTake(txSatStructLock, portMAX_DELAY);
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].enable = true;
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
    xSemaphoreGive(txSatStructLock);
	}
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
	clearPerformanceCounters(satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
	if (satInfo->stateCb != NULL)
		satInfo->stateCb(satInfo->satLinkParent->satLinkInfo->address, satInfo->address, satInfo->opState);
}


void satelite::getSatStats(satPerformanceCounters_t* satStats_p, bool resetStats) {
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
  memcpy((void*)satStats_p, (void*)&(satInfo->performanceCounters), sizeof(satPerformanceCounters_t));
  if (resetStats)
    clearPerformanceCounters(satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
}


void satelite::clearSatStats(void) {
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
  clearPerformanceCounters(satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
}


void satelite::chkErrSec(TimerHandle_t timerHandle) {
	uint16_t ErrSum;
	satelite* satObj;

  satObj = (satelite*) pvTimerGetTimerID(timerHandle);
	if (!satObj->satInfo->errThresHigh)
		return;

  xSemaphoreTake(satObj->satInfo->performanceCounterLock, portMAX_DELAY);
	ErrSum = satObj->satInfo->performanceCounters.rxDataSizeErrSec +
		satObj->satInfo->performanceCounters.rxSymbolErrSec +
		satObj->satInfo->performanceCounters.rxCrcErrSec +
		satObj->satInfo->performanceCounters.remoteCrcErrSec +
		satObj->satInfo->performanceCounters.wdErrSec;
    satObj->satInfo->performanceCounters.rxDataSizeErrSec = 0;
    satObj->satInfo->performanceCounters.rxSymbolErrSec = 0;
    satObj->satInfo->performanceCounters.rxCrcErrSec = 0;
    satObj->satInfo->performanceCounters.remoteCrcErrSec = 0;
    satObj->satInfo->performanceCounters.wdErrSec = 0;
    xSemaphoreGive(satObj->satInfo->performanceCounterLock);

	if (!(satObj->satInfo->opState & SAT_OP_ERR_SEC) && ErrSum >= satObj->satInfo->errThresHigh)
		satObj->opBlock(SAT_OP_ERR_SEC);

  if ((satObj->satInfo->opState & SAT_OP_ERR_SEC) && ErrSum <= satObj->satInfo->errThresLow)
			satObj->opDeBlock(SAT_OP_ERR_SEC);
}


void satelite::filterExp(TimerHandle_t timerHandle) {
  sensor_t* sensor;
  sensor = (sensor_t*) pvTimerGetTimerID(timerHandle);
  if (sensor->currentSensorVal != sensor->filteredSensorVal) {
    sensor->filteredSensorVal = sensor->filteredSensorVal;
    sensor->timerActive = false;
    sensor->satObj->satInfo->senseCb(sensor->satObj->satInfo->satLinkParent->satLinkInfo->address, sensor->satObj->satInfo->address, sensor->address, sensor->filteredSensorVal);
  }
}
