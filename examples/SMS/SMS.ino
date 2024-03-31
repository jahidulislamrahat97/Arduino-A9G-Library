/*********************************************************************************
   If this code works, it was written by Jahidul Islam Rahat.
   If not, I don't know who wrote it.
   :) xD

   Author: Jahidul Islam Rahat.
   Date: 25 March 2024.
*********************************************************************************/

#include <A9G.h>
#include <HardwareSerial.h>

HardwareSerial A9G(2);
GSM gsm(1);


#define PHONE_NO            "017*******"


const int gsm_pin = 15;
unsigned long toc = millis();


void eventDispatch(A9G_Event_t *event) {
  switch (event->id) {
    case EVENT_MQTTPUBLISH:
      Serial.print("Topic: ");
      Serial.println(event->topic);
      Serial.printf("Message: %s\n", event->message);
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

  delay(3000);
  gsm.ActivateTE();
  delay(3000);
  gsm.SetFormatReading(1);
  delay(3000);
  gsm.SetMessageStorageUnit();
  delay(3000);
  gsm.CheckMessageStorageUnit();
  delay(3000);
  gsm.DeleteMessage(1, ALL_MESSAGE);
  delay(3000);
  gsm.CheckMessageStorageUnit();
  delay(3000);
}

void loop() {
  gsm.executeCallback();

  if (millis() - toc >= 30000) {
    Serial.println("Sending SMS");
    // gsm.vSendMessage(PHONE_NO, "Hello Rahat");

    toc = millis();
  }
  delay(15);
}
