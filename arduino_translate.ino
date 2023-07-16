#define LED_PIN 19  
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

#include "web_index.h"

// const char *ssid = "KANGS_2.4G_1F";
// const char *password = "kang3708!@";
const char *ssid = "SK_WiFiGIGA97BE_2.4G";
const char *password = "AWKB0@6876";
#include "FS.h"
#include "SPIFFS.h"

#define RECORDING_TIME 10                           // 녹음 시간 10초
#define RECORDING_DATA_SIZE RECORDING_TIME * 8000  // 8000 = 1초간 레코딩 데이타

const int headerSize = 44;
char filename[20] = "/sound1.wav";
int mode = 0;  // 0 : 재생 모드 , 1 : 녹음 모드
byte header[headerSize];
unsigned long write_data_count = 0;
File file;
unsigned long start_millis;
uint8_t *buffer;

void CreateWavHeader(byte *header, int waveDataSize);

#include <SPI.h>
#include <Wire.h>
//U8G2
#include <U8g2lib.h>
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "u8g2_korea_kang4.h"

//Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 5, /* dc=*/ 14, /* reset=*/ 15); //scl=18, sda=23  SPI로 변경

#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // UART service UUID
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a8" // 
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a9" // 

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
String recentMessage = "";

int nowCallback = 0;
int previousCallback = 0;

int deviceWidth = 128;

int maxCursorY = 0;
int currentCursorY = 0;

int gapWithTextLines = 24;


//긴 텍스트를 위한 스크롤 기능
unsigned long scrollStartTime = 0;
unsigned long accumTimeForScroll = 0;
long previousMillis = 0; 

//아래의 두개 수정가능
int scrollStartDelayTime = 3000; // (3000이면 3초있다가 스크롤 시작)
int scrollDelay = 200;// (이값이 클수록 스크롤 속도가 느려짐)




class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++)
           recentMessage = rxValue.c_str(); 
        nowCallback++;
      }
    }

};


void setup() {
  Serial.begin(19200);
  // Create the BLE Device
  initBLEDevice();
  //Initialize U8G2 
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();

  clearSerialBuffer();

  openingMent();

  
  //이부분은 새로추가된부분
  pinMode(LED_PIN, OUTPUT);  // LED_PIN을 출력으로 설정

  //setupForRecording();


  Serial.begin(115200);
  if (!SPIFFS.begin(true))
  {
   // Serial.println("SPIFFS Mount Failed");
    while (1);
  }
  print_file_list();
  buffer = (uint8_t *)malloc(RECORDING_DATA_SIZE);
  memset(buffer, 0, RECORDING_DATA_SIZE);

  if (String(WiFi.SSID()) != String(ssid))
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
  }
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, get_html);
  server.on("/sound1.wav", HTTP_GET, capture_handler1);
  server.on("/sound2.wav", HTTP_GET, capture_handler2);
  server.on("/sound3.wav", HTTP_GET, capture_handler3);
  server.on("/record1", HTTP_GET, record_handler1);
  server.on("/record2", HTTP_GET, record_handler2);
  server.on("/record3", HTTP_GET, record_handler3);
  server.begin();
}

void initBLEDevice()
{
  BLEDevice::init("banGawer");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  ); 
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pRxCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();

  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

//////////////////////////////////////////////////////////////////////////////////////////LOOP////////////////////////////////////////////////////////////////////////////////////////

  void openingMent()
  {
    String str =  "banGawer";
    u8g2.clearBuffer(); // 버퍼 초기화
    u8g2.setFont(u8g2_font_prospero_bold_nbp_tr);
    u8g2.drawUTF8(34, 36, str.c_str());
    u8g2.sendBuffer();
  }
  void connectedMent()
  {
     String str2 =  "Device Connected";
     u8g2.clearBuffer(); // 버퍼 초기화
     u8g2.setFont(u8g2_font_lubBI08_te);
     u8g2.drawUTF8(13, 34, str2.c_str());
     u8g2.sendBuffer();
  }

void loop() {

  unsigned long currentMillis = millis();
  unsigned long deltaTime = currentMillis - previousMillis;
  
  if (deviceConnected) {
      pTxCharacteristic->setValue(&txValue, 1);
      pTxCharacteristic->notify();
      txValue++;
  delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }
  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;

  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
  // do stuff here on connecting
      oldDeviceConnected = deviceConnected;

      Serial.print("연결완료");
      clearSerialBuffer();
  }


  bool recentMessageExist = nowCallback != previousCallback;
  if(recentMessageExist)
  {
    currentCursorY = 0;
    maxCursorY = 0;
    previousCallback = nowCallback;

    accumTimeForScroll = 0;


  }

  if (recentMessage.length() > 0 && recentMessage.indexOf(":") != -1 && recentMessage.indexOf(";") != -1) 
  {
    int langCode;
    String someMsg;
    parseLangCodeAndMessage(recentMessage, langCode, someMsg);

    if(langCode == 5)
    {
      someMsg = replaceChinesePunctuations(someMsg);
    }
    Message(langCode, someMsg);
  } 
  else 
  {
    Serial.println("Invalid input format. It should be in the format 'langcode:someMsg;'");

    if(recentMessage == "signal"){  

      digitalWrite(LED_PIN, HIGH);   // LED를 켭니다.
      delay(1000);                   // 1초 동안 대기합니다.
      digitalWrite(LED_PIN, LOW);    // LED를 끕니다.
      delay(1000);
    }
  }
  if (accumTimeForScroll >= scrollStartDelayTime)
     scrollWithInterval(scrollDelay);
  else{
    accumTimeForScroll += deltaTime;
  }
  previousMillis = currentMillis;
  

  
  if (mode == 0)
  {
    if (Serial.available())
    {
      int val = Serial.read();
      if (val == '1')
      {
        Serial.println("WRITING START");
        mode = 1;
        write_data_count = 0;
        strcpy(filename, "/sound1.wav");
        start_millis = millis();
      }
      if (val == '2')
      {
        Serial.println("WRITING START");
        mode = 1;
        write_data_count = 0;
        strcpy(filename, "/sound2.wav");
        start_millis = millis();
      }
      if (val == '3')
      {
        Serial.println("WRITING START");
        mode = 1;
        write_data_count = 0;
        strcpy(filename, "/sound3.wav");
        start_millis = millis();
      }
      if (val == 'x')
      {
        Serial.println("FORMAT START");
        SPIFFS.format();
        Serial.println("FORMAT COMPLETED");
      }
    }
  }
  if (mode == 1)
  {
    record_process();
    delayMicroseconds(30);
  }
}
//////////////////////////////////////////////////////////////////////////////////////////LOOP////////////////////////////////////////////////////////////////////////////////////////
void clearSerialBuffer() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}
void scrollWithInterval(long interval)
{
  unsigned long currentMillis = millis();

  // check if it's time to scroll
  if (currentMillis - scrollStartTime >= interval) {
    // do scrolling here
    int maxLineCount = 4; // 수정가능
    int onePageHeight = gapWithTextLines * maxLineCount;
    if (maxCursorY >= onePageHeight && currentCursorY < (maxCursorY - onePageHeight)) {
      currentCursorY ++;
    }
    else {
      scrollStartTime = currentMillis;
    }
  }
  
}

void parseLangCodeAndMessage(String input, int &langCode, String &someMsg) {
  int separatorIndex = input.indexOf(":");
  langCode = input.substring(0, separatorIndex).toInt();
  someMsg = input.substring(separatorIndex + 1, input.indexOf(";"));
}
bool isCharValid(String charStr) {
    for (int i = 0; i < charStr.length(); i++) {
        if (charStr[i] < 0x80) { // ASCII 문자인 경우
            continue;
        }
        // Unicode 범위에 속하지 않는 문자는 유효하지 않다고 판단
        if ((charStr[i] >= 0x80 && charStr[i] <= 0xBF) ||
            (charStr[i] >= 0xC0 && charStr[i] <= 0xDF) ||
            (charStr[i] >= 0xE0 && charStr[i] <= 0xEF) ||
            (charStr[i] >= 0xF0 && charStr[i] <= 0xF7) ||
            (charStr[i] >= 0xF8 && charStr[i] <= 0xFB) ||
            (charStr[i] >= 0xFC && charStr[i] <= 0xFD)) {
            continue;
        }
        return false;
    }
    return true;
}




void Message(int langCode, String str)
{
  u8g2.clearBuffer(); // 버퍼 초기화
  ChangeUTF(langCode);
  u8g2.setFlipMode(0);

  u8g2PrintWithEachChar(langCode, str);
  u8g2.sendBuffer();
  Serial.println("");
}
void u8g2PrintWithEachChar(int langCode, String str)
{
  // str을 String 객체로 변환
  String str_obj = String(str);
  int initialHeight = 5;
  int padding = 2;
  int height = initialHeight + gapWithTextLines;
  u8g2_uint_t x = 0, y = height;
  u8g2.setCursor(padding, y - currentCursorY);
  for (int i = 0; i < str.length();) {
    int charSize = getCharSize(str[i]); // 다음 문자의 크기 계산
    String currentCharStr = str.substring(i, i+charSize); // 다음 문자 추출
    char currentChar = currentCharStr.charAt(0);
    int charWidth = getCharWidth(currentChar, langCode);
    if (x + charWidth >  108) {
        x = padding;
        y += gapWithTextLines;
        u8g2.setCursor(x, y - currentCursorY);
    }
    u8g2.print(currentCharStr);
    x += charWidth;
    i += charSize; // 다음 문자로 이동
  }
  maxCursorY = y + gapWithTextLines * 2;
}
int getCharSize(char c) {
  if ((c & 0x80) == 0) {
    // ASCII 문자인 경우 크기는 1
    return 1;
  } else if ((c & 0xE0) == 0xC0) {
    // 2바이트 문자인 경우 크기는 2
    return 2;
  } else if ((c & 0xF0) == 0xE0) {
    // 3바이트 문자인 경우 크기는 3
    return 3;
  } else if ((c & 0xF8) == 0xF0) {
    // 4바이트 문자인 경우 크기는 4
    return 4;
  } else {
    // 그 외의 경우 잘못된 문자이므로 크기는 1
    return 1;
  }
}
// 입력된 문자가 punctuation인지 여부를 반환하는 함수
bool isPunctuation(char c) {
    if (c >= 32 && c <= 47) return true; // sp!"#$%&'()*+,-./
    if (c >= 58 && c <= 64) return true; // :;<=>?@
    if (c >= 91 && c <= 96) return true; // [\]^_`
    if (c >= 123 && c <= 126) return true; // {|}~
    return false;
}
bool isThaiPunctuation(String s) {
  // Thai punctuation characters
  const String thaiPunctuations[] = {"็", "ั", "ิ", "ี"};

  // Check if s matches any Thai punctuation character
  for (int i = 0; i < 4; i++) {
    if (s == String(thaiPunctuations[i].charAt(0)) + String(thaiPunctuations[i].charAt(1))) {
      return true;
    }
  }
  
  return false;
}

// 입력된 문자가 숫자인지 여부를 반환하는 함수
bool isNumber(char c) {
    return (c >= 48 && c <= 57);
}

// 입력된 문자가 알파벳인지 여부를 반환하는 함수
bool isAlphabet(char c) {
    return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}


String replaceChinesePunctuations(String str) {
  const char* punctuations[] = {"，", "。", "！", "？", "；", "：", "、", "（", "）"};
  const char* punctuationsForReplace[] = {", ", "。", "!", "?", ";", ":", "、", "(", ")"};
  // const char* punctuations[] = {"，", "。", "！", "？", "；", "：", "、", "（", "）"};
  // const char* punctuationsForReplace[] = {", ", "。", "!", "?", ";", ":", "、", "(", ")"};

  for (int i = 0; i < sizeof(punctuations)/sizeof(punctuations[0]); i++) {
    str.replace(punctuations[i], punctuationsForReplace[i]);
  }

  return str;
}
int getCharWidth(char c, int langCode) {

  bool _isPunctuation = isPunctuation(c);
  bool _isAlphabet = isAlphabet(c);
  bool _isNumber = isNumber(c);
  String s = String(c);
  if (langCode == 5) {  // if language is Chinese
    if (_isPunctuation){
      return 4; 
    }
    else if(_isAlphabet)
    {
      return 8;
    } 
    else {
      return 13;
    }
  } 
  else if (langCode == 12) {
    if (_isPunctuation){
      return 4; 
    }
    else {
      return 16; 
    }
  } 
  else if (langCode == 10) {
    if (_isPunctuation){
      return 4; 
    }
    else {
      return 14; 
    }
  } 
  else { 
    if (_isPunctuation){
      return 4; 
    }
    else {
      return 8; 
    }
   // return u8g2.getUTF8Width(s.c_str());
  }
}
void ChangeUTF(int langCodeInt)
{
  int CHARCOUNT_ENGLISH = 17;
  int CHARCOUNT_STANDARD = 15;
  int CHARCOUNT_CHINA = 27;
  int CHARCOUNT_EUROPE = 17;
  int CHARCOUNT_RUSSIA = 27;
  const uint8_t *FONT_ENGLISH = u8g2_font_ncenR12_tr; 
  const uint8_t *FONT_STANDARD = u8g2_font_unifont_t_symbols; 
  const uint8_t *FONT_EUROPE = u8g2_font_7x14_tf; 
  //const uint8_t *FONT_CHINA = u8g2_font_wqy14_t_gb2312a; 
  const uint8_t *FONT_JAPAN = u8g2_font_unifont_t_japanese1; 
  const uint8_t *FONT_RUSSIA = u8g2_font_cu12_t_cyrillic; 

  switch (langCodeInt) {
    case 1: // English
        u8g2.setFont(FONT_ENGLISH);
        break;
    case 2: // Spanish
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 3: // French
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 4: // German
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 5: // Chinese
     //    u8g2.setFont(FONT_CHINA); //중국어 4040자 133,898바이트
        break;
    case 6: // Arabic
        u8g2.setFont(u8g2_font_cu12_t_arabic); 
        break;
    case 7: // Russian
        u8g2.setFont(FONT_RUSSIA); 
        break;
    case 9: // Italian
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 10: // Japanese
        u8g2.setFont(FONT_JAPAN);
        break;
    case 11: // Dutch
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 12: // Korean
     //   u8g2.setFont(u8g2_font_unifont_t_korean2);
        u8g2.setFont(u8g2_korea_kang4);
        break;
    case 13: // Swedish å 
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 14: // Turkish
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 15: // Polish
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 16: // Danish å 
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 17: // Norwegian å 
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 18: // Finnish
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 19: // Czech
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 20: // Thai
        u8g2.setFont(u8g2_font_etl16thai_t);
        break;
    case 21: // Greek
        u8g2.setFont(u8g2_font_unifont_t_greek); 
        break;
    case 22: // Hungarian
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 23: // Hebrew
        u8g2.setFont(u8g2_font_cu12_t_hebrew); 
        break;
    case 24: // Romanian
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 25: // Ukrainian
        u8g2.setFont(FONT_RUSSIA); 
        break;
    case 26: // Vietnamese
        u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
        break;
    case 27: // Icelandic
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 28: // Bulgarian
        u8g2.setFont(FONT_RUSSIA); 
        break;
    case 29: // Lithuanian
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 30: // Latvian
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 31: // Slovenian
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 32: // Croatian
        u8g2.setFont(u8g2_font_helvR12_te); 
        break;
    case 41: // Indonesian
        u8g2.setFont(FONT_EUROPE); 
        break;
    default:
        u8g2.setFont(FONT_STANDARD); 
        break;
  }
}


void record_process()
{
uint16_t val = analogRead(36);
    val = val >> 4;
    buffer[write_data_count] = val;
    write_data_count++;
    if (write_data_count >= RECORDING_DATA_SIZE)
    {
      Serial.println(F("RECORDING COMPLETED"));
      Serial.println(millis() - start_millis);
      SPIFFS.remove(filename);
      delay(100);
      file = SPIFFS.open(filename, "w");
      if (file == 0)
      {
        Serial.println("FILE WRITE FAILED");
      }
      CreateWavHeader(header, RECORDING_DATA_SIZE);
      Serial.println(headerSize);
      int sum_size = 0;
      while (sum_size < headerSize)
      {
        sum_size = sum_size + file.write(header + sum_size, headerSize - sum_size);
      }
      Serial.println(RECORDING_DATA_SIZE);
      sum_size = 0;
      while (sum_size < RECORDING_DATA_SIZE)
      {
        sum_size = sum_size + file.write(buffer + sum_size, RECORDING_DATA_SIZE - sum_size);
      }
      file.flush();
      file.close();
      mode = 0;
      Serial.println("SAVING COMPLETED");
      print_file_list();
    }
}

//format bytes
String formatBytes(size_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
  else
  {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
void print_file_list()
{
  {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
      String fileName = file.name();
      size_t fileSize = file.size();
      file = root.openNextFile();
    }
    file.close();
  }
}

bool exists(String path)
{
  bool yes = false;
  File file = SPIFFS.open(path, "r");
  if (!file.isDirectory())
  {
    yes = true;
  }
  file.close();
  return yes;
}

void CreateWavHeader(byte *header, int waveDataSize)
{
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
  header[4] = (byte)(fileSizeMinus8 & 0xFF);
  header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // monoral
  header[23] = 0x00;
  header[24] = 0x40;  // sampling rate 8000
  header[25] = 0x1F;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x40;  // Byte/sec = 8000x1x1 = 16000
  header[29] = 0x1F;
  header[30] = 0x00;
  header[31] = 0x00;
  header[32] = 0x01;  // 8bit monoral
  header[33] = 0x00;
  header[34] = 0x08;  // 8bit
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(waveDataSize & 0xFF);
  header[41] = (byte)((waveDataSize >> 8) & 0xFF);
  header[42] = (byte)((waveDataSize >> 16) & 0xFF);
  header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}

static void get_html(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", index_html);
}

static bool capture_handler1(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/sound1.wav", "audio/wav");
  request->send(response);
  return true;
}

static bool capture_handler2(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/sound2.wav", "audio/wav");
  request->send(response);
  return true;
}

static bool capture_handler3(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/sound3.wav", "audio/wav");
  request->send(response);
  return true;
}

static bool record_handler1(AsyncWebServerRequest *request)
{
  Serial.println("RECORD 1");
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
  Serial.println("WRITING START");
  mode = 1;
  write_data_count = 0;
  strcpy(filename, "/sound1.wav");
  start_millis = millis();
  return true;
}

static bool record_handler2(AsyncWebServerRequest *request)
{
  Serial.println("RECORD 2");
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
  Serial.println("WRITING START");
  mode = 1;
  write_data_count = 0;
  strcpy(filename, "/sound2.wav");
  start_millis = millis();
  return true;
}

static bool record_handler3(AsyncWebServerRequest *request)
{
  Serial.println("RECORD 3");
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
  Serial.println("WRITING START");
  mode = 1;
  write_data_count = 0;
  strcpy(filename, "/sound3.wav");
  start_millis = millis();
  return true;
}
