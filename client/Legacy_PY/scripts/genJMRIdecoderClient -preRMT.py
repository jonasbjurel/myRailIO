MPYEMULATEDTARGET = 0
MPYTARGET = 1
CPYTARGET = 2
TARGET = MPYTARGET

import time
import io
import inspect
import traceback
import sys
import _thread
import network
if TARGET == CPYTARGET :
    import xml.etree.ElementTree as ET
    import paho.mqtt.client as mqtt
elif TARGET == MPYEMULATEDTARGET :
    import micropython
    from umqtt.simple import MQTTClient as mqtt
    import xml.etree.ElementTree as ET
    import ubinascii
elif TARGET == MPYTARGET :
    import micropython
    import ntptime
    import ElementTree as ET
    from simple import MQTTClient as mqtt
    import machine
    from machine import Pin
    from machine import Timer
    from machine import WDT
    from neopixel import NeoPixel
    import ubinascii



#==============================================================================================================================================
# Constants
#==============================================================================================================================================
#Files
DAY0CONFIGFILE = "day0.xml"

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
MANFLOAT = {"expected" : True, "type" : "float", "values" : None, "except": "PANIC"}
OPTFLOAT = {"expected" : None, "type" : "float", "values" : None, "except": "PANIC"}
MANSPEED = {"expected" : True, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANSPEED = {"expected" : None, "type" : "str", "values" : ["SLOW","NORMAL","FAST"], "except": "PANIC"}
MANLVL = {"expected" : True, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}
OPTLVL = {"expected" : None, "type" : "str", "values" : ["LOW","NORMAL","HIGH"], "except": "PANIC"}

#==============================================================================================================================================
# Decoder parameters
#==============================================================================================================================================

DEFAULTLOGLEVEL = DEBUG_VERBOSE
DEBUGLEVEL = DEFAULTLOGLEVEL
DEFAULTNTPSERVER = "pool.ntp.org"
DEFAULTTIMEZONE = 0 #UTC

# Signal mast transition times
SM_TRANSITION_NORMAL = 100
SM_TRANSITION_FAST = 75
SM_TRANSITION_SLOW = 150

# Signal mast flash frequences
SM_FLASH_NORMAL = 1
SM_FLASH_FAST = 1.5
SM_FLASH_SLOW = 0.75

# Signal mast brightness
SM_BRIGHTNESS_NORMAL = 40
SM_BRIGHTNESS_HIGH = 80
SM_BRIGHTNESS_LOW = 20

# operation point strip
MAX_OP_POINTS = 12                          #Should be higher - Just a test

#==============================================================================================================================================
# MQTT
#==============================================================================================================================================
# MQTT Topics
MQTT_DISCOVERY_REQUEST_TOPIC = "/trains/track/discoveryreq/"
MQTT_DISCOVERY_RESPONSE_TOPIC = "/trains/track/discoveryres/"
MQTT_PING_UPSTREAM_TOPIC = "/trains/track/decoderSupervision/upstream/"
MQTT_PING_DOWNSTREAM_TOPIC = "/trains/track/decoderSupervision/downstream/"
MQTT_CONFIG_TOPIC = "/trains/track/decoderMgmt/"
MQTT_OPSTATE_TOPIC = "/trains/track/opState/"
MQTT_LOG_TOPIC = "/trains/track/log/"
MQTT_ASPECT_TOPIC = "/trains/track/lightgroups/lightgroup/"

# MQTT Payload
DISCOVERY = "<DISCOVERY_REQUEST/>"
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
PING = "<Ping/>"

#==============================================================================================================================================
# PIN Assignments
#==============================================================================================================================================
LEDSTRIP_CH0_PIN = 25

#==============================================================================================================================================
# Other constants
#==============================================================================================================================================
DUMMY_MAC = "01:23:45:67:89:AB" #Dummy MAC used for non target emulation

#==============================================================================================================================================
# Functions: "parse_xml()"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
def parse_xml(xmlTree,tagDict) :
#   tagDict: {"Tag" : {"expected" : bool, "type" : "int/float/int", "values" : [], "except": "error/panic/info}, ....}
    res = {}
    for child in xmlTree :
        tagDesc = tagDict.get(child.tag)
        if tagDesc == None :
            continue
        value = validateXmlText(child.text, tagDesc.get("type"), tagDesc.get("values"))
        if tagDesc.get("expected") == None :
            if value != None :
                res[child.tag] = value
            else : 
                continue
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
        return txt.strip()
    elif type == "str" : res = str(txt).strip()
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
        self.overload = False
        self.ceaseCnt = 0

    def start(self):
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting the timer")
        if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
            global TIM1
            TIM1 = Timer(1)
            self.time = time.ticks_ms() + 10
            TIM1.init(period=10, mode=Timer.ONE_SHOT, callback=self.timerInterrupt)
        elif TARGET == CPYTARGET :
            self.time = int(time.time()*1000)
            runTimer(self, None)

    def insertTimer(self, timeout, cb) :
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
        return 0

    def timerInterrupt(self, dummy):
        micropython.schedule(self.runTimer, None)

    def runTimer(self, dummy) :
        moreTicks = True
        while moreTicks == True:
            self.time += 10
            timeouts = []
            poplevels = 0
            if self.timeTable :
                for timeoutDesc in self.timeTable :
                    if timeoutDesc[0] <= self.time :
                        poplevels += 1
                        cb = timeoutDesc[1]
                        timeouts.insert(0, cb)
                    else :
                        break
            while poplevels :
                del self.timeTable[0]
                poplevels -= 1
            #startTotTime = time.ticks_us()
            for callback in timeouts :
                #print("Calling: " + str(callback))
                #print(callback.__name__)
                #startTime = time.ticks_us()
                callback()
                #stopTime = time.ticks_us()
                #print ("took: " + str(stopTime-startTime))

            #stopTotTime=time.ticks_us()
            #print ("Callbacks took: " + str(stopTotTime-startTotTime))
            if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
                timeToNextTick = self.time - time.ticks_ms() + 10
                if timeToNextTick > 2:
                    if self.overload == True:
                        self.ceaseCnt += 1
                        if self.ceaseCnt > 100:
                            #print("Over-load alarm ceased")
                            self.overload = False
                            self.ceaseCnt = 0
                    TIM1.init(period=timeToNextTick, mode=Timer.ONE_SHOT, callback=self.timerInterrupt)
                    moreTicks = False
                else:
                    if timeToNextTick < -10:
                        self.time = time.ticks_ms() - 10    # Lagging to far behind
                    if self.overload == False:
                        #print("Over-load alarm")
                        self.overload = True
            elif TARGET == CPYTARGET :
                timeToNextTick = self.time - int(time.time()*1000) + 10
                if timeToNextTick > 2:
                    time.sleep(timeToNextTick/1000)


            #print ("runTimer Ended")
            #time.sleep(nextTime)
            #time.sleep_ms(10)

#==============================================================================================================================================
# Function: getDay0Config
# Purpose: Get all pre-provisioned Day-0 configuration parameters from file
# Data structures: global: 
# Methods: do_connect(): Connect to the pre-provisioned Wifi AP
#==============================================================================================================================================
def getDay0Config():
    try:
        f = open(DAY0CONFIGFILE)
        day0 = f.read()
        f.close()
        day0DecoderConfigXmlTree = ET.parse(io.StringIO(day0))
        #day0DecoderConfigXmlTree = ET.parse(io.StringIO(day0.decode("utf-8")))
        #day0DecoderConfigXmlTree = ET.parse(DAY0CONFIGFILE)
    except Exception as e:
         sys.print_exception(e)
         notify(None, PANIC, "Decoder Day-0 configuration does not exsist, or is missformated")
    if str(day0DecoderConfigXmlTree.getroot().tag) != "DecoderConfig" :
            notify(self, PANIC, "Decoder Day-0 configuration is missformated")
            return 1
    else:
        day0DecoderConfig = parse_xml(day0DecoderConfigXmlTree.getroot(), {"SSID":MANSTR, "SSIDPW":OPTSTR, "BROKER":MANSTR, "URI":OPTSTR}, )
    global MAC
    if TARGET == CPYTARGET or TARGET == MPYEMULATEDTARGET:
        MAC = DUMMY_MAC
    else:
        MAC = ubinascii.hexlify(network.WLAN().config('mac'),':').decode()
    global WIFI_SSID
    WIFI_SSID = day0DecoderConfig.get("SSID")
    if WIFI_SSID == None :
        notify(None, PANIC, "Decoder Day-0 configuration WIFI SSID not provided")
    global SSID_PW
    SSID_PW = day0DecoderConfig.get("SSID_PW")
    if SSID_PW == None : 
        SSID_PW = ""
        if DEBUGLEVEL >= INFO:
            notify(None, INFO, "Decoder Day-0 configuration WIFI SSID Password not provided - trying with \"\"")
    global BROKER
    BROKER = day0DecoderConfig.get("BROKER")
    global URI
    URI = day0DecoderConfig.get("URI")

def setDay0URI(URI):
    try:
        f = open(DAY0CONFIGFILE)
        day0 = f.read()
        f.close()
        day0DecoderConfigXmlTree = ET.parse(io.StringIO(day0))
    except Exception as e:
        sys.print_exception(e)
        notify(None, PANIC, "Decoder Day-0 configuration does not exsist, or is missformated")
    if str(day0DecoderConfigXmlTree.getroot().tag) != "DecoderConfig" :
            notify(self, PANIC, "Decoder Day-0 configuration is missformated")
            return 1
    else: #MPY library disfunct - NEED to fix
        day0DecoderConfig = parse_xml(day0DecoderConfigXmlTree.getroot(), {"SSID":MANSTR, "SSIDPW":OPTSTR, "BROKER":MANSTR, "URI":OPTSTR}, )
        WIFI_SSID = day0DecoderConfig.get("SSID")
        SSID_PW = day0DecoderConfig.get("SSID_PW")
        BROKER = day0DecoderConfig.get("BROKER")
        if TARGET == CPYTARGET:
            newday0DecoderConfig = ET.Element("DecoderConfig")
            newday0DecoderConfigTree = ET.ElementTree(newday0DecoderConfig)
            ssid = ET.SubElement(newday0DecoderConfig, "SSID")
            ssid.text = WIFI_SSID
            if SSID_PW != None:
                pw = ET.SubElement(newday0DecoderConfig, "SSID_PW")
                pw.text = SSID_PW
            broker = ET.SubElement(newday0DecoderConfig, "BROKER")
            broker.text = BROKER
            uri = ET.SubElement(newday0DecoderConfig, "URI")
            uri.text = URI
            f = open(DAY0CONFIGFILE, 'w')
            f.write(ET.tostring(newday0DecoderConfig).decode("utf-8"))
            f.close()
        else:
            newday0DecoderConfig = ET.Element()
            newday0DecoderConfig.tag = "DecoderConfig"
            newday0DecoderConfigTree = ET.ElementTree(newday0DecoderConfig)
            ssid=ET.Element()
            ssid.tag = "SSID"
            ssid.text = WIFI_SSID
            newday0DecoderConfig.append(ssid)
            if SSID_PW != None:
                ssidPw = ET.Element()
                ssidPw.tag = "SSID_PW"
                ssidPw.text = SSID_PW
                newday0DecoderConfig.append(ssidPw)
            broker = ET.Element()
            broker.tag = "BROKER"
            broker.text = BROKER
            newday0DecoderConfig.append(broker)
            uri = ET.Element()
            uri.tag = "URI"
            uri.text = URI
            newday0DecoderConfig.append(uri)
            f = open(DAY0CONFIGFILE, 'w')
            newday0DecoderConfig.write(f)
            f.close()
    return 0


#==============================================================================================================================================
# Function: do_connect
# Purpose: Connect to the preprovisioned WIFI network
# Data structures: global: MAC - The decoder MAC address
#                  global: IPADDR - The decoder IP Address
# Methods: do_connect(): Connect to the pre-provisioned Wifi AP
#==============================================================================================================================================
def do_connect():
    sta_if = network.WLAN(network.STA_IF)
    if not sta_if.isconnected():
        if DEBUGLEVEL >= INFO:
            notify(None, INFO, "Connecting to Wifi network: " + WIFI_SSID + ", Password: " + SSID_PW)
        sta_if.active(True)
        sta_if.connect(WIFI_SSID, SSID_PW)
        cnt = 0
        while not sta_if.isconnected():
            time.sleep(0.1)
            if cnt >= 300:
                notify(None, PANIC, "Could not connect to: " +  WIFI_SSID)
            cnt += 1
        if DEBUGLEVEL >= INFO:
            notify(None, INFO, "Connected to; " + WIFI_SSID + ", network config: " + str(sta_if.ifconfig()))
        global IPADDR
        IPADDR = sta_if.ifconfig()[0]
    return 0


#==============================================================================================================================================
# Function: discovery
# Purpose: Send discovery
# Data structures: -
# Methods: discovery()
#==============================================================================================================================================
def discovery():
    MQTT.sendMessage (MQTT_DISCOVERY_REQUEST_TOPIC, DISCOVERY, retain=False)
    MQTT.registerMessageCallback(__onDiscoveryResponse, MQTT_DISCOVERY_RESPONSE_TOPIC)
    if DEBUGLEVEL >= INFO:
        notify(None, INFO, "Sent a discovery request; Waiting for a response with with descriptor for my MAC address: " + str(MAC))
    discovered = False
    cnt=0
    while discovered != True:
        MQTT.__mqtt_loop(timerInsert=False)
        try:
            URI
        except:
            pass
        else:
            if URI != None:
                discovered = True
        if cnt >= 300:
            notify(None, PANIC, "Discovery failed, no discovery response for my MAC adress received")
        cnt += 1
        time.sleep(0.1)
    return 0


#==============================================================================================================================================
# Function: __onDiscoveryResponse
# Purpose: Discovery response, defining the jmri server and the decoder URIs
# Data structures: xml:
#                   <DiscoveryResponse>
#                       <ServerURI>"Server URI"</ServerURI>
#                       <Decoder>
#                           <MAC>"MAC"</MAC>
#                           <URI>"URI"</URI>
#                       </Decoder>
#                   </DiscoveryResponse>
# Methods: -
#==============================================================================================================================================
def __onDiscoveryResponse(client, topic, payload, regexpRes):
    if DEBUGLEVEL >= INFO:
        notify(None, INFO, "Got a discovery response")
    discoveryRespXmlTree = ET.parse(io.StringIO(payload.decode("utf-8")))
    if str(discoveryRespXmlTree.getroot().tag) != "DiscoveryResponse" or str(discoveryRespXmlTree.getroot().attrib) != "{}" or discoveryRespXmlTree.getroot().text is not None :
            notify(self, PANIC, "Received discovery response missformated")
            return 1
    else:
        found = False
        for child in discoveryRespXmlTree.getroot():
            if child.tag == "Decoder" :
                decoderConfig = discoveryRespConfig = parse_xml(child, {"MAC":MANSTR, "URI":MANSTR})
                if decoderConfig.get("MAC") == MAC:
                    found = True
                    global URI
                    if URI != decoderConfig.get("URI"):
                        if DEBUGLEVEL >= INFO:
                            notify(None, INFO, "URI received from discovery response differs from day-0 config - updating day-0 config...")
                        setDay0URI(decoderConfig.get("URI"))
                    else:
                        if DEBUGLEVEL >= INFO:
                            notify(None, INFO, "URI received from discovery response is consistant with day-0 config - doing nothing...")
                    URI = decoderConfig.get("URI")
                    break
        if found != True:
            if DEBUGLEVEL >= ERROR:
                notify(None, ERROR, "The Discovery response did not include a descriptor for my MAC address: " + str(MAC) + " has my MAC address been configured? - Waiting...")
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

    def setDebugLevel(self, debugLevel, output = {"Console" :  True, "rSyslog" : False, "Mqtt" : False}, debugClasses = None, errCallStack = False, errTerminate = False) :
        self.errCallStack = errCallStack
        self.errTerminate = errTerminate
        global DEBUG_LEVEL
        DEBUG_LEVEL = debugLevel
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
                if TARGET == CPYTARGET:
                    notification = str(time.time()) + ": ERROR!:  " + notificationStr + "\nCall Stack:\n" + str(inspect.stack())
                else:
                    try:
                        raise Exception
                    except Exception as e:
                        sys.print_exception(e)
                        notification = str(time.time()) + ": ERROR!:  " + notificationStr + "\nCall Stack:\n"
            else :
                notification = str(time.time()) + ": ERROR!:  " + notificationStr
            self.__deliverNotification(notification)
            if self.errTerminate == True :
                # Call back to registered entries for emergency
                _thread.start_new_thread(self.terminate,(notification,))
            else :
                return

        if severity == PANIC :
            if TARGET == CPYTARGET:
                notification = str(time.time()) + ": PANIC!:  " + notificationStr + "\nCall Stack:\n" + str(inspect.stack())
            else:
                try:
                    raise Exception
                except Exception as e:
                    sys.print_exception(e)
                    notification = str(time.time()) + ": PANIC!:  " + notificationStr + "\nCall Stack:\n"
            # Call back to registered entries for emergency
            _thread.start_new_thread(self.terminate,(notification,))
            while(True):
                time.sleep(1)

    def terminate(self, notification) :
        setSafeMode()
        self.__deliverNotification(notification)
        try:
            MQTT.sendMessage(MQTT_OPSTATE_TOPIC + str(URI) + "/", DECODER_DOWN)
        except: 
            print(str(time.time()) + ": ERROR! Failed to send Decoder down/off-line notification to MQTT broker")
        print(str(time.time()) + "Terminating ....")
        time.sleep(1)
        if TARGET == MPYTARGET :
            machine.reset()
        else:
            _thread.list(False)
            _thread.interrupt_main()
            sys.exit()                                                      # Needs to be ported to ESP32 to restart the machine
        while True:
            time.sleep(1)

    def __classNotification(self, caller, notification) :                   #Currently not supported
        callerClass = type(itertools.count(0)).__name__
        for debugClass in self.debugClasses :
            if debugClass == callerClass :
                self.__deliverNotification(notification)
                return

    def __deliverNotification(self, notification) :
        if self.console == True : print(notification)
        if self.mqtt == True : 
            try:
                MQTT
            except:
                print(str(time.time()) + ": ERROR! Failed to send notification to MQTT broker")
            else:
                if MQTT != None:
                    MQTT.sendMessage(MQTT_LOG_TOPIC + str(URI) + "/", notification, retain=False, notice = False)
                else:
                    print(str(time.time()) + ": ERROR! Failed to send notification to MQTT broker")
        if self.rSyslog == True : rsyslogNotification(notification)

    def rsyslogNotification (self, notification) :
        pass


#==============================================================================================================================================
# Class: "decoderMqtt"
# Purpose: 
# Data structures: 
# Methods:
#==============================================================================================================================================
class decoderMqtt :
    def __init__(self, port=1883, keepalive=60, qos=0) : #MAYBE USE CONSTANTS INSTEAD
        self.opState = OP_INIT
        self.connected = False
        self.broker = BROKER
        self.port = port
        self.keepalive = keepalive
        self.qos = qos
        self.maxMessageCallbacks = 32
        self.mqttTopicMessageCallbacks = [("", "", "", "", False)]*self.maxMessageCallbacks #("Topic", "TopicRegExp", "TopicRegexpRes", "Callback")
        self.callbackQueue = []
        self.message = __message()

    def start(self) :
        if TARGET == MPYEMULATEDTARGET or TARGET == MPYTARGET :
            self.client = mqtt("GenDecoderMqttClient", self.broker, port = self.port, keepalive = self.keepalive)
            cnt = 0
            while (self.client).connect() != 0 :
                time.sleep(0.1)
                cnt += 1
                if cnt == 100 :
                    notify(self, PANIC, "MQTT Could not connect to broker: " + self.broker)
                    return 1
            self.__on_connect(None, None, None, 0)

        elif TARGET == CPYTARGET :
            self.client = mqtt.Client()
            (self.client).on_connect=self.__on_connect
            (self.client).connect(self.broker, port=self.port, keepalive=self.keepalive)
            cnt = 0
            while not self.connected :
                (self.client).loop()
                time.sleep(0.1)
                if cnt == 100 :
                    notify(self, PANIC, "MQTT Could not connect to broker: " + self.broker)
                    return 1

    def __on_connect(self, client, userdata, flags, rc) :
        if rc == 0 :
            self.connected = True
            self.opState = OP_CONNECTED
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "Connected to MQTT broker")

            if TARGET == MPYEMULATEDTARGET or TARGET == MPYTARGET :
                (self.client).set_callback(self.__on_mpy_message)
                (self.client).set_last_will(MQTT_OPSTATE_TOPIC + str(URI) + "/", DECODER_DOWN, qos=self.qos, retain=True)
            elif TARGET == CPYTARGET :
                (self.client).on_message = self.__on_message
                (self.client).will_set(MQTT_OPSTATE_TOPIC + str(URI) + "/", DECODER_DOWN, qos=self.qos, retain=True)
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "Decoder connected to MQTT Broker: " + self.broker) 
                notify(self, INFO, "Decoder declaring it self as down/offline") 
            self.sendMessage(MQTT_OPSTATE_TOPIC + str(URI) + "/", DECODER_DOWN, qos=self.qos, retain=True)
            self.__mqtt_loop()
        else :
            self.opState = OP_FAIL
            notify(self, PANIC, "Could not connect to broker, Return code: " + str(rc)) 

    def __mqtt_loop(self, timerInsert=True) :
        #print("100 ms loop")
        #timer(0.1, self.__mqtt_loop)
        if TARGET == MPYTARGET or TARGET == MPYEMULATEDTARGET :
            self.client.check_msg()
        elif TARGET == CPYTARGET :
            self.client.loop(timeout=0)
        while len(self.callbackQueue) != 0:
            (recTopic, message, regCallback) = self.callbackQueue[0]
            if DEBUGLEVEL >= DEBUG_VERBOSE:
                notify(self, DEBUG_VERBOSE, "Executing " + str(regCallback) + " from the callback queue")
            self.callbackQueue.pop(0)
            regCallback(self, recTopic, message, "")
        #time.sleep_ms(100)
        #print("Insert MQTT loop timer")
        if timerInsert == True:
            timer(0.1, self.__mqtt_loop)
        return 0

    def __on_mpy_message(self, topic, payload) :
        self.message.topic = topic                              #Thread secure - should be?
        self.message.payload = payload
        self.__on_message(None, None, self.message)
        return 0

    def __on_message(self, client, userdata, message) :
        if DEBUGLEVEL >= DEBUG_VERBOSE:
            notify(self, DEBUG_VERBOSE, "Got an MQTT message, Topic: " + str(message.topic) + " Payload: " + str(message.payload)) 
        mqttTopicMessageCallbacksIndex = 0
        while mqttTopicMessageCallbacksIndex < self.maxMessageCallbacks :
            (regTopic, regTopicRegExp, regTopicRegexpRes, regCallback, regQueue) = self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex]
            if regCallback != "" :
                if isinstance(message.topic, str) : recTopic = message.topic
                else : recTopic = message.topic.decode('UTF-8')

                if regTopicRegExp == "" and recTopic == regTopic :
                    if regQueue == True:
                        self.callbackQueue.append((recTopic, message.payload, regCallback))
                        if DEBUGLEVEL >= DEBUG_VERBOSE:
                            notify(self, DEBUG_VERBOSE, "Added " + str(regCallback) + " to the callback queue")
                    else:
                        if DEBUGLEVEL >= DEBUG_VERBOSE:
                            notify(self, DEBUG_VERBOSE, "Not queueing " + str(regCallback))
                        regCallback(self, recTopic, message.payload, "")
                else :
                    pass #Not supported
                    #res = re.search(topicRegExp, topic)
                    #if res == topicRegexpRes :
                    #notify(self, DEBUG_VERBOSE, "Routing MQTT message, Topic: " + str(message.topic) + " Payload: " + str(message.payload) + " to callback: " + str(callback))
                    #callback (self, message.topic, message.payload, res)

            mqttTopicMessageCallbacksIndex += 1

    def registerMessageCallback (self, regCallback, regTopic, regRegexp="", regRegexpResult="", regQueue = False) : # Redo the list handeling to dynamic extention
        if DEBUGLEVEL >= DEBUG_TERSE:
            notify(self, DEBUG_TERSE, "Registering MQTT callback for Topic: " + str(regTopic) + " Topic Regexp: " + regRegexp + " Topic Regexp result: " + regRegexpResult + "Callback: " + str(regCallback))
        mqttTopicMessageCallbacksIndex = 0
        while mqttTopicMessageCallbacksIndex < self.maxMessageCallbacks :
            (topic, topicRegExp, topicRegexpRes, callback, queue) = self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex]
            # We need to look if the same search topic allready exist, any impact on "__on_message"
            if callback == "" :
                self.mqttTopicMessageCallbacks[mqttTopicMessageCallbacksIndex] = (regTopic, regRegexp, regRegexpResult, regCallback, regQueue)
                #print("Subscribing")
                self.client.subscribe(regTopic)
                #print("Subscribed")
                if DEBUGLEVEL >= DEBUG_TERSE:
                    notify(self, DEBUG_TERSE, "Registered MQTT callback for Topic: " + str(regTopic) + " Topic Regexp: " + regRegexp + " Topic Regexp result: " + regRegexpResult + "Callback: " + str(regCallback))
                return 0
            mqttTopicMessageCallbacksIndex += 1
        return 1

    def sendMessage(self, topic, payload, qos=None, retain=True, notice = True) :
        if qos == None:
            qos = self.qos
        self.client.publish(topic, payload, qos=qos, retain=True)

    def getOpState(self) :
        return self.opState

class __message :                           # A helper class to mimic CPY mqtt data structure
    def __init__(self) :
        self.topic = None
        self.payload = None


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
    def __init__(self) :
        self.opState = OP_INIT
        TRACE.setDebugLevel(DEFAULTLOGLEVEL)
        global NTPSERVER
        NTPSERVER = DEFAULTNTPSERVER
        global TIMEZONE
        TIMEZONE = DEFAULTTIMEZONE
        self.cnt = 0
        self.lightDecoder = lightgroupDecoder(0)
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Registering callback for configuration MQTT message")
        MQTT.registerMessageCallback(self.__on_configUpdate, MQTT_CONFIG_TOPIC + URI + "/", regQueue=True) 
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Registered callback for configuration MQTT message")
        # self.lightDecoder = lightgroupDecoder(0) Move up a bit
        timer(1, self.startAll)
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Top decoder initialized")

    def __on_configUpdate(self, client, topic, payload, regexpRes) :
        if self.opState != OP_INIT:
                notify(self, PANIC, "Top decoder cannot update the configuration")
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Top decoder has received an unverified configuration")
        if self.__parseConfig(str(payload.decode('UTF-8'))) != 0 :
            self.opState = OP_FAIL
            notify(self, PANIC, "Top decoder configuratio parsing failed")
            return 1
        self.opState = OP_CONFIG
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Top decoder is configured, opState: " + self.opState)
        return 0

    def __parseConfig(self, xmlStr) :
        global DEBUGLEVEL
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Parsing config .xml")
        decodersXmlTree = ET.parse(io.StringIO(xmlStr))
        if str(decodersXmlTree.getroot().tag) != "Decoder" or str(decodersXmlTree.getroot().attrib) != "{}" or decodersXmlTree.getroot().text is not None :
            notify(self, PANIC, "Received top decoder .xml string missformated:\n")
            return 1
        else:
            topFound = False
            for child in decodersXmlTree.getroot():
                if child.tag == "Top":
                    topFound = True
                    topDecoderXmlConfig = parse_xml(child, {"Author":MANSTR, "Description":MANSTR, "Version":MANSTR, "Date":MANSTR, "URI":OPTSTR, "ServerURI":OPTSTR, "RsyslogReceiver":OPTSTR, "Loglevel":OPTSTR, "NTPServer":OPTSTR, "TimeZone":OPTINT, "PingPeriod":MANFLOAT})
                    global XMLAUTHOR
                    XMLAUTHOR = topDecoderXmlConfig.get("Author")
                    global XMLDESCRIPTION
                    XMLDESCRIPTION = topDecoderXmlConfig.get("Description")
                    global XMLVERSION
                    XMLVERSION = topDecoderXmlConfig.get("Version")
                    global XMLDATE
                    XMLDATE = topDecoderXmlConfig.get("Date")
                    global SYSLOGRECEIVER
                    RSYSLOGRECEIVER = topDecoderXmlConfig.get("RsyslogReceiver")
                    if topDecoderXmlConfig.get("Loglevel") != None :
                        logLevelStr = topDecoderXmlConfig.get("Loglevel")
                        if logLevelStr == "DEBUG_VERBOSE": logLevel = DEBUG_VERBOSE
                        elif logLevelStr == "DEBUG_TERSE": logLevel = DEBUG_TERSE
                        elif logLevelStr == "INFO": logLevel = INFO
                        elif logLevelStr == "ERROR": logLevel = ERROR
                        elif logLevelStr == "PANIC": logLevel = PANIC
                    else:
                        logLevel = DEFAULTLOGLEVEL
                    DEBUGLEVEL = logLevel
                    TRACE.setDebugLevel(logLevel)
                    global NTPSERVER
                    NTPSERVER = topDecoderXmlConfig.get("NTPServer")
                    global TIMEZONE
                    TIMEZONE = topDecoderXmlConfig.get("TimeZone")
                    global PINGPERIOD
                    PINGPERIOD = topDecoderXmlConfig.get("PingPeriod")
                    if DEBUGLEVEL >= INFO:
                        notify(self, INFO, "Top decoder configured: " + str(topDecoderXmlConfig))
            if topFound != True:
                notify(self, PANIC, "Top decoder configuration not found in configuration .xml")

            for child in decodersXmlTree.getroot() :
                if str(child.tag) == "Lightgroups" :
                    if DEBUGLEVEL >= INFO:
                        notify(self, INFO, "Light groups decoder found, configuring it")
                    if (self.lightDecoder).on_configUpdate(child) != 0 :
                        notify(self, PANIC, "Top decoder configuration failed")
                        return 1
                    if DEBUGLEVEL >= INFO:
                        notify(self, INFO, "Light groups decoder successfully configured")
        return 0

    def startAll(self) :
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting the decoder and all its childs")
        if self.getOpState() != OP_CONFIG :
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "Waiting for top encoder and childs to be configured...")
            self.cnt += 1
            if self.cnt > 30:
                notify(self, PANIC, "Have not received a configuration within 30 seconds - restarting....")
            timer(1, self.startAll)
            return
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "A childs have been configured, starting them")
        self.startLightgropupDecoder()
        self.startTurnoutDecoder()
        self.startSensorDecoder()
        self.startSupervision()
        self.opState = OP_WORKING
        unSetSafeMode()
        MQTT.sendMessage(MQTT_OPSTATE_TOPIC + str(URI) + "/", DECODER_UP)
        notify(self, INFO, "Topdecoder with all childs started, opState changed to: " + self.opState)

    def startLightgropupDecoder(self) :
        self.lightDecoder.start()

    def startTurnoutDecoder(self) :
        pass

    def startSensorDecoder(self) :
        pass

    def startSupervision(self) :
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting decoder supervision - PING and watchdog")
        self.missedPings = 0
        if TARGET == MPYTARGET:
            #self.wdt = WDT(timeout=int(PINGPERIOD*1000*3))
            #self.__ntpSync()
            pass
        self.__onWatchdogTimer()
        MQTT.registerMessageCallback(self.__onReturnedPings, MQTT_PING_DOWNSTREAM_TOPIC + URI + "/") 

        return 0

    def __onWatchdogTimer(self):
        if DEBUGLEVEL >= DEBUG_VERBOSE:
            notify(self, DEBUG_VERBOSE, "Watchdog timer - sending a watchdog Ping to the server")
        #print("Watchdog timer")
        #if TARGET == MPYTARGET:
            #self.wdt.feed()
        #print("======== Watchdog period is: " + str(PINGPERIOD))
        print("Insert Watchdog timer")
        timer(PINGPERIOD, self.__onWatchdogTimer)
        MQTT.sendMessage (MQTT_PING_UPSTREAM_TOPIC + URI + "/" , PING, retain=False)
        self.missedPings += 1
        if self.missedPings > 3:
            notify(self, PANIC, "The server is not returning the PING requests, last three recent PING requests returns are missing, rebooting decoder...")
            return 1
        #self.__ntpSync() This should be placed elsewhere
        return 0

    def __onReturnedPings(self, client, topic, payload, regexpRes) :
        self.missedPings = 0
        return 0

    def __ntpSync(self) :
        if NTPSERVER != None:
            if TARGET == CPYTARGET:
                return
            try:
                ntptime.settime()     #Need to fix NTP server and Timezone
            except Exception as e:
                sys.print_exception(e)
                if DEBUGLEVEL >= ERROR:
                    notify(self, ERROR, "NTP time-synchronization failed")

    def getOpState(self) :
        return self.opState


#==============================================================================================================================================
# Class: "lightgroupDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class lightgroupDecoder :
    def __init__(self, channel) :
        self.opState = OP_INIT
        global SMASPECTS
        SMASPECTS = signalMastAspects()
        global FLASHNORMAL
        FLASHNORMAL = flash(SM_FLASH_NORMAL, 50)
        global FLASHSLOW
        FLASHSLOW = flash(SM_FLASH_SLOW, 50)
        global FLASHFAST
        FLASHFAST = flash(SM_FLASH_SLOW, 50) 
        self.lightgroupTable = [None]
        if channel == 0 :
            pin = LEDSTRIP_CH0_PIN
        else : notify(self, PANIC, "Lightdecoder channel: " + str(channel) + " not supported")
        global LEDSTRIP                                             #Should not be global in the end
        LEDSTRIP = WS2811Strip(pin)
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Lightdecoder channel: " + str(channel) + " initialized")
        self.opState = OP_INIT

    def on_configUpdate(self, lightsXmlTree) :
        if self.opState != OP_INIT :
            notify(self, PANIC, "Light group decoder cannot update the configuration")
            self.opState = OP_FAIL
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Light group decoder has received an unverified configuration")
        if self.__parseConfig(lightsXmlTree) != 0 :
            notify(self, PANIC, "Light group decoder configuration failed")
            return 1
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Light group decoder successfully configured")
        self.opState = OP_CONFIG
        cnt = 0
        infoStr = "lightgroupTable:\n"
        for lg in self.lightgroupTable :
            infoStr = infoStr + "lightgroupTable[" + str(cnt) + "] = " + str(lg) +"\n"
            cnt += 1
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, infoStr)
        return 0

    def __parseConfig(self, lightsXmlTree) :
        if str(lightsXmlTree.tag) != "Lightgroups" or str(lightsXmlTree.attrib) != "{}" or lightsXmlTree.text is not None :
            notify(self, PANIC, "Light groups decoder .xml string missformated")
            self.opState = OP_FAIL
            return 1
        else:
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "parsing SignalmastDesc .xml")
            signalmastDescFound = False
            for signalmastDesc in lightsXmlTree :
                if signalmastDesc.tag == "SignalMastDesc":
                    signalmastDescFound = True
                    if SMASPECTS.on_configUpdate(signalmastDesc) != 0:
                        notify(self, PANIC, "Signal mast aspect definitions could not be configured")
                        self.opState = OP_FAIL
                        return 1
                    break
            if signalmastDescFound != True:
                notify(self, PANIC, "Signal mast aspect definitions not found in SignalmastDes .xml fragment")
                self.opState = OP_FAIL
                return 1
            #
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "parsing Lightgroups .xml")
            ligtgroupFound = False
            for lightgroup in lightsXmlTree :
                if str(lightgroup.tag) == "Lightgroup" :
                    ligtgroupFound = True
                    lightDecoderXmlConfig = parse_xml(lightgroup, {"LgType":MANSTR, "LgAddr":MANINT, "LgSystemName":MANSTR})
                    if lightDecoderXmlConfig.get("LgType") == "Signal Mast" :
                        if DEBUGLEVEL >= INFO:
                            notify(self, INFO, "Creating signal mast: " + lightDecoderXmlConfig.get("LgSystemName") + " with LgAddr: " + str(lightDecoderXmlConfig.get("LgAddr")) + " and system name: " + lightDecoderXmlConfig.get("LgSystemName"))
                        mast = mastDecoder(self)
                        while lightDecoderXmlConfig.get("LgAddr") > len(self.lightgroupTable) - 1:
                            self.lightgroupTable.append(None)
                        self.lightgroupTable[lightDecoderXmlConfig.get("LgAddr")] = mast
                        if DEBUGLEVEL >= INFO:
                            notify(self, INFO, "Inserted " + lightDecoderXmlConfig.get("LgSystemName") + " Signal mast object: " + str(self.lightgroupTable[lightDecoderXmlConfig.get("LgAddr")]) + " into lightGroupTable[" + str(lightDecoderXmlConfig.get("LgAddr")) +"]")
                        if mast.on_configUpdate(lightgroup) != 0 :
                            notify(self, PANIC, "Signal mast configuration failed")
                            self.opState = OP_FAIL
                            return 1

                    elif lightDecoderXmlConfig.get("LgType") == "Signal Head" :
                        # Not implemented
                        if DEBUGLEVEL >= ERROR:
                            notify(self, ERROR, lightDecoderXmlConfig.get("LgType") + " is not a support5ed Light group type")

                    elif lightDecoderXmlConfig.get("LgType") == "General Light" :
                        # Not implemented
                        if DEBUGLEVEL >= ERROR:
                            notify(self, ERROR, lightDecoderXmlConfig.get("LgType") + " is not a support5ed Light group type")

                    elif lightDecoderXmlConfig.get("LgType") == "Sequence Lights" :
                        # Not implemented
                        if DEBUGLEVEL >= ERROR:
                            notify(self, ERROR, str(lightDecoderXmlConfig.get("LgType")) + " is not a support5ed Light group type")

                    else : 
                        if DEBUGLEVEL >= ERROR:
                            notify(self, ERROR, str(lightDecoderXmlConfig.get("LgType")) + " is not a supported Light group type")
            if ligtgroupFound == False:
                if DEBUGLEVEL >= INFO:
                    notify(self, INFO, "Info - Ligh groups decoder.xml has no lightgrop elements - the Lightgroup decoder will not start")
                self.opState = OP_UNUSED
                return 1
        return 0


    def start(self) :
        if self.opState == OP_UNUSED :
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "There are no configured Light group instances - no need to start the Ligtht groups object")
            return 0
        if DEBUGLEVEL >= INFO:
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
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting Light group LED strip")
        LEDSTRIP.start()
        return 0

    def getOpState(self) :
        return self.opState


#==============================================================================================================================================
# Class: "mastDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class mastDecoder :
    def __init__(self, upstreamHandle) :
        self.upstreamHandle = upstreamHandle
        self.opState = OP_INIT
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Mast decoder initialized")

    def on_configUpdate(self, mastXmlTree) :
        if DEBUGLEVEL >= DEBUG_TERSE:
            notify(self, DEBUG_TERSE, "Mast decoder has received an unverified configuration")
        if self.opState != OP_INIT:
            notify(self, PANIC, "Mast decoder cannot reinitialize configuration")
        if self.__parseConfig(mastXmlTree) != 0 :
            notify(self, PANIC, "Mast decoder configuration failed")
            return 1
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Mast decoder: " + self.lgSystemName + " successfully configured")
        self.opState = OP_CONFIG
        return 0

    def __parseConfig(self, mastXmlTree) :
        if str(mastXmlTree.tag) != "Lightgroup" or str(mastXmlTree.attrib) != "{}" or mastXmlTree.text is not None :
            notify(self, PANIC, "Lightgroup XML missformated")
            self.opState = OP_FAIL
            return 1
        else :
            mastDecoderXmlConfig = parse_xml( mastXmlTree, {"LgSystemName":MANSTR, "LgUserName":MANSTR, "LgType":MANSTR, "LgAddr":MANINT, "LgSequence":MANINT})
            self.lgSystemName = mastDecoderXmlConfig.get("LgSystemName")
            self.lgUserName = mastDecoderXmlConfig.get("LgUserName")
            self.lgType = mastDecoderXmlConfig.get("LgType")
            self.lgAddr =  mastDecoderXmlConfig.get("LgAddr")
            self.LgLedStripSeq = mastDecoderXmlConfig.get("LgSequence")

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
            if DEBUGLEVEL >= INFO:
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
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Got LgLedStripSeq " + str(self.LgLedStripSeq) + " for Signal mast object: " + str(self) + "\n")

    def start(self) :
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting Mast decoder: " + self.lgSystemName)
        (self.noOfLeds, selfNoOfUsedLeds) = SMASPECTS.getPixelCount(self.mastType)
        self.flashHandleTable=[None]*self.noOfLeds
        LEDSTRIP.register(self.LgLedStripSeq, self.noOfLeds)
        if MQTT.registerMessageCallback(self.on_mastUpdate, "/trains/track/lightgroups/lightgroup/" + URI + "/" + str(self.lgAddr) + "/") != 0 :
            notify(self, PANIC, "Failed to register on_mastUpdate() message callback for topic: /trains/track/lightgroups/lightgroup/" + URI + "/" + str(self.lgAddr) + "/")
            return 1
        else :
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "Mast decoder: " + self.lgSystemName + " started")
            return 0

    def on_mastUpdate(self, client, topic, payload, regexpRes) :
        #print("Sleeping 20 seconds")
        #time.sleep(20)
        (rc, aspect ) = self.parseXmlAspect(payload)
        if rc != 0 :
            if DEBUGLEVEL >= DEBUG_TERSE:
                notify(self, DEBUG_TERSE, "Received signal mast aspect corrupt, skipping...")
        else :
            if DEBUGLEVEL >= DEBUG_TERSE:
                notify(self, DEBUG_TERSE, "Mast: " + self.lgSystemName + ", " + self.mastType + " Received a new aspect: "  + aspect, )
            (rc, appearence) = SMASPECTS.getPixelsAspect(self.mastType, aspect)
            if rc != 0 :
                notify(self, PANIC, "Could not retrieve appearanse for aspect: " + aspect)
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
            notify(self, PANIC, "Aspect XML string missformated")
            return (1, "")
        return (0, str(root.text))

    def visualizeAppearence(self, appearence, pixels) :
        cnt = 0
        print("Appearance changed for mast: " + self.lgSystemName + ": ", end='')
        while cnt < pixels :
            if appearence[cnt] == LIT : print('+', end='')
            if appearence[cnt] == UNLIT : print('-', end='')
            if appearence[cnt] == FLASH : print('*', end='')
            cnt += 1
        print("")

        cnt = 0
        while cnt < pixels :
            if appearence[cnt] == LIT : 
                if self.flashHandleTable[cnt] != None :
                    self.flashObj.unSetflash(self.flashHandleTable[cnt])
                    self.flashHandleTable[cnt] = None
                LEDSTRIP.updateOperationPoint(self.LgLedStripSeq, cnt, self.brightness, self.transitionTime)
            if appearence[cnt] == UNLIT :
                if self.flashHandleTable[cnt] != None :
                    self.flashObj.unSetflash(self.flashHandleTable[cnt])
                    self.flashHandleTable[cnt] = None
                LEDSTRIP.updateOperationPoint(self.LgLedStripSeq, cnt, 0, self.transitionTime) 
            if appearence[cnt] == FLASH :
                if self.flashHandleTable[cnt] == None :
                    self.flashHandleTable[cnt] = self.flashObj.setflash(LEDSTRIP.updateOperationPoint, [self.LgLedStripSeq, cnt, self.brightness, self.transitionTime], LEDSTRIP.updateOperationPoint, [self.LgLedStripSeq, cnt, 0, self.transitionTime])
            cnt += 1


#==============================================================================================================================================
# Class: "signalMastAspects"
# Purpose: Stores the signal mast aspects, signal mast type mappings to the different aspects and each aspect - mast type apearance for involved
#          pixels. The datastructure gets populated from the server provided <Aspects> configuration. Each mast can request information of what 
#          a particular requested aspect means to them in terms of head appearance
#
## Methods: on_configUpdate() - configures the class singleton according to the overall data structure as described below
#          getPixelCount() - Provides both used and total pixels for a particlular mast-type
#          getPixelsAspect() - Provides a pixel appearance map for a certain aspect and mast-type
#
# Data structures
#        self.aspects = ["Stopp",
#                        "Stopp - vänta - begär tillstånd",
#                        "Stopp - manuell växling - vänta - begär tillstånd",
#                        "Stopp - hinder - vänta - begär tillstånd",
#                        "Kör 80 - vänta",
#                        .
#                        .
#                        ]
#
#        self.mastParams = {"Sweden-3HMS:SL-5HL" : {
#                            "noOfPixels" : 6,
#                            "noOfUsedPixels" : 5,
#                            "aspectMap" : [[LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED],
#                                           [FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED],
#                                           [LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED],
#                                           [(FLASH, LIT  , UNLIT, LIT,   UNLIT, UNUSED],
#                                           [LIT,   UNLIT, LIT,   UNLIT, FLASH, UNUSED],
#                                           .
#                                           .
#                                           ]
#                            },
#                           "Sweden-3HMS:DWF-7" : {
#                            "noOfPixels" : 9,
#                            "noOfUsedPixels" : 7,
#                            "aspectMap" : ....
#                            }
#                           }
#
# XML schema fragment:
#           <Aspects>
#				<Aspect>
#					<AspectName>Stopp</AspectName>
#					<Mast>
#						<Type>Sweden-3HMS:SL-5HL></Type>
#						<Head>UNLIT</Head>
#						<Head>LIT</Head>
#						<Head>UNLIT</Head>
#						<Head>UNLIT</Head>
#						<Head>UNLIT</Head>
#						<NoofPxl>6</NoofPxl>
#					</Mast>
#					<Mast>
#                    .
#                    .
#					</Mast>
#				</Aspect>
#           </Aspects>
#==============================================================================================================================================
class signalMastAspects :
    def __init__(self) :
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Initializing signalMastAspects")
        global UNLIT
        UNLIT = 0
        global LIT
        LIT = 1
        global FLASH
        FLASH = 2
        global UNUSED
        UNUSED = 3
        self.aspects = []
        self.mastParams ={}

    def on_configUpdate(self, signalmastDescXml):
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Configuring signalMastAspects")
        found = False
        for signalAspects in signalmastDescXml:
            if signalAspects.tag == "Aspects":
                found = True
                break
        if found != True:
            notify(self, PANIC, "Aspects missing or missformated in signalMast .xml")

        found = False
        for signalAspect in signalAspects:
            if signalAspect.tag == "Aspect":
                found = True
            signalAspectXmlConfig = parse_xml(signalAspect, {"AspectName":MANSTR})
            aspect = signalAspectXmlConfig.get("AspectName")
            self.aspects.append(aspect)
            for mast in signalAspect:
                if mast.tag == "Mast":
                    mastXmlConfig = parse_xml(mast, {"Type":MANSTR, "NoofPxl":MANINT})
                    type = mastXmlConfig.get("Type")
                    NoOfPxl = mastXmlConfig.get("NoofPxl")
                    if self.mastParams.get(type) != None:
                        continue
                    cnt = 0
                    pxl = []
                    for head in mast:
                        if head.tag == "Head":
                            cnt += 1
                    noOfUsedPxl = cnt
                    mastParams = {"NoOfPxl":NoOfPxl, "NoOfUsedPxl":noOfUsedPxl, "aspectMap":None}
                    self.mastParams[type] = mastParams
        if found != True:
            notify(self, PANIC, "Aspect missing or missformated in signalMast .xml")

        for mastType in self.mastParams:
            self.mastParams[mastType]["aspectMap"] = [None] * len(self.aspects)

        for signalAspect in signalAspects:  # maybe check that it is a signalaspect tag
            signalAspectXmlConfig = parse_xml(signalAspect, {"AspectName":MANSTR})
            aspect = signalAspectXmlConfig.get("AspectName")
            index = self.aspects.index(aspect)
            for mast in signalAspect:
                if mast.tag != "Mast":
                    continue
                mastXmlConfig = parse_xml(mast, {"Type":MANSTR, "NoofPxl":MANINT})
                type = mastXmlConfig.get("Type")
                noOfPxl = mastXmlConfig.get("NoofPxl")
                cnt = 0
                appearance = []
                for head in mast:

                    if head.tag == "Head":
                        if head.text == "LIT": appearance.insert(cnt, LIT)
                        elif head.text == "UNLIT": appearance.insert(cnt, UNLIT)
                        elif head.text == "FLASH": appearance.insert(cnt, FLASH)
                        else: notify(self, PANIC, "Aspect is none of LIT, UNLIT or flash")
                        cnt += 1
                while cnt < noOfPxl:
                    appearance.insert(cnt, UNUSED)
                    cnt += 1

                aspectMap = self.mastParams.get(type).get("aspectMap")
                aspectMap.insert(self.aspects.index(aspect), appearance)
                self.mastParams[type]["aspectMap"] = aspectMap
        return 0

    def getPixelCount(self, mastType) :
        return (((self.mastParams).get(mastType)).get("NoOfPxl"), ((self.mastParams).get(mastType)).get("NoOfUsedPxl"))

    def getPixelsAspect(self, mastType, aspect) :
        if aspect == "FAULT" or aspect == "UNKNOWN":
            (noOfpxl, noOfUsedPxl) = self.getPixelCount(mastType)
            pixelAspect =[]
            while noOfpxl != 0:
                pixelAspect.append(LIT)
                noOfpxl -= 1
            return (0, pixelAspect)
        aspectIndex = self.aspects.index(aspect)
        pixelAspect = self.mastParams.get(mastType).get("aspectMap")[aspectIndex]
        if TARGET == MPYTARGET:
            pixelAspect = self.transformAspect(mastType, pixelAspect)
        return (0, pixelAspect)

    def transformAspect(self, mastType, aspect) :
        res = []
        if mastType == "Sweden-3HMS:SL-5HL":
            res.append(aspect[1])
            res.append(aspect[0])
            res.append(aspect[2])
            res.append(aspect[4])
            res.append(aspect[3])
            res.append(aspect[5])
        if mastType == "Sweden-3HMS:SL-3PreHL":
            res.append(aspect[1])
            res.append(aspect[0])
            res.append(aspect[2])
        if mastType == "Sweden-3HMS:PreRoadCrossing":
            res = aspect
        return res

#==============================================================================================================================================
# Class: "WS2811Strip"
# Purpose:
# Methods:
#==============================================================================================================================================

def setSafeMode():
    if DEBUGLEVEL >= INFO:
        notify(None, INFO, "Setting safe-mode")
    if TARGET == MPYTARGET :
        try: 
            LEDSTRIP
        except:
            global OPLEDSTRIP
            OPLEDSTRIP = NeoPixel(Pin(LEDSTRIP_CH0_PIN, Pin.OUT), int(MAX_OP_POINTS/3))
            cnt = 0
            while cnt < int(MAX_OP_POINTS/3):
                OPLEDSTRIP[cnt] = (20, 20, 20)
                cnt += 1
            OPLEDSTRIP.write()
        else:
            LEDSTRIP.setSafeMode()
    return 0

def unSetSafeMode():
    if DEBUGLEVEL >= INFO:
        notify(None, INFO, "Un-setting safe-mode")
    if TARGET == MPYTARGET :
        try: 
            LEDSTRIP
        except:
            if DEBUGLEVEL >= INFO:
                notify(None, INFO, "Could not un-set safe-mode")
        else:
            LEDSTRIP.unSetSafeMode()
    return 0

# Needs refactoring
class WS2811Strip:
    def __init__(self, pin) :
        self.pin = pin
        self.maxOpPoints = MAX_OP_POINTS
        self.totOpPoints = 0
        self.safeMode = False
        self.PopulatingOpWriteBuffer = False
        self.opUpdated = True
        self.OperState = "Init"
        self.sequenceTable = []

    def setSafeMode(self) :
        self.safeMode = True
        cnt = 0
        while cnt < int(MAX_OP_POINTS/3):
            OPLEDSTRIP[cnt] = (20, 20, 20)
            cnt += 1
        OPLEDSTRIP.write()
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Safe mode set")
        return 0

    def unSetSafeMode(self) :
        self.safeMode = False
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Safe mode un-set")
        return 0

    def register(self, sequenceNo, noOfOperationPoints) :                                   # A register request from an OP group (E.g. a signal mast object, 
                                                                                            # a generic light object, or a Turnout object)
        if sequenceNo * noOfOperationPoints > (self.maxOpPoints - 1) :                      # Allocating the OP Sequennce list memory
            notify(self, PANIC, "Number of operation poins exceeds the maximum value of: " + str(self.maxOpPoints))
            return 1
        while len(self.sequenceTable) - 1 < sequenceNo:
            self.sequenceTable.append(None)

        self.sequenceTable[sequenceNo] = {"NoOfOperationPoints":noOfOperationPoints,        # Populating the OP sequence list index with dict for 
                                          "OperationPoints":None}                           # number of OP points handled by the registered OP group 
                                                                                            # instance, the OP status descriptor dict is not yet populated
        opPointTemplate = [None] * noOfOperationPoints                                      # Creating and populating the OP status descriptor dicts
        for n in range(noOfOperationPoints) :                                               # for the Registered OP group with initial values
            opPointTemplate[n] = {"CurrentValue" : 0,
                                  "EndValue" : 0,
                                  "IncrementValue" : 0}
        self.sequenceTable[sequenceNo].update({"OperationPoints" : opPointTemplate})        # Attaching the OP status descriptor dicts to the OP group
                                                                                            # descriptor 
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "OP Group Registered, sequence No: " + str(sequenceNo) + " Number of OP Points: " + str(noOfOperationPoints))

    def start(self) :                                                                       # A request to start the WS2811Strip object
        if DEBUGLEVEL >= INFO:
            notify(self, INFO, "Starting the WS2811 Strip")
        self.opPointWritebuf = [None]                                                       # Allocating the OP Write buffer memory for the WS2811/OP strip
        for seq in self.sequenceTable :
            if seq == None:
                continue
            self.totOpPoints += seq.get("NoOfOperationPoints")
        self.opPointWritebuf = [None]*(self.totOpPoints)
        print("===== Tot no of OP points: " + str(self.totOpPoints))
        if TARGET == MPYTARGET :
            try:
                global OPLEDSTRIP
                OPLEDSTRIP
            except:
                OPLEDSTRIP = NeoPixel(Pin(self.pin, Pin.OUT), int(self.totOpPoints/3))               # Needs checks on the division
        #self.__populateOpPointWriteBuf()                                                   # Calling  __populateOpPointWriteBuf() to populate the OP 
        self.__startOpStripTimer()
        self.OperState = "Started"                                                          # Write buffer memory for the WS2811/OP strip

    def updateOperationPoint(self, OPGroup, OPGroupSeq, newValue, transitionTime=0) :
        #startTime=time.ticks_us()
        if self.safeMode == True:
            if DEBUGLEVEL >= INFO:
                notify(self, INFO, "OP update squelshed due to Safe mode")
            return 1
        #startTime = time.ticks_us()
        opPointTemplate = (self.sequenceTable[OPGroup].get("OperationPoints"))[OPGroupSeq]
        opPointTemplate.update({"EndValue" : newValue})
        
        if DEBUGLEVEL >= DEBUG_VERBOSE:
            notify(self, DEBUG_VERBOSE, "OP update request received, for OP Group: " + str(OPGroup) + " Sequence number: " + str(OPGroupSeq) + " Current value: " + str(opPointTemplate.get("CurrentValue")) + " New value: " + str(newValue) + " Transition time: " + str(transitionTime))
        if transitionTime == 0 :
            incrementValue = 0
        else :

            incrementValue = int((newValue - opPointTemplate.get("CurrentValue"))*10/transitionTime)

        #startTime = time.ticks_us()
        opPointTemplate.update({"IncrementValue" : incrementValue})
        #startTime = time.ticks_us()
        (self.sequenceTable[OPGroup].get("OperationPoints"))[OPGroupSeq] = opPointTemplate
        if DEBUGLEVEL >= DEBUG_VERBOSE:
            notify(self, DEBUG_VERBOSE, "OP updated, for OP Group: " + str(OPGroup) + " Sequence number: " + str(OPGroupSeq) + " Current value: " + str(opPointTemplate.get("CurrentValue")) + " New value: " + str(newValue) + " Transition time: " + str(transitionTime))
        #stopTime=time.ticks_us()
        #print ("Update OpPoint took: " + str(stopTime-startTime))
        return 0

    def __populateOpPointWriteBuf(self) :                                                   # Request to populate the OP writebuffer from the  OP sequence list
                                                                                            # buffer
                                                                                            # This code is always locked by the caller
        opPointStripSeq = 0
        for seq in self.sequenceTable :
            if seq == None:
                continue
            opPointTemplate = seq.get("OperationPoints")
            #print("OP Template: " + str(opPointTemplate))
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
                #print("======= OP Sequence: " + str(opPointStripSeq))
                self.opPointWritebuf[opPointStripSeq] = currentValue
                opPointStripSeq += 1
            #opPointStripSeq += 1

    def __startOpStripTimer(self) :
        #startTime=time.ticks_us()
        #print("Insert OP Strip timer")
        timer(0.01, self.__startOpStripTimer)
        self.__populateOpPointWriteBuf()
        if  self.opUpdated == True :
            self.__startWriteStrip()
        self.opUpdated = False
        #stopTime=time.ticks_us()
        #print ("OpStrip timer took: " + str(stopTime-startTime))

    def __startWriteStrip(self) :
        if TARGET == MPYTARGET :
            pxlCnt = 0
            clrCnt = 0
            print(self.opPointWritebuf)
            for value in self.opPointWritebuf :
                if value == None : value = 0 
                if clrCnt == 0 : 
                    r = value
                    clrCnt += 1
                elif clrCnt == 1 : 
                    g = value
                    clrCnt += 1
                elif clrCnt == 2 : 
                    b = value
                    OPLEDSTRIP[pxlCnt] = (r, g , b)
                    clrCnt = 0
                    pxlCnt += 1
            OPLEDSTRIP.write()
        if (TARGET == CPYTARGET or TARGET == MPYEMULATEDTARGET) and DEBUG_LEVEL == DEBUG_VERBOSE :
            print("OPSTRIP: ", end='')
            for value in self.opPointWritebuf :
                print(str(value) + " ", end='')
                print(", ", end='')
            print("")
        return 0

class flash :
    def __init__(self, freq, dutyCycle) :
        self.handles = []
        self.handleIndex = 0
        self.phase = LIT
        self.ton = dutyCycle/(freq * 100)
        self.toff = 1/freq - self.ton
        self.__on_change()

    def setflash(self, litCallback, litArgs, unlitCallback, unlitArgs) :
        self.handles.append({"Handle" : self.handleIndex, "LitCallback" : litCallback, "LitArgs" : litArgs, "UnlitCallback" : unlitCallback, "UnlitArgs" : unlitArgs})
        if self.phase == LIT :
            litCallback(*litArgs)
        else :
            unlitCallback(*unlitArgs)

        self.handleIndex += 1
        return (self.handleIndex - 1)

    def unSetflash(self, rmhandle) :
        cnt = 0
        found = False
        for handle in self.handles :

            if handle.get("Handle") == rmhandle :
                self.handles.pop(cnt)
                found = True
                break
            cnt += 1
        if found : return 0
        else : return 1

    def __on_change(self) :
        if self.phase == LIT :
            self.phase = UNLIT
            timer(self.toff, self.__on_change)
        else :
            self.phase = LIT
            timer(self.ton, self.__on_change)
        for handle in self.handles :
            if self.phase == LIT :
                cb = handle.get("LitCallback") 
                args = handle.get("LitArgs") 
            else :
                cb = handle.get("UnlitCallback") 
                args = handle.get("UnlitArgs") 
            cb(*args)


#==============================================================================================================================================
# Class: "SensorDecoder"
# Purpose:
# Methods:
#==============================================================================================================================================
class sensorDecoder :
    def __init__(self) :
        pass


#__Main__
#if TARGET == MPYTARGET :
#    _thread.stack_size(20*1024)
#    print("My MAC Address is: " + str(ubinascii.hexlify(network.WLAN().config('mac'),':').decode()))
TRACE = trace()
TRACE.setDebugLevel(DEFAULTLOGLEVEL)
setSafeMode()
getDay0Config()
if TARGET == MPYTARGET :
    do_connect()
TIMER = timer_thread()
MQTT = decoderMqtt()
MQTT.start()
print("MQTT has started")
discovery()
decoder = topDecoder()
#_thread.start_new_thread(decoder.startAll,())
TIMER.start()
#decoder.startAll()
#while True :
    #time.sleep(1)
    #print("Idle loop")
