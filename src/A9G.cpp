/*!
 * @file A9G.cpp
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
 * Written by Jahidul Islam Rahat/Zahan Zib Sarowar Dhrubo for esp32 and arduino.
 *
 * @section license License
 * MIT license, (see LICENSE)
 *
 */

// https://github.com/jahidulislamrahat97/Arduino-A9G-Library

#include "A9G.h"

GSM::GSM(HardwareSerial &A9G, bool debug)
    : _gsm(A9G), _debug(debug), _maxWaitTimeMS(MAX_WAIT_TIME_MS)
{
    is_mqtt_error = false;
    is_mqtt = false;
    is_sms = false;
    is_mqtt_error = false;
    sms_read = false;
    _new_data_received = false;
    complete_data = false;
    received = false;
}

void GSM::init(uint32_t baudRate)
{
    _gsm.begin(baudRate);
}

void GSM::RegisterDataCallback(DataCallbackFunction callback)
{
    _callback = callback; // Store the provided callback function
}
void GSM::EventDispatch(EventDispatchCallback eventCallback)
{
    _eventCallback = eventCallback; // Store the provided callback function
}

uint8_t GSM::_checkTermFromString(const char *term_str)
{
    for (int i = 0; i < TERM_MAX; i++)
    {
        if (!strcmp(term_str, _terms_string[i]))
        {
            return i;
        }
    }
    return TERM_NONE;
}

void GSM::_processTermString(A9G_Event_t *event, const char data[], int data_len)
{
    Serial.println(" IN _processTermString: ");
    Serial.print("event->id: ");
    Serial.println(event->id);
    Serial.print("data: ");
    Serial.println(data);
    Serial.print("data_len: ");
    Serial.println(data_len);

    uint8_t comma_count = 0;

    if (event->id == EVENT_MQTTPUBLISH)
    {
        uint8_t topic_count = 0;
        uint8_t message_count = 0;
        for (int i = 0; i <= data_len; i++)
        {
            if (data[i] == ',')
            {
                comma_count++;
                continue;
            }
            if (comma_count == 1)
            {
                event->topic[topic_count++] = data[i];
                Serial.print(data[i]);
            }
            if(comma_count == 3){
                event->message[message_count++] = data[i];
            }
        }
        event->message[message_count++] = '\0';
        event->topic[topic_count++] = '\0';
    }
    else if (event->id == EVENT_CME)
    {
        event->error = atoi(data);
    }
}

void GSM::executeCallback()
{
    char c;
    int term_length = 0;
    int term_data_count = 0;
    bool term_started = false, term_ended = false;
    bool term_data_started = false, term_data_ended = false;
    char term_data[MAX_MSG_SIZE];

    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));

    while (_gsm.available())
    {
        char c = _gsm.read();
        Serial.print(c);

        if (c == '+' && !term_started && !term_ended)
        {
            // Serial.println("########## New Term!");
            term_started = 1;
            term_ended = 0;
            term_data_count = 0;
            continue;
        }

        if ((c == '=' || c == ':') && !term_ended)
        {
            // Serial.println("********** Term Stop!");
            _term[term_length] = '\0';
            term_ended = 1;
            continue;
        }

        if (term_started && !term_ended)
        {
            _term[term_length++] = c;
            // Serial.print("term: ");
            // Serial.println(c);
        }

        if (term_started && term_ended && !term_data_started)
        {
            Serial.print(">>>term: ");
            Serial.println(_term);

            int term_id = _checkTermFromString(_term);
            // Serial.print("term id: ");
            // Serial.println(term_id);

            if (term_id == TERM_NONE || term_id == TERM_MAX)
            {
                // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                Serial.println("***** Terminating this term here ******");
                term_started = 0;
                term_ended = 0;
                term_length = 0;
                term_data_started = 0;
                break;
            }
            else
            {
                term_data_started = 1;
                Serial.println("***** Term Accepted ******");
                event->id = static_cast<Event_ID_t>(term_id);
            }
        }
        if (term_ended && term_data_started)
        {
            if (c == '\r')
            {
                Serial.print(">>>term data:");
                Serial.println(term_data);
                term_started = 0;
                term_ended = 0;
                term_length = 0;
                term_data_started = 0;
                if (_eventCallback)
                {
                    _processTermString(event, term_data, term_data_count);
                    // Serial.println("Error Callback Triggered");
                    // Serial.print("event->id: ");
                    // Serial.println(event->id);
                    // Serial.print("event->param1: ");
                    // Serial.println(event->param1);

                    _eventCallback(event); // Execute the callback function with the new random number
                }

                break;
            }
            else
            {
                term_data[term_data_count++] = c;
            }
        }
    }
    free(event);
    // yield();
}

bool GSM::bCheckRespose(const int timeout)
{
    char response[150];                    // Assuming the response won't exceed 50 characters
    memset(response, 0, sizeof(response)); // Initialize response to all zeros
    long int start_time = millis();
    int idx = 0; // Index to keep track of response characters

    int term_length = 0;
    int term_data_count = 0;
    bool term_started = false, term_ended = false;
    bool term_data_started = false, term_data_ended = false;
    char term_data[MAX_MSG_SIZE];

    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));

    while ((millis() - start_time) < timeout)
    {
        while (_gsm.available())
        {
            char c = _gsm.read();
            response[idx++] = c;
            Serial.print(c);

            if (c == '+' && !term_started && !term_ended)
            {
                // Serial.println("########## New Term!");
                term_started = 1;
                term_ended = 0;
                term_data_count = 0;
                continue;
            }

            if ((c == '=' || c == ':') && !term_ended)
            {
                // Serial.println("********** Term Stop!");
                _term[term_length] = '\0';
                term_ended = 1;
                continue;
            }

            if (term_started && !term_ended)
            {
                _term[term_length++] = c;
                // Serial.print("term: ");
                // Serial.println(c);
            }

            if (term_started && term_ended && !term_data_started)
            {
                Serial.print(">>>term: ");
                Serial.println(_term);

                int term_id = _checkTermFromString(_term);
                // Serial.print("term id: ");
                // Serial.println(term_id);

                if (term_id == TERM_GPSRD || term_id == TERM_CMGS || term_id == TERM_CMGL || term_id == TERM_CMGR || term_id == TERM_CIEV || term_id == TERM_NONE || term_id == TERM_MAX)
                {
                    // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                    Serial.println("***** Terminating this term here ******");
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                }
                else
                {
                    term_data_started = 1;
                    Serial.println("***** Term Accepted ******");
                    event->id = static_cast<Event_ID_t>(term_id);
                }
            }
            if (term_ended && term_data_started)
            {
                if (c == '\r')
                {
                    Serial.print(">>>term data:");
                    Serial.println(term_data);
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                    if (_eventCallback)
                    {
                        _processTermString(event, term_data, term_data_count);
                        // Serial.println("Error Callback Triggered");
                        // Serial.print("event->id: ");
                        // Serial.println(event->id);
                        // Serial.print("event->param1: ");
                        // Serial.println(event->param1);

                        _eventCallback(event); // Execute the callback function with the new random number
                    }
                }
                else
                {
                    term_data[term_data_count++] = c;
                }
            }

            // Check if "OK" is present in the received characters
            if (strstr(response, "OK"))
            {
                free(event);
                return true;
            }
            if (idx >= sizeof(response) - 1)
            {
                Serial.println("Response buffer overflow!");
                free(event);
                return false;
            }
        }
    }
    free(event);
    return false;
}

bool GSM::bIsReady()
{
    _gsm.println("AT");
    if (bCheckRespose(2000))
    {
        if (_debug)
            Serial.println(F("GSM Ready"));
        return true;
    }
    else
        return false;
}

/**
 * @brief waring!!! blocking function. it will take 15s - 35s.
 * it's not neccesary to use this function.
 * it will not usefull that much.
 * but it would be good for further move
 *
 *
 * @return true
 * @return false
 */
bool GSM::waitForReady()
{
    char responseBuffer[100]; // Adjust the size as per your needs
    int index = 0;


    int term_length = 0;
    int term_data_count = 0;
    bool term_started = false, term_ended = false;
    bool term_data_started = false, term_data_ended = false;
    char term_data[MAX_MSG_SIZE];


    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));


    _gsm.println("AT");

    // need make this function break until it gets ready command
    while (1)
    {
        while (_gsm.available())
        {
            char c = _gsm.read();
            responseBuffer[index++] = c;
            // Serial.print(responseBuffer);


            if (c == '+' && !term_started && !term_ended)
            {
                // Serial.println("########## New Term!");
                term_started = 1;
                term_ended = 0;
                term_data_count = 0;
                continue;
            }

            if ((c == '=' || c == ':') && !term_ended)
            {
                // Serial.println("********** Term Stop!");
                _term[term_length] = '\0';
                term_ended = 1;
                continue;
            }

            if (term_started && !term_ended)
            {
                _term[term_length++] = c;
                // Serial.print("term: ");
                // Serial.println(c);
            }

            if (term_started && term_ended && !term_data_started)
            {
                Serial.print(">>>term: ");
                Serial.println(_term);

                int term_id = _checkTermFromString(_term);
                // Serial.print("term id: ");
                // Serial.println(term_id);

                if (term_id == TERM_GPSRD || term_id == TERM_CMGS || term_id == TERM_CMGL || term_id == TERM_CMGR || term_id == TERM_CIEV || term_id == TERM_NONE || term_id == TERM_MAX)
                {
                    // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                    Serial.println("***** Terminating this term here ******");
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                }
                else
                {
                    term_data_started = 1;
                    Serial.println("***** Term Accepted ******");
                    event->id = static_cast<Event_ID_t>(term_id);
                }
            }
            if (term_ended && term_data_started)
            {
                if (c == '\r')
                {
                    Serial.print(">>>term data:");
                    Serial.println(term_data);
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                    if (_eventCallback)
                    {
                        _processTermString(event, term_data, term_data_count);
                        // Serial.println("Error Callback Triggered");
                        // Serial.print("event->id: ");
                        // Serial.println(event->id);
                        // Serial.print("event->param1: ");
                        // Serial.println(event->param1);

                        _eventCallback(event); // Execute the callback function with the new random number
                    }
                }
                else
                {
                    term_data[term_data_count++] = c;
                }
            }


            if (c == '\n')
            {
                responseBuffer[index] = '\0'; // Null-terminate the string

                if (strstr(responseBuffer, "READY") != NULL)
                {
                    return true;
                }

                index = 0; // Reset index for next response
            }

            if (index >= sizeof(responseBuffer) - 1)
            {
                index = 0; // Reset index if buffer is full
            }
        }
    }

    return false;
}

bool GSM::IsGPRSAttached()
{
    _gsm.println("AT+CGATT?");
    if (bCheckRespose(2000))
    {
        Serial.println(F("GPRS Attached"));
        return true;
    }
    else
    {
        Serial.println(F("GPRS is not Attached"));
        return false;
    }
}

bool GSM::AttachToGPRS()
{
    _gsm.println("AT+CGATT=1");
    if (bCheckRespose(2000))
    {
        Serial.println(F("Attach to GPRS Success"));
        return true;
    }
    else
        return false;
}

bool GSM::DetachToGPRS()
{
    _gsm.println("AT+CGATT=0");
    if (bCheckRespose(2000))
    {
        Serial.println(F("Detach to GPRS Success"));
        return true;
    }
    else
        return false;
}

bool GSM::SetAPN(const char pdp_type[], const char apn[])
{
    _gsm.print("AT+CGDCONT=1,\"");
    _gsm.print(pdp_type);
    _gsm.print("\",\"");
    _gsm.print(apn);
    _gsm.println("\"");

    if (bCheckRespose(2000))
    {
        Serial.println(F("APN Set success"));
        return true;
    }
    else
        return false;
}

bool GSM::ActivatePDP()
{
    _gsm.println("AT+CGACT=1,1");
    if (bCheckRespose(2000))
    {
        Serial.println(F("PDP Activate success"));
        return true;
    }
    else
        return false;
}

bool GSM::ConnectToBroker(const char broker[], int port, const char id[], uint8_t keep_alive, uint16_t clean_session)
{
    _gsm.print("AT+MQTTCONN=\"");
    _gsm.print(broker);
    _gsm.print("\",");
    _gsm.print(port);
    _gsm.print(",\"");
    _gsm.print(id);
    _gsm.print("\",");
    _gsm.print(keep_alive);
    _gsm.print(",");
    _gsm.println(clean_session);

    if (bCheckRespose(2000))
    {
        Serial.println(F("Connected To Broker"));
        return true;
    }
    else
        return false;
}

bool GSM::ConnectToBroker(const char broker[], int port)
{
    char id[10] = "\0";
    sprintf(id, "%d", random(10000, 100000));
    _gsm.print("AT+MQTTCONN=\"");
    _gsm.print(broker);
    _gsm.print("\",");
    _gsm.print(port);
    _gsm.print(",\"");
    _gsm.print(id);
    _gsm.print("\",");
    _gsm.print(120);
    _gsm.print(",");
    _gsm.println(0);

    if (bCheckRespose(2000))
    {
        Serial.println(F("Connected To Broker"));
        return true;
    }
    else
        return false;
}

bool GSM::DisconnectBroker()
{
    _gsm.println("AT+MQTTDISCONN");
    if (bCheckRespose(2000))
    {
        Serial.println(F("MQTT Disconnected From Broker"));
        return true;
    }
    else
        return false;
}
bool GSM::SubscribeToTopic(const char topic[], uint8_t qos, unsigned long timeout)
{
    _gsm.print("AT+MQTTSUB=\"");
    _gsm.print(topic);
    _gsm.print("\",");
    _gsm.print(qos);
    _gsm.print(",");
    _gsm.println(timeout);

    if (bCheckRespose(2000))
    {
        Serial.printf("Subscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}
bool GSM::SubscribeToTopic(const char topic[])
{
    _gsm.print("AT+MQTTSUB=\"");
    _gsm.print(topic);
    _gsm.print("\",");
    _gsm.print(1);
    _gsm.print(",");
    _gsm.println(0);

    if (bCheckRespose(2000))
    {
        Serial.printf("Subscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}

bool GSM::UnsubscribeToTopic(const char topic[])
{
    _gsm.print("AT+MQTTUNSUB=\"");
    _gsm.print(topic);
    _gsm.println("\"");
    if (bCheckRespose(2000))
    {
        Serial.printf("Unsubscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}

// bool GSM::PublishToTopic(const char *topic, uint8_t qos, uint8_t retain, uint8_t dup, const char *msg, uint16_t msg_len, unsigned long timeout){
// AT+MQTTPUB="<topic>",<qos>,<retain>,<dup>,"<message>",<message_len>,<timeout>: Publish a message to an MQTT topic.
// _gsm.print("AT+MQTTPUB=\"");
//     _gsm.print(topic);
//     _gsm.print("\",\"");
//     _gsm.print(msg);
//     _gsm.print("\",");
//     _gsm.print("2");
//     _gsm.print(",");
//     _gsm.print(msg_len);
//     _gsm.print(",");
//     _gsm.println(timeout);

//     if (bCheckRespose(2000))
//     {
//         return true;
//     }
//     else
//         return false;
// }
bool GSM::PublishToTopic(const char topic[], const char msg[])
{
    _gsm.print("AT+MQTTPUB=\"");
    _gsm.print(topic);
    _gsm.print("\",\"");
    _gsm.print(msg);
    _gsm.println("\",2,0,0");

    if (bCheckRespose(2000))
    {
        return true;
    }
    else
        return false;
}

void GSM::errorPrintCME(int ret)
{
    switch (ret)
    {
    case PHONE_FAILURE:
        Serial.printf("PHONE_FAILURE\n");
        break;
    case NO_CONNECT_PHONE:
        Serial.printf("NO_CONNECT_PHONE\n");
        break;
    case PHONE_ADAPTER_LINK_RESERVED:
        Serial.printf("PHONE_ADAPTER_LINK_RESERVED\n");
        break;
    case OPERATION_NOT_ALLOWED:
        Serial.printf("OPERATION_NOT_ALLOWED\n");
        break;
    case OPERATION_NOT_SUPPORTED:
        Serial.printf("OPERATION_NOT_SUPPORTED\n");
        break;
    case PHSIM_PIN_REQUIRED:
        Serial.printf("PHSIM_PIN_REQUIRED\n");
        break;
    case PHFSIM_PIN_REQUIRED:
        Serial.printf("PHFSIM_PIN_REQUIRED\n");
        break;
    case PHFSIM_PUK_REQUIRED:
        Serial.printf("PHFSIM_PUK_REQUIRED\n");
        break;
    case SIM_NOT_INSERTED:
        Serial.printf("SIM_NOT_INSERTED\n");
        break;
    case SIM_PIN_REQUIRED:
        Serial.printf("SIM_PIN_REQUIRED\n");
        break;
    case SIM_PUK_REQUIRED:
        Serial.printf("SIM_PUK_REQUIRED\n");
        break;
    case SIM_FAILURE:
        Serial.printf("SIM_FAILURE\n");
        break;
    case SIM_BUSY:
        Serial.printf("SIM_BUSY\n");
        break;
    case SIM_WRONG:
        Serial.printf("SIM_WRONG\n");
        break;
    case INCORRECT_PASSWORD:
        Serial.printf("INCORRECT_PASSWORD\n");
        break;
    case SIM_PIN2_REQUIRED:
        Serial.printf("SIM_PIN2_REQUIRED\n");
        break;
    case SIM_PUK2_REQUIRED:
        Serial.printf("SIM_PUK2_REQUIRED\n");
        break;
    case MEMORY_FULL:
        Serial.printf("MEMORY_FULL\n");
        break;
    case INVALID_INDEX:
        Serial.printf("INVALID_INDEX\n");
        break;
    case NOT_FOUND:
        Serial.printf("NOT_FOUND\n");
        break;
    case MEMORY_FAILURE:
        Serial.printf("MEMORY_FAILURE\n");
        break;
    case TEXT_LONG:
        Serial.printf("TEXT_LONG\n");
        break;
    case INVALID_CHAR_INTEXT:
        Serial.printf("INVALID_CHAR_INTEXT\n");
        break;
    case DAIL_STR_LONG:
        Serial.printf("DAIL_STR_LONG\n");
        break;
    case INVALID_CHAR_INDIAL:
        Serial.printf("INVALID_CHAR_INDIAL\n");
        break;
    case NO_NET_SERVICE:
        Serial.printf("NO_NET_SERVICE\n");
        break;
    case NETWORK_TIMOUT:
        Serial.printf("NETWORK_TIMOUT\n");
        break;
    case NOT_ALLOW_EMERGENCY:
        Serial.printf("NOT_ALLOW_EMERGENCY\n");
        break;
    case NET_PER_PIN_REQUIRED:
        Serial.printf("NET_PER_PIN_REQUIRED\n");
        break;
    case NET_PER_PUK_REQUIRED:
        Serial.printf("NET_PER_PUK_REQUIRED\n");
        break;
    case NET_SUB_PER_PIN_REQ:
        Serial.printf("NET_SUB_PER_PIN_REQ\n");
        break;
    case NET_SUB_PER_PUK_REQ:
        Serial.printf("NET_SUB_PER_PUK_REQ\n");
        break;
    case SERVICE_PROV_PER_PIN_REQ:
        Serial.printf("SERVICE_PROV_PER_PIN_REQ\n");
        break;
    case SERVICE_PROV_PER_PUK_REQ:
        Serial.printf("SERVICE_PROV_PER_PUK_REQ\n");
        break;
    case CORPORATE_PER_PIN_REQ:
        Serial.printf("CORPORATE_PER_PIN_REQ\n");
        break;
    case CORPORATE_PER_PUK_REQ:
        Serial.printf("CORPORATE_PER_PUK_REQ\n");
        break;
    case PHSIM_PBK_REQUIRED:
        Serial.printf("PHSIM_PBK_REQUIRED\n");
        break;
    case EXE_NOT_SURPORT:
        Serial.printf("EXE_NOT_SURPORT\n");
        break;
    case EXE_FAIL:
        Serial.printf("EXE_FAIL\n");
        break;
    case NO_MEMORY:
        Serial.printf("NO_MEMORY\n");
        break;
    case OPTION_NOT_SURPORT:
        Serial.printf("OPTION_NOT_SURPORT\n");
        break;
    case PARAM_INVALID:
        Serial.printf("PARAM_INVALID\n");
        break;
    case EXT_REG_NOT_EXIT:
        Serial.printf("EXT_REG_NOT_EXIT\n");
        break;
    case EXT_SMS_NOT_EXIT:
        Serial.printf("EXT_SMS_NOT_EXIT\n");
        break;
    case EXT_PBK_NOT_EXIT:
        Serial.printf("EXT_PBK_NOT_EXIT\n");
        break;
    case EXT_FFS_NOT_EXIT:
        Serial.printf("EXT_FFS_NOT_EXIT\n");
        break;
    case INVALID_COMMAND_LINE:
        Serial.printf("INVALID_COMMAND_LINE\n");
        break;
    case GPRS_ILLEGAL_MS_3:
        Serial.printf("GPRS_ILLEGAL_MS_3\n");
        break;
    case GPRS_ILLEGAL_MS_6:
        Serial.printf("GPRS_ILLEGAL_MS_6\n");
        break;
    case GPRS_SVR_NOT_ALLOWED:
        Serial.printf("GPRS_SVR_NOT_ALLOWED\n");
        break;
    case GPRS_PLMN_NOT_ALLOWED:
        Serial.printf("GPRS_PLMN_NOT_ALLOWED\n");
        break;
    case GPRS_LOCATION_AREA_NOT_ALLOWED:
        Serial.printf("GPRS_LOCATION_AREA_NOT_ALLOWED\n");
        break;
    case GPRS_ROAMING_NOT_ALLOWED:
        Serial.printf("GPRS_ROAMING_NOT_ALLOWED\n");
        break;
    case GPRS_OPTION_NOT_SUPPORTED:
        Serial.printf("GPRS_OPTION_NOT_SUPPORTED\n");
        break;
    case GPRS_OPTION_NOT_SUBSCRIBED:
        Serial.printf("GPRS_OPTION_NOT_SUBSCRIBED\n");
        break;
    case GPRS_OPTION_TEMP_ORDER_OUT:
        Serial.printf("GPRS_OPTION_TEMP_ORDER_OUT\n");
        break;
    case GPRS_PDP_AUTHENTICATION_FAILURE:
        Serial.printf("GPRS_PDP_AUTHENTICATION_FAILURE\n");
        break;
    case GPRS_INVALID_MOBILE_CLASS:
        Serial.printf("GPRS_INVALID_MOBILE_CLASS\n");
        break;
    case GPRS_UNSPECIFIED_GPRS_ERROR:
        Serial.printf("GPRS_UNSPECIFIED_GPRS_ERROR\n");
        break;
    case SIM_VERIFY_FAIL:
        Serial.printf("SIM_VERIFY_FAIL\n");
        break;
    case SIM_UNBLOCK_FAIL:
        Serial.printf("SIM_UNBLOCK_FAIL\n");
        break;
    case SIM_CONDITION_NO_FULLFILLED:
        Serial.printf("SIM_CONDITION_NO_FULLFILLED\n");
        break;
    case SIM_UNBLOCK_FAIL_NO_LEFT:
        Serial.printf("SIM_UNBLOCK_FAIL_NO_LEFT\n");
        break;
    case SIM_VERIFY_FAIL_NO_LEFT:
        Serial.printf("SIM_VERIFY_FAIL_NO_LEFT\n");
        break;
    case SIM_INVALID_PARAMETER:
        Serial.printf("SIM_INVALID_PARAMETER\n");
        break;
    case SIM_UNKNOW_COMMAND:
        Serial.printf("SIM_UNKNOW_COMMAND\n");
        break;
    case SIM_WRONG_CLASS:
        Serial.printf("SIM_WRONG_CLASS\n");
        break;
    case SIM_TECHNICAL_PROBLEM:
        Serial.printf("SIM_TECHNICAL_PROBLEM\n");
        break;
    case SIM_CHV_NEED_UNBLOCK:
        Serial.printf("SIM_CHV_NEED_UNBLOCK\n");
        break;
    case SIM_NOEF_SELECTED:
        Serial.printf("SIM_NOEF_SELECTED\n");
        break;
    case SIM_FILE_UNMATCH_COMMAND:
        Serial.printf("SIM_FILE_UNMATCH_COMMAND\n");
        break;
    case SIM_CONTRADICTION_CHV:
        Serial.printf("SIM_CONTRADICTION_CHV\n");
        break;
    case SIM_CONTRADICTION_INVALIDATION:
        Serial.printf("SIM_CONTRADICTION_INVALIDATION\n");
        break;
    case SIM_MAXVALUE_REACHED:
        Serial.printf("SIM_MAXVALUE_REACHED\n");
        break;
    case SIM_PATTERN_NOT_FOUND:
        Serial.printf("SIM_PATTERN_NOT_FOUND\n");
        break;
    case SIM_FILEID_NOT_FOUND:
        Serial.printf("SIM_FILEID_NOT_FOUND\n");
        break;
    case SIM_STK_BUSY:
        Serial.printf("SIM_STK_BUSY\n");
        break;
    case SIM_UNKNOW:
        Serial.printf("SIM_UNKNOW\n");
        break;
    case SIM_PROFILE_ERROR:
        Serial.printf("SIM_PROFILE_ERROR\n");
        break;
    default:
        break;
    }
}