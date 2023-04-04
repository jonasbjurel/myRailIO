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
    bool foundTags[32] = { false };
    char* tagsStr;
    tagsStr = new char[1024];
    tagsStr[0] = '\0';
    for (uint8_t j = 0; j < len; j++) {
        if (tags[j]) {
            strcat(tagsStr, tags[j]);
            strcat(tagsStr, "|");
        }
    }
    if (strlen(tagsStr) > 1)
        tagsStr[strlen(tagsStr) - 1] = '\0';
    else
        strcpy(tagsStr, "-");
    Log.INFO("getTagTxt: Parsing XML-tags: %s" CR, tagsStr);
    while (xmlNode != NULL) {
        for (i = 0; i < len; i++) {
            found = false;
            if (!tags[i]) {
                foundTags[i] = true;
            }
            else {
                if (!strcmp(tags[i], xmlNode->Name())) {
                    if (xmlNode->GetText() != NULL) {
                        Log.VERBOSE("getTagTxt: XML-tag match: \"%s\", value: \"%s\"" CR, xmlNode->Name(), xmlNode->GetText());
                        found = true;
                        foundTags[i] = true;
                        if (xmlTxtBuff[i])
                            delete xmlTxtBuff[i];
                        xmlTxtBuff[i] = new char[strlen(xmlNode->GetText()) + 1];
                        strcpy(xmlTxtBuff[i], xmlNode->GetText());
                    }
                    else {
                        Log.WARN("getTagTxt: XML-tag match: \"%s\", but with a NULL value" CR, xmlNode->Name());
                    }
                    break;
                }
            }
        }
        if (!found) {
            Log.VERBOSE("getTagTxt: XML-tag: \"%s\" present, but was not searched for" CR, xmlNode->Name());
        }
        xmlNode = xmlNode->NextSiblingElement();
    }
    tagsStr[0] = '\0';
    for (uint8_t j=0; j < len; j++) {
        if (!foundTags[j])
            Log.WARN("getTagTxt: XML-tag: \"%s\" was searched for, but not found, or without a corresponding value" CR, tags[j]);
        else if(tags[j]){
            strcat(tagsStr, tags[j]);
            strcat(tagsStr, "|");
        }
    }
    if (strlen(tagsStr) > 1)
        tagsStr[strlen(tagsStr) - 1] = '\0';
    else
        strcpy(tagsStr, "-");
    Log.TERSE("getTagTxt: Found following requested XML-tags with values: %s" CR, tagsStr);
    delete tagsStr;
}
/*==============================================================================================================================================*/
/* END xmlHelpers                                                                                                                               */
/*==============================================================================================================================================*/
