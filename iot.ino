/* 網路相關 */
#include "WiFiEsp.h"

#define _SS_MAX_RX_BUFF 2048 //RX 緩衝區大小
#include "SoftwareSerial.h"
SoftwareSerial esp8266(8, 9); //RX, TX

char ssid[] = ""; //WiFi SSID (名稱)
char pass[] = ""; //WiFi 密碼
int status = WL_IDLE_STATUS;

String controlName, parameter;

WiFiEspServer server(8080); //伺服器端口
WiFiEspClient client;

const byte MAX_PAGE_NAME_LEN = 29; //緩衝區大小
char buffer[MAX_PAGE_NAME_LEN + 1];

/* LED 相關 */
const byte led1Pin = A1;

/* LED 矩陣相關 */
#include <SPI.h>

byte symbol[2][8] = {
  {0B00000000, 0B00001100, 0B01101100, 0B00110000, 0B00110000, 0B01101100, 0B00001100, 0B00000000},
  {0B11111111, 0B11110011, 0B10010011, 0B11001111, 0B11001111, 0B10010011, 0B11110011, 0B11111111}
};

const byte NOOP = 0x0; //不運作
const byte DECODEMODE = 0x9; //解碼模式
const byte INTENSITY = 0xA; //顯示強度
const byte SCANLIMIT = 0xB; //掃描限制
const byte SHUTDOWN = 0xC; //停機
const byte DISPLAYTEST = 0xF; //顯示器檢測

/* LCD 顯示器相關 */
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//設定 LCD I2C 位址
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); //addr, en, rw, rs, d4, d5, d6, d7, bl, blpol

/* 溫溼度感測器相關 */
#include "DHT.h"
DHT dht(A0, DHT22); //設定 DHT 物件，腳位 A0

float t, h;

/* 記錄用 */
unsigned long previousMillis = 0;

byte ledMatrixTmp = 0;

String led1Status = "0"; //控制 LED 紀錄狀態用變數
String ledMatrixStatus = "0"; //控制 LED 矩陣紀錄狀態用變數
String lcdText; //紀錄 LCD 顯示器文字用變數

void setup() {
  Serial.begin(9600);

  /* 網路相關 */
  esp8266.begin(9600);

  WiFi.init(&esp8266); //初始化 ESP8266 模組

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println(F("WiFi shield not present."));
    while (true);
  }

  //嘗試連線到 WiFi
  while (status != WL_CONNECTED) {
    Serial.print(F("Attempting to connect to WPA SSID: "));
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass); //連線到 WPA/WPA2 WiFi 網路
  }

  Serial.println(F("You're connected to the network."));
  printWifiStatus(); //顯示 WiFi 狀態

  server.begin(); //啟動 Web 伺服器

  /* LED 相關 */
  pinMode(led1Pin, OUTPUT);

  /* LED 矩陣相關 */
  pinMode(SS, OUTPUT);
  digitalWrite(SS, LOW);

  SPI.begin(); //初始化 SPI

  max7219(SCANLIMIT, 7); //掃描限制
  max7219(DECODEMODE, 0); //解碼模式
  max7219(INTENSITY, 8); //顯示強度
  max7219(DISPLAYTEST, 0); //顯示器檢測
  max7219(SHUTDOWN, 1); //停機

  for (byte i = 0; i < 8; i++) {
    max7219(i + 1, 0); //設定 LED 矩陣全暗
  }

  /* LCD 顯示器相關 */
  lcd.begin(20, 4); //初始化 LCD，一行 20 的字元，共 4 行，預設開啟背光

  lcd.setCursor(0, 0); //第一行
  lcd.print(F("Hello World, Arduino"));
  lcd.setCursor(0, 1); //第二行
  lcd.print(F(" By. Zhang Wenxiang "));
  lcd.setCursor(0, 3); //第四行
  lcd.print(F("Website: blog.reh.tw"));

  /* 溫溼度感測器相關 */
  dht.begin(); //初始化 DHT
}

void loop() {
  client = server.available(); //監聽客戶端消息
  if (client) {
    Serial.println(F("New client!"));

    /* 取得並解析 GET 資料 */
    if (client.find("GET /")) { //檢索頁面名稱 (router)
      memset(buffer, 0, sizeof(buffer));  //清除緩衝區 (全部設定為 0)

      client.find("?controlName="); //跳過 controlName token
      memset(buffer, 0, sizeof(buffer));  //清除緩衝區 (全部設定為 0)
      client.readBytesUntil('&', buffer, sizeof(buffer)); //檢索 controlName
      controlName = urldecode(buffer); //將取得資料存入 controlName 變數

      client.find("parameter="); //跳過 parameter token
      memset(buffer, 0, sizeof(buffer));  //清除緩衝區 (全部設定為 0)
      client.readBytesUntil(' ', buffer, sizeof(buffer)); //檢索 parameter
      parameter = urldecode(buffer); //將取得資料存入 parameter 變數
    }

    /* 請求資料同步 */
    if (controlName == "sync") {
      sendCustomJson("{\"led1Status\": " + led1Status + ", \"ledMatrixStatus\": " + ledMatrixStatus + ", \"lcdText\": \"" + lcdText + "\"}"); //發送自訂 Json 資料
    }

    /* LED */
    if (controlName == "led1") {
      if (parameter == "1") {
        led1Status = "1";
        sendJson("1", "LED1 已開啟。"); //發送 Json 資料
      } else {
        led1Status = "0";
        sendJson("1", "LED1 已關閉。"); //發送 Json 資料
      }
    }

    /* LED 矩陣 */
    if (controlName == "ledMatrix") {
      if (parameter == "1") {
        ledMatrixStatus = "1";
        sendJson("1", "LED 矩陣已開啟。"); //發送 Json 資料
      } else {
        ledMatrixStatus = "0";
        sendJson("1", "LED 矩陣已關閉。"); //發送 Json 資料
      }
    }

    /* LCD 顯示器 */
    if (controlName == "lcdText") {
      lcdText = parameter;
      sendJson("1", "LCD 顯示器已設定文字「" + lcdText + "」。"); //發送 Json 資料

      lcd.setCursor(0, 3); //第四行
      lcd.print(F("                    ")); //清空
    }

    /* 感測器 */
    if (controlName == "sensorData") {
      /* 溫溼度感測器 */
      String temperature, humidity;

      //判斷取得的溫度數值是否是 NaN
      if (isnan(t)) {
        temperature = "unknown";
      } else {
        temperature = String(t);
      }

      //判斷取得的濕度數值是否是 NaN
      if (isnan(h)) {
        humidity = "unknown";
      } else {
        humidity = String(h);
      }

      /* 回傳 Json 資料 */
      sendCustomJson("{\"dht22\": {\"temperature\": \"" + temperature + "\", \"humidity\": \"" + humidity + "\"}}"); //發送自訂 Json 資料
    }

    Serial.println();
    Serial.println(F("================================"));
    Serial.println(F("Get Data:"));
    Serial.println("- controlName = " + controlName);
    Serial.println("- parameter = " + parameter);
    Serial.println(F("================================"));
    Serial.println();
  }

  /* 控制 LED */
  if (led1Status == "1") {
    digitalWrite(led1Pin, HIGH); //開啟 led1
  } else {
    digitalWrite(led1Pin, LOW); //關閉 led1
  }

  /* 控制 LED 矩陣 */
  if (ledMatrixStatus == "1") {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 1000) {
      previousMillis = currentMillis;

      if (ledMatrixTmp == 0) {
        ledMatrixTmp = 1;
      } else {
        ledMatrixTmp = 0;
      }
    }

    for (byte i = 0; i < 8; i++) {
      max7219(i + 1, symbol[ledMatrixTmp][i]);
    }
  } else {
    for (byte i = 0; i < 8; i++) {
      max7219(i + 1, 0);
    }
  }

  /* 設定 LCD 顯示器文字 */
  if (lcdText != "") {
    lcd.setCursor(0, 3); //第四行
    lcd.print(lcdText);
  }

  /* 溫溼度感測器讀值 */
  t = dht.readTemperature(); //讀取溫度資料
  h = dht.readHumidity(); //讀取濕度資料
}

/* WiFi 狀態 */
void printWifiStatus() {
  Serial.println();
  Serial.println(F("================================"));

  /* 顯示所連接的網路 SSID */
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  /* 顯示 WiFi 區網 IP address */
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  /* 顯示 WiFi 接收訊號強度 */
  long rssi = WiFi.RSSI();
  Serial.print(F("Signal strength (RSSI): "));
  Serial.print(rssi);
  Serial.println(F(" dBm"));

  /* 顯示瀏覽器的位置 */
  Serial.println();
  Serial.print(F("To see this page in action, open a browser to http://"));
  Serial.println(ip);
  Serial.println(F("================================"));
  Serial.println();
}

/* 發送 Json 資料 */
void sendJson(String results, String msg) {
  if (results > 1) {
    results = 1;
  }

  boolean currentLineIsBlank = true; //http 請求以空行結束
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);

      /* 如果收到一個換行符，並且該行為空，則 HTTP 請求已結束，因此可以發送回复 */
      if (c == '\n' && currentLineIsBlank) {
        Serial.println(F("Sending response..."));

        /* 發送標準的 HTTP 響應頭 */
        client.print(F(
                       "HTTP/1.1 200 OK\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Server: ESP8266\r\n"
                       "Content-Type: application/json\r\n"
                       "Connection: Keep-Alive\r\n"
                       "\r\n"));
        /* 發送 Json 格式資料 */
        client.print("{\"results\": " + results + ", \"msg\": \"" + msg + "\"}\r\n");

        break;
      }

      if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }

  delay(10); //為 Web 提供接收資料的時間

  client.stop(); //關閉連接
  Serial.println(F("Client disconnected"));
}

/* 發送自訂 Json 資料 */
void sendCustomJson(String jsonData) {
  boolean currentLineIsBlank = true; //http 請求以空行結束
  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      Serial.write(c);

      /* 如果收到一個換行符，並且該行為空，則 HTTP 請求已結束，因此可以發送回复 */
      if (c == '\n' && currentLineIsBlank) {
        Serial.println(F("Sending response..."));

        /* 發送標準的 HTTP 響應頭 */
        client.print(F(
                       "HTTP/1.1 200 OK\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Server: ESP8266\r\n"
                       "Content-Type: application/json\r\n"
                       "Connection: Keep-Alive\r\n"
                       "\r\n"));
        /* 發送 Json 格式資料 */
        client.print(jsonData + "\r\n");

        break;
      }

      if (c == '\n') {
        currentLineIsBlank = true;
      } else if (c != '\r') {
        currentLineIsBlank = false;
      }
    }
  }

  delay(10); //為 Web 提供接收資料的時間

  client.stop(); //關閉連接
  Serial.println(F("Client disconnected"));
}

/* URL 解碼 */
String urldecode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      encodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {
      encodedString += c;
    }

    yield();
  }

  return encodedString;
}
unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

/* LED 矩陣相關 */
void max7219(byte reg, byte data) {
  digitalWrite(SS, LOW);
  SPI.transfer(reg);
  SPI.transfer(data);
  digitalWrite(SS, HIGH);
}
