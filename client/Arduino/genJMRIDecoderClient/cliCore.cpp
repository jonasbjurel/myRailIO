/*============================================================================================================================================= =*/
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
#include "cliCore.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cliCore                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
telnetCore cliCore::telnetServer;
SimpleCLI cliCore::cliContextObjHandle;
SemaphoreHandle_t cliCore::cliCoreLock;
char* cliCore::clientIp;
cliCore* cliCore::rootCliContext;
cliCore* cliCore::currentContext;
QList<cliCmdTable_t*>* cliCore::cliCmdTable;

cliCore::cliCore(const char* p_moType) {
	cliCoreLock = xSemaphoreCreateMutex();
	strcpy(cliContextDescriptor.moType, p_moType);
	Command helpCliCmd = cliContextObjHandle.addBoundlessCommand("help", onCliCmd);
	Command rebootCliCmd = cliContextObjHandle.addBoundlessCommand("reboot", onCliCmd);
	Command showCliCmd = cliContextObjHandle.addBoundlessCommand("show", onCliCmd);
	Command getCliCmd = cliContextObjHandle.addBoundlessCommand("get", onCliCmd);
	Command setCliCmd = cliContextObjHandle.addBoundlessCommand("set", onCliCmd);
	Command unsetCliCmd = cliContextObjHandle.addBoundlessCommand("unset", onCliCmd);
	Command clearCliCmd = cliContextObjHandle.addBoundlessCommand("clear", onCliCmd);
	Command addCliCmd = cliContextObjHandle.addBoundlessCommand("add", onCliCmd);
	Command deleteCliCmd = cliContextObjHandle.addBoundlessCommand("delete", onCliCmd);
	Command copyCliCmd = cliContextObjHandle.addBoundlessCommand("copy", onCliCmd);
	Command pasteCliCmd = cliContextObjHandle.addBoundlessCommand("paste", onCliCmd);
	Command moveCliCmd = cliContextObjHandle.addBoundlessCommand("move", onCliCmd);
	Command startCliCmd = cliContextObjHandle.addBoundlessCommand("start", onCliCmd);
	Command stopCliCmd = cliContextObjHandle.addBoundlessCommand("stop", onCliCmd);
	Command restartCliCmd = cliContextObjHandle.addBoundlessCommand("restart", onCliCmd);
	cliContextObjHandle.setOnError(onCliError);
}

cliCore::~cliCore(void) {
	panic("cliCore::~cliCore: destruction not supported");
}

void cliCore::regParentContext(const cliCore* p_parentContext) {
	Log.notice("cliCore::regParentContext: Registing parent context: %s-%i to context: %s-%i" CR,
		((cliCore*)p_parentContext)->getCliContextDescriptor()->contextName,
		((cliCore*)p_parentContext)->getCliContextDescriptor()->contextIndex,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);

	cliContextDescriptor.parentContext = (cliCore*)p_parentContext;
}

void cliCore::unRegParentContext(const cliCore* p_parentContext) {
	Log.notice("cliCore::unRegParentContext: Un-registing parent context to context: %s-%i" CR,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);
	cliContextDescriptor.parentContext = NULL;
}

void cliCore::regChildContext(const cliCore* p_childContext, const char* p_contextName, uint16_t p_contextIndex) {
	Log.notice("cliCore::regChildContext: Registing child context: %s-%i to context: %s-%i" CR,
		p_contextName,
		p_contextIndex,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);
	cliContextDescriptor.childContexts->push_back((cliCore*)p_childContext);
	strcpy(((cliCore *)p_childContext)->getCliContextDescriptor()->contextName, p_contextName);
	((cliCore*)p_childContext)->getCliContextDescriptor()->contextIndex = p_contextIndex;
}

void cliCore::unRegChildContext(const cliCore* p_childContext) {
	Log.notice("cliCore::unRegChildContext: Un-registing child context: %s-%i from context: %s-%i" CR,
		((cliCore*)p_childContext)->getCliContextDescriptor()->contextName,
		((cliCore*)p_childContext)->getCliContextDescriptor()->contextIndex,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);
	cliContextDescriptor.childContexts->clear(cliContextDescriptor.childContexts->indexOf((cliCore*)p_childContext));
}

QList<cliCore*>* cliCore::getChildContexts(cliCore* p_cliContext) {
	if (p_cliContext)
		return p_cliContext->cliContextDescriptor.childContexts;
	else
		return cliContextDescriptor.childContexts;
}

void cliCore::setContextName(const char* p_contextName) {
	Log.notice("cliCore::setContextName: Setting context name: %s" CR, p_contextName);
	cliContextDescriptor.contextName = createNcpystr(p_contextName);
}

const char* cliCore::getContextName(void) {
	return cliContextDescriptor.contextName;
}

void cliCore::setContextIndex(uint16_t p_contextIndex) {
	Log.notice("cliCore::setContextName: Setting context index: %s" CR, p_contextIndex);
	cliContextDescriptor.contextIndex = p_contextIndex;
}

uint16_t cliCore::getContextIndex(void) {
	return cliContextDescriptor.contextIndex;
}

void cliCore::setContextSysName(const char* p_contextSysName) {
	Log.notice("cliCore::setContextSysName: Setting context sysName: %s" CR, p_contextSysName);
	cliContextDescriptor.contextSysName = createNcpystr(p_contextSysName);
}

const char* cliCore::getContextSysName(void) {
	return cliContextDescriptor.contextSysName;
}

void cliCore::start(void) {
	Log.notice("cliCore::start: Starting CLI" CR);
	rootCliContext = new cliCore(ROOT_MO_NAME);
	rootCliContext->regChildContext(this, ROOT_MO_NAME, 0);
	regParentContext(rootCliContext);
	currentContext = rootCliContext;
	telnetServer.regTelnetConnectCb(onCliConnect, NULL);
	telnetServer.regTelnetInputCb(onRootIngressCmd, NULL);
	if (telnetServer.start())
		Log.error("cliCore::start: Could not start the Telnet server" CR);
}

void cliCore::onCliConnect(const char* p_clientIp, bool p_connected, void* p_metaData) {
	Log.notice("cliCore::onCliConnect: A new CLI seesion from: %s started" CR, p_clientIp);
	if (clientIp)
		delete clientIp;
	clientIp = createNcpystr(p_clientIp);
	currentContext = rootCliContext;
	printCli("\r\nWelcome to JMRI generic decoder CLI - JMRI version: %s\nConnected from: %s" CR, GENJMRI_VERSION, clientIp);
	printCli("Type help for Help\n");
}

void cliCore::onRootIngressCmd(char* p_contextCmd, void* p_metaData) {
	xSemaphoreTake(cliCoreLock, portMAX_DELAY);
	Log.notice("cliCore::onRootIngressCmd: A new CLI command received: %s" CR, p_contextCmd);
	rc_t rc;
	rc = currentContext->onContextIngressCmd(p_contextCmd, false);
	if (rc) {
		Log.error("cliCore::onRootIngressCmd: Provided CLI context does not exist" CR);
		printCli("Provided CLI context does not exist");
	}
}

rc_t cliCore::onContextIngressCmd(char* p_contextCmd, bool p_setContext) {
	char nextHopContextPathRoute[50];
	Log.notice("cliCore::onContextIngressCmd: Processing cli context command: %s at context: %s-%i" CR,
		p_contextCmd, cliContextDescriptor.contextName,
		cliContextDescriptor.contextName);
	if (parseContextPath(p_contextCmd, nextHopContextPathRoute)) {
		return contextRoute(nextHopContextPathRoute, p_contextCmd, p_setContext);
	}
	else if (p_setContext) {
		currentContext = this;
	}
	cliContextObjHandle.parse(p_contextCmd);
	return RC_OK;
}

bool cliCore::parseContextPath(char* p_cmd, char* p_nextHopContextPathRoute) {
	char tmpCmdBuff[300];
		char* cmd;
	char* args[10];
	char* tmpNextHop;
	char* furureHops[10];
	strcpy(tmpCmdBuff, p_cmd);
	assert(sizeof(tmpCmdBuff) >= strlen(p_cmd));
	cmd = strtok(tmpCmdBuff, " ");
	int i = 0;
	while (args[i] = strtok(NULL, " "))
		i++;
	if (i == 0)
		return false;
	if (!strstr(args[0], "/"))
		return false;
	tmpNextHop = strtok(args[0], "/");
	i = 0;
	while (furureHops[i] = strtok(NULL, "/"))
		i++;
	strcpy(p_nextHopContextPathRoute, tmpNextHop);
	strcpy(p_cmd, cmd);
	strcat(p_cmd, " ");
	i = 0;
	while (furureHops[i]) {
		if (i)
			strcat(p_cmd, "/");
		strcat(p_cmd, furureHops[i++]);
	}
	i = 1;
	while (args[i]) {
		strcat(p_cmd, " ");
		strcat(p_cmd, args[i++]);
	}
	return true;
}

rc_t cliCore::contextRoute(char* p_nextHop, char* p_contextCmd, bool p_setContext) {
	char contextInstance[50];
	Log.notice("cliCore::contextRoute: Routing CLI command %s from CLI context: %s-%i to next hop CLI context:%s" CR, 
		p_contextCmd,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex,
		p_nextHop);
	if (p_nextHop == "..")
		return cliContextDescriptor.parentContext->onContextIngressCmd(p_contextCmd);
	else {
		for (uint16_t i = 0; i < cliContextDescriptor.childContexts->size(); i++) {
			sprintf(contextInstance, "%s-%i", cliContextDescriptor.childContexts->at(i)->getCliContextDescriptor()->contextName, cliContextDescriptor.childContexts->at(i)->getCliContextDescriptor()->contextIndex);
			if (contextInstance == p_nextHop) {
				return cliContextDescriptor.childContexts->at(i)->onContextIngressCmd(p_contextCmd);
			}
		}
		return RC_NOT_FOUND_ERR;
	}
}

rc_t cliCore::getFullCliContextPath(char* p_fullCliContextPath, const cliCore* p_cliContextHandle) {
	const cliCore* cliContextHandle;
	if (p_cliContextHandle)
		cliContextHandle = p_cliContextHandle;
	else
		cliContextHandle = this;

	if (((cliCore*)cliContextHandle)->getCliContextDescriptor()->parentContext)
		((cliCore*)cliContextHandle)->getCliContextDescriptor()->parentContext->getFullCliContextPath(p_fullCliContextPath);
	else {
		strcat(p_fullCliContextPath, ((cliCore*)cliContextHandle)->getCliContextDescriptor()->contextName);
		strcat(p_fullCliContextPath, "-");
		strcat(p_fullCliContextPath, itoa(((cliCore*)cliContextHandle)->getCliContextDescriptor()->contextIndex, NULL, 10));
		strcat(p_fullCliContextPath, "/");
		return RC_OK;
	}
}

cliCore* cliCore::getCliContextHandleByPath(const char* p_path) {
	cliCore* traverseContext = currentContext;
	char rootPath[150];
	strcpy(rootPath, p_path);
	char* traversePath = rootPath;
	bool found;
	if (traversePath[0] == '/') {
		traverseContext = rootCliContext;
		traversePath++;
	}
	while (strlen(traversePath)) {
		found = false;
		for (uint16_t i = 0; i < traverseContext->cliContextDescriptor.childContexts->size(); i++) {
			char contextIdentifier[50];
			sprintf(contextIdentifier, "%s-%i",
				traverseContext->cliContextDescriptor.childContexts->at(i)->cliContextDescriptor.contextName,
				traverseContext->cliContextDescriptor.childContexts->at(i)->cliContextDescriptor.contextIndex
			);
			if (strcmp(strtok(traversePath, "/"), contextIdentifier)) {
				traverseContext = traverseContext->cliContextDescriptor.childContexts->at(i);
				found = true;
				break;
			}
		}
		if (!found)
			return NULL;
	}
	return traverseContext;
}

// Example: https://stackoverflow.com/questions/66094905/how-to-pass-a-formatted-string-as-a-single-argument-in-c
void cliCore::printCli(const char* fmt, ...) {
	// determine required buffer size 
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	if (len < 0) return;
	// format message
	char* msg = new char[512];
	va_start(args, fmt);
	vsnprintf(msg, len + 1, fmt, args);
	va_end(args);
	// call output function
	if (msg[strlen(msg) - 1] == '\a') {
		msg[strlen(msg) - 1] = '\0';
		telnetServer.print(("%s\n", msg));
		char fullCliContextPath[100];
		currentContext->getFullCliContextPath(fullCliContextPath);
		telnetServer.print(fullCliContextPath);
		telnetServer.print(" >> ");
		xSemaphoreGive(cliCoreLock);
	}
	else
		telnetServer.print(("%s", msg));
}

rc_t cliCore::regCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg, cliCore* p_cliContext, cliCmdCb_t* p_cliCmdCb) {
	Log.notice("cliCore::regCmdMoArg: Registering command% for MO: %s - for cli context %s-%i" CR,
		getCliNameByType(p_commandType),
		p_cmdSubMoArg,
		p_cliContext->getCliContextDescriptor()->contextName,
		p_cliContext->getCliContextDescriptor()->contextIndex);
	for (uint16_t i = 0; i < cliCmdTable->size(); i++)
		if ((cliCmdTable->at(i)->cmdType == p_commandType) && (cliCmdTable->at(i)->mo == p_mo) && (cliCmdTable->at(i)->subMo == p_cmdSubMoArg)){
			for (uint16_t j = 0; i < cliCmdTable->at(i)->contextMap->size(); j++){
				if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle == this){
				Log.error("cliCore::regCmdMoArg: Cmd: %s, Mo: %s, sub-MO: %s already exists" CR, getCliNameByType(p_commandType), p_mo, p_cmdSubMoArg);
				return RC_ALREADYEXISTS_ERR;
				}
			}
			cliCmdTable->at(i)->contextMap->push_back(new contextMap_t);
			cliCmdTable->at(i)->contextMap->back()->contextHandle = this;
			cliCmdTable->at(i)->contextMap->back()->commandHandle = getCliCmdHandleByType(p_commandType);
			return RC_OK;
		}
	cliCmdTable->push_back(new cliCmdTable_t);
	cliCmdTable->back()->cmdType = p_commandType;
	strcpy(cliCmdTable->back()->mo, p_mo);
	strcpy(cliCmdTable->back()->subMo, p_cmdSubMoArg);
	cliCmdTable->back()->contextMap->push_back(new contextMap_t);
	cliCmdTable->back()->contextMap->back()->contextHandle = this;
	cliCmdTable->back()->contextMap->back()->commandHandle = getCliCmdHandleByType(p_commandType);
	return RC_OK;
}

rc_t cliCore::unRegCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg) {
	Log.notice("cliCore::unRegCmdMoArg: Un-registering command: %s% for MO: %s, sub-MO; %s - for cli context %s-%i" CR,
		getCliNameByType(p_commandType),
		p_mo,
		p_cmdSubMoArg,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);

	for (uint16_t i = 0; i < cliCmdTable->size(); i++)
		if ((cliCmdTable->at(i)->cmdType == p_commandType) && (cliCmdTable->at(i)->mo == p_mo) && (cliCmdTable->at(i)->subMo == p_cmdSubMoArg)) {
			for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
				if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle == this) {
					delete cliCmdTable->at(i)->contextMap->at(j);
					cliCmdTable->at(i)->contextMap->clear(j);
					if (!cliCmdTable->at(i)->contextMap->size()) {
						delete cliCmdTable->at(i);
						cliCmdTable->clear(i);
					}
					return RC_OK;
				}
			}
			cliCmdTable->clear(i);
			return RC_OK;
		}
	Log.error("cliCore::unRegCmdMoArg: Could not un-register command; %s% for MO: %s, sub-MO: %s - does not exist" CR,
		getCliNameByType(p_commandType),
		p_mo,
		p_cmdSubMoArg);
	return RC_NOT_FOUND_ERR;
}

rc_t cliCore::regCmdHelp(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg, const char* p_helpText) {
	Log.notice("cliCore::regCmdHelp: Registering help text for Command: %s MO: %s" CR, getCliNameByType(p_commandType), p_cmdSubMoArg);
	for (uint16_t i = 0; i < cliCmdTable->size(); i++)
		if ((cliCmdTable->at(i)->cmdType == p_commandType) && (cliCmdTable->at(i)->mo == p_mo) && (cliCmdTable->at(i)->subMo == p_cmdSubMoArg)) {
			cliCmdTable->at(i)->help = p_helpText;
			return RC_OK;
		}
	Log.error("cliCore::regCmdHelp: Registering help text for command: %s MO: %s, sub-MO: %s failed, not found" CR,
		getCliNameByType(p_commandType), 
		p_mo,
		p_cmdSubMoArg);
	return RC_NOT_FOUND_ERR;
}

rc_t cliCore::getHelp(const char* p_cmd, const char* p_cmdSubMoArg) {
	if (!strcmp(p_cmd, "help") && !strcmp(p_cmdSubMoArg, "cli")) {
		printCli("%s\n", GLOBAL_HELP_HELP_TXT);
		printCli("%s", GLOBAL_FULL_CLI_HELP_TXT);
		return RC_OK;
	}
	if (p_cmd && p_cmdSubMoArg) {
		char* moTypes[3] = { NULL, (char*)"global", (char*)"common" };
		moTypes[0] = cliContextDescriptor.moType;
		for (uint16_t i = 0; i < 3; i++) {
			for (uint16_t j = 0; j < cliCmdTable->size(); j++) {
				if ((cliCmdTable->at(i)->cmdType == getCliCmdHandleByType(getCliTypeByName(p_cmd))) && (cliCmdTable->at(i)->mo == moTypes[i]) && (cliCmdTable->at(i)->subMo == p_cmdSubMoArg)) {
					printCli("Help found for Managed Object (MO) type %s:\n", moTypes[i]);
					printCli("%s\n", cliCmdTable->at(i)->help);
					return RC_OK;
				}
			}
		}
		printCli("No Help text avaliable for %s %s\n", p_cmd, p_cmdSubMoArg);
		return RC_NOT_FOUND_ERR;
	}
	if (p_cmd && !p_cmdSubMoArg) {
		printCli(GLOBAL_FULL_CLI_HELP_TXT);
		return RC_OK;
	}
	if (!p_cmd && !p_cmdSubMoArg) {
		QList<cliCmdTable_t*> cliCmdTableSortHelper;
		printCli("%s\n\n", GLOBAL_FULL_CLI_HELP_TXT);
		if (cliCmdTable->size()) {
			for (uint i = 0; i < cliCmdTable->size(); i++)
				cliCmdTableSortHelper.push_back(cliCmdTable->at(i));
			for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
				cliCmdTable_t* lowestMoSortCmdHandle = cliCmdTableSortHelper.at(0);
				for (uint16_t j = 0; j < cliCmdTableSortHelper.size(); j++) {
					if (strcmp(cliCmdTableSortHelper.at(j)->mo, lowestMoSortCmdHandle->mo) < 0)
						lowestMoSortCmdHandle = cliCmdTableSortHelper.at(j);
				}
				printCli("Managed-object: %s help:\n %s\n\n", lowestMoSortCmdHandle->mo, lowestMoSortCmdHandle->help);
				cliCmdTableSortHelper.clear(cliCmdTableSortHelper.indexOf(lowestMoSortCmdHandle));
			}
		}
		else
			printCli("No further helptext available\n");
	}
}

void cliCore::onCliError(cmd_error* e) {
	CommandError cmdError(e); // Create wrapper object
	Log.error("cliCore::onCliError: CLI error: cmdError.toString()" CR);
	notAcceptedCliCommand(CLI_PARSE_ERR, cmdError.toString().c_str());

	// Print command usage
	if (cmdError.hasCommand())
		printCli("Did you mean \"%s\"?. Use \"help\" to show valid commands." CR, cmdError.getCommand().toString());
}

void cliCore::notAcceptedCliCommand(cmdErr_t p_cmdErr, const char* errStr, ...) {
	va_list args;
	va_start(args, errStr);
	int len = vsnprintf(NULL, 0, errStr, args);
	va_end(args);
	if (len < 0) return;
	// format message
	char* msg = new char[512];
	va_start(args, errStr);
	vsnprintf(msg, len + 1, errStr, args);
	va_end(args);

	switch (p_cmdErr) {
	case CLI_PARSE_ERR:
		printCli("ERROR - CLI command parse error: %s\a", errStr);
		return;
	case CLI_NOT_VALID_ARG_ERR:
		printCli("ERROR: - CLI argument error: %s: \a", errStr);
		return;
	case CLI_NOT_VALID_CMD_ERR:
		printCli("ERROR: - CLI command error: %s: \a", errStr);
		return;
	case CLI_GEN_ERR:
		printCli("ERROR: - CLI could not b e executed: %s: \a", errStr);
		return;
	}
}

void cliCore::acceptedCliCommand(successCmdTerm_t p_cmdTermType) {
	switch (p_cmdTermType) {
	case CLI_TERM_QUIET:
		printCli("\a");
		return;
	case CLI_TERM_EXECUTED:
		printCli("EXECUTED\a");
		return;
	case CLI_TERM_ORDERED:
		printCli("ORDERED\a");
		return;
	}
}

void cliCore::onCliCmd(cmd* p_cmd) {
	Command cmd(p_cmd);
	for (uint16_t i = 0; i < cliCmdTable->size(); i++)
		if ((getCliTypeByName(cmd.toString().c_str()) == cliCmdTable->at(i)->cmdType) && (!strcmp(cmd.getArgument(0).getValue().c_str(), cliCmdTable->at(i)->subMo))) {
			for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
				if (cliCmdTable->at(i)->contextMap->at(j)->commandHandle == cmd) {
					cliCmdTable->at(i)->cb(p_cmd, cliCmdTable->at(i)->contextMap->at(j)->contextHandle);
					Log.verbose("cliCore::onCliCmd: CLI context %s-%s received a CLI command %s %s" CR,
						cliCmdTable->at(i)->contextMap->at(j)->contextHandle->getCliContextDescriptor()->contextName,
						cliCmdTable->at(i)->contextMap->at(j)->contextHandle->getCliContextDescriptor()->contextIndex,
						cmd.toString(),
						cmd.getArgument(0).getValue().c_str());
					return;
				}
			}
			Log.info("cliCore::onCliCmd: Received a CLI command which hasnt been registered" CR);
		}
}

Command cliCore::getCliCmdHandleByType(cliMainCmd_t p_commandType) {
	switch (p_commandType){
	case HELP_CLI_CMD:
		return helpCliCmd;
	case REBOOT_CLI_CMD:
		return rebootCliCmd;
	case SHOW_CLI_CMD:
		return showCliCmd;
	case GET_CLI_CMD:
		return getCliCmd;
	case SET_CLI_CMD:
		return setCliCmd;
	case UNSET_CLI_CMD:
		return unsetCliCmd;
	case CLEAR_CLI_CMD:
		return clearCliCmd;
	case ADD_CLI_CMD:
		return addCliCmd;
	case DELETE_CLI_CMD:
		return deleteCliCmd;
	case MOVE_CLI_CMD:
		return moveCliCmd;
	case START_CLI_CMD:
		return startCliCmd;
	case STOP_CLI_CMD:
		return stopCliCmd;
	case RESTART_CLI_CMD:
		return restartCliCmd;
	default:
		return NULL;
	}
}

cliMainCmd_t cliCore::getCliTypeByName(const char* p_commandName) {
	if (!strcmp(p_commandName, "help"))
		return HELP_CLI_CMD;
	if (!strcmp(p_commandName, "reboot"))
		return REBOOT_CLI_CMD;
	if (!strcmp(p_commandName, "show"))
		return SHOW_CLI_CMD;
	if (!strcmp(p_commandName, "get"))
		return GET_CLI_CMD;
	if (!strcmp(p_commandName, "set"))
		return SET_CLI_CMD;
	if (!strcmp(p_commandName, "unset"))
		return UNSET_CLI_CMD;
	if (!strcmp(p_commandName, "clear"))
		return CLEAR_CLI_CMD;
	if (!strcmp(p_commandName, "add"))
		return ADD_CLI_CMD;
	if (!strcmp(p_commandName, "delete"))
		return DELETE_CLI_CMD;
	if (!strcmp(p_commandName, "move"))
		return MOVE_CLI_CMD;
	if (!strcmp(p_commandName, "start"))
		return START_CLI_CMD;
	if (!strcmp(p_commandName, "stop"))
		return STOP_CLI_CMD;
	if (!strcmp(p_commandName, "restart"))
		return RESTART_CLI_CMD;
	return ILLEGAL_CLI_CMD;
}

const char* cliCore::getCliNameByType(cliMainCmd_t p_commandType) {
	switch (p_commandType) {
	case HELP_CLI_CMD:
		return "help";
	case REBOOT_CLI_CMD:
		return "reboot";
	case SHOW_CLI_CMD:
		return "show";
	case GET_CLI_CMD:
		return "get";
	case SET_CLI_CMD:
		return "set";
	case UNSET_CLI_CMD:
		return "unset";
	case CLEAR_CLI_CMD:
		return "clear";
	case ADD_CLI_CMD:
		return "add";
	case DELETE_CLI_CMD:
		return "delete";
	case MOVE_CLI_CMD:
		return "move";
	case START_CLI_CMD:
		return "start";
	case STOP_CLI_CMD:
		return "stop";
	case RESTART_CLI_CMD:
		return "restart";
	default:
		return NULL;
	}
}

cliCore* cliCore::getCurrentContext(void) {
	return currentContext;
}

void cliCore::setCurrentContext(cliCore* p_currentContext) {
	currentContext = p_currentContext;
}

cli_context_descriptor_t* cliCore::getCliContextDescriptor(void) {
	return &cliContextDescriptor;
}

/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/
