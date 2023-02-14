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



//Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif


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

int gapWithTextLines = 16;
int charCountPerLine = 12;


//doing scroll
unsigned long scrollStartTime = 0;
unsigned long accumTimeForScroll = 0;
long previousMillis = 0;

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

  Message(0, "Device Started", false);
}

void initBLEDevice()
{
  // Create the BLE Device
  BLEDevice::init("CANDY3");
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
      Message(0, "Device Connected", false);
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
    previousCallback = nowCallback;
    int langCode;
    String someMsg;
    parseLangCodeAndMessage(recentMessage, langCode, someMsg);
    // Serial.print("langCode ");
    // Serial.print(langCode);
    // Serial.println();
    // Serial.print("someMsg ");
    // Serial.print(someMsg);
    // Serial.println();
    // Serial.print("someMsg : ");
    // Serial.println(someMsg);
    someMsg.trim();
    Message(langCode, someMsg, true);
  } 
  else 
  {
    Serial.println("Invalid input format. It should be in the format 'langcode:someMsg;'");
  }
  if (accumTimeForScroll >= 2000)
     scrollWithInterval(100);
  else{
    accumTimeForScroll += deltaTime;
  }
  previousMillis = currentMillis;
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

//////////////////////////////////////////////////////////////////////////////////////////LOOP////////////////////////////////////////////////////////////////////////////////////////

void parseLangCodeAndMessage(String input, int &langCode, String &someMsg) {
  int separatorIndex = input.indexOf(":");
  langCode = input.substring(0, separatorIndex).toInt();
  someMsg = input.substring(separatorIndex + 1, input.indexOf(";"));
}

void Message(int langCode, String str, bool useScroll)
{
 // u8g2.drawHLine(0, 4 - currentCursorY, deviceWidth);

  ChangeUTF(langCode);
  u8g2.setFlipMode(0);

  // str을 String 객체로 변환
  String str_obj = String(str);

  // 한 줄당 출력할 문자 수
  int line_len = charCountPerLine;

  // 출력 위치 초기화
  int initialHeight = 5;

  int height = initialHeight + gapWithTextLines;
  u8g2_uint_t x = 0, y = height;
  
  // 문자열을 일정 길이 단위로 나누어 출력
  for (int i = 0; i < str_obj.length(); i += line_len) {
    // 출력할 문자열 추출
    String line = str_obj.substring(i, i + line_len);
    
    // UTF-8 문자열을 출력
    u8g2.drawUTF8(x, y - currentCursorY, line.c_str());
    
    // 출력 위치를 다음 줄로 이동
    y += gapWithTextLines;
  }
  maxCursorY = y;

  u8g2.sendBuffer();
  u8g2.clearBuffer(); // 버퍼 초기화

}
void ChangeUTF(int langCodeInt)
{
  charCountPerLine = 12; // 이것은 기본값이므로 여기서 수정하지말고 아래에서 나라별로 수정하세요.

//charCountPerLine는 화면에 표시되는 한줄에 몇개의 글자를 표시할지를 정하는 변수임
//charCountPerLine를 자동으로 계산하고 싶지만 폰트가 다양하고 그 정보가 누락된 경우가많아서 직접지정해줘야 한다.
//화면에 글자가 다 안 담길경우 아래의 charCountPerLine 값을 줄여서 한줄당 표시될 글자수를 줄여주셈.
//화면은 넉넉한데 글자가 몇자 쓰지도않고 줄을바꾸면 charCountPerLine 값을 늘려서 한줄당 표시될 글자수를 늘려주셈.

  switch (langCodeInt) {
    case 1: // English
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_unifont_t_korean2);
        break;
    case 2: // Spanish
        charCountPerLine = 13;
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 3: // French
        charCountPerLine = 13;
        u8g2.setFont(u8g2_font_8x13_tr);   
        break;
    case 4: // German
        charCountPerLine = 13;
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 5: // Chinese
        charCountPerLine = 27;
        u8g2.setFont(u8g2_font_wqy14_t_gb2312a); //중국어 4040자 133,898바이트
        break;
    case 6: // Arabic
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_unifont_t_arabic);
        break;
    case 7: // Russian
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_unifont_t_cyrillic); 
        break;
    case 8: // Portuguese
        charCountPerLine = 13;
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 9: // Italian
        charCountPerLine = 13;
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 10: // Japanese
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_b16_t_japanese2);
        break;
    case 11: // Dutch
        charCountPerLine = 14;
        break;
    case 12: // Korean
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_unifont_t_korean1);
        break;
    case 13: // Swedish
        charCountPerLine = 14;
        break;
    case 14: // Turkish
        charCountPerLine = 14;
        break;
    case 15: // Polish
        charCountPerLine = 14;
        u8g2.setFont(u8g2_font_unifont_t_polish);
        break;
    case 16: // Danish
        break;
    case 17: // Norwegian
        break;
    case 18: // Finnish
        break;
    case 19: // Czech
        break;
    case 20: // Thai
        u8g2.setFont(u8g2_font_etl14thai_t); 
        break;
    case 21: // Greek
        u8g2.setFont(u8g2_font_unifont_t_greek); 
        break;
    case 22: // Hungarian
        break;
    case 23: // Hebrew
        break;
    case 24: // Romanian
        break;
    case 25: // Ukrainian
        break;
    case 26: // Vietnamese
        u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
        break;
    case 27: // Icelandic
        break;
    case 28: // Bulgarian
        break;
    case 29: // Lithuanian
        break;
    case 30: // Latvian
        break;
    case 31: // Slovenian
        break;
    case 32: // Croatian
        break;
    case 33: // Estonian
        break;
    default:
        u8g2.setFont(u8g2_font_unifont_t_korean2);
        break;
  }
}


// u8g2_font_wqy12_t_chinese1	411	9,491
// u8g2_font_wqy12_t_chinese2	574	13,701
// u8g2_font_wqy12_t_chinese3	993	25,038
// u8g2_font_wqy12_t_gb2312a	4041	111,359
// u8g2_font_wqy12_t_gb2312b	4531	120,375
// u8g2_font_wqy12_t_gb2312	7539	208,228
// u8g2_font_wqy13_t_chinese1	411	10,341
// u8g2_font_wqy13_t_chinese2	574	14,931
// u8g2_font_wqy13_t_chinese3	993	27,370
// u8g2_font_wqy13_t_gb2312a	4041	121,327
// u8g2_font_wqy13_t_gb2312b	4531	130,945
// u8g2_font_wqy13_t_gb2312	7539	227,383
// u8g2_font_wqy14_t_chinese1	411	11,368
// u8g2_font_wqy14_t_chinese2	574	16,443
// u8g2_font_wqy14_t_chinese3	993	30,200
// u8g2_font_wqy14_t_gb2312a	4040	133,898
// u8g2_font_wqy14_t_gb2312b	4530	143,477
// u8g2_font_wqy14_t_gb2312	7538	251,515
// u8g2_font_wqy15_t_chinese1	411	12,590
// u8g2_font_wqy15_t_chinese2	574	18,133
// u8g2_font_wqy15_t_chinese3	993	33,165
// u8g2_font_wqy15_t_gb2312a	4041	147,563
// u8g2_font_wqy15_t_gb2312b	4531	158,713
// u8g2_font_wqy15_t_gb2312	7539	276,938
// u8g2_font_wqy16_t_chinese1	411	14,229
// u8g2_font_wqy16_t_chinese2	574	20,245
// u8g2_font_wqy16_t_chinese3	993	37,454
// u8g2_font_wqy16_t_gb2312a	4041	169,286
// u8g2_font_wqy16_t_gb2312b	4531	182,271
// u8g2_font_wqy16_t_gb2312	7539	318,090
