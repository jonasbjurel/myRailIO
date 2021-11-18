
# genericIOSatelite

A model railway automation project part of the GenericJMRIdecoder project (XXXXXXXXX)

#

# **License**

**Copyright (c) 2021**Jonas Bjurel (jonas.bjurel@hotmail.com)

Licensed under the Apache License, Version 2.0 (the &quot;License&quot;);
You may not use this file except in compliance with the License.
 You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law and agreed to in writing, softwaredistributed under the License is distributed on an &quot;AS IS&quot; BASIS,WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

# Description

## Introduction

genericIOSateliteis a model railway sensor- and actuator project part of the the GenericJMRIdecoder project providing railway model automation using the JMRI open-source project and the GenericJMRIdecoder – see XXXXXXXXX.

genericIOSatelite – AKA Satelite provide actuator- and sensor capabilities for use with model railways. It builds on an FPGA design, and comes with a PCB and an Arduino ESP32 software library.
genericIOSatelite can be connected to a ESP32 GenericJMRIdecoder for large scale sensor and actuator capabilities – E.g., for sensing train presence or maneuverings turnouts, servos or solenoids

## Overview

Several genericIOSatelite (here after called Satelites) (up to MAX\_NO\_OF\_SAT\_PER\_CH) can be daisy chained in a loop for each Satelite link. Each of the Satelites holds 4 actuator outputs, each of them individually configured through Satelite link commands (E.g., PWM, Pulses, and static low/high states). They also collect up to 8 sensor inputs - transferred to the master through the Satelite link.

Satelite instructions, control information, alignment information, CRC checksums and Padding (E.g., sensor state to be filled in by the Satelites are pushed from the link master - A.

Satelites fill in the return information (E.g., Sensor state, remote CRC error indication, CRC checksum for the new datagram generatedby the Satelite, and delivers it to the master - B.

This is a synchronous operation, and as the master pushes new data to the Satelites, it will synchronously receive Satelite data (E.g.,sensor status from all the Satelites. B will not only contain feedback information, but also residues from the previous satelliteorders - A.

### The principle

\&lt;pre\&gt;

ADDRESS N+1 ADDRESS N ADDRESS 0

Sense[0:7] Sense[0:7] Sense[0:7]

Act[0:3] ! Act[0:3] ! Act[0:3] !

^ ! ^ ! ^ !

! v ! v ! !

+-------------+ A +-+------+-+ A+B +-+------+-+ A+B +-+------+-+

! !--------/--------\&gt;! Satelite !--------/------\&gt;! Satelite !--------//---------! Satelite !--------+

! Link master ! +----------+ +----------+ N +----------+ !

! !\&lt;-------------------------------------/----------------------------------------------------------+

+-------------+ B

\&lt;/pre\&gt;

## Physical link layer

The Satelite link is based on the WS28XX defacto standard but deviates in a significant way in that the first Satelite doesn&#39;t latch on the first data on the line, instead commands traverse through the link loop in a transparent shift register fashion and gets latched by each Satelite when there is now activity (link low) for \&gt;=100 uS.

The symbol encoding is the following:

1: Link high: 600 ns, Link low: 650 ns (Cycle time 1250 ns).

0: Link high: 250 ns, Link low: 1000 ns (Cycle time 1250 ns).

The physical layer is LVLVDS (2.5 V LVDS)

## Link transport protocol

The transport protocol representation is the following (First transmitted on the link first):

**Link Mater out - A:**

| Field/Bits | Description |
| --- | --- |
| [7:0] SensorVal-PAD | Shall be set to 0 |
| [7:0] ActVal[3] | Actuator value, dependent on ActMode[3] |
| [7:0] ActVal[2] | Actuator value, dependent on ActMode[2] |
| [7:0] ActVal[1] | Actuator value, dependent on ActMode[1] |
| [7:0] ActVal[0] | Actuator value, dependent on ActMode[0] |
| [2:0] ActMode[3] | Actuator Mode:
 SATMODE\_LOW(0): logical low,
 SATMODE\_HIGH(1): logical high,SATMODE\_PWM1\_25K(2): 1.25kHz PWM controlled by
ActVal[3](%),SATMODE\_PWM100(3): 100 Hz PWM controlled by ActVal[3](%), SATMODE\_PULSE(4): Single High pulse controlled by
ActVal[3](ms),
 SATMODE\_PULSE\_INV(5): Single Low pulse controlled by
ActVal[3](ms) |
| [2:0] ActMode[2] | Actuator Mode:
 SATMODE\_LOW(0): logical low,
 SATMODE\_HIGH(1): logical high,SATMODE\_PWM1\_25K(2): 1.25kHz PWM controlled by
ActVal[2](%),SATMODE\_PWM100(3): 100 Hz PWM controlled by ActVal[2](%), SATMODE\_PULSE(4): Single High pulse controlled by
ActVal[2](ms),
 SATMODE\_PULSE\_INV(5): Single Low pulse controlled by
ActVal[2](ms) |
| [2:0] ActMode[1] | Actuator Mode:
 SATMODE\_LOW(0): logical low,
 SATMODE\_HIGH(1): logical high,SATMODE\_PWM1\_25K(2): 1.25kHz PWM controlled by
ActVal[1](%),SATMODE\_PWM100(3): 100 Hz PWM controlled by ActVal[1](%), SATMODE\_PULSE(4): Single High pulse controlled by
ActVal[1](ms),
 SATMODE\_PULSE\_INV(5): Single Low pulse controlled by
ActVal[1](ms) |
| [2:0] ActMode[0] | Actuator Mode:
 SATMODE\_LOW(0): logical low,
 SATMODE\_HIGH(1): logical high,SATMODE\_PWM1\_25K(2): 1.25kHz PWM controlled by
ActVal[0](%),SATMODE\_PWM100(3): 100 Hz PWM controlled by ActVal[0](%), SATMODE\_PULSE(4): Single High pulse controlled by
ActVal[0](ms),
 SATMODE\_PULSE\_INV(5): Single Low pulse controlled by
ActVal[0](ms) |
| cmdcmdWdErr | Generate a watchdog error in the feed-back(fb) message, this is nonintrusive and will not disable actuators. |
| cmdEnable | Enable Satelite: 1, Disable Satelite: 0, if disabled, the Satelite will set all actuators to 0 |
| cmdInvCrc | If set to 1, the Satelite will create an invalid CRC checksum towards the link master, if set to 0 correct CRC checksums will be generated. |
| startMark | Shall be set for the first Satelite datagram sent on the link (Satelite address 0), for following Satelite datagrams it shall be set to 0,this mark is set to allow each Satelite to understand where in the link-chain they sit (their address on the link), this is used for theCRC evaluation and creation. The Satelite closestd to the link master&#39;s input (furthers away from the linkmasters ws28xx output) is designated address 0. |
| fbReserv-PAD[2] | Shall be set to 0 |
| fbWdErr-PAD | Shall be set to 0 |
| fbRemoteCrcErr-PAD | Shall be set to 0 |
| [3:0]CRC | CRC-4 checksum generated over the full Satelite datagram sent from the Satelite, excluding the CRC-4 checksum it self, the CRC-4 checksum polynom is (x^4+x^1+x^0)(+)sateliteAddr[7:4](+)sateliteAddr[3:0], the reason to involve the Satelite address in the CRC checksum is to ensurethat sensor and actuator data do not originate from other Satelites than intended in case the master crashes during its output/scan of the link. |

**Link Master in - B:**

| Field/Bits | Description |
| --- | --- |
| [7:0] SensorVal | Sensor inputs, latched since previous link scan |
| [7:0] ActVal[3] | Don&#39;t care |
| [7:0] ActVal[2] | Don&#39;t care |
| [7:0] ActVal[1] | Don&#39;t care |
| [7:0] ActVal[0] | Don&#39;t care |
| [2:0] ActMode[3] | Don&#39;t care |
| [2:0] ActMode[2] | Don&#39;t care |
| [2:0] ActMode[1] | Don&#39;t care |
| [2:0] ActMode[0] | Don&#39;t care |
| cmdcmdWdErr | Don&#39;t care |
| cmdEnable | Don&#39;t care |
| cmdInvCrc | Don&#39;t care |
| startMark | Transparent - Can be checked by the link-master, should be set for the first pulled Satelite, and 0 for the remaining, otherwise a general error is at hand. |
| fbReserv[2] | Transparent - can be checked by the link-master, should be set for the first pulled Satelite, and 0 for the remaining, otherwise a general error is at hand. |
| fbWdErr | Set to 1 in case the Satelite has experienced a watchdog timeout, the watchdog timeout triggers if the Satelite has not been scannedwithin 500 ms, actuators are then set to low/inactive until the next scan performed by the master. |
| fbRemoteCrcErr | Set to 1 in case the Satelite experienced a CRC error on the Satelite datagram sent from the master. |
| [3:0]CRC | CRC-4 checksum generated over the full Satelite datagram sent from the Satelite, excluding the CRC-4 checksum it self, the CRC-4 checksum polynom is (x^4+x^1+x^0)(+)sateliteAddr[7:4](+)sateliteAddr[3:0], the reason to involve the Satelite address in the CRC checksum is to ensurethat sensor and actuator data do not originate from other Satelites than intended in case the master crashes during its output/scan of the link. |

## **FPGA Pinout:**

| I/O Name: | I/O | Lattice machxo2-1200 FPGAPin(QFP32): | TinyFPGA AX PCB Pin: |
| --- | --- | --- | --- |
| ws2811wireIn + | I | 13 (LVDS 2.5V +) | 1 (LVDS 2.5V +) |
| ws2811wireIn - | I | 14 (LVDS 2.5V -) | 2 (LVDS 2.5V -) |
| sensorInput[0] | I | 20 | 5 |
| sensorInput[1] | I | 21 | 6 |
| sensorInput[2] | I | 23 | 7 |
| sensorInput[3] | I | 25 | 8 |
| sensorInput[4] | I | 26 | 9 |
| sensorInput[5] | I | 27 | 10 |
| sensorInput[6] | I | 28 | 11 |
| sensorInput[7] | I | 12 | 22 |
| ws2811wireOut + | O | 4 (LVDS 2.5V +) | 16 (LVDS 2.5V +) |
| ws2811wireOut - | O | 5 (LVDS 2.5V -) | 17 (LVDS 2.5V -) |
| actuators[0] | O | 11 | 21 |
| actuators[1] | O | 10 | 20 |
| actuators[2] | O | 9 | 19 |
| actuators[3] | O | 8 | 18 |
| Active(Active scan indication) | O | 16 | 3 (Tiny FPGA LED) |
| Err (Error indication, Watchdog- or
CRC-Err) | O | 17 | 4 |

## **Satelite Library functionality and concepts**

### Overview and architecture

The Satelite library consists of two classes:

1. The sateliteLink class which manages the low-level Satelite link (I.e. OSI L1, L2 and L3). It isresponsible for discovery of the and topology management of Satelites sitting on the Satelite link,but also responsible for scanning the link (providing instructions and pulling results from all the Satelites on a Satelite link, responsible for the integrity of the link, and responsible for delivering raw data from/to the satelite class objects of the Satelite link. The sateliteLink class has no, or very littleknowledge around the functionality of the Satelites and their respective class-objects. The sateliteLink objects communicate with their child satelite class objects through shared memory, moreover the sateliteLink class objects create satelite class objects as part of the sateliteLink class auto-discovery process. A previously registered user call-back is called for every satelite object that is created from the auto-discovered Satelite, similarly if the sateliteLink decides to delete a satelite class object based on errors, or other blocking interventions - a call-back is sent to the user of the satelite class object.
2. The satelite class which is responsible for the higher-level functionality of each satelite (OSI L4-L7).Each satelite class object gets created through its parent sateliteLink class object after the sateliteLinkAuto discovery process, Satelite users get informed about the existence of a satelite through call-backs registeredthrough the parent sateliteLink class object.The satelite class objects have methods for setting the actuators, receiving sensor values, and supervisingthe functionality and the integrity of each satelite.

### Cardinality

This library allows thecreation of multiple Satelite Link masters - providing that available resources allow for it,E.g. RMT channels, RMT memory blocks, etc.For each Link master (sateliteLink object) a maximum of MAX\_NO\_OF\_SAT\_PER\_CH can be instantiated.

#### Cardinality view:

+--------+ 1:N +-------------+ 1:M +----------+

! master !--//--\&gt;! Link master !--//--\&gt;! Satelite !

+--------+ +-------------+ +----------+

With &quot;master&quot; is meant an entity that may run multiple instances of this library, E.g., an ESP32 runningthe genericJMRIdecoder project Software found here: XXXXXXXXX, or otherwise. This is outside the scopefor this description.

The Link master refers to the sateliteLink class referred to in this description.

The satelite refers to the satelite class and its object instantiations described in this description.

### Object (Blocking) states

The sateliteLink- and the satelite class objects both have an administrative state and an operational state.

#### Administrative state

The administrative state can be one of either DISABLED or ENABLED. The administrative state is hierarchical and is a consequence of a user ordered change of the administrative state. It is hierarchical in the sensethat the higher layer objects (I.e., sateliteLink) must be administratively enabled before the underlying objects(I.e., Satelites) can be administratively enabled, consequently a higher layer object cannot be disabled if notall underlying objects are disabled.

As long as the hierarchical rules of the administrative states are followed, an ordered change of the administrativestate is always successful.

If an object is disabled, actions, feedback, and error supervision are disabled for that object, E.g., for Satelitesthis means that the actuators are disabled, no sensor data is fed back, etc.

Sometimes we refer to administrative blocking/de-blocking, blocked/de-blocked as the administrative state changeactions and the administrative state.

#### Operational state

The operational state reflects the operational status of an object, I.e., if it functions or not, in difference tothe administrative state, the operational state is not hierarchical, and any object can change its operational stateat any time depending on its operational status (it may though affect the operational status of underlying objects).

Another difference is that the administrative state only can assume distinct states, while the operative states can have multiple states at the same time.

The operational states can have a permutation of the following states:

- &quot;WORKING&quot; - The object is fully working, cannot be combined with any other operational state attributes
- &quot;INIT&quot; - The object is being initialized, can be combined with other operational state attributes
- &quot;DISABLE&quot; - The object is disabled based on its administrative state, cannot be combined with other operational state attributes
- &quot;CTRLBLCK&quot; - The object is control blocked as a higher layer object is not functioning (E.g. Operational state != &quot;WORKING&quot;), can be combined with other operational state attributes.
- &quot;ERRSEC&quot; - The object has experienced errors that goes beyond the threshold set for one second, can be combined with other operational state attributes.
- &quot;FAIL&quot; - The object has experienced a general failure, can be combined with other operational state attributes.

As an example of the &quot;CTRLBLCK&quot; operational state attribute, if a sateliteLink experiences an error leading to an operational state != &quot;WORKING&quot;, all satellite objects belonging to that sateliteLink will assume the &quot;CTRLBLCK&quot; attribute as they cannot functionwithout their parent sateliteLink functioning.

When a sateliteLink object has been in non &quot;WORKING&quot; state for 30 seconds (T\_REESTABLISH\_LINK\_MS), it is assumed that the link topology has changed, and a re-initialization will start. This means that all its satelite objects aredeleted, and a new sateliteLink Auto discovery will be initiated (See further down on the matter of auto-discovery).

### Error supervision

As this design is to be used for various sensor input and actuator output for model railway automation, errors in input and output can lead to unpredicted behaviour, collisions, and though not with catastrophic consequences - certainly with damage of material. This implementation is by no means redundant but aims to catch all faultsthat would lead to unpredicted behaviour, through error checks avoid that inputs and outputs are subject to faultystimuli and transmission propagation. Through Cyclic RedundantChecksum of the transmitted orders- and feed-back,this implementation can detect a squelch spurious events. Furthermore, these error-detection mechanisms can be exercised in a &quot;simulated fashion&quot; such that non-intrusiveself-tests can be run - exercising and validating the error supervision logic without impacting the operation.

The following error supervision is implemented.

#### CRC Checksums

Satelite link masters transmits datagrams to each Satelite on the link with a CRC-4 checksum calculatedover each of the Satelite&#39;s datagram, hashed with each Satelites address.

As each Satelite receives the CRC-4 checksums, they validate the received CRC-4 checksum from the Link master (each individually hashed with its Satelite address) towards the Satelite&#39;s calculated datagram checksum (again hashed with it&#39;s Satelite address). If they match, the actuators are moved according to the actuator commands, if on the other hand there was a CRC error detected, they stay as they were before and the crcRemoteError bitis set towards the Link master such that the Link master has full visibility of the link performance.

Each Satelite transmits a CRC-4 checksum (hashed with its own address towards the Link master, such that the Link master can discriminate reliable sensor- and feed-back messages from those that contains errors.

The hashing of the CRC-4 checksum with each individual Satelite address (calculated from the Satelite Link&#39;s startMark) provides additional protection against misaddressed Satelites - both for actuator actions as wellas for sensor feed-back.

Whenever a Satelite receives a datagram with invalid CRC checksum, the Actuator orders and other Satelitecommands are ignored, and are left as they previously were set.Similarly, Satelite sensor data and other feed-back data are disregarded if the link-master detects a CRCerror from that particular Satelite.

#### Watchdog

Each Satelite implements a watchdog monitoring the link scanning of the Satelite. If the link has not been scanned for 500 ms, the watchdog is triggered, disabling all the actuators for that particular Satelite.When triggered, the fbWdErr bit is set such that the master gets informed about the event at the nextlink-scan.

#### Performance data collection

Performance data are collected from the link. Following performance data exist and can be retrieved through class methods:

- **Symbol error** : The master has detected a symbol error on the receiving side.
- **Rx-size error** : The master has detected that the size of a received datagram was wrong.
- **Timing violation** : The link scan could not be performed within given link scan time.
- **Rx-CRC error** : The master has received a datagram from a particular Satelite with wrong CRC-4 checksum.
- **Remote-CRC error** : A particular Satelite has received a datagram with wrong CRC-4 checksum.
- **Watchdog error** : The watchdog for a particular Satelite has been triggered.

All of the above errors are aggregated on a per sateliteLink object as well as per satelite object.If the sum of these errors exceeds a high threshold level during a second period, the respectiveobject is deemed faulty and goes into operational state &quot;ERRSEC&quot;, this operational state is ceased oncethe sum of errors during a second goes below a low threshold level.

### Auto discovery

The sateliteLink can auto discover the Satelites sitting on the link. This is done by forcing the master to generate all faulty CRC-4 checksums on the transmitting side for MAX\_NO\_OF\_SAT\_PER\_CH + 1 Satelites. Every Sateliteon the link will respond with a remote CRC error bit, for those data grams that has this bit set the masterknows that there is a corresponding Satelite, and for those datagrams that doesn&#39;t have this bit set it knowsthere isn&#39;t a a corresponding Satelite.

### Self-test

The master can perform Satelite self-tests. The self test is exercising the Satelites and master receiver&#39;s CRC-4 checksum detection, as well as the watchdog. The self-test is nonintrusive.

## **Class object methods and helper functions**

### sateliteLink methods

#### sateliteLink creation:

sateliteLink(uint8\_t address\_p, gpio\_num\_t txPin\_p, gpio\_num\_t rxPin\_p, rmt\_channel\_t txCh\_p,
 rmt\_channel\_t rxCh\_p, uint8\_t txRmtMemBank\_p, uint8\_t rxRmtMemBank\_p,
 UBaseType\_t pollTaskPrio\_p, UBaseType\_t pollTaskCore\_p, uint8\_t scanInterval\_p)

**Description:**
 Creates a new sateliteLink object.

**Parameters:**
uint8\_t address\_p: The address of the sateliteLink, an arbitrary number between 0 and 255.
gpio\_num\_t txPin\_p: The Pin number for the link TX side
gpio\_num\_t rxPin\_p: The Pin number for the link RX side
rmt\_channel\_t txCh\_p: ESP RMT TX channel
rmt\_channel\_t rxCh\_p: ESP RMT RX channel
uint8\_t txRmtMemBank\_p: ESP RMT TX memory bank
uint8\_t rxRmtMemBank\_p: ESP RMT RX memory bank
UBaseType\_t pollTaskPrio\_p: The link scan process priority (0-24 Higher is more)
UBaseType\_t pollTaskCore\_p: The link scan process core (0-1)
uint8\_t scanInterval\_p: The link scan period (1-255 ms)

**Returns:**
 The object handle

#### sateliteLink destruction

~sateliteLink(void)

**Description:**
Destroys a previously created sateliteLink object. The sateliteLink shall be disabled before calling the destructor.

**Parameters:**
-

**Returns:**
-

#### enableSatLink

satErr\_t enableSatLink(void)

**Des**** c ****ription:**
 Enables the sateliteLink object, which involves an auto discovery for Satelites, creation of satelite objects andadministrative enabling of the sateliteLink. A call-back function should have been registered through satLinkRegSatDiscoverCb in order to get the handle to newly created Satelites.

**Parameters:**
 -

**Returns:**
 satErr\_t return code

#### disableSatLink

satErr\_t disableSatLink(void)

**Description:**
 Disables the sateliteLink. This requires that all Satelites belonging to the Satelite links have been admiratively disabled. It also results in that all satelite objects belonging to the link are deleted. When those are deleted, the user will get a call-back previously registered through satLinkRegSatDiscoverCb informing about the upcoming deletion of the satellite object.

**Parameters:**
-

**Returns:**
Returns: satErr\_t return code

#### SetErrTresh:

void setErrTresh(uint16\_t p\_errThresHigh, uint16\_t p\_errThresLow)

**Description:**
Sets the error threshold for the sateliteLink &quot;ERRSEC&quot; operational state.

**Parameters:**
uint16\_t p\_errThresHigh: High threshold for going in to &quot;ERRSEC&quot; operational state
uint16\_t p\_errThresLow: Low threshold for ceasing &quot;ERRSEC&quot; operational state.

**Returns:**
-

#### satlinkRegStateCb

void satLinkRegStateCb(satLinkStateCb\_t satLinkStateCb\_p)

**Description:**
Registers a call-back which is called whenever the operational state of the sateliteLink Object is changed. **The call-back must be kept short in time and not call any blocking functions**.

**Parameters:**
satLinkStateCb\_t satLinkStateCb\_p: The pointer to the call-back function.

**Returns:**
-

**Call-back prototype:**
typedef void (\*satLinkStateCb\_t)(sateliteLink\* sateliteLink\_p, uint8\_t LinkAddr\_p,
 satOpState\_t satOpState\_p);

**Callback parameters:**
sateliteLink\* sateliteLink\_p: The sateliteLink object handle.
uint8\_t LinkAddr\_p: The sateliteLink address.
satOpState\_t satOpState\_p: The operational state of the sateliteLink.

#### satLinkUnRegStateCb

void satLinkUnRegStateCb(void)

**Description:**
Unregisters the state call-back.

**Parameters:**
 -

Returns:
 -

**satLinkRegSatDiscoverCb**

void satLinkRegSatDiscoverCb(satDiscoverCb\_t satDiscoverCb\_p)

**Description:**
Registers a call-back informing about creation and deletion of satelite objects. If exists\_p in the call-back is set to true a satelite object was created, if false - a satelite object was deleted. In case of deletion, the user must not return the call-back until allreferences to that object have been destroyed and no methods of that object is exercised, after returning the call-backthe satelite object will be deleted.

**Parameters:**
satDiscoverCb\_t satDiscoverCb\_p: The call-back function pointer.

Returns:
 -

**Call-back prototype:**
typedef void (\*satDiscoverCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 bool exists\_p);

**Call-back parameters:**
satelite\* satelite\_p: Satelite object handle.
uint8\_t LinkAddr\_p: Link address
uint8\_t SatAddr\_p: Satelite address.
bool exists\_p: If true, a satelite object has been created, if false a satelite object is about to be deleted.

#### satLinkUnRegSatDiscoverCb

void satLinkUnRegSatDiscoverCb(void);

**Description:**
 Un register of a discovery call-back.

**Parameters:**
-

**Returns:**
-

getAddress:

uint8\_t getAddress(void)

**Description:**
Returns the address of the sateliteLink object.

**Parameters:**
-

**Returns:**
uint8\_t: sateliteLink address.

#### getSatLinkNoOfSats

uint8\_t getSatLinkNoOfSats(void)

**Description:**
Returns the number of Satelites discovered on the link. The addresses of those are contiguous and starts from 0 (the Satelite closestt to the master&#39;s RX)

// Parameters:
 -

**Returns:**
 uint8\_t Number of Satelites discovered on the link.

#### getSatStats

void getSatStats(satPerformanceCounters\_t\* satStats\_p, bool resetStats)

**Description:**
Provides the performance counters for the sateliteLink object.

**Parameters:**
satPerformanceCounters\_t\* satStats\_p: A pointer to a performance counter object.
bool resetStats: If true, the performance counters are reset.

**Returns:
 -**

#### clearSatStats

void clearSatStats(void)

**Description:**
Clears the PM counters for the sateliteLink object.

Parameters:
**-**

Returns:
 -

#### getsatHandle

satelite\* getsatHandle(uint8\_t satAddr\_p)

**Description:**
Returns the satelite object handle for a given Satelite address. If the Satelite address does not exist NULL is returned.

**Parameters:**
uint8\_t satAddr\_p: Satelite address

**Returns:**
satelite\* satelite handle.

#### admBlock

satErr\_t admBlock(void)

**Description:**
Administratively disables (blocks) the sateliteLink, this requires that all satelite objects belonging to this link alsoare administratively blocked. In difference to satLinkDisable, this method does not delete the related satellite objects.

**Parameters:**
 -

**Returns:**

satErr\_t Return code

#### admDeBlock

satErr\_t admDeBlock(void)

**Description:**

Administratively enables (de-blocks) the sateliteLink. In difference to satLinkEnable, this method does not perform any auto discovery, nor does it create any satellite objects.

**Parameters:**
-

**Returns:**
satErr\_t Return code

#### getAdmState

satAdmState\_t getAdmState(void)

**Description:**
Provides the administrative state of the sateliteLink.

**Parameters:**
-

**Returns:**
satAdmState\_t Administrative state.

#### getOpState:

satOpState\_t getOpState(void)

**Description:**
Provides the operative state of the sateliteLink.

**Parameters:**
-

**Returns:**
satAdmState\_t Administrative state.

### **satelite methods**

#### satErr\_t enableSat

satErr\_t enableSat(void)

**Description:**
Enables a satelite, I.e., administratively enables it. This requires that the parent sateliteLink object is enabled.

**Parameters:**
 -

**Returns:**
satErr\_t Return code.

#### satErr\_t disableSat

satErr\_t disableSat(void)

**Description:**
Disables a satelite, I.e., administratively disables it.

**Parameters:**
 -

**Returns:**
satErr\_t Return code.

#### setErrTresh

void setErrTresh(uint16\_t errThresHigh\_p, uint16\_t errThresLow\_p)

**Description:**
Sets the error threshold for the satelite &quot;ERRSEC&quot; operational state.

**Parameters:**
uint16\_t p\_errThresHigh: High threshold for going in to &quot;ERRSEC&quot; operational state
uint16\_t p\_errThresLow: Low threshold for ceasing &quot;ERRSEC&quot; operational state.

**Returns:**
satErr\_t Return code.

#### setSatActMode

satErr\_t setSatActMode(actMode\_t actMode\_p, uint8\_t actIndex\_p)

**Description:**
Sets the actuator mode for a satelite:
 0: SATMODE\_LOW - logical low
1: SATMODE\_HIGH - logical high
2: SATMODE\_PWM1\_25K - 1.25kHz PWM
3: SATMODE\_PWM100 - 100 Hz PWM
4: SATMODE\_PULSE - Single High pulse
5 SATMODE\_PULSE\_INV: Single Low pulse

**Parameters:**
actMode\_t actMode\_p: Actuator mode
 uint8\_t actIndex\_p: Actuator index (0-3)

#### setSatActVal

satErr\_t setSatActVal(uint8\_t actVal\_p, uint8\_t actIndex\_p)

**Description:**
 Sets the actuator value for an actuator on a Satelite – [%] for PWM mode, [ms] for pulse mode.

**Parameters:**
uint8\_t actVal\_p: Actuator value (0-255) [%]/[ms]
 uint8\_t actIndex\_p: Actuator index (0-3)

**Returns:**
satErr\_t Return code.

#### setSenseFilter

satErr\_t setSenseFilter(uint16\_t senseFilter\_p, uint8\_t senseIndex\_p)

**Description:**
 Set the filter time for the sensors.

**Parameters:**
uint16\_t senseFilter\_p: Sensor filter time (1-255) [ms].
uint8\_t senseIndex\_p: Sensor index (0-7).

**Returns:**
satErr\_t Return code.

#### getSatStats

void getSatStats(satPerformanceCounters\_t\* satStats\_p, bool resetStats)

**Description:**
Provides the performance counters for the satelite object.

**Parameters:**
satPerformanceCounters\_t\* satStats\_p: A pointer to a performance counter object.
bool resetStats: If true, the performance counters are reset.

**Returns:
 -**

#### clearSatStats

void clearSatStats(void)

**Description:**
Clears the PM counters for the satelite object.

Parameters:
**-**

Returns:
 -

#### satRegSenseCb

void satRegSenseCb(satSenseCb\_t fn)

**Description:**
Register sensor callback

**Parameters:**
satSenseCb\_t fn: Pointer to call-back function.

**Returns:
 -**

**Call-back prototype:**
typedef void (\*satSenseCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 uint8\_t senseAddr\_p, bool senseVal\_p);

**Call-back p**** arameters ****:**
 satelite\* satelite\_p: Satelite handle.
 uint8\_t LinkAddr\_p: Satelite link address.
 uint8\_t SatAddr\_p: Satelite address.
 uint8\_t senseAddr\_p: Sensor address/index.
 bool senseVal\_p: Sensor state

#### satUnRegSenseCb

void satUnRegSenseCb(void)

**Description:**
Unregister sensor call-back.

**Parameters:**
-

**Returns:**
 -

#### satSelfTest

satErr\_t satSelfTest(selfTestCb\_t selfTestCb\_p)

**Description:**
Initiate a self-test of a Satelite. The result will be delivered through the call-back provided as parameter.

**Parameters:**
selfTestCb\_t selfTestCb\_p: Pointer to result call-back.

**Returns:**
satErr\_t Return code.

**Call-back prototype:**
typedef void (\*selfTestCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 satErr\_t selftestErr\_p);

**Call-back p**** arameters ****:**
satelite\* satelite\_p: Satelite handle.
 uint8\_t LinkAddr\_p: Satelite link address.
 uint8\_t SatAddr\_p: Satelite address.
 satErr\_t selftestErr\_p: Self-test result.

_getSenseVal_

bool getSenseVal(uint8\_t senseAddr)

**Description:**
 Returns a sensor value.

**Parameters:**
uint8\_t senseAddr: Sensor address/index.

**Returns:**
bool Sensor value.

#### satRegStateCb

void satRegStateCb(satStateCb\_t fn)

**Description:**
 Register a call-back for operational state updates.

**Parameters:**
satStateCb\_t fn: Pointer to call-back function.

**Call-back prototype:**
typedef void (\*satStateCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 satOpState\_t satOpState);

**Call-back**  **parameters**** :**
satelite\* satelite\_p: satelite handle
 uint8\_t LinkAddr\_p: Link address
 uint8\_t SatAddr\_p: Satelite address.
 satOpState\_t satOpState: Operational state

#### satUnRegStateCb

void satUnRegStateCb(void)

**Description:**
 Unregister operational state call-back.

**Parameters:**
-

**Returns:**
-

#### getAddress

uint8\_t getAddress(void)

**Description:**
Returns the address of a Satelite

**Parameters:**
-

**Returns:**
uint8\_t Satelite address.

#### satErr\_t admBlock

satErr\_t admBlock(void)

**Description:**
Administratively disables (blocks) the satelite, this is method is equivalent with satDisable.

**Parameters:**
 -

**Returns:**

satErr\_t Return code

#### satErr\_t admDeBlock

satErr\_t admDeBlock(void)

**Description:**
Administratively enables(de-blocks) the satelite, this is method is equivalent with satEnable. It requires that the parent sateliteLink is administratively enabled.

**Parameters:**
 -

**Returns:**

satErr\_t Return code

#### satAdmState\_t getAdmState

satAdmState\_t getAdmState(void)

**Description:**
Returns the administrative state of a satelite.

**Parameters:**
 -

**Returns:**
satAdmState\_t Administrative state.

####

#### getOpState

satOpState\_t getOpState(void)

**Description:**
Returns the operational state of a satelite.

**Parameters:**
 -

**Returns:**
satOpState\_t Operational state.

### **Helper functions**

satErr\_t opStateToStr(satOpState\_t opState\_p, char\* outputStr\_p, uint8\_t length\_p)
 satErr\_t formatSatStat(char\* reportBuffer\_p, uint16\_t buffSize\_p, uint16\_t\* usedBuff\_p,
 uint16\_t buffOffset\_p, uint8\_t linkAddr\_p, uint8\_t satAddr\_p,
 satAdmState\_t admState\_p, satOpState\_t opState\_p,
 satPerformanceCounters\_t\* pmdata\_p, uint16\_t reportColumnItems,
 uint16\_t reportItemsMask, bool printHead)

####

#### Performance counter format directives

#define LINK\_ADDR
 #define SAT\_ADDR
 #define RX\_SIZE\_ERR
 #define RX\_SYMB\_ERR
 #define TIMING\_VIOLATION\_ERR
 #define TX\_UNDERRUN\_ERR
 #define RX\_OVERRRUN\_ERR
 #define RX\_CRC\_ERR
 #define REMOTE\_CRC\_ERR
 #define WATCHDG\_ERR
 #define ADM\_STATE
 #define OP\_STATE

###

### **Types, definitions and Macros**

#### Return codes

**Return code type definition:**
typedef uint64\_t satErr\_t;

**Description:**
The lower 8 bits defines the local return code produced by the Satelite library – se below:
 #define SAT\_OK 0x00
 #define SAT\_ERR\_SYMBOL\_ERR 0x01
 #define SAT\_ERR\_EXESSIVE\_SATS\_ERR 0x02
 #define SAT\_ERR\_GEN\_SATLINK\_ERR 0x03
 #define SAT\_ERR\_WRONG\_STATE\_ERR 0x04
 #define SAT\_ERR\_DEP\_BLOCK\_STATUS\_ERR 0x05
 #define SAT\_ERR\_PARAM\_ERR 0x06
 #define SAT\_ERR\_RMT\_ERR 0x07
 #define SAT\_ERR\_EXESSIVE\_SATS 0x08
 #define SAT\_ERR\_SCANTASK\_ERR 0x09
 #define SAT\_ERR\_NOT\_EXIST\_ERR 0x0A
 #define SAT\_ERR\_BUFF\_SMALL\_ERR 0x0B
 #define SAT\_ERR\_BUSY\_ERR 0x0C
 #define SAT\_SELFTEST\_SERVER\_CRC\_ERR 0x0D
 #define SAT\_SELFTEST\_CLIENT\_CRC\_ERR 0x0E
 #define SAT\_SELFTEST\_WD\_ERR 0x0F

satErr\_t[63:8] Can from time to time provide return codes coming from other libraries called
 by this library.

####

#### State definitions

**Administrative state:**
typedef uint8\_t satAdmState\_t;
 #define SAT\_ADM\_ENABLE 0x00
 #define SAT\_ADM\_DISABLE

**Operative state:**
Operative states are built up with bitmaps so that several operational states attributes can
 uniquely be overlayed on top of each other, E.g. SAT\_OP\_DISABLE |SAT\_OP\_CONTROLBOCK.
The opStateToStr helper function will provide an operational state clear text string corresponding
 to the current operational state of an object, no matter if it is of class sateliteLink or satelite.

uint8\_t satOpState\_t;
 #define SAT\_OP\_WORKING 0x0000
 #define SAT\_OP\_INIT
 #define SAT\_OP\_DISABLE
 #define SAT\_OP\_CONTROLBOCK
 #define SAT\_OP\_ERR\_SEC
 #define SAT\_OP\_FAIL

####

#### Satelite actuator modes

#define SATMODE\_LOW
 #define SATMODE\_HIGH
#define SATMODE\_PWM1\_25K
#define SATMODE\_PWM100
#define SATMODE\_PULSE
#define SATMODE\_PULSE\_INV

#### Performance counters

struct satPerformanceCounters\_t {
 uint32\_t txUnderunErr; // Not implemented/not applicable
 uint32\_t txUnderunErrSec; // Not implemented/not applicable
 uint32\_t rxOverRunErr; // Not implemented/same as scanTimingViolationErr
 uint32\_t rxOverRunErrSec; // Not implemented/same as scanTimingViolationErrSec
 uint32\_t scanTimingViolationErr; // Only applicable for sateliteLink
 uint16\_t scanTimingViolationErrSec; // For internal library use only
 uint32\_t rxDataSizeErr; // Only applicable for sateliteLink
 uint32\_t rxDataSizeErrSec; // For internal library use only
 uint32\_t rxSymbolErr; // Only applicable for sateliteLink
 uint32\_t rxSymbolErrSec; // For internal library use only
 uint32\_t wdErr; // Applicable for both sateliteLink and satelite
 uint32\_t wdErrSec; // For internal library use only
 uint32\_t rxCrcErr; // Applicable for both sateliteLink and satelite
 uint32\_t rxCrcErrSec; // For internal library use only
 uint32\_t remoteCrcErr; // Applicable for both sateliteLink and satelite
 uint32\_t remoteCrcErrSec; // For internal library use only
 uint32\_t testRemoteCrcErr; // For internal library use only
 uint32\_t testRxCrcErr; // For internal library use only
 uint32\_t testWdErr; // For internal library use only

};

#### Call-back prototypes

typedef void (\*satLinkStateCb\_t)(sateliteLink\* sateliteLink\_p, uint8\_t LinkAddr\_p,
 satOpState\_t satOpState\_p);
 typedef void (\*satStateCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 satOpState\_t satOpState);
 typedef void (\*satSenseCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 uint8\_t senseAddr\_p, bool senseVal\_p);
 typedef void (\*satDiscoverCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 bool exists\_p);
 typedef void (\*selfTestCb\_t)(satelite\* satelite\_p, uint8\_t LinkAddr\_p, uint8\_t SatAddr\_p,
 satErr\_t selftestErr\_p);