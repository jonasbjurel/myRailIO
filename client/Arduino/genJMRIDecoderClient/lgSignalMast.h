/*============================================================================================================================================= =*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonasbjurel@hotmail.com)
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
#ifndef LGSIGNALMAST_H
#define LGSIGNALMAST_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <cstddef>
#include "libraries/tinyxml2/tinyxml2.h"
#include <ArduinoLog.h>
#include "rc.h"
#include "lgBase.h"
#include "lgLink.h"
#include "mqtt.h"
#include "mqttTopics.h"
#include "config.h"
#include "panic.h"
class lgBase;
class lgLink;



/*==============================================================================================================================================*/
/* Class: lgSignalMast                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define SM_TYPE                             0
#define SM_DIMTIME                          1                                           //Property 1 in xml config
#define SM_FLASHFREQ                        2                                           //Property 2 in xml config
#define SM_BRIGHTNESS                       3                                           //Property 3 in xml config
#define SM_FLASH_DUTY                       4                                           //Property 4 in xml config

struct mastDesc_t {
    lgLink* lgLinkHandle;
    lgBase* lgBaseObjHandle;
    uint8_t lgLinkNo;
    uint8_t lgAddress;
    uint8_t lgNoOfLed;
    char lgType[50];
    char lgSysName[100];
    char lgSmType[100];                                                                 //Property 1  in xml config, SM type
    uint8_t smDimTime;                                                                  //Property 2  in xml config, SM Dim timing in ms (0-255) for this signal mast
    uint8_t smFlashFreq;                                                                //Property 3  in xml config, SM Flash Freq. abstraction: {SM_FLASH_NORMAL, SM_FLASH_FAST, SM_FLASH_SLOW}
    uint8_t smBrightness;                                                               //Property 4  in xml config, SM Brightness (0-255) for this signal mast
    uint8_t smFlashDuty;                                                                //Property 5  in xml config, SM Flash duty-cycle (0-255) for this signal mast
};

class lgSignalMast {
public:
    //Public methods
    lgSignalMast(const lgBase* p_lgBaseObjHandle);
    ~lgSignalMast(void);                            //destructor not implemented - if called a fatal exception and reboot will occur!
    rc_t init(void);
    rc_t onConfig(const tinyxml2::XMLElement* p_mastDescXmlElement);
    rc_t start(void);                            //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    void onSysStateChange(sysState_t p_sysState);
    rc_t setProperty(uint8_t p_propertyId, const char* p_propertyValue);
    rc_t getNoOffLeds(uint8_t* p_noOfLeds, bool p_force = false);
    void getShowing(const char* p_showing);
    void setShowing(const char* p_showing);
    rc_t parseProperties(void);
    rc_t setProperties(void);
    static void onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject);
    void onAspectChange(const char* p_topic, const char* p_payload); //Mqtt callback at aspect change
    static void onFlashHelper(bool p_flashState, void* p_flashObj);
    void onFlash(bool p_flashState);          //Flash object call back
    void failSafe(bool p_set);


    //Public data structures
    //--

private:
    //Private methods
    rc_t parseXmlAppearance(const char* p_aspectXml, char* p_aspect); //Parses a new aspect from *p_aspectXml and provides clear text aspect text in p_aspect, Returns RC_OK if successful

    //Private data structures
    mastDesc_t mastDesc;
    bool failsafeSet;
    char* xmlConfig[5];
    bool debug;
    SemaphoreHandle_t lgSignalMastLock;             //Mutex to protect common object datastructures from multi thread parallel access
    SemaphoreHandle_t lgSignalMastReentranceLock;   //Mutex to avoid re-entrance caused by new aspects or flash events
    char aspect[80];                                //Curren mastDecoder aspect - clear text name
    uint8_t* appearance;                            //Holds the Appearance signatures for all heads in the mast (LIT, UNLIT, FLASH, UNUSED..), See definitions in the mastaspect class
    uint8_t* tmpAppearance;                         //Temporarrily stores the appearance as received from the lgs decoder [SM_MAXHEADS]
    uint8_t* appearanceWriteBuff;                   //Holds the wanted raw led wanted values to be passed to the lgLink for rendering
    bool flashOn;                                   //Holds the flash state for all heads in the mast currently flashing
};

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/

#endif /*LGSIGNALMAST_H*/
