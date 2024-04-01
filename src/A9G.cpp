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
 * 
 * Written by Jahidul Islam Rahat
 *
 * 
 * @section license License
 * 
 * MIT license, (see LICENSE)
 *
 */

// https://github.com/jahidulislamrahat97/Arduino-A9G-Library

#include "A9G.h"

GSM::GSM(bool debug)
    : _debug(debug), _maxWaitTimeMS(MAX_WAIT_TIME_MS)
{

}

void GSM::init(Stream *gsm)
{
    _gsm = gsm;
}

void GSM::Test(char *data)
{
    _gsm->println(data);
    while (1)
    {
        while (_gsm->available())
        {
            Serial.write(_gsm->read());
        }
    }
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
    // Serial.println("\nCore >> _processTermString(): ");
    // Serial.print("event->id: ");
    // Serial.println(event->id);
    // Serial.print("data: ");
    // Serial.println(data);
    // Serial.print("data_len: ");
    // Serial.println(data_len);

    uint8_t comma_count = 0;
    uint8_t message_count = 0;

    if (event->id == EVENT_MQTTPUBLISH)
    {
        uint8_t topic_count = 0;
        for (int i = 0; i < data_len; i++)
        {
            if (data[i] == ',' && comma_count < 3)
            {
                comma_count++;
                continue;
            }
            if (comma_count == 1)
            {
                event->topic[topic_count++] = data[i];
                // Serial.print(data[i]);
            }
            if(comma_count >= 3){
                event->message[message_count++] = data[i];
            }
        }
        event->message[message_count++] = '\0';
        event->topic[topic_count++] = '\0';
    }
    else if (event->id == TERM_CME)
    {
        event->error = atoi(data);
    }
    else if(event->id == TERM_CMTI){
        char temp[5] = "\0";
        int j =0;
        for (int i = 0; i <= data_len; i++)
        {
            if (data[i] == ',')
            {
                comma_count++;
                continue;
            }
            if(comma_count ==1){
                temp[j++] = data[i];
            }
        }
        temp[j++] = '\0';

        // Serial.print("Message Index:");
        // Serial.println(temp);

        delay(100);
        ReadMessage(atoi(temp));

    }
    else if(event->id == TERM_CMGR){ //sms read 
        char message[100] = "\0";
        int j =0;
        bool sms = 0;
       
        while(_gsm->available()){
            char c = _gsm->read();
            
            if(c=='\n'){
                sms = 1;
                continue;
            }
            if(c=='\r'){
                sms = 0;
                event->message[j] = '\0';
                break;
            }
            if(sms){
                event->message[j++] = c;
                // Serial.print(event->message);
            }
            
        }

        uint8_t qout_count = 0;
        uint8_t number_count = 0;
        uint8_t date_time_count = 0;
        for (int i = 0; i <= data_len; i++)
        {
            if (data[i] == '"')
            {
                
                qout_count++;
                // Serial.print("###: ");
                // Serial.println(qout_count);
                continue;
            }
            if (qout_count == 3)
            {
                event->number[number_count++] = data[i];
                // Serial.print(event->number);
            }
            if(qout_count == 5){
                event->date_time[date_time_count++] = data[i];
                // Serial.print(event->date_time);
            }
        }
        event->number[number_count++] = '\0';
        event->date_time[date_time_count++] = '\0';

        // Serial.print("Message:");
        // Serial.println(event->message);
        // Serial.print("Number:");
        // Serial.println(event->number);
        // Serial.print("Date & Time:");
        // Serial.println(event->date_time);

    }
    else if(event->id == EVENT_CSQ){
        char buffer[10] = "\0";
        for(int i = 0; i<=data_len; i++){
            if(data[i] == ','){
                buffer[i] = '\0';
                break;
            }
            else{
                buffer[i] = data[i];
            }
        }
        event->param1 =atoi(buffer);
    }
    else if(event->id == EVENT_IMEI){
        strcpy(event->param2,data);
    }
    else if(event->id == EVENT_CCID){
        strcpy(event->param2,data);
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
    char term[MAX_TERM_SIZE];

    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));

    while (_gsm->available())
    {
        char c = _gsm->read();
        // Serial.print(c);

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
            term[term_length] = '\0';
            term_ended = 1;
            continue;
        }

        if (term_started && !term_ended)
        {
            term[term_length++] = c;
            // Serial.print("term: ");
            // Serial.println(c);
        }

        if (term_started && term_ended && !term_data_started)
        {
            // Serial.print(">>>term: ");
            // Serial.println(term);

            int term_id = _checkTermFromString(term);
            // Serial.print("term id: ");
            // Serial.println(term_id);

            if (term_id == TERM_NONE || term_id == TERM_MAX)
            {
                // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                // Serial.println("***** Terminating this term here ******");
                term_started = 0;
                term_ended = 0;
                term_length = 0;
                term_data_started = 0;
                break;
            }
            else
            {
                term_data_started = 1;
                // Serial.println("***** Term Accepted ******");
                event->id = static_cast<Event_ID_t>(term_id);
            }
        }
        if (term_ended && term_data_started)
        {
            if (c == '\r')
            {
                // Serial.print(">>>term data:");
                // Serial.println(term_data);
                term_started = 0;
                term_ended = 0;
                term_length = 0;
                term_data_started = 0;

                event->message[0] = '\0';
                if (_eventCallback)
                {
                    _processTermString(event, term_data, term_data_count);
                    // Serial.println("Error Callback Triggered");
                    // Serial.print("event->id: ");
                    // Serial.println(event->id);
                    // Serial.print("event->param1: ");
                    // Serial.println(event->message);
                    


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
    yield();
}

bool GSM::_checkResponse(const int timeout)
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
    char term[MAX_TERM_SIZE];

    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));

    while ((millis() - start_time) < timeout)
    {
        while (_gsm->available())
        {
            char c = _gsm->read();
            response[idx++] = c;
            // Serial.print(c);

            if (c == '+' && !term_started && !term_ended)
            {
                // Serial.println("########## New Term!");
                term_started = 1;
                term_ended = 0;
                term_data_count = 0;
                continue;
            }

            if ((c == ':') && !term_ended)
            {
                // Serial.println("********** Term Stop!");
                term[term_length] = '\0';
                term_ended = 1;
                continue;
            }

            if (term_started && !term_ended)
            {
                if (c == '\r')
                {
                    term_started = 0;
                    term_length = 0;
                }
                else
                {
                    term[term_length++] = c;
                }

                // Serial.print("term: ");
                // Serial.println(c);
            }

            if (term_started && term_ended && !term_data_started)
            {
                // Serial.print(">>>term: \"");
                // Serial.print(term);
                // Serial.println("\"");

                int term_id = _checkTermFromString(term);
                // Serial.print("term id: ");
                // Serial.println(term_id);

                if (term_id == TERM_GPSRD || term_id == TERM_CMGS || term_id == TERM_CMGL || term_id == TERM_CIEV || term_id == TERM_NONE || term_id == TERM_MAX)
                {
                    // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                    // Serial.println("***** Terminating this term here ******");
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                }
                else
                {
                    term_data_started = 1;
                    // Serial.println("***** Term Accepted ******");
                    event->id = static_cast<Event_ID_t>(term_id);
                }
            }
            if (term_ended && term_data_started)
            {
                if (c == '\r')
                {
                    // Serial.print(">>>term data:");
                    // Serial.println(term_data);
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                    event->message[0] = '\0';
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
    _gsm->println("AT");
    if (_checkResponse(2000))
    {
        if (_debug)
        {
            Serial.println(F("GSM Ready"));
            return true;
        }
    }
    else
        return false;
}


bool GSM::waitForReady()
{
    char responseBuffer[100]; // Adjust the size as per your needs
    int index = 0;


    int term_length = 0;
    int term_data_count = 0;
    bool term_started = false, term_ended = false;
    bool term_data_started = false, term_data_ended = false;
    char term_data[MAX_MSG_SIZE];
    char term[MAX_TERM_SIZE];

    A9G_Event_t *event = NULL;
    event = (A9G_Event_t *)malloc(sizeof(A9G_Event_t));


    _gsm->println("AT");
    // need make this function break until it gets ready command
    while (1)
    {
        if(_gsm->available())
        {
            char c = _gsm->read();
            responseBuffer[index++] = c;
            // Serial.print(responseBuffer);
            // Serial.print(c);


            if (c == '+' && !term_started && !term_ended)
            {
                // Serial.println("########## New Term!");
                term_started = 1;
                term_ended = 0;
                term_data_count = 0;
                continue;
            }

            if ((c == ':') && !term_ended)
            {
                // Serial.println("********** Term Stop!");
                term[term_length] = '\0';
                term_ended = 1;
                continue;
            }

            if (term_started && !term_ended)
            {
                if (c == '\r')
                {
                    term_started = 0;
                    term_length = 0;
                }
                else
                {
                    term[term_length++] = c;
                }
                // Serial.print("term: ");
                // Serial.println(c);
            }

            if (term_started && term_ended && !term_data_started)
            {
                // Serial.print(">>>term: ");
                // Serial.println(term);

                int term_id = _checkTermFromString(term);
                // Serial.print("term id: ");
                // Serial.println(term_id);

                if (term_id == TERM_GPSRD || term_id == TERM_CMGS || term_id == TERM_CMGL || term_id == TERM_CMGR || term_id == TERM_CIEV || term_id == TERM_NONE || term_id == TERM_MAX)
                {
                    // Terminate these term here, we will not process these type in this function. for cemplexity issue.
                    // Serial.println("***** Terminating this term here ******");
                    term_started = 0;
                    term_ended = 0;
                    term_length = 0;
                    term_data_started = 0;
                }
                else
                {
                    term_data_started = 1;
                    // Serial.println("***** Term Accepted ******");
                    event->id = static_cast<Event_ID_t>(term_id);
                }
            }
            if (term_ended && term_data_started)
            {
                if (c == '\r')
                {
                    // Serial.print(">>>term data:");
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
                if (strstr(responseBuffer, "NO SIM CARD") != NULL)
                {
                    Serial.println("NO SIM CARD");
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

// AT+CSQ: Check signal strength (in dBm).
// AT+CCID: Read the ICCID (Integrated Circuit Card Identifier) of the SIM card.

void GSM::ReadIMEI(){
    _gsm->println("AT+EGMR=2,7");
    _checkResponse(1000);
}
/**
 * @brief 
 * @todo need to implement: issue.
 * 
 */
void GSM::ReadCSQ(){
    _gsm->println("AT+CSQ");
    _checkResponse(1000);
}
void GSM::ReadCCID(){
    _gsm->println("AT+CCID");
    _checkResponse(1000);
}

bool GSM::IsGPRSAttached()
{
    _gsm->println("AT+CGATT?");
    if (_checkResponse(2000))
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GSM::AttachToGPRS()
{
    _gsm->println("AT+CGATT=1");
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::DetachToGPRS()
{
    _gsm->println("AT+CGATT=0");
    if (_checkResponse(2000))
    {;
        return true;
    }
    else
        return false;
}

bool GSM::SetAPN(const char pdp_type[], const char apn[])
{
    _gsm->print("AT+CGDCONT=1,\"");
    _gsm->print(pdp_type);
    _gsm->print("\",\"");
    _gsm->print(apn);
    _gsm->println("\"");

    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::ActivatePDP()
{
    _gsm->println("AT+CGACT=1,1");
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}


bool GSM::ConnectToBroker(const char broker[], int port, const char user[], const char pass[], const char id[], uint8_t keep_alive, uint16_t clean_session)
{
    _gsm->print("AT+MQTTCONN=\"");
    _gsm->print(broker);
    _gsm->print("\",");
    _gsm->print(port);
    _gsm->print(",\"");
    _gsm->print(id);
    _gsm->print("\",");
    _gsm->print(keep_alive);
    _gsm->print(",");
    _gsm->print(clean_session);
    _gsm->print(",\"");
    _gsm->print(user);
    _gsm->print("\",\"");
    _gsm->print(pass);
    _gsm->println("\"");

    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::ConnectToBroker(const char broker[], int port, const char id[], uint8_t keep_alive, uint16_t clean_session)
{
    _gsm->print("AT+MQTTCONN=\"");
    _gsm->print(broker);
    _gsm->print("\",");
    _gsm->print(port);
    _gsm->print(",\"");
    _gsm->print(id);
    _gsm->print("\",");
    _gsm->print(keep_alive);
    _gsm->print(",");
    _gsm->println(clean_session);

    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::ConnectToBroker(const char broker[], int port)
{
    char id[10] = "\0";
    sprintf(id, "%d", random(10000, 100000));
    _gsm->print("AT+MQTTCONN=\"");
    _gsm->print(broker);
    _gsm->print("\",");
    _gsm->print(port);
    _gsm->print(",\"");
    _gsm->print(id);
    _gsm->print("\",");
    _gsm->print(120);
    _gsm->print(",");
    _gsm->println(0);

    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::DisconnectBroker()
{
    _gsm->println("AT+MQTTDISCONN");
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}
bool GSM::SubscribeToTopic(const char topic[], uint8_t qos, unsigned long timeout)
{
    _gsm->print("AT+MQTTSUB=\"");
    _gsm->print(topic);
    _gsm->print("\",");
    _gsm->print(qos);
    _gsm->print(",");
    _gsm->println(timeout);

    if (_checkResponse(2000))
    {
        Serial.printf("Subscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}
bool GSM::SubscribeToTopic(const char topic[])
{
    _gsm->print("AT+MQTTSUB=\"");
    _gsm->print(topic);
    _gsm->print("\",");
    _gsm->print(1);
    _gsm->print(",");
    _gsm->println(0);

    if (_checkResponse(2000))
    {
        Serial.printf("Subscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}

bool GSM::UnsubscribeToTopic(const char topic[])
{
    _gsm->print("AT+MQTTUNSUB=\"");
    _gsm->print(topic);
    _gsm->println("\"");
    if (_checkResponse(2000))
    {
        Serial.printf("Unsubscribe To Topic:\"%s\"  success\n", topic);
        return true;
    }
    else
        return false;
}


bool GSM::PublishToTopic(const char topic[], const char msg[])
{
    _gsm->print("AT+MQTTPUB=\"");
    _gsm->print(topic);
    _gsm->print("\",\"");
    _gsm->print(msg);
    _gsm->println("\",2,0,0");

    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}






bool GSM::ActivateTE()
{
    _gsm->println(F("AT+CNMI=0,1,0,0,0"));
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::SetFormatReading(bool mode)
{
    _gsm->print(F("AT+CMGF="));
    _gsm->println(mode);
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool GSM::SetMessageStorageUnit()
{
    _gsm->println(F("AT+CPMS=\"ME\",\"ME\",\"ME\""));
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

bool SetMessageStorageUnit(SMS_Storage_Type_t mem1, SMS_Storage_Type_t mem2, SMS_Storage_Type_t mem3)
{
    char m1[3] = "ME";
    char m2[3] = "ME";
    char m3[3] = "ME";

    if(mem1 == SMS_STORAGE_TYPE_SM)
    {
        strcpy(m1, "SM");
    }
    else if(mem1 == SMS_STORAGE_TYPE_MT)
    {
        strcpy(m1, "MT");
    }
    
    if(mem2 == SMS_STORAGE_TYPE_SM)
    {
        strcpy(m2, "SM");
    }
    else if(mem2 == SMS_STORAGE_TYPE_MT)
    {
        strcpy(m2, "MT");
    }

    if(mem3 == SMS_STORAGE_TYPE_SM)
    {
        strcpy(m3, "SM");
    }
    else if(mem3 == SMS_STORAGE_TYPE_MT)
    {
        strcpy(m3, "MT");
    }

    char command[24];
    sprintf("AT+CPMS=\"%s\",\"%s\",\"%s\"", m1, m2, m3);

    _gsm->println(command);
    if (_checkResponse(2000))
    {
        return true;
    }
    else
        return false;
}

void GSM::CheckMessageStorageUnit(){
    _gsm->println(F("AT+CPBS?"));
}


void GSM::ReadMessage(uint8_t index){
    _gsm->print("AT+CMGR=");
    _gsm->println(index);
}

void GSM::DeleteMessage(uint8_t index,Message_Type_t type){
    _gsm->print(F("AT+CMGD="));
    _gsm->print(index);
    _gsm->print(F(","));
    _gsm->println(type);
}

//still some issue did get responce poperly
bool GSM::bSendMessage(const char number[], const char message[])
{
    _gsm->println(F("AT+CMGF=1"));
    delay(100);
    _gsm->print(F("AT+CMGS=\""));
    _gsm->print(number);
    _gsm->print(F("\"\r\n"));
    delay(100);
    _gsm->print(message);
    _gsm->write(0x1a);
    delay(100);
    _gsm->println("AT");


    if(_checkResponse(2000)){
        return true;
    }
    else{
        return false;
    }
}

void GSM::vSendMessage(const char number[], const char message[])
{

    _gsm->print(F("AT+CMGS=\""));
    _gsm->print(number);
    _gsm->print(F("\"\r\n"));
    delay(100);
    _gsm->print(message);
    _gsm->println((char)26);
    delay(100);

    _gsm->println("AT");
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

void GSM::errorPrintCMS(int ret)
{

    switch (ret)
    {
    case UNASSIGNED_NUM:
        Serial.printf("UNASSIGNED_NUM\n");
        break;
    case OPER_DETERM_BARR:
        Serial.printf("OPER_DETERM_BARR\n");
        break;
    case CALL_BARRED:
        Serial.printf("CALL_BARRED\n");
        break;
    case SM_TRANS_REJE:
        Serial.printf("SM_TRANS_REJE\n");
        break;
    case DEST_OOS:
        Serial.printf("DEST_OOS\n");
        break;
    case UNINDENT_SUB:
        Serial.printf("UNINDENT_SUB\n");
        break;
    case FACILIT_REJE:
        Serial.printf("FACILIT_REJE\n");
        break;
    case UNKONWN_SUB:
        Serial.printf("UNKONWN_SUB\n");
        break;
    case NW_OOO:
        Serial.printf("NW_OOO\n");
        break;
    case TMEP_FAIL:
        Serial.printf("TMEP_FAIL\n");
        break;
    case CONGESTION:
        Serial.printf("CONGESTION\n");
        break;
    case RES_UNAVAILABLE:
        Serial.printf("RES_UNAVAILABLE\n");
        break;
    case REQ_FAC_NOT_SUB:
        Serial.printf("REQ_FAC_NOT_SUB\n");
        break;
    case RFQ_FAC_NOT_IMP:
        Serial.printf("RFQ_FAC_NOT_IMP\n");
        break;
    case INVALID_SM_TRV:
        Serial.printf("INVALID_SM_TRV\n");
        break;
    case INVALID_MSG:
        Serial.printf("INVALID_MSG\n");
        break;
    case INVALID_MAND_INFO:
        Serial.printf("INVALID_MAND_INFO\n");
        break;
    case MSG_TYPE_ERROR:
        Serial.printf("MSG_TYPE_ERROR\n");
        break;
    case MSG_NOT_COMP:
        Serial.printf("MSG_NOT_COMP\n");
        break;
    case INFO_ELEMENT_ERROR:
        Serial.printf("INFO_ELEMENT_ERROR\n");
        break;
    case PROT_ERROR:
        Serial.printf("PROT_ERROR\n");
        break;
    case IW_UNSPEC:
        Serial.printf("IW_UNSPEC\n");
        break;
    case TEL_IW_NOT_SUPP:
        Serial.printf("TEL_IW_NOT_SUPP\n");
        break;
    case SMS_TYPE0_NOT_SUPP:
        Serial.printf("SMS_TYPE0_NOT_SUPP\n");
        break;
    case CANNOT_REP_SMS:
        Serial.printf("CANNOT_REP_SMS\n");
        break;
    case UNSPEC_TP_ERROR:
        Serial.printf("UNSPEC_TP_ERROR\n");
        break;
    case DCS_NOT_SUPP:
        Serial.printf("DCS_NOT_SUPP\n");
        break;
    case MSG_CLASS_NOT_SUPP:
        Serial.printf("MSG_CLASS_NOT_SUPP\n");
        break;
    case UNSPEC_TD_ERROR:
        Serial.printf("UNSPEC_TD_ERROR\n");
        break;
    case CMD_CANNOT_ACT:
        Serial.printf("CMD_CANNOT_ACT\n");
        break;
    case CMD_UNSUPP:
        Serial.printf("CMD_UNSUPP\n");
        break;
    case UNSPEC_TC_ERROR:
        Serial.printf("UNSPEC_TC_ERROR\n");
        break;
    case TPDU_NOT_SUPP:
        Serial.printf("TPDU_NOT_SUPP\n");
        break;
    case SC_BUSY:
        Serial.printf("SC_BUSY\n");
        break;
    case NO_SC_SUB:
        Serial.printf("NO_SC_SUB\n");
        break;
    case SC_SYS_FAIL:
        Serial.printf("SC_SYS_FAIL\n");
        break;
    case INVALID_SME_ADDR:
        Serial.printf("INVALID_SME_ADDR\n");
        break;
    case DEST_SME_BARR:
        Serial.printf("DEST_SME_BARR\n");
        break;
    case SM_RD_SM:
        Serial.printf("SM_RD_SM\n");
        break;
    case TP_VPF_NOT_SUPP:
        Serial.printf("TP_VPF_NOT_SUPP\n");
        break;
    case TP_VP_NOT_SUPP:
        Serial.printf("TP_VP_NOT_SUPP\n");
        break;
    case D0_SIM_SMS_STO_FULL:
        Serial.printf("D0_SIM_SMS_STO_FULL\n");
        break;
    case NO_SMS_STO_IN_SIM:
        Serial.printf("NO_SMS_STO_IN_SIM\n");
        break;
    case ERR_IN_MS:
        Serial.printf("ERR_IN_MS\n");
        break;
    case MEM_CAP_EXCCEEDED:
        Serial.printf("MEM_CAP_EXCCEEDED\n");
        break;
    case SIM_APP_TK_BUSY:
        Serial.printf("SIM_APP_TK_BUSY\n");
        break;
    case SIM_DATA_DL_ERROR:
        Serial.printf("SIM_DATA_DL_ERROR\n");
        break;
    case UNSPEC_ERRO_CAUSE:
        Serial.printf("UNSPEC_ERRO_CAUSE\n");
        break;
    case ME_FAIL:
        Serial.printf("ME_FAIL\n");
        break;
    case SMS_SERVIEC_RESERVED:
        Serial.printf("SMS_SERVIEC_RESERVED\n");
        break;
    case OPER_NOT_SUPP:
        Serial.printf("OPER_NOT_SUPP\n");
        break;
    case INVALID_PDU_PARAM:
        Serial.printf("INVALID_PDU_PARAM\n");
        break;
    case INVALID_TXT_PARAM:
        Serial.printf("INVALID_TXT_PARAM\n");
        break;
    case SIM_NOT_INSERT:
        Serial.printf("SIM_NOT_INSERT\n");
        break;
    case CMS_SIM_PIN_REQUIRED:
        Serial.printf("SIM_PIN_REQUIRED\n");
        break;
    case PH_SIM_PIN_REQUIRED:
        Serial.printf("PH_SIM_PIN_REQUIRED\n");
        break;
    case SIM_FAIL:
        Serial.printf("SIM_FAIL\n");
        break;
    case CMS_SIM_BUSY:
        Serial.printf("SIM_BUSY\n");
        break;
    case CMS_SIM_WRONG:
        Serial.printf("SIM_WRONG\n");
        break;
    case CMS_SIM_PUK_REQUIRED:
        Serial.printf("SIM_PUK_REQUIRED\n");
        break;
    case CMS_SIM_PIN2_REQUIRED:
        Serial.printf("SIM_PIN2_REQUIRED\n");
        break;
    case CMS_SIM_PUK2_REQUIRED:
        Serial.printf("SIM_PUK2_REQUIRED\n");
        break;
    case MEM_FAIL:
        Serial.printf("MEM_FAIL\n");
        break;
    case INVALID_MEM_INDEX:
        Serial.printf("INVALID_MEM_INDEX\n");
        break;
    case MEM_FULL:
        Serial.printf("MEM_FULL\n");
        break;
    case SCA_ADDR_UNKNOWN:
        Serial.printf("SCA_ADDR_UNKNOWN\n");
        break;
    case NO_NW_SERVICE:
        Serial.printf("NO_NW_SERVICE\n");
        break;
    case NW_TIMEOUT:
        Serial.printf("NW_TIMEOUT\n");
        break;
    case NO_CNMA_ACK_EXPECTED:
        Serial.printf("NO_CNMA_ACK_EXPECTED\n");
        break;
    case UNKNOWN_ERROR:
        Serial.printf("UNKNOWN_ERROR\n");
        break;
    case USER_ABORT:
        Serial.printf("USER_ABORT\n");
        break;
    case UNABLE_TO_STORE:
        Serial.printf("UNABLE_TO_STORE\n");
        break;
    case INVALID_STATUS:
        Serial.printf("INVALID_STATUS\n");
        break;
    case INVALID_ADDR_CHAR:
        Serial.printf("INVALID_ADDR_CHAR\n");
        break;
    case INVALID_LEN:
        Serial.printf("INVALID_LEN\n");
        break;
    case INVALID_PDU_CHAR:
        Serial.printf("INVALID_PDU_CHAR\n");
        break;
    case INVALID_PARA:
        Serial.printf("INVALID_PARA\n");
        break;
    case INVALID_LEN_OR_CHAR:
        Serial.printf("INVALID_LEN_OR_CHAR\n");
        break;
    case INVALID_TXT_CHAR:
        Serial.printf("INVALID_TXT_CHAR\n");
        break;
    case TIMER_EXPIRED:
        Serial.printf("TIMER_EXPIRED\n");
        break;
    default:
        break;
    }
}