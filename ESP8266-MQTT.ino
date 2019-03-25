#include <ESP8266WiFi.h>
#include <PubSubClient.h>
char ssid[] = "PorYoung-PC";
char password[] = "return 1;";

//const char* mqtt_server = "mq.tongxinmao.com";
//int mqtt_port = 18830;
//char publishTopic[] = "/public/TEST/tp";
//char subscribeTopic[] = "/public/TEST/ts";

const char* mqtt_server = "iot.smartaq.cn";
int mqtt_port = 1883;
String mqttUser = "device/5c98db0322a74e08f9d4a26b";
String mqttPass = "123456";
char publishTopic[] = "public/test";
char subscribeTopic[] = "public/info";

WiFiClient espClient;
PubSubClient client(espClient);

String strRecv = "";
long now = 0;
long lastRecv = 0;
bool newDataComing = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(10);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.print("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (Serial.available() > 0) {
    char str = char(Serial.read());
    strRecv = strRecv + str;
    lastRecv = millis();
    newDataComing = true;
    delay(2);
  } else {
    now = millis();
    if ((now - lastRecv > 100) && (newDataComing == true)) {
      boolean isOK = client.publish(publishTopic, String(strRecv).c_str());
      strRecv = "";
      newDataComing = false;
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) {
    msg.concat((char)payload[i]);
  }
  Serial.print(msg);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = mqttUser;
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str())) {
      client.subscribe(subscribeTopic);
      client.subscribe(publishTopic);
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
