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

sateliteLink* satLink;
satelite* satelites[MAX_NO_OF_SAT_PER_CH];
bool drain[MAX_NO_OF_SAT_PER_CH];
SemaphoreHandle_t methodAccessCnt[MAX_NO_OF_SAT_PER_CH];

uint8_t noOfSat;
uint8_t actVal;
uint16_t timeCnt = 0;
char* reportBuff;


void newSensorVal(satelite* satelite_p, uint8_t linkAddr_p, uint8_t satAddr_p, uint8_t senseAddr_p, bool state_p) {
  assert(state_p == satelite_p->getSenseVal(senseAddr_p));
  Serial.printf("Sensor %d:%d:%d changed value to %s\n", linkAddr_p, satAddr_p, senseAddr_p, state_p ? "Active" : "Inactive");
}

void newSatState(satelite* satelite_p, uint8_t linkAddr_p, uint8_t satAddr_p, satOpState_t opState_p) {
  char opStateStr[30];

  assert(opState_p == satelite_p->getOpState());
  opStateToStr(opState_p, opStateStr, 30);
  opStateStr[29] = '\0';
  Serial.printf("Satelite %d:%d changed opState to %s\n", linkAddr_p, satAddr_p, opStateStr);
}

void newSatLinkState(sateliteLink* sateliteLink_p, uint8_t linkAddr_p, satOpState_t opState_p) {
  char opStateStr[30];

  assert(opState_p == satLink->getOpState());
  opStateToStr(opState_p, opStateStr, 30);
  opStateStr[29] = '\0';
  Serial.printf("SateliteLink %d: changed opState to %s\n", linkAddr_p, opStateStr);
}


void satdiscovered(satelite* satelite_p, uint8_t linkAddr_p, uint8_t satAddr_p, bool exists_p){
satErr_t rc;

  //Serial.printf("GOT A DISCOVER CALLBACK\n");
  if (exists_p){
    satelites[satAddr_p] = satelite_p;
    noOfSat++;
    drain[satAddr_p] = false;
    Serial.printf("Satelite with address %d discovered, %d discovered satelites\n", satAddr_p, noOfSat);
    if(satelite_p == NULL){
      Serial.printf("Unexpected error - No reference to Satelite %d, breaking\n", satAddr_p);
      assert(false);
    }
    assert(noOfSat == satLink->getSatLinkNoOfSats());
    assert(linkAddr_p == satLink->getAddress());
    assert(satelite_p == satLink->getsatHandle(satAddr_p));
    satelites[satAddr_p]->satRegSenseCb(&newSensorVal);
    satelites[satAddr_p]->satRegStateCb(&newSatState);
    Serial.printf("Registered sensor callback for Satelite %d\n", satAddr_p);
    satelites[satAddr_p]->setSatActMode(SATMODE_PWM1_25K, 0);
    satelites[satAddr_p]->setSatActMode(SATMODE_PWM100, 1);
    satelites[satAddr_p]->setSatActMode(SATMODE_PULSE, 2);
    satelites[satAddr_p]->setSatActMode(SATMODE_PULSE_INV, 3);
    Serial.printf("Set actuator modes for Satelite %d\n", satAddr_p);
    for(uint8_t i = 0; i < NO_OF_ACT; i++)
      satelites[satAddr_p]->setSatActVal(0, i);
    satelites[satAddr_p]->setSenseFilter(5000, 7); //WE NEED TO DEVELOP A TESTCASE AROUND THIS
    satelites[satAddr_p]->setErrTresh(10, 1);
    Serial.printf("Set initial actuator values for Satelite %d\n", satAddr_p);
    rc = satelites[satAddr_p]->enableSat();
    if(rc){
      Serial.printf("Failed to enable satelite %d, return code %x\n", satAddr_p, rc);
      assert(rc == 0);
    }
    satelites[satAddr_p]->clearSatStats();
    Serial.printf("Enabled Satelite %d\n", satAddr_p);
    return;
  }
  else {
    satelites[satAddr_p] = NULL;
    noOfSat--;
    drain[satAddr_p] = true;
    while (uxSemaphoreGetCount(methodAccessCnt[satAddr_p]))
      vTaskDelay(1 / portTICK_PERIOD_MS);
    Serial.printf("Satelite with address %d deleted, %d discovered satelites\n", satAddr_p, noOfSat);
    return;
  }  
}


void selfTestRes(satelite* satHandle_p, uint8_t satLinkAddr_p, uint8_t satAddr_p, satErr_t err_p){
  Serial.printf("Selftest of satelite %d:%d ended with result code %x\n", satLinkAddr_p, satAddr_p, err_p);
  assert (err_p == 0);
}
/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void setup() {
  satErr_t rc;
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++){
    satelites[i] = NULL;
    drain[i] = true;
    methodAccessCnt[i] = xSemaphoreCreateCounting(255,0);
    if (methodAccessCnt[i] == NULL){
      Serial.printf("Failed to create semaphore\n");
      assert(false);
    }
  }
  
  satLink = new sateliteLink(LINK_ADDRESS, (gpio_num_t)LINK_TX_PIN, (gpio_num_t)LINK_RX_PIN, (rmt_channel_t) RMT_TX_CHAN, (rmt_channel_t)RMT_RX_CHAN, RMT_TX_MEMBANK, RMT_RX_MEMBANK, SCAN_PRIO, SCAN_CORE, SCAN_INTERVAL);
  satLink->satLinkRegSatDiscoverCb(&satdiscovered); 
  rc = satLink->enableSatLink();
  if(rc){
    Serial.printf("Failed to enable link, return code %x\n", rc);
    assert(rc == 0);
  }

  satLink->satLinkRegStateCb(&newSatLinkState);
  satLink->setErrTresh(20, 2);
  Serial.printf("Satelite Link enabled\n");
  actVal = 0;
  reportBuff = (char*)malloc(2000);
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
int loopcnt = 0;
void loop() {
  uint16_t buffPointer;
  satPerformanceCounters_t satLinkStats, satStats;
  satErr_t rc;

  if(loopcnt == 45){
    for (uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
      if (!drain[i] && satelites[i] != NULL){
        Serial.printf("Starting self-test for satelite %d\n", i);
        assert(satelites[i]->satSelfTest(&selfTestRes) == SAT_OK);
      }
    }
  }
  if(loopcnt == 90){
    Serial.printf("Disabling all satelites\n");
    for(uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
      if (!drain[i] && satelites[i] != NULL){
        rc = satelites[i]->disableSat();
        if(rc){
          Serial.printf("Failed to disable satelite %d, return code %x\n", i, rc);
          assert(rc == 0);
        }
      } 
    }
  }

  if(loopcnt == 180){
    Serial.printf("Re-enabling all satelites\n");
    for(uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
      if (!drain[i] && satelites[i] != NULL){
        rc = satelites[i]->enableSat();
        if(rc){
          Serial.printf("Failed to disable satelite %d, return code %x\n", i, rc);
          assert(rc == 0);
        }
      } 
    }
  }
  if(loopcnt == 270){
    Serial.printf("disabling satelite Link\n");
    for(uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
      if (!drain[i] && satelites[i] != NULL){
        rc = satelites[i]->disableSat();
        if(rc){
          Serial.printf("Failed to disable satelite %d, return code %x\n", i, rc);
          assert(rc == 0);
        }
      } 
    }
    rc = satLink->disableSatLink();
    if(rc){
      Serial.printf("Failed to disable link, return code %x\n", rc);
      assert(rc == 0);
    }
  }
  if(loopcnt == 360){
    Serial.printf("Re-enabling satelite Link\n");
    rc = satLink->enableSatLink();
    if(rc){
      Serial.printf("Failed to enable link, return code %x\n", rc);
      assert(rc == 0);
    }
  }
/*
  if(loopcnt == 450){
    Serial.printf("Re-enabling satelite Link\n");
    rc = satLink->enableSatLink();
    if(rc){
      Serial.printf("Failed to enable link, return code %x\n", rc);
      assert(rc == 0);
    }
  }*/
    
  for(uint8_t i = 0; i < MAX_NO_OF_SAT_PER_CH; i++) {
    if (!drain[i] && satelites[i] != NULL){
      xSemaphoreGive(methodAccessCnt[i]);
      satelites[i]->setSatActVal(actVal, 0);
      satelites[i]->setSatActVal(actVal, 1);
      satelites[i]->setSatActVal(128, 2);
      satelites[i]->setSatActVal(64, 3);
      xSemaphoreTake(methodAccessCnt[i], portMAX_DELAY);
    }
  }
  //Serial.printf("Count is %d\n", uxSemaphoreGetCount(methodAccessCnt[0]));

  actVal ++;

  if(timeCnt >= 30){
    timeCnt = 0;
    buffPointer = 0;

    satLink->getSatStats(&satLinkStats, false); 
    formatSatStat(reportBuff, 2000, &buffPointer, buffPointer, satLink->getAddress(), 0, satLink->getAdmState(), 
                  satLink->getOpState(), &satLinkStats,
                  LINK_ADDR|SAT_ADDR|RX_SIZE_ERR|RX_SYMB_ERR|TIMING_VIOLATION_ERR|RX_CRC_ERR|REMOTE_CRC_ERR|WATCHDG_ERR|ADM_STATE|OP_STATE,
                  LINK_ADDR|RX_SIZE_ERR|RX_SYMB_ERR|TIMING_VIOLATION_ERR|RX_CRC_ERR|REMOTE_CRC_ERR|WATCHDG_ERR|ADM_STATE|OP_STATE,
                  true);
 
    for(uint8_t i = 0; i < noOfSat; i++){
      if (!drain[i] & satelites[i] != NULL){
        xSemaphoreGive(methodAccessCnt[i]);
        satelites[i]->getSatStats(&satStats, false);
        formatSatStat(reportBuff, 2000, &buffPointer, buffPointer, satLink->getAddress(), satelites[i]->getAddress(), 
                      satelites[i]->getAdmState(), satelites[i]->getOpState(), &satStats, 
                      LINK_ADDR|SAT_ADDR|RX_SIZE_ERR|RX_SYMB_ERR|TIMING_VIOLATION_ERR|RX_CRC_ERR|REMOTE_CRC_ERR|WATCHDG_ERR|ADM_STATE|OP_STATE,
                      LINK_ADDR|SAT_ADDR|RX_CRC_ERR|REMOTE_CRC_ERR|WATCHDG_ERR|ADM_STATE|OP_STATE,
                      false);
        xSemaphoreTake(methodAccessCnt[i], portMAX_DELAY);
      } 
    }
    Serial.printf("%s",reportBuff);
  }
  else {
    timeCnt++;
  }
  loopcnt++;
  vTaskDelay(300 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
