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
EXT_RAM_ATTR telnetCore cliCore::telnetServer;
EXT_RAM_ATTR SimpleCLI cliCore::cliContextObjHandle;
EXT_RAM_ATTR SemaphoreHandle_t cliCore::cliCoreLock;
EXT_RAM_ATTR char* cliCore::clientIp = NULL;
EXT_RAM_ATTR cliCore* cliCore::rootCliContext;
EXT_RAM_ATTR cliCore* cliCore::currentContext;
EXT_RAM_ATTR cliCore* cliCore::currentParsingContext;
EXT_RAM_ATTR QList<cliCmdTable_t*>* cliCore::cliCmdTable;
EXT_RAM_ATTR QList<cliCore*> cliCore::allContexts;

cliCore::cliCore(const char* p_moType, const char* p_moName, uint16_t p_moIndex,
				 cliCore* p_parentContext, bool p_root) {
	if (!(cliCoreLock = xSemaphoreCreateMutex())){
		panic("Could not create Lock objects");
		return;
	}
	allContexts.push_back(this);
	cliContextDescriptor.active = false;
	cliContextDescriptor.moType = NULL;
	cliContextDescriptor.childContexts = NULL;
	cliContextDescriptor.contextIndex = 0;
	cliContextDescriptor.contextName = NULL;
	cliContextDescriptor.contextSysName = NULL;
	cliContextDescriptor.parentContext = NULL;
	setContextType(p_moType);
	setContextName(p_moName);
	setContextIndex(p_moIndex);
	if (p_parentContext) {
		regParentContext(p_parentContext);
		p_parentContext->regChildContext(this);
	}
	cliContextDescriptor.childContexts = new QList<cliCore*>;
	if (p_root) {
		cliCmdTable = new QList<cliCmdTable_t*>;
		LOG_INFO("Creating Root cliCore object: %i, for MO-type %s, " \
				 "CLI context: %s-%i" CR, this, p_moType, p_moName, p_moIndex);
		rootCliContext = this;
		currentContext = this;
		currentParsingContext = this;
		cliContextDescriptor.parentContext = NULL;
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
	else
		LOG_INFO("Creating cliCore object: %i, for MO-type %s, " \
				 "CLI context: %s-%i" CR, this, p_moType, p_moName, p_moIndex);
}

cliCore::~cliCore(void) {
	panic("destruction not supported");
}

void cliCore::regParentContext(const cliCore* p_parentContext) {
	LOG_INFO("Registing parent context: %s-%i " \
             "to context : %s-%i" CR,
			 ((cliCore*)p_parentContext)->getCliContextDescriptor()->contextName,
			 ((cliCore*)p_parentContext)->getCliContextDescriptor()->contextIndex,
			 cliContextDescriptor.contextName,
			 cliContextDescriptor.contextIndex);
	cliContextDescriptor.parentContext = (cliCore*)p_parentContext;
}

void cliCore::unRegParentContext(const cliCore* p_parentContext) {
	LOG_INFO("Un-registing parent context " \
			 "to context: %s-%i" CR,
			 cliContextDescriptor.contextName,
			 cliContextDescriptor.contextIndex);
			 cliContextDescriptor.parentContext = NULL;
}

void cliCore::regChildContext(const cliCore* p_childContext) {
	LOG_INFO("Registing child context: %s-%i " \
			 "to context : %s-%i" CR,
			 ((cliCore*)p_childContext)->getCliContextDescriptor()->contextName,
			 ((cliCore*)p_childContext)->getCliContextDescriptor()->contextIndex,
			 cliContextDescriptor.contextName,
			 cliContextDescriptor.contextIndex);
	cliContextDescriptor.childContexts->push_back((cliCore*)p_childContext);
}

void cliCore::unRegChildContext(const cliCore* p_childContext) {
	LOG_INFO("Un-registing child context: " \
			 "%s-%i from context : %s-%i" CR,
			 ((cliCore*)p_childContext)->getCliContextDescriptor()->contextName,
			 ((cliCore*)p_childContext)->getCliContextDescriptor()->contextIndex,
			 cliContextDescriptor.contextName,
			 cliContextDescriptor.contextIndex);
	cliContextDescriptor.childContexts->clear(cliContextDescriptor.childContexts->
				indexOf((cliCore*)p_childContext));
}

QList<cliCore*>* cliCore::getChildContexts(cliCore* p_cliContext) {
	if (p_cliContext)
		return p_cliContext->cliContextDescriptor.childContexts;
	else
		return cliContextDescriptor.childContexts;
}

const char* cliCore::getConnectedClient(void) {
	return clientIp;
}
void cliCore::setContextType(const char* p_contextType) {
	LOG_INFO("Setting context type: %s" CR, p_contextType);
	if (cliContextDescriptor.moType)
		delete cliContextDescriptor.moType;
	cliContextDescriptor.moType = createNcpystr(p_contextType);
}

const char* cliCore::getContextType(void) {
	return cliContextDescriptor.moType;
}

void cliCore::setContextName(const char* p_contextName) {
	LOG_INFO("Setting context name: %s" CR, p_contextName);
	if (cliContextDescriptor.contextName)
		delete cliContextDescriptor.contextName;
	cliContextDescriptor.contextName = createNcpystr(p_contextName);
}

const char* cliCore::getContextName(void) {
	return cliContextDescriptor.contextName;
}

void cliCore::setContextIndex(uint16_t p_contextIndex) {
	LOG_INFO("Setting context index: %i" CR,
				p_contextIndex);
	cliContextDescriptor.contextIndex = p_contextIndex;
}

uint16_t cliCore::getContextIndex(void) {
	return cliContextDescriptor.contextIndex;
}

void cliCore::setContextSysName(const char* p_contextSysName) {
	LOG_INFO("Setting context sysName: %s" CR,
				p_contextSysName);
	cliContextDescriptor.contextSysName = createNcpystr(p_contextSysName);
}

const char* cliCore::getContextSysName(void) {
	return cliContextDescriptor.contextSysName;
}

void cliCore::start(void) {
	LOG_INFO_NOFMT("Starting Telnet and CLI service" CR);
	telnetServer.regTelnetConnectCb(onCliConnect, NULL);
	telnetServer.regTelnetInputCb(onRootIngressCmd, NULL);
	if (telnetServer.start())
		LOG_ERROR_NOFMT("Could not start the Telnet server" CR);
	LOG_VERBOSE_NOFMT("Telnet and CLI service successfully started" CR);

}

void cliCore::onCliConnect(const char* p_clientIp, bool p_connected, void* p_metaData) {
	if (p_connected) {
		LOG_INFO("A new CLI seesion from: %s started" CR,
					p_clientIp);
		if (clientIp) {
			delete clientIp;
			clientIp = NULL;
		}
		clientIp = createNcpystr(p_clientIp);
		currentContext = rootCliContext;
		printCli("\n\rWelcome to JMRI generic decoder CLI - JMRI version: %s",
				MYRAILIO_VERSION);
		printCli("Connected from: %s",  clientIp);
		printCli("Type help for Help\a");
	}
	else {
		LOG_INFO("The CLI seesion from: %s was closed" CR,
			p_clientIp);
		if (clientIp) {
			delete clientIp;
			clientIp = NULL;
		}
	}
}

void cliCore::onRootIngressCmd(char* p_contextCmd, void* p_metaData) {
	//xSemaphoreTake(cliCoreLock, portMAX_DELAY);
	LOG_INFO("A new CLI command received: \"%s\"" CR,
				p_contextCmd);
	rc_t rc;
	rc = currentContext->onContextIngressCmd(p_contextCmd, false);
	currentParsingContext = currentContext;
	if (rc) {
		LOG_ERROR_NOFMT("Provided CLI context does not exist" CR);
		printCli("Provided CLI context does not exist\a");
	}
	//xSemaphoreGive(cliCoreLock);

}

rc_t cliCore::onContextIngressCmd(char* p_contextCmd, bool p_setContext) {
	currentParsingContext = this;
	char nextHopContextPathRoute[50];
	LOG_INFO("Processing cli context command: \"%s\" " \
				"at context: %s-%i" CR,
		p_contextCmd, cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);
	if (parseContextPath(p_contextCmd, nextHopContextPathRoute)) {
		LOG_VERBOSE("Routing to next hop: %s" CR,
					 nextHopContextPathRoute);
		return contextRoute(nextHopContextPathRoute, p_contextCmd, p_setContext);
	}
	else if (p_setContext) {
		currentContext = this;
		LOG_VERBOSE("On target context %s-%i, " \
					 "no routing needed, setting current context" CR,
					 cliContextDescriptor.contextName, cliContextDescriptor.contextIndex);
	}
	LOG_VERBOSE("On target context %s-%i, " \
				 "no routing needed" CR, cliContextDescriptor.contextName,
				 cliContextDescriptor.contextIndex);
	LOG_VERBOSE("Parsing command %s in context: %s-%i" CR,
				 p_contextCmd, cliContextDescriptor.contextName,
				 cliContextDescriptor.contextIndex);
	cliContextObjHandle.parse(p_contextCmd);
	return RC_OK;
}

bool cliCore::parseContextPath(char* p_cmd, char* p_nextHopContextPathRoute) {
	char tmpCmdBuff[300];
		char* cmd;
	char* args[10];
	char* tmpNextHop;
	char* futureHops[10];
	assert(sizeof(tmpCmdBuff) >= strlen(p_cmd));
	strcpy(tmpCmdBuff, p_cmd);
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
	while (futureHops[i] = strtok(NULL, "/"))
		i++;
	strcpy(p_nextHopContextPathRoute, tmpNextHop);
	strcpy(p_cmd, cmd);
	strcat(p_cmd, " ");
	i = 0;
	while (futureHops[i]) {
		if (i)
			strcat(p_cmd, "/");
		strcat(p_cmd, futureHops[i++]);
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
	LOG_INFO("Routing CLI command \"%s\" from CLI context: " \
				"%s-%i to next hop CLI context: %s" CR, 
				p_contextCmd,
				cliContextDescriptor.contextName,
				cliContextDescriptor.contextIndex,
				p_nextHop);
	if (!strcmp(p_nextHop, ".."))
		return cliContextDescriptor.parentContext->onContextIngressCmd(p_contextCmd, p_setContext);
	else if (!strcmp(p_nextHop, "."))
		return onContextIngressCmd(p_contextCmd, p_setContext);
	else {
		for (uint16_t i = 0; i < cliContextDescriptor.childContexts->size(); i++) {
			sprintf(contextInstance, "%s-%i", cliContextDescriptor.childContexts->
					at(i)->getCliContextDescriptor()->contextName, 
					cliContextDescriptor.childContexts->at(i)->
					getCliContextDescriptor()->contextIndex);
			if (!strcmp(contextInstance, p_nextHop)) {
				return cliContextDescriptor.childContexts->at(i)->
						onContextIngressCmd(p_contextCmd, p_setContext);
			}
		}
		return RC_NOT_FOUND_ERR;
	}
}

rc_t cliCore::getFullCliContextPath(char* p_fullCliContextPath,
									const cliCore* p_cliContextHandle, bool p_first) {
	const cliCore* cliContextHandle;
	if (p_cliContextHandle)
		cliContextHandle = p_cliContextHandle;
	else
		cliContextHandle = this;
	if (p_first)
		strcpy(p_fullCliContextPath, "");
	if (((cliCore*)cliContextHandle)->getCliContextDescriptor()->parentContext) {
		((cliCore*)cliContextHandle)->getCliContextDescriptor()->parentContext->
			getFullCliContextPath(p_fullCliContextPath, NULL, false);
	}
	else {
		strcat(p_fullCliContextPath, ((cliCore*)cliContextHandle)->
			getCliContextDescriptor()->contextName);
		strcat(p_fullCliContextPath, "-");
		char index[4];
		strcat(p_fullCliContextPath, itoa(((cliCore*)cliContextHandle)->
			getCliContextDescriptor()->contextIndex, index, 10));
		strcat(p_fullCliContextPath, "/");
		return RC_OK;
	}
}

cliCore* cliCore::getCliContextHandleByPath(const char* p_path) {
	char* path = (char*)p_path;
	cliCore* traverseContext = currentContext;
	strcpy(path, p_path);
	if (path[0] == '/') {
		traverseContext = rootCliContext;
		path++;
	}
	while (true) {
		bool allowEmptyContext = false;
		if ((path[0] == '.') && (path[1] == '.') && (path[1] = '/')) {
			if (!traverseContext->cliContextDescriptor.parentContext) {
				LOG_VERBOSE_NOFMT("There is no parent context" CR);
				return NULL;
			}
			else {
				traverseContext = traverseContext->cliContextDescriptor.parentContext;
				LOG_VERBOSE_NOFMT("Ascending to parent context" CR);
				allowEmptyContext = true;
				path += 3;
			}
			continue;
		}
		if ((path[0] == '.') && (path[1] == '/')) {
			LOG_VERBOSE_NOFMT("Staying on context" CR);
			allowEmptyContext = true;
			path += 2;
			continue;
		}

		if (!strlen(path)) {
			return traverseContext;
		}
		else {
			char nextContextStr[50];
			uint8_t nextContextStrLen = 0;
			while (strlen(path) && path[0] != '/') {
				nextContextStr[nextContextStrLen] = path[0];
				path++;
				nextContextStrLen++;
			}
			if (path[0] == '/')
				path++;
			nextContextStr[nextContextStrLen] = '\0';
			bool found = false;
			for (uint16_t i = 0; i < traverseContext->
				cliContextDescriptor.childContexts->size(); i++) {
				char contextId[50];
				sprintf(contextId, "%s-%i",
					traverseContext->cliContextDescriptor.childContexts->at(i)->
					cliContextDescriptor.contextName,
					traverseContext->cliContextDescriptor.childContexts->at(i)->
					cliContextDescriptor.contextIndex);
				if (!strcmp(contextId, nextContextStr)) {
					traverseContext = traverseContext->
						cliContextDescriptor.childContexts->at(i);
					found = true;
					break;
				}
			}
			if (!found) {													//TR: HIGH ABSOUTE PATHS NOT WORKING, IE: set context /decoder-0/lglink-0
				LOG_VERBOSE_NOFMT("Child contexts not found" CR);
				return NULL;
			}
		}
	}
}

void cliCore::printCli(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	if (len < 0) return;
	// format message
	char msg[512];
	va_start(args, fmt);
	vsnprintf(msg, len + 1, fmt, args);
	va_end(args);
	// call output function
	if (msg[strlen(msg) - 1] == '\a') {
		msg[strlen(msg) - 1] = '\0';
		telnetServer.print(msg);
		telnetServer.print("\n\r");
		char fullCliContextPath[100];
		currentContext->getFullCliContextPath(fullCliContextPath);
		telnetServer.print(fullCliContextPath);
		telnetServer.print(" >> ");
	}
	else {
		telnetServer.print(msg);
		telnetServer.print("\n\r");
	}
}

void cliCore::printCliNoFormat(char* p_msg) {
	if (p_msg[strlen(p_msg) - 1] == '\a') {
		p_msg[strlen(p_msg) - 1] = '\0';
		telnetServer.print("\n\r");
		char fullCliContextPath[100];
		currentContext->getFullCliContextPath(fullCliContextPath);
		telnetServer.print(fullCliContextPath);
		telnetServer.print(" >> ");
	}
	else {
		telnetServer.print(p_msg);
		telnetServer.print("\n\r");
	}
}

rc_t cliCore::regCmdMoArg(cliMainCmd_t p_commandType, const char* p_mo,
	const char* p_cmdSubMoArg, cliCmdCb_t* p_cliCmdCb) {
	char cmdSubMoArgPrint[50];
	char cmdSubMoArg[50];
	if (p_cmdSubMoArg) {
		strcpy(cmdSubMoArgPrint, p_cmdSubMoArg);
		strcpy(cmdSubMoArg, p_cmdSubMoArg);
	}
	else {
		strcpy(cmdSubMoArgPrint, "-");
		strcpy(cmdSubMoArg, "");
	}
	LOG_VERBOSE("Registering command: " \
		"%s for MO: %s for subMo: %s for cli context: %s-%i" CR,
		getCliNameByType(p_commandType),
		p_mo, cmdSubMoArgPrint, getContextName(), getContextIndex());

	for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
		if ((cliCmdTable->at(i)->cmdType == p_commandType) &&
			(!strcmp(cliCmdTable->at(i)->mo, p_mo)) &&
			(!strcmp(cliCmdTable->at(i)->subMo, cmdSubMoArg))) {
			for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
				if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle == this) {
					LOG_ERROR("Cmd: %s, Mo: %s, sub-MO: " \
						"%s already exists" CR, getCliNameByType(p_commandType),
						p_mo, cmdSubMoArg);
					return RC_ALREADYEXISTS_ERR;
				}
			}
			cliCmdTable->at(i)->contextMap->push_back(new (heap_caps_malloc(sizeof(contextMap_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) contextMap_t);
			if (!cliCmdTable->at(i)->contextMap->back()) {
				panic("Could not create context list item");
				return RC_OUT_OF_MEM_ERR;
			}
			cliCmdTable->at(i)->contextMap->back()->contextHandle = this;
			cliCmdTable->at(i)->contextMap->back()->cb = p_cliCmdCb;
			return RC_OK;
		}
	}
	cliCmdTable->push_back(new (heap_caps_malloc(sizeof(cliCmdTable_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) cliCmdTable_t);
	if (!cliCmdTable->back()) {
		panic("Could not create command table list item");
		return RC_OUT_OF_MEM_ERR;
	}
	cliCmdTable->back()->cmdType = p_commandType;
	strcpy(cliCmdTable->back()->mo, p_mo);
	strcpy(cliCmdTable->back()->subMo, cmdSubMoArg);
	cliCmdTable->back()->contextMap = new (heap_caps_malloc(sizeof(cliCmdTable_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) QList< contextMap_t*>;
	if (!cliCmdTable->back()->contextMap){
		panic("Could not create context map list");
		return RC_OUT_OF_MEM_ERR;
	}
	cliCmdTable->back()->contextMap->push_back(new (heap_caps_malloc(sizeof(contextMap_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) contextMap_t);
	if (!cliCmdTable->back()->contextMap->back()) {
		panic("Could not create context map list item");
		return RC_OUT_OF_MEM_ERR;
	}
	cliCmdTable->back()->contextMap->back()->contextHandle = this;
	cliCmdTable->back()->contextMap->back()->cb = p_cliCmdCb;
	cliCmdTable->back()->commandFlags = NULL;
	return RC_OK;
}

rc_t cliCore::unRegCmdMoArg(cliMainCmd_t p_commandType,
							const char* p_mo, const char* p_cmdSubMoArg) {
	char cmdSubMoArgPrint[50];
	char cmdSubMoArg[50];
	if (p_cmdSubMoArg) {
		strcpy(cmdSubMoArgPrint, p_cmdSubMoArg);
		strcpy(cmdSubMoArg, p_cmdSubMoArg);
	}
	else {
		strcpy(cmdSubMoArgPrint, "-");
		strcpy(cmdSubMoArg, "");
	}
	LOG_TERSE("Un-registering command: " \
			  "%s for MO:%s, sub-MO: %s - for cli context: %s-%i" CR,
		getCliNameByType(p_commandType),
		p_mo,
		cmdSubMoArgPrint,
		cliContextDescriptor.contextName,
		cliContextDescriptor.contextIndex);

	for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
		if ((cliCmdTable->at(i)->cmdType == p_commandType) &&
			(!strcmp(cliCmdTable->at(i)->mo, p_mo)) &&
			(!strcmp(cliCmdTable->at(i)->subMo, cmdSubMoArg))) {
			for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
				if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle == this) {
					delete cliCmdTable->at(i)->contextMap->at(j);
					cliCmdTable->at(i)->contextMap->clear(j);
					if (!cliCmdTable->at(i)->contextMap->size()) {
						delete cliCmdTable->at(i)->contextMap;
						cliCmdTable->clear(i);
					}
					return RC_OK;
				}
			}
			cliCmdTable->clear(i);
			return RC_OK;
		}
	}
	LOG_ERROR("Could not un-register command; %s " \
			  "for MO: %s, sub-MO: %s - does not exist" CR,
		getCliNameByType(p_commandType),
		p_mo,
		p_cmdSubMoArg);
	return RC_NOT_FOUND_ERR;
}

rc_t cliCore::regCmdFlagArg(cliMainCmd_t p_commandType, const char* p_mo,
							const char* p_cmdSubMoArg, const char* p_flag,
							uint8_t p_firstArgPos, bool p_needsValue) {
	char cmdSubMoArgPrint[50];
	char cmdSubMoArg[50];
	if (p_cmdSubMoArg) {
		strcpy(cmdSubMoArgPrint, p_cmdSubMoArg);
		strcpy(cmdSubMoArg, p_cmdSubMoArg);
	}
	else {
		strcpy(cmdSubMoArgPrint, "-");
		strcpy(cmdSubMoArg, "");
	}
	LOG_VERBOSE("Registering flag: %s for Command: " \
			  "%s for sub-Mo: %s for all CLI contexts" CR,
		p_flag,
		getCliNameByType(p_commandType),
		cmdSubMoArgPrint);
	for (uint8_t i = 0; i < cliCmdTable->size(); i++) {
		if ((cliCmdTable->at(i)->cmdType == p_commandType) &&
			(!strcmp(cliCmdTable->at(i)->mo, p_mo)) &&
			(!strcmp(cliCmdTable->at(i)->subMo, cmdSubMoArg))) {
			if (!cliCmdTable->at(i)->commandFlags) {
				if ((cliCmdTable->at(i)->commandFlags =
					new (heap_caps_malloc(sizeof(cmdFlags), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) cmdFlags(p_firstArgPos)) == NULL) {
					panic("Could not create flags argument " \
						  "object");
					return RC_OUT_OF_MEM_ERR;
				}
			}
			rc_t rc = cliCmdTable->at(i)->commandFlags->add(p_flag, p_needsValue);
			if (rc){
				panic("Could not add flag %s to command %s - " \
					  "return code: %i", p_flag,
					   getCliNameByType(p_commandType), cmdSubMoArgPrint, rc);
				return RC_GEN_ERR;
			}
		}
	}
	return RC_OK;
}

rc_t cliCore::unRegCmdFlagArg(cliMainCmd_t p_commandType, const char* p_mo, 
							  const char* p_cmdSubMoArg, const char* p_flag) {
	char cmdSubMoArgPrint[50];
	char cmdSubMoArg[50];
	if (p_cmdSubMoArg) {
		strcpy(cmdSubMoArgPrint, p_cmdSubMoArg);
		strcpy(cmdSubMoArg, p_cmdSubMoArg);
	}
	else {
		strcpy(cmdSubMoArgPrint, "-");
		strcpy(cmdSubMoArg, "");
	}
	LOG_TERSE("Un-registering flag: %s for Command: " \
			  " %s %s for all CLI contexts" CR,
			   p_flag, getCliNameByType(p_commandType), cmdSubMoArgPrint);
	for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
		if ((cliCmdTable->at(i)->cmdType == p_commandType) &&
			(!strcmp(cliCmdTable->at(i)->mo, p_mo)) &&
			(!strcmp(cliCmdTable->at(i)->subMo, cmdSubMoArg))) {
			if (cliCmdTable->at(i)->commandFlags) {
				LOG_ERROR("No Flags entry found for Command: " \
						  "%s %s" CR, getCliNameByType(p_commandType), cmdSubMoArgPrint);
				return RC_NOT_FOUND_ERR;
			}
			cliCmdTable->at(i)->commandFlags->remove(p_flag);
			if (cliCmdTable->at(i)->commandFlags->size() == 0){
				delete cliCmdTable->at(i)->commandFlags;
				cliCmdTable->at(i)->commandFlags = NULL;
			}
			return RC_OK;
		}
	}
	LOG_ERROR("No Command table entry for Command: " \
			  "%s %s" CR, getCliNameByType(p_commandType), cmdSubMoArgPrint);
	return RC_NOT_FOUND_ERR;
}

rc_t cliCore::regCmdHelp(cliMainCmd_t p_commandType, const char* p_mo, 
						 const char* p_cmdSubMoArg, const char* p_helpText) {
	char cmdSubMoArgPrint[50];
	char cmdSubMoArg[50];
	if (p_cmdSubMoArg) {
		strcpy(cmdSubMoArgPrint, p_cmdSubMoArg);
		strcpy(cmdSubMoArg, p_cmdSubMoArg);
	}
	else {
		strcpy(cmdSubMoArgPrint, "-");
		strcpy(cmdSubMoArg, "");
	}
	LOG_VERBOSE("Registering help text for Command: %s MO: " \
			  "%s subMO: %s" CR, getCliNameByType(p_commandType), p_mo,
			   cmdSubMoArgPrint);
	for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
		if ((cliCmdTable->at(i)->cmdType == p_commandType) &&
			(!strcmp(cliCmdTable->at(i)->mo, p_mo)) &&
			(!strcmp(cliCmdTable->at(i)->subMo, cmdSubMoArg))) {
			cliCmdTable->at(i)->help = (char*)p_helpText;
			return RC_OK;
		}
	}
	LOG_ERROR("Registering help text for command: " \
			  "%s MO: %s, sub-MO : %s failed, not found" CR,
			   getCliNameByType(p_commandType), 
			   p_mo, cmdSubMoArgPrint);
	return RC_NOT_FOUND_ERR;
}

//rc_t cliCore::unRegCmdHelp(cliMainCmd_t p_commandType, const char* p_mo, const char* p_cmdSubMoArg) {}

//TR: provide help for commands alone: Eg "help get"
const char* cliCore::getHelpStr(char* p_helpStrBuff, cliMainCmd_t p_cmdType, const char* p_mo, const char* p_subMo, bool p_showCmdSyntax, bool p_showAvailableInMo) {
	strcpy(p_helpStrBuff, "");
	bool found = false;
	strcpy(p_helpStrBuff, "");
	uint16_t i;
	char* globalMoName = new (heap_caps_malloc(strlen(GLOBAL_MO_NAME) +1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(GLOBAL_MO_NAME) + 1];
	strcpy(globalMoName, GLOBAL_MO_NAME);
	char* commonMoName = new (heap_caps_malloc(strlen(COMMON_MO_NAME) + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[strlen(COMMON_MO_NAME) + 1];
	strcpy(commonMoName, COMMON_MO_NAME);
	char* contextItter[3] = { globalMoName, commonMoName, (char*)p_mo };
	for (uint8_t contextItterCnt = 0; p_mo? contextItterCnt < 3 : contextItterCnt < 2; contextItterCnt++) {
		for (i = 0; i < cliCmdTable->size(); i++) {
			if (p_cmdType == cliCmdTable->at(i)->cmdType &&
				!strcmp(cliCmdTable->at(i)->mo, contextItter[contextItterCnt]) &&
				!strcmp(cliCmdTable->at(i)->subMo, p_subMo)) {
				found = true;
				break;
			}
		}
		if (found)
			break;
	}
	delete globalMoName;
	delete commonMoName;
	if (found) {
		if (p_showCmdSyntax) {
			strcat(p_helpStrBuff, getCliNameByType(cliCmdTable->at(i)->cmdType));
			strcat(p_helpStrBuff, " ");
			strcat(p_helpStrBuff, p_subMo);
			if(cliCmdTable->at(i)->cmdType == SET_CLI_CMD) //Have the argument syntax registered instead
				strcat(p_helpStrBuff, " {value} ");			// TR: Create a framework wher the syntax and optionality of CLI commands is better represented in the registration, and a framework for better generic syntax control and visualization of the command syntax.
			if (cliCmdTable->at(i)->cmdType == HELP_CLI_CMD) //Have the argument syntax registered instead
				strcat(p_helpStrBuff, " [{command} [{sub-MO}]]"); TR: //MEDIUM: All help commands (eg help cli) shows arguments in their help text
			strcat(p_helpStrBuff, " ");
			char* flagsStr = new(heap_caps_malloc(sizeof(char[500]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) char[500];
			strcat(p_helpStrBuff, cliCmdTable->at(i)->commandFlags ? cliCmdTable->at(i)->commandFlags->getAllRegisteredStr(flagsStr) : "");
			delete flagsStr;
			strcat(p_helpStrBuff, ": \n\r");
		}
		strcat(p_helpStrBuff, cliCmdTable->at(i)->help);
		if (p_showAvailableInMo) {
			strcat(p_helpStrBuff, "Available in following MOs: ");
			found = false;
			
			for (uint16_t j = 0; j < cliCmdTable->size(); j++) {
				if (cliCmdTable->at(j)->cmdType == p_cmdType && !strcmp(cliCmdTable->at(j)->subMo, p_subMo)) {
					strcat(p_helpStrBuff, "{");
					strcat(p_helpStrBuff, cliCmdTable->at(j)->mo);
					strcat(p_helpStrBuff, "}, ");
					found = true;
				}
			}
			if (found)
				*(p_helpStrBuff + strlen(p_helpStrBuff) - 2) = '\0';
		}
		return p_helpStrBuff;
	}
	else
		return NULL;
}

void cliCore::onCliError(cmd_error* e) {
	CommandError cmdError(e); // Create wrapper object
	LOG_ERROR("CLI error:%s" CR, cmdError.toString().c_str());
	notAcceptedCliCommand(CLI_PARSE_ERR, cmdError.toString().c_str());
	if (cmdError.hasCommand())
		printCli("Did you mean \"%s\"?. Use \"help\" to show valid commands\a",
				  cmdError.getCommand().toString());
}

void cliCore::notAcceptedCliCommand(cmdErr_t p_cmdErr, const char* errStr, ...) {
	va_list args;
	va_start(args, errStr);
	int len = vsnprintf(NULL, 0, errStr, args);
	va_end(args);
	if (len < 0) return;
	// format message
	char* msg = (char*)heap_caps_malloc(sizeof(char[512]), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	va_start(args, errStr);
	vsnprintf(msg, len + 1, errStr, args);
	va_end(args);
	switch (p_cmdErr) {
	case CLI_PARSE_ERR:
		printCli("ERROR - CLI command parse error: %s\a", msg);
		return;
	case CLI_NOT_VALID_ARG_ERR:
		printCli("ERROR: - CLI argument error: %s\a", msg);
		return;
	case CLI_NOT_VALID_CMD_ERR:
		printCli("ERROR: - CLI command error: %s\a", msg);
		return;
	case CLI_GEN_ERR:
		printCli("ERROR: - CLI could not be executed: %s\a", msg);
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
	bool requiresSubMo = true;
	for (uint16_t cmdTableItter = 0; cmdTableItter < cliCmdTable->size(); cmdTableItter++) {
		if (!strcmp(cmd.getName().c_str(), getCliNameByType(cliCmdTable->at(cmdTableItter)->cmdType))) {
			if (!strcmp(cliCmdTable->at(cmdTableItter)->subMo, "")) {
				requiresSubMo = false;
			}
			break;
		}
	}
	for (uint16_t i = 0; i < cliCmdTable->size(); i++) {
		if (!strcmp(cmd.getName().c_str(), getCliNameByType(HELP_CLI_CMD))) {
			for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
				if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle ==
					currentParsingContext) {
					LOG_VERBOSE("CLI context %s-%i received a help " \
								"command" CR,
								cliCmdTable->at(i)->contextMap->at(j)->contextHandle->
									getCliContextDescriptor()->contextName,
								cliCmdTable->at(i)->contextMap->at(j)->contextHandle->
									getCliContextDescriptor()->contextIndex);
					cliCmdTable->at(i)->contextMap->at(j)->contextHandle->cliCmdTable->
						at(i)->contextMap->at(j)->
						cb(p_cmd, cliCmdTable->at(i)->contextMap->at(j)->contextHandle,
							cliCmdTable->at(i));
					return;
				}
			}
			LOG_INFO("Received a Help command which has not been "\
					 "registered" CR);
			printCli("Command not recognized for this CLI context\a");
		}
		if (strcmp(cmd.getName().c_str(), getCliNameByType(cliCmdTable->at(i)->cmdType)) || (strcmp(GLOBAL_MO_NAME, cliCmdTable->at(i)->mo) && strcmp(COMMON_MO_NAME, cliCmdTable->at(i)->mo) && strcmp(cliCmdTable->at(i)->mo, currentParsingContext->cliContextDescriptor.contextName))){ //FIX HERE
			continue;
		}
		if (requiresSubMo && strcmp(cmd.getArgument(0).getValue().c_str(), cliCmdTable->at(i)->subMo)){
			continue;
		}
		for (uint16_t j = 0; j < cliCmdTable->at(i)->contextMap->size(); j++) {
			if (cliCmdTable->at(i)->contextMap->at(j)->contextHandle == 
				currentParsingContext) {
				cliCmdTable->at(i)->contextMap->at(j)->contextHandle->cliCmdTable->
					at(i)->contextMap->at(j)->
					cb(p_cmd, cliCmdTable->at(i)->contextMap->at(j)->contextHandle,
						cliCmdTable->at(i));
				LOG_VERBOSE("CLI context %s-%i received a " \
							"CLI command %s %s" CR,
								cliCmdTable->at(i)->contextMap->at(j)->contextHandle->
								getCliContextDescriptor()->contextName,
								cliCmdTable->at(i)->contextMap->at(j)->contextHandle->
								getCliContextDescriptor()->contextIndex,
								cmd.getName().c_str(),
								cmd.getArgument(0).getValue().c_str());
				return;
			}
		}
		LOG_INFO("Received a CLI command which has " \
					"not been registered" CR);
		printCli("Command not recognized for this CLI context\a");
		return;
	}
	LOG_INFO_NOFMT("The Received CLI command does not exist" CR);
	printCli("Command does not exist\a");
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

const QList<cliCore*>* cliCore::getAllContexts(void) {
	return &allContexts;
}

void cliCore::setCurrentContext(cliCore* p_currentContext) {
	currentContext = p_currentContext;
}

QList<cliCmdTable_t*>* cliCore::getCliCommandTable(void){
	return cliCmdTable;
}

cli_context_descriptor_t* cliCore::getCliContextDescriptor(void) {
	return &cliContextDescriptor;
}

/*==============================================================================================================================================*/
/* END Class cliCore                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cmdFlags                                                                                                                              */
/* Purpose: See cliCore.h																														*/
/* Description: See cliCore.h																													*/
/* Methods: See cliCore.h																														*/
/* Data structures: See cliCore.h																												*/
/*==============================================================================================================================================*/
cmdFlags::cmdFlags(uint8_t p_firstValidPos) {
	firstValidPos = p_firstValidPos;
	strcpy(parseErrStr, "");
	flagList = new (heap_caps_malloc(sizeof(cmdFlag), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) QList<cmdFlag*>;
	foundFlagsList = new (heap_caps_malloc(sizeof(cmdFlag), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) QList<cmdFlag*>;
	if (!flagList || !foundFlagsList) {
		panic("Could not create flag lists");
		return;
	}
}

cmdFlags::~cmdFlags(void){
	while (flagList->size()) {
		delete flagList->back();
		flagList->pop_back();
	}
	delete flagList;
	while (foundFlagsList->size()) {
		delete foundFlagsList->back();
		foundFlagsList->pop_back();
	}
	delete foundFlagsList;
}

rc_t cmdFlags::add(const char* p_flag, bool p_needsValue) {
	for (uint16_t i = 0; i < flagList->size(); i++) {
		if (!strcmp(p_flag, flagList->at(i)->getName()))
			return RC_OK;
	}
	cmdFlag* newFlag = new (heap_caps_malloc(sizeof(cmdFlag), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) cmdFlag(p_flag, p_needsValue);
	if (!newFlag)
		return RC_OUT_OF_MEM_ERR;
	flagList->push_back(newFlag);
	return RC_OK;
}

rc_t cmdFlags::remove(const char* p_flag) {
	for (uint8_t i = 0; i < flagList->size(); i++) {
		if (!strcmp(flagList->at(i)->getName(), p_flag)) {
			delete flagList->at(i);
			flagList->clear(i);
			return RC_OK;
		}
	}
	return RC_NOT_FOUND_ERR;
}

uint8_t cmdFlags::size(void) {
	return flagList->size();
}

cmdFlag* cmdFlags::get(const char* p_flag) {
	for (uint8_t i = 0; i < flagList->size(); i++) {
		if (!strcmp(flagList->at(i)->getName(), p_flag)) {
			return flagList->at(i);
		}
	}
	return NULL;
}

rc_t cmdFlags::parse(Command p_command) {
	strcpy(parseErrStr, "");
	while (foundFlagsList->size() > 0) {
		foundFlagsList->pop_back();
	}
	for (uint8_t i = firstValidPos; i < p_command.countArgs(); i++) {
		if (isFlag(p_command.getArgument(i).getValue().c_str())) {
			bool found = false;
			for (uint8_t j = 0; j < flagList->size(); j++) {
				if (!strcmp(flagList->at(j)->getName(),
					isFlag(p_command.getArgument(i).getValue().c_str()))) {
					if (flagList->at(j)->needsValue()) {
						if (i >= p_command.countArgs() - 1) {
							sprintf(parseErrStr, "Flag %s requires a value, no value " \
									"found", isFlag(p_command.getArgument(i).getValue().
									c_str()));
							return RC_PARAMETERVALUE_ERR;
						}
						i++;
						if (check(p_command.getArgument(i).getValue().c_str())) {
							flagList->at(j)->setValue(p_command.getArgument(i).
								getValue().c_str());
						}
						else {
							sprintf(parseErrStr, "Flag %s requires a value, no valid " \
									"value found, %s is not a valid value",
									isFlag(p_command.getArgument(i).getValue().c_str()),
									p_command.getArgument(i).getValue().c_str());
							return RC_PARAMETERVALUE_ERR;
						}
					}
					foundFlagsList->push_back(flagList->at(j));
					found = true;
					break;
				}
			}
			if (!found) {
				sprintf(parseErrStr, "Flag %s is not a valid flag",
						isFlag(p_command.getArgument(i).getValue().c_str()));
				return RC_NOT_FOUND_ERR;
			}
		}
	}
	return RC_OK;
}

cmdFlag* cmdFlags::isPresent(const char* p_flag) {
	for (uint8_t i = 0; i < foundFlagsList->size(); i++) {
		if (!strcmp(foundFlagsList->at(i)->getName(), p_flag))
			return foundFlagsList->at(i);
	}
	return NULL;
}

QList<cmdFlag*>* cmdFlags::getAllRegistered(void) {
	return flagList;
}

const char* cmdFlags::getAllRegisteredStr(char* p_flags) {
	if(!flagList){
		strcpy(p_flags, "-");
		return p_flags;
	}
	strcpy(p_flags, "");
	for (uint8_t i = 0; i < flagList->size(); i++) {
		strcat(p_flags, "[-");
		strcat(p_flags, flagList->at(i)->getName());
		if (flagList->at(i)->needsValue())
			strcat(p_flags, "{value}");
		strcat(p_flags, "] ");
	}
	return p_flags;
}


QList<cmdFlag*>* cmdFlags::getAllPresent(void) {
	return foundFlagsList;
}

const char* cmdFlags::getAllPresentStr(char* p_flags) {
	strcpy(p_flags, "");
	for (uint8_t i = 0; i < foundFlagsList->size(); i++) {
		strcat(p_flags, foundFlagsList->at(i)->getName());
		if(i +1 < foundFlagsList->size())
			strcat(p_flags, ", ");
	}
	return p_flags;
}

const char* cmdFlags::getParsErrs(void) {
	if (strlen(parseErrStr) == 0)
		return NULL;
	else
		return parseErrStr;
}

const char* cmdFlags::isFlag(const char* p_flag) {
	if (p_flag[0] != '-' || p_flag[1] == '\n')
		return NULL;
	uint8_t i = 1;
	while (p_flag[i] != '\0') {
		if (!isAlphaNumeric(p_flag[i]))
			return NULL;
		i++;
	}
	return p_flag + 1;
}

const char* cmdFlags::check(const char* p_arg) {
	if (isFlag(p_arg))
		return NULL;
	else 
		return p_arg;
}
/*==============================================================================================================================================*/
/* END Class cmdFlags                                                                                                                           */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cmdFlag                                                                                                                               */
/* Purpose: See cliCore.h																														*/
/* Description: See cliCore.h																													*/
/* Methods: See cliCore.h																														*/
/* Data structures: See cliCore.h																												*/
/*==============================================================================================================================================*/

cmdFlag::cmdFlag(const char* p_flag, bool p_needsValue) {
	strcpy(name, p_flag);
	needsValueVar = p_needsValue;
	strcpy(value, "");
}

cmdFlag::~cmdFlag(void) {}

const char* cmdFlag::getName(void) {
	return name;
}

bool cmdFlag::needsValue(void) {
	return needsValueVar;
}

void cmdFlag::setValue(const char* p_value) {
	strcpy(value, p_value);
}

const char* cmdFlag::getValue(void) {
	return value;
}
/*==============================================================================================================================================*/
/* END Class cmdFlag                                                                                                                            */
/*==============================================================================================================================================*/
