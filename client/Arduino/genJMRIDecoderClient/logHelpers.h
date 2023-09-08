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
#pragma once
#ifndef LOGHELPERS_H
#define LOGHELPERS_H



/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
class logg;
#include <stdlib.h>
#include <cstddef>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <QList.h>
#include <SimpleSyslog.h>
#include "config.h"
#include "rc.h"
#include "strHelpers.h"
#include "job.h"
#include "panic.h"



/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/
extern logg Log;

//Debug level
typedef int8_t logSeverity_t;
#define NO_OF_LOG_LEVELS				7
#define GJMRI_DEBUG_SILENT				0
#define GJMRI_DEBUG_PANIC				1
#define	GJMRI_DEBUG_ERROR				2
#define GJMRI_DEBUG_WARN				3
#define GJMRI_DEBUG_INFO				4
#define GJMRI_DEBUG_TERSE				5
#define GJMRI_DEBUG_VERBOSE				6

extern const char* LOG_LEVEL_STR[NO_OF_LOG_LEVELS];

struct rSyslog_t {
	char server[100];
	uint16_t port;
	SimpleSyslog* syslogDest;
};

struct customLogDesc_t {
	char* classNfunc;
	logSeverity_t customLogLevel;
};

struct customLogLevelDescPoiner_t {
	char* classNfunc;
	logSeverity_t customLogLevel;
};

struct logJobDesc_t {
	uint16_t logJobIndex;
	logSeverity_t logLevel;
	const char* filePath;
	const char* classNFunction;
	size_t line;
	char formatedLogMsg[LOG_MSG_SIZE];
	const char* nonFormatedLogMsg;
	timeval tv;
	timezone tz;
};

#define CR							"\n"

/*==============================================================================================================================================*/
/* Functions: Log Helper functions                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define LOG_VERBOSE(LOG_MSG_FMT, ...)			Log.verbose(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_VERBOSE_NOFORMAT(LOG_MSG_FMT, ...)	Log.verbose(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_TERSE(LOG_MSG_FMT, ...)				Log.terse(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_TERSE_NOFORMAT(LOG_MSG_FMT, ...)	Log.terse(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_INFO(LOG_MSG_FMT, ...)				Log.info(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_INFO_NOFORMAT(LOG_MSG_FMT, ...)		Log.info(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_WARN(LOG_MSG_FMT, ...)				Log.warn(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_WARN_NOFORMAT(LOG_MSG_FMT, ...)		Log.warn(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_ERROR(LOG_MSG_FMT, ...)				Log.error(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_ERROR_NOFORMAT(LOG_MSG_FMT, ...)	Log.error(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_FATAL(LOG_MSG_FMT, ...)				Log.fatal(__FILE__, __FUNCTION__, __LINE__, true, LOG_MSG_FMT, ##__VA_ARGS__)
#define LOG_FATAL_NOFORMAT(LOG_MSG_FMT, ...)	Log.fatal(__FILE__, __FUNCTION__, __LINE__, false, LOG_MSG_FMT, ##__VA_ARGS__)



/* Methods                                                                                                                                      */
class logg {
public:
	//Public methods
	logg(void);
	~logg(void);
	static rc_t setLogLevel(logSeverity_t p_loglevel);
	static logSeverity_t getLogLevel(void);
	static rc_t addLogServer(const char* p_server, uint16_t p_port = 514);
	static rc_t deleteLogServer(const char* p_server, uint16_t p_port = 514);
	static void deleteAllLogServers(void);
	static rc_t getLogServer(uint8_t p_index, char* p_server, uint16_t* p_port);
	static rc_t addCustomLogItem(const char* p_classNfunc, logSeverity_t p_logLevel);
	static rc_t getCustomLogItem(uint16_t p_index, char* p_classNfunc, logSeverity_t* p_logLevel);
	static uint16_t getNoOffCustomLogItem(void);
	static rc_t deleteCustomLogItem(const char* p_classNfunc);
	static void deleteAllCustomLogItems(void);
	static void verbose(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static void terse(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static void info(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static void warn(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static void error(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static void fatal(const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, ...);
	static const char* getLogHistory(uint16_t p_logIndex);
	static uint32_t getMissedLogs(void);
	static void clearMissedLogs(void);
	static logSeverity_t transformLogLevelXmlStr2Int(const char* p_loglevelXmlTxt);
	static const char* transformLogLevelInt2XmlStr(logSeverity_t p_loglevelInt);

	//Public data structures
	//-

private:
	//Private methods
	static void enqueueLog(logSeverity_t p_logLevel, const char* p_file, const char* p_classNFunction, size_t p_line, bool p_format, const char* p_logMsgFmt, va_list p_args, bool p_purge = false);
	static void onPrintLog(void* p_jobCbMetaData);
	static void printRSyslog(uint8_t p_rSysLogLevel, const char* p_logMsg);
	static bool shouldLog(logSeverity_t p_severity, const char* p_classNFunction);
	static uint8_t mapSeverityToSyslog(logSeverity_t p_logLevel);
	static void onOverload(bool p_overload);


	//Private data structures
	static logg* logInstance;
	static uint8_t instances;
	static SemaphoreHandle_t logLock;
	static logSeverity_t logLevel;
	static job* logJobHandle;
	static QList<rSyslog_t*>* sysLogServers;
	static QList<customLogDesc_t*>* customLogDescList;
	static QList<customLogDesc_t*>* logLevelList[NO_OF_LOG_LEVELS];
	static char logMsgHistory[LOG_MSG_HISTORY_SIZE][LOG_MSG_SIZE];
	static char nonformatedMsg[4096];
	static uint16_t logHistoryIndex;
	static timeval tv;
	static timezone tz;
	static char timeStamp[35];
	static char microSecTimeStamp[10];
	static const char* fileBaseNameStr;
	static logJobDesc_t logJobDesc[LOGJOBSLOTS];
	static uint16_t logJobDescIndex;
	static bool overload;
	static uint32_t missedLogs;
	static uint32_t totalMissedLogs;

};
#endif //LOGHELPERS_H
