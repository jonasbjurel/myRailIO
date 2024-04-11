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
EXT_RAM_ATTR SemaphoreHandle_t logg::logDestinationLock = NULL;
EXT_RAM_ATTR SemaphoreHandle_t logg::logCustomLock = NULL;
EXT_RAM_ATTR logSeverity_t logg::logLevel = GJMRI_DEBUG_INFO;
EXT_RAM_ATTR job* logg::logJobHandle = NULL;
EXT_RAM_ATTR QList<rSyslog_t*>* logg::sysLogServers = NULL;
EXT_RAM_ATTR QList<customLogDesc_t*>* logg::customLogDescList = NULL;
EXT_RAM_ATTR QList<customLogDesc_t*>* logg::logLevelList[NO_OF_LOG_LEVELS] = { NULL };
EXT_RAM_ATTR char* logg::logMsgOutput;
EXT_RAM_ATTR timeval logg::tv;
EXT_RAM_ATTR timezone logg::tz;
EXT_RAM_ATTR char logg::timeStamp[35] = { '\0' };
EXT_RAM_ATTR char logg::microSecTimeStamp[10] = { '\0' };
EXT_RAM_ATTR tm* logg::tmTod;
EXT_RAM_ATTR const char* logg::fileBaseNameStr = NULL;
EXT_RAM_ATTR logJobDesc_t logg::logJobDesc[LOG_JOB_SLOTS + 1];
EXT_RAM_ATTR uint16_t logg::logJobDescIndex = 0;
EXT_RAM_ATTR bool logg::overload = false;
EXT_RAM_ATTR bool logg::overloadCeased = false;
EXT_RAM_ATTR uint32_t logg::missedLogs = 0;
EXT_RAM_ATTR uint32_t logg::totalMissedLogs = 0;
EXT_RAM_ATTR bool logg::consoleLog = true;



logg::logg(void){
    if (instances++ > 0) {
        panic("Can not create more than one instance of logg" CR);
        return;
    }
    logInstance = this;
    logLock = xSemaphoreCreateMutex();
    logDestinationLock = xSemaphoreCreateMutex();
    logCustomLock = xSemaphoreCreateMutex();
    tz.tz_minuteswest = 0;
    tz.tz_dsttime = 0;
    logLevel = GJMRI_DEBUG_INFO;
//  logJobHandle = new (heap_caps_malloc(sizeof(job), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) job(LOGJOBSLOTS, LOG_TASKNAME, LOG_STACKSIZE_1K * 1024, LOG_PRIO, LOG_WDT_TIMEOUT_MS);
    logJobHandle = new job(LOG_JOB_SLOTS, CPU_LOG_JOB_TASKNAME, CPU_LOG_JOB_STACKSIZE_1K * 1024, CPU_LOG_JOB_PRIO, false, LOG_JOB_WDT_TIMEOUT_MS);
    if (!logJobHandle){
        panic("Could not create the log job task" CR);
        return;
    }
    logJobHandle->setOverloadLevelCease(LOG_JOB_SLOTS / 4);
    logJobHandle->regOverloadCb(onOverload, this);
}

logg::~logg(void) {
    panic("Destructing logg object not supported" CR);
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

void logg::setConsoleLog(bool p_consoleLog) {
    consoleLog = p_consoleLog;
    return;
}

bool logg::getConsoleLog(void) {
    return consoleLog;
}

rc_t logg::addLogServer(const char* p_host, const char* p_server, uint16_t p_port) {
    xSemaphoreTake(logDestinationLock, portMAX_DELAY);
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
            xSemaphoreGive(logDestinationLock);
            return RC_ALREADYEXISTS_ERR;
        }
    }
    sysLogServers->push_back(new (heap_caps_malloc(sizeof(rSyslog_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)) rSyslog_t);
    strcpy(sysLogServers->back()->server, p_server);
    sysLogServers->back()->port = p_port;
    sysLogServers->back()->syslogDest = new SimpleSyslog(p_host, "genJMRIDecoder", p_server, p_port);
    if (!sysLogServers->back()->syslogDest) {
        LOG_ERROR_NOFMT("Failed to create a syslog destination" CR);
        sysLogServers->pop_back();
        xSemaphoreGive(logDestinationLock);
        return RC_OUT_OF_MEM_ERR;
    }
    else
        LOG_INFO("Syslog destination %s created" CR, sysLogServers->back()->server);
    xSemaphoreGive(logDestinationLock);
    return RC_OK;
}

rc_t logg::deleteLogServer(const char* p_server, uint16_t p_port) {
    if (!sysLogServers) {
        return RC_NOT_FOUND_ERR;
    }
    xSemaphoreTake(logDestinationLock, portMAX_DELAY);
    uint8_t sysLogServersItter = 0;
    while (sysLogServers && sysLogServers->size() && sysLogServersItter < sysLogServers->size()) {
        if ((!strcmp(sysLogServers->at(sysLogServersItter)->server, p_server) && (sysLogServers->at(sysLogServersItter)->port == p_port))) {
            delete sysLogServers->at(sysLogServersItter)->syslogDest;
            delete sysLogServers->at(sysLogServersItter);
            sysLogServers->clear(sysLogServersItter);
            if (!sysLogServers->size()) {
                delete sysLogServers;
                sysLogServers = NULL;
            }
            xSemaphoreGive(logDestinationLock);
            return RC_OK;
        }
        sysLogServersItter++;
    }
    xSemaphoreGive(logDestinationLock);
    return RC_NOT_FOUND_ERR;
}

void logg::deleteAllLogServers(void) {
    if (!sysLogServers)
        return;
    xSemaphoreTake(logDestinationLock, portMAX_DELAY);
    while (sysLogServers->size()) {
        delete sysLogServers->back()->syslogDest;
        delete sysLogServers->back();
        sysLogServers->pop_back();
    }
    delete sysLogServers;
    sysLogServers = NULL;
    xSemaphoreGive(logDestinationLock);
}

rc_t logg::getLogServer(uint8_t p_index, char* p_server, uint16_t* p_port) {
    if (!sysLogServers || !(p_index < sysLogServers->size())) {
        p_server = NULL;
        p_port = NULL;
        return RC_NOT_FOUND_ERR;
    }
    xSemaphoreTake(logDestinationLock, portMAX_DELAY);
    if (p_server != NULL)
        strcpy(p_server, sysLogServers->at(p_index)->server);
    if (p_port != NULL)
        *p_port = sysLogServers->at(p_index)->port;
    xSemaphoreGive(logDestinationLock);
    return RC_OK;
}

rc_t logg::addCustomLogItem(const char* p_file, const char* p_classNfunc, logSeverity_t p_logLevel) {
    if (!p_file || p_logLevel < 0 || p_logLevel >= NO_OF_LOG_LEVELS)
        return RC_PARAMETERVALUE_ERR;
    xSemaphoreTake(logCustomLock, portMAX_DELAY);
    if (!customLogDescList) {
        if(!(customLogDescList = new (heap_caps_malloc(sizeof(QList<customLogDesc_t*>), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) QList<customLogDesc_t*>)){
            panic("Could not allocate a custom log list");
            xSemaphoreGive(logCustomLock);
            return RC_OUT_OF_MEM_ERR;
        }
    }
    for (uint16_t customLogDescItter = 0; customLogDescItter < customLogDescList->size(); customLogDescItter++) {
        if (!strcmp(customLogDescList->at(customLogDescItter)->file, p_file)){
            if ((!customLogDescList->at(customLogDescItter)->classNfunc && !p_classNfunc) ||
                (customLogDescList->at(customLogDescItter)->classNfunc && p_classNfunc) ?
                !strcmp(customLogDescList->at(customLogDescItter)->classNfunc, p_classNfunc) :
                false) {
                LOG_WARN_NOFMT("Custom log level descriptor already exists" CR);
                xSemaphoreGive(logCustomLock);
                return RC_ALREADYEXISTS_ERR;
            }
        }
    }
    customLogDescList->push_back(new (heap_caps_malloc(sizeof(customLogDesc_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) customLogDesc_t);
    if (!customLogDescList->back()) {
        panic("Could not allocate a custom log descriptor");
        xSemaphoreGive(logCustomLock);
        return RC_OUT_OF_MEM_ERR;
    }
    if (!customLogDescList->back()) {
        customLogDescList->pop_back();
        LOG_ERROR_NOFMT("Could not allocate custom log level descriptor" CR);
        xSemaphoreGive(logCustomLock);
        return RC_OUT_OF_MEM_ERR;
    }
    customLogDescList->back()->file = p_file;
    customLogDescList->back()->classNfunc = p_classNfunc;
    customLogDescList->back()->customLogLevel = p_logLevel;
    if (!logLevelList[p_logLevel]) {
        logLevelList[p_logLevel] = new QList<customLogDesc_t*>; //SHOUL BE IN SPIRAM INSTEAD
        if (!logLevelList[p_logLevel]) {
            panic("Could not allocate a custom loglevel list");
            xSemaphoreGive(logCustomLock);
            return RC_OUT_OF_MEM_ERR;
        }
    }
    logLevelList[p_logLevel]->push_back(customLogDescList->back());
    xSemaphoreGive(logCustomLock);
    return RC_OK;
}

rc_t logg::getCustomLogItem(uint16_t p_index, const char** p_file, const char** p_classNfunc, logSeverity_t* p_logLevel) {
    xSemaphoreTake(logCustomLock, portMAX_DELAY);
    if (p_index >= customLogDescList->size()){
        LOG_INFO("A customer log item index: %i has been requested - but it does not exist..." CR, p_index);
        xSemaphoreGive(logCustomLock);
        return RC_NOT_FOUND_ERR;
    }
    *p_file = customLogDescList->at(p_index)->file;
    *p_classNfunc = customLogDescList->at(p_index)->classNfunc;
    *p_logLevel = customLogDescList->at(p_index)->customLogLevel;
    xSemaphoreGive(logCustomLock);
    return RC_OK;
}

uint16_t logg::getNoOffCustomLogItem(void) {
    xSemaphoreTake(logCustomLock, portMAX_DELAY);
    if (!customLogDescList) {
        xSemaphoreGive(logCustomLock);
        return 0;
    }
    else {
        xSemaphoreGive(logCustomLock);
        return customLogDescList->size();
    }
}

rc_t logg::deleteCustomLogItem(const char* p_file, const char* p_classNfunc) {
    xSemaphoreTake(logCustomLock, portMAX_DELAY);
    if (!customLogDescList) {
        LOG_WARN_NOFMT("Custom log level descriptor list does not exist - no custom loglevel entries exists" CR);
        xSemaphoreGive(logCustomLock);
        return RC_NOT_FOUND_ERR;
    }
    bool found = false;
    uint16_t customLogDescItter = 0;
    while(customLogDescList->size() && customLogDescItter < customLogDescList->size()){
        if (strcmp(customLogDescList->at(customLogDescItter)->file, p_file)) {
            customLogDescItter++;
            continue;
        }
        if (p_classNfunc) {
            if (!customLogDescList->at(customLogDescItter)->classNfunc || strcmp(customLogDescList->at(customLogDescItter)->classNfunc, p_classNfunc)) {
                customLogDescItter++;
                continue;
            }
        }
        uint16_t customLogLevelItter = 0;
        while (logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->size()){
            if (logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->at(customLogLevelItter) == customLogDescList->at(customLogDescItter)) {
                logLevelList[customLogDescList->at(customLogDescItter)->customLogLevel]->clear(customLogLevelItter);
                delete customLogDescList->at(customLogDescItter);
                customLogDescList->clear(customLogDescItter);
                customLogDescItter--;
                found = true;
                if (p_classNfunc) {
                    xSemaphoreGive(logCustomLock);
                    return RC_OK;
                }
                else
                    break;
            }
            customLogLevelItter++;
        }
        customLogDescItter++;
    }
    xSemaphoreGive(logCustomLock);
    if (found)
        return RC_OK;
    else
        return RC_NOT_FOUND_ERR;
}

void logg::deleteAllCustomLogItems(void) {
    xSemaphoreTake(logCustomLock, portMAX_DELAY);
    for (uint16_t customLogDesItter = 0; customLogDesItter < customLogDescList->size(); customLogDesItter++) {
        delete customLogDescList->at(customLogDesItter);
    }
    customLogDescList->clear();
    delete customLogDescList;
    customLogDescList = NULL;
    for (uint8_t logLevelItter = 0; logLevelItter < NO_OF_LOG_LEVELS; logLevelItter++) {
        if (!logLevelList[logLevelItter]){
            continue;
        }
        else{
            logLevelList[logLevelItter]->clear();
            delete logLevelList[logLevelItter];
            logLevelList[logLevelItter] = NULL;
        }
    }
    xSemaphoreGive(logCustomLock);
}

bool logg::shouldLog(logSeverity_t p_logLevel, const char* p_file, const char* p_classNfunc) {
    if (p_logLevel > GJMRI_DEBUG_VERBOSE || p_logLevel < GJMRI_DEBUG_SILENT)
        return false;
    if (p_logLevel <= logLevel)
        return true;
    if (xSemaphoreTake(logCustomLock, 0) == pdFALSE)
        return false;
    if (customLogDescList){
        for (uint8_t customLogLevelItter = logLevel + 1; customLogLevelItter <= NO_OF_LOG_LEVELS; customLogLevelItter++) {
            if (!logLevelList[customLogLevelItter])
                continue;
            for (uint16_t customLogLevelQueueItter = 0; customLogLevelQueueItter < logLevelList[customLogLevelItter]->size(); customLogLevelQueueItter++) {
                if (!strcmp(logLevelList[customLogLevelItter]->at(customLogLevelQueueItter)->file, fileBaseName(p_file))){
                    if (!logLevelList[customLogLevelItter]->at(customLogLevelQueueItter)->classNfunc) {
                        xSemaphoreGive(logCustomLock);
                        return true;
                    }
                    else if (!strcmp(logLevelList[customLogLevelItter]->at(customLogLevelQueueItter)->classNfunc, p_classNfunc)) {
                        xSemaphoreGive(logCustomLock);
                        return true;
                    }
                }
            }
        }
    }
    xSemaphoreGive(logCustomLock);
    return false;
}

uint8_t logg::mapSeverityToSyslog(logSeverity_t p_logLevel) {
    switch (p_logLevel) {
    case GJMRI_DEBUG_SILENT:
    case GJMRI_DEBUG_FATAL:
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

void logg::enqueueLog(logSeverity_t p_logLevel, const char* p_file, const char* p_classNFunction, size_t p_line, bool p_purge, bool p_format, const char* p_logMsgFmt, ...) {
    if (overload) {
        if(shouldLog(p_logLevel, p_file, p_classNFunction)){
            missedLogs++;
            totalMissedLogs++;
        }
        if (overloadCeased) {
            overloadCeased = false;
            overload = false;
            LOG_WARN("Logging overloaded, missed %u log-entries" CR, missedLogs + 1);
            missedLogs = 0;
            totalMissedLogs++;
        }
        return;
    }
    xSemaphoreTake(logLock, portMAX_DELAY);
    gettimeofday(&logJobDesc[logJobDescIndex].tv, &logJobDesc[logJobDescIndex].tz);
    if (!shouldLog(p_logLevel, p_file, p_classNFunction)) {
        xSemaphoreGive(logLock);
        return;
    }
    if (overload) {
        missedLogs++;
        xSemaphoreGive(logLock);
        return;
    }
    assert(!logJobDesc[logJobDescIndex].busy);
    logJobDesc[logJobDescIndex].logJobIndex = logJobDescIndex;
    logJobDesc[logJobDescIndex].logLevel = p_logLevel;
    logJobDesc[logJobDescIndex].filePath = p_file;
    logJobDesc[logJobDescIndex].classNFunction = p_classNFunction;
    logJobDesc[logJobDescIndex].line = p_line;
    if (p_format) {
        va_list args;
        va_start(args, p_logMsgFmt);
        if(vasprintf(&logJobDesc[logJobDescIndex].logMsgFmt, p_logMsgFmt, args) == -1) {
            Serial.printf("LOG ERROR: Failed to allocate log FMT buffer, skipping log entry" CR);
            xSemaphoreGive(logLock);
            return;
        }
        logJobDesc[logJobDescIndex].format = true;
        va_end(args);
    }
    else {
        logJobDesc[logJobDescIndex].logMsgFmt = (char*)p_logMsgFmt;
        logJobDesc[logJobDescIndex].format = false;
    }
    logJobDesc[logJobDescIndex].busy = true;
    logJobHandle->enqueue(dequeueLog, &logJobDesc[logJobDescIndex], p_purge);
    if (++logJobDescIndex >= LOG_JOB_SLOTS + 1)
        logJobDescIndex = 0;
    xSemaphoreGive(logLock);
}

void logg::dequeueLog(void* p_jobCbMetaData){
    xSemaphoreTake(logLock, portMAX_DELAY);
    assert(((logJobDesc_t*)p_jobCbMetaData)->busy);
    fileBaseNameStr = fileBaseName(((logJobDesc_t*)p_jobCbMetaData)->filePath);
    tmTod = gmtime(&((logJobDesc_t*)p_jobCbMetaData)->tv.tv_sec);
    strftime(timeStamp, 25, "UTC: %Y-%m-%d %H:%M:%S", tmTod);
    strcat(timeStamp, ":");
    strcat(timeStamp, itoa(((logJobDesc_t*)p_jobCbMetaData)->tv.tv_usec, microSecTimeStamp, 10));
    int buffIndex = 0;
    if (asprintf(&logMsgOutput, "%s: %s: %s: %s-%i: %s", timeStamp, LOG_LEVEL_STR[((logJobDesc_t*)p_jobCbMetaData)->logLevel], ((logJobDesc_t*)p_jobCbMetaData)->classNFunction, fileBaseName(((logJobDesc_t*)p_jobCbMetaData)->filePath), ((logJobDesc_t*)p_jobCbMetaData)->line, ((logJobDesc_t*)p_jobCbMetaData)->logMsgFmt) == -1) {
        Serial.printf("LOG ERROR: Failed to allocate log FMT buffer, skipping log entry" CR);
        if (((logJobDesc_t*)p_jobCbMetaData)->format)
            delete ((logJobDesc_t*)p_jobCbMetaData)->logMsgFmt;
        xSemaphoreGive(logLock);
        return;
    }
    if (((logJobDesc_t*)p_jobCbMetaData)->format)
        delete ((logJobDesc_t*)p_jobCbMetaData)->logMsgFmt;
    ((logJobDesc_t*)p_jobCbMetaData)->busy = false;
    xSemaphoreGive(logLock);
    if (consoleLog) {
        Serial.print(logMsgOutput);
    }
    printRSyslog(mapSeverityToSyslog(((logJobDesc_t*)p_jobCbMetaData)->logLevel), logMsgOutput);
    delete logMsgOutput;
}

void logg::printRSyslog(uint8_t p_rSysLogLevel, const char* p_logMsg) {
    if (xSemaphoreTake(logDestinationLock, 0) == pdFALSE)
        return;
    if (!sysLogServers) {
        xSemaphoreGive(logDestinationLock);
        return;
    }
    for (uint8_t sysLogServerItter = 0; sysLogServerItter < sysLogServers->size(); sysLogServerItter++) {
        if (sysLogServers->at(sysLogServerItter)){
            sysLogServers->at(sysLogServerItter)->syslogDest->printf(FAC_USER, p_rSysLogLevel, (char*)p_logMsg);
        }
    }
    xSemaphoreGive(logDestinationLock);
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
        return GJMRI_DEBUG_FATAL;
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
    else if (p_loglevelInt ==  GJMRI_DEBUG_FATAL)
        return "DEBUG-PANIC\0";
    else if (p_loglevelInt == GJMRI_DEBUG_SILENT)
        return "DEBUG-SILENT\0";
    else
        return NULL;
}

void logg::onOverload(void* p_metadata, bool p_overload) {
    if (p_overload) {
        overload = true;
    }
    else{ 
        overloadCeased = true;
    }
}
