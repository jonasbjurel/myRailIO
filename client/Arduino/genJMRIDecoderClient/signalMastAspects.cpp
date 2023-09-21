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
#include "signalMastAspects.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

signalMastAspects::signalMastAspects(const lgLink* p_parentHandle) {
    parentHandle = (lgLink*)p_parentHandle;
    parentHandle->getLink(&lgLinkNo);
    LOG_INFO("Signal mast aspect object %d for lgLink: %d created" CR, this, lgLinkNo);
}

signalMastAspects::~signalMastAspects(void) {
    panic("signalMastAspects::~signalMastAspects: signalMastAspects destructior not supported");
}

rc_t signalMastAspects::onConfig(const tinyxml2::XMLElement* p_smAspectsXmlElement) {
    LOG_INFO_NOFMT("Configuring Mast aspects" CR);
    tinyxml2::XMLElement* smAspectsXmlElement = (tinyxml2::XMLElement * )p_smAspectsXmlElement;
    for (uint8_t i = 0; i < SM_MAXHEADS; i++) {
        failsafeMastAppearance[i] = UNUSED_APPEARANCE;
    }
    //tinyxml2::XMLElement* smAspectsXmlElement = p_smAspectsXmlElement;
    if ((smAspectsXmlElement = smAspectsXmlElement->FirstChildElement("Aspects")) == NULL || (smAspectsXmlElement = smAspectsXmlElement->FirstChildElement("Aspect")) == NULL || smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL) {
        panic("signalMastAspects::onConfig: XML parsing error, missing Aspects, Aspect, or AspectName");
        return RC_PARSE_ERR;
    }
    //Outer loop itterating all signal mast aspects
    for (uint8_t i = 0; true; i++) {
        LOG_INFO("Parsing Signal mast Aspect: %s" CR, smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
        aspects_t* newAspect = new (heap_caps_malloc(sizeof(aspects_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) aspects_t;
        newAspect->name = new (heap_caps_malloc(sizeof(char) * (strlen(smAspectsXmlElement->FirstChildElement("AspectName")->GetText()) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(smAspectsXmlElement->FirstChildElement("AspectName")->GetText()) + 1];
        strcpy(newAspect->name, smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
        aspects.push_back(newAspect);

        //Mid loop itterating the XML mast types for the aspect
        tinyxml2::XMLElement* mastTypeAspectXmlElement = smAspectsXmlElement->FirstChildElement("Mast");
        if (mastTypeAspectXmlElement == NULL || mastTypeAspectXmlElement->FirstChildElement("Type") == NULL || mastTypeAspectXmlElement->FirstChildElement("Type")->GetText() == NULL) {
            panic("signalMastAspects::onConfig: XML parsing error, missing Mast or Type");
            return RC_PARSE_ERR;
        }
        for (uint16_t j = 0; true; j++) {
            if (mastTypeAspectXmlElement == NULL) {
                break;
            }
            LOG_INFO("Parsing Mast type %s for Aspect %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());

            //Inner loop creating head aspects for a particular mast type
            uint8_t k = 0;
            bool mastAlreadyExist = false;
            for (k = 0; k < aspects.back()->mastTypes.size(); k++) {
                if (!strcmp(aspects.back()->mastTypes.at(k)->name, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText())) {
                    mastAlreadyExist = true;
                    break;
                }
            }
            if (mastAlreadyExist) {
                LOG_WARN("Parsing Mast type %s for Aspect %s already exists, skipping..." CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
                break;
            }
            else { //Creating a new signal mast type'
                LOG_INFO("Creating Mast type %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
                aspects.back()->mastTypes.push_back(new (heap_caps_malloc(sizeof(mastType_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) mastType_t);
                char* newMastTypeName = new (heap_caps_malloc(sizeof(char) * (strlen(mastTypeAspectXmlElement->FirstChildElement("Type")->GetText()) + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(mastTypeAspectXmlElement->FirstChildElement("Type")->GetText()) + 1];
                strcpy(newMastTypeName, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
                aspects.back()->mastTypes.back()->name = newMastTypeName;
                aspects.back()->mastTypes.back()->noOfUsedHeads = atoi(mastTypeAspectXmlElement->FirstChildElement("NoofPxl")->GetText());
                aspects.back()->mastTypes.back()->noOfHeads = ceil(aspects.back()->mastTypes.back()->noOfUsedHeads / 3) * 3;
            }
            tinyxml2::XMLElement* headXmlElement = mastTypeAspectXmlElement->FirstChildElement("Head");
            if (headXmlElement == NULL || headXmlElement->GetText() == NULL) {
                panic("XML parsing error, missing Head");
                return RC_PARSE_ERR;
            }
            LOG_TERSE("Adding Asspect %s to MastType %s" CR, smAspectsXmlElement->FirstChildElement("AspectName")->GetText(), mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
            for (uint8_t p = 0; p < SM_MAXHEADS; p++) {
                if (headXmlElement == NULL) {
                    LOG_INFO("No more Head appearances, padding up with UNUSED_APPEARANCE from Head %d" CR, p);
                    for (uint8_t r = p; r < SM_MAXHEADS; r++) {
                        aspects.back()->mastTypes.back()->headAspects[r] = UNUSED_APPEARANCE;
                    }
                    break;
                }
                if (!strcmp(headXmlElement->GetText(), "LIT")) {
                    LOG_TERSE("Adding LIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
                    aspects.back()->mastTypes.back()->headAspects[p] = LIT_APPEARANCE;
                }
                if (!strcmp(headXmlElement->GetText(), "UNLIT")) {
                    LOG_TERSE("Adding UNLIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
                    aspects.back()->mastTypes.back()->headAspects[p] = UNLIT_APPEARANCE;
                }
                if (!strcmp(headXmlElement->GetText(), "FLASH")) {
                    LOG_TERSE("Adding FLASH_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
                    aspects.back()->mastTypes.back()->headAspects[p] = FLASH_APPEARANCE;
                }
                if (!strcmp(headXmlElement->GetText(), "UNUSED")) {
                    LOG_TERSE("Adding UNUSED_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
                    aspects.back()->mastTypes.back()->headAspects[p] = UNUSED_APPEARANCE;
                }
                headXmlElement = headXmlElement->NextSiblingElement("Head");
            }
            //End inner loop
            mastTypeAspectXmlElement = mastTypeAspectXmlElement->NextSiblingElement("Mast");
        }
        //End middle loop

        if ((smAspectsXmlElement = smAspectsXmlElement->NextSiblingElement("Aspect")) == NULL) {
            break;
        }
        if (smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL) {
            panic("signalMastAspects::onConfig: XML parsing error, missing AspectName");
            return RC_PARSE_ERR;
        }
    }
    //End outer loop
    LOG_INFO_NOFMT("Signal mast aspects successfully configured" CR);
    return RC_OK;
}

void signalMastAspects::dumpConfig(void) {
    LOG_INFO_NOFMT("<Aspect config dump Begin>" CR);
    for (uint8_t i = 0; i < aspects.size(); i++) {
        LOG_INFO("     <Aspect: %s>" CR, aspects.at(i)->name);
        for (uint8_t j = 0; j < aspects.at(i)->mastTypes.size(); j++) {
            LOG_INFO("        <Mast type: %s>" CR, aspects.at(i)->mastTypes.at(j)->name);
            for (uint8_t k = 0; k < SM_MAXHEADS; k++) {
                switch (aspects.at(i)->mastTypes.at(j)->headAspects[k]) {
                case LIT_APPEARANCE:
                    LOG_INFO("            <Head %d: LIT>" CR, k);
                    break;
                case UNLIT_APPEARANCE:
                    LOG_INFO("            <Head %d: UNLIT>" CR, k);
                    break;
                case FLASH_APPEARANCE:
                    LOG_INFO("            <Head %d: FLASHING>" CR, k);
                    break;
                case UNUSED_APPEARANCE:
                    LOG_INFO("            <Head %d: UNUSED>" CR, k);
                    break;
                default:
                    LOG_INFO("            <Head %d: UNKNOWN>" CR, k);
                    break;
                }
            }
        }
    }
}

rc_t signalMastAspects::getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance) {
    if (parentHandle->systemState::getOpStateBitmap() & OP_UNCONFIGURED) {
        LOG_ERROR_NOFMT("Aspects not configured, doing nothing..." CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (!strcmp(p_aspect, "FAULT") || !strcmp(p_aspect, "UNDEFINED")) {
        *p_appearance = failsafeMastAppearance;
        return RC_OK;
    }
    for (uint8_t i = 0; true; i++) {
        if (i > aspects.size() - 1) {
            LOG_ERROR_NOFMT("Aspect doesnt exist, setting mast to failsafe appearance and continuing..." CR);
            *p_appearance = failsafeMastAppearance;
            return RC_GEN_ERR;
        }
        if (!strcmp(p_aspect, aspects.at(i)->name)) {
            for (uint8_t j = 0; true; j++) {
                if (j > aspects.at(i)->mastTypes.size() - 1) {
                    LOG_ERROR_NOFMT("Mast type doesnt exist, setting to failsafe appearance and continuing..." CR);
                    *p_appearance = failsafeMastAppearance;
                    return RC_GEN_ERR;
                }
                if (!strcmp(p_smType, aspects.at(i)->mastTypes.at(j)->name)) {
                    *p_appearance = aspects.at(i)->mastTypes.at(j)->headAspects;
                    return RC_OK;
                }
            }
        }
    }
}

rc_t signalMastAspects::getNoOfHeads(const char* p_smType, uint8_t* p_noOfHeads, bool p_force) {
    if ((parentHandle->systemState::getOpStateBitmap() != OP_WORKING) && !p_force) { //NEEDS ATTENTION
        LOG_ERROR_NOFMT("OP_State is not OP_WORKING, doing nothing..." CR);
        *p_noOfHeads = 0;
        return RC_GEN_ERR;
    }
    for (uint8_t i = 0; i < aspects.size(); i++) {
        for (uint8_t j = 0; j < aspects.at(i)->mastTypes.size(); j++) {
            if (!strcmp(aspects.at(i)->mastTypes.at(j)->name, p_smType)) {
                *p_noOfHeads = aspects.at(i)->mastTypes.at(j)->noOfHeads;
                return RC_OK;
            }
        }
    }
    LOG_ERROR_NOFMT("Mast type not found, doing nothing..." CR);
    *p_noOfHeads = 0;
    return RC_GEN_ERR;
}

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/
