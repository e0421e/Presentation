/*
 Basic MQTT example

 This sketch demonstrates the basic capabilities of the library.
 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.
 
*/

#include <WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

char ssid[] = "MakerClub";     // your network SSID (name)
char pass[] = "206206206";  // your network password
int status  = WL_IDLE_STATUS;    // the Wifi radio's status

char mqttServer[]     = "broker.hivemq.com";
char clientId[]       = "amebaClient51165456454";
char publishTopic[]   = "MakerClub/door/d10";
char publishPayload[] = "open";
//char subscribeTopic[] = "inTopic";

const int sw=2;                        //按鍵開關連接至數位接腳第 PA27 腳。
const int led=9;                      //LED 連接至數位接腳第 PA15 腳。
const int debounceDelay=20;            //按鍵開關穩定所需的時間。
int ledStatus=LOW;                     //LED 初始狀態為 LOW。
int val;                               //按鍵開關狀態。

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)(payload[i]));
    }
    Serial.println();
}

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void reconnect() {
    // Loop until we're reconnected
    while (!(client.connected())) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(clientId)) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish(publishTopic, publishPayload);
            // ... and resubscribe
            //client.subscribe(subscribeTopic);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup() {
    pinMode(sw,INPUT_PULLUP);        //設定數位第 2 腳為輸入模式 （ 使用內部上拉電阻 ）。
    //pinMode(led,OUTPUT);             //設定數位第 13 腳為輸出模式。
    Serial.begin(115200);

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }

    client.setServer(mqttServer, 1883);
    client.setCallback(callback);

    // Allow the hardware to sort itself out
    delay(1500);
}

void loop() {

   val=digitalRead(sw);              //讀取按鍵狀態。
   if(val==LOW){                     //按鍵開關被按下？
   
      delay(debounceDelay);          //消除按鍵開關的不穩定狀態 （機械彈跳 ）。
      while(digitalRead(sw)==LOW)    //按鍵開關已放開？
            ;                        //等待放開按鍵開關。
      //ledStatus=!ledStatus;          //改變LED狀態。
      //digitalWrite(led,ledStatus);   //設定 LED 狀態。
      client.publish(publishTopic, publishPayload);
   }
    
   if (!(client.connected())) {
      reconnect();
   }
   client.loop();
}
