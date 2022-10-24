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
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Functions: Helper functions                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/




const uint8_t transformLogLevelXmlStr2Int(const char* p_loglevelXmlTxt) {
    if (!strcmp(p_loglevelXmlTxt, "DEBUG-VERBOSE"))
        return DEBUG_VERBOSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-TERSE"))
        return DEBUG_TERSE;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-INFO"))
        return DEBUG_INFO;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-ERROR"))
        return DEBUG_ERROR;
    else if (!strcmp(p_loglevelXmlTxt, "DEBUG-PANIC"))
        return DEBUG_PANIC;
    else
        return RC_GEN_ERR;
}

const char* transformLogLevelInt2XmlStr(uint8_t p_loglevelInt) {
    if (p_loglevelInt == , DEBUG_VERBOSE)
        return "DEBUG-VERBOSE";
    else if (p_loglevelInt == , DEBUG_TERSE)
        return "DEBUG-TERSE";
    else if (p_loglevelInt == , DEBUG_INFO)
        return "DEBUG-INFO";
    else if (p_loglevelInt == , DEBUG_ERROR)
        return "DEBUG-ERROR";
    else if (p_loglevelInt == , DEBUG_PANIC)
        return "DEBUG-PANIC";
    else
        return NULL;
}



void reboot(TimerHandle_t pxTimer DUMMY) {
    Log.info("reboot: rebooting immediatly..." CR);
    ESP.restart();
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

//application specific watchdog - TODO


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
  wdtTimer_args.arg = this;
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
/* Class: decoder                                                                                                                               */
/* Purpose: The "decoder" class implements a static singlton object responsible for setting up the common decoder mqtt class objects,           */
/*          subscribing to the management configuration topic, parsing the top level xml configuration and forwarding propper xml               */
/*          configuration segments to the different decoder services, E.g. Lightgroup links. Lightgroups [Signal Masts | general Lights |       */
/*          sequencedLights], Satelite Links, Satelites, Sensors, Actueators, etc.                                                              */
/*          Turnouts or sensors...                                                                                                              */
/*          The "decoder" sequences the start up of the the different decoder services. It also holds the decoder infrastructure config such    */
/*          as ntp-, rsyslog-, ntp-, watchdog- and cli configuration and is the cooridnator and root of such servicies.                         */
/* Methods:                                                                                                                                     */
/*     public:                                                                                                                                  */
/*          const uint8_t decoder::init(void): Initializes the decoder and all it's data-structures, initializes common services, performs a    */
/*                                       discovery process, and requests the decoder configuration from the server                              */
/*                                       The method blocks until it successfully got a valid configuration.                                     */
/*                                       Params: -                                                                                              */
/*                                       Returns: const ERR Return Codes                                                                        */
/*                                                                                                                                              */
/*          void decoder::onConfigUpdate(const char* p_topic, const char* p_payload, const void* p_dummy): Config callback                      */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: const char* p_topic: MQTT config topic                                                        */
/*                                                const char* p_payload: Configuration payload                                                  */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          const uint8_t decoder::start(void): Starts the decoder services, if called before the decoder configuration successfully have been  */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: -                                                                                             */
/*                                        Returns: const ERR Return Codes                                                                       */
/*                                                                                                                                              */
/*          void decoder::onSystateChange(const uint16_t p_sysState): system/opstate change callback ...................                        */
/*                                        and validated, it will block until a configuration have been received and successfully validated      */
/*                                        Params: const uint16_t p_sysState: New system state                                                   */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          void decoder::onOpStateChange(const char* p_topic, const char* p_payload, const void* DUMMY): opstatechange callback from server    */
/*                                        Params: const char* p_topic: MQTT opstate change topic                                                */
/*                                                const char* p_payload: MQTT opstate payload                                                   */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          void decoder::onAdmStateChange(const char* p_topic, const char* p_payload, const void* DUMMY): admstatechange callback from server  */
/*                                        Params: const char* p_topic: MQTT admstate change topic                                               */
/*                                                const char* p_payload: MQTT admstate payload                                                  */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*          uint8_t decoder::getSysState(void): Get system state of decoder                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const Operational state bitmap                                                               */
/*                                                                                                                                              */
/*          const char* decoder::getMqttURI(void): Get the MQTT brooker URI                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT URI string pointer reference                                                      */
/*                                                                                                                                              */
/*          const uint16_t decoder::getMqttPort(void): get the MQTT brooker port                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT port                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getMqttPrefix(void): get the MQTT topic prefix                                                                 */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT topic string pointer reference                                                    */
/*                                                                                                                                              */
/*          const float decoder::getKeepAlivePeriod(void): get the MQTT keep-alive period                                                       */
/*                                        Params: -                                                                                             */
/*                                        Returns: const MQTT keep-alive period                                                                 */
/*                                                                                                                                              */
/*          const char* decoder::getNtpServer(void): get the NTP server URI                                                                     */
/*                                        Params: -                                                                                             */
/*                                        Returns: const NTP server URI string pointer reference                                                */
/*                                                                                                                                              */
/*          const uint16_t decoder::getNtpPort(void): get the NTP Port                                                                          */
/*                                        Params: -                                                                                             */
/*                                        Returns: NTP Port                                                                                     */
/*                                                                                                                                              */
/*          const uint8_t decoder::getTz(void): get time zone                                                                                   */
/*                                        Params: -                                                                                             */
/*                                        Returns: const time zone                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getLogLevel(void): get loglevel                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const log level string pointer reference                                                     */
/*                                                                                                                                              */
/*          const bool decoder::getFailSafe(void): get fail-safe                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const fail-safe                                                                              */
/*                                                                                                                                              */
/*          const char* decoder::getSystemName(void): get system name                                                                           */
/*                                        Params: -                                                                                             */
/*                                        Returns: const system name string pointer reference                                                   */
/*                                                                                                                                              */
/*          const char* decoder::getUsrName(void): get user name                                                                                */
/*                                        Params: -                                                                                             */
/*                                        Returns: const user name string pointer reference                                                     */
/*                                                                                                                                              */
/*          const char* decoder::getDesc(void): get description                                                                                 */
/*                                        Params: -                                                                                             */
/*                                        Returns: const description string pointer reference                                                   */
/*                                                                                                                                              */
/*          const char* decoder::getMac(void): get decoder MAC address                                                                          */
/*                                        Params: -                                                                                             */
/*                                        Returns: const decoder MAC address string pointer reference                                           */
/*                                                                                                                                              */
/*          const char* decoder::getUri(void): get decoder URI                                                                                  */
/*                                        Params: -                                                                                             */
/*                                        Returns: const decoder URI string pointer reference                                                   */
/*                                                                                                                                              */
/*          void setDebug(const bool p_debug): set object debug status                                                                          */
/*                                        Params: p_debug set debug status                                                                      */
/*                                        Returns: -                                                                                            */
/*                                                                                                                                              */
/*     private:                                                                                                                                 */
/*                                                                                                                                              */
/* Data structures:                                                                                                                             */
/*     public: sysState: Holds the systemState object, wich needs to be reachable by other objects systemState objects                          */
/*     private: UPDATE NEEDED                                                                                                                   */
/*==============================================================================================================================================*/
char** decoder::xmlconfig; //V NEED TO BE UPDATED V
tinyxml2::XMLDocument* decoder::xmlConfigDoc;
SemaphoreHandle_t decoder::decoderLock; 
lgLinks*[NOOF_LGLINKS] decoder::lgLinks;
satLinks* [NOOF_SATLINKS] decoder::satLinks;
satLinks*[2] decoder::satLinks;
cliContext_t* decoder::lightgropsCliContext;

rc_t decoder::init(void):systemState(this) {
  Log.notice("decoder::init: Initializing decoder" CR);
  regSysStateCb(onSystateChange);
  setOpState(OP_INIT | OP_DISCONNECTED | OP_UNDISCOVERED | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
  decoderLock = xSemaphoreCreateMutex();
  if (decoderLock == NULL) {
      panic("decoder::init: Could not create Lock objects - rebooting...");
  Log.notice("decoder::init: Initializing MQTT " CR);
  mqtt::init( (char*)"test.mosquitto.org",  //Broker URI WE NEED TO GET THE BROKER FROM SOMEWHERE
              1883,                         //Broker port
              (char*)"",                    //User name
              (char*)"",                    //Password
              (char*)MQTT_CLIENT_ID,        //Client ID
              QOS_1,                        //QoS
              MQTT_KEEP_ALIVE,              //Keep alive time
              true);                        //Default retain
  mqtt::regStatusCallback(onMqttChange, NULL);
  Log.notice("decoder::init: Waiting for MQTT connection");
  int i = 0;
  while (mqtt::getState() != MQTT_CONNECTED) {
    if(i++ >= 120)
      panic("decoder::init: Could not connect to MQTT broker - rebooting...");
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  unSetOpState(OP_DISCONNECTED);
  Serial.println("");
  Log.notice("decoder::init: MQTT connected");
  Log.notice("decoder::init: Waiting for discovery process");
  i = 0;
  while(mqtt::getUri() == NULL) {
    if(i++ >= 120)
      panic("decoder::init: Discovery process failed - rebooting...");
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  unSetOpState(OP_UNDISCOVERED);
  Serial.println("");
  Log.notice("decoder::init: Discovered");
  Log.notice("decoder::init: Creating lgLinks");
  for (int lgLinkNo = 0, lgLinkNo < NOOF_LGLINKS, lgLinkNo++) {
      lgLinks[lgLinkNo] = new lgLink(lgLinkNo);
      if (lgLinks[lgLinkNo] == NULL)
          panic("decoder::init: Could not create lgLink objects - rebooting...");
      lgLinks[lgLinkNo].init();
  Log.notice("decoder::init: Creating satLinks");
  for (int satLinkNo = 0, satLinkNo < NOOF_SATLINKS, satLinkNo++) {
      satLinks[satLinkNo] = new satLink(satLinkNo);
      if(satLinks[satLinkNo] == NULL)
          panic("decoder::init: Could not create satLink objects - rebooting...");
      satLinks[satLinkNo].init();
  }
  Log.notice("decoder::init: Creating satLinks");
  for (int satLinkNo = 0, satLinkNo < NOOF_SATLINKS, satLinkNo++) {
      satLinks[satLinkNo] = new lgLink(satLinkNo);
      satLinks[satLinkNo].init();
  }
  Log.notice("decoder::init: Subscribing to decoder configuration topic and sending configuration request");
  const char* subscribeTopic[3] = { MQTT_CONFIG_RESP_TOPIC, mqtt::getUri(), "/"};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 3), onConfigUpdate, NULL))
    panic("decoder::init: Failed to suscribe to configuration response topic - rebooting...");
  if (mqtt::sendMsg(MQTT_CONFIG_REQ_TOPIC, CONFIG_REQ_PAYLOAD, false))
      panic("decoder::init: Failed to send configuration request - rebooting...");
  int i;
  Log.notice("decoder::init: Waiting for configuration");
  while(sysState.getOpState() & OP_UNCONFIGURED){
    if(i++ >= 120)
        panic("decoder::init: Did not receive (a valid) configuration - rebooting...");
    Serial.print('.');
    vTaskDelay(500 / portTICK_PERIOD_MS);
    }
  Serial.println("");
  Log.notice("decoder::init: Got valid configuration");
  Log.notice("decoder::init: Initialized");
  return RC_OK;
}

void decoder::onConfigUpdate(const char* p_topic, const char* p_payload, const void* p_dummy){
  if(~(sysState.getOpState() & OP_UNCONFIGURED))
    panic("decoder:onConfigUpdate: Received a configuration, while the it was already configured, dynamic re-configuration not supported, opState: %d - rebooting...", getOpState());
  Log.notice("decoder::onConfigUpdate: Received an uverified configuration, parsing and validating it..." CR);
  xmlconfig = new char*[14];
  xmlconfig[XML_DECODER_MQTT_URI] = new char[strlen(DEFAULT_MQTTURI) + 1];
  strcpy(xmlconfig[XML_DECODER_MQTT_URI], DEFAULT_MQTTURI);
  xmlconfig[XML_DECODER_MQTT_PORT] = new char[strlen(DEFAULT_MQTTPORT) + 1];
  strcpy(xmlconfig[XML_DECODER_MQTT_PORT], DEFAULT_MQTTPORT);
  xmlconfig[XML_DECODER_MQTT_PREFIX] = new char[strlen(DEFAULT_MQTTPREFIX) + 1];
  strcpy(xmlconfig[XML_DECODER_MQTT_PREFIX], DEFAULT_MQTTPREFIX);
  xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = new char[strlen(DEFAULT_MQTTKEEPALIVEPERIOD) + 1];
  strcpy(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD], DEFAULT_MQTTKEEPALIVEPERIOD);
  xmlconfig[XML_DECODER_NTPURI] = new char[strlen(DEFAULT_NTPURI) + 1];
  strcpy(xmlconfig[XML_DECODER_NTPURI], DEFAULT_NTPURI);
  xmlconfig[XML_DECODER_NTPPORT] = new char[strlen(DEFAULT_NTPPORT) + 1];
  strcpy(xmlconfig[XML_DECODER_NTPPORT], DEFAULT_NTPPORT);
  xmlconfig[XML_DECODER_TZ] = new char[strlen(DEFAULT_TZ) + 1];
  strcpy(xmlconfig[XML_DECODER_TZ], DEFAULT_TZ);
  xmlconfig[XML_DECODER_LOGLEVEL] = new char[strlen(DEFAULT_LOGLEVEL) + 1];
  strcpy(xmlconfig[XML_DECODER_LOGLEVEL], DEFAULT_LOGLEVEL);
  xmlconfig[XML_DECODER_FAILSAFE] = new char[strlen(DEFAULT_FAILSAFE) + 1];
  strcpy(xmlconfig[XML_DECODER_FAILSAFE], DEFAULT_FAILSAFE);
  xmlconfig[XML_DECODER_SYSNAME] = NULL
  xmlconfig[XML_DECODER_USRNAME] = NULL
  xmlconfig[XML_DECODER_DESC] = NULL
  xmlconfig[XML_DECODER_MAC] = NULL
  xmlconfig[XML_DECODER_URI] = NULL
  if (xmlConfigDoc.Parse(p_payload))
    panic("decoder::on_configUpdate: Configuration parsing failed - Rebooting...");
  if (xmlConfigDoc.FirstChildElement("genJMRI") == NULL || xmlConfigDoc.FirstChildElement("genJMRI")->FirstChildElement("Decoder") == NULL || xmlConfigDoc.FirstChildElement("genJMRI").FirstChildElement("Decoder").FirstChildElement("SystemName") == NULL)
    panic("decoder::on_configUpdate: Failed to parse the configuration - xml is missformatted - rebooting...");
  const char* decoderSearchTags[14];
  decoderSearchTags[XML_DECODER_MQTT_URI] = "DecoderMqttURI";
  decoderSearchTags[XML_DECODER_MQTT_PORT] = "DecoderMqttPort";
  decoderSearchTags[XML_DECODER_MQTT_PREFIX] = "DecoderMqttTopicPrefix";
  decoderSearchTags[XML_DECODER_MQTT_KEEPALIVEPERIOD] = "DecoderKeepAlivePeriod";
  decoderSearchTags[XML_DECODER_NTPURI] = "NTPServer";
  decoderSearchTags[XML_DECODER_NTPPORT] = "NTPPort";
  decoderSearchTags[XML_DECODER_TZ] = "TIMEZONE";
  decoderSearchTags[XML_DECODER_LOGLEVEL] = "LogLevel";
  decoderSearchTags[XML_DECODER_FAILSAFE] = "DecodersFailSafe";
  decoderSearchTags[XML_DECODER_SYSNAME] = "SystemName";
  decoderSearchTags[XML_DECODER_USRNAME] = "UserName";
  decoderSearchTags[XML_DECODER_DESC] = "Description";
  decoderSearchTags[XML_DECODER_MAC] = "MAC";
  decoderSearchTags[XML_DECODER_URI] = "URI";
  Log.notice("decoder::onConfigUpdate: Parsing decoder configuration:" CR);
  getTagTxt(xmlConfigDoc.FirstChildElement("genJMRI"), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags)/4); // Need to fix the addressing for portability
  getTagTxt(xmlConfigDoc.FirstChildElement("genJMRI").FirstChildElement("Decoder"), decoderSearchTags, xmlconfig, sizeof(decoderSearchTags) / 4); // Need to fix the addressing for portability
  if (xmlconfig[XML_DECODER_SYSNAME] == NULL)
      panic("decoder::onConfigUpdate: System name was not provided - rebooting...")
  Log.notice("decoder::onConfigUpdate: Decoder System name: %s" CR, xmlconfig[XML_DECODER_SYSNAME]);
  if (xmlconfig[XML_DECODER_USRNAME] == NULL)
      Log.notice("decoder::onConfigUpdate: User name was not provided - using \"\"");
  else
      Log.notice("decoder::onConfigUpdate: User name: %s" CR, xmlconfig[XML_DECODER_USRNAME]);
  if (xmlconfig[XML_DECODER_DESC] == NULL)
      Log.notice("decoder::onConfigUpdate: Description was not provided - using \"\"");
  else
      Log.notice("decoder::onConfigUpdate: Description: %s" CR, xmlconfig[XML_DECODER_DESC]);
  if (!(xmlconfig[XML_DECODER_URI] != NULL) && strcmp(xmlconfig[XML_DECODER_URI], mqtt::getUri()))
      panic("Configuration decoder URI not the same as provided with the discovery response - rebooting ...")
  Log.notice("decoder::onConfigUpdate: Decoder URI: %s" CR, xmlconfig[XML_DECODER_URI]);
  if (!(xmlconfig[XML_DECODER_MAC] != NULL)) && strcmp(xmlconfig[XML_DECODER_URI], networking::getMac(void)())
      panic("Configuration decoder MAC not the same as the physical MAC for this decoder - rebooting ...");
  Log.notice("decoder::onConfigUpdate: MQTT URI: %s" CR, xmlconfig[XML_DECODER_MQTT_URI]);
  Log.notice("decoder::onConfigUpdate: MQTT Port: %s" CR, xmlconfig[XML_DECODER_MQTT_PORT]);
  Log.notice("decoder::onConfigUpdate: MQTT Prefix: %s" CR, xmlconfig[XML_DECODER_MQTT_PREFIX]);
  Log.notice("decoder::onConfigUpdate: MQTT Keep-alive period: %s" CR, xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
  Log.notice("decoder::onConfigUpdate: NTP URI: %s - NOT IMPLEMENTED" CR, xmlconfig[XML_DECODER_NTPURI]);
  Log.notice("decoder::onConfigUpdate: NTP Port: %s:" CR, xmlconfig[XML_DECODER_NTPPORT]);
  Log.notice("decoder::onConfigUpdate: TimeZone: %s" CR, xmlconfig[XML_DECODER_TZ]);
  Log.notice("decoder::onConfigUpdate: Log-level: %s" CR, xmlconfig[XML_DECODER_LOGLEVEL]);
  Log.notice("decoder::onConfigUpdate: Decoder fail-safe: %s" CR, xmlconfig[XML_DECODER_FAILSAFE]);
  Log.notice("decoder::onConfigUpdate: Decoder MAC: %s" CR, xmlconfig[XML_DECODER_MAC]);
  Log.notice("decoder::onConfigUpdate: Successfully parsed the decoder top-configuration:" CR);
  // RESET MQTT CONFIG

  mqtt::setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]));
  // RESET NTP CONFIGURATION
  // RESET TZ CONFIGURATION
  if (transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]) == RC_GEN_ERR) {
      Log.error("decoder::onConfigUpdate: Log-level %s as provided in the decoder configuration is not supported, using default: %s" CR, xmlconfig[XML_DECODER_LOGLEVEL], transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
      delete xmlconfig[XML_DECODER_LOGLEVEL];
      xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(transformLogLevelInt2XmlStr(DEFAULT_LOGLEVEL));
      }
  Log.setLevel(transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]));
  Log.notice("decoder::onConfigUpdate: Successfully set the decoder top-configuration:" CR);
  tinyxml2::XMLElement * lgLinkXmlElement;
  lgLinkXmlElement = xmlConfigDoc.FirstChildElement("genJMRI").FirstChildElement("Decoder").FirstChildElement("LightgroupsLink");
  char* lgLinkSearchTags[4];
  char* lgLinkXmlConfig[4];

  for (int lgLinkItter = 0, lgLinkItter <= NOOF_LGLINKS, lgLinkItter++) {
      if (lgLinkXmlElement == NULL)
          break;
      if(lgLinkItter >= NOOF_LGLINKS)
          panic("decoder::onConfigUpdate: > than %d lgLinks provided - not supported, rebooting...", NOOF_LGLINKS)
      lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
      getTagTxt(lgLinkXmlElement, lgLinkSearchTags, lgLinkXmlConfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
      if (!lgLinkXmlConfig[XML_LGLINK_LINK])
          panic("decoder::onConfigUpdate:: lgLink missing - rebooting..." CR);
      lgLinks[atoi(lgLinkXmlConfig[XML_LGLINK_LINK])].onConfigUpdate(lgLinkXmlElement);
      addSysStateChild([atoi(lgLinkXmlConfig[XML_LGLINK_LINK])]);
      lgLinkXmlElement = xmlConfigDoc.NextSiblingElement("LightgroupsLink");
  }
  tinyxml2::XMLElement* satLinkXmlElement;
  satLinkXmlElement = xmlConfigDoc.FirstChildElement("genJMRI").FirstChildElement("Decoder").FirstChildElement("SateliteLink");
  char* satLinkSearchTags[X];
  char* satXmlconfig[X];
  for (int satLinkItter = 0, satLinkItter <= NOOF_SATLINKS, satLinkItter++) {
      if (satLinkXmlElement == NULL)
          break;
      if (satLinkItter >= NOOF_SATLINKS)
          panic("decoder::onConfigUpdate: > than %d satLinks provided - not supported, rebooting...", NOOF_SATLINKS)
      satLinkSearchTags[XML_SATLINK_LINK] = "Link";
      getTagTxt(satLinkXmlElement, satLinkSearchTags, xmlconfig, sizeof(satLinkSearchTags) / 4); // Need to fix the addressing for portability
      if (~satLinkXmlconfig[XML_LGLINK_LINK])
          panic("decoder::onConfigUpdate:: satLink missing - rebooting..." CR);
      satLinks[atoi(xmlconfig[XML_SATLINK_LINK])].onConfigUpdate(satLinkXmlElement);
      addSysStateChild(satLinks[satLinkItter]);
      satLinkXmlElement = xmlConfigDoc.NextSiblingElement("SateliteLink");
  }
  delete xmlConfigDoc;
  unSetOpState(OP_UNCONFIGURED);
  Log.notice("decoder::on_configUpdate: Configuration successfully finished" CR);
  return;
}

rc_t decoder::start(void){
  Log.notice("decoder::start: Starting decoder" CR);
  uint8_t i = 0;
  while(sysState.getOpState() & OP_UNCONFIGURED){
      if(i==0)
          Log.notice("decoder::start: Waiting for decoder to be configured before it can start" CR);
      if (i++ >= 120)
          panic("decoder::start: Discovery process failed - rebooting...");
      Serial.print('.');
      vTaskDelay(500 / portTICK_PERIOD_MS);
  }
  Log.notice("decoder::start: Subscribing to adm- and op state topics");
  const char* subscribeTopic[4] = { MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onAdmStateChange, NULL))
      panic("decoder::start: Failed to suscribe to admState topic - rebooting...");
  subscribeTopic = { MQTT_DECODER_OPSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onOpStateChange, NULL))
      panic("decoder::start: Failed to suscribe to opState topic - rebooting...");
  Log.notice("decoder::start: Starting lightgroup link Decoders" CR);
  for (int lgLinkItter = 0, lgLinkItter < NOOF_LGLINKS, lgLinkItter++) {
      if (lgLinks[lgLinkItter] == NULL)
          break;
      lgLinks[lgLinkItter]->start();
  }
  Log.notice("decoder::start: Starting satelite link Decoders" CR);
  for (int satLinkItter = 0, satLinkItter < NOOF_LGLINKS, satLinkItter++) {
      if (satLinks[satLinkItter] == NULL)
          break;
      satLinks[satLinkItter]->start();
  }
  unSetOpState(OP_INIT);
  Log.notice("decoder::start: decoder started" CR);
}

void decoder::onSystateChange(const uint16_t p_sysState) {
    if (p_systate & OP_INTFAIL)
        panic("decoder::onSystateChange: decoder has experienced an internal error - rebooting...");
    if ((p_systate & OP_DISCONNECTED) && !(p_systate & OP_INIT))
        panic("decoder::onSystateChange: decoder has been disconnected after initialization phase- rebooting...");
    if (p_systate & ~OP_DISABLED) {
        Log.notice("decoder::onSystateChange: decocoder is marked fault free by server (OP_DISABLED not concidered), opState bitmap: %b - enabling mqtt supervision" CR, p_sysState);
        mqtt::startSupervision();
    else:
    Log.notice("decoder::onSystateChange: decocoder is marked faulty by server (OP_DISABLED not concidered), opState bitmap: %b - enabling mqtt supervision" CR, p_sysState);
    mqtt::stopSupervision();
}

void decoder::onOpStateChange(const char* p_topic, const char* p_payload, const void* DUMMY) {
    if (!cmpstr(p_payload, AVAILABLE_PAYLOAD)) {
         unSetOpState(OP_UNAVAILABLE);
        Log.notice("decoder::onOpStateChange: got available message from server" CR);
    }
    else if (!cmpstr(p_payload, UNAVAILABLE_PAYLOAD)) {
       setOpState(OP_UNAVAILABLE);
        Log.notice("decoder::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.error("decoder::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void decoder::onAdmStateChange(const char* p_topic, const char* p_payload, const void* DUMMY) {
    if (!cmpstr(p_payload, ONLINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("decoder::onAdmStateChange: got online message from server" CR);
    }
    else if (!cmpstr(p_payload, OFFLINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("decoder::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.error("decoder::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

void decoder::onMqttChange(const int p_mqttState) {
    if (p_mqttState == MQTT_CONNECTED)
        unSetOpState(OP_DISCONNECTED);
    else
        setOpState(OP_DISCONNECTED);
}

void decoder::setMqttURI(const char* p_mqttURI, const bool p_force=false) {
    if (!debug || !p_force)
        Log.error("decoder::getMqttURI: cannot set MQTT URI as debug is inactive" CR);
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttURI: cannot set MQTT URI as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::getMqttURI: setting MQTT URI to %s" CR, p_mqttURI);
        delete xmlconfig[XML_DECODER_MQTT_URI];
        xmlconfig[XML_DECODER_MQTT_URI] = createNcpystr(p_mqttURI);
        mqtt::reConnect(p_broker = xmlconfig[XML_DECODER_MQTT_URI]);
        return RC_OK;
    }
}

const char* decoder::getMqttURI(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttURI: cannot get MQTT URI as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MQTT_URI];
}

rc_t decoder::setMqttPort(const uint16_t p_mqttPort, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setMqttPort: cannot set MQTT Port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttPort: cannot set MQTT port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMqttPort: setting MQTT Port to %s" CR, p_mqttPort);
        delete xmlconfig[XML_DECODER_MQTT_PORT];
        xmlconfig[XML_DECODER_MQTT_PORT] = createNcpystr(itoa(p_mqttPort));
        mqtt::reConnect(p_port = atoi(xmlconfig[XML_DECODER_MQTT_PORT]));
        return RC_OK;
    }
}

const uint16_t decoder::getMqttPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttPort: cannot get MQTT port as decoder is not configured" CR);
        return 0;
    }
    return atoi(xmlconfig[XML_DECODER_MQTT_PORT]);
}

rc_t decoder::setMqttPrefix(const char* p_mqttPrefix, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setMqttPrefix: cannot set MQTT Prefix as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMqttPrefix: cannot set MQTT prefix as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMqttPrefix: setting MQTT Prefix to %s" CR, p_mqttPrefix);
        delete xmlconfig[XML_DECODER_MQTT_PREFIX];
        xmlconfig[XML_DECODER_MQTT_PREFIX] = createNcpystr(p_mqttPrefix);
        return RC_OK;
    }
}

const char* decoder::getMqttPrefix(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMqttPrefix: cannot get MQTT prefix as decoder is not configured" CR);
        return RC_GEN_ERR;
    }
    return xmlconfig[XML_DECODER_MQTT_PREFIX];
}

rc_t decoder::setKeepAlivePeriod(const float p_keepAlivePeriod, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setKeepAlivePeriod: cannot set keep-alive period as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setKeepAlivePeriod: cannot set keep-alive period as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setKeepAlivePeriod: setting keep-alive period to %f" CR, p_keepAlivePeriod);
        delete xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD];
        xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD] = createNcpystr(ftoa(p_keepAlivePeriod));
        mqtt::setPingPeriod(atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]));
        return RC_OK;
    }
}

const float decoder::getKeepAlivePeriod(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getKeepAlivePeriod: cannot get keep-alive period as decoder is not configured" CR);
        return RC_GEN_ERR;
    }
    return atof(xmlconfig[XML_DECODER_MQTT_KEEPALIVEPERIOD]);
}

rc_t decoder::setNtpServer(const char* p_ntpServer, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setNtpServer: cannot set NTP server as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setNtpServer: cannot set NTP server as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setNtpServer: cannot set NTP server - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getNtpServer(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getNtpServer: cannot get NTP server as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_NTPURI];
}

rc_t decoder::setNtpPort(const uint16_t p_ntpPort, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setNtpPort: cannot set NTP port as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setNtpPort: cannot set NTP port as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setNtpPort: cannot set NTP port - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const uint16_t decoder::getNtpPort(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getNtpPort: cannot get NTP port as decoder is not configured" CR);
        return 0;
    }
    return atoi(xmlconfig[XML_DECODER_NTPPORT]);
}

rc_t decoder::setTz(const uint8_t p_tz, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setTz: cannot set Time zone as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setTz: cannot set time-zone as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.error("decoder::setTz: cannot set Time zone - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const uint8_t decoder::getTz(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getTz: cannot get Time-zone as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    return atoi(xmlconfig[XML_DECODER_TZ]);
}

rc_t decoder::setLogLevel(const char* p_logLevel, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("decoder::setLogLevel: cannot set Log level as debug is inactive" CR);
            return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setLogLevel: cannot set log-level as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        if (transformLogLevelXmlStr2Int(p_logLevel) == RC_GEN_ERR) {
            Log.error("decoder::setLogLevel: cannot set Log-level %s, log-level not supported, using current log-level: %s" CR, p_logLevel, xmlconfig[XML_DECODER_LOGLEVEL]);
            return RC_GEN_ERR;
        }
        Log.notice("decoder::setLogLevel: Setting Log-level to %s" CR, p_logLevel);
        delete xmlconfig[XML_DECODER_LOGLEVEL];
        xmlconfig[XML_DECODER_LOGLEVEL] = createNcpystr(p_logLevel);
    }
    Log.setLevel(transformLogLevelXmlStr2Int(xmlconfig[XML_DECODER_LOGLEVEL]));
    return RC_OK;
}

const char* decoder::getLogLevel(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getLogLevel: cannot get log-level as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_LOGLEVEL]
}

rc_t decoder::setFailSafe(const bool p_failsafe, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.notice("decoder::setFailSafe: cannot set Fail-safe as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setFailSafe: cannot set fail-safe as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        delete xmlconfig[XML_DECODER_FAILSAFE];
        if (p_failsafe) {
            Log.notice("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_TRUE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_TRUE_PAYLOAD);
        else
            Log.notice("decoder::setFailSafe: Setting Fail-safe to %s" CR, MQTT_BOOL_FALSE_PAYLOAD);
            xmlconfig[XML_DECODER_FAILSAFE] = createNcpystr(MQTT_BOOL_FALSE_PAYLOAD);
        return RC_OK;
    }
}

const bool decoder::getFailSafe(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getFailSafe: cannot get fail-safe as decoder is not configured" CR);
        return false;
    }    
    if cmpstr(xmlconfig[XML_DECODER_LOGLEVEL], BOOL_TRUE_PAYLOAD)
        return true;
    if cmpstr(xmlconfig[XML_DECODER_LOGLEVEL], BOOL_FALSE_PAYLOAD)
        return false;
}

rc_t decoder::setSystemName(const char* p_systemName, const bool p_force = false) {
    Log.error("decoder::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* decoder::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getSystemName: cannot get System name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_SYSNAME];
}

rc_t decoder::setUsrName(const char* p_usrName, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.notice("decoder::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setUsrName: cannot set User name as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_DECODER_USRNAME];
        xmlconfig[XML_DECODER_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* decoder::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getUsrName: cannot get User name as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_USRNAME];
}

rc_t decoder::setDesc(voiconst char* p_description, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.notice("decoder::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setDesc: cannot set Description as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_DECODER_DESC];
        xmlconfig[XML_DECODER_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* decoder::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getDesc: cannot get Description as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_DESC];
}

rc_t decoder::setMac(const char* p_mac, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.notice("decoder::setMac: cannot set MAC as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setMac: cannot set MAC as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setMac: cannot set MAC - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getMac(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getMac: cannot get MAC as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_MAC]
}

rc_t decoder::setDecoderUri(const char* p_decoderUri, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.notice("decoder::setDecoderUri: cannot set decoder URI as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::setDecoderUri: cannot set Decoder URI as decoder is not configured" CR);
        return RC_NOT_CONFIGURED_ERR;
    }
    else {
        Log.notice("decoder::setDecoderUri: cannot set Decoder URI - not implemented" CR);
        return RC_NOTIMPLEMENTED_ERR;
    }
}

const char* decoder::getDecoderUri(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("decoder::getDecoderUri: cannot get Decoder URI as decoder is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_DECODER_URI]
}

void decoder::setDebug(const bool p_debug) {
    debug = p_debug;
}

const bool decoder::getDebug(void) {
    return debug;
}
/*==============================================================================================================================================*/
/* END Class decoder                                                                                                                            */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: "lgLink(lightgroupLink)"                                                                                                              */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
lgLink::lgLink(uint8_t p_linkNo) : systemState(this) {
    Log.notice("lgLink::lgLink: Creating Lightgroup link channel %d" CR, p_linkNo);
    linkNo = p_linkNo;
    regSysStateCb(onSystateChange);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
    lgLinkLock = xSemaphoreCreateMutex();
    if (lgLinkLock == NULL)
        panic("lgLink::satLink: Could not create Lock objects - rebooting...");
    avgSamples = UPDATE_STRIP_LATENCY_AVG_TIME * 1000 / STRIP_UPDATE_MS;
    latencyVect = new int64_t[avgSamples];
    runtimeVect = new uint32_t[avgSamples];
    if (latencyVect == NULL || runtimeVect == NULL)
        panic("lgLink::satLink: Could not create PM vectors - rebooting...");
}

lgLink::~lgLink(void){
  panic("lgLink::~lgLink: lgLink destructior not supported - rebooting...");
}

rc_t lgLink::init(void){
  Log.notice("lgLink::init: Initializing Lightgroup link channel %d" CR, linkNo);
  Log.notice("lgLink::init: Creating lighGrups for link channel %d" CR, linkNo);
  for (uint8_t lgAddress = 0; lgAddress < MAX_LGSTRIPLEN; lgAddress++) {
      lgs[lgAddress] = new lg(lgAddress, this);
      if(lgs[lgAddress] == NULL)
          panic("lgLink::init: Could not create light-group object - rebooting..." CR);
  }
  Log.notice("lgLink::init: Creating flash channels" CR);
  FLASHNORMAL = new flash(SM_FLASH_TIME_NORMAL, 50);
  FLASHSLOW = new flash(SM_FLASH_TIME_SLOW, 50);
  FLASHFAST = new flash(SM_FLASH_TIME_FAST, 50);
  if(FLASHNORMAL == NULL || FLASHSLOW == NULL || FLASHFAST == NULL){
    panic("lgLink::init: Could not create flash objects - rebooting...");
  }
  stripCtrlBuff = new stripLed_t[MAX_LGSTRIPLEN];
  strip = new Adafruit_NeoPixel(MAX_LGSTRIPLEN, LGLINK_PINS[linkNo], NEO_RGB + NEO_KHZ800);
  if(strip == NULL)
    panic("lgLink::init: Could not create NeoPixel object - rebooting..." CR);
  strip.begin();
  stripWritebuff = strip.getPixels();
  return RC_OK;
}

void lgLink::on_configUpdate(tinyxml2::XMLElement* p_lightgroupLinkXmlElement) {
    if (~(sysState.getOpState() & OP_UNCONFIGURED))
        panic("lgLink:onConfigUpdate: Received a configuration, while the it was already configured, dynamic re-configuration not supported, opState: %d - rebooting...", getOpState());
    Log.notice("lgLink::onConfigUpdate: lgLink %d received an uverified configuration, parsing and validating it..." CR, linkNo);
    xmlconfig = new char* [4];
    xmlconfig[XML_LGLINK_SYSNAME] = NULL;
    xmlconfig[XML_LGLINK_USRNAME] = NULL;
    xmlconfig[XML_LGLINK_DESC] = NULL;
    xmlconfig[XML_LGLINK_LINK] = NULL;
    const char* lgLinkSearchTags[4];
    lgLinkSearchTags[XML_LGLINK_SYSNAME] = "SystemName";
    lgLinkSearchTags[XML_LGLINK_USRNAME] = "UserName";
    lgLinkSearchTags[XML_LGLINK_DESC] = "Description";
    lgLinkSearchTags[XML_LGLINK_LINK] = "Link";
    getTagTxt(p_lightgroupLinkXmlElement, lgLinkSearchTags, xmlconfig, sizeof(lgLinkSearchTags) / 4); // Need to fix the addressing for portability
    if (~xmlconfig[XML_LGLINK_SYSNAME])
        panic("lgLink::on_configUpdate: SystemNane missing - rebooting...");
    if (~xmlconfig[XML_LGLINK_USRNAME])
        panic("lgLink::onConfigUpdate: User name missing - rebooting...");
    if (~xmlconfig[XML_LGLINK_DESC])
        panic("lgLink::onConfigUpdate: Description missing - rebooting...");
    if (~xmlconfig[XML_LGLINK_LINK])
        panic("lgLink::on_configUpdate: Link missing - rebooting...");
    if (atoi(xmlconfig[XML_LGLINK_LINK]) != linkNo)
        panic("lgLink::on_configUpdate: Link no inconsistant - rebooting...");
    Log.notice("lgLink::onConfigUpdate: System name: %s" CR, xmlconfig[XML_LGLINK_SYSNAME]);
    Log.notice("lgLink::onConfigUpdate: User name:" CR, xmlconfig[XML_LGLINK_USRNAME]);
    Log.notice("lgLink::onConfigUpdate: Description: %s" CR, xmlconfig[XML_LGLINK_DESC]);
    Log.notice("lgLink::onConfigUpdate: Link: %s" CR, xmlconfig[XML_LGLINK_LINK]);
    Log.notice("lgLink::onConfigUpdate: Creating and configuring signal mast aspect description object");
    signalMastAspectsObject = new signalMastAspects();
    if(signalMastAspectsObject == NULL)
        panic("lgLink:onConfigUpdate: Could not start signalMastAspect object - rebooting...")
    if (~p_lightgroupLinkXmlElement.FirstChildElement("SignalMastDesc"))
        panic("lgLink::on_configUpdate: Signal mast aspect description missing - rebooting...")
    else
        if (signalMastAspectsObject.onConfigUpdate(p_lightgroupLinkXmlElement.FirstChildElement("SignalMastDesc")))
            panic("lgLink::on_configUpdate: Could not configure Signal mast aspect description - rebooting..." CR);
    if (p_lightgroupLinkXmlElement.NextSiblingElement("SignalMastDesc"))
        panic("lgLink::on_configUpdate: Multiple signal mast aspect descriptions provided - rebooting..." CR);
    Log.notice("lgLink::onConfigUpdate: Creating and configuring Light groups");
    tinyxml2::XMLElement* lgXmlElement;
    lgXmlElement = p_lightgroupLinkXmlElement.FirstChildElement("LightGroup");
    char* lgSearchTags[8];
    char* lgXmlConfig[8];
    for (uint16_t lgItter = 0, false, lgItter++) {
        if (lgXmlElement == NULL)
            break;
        if (lgItter > MAX_LGSTRIPLEN)
            panic("lgLink::onConfigUpdate: > than %d lgs provided - not supported, rebooting...", MAX_LGSTRIPLEN)
        lgSearchTags[XML_LG_LINKADDR] = "LinkAddress";
        getTagTxt(lgXmlElement, lgSearchTags, lgXmlConfig, sizeof(lgXmlElement) / 4); // Need to fix the addressing for portability
        if (!lgXmlConfig[XML_LG_LINKADDR])
            panic("lgLink::onConfigUpdate:: LG Linkaddr missing - rebooting..." CR);
        lgs[atoi(lgXmlConfig[XML_LG_LINKADDR])]->onConfigUpdate(lgXmlElement);
        addSysStateChild(lgs[atoi(lgXmlConfig[XML_LG_LINKADDR])]);
        lgXmlElement = p_lightgroupLinkXmlElement->NextSiblingElement("LightGroup");
    }
    uint16_t stripOffset = 0;
    for (uint16_t lgItter = 0, lgItter < MAX_LGSTRIPLEN, lgItter++) {
        lgs[lgItter]->setStripOffset(stripOffset);
        stripOffset += lgs[lgItter]->getNoLeds();
    }
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("lgLink::on_configUpdate: Configuration successfully finished" CR);
}

rc_t lgLink::start(void){
  Log.notice("lgLink::start: Starting lightgroup link: %d" CR, linkNo);
  if (getOpState() & OP_UNCONFIGURED) {
      Log.notice("lgLink::start: LG Link %d not configured - will not start it" CR, linkNo);
      setOpState(OP_UNUSED);
      unSetOpState(OP_INIT);
      return RC_NOT_CONFIGURED_ERR;
  }
  Log.notice("lgLink::start: Subscribing to adm- and op state topics");
  const char* subscribeTopic[4] = {MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onAdmStateChangeHelper, this))
      panic("lgLink::start: Failed to suscribe to admState topic - rebooting...");
  subscribeTopic = {MQTT_DECODER_OPSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
  if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onOpStateChange, this))
      panic("lgLink::start: Failed to suscribe to opState topic - rebooting...");
  for(uint16_t lgItter = 0, lgItter < MAX_LGSTRIPLEN, lgItter++)
      lgs[lgItter]->start();
  xTaskCreatePinnedToCore(
                        updateStripHelper,          // Task function
                        "StripHandler",             // Task function name reference
                        6*1024,                     // Stack size
                        this,                       // Parameter passing
                        UPDATE_STRIP_PRIO,          // Priority 0-24, higher is more
                        NULL,                       // Task handle
                        UPDATE_STRIP_CORE);         // Core [CORE_0 | CORE_1]

  unSetOpState(OP_INIT);
  Log.notice("lgLink::start: lightgroups link %d and all its lightgroupDecoders have started" CR, linkNo);
  return RC_OK;
}

void lgLink::onSystateChange(const uint16_t p_sysState) {
    if (p_systate & OP_INTFAIL)
        panic("lgLink::onSystateChange: lg link %d has experienced an internal error - rebooting...", linkNo);
    if (p_sysState)
        Log.notice("lgLink::onSystateChange: Link %d has received Opstate %b - doing nothing" CR, linkNo, p_sysState);
    else
        Log.notice("lgLink::onSystateChange: Link %d has received a cleared Opstate - doing nothing" CR, linkNo);
}

void lgLink::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onOpStateChange(p_topic, p_payload);
}

void lgLink::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, AVAILABLE_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("lgLink::onOpStateChange: got available message from server" CR);
    }
    else if (!cmpstr(p_payload, UNAVAILABLE_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("lgLink::onOpStateChange: got unavailable message from server" CR);
    }
    else
        Log.error("lgLink::onOpStateChange: got an invalid availability message from server - doing nothing" CR);
}

void lgLink::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgLinkObject) {
    ((lgLink*)p_lgLinkObject)->onAdmStateChange(p_topic, p_payload);
}

void lgLink::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, ONLINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("lgLink::onAdmStateChange: got online message from server" CR);
    }
    else if (!cmpstr(p_payload, OFFLINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("lgLink::onAdmStateChange: got off-line message from server" CR);
    }
    else
        Log.error("lgLink::onAdmStateChange: got an invalid admstate message from server - doing nothing" CR);
}

rc_t lgLink::setSystemName(const char* p_systemName, const bool p_force = false) {
    Log.error("lgLink::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* lgLink::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getSystemName: cannot get System name as lgLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_LGLINK_SYSNAME];
}

rc_t lgLink::setUsrName(const char* p_usrName, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("lgLink::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::setUsrName: cannot set System name as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("lgLink::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_LGLINK_USRNAME];
        xmlconfig[XML_LGLINK_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* lgLink::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getUsrName: cannot get User name as lgLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_LGLINK_USRNAME];
}

rc_t lgLink::setDesc(const char* p_description, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("lgLink::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::setDesc: cannot set Description as lgLink is not configured" CR);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("lgLink::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_LGLINK_DESC];
        xmlconfig[XML_LGLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* lgLink::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getDesc: cannot get Description as lgLink is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_LGLINK_DESC];
}

const rc_t lgLink::setLink(const char* p_link) {
    Log.error("lgLink::setLink: cannot set Link No - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const rc_t lgLink::getLink(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::getLink: cannot get Link No as lgLink is not configured" CR);
        return RC_GEN_ERR;
    }
    return atoi(xmlconfig[XML_LGLINK_LINK]);
}

const void lgLink::setDebug(const bool p_debug) {
    debug = p_debug;
}

const bool lgLink::getDebug(void) {
    return debug;
}

const rc_t lgLink::updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, const uint8_t* p_wantedValueBuff, const uint16_t* p_transitionTimeBuff){
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
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
  xSemaphoreGive(lgLinkLock);
  return RC_OK;
}

void lgLink::updateStripHelper(void* p_lgLinkObject){
  ((lgLink*)p_lgLinkObject)->updateStrip();
}

void lgLink::updateStrip(void){
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

  Log.verbose("gLink::updateStrip: Starting sriphandler channel %d" CR, channel);
  while(true){
    xSemaphoreTake(lgLinkLock, portMAX_DELAY);
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
        strip.show();
      }
    }
    runtime =  esp_timer_get_time() - startTime;
    runtimeVect[avgIndex] = runtime;
    if(runtime > maxRuntime){
      maxRuntime = runtime;
    }
    TickType_t delay;
    if((int)(delay = nextLoopTime - esp_timer_get_time()) > 0){
      xSemaphoreGive(lgLinkLock);
      vTaskDelay((delay/1000)/portTICK_PERIOD_MS);
    }
    else{
      Log.verbose("Strip channel %d overrun" CR, channel);
      overRuns++;
      xSemaphoreGive(lgLinkLock);
      nextLoopTime = esp_timer_get_time();
    }
    avgIndex++;
  }
}

uint32_t lgLink::getOverRuns(void){
  return overRuns;
}

void lgLink::clearOverRuns(void){
  overRuns = 0;
}

int64_t lgLink::getMeanLatency(void){
  int64_t accLatency = 0;
  int64_t meanLatency;
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
  for(uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++)
    accLatency += latencyVect[avgIndex];
  xSemaphoreGive(lgLinkLock);
  meanLatency = accLatency/avgSamples;
  return meanLatency;
}

int64_t lgLink::getMaxLatency(void){
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
  int64_t tmpMaxLatency = maxLatency;
  xSemaphoreGive(lgLinkLock);
  return tmpMaxLatency;
}

void lgLink::clearMaxLatency(void){
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
  maxLatency = -1000000000; //NEEDS FIX
  xSemaphoreGive(lgLinkLock);
}

uint32_t lgLink::getMeanRuntime(void){
  uint32_t accRuntime = 0;
  uint32_t meanRuntime;
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
  for(uint16_t avgIndex = 0; avgIndex < avgSamples; avgIndex++)
    accRuntime += runtimeVect[avgIndex];
  xSemaphoreGive(lgLinkLock);
  meanRuntime = accRuntime/avgSamples;
  return meanRuntime;
}

uint32_t lgLink::getMaxRuntime(void){
  return maxRuntime;
}

void lgLink::clearMaxRuntime(void){
  maxRuntime = 0;
}

flash* lgLink::getFlashObj(uint8_t p_flashType){
  xSemaphoreTake(lgLinkLock, portMAX_DELAY);
  if(opState == OP_CREATED || opState == OP_INIT || opState == OP_FAIL) {
    Log.error("lgLink::getFlashObj: opState %d does not allow to provide flash objects - returning NULL - and continuing..." CR, opState);
    xSemaphoreGive(lgLinkLock);
    return NULL;
  }
  xSemaphoreGive(lgLinkLock);
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

signalMastAspects* lglink::getSignalMastAspectObj(void) {
    return signalMastAspectsObject;
}
/*==============================================================================================================================================*/
/* END Class lgLink                                                                                                                         */
/*==============================================================================================================================================*/










/*==============================================================================================================================================*/
/* Class: "sat(Satelite)"                                                                                                                       */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/*==============================================================================================================================================*/
sat::sat(uint8_t satAddr_p, linkHandle_p) : systemState(this) {
    Log.notice("sat::sat: Creating Satelite adress %d" CR, p_satAddr);
    linkHandle = linkHandle_p;
    satAddr = satAddr_p;
    regSysStateCb(onSystateChange);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_UNDISCOVERED | OP_DISABLED | OP_UNAVAILABLE);
    satLock = xSemaphoreCreateMutex();
    if (satLock == NULL)
        panic("sat::sat: Could not create Lock objects - rebooting...");
}

sat::~sat(void) {
    panic("sat::~sat: sat destructior not supported - rebooting...");
}

rc_t sat::init(void) {
    Log.notice("sat::init: Initializing Satelite address %d" CR, satAddr);
    Log.notice("sat::init: Creating actuators for satelite address %d on link %d" CR, satAddr, linkHandle->getLink());
    for (uint8_t actPort = 0; actIndex < MAX_ACT; actIndex++) {
        acts[actPort] = new act(actPort, this)
            if (acts[actPort] == NULL)
                panic("sat::init: Could not create actuator object for satelite %d, link channel %d - rebooting..." CR, satAddr, linkHandle->getLink());
    }
    Log.notice("sat::init: Creating sensors for satelite address %d on link %d" CR, satAddr, linkHandle->getLink());
    for (uint8_t sensPort = 0; sensPort < MAX_SENSE; sensPort++) {
        senses[sensPort] = new senseBase(sensPort, this)
            if (senses[sensPort] == NULL)
                panic("sat::init: Could not create sensor object for satelite %d, link channel %d - rebooting..." CR, satAddr, linkHandle->getLink());
    }
    txUnderunErr = 0;
    rxOverRunErr = 0;
    scanTimingViolationErr = 0;
    rxCrcErr = 0;
    remoteCrcErr = 0;
    rxSymbolErr = 0;
    rxDataSizeErr = 0;
    wdErr = 0;
    return RC_OK;
}

void sat::on_configUpdate(tinyxml2::XMLElement * p_satXmlElement) {
    if (~(sysState.getOpState() & OP_UNCONFIGURED))
        panic("sat:onConfigUpdate: Received a configuration, while the it was already configured, dynamic re-configuration not supported, opState: %d - rebooting...", getOpState());
    Log.notice("sat::onConfigUpdate: satAddress %d on link %d received an uverified configuration, parsing and validating it..." CR, satAddr, linkHandle->getLink());
    xmlconfig = new char* [4];
    xmlconfig[XML_SAT_SYSNAME] = NULL;
    xmlconfig[XML_SAT_USRNAME] = NULL;
    xmlconfig[XML_SAT_DESC] = NULL;
    xmlconfig[XML_SAT_ADDR] = NULL;
    const char* satSearchTags[4];
    satSearchTags[XML_SAT_SYSNAME] = "SystemName";
    satSearchTags[XML_SAT_USRNAME] = "UserName";
    satSearchTags[XML_SAT_DESC] = "Description";
    satSearchTags[XML_SAT_ADDR] = "Address";
    getTagTxt(p_satXmlElement, satSearchTags, xmlconfig, sizeof(satSearchTags) / 4); // Need to fix the addressing for portability
    if (~xmlconfig[XML_SAT_USRNAME])
        panic("sat::on_configUpdate: SystemNane missing - rebooting...");
    if (~xmlconfig[XML_SAT_USRNAME])
        panic("sat::onConfigUpdate: User name missing - rebooting...");
    if (~xmlconfig[XML_SAT_DESC])
        panic("sat::onConfigUpdate: Description missing - rebooting...");
    if (~xmlconfig[XML_SAT_ADDR])
        panic("sat::on_configUpdate: Adrress missing - rebooting...");
    if (atoi(xmlconfig[XML_SAT_ADDR]) != satAddr)
        panic("sat::on_configUpdate: Address no inconsistant - rebooting...");
    Log.notice("sat::onConfigUpdate: System name: %s" CR, xmlconfig[XML_SAT_SYSNAME]);
    Log.notice("sat::onConfigUpdate: User name:" CR, xmlconfig[XML_SAT_USRNAME]);
    Log.notice("sat::onConfigUpdate: Description: %s" CR, xmlconfig[XML_SAT_DESC]);
    Log.notice("sat::onConfigUpdate: Address: %s" CR, xmlconfig[XML_SAT_ADDR]);
    Log.notice("sat::onConfigUpdate: Configuring Actuators");
    tinyxml2::XMLElement* actXmlElement;
    actXmlElement = p_satXmlElement.FirstChildElement("Actuator");
    char* actSearchTags[6];
    char* actXmlConfig[6];
    for (uint16_t actItter = 0, false, actItter++) {
        if (actXmlElement == NULL)
            break;
        if (actItter >= MAX_ACT)
            panic("sat::onConfigUpdate: > than %d actuators provided - not supported, rebooting...", MAX_ACT);
        actSearchTags[XML_ACT_PORT] = "Port";
        getTagTxt(actXmlElement, actSearchTags, actXmlConfig, sizeof(actXmlElement) / 4); // Need to fix the addressing for portability
        if (!actXmlConfig[XML_ACT_PORT])
            panic("sat::onConfigUpdate:: Actuator port missing - rebooting..." CR);
        acts[atoi(actXmlConfig[XML_ACT_PORT])]->onConfigUpdate(actXmlConfig);
        addSysStateChild(acts[atoi(actXmlConfig[XML_ACT_PORT])]);
        actXmlElement = p_satXmlElement->NextSiblingElement("Actuator");
    }
    Log.notice("sat::onConfigUpdate: Configuring sensors");
    tinyxml2::XMLElement* actXmlElement;
    sensXmlElement = p_satXmlElement.FirstChildElement("Sensor");
    char* sensSearchTags[5];
    char* sensXmlConfig[5];
    for (uint16_t sensItter = 0, false, sensItter++) {
        if (actXmlElement == NULL)
            break;
        if (sensItter >= MAX_SENS)
            panic("sat::onConfigUpdate: > than %d sensors provided - not supported, rebooting...", MAX_SENS);
        actSearchTags[XML_SENS_PORT] = "Port";
        getTagTxt(sensXmlElement, sensSearchTags, sensXmlConfig, sizeof(sensXmlElement) / 4); // Need to fix the addressing for portability
        if (!actXmlConfig[XML_SENS_PORT])
            panic("sat::onConfigUpdate:: Sensor port missing - rebooting..." CR);
        senses[atoi(sensXmlConfig[XML_SENS_PORT])]->onConfigUpdate(sensXmlConfig);
        addSysStateChild(senses[atoi(sensXmlConfig[XML_SENS_PORT])]);
        sensXmlElement = p_satXmlElement->NextSiblingElement("Actuator");
    }
    unSetOpState(OP_UNCONFIGURED);
    Log.notice("sat::on_configUpdate: Configuration successfully finished" CR);
}

rc_t sat::start(void) {
    Log.notice("sat::start: Starting Satelite address: %d on satlink %d" CR, satAddr, linkHandle->getLink());
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("sat::start: Satelite address %d on satlink %d not configured - will not start it" CR, satAddr, linkHandle->getLink());
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    if (getOpState() & OP_UNDISCOVERED) {
        Log.notice("sat::start: Satelite address %d on satlink %d not yet discovered - waiting for discovery before starting it" CR, satAddr, linkHandle->getLink());
        pendingStart = true;
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("sat::start: Subscribing to adm- and op state topics");
    const char* subscribeTopic[4] = { MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getUri(), "/", getSystemName() };
    if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onAdmStateChangeHelper, this))
        panic("sat::start: Failed to suscribe to admState topic - rebooting...");
    subscribeTopic = { MQTT_DECODER_OPSTATE_TOPIC, mqtt::getUri(), "/", getSystemName() };
    if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onOpStateChange, this))
        panic("sat::start: Failed to suscribe to opState topic - rebooting...");
    for (uint16_t actItter = 0, actItter < MAX_ACT, actItter++) {
        acts[actItter]->start();
    }
    for (uint16_t sensItter = 0, sensItter < MAX_SENS, sensItter++) {
        senses[sensItter]->start();
    }
    satLibHandle->satRegStateCb(&onSatLibStateChangeHelper, this);
    satLibHandle->setErrTresh(0, 0);
    rc = satLibHandle->enableSat();
    if (rc)
        panic("sat::start: could not enable Satelite address: %d on satlink %d, return code %d - rebooting...", satAddr, linkHandle->getLink(), rc);
    unSetOpState(OP_INIT);
    Log.notice("sat::start: Satelite address: %d on satlink %d have started" CR, satAddr, linkHandle->getLink());
    return RC_OK;
}

void sat::onDiscovered(sateliteLibHandle_p, satAddr_p, exists_p) {
    Log.notice("sat::onDiscovered: Satelite address %d on satlink %d discovered" CR, satAddr, linkHandle->getLink());
    if (satAddr_p != satAddr)
        panic("sat::onDiscovered: Inconsistant satelite address provided - rebooting...");
    satLibHandle = sateliteLibHandle_p;
    for (uint16_t actItter = 0, actItter < MAX_ACT, actItter++)
        acts[actItter]->onDiscovered(satLibHandle)
    for (uint16_t sensItter = 0, sensItter < MAX_SENS, sensItter++)
        senses[sensItter]->onDiscovered(satLibHandle)
    unSetOpState(OP_UNDISCOVERED);
    if (pendingStart) {
        Log.notice("sat::onDiscovered: Satelite address %d on satlink %d was waiting for discovery before it could be started - now starting it" CR, satAddr, linkHandle->getLink());
        rc = start();
        if (rc)
            panic("sat::onDiscovered: Could not start Satelite address %d on satlink %d, rc: %d - rebooting...", satAddr, linkHandle->getLink(), rc);
    }
}

void sat::onPmPoll(void) {
    satPerformanceCounters_t pmData;
    satLibHandle->getSatStats(&pmData, true);
    rxCrcErr += pmData.rxCrcErr;
    remoteCrcErr += pmData.remoteCrcErr;
    wdErr += pmData.wdErr;
    const char* subscribeTopic[4] = { MQTT_SATLINK_STATS_TOPIC, mqtt::getUri(), "/", getSystemName() };
    if (mqtt::sendMsg(concatStr(publishTopic, 4), ("<statReport>\n"
        "<rxCrcErr>%d</rxCrcErr>\n"
        "<txCrcErr> %d</txCrcErr>\n"
        "<wdErr> %d</wdErr>\n"
        "</statReport>",
        pmData.remoteCrcErr,
        pmData.rxCrcErr,
        pmData.wdErr),
        false)
        Log.error("sat::onPmPoll: Failed to send PM report" CR)
}

void sat::onSatLibStateChangeHelper(const satelite * sateliteLibHandle_p, const uint8_t linkAddr_p, const uint8_t satAddr_p, const satOpState_t satOpState_p, void* satHandle_p) {
    ((*satLink*)satHandle_p)->onSatLibStateChange(satOpState_t satOpState_p);
}

void sat::onSatLibStateChange(const satOpState_t satOpState_p) {
    if (!(getOpstate & OP_INIT) {
        if (satOpState_p)
            setOpState(OP_INTFAIL);
        else
            unSetOpState(OP_INTFAIL);
    }
}

void sat::onSystateChange(const uint16_t p_sysState) {
    if (p_systate & OP_INTFAIL && p_systate & OP_INIT)
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has experienced an internal error while in OP_INIT phase, waiting for initialization to finish before taking actions" CR, satAddr, linkHandle->getLink())
    else if (p_systate & OP_INTFAIL)
        panic("sat::onSystateChange: satelite address %d on satlink %d has experienced an internal error - rebooting...", satAddr, linkHandle->getLink());
    if (p_sysState)
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has received Opstate %b - doing nothing" CR, satAddr, linkHandle->getLink(), p_sysState);
    else
        Log.notice("sat::onSystateChange: satelite address %d on satlink %d has received a cleared Opstate - doing nothing" CR, satAddr, linkHandle->getLink());
}

void sat::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satHandle) {
    ((sat*)p_satHandle)->onOpStateChange(p_topic, p_payload);
}

void sat::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, AVAILABLE_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("sat::onOpStateChange: satelite address %d on satlink %d got available message from server" CR, satAddr, linkHandle->getLink());
    }
    else if (!cmpstr(p_payload, UNAVAILABLE_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("sat::onOpStateChange: satelite address %d on satlink %d got unavailable message from server" CR, satAddr, linkHandle->getLink());
    }
    else
        Log.error("sat::onOpStateChange: satelite address %d on satlink %d got an invalid availability message from server - doing nothing" CR, satAddr, linkHandle->getLink());
}

void sat::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_satLinkHandle) {
    ((satLink*)p_satLinkHandle)->onAdmStateChange(p_topic, p_payload);
}

void sat::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, ONLINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("sat::onAdmStateChange: satelite address %d on satlink %d got online message from server" CR, satAddr, linkHandle->getLink());
    }
    else if (!cmpstr(p_payload, OFFLINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("sat::onAdmStateChange: satelite address %d on satlink %d got off-line message from server" CR, satAddr, linkHandle->getLink());
    }
    else
        Log.error("sat::onAdmStateChange: satelite address %d on satlink %d got an invalid admstate message from server - doing nothing" CR, satAddr, linkHandle->getLink());
}

rc_t sat::setSystemName(const char* p_systemName, const bool p_force = false) {
    Log.error("sat::setSystemName: cannot set System name - not suppoted" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* sat::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getSystemName: cannot get System name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_SYSNAME];
}

rc_t sat::setUsrName(const char* p_usrName, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("sat::setUsrName: cannot set User name as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::setUsrName: cannot set System name as satelite is not configured" CR);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("sat::setUsrName: Setting User name to %s" CR, p_usrName);
        delete xmlconfig[XML_SAT_USRNAME];
        xmlconfig[XML_SAT_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }
}

const char* sat::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getUsrName: cannot get User name as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_USRNAME];
}

rc_t sat::setDesc(const char* p_description, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("sat::setDesc: cannot set Description as debug is inactive" CR);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::setDesc: cannot set Description as satelite is not configured" CR);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("sat::setDesc: Setting Description to %s" CR, p_description);
        delete xmlconfig[XML_SAT_DESC];
        xmlconfig[XML_SAT_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* sat::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getDesc: cannot get Description as satelite is not configured" CR);
        return NULL;
    }
    return xmlconfig[XML_SAT_DESC];
}

const rc_t sat::setAddr(const uint8_t p_addr) {
    Log.error("sat::setAddr: cannot set Address - not supported" CR);
    return RC_NOTIMPLEMENTED_ERR;
}

const int8_t sat::getAddr(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getAddr: cannot get address, as satelite is not configured" CR);
        return -1; //WE NEED TO FIND AS STRATEGY AROUND RETURN CODES CODE WIDE
    }
    return atoi(xmlconfig[XML_SAT_ADDR]);
}

const int8_t sat::getLink(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("sat::getLink: cannot get Link No as satelite is not configured" CR);
        return -1; //WE NEED TO FIND AS STRATEGY AROUND RETURN CODES CODE WIDE
    }
    return linkHandle->getLink();
}

const void sat::setDebug(const bool p_debug) {
    debug = p_debug;
}

const bool sat::getDebug(void) {
    return debug;
}

uint32_t sat::getRxCrcErrs(void) {
    return remoteCrcErr;
}

void sat::clearRxCrcErrs(void) {
    remoteCrcErr = 0;
}

uint32_t sat::getTxCrcErrs(void) {
    return remoteCrcErr;
}

void sat::clearTxCrcErrs(void) {
    remoteCrcErr = 0;
}

uint32_t sat::getWdErrs(void) {
    return wdErr;
}

void sat::clearWdErrs(void) {
    wdErr = 0;
}

/*==============================================================================================================================================*/
/* END Class sat                                                                                                                                */
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
    if(i >= flashData->callbackSubs.size()){mastDecoder
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

uint8_t signalMastAspects::failsafeMastAppearance[SM_MAXHEADS];
QList<aspects_t*> signalMastAspects::aspects;

uint8_t signalMastAspects::onConfigUpdate(tinyxml2::XMLElement* p_smAspectsXmlElement){
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
/* Class: lg (lightGroup)                                                                                                                       */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
void lg::lg(const uint8_t p_lgAddress, const void* p_lgLinkHandle) : systemState(this) {
    Log.notice("lg::lg: Creating  light-group decoder %d on lgLink %d" CR, p_lgAddress, p_lgLinkHandle->getLink());
    lgAddress = p_lgAddress;
    lgLinkHandle = p_lgLinkHandle;
    regSysStateCb(onSystateChange);
    setOpState(OP_INIT | OP_UNCONFIGURED | OP_DISABLED | OP_UNAVAILABLE);
    lgLock = xSemaphoreCreateMutex();
    if (lgLock == NULL)
        panic("lg::init: Could not create Lock objects for light-group address %s on lgLink %d - rebooting...", lgAddress, lgLinkHandle->getLink());
}

void lg::~lg(void) {
    panic("lg::~lg: light-group destructor not supported - rebooting..." CR);
}

rc_t lg::init(void) {
    Log.notice("lg::init: Initializing light-group decoder %d on lgLink %d" CR, lgAddress, lgLinkHandle->getLink());
    return RC_OK;
}

rc_t lg::onConfigure(tinyxml2::XMLElement* p_lgDescXmlElement) {
    if (~(sysState.getOpState() & OP_UNCONFIGURED))
        panic("lg:onConfigUpdate: Received a light-group configuration for light-group address %d on lgLink %d, already configured, dynamic re-configuration not supported, opState: %d - rebooting...", lgAddress, lgLinkHandle->getLink(), getOpState());
    Log.notice("lg::onConfigUpdate: light-group %d on lgLink %d received an uverified configuration, parsing and validating it..." CR, lgAddress, lgLinkHandle->getLink());
    xmlconfig = new char* [5];
    xmlconfig[XML_LG_SYSNAME] = NULL;
    xmlconfig[XML_LG_USRNAME] = NULL;
    xmlconfig[XML_LG_DESC] = NULL;
    xmlconfig[XML_LG_TYPE] = NULL;
    xmlconfig[XML_LG_LINKADDR] = NULL;
    const char* searchLgTags[5];
    searchLgTags[XML_LG_SYSNAME] = "JMRISystemName";
    searchLgTags[XML_LG_USRNAME] = "JMRIUserName";
    searchLgTags[XML_LG_DESC] = "JMRIDescription";
    searchLgTags[XML_LG_TYPE] = "Type";
    searchLgTags[XML_LG_LINKADDR] = "LinkAddress";
    getTagTxt(p_lgDescXmlElement.FirstChildElement(), searchLgTags, xmlconfig, sizeof(searchLgTags) / 4); // Need to fix the addressing for portability
    if (~xmlconfig[XML_LG_SYSNAME])
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d missing systemName configuration - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (~xmlconfig[XML_LG_USRNAME])
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d missing userName configuration - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (~xmlconfig[XML_LG_DESC])
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d missing description configuration - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (~xmlconfig[XML_LG_TYPE])
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d missing light-group type configuration - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (~xmlconfig[XML_LG_LINKADDR])
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d missing link address configuration - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (atoi(xmlconfig[XML_LG_LINKADDR]) != lgAddress)
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d link address configuration not consistant with internal structures - rebooting...", lgAddress, lgLinkHandle->getLink());
    if (!strcmp(xmlconfig[XML_LG_TYPE], "SIGNAL MAST")) {
        Log.notice("lg::onConfigUpdate: light-group address %d on lgLink %d configured as a signal mast, starting a signal mast child object..." CR, lgAddress, lgLinkHandle->getLink());
        lgTypeHandle = new mastDecoder(lgAddress, this, lgLinkHandle);
        if (lgTypeHandle == NULL) {
            panic("lg::init: Could not create light-group type specific object for light-group address %d on lgLink %d - rebooting...", lgAddress, lgLinkHandle->getLink());
    }
    //else if other lgTypes
    else
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d was configured with an unknown/unsuported lgType %s - rebooting...", lgAddress, lgLinkHandle->getLink(), xmlconfig[XML_LG_TYPE]);
    rc_t rc;
    rc = lgTypeHandle.init();
    if (rc)
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d could not initiate child object, return code %d - rebooting...", lgAddress, lgLinkHandle->getLink(), rc);
    rc = lgTypeHandle->onConfigUpdate(p_lgDescXmlElement);
    if (rc)
        panic("lg::onConfigUpdate: light-group address %d on lgLink %d could not configure child object, return code %d - rebooting...", lgAddress, lgLinkHandle->getLink(), rc);
    Log.notice("lg::on_configUpdate: Configuration of light-group address %d on lgLink %d successfully finished" CR, lgAddress, lgLinkHandle->getLink());
    unSetOpState(OP_UNCONFIGURED);
}

rc_t lg::start(void) {
    Log.notice("lg::start: Starting light-group address %d on lgLink %d" CR, lgAddress, lgLinkHandle->getLink());
    if (getOpState() & OP_UNCONFIGURED) {
        Log.notice("lg::start: light-group address %d on light-group %d not configured - will not start it" CR, lgAddress, lgLinkHandle->getLink());
        setOpState(OP_UNUSED);
        unSetOpState(OP_INIT);
        return RC_NOT_CONFIGURED_ERR;
    }
    Log.notice("lg::start: light-group %s subscribing to adm- and op state topics", xmlconfig[XML_LG_SYSNAME]);
    const char* subscribeTopic[4] = {MQTT_DECODER_ADMSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onAdmStateChangeHelper, this))
        panic("lg::start: light-group %s failed to suscribe to admState topic - rebooting...", xmlconfig[XML_LG_SYSNAME]);
    subscribeTopic = {MQTT_DECODER_OPSTATE_TOPIC, mqtt::getUri(), "/", getSystemName()};
    if (mqtt::subscribeTopic(concatStr(subscribeTopic, 4), onOpStateChange, this))
        panic("lg::start: light-group %s failed to suscribe to opState topic - rebooting...", xmlconfig[XML_LG_SYSNAME]);
    rc_t rc;
    rc = lgTypeHandle.start();
    if (rc)
        panic("lg::start: light-group %s failed to suscribe to opState topic - rebooting...", xmlconfig[XML_LG_SYSNAME])
    unSetOpState(OP_INIT);
    Log.notice("lgLink::start: light-group %s has of type %s has successfully started" CR, xmlconfig[XML_LG_SYSNAME], xmlconfig[XML_LG_TYPE]);
    return RC_OK;

void lg::onSystateChange(const uint16_t p_sysState) {
    lgTypeHandle->onSystateChange(p_sysState);
}

void lg::onOpStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgObject) {
    ((lg*)p_lgObject).onOpStateChange(p_topic, p_payload);
}

void lg::onOpStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, AVAILABLE_PAYLOAD)) {
        unSetOpState(OP_UNAVAILABLE);
        Log.notice("lg::onOpStateChange: got a Light-Group available message for %s from server" CR, xmlconfig[XML_LG_SYSNAME]);
    }
    else if (!cmpstr(p_payload, UNAVAILABLE_PAYLOAD)) {
        setOpState(OP_UNAVAILABLE);
        Log.notice("lg::onOpStateChange: got a Light-Group unavailable message for %s from server" CR, xmlconfig[XML_LG_SYSNAME]);
    }
    else
        Log.error("lg::onOpStateChange: got an invalid Light-Group availability message for %s from server - doing nothing" CR, xmlconfig[XML_LG_SYSNAME]);
}

void lg::onAdmStateChangeHelper(const char* p_topic, const char* p_payload, const void* p_lgObject) {
    ((lg*)p_lgObject).onAdmStateChange(p_topic, p_payload);
}

void lg::onAdmStateChange(const char* p_topic, const char* p_payload) {
    if (!cmpstr(p_payload, ONLINE_PAYLOAD)) {
        unSetOpState(OP_DISABLED);
        Log.notice("lg::onAdmStateChange: got Light-Group online message for %s from server" CR, xmlconfig[XML_LG_SYSNAME]);
    }
    else if (!cmpstr(p_payload, OFFLINE_PAYLOAD)) {
        setOpState(OP_DISABLED);
        Log.notice("lg::onAdmStateChange: got Light-Group off-line message for %s from server" CR, xmlconfig[XML_LG_SYSNAME]);
    }
}

const rc_t lg::setSystemName(const char* p_systemName, const bool p_force = false) {
    Log.error("lg::setSystemName: cannot set Light-Group System name for %s - not suppoted" CR, xmlconfig[XML_LG_SYSNAME]);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* lg::getSystemName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getSystemName: cannot get Light-Group System name as Light-group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    }
    return xmlconfig[XML_LG_SYSNAME];
}

const rc_t lg::setUsrName(const char* p_usrName, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("lg::setUsrName: cannot set Light-Group User name as debug is inactive for %s" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::setUsrName: cannot set Light-Group System name as Light-group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("lg::setUsrName: Setting Light-Group User name for %s to %s" CR, , xmlconfig[XML_LG_SYSNAME], p_usrName);
        delete xmlconfig[XML_LG_USRNAME];
        xmlconfig[XML_LG_USRNAME] = createNcpystr(p_usrName);
        return RC_OK;
    }

const char* lg::getUsrName(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getUsrName: cannot get Light-Group User name as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    }
    return xmlconfig[XML_LG_USRNAME];
}
    if (!debug || !p_force) {
        Log.error("lgLink::setDesc: cannot set Light-Group Description as debug is inactive for %s" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lgLink::setDesc: cannot set Light-Group Description as Light-group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("lgLink::setDesc: Setting Light-Group Description for %s to %s" CR, xmlconfig[XML_LG_SYSNAME], p_description);
        delete xmlconfig[XML_LGLINK_DESC];
        xmlconfig[XML_LGLINK_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

rc_t lg::setDesc(const char* p_description, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("lg::setDesc: cannot set Light-Group Description as debug is inactive for %s" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::setDesc: cannot set Light-Group Description as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_NOT_CONFIGURED;
    }
    else {
        Log.notice("lg::setDesc: Setting Light-Group Description for %s to %s" CR, xmlconfig[XML_LG_SYSNAME], p_description);
        delete xmlconfig[XML_LG_DESC];
        xmlconfig[XML_LG_DESC] = createNcpystr(p_description);
        return RC_OK;
    }
}

const char* lg::getDesc(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getDesc: cannot get Light-Group Description as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    }
    return xmlconfig[XML_LG_DESC];
}

const char* lg::setLgType(const char* p_lgType, const bool p_force = false) {
    Log.error("lg::setLgType: cannot set Light-group type for %s - not suppoted" CR, xmlconfig[XML_LG_SYSNAME]);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* lg::getLgType(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getLgType: cannot get Light-Group type as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    }
    return xmlconfig[XML_LG_TYPE];
}

const char* lg::setLinkAddr(const uint8_t p_linkAddress, , const bool p_force = false) {
    Log.error("lg::setLinkAddr: cannot set Light-group link address for %s - not suppoted" CR, xmlconfig[XML_LG_SYSNAME]);
    return RC_NOTIMPLEMENTED_ERR;
}

const char* lg::getLinkAddr(void) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getLgType: cannot get Light-group link address as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    }
    return xmlconfig[XML_LG_ADRESS];
}

void lg::setProperty(const uint8_t p_propertyId, const char* p_propertyValue, const bool p_force = false) {
    if (!debug || !p_force) {
        Log.error("lg::setProperty: cannot set Light-Group Property as debug is inactive for %s" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_DEBUG_NOT_SET_ERR;
    }
    else if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::setProperty: cannot set Light-Group Property as Light-Group %s is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return RC_NOT_CONFIGURED;
    }
    else
        return lgTypeHandle->setProperty(p_propertyId, p_propertyValue);
}

const char* lg::getProperty(const uint8_t p_propertyId) {
    if (getOpState() & OP_UNCONFIGURED) {
        Log.error("lg::getProperty: cannot get Light-group link address as Light-Group is not configured" CR, xmlconfig[XML_LG_SYSNAME]);
        return NULL;
    return lgTypeHandle->getProperty(p_propertyId);
}

void lg::setDebug(const bool p_debug) {
    debug = p_debug;
}

const bool lg::getDebug(void) {
    return debug;
}

void lg::setStripOffset(const uint16_t p_stripOffset) {
    stripOffset = p_stripOffset
        return;
}

const uint16_t lg::getStripOffset() {
    return stripOffset;
}

IM HERE - NEXT IS TO IMPLEMENT CORRESPONDING GET/SET IN MAST DECODER



/*==============================================================================================================================================*/
/* Class: mastDecoder                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
mastDecoder::mastDecoder(const uint8_t p_lgAddress, const void* p_lgBaseObjHandle, const void* p_lgLinkHandle){
    Log.notice("mastDecoder::mastDecoder: Creating mast decoder %d" CR, p_lgAddress);
    lgAddress = p_lgAddress;
    lgBaseObjHandle = p_lgBaseObjHandle;
    lgLinkHandle = p_lgLinkHandle;
    mastDecoderLock = xSemaphoreCreateMutex();
    mastDecoderReentranceLock = xSemaphoreCreateMutex();
    if (mastDecoderLock == NULL || mastDecoderReentranceLock == NULL)
        panic("mastDecoder::init: Could not create Lock objects - rebooting...");
}

mastDecoder::~mastDecoder(void){
  panic("mastDecoder::~mastDecoder: Destructor not supported - rebooting..." CR);
}

rc_t mastDecoder::init(void){
  Log.notice("mastDecoder::init: Initializing mast decoder" CR);
  //failsafe();
  return RC_OK;
}
rc_t mastDecoder::onConfigure(tinyxml2::XMLElement* p_mastDescXmlElement){
  if (~(lgBaseObjHandle->getOpState() & OP_UNCONFIGURED))
      panic("mastDecoder:onConfigUpdate: Received a configuration, while the it was already configured, dynamic re-configuration not supported, opState: %d - rebooting...", lgBaseObjHandle->getOpState());
  if (p_mastDescXmlElement == NULL) {
      Log.fatal("mastDecoder::onConfigure: No mastDescXml provided - rebooting..." CR);
  Log.notice("mastDecoder::onConfigUpdate:  %d on lgLink %d received an uverified configuration, parsing and validating it..." CR, lgAddress, lgLinkHandle->getLink());
  const char* searchMastTags[4];
  searchMastTags[SM_TYPE] = "Property1";
  searchMastTags[SM_DIMTIME] = "Property2";
  searchMastTags[SM_FLASHFREQ] = "Property3";
  searchMastTags[SM_BRIGHTNESS] = "Property4"; //NEEDS TO BE IMPLEMENTED BY SERVER SIDE
  searchMastTags[SM_FLASH_DUTY] = "Property5"; //NEEDS TO BE IMPLEMENTED BY SERVER SIDE
  char* xmlConfig[4];
  getTagTxt(p_mastDescXmlElement->FirstChildElement(), searchMastTags, xmlConfig, sizeof(searchMastTags)/4); // Need to fix the addressing for portability
  if(xmlConfig[SM_TYPE] == NULL ||
     xmlConfig[SM_DIMTIME] == NULL ||
     xmlConfig[SM_FLASHFREQ] == NULL ||
     xmlConfig[SM_BRIGHTNESS] == NULL) ||
     xmlConfig[SM_FLASH_DUTY] == NULL) ||)
     panic("mastDecoder::onConfigure: mastDescXml missformated - rebooting...")
  lgNoOfLed = lgLinkHandle->getSignalMastAspectObj()->getNoOfHeads(xmlConfig[SM_TYPE]);
  appearance = new uint8_t[lgNoOfLed];
  appearanceWriteBuff = new uint8_t[lgNoOfLed];
  appearanceDimBuff = new uint16_t[lgNoOfLed];
  if(!strcmp(xmlConfig[SM_DIMTIME], "NORMAL")){
    smDimTime = SM_DIM_NORMAL;
  }
  else if(!strcmp(xmlConfig[SM_DIMTIME], "FAST")){
    smDimTime = SM_DIM_FAST;
  }
  else if(!strcmp(xmlConfig[SM_DIMTIME], "SLOW")){
    smDimTime = SM_DIM_SLOW;
  }
  else {
    Log.error("mastDecoder::onConfigure: smDimTime is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    smDimTime = SM_DIM_NORMAL;
  }
  if(!strcmp(xmlConfig[SM_BRIGHTNESS], "HIGH")){
    smBrightness = SM_BRIGHNESS_HIGH;
  }
  else if(!strcmp(xmlConfig[SM_BRIGHTNESS], "NORMAL")){
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  else if(!strcmp(xmlConfig[SM_BRIGHTNESS], "LOW")){
    smBrightness = SM_BRIGHNESS_LOW;
  }
  else{
    Log.error("mastDecoder::onConfigure: smBrighness is non of HIGH, NORMAL or LOW - using NORMAL..." CR);
    smBrightness = SM_BRIGHNESS_NORMAL;
  }
  return RC_OK;
}

rc_t mastDecoder::start(void){
  Log.notice("mastDecoder::start: Starting mast decoder %s" CR, p_lgBaseObjHandle->getSystemName());
  if(strcmp(xmlConfig[SM_FLASHFREQ], "FAST")){
      lgLinkHandle->getFlashObj(SM_FLASH_TYPE_FAST)->subscribe(mastDecoder::onFlashHelper, this);
  } 
  else if(strcmp(xmlConfig[SM_FLASHFREQ], "NORMAL")){
      lgLinkHandle->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else if(strcmp(xmlConfig[SM_FLASHFREQ], "SLOW")){
      lgLinkHandle->getFlashObj(SM_FLASH_TYPE_SLOW)->subscribe(mastDecoder::onFlashHelper, this);
  }
  else{
    Log.error("mastDecoder::start: smFlashFreq is non of FAST, NORMAL or SLOW - using NORMAL..." CR);
    lgLinkHandle->getFlashObj(SM_FLASH_TYPE_NORMAL)->subscribe(mastDecoder::onFlashHelper, this);
  }
  char lgAddrTxtBuff[5];
  const char* subscribeTopic[5] = {MQTT_ASPECT_TOPIC, mqtt::getUri(), "/", lgBaseObjHandle->getSystemName(), "/"};
  mqtt::subscribeTopic(concatStr(subscribeTopic, 5), mastDecoder::onAspectChangeHelper, this);
  return RC_OK;
}

void mastDecoder::onSystateChange(const uint16_t p_sysState) {
    if (p_systate & OP_INTFAIL) {
        //FAILSAFE
        panic("lg::onSystateChange: Signal-mast %d on lgLink %d has experienced an internal error - seting fail-safe aspect and rebooting...", lgAddress, lgLinkHandle->getLink());
    }
    if (p_sysState) {
        //FAILSAFE
        Log.notice("lgLink::onSystateChange: Signal-mast %d on lgLink %d has received Opstate %b - seting fail-safe aspect" CR, lgAddress, lgLinkHandle->getLink(), p_sysState);
    }
    else {
        //RESUME LAST RECEIVED ASPECT
        Log.notice("lgLink::onSystateChange:  Signal-mast %d on lgLink %d has received a WORKING Opstate - resuming last known aspect" CR, linkNo);
    }
}

const mastDecoder::setProperty(const uint8_t p_propertyId, const char* p_propertyValue) {
    Log.notify("mastDecoder::setProperty: Setting light-group property for %s, property Id %d, property value %s" CR, lgBaseObjHandle->getSystemName(), p_propertyId, p_propertyValue);
    ......

void mastDecoder::onAspectChange(const char* p_topic, const char* p_payload){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  xSemaphoreTake(mastDecoderLock, portMAX_DELAY);
  if(lgBaseObjHandle->getOpState()){
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
  Log.verbose("mastDecoder::onAspectChange: A new aspect: %s received for signal mast %s" CR, aspect, lgBaseObjHandle->getSystemName());

  getAppearance(xmlConfig[SM_TYPE], aspect, &appearance);
  for(uint8_t i=0; i<lgNoOfLed; i++){
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
        Log.error("mastDecoder::onAspectChange: The appearance is none of LIT, UNLIT, FLASH or UNUSED - setting mast to SM_BRIGHNESS_FAIL and continuing..." CR);
        appearanceWriteBuff[i] = SM_BRIGHNESS_FAIL; //HERE WE SHOULD SET THE HOLE MAST TO FAIL ASPECT
    }
  }
  lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset(), lgNoOfLed, appearanceWriteBuff, appearanceDimBuff);
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
}

rc_t mastDecoder::parseXmlAppearance(const char* p_aspectXml, char* p_aspect){
  tinyxml2::XMLDocument aspectXmlDocument;
  if(aspectXmlDocument.Parse(p_aspectXml) || aspectXmlDocument.FirstChildElement("Aspect") == NULL || aspectXmlDocument.FirstChildElement("Aspect")->GetText() == NULL){
    Log.error("mastDecoder::parseXmlAppearance: Failed to parse the new aspect - continuing..." CR);
    return RC_PARSE_ERR;
  }
  strcpy(p_aspect, aspectXmlDocument.FirstChildElement("Aspect")->GetText());
  return RC_OK;
}

void mastDecoder::onFlash(const bool p_flashState){
  xSemaphoreTake(mastDecoderReentranceLock, portMAX_DELAY);
  flashOn = p_flashState;
  for (uint16_t i=0; i<lgNoOfLed; i++){
    if(appearance[i] == FLASH_APPEARANCE){
      if(flashOn){
          lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &smBrightness, &smDimTime);
      }
      else{
        uint8_t zero = 0;
        lgLinkHandle->updateLg(lgBaseObjHandle->getStripOffset() + i, (uint8_t)1, &zero, &smDimTime);
      }
    }
  }
  xSemaphoreGive(mastDecoderReentranceLock);
  return;
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
  decoder::init();
  decoder::start();
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
