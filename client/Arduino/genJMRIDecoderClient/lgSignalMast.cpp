/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2022 Jonas Bjurel (jonas.bjurel@hotmail.com)
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
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "lgSignalMast.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lgSignalMast                                                                                                                          */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
lgSignalMast::lgSignalMast(const lgBase* p_lgBaseObjHandle) {
    lgBaseObjHandle = (lgBase*)p_lgBaseObjHandle;
    lgLinkHandle = (lgLink*)lgBaseObjHandle->lgLinkHandle;
    lgBaseObjHandle->getSystemName(lgSysName);
    lgBaseObjHandle->getAddress(&lgAddress);
    lgLinkHandle->getLink(&lgLinkNo);
    Log.notice("mastDecoder::mastDecoder: Creating Lg signal mast: %s, with Lg link address: %d, on Lg link %d" CR, lgSysName, lgAddress, lgLinkNo);
    lgSignalMastLock = xSemaphoreCreateMutex();
    lgSignalMastReentranceLock = xSemaphoreCreateMutex();
    if (lgSignalMastLock == NULL || lgSignalMastReentranceLock == NULL)
        panic("mastDecoder::init: Could not create Lock objects - rebooting...");
}

lgSignalMast::~lgSignalMast(void) {
    panic("mastDecoder::~mastDecoder: Destructor not supported - rebooting...");
}

rc_t lgSignalMast::init(void) {
    Log.notice("mastDecoder::init: Initializing mast decoder" CR);
    //failsafe();
    return RC_OK;
}

rc_t lgSignalMast::onConfig(const tinyxml2::XMLElement* p_mastDescXmlElement) {
    if (!(lgBaseObjHandle->getOpState() & OP_UNCONFIGURED))
        panic("mastDecoder:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting...");
    lgBaseObjHandle->getSystemName(lgSysName);
    if (p_mastDescXmlElement == NULL)
        panic("mastDecoder::onConfig: No mastDescXml provided - rebooting...");
    Log.notice("mastDecoder::onConfig:  %d on lgLink %d received an uverified configuration, parsing and validating it..." CR, lgAddress, lgLinkNo);
    const char* searchMastTags[4];
    searchMastTags[SM_TYPE] = "Property1";
    searchMastTags[SM_DIMTIME] = "Property2";
    searchMastTags[SM_FLASHFREQ] = "Property3";
    searchMastTags[SM_BRIGHTNESS] = "Property4"; //NEEDS TO BE IMPLEMENTED BY SERVER SIDE
    searchMastTags[SM_FLASH_DUTY] = "Property5"; //NEEDS TO BE IMPLEMENTED BY SERVER SIDE
    getTagTxt(p_mastDescXmlElement->FirstChildElement(), searchMastTags, xmlConfig, sizeof(searchMastTags) / 4); // Need to fix the addressing for portability
    if (xmlConfig[SM_TYPE] == NULL ||
        xmlConfig[SM_DIMTIME] == NULL ||
        xmlConfig[SM_FLASHFREQ] == NULL ||
        xmlConfig[SM_BRIGHTNESS] == NULL ||
        xmlConfig[SM_FLASH_DUTY] == NULL)
        panic("mastDecoder::onConfig: mastDescXml missformated - rebooting...");
    lgLinkHandle->getSignalMastAspectObj()->getNoOfHeads(xmlConfig[SM_TYPE], &lgNoOfLed);
    appearance = new uint8_t[lgNoOfLed];
    appearanceWriteBuff = new uint8_t[lgNoOfLed];
    appearanceDimBuff = new uint16_t[lgNoOfLed];
    if (!strcmp(xmlConfig[SM_DIMTIME], "NORMAL")) {
        smDimTime = SM_DIM_NORMAL_MS;
    }
    else if (!strcmp(xmlConfig[SM_DIMTIME], "FAST")) {
        smDimTime = SM_DIM_FAST_MS;
    }
    else if (!strcmp(xmlConfig[SM_DIMTIME], "SLOW")) {
        smDimTime = SM_DIM_SLOW_MS;
    }
    else {
        Log.error("mastDecoder::onConfig: smDimTime is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
        smDimTime = SM_DIM_NORMAL_MS;
    }
    if (!strcmp(xmlConfig[SM_BRIGHTNESS], "HIGH")) {
        smBrightness = SM_BRIGHNESS_HIGH;
    }
    else if (!strcmp(xmlConfig[SM_BRIGHTNESS], "NORMAL")) {
        smBrightness = SM_BRIGHNESS_NORMAL;
    }
    else if (!strcmp(xmlConfig[SM_BRIGHTNESS], "LOW")) {
        smBrightness = SM_BRIGHNESS_LOW;
    }
    else {
        Log.error("mastDecoder::onConfig: smBrighness is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
        smBrightness = SM_BRIGHNESS_NORMAL;
    }
    return RC_OK;
}

rc_t lgSignalMast::start(void) {
    Log.notice("mastDecoder::start: Starting mast decoder %s" CR, lgSysName);
    if (strcmp(xmlConfig[SM_FLASHFREQ], "FAST")) {
        lgLinkHandle->getFlashObj(SM_FLASH_FAST)->subscribe(&onFlashHelper, this);
    }
    else if (strcmp(xmlConfig[SM_FLASHFREQ], "NORMAL")) {
        lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->subscribe(&onFlashHelper, this);
    }
    else if (strcmp(xmlConfig[SM_FLASHFREQ], "SLOW")) {
        lgLinkHandle->getFlashObj(SM_FLASH_SLOW)->subscribe(&onFlashHelper, this);
    }
    else {
        Log.error("mastDecoder::start: smFlashFreq is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
        lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->subscribe(&onFlashHelper, this);
    }
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_ASPECT_TOPIC, "/", mqtt::getDecoderUri(), "/", lgSysName);
    mqtt::subscribeTopic(subscribeTopic, &onAspectChangeHelper, this);
    return RC_OK;
}

void lgSignalMast::onSysStateChange(uint16_t p_sysState) {
    if (p_sysState & OP_INTFAIL) {
        //FAILSAFE
        panic("lg::onSystateChange: Signal-mast has experienced an internal error - seting fail-safe aspect and rebooting...");
    }
    if (p_sysState) {
        //FAILSAFE
        Log.notice("lgLink::onSystateChange: Signal-mast %d on lgLink %d has received Opstate %b - seting fail-safe aspect" CR, lgAddress, lgLinkNo, p_sysState);
    }
    else {
        //RESUME LAST RECEIVED ASPECT
        Log.notice("lgLink::onSystateChange:  Signal-mast %d on lgLink %d has received a WORKING Opstate - resuming last known aspect" CR, lgLinkNo);
    }
}

rc_t lgSignalMast::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    Log.notice("mastDecoder::setProperty: Setting light-group property for %s, property Id %d, property value %s" CR, lgSysName, p_propertyId, p_propertyValue);
    //......
}

rc_t lgSignalMast::getProperty(uint8_t p_propertyId, const char* p_propertyValue) {
    Log.notice("mastDecoder::getProperty: Not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
    //......
}

rc_t lgSignalMast::getNoOffLeds(uint8_t* p_noOfLeds) {
    *p_noOfLeds = lgNoOfLed;
}

void lgSignalMast::onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject) {
    ((lgSignalMast*)p_mastObject)->onAspectChange(p_topic, p_payload);
}

void lgSignalMast::onAspectChange(const char* p_topic, const char* p_payload) {
    xSemaphoreTake(lgSignalMastLock, portMAX_DELAY);
    xSemaphoreTake(lgSignalMastReentranceLock, portMAX_DELAY);
    if (lgBaseObjHandle->getOpState()) {
        xSemaphoreGive(lgSignalMastLock);
        xSemaphoreGive(lgSignalMastReentranceLock);
        Log.error("mastDecoder::onAspectChange: A new aspect received, but mast decoder opState is not OP_WORKING - continuing..." CR);
        return;
    }
    xSemaphoreGive(lgSignalMastLock);
    if (parseXmlAppearance(p_payload, aspect)) {
        xSemaphoreGive(lgSignalMastReentranceLock);
        Log.error("mastDecoder::onAspectChange: Failed to parse appearance - continuing..." CR);
        return;
    }
    Log.verbose("mastDecoder::onAspectChange: A new aspect: %s received for signal mast %s" CR, aspect, lgSysName);
    lgLinkHandle->getSignalMastAspectObj()->getAppearance(xmlConfig[SM_TYPE], aspect, &appearance);
    for (uint8_t i = 0; i < lgNoOfLed; i++) {
        appearanceDimBuff[i] = smDimTime;
        switch (appearance[i]) {
        case LIT_APPEARANCE:
            appearanceWriteBuff[i] = smBrightness;
            break;
        case UNLIT_APPEARANCE:
            appearanceWriteBuff[i] = 0;
            break;
        case UNUSED_APPEARANCE:
            appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
            break;
        case FLASH_APPEARANCE:
            if (flashOn) {
                appearanceWriteBuff[i] = smBrightness;
            }
            else {
                appearanceWriteBuff[i] = 0;
            }
            break;
        default:
            Log.error("mastDecoder::onAspectChange: The appearance is none of LIT, UNLIT, FLASH or UNUSED - setting mast to SM_BRIGHNESS_FAIL and continuing..." CR);
            appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL; //HERE WE SHOULD SET THE HOLE MAST TO FAIL ASPECT
        }
    }
    lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset(), lgNoOfLed, appearanceWriteBuff, appearanceDimBuff);
    xSemaphoreGive(lgSignalMastReentranceLock);
    return;
}

rc_t lgSignalMast::parseXmlAppearance(const char* p_aspectXml, char* p_aspect) {
    tinyxml2::XMLDocument aspectXmlDocument;
    if (aspectXmlDocument.Parse(p_aspectXml) || aspectXmlDocument.FirstChildElement("Aspect") == NULL || aspectXmlDocument.FirstChildElement("Aspect")->GetText() == NULL) {
        Log.error("mastDecoder::parseXmlAppearance: Failed to parse the new aspect - continuing..." CR);
        return RC_PARSE_ERR;
    }
    strcpy(p_aspect, aspectXmlDocument.FirstChildElement("Aspect")->GetText());
    return RC_OK;
}

void lgSignalMast::onFlashHelper(bool p_flashState, void* p_flashObj) {
    ((lgSignalMast*)p_flashObj)->onFlash(p_flashState);
}

void lgSignalMast::onFlash(bool p_flashState) {
    xSemaphoreTake(lgSignalMastReentranceLock, portMAX_DELAY);
    flashOn = p_flashState;
    for (uint16_t i = 0; i < lgNoOfLed; i++) {
        if (appearance[i] == FLASH_APPEARANCE) {
            if (flashOn) {
                lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &smBrightness, &smDimTime);
            }
            else {
                uint8_t zero = 0;
                lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &zero, &smDimTime);
            }
        }
    }
    xSemaphoreGive(lgSignalMastReentranceLock);
    return;
}

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/
