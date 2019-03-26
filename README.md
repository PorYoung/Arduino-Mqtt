# Arduino R3 Uno + ESP8266 MQTT连接记录

使用DFRobot的arduino扩展板和WiFi Bee模块连接mqtt服务器，过程十分艰辛，从`arduino uno`和`Obloq`模块转到`arduino R3 Uno`、`IO扩展板`和`WiFi Bee`模块，相关文档和代码十分有限，网上大部分教程都无法使用，有各种错误，这里记录了最终成功的过程。

## 材料

### 所用设备

1. [DFRduino UNO R3](http://www.dfrobot.com.cn/goods-521.html)
2. [IO 传感器扩展板 V7.1](http://www.dfrobot.com.cn/goods-791.html)
3. [ESP8266 WiFi Bee模块](http://www.dfrobot.com.cn/goods-1121.html)
4. [XBee USB Adapter 适配器（带FTDI烧写功能）](http://www.dfrobot.com.cn/goods-103.html)
5. USB线、连接线若干

### 软件及库

1. `Arduino IDE 1.8.7`
2. [串口调试软件CoolTerm](http://freeware.the-meiers.org/)
3. `FTDI USB Drivers`驱动，[国内](https://www.ftdichip.cn/FTDrivers.htm)，[海外](https://www.ftdichip.com/FTDrivers.htm)
4. `firebeetle8266 Arduino`开发环境，国内使用[http://git.oschina.net/dfrobot/firebeetle-esp8266/raw/master/package_firebeetle8266_index.json]()，海外使用[https://raw.githubusercontent.com/DFRobot/FireBeetle-ESP8266/master/package_firebeetle8266_index.json]()
5. `pubsubclient`，[官网](https://pubsubclient.knolleary.net)，[github主页](https://github.com/knolleary/pubsubclient)

## 开发记录

### XBee USB Adapter适配器

`arduino`自带FTDI驱动，如果无法连接，**请检查连接线是否兼容**，若没有驱动，可前往[https://www.ftdichip.cn/FTDrivers.htm]()下载

### WiFi Bee-ESP8266编程环境配置

`WiFi Bee`模块官方封装了`esp8266.h`库，但不满足我们的使用`pubsubclient`连接`MQTT`服务器的要求，因此需要烧录程序。

在`arduino`*首选项*的*附加开发板管理网址*里添加网址[http://git.oschina.net/dfrobot/firebeetle-esp8266/raw/master/package_firebeetle8266_index.json]()，更新索引并下载`firebeetle8266`开发板。

将`WiFi Bee`拨到`UART`，插在适配器上连接电脑，选择`firebeetle8266`开发板，添加测试代码`Serial.println("test");`，上传代码，若上传失败，断电重试。

查看串口监视器，是否有输出。

> 遗留问题：传代码速度的选择影响上传速度，但不知道是不是都可以选，测试了9600和115200没有报错，921600报错

### 烧录连接WiFi和MQTT服务器代码到ESP

尝试了115200和9600两种不同的波特率，貌似只有9600没有问题。

```c++
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
char ssid[] = "AP Name";
char password[] = "AP Pass";

//Public MQTT Server
const char* mqtt_server = "mq.tongxinmao.com";
int mqtt_port = 18830;
char publishTopic[] = "/public/TEST/tp";
char subscribeTopic[] = "/public/TEST/ts";

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
      Serial.println(" DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
```

### Arduino控制代码

以水温采集传感器为例，[防水温度传感器](http://www.dfrobot.com.cn/goods-799.html)，此处传感器数据线连3号引脚。

```c++
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
```

### Debug串口

使用XBee USB Adapter适配器，将接地引脚、TX、RX分别接在IO扩展板的接地、10、11上，并用usb线连接电脑，打开CoolTerm，选择不同的端口，查看调试信息，进行调试。

### 其他问题

#### 数据上行与下行的若干问题【2019-3-26更新】

##### pubsubclient mqtt的上行和下行数据大小限制

`pubsubclient`默认下行数据大小`128bytes`，超过大小的都会忽略，可以在`pubsubclient.h`中修改。

`pubsubclient`默认没有下行数据的限制，最大数据受制于硬件，但使用上例的`client.publish()`*似乎*无法直接发送超过100字节的数据，需要使用以下（不止这一种）方式替换：

```js
client.beginPublish(publishTopic, String(strRecv).length(), false);
client.print(String(strRecv).c_str());
client.endPublish();
```

##### 使用`Serial.flush`避免串口数据混淆