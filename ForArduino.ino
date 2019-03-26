#include "Arduino.h"
#include "SoftwareSerial.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#include <SoftwareSerial.h>
SoftwareSerial debugSerial(10, 11); // RX, TX
SoftwareSerial espSerial(0, 1);
int value = 0;
long lastRecv = 0;
long lastTime = 0;
bool newDataComing = false;
String JsonDataTemplator = "{\"TH2O\":\"TH2O_VALUE\",\"PHH2O\":\"PHH2O_VALUE\",\"TDS\":\"TDS_VALUE\",\"ORP\":\"ORP_VALUE\",\"LEVEL\":\"LEVEL_VALUE\",\"TAIR\":\"TAIR_VALUE\",\"LIGHT\":\"LIGHT_VALUE\",\"RHSUB\":\"RHSUB_VALUE\",\"RHAIR\":\"RHAIR_VALUE\"}";

String strRecv = "";
bool mqttConnected = false;
bool stopUploadAllData = false;

float getTH2OTemp() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  //  Serial.print("Temperature for Device 1 is: ");
  float temp = sensors.getTempCByIndex(0);
  return temp;
}

void setup() {
  // put your setup code here, to run once:
  delay(5000);        // it will be better to delay 2s to wait esp8266 module OK
  Serial.begin(9600);
  debugSerial.begin(9600);
  espSerial.begin(9600);
  sensors.begin();
}
void loop() {
  // put your main code here, to run repeatedly:
  long now = millis();
  if (espSerial.available() > 0) {
    char str = char(espSerial.read());
    strRecv = strRecv + str;
    lastRecv = millis();
    newDataComing = true;
    delay(2);
  } else if ((now - lastRecv > 100) && (newDataComing == true)) {
    debugSerial.print("[Receive:");
    debugSerial.println(strRecv);
    debugSerial.println("]");
    if (strRecv.equals("-1")) {
      mqttConnected = false;
    } else {
      mqttConnected = true;
      if (strRecv.indexOf("|1|") != -1 && !stopUploadAllData) {
        stopUploadAllData = true;
      } else if (strRecv.indexOf("|2|") != -1 && stopUploadAllData) {
        stopUploadAllData = false;
      }
    }
    strRecv = "";
    newDataComing = false;
  }
  if (now - lastTime > 5000 && mqttConnected && !stopUploadAllData) {
    lastTime = now;
    float TH2OTemp = getTH2OTemp();
    String JsonData = JsonDataTemplator;
    JsonData.replace("TH2O_VALUE", String(TH2OTemp));
    JsonData.replace("PHH2O_VALUE", String(0));
    JsonData.replace("ORP_VALUE", String(0));
    JsonData.replace("LEVEL_VALUE", String(0));
    JsonData.replace("TAIR_VALUE", String(0));
    JsonData.replace("LIGHT_VALUE", String(0));
    JsonData.replace("RHSUB_VALUE", String(0));
    JsonData.replace("RHAIR_VALUE", String(0));
    debugSerial.println("[Send:");
    debugSerial.println(JsonData);
    debugSerial.println("]");
    Serial.println(JsonData);
    Serial.flush();
  }
}
