/*==============================================================================================================================================*/
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
    mastDesc.lgBaseObjHandle = (lgBase*)p_lgBaseObjHandle;
    mastDesc.lgLinkHandle = (lgLink*)mastDesc.lgBaseObjHandle->lgLinkHandle;
    mastDesc.lgLinkHandle->getLink(&mastDesc.lgLinkNo);
    mastDesc.lgBaseObjHandle->getAddress(&mastDesc.lgAddress, true);
    mastDesc.lgBaseObjHandle->getSystemName(mastDesc.lgSysName, true);
    Log.INFO("mastDecoder::mastDecoder: Creating Lg signal mast: %s, with Lg link address: %d, on Lg link %d" CR, mastDesc.lgSysName, mastDesc.lgAddress, mastDesc.lgLinkNo);
    lgSignalMastLock = xSemaphoreCreateMutex();
    lgSignalMastReentranceLock = xSemaphoreCreateMutex();
    if (lgSignalMastLock == NULL || lgSignalMastReentranceLock == NULL)
        panic("mastDecoder::init: Could not create Lock objects - rebooting..." CR);
    xmlConfig[SM_TYPE] = NULL;
    xmlConfig[SM_DIMTIME] = createNcpystr("NORMAL");
    xmlConfig[SM_FLASHFREQ] = createNcpystr("NORMAL");
    xmlConfig[SM_BRIGHTNESS] = createNcpystr("NORMAL"); //NOT SUPORTED BY SERVER
    xmlConfig[SM_FLASH_DUTY] = createNcpystr("50"); //NOT SUPORTED BY EITHER SERVER NOR CLIENT
}

lgSignalMast::~lgSignalMast(void) {
    panic("mastDecoder::~mastDecoder: Destructor not supported - rebooting..." CR);
}

rc_t lgSignalMast::init(void) {
    Log.INFO("mastDecoder::init: Initializing mast decoder" CR);
    //failsafe();
    return RC_OK;
}
rc_t lgSignalMast::onConfig(const tinyxml2::XMLElement* p_mastDescXmlElement) {
    if (!(mastDesc.lgBaseObjHandle->systemState::getOpState() & OP_UNCONFIGURED))
        panic("mastDecoder:onConfig: Received a configuration, while the it was already configured, dynamic re-configuration not supported - rebooting..." CR);
    if (p_mastDescXmlElement == NULL)
        panic("mastDecoder::onConfig: No mastDescXml provided - rebooting..." CR);
    Log.INFO("mastDecoder::onConfig:  %d on lgLink %d received an unverified configuration, parsing and validating it..." CR, mastDesc.lgAddress, mastDesc.lgLinkNo);
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
    if (parseProperties())
        panic("mastDecoder::onConfig: Could not set lgMast properties - rebooting...");
    Log.info("lgSignalMast::onConfig: Successfully configured lgMast - Mast Type: %s, Dim time: %s, Flash Freq: %s, Flash duty: %s, Brightness: %s" CR, xmlConfig[SM_TYPE], xmlConfig[SM_DIMTIME], xmlConfig[SM_FLASHFREQ], xmlConfig[SM_FLASH_DUTY], xmlConfig[SM_BRIGHTNESS]);
    mastDesc.lgLinkHandle->getSignalMastAspectObj()->getNoOfHeads(mastDesc.lgSmType, &mastDesc.lgNoOfLed, true);
    Serial.printf("####No of LEDs: %i\n", mastDesc.lgNoOfLed);
    appearance = new uint8_t[mastDesc.lgNoOfLed];
    appearanceWriteBuff = new uint8_t[mastDesc.lgNoOfLed];
    appearanceDimBuff = new uint8_t[mastDesc.lgNoOfLed];
    return RC_OK;
}

rc_t lgSignalMast::start(void) {
    Log.INFO("mastDecoder::start: Starting mast decoder %s" CR, mastDesc.lgSysName);
    char subscribeTopic[300];
    sprintf(subscribeTopic, "%s%s%s%s%s", MQTT_ASPECT_TOPIC, "/", mqtt::getDecoderUri(), "/", mastDesc.lgSysName);
    mqtt::subscribeTopic(subscribeTopic, &onAspectChangeHelper, this);
    return RC_OK;
}

void lgSignalMast::onSysStateChange(uint16_t p_sysState) {
    if (p_sysState & OP_INTFAIL) {
        //FAILSAFE
        panic("mastDecoder::onSystateChange: Signal-mast has experienced an internal error - seting fail-safe aspect and rebooting..." CR);
    }
    if (p_sysState) {
        //FAILSAFE
        Log.INFO("mastDecoder::onSystateChange: Signal-mast %d on lgLink %d has received Opstate %b - seting fail-safe aspect" CR, mastDesc.lgAddress, mastDesc.lgLinkNo, p_sysState);
    }
    else {
        //RESUME LAST RECEIVED ASPECT
        Log.INFO("mastDecoder::onSystateChange:  Signal-mast %d on lgLink %d has received a WORKING Opstate - resuming last known aspect" CR, mastDesc.lgLinkNo);
    }
}

rc_t lgSignalMast::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    Log.INFO("mastDecoder::setProperty: Setting light-group property for %s, property Id %d, property value %s" CR, mastDesc.lgSysName, p_propertyId, p_propertyValue);
    strcpy(xmlConfig[SM_TYPE + p_propertyId], p_propertyValue);
    parseProperties();
    setProperties();
    return RC_OK;
}

rc_t lgSignalMast::getNoOffLeds(uint8_t* p_noOfLeds, bool p_force) {
    *p_noOfLeds = mastDesc.lgNoOfLed;
    return RC_OK;
}

void lgSignalMast::getShowing(const char* p_showing){
    p_showing = aspect;
}

void lgSignalMast::setShowing(const char* p_showing) {
    char aspect[100];
    sprintf(aspect, "<Aspect>%s</Aspect>");
    onAspectChange(NULL, aspect);
}

rc_t lgSignalMast::parseProperties(void) {
    strcpy(mastDesc.lgSmType, xmlConfig[SM_TYPE]);
    if (!strcmp(xmlConfig[SM_DIMTIME], "NORMAL")) {
        mastDesc.smDimTime = SM_DIM_NORMAL_MS;
    }
    else if (!strcmp(xmlConfig[SM_DIMTIME], "FAST")) {
        mastDesc.smDimTime = SM_DIM_FAST_MS;
    }
    else if (!strcmp(xmlConfig[SM_DIMTIME], "SLOW")) {
        mastDesc.smDimTime = SM_DIM_SLOW_MS;
    }
    else {
        Log.ERROR("mastDecoder::parseProperties: property2/smDimTime is none of FAST, NORMAL or SLOW - using NORMAL..." CR);
        mastDesc.smDimTime = SM_DIM_NORMAL_MS;
    }
    if (!strcmp(xmlConfig[SM_FLASHFREQ], "NORMAL")) {
        mastDesc.smFlashFreq = SM_FLASH_NORMAL;
        //lgLinkHandle->getFlashObj(SM_FLASH_FAST)->subscribe(&onFlashHelper, this);
    }
    else if (!strcmp(xmlConfig[SM_FLASHFREQ], "FAST")) {
        mastDesc.smFlashFreq = SM_FLASH_FAST;
        //lgLinkHandle->getFlashObj(SM_FLASH_FAST)->subscribe(&onFlashHelper, this);
    }
    else if (!strcmp(xmlConfig[SM_FLASHFREQ], "SLOW")) {
        mastDesc.smFlashFreq = SM_FLASH_SLOW;
        //lgLinkHandle->getFlashObj(SM_FLASH_SLOW)->subscribe(&onFlashHelper, this);
    }
    else {
        Log.ERROR("mastDecoder::parseProperties: property3/smFlashFreq is none of FAST, NORMAL or SLOW - using NORMAL..." CR);
        mastDesc.smFlashFreq = SM_FLASH_NORMAL;
        //lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->subscribe(&onFlashHelper, this);
    }
    if (!strcmp(xmlConfig[SM_BRIGHTNESS], "HIGH")) {
        mastDesc.smBrightness = SM_BRIGHNESS_HIGH;
    }
    else if (!strcmp(xmlConfig[SM_BRIGHTNESS], "NORMAL")) {
        mastDesc.smBrightness = SM_BRIGHNESS_NORMAL;
    }
    else if (!strcmp(xmlConfig[SM_BRIGHTNESS], "LOW")) {
        mastDesc.smBrightness = SM_BRIGHNESS_LOW;
    }
    else {
        Log.ERROR("mastDecoder::parseProperties: smBrighness is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
        mastDesc.smBrightness = SM_BRIGHNESS_NORMAL;
    }
    if (!strcmp(xmlConfig[SM_FLASH_DUTY], "HIGH")) {
        mastDesc.smFlashDuty = SM_DUTY_HIGH;
    }
    else if (!strcmp(xmlConfig[SM_FLASH_DUTY], "NORMAL")) {
        mastDesc.smFlashDuty = SM_DUTY_NORMAL;
    }
    else if (!strcmp(xmlConfig[SM_FLASH_DUTY], "LOW")) {
        mastDesc.smFlashDuty = SM_DUTY_LOW;
    }
    else {
        Log.ERROR("mastDecoder::parseProperties: smDuty is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
        mastDesc.smFlashDuty = SM_DUTY_NORMAL;
    }
    return RC_OK;
}

rc_t lgSignalMast::setProperties(void) {
    mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->unSubscribe(&onFlashHelper);
    mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_FAST)->unSubscribe(&onFlashHelper);
    mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_SLOW)->unSubscribe(&onFlashHelper);
    if (mastDesc.smFlashFreq == SM_FLASH_NORMAL) {
        mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->subscribe(&onFlashHelper, this);
    }
    else if (mastDesc.smFlashFreq == SM_FLASH_FAST) {
        mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_FAST)->subscribe(&onFlashHelper, this);
    }
    else if (mastDesc.smFlashFreq == SM_FLASH_SLOW) {
        mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_SLOW)->subscribe(&onFlashHelper, this);
    }
    else {
        Log.ERROR("mastDecoder::start: smFlashFreq is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
        mastDesc.lgLinkHandle->getFlashObj(SM_FLASH_NORMAL)->subscribe(&onFlashHelper, this);
    }
    return RC_OK;
}


void lgSignalMast::onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject) {
    ((lgSignalMast*)p_mastObject)->onAspectChange(p_topic, p_payload);
}

void lgSignalMast::onAspectChange(const char* p_topic, const char* p_payload) {
    xSemaphoreTake(lgSignalMastLock, portMAX_DELAY);
    xSemaphoreTake(lgSignalMastReentranceLock, portMAX_DELAY);
    if (mastDesc.lgBaseObjHandle->systemState::getOpState()) {
        xSemaphoreGive(lgSignalMastLock);
        xSemaphoreGive(lgSignalMastReentranceLock);
        Log.ERROR("mastDecoder::onAspectChange: A new aspect received, but mast decoder opState is not OP_WORKING - continuing..." CR);
        return;
    }
    xSemaphoreGive(lgSignalMastLock);
    if (parseXmlAppearance(p_payload, aspect)) {
        xSemaphoreGive(lgSignalMastReentranceLock);
        Log.ERROR("mastDecoder::onAspectChange: Failed to parse appearance - continuing..." CR);
        return;
    }
    Log.VERBOSE("mastDecoder::onAspectChange: A new aspect: %s received for signal mast %s" CR, aspect, mastDesc.lgSysName);
    mastDesc.lgLinkHandle->getSignalMastAspectObj()->getAppearance(xmlConfig[SM_TYPE], aspect, &appearance);
    for (uint8_t i = 0; i < mastDesc.lgNoOfLed; i++) {
        appearanceDimBuff[i] = mastDesc.smDimTime;
        switch (appearance[i]) {
        case LIT_APPEARANCE:
            appearanceWriteBuff[i] = mastDesc.smBrightness;
            break;
        case UNLIT_APPEARANCE:
            appearanceWriteBuff[i] = 0;
            break;
        case UNUSED_APPEARANCE:
            appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
            break;
        case FLASH_APPEARANCE:
            if (flashOn) {
                appearanceWriteBuff[i] = mastDesc.smBrightness;
            }
            else {
                appearanceWriteBuff[i] = 0;
            }
            break;
        default:
            Log.ERROR("mastDecoder::onAspectChange: The appearance is none of LIT, UNLIT, FLASH or UNUSED - setting mast to SM_BRIGHNESS_FAIL and continuing..." CR);
            appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL; //HERE WE SHOULD SET THE HOLE MAST TO FAIL ASPECT
        }
    }
    mastDesc.lgLinkHandle->updateLg(mastDesc.lgBaseObjHandle->getStripOffset(), mastDesc.lgNoOfLed, appearanceWriteBuff, appearanceDimBuff);
    xSemaphoreGive(lgSignalMastReentranceLock);
    return;
}

rc_t lgSignalMast::parseXmlAppearance(const char* p_aspectXml, char* p_aspect) {
    tinyxml2::XMLDocument aspectXmlDocument;
    if (aspectXmlDocument.Parse(p_aspectXml) || aspectXmlDocument.FirstChildElement("p_showing") == NULL || aspectXmlDocument.FirstChildElement("Aspect")->GetText() == NULL) {
        Log.ERROR("mastDecoder::parseXmlAppearance: Failed to parse the new aspect - continuing..." CR);
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
    for (uint16_t i = 0; i < mastDesc.lgNoOfLed; i++) {
        if (appearance[i] == FLASH_APPEARANCE) {
            if (flashOn) {
                mastDesc.lgLinkHandle->updateLg(mastDesc.lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &mastDesc.smBrightness, &mastDesc.smDimTime);
            }
            else {
                uint8_t zero = 0;
                mastDesc.lgLinkHandle->updateLg(mastDesc.lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &zero, &mastDesc.smDimTime);
            }
        }
    }
    xSemaphoreGive(lgSignalMastReentranceLock);
    return;
}

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/
