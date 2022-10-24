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
    while (xmlNode != NULL) {
        for (i = 0; i < len; i++) {
            if (!strcmp(tags[i], xmlNode->Name())) {
                if (xmlNode->GetText() != NULL) {
                    if (xmlTxtBuff[i] != NULL) {
                        delete xmlTxtBuff[i];
                    }
                    xmlTxtBuff[i] = new char[strlen(xmlNode->GetText()) + 1];
                    strcpy(xmlTxtBuff[i], xmlNode->GetText());
                }
                break;
            }
        }
        xmlNode = xmlNode->NextSiblingElement();
    }
}

/*==============================================================================================================================================*/
/* END xmlHelpers                                                                                                                               */
/*==============================================================================================================================================*/
