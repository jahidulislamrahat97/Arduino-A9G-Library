/*!
 * @file A9G.h
 *
 * @mainpage A9G Library
 *
 * @section intro_sec Introduction
 *
 * This is a library for the A9/A9G
 *
 * Designed specifically to work with the A9 & A9G Breakout
 * ----> http://www.ai-thinker.com/pro_view-77.html
 * ----> http://www.ai-thinker.com/pro_view-78.html
 *
 * These A9/A9G communicate using UART, 2 pins are required to interface.
 * We invests some time and resources providing this open source code,
 * please support open-source hardware and software by contribute your talent.
 *
 * @section author Author
 *
 * Written by Jahidul Islam Rahat/Zahan Zib Sarowar Dhrubo for esp32 and arduino.
 * 
 * @section license License
 * 
 * MIT license, (see LICENSE)
 * 
 */

#ifndef A9G_H
#define A9G_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "A9G_Event.h"

#define MAX_WAIT_TIME_MS 60000
#define MAX_TERM_SIZE 100
#define MAX_AT_RESPONSE_SIZE 128
#define MAX_MSG_SIZE 128

/**
 * @brief Main GSM class
 *
 */
class GSM
{
private:
    HardwareSerial &_gsm;

    bool _debug;
    unsigned long _maxWaitTimeMS;



    bool is_mqtt_error;
    bool is_mqtt;
    bool is_sms;
    bool sms_read;
    bool complete_data;
    bool received;

    char did[21];
    char _term[MAX_TERM_SIZE];
    char msg[MAX_MSG_SIZE];


    bool _new_data_received;

    char _source[MAX_MSG_SIZE];
    char _payload[MAX_MSG_SIZE];
    char _topic[MAX_MSG_SIZE];
    char _number[16];
    char _errorType[MAX_MSG_SIZE];
    uint16_t _error;

    typedef void (*DataCallbackFunction)(const char *source, const char *payload, const char *topic);
    typedef void (*EventDispatchCallback)(A9G_Event_t *event);
    DataCallbackFunction _callback = nullptr;
    EventDispatchCallback _eventCallback = nullptr;


    /**
     * @brief 
     * @todo write comment for every term [https://wiki.dfrobot.com/A9G_Module_SKU_TEL0134]
     */
    typedef enum Term_List_t
    {
        TERM_CREG=0,
        TERM_CTZV, //network related: not clear
        TERM_CIEV, // call
        TERM_CPMS,
        TERM_CMT,
        TERM_CMGL,
        TERM_CMGR,
        TERM_GPSRD,
        TERM_CGATT,
        TERM_AGPS,
        TERM_GPNT,
        TERM_MQTTPUBLISH,
        TERM_CMGS,
        TERM_CME,
        TERM_CMS,
        TERM_MAX,
        TERM_NONE
    } Term_List_t;

    const char _terms_string[20][15] = {"CREG","CTZV","CIEV","CPMS","CMT","CMGL","CMGR","GPSRD","CGATT","AGPS","GPNT","MQTTPUBLISH","CMGS","CME ERROR", "CMS ERROR"};
    uint8_t _checkTermFromString(const char* term_str);
    void _processTermString(A9G_Event_t *event, const char data[], int data_len);
public:

    GSM(HardwareSerial &A9G, bool debug);

    //pass a param baudrate in init fun. remove baudrate from class
    void init(uint32_t baudRate);
    
    bool bCheckRespose(const int timeout); // need to fix timeout need to reduce
    void EventDispatch(EventDispatchCallback eventCallback);
    void executeCallback();

    // do not use it further.
    void vProcessIncomingData();
    void RegisterDataCallback(DataCallbackFunction callback);

    void errorPrintCME(int ret);
    void errorPrintCMS(int ret);
    
    


    bool bIsReady();
    bool waitForReady();

    // GPRS/Internet Commands
    bool IsGPRSAttached();
    bool AttachToGPRS();
    bool DetachToGPRS();
    bool SetAPN(const char pdp_type[], const char apn[]);
    bool ActivatePDP();
    bool DeactivatePDP();

    // MQTT Command
    bool ConnectToBroker(const char broker[], int port, const char id[], uint8_t keep_alive, uint16_t clean_session);
    bool ConnectToBroker(const char broker[], int port);
    bool DisconnectBroker();
    bool SubscribeToTopic(const char topic[], uint8_t qos, unsigned long timeout);
    bool SubscribeToTopic(const char topic[]);
    bool UnsubscribeToTopic(const char topic[]);
    bool PublishToTopic(const char topic[], const char msg[], uint8_t qos, uint8_t retain, uint8_t dup, uint16_t msg_len, unsigned long timeout); // not ready
    bool PublishToTopic(const char topic[], const char msg[]);

    // bool vConnectGPRS();
    // void vConnectToBroker(const char *broker, int port, const char *id);
    // void vDisconnectBroker();
    // void vSubscribeToTopic(char *topic);
    // void vPublishToTopic(char *topic, const char *msg);
    // void vPublishStartMessage(char *topic);
    // void vReadMessage(char mode);
    // void vSendMessage(const char *message, const char *number);
    // void vDeleteMessage(char mode);
};

#endif

/*
Basic Commands:

AT: Test command. Returns "OK" if the module is responsive.

AT+IPR=<baudrate>: Set the serial port baud rate.

AT+CPIN?: Check if a SIM card is inserted and if it requires a PIN.

AT+CPIN=<PIN>: Enter the SIM card PIN.

AT+CREG?: Check for network registration status.

AT+CGREG?: Check for GPRS network registration status.

AT+CSQ: Check signal strength (in dBm).

AT+CCID: Read the ICCID (Integrated Circuit Card Identifier) of the SIM card.

AT+CGSN: Get the IMEI (International Mobile Equipment Identity) of the module.




GPRS/Internet Commands:

AT+CGATT?: Check if GPRS is attached.

AT+CGATT=1: Attach to GPRS.

AT+CGATT=0: Detach from GPRS.

AT+CGDCONT=<cid>,"<PDP_type>","<APN>": Set the APN for data connection.

AT+CGACT=<state>[,<cid>]: Activate or deactivate a PDP context.

//TCP/IP Commands:
AT+CIPSTATUS: Check current connection status.

AT+CIPSTART=<type>,"<remote_IP>",<remote_port>: Start a TCP/IP connection.

AT+CIPSEND=<length>: Send data over a TCP/IP connection.

AT+CIPCLOSE: Close a TCP/IP connection.

AT+CIPMUX=<mode>: Set multiple connections mode.

AT+CIFSR: Get local IP address.





MQTT Commands (if supported):

AT+MQTTCONN="<broker>","<port>","<client_id>",<keep_alive>,<clean_session>: Connect to MQTT broker.

AT+MQTTDISCONN: Disconnect from MQTT broker.

AT+MQTTPUB="<topic>",<qos>,<retain>,<dup>,"<message>",<message_len>,<timeout>: Publish a message to an MQTT topic.

AT+MQTTSUB="<topic>",<qos>,<timeout>: Subscribe to an MQTT topic.

AT+MQTTUNSUB="<topic>": Unsubscribe from an MQTT topic.



Other Commands:

AT+CPBW: Phonebook related commands.

AT+CMGF: SMS message format.

AT+CMGS: Send SMS message.

AT+CMGL: List SMS messages.

AT+CMGR: Read SMS message.

AT+CMGD: Delete SMS message.

AT+CNMI: New SMS message indications.

AT+CMEE: Set command error result code.

AT+GSV: Get software version information.

AT+ICCID: Read the ICCID of the SIM card.

AT+CBC: Check battery status.
*/