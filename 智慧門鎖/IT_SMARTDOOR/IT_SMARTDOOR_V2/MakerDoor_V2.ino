#include <Arduino.h>
#if defined(ESP32) || defined(PICO_RP2040)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <SPI.h>

#include <Wire.h>

#include <MFRC522.h>

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#define RST_PIN         2          // Configurable, see typical pin layout above
#define SS_PIN          4         // Configurable, see typical pin layout above

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Huang"
#define WIFI_PASSWORD "0912383804"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyCOO7S3sscg-oxc8x9Urom_yNQM-rVNzMQ"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://s1101881-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "s1101881@gm.pu.edu.tw"
#define USER_PASSWORD "P124763712"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

int relayPin = 15;                     // 連接繼電器腳位
int count;
int ccc = 1;

String membername[100] = {};
String id[100] = {};

MFRC522 mfrc522(SS_PIN, RST_PIN);   // 建立 MFRC522
void setup()
{
  pinMode(relayPin, OUTPUT);          // 設定繼電器接腳為輸出腳位
  
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
  // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
  fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();

  digitalWrite(relayPin, HIGH);

  Serial.println("可開始讀取卡片");
  Serial.println();
}

void loop(){
  // Firebase.ready() should be called repeatedly to handle authentication tasks.

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();

    //Firebase.RTDB.set(&fbdo, F("/test/newtry"), true);
    Serial.printf("Get newtry...%s\n",Firebase.RTDB.get(&fbdo, F("/MakerClub"))? fbdo.to<String>().c_str() : fbdo.errorReason().c_str());
    namerecord(Firebase.RTDB.get(&fbdo, F("/MakerClub"))? fbdo.to<String>() : fbdo.errorReason().c_str());
    for(int i = 1; i <= 20; i++){
      Serial.print(membername[i]);
      Serial.println("ID "+id[i]+" 開門了");
    }
    String mname = Serial.readString();
    String no = "MakerClubRecord/" + String(ccc);
    Serial.println("input ur name");
    if(mname == id[1]){
      Serial.println("welcome Biang");
      Firebase.RTDB.set(&fbdo, (no), membername[1] + id[1]);
      ccc++;
    }
    else{
      Serial.println("No No");
    }
  }

  // 是否為新卡？
  if ( ! mfrc522.PICC_IsNewCardPresent()){
    return;
  }
  // 選擇一張卡
  if ( ! mfrc522.PICC_ReadCardSerial()){
    return;
  }
  // 在監控視窗顯示UID
  Serial.print("Card No. :");
  String content = "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++){
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
  Serial.println(content);
  // 判斷是否為成員
  for(int i = 1; i <= count; i++){
    // 成員
    if(content == id[i]){
      Serial.println(content);
      Serial.println(" 合法卡... ");
      Serial.println();
    
      digitalWrite(relayPin, LOW);               // 繼電器常開端(NO)導通           // 綠LED燈亮
      delay(1000);                                // 延遲1秒
      digitalWrite(relayPin, HIGH);                // 繼電器常閉端(NC)導通
      break;
    }
    // 非成員
    if(content != id[i] && i == count){
      Serial.println(content);
      Serial.println(" 不合法卡...");
      Serial.println();
      digitalWrite(relayPin, HIGH);                // 繼電器常閉端(NC)導通
      delay(1000);
    }
  }
}
void namerecord(String name){
  String n;
  String tem;
  count = 1;
  int counter = 0;
  byte buf[1000];
  name.getBytes(buf,1000);
  for(int i = 0; i < 1000; i++){
    tem = char(buf[i]);
    if(tem != "{" && tem != "}" && tem != ":" && tem != ","){
      if(tem == "\""){
        counter++;
        if(counter == 2){
          membername[count] = n;
          n = "";
        }
        if(counter == 4){
          counter = 0;
          id[count] = n;
          count++;
          n = "";
        }
        tem = "";
      }
      n = n + tem;
    }
  }
  
}






/** Timeout options.

  //WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.
  config.timeout.wifiReconnect = 10 * 1000;

  //Socket connection and SSL handshake timeout in ms (1 sec - 1 min).
  config.timeout.socketConnection = 10 * 1000;

  //Server response read timeout in ms (1 sec - 1 min).
  config.timeout.serverResponse = 10 * 1000;

  //RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.
  config.timeout.rtdbKeepAlive = 45 * 1000;

  //RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.
  config.timeout.rtdbStreamReconnect = 1 * 1000;

  //RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream
  //will return false (error) when it called repeatedly in loop.
  config.timeout.rtdbStreamError = 3 * 1000;

  Note:
  The function that starting the new TCP session i.e. first time server connection or previous session was closed, the function won't exit until the
  time of config.timeout.socketConnection.

  You can also set the TCP data sending retry with
  config.tcp_data_sending_retry = 1;

  */






/** NOTE:
 * When you trying to get boolean, integer and floating point number using getXXX from string, json
 * and array that stored on the database, the value will not set (unchanged) in the
 * FirebaseData object because of the request and data response type are mismatched.
 *
 * There is no error reported in this case, until you set this option to true
 * config.rtdb.data_type_stricted = true;
 *
 * In the case of unknown type of data to be retrieved, please use generic get function and cast its value to desired type like this
 *
 * Firebase.RTDB.get(&fbdo, "/path/to/node");
 *
 * float value = fbdo.to<float>();
 * String str = fbdo.to<String>();
 *
 */

/// PLEASE AVOID THIS ////

// Please avoid the following inappropriate and inefficient use cases
/**
 *
 * 1. Call get repeatedly inside the loop without the appropriate timing for execution provided e.g. millis() or conditional checking,
 * where delay should be avoided.
 *
 * Everytime get was called, the request header need to be sent to server which its size depends on the authentication method used,
 * and costs your data usage.
 *
 * Please use stream function instead for this use case.
 *
 * 2. Using the single FirebaseData object to call different type functions as above example without the appropriate
 * timing for execution provided in the loop i.e., repeatedly switching call between get and set functions.
 *
 * In addition to costs the data usage, the delay will be involved as the session needs to be closed and opened too often
 * due to the HTTP method (GET, PUT, POST, PATCH and DELETE) was changed in the incoming request.
 *
 *
 * Please reduce the use of swithing calls by store the multiple values to the JSON object and store it once on the database.
 *
 * Or calling continuously "set" or "setAsync" functions without "get" called in between, and calling get continuously without set
 * called in between.
 *
 * If you needed to call arbitrary "get" and "set" based on condition or event, use another FirebaseData object to avoid the session
 * closing and reopening.
 *
 * 3. Use of delay or hidden delay or blocking operation to wait for hardware ready in the third party sensor libraries, together with stream functions e.g. Firebase.RTDB.readStream and fbdo.streamAvailable in the loop.
 *
 * Please use non-blocking mode of sensor libraries (if available) or use millis instead of delay in your code.
 *
 * 4. Blocking the token generation process.
 *
 * Let the authentication token generation to run without blocking, the following code MUST BE AVOIDED.
 *
 * while (!Firebase.ready()) <---- Don't do this in while loop
 * {
 *     delay(1000);
 * }
 *
 */
