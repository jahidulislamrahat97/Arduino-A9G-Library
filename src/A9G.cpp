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

//https://github.com/jahidulislamrahat97/Arduino-A9G-Library

#include "A9G.h"

GSM::GSM(HardwareSerial &A9G, bool debug)
    : _gsm(A9G), _debug(debug), _maxWaitTimeMS(MAX_WAIT_TIME_MS)
{
    is_mqtt_error = false;
    is_mqtt = false;
    is_sms = false;
    is_mqtt_error = false;
    sms_read = false;
    new_command_received = false;
    complete_data = false;
    received = false;
    gsm_config = false;
}

void GSM::init(uint32_t baudRate)
{
    _gsm.begin(baudRate);
}

void GSM::vProcessIncomingData()
{
    char c;
    int term_length = 0;
    bool term_started = 0, term_ended = 0;

    bool command_started = 0, command_ended = 0;

    uint8_t comma_count = 0;
    // int f = 0;

    uint8_t dquote = 0, j = 0, n = 0;
    bool is_msg = 0, is_number = 0;
    char prev_char;
    // Serial.println(F("Incoming data"));

    while (_gsm.available())
    {
        if (term_length >= MAX_TERM_SIZE)
        {
            if (term_length >= MAX_TERM_SIZE)
            {
                term[term_length] = '\0';
                term_length = 0;
            }

            if (_debug)
            {
                Serial.print(F("term_length, gps_term_length: "));
                Serial.print(term_length);
                Serial.print(F(", "));
            }
            // break;
        }

        c = char(_gsm.read());
        Serial.print(c);

        if (c == '+' && !term_started && !term_ended)
        {
            term_started = 1;
            term_ended = 0;
            continue;
        }

        if (c == ':' && !term_ended)
        {
            term[term_length] = '\0';
            term_ended = 1;
            continue;
        }

        if (term_started && !term_ended)
        {
            term[term_length++] = c;
        }

        if (term_started && term_ended && !command_started)
        {
            Serial.println(term);
            if (term[0] == 'M' || term[0] == 'C')
            {

                is_mqtt = (!strcmp(term, "MQTTPUBLISH"));
                is_sms = (!strcmp(term, "CMGL"));
                sms_read = (!strcmp(term, "CIEV"));
                is_mqtt_error = ((!strcmp(term, "CME ERROR")) || (!strcmp(term, "MQTTDISCONNECTED")));
                command_started = (is_sms || is_mqtt);
            }
        }

        if (term_ended && command_started && !command_ended)
        {
            delay(1);
            // Serial.print(c);
            if (is_mqtt)
            {
                // command[j++] = c;
                // command[j] = '\0';
                if (c == ',' and comma_count < 3)
                {
                    comma_count++;
                    continue;
                }

                if (c == '\r')
                {
                    // Serial.println(F("#"));
                    command[j] = '\0';

                    command_ended = 1;
                    new_command_received = 1;
                    continue;
                }

                if (comma_count >= 3)
                {
                    // Serial.print(c);
                    command[j++] = c;
                    command[j] = '\0';
                }
            }
            else if (is_sms)
            {
                // Serial.print(c);
                if (c == '"')
                {
                    dquote++;
                    // continue;
                }
                else if (c == '\n' && prev_char == '\r')
                {
                    is_msg = 1;
                    continue;
                }

                if (dquote == 3 and !is_number)
                {
                    if (prev_char == '8' and c == '8')
                    {
                        is_number = 1;
                        continue;
                    }
                }

                if (is_number && c == '"')
                {
                    is_number = 0;
                    number[n] = '\0';
                    // Serial.print(F("Number: "));
                    // Serial.println(number);
                    // continue;
                }

                if (is_msg && c == '\r')
                {
                    is_msg = 0;
                    command[j] = '\0';
                    command_started = 0;
                    new_command_received = 1;
                    // Serial.print(F("Command: "));
                    // Serial.println(command);
                    command_ended = 1;
                    // break;
                }

                if (is_msg)
                {
                    command[j++] = c;
                }

                if (is_number)
                {
                    number[n++] = c;
                }

                prev_char = c;
            }
        }
        yield();
    }
}

bool GSM::bCheckRespose(const int timeout)
{
    char response[150];                    // Assuming the response won't exceed 50 characters
    memset(response, 0, sizeof(response)); // Initialize response to all zeros
    long int start_time = millis();

    int idx = 0; // Index to keep track of response characters

    while ((millis() - start_time) < timeout)
    {
        while (_gsm.available())
        {
            char c = _gsm.read();
            response[idx++] = c;

            // Check if "OK" is present in the received characters
            if (strstr(response, "OK"))
            {
                return true;
            }

            // Serial.print(response);
            // Check for buffer overflow
            if (idx >= sizeof(response) - 1)
            {
                Serial.println("Response buffer overflow!");
                return false;
            }
        }
    }

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

    _gsm.println("AT");

    // need make this function break until it gets ready command
    while (1)
    {
        while (_gsm.available())
        {
            char c = _gsm.read();
            responseBuffer[index++] = c;
            // Serial.print(responseBuffer);

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

// void GSM::vReadResidualData(bool print)
// {
//     char c;
//     while (_gsm.available() > 0)
//     {
//         c = _gsm.read();
//         if (print)
//             Serial.print(c);
//         delay(2);
//     }
//     if (print)
//         Serial.println();
// }

// void GSM::vWaitForResponse(unsigned long timeout)
// {
//     unsigned long tic = millis();
//     while (!_gsm.available())
//     {
//         if ((millis() - tic) >= MAX_WAIT_TIME_MS)
//         {
//             if (_debug)
//             {
//                 Serial.println(F("Restarting....!"));
//             }

//             vGSMReset();
//             break;
//         }
//     }
// }

// bool GSM::bReadResponse()
// {
//  // Serial.println(F("AT OK command"));
//     char response[MAX_AT_RESPONSE_SIZE], c;     // A response string and a character variable
//     bool ok_rcv = false;                     // Initial state OK_RECEIVE is "False"
//     int i = 0;                               // String indices
//     // Serial.println(F("Available before while"));
//     vWaitForResponse(100);                 // while no response available, do nothing

//     while(_gsm.available())                   // while responses available, do->>>
//     {

//         // Serial.println(F("Available"));
//         delay(2);
//         if(i >= MAX_AT_RESPONSE_SIZE)
//         {
//             response[i] = '\0';
//             i = 0;
//         }

//         c = _gsm.read();                        // read response in character by character
//         response[i++] = c;                     // store it as a string
//         if(_debug)
//             Serial.print(c);
//         /*The ultimate checking loop*/
//         if(!ok_rcv && response[i - 1] == 'O')  // check if the function is in false state AND the previous character found is "O"
//         {
//             if(response[i] == 'K')               // check if the next character is "K"
//             {
//                 ok_rcv = true;                     // if so, Set the state as "True"
//             }
//         }
//     }
//     response[i] = '\0';                      // End of the string
//     // Serial.print("bReadResponse");

//     Serial.println(response);
//     if(_debug)
//         Serial.println(response);

//     return ok_rcv;
// }
