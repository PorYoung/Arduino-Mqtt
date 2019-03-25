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

float getTemp() {
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
String strRecv = "";
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
    strRecv = "";
    newDataComing = false;
  }
  if (now - lastTime > 1000) {
    lastTime = now;
    float temp = getTemp();
    String JsonData = "{\"Temp\":\"TEMP_VALUE\"}";
    JsonData.replace("TEMP_VALUE", String(temp));
    debugSerial.println("[Send:");
    debugSerial.println(JsonData);
    debugSerial.println("]");
    Serial.println(JsonData);
  }
}
