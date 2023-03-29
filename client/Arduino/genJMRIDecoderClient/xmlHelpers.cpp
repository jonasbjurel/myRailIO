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
#include "xmlHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Helper: xmlHelpers                                                                                                                           */
/* Purpose: Provides helper functions for xml handling                                                                                          */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
void getTagTxt(const tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len) {
    int i;
    bool found = false;
    bool foundTags[256] = { false };
    while (xmlNode != NULL) {
        for (i = 0; i < len; i++) {
            found = false;
            if (!tags[i]) {
                Serial.printf("### Search-tag %i is NULL\n", i);
                foundTags[i] = true;
            }
            else {
                Serial.printf("Search-tag: \"%s\", xml-tag: \"%s\"\n", tags[i], xmlNode->Name());
                if (!strcmp(tags[i], xmlNode->Name())) {
                    if (xmlNode->GetText() != NULL) {
                        Log.VERBOSE("getTagTxt: XML search match, tag: \"%s\", value: \"%s\"" CR, xmlNode->Name(), xmlNode->GetText());
                        found = true;
                        foundTags[i] = true;
                        if (xmlTxtBuff[i])
                            delete xmlTxtBuff[i];
                        xmlTxtBuff[i] = new char[strlen(xmlNode->GetText()) + 1];
                        strcpy(xmlTxtBuff[i], xmlNode->GetText());
                    }
                    else {
                        Log.ERROR("getTagTxt: XML search match, tag: \"%s\", but with a NULL value" CR, xmlNode->Name());
                    }
                    break;
                }
            }
        }
        if (!found) {
            Log.VERBOSE("getTagTxt: XML tag: \"%s\" present, but was not searched for" CR, xmlNode->Name());
        }
        xmlNode = xmlNode->NextSiblingElement();
    }
    for (uint8_t j=0; j < len; j++) {
        if (!foundTags[j])
            Log.ERROR("getTagTxt: XML tag: \"%s\" was searched for, but not found" CR, tags[j]);
    }
}
/*==============================================================================================================================================*/
/* END xmlHelpers                                                                                                                               */
/*==============================================================================================================================================*/
