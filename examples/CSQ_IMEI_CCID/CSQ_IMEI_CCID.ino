#include <Arduino.h>
#include <A9G.h>

HardwareSerial A9G(2);
GSM gsm(1);

const int gsm_pin = 15;


void eventDispatch(A9G_Event_t *event)
{
  switch (event->id)
  {
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

void setup()
{
  Serial.begin(115200);

  pinMode(gsm_pin, OUTPUT);
  digitalWrite(gsm_pin, HIGH);
  delay(4000);
  digitalWrite(gsm_pin, LOW);
  delay(2000);
  Serial.println("A9G Test Begin !");

  A9G.begin(115200);
  gsm.init(&A9G);

  gsm.EventDispatch(eventDispatch);

  if (gsm.waitForReady())
  {
    Serial.println("A9G Ready");
  }
  else
  {
    Serial.println("A9G Not Ready");
  }


  delay(3000);
  Serial.println("***********Read CSQ ***********");
  gsm.ReadCSQ();

  delay(3000);
  Serial.println("***********Read IMEI ***********");
  gsm.ReadIMEI();

  delay(3000);
  Serial.println("***********Read CCID ***********");
  gsm.ReadCCID();
}

void loop()
{
  gsm.executeCallback();

  delay(15);
}
