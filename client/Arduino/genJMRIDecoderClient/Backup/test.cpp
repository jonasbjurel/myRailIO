/*==============================================================================================================================================*/
/* License                                                                                                                                      */
/*==============================================================================================================================================*/
// Copyright (c)2021 Jonas Bjurel (jonas.bjurel@hotmail.com)
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
/* Description                                                                                                                                  */
/*==============================================================================================================================================*/
//
//
/*==============================================================================================================================================*/
/* END Description                                                                                                                              */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* Include files                                                                                                                                */
/*==============================================================================================================================================*/
#include "genJMRIDecoderClient.h"

/*==============================================================================================================================================*/
/* END Include files*/
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Functions: Helper functions                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

char* createNcpystr(const char* src) {
  int length = strlen(src);
  char* dst = new char[length + 1];
  if (dst == NULL) {
    Log.error("createNcpystr: Failed to allocate memory from heap - rebooting..." CR);
    ESP.restart();
  }
  strcpy(dst, src);
  dst[length] = '\0';
  return dst;
}

char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings) {
  int resLen = 0;
  char* dst;
  
  for(uint8_t i=0; i<noOfSrcStrings; i++){
    resLen += strlen(srcStrings[i]);
  }
  dst = new char[resLen + 1];
  char* stringptr = dst;
  for(uint8_t i=0; i<noOfSrcStrings; i++){
    strcpy(stringptr, srcStrings[i]);
    stringptr += strlen(srcStrings[i]);
  }
  dst[resLen] = '\0';
  return dst;
}

uint8_t getTagTxt(tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len) {
  int i;
  while (xmlNode != NULL) {
    for (i=0; i<len; i++){
      if (!strcmp(tags[i], xmlNode->Name())){
        if (xmlNode->GetText() != NULL) {
          xmlTxtBuff[i] = new char[strlen(xmlNode->GetText())+1];
          strcpy(xmlTxtBuff[i], xmlNode->GetText());
        }
        break;
      }
    }
    xmlNode = xmlNode->NextSiblingElement();
  }
}

/*==============================================================================================================================================*/
/* END Helper functions                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: cpu                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

/*==============================================================================================================================================*/
/* END class cpu                                                                                                                                */
/*==============================================================================================================================================*/
cpu CPU;

SemaphoreHandle_t cpu::cpuPMLock;
uint64_t cpu::accBusyTime0;
uint64_t cpu::accTime0;
uint64_t cpu::accBusyTime1;
uint64_t cpu::accTime1;
uint64_t cpu::totalUsHistory0[CPU_HISTORY_SIZE];
uint64_t cpu::totalUsHistory1[CPU_HISTORY_SIZE];
uint64_t cpu::busyUsHistory0[CPU_HISTORY_SIZE];
uint64_t cpu::busyUsHistory1[CPU_HISTORY_SIZE];
uint8_t cpu::index;
uint32_t cpu::totIndex;
float cpu::maxCpuLoad0;
float cpu::maxCpuLoad1;


void cpu::init(void){
  cpuPMLock = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
                          cpuMeasurment0,             // Task function
                          "CPU-MEAS-0",               // Task function name reference
                          1024,                       // Stack size
                          NULL,                       // Parameter passing
                          tskIDLE_PRIORITY + 1,       // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          CORE_0);                    // Core [CORE_0 | CORE_1]

  xTaskCreatePinnedToCore(
                          cpuMeasurment1,             // Task function
                          "CPU-MEAS-1",               // Task function name reference
                          1024,                       // Stack size
                          NULL,                       // Parameter passing
                          tskIDLE_PRIORITY + 1,       // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          CORE_1);                    // Core [CORE_0 | CORE_1]
                          
  /*xTaskCreatePinnedToCore(                          // *** Debug: Load calibration task ***
                          load,                       // Task function
                          "LOAD",                     // Task function name reference
                          1024,                       // Stack size
                          NULL,                       // Parameter passing
                          4,                          // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          CORE_1);                    // Core [CORE_0 | CORE_1]*/
                          
  xTaskCreatePinnedToCore(
                          cpuPM,                      // Task function
                          "CPU-PM",                   // Task function name reference
                          6*1024,                     // Stack size
                          NULL,                       // Parameter passing
                          CPU_PM_POLL_PRIO,           // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          CPU_PM_CORE);               // Core [CORE_0 | CORE_1]
}

void cpu::load(void* dummy){                            //Calibrating load function - steps the system core load every 5 seconds
  while(true){
    for(uint16_t load = 0; load < 10000; load +=1000){
      Serial.print("Generating ");
      Serial.print((float)load/(1000+load)*100);
      Serial.println("% load"); 
      uint64_t _5Sec = esp_timer_get_time();
      while(esp_timer_get_time() - _5Sec < 5000000){
        uint64_t time = esp_timer_get_time();
        while(esp_timer_get_time() - time < load){
        }
        vTaskDelay(1);
      }
    }
  }
}

void cpu::cpuMeasurment0(void* dummy){                            // Core CPU load measuring function - disables built in idle function, and thus legacy watchdog and heap housekeeping.
  int64_t measureBusyTimeStart;
  int64_t measureTimeStart;
  accBusyTime0 = 0;
  accTime0 = 0;
  vTaskSuspendAll();                                              // Suspend the task scheduling for this core - to take full controll of the scheduling.
  while(true){
    measureTimeStart = esp_timer_get_time();                      // Start a > 100 mS time measuring loop, we cannot measure the time for each idividual loop for truncation error reasons
    while(esp_timer_get_time() - measureTimeStart < 100000){      // Wait 100 mS before granting Idle task runtime
      measureBusyTimeStart = esp_timer_get_time();                // Start of busy run-time measurement
      xTaskResumeAll();                                           // By shortly enabling scheduling we will trigger scheduling of queued higher priority tasks
      vTaskSuspendAll();                                          // After completion of the higher priority tasks we will disable scheduling again
      if(esp_timer_get_time() - measureBusyTimeStart > 10){       // If the time duration for enabled scheduling was more than 10 uS we assume that the time was spent for busy workloaads and account the time as busy
        accBusyTime0 += esp_timer_get_time() - measureBusyTimeStart;
      }
    }
    accTime0 += esp_timer_get_time() - measureTimeStart;          // Exclude idle task runtime or potentially highrér priority tasks runtime from the measurement samples
    xTaskResumeAll();
    vTaskDelay(1);                                                // The hope here is that the task queue is empty - Give IDLE task time to run
    vTaskSuspendAll();
  }
}

void cpu::cpuMeasurment1(void* dummy){                            // Core CPU load measuring function - disables built in idle function, and thus legacy watchdog and heap housekeeping.
  int64_t measureBusyTimeStart;
  int64_t measureTimeStart;
  accBusyTime1 = 0;
  accTime1 = 0;
  vTaskSuspendAll();                                              // Suspend the task scheduling for this core - to take full controll of the scheduling.
  while(true){
    measureTimeStart = esp_timer_get_time();                      // Start a > 100 mS time measuring loop, we cannot measure the time for each idividual loop for truncation error reasons
    while(esp_timer_get_time() - measureTimeStart < 100000){      // Wait 100 mS before granting Idle task runtime
      measureBusyTimeStart = esp_timer_get_time();                // Start of busy run-time measurement
      xTaskResumeAll();                                           // By shortly enabling scheduling we will trigger scheduling of queued higher priority tasks
      vTaskSuspendAll();                                          // After completion of the higher priority tasks we will disable scheduling again
      if(esp_timer_get_time() - measureBusyTimeStart > 10){       // If the time duration for enabled scheduling was more than 10 uS we assume that the time was spent for busy workloaads and account the time as busy
        accBusyTime1 += esp_timer_get_time() - measureBusyTimeStart;
      }
    }
    accTime1 += esp_timer_get_time() - measureTimeStart;          // Exclude idle task runtime or potentially highrér priority tasks runtime from the measurement samples
    xTaskResumeAll();
    vTaskDelay(1);                                                // The hope here is that the task queue is empty - Give IDLE task time to run
    vTaskSuspendAll();
  }
}

void cpu::cpuPM(void* dummy){
  float cpuLoad0 = 0;
  float cpuLoad1 = 0;
  uint64_t prevAccBusyTime0 = 0;
  uint64_t prevAccTime0 = 0;
  uint64_t prevAccBusyTime1 = 0;
  uint64_t prevAccTime1 = 0;
  
  xSemaphoreTake(cpuPMLock, portMAX_DELAY);
  index = 0;
  totIndex = 0;
  while(true){
    busyUsHistory0[index] = accBusyTime0; 
    totalUsHistory0[index] = accTime0; //FIX Should be total acc time
    busyUsHistory1[index] = accBusyTime1;
    totalUsHistory1[index] = accTime1; //FIX Should be total acc time
    xSemaphoreGive(cpuPMLock);
    cpuLoad0 = getAvgCpuLoadCore(CORE_0, 1);
    cpuLoad1 = getAvgCpuLoadCore(CORE_1, 1);
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    if(cpuLoad0>maxCpuLoad0){
      maxCpuLoad0 = cpuLoad0;
    }
    if(cpuLoad1>maxCpuLoad1){
      maxCpuLoad1 = cpuLoad1;
    }
    xSemaphoreGive(cpuPMLock);
    Serial.print("--- CPU load report for index: ");
    Serial.print(index);
    Serial.println(" ---");
    Serial.println("CPU,  1 Sec, 10 Sec, 1 Min, Max");
    Serial.print  ("0,    ");
    Serial.print(getAvgCpuLoadCore(CORE_0, 1));
    Serial.print(",   ");
    Serial.print(getAvgCpuLoadCore(CORE_0, 10));
    Serial.print(",   ");
    Serial.print(getAvgCpuLoadCore(CORE_0, 60));
    Serial.print(", ");
    Serial.println(maxCpuLoad0);
    Serial.print  ("1,    ");
    Serial.print(getAvgCpuLoadCore(CORE_1, 1));
    Serial.print(",   ");
    Serial.print(getAvgCpuLoadCore(CORE_1, 10));
    Serial.print(",   ");
    Serial.print(getAvgCpuLoadCore(CORE_1, 60));
    Serial.print(",   ");
    Serial.println(maxCpuLoad1);
    Serial.println("--- END CPU load report ---");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    if(++index == CPU_HISTORY_SIZE){
      index = 0;
    }
    totIndex++;
  }
}

float cpu::getAvgCpuLoadCore(uint8_t p_core, uint8_t p_period){
  int8_t tmp_startIndex;
  if(p_period > CPU_HISTORY_SIZE){
    Log.error("cpu::getAvgCpuLoadCore: invalid average period provided" CR);
    return RC_GEN_ERR;
  }
  xSemaphoreTake(cpuPMLock, portMAX_DELAY);
  if(totIndex < p_period){
    xSemaphoreGive(cpuPMLock);
    //Log.verbose("cpu::getAvgCpuLoadCore: Not enough samples to provide CPU load average" CR);
    return 0;
  }
  if(p_core == CORE_0){
    tmp_startIndex = index - p_period;
    if(tmp_startIndex<0){
      tmp_startIndex = CPU_HISTORY_SIZE + tmp_startIndex;
    }
    if(totalUsHistory0[index]-totalUsHistory0[tmp_startIndex] == 0){
      xSemaphoreGive(cpuPMLock);
      return 100;
    }
    float tmp_AvgLoad0 = ((float)(busyUsHistory0[index]-busyUsHistory0[tmp_startIndex])/(totalUsHistory0[index]-totalUsHistory0[tmp_startIndex]))*100;
    xSemaphoreGive(cpuPMLock);
    return tmp_AvgLoad0;
  }
  else if(p_core == CORE_1){
    tmp_startIndex = index - p_period;
    if(tmp_startIndex<0){
      tmp_startIndex = CPU_HISTORY_SIZE + tmp_startIndex;
    }
    if(totalUsHistory1[index]-totalUsHistory1[tmp_startIndex] == 0){
      xSemaphoreGive(cpuPMLock);
      return 100;
    }
    float tmp_AvgLoad1 = ((float)(busyUsHistory1[index]-busyUsHistory1[tmp_startIndex])/(totalUsHistory1[index]-totalUsHistory1[tmp_startIndex]))*100;
    xSemaphoreGive(cpuPMLock);
    return tmp_AvgLoad1;
  }
  else{
    xSemaphoreGive(cpuPMLock);
    Log.error("cpu::get1SecCpuLoadCore: Invalid core provided, continuing ..." CR);
    return RC_GEN_ERR;
  }
}

uint8_t cpu::getCpuMaxLoadCore(uint8_t p_core){
  uint8_t tmp_maxCpuLoad;
  if(p_core == CORE_0){
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    tmp_maxCpuLoad = maxCpuLoad0;
    xSemaphoreGive(cpuPMLock);
    return tmp_maxCpuLoad;
  }
  else if(p_core == CORE_1){
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    tmp_maxCpuLoad = maxCpuLoad1;
    xSemaphoreGive(cpuPMLock);
    return tmp_maxCpuLoad;
  }
  else{
    return RC_GEN_ERR;
  }
}

uint8_t cpu::clearCpuMaxLoadCore(uint8_t p_core){
  if(p_core == CORE_0){
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    maxCpuLoad0 = 0;
    xSemaphoreGive(cpuPMLock);
    return RC_OK;
  }
  else if(p_core == CORE_1){
    xSemaphoreTake(cpuPMLock, portMAX_DELAY);
    maxCpuLoad1 = 0;
    xSemaphoreGive(cpuPMLock);
    return RC_OK;
  }
  else{
    return RC_GEN_ERR;
  }
}

void cpu::getTaskInfoAll(char* p_taskInfoTxt){ //TODO
}

uint8_t cpu::getTaskInfoByTask(char* p_task, char* p_taskInfoTxt){ //TODO
}

uint32_t cpu::getHeapMemInfoAll(void){ //Migrate function from CLI class to here
}

uint32_t cpu::getHeapMemInfoMaxAll(void){ //Migrate function from CLI class to here
}

uint32_t cpu::getHeapMemTrend10minAll(void){ //TODO
}

uint32_t cpu::getStackByTask(char* p_task){ //TODO
  
}

uint32_t cpu::getMaxStackByTask(char* p_task){ //TODO
//  return uxTaskGetStackHighWaterMark(xTaskGetHandle(p_task));
}

//applicatiin specific watchdog - TODO


/*==============================================================================================================================================*/
/* Class: cli                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
//The basic CLI conventions:
//"menue/undermenue/../.." - goes into the child cli context
//exit, ../ - pops the context one level; ../../.. pops the context several levels...
//quit - ends the cli session

//get param [flags, qualifiers] - prints states and stats

//set param [flags, qualifiers] - sets, clears, modifies states, memory, and configuration into volitile memory, some may be persistant and will
//be asked to be confirmed.  
/* root: get[help, all, lightgroupschannels, flashes, turnoutgroupschannels, sensorgroups]
/* General cli style
 *  help:
 *  set:
 *  get:
 *  show:
 *  exec:
 *  exit:
 *        update:
 *        wifi: get[opstate, ssid, snr, ipaddr, ipmask, ipgw, dns, ntp, rsyslog], set[wps, ssid, encr, appwd, ipaddr, ipmask, ipgw, dns, ntp, rsyslog]
 *        mqtt: get[opState, broker, lwt, stats, subscribers], set[broker, qos, retain, lwt, subscriber, clearstats]
 *        wdt: get[help, subscribers, stats], set[modifysub, clearstats]
 *        debug: get[help, tasks, memstats, peek, restartdump, locks], set[opstate, releaselock, poke, watchpoint, halt]
 *        log: get[help, log], set[logverbosity, logtarget]
 *        sysstat: get[help, app, cpu, task, memstats], set[clearall]
 *        
 *    flash-n: get[help, opstate, stats, subscribers], set[clearstats, subscriber]
 *        
 *    lightgroupschannel-n: get[help, opstate, lightgroups, stripstat, ], set[clearstripstat]
 *    .
 *    .
 *        lightgroup-m
 *        .
 *        .
 *    turnouts
 */
/*==============================================================================================================================================*/

  decoderCli CLI;
  QList<cliContext_t*>* decoderCli::cliContextList;
  cliContext_t* decoderCli::currentCliContext;
  cliContext_t* decoderCli::newCliContext;
  SimpleCLI* decoderCli::cli;
  ESPTelnet decoderCli::telnet;
  cliContext_t* decoderCli::rootCliContext;
  uint8_t decoderCli::connections;
  Command decoderCli::help;
  Command decoderCli::set;
  Command decoderCli::get;
  Command decoderCli::exec;
  
void decoderCli::init(void){
  Log.notice("decoderCli::init: Starting CLI" CR);
  //SimpleCLI cli;
  //ESPTelnet telnet;
  cliContextList = new QList<cliContext_t*>;
  rootCliContext = new cliContext_t;
  rootCliContext->cliContext = (char*)"root";
  rootCliContext->cliContextInitCb = rootCliContextInit;
  rootCliContext->cliContextCb = onRootCliCmd;
  registerCliContext(rootCliContext);
  newCliContext = getCliContext((char*)"root");
  setCliContext(newCliContext);
  telnet.onConnect(onTelnetConnect);
  telnet.onConnectionAttempt(onTelnetConnectionAttempt);
  telnet.onReconnect(onTelnetReconnect);
  telnet.onDisconnect(onTelnetDisconnect);
  connections = 0;
  
  // passing a lambda function
  telnet.onInputReceived([](String str) {
    // checks for a certain command
      decoderCli::onTelnetString(str);
  });
  if (telnet.begin()) {
    Log.notice("decoderCli::init: CLI started" CR);
    xTaskCreatePinnedToCore(
                            decoderCli::cliPoll,        // Task function
                            "CLI polling",              // Task function name reference
                            6*1024,                     // Stack size
                            NULL,                       // Parameter passing
                            CLI_POLL_PRIO,              // Priority 0-24, higher is more
                            NULL,                       // Task handle
                            CLI_POLL_CORE);             // Core [CORE_0 | CORE_1]
  }
  else{
    Log.error("decoderCli::init: Failed to start CLI" CR);
  }
}

void decoderCli::onTelnetConnect(String ip){
  connections++;
  Log.notice("decoderCli::onTelnetConnect: CLI connected from: %s" CR, ip);
  telnet.print("\r\nWelcome to JMRI generic decoder CLI - version: \r\nConnected from: ");
  telnet.print(ip);
  telnet.print("\r\n\nType help for Help\r\n\n");
  printToCli("");
}

void decoderCli::onTelnetDisconnect(String ip){
  connections--;
  Log.notice("decoderCli::onTelnetDisconnect: CLI disconnected from: %s" CR, ip);
}

void decoderCli::onTelnetReconnect(String ip){
  Log.notice("decoderCli::onTelnetReconnect: CLI reconnected from: %s" CR, ip);
}

void decoderCli::onTelnetConnectionAttempt(String ip){
  Log.notice("decoderCli::onTelnetConnectionAttempt: CLI connection failed from: %s" CR, ip);
}

void decoderCli::onTelnetString(String p_cmd){
  if(currentCliContext != newCliContext){
    setCliContext(newCliContext);
  }
  cli->parse(p_cmd);
  if (cli->errored()) {
    CommandError cmdError = cli->getError();
    telnet.print("ERROR: ");
    telnet.print(cmdError.toString());
    if (cmdError.hasCommand()) {
      telnet.print("\r\nDid you mean \"");
      telnet.print(cmdError.getCommand().toString());
      telnet.print("\"?\r\n");
    }
  }
}

void decoderCli::registerCliContext(cliContext_t* p_context){
  cliContextList->push_back(p_context);
}

void decoderCli::onCliCommand(cmd* p_cmd){
  if(contextCommonCliCmd(p_cmd) == CLI_PARSE_CONTINUE){
    if(currentCliContext->cliContextCb(p_cmd) == CLI_PARSE_CONTINUE){
      printToCli("Failed to parse CLI command or its arguments");
    }
  } 
}

cliContext_t* decoderCli::getCliContext(char* p_context){
  bool found = false;
  for(uint8_t i= 0; i<cliContextList->size(); i++){
    if(!strcmp(cliContextList->at(i)->cliContext, p_context)){
      found = true;
      return cliContextList->at(i);
    }
  }
  return NULL;
}

void decoderCli::setCliContext(cliContext_t* p_context){
  resetCliContext();
  commonCliContextInit();
  p_context->cliContextInitCb();
  currentCliContext = p_context;
}

void decoderCli::resetCliContext(void){
  if(cli != NULL){
    delete cli;
  }
  cli = new SimpleCLI;

  help = cli->addCmd("help", onCliCommand);
  set = cli->addCmd("set", onCliCommand);
  get = cli->addCmd("get", onCliCommand);
  exec = cli->addCmd("exec", onCliCommand);
}

void decoderCli::printToCli(String p_output, bool partial){
  telnet.print(p_output);
  if(!partial){
    telnet.print("\r\n\n");
    telnet.print(">> ");
  }
}

void decoderCli::commonCliContextInit(void){
  set.addArg("context");
  get.addFlagArgument("context");
  get.addFlagArgument("uptime");
  get.addFlagArgument("time");
  get.addFlagArgument("cpu");
  get.addFlagArgument("mem/ory");
  exec.addFlagArgument("reboot");
}

uint8_t decoderCli::contextCommonCliCmd(cmd* p_cmd){
  Command cmd(p_cmd);
  String command;
  printToCli("Command accepted - parsing..\n\r", true);
  command = cmd.getName();
  if(command == "help"){
    printToCli("This is common help text");
    return CLI_PARSE_CONTINUE;
  }
  if(command == "set"){
    Argument setContextTopic = cmd.getArgument("context");
    String argBuff;
    if(argBuff = setContextTopic.getValue()){
      if(newCliContext = getCliContext((char*)argBuff.c_str())){
        printToCli("Context is now: ", true);
        printToCli(newCliContext->cliContext);
      }
      else{
        telnet.print("ERROR - context: ");
        telnet.print(argBuff);
        telnet.print(" does not exist.\n");
      }
    }
  }

  if(command == "get"){
    Argument getContextTopic = cmd.getArgument("context");
    Argument getUptimeTopic = cmd.getArgument("uptime");
    Argument getCpuTopic = cmd.getArgument("cpu");
    Argument getTimeTopic = cmd.getArgument("time");
    Argument getMemTopic = cmd.getArgument("mem/ory");

    if(getContextTopic.isSet()){
      printToCli("Current context: ", true);
      printToCli(currentCliContext->cliContext);
      return CLI_PARSE_STOP;
    }

    if(getUptimeTopic.isSet()){
      printToCli("System uptime: ", true);
      char uptime[10];
      printToCli(itoa(esp_timer_get_time()/1000000, uptime, 10), true);
      printToCli(" Seconds");
      return CLI_PARSE_STOP;
    }

    //get -cpu cli command
    if(getCpuTopic.isSet()){
      char cpuLoad[10];
      printToCli("CPU load report (max CPU load will be reset):\r\n", true);
      printToCli("CPU#,  1 Sec[%], 10 Sec[%], 1 Min[%], Max[%]\r\n", true);
      printToCli("0,     ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_0, 1), cpuLoad, 10), true);
      printToCli(",         ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_0, 10), cpuLoad, 10), true);
      printToCli(",         ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_0, 60), cpuLoad, 10), true);
      printToCli(",       ", true);
      printToCli(itoa((uint8_t)CPU.getCpuMaxLoadCore(CORE_0), cpuLoad, 10), true);
      printToCli("\r\n1,     ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_1, 1), cpuLoad, 10), true);
      printToCli(",         ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_1, 10), cpuLoad, 10), true);
      printToCli(",         ", true);
      printToCli(itoa((uint8_t)CPU.getAvgCpuLoadCore(CORE_1, 60), cpuLoad, 10), true);
      printToCli(",       ", true);
      printToCli(itoa((uint8_t)CPU.getCpuMaxLoadCore(CORE_1), cpuLoad, 10));
      CPU.clearCpuMaxLoadCore(CORE_0);
      CPU.clearCpuMaxLoadCore(CORE_1);
      return CLI_PARSE_STOP;
    }   

    if(getTimeTopic.isSet()){
      time_t rawtime;
      struct tm * timeinfo;
      time (&rawtime);
      timeinfo = localtime (&rawtime);
      printToCli ("Local time: ", true);
      printToCli (asctime(timeinfo));
      return CLI_PARSE_STOP;
    }
    if(getMemTopic.isSet()){
      multi_heap_info_t heapInfo;
      heap_caps_get_info(&heapInfo, MALLOC_CAP_8BIT);
      char outputBuff[30];
      printToCli("Allocated bytes: ", true);
      printToCli(itoa(heapInfo.total_allocated_bytes, outputBuff, 10), true);
      printToCli("  Historically minimum free bytes: ", true);
      printToCli(itoa(heapInfo.minimum_free_bytes, outputBuff, 10), true);
      printToCli("  Free bytes: ", true);
      printToCli(itoa(heapInfo.total_free_bytes, outputBuff, 10), true);
      printToCli("\r\n", true);
      printToCli("Allocated blocks: ", true);
      printToCli(itoa(heapInfo.allocated_blocks, outputBuff, 10), true);
      printToCli("  Free blocks: ", true);
      printToCli(itoa(heapInfo.free_blocks, outputBuff, 10));
      return CLI_PARSE_STOP;
    }
    return CLI_PARSE_CONTINUE;
  }
  if(command == "exec"){
    Argument rebootTopic = cmd.getArgument("reboot");
    if(rebootTopic.isSet()){
      ESP.restart();
      return CLI_PARSE_STOP;
    }
  }
}

void decoderCli::rootCliContextInit(void){
}
  
uint8_t decoderCli::onRootCliCmd(cmd* p_cmd){
}

void decoderCli::cliPoll(void* dummy){
  Log.notice("decoderCli::cliPoll: CLI polling started" CR);
  while(true){
    telnet.loop();
    if(connections){
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    else{
      vTaskDelay(1000 / portTICK_PERIOD_MS);      
    }
  }
}

/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
wdt::wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction){
  wdtData = new wdt_t;
  wdtData->wdtTimeout = p_wdtTimeout*1000;
  strcpy(wdtData->wdtDescription, p_wdtDescription);
  wdtData->wdtAction = p_wdtAction;
  wdtTimer_args.arg =this;
  wdtTimer_args.callback = reinterpret_cast<esp_timer_cb_t>(&wdt::kickHelper);
  wdtTimer_args.dispatch_method = ESP_TIMER_TASK;
  wdtTimer_args.name = "FlashTimer";
  esp_timer_create(&wdtTimer_args, &wdtData->timerHandle);
  esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
}

wdt::~wdt(void){
  esp_timer_stop(wdtData->timerHandle);
  esp_timer_delete(wdtData->timerHandle);
  delete wdtData;
  return;
}

void wdt::feed(void){
  esp_timer_stop(wdtData->timerHandle);
  esp_timer_start_once(wdtData->timerHandle, wdtData->wdtTimeout);
  return;
}

void wdt::kickHelper(wdt* p_wdtObject){
  p_wdtObject->kick();
}
void wdt::kick(void){ //FIX
  return;
}
/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: networking                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
String networking::ssid = "";
uint8_t networking::channel = 255;
long  networking::rssi = 0;
String networking::mac = "";
IPAddress networking::ipaddr = IPAddress(0, 0, 0, 0);
IPAddress networking::ipmask = IPAddress(0, 0, 0, 0);
IPAddress networking::gateway = IPAddress(0, 0, 0, 0);
IPAddress networking::dns = IPAddress(0, 0, 0, 0);
IPAddress networking::ntp = IPAddress(0, 0, 0, 0);
String networking::hostname = "MyHost";
esp_wps_config_t networking::config;
uint8_t networking::opState = OP_INIT;

void networking::start(void){
  pinMode(WPS_PIN, INPUT);
  WiFi.onEvent(WiFiEvent);
  WiFi.mode(WIFI_MODE_STA);
  mac = WiFi.macAddress();
  Log.notice("networking::start: MAC Address: %s" CR, (char*)&mac[0]);
  Serial.println("MAC Address: " + mac);
  uint8_t wpsPBT = 0;
  while (digitalRead(WPS_PIN)){
    wpsPBT++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  if(wpsPBT == 0){
    WiFi.begin();
  } else if (wpsPBT < 10){
    Log.notice("networking::start: WPS button was hold down for less thann 10 seconds - connecting to a new WIFI network, push the WPS button on your WIFI router..." CR);
    Serial.println("WPS button was hold down for less thann 10 seconds - connecting to a new WIFI network, push the WPS button on your WIFI router..." CR);
    networking::wpsStart();
  } else {
  Log.notice("networking::start: WPS button was hold down for more that 10 seconds - forgetting previous learnd WIFI networks and rebooting..." CR);
  Serial.println("WPS button was hold down for more that 10 seconds - forgetting previous learnd WIFI networks and rebooting...");
  //WiFi.disconnect(eraseap = true);
  WiFi.disconnect();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  ESP.restart();
  }
  return;
}

void networking::regCallback(const netwCallback_t p_callback){
  callback = p_callback;
  return;
}

void networking::wpsStart(){
  Log.notice("networking::wpsStart: Starting WPS" CR);
  Serial.println("Starting WPS");
  networking::wpsInitConfig();
  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
  return;
}

void networking::wpsInitConfig(){
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  config.wps_type = ESP_WPS_MODE;
  strcpy(config.factory_info.manufacturer, ESP_MANUFACTURER);
  strcpy(config.factory_info.model_number, ESP_MODEL_NUMBER);
  strcpy(config.factory_info.model_name, ESP_MODEL_NAME);
  strcpy(config.factory_info.device_name, ESP_DEVICE_NAME);
  return;
}

String networking::wpspin2string(uint8_t a[]){
  char wps_pin[9];
  for(int i=0; i<8; i++){
    wps_pin[i] = a[i];
  }
  wps_pin[8] = '\0';
  return (String)wps_pin;
}

void networking::WiFiEvent(WiFiEvent_t event, system_event_info_t info){
  switch(event){
    if(callback != NULL) {
      callback(event);
    }
    case SYSTEM_EVENT_STA_START:
      Log.notice("networking::WiFiEvent: Station Mode Started" CR);
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      opState = OP_CONNECTED;
      ssid = WiFi.SSID();
      channel = WiFi.channel();
      rssi = WiFi.RSSI();
      Log.notice("networking::WiFiEvent: Connected to: %s, channel: %d, RSSSI: %d" CR, (char*)&ssid[0], channel, rssi);
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      opState = OP_WORKING;
      ipaddr = WiFi.localIP();
      ipmask = WiFi.subnetMask();
      gateway = WiFi.gatewayIP();
      dns = WiFi.dnsIP();
      ntp = dns;
      hostname = "My host";
      Log.notice("networking::WiFiEvent: Got IP-address: %i.%i.%i.%i, Mask: %i.%i.%i.%i, \nGateway: %i.%i.%i.%i, DNS: %i.%i.%i.%i, \nNTP: %i.%i.%i.%i, Hostname: %s" CR, 
                  ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3], ipmask[0], ipmask[1], ipmask[2], ipmask[3], gateway[0], gateway[1], gateway[2], gateway[3],
                  dns[0], dns[1], dns[2], dns[3], ntp[0], ntp[1], ntp[2], ntp[3], hostname);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      opState = OP_FAIL;
      Log.error("networking::WiFiEvent: Lost IP - reconnecting" CR);
      ssid = "";
      channel = 255;
      rssi = 0;
      ipaddr = IPAddress(0, 0, 0, 0);
      gateway = IPAddress(0, 0, 0, 0);
      dns = IPAddress(0, 0, 0, 0);
      ntp = IPAddress(0, 0, 0, 0);
      hostname = "";
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      networking::opState = OP_FAIL;
      Log.notice("networking::WiFiEvent: Disconnected from station, attempting reconnection" CR);
      ssid = "";
      channel = 255;
      rssi = 0;
      ipaddr = IPAddress(0, 0, 0, 0);
      gateway = IPAddress(0, 0, 0, 0);
      dns = IPAddress(0, 0, 0, 0);
      ntp = IPAddress(0, 0, 0, 0);
      hostname = "";
      WiFi.reconnect();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      opState = OP_CONFIG;
      Log.notice("networking::WiFiEvent: WPS Successful, stopping WPS and connecting to: %s" CR, String(WiFi.SSID()));
      esp_wifi_wps_disable();
      delay(10);
      WiFi.begin();
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      networking::opState = OP_FAIL;
      Log.error("networking::WiFiEvent: WPS Failed, retrying" CR);
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      opState = OP_FAIL;
      Log.error("networking::WiFiEvent: WPS Timeout, retrying" CR);
      esp_wifi_wps_disable();
      esp_wifi_wps_enable(&config);
      esp_wifi_wps_start(0);
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WPS_PIN = " + networking::wpspin2string(info.sta_er_pin.pin_code));
      break;
    default:
      break;
  }
  return;
}

const char* networking::getSsid(void){
  return ssid.c_str();
}

uint8_t networking::getChannel(void){
  return channel;
}

long networking::getRssi(void){
  return rssi;
}

const char* networking::getMac(void){
  return mac.c_str();
}

IPAddress networking::getIpaddr(void){
  return ipaddr;
}

IPAddress networking::getIpmask(void){
  return ipmask;
}

IPAddress networking::getGateway(void){
  return gateway;
}

IPAddress networking::getDns(void){
  return dns;
}

IPAddress networking::getNtp(void){
  return ntp;
}

const char* networking::getHostname(void){
  return hostname.c_str();
}

uint8_t networking::getOpState(void){
  return opState;
}


/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t mqtt::opState = OP_CREATED;
WiFiClient mqtt::espClient;
PubSubClient mqtt::mqttClient(espClient);
SemaphoreHandle_t mqtt::mqttLock;
char* mqtt::broker;
uint16_t mqtt::port;
char* mqtt::uri;
char* mqtt::user;
char* mqtt::pass;
char* mqtt::clientId;
uint8_t mqtt::defaultQoS;
uint8_t mqtt::keepAlive;
char* mqtt::opStateTopic;
char* mqtt::upPayload;
char* mqtt::downPayload;
bool mqtt::opStateTopicSet;
char* mqtt::mqttPingUpstreamTopic;
uint8_t mqtt::missedPings;
int mqtt::mqttStatus;
uint8_t mqtt::defaultQos;
bool mqtt::defaultRetain;
float mqtt::pingPeriod;
bool mqtt::discovered;
uint32_t mqtt::overRuns;
uint32_t mqtt::maxLatency;
uint16_t mqtt::avgSamples;
uint32_t* mqtt::latencyVect;
QList<mqttTopic_t*> mqtt::mqttTopics;
mqttStatusCallback_t mqtt::statusCallback;



uint8_t mqtt::init(const char* p_broker, uint16_t p_port, const char* p_user, const char* p_pass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, bool p_defaultRetain){
  if(opState != OP_CREATED) {
      Log.fatal("mqtt::init: opState is not OP_CREATED - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
  }
  Log.notice("mqtt::init: Initializing and starting the MQTT client" CR);
  QList<mqttTopic_t*> mqttTopics;
  missedPings = 0;
  opStateTopicSet = false;
  discovered = false;
  mqttLock = xSemaphoreCreateMutex();
  //WiFiClient espClient;
  //PubSubClient mqttClient(espClient);
  broker = createNcpystr(p_broker);
  port = p_port;
  user = createNcpystr(p_user);
  pass = createNcpystr(p_pass);
  clientId = createNcpystr(p_clientId);
  defaultQoS = p_defaultQoS;
  keepAlive = p_keepAlive;
  defaultRetain = p_defaultRetain;
  avgSamples =int(POLL_MQTT_LATENCY_AVG_TIME * 1000 / MQTT_POLL_PERIOD_MS);
  latencyVect = new uint32_t[avgSamples];
  mqttClient.setServer(broker, port); //Shouldnt it be MQTT.setServer(broker, port);
  mqttClient.setKeepAlive(keepAlive);
  mqttStatus = mqttClient.state();
  if (!mqttClient.setBufferSize(MQTT_BUFF_SIZE)) {
    Log.fatal("mqtt::init: Could not allocate MQTT buffers, with buffer size: %d - rebooting..." CR, MQTT_BUFF_SIZE);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  mqttClient.setCallback(onMqttMsg);
  mqttClient.connect(clientId, //Should we let poll establish the initial connection? A little bit more thinking?
                     user, 
                     pass);

  //mqttStatus = mqttClient.state();
  if (statusCallback != NULL){
    statusCallback(mqttStatus);
  }
  Log.notice("mqtt::init: Spawning MQTT poll task" CR);
  xTaskCreatePinnedToCore(
                          poll,                       // Task function
                          "MQTT polling",             // Task function name reference
                          6*1024,                     // Stack size
                          NULL,                       // Parameter passing
                          MQTT_POLL_PRIO,             // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          MQTT_POLL_CORE);                    // Core [CORE_0 | CORE_1]

  while(true) {
    Log.notice("mqtt::init: Waiting for opState to become OP_WORKING..." CR);
    if(opState == OP_WORKING) {
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  Log.notice("mqtt::init: Starting discovery process..." CR);
  discover();
  int tries = 0;
  while(true) {
    bool tmpDiscovered;
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    tmpDiscovered = discovered;
    xSemaphoreGive(mqttLock);
    if(tmpDiscovered){
      break;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (tries++ >= 100){
      Log.fatal("mqtt::init: Discovery process failed, no discovery response was received - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  Log.notice("mqtt::init: Decoder successfully discovered; URI: %s" CR, uri);
  if(statusCallback != NULL){
    statusCallback(mqttStatus);
  }
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  opState = OP_INIT;
  xSemaphoreGive(mqttLock);
  return RC_OK;
}

void mqtt::discover(void){
  const char* mqtt_discovery_response_topic = MQTT_DISCOVERY_RESPONSE_TOPIC;
  subscribeTopic(mqtt_discovery_response_topic, onDiscoverResponse, NULL);
  const char* mqtt_discovery_request_topic = MQTT_DISCOVERY_REQUEST_TOPIC;
  const char* discovery_req = DISCOVERY_REQ;
  sendMsg(mqtt_discovery_request_topic, discovery_req, false);
}

void mqtt::onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  if(discovered){
    xSemaphoreGive(mqttLock);
    Log.notice("mqtt::discover: Discovery response receied several times - doing nothing ..." CR);
    return;
  }
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::discover: Got a discover response, parsing and validating it..." CR);
  tinyxml2::XMLDocument* xmlDiscoveryDoc;
  xmlDiscoveryDoc = new tinyxml2::XMLDocument;
  heap_caps_check_integrity_all(true); //Test code
  tinyxml2::XMLElement* xmlDiscoveryElement;
  Log.notice("mqtt::onDiscoverResponse: Parsing discovery response: %s" CR, p_payload);
  if (xmlDiscoveryDoc->Parse(p_payload) || (xmlDiscoveryElement=xmlDiscoveryDoc->FirstChildElement("DiscoveryResponse")) == NULL) {
    Log.fatal("mqtt::discover: Discovery response parsing or validation failed - Rebooting..." CR);
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    opState = OP_FAIL;
    xSemaphoreGive(mqttLock);
    //failsafe();
    ESP.restart();
    delete xmlDiscoveryDoc;
    return;
  }
  xmlDiscoveryElement = xmlDiscoveryElement->FirstChildElement("Decoder");
  bool found = false;
  while(xmlDiscoveryElement != NULL) {
    if(!strcmp(xmlDiscoveryElement->Value(), "Decoder") && xmlDiscoveryElement->FirstChildElement("MAC") != NULL && xmlDiscoveryElement->FirstChildElement("URI") != NULL && xmlDiscoveryElement->FirstChildElement("URI")->GetText() != NULL && !strcmp(xmlDiscoveryElement->FirstChildElement("MAC")->GetText(), networking::getMac())){
      uri = new char[strlen(xmlDiscoveryElement->FirstChildElement("URI")->GetText())+1];
      strcpy(uri, xmlDiscoveryElement->FirstChildElement("URI")->GetText());
      found = true;
      break;
    }
    xmlDiscoveryElement = xmlDiscoveryElement->NextSiblingElement();
  }
  if(!found){
    Log.fatal("mqtt::discover: Discovery response doesn't provide any information about this decoder (MAC) - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    delete xmlDiscoveryDoc;
    return;
  }
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  discovered = true;
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::discover: Discovery response successful, set URI to %s for this decoders MAC %s" CR, uri, networking::getMac());
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  opState = OP_CONFIG;
  xSemaphoreGive(mqttLock);
  delete xmlDiscoveryDoc;
  return;
}

uint8_t mqtt::setOpstateTopic(char* p_opStateTopic, char* p_upPayload, char* p_downPayload){
  Log.notice("mqtt::setOpstateTopic: Setting opState topic and payload for mqtt up/down/lw" CR);
  opStateTopic = createNcpystr(p_opStateTopic);
  upPayload = createNcpystr(p_upPayload);
  downPayload = createNcpystr(p_downPayload);
  opStateTopicSet = true;
  mqttClient.disconnect();
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  opState = OP_FAIL;
  xSemaphoreGive(mqttLock); 
  opState = OP_FAIL;
  return RC_OK;
}

uint8_t mqtt::regStatusCallback(const mqttStatusCallback_t p_statusCallback){
  xSemaphoreTake(mqttLock, portMAX_DELAY);

  statusCallback = p_statusCallback;
  xSemaphoreGive(mqttLock); 
  return RC_OK;
}

void mqtt::setPingPeriod(float p_pingPeriod){
  Log.notice("mqtt::setPingPerio: Setting MQTT ping period to %d" CR, p_pingPeriod);
  pingPeriod = p_pingPeriod;
  return;
}

uint8_t mqtt::subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args){
  bool found = false;
  int i;
  int j;
  char* topic = createNcpystr(p_topic);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  if(opState != OP_WORKING){
    xSemaphoreGive(mqttLock); //Serial.print("Released lock at "); //Serial.println(__LINE__);
    Log.fatal("mqtt::subscribeTopic: Could not subscribe to topic %s, MQTT is not running - rebooting..." CR, topic);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  xSemaphoreGive(mqttLock);
  Log.notice("mqtt::subscribeTopic: Subscribing to topic %s" CR, topic);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  for (i=0; i < mqttTopics.size(); i++){
    if (!strcmp(mqttTopics.at(i)->topic, topic)){
      found = true;
      break;
    }
  }
  if (!found){
    mqttTopics.push_back(new(mqttTopic_t));
    mqttTopics.back()->topic = topic;
    mqttTopics.back()->topicList = new QList<mqttSub_t*>;
    mqttTopics.back()->topicList->push_back(new(mqttSub_t));
    mqttTopics.back()->topicList->back()->topic = topic;
    mqttTopics.back()->topicList->back()->mqttSubCallback = p_callback;
    mqttTopics.back()->topicList->back()->mqttCallbackArgs = (void*)p_args;
    xSemaphoreGive(mqttLock);
    if (!mqttClient.subscribe(topic, defaultQoS)){
      Log.fatal("mqtt::subscribeTopic: Could not subscribe Topic: %s from broker - rebooting..." CR, topic);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Log.notice("mqtt::subscribeTopic: Subscribed to %s, QoS %d" CR, mqttTopics.back()->topicList->back()->topic, defaultQoS);
    logSubscribers();
    return RC_OK;
  }
  else{
    for (j=0; j<mqttTopics.at(i)->topicList->size(); j++){
      if (mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback){
        Log.warning("mqtt::subscribeTopic: MQTT-subscribeTopic: subscribeTopic was called, but the callback 0x%x for Topic %s already exists - doing nothing" CR, (int)p_callback, topic);
        xSemaphoreGive(mqttLock);
        return RC_OK;
      }
    }
    mqttTopics.at(i)->topicList->push_back(new(mqttSub_t));
    mqttTopics.at(i)->topicList->back()->topic = topic;
    mqttTopics.at(i)->topicList->back()->mqttSubCallback = p_callback;
    mqttTopics.at(i)->topicList->back()->mqttCallbackArgs = (void*)p_args;
    xSemaphoreGive(mqttLock);
    logSubscribers();
    return RC_OK;
  }
  return RC_GEN_ERR;   //Should never reach here
}

uint8_t mqtt::reSubscribe(void){
  Log.notice("mqtt::reSubscribe: Resubscribing all registered topics" CR);
  logSubscribers();
  for (int i=0; i < mqttTopics.size(); i++){
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    if (!mqttClient.subscribe(mqttTopics.at(i)->topic, defaultQoS)){
      Log.fatal("mqtt::reSubscribe: Failed to resubscribe topic: %s from broker" CR, mqttTopics.at(i)->topic);
      xSemaphoreGive(mqttLock);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    xSemaphoreGive(mqttLock);
  }
  Log.notice("mqtt::reSubscribe: Successfully resubscribed to all registered topics" CR);
  return RC_OK;
}

uint8_t mqtt::unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback){
  bool topicFound = false;
  bool cbFound = false;
  char* topic = createNcpystr(p_topic);
  Log.notice("MQTT-Unsubscribe, Un-subscribing to topic %s" CR, topic);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  for(int i=0; i < mqttTopics.size(); i++){
    if (!strcmp(mqttTopics.at(i)->topic, topic)){
      topicFound = true;
      for (int j=0; j<mqttTopics.at(i)->topicList->size(); j++){
        if(mqttTopics.at(i)->topicList->at(j)->mqttSubCallback == p_callback){
          cbFound = true;
          mqttTopics.at(i)->topicList->clear(j);
          Log.notice("MQTT-Unsubscribe, Removed callback for %s" CR, topic);
          if (mqttTopics.at(i)->topicList->size() == 0){
            delete mqttTopics.at(i)->topicList;
            delete mqttTopics.at(i)->topic;
            mqttTopics.clear(i);
            if (!mqttClient.unsubscribe(topic)){
              Log.error("mqtt::unSubscribeTopic: could not unsubscribe Topic: %s from broker" CR, topic);
              logSubscribers();
              delete topic;
              xSemaphoreGive(mqttLock);
              return RC_GEN_ERR;
            }
            Log.notice("mqtt::unSubscribeTopic: Last callback for %s unsubscribed - unsubscribed Topic from broker" CR, topic);
            logSubscribers();
            delete topic;
            xSemaphoreGive(mqttLock);
            return RC_OK;
          }
        }
      }
    }
  }
  if(!topicFound) {
    Log.error("mqtt::unSubscribeTopic: Topic %s not found" CR, topic);
    logSubscribers();
    delete topic;
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;
  }
  if(!cbFound) {
    Log.error("MQTT-Unsubscribe, callback not found while unsubscribing topic %s" CR, topic);
    logSubscribers();
    xSemaphoreGive(mqttLock);
    return 1;
  }
  logSubscribers();
  delete topic;
  xSemaphoreGive(mqttLock);
  return RC_GEN_ERR;
}

void mqtt::logSubscribers(void){
  // if loglevel < verbose - return
  Log.verbose("mqtt::logSubscribers: ---CURRENT SUBSCRIBERS---" CR);
  for (int i=0; i < mqttTopics.size(); i++){
    Log.verbose("mqtt::logSubscribers: Topic: %s" CR, mqttTopics.at(i)->topic);
    for (int j=0; j < mqttTopics.at(i)->topicList->size(); j++){
      Log.verbose("mqtt::logSubscribers:    Callback: 0x%x" CR, (int)mqttTopics.at(i)->topicList->at(j)->mqttSubCallback);
    }
  }
  Log.verbose("mqtt::logSubscribers: ---END SUBSCRIBERS---" CR);
  return;
}

uint8_t mqtt::up(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  if (tmpOpState != OP_WORKING) { //We should just return fail - and have poll bringing it up
    Log.warning("mqtt::up, could not declare MQTT up as opState is not OP_WORKING");
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;
  }
  if(uri == NULL){
    Log.warning("mqtt::up: The URI for this decodet has not been defined - waiting for successful discovery process, cannot declare MQTT up" CR);
    xSemaphoreGive(mqttLock);
    return RC_GEN_ERR;
  }
  Log.notice("mqtt::up: Client is up and working, sending up-message and starting MQTT ping supervision" CR);
  xSemaphoreGive(mqttLock);
  sendMsg(opStateTopic, upPayload, true);
  const char* pingUpstreamTopicStrings[3] = {MQTT_PING_UPSTREAM_TOPIC, uri, "/"};
  mqttPingUpstreamTopic = concatStr(pingUpstreamTopicStrings, 3);
  const char* pingDownstreamTopicStrings[3] = {MQTT_PING_DOWNSTREAM_TOPIC, uri, "/"};
  if (subscribeTopic(concatStr(pingDownstreamTopicStrings, 3), onMqttPing , NULL)){
    Log.fatal("mqtt::up: Failed to to subscribe to MQTT ping topic - rebooting..." CR);
    //Failsafe
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    opState = OP_FAIL;
    xSemaphoreGive(mqttLock);
    ESP.restart();
    return RC_GEN_ERR;
  }
  xTaskCreatePinnedToCore(
                          mqttPingTimer,              // Task function
                          "mqttPingTimer",            // Task function name reference
                          6*1024,                     // Stack size
                          NULL,                       // Parameter passing
                          PING_TIMER_PRIO,            // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          PING_TIMER_CORE);                    // Core [CORE_0 | CORE_1]

  return RC_OK;
}


uint8_t mqtt::down(void){
  Log.notice("mqtt::down, Disconnecting Mqtt client" CR);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  opState = OP_DISABLE;
  xSemaphoreGive(mqttLock);
  //esp_timer_stop(mqttPingTimerHandle); //need to fix - killing the timer process
  sendMsg(opStateTopic, downPayload, true);
  vTaskDelay(200 / portTICK_PERIOD_MS);
  mqttClient.disconnect();
  return RC_OK;
}

void mqtt::onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length) {
  bool subFound = false;
  char* payload = new char[p_length +1];
  memcpy(payload, p_payload, p_length);
  payload[p_length] = '\0';
  Log.verbose("mqtt::onMqttMsg, Received an MQTT mesage, topic: %s, payload: %s, length: %d" CR, p_topic, payload, p_length);
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  for (int i=0; i < mqttTopics.size(); i++){
    if (!strcmp(mqttTopics.at(i)->topic, p_topic)) {
      for (int j=0; j<mqttTopics.at(i)->topicList->size(); j++){
        subFound = true;
        //mqttSubCallback_t cb = &(mqttTopics.at(i)->topicList->at(j)->mqttSubCallback(p_topic, payload, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs)); //We should get this working
        xSemaphoreGive(mqttLock);
        //cb;
        mqttTopics.at(i)->topicList->at(j)->mqttSubCallback(p_topic, payload, mqttTopics.at(i)->topicList->at(j)->mqttCallbackArgs);
        xSemaphoreTake(mqttLock, portMAX_DELAY);
      }
    } 
  }
  if(!subFound) {
    Log.error("mqtt::onMqttMsg, could not find any subscription for received message topic: %s" CR, p_topic);
    delete payload;
    xSemaphoreGive(mqttLock);
    return;
  }
  delete payload;
  xSemaphoreGive(mqttLock);
  return;
}

uint8_t mqtt::sendMsg(const char* p_topic, const char* p_payload, bool p_retain){
  if (!mqttClient.publish(p_topic, p_payload, p_retain)){
    Log.error("mqtt::sendMsg: could not send message, topic: %s, payload: %s" CR, p_topic, p_payload);
    return RC_GEN_ERR;
  } else {
    Log.verbose("mqtt::sendMsg: sent a message, topic: %s, payload: %s" CR, p_topic, p_payload);    
  }
  return RC_OK;
}

void mqtt::poll(void* dummy){
  int64_t  nextLoopTime = esp_timer_get_time();
  int64_t thisLoopTime;
  uint16_t latencyIndex = 0;
  int32_t latency = 0; 
  uint8_t retryCnt = 0;
  int stat;
  uint8_t tmpOpState;
  //esp_task_wdt_init(1, true); //enable panic so ESP32 restarts
  //esp_task_wdt_add(NULL); //add current thread to WDT watch
  while(true){
    thisLoopTime = nextLoopTime;
    nextLoopTime += MQTT_POLL_PERIOD_MS * 1000;
    //esp_task_wdt_reset();
    mqttClient.loop();
    stat = mqttClient.state();
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    tmpOpState = opState;
    xSemaphoreGive(mqttLock);

    switch (stat) {
      case MQTT_CONNECTED:
      xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_WORKING;
        xSemaphoreGive(mqttLock);
        if(mqttStatus != stat){
          Log.notice("mqtt::poll, MQTT connection established - opState set to OP_WORKING" CR);
        }
        retryCnt = 0;
        break;
      case MQTT_CONNECTION_TIMEOUT:
      case MQTT_CONNECTION_LOST:
      case MQTT_CONNECT_FAILED:
      case MQTT_DISCONNECTED:
        if (tmpOpState == OP_DISABLE){ //We need to resubscribe after re-establishment..............................
          break;
        }
        xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_FAIL;
        xSemaphoreGive(mqttLock);
        if (retryCnt >= MAX_MQTT_CONNECT_ATTEMPTS_100MS){
          Log.fatal("mqtt::poll, Max number of MQTT connect/reconnect attempts reached, cause: %d - rebooting..." CR, stat);
          opState = OP_FAIL;
          //failsafe();
          ESP.restart();
          return;
        }
        Log.notice("mqtt::poll: Connecting to Mqtt" CR);
        if(opStateTopicSet){
          Log.notice("mqtt::poll: reconnecting with Last will" CR);
          mqttClient.connect(clientId, 
                             user, 
                             pass, 
                             opStateTopic, 
                             QOS_1, 
                             true, 
                             downPayload);
        }
        else{
          mqttClient.connect(clientId, 
                             user, 
                             pass);

        }
        Log.notice("mqtt::poll: Tried to connect");
        if(mqttStatus != stat){
          Log.error("mqtt::poll, MQTT connection not established or lost - opState set to OP_FAIL, cause: %d - retrying..." CR, stat); //This never times out but freezes
        }
        retryCnt++;
        break;
      case MQTT_CONNECT_BAD_PROTOCOL:
      case MQTT_CONNECT_BAD_CLIENT_ID:
      case MQTT_CONNECT_UNAVAILABLE:
      case MQTT_CONNECT_BAD_CREDENTIALS:
      case MQTT_CONNECT_UNAUTHORIZED:
        xSemaphoreTake(mqttLock, portMAX_DELAY);
        opState = OP_FAIL;
        xSemaphoreGive(mqttLock);
        Log.fatal("mqtt::poll, Fatal MQTT error, one of BAD_PROTOCOL, BAD_CLIENT_ID, UNAVAILABLE, BAD_CREDETIALS, UNOTHORIZED, cause: %d - rebooting..." CR, stat);
        //failsafe();
        ESP.restart();
        return;
    }
    if (mqttStatus != stat){
      mqttStatus = stat;
      if (statusCallback != NULL){
        statusCallback(mqttStatus);
      }
    }
    if(latencyIndex >= avgSamples) {
      latencyIndex = 0;
    }
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    latency = esp_timer_get_time()-thisLoopTime;
    latencyVect[latencyIndex++] = latency;
    if(latency > maxLatency) {
      maxLatency = latency;
    }
    xSemaphoreGive(mqttLock);
    TickType_t delay;
    if((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
      vTaskDelay((delay/1000)/portTICK_PERIOD_MS);

    }
    else{
      Log.verbose("mqtt::poll: MQTT Overrun" CR);
      xSemaphoreTake(mqttLock, portMAX_DELAY);
      overRuns++;
      xSemaphoreGive(mqttLock);
      nextLoopTime = esp_timer_get_time();
    }
  }
  return;
}

void mqtt::mqttPingTimer(void* dummy){
  while(true){
    Log.notice("mqtt::poll: MQTT Ping timer started" CR);
    xSemaphoreTake(mqttLock, portMAX_DELAY);
    uint8_t tmpOpState = opState;
    if(tmpOpState == OP_WORKING && pingPeriod != 0) {
      if(++missedPings >= MAX_MQTT_LOST_PINGS){
       Log.fatal("mqtt::mqttPingTimer, Lost %d ping responses - bringing down MQTT and rebooting..." CR, MAX_MQTT_LOST_PINGS);
       xSemaphoreGive(mqttLock);
       down();
       //failsafe();
       ESP.restart();
      }
      xSemaphoreGive(mqttLock);
      sendMsg(mqttPingUpstreamTopic, PING, false);
    }
    vTaskDelay(pingPeriod*1000 / portTICK_PERIOD_MS);
  }
}

void mqtt::onMqttPing(const char* p_topic, const char* p_payload, const void* p_dummy){ //Relying on the topic, not parsing the payload for performance
   Log.verbose("mqtt::onMqttPing: Received a Ping response" CR);
   xSemaphoreTake(mqttLock, portMAX_DELAY);
   missedPings = 0;
   xSemaphoreGive(mqttLock);
   return;
}

uint8_t mqtt::getOpState(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(mqttLock);
  return tmpOpState;
}

char* mqtt::getUri(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  char* tmpUri = uri;
  xSemaphoreGive(mqttLock);
  return tmpUri;
}

uint32_t mqtt::getOverRuns(void){
  unsigned long tmpOverRuns;
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  tmpOverRuns = overRuns;
  xSemaphoreGive(mqttLock);
  return tmpOverRuns;
}

void mqtt::clearOverRuns(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  overRuns = 0;
  xSemaphoreGive(mqttLock);
  return;
}

uint32_t mqtt::getMeanLatency(void){
  uint32_t* tmpLatencyVect = new uint32_t[avgSamples]; //Wee need to fix all latencies to type int32_t, and all other performance metrics to int....
  uint32_t accLatency;
  uint32_t meanLatency;
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  memcpy(tmpLatencyVect, latencyVect, avgSamples);
  xSemaphoreGive(mqttLock);
  for(uint16_t latencyIndex = 0; latencyIndex < avgSamples; latencyIndex++){
    accLatency += latencyVect[latencyIndex];
  }
  meanLatency = accLatency/avgSamples;
  delete tmpLatencyVect;
  return meanLatency;
}

uint32_t mqtt::getMaxLatency(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  uint32_t tmpMaxLatency = maxLatency;
  xSemaphoreGive(mqttLock);
  return tmpMaxLatency;
}

void mqtt::clearMaxLatency(void){
  xSemaphoreTake(mqttLock, portMAX_DELAY);
  maxLatency = 0;
  xSemaphoreGive(mqttLock);
  return;
}

/*==============================================================================================================================================*/
/* END Class mqtt                                                                                                                               */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: topDecoder                                                                                                                            */
/* Purpose: The "topDecoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,        */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroups [Signal Masts | general Lights | sequencedLights],       */
/*          Turnouts or sensors...                                                                                                              */
/*          The "topDecoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
uint8_t topDecoder::opState = OP_CREATED;
char* topDecoder::mac;
char** topDecoder::xmlconfig;
char* topDecoder::xmlVersion;
char* topDecoder::xmlDate;
char* topDecoder::ntpServer;
uint8_t topDecoder::timeZone;
tinyxml2::XMLDocument* topDecoder::xmlConfigDoc;
SemaphoreHandle_t topDecoder::topDecoderLock;
lgLinks*[2] topDecoder::lgLinks;
satLinks*[2] topDecoder::satLinks;
cliContext_t* topDecoder::lightgropsCliContext;


uint8_t topDecoder::init(void){
  if(opState != OP_CREATED){
    Log.fatal("topDecoder::init: opState is not OP_CREATED - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  /*MOVE TO CREATION OF LINKS
  lightgropsCliContext = new cliContext_t;
  lightgropsCliContext->cliContext = (char*)"lightGroupLink";
  lightgropsCliContext->cliContextInitCb = lightgroupLinkCliContextInit;
  lightgropsCliContext->cliContextCb = onLightgropsCliCmd;
  CLI.registerCliContext(lightgropsCliContext);
  */
  Log.notice("topDecoder::init: Initializing topDecoder" CR);
  topDecoderLock = xSemaphoreCreateMutex();
  Log.notice("topDecoder::init: Initializing MQTT " CR);
  mqtt::init( (char*)"test.mosquitto.org",  //Broker URI WE NEED TO GET THE BROKER FROM SOMEWHERE
              1883,                         //Broker port
              (char*)"",                    //User name
              (char*)"",                    //Password
              (char*)MQTT_CLIENT_ID,        //Client ID
              QOS_1,                        //QoS
              MQTT_KEEP_ALIVE,              //Keep alive time
              true);                        //Default retain

  int i = 0;
  while (mqtt::getOpState() != OP_WORKING) {
    if(i++ >= 120){
      Log.fatal("topDecoder::init: Could not connect to MQTT broker - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Serial.println("");
  i = 0;
  while(mqtt::getUri() == NULL) {
    if(i == 0){
      Log.notice("topDecoder::init: Waiting for discovery process");
    }
    if(i++ >= 120){
      Log.fatal("topDecoder::init: Discovery process failed - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Serial.println("");
  const char* opStateTopicStrings[3] = {MQTT_OPSTATE_TOPIC, mqtt::getUri(), "/"};
  mqtt::setOpstateTopic(concatStr(opStateTopicStrings, 3), (char*)DECODER_UP, (char*)DECODER_DOWN);
  i = 0;
  Log.notice("topDecoder::init: Waiting for MQTT to come back to working state" CR);
  while(mqtt::getOpState() != OP_WORKING){
    if(i++ >= 120){
      Log.fatal("topDecoder::init: MQTT failed to come back to working state - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
    i++;
  }
  Log.notice("topDecoder::init: Subscribing to decoder configuration topic and sending configuration request");
  const char* subscribeTopic[3] = {MQTT_CONFIG_REQ_TOPIC, mqtt::getUri(), "/"};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 3), on_configUpdate, NULL)){
    Log.fatal("topDecoder::init: Failed to suscribe to configuration response topic - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  mqtt::sendMsg(MQTT_CONFIG_REQ_TOPIC, CONFIG_REQ_PAYLOAD, false);
  opState = OP_INIT;
  int i;
  Log.notice("topDecoder::init: Waiting for configuration");
  while(opState == OP_INIT){
    if(i++ >= 120){
        Log.fatal("topDecoder::init: Did not receive a configuration - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_GEN_ERR;
        }
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  return RC_OK;
}

void topDecoder::on_configUpdate(const char* p_topic, const char* p_payload, const void* p_dummy){
  if(opState != OP_INIT){
    Log.fatal("topdecoder-on_configUpdate: Received a configuration, while the topdecoder already had an earlier configuration, dynamic re-configuration not supported, opState: %d - rebooting..." CR, opState);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  Log.notice("topDecoder::on_configUpdate: Received an uverified configuration, parsing and validating it..." CR);
  xmlconfig = new char*[9];
  xmlconfig[XMLAUTHOR] = NULL;
  xmlconfig[XMLDESCRIPTION] = NULL;
  xmlconfig[XMLVERSION] = NULL;
  xmlconfig[XMLDATE] = NULL;
  //const char* default_rsyslogreceiver = DEFAULT_RSYSLOGRECEIVER;
  //xmlconfig[XMLRSYSLOGRECEIVER] = (char*)default_rsyslogreceiver;
  xmlconfig[XMLRSYSLOGRECEIVER]= new char[strlen(DEFAULT_RSYSLOGRECEIVER)+1]; //Memory leak
  strcpy(xmlconfig[XMLRSYSLOGRECEIVER], DEFAULT_RSYSLOGRECEIVER);
  xmlconfig[XMLLOGLEVEL] = new char[4];
  itoa(DEFAULT_LOGLEVEL, xmlconfig[XMLLOGLEVEL], 10);
  //const char* default_ntpserver = DEFAULT_NTPSERVER;
  //xmlconfig[XMLNTPSERVER] = (char*)default_ntpserver;
  xmlconfig[XMLNTPSERVER]= new char[strlen(DEFAULT_NTPSERVER)+1];
  strcpy(xmlconfig[XMLNTPSERVER], DEFAULT_NTPSERVER);
  xmlconfig[XMLTIMEZONE] = new char[3];
  itoa(DEFAULT_TIMEZONE, xmlconfig[XMLTIMEZONE], 10);
  xmlconfig[XMLPINGPERIOD] = new char[6];
  itoa(DEFAULT_PINGPERIOD, xmlconfig[XMLPINGPERIOD], 10);
  mac = NULL;
  xmlVersion = NULL;
  xmlDate = NULL;
  ntpServer = NULL;
  timeZone = 0;
  xmlConfigDoc = new tinyxml2::XMLDocument;

  //Reset XMLDocument object???
  if (xmlConfigDoc->Parse(p_payload)) {
    Log.fatal("topDecoder::on_configUpdate: Configuration parsing failed - Rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  if (xmlConfigDoc->FirstChildElement("Decoder") == NULL || xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top") == NULL || xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top")->FirstChildElement() == NULL ) {
    Log.fatal("topDecoder::on_configUpdate: Failed to parse the configuration - xml is missformatted - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return;
  }
  const char* topSearchTags[9];
  topSearchTags[XMLAUTHOR] = "Author";
  topSearchTags[XMLDESCRIPTION] = "Description";
  topSearchTags[XMLVERSION] = "Version";
  topSearchTags[XMLDATE] = "Date";
  topSearchTags[XMLRSYSLOGRECEIVER] = "RsyslogReceiver";
  topSearchTags[XMLLOGLEVEL] = "Loglevel";
  topSearchTags[XMLNTPSERVER] = "NTPServer";
  topSearchTags[XMLTIMEZONE] = "TimeZone";
  topSearchTags[XMLPINGPERIOD] = "PingPeriod";
  
  getTagTxt(xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Top")->FirstChildElement(), topSearchTags, xmlconfig, sizeof(topSearchTags)/4); // Need to fix the addressing for portability
  Log.notice("topDecoder::on_configUpdate: Successfully parsed the topdecoder configuration:" CR);
  if(xmlconfig[XMLAUTHOR] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Author: %s" CR, xmlconfig[XMLAUTHOR]);
  } else {
    Log.notice("topDecoder::on_configUpdate: XML Author not provided - skipping..." CR);
  }
  if(xmlconfig[XMLDATE] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Date: %s" CR, xmlconfig[XMLDATE]);
  }
  else {
    Log.notice("topDecoder::on_configUpdate: XML Date not provided - skipping..." CR);
  }
  if(xmlconfig[XMLVERSION] != NULL) {
    Log.notice("topDecoder::on_configUpdate: XML Version: %s" CR, xmlconfig[XMLVERSION]);
  } else {
    Log.notice("topDecoder::on_configUpdate: XML Version not provided - skipping..." CR);
  }
  if(xmlconfig[XMLRSYSLOGRECEIVER] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Rsyslog receiver: %s - NOT IMPLEMENTED" CR, xmlconfig[XMLRSYSLOGRECEIVER]);
  }
  if(xmlconfig[XMLLOGLEVEL] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Log-level: %s" CR, xmlconfig[XMLLOGLEVEL]);
    /* NEEDS TO BE FIXED!!!!!!
    switch (atoi(xmlconfig[XMLLOGLEVEL])){
      case DEBUG_VERBOSE:
        Log.setLevel(LOG_LEVEL_VERBOSE);
        break;
      case DEBUG_TERSE:
        Log.setLevel(LOG_LEVEL_TRACE);
        break;
      case INFO:
        Log.setLevel(LOG_LEVEL_NOTICE);
        break;
      case ERROR:
        Log.setLevel(LOG_LEVEL_ERROR);
        break;
      case PANIC:
        Log.setLevel(LOG_LEVEL_FATAL);
        break;
      default:
        Log.error("topDecoder::on_configUpdate: %s is not a valid log-level, will keep-on to the default log level %d" CR, xmlconfig[XMLLOGLEVEL], INFO);
        itoa(INFO, xmlconfig[XMLLOGLEVEL], 10);
    }
    */
  } else {
    Log.notice("topDecoder::on_configUpdate: Loglevel not provided - using default log-level (Notice)" CR);
    //Log.setLevel(LOG_LEVEL_NOTICE);
    itoa(INFO, xmlconfig[XMLLOGLEVEL], 10);
  }
  if(xmlconfig[XMLNTPSERVER] != NULL) {
    Log.notice("topDecoder::on_configUpdate: NTP-server: %s" CR, xmlconfig[XMLNTPSERVER]);
  } else {
    Log.notice("topDecoder::on_configUpdate: NTP server not provided - skipping..." CR);
  }
  if(xmlconfig[XMLTIMEZONE] != NULL) {
    Log.notice("topDecoder::on_configUpdate: Time zone: %s" CR, xmlconfig[XMLTIMEZONE]);
  } else {
    Log.notice("topDecoder::on_configUpdate: Timezone not provided - using UTC..." CR);
    itoa(0, xmlconfig[XMLTIMEZONE], 10);
  }
  if(xmlconfig[XMLPINGPERIOD] != NULL) {
    Log.notice("topDecoder::on_configUpdate: MQTT Ping period: %s" CR, xmlconfig[XMLPINGPERIOD]);
    mqtt::setPingPeriod(strtof(xmlconfig[XMLPINGPERIOD], NULL)); //Should be moved to a supervision class
  } else {
    Log.notice("topDecoder::on_configUpdate: Ping-period not provided - using default %f" CR, DEFAULT_MQTT_PINGPERIOD); 
    mqtt::setPingPeriod(DEFAULT_MQTT_PINGPERIOD); //Should be moved to a supervision class
    xmlconfig[XMLPINGPERIOD] = new char[10];
    dtostrf(DEFAULT_MQTT_PINGPERIOD, 4, 3,xmlconfig[XMLPINGPERIOD]);
  }
  tinyxml2::XMLElement* lgXmlElement;
  if ((lgXmlElement = xmlConfigDoc->FirstChildElement("Decoder")->FirstChildElement("Lightgroups")) != NULL){
    lighGroupsDecoder_0 = new lgsDecoder(); //Should eventually support up to 4 channels
    lighGroupsDecoder_0->init(0);
    lighGroupsDecoder_0->on_configUpdate(lgXmlElement);
  }
  //Calls to other decoder types are placed here
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  opState = OP_CONFIG;
  xSemaphoreGive(topDecoderLock);
  delete xmlConfigDoc;
  Log.notice("topDecoder::on_configUpdate: Configuration finished" CR);
  return;
}

uint8_t topDecoder::start(void){
  Log.notice("topDecoder::start: Starting topDecoder" CR);
  while(true){
    xSemaphoreTake(topDecoderLock, portMAX_DELAY);
    if(opState == OP_CONFIG){
      xSemaphoreGive(topDecoderLock);
      break;
    }
    xSemaphoreGive(topDecoderLock);
    Log.notice("topDecoder::start: Waiting for Top decoder to be configured before it can start" CR);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Log.notice("topDecoder::start: Starting lightgroupsDecoders" CR);
  //mqtt::up();
  if(lighGroupsDecoder_0 != NULL){
    if(lighGroupsDecoder_0->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 0 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_1 != NULL){
    if(lighGroupsDecoder_1->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 1 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_2 != NULL){
    if(lighGroupsDecoder_2->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 2 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  if(lighGroupsDecoder_3 != NULL){
    if(lighGroupsDecoder_3->start()){
      Log.notice("topDecoder::start: Failed to start Lightdecoder channel 3 - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_GEN_ERR;
    }
  }
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(topDecoderLock);
  mqtt::up();
  Log.notice("topDecoder::start: TopDecoder started" CR);
}

char* topDecoder::getXmlAuthor(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlAuthor = xmlconfig[XMLAUTHOR];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlAuthor;
}

char* topDecoder::getXmlDate(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlDate = xmlconfig[XMLDATE];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlDate;
}

char* topDecoder::getXmlVersion(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpXmlVersion = xmlconfig[XMLVERSION];
  xSemaphoreGive(topDecoderLock);
  return tmpXmlVersion;
}

char* topDecoder::getRsyslogReceiver(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpRsyslogReceiver = xmlconfig[XMLRSYSLOGRECEIVER];
  xSemaphoreGive(topDecoderLock);
  return tmpRsyslogReceiver;
}

uint8_t topDecoder::getLogLevel(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpLoglevel = atoi(xmlconfig[XMLLOGLEVEL]);
  xSemaphoreGive(topDecoderLock);
  return tmpLoglevel;
}

char* topDecoder::getNtpServer(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  char* tmpNtpServer = xmlconfig[XMLNTPSERVER];
  xSemaphoreGive(topDecoderLock);
  return tmpNtpServer;
}

uint8_t topDecoder::getTimezone(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpTimezone = atoi(xmlconfig[XMLTIMEZONE]);
  xSemaphoreGive(topDecoderLock);
  return tmpTimezone;
}

float topDecoder::getMqttPingperiod(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  float tmpXmlPingPeriod = atof(xmlconfig[XMLPINGPERIOD]);
  xSemaphoreGive(topDecoderLock);
  return tmpXmlPingPeriod;
}

uint8_t topDecoder::getopState(void) {
  xSemaphoreTake(topDecoderLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(topDecoderLock);
  return tmpOpState;
}

void topDecoder::lightgroupLinkCliContextInit(void){
  Serial.println("Adding lightgroups context args");
  decoderCli::set.addArgument("ch/annel");
  decoderCli::set.addArgument("aspect");
  decoderCli::set.addArgument("lg/addr");
  decoderCli::get.addFlagArgument("op/state");
  decoderCli::get.addFlagArgument("over/runs");
  decoderCli::get.addFlagArgument("lat/ency");
  decoderCli::get.addFlagArgument("maxlat/ency");
  decoderCli::get.addFlagArgument("run/time");
  decoderCli::get.addFlagArgument("maxrun/time");
  decoderCli::get.addFlagArgument("light/group");
  decoderCli::get.addArgument("obj/ect", "");
  decoderCli::get.addArgument("adr/ess", "");
}

uint8_t topDecoder::onLightgropsCliCmd(cmd* p_cmd){
  Command cmd(p_cmd);
  String command;
  CLI.printToCli("Command accepted - parsing..\n\r", true);
  command = cmd.getName();
  if(command == "help"){
    CLI.printToCli("This is the lightgroups help");
    return CLI_PARSE_STOP;
  }
  if(command == "get"){
    Argument getopStateTopic = cmd.getArgument("op/state");
    Argument getOverrunTopic = cmd.getArgument("over/runs");
    Argument getLatencyTopic = cmd.getArgument("lat/ency");
    Argument getMaxLatencyTopic = cmd.getArgument("maxlat/ency");
    Argument getRuntimeTopic = cmd.getArgument("run/time");
    Argument getMaxRuntimeTopic = cmd.getArgument("maxrun/time");
    Argument getLgTopic = cmd.getArgument("light/group");

    char outputBuff[120];
    if(getopStateTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 operational state: ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getOpState(), outputBuff, 10));
      return CLI_PARSE_STOP;
    }
    if(getOverrunTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 strip overruns: ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getOverRuns(), outputBuff, 10));
      lighGroupsDecoder_0->clearOverRuns();
      return CLI_PARSE_STOP;
    }
    if(getLatencyTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 strip average latency: ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getMeanLatency(), outputBuff, 10));
      return CLI_PARSE_STOP;
    }
    if(getMaxLatencyTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 strip max latency (is reset by this command): ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getMaxLatency(), outputBuff, 10));
      lighGroupsDecoder_0->clearMaxLatency();
      return CLI_PARSE_STOP;
    }
    if(getRuntimeTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 strip average runtime: ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getMeanRuntime(), outputBuff, 10));
      return CLI_PARSE_STOP;
    }
    if(getMaxRuntimeTopic.isSet()){
      CLI.printToCli("lightgroups decoder channel-0 strip max runtime (is reset by this command): ", true);
      CLI.printToCli(itoa(lighGroupsDecoder_0->getMaxRuntime(), outputBuff, 10));
      lighGroupsDecoder_0->clearMaxRuntime();
      return CLI_PARSE_STOP;
    }
    if(getLgTopic.isSet()){
      Argument getObjTopic = cmd.getArgument("obj/ect");
      Serial.println("Printing");
      Serial.println(getObjTopic.getValue());
      Argument getAdrTopic = cmd.getArgument("adr/ess");
      Serial.println(getAdrTopic.getValue());
      if(getAdrTopic.getValue() == "" || getAdrTopic.getValue() == ""){
        CLI.printToCli("ERROR: lightgroup object or adress missing");
        return CLI_PARSE_STOP; 
      }
      if(getObjTopic.getValue() == "type"){
        CLI.printToCli("Request for lightgroup type for lightgroup adress ", true);
        CLI.printToCli(getAdrTopic.getValue());
        return CLI_PARSE_STOP; 
      }
      else if (getObjTopic.getValue() == "aspect"){
        CLI.printToCli("Request for lightgroup aspect for lightgroup adress ", true);
        CLI.printToCli(getAdrTopic.getValue());
        return CLI_PARSE_STOP; 
      }
      else if (getObjTopic.getValue() == "sequence"){
        CLI.printToCli("Request for lightgroup sequence start for lightgroup adress ", true);
        CLI.printToCli(getAdrTopic.getValue());
        return CLI_PARSE_STOP;
      }
      else{
        CLI.printToCli("ERROR: lightgroup object invalid");
        return CLI_PARSE_STOP;
      }
    }
  }
  return CLI_PARSE_CONTINUE;
}

/*==============================================================================================================================================*/
/* END Class topDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgsDecoder(lightgroupsDecoder)"                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
lgsDecoder::lgsDecoder(void){
  avgSamples = UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS;
  latencyVect = new int64_t[avgSamples];
  runtimeVect = new uint32_t[avgSamples];
  Log.notice("lgsDecoder::lgDecoder: Creating Lightgroups decoder channel" CR);
  opState = OP_CREATED;
}

lgsDecoder::~lgsDecoder(void){
  Log.fatal("lgsDecoder::~lgDecoder: Destruction not supported - rebooting..." CR);
  opState = OP_FAIL;
  //failsafe();
  ESP.restart();
  return;
}

uint8_t lgsDecoder::init(const uint8_t p_channel){
  if(opState != OP_CREATED) {
    Log.notice("lgsDecoder::init: opState is not OP_CREATED - Rebooting..." CR);
  }
  Log.notice("lgsDecoder::init: Initializing Lightgroups decoder channel %d" CR, p_channel);
  channel = p_channel;
  lgsDecoderLock = xSemaphoreCreateMutex();

  switch(channel){
    case 0 :
      pin = LEDSTRIP_CH0_PIN;
      break;
    case 1 :
      pin = LEDSTRIP_CH1_PIN;
      break;
    case 2 :
      pin = LEDSTRIP_CH2_PIN;
      break;
    case 3 :
      pin = LEDSTRIP_CH3_PIN;
      break;
    default:
      Log.fatal("lgsDecoder::init: Lightgroup channel %d not supported - rebooting..." CR, channel);
      opState = OP_FAIL;
      //failSafe();
      ESP.restart();
      return RC_NOTIMPLEMENTED_ERR;
  }
/* In case we want to put SMASPECTS on the heap instead
 if(SMASPECTS == NULL){
    Log.fatal("lgsDecoder::init: Could not create an smAspect object - rebooting..." CR);
    //failSafe();
    opState = OP_FAILED;
    ESP.restat();
    return RC_OUT_OF_MEM_ERR;
  }
  */
  FLASHNORMAL = new flash((float)SM_FLASH_TIME_NORMAL, (uint8_t)50); // Move to top_decoder?
  FLASHSLOW = new flash(SM_FLASH_TIME_SLOW, 50); // Move to top_decoder?
  FLASHFAST = new flash(SM_FLASH_TIME_FAST, 50); // Move to top_decoder?
  if(FLASHNORMAL == NULL || FLASHSLOW == NULL || FLASHFAST == NULL){
    Log.fatal("lgsDecoder::init: Could not create flash objectst - rebooting..." CR);
    opState = OP_FAIL;
    //failSafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  //stripLed_t stripCtrlBuff[MAXSTRIPLEN];
  stripCtrlBuff = new stripLed_t[MAXSTRIPLEN];
  strip = new Adafruit_NeoPixel(MAXSTRIPLEN, pin, NEO_RGB + NEO_KHZ800);
  if(strip == NULL){
    Log.fatal("lgsDecoder::init: Could not create NeoPixel object - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  strip->begin();
  stripWritebuff = strip->getPixels();
  //failSafe();
  opState = OP_INIT;
  return RC_OK;
}

uint8_t lgsDecoder::on_configUpdate(tinyxml2::XMLElement* p_lightgroupsXmlElement){
  if(opState != OP_INIT){
    Log.fatal("lgsDecoder::on_configUpdate: opState is not OP_INIT - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  if(signalMastAspects::onConfigUpdate(p_lightgroupsXmlElement->FirstChildElement("SignalMastDesc"))){ // We should move this outside the lightgroups scope
    Log.error("lgsDecoder::on_configUpdate: Could not configure SMASPECTS - continuing..." CR);
  }
  lightGroupXmlElement = p_lightgroupsXmlElement->FirstChildElement("Lightgroup");
  const char* lgSearchTags[5];
  lgSearchTags[XMLLGADDR] = "LgAddr";
  lgSearchTags[XMLLGSEQ] = "LgSequence";
  lgSearchTags[XMLLGSYSTEMNAME] = "LgSystemName";
  lgSearchTags[XMLLGUSERNAME] = "LgUserName";
  lgSearchTags[XMLLGTYPE] = "LgType";
  char* lgConfigTxtBuff[5];
  while(lightGroupXmlElement != NULL){
    if(getTagTxt(lightGroupXmlElement->FirstChildElement(), lgSearchTags, lgConfigTxtBuff, sizeof(lgSearchTags)/4)){
      Log.fatal("lgsDecoder::on_configUpdate: No lightGroupXml provided - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
    }
    if(lgConfigTxtBuff[XMLLGADDR] == NULL || 
       lgConfigTxtBuff[XMLLGSEQ] == NULL ||
       lgConfigTxtBuff[XMLLGSYSTEMNAME] == NULL ||
       lgConfigTxtBuff[XMLLGUSERNAME] == NULL ||
       lgConfigTxtBuff[XMLLGTYPE] == NULL) {
        Log.fatal("lgsDecoder::on_configUpdate: Failed to parse lightGroupXml - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
    }
    lightGroup_t* lg = new lightGroup_t;
    lg->lightGroupsChannel = this;
    lg->lgAddr = atoi(lgConfigTxtBuff[XMLLGADDR]);
    lg->lgSeq = atoi(lgConfigTxtBuff[XMLLGSEQ]);
    lg->lgSystemName = lgConfigTxtBuff[XMLLGSYSTEMNAME];
    lg->lgUserName = lgConfigTxtBuff[XMLLGUSERNAME];
    lg->lgType = lgConfigTxtBuff[XMLLGTYPE];
    if(!strcmp(lg->lgType, "Signal Mast")){
      if(lightGroupXmlElement->FirstChildElement("LgDesc")==NULL){
        Log.fatal("lgsDecoder::on_configUpdate: LgDesc missing - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
      }
      if((lg->lightGroupObj = new mastDecoder()) == NULL){
        Log.fatal("lgsDecoder::on_configUpdate: Could not create mastDecoder object - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_OUT_OF_MEM_ERR;
      }
      if(lg->lightGroupObj->init()){
        Log.error("lgsDecoder::on_configUpdate: Could not initialize mastDecoder object - continuing..." CR);
      }
      if(lg->lightGroupObj->onConfigure(lg, lightGroupXmlElement->FirstChildElement("LgDesc"))){
        Log.error("lgsDecoder::on_configUpdate: Could not configure mastDecoder object - continuing..." CR);
      }
    }
    // else if OTHER LG TYPES....
    else {
      Log.error("lgsDecoder::on_configUpdate: LG Type %s does is not implemented - continuing..." CR);
    }
    lgList.push_back(lg);
    lightGroupXmlElement = lightGroupXmlElement->NextSiblingElement("Lightgroup");
  }
  opState = OP_CONFIG;
  Log.notice("lgsDecoder::on_configUpdate: Configuration finished" CR);

  return RC_OK;
}

uint8_t lgsDecoder::start(void){
  Log.notice("lgsDecoder::start: Starting lightgroups decoder channel: %d" CR, channel);

  if(opState != OP_CONFIG){
    Log.fatal("lgsDecoder::start: opState is not OP_CONFIG for Light groups decoder channe: %d- rebooting..." CR, channel);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  int prevSeq = -1;
  int foundSeq;
  int seqOffset = 0;
  for(uint16_t i=0; i<lgList.size(); i++){
    int nextSeq = MAXSTRIPLEN;
    for(uint16_t j=0; j<lgList.size(); j++){
      if(lgList.at(j)->lgSeq <= prevSeq){
        continue;
      }
      if(lgList.at(j)->lgSeq < nextSeq){
        nextSeq = lgList.get(j)->lgSeq;
        foundSeq = j;
      }
    }
    prevSeq = nextSeq;
    lgList.at(foundSeq)->lgSeqOffset = seqOffset;
    seqOffset += lgList.at(foundSeq)->lgNoOfLed;
  }
  for(uint16_t i=0; i<lgList.size(); i++){
    lgList.at(i)->lightGroupObj->start();
  }
  xTaskCreatePinnedToCore(
                        updateStripHelper,          // Task function
                        "StripHandler",             // Task function name reference
                        6*1024,                     // Stack size
                        this,                       // Parameter passing
                        UPDATE_STRIP_PRIO,          // Priority 0-24, higher is more
                        NULL,                       // Task handle
                        UPDATE_STRIP_CORE);                    // Core [CORE_0 | CORE_1]

  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(lgsDecoderLock);
  Log.notice("lgsDecoder::start: licghtgroupsDecoder - channel %d and all its lightgroupDecoders have started" CR, channel);
  return RC_OK;
}

uint8_t lgsDecoder::updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, const uint8_t* p_wantedValueBuff, const uint16_t* p_transitionTimeBuff){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  for(uint16_t i=0; i<p_buffLen; i++){
    stripCtrlBuff[i+p_seqOffset].incrementValue = floor(abs(p_wantedValueBuff[i] - stripCtrlBuff[i+p_seqOffset].currentValue) / (p_transitionTimeBuff[i]/STRIP_UPDATE_MS));
    stripCtrlBuff[i+p_seqOffset].wantedValue = p_wantedValueBuff[i];
    stripCtrlBuff[i+p_seqOffset].dirty = true;
    bool alreadyDirty = false;
    for(uint16_t j=0; j<lgList.size(); j++){
      if(dirtyList.get(j) == &stripCtrlBuff[i+p_seqOffset]){
        alreadyDirty = true;
        break;
      }
    }
    if(!alreadyDirty){
      dirtyList.push_back(&stripCtrlBuff[i+p_seqOffset]);
    }
  }
  xSemaphoreGive(lgsDecoderLock);
  return RC_OK;
}

void lgsDecoder::updateStripHelper(void* p_lgsObject){
  ((lgsDecoder*)p_lgsObject)->updateStrip();
  return;
}

void lgsDecoder::updateStrip(void){
  int currentValue;
  int wantedValue;
  int incrementValue;
  int64_t nextLoopTime = esp_timer_get_time();
  int64_t thisLoopTime;
  uint32_t startTime;
  uint16_t avgIndex = 0;
  int64_t latency = 0;
  uint32_t runtime = 0;
  maxLatency = 0;
  maxRuntime = 0;
  overRuns = 0;
  uint32_t maxAvgIndex = floor(UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS);
  uint32_t loopTime = STRIP_UPDATE_MS * 1000;

  Log.verbose("Starting sriphandler channel %d" CR, channel);
  while(true){
    xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
    startTime = esp_timer_get_time();
    thisLoopTime = nextLoopTime;
    nextLoopTime += loopTime;
    if(avgIndex >=  maxAvgIndex){
      avgIndex = 0;
    }
    latency = startTime - thisLoopTime;
    latencyVect[avgIndex] = latency;
    if(latency > maxLatency) {
      maxLatency = latency;
    }
    if(dirtyList.size() > 0){ //Test code diablement
//    if(true){                   //Test code

      if(!strip->canShow()){
        overRuns++;
        Log.verbose("Couldnt update strip channel %d, continuing..." CR, channel);
      }
      else{
        for(int i=0; i<dirtyList.size(); i++){
          currentValue = (int)dirtyList.get(i)->currentValue;
          wantedValue = (int)dirtyList.get(i)->wantedValue;
          incrementValue = (int)dirtyList.get(i)->incrementValue;
          if(wantedValue > currentValue){
            currentValue += dirtyList.get(i)->incrementValue;
            if(currentValue > wantedValue){
              currentValue = wantedValue;
            }
          } else{
            currentValue -= dirtyList.get(i)->incrementValue;
            if(currentValue < wantedValue){
                currentValue = wantedValue;
            }
          }
          stripWritebuff[dirtyList.get(i)-stripCtrlBuff] = currentValue;
          dirtyList.get(i)->currentValue = (uint8_t)currentValue;
          if(wantedValue == currentValue){
            dirtyList.get(i)->dirty = false;
            dirtyList.clear(i);
          }
        }
        strip->show(); //WE NEED TO FIX PIN ASSIGNMENT
      }
    }
    runtime =  esp_timer_get_time() - startTime;
    runtimeVect[avgIndex] = runtime;
    if(runtime > maxRuntime){
      maxRuntime = runtime;
    }
    TickType_t delay;
    if((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
      xSemaphoreGive(lgsDecoderLock);
      vTaskDelay((delay/1000)/portTICK_PERIOD_MS);
    }
    else{
      Log.verbose("Strip channel %d overrun" CR, channel);
      overRuns++;
      xSemaphoreGive(lgsDecoderLock);
      nextLoopTime = esp_timer_get_time();
    }
    avgIndex++;
  }
}

uint32_t lgsDecoder::getOverRuns(void){
  unsigned long tmpOverRuns;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  tmpOverRuns = overRuns;
  xSemaphoreGive(lgsDecoderLock);
  return tmpOverRuns;
}

void lgsDecoder::clearOverRuns(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  overRuns = 0;
  xSemaphoreGive(lgsDecoderLock);
  return;
}

int64_t lgsDecoder::getMeanLatency(void){
  int64_t accLatency = 0;
  int64_t meanLatency;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  for(uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++){
    accLatency += latencyVect[avgIndex];
    xSemaphoreGive(lgsDecoderLock);
  }
  meanLatency = accLatency/avgSamples;
  return meanLatency;
}

int64_t lgsDecoder::getMaxLatency(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  int64_t tmpMaxLatency = maxLatency;
  xSemaphoreGive(lgsDecoderLock);
  return tmpMaxLatency;
}

void lgsDecoder::clearMaxLatency(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  maxLatency = -1000000000; //NEEDS FIX
  xSemaphoreGive(lgsDecoderLock);
  return;
}

uint32_t lgsDecoder::getMeanRuntime(void){
  uint32_t accRuntime = 0;
  uint32_t meanRuntime;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  for(uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++){
    accRuntime += runtimeVect[avgIndex];
    xSemaphoreGive(lgsDecoderLock);
  }
  meanRuntime = accRuntime/avgSamples;
  return meanRuntime;
}

uint32_t lgsDecoder::getMaxRuntime(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  uint32_t tmpMaxRuntime = maxRuntime;
  xSemaphoreGive(lgsDecoderLock);
  return tmpMaxRuntime;
}

void lgsDecoder::clearMaxRuntime(void){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  maxRuntime = 0;
  xSemaphoreGive(lgsDecoderLock);
  return;
}

flash* lgsDecoder::getFlashObj(uint8_t p_flashType){
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  if(opState == OP_CREATED || opState == OP_INIT || opState == OP_FAIL) {
    Log.error("lgsDecoder::getFlashObj: opState %d does not allow to provide flash objects - returning NULL - and continuing..." CR, opState);
    xSemaphoreGive(lgsDecoderLock);
    return NULL;
  }
  xSemaphoreGive(lgsDecoderLock);
  switch(p_flashType){
    case SM_FLASH_TYPE_SLOW:
    return FLASHSLOW;
    break;
    
    case SM_FLASH_TYPE_NORMAL:
    return FLASHNORMAL;
    break;

    case SM_FLASH_TYPE_FAST:
    return FLASHFAST;
    break;
  }
}

uint8_t lgsDecoder::getOpState(void){
  uint8_t tmpOpState;
  xSemaphoreTake(lgsDecoderLock, portMAX_DELAY);
  tmpOpState = opState;
  xSemaphoreGive(lgsDecoderLock);
  return tmpOpState;
}

/*==============================================================================================================================================*/
/* END Class lgsDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
flash::flash(float p_freq, uint8_t p_duty){
  flashData = new flash_t;
  if(p_duty > 99){
    p_duty = 99;
  }
  flashData->onTime = uint32_t((float)(1/(float)p_freq)*(float)((float)p_duty/100)*1000000);
  flashData->offTime = uint32_t((float)(1/(float)p_freq)*(float)((100-(float)p_duty)/100)*1000000);
  Log.notice("flash::flash: Creating flash object %d with flash frequency %d Hz and dutycycle %d" CR, this, p_freq, p_duty);
  flashData->flashState = true;
  flashLock = xSemaphoreCreateMutex();
  overRuns = 0;
  maxLatency = 0;
  maxAvgSamples = p_freq * FLASH_LATENCY_AVG_TIME;
  latencyVect = new uint32_t[maxAvgSamples];

  xTaskCreatePinnedToCore(
                          flashLoopStartHelper,       // Task function
                          "FlashLoop",                // Task function name reference
                          6*1024,                     // Stack size
                          this,                       // Parameter passing
                          FLASH_LOOP_PRIO,            // Priority 0-24, higher is more
                          NULL,                       // Task handle
                          FLASH_LOOP_CORE);           // Core [CORE_0 | CORE_1]
  opState = OP_WORKING;
}
  
flash::~flash(void){
// need to stop the timer task
  delete flashData; //Much more to do/delete
  delete latencyVect;
}

uint8_t flash::subscribe(flashCallback_t p_callback, void* p_args){
  Log.notice("flash::subcribe: Subscribing to flash object %d with callback %d" CR, this, p_callback);
  callbackSub_t* newCallbackSub = new callbackSub_t;
  xSemaphoreTake(flashLock, portMAX_DELAY);
  flashData->callbackSubs.push_back(newCallbackSub);
  flashData->callbackSubs.back()->callback = p_callback;
  flashData->callbackSubs.back()->callbackArgs = p_args;
  xSemaphoreGive(flashLock);
  return RC_OK;
}

uint8_t flash::unSubscribe(flashCallback_t p_callback){
  Log.info("flash::unSubcribe: Unsubscribing flash callback %d from flash object %d" CR, p_callback, this);
  uint16_t i = 0;
  bool found = false;
  xSemaphoreTake(flashLock, portMAX_DELAY);
  for(i=0; true; i++){
    if(i >= flashData->callbackSubs.size()){
      break;
    }
    if(flashData->callbackSubs.get(i)->callback == p_callback)
      found = true;
      Log.info("flash::unSubcribe: Deleting flash subscription %d from flash object %d" CR, p_callback, this);
      delete flashData->callbackSubs.get(i);
      flashData->callbackSubs.clear(i);
  }
  if(found){
    return RC_OK;
  }
  else{
    Log.error("flash::unSubcribe: Could not find flash subscription %d from flash object %d to delete" CR, p_callback, this);
    return RC_NOT_FOUND_ERR;
  }
}

void flash::flashLoopStartHelper(void* p_flashObject){
  ((flash*)p_flashObject)->flashLoop();
  return;
}

void flash::flashLoop(void){
  int64_t  nextLoopTime = esp_timer_get_time();
  int64_t  thisLoopTime;
  uint16_t latencyIndex = 0;
  uint32_t latency = 0;
  Log.notice("flash::flashLoop: Starting flash object %d" CR, this);
  while(true){
    thisLoopTime = nextLoopTime;
    if(flashData->flashState){
        nextLoopTime += flashData->offTime;
        flashData->flashState = false;
    }
    else{
      nextLoopTime += flashData->onTime;
      flashData->flashState = true;
    }
    for(uint16_t i=0; i<flashData->callbackSubs.size(); i++){
      flashData->callbackSubs.get(i)->callback(flashData->flashState, flashData->callbackSubs.get(i)->callbackArgs);
    }
    if(latencyIndex >= maxAvgSamples) {
      latencyIndex = 0;
    }
    xSemaphoreTake(flashLock, portMAX_DELAY);
    latency = esp_timer_get_time()-thisLoopTime;
    latencyVect[latencyIndex++] = latency;
    if(latency > maxLatency) {
      maxLatency = latency;
    }
    xSemaphoreGive(flashLock);
    TickType_t delay;
    if((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
      vTaskDelay((delay/1000)/portTICK_PERIOD_MS);
    }
    else{
      Log.verbose("flash::flashLoop: Flash object %d overrun" CR, this);
      xSemaphoreTake(flashLock, portMAX_DELAY);
      overRuns++;
      xSemaphoreGive(flashLock);
      nextLoopTime = esp_timer_get_time();
    }
  }
}

int flash::getOverRuns(void){
  xSemaphoreTake(flashLock, portMAX_DELAY);
  uint32_t tmpOverRuns = overRuns;
  xSemaphoreGive(flashLock);
  return tmpOverRuns;
}

void flash::clearOverRuns(void){
  xSemaphoreTake(flashLock, portMAX_DELAY);
  overRuns = 0;
  xSemaphoreGive(flashLock);
  return;
}

uint32_t flash::getMeanLatency(void){
  uint32_t* tmpLatencyVect = new uint32_t[maxAvgSamples];
  uint32_t accLatency;
  uint32_t meanLatency;
  xSemaphoreTake(flashLock, portMAX_DELAY);
  memcpy(tmpLatencyVect, latencyVect, maxAvgSamples);
  xSemaphoreGive(flashLock);
  for(uint16_t latencyIndex = 0; latencyIndex < maxAvgSamples; latencyIndex++){
    accLatency += tmpLatencyVect[latencyIndex];
  }
  meanLatency = accLatency/maxAvgSamples;
  delete tmpLatencyVect;
  return meanLatency;
}

uint32_t flash::getMaxLatency(void){
  xSemaphoreTake(flashLock, portMAX_DELAY);
  uint32_t tmpMaxLatency = maxLatency;
  xSemaphoreGive(flashLock);
  return tmpMaxLatency;
}

void flash::clearMaxLatency(void){
  xSemaphoreTake(flashLock, portMAX_DELAY);
  maxLatency = 0;
  xSemaphoreGive(flashLock);
  return;
}

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
/*
# XML schema fragment:
#           <Aspects>
#        <Aspect>
#         <AspectName>Stopp</AspectName>
#         <Mast>
#           <Type>Sweden-3HMS:SL-5HL></Type>
#           <Head>UNLIT</Head>
#           <Head>LIT</Head>
#           <Head>UNLIT</Head>
#           <Head>UNLIT</Head>
#           <Head>UNLIT</Head>
#           <NoofPxl>6</NoofPxl>
#         </Mast>
#         <Mast>
#                    .
#                    .
#         </Mast>
#       </Aspect>
#           </Aspects>
*/

uint8_t signalMastAspects::opState = OP_INIT;
uint8_t signalMastAspects::failsafeMastAppearance[SM_MAXHEADS];
QList<aspects_t*> signalMastAspects::aspects;

uint8_t signalMastAspects::onConfigUpdate(tinyxml2::XMLElement* p_smAspectsXmlElement){
  if(opState != OP_INIT){
    Log.fatal("signalMastAspects::onConfigure: opState is not OP_INIT - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_GEN_ERR;
  }
  Log.notice("signalMastAspects::onConfigure: Configuring Mast aspects" CR);
  for(uint8_t i=0; i<SM_MAXHEADS; i++){
    failsafeMastAppearance[i] = UNUSED_APPEARANCE;
   }
  tinyxml2::XMLElement* smAspectsXmlElement = p_smAspectsXmlElement;
  if((smAspectsXmlElement=smAspectsXmlElement->FirstChildElement("Aspects")) == NULL || (smAspectsXmlElement=smAspectsXmlElement->FirstChildElement("Aspect"))==NULL || smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL){
    Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Aspects, Aspect, or AspectName - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_PARSE_ERR;
  }
  //Outer loop itterating all signal mast aspects
  for(uint8_t i=0; true; i++){
    Log.notice("signalMastAspects::onConfigure: Parsing Signal mast Aspect: %s" CR, smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
    aspects_t* newAspect = new aspects_t;
    newAspect->name = new char[strlen(smAspectsXmlElement->FirstChildElement("AspectName")->GetText())+1];
    strcpy(newAspect->name, smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
    aspects.push_back(newAspect);
    
    //Mid loop itterating the XML mast types for the aspect
    tinyxml2::XMLElement* mastTypeAspectXmlElement = smAspectsXmlElement->FirstChildElement("Mast");
    if( mastTypeAspectXmlElement == NULL || mastTypeAspectXmlElement->FirstChildElement("Type") == NULL || mastTypeAspectXmlElement->FirstChildElement("Type")->GetText() == NULL){
      Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Mast or Type - rebooting..." CR);
      opState = OP_FAIL;
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
    }
    for(uint16_t j=0; true; j++){
      if(mastTypeAspectXmlElement == NULL){
        break;
      }
      Log.notice("signalMastAspects::onConfigure: Parsing Mast type %s for Aspect %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());

      //Inner loop creating head aspects for a particular mast type
      uint8_t k = 0;
      bool mastAlreadyExist = false;
      for(k=0; k<aspects.back()->mastTypes.size(); k++){
        if(!strcmp(aspects.back()->mastTypes.at(k)->name, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText())){
          mastAlreadyExist = true;
          break;
        }
      }
      if(mastAlreadyExist){
        Log.warning("signalMastAspects::onConfigure: Parsing Mast type %s for Aspect %s already exists, skipping..." CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
        break;
      }
      else { //Creating a new signal mast type'
        Log.notice("signalMastAspects::onConfigure: Creating Mast type %s" CR, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
        aspects.back()->mastTypes.push_back(new mastType_t);
        char* newMastTypeName = new char[strlen(mastTypeAspectXmlElement->FirstChildElement("Type")->GetText())+1];
        strcpy(newMastTypeName, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
        aspects.back()->mastTypes.back()->name = newMastTypeName;
        aspects.back()->mastTypes.back()->noOfUsedHeads = atoi(mastTypeAspectXmlElement->FirstChildElement("NoofPxl")->GetText());
        aspects.back()->mastTypes.back()->noOfHeads = ceil(aspects.back()->mastTypes.back()->noOfUsedHeads/3)*3;
      }
      tinyxml2::XMLElement* headXmlElement = mastTypeAspectXmlElement->FirstChildElement("Head");
      if(headXmlElement == NULL || headXmlElement->GetText() == NULL){
        Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing Head - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
      }
      Log.notice("signalMastAspects::onConfigure: Adding Asspect %s to MastType %s" CR, smAspectsXmlElement->FirstChildElement("AspectName")->GetText(), mastTypeAspectXmlElement->FirstChildElement("Type")->GetText());
      for(uint8_t p=0; p<SM_MAXHEADS; p++){
        if(headXmlElement == NULL){
          Log.notice("signalMastAspects::onConfigure: No more Head appearances, padding up with UNUSED_APPEARANCE from Head %d" CR, p);
          for(uint8_t r=p; r < SM_MAXHEADS; r++){
            aspects.back()->mastTypes.back()->headAspects[r] = UNUSED_APPEARANCE;
          }
          break;
        }
        if(!strcmp(headXmlElement->GetText(), "LIT")){
          Log.notice("signalMastAspects::onConfigure: Adding LIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
          aspects.back()->mastTypes.back()->headAspects[p] = LIT_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "UNLIT")){
          Log.notice("signalMastAspects::onConfigure: Adding UNLIT_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
          aspects.back()->mastTypes.back()->headAspects[p] = UNLIT_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "FLASH")){
          Log.notice("signalMastAspects::onConfigure: Adding FLASH_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
          aspects.back()->mastTypes.back()->headAspects[p] = FLASH_APPEARANCE;
        }
        if(!strcmp(headXmlElement->GetText(), "UNUSED")){
          Log.notice("signalMastAspects::onConfigure: Adding UNUSED_APPEARANCE for head %d, MastType %s and Appearance %s" CR, p, mastTypeAspectXmlElement->FirstChildElement("Type")->GetText(), smAspectsXmlElement->FirstChildElement("AspectName")->GetText());
          aspects.back()->mastTypes.back()->headAspects[p] = UNUSED_APPEARANCE;
        }
        headXmlElement = headXmlElement->NextSiblingElement("Head");
      }
      //End inner loop
      mastTypeAspectXmlElement = mastTypeAspectXmlElement->NextSiblingElement("Mast");
    }
    //End middle loop

    if((smAspectsXmlElement = smAspectsXmlElement->NextSiblingElement("Aspect")) == NULL){
      break;
    }
    if(smAspectsXmlElement->FirstChildElement("AspectName") == NULL || smAspectsXmlElement->FirstChildElement("AspectName")->GetText() == NULL){
      Log.fatal("signalMastAspects::onConfigure: XML parsing error, missing AspectName - rebooting..." CR);
        opState = OP_FAIL;
        //failsafe();
        ESP.restart();
        return RC_PARSE_ERR;
    }
  }
  //End outer loop

  dumpConfig();
  opState = OP_WORKING;
  return RC_OK;
}

void signalMastAspects::dumpConfig(void){
    Log.notice("signalMastAspects::dumpConfig: <Aspect config dump Begin>" CR);
  for(uint8_t i=0; i<aspects.size(); i++){
    Log.notice("signalMastAspects::dumpConfig:     <Aspect: %s>" CR, aspects.at(i)->name);
    for(uint8_t j=0; j<aspects.at(i)->mastTypes.size(); j++){
      Log.notice("signalMastAspects::dumpConfig:        <Mast type: %s>" CR, aspects.at(i)->mastTypes.at(j)->name);
      for(uint8_t k=0; k<SM_MAXHEADS; k++){
        switch(aspects.at(i)->mastTypes.at(j)->headAspects[k]){
          case LIT_APPEARANCE:
            Log.notice("signalMastAspects::dumpConfig:            <Head %d: LIT>" CR, k);
            break;
          case UNLIT_APPEARANCE:
            Log.notice("signalMastAspects::dumpConfig:            <Head %d: UNLIT>" CR, k);
            break;
          case FLASH_APPEARANCE:
            Log.notice("signalMastAspects::dumpConfig:            <Head %d: FLASHING>" CR, k);
            break;
          case UNUSED_APPEARANCE:
            Log.notice("signalMastAspects::dumpConfig:            <Head %d: UNUSED>" CR, k);
            break;
          default:
            Log.notice("signalMastAspects::dumpConfig:            <Head %d: UNKNOWN>" CR, k);
            break;
        }
      }
    }     
  }
}

uint8_t signalMastAspects::getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance){
  if(opState != OP_WORKING){
    Log.error("signalMastAspects::getAppearance: OP_State is not OP_WORKING, doing nothing..." CR);
    *p_appearance = NULL;
    return RC_GEN_ERR;
  }
  if(!strcmp(p_aspect, "FAULT") || !strcmp(p_aspect, "UNDEFINED")){
    *p_appearance = failsafeMastAppearance;
    return RC_OK;
  }
  for(uint8_t i=0; true; i++){
    if(i>aspects.size()-1){
      Log.error("signalMastAspects::getAppearance: Aspect doesnt exist, setting mast to failsafe appearance and continuing..." CR);
      *p_appearance = failsafeMastAppearance;
      return RC_GEN_ERR;
    }
    if(!strcmp(p_aspect, aspects.at(i)->name)){
      for(uint8_t j=0; true; j++){
        if(j>aspects.at(i)->mastTypes.size()-1){
          Log.error("signalMastAspects::getAppearance: Mast type doesnt exist, setting to failsafe appearance and continuing..." CR);
          *p_appearance = failsafeMastAppearance;
          return RC_GEN_ERR;
        }
        if(!strcmp(p_smType, aspects.at(i)->mastTypes.at(j)->name)){
          *p_appearance = aspects.at(i)->mastTypes.at(j)->headAspects;
          return RC_OK;
        }
      }
    } 
  }
}

uint8_t signalMastAspects::getNoOfHeads(char* p_smType){
  if(opState != OP_WORKING){
    Log.error("signalMastAspects::getNoOfHeads: OP_State is not OP_WORKING, doing nothing..." CR);
    return RC_GEN_ERR;
  }
  for(uint8_t i=0; i<aspects.size(); i++){
    for (uint8_t j=0; j<aspects.at(i)->mastTypes.size(); j++){
      if(!strcmp(aspects.at(i)->mastTypes.at(j)->name, p_smType)){
        return aspects.at(i)->mastTypes.at(j)->noOfHeads;
      }
    }
  }
  Log.error("signalMastAspects::getNoOfHeads: Mast type not found, doing nothing..." CR);
  return RC_GEN_ERR;
}

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mastDecoder                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
mastDecoder::mastDecoder(void){
  Log.notice("mastDecoder::mastDecoder: Creating mast decoder" CR);
  opState = OP_CREATED;
  uint8_t* tmpAppearance = new uint8_t[SM_MAXHEADS];

}

mastDecoder::~mastDecoder(void){
  Log.fatal("mastDecoder::~mastDecoder: Destructor not supported - rebooting..." CR);
  opState = OP_FAIL;
  //failsafe();
  ESP.restart();
}

uint8_t mastDecoder::init(void){
  Log.notice("mastDecoder::init: Initializing mast decoder" CR);
  mastDecoderLock = xSemaphoreCreateMutex();
  mastDecoderReentranceLock = xSemaphoreCreateMutex();
  if(mastDecoderLock == NULL || mastDecoderReentranceLock == NULL){
    Log.fatal("mastDecoder::init: Could not create Lock objects - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_OUT_OF_MEM_ERR;
  }
  strcpy(aspect, "FailSafe");
  opState = OP_INIT;
  return RC_OK;
}

uint8_t mastDecoder::onConfigure(lightGroup_t* p_genLgDesc, tinyxml2::XMLElement* p_mastDescXmlElement){
  if(opState != OP_INIT) {
    Log.fatal("mastDecoder::onConfigure: Reconfigurarion not supported - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_NOTIMPLEMENTED_ERR;
  }
  genLgDesc = p_genLgDesc; //Note SHARED DATA
  const char* searchMastTags[4];
  searchMastTags[SM_TYPE] = "SmType";
  searchMastTags[SM_DIMTIME] = "SmDimTime";
  searchMastTags[SM_FLASHFREQ] = "SmFlashFreq";
  searchMastTags[SM_BRIGHTNESS] = "SmBrightness";
  if(p_mastDescXmlElement == NULL){
    Log.fatal("mastDecoder::onConfigure: No mastDescXml provided - rebooting..." CR);
    opState = OP_FAIL;
    //failsafe();
    ESP.restart();
    return RC_PARSE_ERR;
  }
  char* mastDescBuff[4];
  getTagTxt(p_mastDescXmlElement->FirstChildElement(), searchMastTags, mastDescBuff, sizeof(searchMastTags)/4); // Need to fix the addressing for portability
  if(mastDescBuff[SM_TYPE] == NULL ||
     mastDescBuff[SM_DIMTIME] == NULL ||
     mastDescBuff[SM_FLASHFREQ] == NULL ||
     mastDescBuff[SM_BRIGHTNESS] == NULL) {
      Log.fatal("mastDecoder::onConfigure: mastDescXml missformated - rebooting..." CR);
      //failsafe();
      ESP.restart();
      return RC_PARSE_ERR;
  }
  mastDesc = new mastDesc_t;
  mastDesc->smType = mastDescBuff[SM_TYPE];
  mastDesc->smDimTime = mastDescBuff[SM_DIMTIME];
  mastDesc->smFlashFreq = mastDescBuff[SM_FLASHFREQ];
  mastDesc->smBrightness = mastDescBuff[SM_BRIGHTNESS];
  genLgDesc->lgNoOfLed = signalMastAspects::getNoOfHeads(mastDesc->smType);
  appearance = new uint8_t[genLgDesc->lgNoOfLed];
  appearanceWriteBuff = new uint8_t[genLgDesc->lgNoOfLed];
  appearanceDimBuff = new uint16_t[genLgDesc->lgNoOfLed];
  if(!strcmp(mastDesc->smDimTime, "NORMAL")){
    smDimTime = SM_DIM_NORMAL;
  }
  else if(!strcmp(mastDesc->smDimTime, "FAST")){
    smDimTime = SM_DIM_FAST;
  }
  else if(!strcmp(mastDesc->smDimTime, "SLOW")){
    smDimTime = SM_DIM_SLOW;
  }
  else {
    Log.error("mastDecoder::onConfigure: smDimTime is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    smDimTime = SM_DIM_NORMAL;
  }
  if(!strcmp(mastDesc->smBrightness, "HIGH")){
    smBrightness = SM_BRIGHNESS_HIGH;
    
  }
  else if(!strcmp(mastDesc->smBrightness, "NORMAL")){
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  else if(!strcmp(mastDesc->smBrightness, "LOW")){
    smBrightness = SM_BRIGHNESS_LOW;
  }
  else{
    Log.error("mastDecoder::onConfigure: smBrighness is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  opState = OP_CONFIG;
  return RC_OK;
}

uint8_t mastDecoder::start(void){
  Log.notice("mastDecoder::start: Starting mast decoder %s" CR, genLgDesc->lgSystemName);
  if(strcmp(mastDesc->smFlashFreq, "FAST")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_FAST)->subscribe(mastDecoder::onFlashHelper, this);
  } 
  else if(strcmp(mastDesc->smFlashFreq, "NORMAL")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else if(strcmp(mastDesc->smFlashFreq, "SLOW")){
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_SLOW)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else{
    Log.error("mastDecoder::start: smFlashFreq is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    genLgDesc->lightGroupsChannel->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  char lgAddrTxtBuff[5];
  const char* subscribeTopic[5] = {MQTT_ASPECT_TOPIC, mqtt::getUri(), "/", itoa(genLgDesc->lgAddr, lgAddrTxtBuff, 10), "/"};
  mqtt::subscribeTopic(concatStr(subscribeTopic, 5), mastDecoder::onAspectChangeHelper, this);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  opState = OP_WORKING;
  xSemaphoreGive(mastDecoderLock);
  return RC_OK;
}

uint8_t mastDecoder::stop(void){
  Log.fatal("mastDecoder::stop: stop not supported - rebooting..." CR);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  opState = OP_FAIL;
  xSemaphoreGive(mastDecoderLock);
  //failsafe();
  ESP.restart();
  return RC_NOTIMPLEMENTED_ERR;
}

void mastDecoder::onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject){
  ((mastDecoder*)p_mastObject)->onAspectChange(p_topic, p_payload);
}

void mastDecoder::onAspectChange(const char* p_topic, const char* p_payload){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  if(opState != OP_WORKING){
    xSemaphoreGive(mastDecoderLock);
    xSemaphoreGive(mastDecoderReentranceLock);
    Log.error("mastDecoder::onAspectChange: A new aspect received, but mast decoder opState is not OP_WORKING - continuing..." CR);
    return;
  }
  xSemaphoreGive(mastDecoderLock);
  if(parseXmlAppearance(p_payload, aspect)){
    xSemaphoreGive(mastDecoderReentranceLock);
    Log.error("mastDecoder::onAspectChange: Failed to parse appearance - continuing..." CR);
    return;
  }
  Log.verbose("mastDecoder::onAspectChange: A new aspect: %s received for signal mast %s" CR, aspect, genLgDesc->lgSystemName);

  signalMastAspects::getAppearance(mastDesc->smType, aspect, &tmpAppearance);
  for(uint8_t i=0; i<genLgDesc->lgNoOfLed; i++){
    appearance[i] = tmpAppearance[i];
    appearanceDimBuff[i] = smDimTime;
    switch(appearance[i]){
      case LIT_APPEARANCE:
        appearanceWriteBuff[i] = smBrightness;
        break;
      case UNLIT_APPEARANCE:
        appearanceWriteBuff[i] = 0;
        break;
      case UNUSED_APPEARANCE:
        appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
        break;
      case FLASH_APPEARANCE:
        if(flashOn){
          appearanceWriteBuff[i] = smBrightness;
        } else{
          appearanceWriteBuff[i] = 0;
        }
        break;
      default:
        Log.error("mastDecoder::onAspectChange: The appearance is none of LIT, UNLIT, FLASH or UNUSED - setting head to SM_BRIGHNESS_FAIL and continuing..." CR);
        appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL;
        appearanceWriteBuff[i] = 0;
    }
  }
  genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset, genLgDesc->lgNoOfLed, appearanceWriteBuff, appearanceDimBuff);
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
}

uint8_t mastDecoder::parseXmlAppearance(const char* p_aspectXml, char* p_aspect){
  tinyxml2::XMLDocument aspectXmlDocument;
  if(aspectXmlDocument.Parse(p_aspectXml) || aspectXmlDocument.FirstChildElement("Aspect") == NULL || aspectXmlDocument.FirstChildElement("Aspect")->GetText() == NULL){
    Log.error("mastDecoder::parseXmlAppearance: Failed to parse the new aspect - continuing..." CR);
    return RC_PARSE_ERR;
  }
  strcpy(p_aspect, aspectXmlDocument.FirstChildElement("Aspect")->GetText());
  return RC_OK;
}

void mastDecoder::onFlashHelper(const bool p_flashState, void* p_flashObj){
  ((mastDecoder*)p_flashObj)->onFlash(p_flashState);
}

void mastDecoder::onFlash(const bool p_flashState){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  flashOn = p_flashState;
  for (uint16_t i=0; i<genLgDesc->lgNoOfLed; i++){
    if(appearance[i] == FLASH_APPEARANCE){
      if(flashOn){
        genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset + i, (uint8_t)1, &smBrightness, &smDimTime);
      }else{
        uint8_t zero = 0;
        genLgDesc->lightGroupsChannel->updateLg(genLgDesc->lgSeqOffset + i, (uint8_t)1, &zero, &smDimTime);
      }
    }
  }
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
}

uint8_t mastDecoder::getOpState(void){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  uint8_t tmpOpState = opState;
  xSemaphoreGive(mastDecoderReentranceLock);
  return tmpOpState;
}

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: setup                                                                                                                               */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while(!Serial && !Serial.available()){}
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
//  Log.setPrefix(printPrefix); // set prefix similar to NLog
  Log.notice("Logging started towards Serial" CR);
  cpu::init();
  networking::start();
  uint8_t wifiWait = 0;
  while(networking::opState != OP_WORKING){
    if(wifiWait >= 60) {
      Log.fatal("Could not connect to wifi - rebooting..." CR);
      ESP.restart();
    } else {
      Log.notice("Waiting for WIFI to connect" CR);
    }
    wifiWait++;
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  Log.notice("WIFI connected" CR);
  CLI.init();
  topDecoder::init();
  topDecoder::start();
}

/*==============================================================================================================================================*/
/* END setup                                                                                                                                    */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* ARDUINO: loop                                                                                                                                */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void loop() {
  // put your main code here, to run repeatedly:
  // Serial.print("Im in the background\n");
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
/*==============================================================================================================================================*/
/* END loop                                                                                                                                     */
/*==============================================================================================================================================*/
