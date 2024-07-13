-----------------------------------------------------------------------
MyRailIO<sup>©</sup>
-----------------------------------------------------------------------

MyRailIO<sup>©</sup> Readme

A model railway controller backend for signal masts-, sensors-, light
effects-, actuators and more.

# License and copyright
MyRailIO<sup>©</sup> is licensed under the Apache version 2.0 license (ASLv2),

This document is licensed under the Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International License (CC
BY-NC-SA 4.0). This means that you are free to copy and redistribute the
material in any medium or format, and remix, transform, and build upon
the material, as long as you give appropriate credit to the original
author, use the material for non-commercial purposes only, and
distribute your modifications under the same license as the original.

MyRailIO<sup>©</sup> is a trademark of Jonas Bjurel. All rights reserved. The
MyRailIO logo is a copy right design of Jonas Bjurel and may not be used
outside of the MyRailIO project without permission.

# Resources

All MyRailIO resources can be found at <https://www.myrail.io/>

The MyRailIO source code can be found at
<https://github.com/jonasbjurel/myRailIO>

# Introduction

MyRailIO (maɪreɪlio) provides an Input/Output peripheral backend to
model railway controllers such as JMRI, RockRail, etc. (at current only
JMRI is supported).

MyRailIO provides flexible and configurable capabilities for the model
railway controller to throughout the layout capture diverse types of
sensors-, control multiple types of signaling masts-, control diverse
types of light-effects-, and maneuver actuators such as turnouts-,
servos-, and solenoids.

The myRailIO decoders run on multiple cheap ESP32 micro controllers,
each connected to multiple cheap I/O satellite FPGAs; All managed and
supervised by a centralized management software which in principle is OS
independent (currently only evaluated with Windows 11). The MyRailIO
management software provides a Graphical User Interface for overall
MyRailIO configuration-, status-, alarm-, performance metrics-, and
log/event overview. Apart from a view of the current state of these
metrics, the history of these metrics is stored and can be viewed with
associated time-stamps for later trouble-shooting and debug post-mortem.

The design is entirely open source and licensed under the Apache version
2.0 license (ASLv2), contribution of features-, code-, CI/CD-, testing-,
trouble reporting and bugfixes are highly appreciated.

# Features

MyRailIO features a scalable and distributed architecture for model
train control peripherals such as:

-   Signal masts (any signal system supported by the Train controller)

-   General Light groups where several (tri-colored) pixels work
    together creating various light effects.

-   Sensors (currently only digital sensors)

-   Actuators (on/off, pulse, solenoid, pwm)

Special attention has been paid to scalability and extensibility.

MyRailIO can easily be extended with new Light group effects such as
multiple road work lights playing in concert, or simulation of
Television ambient flicker, etc. Although it currently only integrates
with the JMRI Train controller it should be straight forward to
integrate with other Train controllers. Scalability is achieved by
adding more MyRailIO decoders and satellites, all managed by one central
MyRailIO server.

Although not redundant, the goal is to fail fast and reliably apply
fail-safe measures avoiding unsafe track movements and keeping the
safety as a center pillow.

Moreover, MyRailIO supports the following features:

-   A decoder CLI for debugging.

-   NTP time synchronization.

-   RSysLog for log aggregation and log rotation.

-   Statistics and performance metrics.

-   Alarms and alarm lists.

# High-level architecture

The architecture depicted below shows how myRailIO is composed of the
central management server, one or more MyRailIO decoders, each of them
connected to zero or more I/O satellites. These components interact with
the Train controller which is responsible to issue Signal mast, light
and actuator orders, as well as consuming sensor data. The main
communication procedures across these components are via MQTT -- a light
weight pub/sub bus, however Remote Procedure Calls (RPC) are used
in-between the MyRailIO management server and the Train controller.

![](documentation/userGuide/rendered/images/media/image3.svg)

## MyRailIO Server

The MyRailIO (management) server is responsible for the configuration
and management of the underlying decoders. It is also the main
integration point with various model railway controllers. It
communicates with the Train controller through Remote Procedure Calls
(RPC) to exchange status and configure the controller to directly
communicate with the MyRailIO decoders. The RPC server is a piece of
software that implements a set of management and configuration methods
defined by the RPC client; these methods are used to exchange
configuration and status information. The RPC server runs in the context
of the Train controller, and in the JMRI case it is implemented as a
JMRI Jython script.

To ensure low latency and high robustness the actual signal mast-,
light-, actuator-, and sensor information is communicated directly
between the Train controller and the MyRailIO decoders using MQTT and
never via the MyRailIO server.

Another important aspect of the MyRailIO architecture is to maintain
separation of concerns between MyRailIO and the Train controller such
that it is possible to integrate with other Train controllers without
impacting the core and protocols of MyRailIO, this is done by
concentrating the adaptations in the RPC server alone, acting as a shim
layer.

The MyRailIO server is also responsible for configuring all the managed
objects of the decoder and otherwise, it provides a Graphical User
Interface (GUI) for all of the configurations and pushes the
configuration to all of its decoders. It also keeps track of the status
for all the managed objects, provides an alarm list for the managed
objects, and aggregates all the logs from the decoders.

The MyRailIO server and the related RPC components are all implemented
in Python and are in principle platform independent.

## MyRailIO Decoders and Satellites

One or several MyRailIO decoders can be connected to a MyRailIO server
using MQTT/IP/WiFi. The decoder implements all necessary supervision-
and business logic to control signal mast aspects, lights and light
effects, actuators; as well as delivering sensor data in a reliable and
effective way.

The MyRailIO decoder runs on cheep ESP32 micro-controllers and is
implemented in C++.

The following concepts are fundamental for the understanding and
operation of MyRailIO and the MyRailIO decoder.

### Light group and Light group link

A Light group is a set of LEDs or multi-color pixels that sits on a
Light group link and together forms some sort of managed object.
Example: a signal mast with one or multiple signal lights, a set of road
work warning lights with synchronized aspects, a multi-color pixel
emulating a television ambient flicker, etc. The decoder does not itself
have the notion of light group aspects or pixel effects, the mapping
between an aspect name and the actual behavior comes as configuration
data from the server. In that way, many types of Light groups can be
defined and share the same Light group link, and the signaling system
used (Swedish, German, US, ...) is just a matter of configuration.
Although there is a limit to how many pixels the Light group link can
carry, a set (more than one) can sit on the same Light group link.

A Light group links is a serial link based on the WS2811/WS2812 protocol
on which the light groups sit. Up to two Light group links can be
connected to a MyRailIO decoder.

### Sensors, Actuators, Satellite and Satellite links

Many sensors and actuators can be connected to MyRailIO. These are
connected to MyRailIO via so-called Satellites. A sensor is currently a
binary sensor (analogue sensors are on the to-do list), up to 8 sensors
can be connected to a MyRailIO satelite. Up to 4 actuators can be
connected to a MyRailIO Satellite. These actuator ports can be
configured for various purposes such as: on/off, pulse, solenoid, servo,
pwm, etc.

As mentioned above, the MyRailIO satelite implements the sensor and
actuator ports. One or more Satellites can be connected to a Satelite
link connected to the decoder. The Satellite link implements a ring
topology using a proprietary serial protocol where Satelite orders,
meta-data, and fault detection checksums are shifted out from the
MyRailIO decoder, while sensor-, and fault data is retrieved by the
decoder in the other end of the SateliteLink. The Satellites monitors
that there are regular Satelite link polls, and if that is not the case
-- watchdog errors will be reported back to the decoder. With this
implementation, the Satelite link can be monitored for transmit- and
receive errors, watchdog errors, etc. Similarly, each satelite can be
monitored for transmit-, receive and watchdog errors that it has been
impacted by. In case a Satelite experiences a receive error the actuator
state will remain the same as for the previous poll interval.

A MyRailIO decoder can connect up to two Satelite links.

The MyRailIO Satellites are implemented with a cheap FPGA
implementation, see the open-source project here
[jonasbjurel/genericIOSatellite: A model railway stackable and large
scale sensor and actuator framework
(github.com)](https://github.com/jonasbjurel/genericIOSatellite)

## MyRailIO managed class/object model

MyRailIO implements a hierarchical managed class model where each class
implements business-, supervision-, and management logic for a certain
resource type. Each managed class gets instantiated in one or more
managed objects with a certain configuration, operating and supervising
a particular resource instance (E.g., a sensor).

The following managed class models are defined: topDecoder, decoder,
lightGroupLink, lightGroup, sateliteLink, satelite, actuator, and
sensor.

![](documentation/userGuide/rendered/images/media/image5.svg)

The managed Object Model is hierarchical with the cardinality as shown
above. Each managed object holds its own configuration with its own GUI
and CLI context. All of the managed objects except the topDecoder carry
an object system name, an object username and an object description
collectively later referred to as "object identification". The system
name is a globally unique name which cannot be altered once configured,
the object username is a lazy descriptive name which can be altered at
any time, just like the object description. For the managed class
objects which the Train controller need to be aware of - the system
names need to correspond between MyRailIO and the Train controller and
need to adhere to the Train controller's system name conventions, those
managed class objects are: "lightGroup", "actuator", and "sensor"; the
Train controller is entirely unaware of the rest of the managed object
classes.

### topDecoder

The topDecoder management object is a singleton, unlike all the other
managed objects it does not manage any physical resources, but rather
holds configurations common to the entire MyRailIO setup, such as NTP-,
RSysLog-, MQTT-, and RPC configurations and it is responsible for the
server communication.

### decoder

The decoder managed object manages the configuration- and operations of
a MyRailIO decoder. It holds information such as the decoder identity
(MAC Address)-, object identification-, administrative blocking state-,
operational state-, and various performance metrics of the decoder.

### lightGroupLink

The lightGroupLink managed object manages the configuration- and
operations of a MyRailIO lightGroupLink. It holds information such as
the object identification-, link number-, administrative blocking
state-, operational state-, and various performance metrics of the
lightGroupLink.

### lightGroup

The lightGroup managed object is (like actuator and sensor) a bit
different than the others. It is a "stem object" which has no DNA and is
unaware of the actual type of Light group it will eventually represent.
At configuration of the Light group, it will in run-time inherit a
"lightGroupType" DNA class object which will implement the actual
functionality of the light-group. Examples of a lightGroupType is
"signalMast" -- more about this later.

The lightGroup managed object manages the configuration- and operations
of a MyRailIO lightGroup. It holds information such as the object
identification-, lightGroup address, administrative blocking state-,
operational state-, current aspect, and various performance metrics of
the lightGroup.

### satelliteLink

The satelliteLink managed object manages the configuration- and
operations of a MyRailIO satelliteLink. It holds information such as the
object identification-, link number-, administrative blocking state-,
operational state-, and various performance metrics of the sateliteLink.

### satellite

The satellite managed object manages the configuration- and operations
of a MyRailIO satellite. It holds information such as the object
identification-, satellite address-, administrative blocking state-,
operational state-, and various performance metrics of the satelite.

### actuator

The actuator managed object is (like the lightgroup and sensor) a bit
different than the others. It is a "stem object" which has no DNA and is
unaware of the actual type of Actuator it will eventually represent. At
configuration of the Actuator, it will in run-time inherit a
"actuatorType" DNA class object which will implement the actual
functionality of the actuator. Examples of actuatorTypes are "actMem"-,
"actLight"-, and "actTurn" -- more about this later.

The actuator managed object manages the configuration- and operations of
a MyRailIO actuator. It holds information such as the object
identification-, actuator port, administrative blocking state-,
operational state-, current position, and various performance metrics of
the actuator.

### sensor

The sensor's managed object is (like lightgroup and actuator) a bit
different than the others. It is a "stem object" which has no DNA and is
unaware of the actual type of Sensor it will eventually represent. At
configuration of the Sensor, it will in run-time inherit a "sensorType"
DNA class object which will implement the actual functionality of the
sensor. An example of an "sensorType" is "sensDigital" -- more about
this later.

The actuator's managed object manages the configuration- and operations
of a MyRailIO actuator. It holds information such as the object
identification-, actuator port, administrative blocking state-,
operational state-, current position, and various performance metrics of
the actuator.

## MyRailIO administrative-, and operational states

MyRailIO uses the concept of "administrative states" and "operational
states". Each managed object has an "administrative state" and
"operational state".

The administrative state of a managed object is a result of a manual
intervention -- "Enabling" or "Disabling" a managed object. By
"Disabling" a managed object it is no longer operational, but in
maintenance mode - such that reconfiguration of the managed object is
allowed. Any reconfiguration during a "Disabled" maintenance period is
never propagated to the actual managed object resource until the managed
object is "Enabled" through manual intervention. A managed object cannot
be reconfigured when "Enabled". To "Disable" a managed object requires
that all subsequent hierarchic managed objects be "Disabled" -- and thus
out of operation. E.g., disabling a Satelite managed objects requires
that all its actuators-, and sensors managed objects are "Disabled" in
forehand. A "Disabled" managed object triggers the "DABL" (Disabled)
operational state bit to be set for that managed object -- see
operational states below.

The operational state of a managed object reflects the functional/error
state of that managed object. Operational states propagate down into all
subsequent hierarchical management objects such that if the parent
managed object of a managed object has any of its operational state bits
set, the "CBL" (Control Blocked) operational state bit is set -- which
will trickle further down in the managed object hierarchy. Operational
state bits are set as a consequence of managed object initialization-,
managed object-, and managed object parent faults. Some examples of
operational state bits (more than one can be set at the same time):

-   "INIT": The managed object is initializing.

-   "DISC": The managed object is disconnected (I.e., from WiFi-, MQTT-,
    or RPC).

-   \"NOIP\": The managed object has not been assigned an IP-address.

-   "UDISC": The managed object has not been discovered.

-   "UCONF": The managed object has not been configured.

-   "DABL": The managed object has been administratively "Disabled" --
    see above.

-   \"SUAVL\": The MyRailIO server is missing excessive ping supervision
    messages from a decoder.

-   \"CUAVL\": A MyRailIO decoder is missing excessive ping supervision
    messages from the server.

-   "ESEC": A Satelite link or a Satelite has experienced a second with
    extensive errors.

-   "ERR": The managed object has experienced a recoverable error.

-   "FAIL": The managed object has experienced an unrecoverable error.

-   "CBL": The managed object is control blocked due to errors higher up
    in the managed object hierarchy.

-   "UUSED": The managed object is unused.

Whenever a managed object has one of its operational status bits set it
is considered non-operational/un-safe, and consequently any physical
resources related to the managed object are set in a fail-safe mode -
preventing any consequent unsafe train movements.

## MyRailIO alarms

MyRailIO uses the concept of alarms. An alarm is a stateful event
indicating a malfunction, an alarm is raised whenever a malfunction
appears, and is ceased when the malfunction situation disappears. Alarms
as such do not declare a managed object down, and do not trigger any
fail-safe operations -- on the contrary alarms may be the result of a
managed object operational state transition that has triggered a
failsafe operation, but an alarm does not by itself. Furthermore, alarms
do not necessarily relate to managed objects, but may be connected to
any logic failures that have a stateful property (the failure can come
and go).

Alarms comes with three different priorities:

-   "A": A Critical alarm indicating that certain MyRailIO services may
    be inoperative all together.

-   "B": A Non-critical alarm indicating that certain MyRailIO services
    maybe degraded in its operations.

-   "C": A notice alarm indicating that certain MyRailIO services may be
    limited in their functionality, but in no way impacting the
    operations.

MyRailIO alarms are captured and stored in an alarm-list, the alarm-list
captures active alarms, as well as all historical inactive/ceased alarms
with: Alarm Id, Severity, Raise-time, Cease-time, duration, Alarm-type,
Alarm-source, Alarm-slogan, Alarm-raise reason, and Alarm-cease reason.

At current, the alarm-list is volatile and resets after each MyRailIO
restart.

## MyRailIO resiliency, reliability, and track safety

MyRailIO is its nature inherently non redundant, a failure in one of its
components caused by unrecoverable software-, communication-, or
configuration errors will lead to disruption of the MyRailIO operations
-- and thereby inability to set signal mast aspects, actuators, and
detect sensor inputs. The strategy to manage such faults is to "fail
fast", and in these failure scenarios have several layers of software
error exemption handlers that tries to apply the proper fail-safe
mechanisms including:

-   Setting Signal masts, Turnouts and Actuators in a failsafe position.

-   Setting the sensor values reported to the Train controller in a
    failsafe position.

-   Optionally turning of the track power.

To early detect errors, MyRailIO implements several layers of
supervision and fault detection mechanisms:

-   Each process-, task-, and poll-loop is supervised by watch dogs.

-   All communication paths are supervised by keep-alive supervision
    messages.

-   All communication have checksums appended.

# System requirements, dependencies, and compatibility

**MyRailIO Server:**

-   Windows 10 or later

-   Python 3

**MyRailIO Decoder:**

-   Esspresif ESP32 WROVER (I)E, 4MB PSRAM, 4MB FLASH

**Train controller:**

-   JMRI 4.24 (Uplift is planned)

# Installation

TBD
