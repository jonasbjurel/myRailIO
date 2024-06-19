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
#include "ntpTime.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: ntpTime																																*/
/* Purpose: see time.h																															*/
/* Description see time.h																														*/
/* Methods: see time.h																															*/
/* Data structures: see time.h																													*/
/*==============================================================================================================================================*/
EXT_RAM_ATTR ntpDescriptor_t ntpTime::ntpDescriptor;

void ntpTime::init(void) {
	LOG_TERSE_NOFMT("Initializing ntptime" CR);
	ntpDescriptor.ntpServerIndexMap = 0;
	ntpDescriptor.opState = NTP_CLIENT_DISABLED | NTP_CLIENT_NOT_SYNCHRONIZED;
	ntpDescriptor.dhcp = false;
	setTz(NTP_DEFAULT_TZ_AREA_ENCODED_TEXT);
	ntpDescriptor.syncMode = NTP_DEFAULT_SYNCMODE;
	ntpDescriptor.syncStatus = SNTP_SYNC_STATUS_RESET;
	ntpDescriptor.noOfServers = 0;
	ntpDescriptor.pollPeriodMs = NTP_POLL_PERIOD_S;
	ntpDescriptor.ntpServerHosts = new QList<ntpServerHost_t*>;
	updateNtpDescriptorStr(&ntpDescriptor);
}

rc_t ntpTime::start(bool p_dhcp) {
	LOG_TERSE_NOFMT("Starting ntptime" CR);
	sntp_setoperatingmode(SNTP_OPMODE_POLL);
	sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
	sntp_set_sync_interval(ntpDescriptor.pollPeriodMs * 1000);
	sntp_set_time_sync_notification_cb(sntpCb);
	if (p_dhcp)
		sntp_servermode_dhcp(true);
	else
		sntp_servermode_dhcp(false);
	sntp_init();
	unSetNtpOpState(&ntpDescriptor, NTP_CLIENT_DISABLED);
	if (p_dhcp)
		ntpDescriptor.dhcp = true;
	else
		ntpDescriptor.dhcp = false;
	sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::stop(void) {
	LOG_TERSE_NOFMT("Stopping ntptime" CR);
	sntp_stop();
	setNtpOpState(&ntpDescriptor, NTP_CLIENT_DISABLED);
	sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::restart(void) {
	LOG_TERSE_NOFMT("Restarting ntptime" CR);
	ntpOpState_t opState;
	getNtpOpState(&opState);
	if (opState & NTP_CLIENT_DISABLED) {
		LOG_INFO_NOFMT("Cannot restart NTP-client, as it is not running" CR);
		return RC_NOTRUNNING_ERR;
	}
	sntp_restart();
	return RC_OK;
}

bool ntpTime::getNtpDhcp(void) {
	return ntpDescriptor.dhcp;
}

rc_t ntpTime::addNtpServer(IPAddress p_ntpServerAddr, uint16_t p_port) {
	ip_addr_t ntpHostAddr;
	LOG_TERSE("Adding NTP server: %s:%i" CR, p_ntpServerAddr.toString().c_str(), p_port);
	if (!ipaddr_aton(p_ntpServerAddr.toString().c_str(), &ntpHostAddr)) {
		LOG_WARN("%s is not a valid IP Address" CR, p_ntpServerAddr.toString().c_str());
		return RC_PARAMETERVALUE_ERR;
	}
	uint8_t i;
	for (i = 0; i < NTP_MAX_NTPSERVERS; i++) {
		if (!(ntpDescriptor.ntpServerIndexMap & (0b00000001 << i))){
			ntpDescriptor.ntpServerIndexMap = ntpDescriptor.ntpServerIndexMap | (0b00000001 << i);
			break;
		}
	}
	if (i >= NTP_MAX_NTPSERVERS) {
		LOG_WARN("Cannot add another NTP server, maximum number (%i) of servers already configured" CR, NTP_MAX_NTPSERVERS);
		return RC_MAX_REACHED_ERR;
	}
	sntp_setserver(i, &ntpHostAddr);
	restart();
	ntpDescriptor.ntpServerHosts->push_back(new (heap_caps_malloc(sizeof(ntpServerHost_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) ntpServerHost_t);
	ntpDescriptor.ntpServerHosts->back()->index = i;
	ntpDescriptor.ntpServerHosts->back()->ntpHostAddress = p_ntpServerAddr;
	ntpDescriptor.ntpServerHosts->back()->ntpPort = p_port;
	strcpy(ntpDescriptor.ntpServerHosts->back()->ntpHostName, "");
	ntpDescriptor.ntpServerHosts->back()->stratum = 0;
	ntpDescriptor.noOfServers++;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::addNtpServer(const char* p_ntpServerName, uint16_t p_port) {
	LOG_TERSE("Adding NTP server: %s:%i" CR, p_ntpServerName, p_port);
	if (!isUri(p_ntpServerName)){
		LOG_WARN("%s is not a valid URI" CR, p_ntpServerName);
		return RC_PARAMETERVALUE_ERR;
	}
	uint8_t i;
	for (i = 0; i < NTP_MAX_NTPSERVERS; i++) {
		if (!(ntpDescriptor.ntpServerIndexMap & (0b00000001 << i))) {
			ntpDescriptor.ntpServerIndexMap = ntpDescriptor.ntpServerIndexMap | (0b00000001 << i);
			break;
		}
	}
	if (i >= NTP_MAX_NTPSERVERS) {
		LOG_WARN("Cannot add another NTP server, maximum number (%i) of servers already configured" CR, NTP_MAX_NTPSERVERS);
		return RC_MAX_REACHED_ERR;
	}
	sntp_setservername(i, p_ntpServerName);
	restart();
	ntpDescriptor.ntpServerHosts->push_back(new (heap_caps_malloc(sizeof(ntpServerHost_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) ntpServerHost_t);
	ntpDescriptor.ntpServerHosts->back()->index = i;
	strcpy(ntpDescriptor.ntpServerHosts->back()->ntpHostName, p_ntpServerName);
	ntpDescriptor.ntpServerHosts->back()->ntpPort = p_port;
	ntpDescriptor.ntpServerHosts->back()->ntpHostAddress.fromString("0.0.0.0");
	ntpDescriptor.ntpServerHosts->back()->stratum = 0;
	ntpDescriptor.noOfServers++;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::deleteNtpServer(IPAddress p_ntpServerAddr) {
	uint8_t i;
	for (i = 0; i < ntpDescriptor.ntpServerHosts->size(); i++) {
		if (ntpDescriptor.ntpServerHosts->at(i)->ntpHostAddress == p_ntpServerAddr) {
			ntpDescriptor.ntpServerIndexMap = ntpDescriptor.ntpServerIndexMap & ~(0b00000001 << i);
			break;
		}
	}
	if (i >= ntpDescriptor.ntpServerHosts->size()) {
		LOG_WARN("%s not found" CR, p_ntpServerAddr.toString().c_str());
		return RC_NOT_FOUND_ERR;
	}
	sntp_setservername(i, NULL);
	restart();
	delete ntpDescriptor.ntpServerHosts->at(i);
	ntpDescriptor.ntpServerHosts->clear(i);
	ntpDescriptor.noOfServers--;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::deleteNtpServer(const char* p_ntpServerName) {
	if (!isUri(p_ntpServerName)){
		LOG_WARN("%s is not a valid URI" CR, p_ntpServerName);
		return RC_PARAMETERVALUE_ERR;
	}
	uint8_t i;
	for (i = 0; i < ntpDescriptor.ntpServerHosts->size(); i++) {
		if (!strcmp(ntpDescriptor.ntpServerHosts->at(i)->ntpHostName, p_ntpServerName)) {
			ntpDescriptor.ntpServerIndexMap = ntpDescriptor.ntpServerIndexMap & ~(0b00000001 << i);
			break;
		}
	}
	if (i >= ntpDescriptor.ntpServerHosts->size()) {
		LOG_WARN("%s not found" CR, p_ntpServerName);
		return RC_NOT_FOUND_ERR;
	}
	sntp_setservername(i, NULL);
	restart();
	delete ntpDescriptor.ntpServerHosts->at(i);
	ntpDescriptor.ntpServerHosts->clear(i);
	ntpDescriptor.noOfServers--;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::deleteNtpServer(void) {
	while (ntpDescriptor.ntpServerHosts->size()) {
		sntp_setservername(ntpDescriptor.ntpServerHosts->back()->index, NULL);
		delete ntpDescriptor.ntpServerHosts->back();
		ntpDescriptor.ntpServerHosts->pop_back();
	}
	ntpDescriptor.ntpServerIndexMap = 0b00000000;
	ntpDescriptor.noOfServers = 0;
	return RC_OK;
}

rc_t ntpTime::getNtpServer(const IPAddress* p_ntpServerAddr, ntpServerHost_t* p_serverStatus) {
	ip_addr_t ntpHostAddr;
	if (!ipaddr_aton(p_ntpServerAddr->toString().c_str(), &ntpHostAddr)) {
		LOG_WARN("%s is not a valid IP Address" CR, p_ntpServerAddr->toString().c_str());
		return RC_PARAMETERVALUE_ERR;
	}
	uint8_t i;
	for (i = 0; i < ntpDescriptor.ntpServerHosts->size(); i++) {
		if (ntpDescriptor.ntpServerHosts->at(i)->ntpHostAddress = *p_ntpServerAddr)
			break;
	}
	if (i >= ntpDescriptor.ntpServerHosts->size()) {
		LOG_WARN("%s not found" CR, p_ntpServerAddr->toString().c_str());
		return RC_NOT_FOUND_ERR;
	}
	p_serverStatus = ntpDescriptor.ntpServerHosts->at(i);
	return RC_OK;
}

rc_t ntpTime::getNtpServer(const char* p_ntpServerName, ntpServerHost_t* p_serverStatus) {
	if (!isUri(p_ntpServerName)) {
		LOG_WARN("%s is not a valid URI" CR, p_ntpServerName);
		return RC_PARAMETERVALUE_ERR;
	}
	uint8_t i;
	for (i = 0; i < ntpDescriptor.ntpServerHosts->size(); i++) {
		if (strcmp(ntpDescriptor.ntpServerHosts->at(i)->ntpHostName, p_ntpServerName))
			break;
	}
	if (i >= ntpDescriptor.ntpServerHosts->size()) {
		LOG_WARN("%s not found" CR, p_ntpServerName);
		return RC_NOT_FOUND_ERR;
	}
	p_serverStatus = ntpDescriptor.ntpServerHosts->at(i);
	return RC_OK;
}

rc_t ntpTime::getNtpServers(QList<ntpServerHost_t*>** p_ntpServerHosts) {
	*p_ntpServerHosts = ntpDescriptor.ntpServerHosts;
	return RC_OK;
}

uint8_t ntpTime::getReachability(uint8_t p_idx) {
	return sntp_getreachability(p_idx);
}

rc_t ntpTime::getNoOfNtpServers(uint8_t* p_ntpNoOfServerHosts) {
	*p_ntpNoOfServerHosts = ntpDescriptor.noOfServers;
	return RC_OK;
}

rc_t ntpTime::getNtpOpState(ntpOpState_t* p_ntpOpState) {
	*p_ntpOpState = ntpDescriptor.opState;
	Serial.printf("======================== %i" CR, ntpDescriptor.opState);
	return RC_OK;
}

rc_t ntpTime::getNtpOpStateStr(char* p_ntpOpStateStr) {
	strcpy(p_ntpOpStateStr, ntpDescriptor.opStateStr);
	return RC_OK;
}

rc_t ntpTime::setSyncMode(const uint8_t p_syncMode){
	char syncModeStr[50];
	LOG_INFO("Changing NTP sync mode to %s" CR, syncModeToStr(syncModeStr, p_syncMode));
	if (p_syncMode != SNTP_SYNC_MODE_IMMED && p_syncMode != SNTP_SYNC_MODE_SMOOTH){
		LOG_WARN("%i is not a valid NTP Sync mode" CR, p_syncMode);
		return RC_PARAMETERVALUE_ERR;
	}
	sntp_set_sync_mode((sntp_sync_mode_t)p_syncMode);
	restart();
	ntpDescriptor.syncMode = p_syncMode;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}
rc_t ntpTime::setSyncModeStr(const char* p_syncModeStr){
	uint8_t syncMode;
	LOG_INFO("Changing NTP sync mode to %s" CR, p_syncModeStr);
	if (syncModeToInt(p_syncModeStr, &syncMode)) {
		LOG_WARN("%s is not a valid NTP Sync mode" CR, p_syncModeStr);
		return RC_PARAMETERVALUE_ERR;
	}
	sntp_set_sync_mode((sntp_sync_mode_t)syncMode);
	restart();
	ntpDescriptor.syncMode = syncMode;
	updateNtpDescriptorStr(&ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::getSyncMode(uint8_t* p_syncMode){
	*p_syncMode = ntpDescriptor.syncMode;
	return RC_OK;
}

rc_t ntpTime::getSyncModeStr(char* p_syncModeStr){
	strcpy(p_syncModeStr, ntpDescriptor.syncMode_Str);
	return RC_OK;
}

rc_t ntpTime::getNtpSyncStatus(uint8_t* p_ntpSyncStatus) {
	*p_ntpSyncStatus = ntpDescriptor.syncStatus;
	return RC_OK;
}

rc_t ntpTime::getNtpSyncStatusStr(char* p_ntpSyncStatusStr) {
	strcpy(p_ntpSyncStatusStr, ntpDescriptor.syncStatusStr);
	return RC_OK;
}

rc_t ntpTime::setTimeOfDay(const char* p_timeOfDayStr, char* p_resultStr) {
	ntpDescriptor_t ntpDescriptor;
	struct tm t;
	rc_t rc;
	LOG_TERSE("Setting time of day from string to %s" CR, p_timeOfDayStr);
	if (p_resultStr)
		strcpy(p_resultStr, "");
	ntpOpState_t opState;
	getNtpOpState(&opState);
	if (!(opState & NTP_CLIENT_DISABLED)) {
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set time while NTP Client is running");
		LOG_WARN_NOFMT("Cannot set time while NTP Client is running" CR);
		return RC_ALREADYRUNNING_ERR;
	}
	if (!strptime(p_timeOfDayStr, "%Y-%m-%dT%H:%M:%S", &t)){
		if (p_resultStr)
			sprintf(p_resultStr, "Not a valid time format %s, expected format \"YYYY-MM-DDTHH:MM:SS\"" CR, p_timeOfDayStr);
		LOG_WARN("Not a valid time format %s, expected format \"YYYY-MM-DDTHH:MM:SS\"" CR, p_timeOfDayStr);
		return RC_PARAMETERVALUE_ERR;
	}
	if (rc = setTimeOfDay(&t)) {
		switch (rc) {
		case RC_PARAMETERVALUE_ERR:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set time of day, % s is not a valid time of day, expected format is YYYY - MM - DDTHH:MM:SS" CR, p_timeOfDayStr);
			LOG_WARN("Cannot set time of day, % s is not a valid time of day, expected format is YYYY - MM - DDTHH:MM:SS" CR, p_timeOfDayStr);
			break;
		case RC_ALREADYRUNNING_ERR:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set time of day while NTP client is running" CR);
			LOG_WARN_NOFMT("Cannot set time of day while NTP client is running" CR);
			break;
		case RC_GEN_ERR:
		default:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set epoch time of day - general error" CR);
			LOG_WARN_NOFMT("Cannot set time of day - general error" CR);
			break;
		}
		return rc;
	}
	LOG_VERBOSE_NOFMT("Successfully set time of day" CR);
	return RC_OK;
}

rc_t ntpTime::setTimeOfDay(tm* p_tm, char* p_resultStr) {
	timeval tv;
	tv.tv_usec = 0;
	LOG_TERSE_NOFMT("Setting time of day from tm" CR);
	if (p_resultStr)
		strcpy(p_resultStr, "");
	if (!(ntpDescriptor.opState & NTP_CLIENT_DISABLED)) {
		LOG_WARN_NOFMT("Cannot set time as NTP Client is running" CR);
		return RC_ALREADYRUNNING_ERR;
	}
	if ((tv.tv_sec = mktime(p_tm)) < 0 || settimeofday(&tv, NULL)) {
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set time of day, \"tm struct\" is not a valid time of day");
		LOG_WARN_NOFMT("Cannot set time of day, \"tm struct\"is not a valid time of day" CR);
		return RC_PARAMETERVALUE_ERR;
	}
	return RC_OK;
}

rc_t ntpTime::getTimeOfDay(char* p_timeOfDayStr, bool p_utc) {
	struct tm* t;
	getTimeOfDay(&t, p_utc);
	if (p_utc)
		strftime(p_timeOfDayStr, 50, "%a %b %e %Y - %H:%M:%S - UTC time", t);
	else
		strftime(p_timeOfDayStr, 50, "%a %b %e %Y - %H:%M:%S - Local time", t);
	return RC_OK;
}

rc_t ntpTime::getTimeOfDay(tm** p_timeOfDayTm, bool p_utc) {
	timeval tv;
	gettimeofday(&tv, NULL);
	if (p_utc){
		*p_timeOfDayTm = gmtime(&tv.tv_sec);
	}
	else
		*p_timeOfDayTm = localtime(&tv.tv_sec);
	return RC_OK;
}

rc_t ntpTime::setEpochTime(const char* p_epochTimeStr, char* p_resultStr) {
	timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = atoi(p_epochTimeStr);
	rc_t rc;
	LOG_TERSE("Setting epoch time from string to %s" CR, p_epochTimeStr);

	if (p_resultStr)
		strcpy(p_resultStr, "");
	if (!(ntpDescriptor.opState & NTP_CLIENT_DISABLED)) {
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set time as NTP Client is running");
		LOG_WARN_NOFMT(": Cannot set time as NTP Client is running" CR);
		return RC_ALREADYRUNNING_ERR;
	}
	if (!isIntNumberStr(p_epochTimeStr) || atoi(p_epochTimeStr) <= 0) {
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set epoch time, %s is not a valid epoch time" CR, p_epochTimeStr);
		LOG_WARN("Cannot set epoch time, %s is not a valid epoch time" CR, p_epochTimeStr);
		return RC_PARAMETERVALUE_ERR;
	}
	if (rc = setEpochTime(&tv, NULL)) {
		switch (rc) {
		case RC_PARAMETERVALUE_ERR:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set epoch time, %s is not a valid epoch time" CR, p_epochTimeStr);
			LOG_WARN("Cannot set epoch time, %s is not a valid epoch time" CR, p_epochTimeStr);
			break;
		case RC_ALREADYRUNNING_ERR:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set epoch time while NTP client is running" CR);
			LOG_WARN_NOFMT("Cannot set epoch time while NTP client is running" CR);
			break;
		case RC_GEN_ERR:
		default:
			if (p_resultStr)
				sprintf(p_resultStr, "Cannot set epoch time - general error" CR);
			LOG_WARN_NOFMT("Cannot set epoch time - general error" CR);
			break;
		}
		return rc;
	}
	return RC_OK;
}

rc_t ntpTime::setEpochTime(const timeval* p_tv, char* p_resultStr) {
	LOG_TERSE("Setting epoch time from tv to %i" CR, p_tv->tv_sec);
	if (p_resultStr)
		strcpy(p_resultStr, "");
	if (!(ntpDescriptor.opState & NTP_CLIENT_DISABLED)) {
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set time as NTP Client is running");
		LOG_WARN_NOFMT("Cannot set time as NTP Client is running" CR);
		return RC_ALREADYRUNNING_ERR;
	}
	if (settimeofday(p_tv, NULL)){
		if (p_resultStr)
			sprintf(p_resultStr, "Cannot set epoch time, \"tv struct\" is not a valid epoch time");
		LOG_WARN_NOFMT("Cannot set epoch time, \"tv struct\" is not a valid epoch time" CR);
		return RC_PARAMETERVALUE_ERR;
	}
	return RC_OK;
}

rc_t ntpTime::getEpochTime(char* p_epochTimeStr) {
	timeval tv;
	getEpochTime(&tv);
	itoa(tv.tv_sec, p_epochTimeStr, 10);
	return RC_OK;
}

rc_t ntpTime::getEpochTime(timeval* p_tv) {
	gettimeofday(p_tv, NULL);
	return RC_OK;
}

rc_t ntpTime::setTz(const char* p_tz, char* p_resultStr) {
	LOG_TERSE("Setting time-zone from string to %s" CR, p_tz);
	if (p_resultStr)
		strcpy(p_resultStr, "");
	setenv("TZ", p_tz, 1);
	tzset();
	restart();
	strcpy(ntpDescriptor.timeZone, p_tz);
	return RC_OK;
}

rc_t ntpTime::getTz(char* p_tz) {
	tm* t;
	getTimeOfDay(&t);
	strftime(p_tz, 50, "%Z%z", t);
	return RC_OK;
}

rc_t ntpTime::getTz(timezone** p_tz) {
	return gettimeofday(NULL, *p_tz);
}

rc_t ntpTime::setDayLightSaving(bool p_daylightSaving) {
	timeval tv;
	timezone tz;
	tm t;
	if(p_daylightSaving)
		LOG_TERSE_NOFMT("Setting daylightsaving to \"true\"" CR);
	else
		LOG_TERSE_NOFMT("Setting daylightsaving to \"false\"" CR);
	gettimeofday(&tv, &tz);
	
	t = *localtime(&(tv.tv_sec));
	t.tm_isdst = p_daylightSaving;
	settimeofday(&tv, &tz);
	return RC_OK;
}

rc_t ntpTime::getDayLightSaving(bool* p_daylightSaving) {
	timeval tv;
	timezone tz;
	tm t;
	gettimeofday(&tv, &tz);
	t = *localtime(&(tv.tv_sec));
	*p_daylightSaving = (bool)t.tm_isdst;
	return RC_OK;
}

rc_t ntpTime::setNtpOpState(ntpDescriptor_t* p_ntpDescriptor, ntpOpState_t p_opState){
	p_ntpDescriptor->opState = p_ntpDescriptor->opState | p_opState;
	updateNtpDescriptorStr(p_ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::unSetNtpOpState(ntpDescriptor_t* p_ntpDescriptor, ntpOpState_t p_opState) {
	p_ntpDescriptor->opState = p_ntpDescriptor->opState & ~p_opState;
	updateNtpDescriptorStr(p_ntpDescriptor);
	return RC_OK;
}

rc_t ntpTime::updateNtpDescriptorStr(ntpDescriptor_t* p_ntpDescriptor) {
	if (p_ntpDescriptor->opState == NTP_WORKING)
		sprintf(p_ntpDescriptor->opStateStr, "WORKING");
	else {
		strcpy(p_ntpDescriptor->opStateStr, "");
		if (p_ntpDescriptor->opState & NTP_CLIENT_DISABLED)
			strcat(p_ntpDescriptor->opStateStr, "DISABLED|");
		if (p_ntpDescriptor->opState & NTP_CLIENT_NOT_SYNCHRONIZED)
			strcat(p_ntpDescriptor->opStateStr, "NOTSYCRONIZED|");
		if (p_ntpDescriptor->opState & NTP_CLIENT_SYNCHRONIZING)
			strcat(p_ntpDescriptor->opStateStr, "SYNCHRONIZING|");
		p_ntpDescriptor->opStateStr[strlen(p_ntpDescriptor->opStateStr) - 1] = '\0';
	}
	switch (p_ntpDescriptor->syncStatus) {
	case SNTP_SYNC_STATUS_COMPLETED:
		strcpy(p_ntpDescriptor->syncStatusStr, "SYNC_STATUS_COMPLETED");
		break;
	case SNTP_SYNC_STATUS_IN_PROGRESS:
		strcpy(p_ntpDescriptor->syncStatusStr, "SYNC_STATUS_IN_PROGRESS");
		break;
	case SNTP_SYNC_STATUS_RESET:
		strcpy(p_ntpDescriptor->syncStatusStr, "SYNC_STATUS_RESET");
		break;
	default:
		strcpy(p_ntpDescriptor->syncStatusStr, "SYNC_STATUS_UNKNOWN");
		break;
	}
	switch (p_ntpDescriptor->syncMode) {
	case SNTP_SYNC_MODE_IMMED:
		strcpy(p_ntpDescriptor->syncMode_Str, "SYNC_MODE_IMMED");
		break;
	case SNTP_SYNC_MODE_SMOOTH:
		strcpy(p_ntpDescriptor->syncMode_Str, "SYNC_MODE_SMOOTH");
		break;
	default:
		strcpy(p_ntpDescriptor->syncMode_Str, "SYNC_MODE_UNKNOWN");
		break;
	}
	return RC_OK;
}

rc_t ntpTime::syncModeToInt(const char* p_syncModeStr, uint8_t* p_syncMode) {
	if (!strcmp(p_syncModeStr, "SYNC_MODE_IMMED"))
		*p_syncMode = SNTP_SYNC_MODE_IMMED;
	else if (!strcmp(p_syncModeStr, "SYNC_MODE_SMOOTH"))
		*p_syncMode = SNTP_SYNC_MODE_SMOOTH;
	else 
		return RC_PARAMETERVALUE_ERR;
	return RC_OK;
}

rc_t ntpTime::syncModeToStr(char* p_syncModeStr, uint8_t p_syncMode) {
	switch (p_syncMode) {
	case SNTP_SYNC_MODE_IMMED:
		strcpy(p_syncModeStr, "SYNC_MODE_IMMED");
		return RC_OK;
		break;
	case SNTP_SYNC_MODE_SMOOTH:
		strcpy(p_syncModeStr, "SYNC_MODE_SMOOTH");
		return RC_OK;
		break;
	default:
		return RC_PARAMETERVALUE_ERR;
	};
}

void ntpTime::sntpCb(timeval* p_tv) {
	Serial.printf("/////////////////////////////// NTP STATUS UPDATE");
	bool newSyncstate = false;
	ntpDescriptor.syncStatus = sntp_get_sync_status();
	switch (ntpDescriptor.syncStatus) {
	case SNTP_SYNC_STATUS_RESET:
		ntpDescriptor.opState = ntpDescriptor.opState | NTP_CLIENT_NOT_SYNCHRONIZED;
		ntpDescriptor.opState = ntpDescriptor.opState & ~NTP_CLIENT_SYNCHRONIZING;
		LOG_WARN_NOFMT("NTP not synchronized and is not synchronizing" CR);
		break;
	case SNTP_SYNC_STATUS_IN_PROGRESS:
		ntpDescriptor.opState = ntpDescriptor.opState | NTP_CLIENT_SYNCHRONIZING;
		ntpDescriptor.opState = ntpDescriptor.opState & ~NTP_CLIENT_NOT_SYNCHRONIZED;
		LOG_INFO_NOFMT("NTP is in the process of synchronizing" CR);
		break;
	case SNTP_SYNC_STATUS_COMPLETED:
		ntpDescriptor.opState = ntpDescriptor.opState & ~NTP_CLIENT_SYNCHRONIZING;
		ntpDescriptor.opState = ntpDescriptor.opState & ~NTP_CLIENT_NOT_SYNCHRONIZED;
		LOG_INFO_NOFMT("NTP is fully synchronized" CR);
		break;
	default:
		LOG_ERROR("Got an unexpected NTP synchronization state %i - resetting the NTP state-mashine" CR, ntpDescriptor.syncStatus);
		sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
		break;
	};
	for(uint8_t i=0; i<ntpDescriptor.ntpServerHosts->size(); i++)
		ntpDescriptor.ntpServerHosts->at(i)->reachability = sntp_getreachability(ntpDescriptor.ntpServerHosts->at(i)->index);
	updateNtpDescriptorStr(&ntpDescriptor);
	LOG_VERBOSE("Got an NTP server response with epoch time %i" CR, p_tv->tv_sec);
}
/*==============================================================================================================================================*/
/* END class time                                                                                                                               */
/*==============================================================================================================================================*/
