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
satelite* satelite[8];
uint8_t noOfSat;
uint8_t actVal;


void newSensorVal(uint8_t linkAddr_p, uint8_t satAddr_p, uint8_t senseAddr_p, bool state_p) {
  Serial.printf("Satelite %d:%d:%d changed value to %d\n", linkAddr_p, satAddr_p, senseAddr_p, state_p);
}

void newSatState(uint8_t linkAddr_p, uint8_t satAddr_p, uint8_t senseAddr_p, satOpState_t opState_p) {
  Serial.printf("Satelite %d:%d:%d changed state to OPSTATE to %x\n", linkAddr_p, satAddr_p, opState_p);
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
  satLink = new sateliteLink(LINK_ADDR, (gpio_num_t)LINK_TX_PIN, (gpio_num_t)LINK_RX_PIN, (rmt_channel_t) RMT_TX_CHAN, (rmt_channel_t)RMT_RX_CHAN, RMT_TX_MEMBANK, RMT_RX_MEMBANK, SCAN_PRIO, SCAN_CORE, SCAN_INTERVAL);
  rc = satLink->enableSatLink();
  if(rc){
    Serial.printf("Failed to enable link, return code %x\n", rc);
    assert(rc == 0);
  }
  noOfSat = satLink->getSatLinkNoOfSats();
  Serial.printf("Found %d satelites on the link\n", noOfSat);
  //assert(noOfSat > 0);
  
  for(uint8_t i = 0; i < noOfSat; i++) {
    satelite[i] = satLink->getsatHandle(i);
    if(satelite[i] == NULL){
      Serial.printf("Unexpected error - No reference to Satelite %d, breaking\n", i);
      vTaskDelay(1000 / portTICK_PERIOD_MS);

      assert(false);
    }
    satelite[i]->satRegSenseCb(&newSensorVal);
    satelite[i]->setSatActMode(SATMODE_PWM1_25K, 0);
    satelite[i]->setSatActMode(SATMODE_PWM100, 1);
    satelite[i]->setSatActMode(SATMODE_PULSE, 2);
    satelite[i]->setSatActMode(SATMODE_PULSE_INV, 3);
    satelite[i]->setSatActVal(0, 0);
    satelite[i]->setSatActVal(0, 1);
    satelite[i]->setSatActVal(0, 2);
    satelite[i]->setSatActVal(0, 3);
    rc = satelite[i]->enableSat();
    if(rc){
      Serial.printf("Failed to enable satelite, return code %x\n", rc);
      assert(rc == 0);
    }
  }
  Serial.printf("%d Satelite enabled\n", noOfSat);
  actVal = 0;
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
  for(uint8_t i = 0; i < noOfSat; i++) {
    satelite[i]->setSatActVal(actVal, 0);
    satelite[i]->setSatActVal(actVal, 1);
    satelite[i]->setSatActVal(actVal, 2);
    satelite[i]->setSatActVal(actVal, 3);
  }
  actVal ++;
  vTaskDelay(300 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
