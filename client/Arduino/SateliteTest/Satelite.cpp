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




/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function:                                                                                 */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satErr_t opStateToStr(satOpState_t opState_p, char* outputStr_p, uint8_t length_p){
  uint8_t length = 0;
  
  if(opState_p == SAT_OP_WORKING) {
    if(length_p < sizeof(SAT_OP_WORKING_STR))
      return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p, "%s", SAT_OP_WORKING_STR);
    length += sizeof(SAT_OP_WORKING_STR)-1;
  }
  if(opState_p & SAT_OP_INIT){
    if(length_p < length + sizeof(SAT_OP_INIT_STR) + 1)
       return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p + length, "%s%s", (length==0) ? "":",", SAT_OP_INIT_STR);
    length += sizeof(SAT_OP_INIT_STR) - (length == 0);
  }
  if(opState_p & SAT_OP_DISABLE){
    if(length_p < length + sizeof(SAT_OP_DISABLE_STR) + 1)
       return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p + length, "%s%s", (length==0) ? "":",", SAT_OP_DISABLE_STR);
    length += sizeof(SAT_OP_DISABLE_STR) - (length == 0);
  }
  if(opState_p & SAT_OP_CONTROLBOCK){
    if(length_p < length + sizeof(SAT_OP_CONTROLBOCK_STR) + 1)
       return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p + length, "%s%s", (length==0) ? "":",", SAT_OP_CONTROLBOCK_STR);
    length += sizeof(SAT_OP_CONTROLBOCK_STR) - (length == 0);
  }
  if(opState_p & SAT_OP_ERR_SEC){
    if(length_p < length + sizeof(SAT_OP_ERR_SEC_STR) + 1)
       return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p + length, "%s%s", (length==0) ? "":",",SAT_OP_ERR_SEC_STR);
    length += sizeof(SAT_OP_ERR_SEC_STR) - (length == 0);
  }
  if(opState_p & SAT_OP_FAIL){
    if(length_p < length + sizeof(SAT_OP_FAIL_STR) + 1)
       return SAT_ERR_BUFF_SMALL_ERR;
    sprintf(outputStr_p + length, "%s%s", (length==0) ? "":",",SAT_OP_FAIL_STR);
    length += sizeof(SAT_OP_FAIL_STR) - (length == 0);
  }
  for (uint8_t i = 0; i < length_p - length; i++)
        *(outputStr_p + length + i) = 32;
  return SAT_OK;
}

satErr_t formatSatStat(char* reportBuffer_p, uint16_t buffSize_p, uint16_t* usedBuff_p, uint16_t buffOffset_p, uint8_t linkAddr_p,
                        uint8_t satAddr_p, satAdmState_t admState_p, satOpState_t opState_p, satPerformanceCounters_t* pmdata_p, uint16_t reportColumnItems,
                        uint16_t reportItemsMask, bool printHead){
  uint16_t size = 0;

  if (printHead){
    if (reportColumnItems & LINK_ADDR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "LinkAddr:");
      size += 11;
    }
    if (reportColumnItems & SAT_ADDR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-10s", "SatAddr:");
      size += 10;
    }
    if (reportColumnItems & RX_SIZE_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-10s", "Sizeerr:");
      size += 10;
    }
    if (reportColumnItems & RX_SYMB_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-12s", "Symbolerr:");
      size += 12;
    }
    if (reportColumnItems & TIMING_VIOLATION_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-12s", "Timingerr:");
      size += 12;
    }
    if (reportColumnItems & TX_UNDERRUN_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-14s", "Underrunerr:");
      size += 14;
    }
  
    if (reportColumnItems & RX_OVERRRUN_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-13s", "Overrunerr:");
      size += 13;
    }
    if (reportColumnItems & RX_CRC_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "RxCRCerr:");
      size += 11;
    }
    if (reportColumnItems & REMOTE_CRC_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "RmCRCerr:");
      size += 11;
    }
    if (reportColumnItems & WATCHDG_ERR) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-8s", "Wderr:");
      size += 8;
    }
    if (reportColumnItems & ADM_STATE) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "AdmState:");
      size += 11;
    }
    if (reportColumnItems & OP_STATE) {
      sprintf(reportBuffer_p + buffOffset_p + size,"%-40s", "OpState:");
      size += 25;
    }
    
    sprintf(reportBuffer_p + buffOffset_p + size,"\n");
    size++;
  }
  
  if (reportColumnItems & LINK_ADDR) {
      if (reportItemsMask & LINK_ADDR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-11u", linkAddr_p);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "-" );
     
      size += 11;
  }
  if (reportColumnItems & SAT_ADDR) {
      if (reportItemsMask & SAT_ADDR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-10u", satAddr_p);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-10s", "-" );
     
      size += 10;
  }
  if (reportColumnItems & RX_SIZE_ERR) {
      if (reportItemsMask & RX_SIZE_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-10u", pmdata_p->rxDataSizeErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-10s", "-" );
      size += 10;
  }
  if (reportColumnItems & RX_SYMB_ERR) {
      if (reportItemsMask & RX_SYMB_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-12u", pmdata_p->rxSymbolErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-12s", "-" );
      size += 12;
  } 
  if (reportColumnItems & TIMING_VIOLATION_ERR) {
      if (reportItemsMask & TIMING_VIOLATION_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-12u", pmdata_p->scanTimingViolationErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-12s", "-" );
      size += 12;
  } 
  if (reportColumnItems & TX_UNDERRUN_ERR) {
      if (reportItemsMask & TX_UNDERRUN_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-14u", pmdata_p->txUnderunErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-14s", "-" );
      size += 14;
  } 
  if (reportColumnItems & RX_OVERRRUN_ERR) {
      if (reportItemsMask & RX_OVERRRUN_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-13u", pmdata_p->rxOverRunErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-13s", "-" );
      size += 13;
  }
  if (reportColumnItems & RX_CRC_ERR) {
      if (reportItemsMask & RX_CRC_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-11u", pmdata_p->rxCrcErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "-" );
      size += 11;
  } 
  if (reportColumnItems & REMOTE_CRC_ERR) {
      if (reportItemsMask & REMOTE_CRC_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-11u", pmdata_p->remoteCrcErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "-" );
      size += 11;
  }
  if (reportColumnItems & WATCHDG_ERR) {
      if (reportItemsMask & WATCHDG_ERR)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-8u", pmdata_p->wdErr);
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-8s", "-" );
      size += 8;
  }
  if (reportColumnItems & ADM_STATE) {
      if (reportItemsMask & ADM_STATE)
        sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", admState_p ? "DISABLE" : "ENABLE");
      else
         sprintf(reportBuffer_p + buffOffset_p + size,"%-11s", "-" );
      size += 11;
  }
  if (reportColumnItems & OP_STATE) {
    if (reportItemsMask & OP_STATE)
      opStateToStr(opState_p, reportBuffer_p + buffOffset_p + size, 40);
    else
      sprintf(reportBuffer_p + buffOffset_p + size,"%-40s", "-" );
    size += 25;
  } 
  
  sprintf(reportBuffer_p + buffOffset_p + size,"\n");
  size ++;
  sprintf(reportBuffer_p + buffOffset_p + size,"\0");
  *usedBuff_p += size;
  return SAT_OK;
}


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: ws28xx_rmt_tx_translator & ws28xx_rmt_rx_translator                                                                                */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
static void IRAM_ATTR ws28xx_rmt_tx_translator(const void* src, rmt_item32_t* dest, size_t src_size, size_t wanted_num, 
            size_t* translated_size, size_t* item_num) {
  
  //Serial.println("Encoding");
           
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
    //Serial.println("Symbol ERR-1");
    return SAT_ERR_SYMBOL_ERR;
  }
  for (uint16_t i = 0; i < len / 8; i++) {
    dest[i] = 0;
    for (uint8_t j = 8; j > 0; j--) {
      //Serial.printf("Duration 0: %d\n", src[i * 8 + 8 - j].duration0);
      if (src[i * 8 + 8 - j].duration0 >= WS28XX_T1H_CYC_MIN && src[i * 8 + 8 - j].duration0 <= WS28XX_T1H_CYC_MAX)
        dest[i] = dest[i] | (0x01 << j - 1);
      else if(src[i * 8 + 8 - j].duration0 < WS28XX_T0H_CYC_MIN || src[i * 8 + 8 - j].duration0 > WS28XX_T0H_CYC_MAX) {
        //Serial.println("Symbol ERR-2");
        return SAT_ERR_SYMBOL_ERR;
      }
      else if ((i != 7 && j != 1) && (src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 < WS28XX_CYC_CYC_MIN ||
           src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1 > WS28XX_CYC_CYC_MAX)){
        //Serial.printf("Symbol ERR-3, cycle: %d, i=%d j=%d\n", src[i * 8 + 8 - j].duration0 + src[i * 8 + 8 - j].duration1,i,j);
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
	satWireBuff_p[6] = satWireBuff_p[6] | (satWireStruct_p->cmdWdErr & 0x01) << 3;
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
	satWireStruct_p->cmdWdErr = 0x00 | (satWireBuff_p[6] & 0x08) >> 3;
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
void clearPerformanceCounters(satPerformanceCounters_t* performanceCounters_p) {
  //Serial.printf("Clearing stats\n");
  performanceCounters_p->txUnderunErr = 0;
  performanceCounters_p->txUnderunErrSec = 0;
  performanceCounters_p->rxOverRunErr = 0;
  performanceCounters_p->rxOverRunErrSec = 0;
  //Serial.printf("Clearing Timing violation counters\n");
  performanceCounters_p->scanTimingViolationErr = 0;
  performanceCounters_p->scanTimingViolationErrSec = 0;
  performanceCounters_p->rxDataSizeErr = 0;
  performanceCounters_p->rxDataSizeErrSec = 0;
  performanceCounters_p->rxSymbolErr = 0;
  performanceCounters_p->rxSymbolErrSec = 0;
  performanceCounters_p->wdErr = 0;
  performanceCounters_p->wdErrSec = 0;
  performanceCounters_p->rxCrcErr = 0;
  performanceCounters_p->rxCrcErrSec = 0;
  performanceCounters_p->remoteCrcErr = 0;
  performanceCounters_p->remoteCrcErrSec = 0;
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
  satLinkInfo->performanceCounterLock = xSemaphoreCreateMutex(); // needed for asynchronous read-modify-write
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
	satLinkInfo->errThresHigh = 20;
	satLinkInfo->errThresLow = 2;
	satLinkInfo->noOfSats = 0;
	satLinkInfo->opState = SAT_OP_DISABLE|SAT_OP_INIT;
	satLinkInfo->admState = SAT_ADM_DISABLE;
	satLinkInfo->satLinkStateCb = NULL;
  satLinkInfo->satDiscoverCb = NULL;
	satLinkInfo->serverCrcTst = SAT_CRC_TEST_INACTIVE;
  satLinkInfo->clientCrcTst = SAT_CRC_TEST_INACTIVE;
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++){
		satLinkInfo->sateliteHandle[i] = NULL;
    satLinkInfo->txSatStruct[i].enable = false;
    satLinkInfo->txSatStruct[i].setWdErr = false;
    satLinkInfo->txSatStruct[i].invServerCrc = false;
    satLinkInfo->txSatStruct[i].invClientCrc = false;
    satLinkInfo->txSatStruct[i].sensorVal = 0x00;
    satLinkInfo->txSatStruct[i].actVal[0] = 0x00;
    satLinkInfo->txSatStruct[i].actVal[1] = 0x00;
    satLinkInfo->txSatStruct[i].actVal[2] = 0x00;
    satLinkInfo->txSatStruct[i].actVal[3] = 0x00;
    satLinkInfo->txSatStruct[i].actMode[0] = SATMODE_LOW;
    satLinkInfo->txSatStruct[i].actMode[1] = SATMODE_LOW;
    satLinkInfo->txSatStruct[i].actMode[2] = SATMODE_LOW;
    satLinkInfo->txSatStruct[i].actMode[3] = SATMODE_LOW;
    satLinkInfo->txSatStruct[i].cmdWdErr = 0x00;
    satLinkInfo->txSatStruct[i].cmdEnable = 0x00;
    satLinkInfo->txSatStruct[i].cmdInvCrc = 0x00;
    satLinkInfo->txSatStruct[i].startMark = 0x00;
    satLinkInfo->txSatStruct[i].fbReserv = 0x00;
    satLinkInfo->txSatStruct[i].fbWdErr = 0x00;
    satLinkInfo->txSatStruct[i].fbRemoteCrcErr = 0x00;
    satLinkInfo->txSatStruct[i].dirty = true;
	}
	satLinkInfo->scan = false;
	clearPerformanceCounters(&satLinkInfo->performanceCounters);
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
	rmtRxConfig.rx_config.filter_en = true;
	rmtRxConfig.rx_config.filter_ticks_thresh = 5;
	rmtRxConfig.rx_config.idle_threshold = WS28XX_GUARD_CYC;
	rmt_config(&rmtRxConfig);
	rmt_driver_install(satLinkInfo->rxCh, 10000, 0);
	rmt_get_ringbuf_handle(satLinkInfo->rxCh, &satLinkInfo->rb);
  satLinkInfo->linkReEstablishTimerHandle = xTimerCreate("LinkReEstablishTimer", pdMS_TO_TICKS(T_REESTABLISH_LINK_MS), pdFALSE, (void*)this, &linkReEstablish);
}


/*sateliteLink Destructur*/
sateliteLink::~sateliteLink(void) {
	satLinkStopScan();
	assert(satLinkInfo->opState == SAT_OP_DISABLE);
	assert(rmt_driver_uninstall(satLinkInfo->txCh) == ESP_OK);
	assert(rmt_driver_uninstall(satLinkInfo->rxCh) == ESP_OK);
  vSemaphoreDelete(satLinkInfo->performanceCounterLock);
	delete satLinkInfo;
}


/*sateliteLink enableSatLink*/
satErr_t sateliteLink::enableSatLink(void){
    //Serial.printf("Check sateliteHandle-3 satLinkInfo->sateliteHandle[8], value is now %i\n", satLinkInfo->sateliteHandle[8]);

	esp_err_t rmtRc;
	satErr_t rc;
  
	if(satLinkInfo->admState != SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	if(rmtRc = rmt_rx_start(satLinkInfo->rxCh, 1) != ESP_OK)
		return (returnCode(SAT_ERR_RMT_ERR, rmtRc));
	if (rc = admDeBlock())
		return (returnCode(rc, 0));
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		satLinkInfo->txSatStruct[i].invServerCrc = false;
		satLinkInfo->txSatStruct[i].invClientCrc = false;
		satLinkInfo->txSatStruct[i].enable = false;
		satLinkInfo->txSatStruct[i].dirty = true;
	}
	if(rc = satLinkStartScan())
		return (returnCode(rc, 0));
  
  vTaskDelay(500 / portTICK_PERIOD_MS);
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
		if (satLinkInfo->sateliteHandle[i] != NULL){
      if (satLinkInfo->satDiscoverCb != NULL)
        satLinkInfo->satDiscoverCb(satLinkInfo->sateliteHandle[i], satLinkInfo->address, i, false);
			delete satLinkInfo->sateliteHandle[i];
      satLinkInfo->sateliteHandle[i] = NULL;
		}
    return (returnCode(SAT_OK, 0));
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


void sateliteLink::satLinkRegSatDiscoverCb(satDiscoverCb_t satDiscoverCb_p) {
  //Serial.printf("Got a satLinkRegSatDiscoverCb registration\n");
  satLinkInfo->satDiscoverCb = satDiscoverCb_p;
}


void sateliteLink::satLinkUnRegSatDiscoverCb(void) {
  satLinkInfo->satDiscoverCb = NULL;
}

uint8_t sateliteLink::getAddress(void){
  return satLinkInfo->address;
}
/*sateliteLink getSatLinkNoOfSats*/
uint8_t sateliteLink::getSatLinkNoOfSats(void) {
	return satLinkInfo->noOfSats;
}


satErr_t sateliteLink::getSensorValRaw(uint8_t satAddress_p, uint8_t* sensorVal_p){
  if (satAddress_p >= satLinkInfo->noOfSats)
    return (returnCode(SAT_ERR_NOT_EXIST_ERR, 0));
  *sensorVal_p = satLinkInfo->rxSatStruct[satAddress_p].sensorVal;
  return (returnCode(SAT_OK, 0));
}


void sateliteLink::getSatStats(satPerformanceCounters_t* satStats_p, bool resetStats) {
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
  memcpy((void*)satStats_p, (void*)&(satLinkInfo->performanceCounters), sizeof(satPerformanceCounters_t));
  if (resetStats)
    clearPerformanceCounters(&satLinkInfo->performanceCounters);
  xSemaphoreGive(satLinkInfo->performanceCounterLock);
}


void sateliteLink::clearSatStats(void) {
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
  clearPerformanceCounters(&satLinkInfo->performanceCounters);
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

  //Serial.printf("Running discovery\n");
  opBlock(SAT_OP_INIT);
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
		if (satLinkInfo->sateliteHandle[i] != NULL) {
      //Serial.printf("Deleting Sat %d\n", i);
      if (satLinkInfo->satDiscoverCb != NULL)
        satLinkInfo->satDiscoverCb(satLinkInfo->sateliteHandle[i], satLinkInfo->address, i, false);
      satTmp = satLinkInfo->sateliteHandle[i];
      satLinkInfo->sateliteHandle[i] = NULL;
			delete satTmp;
		}
   satLinkInfo->satStatus[i].dirty = false;
	}
  satLinkInfo->noOfSats = MAX_NO_OF_SAT_PER_CH + 1;       //Scan one more sat than max
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    //Serial.printf("Ask for inverting CRC for sat %d\n", i);
    
		satLinkInfo->txSatStruct[i].invServerCrc = true;
		satLinkInfo->txSatStruct[i].dirty = true;
	}
	vTaskDelay(1000 / portTICK_PERIOD_MS);
  for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++)
    satLinkInfo->satStatus[i].dirty = false;
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  if (satLinkInfo->satStatus[MAX_NO_OF_SAT_PER_CH].remoteCrcErr) {   //Checking sat MAX_NO_OF_SAT_PER_CH + 1 for remote CRC 
    satLinkInfo->noOfSats = 0;
    opBlock(SAT_OP_FAIL);
    opDeBlock(SAT_OP_INIT);
    //Serial.printf("SAT_ERR_EXESSIVE_SATS\n");
		return (returnCode(SAT_ERR_EXESSIVE_SATS, 0));
	}

	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
		bool endOfSat = false;
		if (!satLinkInfo->satStatus[i].remoteCrcErr){
			endOfSat = true;
      //Serial.printf("EndSatelite %d\n", i);
		}
		if (satLinkInfo->satStatus[i].remoteCrcErr && endOfSat) {
      satLinkInfo->noOfSats = 0;
      opBlock(SAT_OP_FAIL);
      opDeBlock(SAT_OP_INIT);
      //Serial.printf("SAT_ERR_GEN_SATLINK_ERR\n");
			return (returnCode(SAT_ERR_GEN_SATLINK_ERR, 0));
		}
	}
	for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    satLinkInfo->noOfSats = i;
		if (!satLinkInfo->satStatus[i].remoteCrcErr)
      break;
	}  
  for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH + 1; i++) {
    satLinkInfo->txSatStruct[i].invServerCrc = false;
    satLinkInfo->txSatStruct[i].dirty = true;
  }
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
    //Serial.printf("Creating satelite %d\n", i);
		satLinkInfo->sateliteHandle[i] = new satelite(this, i);
    if (satLinkInfo->satDiscoverCb != NULL){
      satLinkInfo->satDiscoverCb(satLinkInfo->sateliteHandle[i], satLinkInfo->address, i, true);
      //Serial.printf("Sent a discovery callback message for satelite %d\n", i);
    }
    //else
      //Serial.printf("Could not Send a discovery callback message for satelite %d, no callback registered\n", i);
  }
  if (satLinkInfo->noOfSats){
      opDeBlock(SAT_OP_FAIL);
      opDeBlock(SAT_OP_INIT);
      //Serial.printf("FOUND SATELITES\n");
  }
  else {
      //Serial.printf("DID NOT FIND ANY SATELITES\n");
      opBlock(SAT_OP_FAIL);
      opDeBlock(SAT_OP_INIT);
  }
  //Serial.printf("End discovery, %d Satelites discovered\n", satLinkInfo->noOfSats);
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
  satLinkInfo_t* satLinkInfo = ((sateliteLink*)satLinkObj)->satLinkInfo;
  int64_t t0;
  bool rxSymbolErr = false;
  bool rxDataSizeErr = false;
  uint16_t tot_rx_size = 0;
  rmt_item32_t* items = NULL;
  size_t rx_size = 0;
  uint8_t index = 0;
  uint8_t crcCalc;
  int64_t nextTime;

  while(true) {
    t0 = esp_timer_get_time();
    for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
    //for (uint8_t i = 0; i < 1; i++) {
      //Serial.printf("Satelite %d, Inv Server CRC Cmd: %s\n", i, satLinkInfo->txSatStruct[i].invServerCrc ? "true" : "false");                        
      if (i == 0)
        satLinkInfo->txSatStruct[i].startMark = 0x01;
      else
        satLinkInfo->txSatStruct[i].startMark = 0x00;
    
      if (satLinkInfo->txSatStruct[i].dirty == true) {
        if (satLinkInfo->txSatStruct[i].invServerCrc){
          //Serial.printf("Server CRC Inv ordered\n");
          satLinkInfo->serverCrcTst = SAT_CRC_TEST_ACTIVE;
        }
        else if (!satLinkInfo->txSatStruct[i].invServerCrc && satLinkInfo->serverCrcTst == SAT_CRC_TEST_ACTIVE)
          satLinkInfo->serverCrcTst = SAT_CRC_TEST_DEACTIVATING;
        else if (satLinkInfo->serverCrcTst != SAT_CRC_TEST_ACTIVE && satLinkInfo->serverCrcTst != SAT_CRC_TEST_INACTIVE)
          satLinkInfo->serverCrcTst--;
                
        if (satLinkInfo->txSatStruct[i].invClientCrc) {
          //Serial.printf("Client CRC Inv ordered\n");
          satLinkInfo->clientCrcTst = SAT_CRC_TEST_ACTIVE;
          satLinkInfo->txSatStruct[i].cmdInvCrc = true;
        }
        else if (!satLinkInfo->txSatStruct[i].invClientCrc && satLinkInfo->clientCrcTst == SAT_CRC_TEST_ACTIVE){
          satLinkInfo->clientCrcTst = SAT_CRC_TEST_DEACTIVATING;
          satLinkInfo->txSatStruct[i].cmdInvCrc = false;
        }
        else if (satLinkInfo->clientCrcTst != SAT_CRC_TEST_ACTIVE && satLinkInfo->clientCrcTst != SAT_CRC_TEST_INACTIVE)
          satLinkInfo->clientCrcTst--;
          
        if (satLinkInfo->txSatStruct[i].setWdErr) {
          //Serial.printf("WD err ordered\n");
          satLinkInfo->wdTst = SAT_WD_TEST_ACTIVE;
          satLinkInfo->txSatStruct[i].cmdWdErr = true;
        }
        else if (!satLinkInfo->txSatStruct[i].setWdErr && satLinkInfo->wdTst == SAT_WD_TEST_ACTIVE){
          satLinkInfo->wdTst = SAT_WD_TEST_DEACTIVATING;
          satLinkInfo->txSatStruct[i].cmdWdErr = false;
        }
        else if (satLinkInfo->wdTst != SAT_WD_TEST_ACTIVE && satLinkInfo->wdTst != SAT_WD_TEST_INACTIVE)
          satLinkInfo->wdTst--;

        if (satLinkInfo->txSatStruct[i].enable)
          satLinkInfo->txSatStruct[i].cmdEnable = 0x01;
        else
          satLinkInfo->txSatStruct[i].cmdEnable = 0x00;

        populateSatLinkBuff(&satLinkInfo->txSatStruct[i], &satLinkInfo->txSatBuff[i * 8]);
        crc(&satLinkInfo->txSatBuff[i * 8 + SATBUF_CRC_BYTE_OFFSET], &satLinkInfo->txSatBuff[i * 8],
            8, (satLinkInfo->txSatStruct[i].invServerCrc == 0x01));
        satLinkInfo->txSatStruct[i].dirty = false;
      }
      //Serial.printf("Transmitbuffer for Satelite %d : ", i);
      //for(uint8_t m = 0; m < 8; m++)
        //Serial.printf("%x:", satLinkInfo->txSatBuff[i * 8 + m]);
      //Serial.printf("\n");
    }
    for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
    //for (uint8_t i = 0; i < 1; i++) { 
      rmt_write_sample(satLinkInfo->txCh, &satLinkInfo->txSatBuff[i * 8], (size_t)8, false);
    }

    tot_rx_size = 0;
    uint64_t titem, titemstop;
    //titem = esp_timer_get_time();

    index = 0;
    while (true) {
      items = (rmt_item32_t*)xRingbufferReceive(satLinkInfo->rb, &rx_size, 1);
      if (items == NULL) {
        if (tot_rx_size != satLinkInfo->noOfSats * 8 * 8) {
        //if (tot_rx_size != 8 * 8) {
          rxDataSizeErr = true;
          //Serial.printf("Size ERR, size: %d, number of sats: %d\n", tot_rx_size, satLinkInfo->noOfSats);
          break;
        }
        else
          break;
      }
      else {
        tot_rx_size += rx_size / 4;
        if (ws28xx_rmt_rx_translator(items, &satLinkInfo->rxSatBuff[index * 8], rx_size / 4)) {
          vRingbufferReturnItem(satLinkInfo->rb, (void*)items);
          rxSymbolErr = true;
          while (items = (rmt_item32_t*)xRingbufferReceive(satLinkInfo->rb, &rx_size, 10))
            vRingbufferReturnItem(satLinkInfo->rb, (void*)items);
          break;
        }
        vRingbufferReturnItem(satLinkInfo->rb, (void*)items);
        //Serial.printf("Receivebuffer for Satelite %d : ", index);
        //for(uint8_t m = 0; m < 8; m++)
          //Serial.printf("%x:", satLinkInfo->rxSatBuff[index * 8 + m]);
        //Serial.printf("\n");
      }
      index++;
    }
    //titemstop = esp_timer_get_time()- titem;
    //Serial.printf("Took %d uS to retrieve items \n", titemstop);


    for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
    //for (uint8_t i = 0; i < 1; i++) {                                                               // Decode the RX buffer
      bool senseChange;
      bool statusBad;
      bool rxCrcErr;
      bool remoteCrcErr;
      bool wdErr;
      
      senseChange = populateSatWireStruct(&satLinkInfo->rxSatStruct[i], &satLinkInfo->rxSatBuff[i * 8]); // Populate the Satelite rx struct
      crc(&crcCalc, &satLinkInfo->rxSatBuff[i*8], 8, false);                                        // Calculate incomming RX buffer CRC
      rxCrcErr  = (crcCalc != satLinkInfo->rxSatStruct[i].crc);
      remoteCrcErr = (satLinkInfo->rxSatStruct[i].fbRemoteCrcErr == 0x01);
      wdErr = (satLinkInfo->rxSatStruct[i].fbWdErr == 0x01);
      //Serial.printf("%s , %s\n", satLinkInfo->satStatus[i].remoteCrcErr ? "true" : "false", remoteCrcErr ? "true" : "false");
      //if (satLinkInfo->satStatus[i].remoteCrcErr != remoteCrcErr)
        //printf("Remote CRC error status changed for satelite %d, now %s\n", i, remoteCrcErr ? "true" : "false");
      statusBad = (rxDataSizeErr || rxSymbolErr || rxCrcErr || remoteCrcErr || wdErr);

      //Serial.printf("Satelite %d Sensors: %x\n", i, satLinkInfo->rxSatStruct[i].sensorVal);
      //Serial.printf("Satelite %d Status - rxCrcErr: %s, remoteCrcErr: %s, wdErr: %s \n", i, rxCrcErr ? "true" : "false", remoteCrcErr ? "true" : "false"  , wdErr ? "true" : "false");

      if (statusBad) {
        xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
        satLinkInfo->performanceCounters.rxDataSizeErr += rxDataSizeErr;
        satLinkInfo->performanceCounters.rxDataSizeErrSec += rxDataSizeErr;
        satLinkInfo->performanceCounters.rxSymbolErr += rxSymbolErr;
        satLinkInfo->performanceCounters.rxSymbolErrSec += rxSymbolErr;
        
        if (satLinkInfo->serverCrcTst == SAT_CRC_TEST_INACTIVE){
          satLinkInfo->performanceCounters.remoteCrcErr += remoteCrcErr;
          satLinkInfo->performanceCounters.remoteCrcErrSec += remoteCrcErr;
        }
        
        if (satLinkInfo->clientCrcTst == SAT_CRC_TEST_INACTIVE){
          satLinkInfo->performanceCounters.rxCrcErr += rxCrcErr;
          satLinkInfo->performanceCounters.rxCrcErrSec += rxCrcErr;
        }
        
        if (satLinkInfo->wdTst == SAT_WD_TEST_INACTIVE){
          satLinkInfo->performanceCounters.wdErr += wdErr;
          satLinkInfo->performanceCounters.wdErrSec += wdErr;
        }
        xSemaphoreGive(satLinkInfo->performanceCounterLock);

        //Serial.printf("Bad status:  dirty %i, rxDataSizeErr %i, rxSymbolErr %i, rxCrcErr %i, remoteCrcErr %i, wdErr %i\n", satLinkInfo->satStatus[i].dirty, rxDataSizeErr, rxSymbolErr, rxCrcErr, remoteCrcErr, wdErr);
        if (!satLinkInfo->satStatus[i].dirty) {               //If satStatus has been read by the receiver, clear all status data
          satLinkInfo->satStatus[i].rxDataSizeErr = rxDataSizeErr;
          satLinkInfo->satStatus[i].rxSymbolErr = rxSymbolErr;
          satLinkInfo->satStatus[i].rxCrcErr = rxCrcErr;
          satLinkInfo->satStatus[i].remoteCrcErr = remoteCrcErr;
          //TESTCODE
          if (i == 0) {
            //Serial.printf("Dirty was false and remoteCRC is %s, stored Remote CRC %s\n", remoteCrcErr ? "true":"false", satLinkInfo->satStatus[i].remoteCrcErr ? "true":"false");
          }
          satLinkInfo->satStatus[i].wdErr = wdErr;
        }
        else {
          if (!satLinkInfo->satStatus[i].rxDataSizeErr)
            satLinkInfo->satStatus[i].rxDataSizeErr = rxDataSizeErr;
          if (!satLinkInfo->satStatus[i].rxSymbolErr)
            satLinkInfo->satStatus[i].rxSymbolErr = rxSymbolErr;
          if (!satLinkInfo->satStatus[i].rxCrcErr)
            satLinkInfo->satStatus[i].rxCrcErr = rxCrcErr;
          if (!satLinkInfo->satStatus[i].remoteCrcErr)
            satLinkInfo->satStatus[i].remoteCrcErr = remoteCrcErr;
            //Serial.printf("After 2 %s , %s\n", satLinkInfo->satStatus[i].remoteCrcErr ? "true" : "false", remoteCrcErr ? "true" : "false");
          // TESTCODE
          if (i == 0) {
            //Serial.printf("Dirty was true and remoteCRC is %s, stored Remote CRC %s\n", remoteCrcErr ? "true":"false", satLinkInfo->satStatus[i].remoteCrcErr ? "true":"false");
          }
          if (!satLinkInfo->satStatus[i].wdErr)
            satLinkInfo->satStatus[i].wdErr = wdErr;     
        }
        satLinkInfo->satStatus[i].dirty = true;
        if(satLinkInfo->sateliteHandle[i] != NULL)
          satLinkInfo->sateliteHandle[i]->statusUpdate(&satLinkInfo->satStatus[i]);
      }
      if (senseChange && (satLinkInfo->sateliteHandle[i] != NULL) && !satLinkInfo->opState && !rxDataSizeErr && !rxSymbolErr && !rxCrcErr)
        satLinkInfo->sateliteHandle[i]->senseUpdate(&satLinkInfo->rxSatStruct[i]);
        //Serial.printf("After 3 %s , %s\n", satLinkInfo->satStatus[i].remoteCrcErr ? "true" : "false", remoteCrcErr ? "true" : "false");

        rxDataSizeErr = false;
        rxSymbolErr = false;
        remoteCrcErr = false;
        rxCrcErr = false;
        remoteCrcErr = false;
        wdErr = false;
    }
    ((sateliteLink*)satLinkObj)->chkErrSec();
    if ((nextTime = satLinkInfo->scanInterval - ((esp_timer_get_time() - t0) / 1000)) <= 0) {
      //Serial.printf("Overrun next time %d ms\n", nextTime);
      //Serial.printf("Scan ended-1 took %d us\n", esp_timer_get_time() - t0);
      xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
      satLinkInfo->performanceCounters.scanTimingViolationErr++;
      satLinkInfo->performanceCounters.scanTimingViolationErrSec++;
      xSemaphoreGive(satLinkInfo->performanceCounterLock);
    }
    else{
      //Serial.printf("Scan ended-2 time %d us\n", esp_timer_get_time());
      //Serial.printf("Delaying next scan %d ms, now:%llu \n", nextTime, esp_timer_get_time());
      vTaskDelay(nextTime / (portTICK_PERIOD_MS));
    }
  }
}


/*sateliteLink opBlock*/
void sateliteLink::opBlock(satOpState_t opState_p) {
  //Serial.printf("Satelitelink OP blocked received 0x%x\n", opState_p);
	if (!satLinkInfo->opState)
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++)
      if (satLinkInfo->sateliteHandle[i] != NULL)
			  satLinkInfo->sateliteHandle[i]->opBlock(SAT_OP_CONTROLBOCK);
	satLinkInfo->opState = satLinkInfo->opState | opState_p;
	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(this, satLinkInfo->address, satLinkInfo->opState);

  if (opState_p & SAT_OP_FAIL || opState_p & SAT_OP_ERR_SEC){
     assert(xTimerStart(satLinkInfo->linkReEstablishTimerHandle, 10) != pdFAIL);
     //Serial.printf("Starting Re-establish timer\n");
  }
}


/*sateliteLink opDeBlock*/
void sateliteLink::opDeBlock(satOpState_t opState_p) {
  //Serial.printf("Satelitelink OP deblocked received :0x%x\n", opState_p);
	if (!(satLinkInfo->opState = satLinkInfo->opState & ~opState_p)){
		for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++)
      if (satLinkInfo->sateliteHandle[i] != NULL)
			  satLinkInfo->sateliteHandle[i]->opDeBlock(SAT_OP_CONTROLBOCK);
	}
  xSemaphoreTake(satLinkInfo->performanceCounterLock, portMAX_DELAY);
	clearPerformanceCounters(&satLinkInfo->performanceCounters);
  xSemaphoreGive(satLinkInfo->performanceCounterLock);

	if (satLinkInfo->satLinkStateCb != NULL)
		satLinkInfo->satLinkStateCb(this, satLinkInfo->address, satLinkInfo->opState);
}


satOpState_t sateliteLink::getOpState(void){
  return satLinkInfo->opState;
}


/*sateliteLink admBlock*/
satErr_t sateliteLink::admBlock(void) {
	if (satLinkInfo->admState == SAT_ADM_DISABLE)
		return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
	for (uint8_t i = 0; i < satLinkInfo->noOfSats; i++) {
		if (satLinkInfo->sateliteHandle[i]->getAdmState() != SAT_ADM_DISABLE)
			return (returnCode(SAT_ERR_DEP_BLOCK_STATUS_ERR, 0));
	}
	satLinkInfo->admState = SAT_ADM_DISABLE;
	opBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}


/*sateliteLink admDeBlock*/
satErr_t sateliteLink::admDeBlock(void) {
	if (satLinkInfo->admState == SAT_ADM_ENABLE)
		return (returnCode(SAT_ERR_DEP_BLOCK_STATUS_ERR, 0));
	satLinkInfo->admState = SAT_ADM_ENABLE;
	opDeBlock(SAT_OP_DISABLE);
	return (returnCode(SAT_OK, 0));
}


satAdmState_t sateliteLink::getAdmState(void){
  return satLinkInfo->admState;
}


/*sateliteLink chkErrSec*/
void sateliteLink::chkErrSec(void) {
	uint16_t ErrSum;
	int64_t now;
  
	if (!satLinkInfo->errThresHigh)
		return;
  //Serial.printf("Im here\n");
	ErrSum = satLinkInfo->performanceCounters.rxDataSizeErrSec +
		satLinkInfo->performanceCounters.rxSymbolErrSec +
		satLinkInfo->performanceCounters.rxCrcErrSec +
		satLinkInfo->performanceCounters.remoteCrcErrSec +
		satLinkInfo->performanceCounters.scanTimingViolationErrSec +
		satLinkInfo->performanceCounters.wdErrSec;

	if (!(satLinkInfo->opState & SAT_OP_ERR_SEC) && ErrSum >= satLinkInfo->errThresHigh)
		opBlock(SAT_OP_ERR_SEC);

	if ((now = esp_timer_get_time()) - satLinkInfo->oneSecTimer >= ONE_SEC_US) {
    //Serial.printf("One Second passed, total errors: tot %d, %d, %d, %d, %d, %d, %d\n", 
    //               ErrSum, satLinkInfo->performanceCounters.rxDataSizeErrSec, satLinkInfo->performanceCounters.rxSymbolErrSec, satLinkInfo->performanceCounters.rxCrcErrSec,
    //               satLinkInfo->performanceCounters.remoteCrcErrSec, satLinkInfo->performanceCounters.scanTimingViolationErrSec, satLinkInfo->performanceCounters.wdErrSec);
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
}


void sateliteLink::linkReEstablish(TimerHandle_t timerHandle){
  sateliteLink* sateliteLinkObj = (sateliteLink*)pvTimerGetTimerID(timerHandle);
  //Serial.printf("Got ReEstablishTimer, opState is: %x\n", sateliteLinkObj->satLinkInfo->opState);
  //Serial.printf("%x, %x\n", sateliteLinkObj->satLinkInfo->opState & SAT_OP_FAIL, sateliteLinkObj->satLinkInfo->opState & SAT_OP_ERR_SEC);
  //if ((sateliteLinkObj->satLinkInfo->opState & SAT_OP_ERR_SEC) != 0x00){

  if ((sateliteLinkObj->satLinkInfo->opState & SAT_OP_FAIL) != 0x00 || (sateliteLinkObj->satLinkInfo->opState & SAT_OP_ERR_SEC) != 0x00){
    //Serial.printf("Restarting discovery\n");
    sateliteLinkObj->satLinkDiscover();
  }
}
/*----------------------------------------------------- END Class sateliteLink -----------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: satelite                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satelite::satelite(sateliteLink* satLink_p, uint8_t satAddr_p) {
  //Serial.printf("Creating Satelite %d\n", satAddr_p);
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
		satInfo->sensors[i].filterTime = 1;
		satInfo->sensors[i].timerHandle = xTimerCreate("SenseTimer", pdMS_TO_TICKS(satInfo->sensors[i].filterTime), pdFALSE, (void*)&(this->satInfo->sensors[i]), &filterExp);
		satInfo->sensors[i].timerActive = false;
		satInfo->sensors[i].currentSensorVal = false;
		satInfo->sensors[i].filteredSensorVal = false;
	}
  satInfo->oneSecTimerHandle = xTimerCreate("oneSecTimer", pdMS_TO_TICKS(1000), pdTRUE, this, &chkErrSec);
  satInfo->selfTestTimerHandle = xTimerCreate("selftestTimer", pdMS_TO_TICKS(satInfo->satLinkParent->satLinkInfo->scanInterval*50), pdFALSE, this, &selfTestTimeout);
	satInfo->stateCb = NULL;
	satInfo->senseCb = NULL;
  clearPerformanceCounters(&satInfo->performanceCounters);
  satInfo->performanceCounters.testRemoteCrcErr = 0;
  satInfo->performanceCounters.testRxCrcErr = 0;
  satInfo->performanceCounters.testWdErr = 0;
  satInfo->serverCrcTest = SAT_CRC_TEST_INACTIVE;
  satInfo->clientCrcTest = SAT_CRC_TEST_INACTIVE;
  satInfo->wdTest = SAT_WD_TEST_INACTIVE;
  satInfo->selftestPhase = NO_TEST;
  satInfo->selfTestCb = NULL;
	satInfo->errThresHigh = 10;
	satInfo->errThresLow = 1;
	satInfo->admState = SAT_ADM_DISABLE;
	satInfo->opState = SAT_OP_DISABLE;
  satInfo->selfTestCb = NULL;
}


satelite::~satelite(void) {
	delete satInfo;
  for (uint8_t i = 0; i<8; i++)
    xTimerDelete(satInfo->sensors[i].timerHandle, 10);
  xTimerDelete(satInfo->oneSecTimerHandle, 10);
  xTimerDelete(satInfo->selfTestTimerHandle, 10);
  vSemaphoreDelete(satInfo->performanceCounterLock);
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
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].actMode[actIndex_p] = actMode_p;
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
	return(returnCode(SAT_OK, 0));
}


satErr_t satelite::setSatActVal(uint8_t actVal_p, uint8_t actIndex_p) {
	if (actIndex_p > 3)
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->actVal[actIndex_p] = actVal_p;
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].actVal[actIndex_p] = actVal_p;
	satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
	return(returnCode(SAT_OK, 0));
}


satErr_t satelite::setSenseFilter(uint16_t senseFilter_p, uint8_t senseIndex_p) {
	if (senseIndex_p > 7 || senseFilter_p == 0)                                                                                   // 0 ms filter is not allowed
		return(returnCode(SAT_ERR_PARAM_ERR, 0));
	satInfo->sensors[senseIndex_p].filterTime = senseFilter_p;
  assert(xTimerChangePeriod( satInfo->sensors[senseIndex_p].timerHandle, pdMS_TO_TICKS(satInfo->sensors[senseIndex_p].filterTime), 10) != pdFAIL);
  assert(xTimerStop(satInfo->sensors[senseIndex_p].timerHandle, 10) != pdFAIL); //Changing timer perid starts the timer, we need to stop it
	return(returnCode(SAT_OK, 0));
}


void satelite::satRegSenseCb(satSenseCb_t fn) {
	satInfo->senseCb = fn;
  for(uint8_t i = 0; i < 8; i++){
    satInfo->senseCb(this, satInfo->satLinkParent->getAddress(), satInfo->address, satInfo->sensors[i].address, satInfo->sensors[i].filteredSensorVal);
  }
}


void satelite::satUnRegSenseCb(void) {
	satInfo->senseCb = NULL;
}


satErr_t satelite::satSelfTest(selfTestCb_t selfTestCb_p){
  if(satInfo->opState){
    //Serial.printf("Self-test, satelite in wrong state\n");
    return(returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
    //return 0;
  }
  if(satInfo->selfTestCb != NULL){
    //Serial.printf("Self-test, satelite self test busy\n");
    return(returnCode(SAT_ERR_BUSY_ERR, 0));
    //return 0;
  }
  satInfo->selfTestCb = selfTestCb_p;
  genServerCrcErr();
  return SAT_OK;
}


bool satelite::getSenseVal(uint8_t senseAddr){
  return satInfo->sensors[senseAddr].filteredSensorVal;
}

void satelite::satRegStateCb(satStateCb_t fn) {
	satInfo->stateCb = fn;
  satInfo->stateCb(this, satInfo->satLinkParent->getAddress(), satInfo->address, satInfo->opState);
}


void satelite::satUnRegStateCb(void) {
	satInfo->stateCb = NULL;
}


uint8_t satelite::getAddress(void){
  return satInfo->address;
}


void satelite::senseUpdate(satWire_t* rxData_p) {
  //Serial.printf("Got a sensor update: 0x%x\n", rxData_p->sensorVal);
	for (uint8_t i = 0; i < 8; i++) {
    satInfo->sensors[i].currentSensorVal = (rxData_p->sensorVal & 0b00000001 << i) > 0;
		if (satInfo->sensors[i].currentSensorVal != satInfo->sensors[i].filteredSensorVal) {
      //Serial.printf("Sensor %d changed value to %s\n", i, satInfo->sensors[i].currentSensorVal ? "Active" : "Inactive");
			assert(xTimerStart(satInfo->sensors[i].timerHandle, 10) != pdFAIL);
      //Serial.printf("Started the filter timer for sensor %d\n", i);
			satInfo->sensors[i].timerActive = true;
		}
    else{
      if (satInfo->sensors[i].timerActive) {
        assert(xTimerStop(satInfo->sensors[i].timerHandle, 10) != pdFAIL);
        satInfo->sensors[i].timerActive = false;
        //Serial.printf("Stoped an ongoing filter timer for sensor %d\n", i);
      }
    }
	}
}


void satelite::statusUpdate(satStatus_t* status_p) {
	if (status_p->dirty != true)
		return;
  //Serial.printf("Got a Satelite status update with remoteCrc: %s, rxCrc %s, wdErr %s, serverCrcTest %d, clientCrcTest %d, wdTest %d\n", status_p->remoteCrcErr ? "true" : "false", status_p->rxCrcErr ? "true" : "false", status_p->wdErr ? "true" : "false", satInfo->serverCrcTest, satInfo->clientCrcTest,  satInfo->wdTest);
  
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);

	if (status_p->remoteCrcErr && (satInfo->serverCrcTest == SAT_CRC_TEST_INACTIVE)) {
		satInfo->performanceCounters.remoteCrcErr++;
		satInfo->performanceCounters.remoteCrcErrSec++;
	}
  else if (status_p->remoteCrcErr && satInfo->serverCrcTest == SAT_CRC_TEST_ACTIVE){
    //Serial.printf("Counting remoteCrc test counters\n");
    satInfo->performanceCounters.testRemoteCrcErr ++;
    satInfo->serverCrcTest = SAT_CRC_TEST_DEACTIVATING;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].invServerCrc = false;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  }
  else if (status_p->remoteCrcErr){
    satInfo->serverCrcTest--;
    satInfo->performanceCounters.testRemoteCrcErr++;
  }

	if (status_p->rxCrcErr && satInfo->clientCrcTest == SAT_CRC_TEST_INACTIVE) {
    //Serial.printf("Counting rxCrc counters: now %d\n", satInfo->performanceCounters.rxCrcErr);
		satInfo->performanceCounters.rxCrcErr++;
		satInfo->performanceCounters.rxCrcErrSec++;
	}
  else if (status_p->rxCrcErr && satInfo->clientCrcTest == SAT_CRC_TEST_ACTIVE){
    //Serial.printf("Counting rxCrc test counters\n");
    satInfo->performanceCounters.testRxCrcErr++;
    satInfo->clientCrcTest = SAT_CRC_TEST_DEACTIVATING;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].invClientCrc = false;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  }
   else if (status_p->rxCrcErr){
    satInfo->clientCrcTest--;
    satInfo->performanceCounters.testRxCrcErr++;
  }

  if (status_p->wdErr && satInfo->wdTest == SAT_WD_TEST_INACTIVE) {
    satInfo->performanceCounters.wdErr++;
    satInfo->performanceCounters.wdErrSec++;
  }
  else if (status_p->wdErr && satInfo->wdTest == SAT_WD_TEST_ACTIVE){
    //Serial.printf("Counting wdErr test counters\n");
    satInfo->performanceCounters.testWdErr++;
    satInfo->wdTest = SAT_WD_TEST_DEACTIVATING;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].setWdErr = false;
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  }
  else if (status_p->wdErr){
    satInfo->wdTest--;
    satInfo->performanceCounters.testWdErr++;
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
  status_p->dirty = false;
}


/*satelite admBlock*/
satErr_t satelite::admBlock(void) {
  if(satInfo->admState == SAT_ADM_DISABLE){
    //Serial.printf("admBlock Wrong state\n");
    return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
  }
  assert(xTimerStop(satInfo->oneSecTimerHandle, 10) != pdFAIL);
  satInfo->admState = SAT_ADM_DISABLE;
  opBlock(SAT_OP_DISABLE);
  //Serial.printf("admBlock OK\n");

  return (returnCode(SAT_OK, 0));
}


/*satelite admDeBlock*/
satErr_t satelite::admDeBlock(void) {
  uint8_t tmpSensorVal;
  if (satInfo->admState == SAT_ADM_ENABLE)
    return (returnCode(SAT_ERR_WRONG_STATE_ERR, 0));
  if (satInfo->satLinkParent->getAdmState() != SAT_ADM_ENABLE)
    return (returnCode(SAT_ERR_DEP_BLOCK_STATUS_ERR, 0));    
  assert(xTimerStart(satInfo->oneSecTimerHandle, 10) != pdFAIL);
  satInfo->satLinkParent->getSensorValRaw(satInfo->address, &tmpSensorVal);

  for(uint8_t i = 0; i < 8; i++){
    satInfo->sensors[i].currentSensorVal = (tmpSensorVal & 0b00000001 << i) > 0;
    satInfo->sensors[i].filteredSensorVal = satInfo->sensors[i].currentSensorVal;
    if (satInfo->senseCb != NULL)
      satInfo->senseCb(this, satInfo->satLinkParent->getAddress(), satInfo->address, satInfo->sensors[i].address, satInfo->sensors[i].filteredSensorVal);
  }
  opDeBlock(SAT_OP_DISABLE);
  satInfo->admState = SAT_ADM_ENABLE;
  return (returnCode(SAT_OK, 0));
}
satAdmState_t satelite::getAdmState(void){
  return satInfo->admState;
}


/*satelite opBlock*/
void satelite::opBlock(satOpState_t opState_p) {
  //Serial.printf("Satelite OP blocked received for satelite %d, requesting OpState: 0x%x, previous OpState: 0x%x\n", satInfo->address, opState_p, satInfo->opState);

	if (!satInfo->opState) {
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].enable = false;
    for(uint8_t i = 0; i < NO_OF_ACT; i++){
      satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].enable = false;
      //Serial.printf("Disabled Actuator %d:%d \n", satInfo->address, i);
    }
    satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
	}
	satInfo->opState = satInfo->opState | opState_p;
	if (satInfo->stateCb != NULL)
		satInfo->stateCb(this, satInfo->satLinkParent->getAddress(), satInfo->address, satInfo->opState);
}


/*satelite opDeBlock*/
void satelite::opDeBlock(satOpState_t opState_p) {
  //Serial.printf("Satelite OP deblocked received :0x%x\n", opState_p);

	if (!(satInfo->opState = satInfo->opState & ~opState_p)){
    satSelfTest(&selftestRes);
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].enable = true;
    for(uint8_t i = 0; i < NO_OF_ACT; i++)
      satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].actMode[i] = satInfo->actMode[i];
		satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
	}
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
	clearPerformanceCounters(&satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
  //Serial.printf("Cleared all perf counters %u\n", satInfo->performanceCounters.rxCrcErr);
	if (satInfo->stateCb != NULL)
		satInfo->stateCb(this, satInfo->satLinkParent->getAddress(), satInfo->address, satInfo->opState);
}

satOpState_t satelite::getOpState(void){
  return satInfo->opState;
}

void satelite::getSatStats(satPerformanceCounters_t* satStats_p, bool resetStats) {
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
  memcpy((void*)satStats_p, (void*)&(satInfo->performanceCounters), sizeof(satPerformanceCounters_t));
  if (resetStats)
    clearPerformanceCounters(&satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
}


void satelite::clearSatStats(void) {
  xSemaphoreTake(satInfo->performanceCounterLock, portMAX_DELAY);
  clearPerformanceCounters(&satInfo->performanceCounters);
  xSemaphoreGive(satInfo->performanceCounterLock);
}


void satelite::chkErrSec(TimerHandle_t timerHandle) {
	uint16_t ErrSum;
	satelite* satObj;

  satObj = (satelite*) pvTimerGetTimerID(timerHandle);
  //Serial.printf("CRC Check %u\n", satObj->satInfo->performanceCounters.rxCrcErr);

	if (!satObj->satInfo->errThresHigh)
		return;
 
  //Serial.printf("Check Satelite errored second\n");
  xSemaphoreTake(satObj->satInfo->performanceCounterLock, portMAX_DELAY);
	ErrSum = satObj->satInfo->performanceCounters.rxCrcErrSec +
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
  sensor->timerActive = false;

  //Serial.printf("Filter timer expired for sensor %d\n", sensor->address);

  if (sensor->currentSensorVal != sensor->filteredSensorVal) {
    sensor->filteredSensorVal = sensor->currentSensorVal;
    sensor->satObj->satInfo->senseCb(sensor->satObj, sensor->satObj->satInfo->satLinkParent->getAddress(), sensor->satObj->getAddress(), sensor->address, sensor->filteredSensorVal);
  }
}


void satelite::genServerCrcErr(void){
  //Serial.printf("Generating Server CRC Error\n");
  satInfo->performanceCounters.testRemoteCrcErr = 0;
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].invServerCrc = true;
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  satInfo->serverCrcTest = SAT_CRC_TEST_ACTIVE;
  satInfo->selftestPhase = SERVER_CRC_TEST;
  assert(xTimerStart(satInfo->selfTestTimerHandle, 10) != pdFAIL);
}


void satelite::genClientCrcErr(void){                                                           // Move to private
  //Serial.printf("Generating Client CRC Error\n");
  satInfo->performanceCounters.testRxCrcErr = 0;
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].invClientCrc = true;
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  satInfo->clientCrcTest = SAT_CRC_TEST_ACTIVE;
  satInfo->selftestPhase = CLIENT_CRC_TEST;
  assert(xTimerStart(satInfo->selfTestTimerHandle, 10) != pdFAIL);
}


void satelite::genWdErr(void){                                                                 // Move to private , non intrusive - does not disable actuators
  //Serial.printf("Generating WD Error\n");
  satInfo->performanceCounters.testWdErr = 0;                                                  // Neds new states and masking in the Link class
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].setWdErr = true;
  satInfo->satLinkParent->satLinkInfo->txSatStruct[satInfo->address].dirty = true;
  satInfo->wdTest = SAT_WD_TEST_ACTIVE;
  satInfo->selftestPhase = WD_TEST;
  assert(xTimerStart(satInfo->selfTestTimerHandle, 10) != pdFAIL);
}


void satelite::selfTestTimeout(TimerHandle_t timerHandle){
  satelite* satObj = (satelite*) pvTimerGetTimerID(timerHandle);
  //Serial.printf("Entering\n");

  switch (satObj->satInfo->selftestPhase){
    case SERVER_CRC_TEST:
      if (!satObj->satInfo->performanceCounters.testRemoteCrcErr){
        //Serial.printf("Selftest expected remote CRC errors but got none\n");
        satObj->satInfo->selfTestCb(satObj, satObj->satInfo->satLinkParent->satLinkInfo->address, satObj->satInfo->address, SAT_SELFTEST_SERVER_CRC_ERR);
        satObj->satInfo->serverCrcTest = SAT_CRC_TEST_INACTIVE;
        satObj->satInfo->selftestPhase = NO_TEST;
        satObj->satInfo->selfTestCb = NULL;
      }
      else {
        //Serial.printf("Selftest got %d remote CRC errors\n", satObj->satInfo->performanceCounters.testRemoteCrcErr);
        satObj->satInfo->serverCrcTest = SAT_CRC_TEST_INACTIVE;
        satObj->genClientCrcErr();
      }
      break;
      
    case CLIENT_CRC_TEST:
      if (!satObj->satInfo->performanceCounters.testRxCrcErr){
        //Serial.printf("Selftest expected remote CRC errors but got none\n");
        satObj->satInfo->selfTestCb(satObj, satObj->satInfo->satLinkParent->satLinkInfo->address, satObj->satInfo->address, SAT_SELFTEST_CLIENT_CRC_ERR);
        satObj->satInfo->clientCrcTest = SAT_CRC_TEST_INACTIVE;
        satObj->satInfo->selftestPhase = NO_TEST;
        satObj->satInfo->selfTestCb = NULL;

      }
      else {
        //Serial.printf("Selftest got %d rx CRC errors\n", satObj->satInfo->performanceCounters.testRxCrcErr);
        satObj->satInfo->clientCrcTest = SAT_CRC_TEST_INACTIVE;
        satObj->genWdErr();
      }
      break;
      
    case WD_TEST:
      if (!satObj->satInfo->performanceCounters.testWdErr){
        //Serial.printf("Selftest expected WD errors but got none\n");
        satObj->satInfo->selfTestCb(satObj, satObj->satInfo->satLinkParent->satLinkInfo->address, satObj->satInfo->address, SAT_SELFTEST_WD_ERR);
        satObj->satInfo->wdTest = SAT_WD_TEST_INACTIVE;
        satObj->satInfo->selftestPhase = NO_TEST;
        satObj->satInfo->selfTestCb = NULL;
      }
      else {
        //Serial.printf("Selftest got %d wd errors\n", satObj->satInfo->performanceCounters.testWdErr);
        satObj->satInfo->wdTest = SAT_WD_TEST_INACTIVE;
        satObj->satInfo->selftestPhase = NO_TEST;
        satObj->satInfo->selfTestCb(satObj, satObj->satInfo->satLinkParent->satLinkInfo->address, satObj->satInfo->address, SAT_OK);
        satObj->satInfo->selfTestCb = NULL;
      }
      //Serial.printf("Breaking\n");
      break;

    default:
      assert(true == false);
      break;
  }
}
void satelite::selftestRes(satelite* satHandle_p, uint8_t satLinkAddr_p, uint8_t satAddr_p, satErr_t err_p){
  if(err_p)
    satHandle_p->opBlock(SAT_OP_FAIL);
  //Serial.printf("Selftest executed\n");
  return;
}
