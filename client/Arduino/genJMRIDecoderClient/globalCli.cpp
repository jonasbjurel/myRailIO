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
#include "globalCli.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: globalCli                                                                                                                             */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
globalCli* globalCli::rootHandle;


globalCli::globalCli(const char* p_moType, bool p_root) : cliCore(p_moType) {
	if (p_root)
		rootHandle = this;
	regCliMOCmds();
}

globalCli::~globalCli(void) {

}

void globalCli::regCliMOCmds(void) {
	regGlobalCliMOCmds();
	regCommonCliMOCmds();
}

void globalCli::regGlobalCliMOCmds(void) {
	regCmdMoArg(HELP_CLI_CMD, GLOBAL_MO_NAME, NULL, NULL, onCliHelp);
	regCmdHelp(HELP_CLI_CMD, GLOBAL_MO_NAME, NULL, GLOBAL_HELP_HELP_TXT);

	regCmdMoArg(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, NULL, onCliReboot);
	regCmdHelp(REBOOT_CLI_CMD, GLOBAL_MO_NAME, NULL, GLOBAL_REBOOT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, NULL, onCliGetContextHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, GLOBAL_GET_CONTEXT_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, NULL, onCliSetContextHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, CONTEXT_SUB_MO_NAME, GLOBAL_SET_CONTEXT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, NULL, onCliGetUptime);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, UPTIME_SUB_MO_NAME, GLOBAL_GET_UPTIME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, WIFI_SUB_MO_NAME, NULL, onCliGetWifi);
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("ssid");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("addr");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("mask");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("gw");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("dns");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("ntp");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("host/name");
	getCliCmdHandleByType(GET_CLI_CMD).addFlagArgument("broker");
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, WIFI_SUB_MO_NAME, GLOBAL_GET_WIFI_HELP_TXT);

	regCmdMoArg(SHOW_CLI_CMD, GLOBAL_MO_NAME, TOPOLOGY_SUB_MO_NAME, NULL, onClishowTopology);
	regCmdHelp(SHOW_CLI_CMD, GLOBAL_MO_NAME, TOPOLOGY_SUB_MO_NAME, GLOBAL_SHOW_TOPOLOGY_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTTBROKER_SUB_MO_NAME, this, onCliGetMqttBrokerURIHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, MQTTBROKER_SUB_MO_NAME, DECODER_GET_MQTTBROKER_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTTBROKER_SUB_MO_NAME, this, onCliSetMqttBrokerURIHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, MQTTBROKER_SUB_MO_NAME, DECODER_SET_MQTTBROKER_HELP_TXT);
	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, MQTTPORT_SUB_MO_NAME, this, onCliGetMqttPortHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, MQTTPORT_SUB_MO_NAME, DECODER_GET_MQTTPORT_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, MQTTPORT_SUB_MO_NAME, this, onCliSetMqttPortHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, MQTTPORT_SUB_MO_NAME, DECODER_SET_MQTTPORT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, KEEPALIVE_SUB_MO_NAME, this, onCliGetKeepaliveHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, KEEPALIVE_SUB_MO_NAME, DECODER_GET_KEEPALIVE_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, KEEPALIVE_SUB_MO_NAME, this, onCliSetKeepaliveHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, KEEPALIVE_SUB_MO_NAME, DECODER_SET_KEEPALIVE_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, PINGSUPERVISION_SUB_MO_NAME, this, onCliGetPingHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, PINGSUPERVISION_SUB_MO_NAME, DECODER_GET_PINGSUPERVISION_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, PINGSUPERVISION_SUB_MO_NAME, this, onCliSetPingHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, PINGSUPERVISION_SUB_MO_NAME, DECODER_SET_PINGSUPERVISION_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, NTPSERVER_SUB_MO_NAME, this, onCliGetNtpserverHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, NTPSERVER_SUB_MO_NAME, DECODER_GET_NTPSERVER_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, NTPSERVER_SUB_MO_NAME, this, onCliSetNtpserverHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, NTPSERVER_SUB_MO_NAME, DECODER_SET_NTPSERVER_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, NTPPORT_SUB_MO_NAME, this, onCliGetNtpportHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, NTPPORT_SUB_MO_NAME, DECODER_GET_NTPPORT_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, NTPPORT_SUB_MO_NAME, this, onCliSetNtpportHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, NTPPORT_SUB_MO_NAME, DECODER_SET_NTPPORT_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, TZ_SUB_MO_NAME, this, onCliGetTzHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, TZ_SUB_MO_NAME, DECODER_GET_TZ_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, TZ_SUB_MO_NAME, this, onCliSetTzHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, TZ_SUB_MO_NAME, DECODER_SET_TZ_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, LOGLEVEL_SUB_MO_NAME, this, onCliGetLoglevelHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, LOGLEVEL_SUB_MO_NAME, DECODER_GET_LOGLEVEL_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, LOGLEVEL_SUB_MO_NAME, this, onCliSetLoglevelHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, LOGLEVEL_SUB_MO_NAME, DECODER_SET_LOGLEVEL_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, this, onCliGetFailsafeHelper);
	regCmdHelp(GET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_GET_FAILSAFE_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, this, onCliSetFailsafeHelper);
	regCmdHelp(SET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_SET_FAILSAFE_HELP_TXT);
	regCmdMoArg(UNSET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, this, onCliUnsetFailsafeHelper);
	regCmdHelp(UNSET_CLI_CMD, GLOBAL_MO_NAME, FAILSAFE_SUB_MO_NAME, DECODER_UNSET_FAILSAFE_HELP_TXT);
}

void globalCli::regCommonCliMOCmds(void) {
	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, OPSTATE_SUB_MO_NAME, this, onCliGetOpStateHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, OPSTATE_SUB_MO_NAME, COMMON_GET_OPSTATE_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, this, onCliGetSysNameHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, COMMON_GET_SYSNAME_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, this, onCliSetSysNameHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, SYSNAME_SUB_MO_NAME, COMMON_SET_SYSNAME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, this, onCliGetUsrNameHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, COMMON_GET_USRNAME_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, this, onCliSetUsrNameHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, USER_SUB_MO_NAME, COMMON_SET_USRNAME_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, this, onCliGetDescHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, COMMON_GET_DESCRIPTION_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, this, onCliSetDescHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, DESC_SUB_MO_NAME, COMMON_SET_DESCRIPTION_HELP_TXT);

	regCmdMoArg(GET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, this, onCliGetDebugHelper);
	regCmdHelp(GET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, DECODER_GET_DEBUG_HELP_TXT);
	regCmdMoArg(SET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, this, onCliSetDebugHelper);
	regCmdHelp(SET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, DECODER_SET_DEBUG_HELP_TXT);
	regCmdMoArg(UNSET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, this, onCliUnsetDebugHelper);
	regCmdHelp(UNSET_CLI_CMD, COMMON_MO_NAME, DEBUG_SUB_MO_NAME, DECODER_UNSET_DEBUG_HELP_TXT);
}

void globalCli::regContextCliMOCmds(void) {
	Log.notice("globalCli::regContextCliMOCmds: No context unique MOs supported for CLI context BlaBla - would be good to have full path: full-path/contextName-context-index");
}


/* Global CLI decoration methods */
void globalCli::onCliHelp(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	rc_t rc;
	if (rc=p_cliContext->getHelp((char*)cmd.getArgument(0).getValue().c_str(), (char*)cmd.getArgument(1).getValue().c_str()))
		notAcceptedCliCommand(CLI_GEN_ERR, "Help request not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::onCliReboot(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(0)) 
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("rebooting...");
		acceptedCliCommand(CLI_TERM_EXECUTED);
		panic("\"CLI reboot order\" - rebooting...");
	}
}

void globalCli::onCliGetContextHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliGetContext(p_cmd, p_cliContext);
}

void globalCli::onCliGetContext(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		char fullCliContextPath[100];
		getFullCliContextPath(fullCliContextPath);
		printCli("Current context: %s", fullCliContextPath);
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

void globalCli::onCliSetContextHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliSetContext(p_cmd, p_cliContext);
}

void globalCli::onCliSetContext(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("Setting context to: %s\n", cmd.getArgument(1));
		Log.notice("Current context changed: %s\n", getCurrentContext());
		setCurrentContext(this);
		acceptedCliCommand(CLI_TERM_EXECUTED);
	}
}

void globalCli::onCliGetUptime(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	else {
		printCli("Uptime: %i", esp_timer_get_time()/1000);
		acceptedCliCommand(CLI_TERM_QUIET);
	}
}

void globalCli::onCliGetWifi(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	char configHeading[150];
	char config[150];
	Argument addrArg = cmd.getArgument("addr");
	Argument maskArg = cmd.getArgument("mask");
	Argument gwArg = cmd.getArgument("gw");
	Argument dnsArg = cmd.getArgument("dns");
	Argument ntpArg = cmd.getArgument("ntp");
	Argument hostArg = cmd.getArgument("hostname");
	Argument brokerArg = cmd.getArgument("broker");
	if (addrArg.isSet()) {
		strcat(configHeading, "Address:\t\t");
		strcat(config, ("%s\t\t", networking::getIpaddr().toString().c_str()));
	}
	if (maskArg.isSet()) {
		strcat(configHeading, "Mask:\t\t");
		strcat(config, ("%s\t\t", networking::getIpmask().toString().c_str()));
	}
	if (gwArg.isSet()) {
		strcat(configHeading, "Gateway:\t\t");
		strcat(config, ("%s\t\t", networking::getGateway().toString().c_str()));
	}
	if (dnsArg.isSet()) {
		strcat(configHeading, "DNS:\t\t");
		strcat(config, ("%s\t\t", networking::getDns().toString().c_str()));
	}
	if (ntpArg.isSet()) {
		strcat(configHeading, "NTP:\t\t");
		strcat(config, ("%s\t\t", networking::getNtp().toString().c_str()));
	}
	if (hostArg.isSet()) {
		strcat(configHeading, "Hostname:\t");
		strcat(config, ("%s\t", networking::getHostname()));
	}
	if (brokerArg.isSet()) {
		strcat(configHeading, "Broker:\t");
		strcat(config, ("%s\t", mqtt::getBrokerUri()));
	}
	printCli("%s\n%s", configHeading, config);
	acceptedCliCommand(CLI_TERM_QUIET);
}

void globalCli::onClishowTopology(cmd* p_cmd, cliCore* p_cliContext) {
	char contextPath[150];
	Command cmd(p_cmd);
	if (cmd.getArgument(2))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	if (cmd.getArgument(1)) {
		strcpy(contextPath, cmd.getArgument(1).getValue().c_str());
	}
	else
		strcpy(contextPath, "/");	
	reinterpret_cast<globalCli*>(getCliContextHandleByPath(contextPath))->printTopology();
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::printTopology(bool p_begining) {
	uint8_t noOfChilds;
	char contextPath[150];
	if (p_begining)
		printCli("Context-path:\t\t\tContext:\t\t\tSystem name:");
	getFullCliContextPath(contextPath);
	printCli("%s\t\t\t%s-%i\t\t\t%s",
		contextPath,
		getCliContextDescriptor()->contextName,
		getCliContextDescriptor()->contextIndex,
		getCliContextDescriptor()->contextSysName);

	for (uint8_t i = 0; i < getChildContexts(NULL)->size(); i++) {
		reinterpret_cast<globalCli*>(getChildContexts(NULL)->at(i))->printTopology(false);
	}
	if (p_begining) {
		printCli("END");
		acceptedCliCommand(CLI_TERM_QUIET);
		return RC_OK;
	}
}

void globalCli::onCliGetMqttBrokerURIHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	printCli("MQTT broker: %s", rootHandle->getMqttBrokerURI());
	acceptedCliCommand(CLI_TERM_QUIET);
}

const char* globalCli::getMqttBrokerURI(void) {
	return NULL;
}

void globalCli::onCliSetMqttBrokerURIHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setMqttBrokerURI(cmd.getArgument(0).getValue().c_str()))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of MQTT broker not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setMqttBrokerURI(const char* p_mqttBrokerURI, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetMqttPortHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("MQTT port: %i", rootHandle->getMqttPort());
	acceptedCliCommand(CLI_TERM_QUIET);
}

uint16_t globalCli::getMqttPort(void){}

void globalCli::onCliSetMqttPortHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setMqttPort(atoi(cmd.getArgument(0).getValue().c_str())))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of MQTT port not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setMqttPort(const uint16_t p_mqttPort, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetKeepaliveHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("MQTT keep-alive period: %i", rootHandle->getKeepAlivePeriod());
	acceptedCliCommand(CLI_TERM_QUIET);
}

float globalCli::getKeepAlivePeriod(void){}

void globalCli::onCliSetKeepaliveHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setKeepAlivePeriod(atof(cmd.getArgument(0).getValue().c_str())))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of MQTT Keep-alive period not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setKeepAlivePeriod(const float p_keepAlivePeriod, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetPingHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("Ping supervision period: %i", rootHandle->getPingPeriod());
	acceptedCliCommand(CLI_TERM_QUIET);
}

float globalCli::getPingPeriod(void){}

void globalCli::onCliSetPingHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setPingPeriod(atof(cmd.getArgument(0).getValue().c_str())))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of Ping supervision period not accepted, rerurn code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setPingPeriod(const float p_pingPeriod, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetNtpserverHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("NTP server: %s", rootHandle->getNtpServer());
	acceptedCliCommand(CLI_TERM_QUIET);
}

const char* globalCli::getNtpServer(void) {
	return NULL;
}

void globalCli::onCliSetNtpserverHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setNtpServer(cmd.getArgument(0).getValue().c_str()))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of NTP server URI not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setNtpServer(const char* p_ntpServer, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetNtpportHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("NTP port: %i", rootHandle->getNtpPort());
	acceptedCliCommand(CLI_TERM_QUIET);
}

uint16_t globalCli::getNtpPort(void) {}

void globalCli::onCliSetNtpportHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setNtpPort(atoi(cmd.getArgument(0).getValue().c_str())))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of NTP port URI not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setNtpPort(const uint16_t p_ntpPort, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetTzHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("Time zone: %i", rootHandle->getTz());
	acceptedCliCommand(CLI_TERM_QUIET);
}

uint8_t globalCli::getTz(void){}

void globalCli::onCliSetTzHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setTz(atoi(cmd.getArgument(0).getValue().c_str())))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of TZ not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setTz(const uint8_t p_tz, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetLoglevelHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	printCli("Log-level: %i", rootHandle->getLogLevel());
	acceptedCliCommand(CLI_TERM_QUIET);
}

const char* globalCli::getLogLevel(void) {
	return NULL;
}

void globalCli::onCliSetLoglevelHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setLogLevel(cmd.getArgument(0).getValue().c_str()))
		notAcceptedCliCommand(CLI_GEN_ERR, "Setting of NTP port URI not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setLogLevel(const char* p_logLevel, bool p_force) {
	return RC_GEN_ERR;
}

void globalCli::onCliGetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rootHandle->getFailSafe())
		printCli("Failsafe: Active");
	else
		printCli("Failsafe: Inactive");
	acceptedCliCommand(CLI_TERM_QUIET);
}

bool globalCli::getFailSafe(void){}

void globalCli::onCliSetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setFailSafe(true))
		notAcceptedCliCommand(CLI_GEN_ERR, "Activating fail-safe not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliUnsetFailsafeHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (rc = rootHandle->setFailSafe(false))
		notAcceptedCliCommand(CLI_GEN_ERR, "In-activating fail-safe not accepted, return code: %i", rc);
	else
		acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setFailSafe(const bool p_failsafe, bool p_force) {
	return RC_GEN_ERR;
}

/* Common CLI decoration methods */
void globalCli::onCliGetOpStateHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliGetOpState(p_cmd, p_cliContext);
}

void globalCli::onCliGetOpState(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	char opStateStr[100];
	if (!cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	if (!getOpStateStr(opStateStr)){
		printCli("OP-state: %s", opStateStr);
		acceptedCliCommand(CLI_TERM_QUIET);
	}
	else {
		notAcceptedCliCommand(CLI_GEN_ERR, "get opstate failed");
	}
}

rc_t globalCli::getOpStateStr(char* p_opStateStr){
	notAcceptedCliCommand(CLI_GEN_ERR, "getOpstate not implemented for context %s-%i", getContextName(), getContextIndex());
	return NULL;
}

void globalCli::onCliGetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliGetSysName(p_cmd, p_cliContext);
}

void globalCli::onCliGetSysName(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1))
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
	char sysName[50];
	getSystemName(sysName);
	printCli("System name: %s", sysName);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getSystemName(const char* p_systemName) {
	notAcceptedCliCommand(CLI_GEN_ERR, "getSystemName not implemented for context %s-%i", getContextName(), getContextIndex());
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliSetSysNameHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliSetSysName(p_cmd, p_cliContext);
}
void globalCli::onCliSetSysName(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	setSystemName(cmd.getArgument(1).getValue().c_str());
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setSystemName(const char* p_systemName) {
	notAcceptedCliCommand(CLI_GEN_ERR, "setSystemName not implemented for context %s-%i", getContextName(), getContextIndex());
	return RC_NOTIMPLEMENTED_ERR;
}

void globalCli::onCliGetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliGetUsrName(p_cmd, p_cliContext);
}

void globalCli::onCliGetUsrName(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	char usrName[50];
	getUsrName(usrName);
	printCli("User name: %s", usrName);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getUsrName(const char* p_usrName) {
	notAcceptedCliCommand(CLI_GEN_ERR, "getUsrName not implemented for context %s-%i", getContextName(), getContextIndex());
}

void globalCli::onCliSetUsrNameHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliSetUsrName(p_cmd, p_cliContext);
}

void globalCli::onCliSetUsrName(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	setUsrName(cmd.getArgument(1).getValue().c_str());
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setUsrName(const char* p_usrName) {
	notAcceptedCliCommand(CLI_GEN_ERR, "setUsrName not implemented for context %s-%i", getContextName(), getContextIndex());
}

void globalCli::onCliGetDescHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliGetDesc(p_cmd, p_cliContext);
}

void globalCli::onCliGetDesc(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	char description[50];
	getDesc(description);
	printCli("Description: %s", description);
	acceptedCliCommand(CLI_TERM_QUIET);
}

rc_t globalCli::getDesc(const char* p_description) {
	notAcceptedCliCommand(CLI_GEN_ERR, "getDesc not implemented for context %s-%i", getContextName(), getContextIndex());
}

void globalCli::onCliSetDescHelper(cmd* p_cmd, cliCore* p_cliContext) {
	reinterpret_cast<globalCli*>(p_cliContext)->onCliSetDesc(p_cmd, p_cliContext);
}

void globalCli::onCliSetDesc(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (!cmd.getArgument(1) || cmd.getArgument(2)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	setDesc(cmd.getArgument(1).getValue().c_str());
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

rc_t globalCli::setDesc(const char* p_description) {
	notAcceptedCliCommand(CLI_GEN_ERR, "setDesc not implemented for context %s-%i", getContextName(), getContextIndex());
}

void globalCli::onCliGetDebugHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	rc_t rc;
	if (reinterpret_cast<globalCli*>(p_cliContext)->getDebug())
		printCli("Debug: Active");
	else
		printCli("Debug: Inactive");
	acceptedCliCommand(CLI_TERM_QUIET);
}

bool globalCli::getDebug(void) {}

void globalCli::onCliSetDebugHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	reinterpret_cast<globalCli*>(p_cliContext)->setDebug(true);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::onCliUnsetDebugHelper(cmd* p_cmd, cliCore* p_cliContext) {
	Command cmd(p_cmd);
	if (cmd.getArgument(1)) {
		notAcceptedCliCommand(CLI_NOT_VALID_ARG_ERR, "Bad number of arguments");
		return;
	}
	reinterpret_cast<globalCli*>(p_cliContext)->setDebug(false);
	acceptedCliCommand(CLI_TERM_EXECUTED);
}

void globalCli::setDebug(bool p_debug){}

/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/
