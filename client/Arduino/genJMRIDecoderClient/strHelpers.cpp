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
#include "strHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Helper: strHelpers                                                                                                                           */
/* Purpose: Provides helper functions for string handling                                                                                       */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
char* createNcpystr(const char* src) {
    int length = strlen(src);
    char* dst = new char[length + 1];
    if (dst == NULL) {
        Log.ERROR("createNcpystr: Failed to allocate memory from heap - rebooting..." CR);
        ESP.restart();
    }
    strcpy(dst, src);
    dst[length] = '\0';
    return dst;
}

/* Flawed
char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings) {
    int resLen = 0;
    char* dst;
    for (uint8_t i = 0; i < noOfSrcStrings; i++) {
        resLen += strlen(srcStrings[i]);
    }
    dst = new char[resLen + 1];
    char* stringptr = dst;
    for (uint8_t i = 0; i < noOfSrcStrings; i++) {
        strcpy(stringptr, srcStrings[i]);
        stringptr += strlen(srcStrings[i]);
    }
    dst[resLen] = '\0';
    return dst;
}
*/

bool isUri(const char* p_uri) {
    bool prevDot = false;
    if (p_uri[0] == '.' || p_uri[strlen(p_uri) - 1] == '.')
        return false;
    for (uint16_t i = 0; i < strlen(p_uri); i++) {
        if (!(isAlphaNumeric(p_uri[i]) || p_uri[i] == '.' || p_uri[i] == '_'))
            return false;
        if (p_uri[i] == '.') {
            if (prevDot)
                return false;
            else
                prevDot = true;
        }
        else
            prevDot = false;
    }
    return true;
}

bool isIntNumberStr(const char* p_numberStr) {
    for (uint8_t i = 0; i < strlen(p_numberStr); i++) {
        if (!isDigit(*(p_numberStr + i)))
            return false;
    }
    return true;
}

bool isFloatNumberStr(const char* p_numberStr) {
    char* floatItter;
    char* floatTok;
    floatTok = (char*)p_numberStr;
    if (!(floatItter = strtok(floatTok, ".")))
        return false;
    if (!isIntNumberStr(floatItter))
        return false;
    if (!isIntNumberStr(floatTok))
        return false;
    return true;
}

char* trimSpace(char* p_s) {
    Serial.printf("Before: 0x%X, 0x%X, 0x%X, 0x%X \n", *p_s, *(p_s + 1), *(p_s + 2), *(p_s + 3));
    char* s = p_s;
    char* d = p_s;
    uint8_t itter = 0;
    do {
        Serial.printf("Itteration %i\n", itter++);
        Serial.printf("Itter result: 0x%X, 0x%X, 0x%X, 0x%X \n", *p_s, *(p_s + 1), *(p_s + 2), *(p_s + 3));
        while (*d == ' ') {
            Serial.printf("Forwarding");
            ++d;
        } 
        Serial.printf("Setting result to 0x%X\n", *d);
    } while (*s++ = *d++);
    Serial.printf("After: 0x%X, 0x%X, 0x%X, 0x%X \n", *p_s, *(p_s + 1), *(p_s + 2), *(p_s + 3));
    Serial.printf("AfterStr: %s\n", p_s);
    return p_s;
}

/*==============================================================================================================================================*/
/* END strHelpers                                                                                                                               */
/*==============================================================================================================================================*/
