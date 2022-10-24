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
#include "cli.h"
/*==============================================================================================================================================*/
/* END Include files                                                                                                                            */
/*==============================================================================================================================================*/



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

void decoderCli::init(void) {
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
            6 * 1024,                     // Stack size
            NULL,                       // Parameter passing
            CLI_POLL_PRIO,              // Priority 0-24, higher is more
            NULL,                       // Task handle
            CLI_POLL_CORE);             // Core [CORE_0 | CORE_1]
    }
    else {
        Log.error("decoderCli::init: Failed to start CLI" CR);
    }
}

void decoderCli::onTelnetConnect(String ip) {
    connections++;
    Log.notice("decoderCli::onTelnetConnect: CLI connected from: %s" CR, ip);
    telnet.print("\r\nWelcome to JMRI generic decoder CLI - version: \r\nConnected from: ");
    telnet.print(ip);
    telnet.print("\r\n\nType help for Help\r\n\n");
    printToCli("");
}

void decoderCli::onTelnetDisconnect(String ip) {
    connections--;
    Log.notice("decoderCli::onTelnetDisconnect: CLI disconnected from: %s" CR, ip);
}

void decoderCli::onTelnetReconnect(String ip) {
    Log.notice("decoderCli::onTelnetReconnect: CLI reconnected from: %s" CR, ip);
}

void decoderCli::onTelnetConnectionAttempt(String ip) {
    Log.notice("decoderCli::onTelnetConnectionAttempt: CLI connection failed from: %s" CR, ip);
}

void decoderCli::onTelnetString(String p_cmd) {
    if (currentCliContext != newCliContext) {
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

void decoderCli::registerCliContext(cliContext_t* p_context) {
    cliContextList->push_back(p_context);
}

void decoderCli::onCliCommand(cmd* p_cmd) {
    if (contextCommonCliCmd(p_cmd) == CLI_PARSE_CONTINUE) {
        if (currentCliContext->cliContextCb(p_cmd) == CLI_PARSE_CONTINUE) {
            printToCli("Failed to parse CLI command or its arguments");
        }
    }
}

cliContext_t* decoderCli::getCliContext(char* p_context) {
    bool found = false;
    for (uint8_t i = 0; i < cliContextList->size(); i++) {
        if (!strcmp(cliContextList->at(i)->cliContext, p_context)) {
            found = true;
            return cliContextList->at(i);
        }
    }
    return NULL;
}

void decoderCli::setCliContext(cliContext_t* p_context) {
    resetCliContext();
    commonCliContextInit();
    p_context->cliContextInitCb();
    currentCliContext = p_context;
}

void decoderCli::resetCliContext(void) {
    if (cli != NULL) {
        delete cli;
    }
    cli = new SimpleCLI;

    help = cli->addCmd("help", onCliCommand);
    set = cli->addCmd("set", onCliCommand);
    get = cli->addCmd("get", onCliCommand);
    exec = cli->addCmd("exec", onCliCommand);
}

void decoderCli::printToCli(String p_output, bool partial) {
    telnet.print(p_output);
    if (!partial) {
        telnet.print("\r\n\n");
        telnet.print(">> ");
    }
}

void decoderCli::commonCliContextInit(void) {
    set.addArg("context");
    get.addFlagArgument("context");
    get.addFlagArgument("uptime");
    get.addFlagArgument("time");
    get.addFlagArgument("cpu");
    get.addFlagArgument("mem/ory");
    exec.addFlagArgument("reboot");
}

uint8_t decoderCli::contextCommonCliCmd(cmd* p_cmd) {
    Command cmd(p_cmd);
    String command;
    printToCli("Command accepted - parsing..\n\r", true);
    command = cmd.getName();
    if (command == "help") {
        printToCli("This is common help text");
        return CLI_PARSE_CONTINUE;
    }
    if (command == "set") {
        Argument setContextTopic = cmd.getArgument("context");
        String argBuff;
        if (argBuff = setContextTopic.getValue()) {
            if (newCliContext = getCliContext((char*)argBuff.c_str())) {
                printToCli("Context is now: ", true);
                printToCli(newCliContext->cliContext);
            }
            else {
                telnet.print("ERROR - context: ");
                telnet.print(argBuff);
                telnet.print(" does not exist.\n");
            }
        }
    }

    if (command == "get") {
        Argument getContextTopic = cmd.getArgument("context");
        Argument getUptimeTopic = cmd.getArgument("uptime");
        Argument getCpuTopic = cmd.getArgument("cpu");
        Argument getTimeTopic = cmd.getArgument("time");
        Argument getMemTopic = cmd.getArgument("mem/ory");

        if (getContextTopic.isSet()) {
            printToCli("Current context: ", true);
            printToCli(currentCliContext->cliContext);
            return CLI_PARSE_STOP;
        }

        if (getUptimeTopic.isSet()) {
            printToCli("System uptime: ", true);
            char uptime[10];
            printToCli(itoa(esp_timer_get_time() / 1000000, uptime, 10), true);
            printToCli(" Seconds");
            return CLI_PARSE_STOP;
        }

        //get -cpu cli command
        if (getCpuTopic.isSet()) {
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

        if (getTimeTopic.isSet()) {
            time_t rawtime;
            struct tm* timeinfo;
            time(&rawtime);
            timeinfo = localtime(&rawtime);
            printToCli("Local time: ", true);
            printToCli(asctime(timeinfo));
            return CLI_PARSE_STOP;
        }
        if (getMemTopic.isSet()) {
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
    if (command == "exec") {
        Argument rebootTopic = cmd.getArgument("reboot");
        if (rebootTopic.isSet()) {
            ESP.restart();
            return CLI_PARSE_STOP;
        }
    }
}

void decoderCli::rootCliContextInit(void) {
}

uint8_t decoderCli::onRootCliCmd(cmd* p_cmd) {
}

void decoderCli::cliPoll(void* dummy) {
    Log.notice("decoderCli::cliPoll: CLI polling started" CR);
    while (true) {
        telnet.loop();
        if (connections) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

/*==============================================================================================================================================*/
/* End Class cli                                                                                                                                */
/*==============================================================================================================================================*/
