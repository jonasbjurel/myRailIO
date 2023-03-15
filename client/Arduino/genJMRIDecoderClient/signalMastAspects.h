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

#ifndef SMASPECTS_H
#define SMASPECTS_H

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <QList.h>
#include "libraries/tinyxml2/tinyxml2.h"
#include "libraries/ArduinoLog/ArduinoLog.h"
#include "lgLink.h"
#include "rc.h"
#include "config.h"
#include "panic.h"
#include "strHelpers.h"
#include "xmlHelpers.h"
class lgLink;



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define SM_MAXHEADS             12
#define UNLIT_APPEARANCE        0
#define LIT_APPEARANCE          1
#define FLASH_APPEARANCE        2
#define UNUSED_APPEARANCE       3

struct mastType_t {
    char* name;
    uint8_t headAspects[SM_MAXHEADS];
    uint8_t noOfHeads;
    uint8_t noOfUsedHeads;
};

struct aspects_t {
    char* name;
    QList<mastType_t*> mastTypes;
};


class signalMastAspects {
public:
    //Public methods
    signalMastAspects(const lgLink* p_parentHandle);
    ~signalMastAspects(void);
    rc_t onConfig(tinyxml2::XMLElement* p_smAspectsXmlElement);
    void dumpConfig(void);
    rc_t getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance);
    rc_t getNoOfHeads(const char* p_smType, uint8_t* p_noOfHeads);
    //Public data structures
    //--

private:
    //Private methods
    //--

    //Private data structures
    lgLink* parentHandle;
    uint8_t lgLinkNo;
    uint8_t failsafeMastAppearance[SM_MAXHEADS];
    QList<aspects_t*> aspects;
};

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/
#endif /*SMASPECTS_H*/
