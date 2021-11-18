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
#include Satelite.h
/*= END Include files ==========================================================================================================================*/

/*==============================================================================================================================================*/
/* Function and Class implementation                                                                                                            */
/*==============================================================================================================================================*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: ws28xx_rmt_tx_translator & ws28xx_rmt_rx_translator                                                                                */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

static void IRAM_ATTR ws28xx_rmt_tx_translator(const void* src, rmt_item32_t* dest, size_t src_size,
	size_t wanted_num, size_t* translated_size, size_t* item_num) {
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
	if (src->level0 != 1)
		return SAT_ERR_SYMBOLERR;
	for (uint16_t i = offset; i < len / 8; i++) {
		dest[i] = 0;
		for (uint8_t j = 8; j > 0; j--) {
			if (src[i * 8 + 8 - j].duration0 >= WS28XX_T1H_CYC_MIN && src[i * 8 + 8 - j].duration0 =< WS28XX_T1H_CYC_MAX)
				dest[i] = dest[i] | (0x01 << j - 1);
			else if(src[i * 8 + 8 - j].duration0 < WS28XX_T0H_CYC_MIN || src[i * 8 + 8 - j].duration0 > WS28XX_T0H_CYC_MAX)
				return SAT_ERR_SYMBOLERR;
			else if (src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 < WS28XX_CYC_CYC_MIN ||
					 src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 > WS28XX_CYC_CYC_MAX)
				return SAT_ERR_SYMBOLERR;
		}
	}
	return SAT_ERR_OK;
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
	return callFnErr << 16 | localErr;
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
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->enable & 0x01) << 2;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->invCrc & 0x1) << 1;
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->startMark & 0x01);
	satWireBuff_p[7] = 0x00;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->fbReserv & 0x3) << 6;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->wdErr & 0x1) << 5;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->remoteCrcErr & 0x1) << 4;
	satWireBuff_p[7] = satWireBuff_p[7] | (satWireStruct_p->crc & 0xF);
}

void populateSatWireStruct(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p) {
	satWireBuff_p[0] = satWireStruct_p->sensorVal;
	satWireStruct_p->actVal[3] = satWireBuff_p[1];
	satWireStruct_p->actVal[2] = satWireBuff_p[2];
	satWireStruct_p->actVal[1] = satWireBuff_p[3];
	satWireStruct_p->actVal[0] = satWireBuff_p[4];
	satWireStruct_p->actMode[3] = 0x00 | (satWireBuff_p[5] & 0xE0) >> 5;
	satWireStruct_p->actMode[2] = 0x00 | (satWireBuff_p[5] & 0x1C) >> 2;
	satWireStruct_p->actMode[1] = 0x00 | (satWireBuff_p[5] & 0x03) << 1 | (satWireBuff_p[6] & 0x80) >> 7;
	satWireStruct_p->actMode[0] = 0x00 | (satWireBuff_p[6] & 0x70) >> 4;
	satWireStruct_p->cmdReserv = 0x00 | (satWireBuff_p[6] & 0x08) >> 3;
	satWireStruct_p->enable = 0x00 | (satWireBuff_p[6] & 0x04) >> 2;
	satWireStruct_p->invCrc = 0x00 | (satWireBuff_p[6] & 0x02) >> 1;
	satWireStruct_p->startMark = 0x00 | (satWireBuff_p[6] & 0x01);
	satWireStruct_p->fbReserv = 0x00 | (satWireBuff_p[7] & 0xC0) >> 6;
	satWireStruct_p->wdErr = 0x00 | (satWireBuff_p[7] & 0x20) >> 5;
	satWireStruct_p->remoteCrcErr = 0x00 | (satWireBuff_p[7] & 0x10) >> 4;
	satWireStruct_p->crc = 0x00 | (satWireBuff_p[7] & 0x0F);
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: sateliteLink                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

/*(sateliteLink) satLinkScan*/



/***** PUBLIC MEMBERS *****/

/*satLinkInfo public data structure*/
sateliteLink::satLinkInfo;

/*sateliteLink Constructur*/
sateliteLink::sateliteLink(uint8_t address_p, gpio_num_t p_txPin, gpio_num_t p_rxPin, rmt_channel_t p_txCh, rmt_channel_t p_rxCh, uint8_t p_txRmtMemBank, uint8_t p_rxRmtMemBank, UBaseType_t p_pollTaskPrio, UBaseType_t p_pollTaskCore, uint8_t p_scanInterval) {
	satLinkInfo_t satLinkInfo;
	satLinkInfo_t satLinkInfo;
	rmt_config_t rmtTxConfig;
	rmt_config_t rmtRxConfig;

	satLinkInfo = new satLinkInfo_t;
	satLinkInfo->address = address_p;
	satLinkInfo->txPin = p_txPin;
	satLinkInfo->rxPin = p_rxPin;
	satLinkInfo->txCh = p_txCh;
	satLinkInfo->rxCh = p_rxCh;
	satLinkInfo->txRmtMemBank = p_txRmtMemBank;
	satLinkInfo->rxRmtMemBank = p_rxRmtMemBank;
	satLinkInfo->pollTaskPrio = p_pollTaskPrio;
	satLinkInfo->pollTaskCore = p_pollTaskCore;
	satLinkInfo->scanInterval = p_scanInterval;
	satLinkInfo->errThresHigh = 0;
	satLinkInfo->errThresLow = 0;
	satLinkInfo->noOfSat = 255;
	satLinkInfo->sateliteRefs = NULL;
	satLinkInfo->opState = SAT_OP_DISABLE;
	satLinkInfo->admState = SAT_ADM_DISABLE;
	satLinkInfo->satLinkInfo->linkstateCb = NULL;
	satLinkInfo->satLinkInfo->crcTst = false;
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++)
		satLinkInfo->sateliteHandle[i] = NULL;
	satLinkInfo->txStructLock = xSemaphoreCreateMutex();
	satLinkInfo->rxStructLock = xSemaphoreCreateMutex();
	satLinkInfo->satTxStructs[i].enable = false;
	satLinkInfo->satTxStructs[i].dirty = true;
	satLinkInfo->scan = false;
	clearSatLinkStats();
	satLinkInfo->oneSecTimer = esp_timer_get_time();

	rmtTxConfig.rmt_mode = RMT_MODE_TX;
	rmtTxConfig.channel = satLinkInfo->txCh;
	rmtTxConfig.gpio_num = satLinkInfo->txPin;
	rmtTxConfig.clk_div = 1;
	rmtTxConfig.mem_block_num = satLinkInfo->txRmtMemBank;
	rmtTxConfig.tx_config.carrier_freq_hz = 38000;
	rmtTxConfig.tx_config.carrier_level = RMT_CARRIER_LEVEL_HIGH;
	rmtTxConfig.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	rmtTxConfig.tx_config.carrier_duty_percent = 33;
	rmtTxConfig.tx_config.carrier_en = false;
	rmtTxConfig.tx_config.loop_en = false;
	rmtTxConfig.tx_config.idle_output_en = true;
	ASSERT(rmt_config(&rmtTxConfig) != ESP_OK);
	ASSERT(rmt_driver_install(satLinkInfo->txCh, 0, 0) != ESP_OK);
	rmt_translator_init(config.channel, ws28xx_rmt_tx_translator);

	rmtRxConfig.rmt_mode = RMT_MODE_RX;
	rmtRxConfig.channel = satLinkInfo->rxCh;
	rmtRxConfig.gpio_num = satLinkInfo->rxPin;
	rmtRxConfig.clk_div = 1;
	rmtRxConfig.mem_block_num = satLinkInfo->rxRmtMemBank;
	rmtRxConfig.rx_config.filter_en = false;
	rmtRxConfig.rx_config.filter_ticks_thresh = 0;
	rmtRxConfig.rx_config.idle_threshold = WS28XX_GUARD_CYC;
	rmt_config(&rmtRxConfig);
	rmt_driver_install(satLinkInfo->rxCh, 10000, 0);
	rmt_get_ringbuf_handle(satLinkInfo->rxCh, &satLinkInfo->rb);
}

/*sateliteLink Destructur*/
sateliteLink::~sateliteLink(void) {
	satLinkStopScan(void);
	ASSERT(satLinkInfo->opState != SAT_OP_DISABLE);
	ASSERT(rmt_driver_uninstall(satLinkInfo->txCh) != ESP_OK);
	ASSERT(rmt_driver_uninstall(satLinkInfo->rxCh) != ESP_OK);
	delete satLinkInfo;
}

/*sateliteLink enableSatLink*/
satErr_t sateliteLink::enableSatLink(void){
	esp_err_t rmtRc;
	satErr_t rc;
	if(satLinkInfo->admState != SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE, 0));
	if(rmtRc = rmt_rx_start(satLinkInfo->rxCh, 1) != ESP_OK)
		return (returnCode(SAT_ERR_RMT_ERR, rmtRc));
	if (rc = admDeBlock())
		return (returnCode(rc, 0));
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		xSemaphoreTake(satLinkInfo->txStructLock, portMAX_DELAY);
		satLinkInfo->satTxStructs[i].invServerCrc = false;
		satLinkInfo->satTxStructs[i].invClientCrc = false;
		satLinkInfo->satTxStructs[i].enable = false;
		satLinkInfo->satTxStructs[i].dirty = true;
		xSemaphoreGive(satLinkInfo->txStructLock);
	}
	if(rc = satLinkStartScan(void))
		return (returnCode(rc, 0));
	return (returnCode(SAT_OK, 0));
}

/*sateliteLink disableSatLink*/
satErr_t sateliteLink::disableSatLink(void) {
	esp_err_t rmtRc;
	satErr_t rc;
	if (satLinkInfo->admState != SAT_ADM_ENABLE)
		return (returnCode(SAT_ERR_WRONG_STATE, 0));
	if (rc = admBlock())
		return (returnCode(rc, 0));
	if (rmtRc = rmt_rx_stop(satLinkInfo->rxCh) != ESP_OK)
		return (returnCode(SAT_ERR_RMT_ERR, rmtRc));
	if (rc = satLinkStopScan(void))
		return (returnCode(rc, 0));
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++)
		if (sats[i] != NULL)
			delete sats[i];
	}
}

/*sateliteLink setErrTresh*/
void sateliteLink::setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow) {
	satLinkInfo->errThresHigh = p_errThresHigh;
	satLinkInfo->errThresLow = p_errThresLow;
}

/*sateliteLink satLinkRegStateCb*/
void sateliteLink::satLinkRegStateCb(satLinkStateCb_t p_satLinkStateCb) {
	satLinkInfo_t->satLinkStateCb = p_satLinkRegStateCb; //DESIGN MAKE THIS A QUEUE OF CB
}

/*sateliteLink satLinkUnRegStateCb*/
void sateliteLink::satLinkUnRegStateCb(void) {
	satLinkInfo_t->satLinkStateCb = NULL;
}



/*sateliteLink getSatLinkNoOfSats*/
uint8_t sateliteLink::getSatLinkNoOfSats(void) {
	return satLinkInfo->noOfSat;
}

/*sateliteLink getsatRef*/
satelite* sateliteLink::getsatHandle(satAddr_t satAddr_p) {
	return satLinkInfo->sateliteHandle[satAddr_p];
}

/*sateliteLink clearSatLinkStats*/
void sateliteLink::clearSatLinkStats(void) {
	satLinkInfo->symbolRxErr = 0;
	satLinkInfo->rxDataSizeErr = 0;
	satLinkInfo->txUnderunErr = 0;
	satLinkInfo->rxOverRunErr = 0;
	satLinkInfo->scanTimingViolation = 0;
	satLinkInfo->rxCrcErr = 0;
	satLinkInfo->remoteCrcErr = 0;
	satLinkInfo->wdErr = 0;
	satLinkInfo->rxDataSizeErrSec = 0;
	satLinkInfo->symbolRxErrSec = 0;
	satLinkInfo->rxCrcErrSec = 0;
	satLinkInfo->remoteCrcErrSec = 0;
	satLinkInfo->scanTimingViolationErrSec = 0;
	satLinkInfo->wdErrSec = 0;
}

/***** PRIVATE MEMBERS *****/

/*sateliteLink satLinkDiscover*/
satErr_t sateliteLink::satLinkDiscover(void) {
	satLinkInfo->noOfSats = MAX_NO_OF_SAT_PER_CH + 1;				//Scan one more sat than max
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
		if (satLinkInfo->sateliteHandle[i] != NULL) {
			delete satLinkInfo->sateliteHandle[i];
			satLinkInfo->sateliteHandle[i] = NULL;
		}
	}
	xSemaphoreTake(satLinkInfo->txStructLock, portMAX_DELAY);
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		satLinkInfo->satTxStructs[i].invServerCrc = true;
		satLinkInfo->satTxStructs[i].dirty = true;
	}
	vTaskDelay(50 / portTICK_PERIOD_MS);
	if (satLinkInfo->satStatus[MAX_NO_OF_SAT_PER_CH + 1].remoteCrcErr) {
		return (returnCode(SAT_ERR_EXESSIVE_SATS, 0));
	}
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		bool endOfSat = false;
		if (!satLinkInfo->satStatus[i].remoteCrcErr)
			endOfSat = true;
		if (satLinkInfo->satStatus[i].remoteCrcErr && endOfSat) {
			return (returnCode(SAT_ERR_GEN_SATLINK_ERR, 0));
		}
	}
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		if (satLinkInfo->satStatus[i].remoteCrcErr) {
			sateliteHandle[i] = satelite(this, i);
			satLinkInfo->noOfSats = i + 1;
		}
	}
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		satLinkInfo->satTxStructs[i].invServerCrc = false;
		satLinkInfo->satTxStructs[i].dirty = true;
	}
	vTaskDelay(50 / portTICK_PERIOD_MS);
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		satLinkInfo->satStatus[i].dirty = false;
	}
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink satLinkStartScan*/
satErr_t sateliteLink::satLinkStartScan(void) {
	BaseType_t taskRc;
	if(satLinkInfo->scan == true)
		return (returnCode(SAT_ERR_WRONG_STATE, 0));

	taskRc = xTaskCreatePinnedToCore(
		satLinkScan,											// Task function
		"satLinkScan",											// Task function name reference
		6 * 1024,												// Stack size
		this,													// Parameter passing
		satLinkInfo->pollTaskPrio,							// Priority 0-24, higher is more
		&satLinkInfo->scanTaskHandle,					// Task handle
		satLinkInfo->pollTaskCore);							//

	if (taskRc)
		return (returnCode(SAT_ERR_SCANTASK_ERR, taskRc));

	satLinkInfo->scan = true;
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink satLinkStopScan*/
satErr_t sateliteLink::satLinkStopScan(void) {
	if (satLinkInfo->scan == false)
		return (returnCode(SAT_ERR_WRONG_STATE, 0));
	vTaskDelete(satLinkInfo->scanTaskHandle);
	satLinkInfo->scan = false;
	return (returnCode(SAT_OK, 0));
}



void sateliteLink::satLinkScan(void* satLinkObj) {
	while (true) {
		int64_t t0;
		t0 = esp_timer_get_time();
		satLink_t* satLinkInfo = (sateliteLink*)satLinkObj->satLinkInfo;

		xSemaphoreTake(satLinkInfo->txStructLock, portMAX_DELAY);
		for (uint8_t i = 0; i satLinkInfo->noOfSats) {
			if (i == 0)
				satLinkInfo->satTxStructs[i].startMark = 0x01;
			else
				satLinkInfo->satTxStructs[i].startMark = 0x00;

			if (satLinkInfo->satTxStructs[i].dirty == true) {
				if (satLinkInfo->satTxStructs[i].invServerCrc) {
					satLinkInfo->serverCrcTst = SAT_CRC_TEST_ACTIVE;
				}
				if (satLinkInfo->satTxStructs[i].invClientCrc) {
					satLinkInfo->satTxStructs[i].cmdInvCrc = 0x01;
					satLinkInfo->clientCrcTst = SAT_CRC_TEST_ACTIVE;
				}
				else {
					satLinkInfo->satTxStructs[i].cmdInvCrc = 0x00;
				}
				if (satLinkInfo->satTxStructs[i].enable)
					satLinkInfo->satTxStructs[i].enable = 0x01;
				else
					satLinkInfo->satTxStructs[i].enable = 0x00;

				populateSatLinkBuff(&satLinkInfo->satTxStructs[i], &satLinkInfo_t->satTxBuff[i * 8]);
				crc(&satTxBuff[i * 8 + SATBUF_CRC_BYTE_OFFSET], &satLinkInfo_t->satTxBuff[i * 8], 8, (satLinkInfo->satTxStructs[i].invServerCrc == 0x01));
				satLinkInfo->satTxStructs[i].dirty = false;
			}

			if (satLinkInfo->serverCrcTst != SAT_CRC_TEST_INACTIVE)
				satLinkInfo->serverCrcTst--;
			if (satLinkInfo->clientCrcTst != SAT_CRC_TEST_INACTIVE)
				satLinkInfo->clientCrcTst--;
		}

		xSemaphoreGive(satLinkInfo->txStructLock);
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++)
			rmt_write_sample(satLinkInfo->txCh, &satLinkInfo_t->satTxBuff[i * 8], (size_t)8, true);
		bool rxSymbolErr, rxDataSizeErr;
		rxSymbolErr = false;
		rxDataSizeErr = false;
		while (true) {
			rmt_item32_t* items = NULL;
			size_t rx_size = 0;
			uint16_t tot_rx_size = 0;

			items = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 10);
			tot_rx_size += rx_size / 4;
			if (!(items = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 10))) {
				if (tot_rx_size !) satLinkInfo->noOfSats * 8 * 8) {
				rxDataSizeErr = true;
				xSemaphoreTake(satLinkInfo->genLock, portMAX_DELAY);
				satLinkInfo->rxDataSizeErr++;
				satLinkInfo->rxDataSizeErrSec++;
				xSemaphoreGive(satLinkInfo->genLock);
				break;
				}
				else
					break;
			}
			else {
				if (parseWs2811(items, &satLinkInfo_t->satTxBuff[i * 8], rx_size / 4)) {
					rxSymbolErr = true;
					vRingbufferReturnItem(rb, (void*)items);
					xSemaphoreTake(satLinkInfo->genLock, portMAX_DELAY);
					satLinkInfo->symbolRxErr++;
					satLinkInfo->symbolRxErrSec++;
					satLinkInfo->satStatus.dirty = true;
					xSemaphoreGive(satLinkInfo->genLock);
					while (items = (rmt_item32_t*)xRingbufferReceive(rb, &rx_size, 10))
						vRingbufferReturnItem(rb, (void*)items);
					break;
				}
			}
		}
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
			*---------------------------------------------------------- -
				bool change;
			uint8_t crc;
			xSemaphoreTake(satLinkInfo->rxStructLock, portMAX_DELAY);
			change = populateSatWireStruct(&satLinkInfo->satRxStructs[i], &satLinkInfo->satRxBuff[i * 8]))
			crc(&crc, &satLinkInfo->satRxStructs[i], uint16_t 8, false);
			xSemaphoreGive(satLinkInfo->rxStructLock);
			if (crc != satLinkInfo->satRxStructs[i].crc || satLinkInfo->satRxStructs[i].fbRemoteCrcErr || satLinkInfo->satRxStructs[i].fbRemoteCrcErr || rxDataSizeErr || rxSymbolErr) {
				if (rxDataSizeErr)
					satLinkInfo->satStatus[i].rxDataSizeErr = true;

				else if (rxSymbolErr)
					satLinkInfo->satStatus[i].symbolErr = true;

				else {
					xSemaphoreTake(satLinkInfo->rxStructLock, portMAX_DELAY);
					if (satLinkInfo->satStatus[i].rxCrcErr = (crc != satLinkInfo->satRxStructs[i].crc)) {
						if (!satLinkInfo->clientCrcTst) {
							satLinkInfo->rxCrcErr++;
							satLinkInfo->rxCrcErrSec++;
						}
					}
					if (satLinkInfo->satStatus[i].remoteCrcErr = (satLinkInfo->satRxStructs[i].fbRemoteCrcErr != 0)) {
						if (!satLinkInfo->serverCrcTst) {
							satLinkInfo->remoteCrcErr++;
							satLinkInfo->remoteCrcErrSec++;
						}
					}
					if (satLinkInfo->satStatus[i].wdErr = (satLinkInfo->satRxStructs[i].fbWdErr != 0)) {
						satLinkInfo->wdErr++;
						satLinkInfo->wdErrSec++;
					}
					xSemaphoreGive(satLinkInfo->rxStructLock);
				}
				satLinkInfo->satStatus[i].dirty = true;
				satLinkInfo.sats[i].statusUpdate(&satLinkInfo->satStatus[i]);
			}
			else if (change) {
				xSemaphoreTake(satLinkInfo->rxStructLock, portMAX_DELAY);
				satLinkInfo->satRxStructs[i].dirty == true; Ä * +satLinkInfo.sats[i].senseUpdate(&satLinkInfo->satRxStructs[i]);
			}
		}
		if (chkErrSec())
			opBlock(SAT_OP_ERR_SEC);
		else
			opDeBlock(SAT_OP_ERR_SEC);
		uint64_t nextTime;
		if (nextTime = (esp_timer_get_time() - t0) / 1000 > satLinkInfo->scanInterval) {
			satLinkInfo->scanTimingViolationErr++;
			satLinkInfo->scanTimingViolationSec++;
		}
		else
			vTaskDelay(nextTime / (portTICK_PERIOD_MS * 1000));
	}
}


/*sateliteLink opBlock*/
void sateliteLink::opBlock(satOpState_t p_opState) {
	if (!satLinkInfo->opState)
		for (uint8_t i = 0; i < noOfSats; i++)
			satLinkInfo->sateliteHandle[i].opBlock(SAT_OP_CONTROLBOCK);
	satLinkInfo->opState = satLinkInfo->opState | p_opState;
	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(satLinkInfo->opState);		// DESIGN MAKE THIS A QUEUE OF CALL BACKS
}


/*sateliteLink opDeBlock*/
void sateliteLink::opDeBlock(satOpState_t p_opState) {
	if (!(satLinkInfo->opState = satLinkInfo->opState & ~p_opState))
		for (uint8_t i = 0; i < noOfSats; i++)
			satLinkInfo->sateliteHandle[i].opDeBlock(SAT_OP_CONTROLBOCK);
	clearSatLinkStats();
	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(satLinkInfo->opState);		// DESIGN MAKE THIS A QUEUE OF CALL BACKS
}


/*sateliteLink admBlock*/
satErr_t sateliteLink::admBlock(void) {
	satelite_t* satInfo;
	if (satLinkInfo->admState == SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	for (uint8_t i = 0; i < noOfSats; i++) {
		satLinkInfo->sateliteHandle[i].getSatInfo(satInfo);
		if (satInfo->admState != SAT_ADM_DISABLE)
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
void chkErrSec(void) {
	uint16_t ErrSum;
	int64_t now;

	if (!satLinkInfo->errThresHigh)
		return;
	ErrSum = satLinkInfo->rxDataSizeErrSec +
		satLinkInfo->symbolRxErrSec +
		satLinkInfo->rxCrcErrSec +
		satLinkInfo->remoteCrcErrSec +
		satLinkInfo->scanTimingViolationErrSec +
		satLinkInfo->wdErrSec;

	if (!(satLinkInfo->opState & SAT_OP_ERR_SEC) && ErrSum >= satLinkInfo->errThresHigh)
		opBlock(SAT_OP_ERR_SEC);

	if ((now = esp_timer_get_time()) - satLinkInfo->oneSecTimer >= ONE_SEC_US) {
		satLinkInfo->oneSecTimer = now;
		if ((satLinkInfo->opState & SAT_OP_ERR_SEC) && ErrSum = < satLinkInfo->errThresLOW)
			opDeBlock(SAT_OP_ERR_SEC);
		satLinkInfo->rxDataSizeErrSec = 0;
		satLinkInfo->symbolRxErrSec = 0;
		satLinkInfo->rxCrcErrSec = 0;
		satLinkInfo->remoteCrcErrSec = 0;
		satLinkInfo->scanTimingViolationErrSec = 0;
		satLinkInfo->wdErrSec = 0;
	}
}
/*----------------------------------------------------- END Class sateliteLink -----------------------------------------------------------------*/


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: satelite                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void filterExp(TimerHandle_t timerHandle) {
	sensor_t* sensor;

	sensor = (sensor_t*)pvTimerGetTimerID(timerHandle);
	if (sensor->currentSensorVal != sensor->filteredSensorVal) {
		sensor->filteredSensorVal = sensor->currentSensorVal;
		sensor->timerActive = false;
		sensor->satObj->satInfo->senseCallback(sensor->satObj->satLink->satLinkInfo->address,
			sensor->satObj->address, sensor->address, sensor->filteredSensorVal);
	}
}



satelite::satelite(sateliteLink* satLink_p, satAddr_t satAddr_p) {
	satInfo = new satelite_t;
	satInfo->address = satAddr_p;
	satInfo->satLink = satLink_p;
	for (uint8_t i = 0; i < 4; i++) {
		satInfo->actMode[i] = #define SATMODE_LOW;
		satInfo->actVal[i] = 0;
	}
	for (uint8_t i = 0; i < 8; i++) {
		satInfo->sensors[i].satObj = this;
		satInfo->sensors[i].addres = i;
		satInfo->sensors[i].filterTime = 0;
		satInfo->sensors[i].senseCb = NULL;
		satInfo->sensors[i].timerHandle = xTimerCreate("SenseTimer", pdMS_TO_TICKS(0), pdFalse, (void*)&satInfo->sensors[i], &filterExp);
		satInfo->timerActive = false
		satInfo->sensors[i].currentSensorVal = false;
		satInfo->sensors[i].filteredSensorVal = false;
	}
	satInfo->stateCallback = NULL;
	satInfo->senseCallback = NULL;
	satInfo->symbolRxErr = 0;
	satInfo->rxDataSizeErr = 0;
	satInfo->rxCrcErr = 0;
	satInfo->remoteCrcErr = 0;
	satInfo->wdErr = 0;
	satInfo->errThresHigh = 0;
	satInfo->errThresLow = 0;
	satInfo->admState = SAT_ADM_DISABLE;
	satInfo->opState = SAT_OP_DISABLE;
	satInfo->oneSecTimer = esp_timer_get_time();
}



satErr_t satelite::~satelite(void) {
	delete satInfo;
}



satErr_t satelite::enableSat(void) {
	return(returnCode(admDeBlock(), 0));
}



satErr_t satelite::disableSat(void) {
	return(returnCode(admBlock(), 0));
}



void satelite::setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow) {
	satInfo->errThresHigh = p_errThresHigh;
	satInfo->errThresLow = p_errThresLow;
}



satErr_t satelite::setSatActMode(satActMode_t actMode_p, uint8_t actIndex_p) {
	if(actIndex_p > 3 || actMode > 5)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->actMode[actIndex_p] = actMode_p;
	satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].actMode[actIndex_p] = actMode_p;
	satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].dirty = true;
	return(returnCode(SAT_OK, 0));
}



satErr_t satelite::setSatActVal(satActMode_t actVal_p, uint8_t actIndex_p) {
	if (actIndex_p > 3)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->actVal[actIndex_p] = actVal_p;
	satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].actVal[actIndex_p] = actVal_p;
	satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].dirty = true;
	return(returnCode(SAT_OK, 0));
}



setErr_t satelite::setSenseFilter(satFilter_t senseFilter_p, uint8_t actIndex_p) {
	if (actIndex_p > 7)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->sensors[actIndex_p].filterTime = pdMS_TO_TICKS(senseFilter_p);
	return(returnCode(SAT_OK, 0));
}



void satelite::getSatInfo(satelite_t* satInfo_p) {
	satInfo_p = satInfo;
}



satErr_t satelite::satRegSenseCb(satSenseCb_t fn) {
	satInfo->senseCallback = fn;
}



satErr_t satelite::satUnRegSenseCb(satSenseCb_t fn) {
	satInfo->senseCallback = NULL;
}



void satelite::satRegStateCb(satStateCb_t fn) {
	satInfo->stateCallback = fn;
}



void satelite::satUnRegStateCb(void) {
	satInfo->stateCallback = NULL;
}



void satelite::senseUpdate(satWire_t* rxData_p) {
	if (satInfo->senseCallback == NULL)
		return;
	uint8_t bitTest = 0b00000001;
	for (uint8_t i = 0; i < 8; i++) {
		if (rxData_p->sensorVal & (bitTest << i))
			satInfo->sensors[i].currentSensorVal = true;
		else
			satInfo->sensors[i].currentSensorVal = false;

		if (satInfo->sensors[i].timerActive) {
			ASSERT(xTimerStop(satInfo->sensors[i].timerHandle, 10 != pdPASS);
			satInfo->sensors[i].timerActive = false;
		}

		if (satInfo->sensors[i].currentSensorVal != satInfo->sensors[i].currentSensorVal) {
			ASSERT(xTimerStart(satInfo->sensors[i].timerHandle, 10) != pdPASS);
			satInfo->sensors[i].timerActive = true;
		}
	}
}



void satelite::statusUpdate(satStatus_t* status) {
	if (status->dirty != true)
		return;
	if (wdErr) {
		satInfo->wdErr++;
		satInfo->wdErrSec++;
	}
	if (remoteCrcErr) {
		satInfo->remoteCrcErr++;
		satInfo->remoteCrcErrSec++;
	}
	if (rxCrcErr) {
		satInfo->rxCrcErr++;
		satInfo->rxCrcErrSec++;
	}
	if (rxSymbolErr) {
		satInfo->rxSymbolErr++;
		satInfo->rxSymbolErrSec++;
	}
	if (rxDataSizeErr) {
		satInfo->rxDataSizeErr++;
		satInfo->rxDataSizeErrSec++;
	}


}



/*satelite opBlock*/
void satelite::opBlock(satOpState_t p_opState) {
	if (!satInfo->opState) {
		satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].enable = false;
		satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].dirty = true;
	}
		satInfo->opState = satInfo->opState | p_opState;
	if (satInfo->stateCallback != NULL)
		satInfo->stateCallback(satInfo->opState);		// DESIGN MAKE THIS A QUEUE OF CALL BACKS
}



/*sateliteLink opDeBlock*/
void satelite::opDeBlock(satOpState_t p_opState) {
	if (!(satInfo->opState = satInfo->opState & ~p_opState)){
		satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].enable = true;
		satInfo->satLink->satLinkInfo->satTxStructs[satInfo->address].dirty = true;
	}
	clearSatStats();
	if (satInfo->stateCallback != NULL)
		satInfo->stateCallback(satInfo->opState);		// DESIGN MAKE THIS A QUEUE OF CALL BACKS
}



/*sateliteLink admBlock*/
satErr_t satelite::admBlock(void) {
	if(satInfo->admState == SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	satInfo->admState = SAT_ADM_DISABLE;
	opBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}



/*sateliteLink admDeBlock*/
satErr_t satelite::admDeBlock(void) {
	if (satInfo->admState == SAT_ADM_ENABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	satInfo->admState = SAT_ADM_ENABLE;
	opDeBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}


void satelite::clearSatStats(void) {
	satInfo->symbolRxErr = 0;
	satInfo->symbolRxErrSec = 0;
	satInfo->rxDataSizeErr = 0;
	satInfo->rxDataSizeErrSec = 0;
	satInfo->rxCrcErr = 0;
	satInfo->rxCrcErrSec = 0;
	satInfo->remoteCrcErr = 0;
	satInfo->remoteCrcErrSec = 0;
	satInfo->wdErr = 0;
	satInfo->wdErrSec = 0;
}


void satelite::chkErrSec(void) {
	uint16_t ErrSum;
	int64_t now;

	if (!satInfo->errThresHigh)
		return;
	ErrSum = satInfo->rxDataSizeErrSec +
		satInfo->symbolRxErrSec +
		satInfo->rxCrcErrSec +
		satInfo->remoteCrcErrSec +
		satInfo->scanTimingViolationErrSec +
		satInfo->wdErrSec;

	if (!(satInfo->opState & SAT_OP_ERR_SEC) && ErrSum >= satInfo->errThresHigh)
		opBlock(SAT_OP_ERR_SEC);

	if ((now = esp_timer_get_time()) - satInfo->oneSecTimer >= ONE_SEC_US) {
		satInfo->oneSecTimer = now;
		if ((satInfo->opState & SAT_OP_ERR_SEC) && ErrSum = < satInfo->errThresLow)
			opDeBlock(SAT_OP_ERR_SEC);
		satInfo->rxDataSizeErrSec = 0;
		satInfo->symbolRxErrSec = 0;
		satInfo->rxCrcErrSec = 0;
		satInfo->remoteCrcErrSec = 0;
		satInfo->scanTimingViolationErrSec = 0;
		satInfo->wdErrSec = 0;
	}
}