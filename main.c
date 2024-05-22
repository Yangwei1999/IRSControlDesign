#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h> 
#include <PubSubClient.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <math.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <SPI.h>

#define G         4
#define CtrlRe    5
#define BoardRow (8)
#define BoardCol (8)
#define BoardBit (1)
#define BoardNum (1)
#define BuffValidLen (BoardRow * BoardCol * BoardBit * BoardNum / 8)
#define SERIAL_BUFFER_SIZE 512
#define Debug 1
byte SerialBuff[128];

// add Address division
int Orderbits = 3;
int CascadeNums = 3;

const char* mqttServer = "yangwei.link";
const char* Version = "1_1_1";

// 遗嘱设置

const char *willMsg = "Client-Offline";
const int willQos = 0;
const bool willRetain = true;

// 连接MQTT所需参数
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// 定时器 所需要参数
//Ticker ticker;

// AD 所需参数   3.13 /1024 * sensorValue
const int analogInPin = A0;  //ESP8266模拟引脚ADC0,也就是A0
int count_1 = 0;       // 1s 计时
int count_60 = 0;      // 602 计时
int sensorValue = 0;
float voltage_bat = 0.0;
char voltage_char[30];

// 存储MQTT接受消息
char receive_contents2[400];
char receive_contents[400];
char send_contents[100];
char topic_name_array[50];
char publish_conetents_array[50];//37
byte serialBuffer[100];
uint16_t bufferLength = 1;
unsigned char ConfigFlag = 0; // 配网标识



//--------
ESP8266WiFiMulti WiFiMulti;
//-------串口所用参数--------------
String buff_string;
String temp_string;
String RegStr;
String VltStr;
String AckStr;

void setup() {
  //初始化串口设置
  Serial.begin(115200);
  GPIO_Init();
  // 初始化WiFi连接
  WiFi_Manager_init();

  Get_Update();

  


//  // 连接MQTT 服务器
// connectMQTTServer();
//
//  // 定时器初始化

//
//  // 订阅主题
RegStr = "to8266/" + WiFi.macAddress() + "/reg";
VltStr = "tocloud/" + WiFi.macAddress() + "/voltage";
AckStr = "tocloud/" + WiFi.macAddress() + "/ack";
// subscribeTopic(RegStr); // d订阅mac地址
                        //  subscribeTopic("to8266/ris-1");

// SCLR low level valid , forcing output zeros , usually pulled up high level
// G low level valid , wo can up rising after a data transfer finished

}
 
void loop() {

    // 发送16个Byte
//  if (Serial.available())
//  { // 当串口接收到信息后
//      delay(10);
//      int n = Serial.available();
//      //Serial.println("Received Serial Data:");
//      Serial.readBytes(receive_contents2, n); // 将接收到的信息使用readBytes读取
//      receive_contents2[n] = '\0';
//      publish_topic("tocloud/DebugSerial",String(receive_contents2));
//
// 
//  }

//
//  if (mqttClient.connected()) { // 如果开发板成功连接服务器
//        // 每隔3秒钟发布一次信息 
//        // 保持心跳
//        mqttClient.loop();
//    } else {                  // 如果开发板未能成功连接服务器
//        connectMQTTServer();    // 则尝试连接服务器
//    }


}

void GPIO_Init()
{
    // 初始化串口设置
    Serial.begin(115200);
    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    SPI.setHwCs(true);

    pinMode(G, OUTPUT);
    digitalWrite(G, LOW);
 
    pinMode(CtrlRe, OUTPUT);
    digitalWrite(CtrlRe, LOW);

    for(int i =0;i<8;i++)
    {
      SPI.transfer(0x00);
    }
}



// WiFi Manager
void WiFi_Manager_init()
{
        WiFiManager wifiManager;
        wifiManager.autoConnect("AutoConnectAP");
        Serial.println("");
        Serial.print("ESP8266 Connected to ");
        Serial.println(WiFi.SSID()); // WiFi名称
        Serial.print("IP address:\t");
        Serial.println(WiFi.localIP()); // IP
    
}

void connectMQTTServer(){
  String clientId = "esp8266-" + WiFi.macAddress();
  mqttClient.setServer(mqttServer,1883);
  mqttClient.setCallback(receiveCallback);
  String willString = "tocloud/" + WiFi.macAddress() + "/will";
  if (mqttClient.connect(clientId.c_str(), willString.c_str(),willQos,willRetain,willMsg))
  {
        Serial.println("MQTT Server Connected.");
        Serial.println("Server Address: ");
        Serial.println(mqttServer);
        Serial.println("ClientId:");
        Serial.println(clientId);
        publish_topic("tocloud/DebugMac", WiFi.macAddress());
        subscribeTopic(RegStr); // d订阅mac地址   掉线重连
        subscribeTopic("to8266/DebugSerial");
        publish_topic_retain(willString, "Client-Online");
  }
  else
  {
        Serial.print("MQTT Server Connect Failed. Client State:");
        Serial.println(mqttClient.state());
        delay(3000);
  }
}


void publish_topic(String topic,String contents)
{
  strcpy(topic_name_array,topic.c_str());
  strcpy(publish_conetents_array,contents.c_str());
  if(mqttClient.publish(topic_name_array,publish_conetents_array))
  {
    #ifdef Debug
        Serial.println("Publish Topic:");Serial.println(topic_name_array);
        Serial.println("Publish message:");Serial.println(publish_conetents_array);
    #endif
  }
  else
  {
    //Serial.println("publish failed ");
  }
}

void publish_topic_retain(String topic, String contents)
{
  strcpy(topic_name_array, topic.c_str());
  strcpy(publish_conetents_array, contents.c_str());
  if (mqttClient.publish(topic_name_array, publish_conetents_array,true))
  {
#ifdef Debug
        Serial.println("Publish Topic:");
        Serial.println(topic_name_array);
        Serial.println("Publish message:");
        Serial.println(publish_conetents_array);
#endif
  }
  else
  {
        // Serial.println("publish failed ");
  }
}
// 订阅指定主题
void subscribeTopic(String topicString){

  char subTopic[topicString.length() + 1];  
  strcpy(subTopic, topicString.c_str());
  // 通过串口监视器输出是否成功订阅主题以及订阅的主题名称
  if(mqttClient.subscribe(subTopic)){
    Serial.println("Subscrib Topic:");
    Serial.println(subTopic);
  } else {
    Serial.print("Subscribe Fail...");
  }  
}

void receiveCallback(char* topic, byte* payload, unsigned int length) {
  
  unsigned char temp[BuffValidLen*CascadeNums];
  int i;
  StaticJsonDocument<512>doc;
  DeserializationError error;
  JsonArray A1;
  for(i = 0;i<length;i++)
  {
    receive_contents[i] = (char)payload[i];
  }
    receive_contents[length] = '\0';

    if (strcmp(topic, RegStr.c_str()) == 0)
    {
      Serial.flush();
//        Serial.println(receive_contents);
    // 解析 代码
        error= deserializeJson(doc, receive_contents,length);
        A1 = doc["data"];
        for (i = 0; i < BuffValidLen*CascadeNums; i++)
        {
            temp[i]  =A1[i];
//            send_8bit(temp);
//            Serial.write(temp[i]);
        }
         SPI.transfer(temp + (BuffValidLen)*(Orderbits - 1), BuffValidLen);

         digitalWrite(CtrlRe, HIGH);
         delay(10);
         Serial.write(temp,BuffValidLen*CascadeNums);
         delay(10);
         digitalWrite(CtrlRe, LOW);


  }
  


}


int Get_Update()
{

    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
  
    // Add optional callback notifiers
    ESPhttpUpdate.onStart(update_started);
    ESPhttpUpdate.onEnd(update_finished);
    ESPhttpUpdate.onProgress(update_progress);
    ESPhttpUpdate.onError(update_error);
  if (1) {

    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://47.100.51.6:8080/get_version")) {  // HTTP


      Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
    

            StaticJsonDocument<48>doc1;
            DeserializationError error1=deserializeJson(doc1, payload);
//            error1= deserializeJson(doc1, payload,payload.length());
            const char *A2 = doc1["version"];
            Serial.print("Local version :");
            Serial.println(Version);
            
            Serial.print("Romote version :");
            Serial.println(A2);     
            
            if (strcmp(Version, A2) == 0)
          {
            Serial.println("Remote version == Local verision, ");
          }else{
            Serial.println("Remote version != Local verision, need update");

//            WiFiClient client;


          t_httpUpdate_return ret = ESPhttpUpdate.update(client, "http://47.100.51.6:8080/get_file"); // 需要返回文件
 

          switch (ret) {
            case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;
      
            case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;
      
            case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
          }
     
            }
//          Serial.println(payload);
//          Serial.println(A2);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP] Unable to connect");
    }
  }
}


void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
}

void update_progress(int cur, int total) {
  static int iup = 0;
  iup++;
  if(iup==10)
    {
      Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
      iup = 0;
      }
    
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}