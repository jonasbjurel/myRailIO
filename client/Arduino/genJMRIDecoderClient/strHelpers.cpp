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
char* createNcpystr(const char* src, bool internal) {
    int length = strlen(src);
    char* dst;
    if(internal)
        dst = new(heap_caps_malloc(sizeof(char) * (length + 1), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) char[length + 1];
    else
        dst = new(heap_caps_malloc(sizeof(char) * (length + 1), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[length + 1];
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
    char* s = p_s;
    char* d = p_s;
    uint8_t itter = 0;
    do {
        while (*d == ' ') {
            ++d;
        } 
    } while (*s++ = *d++);
    return p_s;
}

const char* trimNlCr(char* p_str) {
    for (uint32_t i = 0; i < strlen(p_str); i++) {
        if (*(p_str + i) == '\n' || *(p_str + i) == '\r')
            *(p_str + i) = ' ';
    }
    return p_str;
}

void strcpyTruncMaxLen(char* p_dest, const char* p_src, uint p_maxStrLen) {
    if (strlen(p_src) <= p_maxStrLen) {
        strcpy(p_dest, p_src);
    }
    else {
        memcpy(p_dest, p_src, p_maxStrLen - 3);
        memcpy(p_dest + p_maxStrLen - 3, "...\0", 4);
    }
    trimNlCr(p_dest);
}

void strcatTruncMaxLen(char* p_src, const char* p_cat, uint p_maxStrLen) {
    if (strlen(p_src) + strlen(p_cat) <= p_maxStrLen) {
        strcat(p_src, p_cat);
    }
    else {
        if ((strlen(p_src) >= p_maxStrLen) && (p_maxStrLen <= 3)) {
            for (uint8_t i = 0; i < p_maxStrLen; i++) {
                memcpy(p_src + i, ".", 1);
            }
            memcpy(p_src + p_maxStrLen, "\0", 1);
        }
        else if (strlen(p_src) + 3 >= p_maxStrLen) {
            memcpy(p_src + p_maxStrLen - 3, "...\0", 4);
        }
        else {
            memcpy(p_src + strlen(p_src), p_cat, p_maxStrLen - strlen(p_src));
            memcpy(p_src + p_maxStrLen - 3, "...\0", 4);
        }
    }
    trimNlCr(p_src);
}

/*==============================================================================================================================================*/
/* END strHelpers                                                                                                                               */
/*==============================================================================================================================================*/
