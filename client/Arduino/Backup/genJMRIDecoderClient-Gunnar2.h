#ifndef GENJMRI_H
#define GENJMRI_H

#include <MqttClient.h>
#include <ArduinoLog.h>
//#include <C:\Users\jonas\AppData\Local\Arduino15\packages\esp32\hardware\esp32\1.0.6\libraries\WiFi\src\WiFi.h>
#include "WiFi.h"
#include "esp_wps.h"
#include "esp_timer.h"
#include <PubSubClient.h>
#include <QList.h>
#include <tinyxml2.h>
#include <esp_task_wdt.h>
#include <Adafruit_NeoPixel.h>

class wdt;
class networking;
class mqtt;
class topDecoder;
class lgsDecoder;
class signalMastAspects;
class flash;
class mastDecoder;



//Pinout
#define LEDSTRIP_CH0_PIN                35
#define LEDSTRIP_CH1_PIN                36
#define LEDSTRIP_CH2_PIN                37
#define LEDSTRIP_CH3_PIN                38
#define WPS_PIN                         34

//Operational states
#define OP_CREATED                      0
#define OP_INIT                         1   //Object created/initialized (Mandatory for non static objects)
#define OP_CONFIG                       2   //Object Configured - but not started)
#define OP_WORKING                      3   //Object Started and working
#define OP_CONNECTED                    4
#define OP_FAIL                         5   //Object Failed - trying to recover
#define OP_DISABLE                      6   //Object Disabled from an external actor - waiting for someone to enable it

//Debug level
#define DEBUG_VERBOSE                   4
#define DEBUG_TERSE                     3
#define INFO                            2
#define ERROR                           1
#define PANIC                           0

//Top decoder parameters
#define DEFAULT_RSYSLOGRECEIVER         "syslog.jmri.local"
#define DEFAULT_LOGLEVEL                INFO
#define DEFAULT_NTPSERVER               "pool.ntp.org"
#define DEFAULT_TIMEZONE                0
#define DEFAULT_PINGPERIOD              1

//Signal mast params
#define MAXSTRIPLEN                     255
#define SM_DIM_SLOW                     150 // Dim time in ms (0-65535)      
#define SM_DIM_NORMAL                   75
#define SM_DIM_FAST                     40
#define SM_FLASH_TYPE_SLOW              0
#define SM_FLASH_TYPE_NORMAL            1
#define SM_FLASH_TYPE_FAST              2
#define SM_FLASH_TIME_SLOW              0.5 // Signal mast flash frequency (Hz)
#define SM_FLASH_TIME_NORMAL            1
#define SM_FLASH_TIME_FAST              1.5
#define SM_BRIGHNESS_FAIL               120 // Brighness for a fully saturated mast head (0-255)
#define SM_BRIGHNESS_HIGH               80
#define SM_BRIGHNESS_NORMAL             40
#define SM_BRIGHNESS_LOW                20

// MQTT Params
#define MQTT_BUFF_SIZE                  16384
#define MQTT_CLIENT_ID                  "JMRI generic decoder"
#define MQTT_KEEP_ALIVE                 10
#define MAX_MQTT_CONNECT_ATTEMPTS_100MS 100
#define DEFAULT_MQTT_PINGPERIOD         10
#define QOS_0                           0     
#define QOS_1                           1

// MQTT Topics
#define MQTT_DISCOVERY_REQUEST_TOPIC  "/trains/track/discoveryreq/"
#define MQTT_DISCOVERY_RESPONSE_TOPIC "/trains/track/discoveryres/"
#define MQTT_PING_UPSTREAM_TOPIC      "/trains/track/decoderSupervision/upstream/"
#define MQTT_PING_DOWNSTREAM_TOPIC    "/trains/track/decoderSupervision/downstream/"
#define MQTT_CONFIG_TOPIC             "/trains/track/decoderMgmt/"
#define MQTT_OPSTATE_TOPIC            "/trains/track/opState/"
#define MQTT_LOG_TOPIC                "/trains/track/log/"
#define MQTT_ASPECT_TOPIC             "/trains/track/lightgroups/lightgroup/"

// MQTT Payloads
#define DISCOVERY_REQ                 "<DISCOVERY_REQUEST/>"                                  //Decoder Discovey request payload
#define DECODER_UP                    "<OPState>onLine</OPState>"                             //Decoder On-line message payload
#define DECODER_DOWN                  "<OPState>offLine</OPState>"                            //Decoder Off-line message payload
#define PING                          "<Ping/>"                                               //MQTT Ping payload

// Threading
#define CORE_0                        0                                                       //Core 0 used for scheduling tasks, shared with ESP32 infrastructure real time tasks (Eg WIFI)
#define CORE_1                        1                                                       //Core 1 used for scheduling tasks, purely for applications

// Common internal defininitions
// Return Codes
#define RC_OK                         0                                                       //Do not change, 0 should always be success
#define RC_GEN_ERR                    255                                                     //Do not change, 255 should always be unspecified error 
#define RC_OUT_OF_MEM_ERR             1                                                       //No more memory
#define RC_PARSE_ERR                  2                                                       //Could not the parse eg XML string
#define RC_NOTIMPLEMENTED_ERR         3                                                       //Called method not implemented

//Other Params
#define STRIP_UPDATE_MS               5                                                       //Lightgoups 2811 strip update periond [ms]



/*==============================================================================================================================================*/
/* Functions: Helper functions                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
char* createNcpystr(const char* src);
char* concatStr(const char* srcStrings[], uint8_t noOfSrcStrings);
uint8_t getTagTxt(tinyxml2::XMLElement* xmlNode, const char* tags[], char* xmlTxtBuff[], int len);

/*==============================================================================================================================================*/
/* END Helper functions                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: wdt                                                                                                                                   */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define FAULTACTION_REBOOT                  1<<7        // Reboots the entire decoder
#define FAULTACTION_MQTTDOWN                1<<6        // Brings down MQTT
#define FAULTACTION_FREEZE                  1<<5        // Freezes the state of the decoder, inhibits MQTT bring down and reboot 
#define FAULTACTION_FAILSAFE_LGS            1<<4        // Fail safes the Light groups channels decoders
#define FAULTACTION_FAILSAFE_SENSORS        1<<3        // Fail safes the sensor decoders
#define FAULTACTION_FAILSAFE_TURNOUTS       1<<2        // Fail safes the turnout decoders
#define FAULTACTION_FAILSAFE_ALL            FAULTACTION_FAILSAFE_LGS|FAULTACTION_FAILSAFE_SENSORS|FAULTACTION_FAILSAFE_TURNOUTS // Fail safe alias for all of the decoders
#define FAULTACTION_DUMP_ALL                1<<1        // Tries to dump as much decoder information as possible
#define FAULTACTION_DUMP_LOCAL              1           // Dumps information from the object at fault

struct wdt_t {
  long unsigned int wdtTimeout;
  esp_timer_handle_t timerHandle;
  char* wdtDescription;
  uint8_t wdtAction;
};

class wdt{
  public:
    //methods
    wdt(uint16_t p_wdtTimeout, char* p_wdtDescription, uint8_t p_wdtAction);
    ~wdt(void);
    void feed(void);

    //Data structures
    //--
    
  private:
    //methods
    static void kickHelper(wdt* p_wdtObject);
    void kick(void);

    //Data structures
    wdt_t* wdtData;
    esp_timer_create_args_t wdtTimer_args;
};

/*==============================================================================================================================================*/
/* END Class wdt                                                                                                                                */
/*==============================================================================================================================================*/


/*==============================================================================================================================================*/
/* Class: networking                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void (*netwCallback_t)(uint8_t);

#define ESP_WPS_MODE      WPS_TYPE_PBC                            //Wifi WPS mode set to "push-button
#define ESP_MANUFACTURER  "ESPRESSIF" 
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"

class networking{
  public:
    //Public methods
    static void start(void);                                      //Start ZTP WIFI
    static void regCallback(const netwCallback_t p_callback);     //Register callback for networking status changes
    static const char* getSsid(void);                             //Get connected WIFI SSID
    static uint8_t getChannel(void);                              //Get connected WIFI Channel
    static long getRssi(void);                                    //Get connected WIFI signal SNR
    static const char* getMac(void);                              //Get connected WIFI MAC adress
    static IPAddress getIpaddr(void);                             //Get host IP address received by DHCP
    static IPAddress getIpmask(void);                             //Get network IP mask received by DHCP
    static IPAddress getGateway(void);                            //Get gateway router address received by DHCP
    static IPAddress getDns(void);                                //Get DNS server address received by DHCP
    static IPAddress getNtp(void);                                //Get NTP server address received by DHCP
    static const char* getHostname(void);                         //Get hostname received by DHCP
    static uint8_t getOpState(void);                              //Get current Networking Operational state

    //Public data structures
    static uint8_t opState;                                       //Holds current Operational state

  private:
    //Private methods
    static void wpsStart(void);                                   
    static void wpsInitConfig(void);
    static String wpspin2string(uint8_t a[]);
    static void WiFiEvent(WiFiEvent_t event, system_event_info_t info);

    //Private data structures
    static String ssid;                                           //Connected WIFI SSID
    static uint8_t channel;                                       //Connected WIFI channel
    static long rssi;                                             //RSSI WIFI signal SNR
    static String mac;                                            //WIFI MAC address
    static IPAddress ipaddr;                                      //Host IP address received by DHCP
    static IPAddress ipmask;                                      //Network IP mask received by DHCP
    static IPAddress gateway;                                     //Gateway router received by DHCP
    static IPAddress dns;                                         //DNS server received by DHCP
    static IPAddress ntp;                                         //NTP server received by DHCP
    static String hostname;                                       //Hostname received by DHCP
    static netwCallback_t callback;                               //Reference to callback for network status change
    static esp_wps_config_t config;                               //Internal WIFI configuration object
};
networking NETWORK;

/*==============================================================================================================================================*/
/* END Class networking                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mqtt                                                                                                                                  */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void(*mqttSubCallback_t)(const char* topic, const char* payload, const void* args);
typedef void(*mqttStatusCallback_t)(uint8_t mqttStatus);


struct mqttSub_t {
  char* topic;
  mqttSubCallback_t mqttSubCallback;
  void* mqttCallbackArgs;
};

struct mqttTopic_t {
  char* topic;
  QList<mqttSub_t*>* topicList;
};

class mqtt {
  public:
    //Public methods
    static uint8_t init(const char* p_broker, uint16_t p_port, const char* p_user, const char* p_pass, const char* p_clientId, uint8_t p_defaultQoS, uint8_t p_keepAlive, bool p_defaultRetain, const char* p_lwTopic, const char* p_lwPayload, const char* p_upTopic, const char* p_upPayload);
    static uint8_t subscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback, const void* p_args);
    static uint8_t unSubscribeTopic(const char* p_topic, const mqttSubCallback_t p_callback);
    static uint8_t sendMsg(const char* p_topic, const char* p_payload, bool p_retain = defaultRetain);
    static uint8_t regStatusCallback(const mqttStatusCallback_t p_statusCallback);
    static void setPingPeriod(float p_pingPeriod);
    static uint8_t up(void);
    static uint8_t down(void);
    static char* getUri(void);
    static uint8_t getOpState(void);

    //Public data structures
    static uint8_t opState;

  private:
    //Private methods
    static void discover(void);
    static void onDiscoverResponse(const char* p_topic, const char* p_payload, const void* p_dummy);
    static void logSubscribers(void);
    static void poll(void* dummy);
    static void onMqttMsg(const char* p_topic, const byte* p_payload, unsigned int p_length);
    static uint8_t reSubscribe(void);
    static void mqttPing(void);

    //Private data structures
    static PubSubClient mqttClient;
    static SemaphoreHandle_t mqttLock;
    static char* broker;
    static uint16_t port;
    static char* uri;
    static char* user;
    static char* pass;
    static char* clientId;
    static uint8_t defaultQoS;
    static uint8_t keepAlive;
    static char* lwTopic;
    static char* lwPayload;
    static char* upTopic;
    static char* upPayload;
    static int mqttStatus;
    static uint8_t defaultQos;
    static bool defaultRetain;
    static esp_timer_handle_t mqttPingTimer;
    static esp_timer_create_args_t mqttPingTimer_args;
    static float pingPeriod;
    static char* mqttPingTopic;
    static bool discovered;
    static QList<mqttTopic_t*> mqttTopics;
    static mqttStatusCallback_t statusCallback;
};
mqtt MQTT;

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
#define XMLAUTHOR             0
#define XMLDESCRIPTION        1
#define XMLVERSION            2
#define XMLDATE               3
#define XMLRSYSLOGRECEIVER    4
#define XMLLOGLEVEL           5
#define XMLNTPSERVER          6
#define XMLTIMEZONE           7
#define XMLPINGPERIOD         8

class topDecoder{
  public:
    //Public methods
    static uint8_t init(void);
    static void on_configUpdate(const char* p_topic, const char* p_payload, const void* p_dummy);
    static uint8_t start(void);
    static char* getXmlAuthor(void);
    static char* getXmlDate(void);
    static char* getXmlVersion(void);
    static char* getRsyslogReceiver(void);
    static uint8_t getLogLevel(void);
    static char* getNtpServer(void);
    static uint8_t getTimezone(void);
    static float getMqttPingperiod(void);
    static uint8_t getopState(void);

    //Public data structures
    static uint8_t opState;

  private:
    //Private methods
    //--
    
    //Private data structures
    static char* mac;
    static char** xmlconfig;
    static const char* searchTags[9];
    static char* xmlVersion;
    static char* xmlDate;
    static char* ntpServer;
    static uint8_t timeZone;
    static tinyxml2::XMLDocument* xmlConfigDoc;
    static SemaphoreHandle_t topDecoderLock;
    static lgsDecoder* lighGroupsDecoder_0;
    static lgsDecoder* lighGroupsDecoder_1;
    static lgsDecoder* lighGroupsDecoder_2;
    static lgsDecoder* lighGroupsDecoder_3;
};
topDecoder TOPDECODER;

/*==============================================================================================================================================*/
/* END Class topDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: lgsDecoder                                                                                                                            */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define XMLLGADDR             0
#define XMLLGSEQ              1
#define XMLLGSYSTEMNAME       2
#define XMLLGUSERNAME         3
#define XMLLGTYPE             4

struct lightGroup_t {
  mastDecoder* lightGroupObj;
  lgsDecoder* lightGroupsChannel;
  uint16_t lgAddr;
  uint16_t lgSeq;
  uint8_t lgNoOfLed; 
  uint16_t lgSeqOffset;
  char* lgSystemName;
  char* lgUserName;
  char* lgType;
};

struct stripLed_t {
  uint8_t currentValue = 0;
  uint8_t wantedValue = SM_BRIGHNESS_FAIL;
  uint8_t incrementValue = 0;
  bool dirty = false;
};

class lgsDecoder{
  public:
    //Public methods
    lgsDecoder(void);
    ~lgsDecoder(void);
    uint8_t init(const uint8_t p_channel);
    uint8_t on_configUpdate(tinyxml2::XMLElement* p_lightgroupsXmlElement);
    uint8_t start(void);
    uint8_t updateLg(uint16_t p_seqOffset, uint8_t p_buffLen, const uint8_t* p_wantedValueBuff, const uint16_t* p_transitionTimeBuff);
    unsigned long getOverRuns(void);
    flash* getFlashObj(uint8_t p_flashType);
    void clearOverRuns(void);
    uint8_t getOpState(void);

    //Public data structures
    //--

  private:
    //Private methods
    static void updateStripHelper(lgsDecoder* p_lgsObject);
    void updateStrip(void);

    //Private data structures
    uint8_t opState;
    uint8_t channel;
    int pin;
    QList<lightGroup_t*> lgList;
    QList<stripLed_t*> dirtyList;
    tinyxml2::XMLElement* lightGroupXmlElement;
    unsigned long overRuns = 0;
    stripLed_t* stripCtrlBuff;
    Adafruit_NeoPixel* strip;
    uint8_t* stripWritebuff;
    esp_timer_handle_t stripUpdateTimerHandle = NULL;
    SemaphoreHandle_t updateStripReentranceLock = NULL;
    SemaphoreHandle_t lgsDecoderLock = NULL;
    flash* FLASHSLOW;
    flash* FLASHNORMAL;
    flash* FLASHFAST;
};

/*==============================================================================================================================================*/
/* END Class lgsDecoder                                                                                                                         */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: flash                                                                                                                                 */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
typedef void(*flashCallback_t)(bool p_flashState, void* args);

struct callbackSub_t {
  flashCallback_t callback;
  void* callbackArgs;
};

struct flash_t {
  QList<callbackSub_t*> callbackSubs;
  uint16_t onTime;
  uint16_t offTime;
  bool flashState;
  esp_timer_handle_t timerHandle;
};

class flash{
  public:
    //Public methods
    flash(float p_freq, uint8_t p_duty);
    ~flash(void);
    uint8_t subscribe(flashCallback_t p_callback, void* p_args);
    uint8_t unSubscribe(flashCallback_t cb);
    static void flashTimeoutHelper(flash* p_flashObject);
    void flashTimeout(void);
    int getOverRuns(void);
    void clearOverRuns(void);

    //Public data structures
    uint8_t opState;

  private:
    //Privat methods
    //--
    
    //Private data structures
    int overRuns;
    flash_t* flashData;
    esp_timer_create_args_t flashTimer_args;
    SemaphoreHandle_t flashReentranceLock;
};

/*==============================================================================================================================================*/
/* END Class flash                                                                                                                              */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: signalMastAspects                                                                                                                     */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/
#define SM_MAXHEADS             12
#define UNLIT_APPEARANCE        0
#define LIT_APPEARANCE          1
#define FLASH_APPEARANCE        2
#define UNUSED_APPEARANCE       3

struct mastType_t {
  char* name;
  QList<uint8_t*> headAspects;
  uint8_t noOfHeads;
  uint8_t noOfUsedHeads;
};

class signalMastAspects{
  public:
    //Public methods
    static uint8_t onConfigUpdate(tinyxml2::XMLElement* p_smAspectsXmlElement);
    static uint8_t getAppearance(char* p_smType, char* p_aspect, uint8_t** p_appearance);
    static uint8_t getNoOfHeads(char* p_smType);
    
    //Public data structures
    static uint8_t opState;

  private:
    //Private methods
    //--
    
    //Private data structures
    static QList<char*> aspects;
    static QList<mastType_t*> mastTypes;
};
signalMastAspects SMASPECTS;

/*==============================================================================================================================================*/
/* END Class signalMastAspects                                                                                                                  */
/*==============================================================================================================================================*/



/*==============================================================================================================================================*/
/* Class: mastDecoder                                                                                                                           */
/* Purpose:                                                                                                                                     */
/* Methods:                                                                                                                                     */
/* Data structures:                                                                                                                             */
/*==============================================================================================================================================*/

#define SM_TYPE               0
#define SM_DIMTIME            1
#define SM_FLASHFREQ          2
#define SM_BRIGHTNESS         3

struct mastDesc_t {
  char* smType;
  char* smDimTime;
  char* smFlashFreq;
  char* smBrightness;
};

class mastDecoder{
  public:
    //Public methods
    mastDecoder(void);          
    ~mastDecoder(void); //destructor not implemented - if called a fatal exception and reboot will occur!
    uint8_t init(void);
    uint8_t onConfigure(lightGroup_t* p_genLgDesc, tinyxml2::XMLElement* p_mastDescXmlElement); //Returns RC_OK if successful
    uint8_t start(void); //Starting the mastDecoder, subscribing to aspect changes, and flash events, returns RC_OK if successful
    uint8_t stop(void);
    static void onAspectChangeHelper(const char* p_topic, const char* p_payload, const void* p_mastObject);
    void onAspectChange(const char* p_topic, const char* p_payload); //Mqtt callback at aspect change
    static void onFlashHelper(const bool p_flashState, void* p_flashObj);
    void onFlash(const bool p_flashState); //Flash object call back
    uint8_t getOpState(void); //Returns the Objects Operational state

    //Public data structures
    //--

  private:
    //Private methods
    uint8_t parseXmlAppearance(const char* p_aspectXml, char* p_aspect); //Parses a new aspect from *p_aspectXml and provides clear text aspect text in p_aspect, Returns RC_OK if successful

    //Private data structures
    uint8_t opState; //Operational state of the mastDecoder object
    SemaphoreHandle_t mastDecoderLock; //Mutex to protect common object datastructures from multi thread parallel access
    SemaphoreHandle_t mastDecoderReentranceLock; // Mutex to avoid re-entrance caused by new aspects or flash events
    lightGroup_t* genLgDesc; //General light group descriptor - see lgsDecoder definitions
    mastDesc_t* mastDesc; // mastDecoder specific descriptor
    char aspect[80]; //Curren mastDecoder aspect - clear text name
    uint8_t* appearance; //Holds the Appearance signatures for all heads in the mast (LIT, UNLIT, FLASH, UNUSED..), See definitions in the mastaspect class
    uint8_t* tmpAppearance; //Temporarrily stores the appearance as received from the lgs decoder [SM_MAXHEADS]
    uint8_t* appearanceWriteBuff; //Holds the wanted raw led wanted values to be passed to the lgsDecoder for rendering
    uint16_t* appearanceDimBuff; //Holds the Dim timing for each head in the mast to be passed to the lgsDecoder for rendering
    uint16_t smDimTime; //Dim timing in ms for this signal mast
    uint8_t smBrightness; //Brighness 0-255
    bool flashOn; //Holds the flash state for all heads in the mast currently flashing
};

/*==============================================================================================================================================*/
/* END Class mastDecoder                                                                                                                        */
/*==============================================================================================================================================*/

#endif /*GENJMRI_H*/
