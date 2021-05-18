MPYEMULATEDTARGET = 0
MPYTARGET = 1
CPYTARGET = 2
TARGET = MPYTARGET

import time
if TARGET == CPYTARGET :
    import xml.etree.ElementTree as ET
    import paho.mqtt.client as mqtt
elif TARGET == MPYEMULATEDTARGET :
    import micropython
    from umqtt.simple import MQTTClient as mqtt
    import xml.etree.ElementTree as ET
elif TARGET == MPYTARGET :
    import micropython
    import ntptime
    import ElementTree as ET
    from simple import MQTTClient as mqtt
    from machine import Pin
    from machine import Timer
    from neopixel import NeoPixel

import io
import inspect
import sys
import _thread
import network


#==============================================================================================================================================
# Constants
#==============================================================================================================================================
# Logging constants
DEBUG_VERBOSE = 4
DEBUG_TERSE = 3
INFO = 2
ERROR = 1
PANIC = 0
MQTT_LOG_TOPIC = "/trains/tracks/log/"

#OP states
OP_NONE = "None"
OP_INIT = "Init"
OP_CONFIG = "Configured"
OP_UNUSED = "Unused"
OP_CONNECTED = "Connected"
OP_WORKING = "Working"
OP_FAIL = "Failed"

# XML parse filters
MANSTR = {"expected" : True, "type" : "str", "values" : None, "except": "PANIC"}
OPTSTR = {"expected" : None, "type" : "str", "values" : None, "except": "PANIC"}
MANINT = {"expected" : True, "type" : "int", "values" : None, "except": "PANIC"}
OPTINT = {"expected" : None, "type" : "int", "values" : None, "except": "PANIC"}
MANSPEED = {"expected" : True, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANSPEED = {"expected" : None, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANLVL = {"expected" : True, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
OPTLVL = {"expected" : None, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}

#==============================================================================================================================================
# Decoder parameters
#==============================================================================================================================================
# Wifi parameters
WIFI_SSID = "Brevik-guest"                                                              #Todo to do WPS

# MQTT parameters
BROKER="test.mosquitto.org"                                                             #TODO use DNS

DEFAULTLOGLEVEL = DEBUG_VERBOSE
DEFAULTNTPSERVER = "pool.ntp.org"
DEFAULTTIMEZONE = 0 #UTC

# Signal mast transition times
SM_TRANSITION_NORMAL = 75
SM_TRANSITION_FAST = 50
SM_TRANSITION_SLOW = 100

# Signal mast flash frequences
SM_FLASH_NORMAL = 1
SM_FLASH_FAST = 1.5
SM_FLASH_SLOW = 0.75

# Signal mast brightness
SM_BRIGHTNESS_NORMAL = 40
SM_BRIGHTNESS_HIGH = 80
SM_BRIGHTNESS_LOW = 20

#==============================================================================================================================================
# PIN Assignments
#==============================================================================================================================================
LEDSTRIP_CH0_PIN = 21



def parse_xml(xmlTree,tagDict) :
#   tagDict: {"Tag" : {"expected" : bool, "type" : "int/float/int", "values" : [], "except": "error/panic/info}, ....}
    res = {}
    for child in xmlTree :
        tagDesc = tagDict.get(child.tag)

        if tagDesc == None :
            continue
        value = validateXmlText(child.text.strip(), tagDesc.get("type"), tagDesc.get("values"))
        if tagDesc.get("expected") == None :
            if value != None :
                res[child.tag] = value
            else : through_xml_error(tagDesc.get("except"), "Tag: " + child.tag + " Tag.text: " + child.text + " did not pass type checks")
        elif tagDesc.get("expected") == True : 
            if value != None :
                res[child.tag] = value
            else : through_xml_error(tagDesc.get("except"), "Tag: " + child.tag + " Tag.text: " + child.text + " did not pass type checks")
        else : through_xml_error(tagDesc.get("except"), "Tag: " + child.tag + " was not expected")

    for tag in tagDict :
        if tagDict.get(tag).get("expected") != None :
            if res.get(tag) == None :
                if tagDict.get(tag).get("expected") == True : through_xml_error(tagDict.get(tag).get("except"), "Tag: " + tag + " was expected but not found")
    return res

def validateXmlText(txt, type, values) :
    if txt == None :
        return None
    if type == None :
        return txt
    elif type == "str" : res = str(txt) 
    elif type == "int" : res = int(txt)
    elif type == "float" : res = float(txt)
    else : 
        return None

    if values == None :
        return res
    else :
        for value in values :
            if res == value :
                return res
        return None

def through_xml_error(_except, errStr) :
    if _except == "DEBUG_VERBOSE" : debug = DEBUG_VERBOSE
    elif _except == "DEBUG_TERSE" : debug = DEBUG_TERSE
    elif _except == "INFO" : debug = INFO
    elif _except == "ERROR" : debug = ERROR
    elif _except == "PANIC" : debug = PANIC
    else : debug = PANIC
    notify(None, debug, errStr)
    return 0



#==============================================================================================================================================
# Functions: "timeSync"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
def timeSync(ntpServer, timeZone = 0) :
    print("Syncing time")
    #ntptime.settime(timeZone, ntpServer)


#==============================================================================================================================================
# Class: "timer"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
def timer(time, cb) :
    return TIMER.insertTimer(int(time*1000), cb)

class timer_thread :
    def __init__(self) :
        self.time = 0
        self.timeTable = []
        self.lock = _thread.allocate_lock()
        _thread.start_new_thread(self.runTimer,())

    def insertTimer(self, timeout, cb) :
        (self.lock).acquire()
        cnt = 0
        timeoutDesc = [timeout + self.time, cb]
        if not self.timeTable :
            self.timeTable.insert(0, timeoutDesc)
        else :
            inserted = False
            for timeouts in self.timeTable :
                if timeouts[0] > timeout + self.time :
                    self.timeTable.insert(cnt, timeoutDesc)
                    inserted = True
                    break
                cnt += 1
            if not inserted :
                self.timeTable.insert(cnt, timeoutDesc)
        (self.lock).release()
        return 0

    def runTimer(self) :
        while True :
            if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
                currentTime = time.ticks_ms()
            elif TARGET == CPYTARGET :
                currentTime = int(time.time() * 1000)
            timeouts = []
            timoutbool = False
            poplevels = 0
            (self.lock).acquire()
            if self.timeTable :
                for timeoutDesc in self.timeTable :
                    if timeoutDesc[0] <= self.time :
                        timoutbool = True
                        poplevels += 1
                        cb = timeoutDesc[1]
                        timeouts.insert(0, cb)
                    else :
                        break
            while poplevels :
                del self.timeTable[0]
                poplevels -= 1
            (self.lock).release()
            for callback in timeouts :
                callback()
            self.time += 10
            if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
                nextTime = 0.01-(time.ticks_ms()- currentTime)/1000
            elif TARGET == CPYTARGET :
                nextTime = 0.01-(int(time.time()*1000)- currentTime)/1000

            if nextTime < 0 :
                nextTime = 0
            time.sleep(nextTime)




#==============================================================================================================================================
# Function: do_connect
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
def do_connect():
    sta_if = network.WLAN(network.STA_IF)
    if not sta_if.isconnected():
        notify(0, INFO, "connecting to network...") # FIX
        print('connecting to network...')
        sta_if.active(True)
        sta_if.connect(WIFI_SSID, "")
        while not sta_if.isconnected():
            time.sleep(0.1)
        notify(0, INFO, "network config: " + str(sta_if.ifconfig())) # FIX
    return 0


#==============================================================================================================================================
# Class: "trace"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
def notify(caller, severity, notificationStr) :
    TRACE.notify(caller, severity, notificationStr)

class trace :
    def __init__(self) :
        self.debugLevel = DEBUG_TERSE
        self.debugClasses = None
        self.console = True
        self.mqtt = False

    def setDebugLevel(self, debugLevel, output = {"Console" :  True, "rSyslog" : False, "Mmqtt" : False}, debugClasses = None, errCallStack = True, errTerminate = False) :
        self.errCallStack = errCallStack
        self.errTerminate = errTerminate
        self.debugLevel = debugLevel
        self.console = output.get("Console")
        self.rSyslog = output.get("rSyslog")
        self.mqtt = output.get("Mqtt")
        if debugClasses == None :
            self.debugClasses = None
        else :
            self.debugClasses = debugClasses

    def notify(self, caller, severity, notificationStr) :
        if severity == DEBUG_VERBOSE :
            if self.debugLevel >= DEBUG_VERBOSE :
                notification = str(time.time()) + ": VERBOSE DEBUG:  " + notificationStr
                if self.debugClasses == None :
                    self.__deliverNotification(notification)
                    return
                else :
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == DEBUG_TERSE :
            if self.debugLevel >= DEBUG_TERSE :
#                notification = time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.localtime()) + ": TERSE DEBUG:  " + notificationStr
                notification = str(time.time()) + ": TERSE DEBUG:  " + notificationStr
                if self.debugClasses == None :
                    self.__deliverNotification(notification)
                    return
                else :
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == INFO :
            if self.debugLevel >= INFO :
#                notification = time.strftime("%a, %d %b %Y %H:%M:%S +0000", time.localtime()) + ": INFO:  " + notificationStr
                notification = str(time.time()) + ": INFO:  " + notificationStr

                if self.debugClasses == None :
                    self.__deliverNotification(notification)
                    return
                else :
                    self.__classNotification(caller, notification)
                    return
            return

        if severity == ERROR :
            if self.errCallStack == True :
                notification = str(time.time()) + ": ERROR!:  " + notificationStr + "\nCall Stack:\n" + str(inspect.stack())
            else :
                notification = str(time.time()) + ": ERROR!:  " + notificationStr
            self.__deliverNotification(notification)
            if self.errTerminate == True :
                # Call back to registered entries for emergency
                _thread.start_new_thread(self.terminate,(notification,))
            else :
                return

        if severity == PANIC :
            notification = str(time.time()) + ": PANIC!:  " + notificationStr + "\nCall Stack:\n" + str(inspect.stack())
            # Call back to registered entries for emergency
            _thread.start_new_thread(self.terminate,(notification,))
            return 0

    def terminate(self, notification) :
        self.__deliverNotification(notification)
        MQTT.sendMessage("/trains/track/supervision/" + URL + "/", "offLine")
        print("Terminating ....")
        time.sleep(1)
        _thread.interrupt_main()
        sys.exit() # Needs to be ported to ESP32 to restart the machine
        return

    def __classNotification(self, caller, notification) :                   #Currently not supported
        callerClass = type(itertools.count(0)).__name__
        for debugClass in self.debugClasses :
            if debugClass == callerClass :
                self.__deliverNotification(notification)
                return

    def __deliverNotification(self, notification) :
        if self.console == True : print(notification)
        #if self.rSyslog == True :
            #RSYSLOG.post(notiffication)
        if self.mqtt == True : self.mqttPostNotification(notification)

    def mqttPostNotification(self, notification) :
        sendMessage(MQTT_LOG_TOPIC + "/" + URL + "/", notification)

    def rsyslogNotification (self) :
        pass




#==============================================================================================================================================
# Class: "decoderMqtt"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
class decoderMqtt :
    def __init__(self, broker, URL, qos=0) :
        self.opState = OP_INIT
        self.connected = False
        self.broker = broker
        self.URL = URL
        self.qos = qos
        self.maxMessageCallbacks = 64
        self.mqttTopicMessageCallbacks = [("", "", "", "")]*self.maxMessageCallbacks #("Topic", "TopicRegExp", "TopicRegexpRes", "Callback")
        self.lock = _thread.allocate_lock()

    def start(self) :
        if TARGET == MPYEMULATEDTARGET or TARGET == MPYTARGET :
            self.client = mqtt("GenDecoderMqttClient", self.broker, port = 1883, keepalive = 60)
            cnt = 0
            while (self.client).connect() != 0 :
                time.sleep(0.1)
                cnt += 1
                if cnt == 600 :
                    notify(self, PANIC, "MQTT Could not connect to broker: " + self.broker)
                    return 1
            self.__on_connect(None, None, None, 0)

        elif TARGET == CPYTARGET :
            self.client = mqtt.Client()
            (self.client).on_connect=self.__on_connect
            (self.client).connect(self.broker, port=1883, keepalive=60)
            cnt = 0
            while not self.connected :
                (self.client).loop()
                time.sleep(0.1)
                if cnt == 600 :
                    notify(self, PANIC, "MQTT Could not connect to broker: " + self.broker)
                    return 1

    def __on_connect(self, client, userdata, flags, rc) :
        print("onConnect")
        if rc == 0 :
            self.connected = True
            self.opState = OP_CONNECTED
            if TARGET == MPYEMULATEDTARGET or TARGET == MPYTARGET :
                (self.client).set_callback(self.__on_mpy_message)
                (self.client).set_last_will("/trains/track/supervision/" + self.URL + "/", "offLine", qos=self.qos, retain=True)
            elif TARGET == CPYTARGET :
                (self.client).on_message = self.__on_message
                (self.client).will_set("/trains/track/supervision/" + self.URL + "/", payload="offLine", qos=self.qos, retain=True)

            notify(self, INFO, "MQTT Connected to Broker: " + self.broker) 
            #Need to subscribe on the supervision topic
            self.sendMessage("/trains/track/supervision/" + self.URL + "/", payload="onLine")
            print("MQTT looping starting")
            self.__mqtt_loop()    #HW timer driven loop
            print("MQTT looping started")
        else :
            self.opState = OP_FAIL
            notify(self, PANIC, "Could not connect to broker, Return code: " + str(rc)) 

    def __mqtt_loop(self) :
        timer(0.1, self.__mqtt_loop)
        if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
            (self.lock).acquire()
            (self.client).check_msg()
            (self.lock).release()
            return 0
        elif TARGET == CPYTARGET :
            (self.lock).acquire()
            print("IM Here")
            (self.client).loop(timeout=0)
            (self.lock).release()
            return 0

    def __on_mpy_message(self, topic, payload) :
        message = __message(topic, payload)
        self.__on_message(None, None, message)
        return 0

    def __on_message(self, client, userdata, message) :
        notify(self, DEBUG_TERSE, "Got an MQTT message, Topic: " + str(message.topic) + " Payload: " + str(message.payload)) 
        mqttTopicMessageCallbacksIndex = 0
        while mqttTopicMessageCallbacksIndex < self.maxMessageCallbacks :
            (regTopic, regTopicRegExp, regTopicRegexpRes, regCallback) = self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex]
            if regCallback != "" :
                if isinstance(message.topic, str) : recTopic = message.topic
                else : recTopic = message.topic.decode('UTF-8')

                if regTopicRegExp == "" and recTopic == regTopic :
                    regCallback(self, recTopic, message.payload, "")
                else :
                    pass #Not supported
                    #res = re.search(topicRegExp, topic)
                    #if res == topicRegexpRes :
                    #notify(self, DEBUG_VERBOSE, "Routing MQTT message, Topic: " + str(message.topic) + " Payload: " + str(message.payload) + " to callback: " + str(callback))
                    #callback (self, message.topic, message.payload, res)

            mqttTopicMessageCallbacksIndex += 1

    def registerMessageCallback (self, regCallback, regTopic, regRegexp="", regRegexpResult="") : # Redo the list handeling to dynamic extention
        notify(self, DEBUG_TERSE, "Registering MQTT callback for Topic: " + str(regTopic) + " Topic Regexp: " + regRegexp + " Topic Regexp result: " + regRegexpResult + "Callback: " + str(regCallback))
        mqttTopicMessageCallbacksIndex = 0

        while mqttTopicMessageCallbacksIndex < self.maxMessageCallbacks :
            (topic, topicRegExp, topicRegexpRes, callback) = self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex]
            # We need to look if the same search topic allready exist, any impact on "__on_message"
            if callback == "" :
                self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex] = (regTopic, regRegexp, regRegexpResult, regCallback)
                (self.lock).acquire()
                (self.client).subscribe(regTopic)
                (self.lock).release()
                notify(self, DEBUG_TERSE, "Registered MQTT callback for Topic: " + str(regTopic) + " Topic Regexp: " + regRegexp + " Topic Regexp result: " + regRegexpResult + "Callback: " + str(regCallback))
                return 0
            mqttTopicMessageCallbacksIndex += 1
        return 1

    def sendMessage (self, topic, payload) :
        (self.lock).acquire()
        (self.client).publish(topic + self.URL + "/", payload, qos=self.qos, retain=True)
        notify(self, DEBUG_TERSE, "Sent an MQTT message, Topic: " + str(topic) + " Payload: " + str(payload))
        (self.lock).release()

    def getOpState(self) :
        return self.opState

    def evalVar(self, var) :
        try : eval("self." + var)
        except : return (1, None)
        else : return (0, eval("self." + var))

class __message :                           # A helper class
    def __init__(self, topic, payload) :
        self.topic = topic
        self.payload = payload




#==============================================================================================================================================
# Class: "topDecoder"
# Purpose: The "topDecoder" class implements a singlton object responsible for setting up the common decoder mqtt class object, subscribing
#          to the management/configuration topic, parsing the top level xml configuration and forwarding propper xml configuration segments
#          to the different decoder services - E.g. Lightgroups [Signal Masts | general Lights | sequencedLights], Turnouts or Sensors.
#          "topDecoder" sequences the start up of the the different decoder services.
#          "topdecoder holds the decoder infrastructure config such as ntp-, rsyslog-, ntp-, etc endpoints 
# Data structures: 
# Methods:
#==============================================================================================================================================
class topDecoder :
    def __init__(self, broker, URL) :
        notify(self, INFO, "Top decoder initialized")
        self.opState = OP_INIT
        self.broker = broker
        self.URL = URL
        self.configured = False
        global LOGLEVEL
        LOGLEVEL = DEFAULTLOGLEVEL
        global NTPSERVER
        NTPSERVER = DEFAULTNTPSERVER
        global TIMEZONE
        TIMEZONE = DEFAULTTIMEZONE
        global MQTT
        MQTT = decoderMqtt(self.broker, self.URL)
        notify(self, INFO, "MQTT Initialized, handle: " + str(MQTT))
        MQTT.start()
        notify(self, INFO, "MQTT started, handle: " + str(MQTT))
        MQTT.registerMessageCallback(self.__on_configUpdate, "/trains/track/lightgroups/controllerMgmt/" + self.URL + "/") #topic should change to cover all decoder types
        self.lightDecoder = lightgroupDecoder(0)

    def __on_configUpdate(self, client, topic, payload, regexpRes) :
        if self.configured == True :
                notify(self, PANIC, "Top decoder cannot update the configuration")
        self.configured = True
        notify(self, INFO, "Top decoder has received an unverified configuration")
        if self.__parseConfig(str(payload.decode('UTF-8'))) == 0 :
            self.configured = True
        else :
            self.configured = False
            self.opState = OP_FAIL
            notify(self, PANIC, "Top decoder configuratio parsing failed")
            return 1
        self.opState = OP_CONFIG
        notify(self, INFO, "Top decoder is working")
        return 0

    def __parseConfig(self, xmlStr) :
        controllersXmlTree = ET.parse(io.StringIO(xmlStr))
        if str(controllersXmlTree.getroot().tag) != "Controller" or str(controllersXmlTree.getroot().attrib) != "{}" or controllersXmlTree.getroot().text is not None :
            notify(self, PANIC, "Received top decoder .xml string missformated:\n")
            return 1
        else:
            topDecoderXmlConfig = parse_xml(controllersXmlTree.getroot(), {"Author":MANSTR, "Description":OPTSTR, "Version":MANSTR, "Date":MANSTR, "URL":MANSTR, "ServerURL":OPTSTR, "RsyslogReceiver":OPTSTR, "Loglevel":OPTSTR, "NTPServer":OPTSTR, "TimeZone":OPTINT})
            global XMLAUTHOR
            XMLAUTHOR = topDecoderXmlConfig.get("Author")
            global XMLDESCRIPTION
            XMLDESCRIPTION = topDecoderXmlConfig.get("Description")
            global XMLVERSION
            XMLVERSION = topDecoderXmlConfig.get("Version")
            global XMLDATE
            XMLDATE = topDecoderXmlConfig.get("Date")
            if topDecoderXmlConfig.get("URL") != self.URL :
                notify(self, PANIC, "Top decoder .xml <URL> does not match actual controller URL")
            global SERVERURL
            SERVERURL = topDecoderXmlConfig.get("ServerURL")
            global TRAPRECEIVER
            TRAPRECEIVER = topDecoderXmlConfig.get("SnmpTrapReceiver")
            global SYSLOGRECEIVER
            RSYSLOGRECEIVER = topDecoderXmlConfig.get("RsyslogReceiver")
            global LOGLEVEL
            if topDecoderXmlConfig.get("Loglevel") == None :
                LOGLEVEL = INFO
            else :
                LOGLEVEL = topDecoderXmlConfig.get("Loglevel")
            global NTPSERVER
            NTPSERVER = topDecoderXmlConfig.get("NTPServer")
            global TIMEZONE
            TIMEZONE = topDecoderXmlConfig.get("TimeZone")
            notify(self, INFO, "Top decoder configured: " + str(topDecoderXmlConfig))

            for child in controllersXmlTree.getroot() :
                if str(child.tag) == "Lightgroups" :
                    notify(self, INFO, "Light groups decoder found, configuring it")
                    if (self.lightDecoder).on_configUpdate(child) != 0 :
                        notify(self, PANIC, "Top decoder configuration failed")
                        return 1
                    notify(self, INFO, "Light groups decoder successfully configured")
        return 0

    def startAll(self) :
        startTime = time.time()
        while self.getOpState() != OP_CONFIG :
            notify(self, INFO, "Waiting for top encoder and childs to be configured...")
            time.sleep(1)
            if time.time() - startTime > 10 :
                notify(self, PANIC, "Have not received a configuration within 10 seconds - restarting....")
        if self.startMe() != 0 : notify(self, PANIC, "Failed to start top decoder - restarting....")
        self.startLightgropupDecoder()
        self.startTurnoutDecoder()
        self.startSensorDecoder()

    def startMe(self) :
        if self.opState == OP_CONFIG :
            self.opState = OP_WORKING
            return 0
        else:
            return 1

    def startLightgropupDecoder(self) :
        self.lightDecoder.start()

    def startTurnoutDecoder(self) :
        pass

    def startSensorDecoder(self) :
        pass

    def startNtp(self) :
        pass

    def getOpState(self) :
        return self.opState

    def evalVar(self, var) :
        try : eval("self." + var)
        except : return (1, None)
        else : return (0, eval("self." + var))



#==============================================================================================================================================
# Class: "lightgroupDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class lightgroupDecoder :
    def __init__(self, channel) :
        global SMASPECTS
        SMASPECTS = signalMastAspects()
        global FLASHNORMAL
        FLASHNORMAL = flash(SM_FLASH_NORMAL, 50)
        global FLASHSLOW
        FLASHSLOW = flash(SM_FLASH_SLOW, 50)
        global FLASHFAST
        FLASHFAST = flash(SM_FLASH_SLOW, 50) 
        self.LgLedstripSeq = 0
        self.configured = False
        self.lightgroupTable = [None]
        if channel == 0 :
                pin = LEDSTRIP_CH0_PIN
        else : notify(self, PANIC, "Lightdecoder channel: " + str(channel) + " not supported")
        global LEDSTRIP                                             #Should not be global in the end
        LEDSTRIP = WS2811Strip(pin)
        notify(self, INFO, "Lightdecoder channel: " + str(channel) + " initialized")
        self.opState = OP_INIT

    def on_configUpdate(self, lightsXmlTree) :
        if self.configured == True :
            notify(self, PANIC, "Light group decoder cannot update the configuration")
            self.opState = OP_FAIL
        self.configured = True
        notify(self, INFO, "Light group decoder has received an unverified configuration")
        if self.__parseConfig(lightsXmlTree) != 0 :
            notify(self, PANIC, "Light group decoder configuration failed")
            return 1
        notify(self, INFO, "Light group decoder successfully configured")
        self.opState = OP_CONFIG
        cnt = 0
        infoStr = "lightgroupTable:\n"
        for lg in self.lightgroupTable :
            infoStr = infoStr + "lightgroupTable[" + str(cnt) + "] = " + str(lg) +"\n"
            cnt += 1
        notify(self, INFO, infoStr)
        return 0

    def __parseConfig(self, lightsXmlTree) :
        if str(lightsXmlTree.tag) != "Lightgroups" or str(lightsXmlTree.attrib) != "{}" or lightsXmlTree.text is not None :
            notify(self, PANIC, "Light groups decoder .xml string missformated")
            self.opState = OP_FAIL
            return 1
        else:
            notify(self, INFO, "parsing Lightgroups .xml")

            for lightgroup in lightsXmlTree :
                if str(lightgroup.tag) == "Lightgroup" :
                    lightDecoderXmlConfig = parse_xml(lightgroup, {"LgSystemName":MANSTR, "LgType":MANSTR, "LgAddr":MANINT})
                    if lightDecoderXmlConfig.get("LgType") == "Signal Mast" :
                        notify(self, INFO, "Creating signal mast: " + lightDecoderXmlConfig.get("LgSystemName") + " with LgAddr: " + str(lightDecoderXmlConfig.get("LgAddr")) + " and system name: " + lightDecoderXmlConfig.get("LgSystemName"))
                        mast = mastDecoder(self)
                        (self.lightgroupTable).insert(lightDecoderXmlConfig.get("LgAddr"), mast)
                        notify(self, INFO, "Inserted " + lightDecoderXmlConfig.get("LgSystemName") + " Signal mast object: " + str(self.lightgroupTable[lightDecoderXmlConfig.get("LgAddr")]) + " into lightGroupTable[" + str(lightDecoderXmlConfig.get("LgAddr")) +"]")
                        notify(self, INFO, lightDecoderXmlConfig.get("LgSystemName") + " has Light group sequence: " + str(self.LgLedstripSeq) + " on the LED strip")
                        mast.setLgLedStripSeq(self.LgLedstripSeq)
                        if mast.on_configUpdate(lightgroup) != 0 :
                            notify(self, PANIC, "Signal mast configuration failed")
                            self.opState = OP_FAIL
                            return 1

                    elif lightDecoderXmlConfig.get("LgType") == "Signal Head" :
                        # Not implemented
                        notify(self, ERROR, lightDecoderXmlConfig.get("LgType") + " is not a support5ed Light group type")

                    elif lightDecoderXmlConfig.get("LgType") == "General Light" :
                        # Not implemented
                        notify(self, ERROR, lightDecoderXmlConfig.get("LgType") + " is not a support5ed Light group type")

                    elif lightDecoderXmlConfig.get("LgType") == "Sequence Lights" :
                        # Not implemented
                        notify(self, lightDecoderXmlConfig.get("LgType"), lgType + " is not a support5ed Light group type")

                    else : 
                        notify(self, ERROR, lightDecoderXmlConfig.get("LgType") + " is not a support5ed Light group type")
                else :
                    notify(self, INFO, "Info - Ligh groups decoder.xml has no lightgrop elements - the Lightgroup controller will not start")
                    self.opState = OP_UNUSED
                    return 0
            self.LgLedstripSeq += 1
        self.opState = OP_CONFIG 
        return 0

    def start(self) :
        if self.opState == OP_UNUSED :
            notify(self, INFO, "There are no configured Light group instances - no need to start the Ligtht groups object")
            return 0
        notify(self, INFO, "Starting the Light groups decoder")
        cnt = 0
        for lg in self.lightgroupTable :
            if lg != None :
                if lg.start() != 0 :
                    self.state = OP_FAIL
                    notify(self, PANIC, "Lightgroup with System name: " + lg.getSystemName() + " and Address: " + str(cnt) + " Failed to start")
                    self.opState = OP_FAIL
                    return 1
            cnt += 1
        self.opState = OP_WORKING
        notify(self, INFO, "Starting Light group LED strip")
        LEDSTRIP.start()
        return 0

    def getOpState(self) :
        return self.opState

    def evalVar(self, var) :
        try : eval("self." + var)
        except : return (1, None)
        else : return (0, eval("self." + var))




#==============================================================================================================================================
# Class: "mastDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class mastDecoder :
    def __init__(self, upstreamHandle) :
        self.upstreamHandle = upstreamHandle
        self.opState = OP_INIT
        notify(self, INFO, "Mast decoder initialized")

    def on_configUpdate(self, mastXmlTree) :
        notify(self, DEBUG_TERSE, "Mast decoder has received an unverified configuration")
        if self.__parseConfig(mastXmlTree) != 0 :
            notify(self, PANIC, "Mast decoder configuration failed")
            return 1
        notify(self, INFO, "Mast decoder: " + self.lgSystemName + " successfully configured")
        self.opState = OP_CONFIG
        return 0

    def __parseConfig(self, mastXmlTree) :
        if str(mastXmlTree.tag) != "Lightgroup" or str(mastXmlTree.attrib) != "{}" or mastXmlTree.text is not None :
            notify(self, PANIC, "Lightgroup XML missformated")
            self.opState = OP_FAIL
            return 1
        else :
            mastDecoderXmlConfig = parse_xml( mastXmlTree, {"LgSystemName":MANSTR, "LgUserName":MANSTR, "LgType":MANSTR, "LgAddr":MANINT})
            self.lgSystemName = mastDecoderXmlConfig.get("LgSystemName")
            self.lgUserName = mastDecoderXmlConfig.get("LgUserName")
            self.lgType = mastDecoderXmlConfig.get("LgType")
            self.lgAddr =  mastDecoderXmlConfig.get("LgAddr")

            mastDescFound = False
            for mastDesc in mastXmlTree :
                if str(mastDesc.tag) == "LgDesc" :
                    mastDescFound = True
                    break
            if mastDescFound == False : info(self, PANIC, "No Lg/mast descriptor found for mast: " + self.lgSystemName)
            mastXmlProp = parse_xml( mastDesc, {"SmType":MANSTR, "SmDimTime":MANSPEED, "SmFlashFreq":MANSPEED, "SmBrightness":OPTLVL})
            self.mastType = mastXmlProp.get("SmType")
            self.smDimTime = mastXmlProp.get("SmDimTime")
            self.smFlashFreq = mastXmlProp.get("SmFlashFreq")
            self.smBrightness = mastXmlProp.get("SmBrightness")
            notify(self, INFO, "Signal mast " + self.lgSystemName + " successfully configured: " + str(mastDecoderXmlConfig) + str(mastXmlProp))

            if self.smDimTime == "NORMAL" : self.transitionTime = SM_TRANSITION_NORMAL
            elif self.smDimTime == "FAST" : self.transitionTime = SM_TRANSITION_FAST
            elif self.smDimTime == "SLOW" : self.transitionTime = SM_TRANSITION_SLOW

            if self.smFlashFreq == "NORMAL" : self.flashObj = FLASHNORMAL
            elif self.smFlashFreq == "FAST" : self.flashObj = FLASHFAST
            elif self.smFlashFreq == "SLOW" : self.flashObj = FLASHSLOW

            if self.smBrightness == "NORMAL" : self.brightness = SM_BRIGHTNESS_NORMAL
            elif self.smBrightness == "HIGH" : self.brightness = SM_BRIGHTNESS_HIGH
            elif self.smBrightness == "LOW" : self.brightness = SM_BRIGHTNESS_LOW
            else : self.brightness = SM_BRIGHTNESS_NORMAL                           # Can be removed when NB supports it

            (self.noOfPixels, self.noOfUsedPixels) = SMASPECTS.getPixelCount(self.mastType)
            self.opState = OP_CONFIG
            return 0

    def setLgLedStripSeq(self, seq) :
        self.LgLedStripSeq = seq
        notify(self, INFO, "Got LgLedStripSeq " + str(self.LgLedStripSeq) + " for Signal mast object: " + str(self) + "\n")

    def start(self) :
        notify(self, INFO, "Starting Mast decoder: " + self.lgSystemName)
        (self.noOfLeds, selfNoOfUsedLeds) = SMASPECTS.getPixelCount(self.mastType)
        self.flashHandleTable=[None]*self.noOfLeds
        print(str(LEDSTRIP))
        LEDSTRIP.register(self.LgLedStripSeq, self.noOfLeds)
        if MQTT.registerMessageCallback(self.on_mastUpdate, "/trains/track/lightgroups/lightgroup/" + URL + "/" + str(self.lgAddr) + "/") != 0 :
            print("Failed to register on_mastUpdate() message callback for topic: /trains/track/lightgroups/lightgroup/" + URL + "/" + str(self.lgAddr) + "/")
            return 1
        else :
            print("Registered on_mastUpdate() message callback for topic: /trains/track/lightgroups/lightgroup/" + URL + "/" + str(self.lgAddr) + "/")
            notify(self, INFO, "Mast decoder: " + self.lgSystemName + " started")
            return 0

    def on_mastUpdate(self, client, topic, payload, regexpRes) :
        print("============mastUpdate==============")
        (rc, aspect ) = self.parseXmlAspect(payload)
        if rc != 0 :
            print("Received signal mast aspect corrupt, skipping...")
            print ("PANIC")
        else :
            print('Received a new aspect: "' + aspect +'"')
            (rc, appearence) = SMASPECTS.getPixelsAspect(self.mastType, aspect)
            if rc != 0 :
                print ("PANIC")                
            else :
                self.visualizeAppearence(appearence, self.noOfUsedPixels)

    def getSystemName(self) :
        return self.lgSystemName

    def getUserName(self) :
        return self.lgUserName

    def parseXmlAspect(self, xmlStr) :
        aspectXmlTree = ET.parse(io.StringIO(str(xmlStr.decode('UTF-8'))))
        root = aspectXmlTree.getroot()
        if str(root.tag) != "Aspect" or str(root.attrib) != "{}" or root.text is None or root.text == "":
            print("Aspect XML string missformated")
            return (1, "")
        return (0, str(root.text))

    def visualizeAppearence(self, appearence, pixels) :
        cnt = 0

        while cnt < pixels :
            if appearence[cnt] == LIT : print('+', end='')
            if appearence[cnt] == UNLIT : print('-', end='')
            if appearence[cnt] == FLASH : print('*', end='')
            cnt += 1
        print("")


        cnt = 0
        while cnt < pixels :
            if self.flashHandleTable[cnt] != None :
                self.flashObj.unSetflash(cnt)
                self.flashHandleTable[cnt] = None
            if appearence[cnt] == LIT : 
                LEDSTRIP.updateOperationPoint(self.LgLedStripSeq, cnt, self.brightness, self.transitionTime)
            if appearence[cnt] == UNLIT :
                LEDSTRIP.updateOperationPoint(self.LgLedStripSeq, cnt, 0, self.transitionTime) 
            if appearence[cnt] == FLASH :
                self.flashHandleTable[cnt] = self.flashObj.setflash(LEDSTRIP.updateOperationPoint, [self.LgLedStripSeq, cnt, self.brightness, self.transitionTime], LEDSTRIP.updateOperationPoint, [self.LgLedStripSeq, cnt, 0, self.transitionTime])
            cnt += 1




#==============================================================================================================================================
# Class: "signalMastAspects"
# Purpose:
# Methods:
#==============================================================================================================================================
# Needs refactoring
class signalMastAspects :
    def __init__(self) :
        print("Initializing")
        global UNLIT
        UNLIT = 0
        global LIT
        LIT = 1
        global FLASH
        FLASH = 2
        global UNUSED
        UNUSED = 3
        print("==========================================INITIATED==================================================")

        self.aspects = ("Stopp",                                                # 0
                        "Stopp - vänta - begär tillstånd",                      # 1
                        "Stopp - manuell växling - vänta - begär tillstånd",    # 2
                        "Stopp - hinder - vänta - begär tillstånd",             # 3
                        "Kör 80 - vänta",                                       # 4
                        "Kör 40 - vänta",                                       # 5
                        "Kör 40 - vänta - kör 40",                              # 6
                        "Kör 40 - vänta - stopp",                               # 7
                        "Kör 80 - vänta - kör 80",                              # 8
                        "Kör 80 - vänta - kör 40",                              # 9
                        "Kör 80 - vänta - stopp",                               # 10
                        "Vänta - stopp",                                        # 11
                        "Vänta - kör 80",                                       # 12
                        "Vänta - kör 40",                                       # 13
                        "Hinder aktivering",                                    # 14
                        "Manuell växling aktivering")                           # 15

        self.mastParams = {"Sweden-3HMS:SL-5HL" : {
                            "noOfPixels" : 6,
                            "noOfUsedPixels" : 5,
                            "aspectMap" : ((LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED),
                                           (FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED))
                            },
                           "Sweden-3HMS:DWF-7" : {
                            "noOfPixels" : 9,
                            "noOfUsedPixels" : 7,
                            "aspectMap" : ((LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED),
                                           (LIT,   UNLIT, LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED, UNUSED),
                                           (FLASH, LIT,   UNLIT, LIT  , UNLIT, LIT,   UNLIT, UNUSED, UNUSED))
                            }
                           }
# "Sweden-3HMS:SL-5HL"
# "Sweden-3HMS:SL-4HL"
# "Sweden-3HMS:SL-3HL"
# "Sweden-3HMS:SL-2HL"
# "Sweden-3HMS:SL-3PreHL"
# "Sweden-3HMS:SL-1PreHL"
# "Sweden-3HMS:DWF-7"
# "Sweden-3HMS:DWF-4"
# "Sweden-3HMS:EndStop"
# "Sweden-3HMS:EndStop"
# "Sweden-3HMS:PreRoadCrossing"
# "Sweden-3HMS:RoadCrossing"
# "Sweden-3HMS:RoadCrossing"

    def getPixelCount(self, mastType) :
        return (((self.mastParams).get(mastType)).get("noOfPixels"), ((self.mastParams).get(mastType)).get("noOfUsedPixels"))

    def getPixelsAspect(self, mastType, aspect) :
        try :
            aspectIndex = self.aspects.index(aspect)
            pixelAspect = ((self.mastParams).get(mastType).get("aspectMap")[aspectIndex])
        except:
            return (1, None)
        return (0, pixelAspect)



#==============================================================================================================================================
# Class: "WS2811Strip"
# Purpose:
# Methods:
#==============================================================================================================================================
# Needs refactoring
class WS2811Strip:
    def __init__(self, pin) :
        self.pin = pin
        self.maxOpPoints = 256                                                              # Equal 85 RGB Light Groups, or 42 5 head masts
        self.sequenceTableEnd = 0
        self.totOpPoints = 0
        self.PopulatingOpWriteBuffer = False
        self.lock = _thread.allocate_lock()
        self.wblock = _thread.allocate_lock()
        self.opUpdated = True
        self.OperState = "Init"

    def selftest(self) :
        pass

    def register(self, sequenceNo, noOfOperationPoints) :                                   # A register request from an OP group (E.g. a signal mast object, 
                                                                                            # a generic light object, or a Turnout object)
        if sequenceNo > (self.maxOpPoints - 1) :                                            # Allocating the OP Sequennce list memory
            return 1
        if self.sequenceTableEnd == 0 : 
            self.sequenceTable = [None]
            self.sequenceTableEnd = 1
        if sequenceNo > self.sequenceTableEnd :
            cnt = self.sequenceTableEnd
            while sequenceNo >= self.sequenceTableEnd :
                self.sequenceTable.append(None)
                self.sequenceTableEnd += 1

        self.sequenceTable[sequenceNo] = {"NoOfOperationPoints" : noOfOperationPoints,      # Populating the OP sequence list index with dict for 
                                          "OperationPoints" : None}                         # number of OP points handled by the registered OP group 
                                                                                            # instance, the OP status descriptor dict is not yet populated

        opPointTemplate = [None]*(noOfOperationPoints-1)                                    # Creating and populating the OP status descriptor dicts
        for n in range(noOfOperationPoints-1) :                                             # for the Registered OP group with initial values
            opPointTemplate[n] = {"CurrentValue" : 0,
                                  "EndValue" : 0,
                                  "IncrementValue" : 0}
        self.sequenceTable[sequenceNo].update({"OperationPoints" : opPointTemplate})        # Attaching the OP status descriptor dicts to the OP group
                                                                                            # descriptor 
        print("OP Group Registered, sequence No: " + str(sequenceNo) + " Number of OP Points: " + str(noOfOperationPoints))

    def start(self) :                                                                       # A request to start the WS2811Strip object
        print("Starting the WS2811 Strip")
        self.opPointWritebuf = [None]                                                       # Allocating the OP Write buffer memory for the WS2811/OP strip
        for seq in self.sequenceTable :
            self.totOpPoints += seq.get("NoOfOperationPoints")
        self.opPointWritebuf = [None]*(self.totOpPoints)
        print("Total number of operation points", self.totOpPoints)

        if TARGET == MPYTARGET :
            print(int(self.totOpPoints/3))
            print(self.pin)
            print(Pin(self.pin))
            global OPLEDSTRIP
            OPLEDSTRIP = NeoPixel(Pin(self.pin), int(self.totOpPoints/3))   # Needs checks on the division
        #self.__populateOpPointWriteBuf()                                                    # Calling  __populateOpPointWriteBuf() to populate the OP 
        self.__startOpStripTimer()
        self.OperState = "Started"                                                          # Write buffer memory for the WS2811/OP strip

    def updateOperationPoint(self, OPGroup, OPGroupSeq, newValue, transitionTime=0) :
        (self.lock).acquire()
        opPointTemplate = (self.sequenceTable[OPGroup].get("OperationPoints"))[OPGroupSeq]
        opPointTemplate.update({"EndValue" : newValue})
        if transitionTime == 0 :
            incrementValue = 0
        else :
            incrementValue = int((newValue - opPointTemplate.get("CurrentValue"))*10/transitionTime)
        opPointTemplate.update({"IncrementValue" : incrementValue})
        (self.sequenceTable[OPGroup].get("OperationPoints"))[OPGroupSeq] = opPointTemplate 
        print("OP updated, for OP Group: " + str(OPGroup) + " Sequence number: " + str(OPGroupSeq) + " Current value: " + str(opPointTemplate.get("CurrentValue")) + " New value: " + str(newValue) + " Transition time: " + str(transitionTime))
        (self.lock).release()
        return 0

    def __populateOpPointWriteBuf(self) :                                                   # Request to populate the OP writebuffer from the  OP sequence list
                                                                                            # buffe
                                                                                            # This code is always locked by the caller
        (self.wblock).acquire()
        opPointStripSeq = 0
        for seq in self.sequenceTable :
            opPointTemplate = seq.get("OperationPoints")
            for op in opPointTemplate :
                currentValue = op.get("CurrentValue")
                incrementValue = op.get("IncrementValue")
                endValue = op.get("EndValue")
                
                if currentValue == endValue : incrementValue = 0
                elif incrementValue > 0 :
                    currentValue += incrementValue
                    self.opUpdated = True
                    if currentValue > endValue : 
                        currentValue = endValue
                        incrementValue = 0
                elif incrementValue < 0 :
                    currentValue += incrementValue
                    self.opUpdated = True
                    if currentValue < endValue : 
                        currentValue = endValue
                        incrementValue = 0

                op.update({"CurrentValue" : currentValue})
                op.update({"IncrementValue" : incrementValue})
                op.update({"EndValue" : endValue})
                self.opPointWritebuf[opPointStripSeq] = currentValue
                opPointStripSeq += 1
            opPointStripSeq += 1
        (self.wblock).release()

    def __startOpStripTimer(self) :
        (self.lock).acquire()
        timer(0.01, self.__startOpStripTimer)
        self.__populateOpPointWriteBuf()
        if  self.opUpdated == True :
            self.__startWriteStrip()
        self.opUpdated = False
        (self.lock).release()

    def __startWriteStrip(self) :
        (self.wblock).acquire()
#        print("Got Lock for: __startWriteStrip ")
        if TARGET == MPYTARGET :
            pxlCnt = 0
            clrCnt = 0
            for value in self.opPointWritebuf :
                if value == None : value = 0 
#                print("Value: " + str(value))
                if clrCnt == 0 : 
                    r = value
                    clrCnt += 1
                elif clrCnt == 1 : 
                    g = value
                    clrCnt += 1
                elif clrCnt == 2 : 
                    b = value
#                    print("Writing pixel: " + str(pxlCnt) + " to OP Strip - R = " + str(r) + " G = " + str(g) +" B = " + str(b))
                    OPLEDSTRIP[pxlCnt] = (r, g , b)
                    clrCnt = 0
                    pxlCnt += 1
#            print("Writing all pixels to OP strip")
            OPLEDSTRIP.write()
            if TARGET == CPYTARGET or TARGET == MPYEMULATEDTARGET :
               for value in self.opPointWritebuf :
                   print(str(value) + " ", end='')
               print("")

        (self.wblock).release()
        return 0

class flash :
    def __init__(self, freq, dutyCycle) :
        self.handles = []
        self.handleIndex = 0
        self.phase = LIT
        self.ton = dutyCycle/(freq * 100)
        self.toff = 1/freq - self.ton
        self.lock = _thread.allocate_lock()
        #self.lock = threading.Lock()
        self.__on_change()

    def setflash(self, litCallback, litArgs, unlitCallback, unlitArgs) :
        (self.lock).acquire()
        try :
            self.handles.append = {"Handle" : self.handleIndex, "LitCallback" : litCallback, "LitArgs" : litArgs, "UnlitCallback" : unlitCallback, "UnlitArgs" : unlitArgs}
        except :
            self.handles = [{"Handle" : self.handleIndex, "LitCallback" : litCallback, "LitArgs" : litArgs, "UnlitCallback" : unlitCallback, "UnlitArgs" : unlitArgs}]

        if self.phase == LIT :
            litCallback(*litArgs)
            
        else :
            unlitCallback(*unlitArgs)

        self.handleIndex += 1
        (self.lock).release()
        return (self.handleIndex - 1)

    def unSetflash(self, rmhandle) :
        print("===================================================================================")
        print ("Stop flash")

        cnt = 0
        found = False
        (self.lock).acquire()
        for handle in self.handles :
            if handle.get("Handle") == rmhandle :
                self.handles.pop(cnt)
                found = True
                break
            cnt += 0
        (self.lock).release()
        if found : return 0
        else : return 1

    def __on_change(self) :
        if self.phase == LIT :
            self.phase = UNLIT
            timer(self.toff, self.__on_change)
        else :
            self.phase = LIT
            timer(self.ton, self.__on_change)
        (self.lock).acquire()
        for handle in self.handles :
            if self.phase == LIT :
                cb = handle.get("LitCallback") 
                args = handle.get("LitArgs") 
            else :
                cb = handle.get("UnlitCallback") 
                args = handle.get("UnlitArgs") 
            cb(*args)
        (self.lock).release()




#==============================================================================================================================================
# Class: "SensorDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class sensorDecoder :
    def __init__(self) :
        pass

URL = "lightcontroller1.bjurel.com"                                                     #TODO fetch the URL from DNS
if TARGET == MPYTARGET :
    print(_thread.stack_size(7*1024))
TRACE = trace()
if TARGET == MPYTARGET :
    do_connect()
print(BROKER)
global TIMER
TIMER = timer_thread()
decoder = topDecoder(BROKER, URL)
TRACE.setDebugLevel(DEBUG_VERBOSE)
time.sleep(2)
decoder.startAll()
while True :
    time.sleep(1)
