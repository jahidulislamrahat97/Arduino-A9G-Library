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
    bool _isSMS;

    typedef void (*EventDispatchCallback)(A9G_Event_t *event);
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
        TERM_CMTI,
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

    const char _terms_string[20][15] = {"CREG","CTZV","CIEV","CPMS","CMT","CMTI","CMGL","CMGR","GPSRD","CGATT","AGPS","GPNT","MQTTPUBLISH","CMGS","CME ERROR", "CMS ERROR"};
    uint8_t _checkTermFromString(const char* term_str);
    void _processTermString(A9G_Event_t *event, const char data[], int data_len);
    bool _checkResponse(const int timeout); //  it will be private. need to fix timeout
    bool _checkOk(const int timeout);
    
public:



    GSM(HardwareSerial &A9G, bool debug);
    void init(uint32_t baudRate);

    //callback function for event dispatch
    void EventDispatch(EventDispatchCallback eventCallback);
    void executeCallback();

   //debuging function
    void errorPrintCME(int ret);
    void errorPrintCMS(int ret);
    
    
    bool bIsReady();
    bool waitForReady();

    void ReadIMEI();

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
    bool PublishToTopic(const char topic[], const char msg[]);

    //SMS Command
    bool ActivateTE();
    bool SetFormatReading(bool mode); //1 for txt, 0 for pud format. i don't know what is pud format.
    /*
        Drubo: AT+CPMS=\"ME\",\"ME\",\"ME\".  :::
        DfRobot:>>>
        AT+CPMS="SM","SM","SM"    //Set message storage unit, and you can also check message capacity 
        +CPMS: 0,50,0,50,0,50
    */
    bool SetMessageStorageUnit();
    /**
     * @brief   AT+CPBS?
     * 
     * 
     * @return
     * 
     */
    void CheckMessageStorageUnit();
    
    
    /*
        AT+CMGR=1        //Read the first message 
        +CMGR: "REC READ","+86xxxxxxxxxxx",,"2017/10/09,09:14:52+08"
        +CMGR: "REC UNREAD","+8801687223094",,"2023/10/19,14:18:26+06"
    */
    void ReadMessage(uint8_t index); // working..


    void DeleteMessage(uint8_t index, Message_Type_t type);
    bool DeleteAllMessage();



    /*
        A9G.print(F("AT+CMGS=\""));
        A9G.print(number);
        A9G.print(F("\"\r"));
        A9G.print(message);
        A9G.println((char)26);
    */
    bool SendMessage(const char number[], const char message[]);
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