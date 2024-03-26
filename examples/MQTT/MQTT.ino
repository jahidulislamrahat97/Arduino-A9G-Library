#include <Arduino.h>
#include <A9G.h>

HardwareSerial A9G(2);
GSM gsm(1);

const int gsm_pin = 15;

unsigned long tic = millis();
int counter = 0;
char data[50] = "\0";



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

  // gprs connection
  gsm.AttachToGPRS();
  gsm.SetAPN("IP", "internet");
  gsm.ActivatePDP();

  //mqtt connection
  gsm.DisconnectBroker();
  // gsm.ConnectToBroker("broker.hivemq.com", 1883, "broker user id", "broker password", "akdvnkadfjvdfkj", 120, 0)
  // gsm.ConnectToBroker("broker.hivemq.com", 1883, "dknvkfdnvj", 120, 0)
  gsm.ConnectToBroker("broker.hivemq.com", 1883);
  gsm.SubscribeToTopic("Robot/SUB", 1, 0);
  gsm.PublishToTopic("Robot/PUB", "Robot Started");

}

void loop()
{
  gsm.executeCallback();

  if (millis() - tic >= 10000)
  {
    counter++;
    sprintf(data, "Hello Robot %d", counter);
    if (gsm.PublishToTopic("Robot/PUB", data))
    {
      Serial.println("Mqtt send success");
    }
    else
    {
      Serial.println("Mqtt send fail");
    }
    tic = millis();
  }

  delay(15);
}
