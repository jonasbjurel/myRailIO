#ifndef SATELITE_H
#define SATELITE_H
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
// See readme.md
/*==============================================================================================================================================*/
/* END Description                                                                                                                              */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/rmt.h"
#include "Arduino.h"
class sateliteLink;
class satelite;
/*==============================================================================================================================================*/
/* END Include files																															*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Configuration                                                                                                                                */
/*==============================================================================================================================================*/

#define MAX_NO_OF_SAT_PER_CH        8

/*==============================================================================================================================================*/
/* END Configuration																															*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Library constants                                                                                                                            */
/*==============================================================================================================================================*/
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

#define ONE_SEC_US                  1000000

#define SATBUF_CRC_BYTE_OFFSET      7

#define NO_OF_ACT                   4
#define NO_OF_SENS                  8

#define T_REESTABLISH_LINK_MS       20000



/*==============================================================================================================================================*/
/* END Library constants																														*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Data types, structures, and enumurations                                                                                                     */
/*==============================================================================================================================================*/
// Data prototypes
typedef uint64_t satErr_t;
typedef uint8_t satAdmState_t;
typedef uint8_t satOpState_t;
typedef uint8_t actMode_t;

//Call-back prototypes
typedef void (*satLinkStateCb_t)(sateliteLink* sateliteLink_p, uint8_t LinkAddr_p, satOpState_t satOpState_p);
typedef void (*satStateCb_t)(satelite* satelite_p, uint8_t LinkAddr_p, uint8_t SatAddr_p, satOpState_t satOpState);
typedef void (*satSenseCb_t)(satelite* satelite_p, uint8_t LinkAddr_p, uint8_t SatAddr_p, uint8_t senseAddr_p, bool senseVal_p);
typedef void (*satDiscoverCb_t)(satelite* satelite_p, uint8_t LinkAddr_p, uint8_t SatAddr_p, bool exists_p);
typedef void (*selfTestCb_t)(satelite* satelite_p, uint8_t LinkAddr_p, uint8_t SatAddr_p, satErr_t selftestErr_p);


struct satWire_t {
    bool dirty;                         // If true the values have been updated
    bool invServerCrc;                  // If true, outgoing Server CRC checksum will be invalidated
    bool invClientCrc;                  // If true, outgoing SAT CRC checksum will be invalidated
    bool setWdErr;                      // If true the satelite with simulate a wd error without disabling the actuators
    bool enable;                        // If true the satelite actuators will be enabled
    // Link data representation in transmission order - MSB is sent first
    uint8_t sensorVal;                  // 8 bits (MSB). Sensor value, can be written by any one
    uint8_t actVal[4];                  // 8 bits x 4
    uint8_t actMode[4];                 // 3 bits x 4
    uint8_t cmdWdErr;                   // 1 bits
    uint8_t cmdEnable;                  // 1 bit
    uint8_t cmdInvCrc;                  // 1 bit
    uint8_t startMark;                  // 1 bit
    uint8_t fbReserv;                   // 2 bit
    uint8_t fbWdErr;                    // 1 bit
    uint8_t fbRemoteCrcErr;             // 1 bit
    uint8_t crc;                        // 4 bit
};


struct satStatus_t { 
    bool dirty;                         //dirty is set by the producer and cleared by the consumer, while dirty is set the producer will not change the values
    bool wdErr;                         //Latched until dirty is reset
    bool remoteCrcErr;                  //Latched until dirty is reset
    bool rxCrcErr;                      //Latched until dirty is reset
    bool rxSymbolErr;                   //Latched until dirty is reset
    bool rxDataSizeErr;                 //Latched until dirty is reset
};


struct sensor_t {
    satelite* satObj;
    uint8_t address;
    uint16_t filterTime;
    TimerHandle_t timerHandle;
    bool timerActive;
    bool currentSensorVal;
    bool filteredSensorVal;
};


struct satPerformanceCounters_t {
  uint32_t txUnderunErr;
  uint32_t txUnderunErrSec;
  uint32_t rxOverRunErr;
  uint32_t rxOverRunErrSec;
  uint32_t scanTimingViolationErr;
  uint16_t scanTimingViolationErrSec;
  uint32_t rxDataSizeErr;
  uint32_t rxDataSizeErrSec;
  uint32_t rxSymbolErr;
  uint32_t rxSymbolErrSec;
  uint32_t wdErr;
  uint32_t wdErrSec;
  uint32_t rxCrcErr;
  uint32_t rxCrcErrSec;
  uint32_t remoteCrcErr;
  uint32_t remoteCrcErrSec;
  uint32_t testRemoteCrcErr;
  uint32_t testRxCrcErr;
  uint32_t testWdErr;
};


struct satLinkInfo_t {
    uint8_t address;
    rmt_channel_t txCh;                 // Satelite link TX channel
    rmt_channel_t rxCh;                 // Satelite link RX channel
    gpio_num_t txPin;                   // Satelite link TX Pin
    gpio_num_t rxPin;                   // Satelite link RX Pin
    uint8_t txMemblck;                  // Satelite link TX RMT Memory block
    uint8_t rxMemblck;                  // Satelite link RX RMT Memory block
    RingbufHandle_t rb;
    UBaseType_t pollTaskPrio;
    UBaseType_t pollTaskCore;
    uint8_t scanInterval;               // link Scan intervall
    satAdmState_t admState;             // Satelite link operational status - a bitmap showing the origin of the operational state  //LOCK FOR LINKSTATES LOCAL TO LINK????
    satOpState_t opState;               // Satelite link operational status - a bitmap showing the cause
    satWire_t txSatStruct[MAX_NO_OF_SAT_PER_CH + 1];
    satWire_t rxSatStruct[MAX_NO_OF_SAT_PER_CH + 1];
    uint8_t txSatBuff[(MAX_NO_OF_SAT_PER_CH + 1) * 8];
    uint8_t rxSatBuff[(MAX_NO_OF_SAT_PER_CH + 1) * 8];
    rmt_item32_t txItems[(MAX_NO_OF_SAT_PER_CH + 1) * 8 * 8];
    rmt_item32_t rxItems[(MAX_NO_OF_SAT_PER_CH + 1) * 8 * 8];
    satPerformanceCounters_t performanceCounters;
    SemaphoreHandle_t performanceCounterLock;
    uint32_t errThresHigh;              // Cumulative CRC error threshold (local and remote) for bringing the link down.
    uint32_t errThresLow;
    int64_t oneSecTimer;
    uint8_t noOfSats;                   // Satelite link operational status - a bitmap showing the origin of the operational state
    satelite* sateliteHandle[MAX_NO_OF_SAT_PER_CH + 1];
    satStatus_t satStatus[MAX_NO_OF_SAT_PER_CH + 1];  //These data structures are per link EXCLUSIVELY protected by satStatusLock
    satLinkStateCb_t satLinkStateCb;
    satDiscoverCb_t satDiscoverCb;
    TaskHandle_t scanTaskHandle;
    TimerHandle_t linkReEstablishTimerHandle;
    uint8_t serverCrcTst;
    uint8_t clientCrcTst;
    uint8_t wdTst;
    bool scan;
    //locks
};


struct satInfo_t {
    uint8_t address;                    // Satelite local address on the Satelite link
    actMode_t actMode[NO_OF_ACT];               // Satelite Actuators mode
    uint8_t actVal[NO_OF_ACT];                  // Satelite Actuators value
    sensor_t sensors[NO_OF_SENS];                // Sensor struct
    satStateCb_t stateCb;               // Callback function for state changes
    satSenseCb_t senseCb;               // Callback function for sensor changes
    selfTestCb_t selfTestCb;
    satPerformanceCounters_t performanceCounters;
    uint8_t clientCrcTest;
    uint8_t serverCrcTest;
    uint8_t wdTest;
    uint8_t selftestPhase;
    TimerHandle_t selfTestTimerHandle;
    TimerHandle_t oneSecTimerHandle;
    uint32_t errThresHigh;              // Cumulative high threshold for all errors
    uint32_t errThresLow;               // Cumulative low threshold for all errors
    satAdmState_t admState;                   // Satelite operational status - a bitmap showing origin of the operational state
    satOpState_t opState;                    // Satelite operational status - a bitmap showing the cause
    sateliteLink* satLinkParent;        // A handle to the parent Satelite link
    SemaphoreHandle_t performanceCounterLock;
};


// Adminastrive states
#define SAT_ADM_ENABLE                0   // The ADMIN state has been set to ENABLE, decribe both satLink and sats
#define SAT_ADM_DISABLE               1   // The ADMIN state has been set to DISABLE, 

// Adminastrive states and causes
#define SAT_OP_WORKING                0x0000   // The OP state is set to working, link scanning is ongoing, 
#define SAT_OP_WORKING_STR            "WORKING"
#define SAT_OP_INIT                   0x0001
#define SAT_OP_INIT_STR               "INIT"
#define SAT_OP_DISABLE                0x0002
#define SAT_OP_DISABLE_STR            "DISABLE"
#define SAT_OP_CONTROLBOCK            0x0004
#define SAT_OP_CONTROLBOCK_STR        "CTRLBLCK"
#define SAT_OP_ERR_SEC                0x0008
#define SAT_OP_ERR_SEC_STR            "ERRSEC"
#define SAT_OP_FAIL                   0x0010
#define SAT_OP_FAIL_STR               "FAIL"

//Error codes
#define SAT_OK                        0x00
#define SAT_ERR_SYMBOL_ERR            0x01
#define SAT_ERR_EXESSIVE_SATS_ERR     0x02
#define SAT_ERR_GEN_SATLINK_ERR       0x03
#define SAT_ERR_WRONG_STATE_ERR       0x04
#define SAT_ERR_DEP_BLOCK_STATUS_ERR  0x05
#define SAT_ERR_PARAM_ERR             0x06
#define SAT_ERR_RMT_ERR               0x07
#define SAT_ERR_EXESSIVE_SATS         0x08
#define SAT_ERR_SCANTASK_ERR          0x09
#define SAT_ERR_NOT_EXIST_ERR         0x0A
#define SAT_ERR_BUFF_SMALL_ERR        0x0B
#define SAT_ERR_BUSY_ERR              0x0C
#define SAT_SELFTEST_SERVER_CRC_ERR   0x0D
#define SAT_SELFTEST_CLIENT_CRC_ERR   0x0E
#define SAT_SELFTEST_WD_ERR           0x0F



// PM report format items
#define LINK_ADDR                     0x0001
#define SAT_ADDR                      0x0002
#define RX_SIZE_ERR                   0x0004
#define RX_SYMB_ERR                   0x0008
#define TIMING_VIOLATION_ERR          0x0010
#define TX_UNDERRUN_ERR               0x0020
#define RX_OVERRRUN_ERR               0x0040
#define RX_CRC_ERR                    0x0080
#define REMOTE_CRC_ERR                0x0100
#define WATCHDG_ERR                   0x0200
#define ADM_STATE                     0x0400
#define OP_STATE                      0x0800

//Satelite modes
#define SATMODE_LOW                   0x00
#define SATMODE_HIGH                  0x01
#define SATMODE_PWM1_25K              0x02
#define SATMODE_PWM100                0x03
#define SATMODE_PULSE                 0x04
#define SATMODE_PULSE_INV             0x05

// Test phases
#define SAT_CRC_TEST_ACTIVE           0xFF
#define SAT_CRC_TEST_DEACTIVATING     0x0F
#define SAT_CRC_TEST_INACTIVE         0x00
#define SAT_WD_TEST_ACTIVE            0xFF
#define SAT_WD_TEST_DEACTIVATING      0x0F
#define SAT_WD_TEST_INACTIVE          0x00
#define NO_TEST                       0x00
#define SERVER_CRC_TEST               0x01
#define CLIENT_CRC_TEST               0x02
#define WD_TEST                       0x03


/*==============================================================================================================================================*/
/* END Data structures and enumurations																											*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Function and Class prototypes                                                                                                                */
/*==============================================================================================================================================*/


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: Helper functions: opStateToStr, formatSatStat                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satErr_t opStateToStr(satOpState_t opState_p, char* outputStr_p, uint8_t length_p);

satErr_t formatSatStat(char* reportBuffer_p, uint16_t buffSize_p, uint16_t* usedBuff_p, uint16_t buffOffset_p, uint8_t linkAddr_p,
                        uint8_t satAddr_p, satAdmState_t admState_p, satOpState_t opState_p, satPerformanceCounters_t* pmdata_p, uint16_t reportColumnItems,
                        uint16_t reportItemsMask, bool printHead);
                        
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: ws28xx_rmt_tx_translator & ws28xx_rmt_rx_translator                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
static void IRAM_ATTR ws28xx_rmt_tx_translator(const void* src, rmt_item32_t* dest, size_t src_size,
    size_t wanted_num, size_t* translated_size, size_t* item_num);
satErr_t IRAM_ATTR ws28xx_rmt_rx_translator(const rmt_item32_t* src, uint8_t* dest, uint16_t len);



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: crc												                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void crc(uint8_t * p_crc, uint8_t * p_buff, uint16_t p_buffSize, bool p_invalidate);


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: populateSatLinkBuff & populateSatWireStruct                                                                                        */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void populateSatLinkBuff(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p);
bool populateSatWireStruct(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p);


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: returnCode												                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satErr_t returnCode(uint8_t localErr, uint32_t callFnErr);


/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: clearPerformanceCounters                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Parameters:                                                                                                                                  */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
void clearPerformanceCounters(satPerformanceCounters_t* performanceCounters_p);

  
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: sateliteLink                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
class sateliteLink {
public:
    //methods
    sateliteLink(uint8_t address_p, gpio_num_t txPin_p, gpio_num_t rxPin_p, rmt_channel_t txCh_p, rmt_channel_t rxCh_p, uint8_t txRmtMemBank_p,
                 uint8_t rxRmtMemBank_p, UBaseType_t pollTaskPrio_p, UBaseType_t pollTaskCore_p, uint8_t scanInterval_p);
    ~sateliteLink(void);
    satErr_t enableSatLink(void);
    satErr_t disableSatLink(void);
    void setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow);
    void satLinkRegStateCb(satLinkStateCb_t satLinkStateCb_p);
    void satLinkUnRegStateCb(void);
    void satLinkRegSatDiscoverCb(satDiscoverCb_t satDiscoverCb_p);
    void satLinkUnRegSatDiscoverCb(void);
    uint8_t getAddress(void);
    uint8_t getSatLinkNoOfSats(void);
    satErr_t getSensorValRaw(uint8_t satAddress_p, uint8_t* sensorVal_p);
    void getSatStats(satPerformanceCounters_t* satStats_p, bool resetStats);
    void clearSatStats(void);
    satelite* getsatHandle(uint8_t satAddr_p);
    satErr_t admBlock(void);
    satErr_t admDeBlock(void);
    satAdmState_t getAdmState(void);
    void opBlock(satOpState_t opState_p);
    void opDeBlock(satOpState_t opState_p);
    satOpState_t getOpState(void);

    //Data structures
    satLinkInfo_t* satLinkInfo;

private:
    // methods
    satErr_t satLinkDiscover(void);
    satErr_t satLinkStartScan(void);
    satErr_t satLinkStopScan(void);
    static void satLinkScan(void* satLinkObj);
    void chkErrSec(void);
    static void linkReEstablish(TimerHandle_t timerHandle);


    // Data structures
    // -
};
/*----------------------------------------------------- END Class sateliteLink -----------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: satelite                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
// Class fixed constants

class satelite {
public:
    // Methods

    satelite(sateliteLink* satLink_p, uint8_t satAddr_p);
    ~satelite(void);
    satErr_t enableSat(void);
    satErr_t disableSat(void);
    void setErrTresh(uint16_t errThresHigh_p, uint16_t errThresLow_p);
    satErr_t setSatActMode(actMode_t actMode_p, uint8_t actIndex_p);
    satErr_t setSatActVal(uint8_t actVal_p, uint8_t actIndex_p);
    satErr_t setSenseFilter(uint16_t senseFilter_p, uint8_t senseIndex_p);
    void getSatStats(satPerformanceCounters_t* satStats_p, bool resetStats);
    void clearSatStats(void);
    void satRegSenseCb(satSenseCb_t fn);
    void satUnRegSenseCb(void);
    satErr_t satSelfTest(selfTestCb_t selfTestCb_p);
    bool getSenseVal(uint8_t senseAddr);
    void satRegStateCb(satStateCb_t fn);
    void satUnRegStateCb(void);
    uint8_t getAddress(void);
    void senseUpdate(satWire_t* rxData);
    void statusUpdate(satStatus_t* status_p);
    satErr_t admBlock(void);
    satErr_t admDeBlock(void);
    satAdmState_t getAdmState(void);
    void opBlock(satOpState_t opState_p); 
    void opDeBlock(satOpState_t opState_p);
    satOpState_t getOpState(void);


    // Data structures
    satInfo_t* satInfo;
    
private:
    // Methods
    static void chkErrSec(TimerHandle_t timerHandle);
    static void filterExp(TimerHandle_t timerHandle);
    void genServerCrcErr(void);
    void genClientCrcErr(void);
    void genWdErr(void);
    static void selfTestTimeout(TimerHandle_t timerHandle);
    static void selftestRes(satelite* satHandle_p, uint8_t satLinkAddr_p, uint8_t satAddr_p, satErr_t err_p);

    // Data structures
    // -
};
/*------------------------------------------------------- END Class satelite -------------------------------------------------------------------*/

/*==============================================================================================================================================*/
/* END Function and Class prototypes																											*/
/*==============================================================================================================================================*/

#endif /*SATELITE_H*/
