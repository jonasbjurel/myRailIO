# Sample script to set all un-closed turnouts to Closed
#
# After this script is turn, all the Turnouts defined in JMRI
# should be in the CLOSED state.
#
# Note that some "Turnouts" may actually drive other things on the layout,
# such as signal heads. This script should be run _before_ anything that
# sets those.  
#
# Part of the JMRI distribution

import java
import java.beans
import jmri
from time import sleep
import xml.etree.ElementTree as ET
import xml.dom.minidom
from org.python.core.util import StringUtil

class myMqttListener(jmri.jmrix.mqtt.MqttEventListener) :
    def notifyMqttMessage(self, topic, message) :
        # all this listener does is print
        print("Received from MQTT \""+topic+"\" \""+message+"\"")

class __lightgroup :
    def __init__(self) :
        pass

    def setControllerURL(self, controllerURL) :
        self.controllerURL = controllerURL

    def getControllerURL(self) :
        return self.controllerURL

    def setLgAddr(self, LgAddr) :
        self.LgAddr = LgAddr
    def getLgAddr(self) :
        return self.LgAddr

    def setSystemName(self, SystemName) :
        self.SystemName = SystemName
    def getSystemName(self) :
        return self.SystemName

    def setUserName(self, UserName) :
        self.UserName = UserName
    def getUserName(self) :
        return self.UserName

    def setLightgroupType (self, lightgroupType) :
        self.lightgroupType = lightgroupType
    def getLightgroupType (self) :
        return self.lightgroupType

    def setLgSeq(self, LgSeq) :
        self.LgSeq = LgSeq
    def getLgSeq(self) :
        return self.LgSeq

    def setLightgroupDesc(self, LightgroupDesc) :
        self.LightgroupDesc = LightgroupDesc
    def getLightgroupDesc(self) :
        return self.LightgroupDesc

    def setLightgroupComment(self, LightgroupComment) :
        self.LightgroupComment = LightgroupComment
    def getLightgroupComment(self) :
        return self.LightgroupComment

    def verify(self) :
        try :
            self.controllerURL
            self.LgAddr
            self.SystemName
            self.UserName
            self.lightgroupType
            self.LgSeq
            self.LightgroupDesc
        except :
            return 1
        else :
            return 0

    def printStruct (self) :
        print "Controller URL: " + self.controllerURL
        print "LG Address: "+ str(self.LgAddr)
        print "System Name: " + self.SystemName
        print "User Name: " + self.UserName
        print "Lightgroup type: " + self.lightgroupType
        print "Lightgroup Sequence: " + str(self.LgSeq)
        print "Lightgroup Descriptor: " + str(self.LightgroupDesc)
        
class __signalMastlightgroup :
    def __init__(self) :
        pass

    def setSmType (self, SmType) :
        self.SmType = SmType
    def getSmType (self) :
        return self.SmType

    def setSmAspect (self, SmAspect) :
        self.SmAspect = SmAspect
    def getSmAspect (self) :
        return self.SmAspect

    def setSmDimTime (self, SmDimTime) :
        self.SmDimTime = SmDimTime
    def getSmDimTime (self) :
        return self.SmDimTime

    def setSmFlashFreq (self, SmFlashFreq) :
        self.SmFlashFreq = SmFlashFreq
    def getSmFlashFreq (self) :
        return self.SmFlashFreq

class SmListener(java.beans.PropertyChangeListener):
  def propertyChange(self, event):
    print "change",event.propertyName
    print "from", event.oldValue, "to", event.newValue
    print "source systemName", event.source.systemName
    print "source userName", event.source.userName
    print "LGAddr: " + str(smLg(event.source.systemName))
    lightsMQTT.aspectUpdate(event.source.systemName, event.newValue)

def decodeSmSystemName(systemName) :
    if str(systemName).index("IF$vsm:") == 0 :
        delimit = str(systemName).index("($")
        cnt = 0
        mastType = ""
        signalgroup_str = ""
        for x in str(systemName) :
            cnt +=1
            if cnt > 7 and cnt <= delimit :
                mastType=mastType+x
            if cnt > delimit+2 and x != ")" :
                signalgroup_str=signalgroup_str+x
        return [1, int(signalgroup_str), mastType]
    else :
        return [0, int(signalgroup_str), mastType]

def smIsVirtual (systemName) :
    return decodeSmSystemName(systemName)[0]

def smLg (systemName) :
    return decodeSmSystemName(systemName)[1]

def smType (systemName) :
        return decodeSmSystemName(systemName)[2]

class __lightsMQTT :
    def __init__(self) :
        self.mqttAdapter = jmri.InstanceManager.getDefault(jmri.jmrix.mqtt.MqttSystemConnectionMemo).getMqttAdapter()
        #TestCode
        #self.mqttAdapter.subscribe("track/lightgroups/controllerMgmt/lightcontroller1.bjurel.com/", myMqttListener())
        #self.mqttAdapter.subscribe("track/lightgroups/controllerMgmt/lightcontroller2.bjurel.com/", myMqttListener())
        #self.mqttAdapter.subscribe("track/lightgroups/lightgroup/lightcontroller1.bjurel.com/1/", myMqttListener())
        #self.mqttAdapter.subscribe("track/lightgroups/lightgroup/lightcontroller2.bjurel.com/2/", myMqttListener())
        self.tid = 0

    def getMqttAdapter(self) :
        return self.mqttAdapter

    def controllerUpdate (self, controllerURL, payload) :
        topic = "track/lightgroups/controllerMgmt/" + controllerURL + "/"
        #payload = "<tid>" + str(self.tid) + "</tid>\n" + str(payload) #- tid cannot be handled this way for correct XMLsyntax
        payload = str(payload)

        print "Sending Controller update to MQTT brooker:" + "\ntopic:" + str(topic) + "\nPayload:\n" + str(payload)
        self.mqttAdapter.publish(topic, payload)
        self.tid += 1

    def controllerSelfTest (self, controllerURL) :
        topic = "track/lightgroups/controllerMgmt/" + controllerURL + "/"
        #payload = "<tid>" + str(self.tid) + "</tid>\n" + "<SelfTest/>" #- tid cannot be handled this way for correct XMLsyntax
        payload = "<SelfTest/>"
        print "Sending Controller update to MQTT brooker:" + "\ntopic:" + str(topic) + "\nPayload:\n" + str(payload)
        self.mqttAdapter.publish(topic, payload)
        self.tid += 1

    def aspectUpdate (self, systemName, aspect) :
        lg = smLg(systemName)
        print lg
        url = lightgroupsTable[lg].getControllerURL()
        print url
        topic = "track/lightgroups/lightgroup/" + url + "/" + str(lg) + "/"
        #payload = "<tid>" + str(self.tid) + "</tid>\n" + "<Aspect>" + aspect + "</Aspect>" #- tid cannot be handled this way for correct XMLsyntax
        payload = "<Aspect>" + aspect + "</Aspect>"

        print "Sending aspect update to MQTT brooker\ntopic: " + topic + "\nPayload: " + payload
        self.mqttAdapter.publish(topic, payload)
        (lightgroupsTable[lg].getLightgroupDesc()).setSmAspect(aspect)
        self.tid += 1

#aspects=    [Stopp,
#            Stopp_vanta_begar_tillstand, 
#            Stopp_manuellvaxling_vanta_begar_tillstand, 
#            Stopp_hinder_vanta_begar_tillstand, 
#            Kor80_vanta, 
#            Kor40_v�nta, 
#            Kor40_vanta_kor40, 
#            Kor40_vanta_stopp, 
#            Kor80_vanta_kor80, 
#            Kor80_vanta_kor40, 
#            Kor80_vanta_stopp, 
#            Vanta_stopp, 
#            Vanta_kor80, 
#            Vanta_kor40]

xmlAuthor = ""
xmlDescription = ""
xmlVersion = ""
xmlDate = ""
url = ""
lightgroupsTable = [""] * 16385
controllersTable = [""] * 64
maxLgAddr = 0
lightsMQTT = __lightsMQTT()
#(lightsMQTT.getMqttAdapter).subscribe("test", myMqttListener())


controllersXmlTree = ET.parse('C:\Users\jonas\JMRI\MQTT_test.jmri\lightcontrollers.xml')
if str(controllersXmlTree.getroot().tag) != "Lightcontrollers" or str(controllersXmlTree.getroot().attrib) != "{}" or str(controllersXmlTree.getroot().text.strip()) != "":
    raise Exception("Error - lightcontroller.xml is missformated - exped root tag: Lightcontrollers with no attributes or values")

for child in controllersXmlTree.getroot() :

    if str(child.tag) == "Author" :
        if str(child.attrib) != "{}" :
            raise Exception("Error - lightcontroller.xml is missformated - <Author> tag not expected to have any attributes")
        xmlAuthor = str(child.text.strip())

    if str(child.tag) == "Description" :
        if str(child.attrib) != "{}" :
            raise Exception("Error - lightcontroller.xml is missformated - <Description> tag not expected to have any attributes")
        xmlDescription = str(child.text.strip())

    if str(child.tag) == "Version" :
        if str(child.attrib) != "{}" :
            raise Exception("Error - lightcontroller.xml is missformated - <Version> tag not expected to have any attributes")
        xmlVersion = str(child.text.strip())

    if str(child.tag) == "Date" :
        if str(child.attrib) != "{}" :
            raise Exception("Error - lightcontroller.xml is missformated - <Date> tag not expected to have any attributes")
        xmlDate = str(child.text.strip())

    if child.tag == "Controllers" :
        if str(child.attrib) != "{}" or str(child.text.strip()) != "" :
            raise Exception("Error - lightcontroller.xml is missformated - <Controllers> tag not expected to have any attributes")
        controllerCnt = 0
        for controllers in child :
            if controllers.tag == "Controller" :
                print "Building controller Table instance"
                for controller in controllers :
                    if str(controllers.attrib) != "{}" or str(controllers.text.strip()) != "" :
                        raise Exception("Error - lightcontroller.xml is missformated - <Controller> tag not expected to have any attributes")
                    if str(controller.tag) == "URL" :
                        url=str(controller.text)
                        controllersTable[controllerCnt] = url
                        print "controllerTable[" + str(controllerCnt) + "] = " + controllersTable[controllerCnt]
                        controllerCnt += 1
                if url == "" :
                    raise Exception("Error - lightcontroller.xml is missformated - <URL> is missing for controller " + controllerCnt)
                print "-----------"

                print "Building lightgroupsTable instance from lightcontrollers.xml"
                for controller in controllers :
                    LgSeqCnt = 0
                    if str(controller.tag) == "Lightgroups" :
                        for lightgroups in controller :
                            if str(lightgroups.attrib) != "{}" :
                                raise Exception("Error - lightcontroller.xml is missformated - <URL> is missing for controller")
                            lightgroupsTable[int(str(lightgroups.text))] = __lightgroup()
                            lightgroupsTable[int(lightgroups.text.strip())].setControllerURL(url)
                            lightgroupsTable[int(str(lightgroups.text.strip()))].setLgAddr(int(lightgroups.text.strip()))
                            if maxLgAddr < int(lightgroups.text.strip()) :
                                maxLgAddr = int(lightgroups.text.strip())
                            lightgroupsTable[int(str(lightgroups.text.strip()))].setLgSeq(LgSeqCnt)
                            LgSeqCnt +=1
                            print "--------"
if xmlAuthor == "" :
    raise Exception("Error - lightcontroller.xml is missformated - no mandatory <Author> tag/property/value found")
if xmlDescription == "" :
     raise Exception("Error - lightcontroller.xml is missformated - no mandatory <Description> tag/property/value found")
if xmlVersion == "" :
     raise Exception("Error - lightcontroller.xml is missformated - no mandatory <Version> tag/property/value found")
if xmlDate == "" :
     raise Exception("Error - lightcontroller.xml is missformated - no mandatory <Date> tag/property/value found")

# loop thru all defined signal masts
smCnt = 0
print "Building lightgroupsTable from JMRI signal mast definitions"
for sm in masts.getNamedBeanSet() :
    print "Building lightgroupsTable instance from JMRI signal mast definitions"
    smCnt += 1
    print "System Name:" + str(sm)
    if smIsVirtual(sm) == 1 :
        lightgroupsTable[smLg(sm)].setSystemName(str(sm))
        lightgroupsTable[smLg(sm)].setUserName(sm.getUserName())
        lightgroupsTable[smLg(sm)].setLightgroupType(sm.getBeanType())
        lightgroupsTable[smLg(sm)].setLightgroupComment(str(sm.getComment()))

        lgDesc = __signalMastlightgroup()
        lightgroupsTable[smLg(sm)].setLightgroupDesc(lgDesc)
        lgDesc.setSmType(smType(sm))
        lgDesc.setSmAspect(sm.getAspect())
        lgDesc.setSmDimTime("NORMAL")
        lgDesc.setSmFlashFreq("NORMAL")
        print "---------"

print str(smCnt) + " JMRI Signal masts found"
print "---------"

# Validate lightgroupsTable - check if complete
print "Central lightgroups and controllers data structures built"
print "--------"

print "Now building controller specific xml data structures"
controllerCnt = 0
while controllersTable[controllerCnt] != "" and controllerCnt < 64 :
    print "Controller: " + str(controllersTable[controllerCnt])
    #waitMsec(100)
    controllerXmlTree = ET.TreeBuilder()
    controllerXmlTree.start("Controller", {})
    controllerXmlTree.start("Author", {})
    controllerXmlTree.data(xmlAuthor)
    controllerXmlTree.end("Author")
    controllerXmlTree.start("Version", {})
    controllerXmlTree.data(xmlVersion)
    controllerXmlTree.end("Version")
    controllerXmlTree.start("Date", {})
    controllerXmlTree.data(xmlDate)
    controllerXmlTree.end("Date")
    controllerXmlTree.start("URL", {})
    controllerXmlTree.data(str(controllersTable[controllerCnt]))
    controllerXmlTree.end("URL")
    controllerXmlTree.start("Lightgroups", {})
    lightgroupCnt = 0
    lgSeqTable = [""] * 16384 #optimize by defining Max Sequence Cnt per controller
    print "Building lgSeqTable for controller: " + controllersTable[controllerCnt]
    while lightgroupCnt < 10 : #optimize by defining Max Lightgroup Cnt
        if lightgroupsTable[lightgroupCnt] != "" :
            if lightgroupsTable[lightgroupCnt].getControllerURL() == controllersTable[controllerCnt] :
                lgSeqTable[lightgroupsTable[lightgroupCnt].getLgSeq()] = lightgroupCnt
                print "Lightgroup: " + str(lightgroupCnt) + "  Led strip sequence: " + str(lgSeqTable[lightgroupsTable[lightgroupCnt].getLgSeq()])
        lightgroupCnt += 1
    print "Following lgSseqTable was built for controller: " + str(controllersTable[controllerCnt])
    print "lgSeq, LgAddr"
    lgSeqCnt = 0
    while lgSeqTable[lgSeqCnt] != "" :
        print str(lgSeqCnt) + "       " + str(lgSeqTable[lgSeqCnt])
        lgSeqCnt += 1

    print "---------"

    lightgroupCnt = 0
    while lightgroupCnt < 10 :
        if lgSeqTable[lightgroupCnt] != "" :
            if lightgroupsTable[lgSeqTable[lightgroupCnt]].verify() == 0 :
                lightgroupsTable[lgSeqTable[lightgroupCnt]].printStruct()
                controllerXmlTree.start("Lightgroup",{})
                controllerXmlTree.start("LgAddr",{})
                controllerXmlTree.data(str(lgSeqTable[lightgroupCnt]))
                controllerXmlTree.end("LgAddr")
                controllerXmlTree.start("LgSystemName",{})
                print lgSeqTable[lightgroupCnt]
                controllerXmlTree.data(lightgroupsTable[lgSeqTable[lightgroupCnt]].getSystemName())
                controllerXmlTree.end("LgSystemName")
                controllerXmlTree.start("LgUserName",{})
                controllerXmlTree.data(lightgroupsTable[lgSeqTable[lightgroupCnt]].getUserName())
                controllerXmlTree.end("LgUserName")
                controllerXmlTree.start("LgType",{})
                controllerXmlTree.data(lightgroupsTable[lgSeqTable[lightgroupCnt]].getLightgroupType())
                controllerXmlTree.end("LgType")
                controllerXmlTree.start("LgDesc",{})
                controllerXmlTree.start("SmType",{})
                controllerXmlTree.data((lightgroupsTable[lgSeqTable[lightgroupCnt]].getLightgroupDesc()).getSmType())
                controllerXmlTree.end("SmType")
                controllerXmlTree.start("SmDimTime",{})
                controllerXmlTree.data(str((lightgroupsTable[lgSeqTable[lightgroupCnt]].getLightgroupDesc()).getSmDimTime()))
                controllerXmlTree.end("SmDimTime")
                controllerXmlTree.start("SmFlashFreq",{})
                controllerXmlTree.data(str((lightgroupsTable[lgSeqTable[lightgroupCnt]].getLightgroupDesc()).getSmFlashFreq()))
                controllerXmlTree.end("SmFlashFreq")
                controllerXmlTree.end("LgDesc")
                controllerXmlTree.end("Lightgroup")
            else :
                print "Lightgroup: " + str(lgSeqTable[lightgroupCnt]) + " not complete - skipping"
        lightgroupCnt += 1
    controllerXmlTree.end("Lightgroups")
    controllerXmlTree.end("Controller")
    controllerXmlTree = controllerXmlTree.close()
    prettyControllerXmlStr = (xml.dom.minidom.parseString(ET.tostring(controllerXmlTree))).toprettyxml()
    controllerXmlStr = ET.tostring(controllerXmlTree)
    print prettyControllerXmlStr
    print controllersTable[controllerCnt]
    lightsMQTT.controllerUpdate(controllersTable[controllerCnt], controllerXmlStr)
    controllerCnt += 1

smCallBack = SmListener()
# Delete previous listeners
for sm in masts.getNamedBeanSet() :
    try :
        sm.removePropertyChangeListener(smCallBack)
    except :
        pass
    else :
        print "removing listener for: " + str(sm)

# Start listeners
lightgroupCnt = 0
while lightgroupCnt < 10 :
    if lightgroupsTable[lightgroupCnt] != "" :
        if lightgroupsTable[lightgroupCnt].verify() == 0 :
            print "setting listener for: " + lightgroupsTable[lightgroupCnt].getSystemName()
            sm = masts.getBySystemName(lightgroupsTable[lightgroupCnt].getSystemName())
            sm.addPropertyChangeListener(smCallBack)
    lightgroupCnt += 1

print "End of script"