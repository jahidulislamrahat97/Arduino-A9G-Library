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
 * Written by Jahidul Islam Rahat for esp32.
 *
 * 
 * @section license License
 *
 * MIT license, (see LICENSE)
 *
 */

#ifndef A9G_H
#define A9G_H

#include <Arduino.h>
#include <Stream.h>
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
    Stream *_gsm;

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
        TERM_CREG = 0,
        TERM_CTZV, // CTZR time zone related: not clear
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
        TERM_CSQ,
        TERM_EGMR,
        TERM_CCID,
        TERM_MAX,
        TERM_NONE
    } Term_List_t;

    const char _terms_string[25][15] = {"CREG", "CTZV", "CIEV", "CPMS", "CMT", "CMTI", "CMGL", "CMGR", "GPSRD", "CGATT", "AGPS", "GPNT", "MQTTPUBLISH", "CMGS", "CME ERROR", "CMS ERROR", "CSQ","EGMR","CCID"};

    uint8_t _checkTermFromString(const char *term_str);
    void _processTermString(A9G_Event_t *event, const char data[], int data_len);
    bool _checkResponse(const int timeout); //  it will be private. need to fix timeout
    bool _checkOk(const int timeout);
    bool _sms;
    int _sms_i;

public:
    GSM(bool debug);

    /**
     * @brief Initialize the GSM module with the specified baud rate.
     *
     * This function initializes the GSM module with the provided baud rate.
     *
     * @param baudRate The communication baud rate for the GSM module.
     */
    void init(Stream *gsm);
    void Test(char *data);


    /**
     * @brief Register a callback function for event dispatching.
     *
     * @param eventCallback The callback function to be registered.
     */
    void EventDispatch(EventDispatchCallback eventCallback);

    /**
     * @brief Execute callback function for processing GSM module output.
     *
     * This function reads data from the GSM module, processes it to identify and extract terms,
     * and triggers the associated callback function for further processing.
     */
    void executeCallback();

    /**
     * @brief Prints the corresponding error message for CME error codes.
     *
     * @param ret The CME error code.
     */
    void errorPrintCME(int ret);

    /**
     * @brief Prints the corresponding error message for CMS error codes.
     *
     * @param ret The CME error code.
     */
    void errorPrintCMS(int ret);

    /*###############################################*/
    /*********************  BASIC ********************/
    /*###############################################*/

    /**
     * @brief Checks if the GSM module is ready.
     *
     * Sends an "AT" command to the GSM module and waits for a response.
     *
     * @return True if the GSM module is ready, false otherwise.
     */
    bool bIsReady();

    /**
     * @brief Waits for the GSM module to become ready.
     *
     * This function sends an "AT" command to the GSM module and waits for a response indicating readiness.
     * Warning: This is a blocking function and may take 15s to 60s to complete. It is not necessary to use this function,
     * but it may be useful for further operations.
     *
     * @return True if the GSM module is ready, false otherwise.
     */
    bool waitForReady();

    /**
     * @brief Reads the IMEI (International Mobile Equipment Identity) from the GSM module.
     */
    void ReadIMEI();

    void ReadCSQ();
    void ReadCCID();

    

    
    /*###############################################*/
    /*********************  GPRS *********************/
    /*###############################################*/

    /**
     * @brief Checks if the GSM module is attached to GPRS (General Packet Radio Service).
     *
     * @return true if GPRS is attached, false otherwise.
     */
    bool IsGPRSAttached();

    /**
     * @brief Attempts to attach the GSM module to GPRS (General Packet Radio Service).
     *
     * @return true if the attachment is successful, false otherwise.
     */
    bool AttachToGPRS();

    /**
     * @brief Attempts to detach the GSM module from GPRS (General Packet Radio Service).
     *
     * @return true if the detachment is successful, false otherwise.
     */
    bool DetachToGPRS();

    /**
     * @brief Sets the Access Point Name (APN) and PDP (Packet Data Protocol) type for GPRS connection.
     *
     * @param pdp_type The PDP type.
     * @param apn The Access Point Name.
     * @return true if the APN is set successfully, false otherwise.
     */
    bool SetAPN(const char pdp_type[], const char apn[]);

    /**
     * @brief Activates the Packet Data Protocol (PDP) context for GPRS connection.
     *
     * @return true if the PDP context is activated successfully, false otherwise.
     */
    bool ActivatePDP();

    /**
     * @brief Deactivates the Packet Data Protocol (PDP) context for GPRS connection.
     * @todo Need to finish this function.
     *
     * @return true if the PDP context is deactivated successfully, false otherwise.
     */
    bool DeactivatePDP();

    /*###############################################*/
    /*********************  MQTT *********************/
    /*###############################################*/

    /**
     * @brief Connects to the MQTT broker with the specified parameters.
     *
     * @param broker The MQTT broker address.
     * @param port The port number for the MQTT connection.
     * @param id The client ID to use for the MQTT connection.
     * @param keep_alive The keep-alive interval for the MQTT connection.
     * @param clean_session The clean session flag for the MQTT connection.
     * @return true if the connection is successful, false otherwise.
     */
    bool ConnectToBroker(const char broker[], int port, const char id[], uint8_t keep_alive, uint16_t clean_session);

    /**
     * @brief Connects to the MQTT broker with the specified parameters.
     *
     * @param broker The MQTT broker address.
     * @param port The port number for the MQTT connection.
     * @return true if the connection is successful, false otherwise.
     */
    bool ConnectToBroker(const char broker[], int port);

    /**
     * @brief Disconnects from the MQTT broker.
     *
     * @return true if the disconnection is successful, false otherwise.
     */
    bool DisconnectBroker();

    /**
     * @brief Subscribes to a topic with the specified quality of service (QoS) level.
     *
     * @param topic The topic to subscribe to.
     * @param qos The quality of service level (0, 1, or 2).
     * @param timeout The timeout for the subscription operation, in milliseconds.
     * @return true if the subscription is successful, false otherwise.
     */
    bool SubscribeToTopic(const char topic[], uint8_t qos, unsigned long timeout);

    /**
     * @brief Subscribes to a topic with default quality of service (QoS) level 0.
     *
     * @param topic The topic to subscribe to.
     * @return true if the subscription is successful, false otherwise.
     */
    bool SubscribeToTopic(const char topic[]);

    /**
     * @brief Unsubscribes from a topic.
     *
     * @param topic The topic to unsubscribe from.
     * @return true if the unsubscription is successful, false otherwise.
     */
    bool UnsubscribeToTopic(const char topic[]);

    /**
     * @brief Publishes a message to a topic.
     *
     * @param topic The topic to publish the message to.
     * @param msg The message to be published.
     * @return true if the publication is successful, false otherwise.
     */
    bool PublishToTopic(const char topic[], const char msg[]);



    /*###############################################*/
    /*********************  SMS  *********************/
    /*###############################################*/


    /**
     * @brief Activates the Terminal Equipment (TE) to receive unsolicited result codes.
     *
     * @return true if activation is successful, false otherwise.
     */
    bool ActivateTE();

    /**
     * @brief Sets the format for reading messages.
     *
     * @param mode Set to true for text mode, false for PDU mode.
     * @return true if the command is successful, false otherwise.
     */
    bool SetFormatReading(bool mode); // 1 for txt, 0 for pud format. i don't know what is pud format.
    
    /*
        Drubo: AT+CPMS=\"ME\",\"ME\",\"ME\".  :::
        DfRobot:>>>
        AT+CPMS="SM","SM","SM"    //Set message storage unit, and you can also check message capacity
        +CPMS: 0,50,0,50,0,50
    */

    /**
     * @brief Sets the message storage unit.
     *
     * @todo Here, <mem1>, <mem2>, and <mem3> are placeholders for the storage locations of SMS messages, status reports, and broadcast messages, respectively.

        The possible values for <memX> are:

        "SM": This refers to the SIM card memory, where messages are stored on the SIM card.
        "ME": This stands for the phone's memory, where messages are stored on the device itself.
        "MT": This is the combined memory of both the SIM card and the phone's memory.
    *
    * @return true if the command is successful, false otherwise.
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

        +CIEV: +CIEV,"MESSAGE",1
        +CMTI: +CMTI,"ME",17
        AT+CMGR=1        //Read the first message
        +CMGR: "REC READ","+86xxxxxxxxxxx",,"2017/10/09,09:14:52+08"
        +CMGR: "REC UNREAD","+8801687223094",,"2023/10/19,14:18:26+06"
    */
    /**
     * @brief Reads a specific SMS message from the GSM module.
     *
     * This function sends the AT+CMGR command followed by the index of the message to be read.
     *
     * @param index The index or location of the message to be read.
     *
     */
    void ReadMessage(uint8_t index);
    
    /**
     * @brief Deletes a specific message from the GSM module.
     *
     * @param index The index or location of the message to be deleted.
     * @param type The type of message storage. Refer to Message_Type_t enum for options.
     */
    void DeleteMessage(uint8_t index, Message_Type_t type);
    bool DeleteAllMessage();

    /**
     * @brief Sends an SMS message to a specified phone number.
     *
     * This function sends the AT+CMGS command followed by the destination phone number and the message content.
     *
     * @param number The destination phone number in string format.
     * @param message The content of the SMS message.
     *
     * @return true if the message was sent successfully, false otherwise.
     *
     */
    bool bSendMessage(const char number[], const char message[]);
    void vSendMessage(const char number[], const char message[]);




    /*###############################################*/
    /*********************  TCP/IP *******************/
    /*###############################################*/



    /*###############################################*/
    /*********************  GPS  *********************/
    /*###############################################*/


};

#endif

/*
Basic Commands:

AT+IPR=<baudrate>: Set the serial port baud rate.

AT+CPIN?: Check if a SIM card is inserted and if it requires a PIN.

AT+CPIN=<PIN>: Enter the SIM card PIN.

AT+CREG?: Check for network registration status.

AT+CGREG?: Check for GPRS network registration status.





//TCP/IP Commands:
AT+CIPSTATUS: Check current connection status.

AT+CIPSTART=<type>,"<remote_IP>",<remote_port>: Start a TCP/IP connection.

AT+CIPSEND=<length>: Send data over a TCP/IP connection.

AT+CIPCLOSE: Close a TCP/IP connection.

AT+CIPMUX=<mode>: Set multiple connections mode.

AT+CIFSR: Get local IP address.






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

AT+CBC: Check battery status.

*/