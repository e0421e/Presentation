#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define RST_PIN         2          // Configurable, see typical pin layout above
#define SS_PIN          4         // Configurable, see typical pin layout above
#define TOPIC_ID        "/id/"
#define TOPIC_FDMEMBER    "/fdmember/"

int relayPin = 15;                     // 連接繼電器腳位
int ledGreenPin = 5;                  // 連接綠LED燈腳位
int ledRedPin = 16;
int memcount = 0;
// 連接紅LED燈腳位
String msgStr = "";
String member[] = {};
String memberid = "";

char json[100];
// (*) Wifi AP 連線設定
const char* ssid = "Biang";     //"CTAS_information04";   // 04 或 06
const char* password = "0905570919"; //"24071400";            // CTAS Fax#
unsigned long previousMillis = 0;
unsigned long previousMillis2 = 0;
unsigned long interval = 30000;

void(* resetFunc) (void) = 0;

//----------------------------------------------------------------------------------------------
// (*) MQTT Broker IP Address/username/password/topic
#define MQTT_SERVER                  "broker.hivemq.com"   // "m2m.eclipse.org" // "120.109.165.106"
#define MQTT_PORT                    1883
// --------------------------------------------------------------
#define UID                          "s1101881"
#define USERNAME                     "user001"
#define PASSWORD                     "12345678"
#define TOPIC_ANY_LED                "MakerClub/door/#"         // +, # ：是萬用字元（for單一主題、多重主題）
#define TOPIC_MEMBER                 "MakerClub/member"
#define TOPIC_RLNAME                 "MakerClub/rlname"

// ---------------------------------------------------------------------------------------------
String byteArrayToString( byte *data, int len);
String byteArrayToHexString( byte *data, int len);
// Clients
WiFiClient espClient;
void connectAndSubscribe();
void callback(char* topic, byte* payload, unsigned int length);
MFRC522 mfrc522(SS_PIN, RST_PIN);   // 建立 MFRC522
PubSubClient mqttclient( MQTT_SERVER , MQTT_PORT, callback, espClient);
// ------------- 訂閱不同主題資訊 -------------------------------
void connectAndSubscribe() {
  if (mqttclient.connect( UID, USERNAME, PASSWORD)) {
     Serial.println(("Connected with MQTT Broker!"));
     mqttclient.subscribe(TOPIC_MEMBER);
     mqttclient.subscribe(TOPIC_ANY_LED);
     
  }
  else {
    Serial.println(("Fail to build connection with MQTT Broker!?"));
  }
}
void reconnect() {
  // Loop until we're reconnected
  while (!mqttclient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttclient.connect(UID, USERNAME, PASSWORD)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //mqttclient.publish("outTopic", "hello world");
      // ... and resubscribe
      mqttclient.subscribe(TOPIC_MEMBER);
      mqttclient.subscribe(TOPIC_ANY_LED);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttclient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
// ---------------------------------------------------------------------------------------------
void callback(char* topic, byte* payload, unsigned int length) {
   Serial.print("MQTT from: MQTT Broker/");
   Serial.println(topic);
   Serial.print("payload:");
   Serial.write(payload, length);
   Serial.println("");
   
   // 將位元組陣列與字元陣列轉換為字串
   String cmd = memberbyteArrayToString(payload, length);
   String newTopic = String( topic);        
   // https://www.arduino.cc/reference/en/language/variables/data-types/string/functions/tolowercase/         
   cmd.toLowerCase();        // 轉為小寫字串
   newTopic.toLowerCase();

   // 控制腳位編號 : Google ESP8266, NodeMCU pinout 
   int pin = 15;
   // 確認要打開或關閉 LED ?
   if ( cmd.indexOf("open") == 0) {
      pinMode( pin, OUTPUT);       
      digitalWrite( pin, LOW);
      delay(1000);
      digitalWrite( pin, HIGH);// 建立MQTT訊息（JSON格式的字串）
      msgStr = "Door opening!!!";
      // 把String字串轉換成字元陣列格式
      msgStr.toCharArray(json, 50);
      // 發布MQTT主題與訊息
      mqttclient.publish(TOPIC_ID, json);
      // 清空MQTT訊息內容
      msgStr = "";
   }
   else if ( cmd.indexOf("close") == 0) {
      pinMode( pin, OUTPUT);      
      digitalWrite( pin, HIGH);
      msgStr = "Door closing!!!";
      // 把String字串轉換成字元陣列格式
      msgStr.toCharArray(json, 50);
      // 發布MQTT主題與訊息
      mqttclient.publish(TOPIC_ID, json);
      // 清空MQTT訊息內容
      msgStr = "";
   }
}
String memberbyteArrayToString( byte *data, int len) {
   String result = "";
   String tem = "";
   int count = 0;
   int counter = 0;
   for (int i=0; i<len; i++) {
     tem = char(data[i]);
     if(tem != "[" && tem != "]" && tem != "," && tem != " "){
       while(tem == "\""){
        counter++;
        if(counter == 2){
         counter = 0;
         member[count] = result;
         Serial.println(member[count] + "membercount++");
         count++;
         memcount = count;
         result = "";
        }
        tem = "";
       }
       result = result + tem;
     }
   }
   //
   return result;
}

String byteArrayToString( byte *data, int len) {
   String result = "";
   String tem = "";
   int count = 0;
   int counter = 0;
   for (int i=0; i<len; i++) {
     tem = char(data[i]);
     if(tem != "[" && tem != "]" && tem != "," && tem != " "){
       while(tem == "\""){
        counter++;
        if(counter == 2){
         counter = 0;
         member[count] = result;
         count++;
         result = "";
        }
        tem = "";
       }
       result = result + tem;
     }
   }
   //
   return result;
}
void setup() {
  pinMode(relayPin, OUTPUT);          // 設定繼電器接腳為輸出腳位
  pinMode(ledGreenPin, OUTPUT);       // 設定綠LED燈接腳為輸出腳位
  pinMode(ledRedPin, OUTPUT);         // 設定紅LED燈接腳為輸出腳位
  
  Serial.begin(115200);
  Serial.println("Beginning...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected! IP Address:");
  // Print the IP address
  Serial.println(WiFi.localIP());
  
  
  // 設定隨機種子，讓 random() 更隨機
  randomSeed( millis() );
  connectAndSubscribe();
  mqttclient.publish(TOPIC_FDMEMBER,"");
  
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();
  
  digitalWrite(relayPin, HIGH);
  digitalWrite(ledGreenPin, LOW);
  digitalWrite(ledRedPin, LOW);
  
  Serial.println("可開始讀取卡片");
  Serial.println();
}

void loop() {
  if (!mqttclient.connected()) {
    reconnect();
  }
  // 處理 MQTT 訊息
  mqttclient.loop();
  unsigned long currentMillis2 = millis();
  if (currentMillis2 - previousMillis2 >= interval){
    mqttclient.publish(TOPIC_FDMEMBER,"");
    Serial.println("member renew");
    previousMillis2 = currentMillis2;
  }
  // 是否為新卡？
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  // 選擇一張卡
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }
  // 在監控視窗顯示UID
  Serial.print("Card No. :");
  String content = "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  // 讓在讀卡區的RFID卡進入休眠狀態，不再重複讀卡
  mfrc522.PICC_HaltA();
  // 停止讀卡模組編碼  
  mfrc522.PCD_StopCrypto1();
  
  Serial.println();
  Serial.print(" Message :");
  content.toUpperCase();
  tem = content;
  Serial.println(content);
  
  for(int i = 0; i < memcount; i++){
    if(content == member[i]){
      Serial.println(content);
      Serial.println(" 合法卡... ");
      Serial.println();
    
      digitalWrite(relayPin, LOW);               // 繼電器常開端(NO)導通
      digitalWrite(ledGreenPin, HIGH);            // 綠LED燈亮
      delay(1000);                                // 延遲1秒
      digitalWrite(relayPin, HIGH);                // 繼電器常閉端(NC)導通
      digitalWrite(ledGreenPin, LOW);             // 綠LED燈滅
  
      msgStr = "Card UID: " + content + " " + " success to open door";
      // 把String字串轉換成字元陣列格式
      msgStr.toCharArray(json, 100);
      // 發布MQTT主題與訊息
      mqttclient.publish(TOPIC_ID, json);
      // 清空MQTT訊息內容
      msgStr = "";
      /*
      unsigned long currentMillis = millis();
      if ((WiFi.status () != WL_CONNECTED) && (currentMillis - previousMillis >= interval)){
        WiFi.disconnect();
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println("Reset");
          resetFunc();
        }
        previousMillis = currentMillis;
      }
      */
      break;
    }
    if(content != member[i] && i == (memcount - 1)){
      Serial.println(content);
      Serial.println(" 不合法卡...");
      Serial.println();
    
      digitalWrite(ledRedPin, HIGH);              // 綠LED燈亮
      digitalWrite(relayPin, HIGH);                // 繼電器常閉端(NC)導通
      delay(1000);                                // 延遲1秒
      digitalWrite(ledRedPin, LOW);               // 綠LED燈滅

      msgStr = "Card UID: " + content + " failed to open door";
      // 把String字串轉換成字元陣列格式
      msgStr.toCharArray(json, 100);
      // 發布MQTT主題與訊息
      mqttclient.publish(TOPIC_ID, json);
      // 清空MQTT訊息內容
      msgStr = "";
    }
  }
}
