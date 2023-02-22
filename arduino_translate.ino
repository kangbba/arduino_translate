// CHARACTERISTIC_UUID_TX를 통해 데이터를 보내야 합니다.

/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
// #define SERVICE_UUID           "2ef826d2-a48d-11ed-a8fc-0242ac120002" // UART service UUID
// #define CHARACTERISTIC_UUID_RX "2ef82e8e-a48d-11ed-a8fc-0242ac120002"
// #define CHARACTERISTIC_UUID_TX "2ef83078-a48d-11ed-a8fc-0242ac120002"
/*
  Fontname: -FontForge-DOSGothic-Medium-R-Normal--16-150-75-75-P-159-ISO10646-1
  Copyright: Copyright (c) 2016 Damheo Lee
  Glyphs: 95/24869
  BBX Build Mode: 0
*/


#include <utf8.h>

#include "BluetoothSerial.h"
#include <SPI.h>
#include <Wire.h>
//U8G2
#include <U8g2lib.h>
#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// #include "u8g2_korea_kang4.h"

//Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


//U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 5, /* dc=*/ 14, /* reset=*/ 15); //scl=18, sda=23  SPI로 변경
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);
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
        Serial.println("********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();

        recentMessage = rxValue.c_str(); 

        Serial.print("Received Value to String : ");
        Serial.print(recentMessage);
        Serial.println();

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
}

void initBLEDevice()
{
  // Create the BLE Device
  BLEDevice::init("banGawer");
  // Create the BLE Server
  pServer = BLEDevice::createServer();
 // pServer->setMtu(256);
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
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
  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

//////////////////////////////////////////////////////////////////////////////////////////LOOP////////////////////////////////////////////////////////////////////////////////////////

  void openingMent()
  {
    String str =  "banGawer";
    u8g2.clearBuffer(); // 버퍼 초기화
    u8g2.setFont(u8g2_font_lubBI08_te);
    u8g2.drawUTF8(32, 34, str.c_str());
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
  }
  if (accumTimeForScroll >= scrollStartDelayTime)
     scrollWithInterval(scrollDelay);
  else{
    accumTimeForScroll += deltaTime;
  }
  previousMillis = currentMillis;


    // Serial.print("langCode ");
    // Serial.print(langCode);
    // Serial.println();
    // Serial.print("someMsg ");
    // Serial.print(someMsg);
    // Serial.println();
    // Serial.print("someMsg : ");
    // Serial.println(someMsg);
   // 
  
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
      // reset scrollStartTime to currentMillis so the next scroll interval starts from now
      scrollStartTime = currentMillis;
    }
  }
  
}

void parseLangCodeAndMessage(String input, int &langCode, String &someMsg) {
  int separatorIndex = input.indexOf(":");
  langCode = input.substring(0, separatorIndex).toInt();
  someMsg = input.substring(separatorIndex + 1, input.indexOf(";"));
}

// void Message(int langCode, String str, bool useScroll)
// {
//   u8g2.setCursor(8,8);
//   u8g2.setFont(u8g2_font_unifont_t_korean2);
//   u8g2.print("안녕하세요");
// }


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
        Serial.print("유효하지않은 char 발견 ");
        Serial.println(charStr);
        return false;
    }
    return true;
}




void Message(int langCode, String str)
{
  ChangeUTF(langCode);
  u8g2.setFlipMode(0);

  // str을 String 객체로 변환
  String str_obj = String(str);

  // 출력 위치 초기화
  int initialHeight = 5;
  int padding = 2;
  int height = initialHeight + gapWithTextLines;

  u8g2_uint_t x = 0, y = height;
  u8g2.setCursor(padding, y - currentCursorY);
  for (int i = 0; i < str.length();) {
    int charSize = getCharSize(str[i]); // 다음 문자의 크기 계산
    String currentCharStr = str.substring(i, i+charSize); // 다음 문자 추출
    Serial.print(currentCharStr);
    //  if (!isCharValid(currentCharStr)) { // 문자가 유효하지 않은 경우 스킵
    //     i += charSize; // 다음 문자로 이동
    //     continue;
    // }
    char currentChar = currentCharStr.charAt(0);
    int charWidth = getCharWidth(currentChar, langCode);
   // int charWidth = charSize;
    // 이번 문자 출력
    if (x + charWidth >  108) {
        // if (isNumber(currentChar) || isPunctuation(currentChar)) {
        //     continue;  // skip space and punctuation characters
        // }
        x = padding;
        y += gapWithTextLines;
        u8g2.setCursor(x, y - currentCursorY);
    }
    u8g2.print(currentCharStr);
    x += charWidth;

    i += charSize; // 다음 문자로 이동
  }
  maxCursorY = y + gapWithTextLines * 2;
  u8g2.sendBuffer();
  u8g2.clearBuffer(); // 버퍼 초기화
  Serial.println("");
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

// 입력된 문자가 숫자인지 여부를 반환하는 함수
bool isNumber(char c) {
    return (c >= 48 && c <= 57);
}

// 입력된 문자가 알파벳인지 여부를 반환하는 함수
bool isAlphabet(char c) {
    return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122));
}

// // 입력된 문자가 중국어 문자인지 여부를 반환하는 함수
// bool isChinese(char c) {
//     return (c >= -24389 && c <= -20167); // 중국어 문자의 유니코드 범위
// }

// // 입력된 문자가 한글 문자인지 여부를 반환하는 함수
// bool isKorean(char c) {
//     return ((c >= -52268 && c <= -49324) || (c >= -8400 && c <= -8179)); // 한글 문자의 유니코드 범위
// }

String replaceChinesePunctuations(String str) {
  // const char* punctuations[] = {"，", "。", "！", "？", "；", "：", "、", "（", "）"};
  // const char* punctuationsForReplace[] = {", ", "。", "!", "?", ";", ":", "、", "(", ")"};
  const char* punctuations[] = {"，", "？"};
  const char* punctuationsForReplace[] = {",", "?"};

  for (int i = 0; i < sizeof(punctuations)/sizeof(punctuations[0]); i++) {
    str.replace(punctuations[i], punctuationsForReplace[i]);
  }

  return str;
}
int getCharWidth(char c, int langCode) {

  bool _isPunctuation = isPunctuation(c);
  bool _isAlphabet = isAlphabet(c);
  bool _isNumber = isNumber(c);
  if (langCode == 5) {  // if language is Chinese
    if (_isPunctuation){
      return 4; 
    }
    else if(_isAlphabet)
    {
      return 4;
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
    String s = String(c);
    return u8g2.getUTF8Width(s.c_str());
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
  const uint8_t *FONT_KOREA = u8g2_font_unifont_t_korean2; 
  const uint8_t *FONT_STANDARD = u8g2_font_unifont_t_symbols; 
  const uint8_t *FONT_EUROPE = u8g2_font_7x14_tf; 
  const uint8_t *FONT_CHINA = u8g2_font_wqy14_t_gb2312a; 
  const uint8_t *FONT_JAPAN = u8g2_font_unifont_t_japanese1; 
  const uint8_t *FONT_RUSSIA = u8g2_font_cu12_t_cyrillic; 

// best u8g2_font_unifont_t_cyrillic
// u8g2_font_crox3h_tr x
// u8g2_font_9x15_t_cyrillic o (일부생략)
// u8g2_font_10x20_t_cyrillic
// u8g2_font_unifont_t_cyrillic o
// u8g2_font_fub25_tu x 

//best u8g2_font_t0_13b_tf
//후보  u8g2_font_helvR12_tr u8g2_font_helvR10_tf 뭔가 안깔끔 u8g2_font_profont12_tf u8g2_font_4x6_mr u8g2_font_t0_14b_tf 
//europe u8g2_font_helvR14_tr u8g2_font_fub11_tf너무 굵음u8g2_font_helvR14_tf u8g2_font_ncenB14_tr

//best : u8g2_font_f16_t_japanese2
//u8g2_font_k14_t_japanese1, u8g2_font_k14_t_japanese2
//일본어 u8g2_font_wqy15_t_gb2312 u8g2_font_wqy13_t_gb2312 u8g2_font_wqy14_t_gb2312한자짤림 
//u8g2_font_wqy15_t_gb2312 u8g2_font_wqy16_t_gb2312양호 한자좀짤림 


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
         u8g2.setFont(FONT_CHINA); //중국어 4040자 133,898바이트
        break;
    case 6: // Arabic
        u8g2.setFont(u8g2_font_unifont_t_arabic); 
        break;
    case 7: // Russian
        u8g2.setFont(FONT_RUSSIA); 
        break;
    case 8: // Portuguese
        u8g2.setFont(FONT_STANDARD); 
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
        u8g2.setFont(u8g2_font_unifont_t_korean2);
       // u8g2.setFont(u8g2_korea_kang4);
        break;
    case 13: // Swedish å 
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 14: // Turkish
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 15: // Polish
        u8g2.setFont(FONT_EUROPE); 
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
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 20: // Thai
        u8g2.setFont(u8g2_font_etl14thai_t);
        //u8g2_font_etl14thai_t
        //u8g2_font_ncenB08_tr
        //u8g2_font_ncenB14_tr 
        break;
    case 21: // Greek
        u8g2.setFont(FONT_EUROPE); 
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
        u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
        break;
    case 27: // Icelandic
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 28: // Bulgarian
        u8g2.setFont(FONT_STANDARD); 
        break;
    case 29: // Lithuanian
        u8g2.setFont(FONT_STANDARD); 
        break;
    case 30: // Latvian
        u8g2.setFont(FONT_STANDARD); 
        break;
    case 31: // Slovenian
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 32: // Croatian
        u8g2.setFont(FONT_EUROPE); 
        break;
    case 33: // Estonian
        u8g2.setFont(FONT_STANDARD); 
        break;
    default:
        u8g2.setFont(FONT_STANDARD); 
        break;
  }
}

/*
u8g2_font_wqy12_t_chinese1	411	9,491
u8g2_font_wqy12_t_chinese2	574	13,701
u8g2_font_wqy12_t_chinese3	993	25,038
u8g2_font_wqy12_t_gb2312a	4041	111,359
u8g2_font_wqy12_t_gb2312b	4531	120,375
u8g2_font_wqy12_t_gb2312	7539	208,228
u8g2_font_wqy13_t_chinese1	411	10,341
u8g2_font_wqy13_t_chinese2	574	14,931
u8g2_font_wqy13_t_chinese3	993	27,370
u8g2_font_wqy13_t_gb2312a	4041	121,327
u8g2_font_wqy13_t_gb2312b	4531	130,945
u8g2_font_wqy13_t_gb2312	7539	227,383
u8g2_font_wqy14_t_chinese1	411	11,368
u8g2_font_wqy14_t_chinese2	574	16,443
u8g2_font_wqy14_t_chinese3	993	30,200
u8g2_font_wqy14_t_gb2312a	4040	133,898
u8g2_font_wqy14_t_gb2312b	4530	143,477
u8g2_font_wqy14_t_gb2312	7538	251,515
u8g2_font_wqy15_t_chinese1	411	12,590
u8g2_font_wqy15_t_chinese2	574	18,133
u8g2_font_wqy15_t_chinese3	993	33,165
u8g2_font_wqy15_t_gb2312a	4041	147,563
u8g2_font_wqy15_t_gb2312b	4531	158,713
u8g2_font_wqy15_t_gb2312	7539	276,938
u8g2_font_wqy16_t_chinese1	411	14,229
u8g2_font_wqy16_t_chinese2	574	20,245
u8g2_font_wqy16_t_chinese3	993	37,454
u8g2_font_wqy16_t_gb2312a	4041	169,286
u8g2_font_wqy16_t_gb2312b	4531	182,271
u8g2_font_wqy16_t_gb2312	7539	318,090
*/

/*
u8g2_font_f16_t_japanese1:

지원 글자 수: 3,338자 (JIS X 0208 + NEC + IBM Extended)
폰트 용량: 약 65.5KB
u8g2_font_f16_t_japanese2:

지원 글자 수: 4,965자 (JIS X 0208 + NEC + IBM Extended + IBM Selectric Composer)
폰트 용량: 약 98.1KB
u8g2_font_f16_t_japanese3:

지원 글자 수: 13,108자 (JIS X 0208 + NEC + IBM Extended + IBM Selectric Composer + JIS X 0213:2004)
폰트 용량: 약 257.9KB
u8g2_font_f16_t_japanese4:

지원 글자 수: 24,083자 (JIS X 0208 + NEC + IBM Extended + IBM Selectric Composer + JIS X 0213:2004 + 管理画面漢字)
폰트 용량: 약 475.2KB
*/
