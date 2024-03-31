/*********************************************************************************
   If this code works, it was written by Jahidul Islam Rahat.
   If not, I don't know who wrote it.
   :) xD

   Author: Jahidul Islam Rahat.
   Date: 25 March 2024.
*********************************************************************************/


#include <Arduino.h>
#include <A9G.h>

#define BROKER_NAME     "broker.hivemq.com"
#define PORT            1883
#define USER            ""
#define PASS            ""
#define UNIQUE_ID       "dknvkfdnvj"
#define PUB_TOPIC       "IoT/PUB"
#define SUB_TOPIC       "IoT/SUB"
#define CLEAN_SEASSION  0
#define KEEP_ALIVE      120

HardwareSerial A9G(2);
GSM gsm(1);

const int gsm_pin = 15;
unsigned long tic = millis();
char data[50] = "Hello IoT";



void eventDispatch(A9G_Event_t *event) {
  switch (event->id) {
    case EVENT_MQTTPUBLISH:
      Serial.print("Topic: ");
      Serial.println(event->topic);
      Serial.printf("message: %s\n", event->message);
      break;

    case EVENT_NEW_SMS_RECEIVED:
      Serial.print("Number: ");
      Serial.println(event->number);
      Serial.print("Message: ");
      Serial.println(event->message);
      Serial.print("Date & Time: ");
      Serial.println(event->date_time);
      break;

    case EVENT_CSQ:
      Serial.print("CSQ: ");
      Serial.println(event->param1);
      break;

    case EVENT_IMEI:
      Serial.print("IMEI: ");
      Serial.println(event->param2);
      break;

    case EVENT_CCID:
      Serial.print("CCID: ");
      Serial.println(event->param2);
      break;

    case EVENT_CME:
      Serial.print("CME ERROR Message:");
      gsm.errorPrintCME(event->error);
      break;

    case EVENT_CMS:
      Serial.print("CMS ERROR Message:");
      gsm.errorPrintCMS(event->error);
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // GSM power reset would be best for specially in bangladesh 2g/3g network. it's not mandatory but try to use it.
  pinMode(gsm_pin, OUTPUT);
  digitalWrite(gsm_pin, HIGH);
  delay(4000);
  digitalWrite(gsm_pin, LOW);
  delay(2000);
  Serial.println("A9G Test Begin !");

  A9G.begin(115200);
  gsm.init(&A9G);
  gsm.EventDispatch(eventDispatch);


 
  //Don not use this function if you are not aware of this funciton. it's fully blocking code but it's very usefull.
  if(gsm.waitForReady()){
    Serial.println("A9G Ready");
  }

  // if (gsm.bIsReady()) {
  //   Serial.println("A9G Ready");
  // } else {
  //   Serial.println("A9G Not Ready");
  // }

  // gprs connection
  if (gsm.AttachToGPRS()) {
    Serial.println("GPRS Attach Success");
  } else {
    Serial.println("GPRS Attach Fail");
  }


  /*
    Set APN for Specific SIM Card
    In Bangladesh:
      Robi          : PDP>IP, APN>internet
      Banglalink    : PDP>IP, APN>blweb
      Grameen Phone : PDP>IP, APN>internet
      Teletalk      : PDP>IP, APN>wap
  */
  if (gsm.SetAPN("IP", "internet")) {
    Serial.println("APN Set Success");
  } else {
    Serial.println("APN Set Fail");
  }

   //Activate PDP Context
  if (gsm.ActivatePDP()) {
    Serial.println("Activate PDP Success");
  } else {
    Serial.println("Activate PDP Fail");
  }

  //disconnect the broker
  if (gsm.DisconnectBroker()) {
    Serial.println("Broker Disconnect Success");
  } else {
    Serial.println("Broker Disconnect Fail");
  }

  // connect with broker
  // gsm.ConnectToBroker(BROKER_NAME, PORT, USER, PASS, UNIQUE_ID, KEEP_ALIVE, CLEAN_SEASSION)
  // gsm.ConnectToBroker(BROKER_NAME, PORT);
  if (gsm.ConnectToBroker(BROKER_NAME, PORT, UNIQUE_ID, KEEP_ALIVE, CLEAN_SEASSION)) {
    Serial.println("Broker Connect Success");
  } else {
    Serial.println("Broker Connect Fail");
  }

  //subscribe the broker
  if (gsm.SubscribeToTopic(SUB_TOPIC, 1, 0)) {
    Serial.println("Subscribe Success");
  } else {
    Serial.println("Subscribe Fail");
  }

  //publish data to broker
  if (gsm.PublishToTopic(PUB_TOPIC, "Hello IoT Started")) {
    Serial.println("Message Publish Success");
  } else {
    Serial.println("Message Publish Fail");
  }
}

void loop() {
  gsm.executeCallback();

  if (millis() - tic >= 5000) {
    if (gsm.PublishToTopic(PUB_TOPIC, data)) {
      Serial.println("data send success");
    } else {
      Serial.println("data send fail");
    }
    tic = millis();
  }

  delay(15);
}
