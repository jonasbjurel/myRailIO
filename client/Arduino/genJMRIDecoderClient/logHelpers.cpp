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
#include "logHelpers.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Functions: Log Helper functions                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
EXT_RAM_ATTR logg Log = logg();
EXT_RAM_ATTR const char* LOG_LEVEL_STR[NO_OF_LOG_LEVELS] = { "SILENT", "FATAL", "ERROR", "WARNING", "INFO", "TERSE", "VERBOSE" };
EXT_RAM_ATTR uint8_t logg::instances = 0;
EXT_RAM_ATTR logg* logg::logInstance;
EXT_RAM_ATTR SemaphoreHandle_t logg::logLock = NULL;
EXT_RAM_ATTR logSeverity_t logg::logLevel = GJMRI_DEBUG_INFO;
EXT_RAM_ATTR job* logg::logJobHandle = NULL;
EXT_RAM_ATTR QList<rSyslog_t*>* logg::sysLogServers = NULL;
EXT_RAM_ATTR QList<customLogDesc_t*>* logg::customLogDescList = NULL;
EXT_RAM_ATTR QList<customLogDesc_t*>* logg::logLevelList[NO_OF_LOG_LEVELS] = { NULL };
EXT_RAM_ATTR char logg::logMsgHistory[LOG_MSG_HISTORY_SIZE][LOG_MSG_SIZE] = { '\0' };
EXT_RAM_ATTR char logg::nonformatedMsg[4096];
EXT_RAM_ATTR uint16_t logg::logHistoryIndex = 0;
EXT_RAM_ATTR timeval logg::tv;
EXT_RAM_ATTR timezone logg::tz;
EXT_RAM_ATTR char logg::timeStamp[35] = { '\0' };
EXT_RAM_ATTR char logg::microSecTimeStamp[10] = { '\0' };
EXT_RAM_ATTR const char* logg::fileBaseNameStr = NULL;
EXT_RAM_ATTR logJobDesc_t logg::logJobDesc[LOGJOBSLOTS];
EXT_RAM_ATTR uint16_t logg::logJobDescIndex = 0;
EXT_RAM_ATTR bool logg::overload = false;
EXT_RAM_ATTR uint32_t logg::missedLogs = 0;
EXT_RAM_ATTR uint32_t logg::totalMissedLogs = 0;


logg::logg(void){
    if (instances++ > 0) {
        panic("Can not create more than one instance of logg" CR);
        return;
    }
    logInstance = this;
    logLock = xSemaphoreCreateMutex();
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    logLevel = GJMRI_DEBUG_INFO;
    logJobHandle = new job(LOGJOBSLOTS, LOG_TASKNAME, LOG_STACKSIZE_1K * 1024, LOG_PRIO);
//    logJobHandle = new (heap_caps_malloc(sizeof(job), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) job(LOGJOBSLOTS, LOG_TASKNAME, LOG_STACKSIZE_1K * 1024, LOG_PRIO);
    logJobHandle->setjobQueueUnsetOverloadLevel(LOGJOBSLOTS / 4);
    logJobHandle->regOverloadCb(onOverload, this);
    return;
}

logg::~logg(void) {
    panic("Destructing logg object" CR);
    deleteAllLogServers();
    if (sysLogServers)
        delete sysLogServers;
    deleteAllCustomLogItems();
    if (customLogDescList)
        delete customLogDescList;
    delete logJobHandle;
}

rc_t logg::setLogLevel(logSeverity_t p_loglevel) {
    if (p_loglevel < GJMRI_DEBUG_SILENT || p_loglevel > GJMRI_DEBUG_VERBOSE)
        return RC_PARAMETERVALUE_ERR;
    else {
        logLevel = p_loglevel;
        return RC_OK;
    }
}

logSeverity_t logg::getLogLevel(void) {
        return logLevel;
}

rc_t logg::addLogServer(const char* p_server, uint16_t p_port) {
    if (!isUri(p_server) || p_port < 0 || p_port > 65535) {
        LOG_ERROR("Provided Rsyslog server URI:%s, Port: %i is not valid, will not start rSyslog" CR, p_server, RSYSLOG_DEFAULT_PORT);
            return RC_PARAMETERVALUE_ERR;
    }
    if (p_port != RSYSLOG_DEFAULT_PORT)
        LOG_ERROR("Support for other RSyslog ports than default: %i is not implemented, using default port" CR, RSYSLOG_DEFAULT_PORT);
    if (!sysLogServers)
        sysLogServers = new QList<rSyslog_t*>;
    for (uint8_t sysLogServersItter = 0; sysLogServersItter < sysLogServers->size(); sysLogServersItter++) {
        if ((!strcmp(sysLogServers->at(sysLogServersItter)->server, p_server) && (sysLogServers->at(sysLogServersItter)->port == p_port))) {
            LOG_ERROR("RSyslog server %s already exists" CR, p_server);
            break;
        }
    }
    sysLogServers->push_back(new (heap_caps_malloc(sizeof(rSyslog_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) rSyslog_t);
    strcpy(sysLogServers->back()->server, p_server);
    sysLogServers->back()->port = p_port;
    sysLogServers->back()->syslogDest = new SimpleSyslog("MyHost", "MyApp", p_server, p_port); //FIX HOSTNAME AND APP, AND MAKE SURE IT GETS UPDATED
    if (!sysLogServers->back()->syslogDest) {
        LOG_ERROR("Failed to create a syslog destination" CR);
        delete sysLogServers->back();
        sysLogServers->pop_back();
    }
    else
        LOG_INFO("Syslog destination %s created" CR, sysLogServers->back()->server);
    return RC_OK;
}

rc_t logg::deleteLogServer(const char* p_server, uint16_t p_port) {
    if (!sysLogServers) {
        return RC_NOT_FOUND_ERR;
    }
    for (uint8_t sysLogServersItter = 0; sysLogServersItter < sysLogServers->size(); sysLogServersItter++) {
        if ((!strcmp(sysLogServers->at(sysLogServersItter)->server, p_server) && (sysLogServers->at(sysLogServersItter)->port == p_port))) {
            delete sysLogServers->at(sysLogServersItter)->syslogDest;
            delete sysLogServers->at(sysLogServersItter);
            sysLogServers->clear(sysLogServersItter);
        }
    }
    for (uint8_t sysLogServersItter = 0; sysLogServersItter < sysLogServers->size(); sysLogServersItter++) {
        if (!sysLogServers->back()) {
            sysLogServers->pop_back();
        }
        else {
            sysLogServers->push_front(sysLogServers->back());
            sysLogServers->pop_back();
        }
    }
    return RC_OK;
}

void logg::deleteAllLogServers(void) {
    if (!sysLogServers)
        return;
    for (uint8_t sysLogServersItter = 0; sysLogServersItter < sysLogServers->size(); sysLogServersItter++) {
        delete sysLogServers->back()->syslogDest;
        delete sysLogServers->back();
        sysLogServers->pop_back();
    } 
}

rc_t logg::getLogServer(uint8_t p_index, char* p_server, uint16_t* p_port) {
    if (!sysLogServers || !(p_index < sysLogServers->size())) {
        p_server = NULL;
        p_port = NULL;
        return RC_NOT_FOUND_ERR;
    }
    if (p_server != NULL)
        strcpy(p_server, sysLogServers->at(p_index)->server);
    if (p_port != NULL)
        *p_port = sysLogServers->at(p_index)->port;
    return RC_OK;
}

rc_t logg::addCustomLogItem(const char* p_classNfunc, logSeverity_t p_logLevel) {
    if (p_logLevel < 0 || p_logLevel >= NO_OF_LOG_LEVELS)
        return RC_PARAMETERVALUE_ERR;
    if (customLogDescList){
        for (uint16_t customLogDesItter = 0; customLogDesItter < customLogDescList->size(); customLogDesItter++) {
            if (customLogDescList->at(customLogDesItter)->classNfunc == p_classNfunc) {
                LOG_WARN("Custom log level descriptor already exists" CR);
                return RC_ALREADYEXISTS_ERR;
            }
        }
    }
    else {
        customLogDescList = new (heap_caps_malloc(sizeof(QList<customLogDesc_t*>), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) QList<customLogDesc_t*>;
        if (!customLogDescList) {
            LOG_ERROR("Could not allocate custom log level descriptor List" CR);
            return RC_OUT_OF_MEM_ERR;
        }
    }
    customLogDescList->push_back(new (heap_caps_malloc(sizeof(customLogDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) customLogDesc_t);
    if (!customLogDescList->back()) {
        delete customLogDescList;
        customLogDescList = NULL;
        LOG_ERROR("Could not allocate custom log level descriptor" CR);
        return RC_OUT_OF_MEM_ERR;
    }
    customLogDescList->back()->classNfunc = createNcpystr(p_classNfunc);
    customLogDescList->back()->customLogLevel = p_logLevel;
    logLevelList[p_logLevel]->push_back(customLogDescList->back());
    return RC_OK;
}

rc_t logg::getCustomLogItem(uint16_t p_index, char* p_classNfunc, logSeverity_t* p_logLevel) {
    if (p_index < customLogDescList->size()){
        LOG_INFO("A customer log item index: %i has been requested - but it does not exist..." CR, p_index);
        return RC_NOT_FOUND_ERR;
    }
    strcpy(p_classNfunc, customLogDescList->at(p_index)->classNfunc);
    *p_logLevel = customLogDescList->at(p_index)->customLogLevel;
    return RC_OK;
}

uint16_t logg::getNoOffCustomLogItem(void) {
    if (!customLogDescList)
        return 0;
    else
        return customLogDescList->size();
}

rc_t logg::deleteCustomLogItem(const char* p_classNfunc) {
    if (!customLogDescList) {
        LOG_WARN("Custom log level descriptor list does not exist - no custom loglevel entries exists" CR);
        return RC_NOT_FOUND_ERR;
    }
    bool found = false;
    for (uint16_t customLogDescItter = 0; customLogDescItter < customLogDescList->size(); customLogDescItter++) {
        if (!strcmp(customLogDescList->at(customLogDescItter)->classNfunc, p_classNfunc)) {
            for (uint16_t customLogLevelItter = 0; customLogLevelItter < logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->size(); customLogLevelItter++) {
                if (logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->at(customLogLevelItter) == customLogDescList->at(customLogDescItter)) {
                    logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->clear(customLogLevelItter);
                    delete customLogDescList->at(customLogDescItter)->classNfunc;
                    delete customLogDescList->at(customLogDescItter);
                    customLogDescList->clear(customLogDescItter);
                    return RC_OK;
                }
            }
        }
    }
    return RC_NOT_FOUND_ERR;
}

void logg::deleteAllCustomLogItems(void) {
    for (uint16_t customLogDesItter = 0; customLogDesItter < customLogDescList->size(); customLogDesItter++) {
        delete customLogDescList->at(customLogDesItter)->classNfunc;
        delete customLogDescList->at(customLogDesItter);
    }
    customLogDescList->clear();
    delete customLogDescList;
    customLogDescList = NULL;
    for (uint8_t logLevelItter = 0; logLevelItter < NO_OF_LOG_LEVELS; logLevelItter++) {
        logLevelList[logLevelItter]->clear();
    }
}

bool logg::shouldLog(logSeverity_t p_logLevel, const char* p_classNFunction) {
    if (p_logLevel > GJMRI_DEBUG_VERBOSE || p_logLevel < GJMRI_DEBUG_SILENT)
        return false;
    if (p_logLevel <= logLevel)
        return true;
    if (customLogDescList)
        for (uint8_t customLogLevelItter = logLevel + 1; customLogLevelItter <= 7; customLogLevelItter++) {
            for (uint16_t customLogLevelQueueItter = 0; customLogLevelQueueItter < logLevelList[customLogLevelItter]->size(); customLogLevelQueueItter++) {
                if (!strcmp(logLevelList[customLogLevelItter]->at(customLogLevelQueueItter)->classNfunc, p_classNFunction)){
                    return true;
                    break;
                }
            }
        }
    return false;
}

uint8_t logg::mapSeverityToSyslog(logSeverity_t p_logLevel) {
    switch (p_logLevel) {
    case GJMRI_DEBUG_SILENT:
    case GJMRI_DEBUG_PANIC:
        return PRI_EMERGENCY;
        break;
    case GJMRI_DEBUG_ERROR:
        return PRI_ERROR;
        break;
    case GJMRI_DEBUG_WARN:
        return PRI_WARNING;
        break;
    case GJMRI_DEBUG_INFO:
        return PRI_NOTICE;
        break;
    case GJMRI_DEBUG_TERSE:
        return PRI_INFO;
    case GJMRI_DEBUG_VERBOSE:
        return PRI_DEBUG;
        break;
    default:
        return PRI_NOTICE;
        break;
    }
}

void logg::verbose(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_VERBOSE, p_classNFunction)){
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_VERBOSE, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::terse(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_TERSE, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_TERSE, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::info(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_INFO, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_INFO, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::warn(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_WARN, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_WARN, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::error(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_ERROR, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_ERROR, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::fatal(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...) {
    xSemaphoreTake(logLock, portMAX_DELAY);
    if (!shouldLog(GJMRI_DEBUG_PANIC, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    va_list args;
    va_start(args, p_logMsgFmt);
    enqueueLog(GJMRI_DEBUG_PANIC, p_file, p_classNFunction, p_line, p_format, p_logMsgFmt, args, true);
    va_end(args);
    xSemaphoreGive(logLock);
}

void logg::enqueueLog(logSeverity_t p_logLevel, const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, va_list p_args, bool p_purge) {
    //Serial.printf(">>>>>>>>>>>>>>> Start Enqueue, pending Jobs: %i" CR, logJobHandle->getPendingJobSlots());
    if (overload){
        missedLogs++;
        totalMissedLogs++;
    }
    logJobDesc[logJobDescIndex].logJobIndex = logJobDescIndex;
    logJobDesc[logJobDescIndex].logLevel = p_logLevel;
    logJobDesc[logJobDescIndex].filePath = p_file;
    logJobDesc[logJobDescIndex].classNFunction = p_classNFunction;
    logJobDesc[logJobDescIndex].line = p_line;
    int msgSize;
    if (p_format){
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Enqueueing formated message %s: to jobIndex %i" CR, logJobDesc[logJobDescIndex].formatedLogMsg, logJobDescIndex);
        msgSize = vsnprintf(NULL, 0, p_logMsgFmt, p_args);
        //Serial.printf("Enqueue setting formatted Log msg" CR);
        vsnprintf(logJobDesc[logJobDescIndex].formatedLogMsg, msgSize + 1, p_logMsgFmt, p_args);
        logJobDesc[logJobDescIndex].nonFormatedLogMsg = NULL;
    }
    else{
        //Serial.printf("Enqueue setting non-formatted Log msg" CR);
        logJobDesc[logJobDescIndex].nonFormatedLogMsg = p_logMsgFmt;
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Enqueueing non-formated message %s: to jobIndex %i" CR, logJobDesc[logJobDescIndex].nonFormatedLogMsg, logJobDescIndex);

    }
    gettimeofday(&logJobDesc[logJobDescIndex].tv, &logJobDesc[logJobDescIndex].tz);
    logJobHandle->enqueue(onPrintLog, &logJobDesc[logJobDescIndex], 0, p_purge);
    //if (!logJobDesc[logJobDescIndex].nonFormatedLogMsg)
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Enqueued formatted message %s: to jobIndex %i (%i), jobDescriptor %i" CR, logJobDesc[logJobDescIndex].formatedLogMsg, logJobDescIndex, logJobDesc[logJobDescIndex].logJobIndex, &logJobDesc[logJobDescIndex]);
    //else
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Enqueued non-formatted message %s: to jobIndex %i (%i), jobDescriptor %i" CR, logJobDesc[logJobDescIndex].nonFormatedLogMsg, logJobDescIndex, logJobDesc[logJobDescIndex].logJobIndex, &logJobDesc[logJobDescIndex]);
    if (++logJobDescIndex >= LOGJOBSLOTS)
        logJobDescIndex = 0;
    //Serial.printf("<<<<<<<<<<<<<<<<< End Enqueue" CR);
}

void logg::onPrintLog(void* p_jobCbMetaData){
    //Serial.printf(">>>>>>>>>>>>>>>>> Start PrintLog LoghistoryIndex: %i" CR, logHistoryIndex);

    fileBaseNameStr = fileBaseName(((logJobDesc_t*)p_jobCbMetaData)->filePath);
    tm* tmTod = gmtime(&((logJobDesc_t*)p_jobCbMetaData)->tv.tv_sec);
    strftime(timeStamp, 25, "UTC:%H:%M:%S", tmTod);
    strcat(timeStamp, ":"); 
    strcat(timeStamp, itoa(((logJobDesc_t*)p_jobCbMetaData)->tv.tv_usec, microSecTimeStamp, 10));
    if (!((logJobDesc_t*)p_jobCbMetaData)->nonFormatedLogMsg) {
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Print formatted message %s: from jobIndex %i, jobDescriptor %i" CR, ((logJobDesc_t*)p_jobCbMetaData)->formatedLogMsg, ((logJobDesc_t*)p_jobCbMetaData)->logJobIndex, p_jobCbMetaData);
        sprintf(logMsgHistory[logHistoryIndex], "%s: %s: %s: %s-%i: %s", timeStamp, LOG_LEVEL_STR[((logJobDesc_t*)p_jobCbMetaData)->logLevel], ((logJobDesc_t*)p_jobCbMetaData)->classNFunction, fileBaseName(((logJobDesc_t*)p_jobCbMetaData)->filePath), ((logJobDesc_t*)p_jobCbMetaData)->line, ((logJobDesc_t*)p_jobCbMetaData)->formatedLogMsg);
        Serial.print(logMsgHistory[logHistoryIndex]);
        printRSyslog(mapSeverityToSyslog(((logJobDesc_t*)p_jobCbMetaData)->logLevel), logMsgHistory[logHistoryIndex]);
    }
    else {
        //Serial.printf("&&&&&&&&&&&&&&&&&&&Print non-formatted message %s: from jobIndex %i, jobDescriptor %i" CR, ((logJobDesc_t*)p_jobCbMetaData)->nonFormatedLogMsg, ((logJobDesc_t*)p_jobCbMetaData)->logJobIndex, p_jobCbMetaData);
        sprintf(nonformatedMsg, "%s: %s: %s: %s-%i: %s", timeStamp, LOG_LEVEL_STR[((logJobDesc_t*)p_jobCbMetaData)->logLevel], ((logJobDesc_t*)p_jobCbMetaData)->classNFunction, fileBaseName(((logJobDesc_t*)p_jobCbMetaData)->filePath), ((logJobDesc_t*)p_jobCbMetaData)->line, ((logJobDesc_t*)p_jobCbMetaData)->nonFormatedLogMsg);
        Serial.print(nonformatedMsg);
        printRSyslog(mapSeverityToSyslog(((logJobDesc_t*)p_jobCbMetaData)->logLevel), nonformatedMsg );
        strcpyTruncMaxLen(logMsgHistory[logHistoryIndex], nonformatedMsg, LOG_MSG_SIZE);
    }
    if (++logHistoryIndex == LOG_MSG_HISTORY_SIZE)
        logHistoryIndex = 0;
    //Serial.printf("<<<<<<<<<<<<<<<<<< End PrintLog LoghistoryIndex: %i" CR, logHistoryIndex);

}

void logg::printRSyslog(uint8_t p_rSysLogLevel, const char* p_logMsg) {
    if (!sysLogServers)
        return;
    for (uint8_t sysLogServerItter = 0; sysLogServerItter < sysLogServers->size(); sysLogServerItter++) {
        if (sysLogServers->at(sysLogServerItter)){
            Serial.printf("Printing to Syslog server %s\n", sysLogServers->at(sysLogServerItter)->server);
            sysLogServers->at(sysLogServerItter)->syslogDest->printf(FAC_USER, p_rSysLogLevel, (char*)p_logMsg);
        }
    }
}

const char* logg::getLogHistory(uint16_t p_logIndex){
    if (logHistoryIndex >= p_logIndex)
        return logMsgHistory[logHistoryIndex];
    else
        return logMsgHistory[LOG_MSG_HISTORY_SIZE - (p_logIndex - logHistoryIndex)];
}

uint32_t logg::getMissedLogs(void) {
    return totalMissedLogs;
}

void logg::clearMissedLogs(void) {
    totalMissedLogs = 0;
}

logSeverity_t logg::transformLogLevelXmlStr2Int(const char* p_loglevelXmlTxt) {
    if (!strcmp(p_loglevelXmlTxt, "DEBUG-VERBOSE"))
        return GJMRI_DEBUG_VERBOSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-TERSE"))
        return GJMRI_DEBUG_TERSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-INFO"))
        return GJMRI_DEBUG_INFO;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-WARN"))
        return GJMRI_DEBUG_WARN;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-ERROR"))
        return GJMRI_DEBUG_ERROR;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-PANIC"))
        return GJMRI_DEBUG_PANIC;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-SILENT"))
        return GJMRI_DEBUG_SILENT;
    else
        return RC_GEN_ERR;
}

const char* logg::transformLogLevelInt2XmlStr(logSeverity_t p_loglevelInt) {
    if (p_loglevelInt == GJMRI_DEBUG_VERBOSE)
        return "DEBUG-VERBOSE\0";
    else if (p_loglevelInt == GJMRI_DEBUG_TERSE)
        return "DEBUG-TERSE\0";
    else if (p_loglevelInt == GJMRI_DEBUG_INFO)
        return "DEBUG-INFO\0";
    else if (p_loglevelInt == GJMRI_DEBUG_WARN)
        return "DEBUG-WARN\0";
    else if (p_loglevelInt == GJMRI_DEBUG_ERROR)
        return "DEBUG-ERROR\0";
    else if (p_loglevelInt ==  GJMRI_DEBUG_PANIC)
        return "DEBUG-PANIC\0";
    else if (p_loglevelInt == GJMRI_DEBUG_SILENT)
        return "DEBUG-SILENT\0";
    else
        return NULL;
}

void logg::onOverload(bool p_overload) {
    if (p_overload)
        overload = true;
    else{
        LOG_WARN("Log system overloaded - missed %i logs" CR, missedLogs);
        missedLogs = 0;
        overload = false;
    }
}
