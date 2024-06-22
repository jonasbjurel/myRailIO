import sys
import time
import os
import threading
import paho.mqtt.client as paho

# OP-States NEEDS CONSOLIDATION
# ---------
OP_WORKING =                0b00000000              # The OP state has the atribute WORKING, the object is working
OP_WORKING_STR =            "WORKING"
SAT_INIT =                  0b00000001
SAT_FAIL =                  0b00000010
SAT_ERRSEC =                0b00000100
OP_DISABLE =                0b00001000              # The OP state has the atribute DISABLE - disbled by the administrative state of the object.
OP_DISABLE_STR =            "DISABLE"
OP_CONTROLBLOCK =           0b00010000              # The OP state has the atribute CONTROLBLOCK - disbled by the operational state of a higher order object.
OP_CONTROLBLOCK_STR =       "CTRLBLCK"


# Administrative states
# ---------------------
ADM_ENABLE =                0                       # The ADMIN state ENABLE
ADM_DISABLE =               1                       # The ADMIN state DISABLE

# MQTT Topics
# -----------
MQTT_PREFIX = "/trains/"
MQTT_DISCOVERY_REQUEST_TOPIC = "track/discoveryreq/"
MQTT_DISCOVERY_RESPONSE_TOPIC = "track/discoveryres/"
MQTT_PING_UPSTREAM_TOPIC = "track/decoderSupervision/upstream/"
MQTT_PING_DOWNSTREAM_TOPIC = "track/decoderSupervision/downstream/"
MQTT_CONFIG_TOPIC = "track/decoderMgmt/"
MQTT_OPSTATE_TOPIC = "track/opState/"
MQTT_LOG_TOPIC = "track/log/"
MQTT_ASPECT_TOPIC = "track/lightgroups/lightgroup/"
MQTT_TURNOUT_TOPIC = "track/turnout/"
MQTT_LIGHT_TOPIC = "track/light/"
MQTT_SATLINK_ADMBLOCK_TOPIC = "track/sateliteLink/admblock/"
MQTT_SATLINK_OPBLOCK_TOPIC = "track/sateliteLink/opblock/"
MQTT_SAT_ADMBLOCK_TOPIC = "track/satelite/admblock/"
MQTT_SAT_OPBLOCK_TOPIC = "track/satelite/opblock/"
MQTT_SAT_PANIC_TOPIC = "track/satelite/panic/" # IS NOT USED

# MQTT payload
# ------------
DISCOVERY = "<DISCOVERY_REQUEST/>"
DECODER_UP = "<OPState>onLine</OPState>"
DECODER_DOWN = "<OPState>offLine</OPState>"
FAULT_ASPECT = "<Aspect>FAULT</Aspect>"
NOFAULT_ASPECT = "<Aspect>NOFAULT</Aspect>"
PING = "<Ping/>"
ADM_BLOCK = "<AdmState>BLOCK</AdmState>"
ADM_DEBLOCK = "<AdmState>DEBLOCK</AdmState>"
PANIC_MSG = "<PANIC/>"
NOPANIC_MSG = "<NOPANIC/>"

# Parameters
# ----------
BROKER = "broker.emqx.io"
DECODER1_URI = "decoder1.jmri.bjurel.com"
DECODER2_URI = "decoder1.jmri.bjurel.com"
SATLINK_NO = 2
SAT_NO = 16
ACT_NO = 4
SENS_NO = 8
LG_NO = 16

BROKER = "broker.emqx.io"

# Common return codes
# -------------------
RC_OK = 0
RC_GEN_ERR = 255
RC_BUSY = 1
RC_PARAM_ERR = 2
RC_NOT_FOUND = 3
RC_ALREADY_EXISTS = 4


# ==============================================================================================================================================
# Class: systemState
# ==============================================================================================================================================
class systemState:
    def __init__(self):
        self.admBlockState = ADM_BLOCK
        self.opBlockState = 0
        self.parent = None
        self.childs = []
        self.callBacks = []
        self.lock = threading.Lock()

    def setParent(self, parent):
        self.parent = parent
        return RC_OK

    def regCb(self, callback):
        self.lock.acquire()
        self.callBacks.append(callback)
        callback()
        self.lock.release()
        return RC_OK

    def unRegCb(self, callback):
        self.lock.acquire()
        self.callBacks = [x for x in self.callBacks if x != callBack]
        self.lock.release()
        return RC_OK

    def addChild(self, child):
        self.lock.acquire()
        self.childs.append(child)
        self.__evalStates()
        self.lock.release()
        return RC_OK

    def deleteChild(self, child):
        self.lock.acquire()
        self.childs = [x for x in self.childs if x != child]
        child.opBlock(OP_CTRLBLCK)
        self.lock.release()

    def getChilds(self):
        return self.childs

    def admBlock(self):
        self.lock.acquire()
        if self.admBlockState == ADM_DISABLE:
            self.lock.release()
            return RC_WRONG_STATE
        for child in self.childs:
            if child.getAdmState == ADM_ENABLE:
                self.lock.release()
                return RC_WRONG_STATE
        self.admBlockState = DISABLE
        self.__evalStates()
        self.lock.release()
        return RC_OK

    def admDeBlock(self):
        self.lock.acquire()
        if self.admBlockState == ADM_ENABLE:
            self.lock.release()
            return RC_WRONG_STATE
        try:
            if self.parent.getAdmState() == ADM_DISABLE:
                self.lock.release()
                return RC_WRONG_STATE
            else: 
                self.admBlockState == ADM_ENABLE
                self.__evalStates()
                self.lock.release()
                return RC_OK
        except:
            self.admBlockState == ADM_ENABLE
            self.__evalStates()
            self.lock.release()
            return RC_OK

    def opBlock(self, opBlockState):
        self.lock.acquire()
        tmpOpBlockState = self.opBlockState
        self.opBlockState = self.opBlockState | opBlockState
        if self.opBlockState != tmpOpBlockState:
            self.__evalStates()
        self.lock.release()
        return RC_OK

    def opDeBlock(self, opDeBlockState):
        self.lock.acquire()
        tmpOpBlockState = self.opBlockState
        self.opBlockState = self.opBlockState & ~opDeBlockState
        if self.opBlockState != tmpOpBlockState:
            self.__evalStates()
        self.lock.release()
        return RC_OK

    def getAdmState(self):
        return self.admBlockState

    def getOpState(self):
        return self.opBlockState

    def __evalStates(self):
        if self.admBlockState == ADM_DISABLE:
            self.opBlockState = self.opBlockState | OP_DISABLE
        else:
            self.opBlockState = self.opBlockState & ~OP_DISABLE
        for child in self.childs:
            if (self.opBlockState == OP_WORKING) and (child.getOpState() & OP_CONTROLBLOCK):
                child.opDeBlock(OP_CONTROLBLOCK)
            elif (self.opBlockState != OP_WORKING) and not (child.getOpState() & OP_CONTROLBLOCK):
                child.opBlock(OP_CONTROLBLOCK)
        for callback in self.callBacks:
            callback()
        return RC_OK

    def __del__(self):
        pass


# ==============================================================================================================================================
# Class: mqttListener
# Purpose:  Listen and dispatches incomming mqtt mesages
# Data structures: None
# ==============================================================================================================================================
class mqttListener:
    def __init__(self, broker):
        self.client = paho.Client("client-001")
        self.client.on_message = self.notifyMqttMessage
        self.client.loop_start()
        print("connecting to broker ", broker)
        self.client.connect(broker)
        time.sleep(2)
        self.subscriptions = {}

    def subscribe(self, topic, cb):
        if not self.subscriptions.get(topic):
            self.subscriptions[topic] = []
            self.client.subscribe(topic) #subscribe
        self.subscriptions[topic].append(cb)
        return RC_OK

    def unSubscribe(self, topic, cb):
        while True:
            try:
                self.subscriptions[topic].delete(cb)
            except:
                break
        try:
            if self.subscriptions[topic] == []:
                self.subscriptions.pop(topic)
                self.client.unsubscribe(topic)
        except:
            pass
        return RC_OK

    def notifyMqttMessage(self, client, userdata, message):
        try:
            for cb in self.subscriptions.get(message.topic):
                threading.Thread(target=cb, args=(message.topic, message.payload, )).start()
        except:
            pass

    def publish(self, topic, message=None):
        self.client.publish(topic, message)
        return RC_OK

    def __delete__(self):
        self.client.disconnect(broker)
        del self.client


# ==============================================================================================================================================
# Class: decoder
# Purpose: 
# Data structures: None
# ==============================================================================================================================================
class decoder:
    def __init__(self, decoderURI):
        self.decoderURI = decoderURI
        self.supervision = False
        self.missedPingCnt = 0
        self.configured = False
        self.discovered = False
        self.MQTT = mqttListener(BROKER)
        self.state = systemState()
        self.state.regCb(self.onOpStateChange)
        self.satLinks = [None] * SATLINK_NO
        self.lgs = [None] * LG_NO
        self.state.opBlock(SAT_INIT)

    def start(self):
        self.__discover()
        self.__getConfig()
        for satLink in range(SATLINK_NO):
            self.satLinks[satLink] = sateliteLink(self.decoderURI, satLink, self.MQTT, self)
            self.state.addChild(self.satLinks[satLink].state)
            self.satLinks[satLink].start()
        for lg in range(LG_NO):
            self.lgs[lg] = lightGroup(self.decoderURI, lg, self.MQTT, self)
            self.state.addChild(self.lgs[lg].state)
            self.lgs[lg].start()
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        self.startSupervision()
        return RC_OK

    def startSupervision(self):
        self.__startPing()
        return RC_OK

    def stopSupervision(self):
        self.__stopPing()
        return RC_OK

    def setSens(self, satLink, sat, sens, sensVal):
        self.satLinks[satLink].setSense(sat, sens, sensVal)
        return RC_OK

    def __discover(self):
        self.discovered = False
        self.MQTT.publish(MQTT_PREFIX + MQTT_DISCOVERY_REQUEST_TOPIC)
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_DISCOVERY_RESPONSE_TOPIC, self.onDiscovery)
        time.sleep(5)
        if not self.discovered:
            print("Did not get a discovery response - Rebooting...")
            self.__reboot()
        return RC_OK

    def onDiscovery(self, topic, message):
        self.discovered = True
        print("Got a discovery response:")
        print(message)

    def __getConfig(self):
        self.configured = False
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_CONFIG_TOPIC + self.decoderURI + "/", self.onConfig)
        time.sleep(5)
        if not self.configured:
            print("Did not get a configuration - Rebooting...")
            self.__reboot()
        return RC_OK

    def onConfig(self, topic, message):
        if self.configured:
            print("Got a spurious configuration - Rebooting")
            self.__reboot()
        self.configured = True
        print("Got configuration:")
        print(message)
        return RC_OK

    def __startPing(self):
        if self.supervision:
            return RC_ALREADY_EXISTS
        self.supervision = True
        self.missedPingCnt = 0
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_PING_DOWNSTREAM_TOPIC + self.decoderURI + "/", self.onPing)
        self.pingTimer = threading.Timer(10, self.onPingTimer)
        self.pingTimer.start()
        return RC_OK

    def __stopPing(self):
        if not self.supervision:
            return RC_NOT_FOUND
        self.supervision = False
        self.MQTT.unSubscribe(MQTT_PREFIX + MQTT_PING_DOWNSTREAM_TOPIC + self.decoderURI + "/", self.onPing)
        self.pingTimer.cancel()
        return RC_OK

    def onPing(self, topic, message):
        self.missedPingCnt = 0

    def onPingTimer(self):
        print("onPingTimer")
        self.pingTimer = threading.Timer(10, self.onPingTimer)
        self.pingTimer.start()
        self.missedPingCnt += 1
        if self.missedPingCnt > 3:
            print("Missed > 3 ping responses - Rebooting")
            self.__reboot()
        self.MQTT.publish(MQTT_PREFIX + MQTT_PING_UPSTREAM_TOPIC + self.decoderURI + "/", PING)

    def onOpStateChange(self):
        if self.state.getOpState():
            self.MQTT.publish(MQTT_PREFIX + MQTT_OPSTATE_TOPIC + self.decoderURI + "/", DECODER_DOWN)
        else:
            self.MQTT.publish(MQTT_PREFIX + MQTT_OPSTATE_TOPIC + self.decoderURI + "/", DECODER_UP)
        print("Decoder: " + self.decoderURI + "/" + " opState changed:" +
              str(self.state.getOpState()))

    def blockDecoderOpState(self, opState):
        self.state.opBlock(opState)
        return RC_OK

    def deBlockDecoderOpState(self, opState):
        self.state.opDeBlock(opState)
        return RC_OK


    def blockSatLinkOpState(self, satLink, opState):
        self.satLinks[satLink].blockSatLinkOpState(opState)
        return RC_OK

    def deBlockSatLinkOpState(self, satLink, opState):
        self.satLinks[satLink].deBlockSatLinkOpState(opState)
        return RC_OK

    def blockSatOpState(self, satLink, sat, opState):
        self.satLinks[satLink].blockSatOpState(sat, opState)
        return RC_OK

    def __unBlockSatOpState(self, satLink, sat, opState):
        self.satLinks[satLink].deBlockSatOpState(sat, opState)

    def __failSafe(self):
        pass

    def __reboot(self):
        os.execv(sys.executable, ['python'] + sys.argv)        #os.execl(sys.executable, sys.executable, *sys.argv)
        sys.exit()


# ==============================================================================================================================================
# Class: sateliteLink
# ==============================================================================================================================================
class sateliteLink:
    def __init__(self, decoderURI, satLink, mqttHandle, parentHandle):
        self.decoderURI = decoderURI
        self.satLink = satLink
        self.MQTT = mqttHandle
        self.parentHandle = parentHandle
        self.state = systemState()
        self.state.setParent(self.parentHandle.state)
        self.state.regCb(self.onOpStateChange)
        self.sats = [None] * SAT_NO
        self.state.opBlock(SAT_INIT)

    def start(self):
        print("Starting Satelite Link")
        for sat in range(SAT_NO):
            print("Starting Satelite" + str(sat))
            self.sats[sat] = satelite(self.decoderURI, self.satLink, sat, self.MQTT, self)
            self.state.addChild(self.sats[sat].state)
            self.sats[sat].start()
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        return RC_OK

    def setSens(self, sat, sens, sensVal):
        self.sats[sat].setSens(sens, sensVal)
        return RC_OK

    def onOpStateChange(self):
        self.MQTT.publish(MQTT_PREFIX + MQTT_SATLINK_OPBLOCK_TOPIC + self.decoderURI + "/" +
                          str(self.satLink) + "/", str(self.state.getOpState()))
        print("Satelite Link: " + self.decoderURI + "/" + str(self.satLink) + "/" + " opState changed:" +
              str(self.state.getOpState()))

    def blockSatLinkOpState(self, opState):
        self.state.opBlock(opState)
        return RC_OK

    def deBlockSatLinkOpState(self, opState):
        self.state.opDeBlock(opState)
        return RC_OK

    def blockSatOpState(self, sat, opState):
        self.sats[sat].blockSatOpState(opState)
        return RC_OK

    def deBlockSatOpState(self, sat, opState):
        self.sats[sat].deBlockSatOpState(opState)
        return RC_OK


# ==============================================================================================================================================
# Class: satelite
# ==============================================================================================================================================
class satelite:
    def __init__(self, decoderURI, satLink, sat, mqttHandle, parentHandle):
        self.decoderURI = decoderURI
        self.satLink = satLink
        self.sat = sat
        self.acts = [None] * ACT_NO
        self.sens = [None] * SENS_NO
        self.parentHandle = parentHandle
        self.MQTT = mqttHandle
        self.state = systemState()
        self.state.setParent(self.parentHandle.state)
        self.state.regCb(self.onOpStateChange)
        self.state.opBlock(SAT_INIT)

    def start(self):
        print("Starting")
        for sens in range(SENS_NO):
            self.sens[sens] = sensor(self.decoderURI, self.satLink, self.sat, sens, self.MQTT, self)
            self.state.addChild(self.sens[sens].state)
            self.sens[sens].start()
        for acts in range(ACT_NO):
            self.acts[acts] = actuator(self.decoderURI, self.satLink, self.sat, acts, self.MQTT, self)
            self.state.addChild(self.sens[sens].state)
            self.acts[acts].start()
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        return RC_OK

    def setSens(self, sens, sensVal):
        self.sens[sens].setSens(sensVal)
        return RC_OK

    def onOpStateChange(self):
        self.MQTT.publish(MQTT_PREFIX + MQTT_SAT_OPBLOCK_TOPIC + self.decoderURI + "/" +
                          str(self.satLink) + "/" + str(self.sat) + "/", str(self.state.getOpState()))
        print("Satelite: " + self.decoderURI + "/" + str(self.satLink) + "/" + str(self.sat) + "/" + " opState changed:" +
              str(self.state.getOpState()))

    def blockSatOpState(self, opState):
        self.state.opBlock(opState)
        return RC_OK

    def deBlockSatOpState(self, opState):
        self.state.opDeBlock(opState)
        return RC_OK


# ==============================================================================================================================================
# Class: actuator
# ==============================================================================================================================================
class actuator:
    def __init__(self, decoderURI, satLink, sat, act, mqttHandle, parentHandle):
        self.decoderURI = decoderURI
        self.satLink = satLink
        self.sat = sat
        self.act = act
        self.MQTT = mqttHandle
        self.parentHandle = parentHandle
        self.state = systemState()
        self.state.setParent(self.parentHandle.state)
        self.state.regCb(self.onOpStateChange)
        self.state.opBlock(SAT_INIT)

    def start(self):
        print(">>>>>>>>>>>>>>Subscribing to: " + MQTT_PREFIX + MQTT_TURNOUT_TOPIC + str(self.act))
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_TURNOUT_TOPIC + str(self.act),
                            self.onTurnout)
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_LIGHT_TOPIC + str(self.act),
                            self.onLight)
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        return RC_OK

    def onTurnout(self, topic, message):
        if self.state.getOpState():
            print("Actuator: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.act) + "/" +
                  " Received new Turnout position, but opState prevents movement - rejecting it")
        else:
            print("Actuator: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.act) + "/" +
                  " Received new TurnOut position:" + str(message))

    def onLight(self, topic, message):
        if self.state.getOpState():
            print("Actuator: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.act) + "/" +
                  " Received new Light signature, but opState prevents the change - rejecting it")
        else:
            print("Actuator: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.act) + "/" +
                  " Received new Light signature:" + str(message))

    def onOpStateChange(self):
        print("Actuator: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.act) + "/" + " opState changed:" +
                  str(self.state.getOpState()))


# ==============================================================================================================================================
# Class: sensor
# ==============================================================================================================================================
class sensor:
    def __init__(self, decoderURI, satLink, sat, sens, mqttHandle, parentHandle):
        self.decoderURI = decoderURI
        self.satLink = satLink
        self.sat = sat
        self.sens = sens
        self.MQTT = mqttHandle
        self.parentHandle = parentHandle
        self.state = systemState()
        self.state.setParent(self.parentHandle.state)
        self.state.regCb(self.onOpStateChange)
        self.state.opBlock(SAT_INIT)

    def start(self):
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        return RC_OK

    def setSense(self, sensVal):
        if self.state.getOpState():
            print("Sensor: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.sens) +
                  "/ Received a new Sensor value, but opState prevents reporting - rejecting it")
        else:
            print("Sensor: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.sens) +
                  "/ Received a new Sensor value: " + str(sensVal))
            if sensVal == 0: order = "INACTIVE"
            else:            order = "ACTIVE"
            self.MQTT.publish(MQTT_PREFIX + MQTT_SATLINK_OPBLOCK_TOPIC + self.decoderURI + "/" +
                              str(self.satLink) + "/" + str(self.sat) + "/" + str(self.sens) + "/",
                              order)

    def onOpStateChange(self):
        print("Sensor: " + self.decoderURI + "/" + str(self.satLink) + "/" +
                  str(self.sat) + "/" + str(self.sens) + "/" + " opState changed:" +
                  str(self.state.getOpState()))


# ==============================================================================================================================================
# Class: lightGroup
# ==============================================================================================================================================
class lightGroup:
    def __init__(self, decoderURI, lg, mqttHandle, parentHandle):
        self.decoderURI = decoderURI
        self.lg = lg
        self.MQTT = mqttHandle
        self.parentHandle = parentHandle
        self.state = systemState()
        self.state.setParent(self.parentHandle.state)
        self.state.regCb(self.onOpStateChange)
        self.state.opBlock(SAT_INIT)

    def start(self):
        self.MQTT.subscribe(MQTT_PREFIX + MQTT_ASPECT_TOPIC + self.decoderURI + "/"
                            + str(self.lg) + "/",
                            self.onLightGroup)
        self.state.opDeBlock(SAT_INIT)
        self.state.admDeBlock()
        return RC_OK

    def onLightGroup(self, topic, message):
        if self.state.getOpState():
            print("LightGroup: " + self.decoderURI + "/" + str(self.lg) + "/" +
                  " Received new Aspect, but opState prevents change - rejecting it")
        else:
            print("LightGroup: " + self.decoderURI + "/" + str(self.lg) + "/" +
                  " Received new Aspect:" + str(message))

    def onOpStateChange(self):
        print("LightGroup: " + self.decoderURI + "/" + str(self.lg) + "/" + " opState changed:" +
                  str(self.state.getOpState()))


#__main__
decoder1Handle = decoder(DECODER1_URI)
decoder1Handle.start()
print("System started")
time.sleep(30)
print("Blocking satelite links")
for link in range(SATLINK_NO):
    print("ERR SEC for link: " + str(link))
    decoder1Handle.blockSatLinkOpState(link, SAT_ERRSEC)
    time.sleep(5)
    print("FAIL for link: " + str(link))
    decoder1Handle.deBlockSatLinkOpState(link, SAT_ERRSEC)
    decoder1Handle.blockSatLinkOpState(link, SAT_FAIL)
    time.sleep(5)
    print("WORKING for link: " + str(link))
    decoder1Handle.deBlockSatLinkOpState(link, SAT_FAIL)

while True:
    time.sleep(1)
