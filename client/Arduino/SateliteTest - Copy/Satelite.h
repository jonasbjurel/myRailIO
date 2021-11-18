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
//
//
/*==============================================================================================================================================*/
/* END Description                                                                                                                              */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* END Include files																															*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Configuration                                                                                                                                */
/*==============================================================================================================================================*/

MAX_NO_OF_SAT_PER_CH        8

/*==============================================================================================================================================*/
/* END Configuration																															*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Library constants                                                                                                                            */
/*==============================================================================================================================================*/
#define CPUFREQ                     80000000
#define WS28XX_T0H_NS               400
#define WS28XX_T1H_NS               800
#define WS28XX_CYC_NS               1250
#define WS28XX_GUARD_NS             10000

#define WS28XX_T0H_CYC              32              //WS28XX_T0H_NS*CPUFREQ/1000000000
#define WS28XX_T0H_CYC_MIN          29              //WS28XX_T0H_CYC * 0.9
#define WS28XX_T0H_CYC_MIN          35              //WS28XX_T0H_CYC * 1.1
#define WS28XX_T0L_CYC              (WS28XX_CYC_CYC - WS28XX_T0H_CYC)
#define WS28XX_T1H_CYC              64              //WS28XX_T1H_NS*CPUFREQ/1000000000
#define WS28XX_T1H_CYC_MIN          58              //WS28XX_T1H_CYC * .9
#define WS28XX_T1H_CYC_MAX          70              //WS28XX_T1H_CYC * 1.1
#define WS28XX_T1L_CYC              (WS28XX_CYC_CYC - WS28XX_T1H_CYC)
#define WS28XX_CYC_CYC              100             //WS28XX_CYC_NS*CPUFREQ/1000000000
#define WS28XX_CYC_CYC_MIN          90              //WS28XX_CYC_CYC*0.9
#define WS28XX_CYC_CYC_MAX          110             //WS28XX_CYC_CYC*1.1
#define WS28XX_GUARD_CYC            800             //WS28XX_GUARD_NS*CPUFREQ/1000000000

#define ONE_SEC_US                  1000000

#define SATBUF_CRC_BYTE_OFFSET      8

#define SAT_CRC_TEST_ACTIVE         0x0F
#define SAT_CRC_TEST_INACTIVE       0x00




/*==============================================================================================================================================*/
/* END Library constants																														*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Data types, structures, and enumurations                                                                                                     */
/*==============================================================================================================================================*/
// Data types
#define satErr_t    uint64_t


struct satWire_t {
    bool dirty;                         // If true the values have been updated
    bool invServerCrc;                  // If true, outgoing Server CRC checksum will be invalidated
    bool invClientCrc:                  // If true, outgoing SAT CRC checksum will be invalidated
    bool enable;                        // If true the satelite actuators will be enabled - overrides
    // Link data representation in transmission order - MSB is sent first
    uint8_t sensorVal;                  // 8 bits (MSB). Sensor value, can be written by any one
    uint8_t actVal[4];                  // 8 bits x 4
    uint8_t actMode[4];                 // 3 bits x 4
    uint8_t cmdReserv;                  // 1 bits
    uint8_t cmdEnable;                  // 1 bit
    uint8_t cmdInvCrc;                  // 1 bit
    uint8_t startMark;                  // 1 bit
    uint8_t fbReserv;                   // 2 bit
    uint8_t fbWdErr;                    // 1 bit
    uint8_t fbRemoteCrcErr;             // 1 bit
    uint8_t crc;                        // 4 bit
};



struct satStatus_t {
    bool dirty;
    bool wdErr;                         //Latched
    bool remoteCrcErr;                  //Latched
    bool rxCrcErr;                      //Latched
    bool rxSymbolErr;                   //Latched
    bool rxDataSizeErr;                 //Latched
};



struct sensor_t {
    satelite* satObj;
    uint8_t address;
    TickType_t filterTime;
    TimerHandle_t timerHandle;
    bool timerActive;
    bool currentSensorVal;
    bool filteredSensorVal;
}



struct satLink_t {
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
    satAdmState_t admState;             // Satelite link operational status - a bitmap showing the origin of the operational state
    satOpState_t opState;               // Satelite link operational status - a bitmap showing the cause
    satWire_t satTxStructs[MAX_NO_OF_SAT_PER_CH + 1];
    satWire_t satRxStructs[MAX_NO_OF_SAT_PER_CH + 1];
    uint8_t* satTxBuff[(MAX_NO_OF_SAT_PER_CH + 1) * 8];
    uint8_t* satRxBuff[(MAX_NO_OF_SAT_PER_CH + 1) * 8];
    rmt_item32_t txItems[(MAX_NO_OF_SAT_PER_CH + 1) * 8 * 8];
    rmt_item32_t rxItems[(MAX_NO_OF_SAT_PER_CH + 1) * 8 * 8];
    uint32_t symbolRxErr;               // Cumulative counter for all Rx Symbol errors reported on this satelite link
    uint16_t symbolRxErrSec;
    uint32_t rxDataSizeErr;             // Missing data errors
    uint16_t rxDataSizeErrSec;
    uint32_t txUnderunErr;              // Missed to deliver TX data within specified time for satelite latching - CRC error would capture this so no bogus Actuator data is traversing to output
    uint32_t rxOverRunErr;              // Rx buffer full, should be a very rare event - if it can at all happen?
    uint32_t scanTimingViolationErr;    // The Tx/Rx poll was outside of specified time +50%
    uint16_t scanTimingViolationErrSec;
    uint32_t rxCrcErr;                  // Cumulative counter for all Rx CRC errors reported on this satelite link
    uint16_t rxCrcErrSec;
    uint32_t remoteCrcErr;              // Cumulative counter for all remote CRC errors reported on this satelite link
    uint16_t remoteCrcErrSec;
    uint32_t wdErr;                     // Cumulative counter for all CRC errors reported on this satelite link
    uint16_t wdErrSec;
    uint32_t errThresHigh;              // Cumulative CRC error threshold (local and remote) for bringing the link down.
    uint32_t errThresLow;
    int64_t oneSecTimer;
    uint8_t noOfSats;                   // Satelite link operational status - a bitmap showing the origin of the operational state
    satelite* sateliteHandle[MAX_NO_OF_SAT_PER_CH];
    satStatus_t satStatus[MAX_NO_OF_SAT_PER_CH + 1];
    satLinkStateCb_t satLinkStateCb;
    SemaphoreHandle_t txStructLock;
    SemaphoreHandle_t rxStructLock;
    SemaphoreHandle_t genLock;
    TaskHandle_t scanTaskHandle;
    uint8_t serverCrcTst;
    uint8_t clientCrcTst;
    bool scan;
};



struct satelite_t {
    uint8_t address;                     // Satelite local address on the Satelite link
    actMode_t actMode[3];               // Satelite Actuators mode
    uint8_t actVal[3];                  // Satelite Actuators value
    sensort_t sensors[8];               // Sensor struct
    stateCallback_t stateCallback;      // Callback function for state changes
    senseCallback_t senseCallback;      // Callback function for sensor changes

    uint32_t symbolRxErr;               // Cumulative counter for all Rx Symbol errors reported on this satelite link
    uint32_t symbolRxSec;               // Second counter for all Rx Symbol errors reported on this satelite link
    uint32_t rxDataSizeErr;             // Cumulative counter for all missing data errors
    uint32_t rxDataSizeErrSec;          // Second counter for all missing data errors
    uint32_t rxCrcErr;                  // Cumulative counter for all Rx CRC errors
    uint32_t rxCrcErrSec;               // Second counter for all Rx CRC errors
    uint32_t remoteCrcErr;              // Cumulative counter for all remote CRC errors
    uint32_t remoteCrcErrSec;           // Second counter for all remote CRC errors
    uint32_t wdErr;                     // Cumulative counter for all watchdog errors
    uint32_t wdErrSec;                  // Second counter for all watchdog errors
    int64_t oneSecTimer;
    uint32_t errThresHigh;              // Cumulative high threshold for all errors
    uint32_t errThresLow;               // Cumulative low threshold for all errors
    uint8_t admState;                   // Satelite operational status - a bitmap showing origin of the operational state
    uint8_t opState;                    // Satelite operational status - a bitmap showing the cause
    satLink* satLink;                   // A handle to the parent Satelite link
};



// Adminastrive states
#define SAT_ADM_ENABLE              0   // The ADMIN state has been set to ENABLE, decribe both satLink and sats
#define SAT_ADM_DISABLE             1   // The ADMIN state has been set to DISABLE, 

// Adminastrive states and causes
#define SAT_OP_WORKING              0x0000   // The OP state is set to working, link scanning is ongoing, 
#define SAT_OP_INIT                 0x0001
#define SAT_OP_DISABLE              0x0002
#define SAT_OP_CONTROLBOCK          0x0004
#define SAT_OP_ERR_SEC              0x0008
#define SAT_OP_FAIL                 0x0010



//Error codes
#define SAT_OK
#define SAT_ERR_SYMBOL_ERR
#define SAT_ERR_EXESSIVE_SATS_ERR
#define SAT_ERR_GEN_SATLINK_ERR
#define SAT_ERR_WRONG_STATE_ERR
#define SAT_ERR_SAT_NOTBLOCKED_ERR
#define SAT_ERR_PARAM_ERR



//Satelite modes
#define SATMODE_LOW                 0
#define SATMODE_HIGH                1
#define SATMODE_PWM1_25K            2
#define SATMODE_PWM100              3
#define SATMODE_PULSE               4
#define SATMODE_PULSE_INV           5



/*==============================================================================================================================================*/
/* END Data structures and enumurations																											*/
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Function and Class prototypes                                                                                                                */
/*==============================================================================================================================================*/
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: ws28xx_rmt_tx_translator & ws28xx_rmt_rx_translator                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
static void IRAM_ATTR ws28xx_rmt_tx_translator(const void* src, rmt_item32_t* dest, size_t src_size,
    size_t wanted_num, size_t* translated_size, size_t* item_num);
satErr_t IRAM_ATTR ws28xx_rmt_rx_translator(const rmt_item32_t* src, uint8_t* dest, uint16_t len) {



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
void populateSatWireStruct(satWire_t* satWireStruct_p, uint8_t* satWireBuff_p);



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Function: returnCode												                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
satErr_t returnCode(uint8_t localErr, uint32_t callFnErr);




/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: sateliteLink                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/

class sateliteLink {
public:
    //methods
    sateliteLink(uint_t address, gpio_num_t p_txPin, gpio_num_t p_rxPin, rmt_channel_t p_txCh, rmt_channel_t p_rxCh, uint8_t p_txRmtMemBank, uint8_t p_rxRmtMemBank, UBaseType_t p_pollTaskPrio, uint8_t p_scanInterval);
    ~sateliteLink(void);
    satErr_t enableSatLink(void);
    satErr_t disableSatLink(void);
    void setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow);
    void satLinkRegStateCb(satLinkStateCb_t p_satLinkStateCb);
    void satLinkUnRegStateCb(void);
    uint8_t getSatLinkNoOfSats(void);
    satelite_p getsatRef(satAddr_t satAddr_p);
    void clearSatLinkStats(void);

    //Data structures
    satLinkInfo_t* satLinkInfo;

private:
    //methods
    satErr_t satLinkDiscover(void);
    satErr_t satLinkStartScan(void);
    satErr_t satLinkStopScan(void);
    static void satLinkScan(void* satLinkObj);
    satErr_t admBlock(void);
    satErr_t admDeBlock(void);
    void opBlock(satOpState_t opState);
    void opDeBlock(satOpState_t opState);
    satErr_t admBlock(void);
    satErr_t admDeBlock(void);
    void chkErrSec(void);

    //Data structures
    satLinkInfo_t satLinkInfo;
}
/*----------------------------------------------------- END Class sateliteLink -----------------------------------------------------------------*/



/*----------------------------------------------------------------------------------------------------------------------------------------------*/
/* Class: satelite                                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*----------------------------------------------------------------------------------------------------------------------------------------------*/
//Class fixed constants

class satelite {
public:
    satelite(sateliteLink* satLink_p, sataddr_t satAddr_p);
    ~satelite(void);
    satErr_t enableSat(void);
    satErr_t disableSat(void);
    void setErrTresh(uint16_t p_errThresHigh, uint16_t p_errThresLow);
    satErr_t setSatActMode(satActMode_t actMode_p, uint8_t actIndex_p);
    satErr_t setSatActVal(satActMode_t actVal_p, uint8_t actIndex_p);

    setErr_t setSenseFilter(satFilter_t senseFilter_p, uint8_t actIndex_p);
    void getSatInfo(satelite_t* satInfo_p);
    void clearSatStats(void);
    void satRegSenseCb(satSenseCb_t fn);
    void satUnRegSenseCb(void);
    void satRegStateCb(satStateCb_t fn);
    void satUnRegStateCb(void);
    void senseUpdate(satWire_t* rxData);
    void statusUpdate(satStatus_t* status);

    //Data structures
    satelite_t satInfo;

private:
    //methods

    void getSatLinkRef(satLink* satLinkRef);
    void opBlock(satOpState_t opState);
    void opDeBlock(satOpState_t opState);
    satErr_t admBlock(void);
    satErr_t admDeBlock(void);

    // 
    //Data structures
    //--
}
/*------------------------------------------------------- END Class satelite -------------------------------------------------------------------*/







/*==============================================================================================================================================*/
/* END Function and Class prototypes																											*/
/*==============================================================================================================================================*/

#endif /*SATELITE_H*/
