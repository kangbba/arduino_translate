/////////////////////////////////////////////////////VERSION : BANGAWER 7 /////////////////////////////////////////////////////////////

//3월14일 (BANGAWER 7)
//인도네시아어 데이터 잘 넘어오고 출력도 잘되는지 확인했음. 나중에 어플에서 인도네시아어 고를수있게 풀어주면됨.
//앞으로 타이어만 하면되는상황.

//2월28일 (BANGAWER 6)
//중요 : 중국어용 구두점 출력안되는 현상 수정

//2월26일 (BANGAWER5)
//중요 : 재실행시 이전 정보가 남아있는 버그 수정.
//openingment banGawer 기존거 좀 올드해보여서 글씨체 바꿔보는중 (마음에 안들면 openingment() 함수안의 내용을 수정하면 됨)
//실패 : 태국어 몇글자 폰트, 아랍어 몇글자 폰트

//2월25일 (BANGAWER4)
//이번버전에 해결됨 : 폴란드어, 체코어, 리투아니안어(s위에 ^) , 라트비안 (c위에 ^) , 슬로베니아(c위에 ^), 크로아티아 (c위에 ^)
//실패 : 태국어 몇글자 폰트, 아랍어 몇글자 폰트

//2월24일 (BANGAWER3)
//줄바꿈 + 스크롤 버전 
//한글 출력 성공이지만 옛날 한글로 해둠.
//성공 : 프랑스, 베트남, 중국, 일본, 구한국어, 신한국어, 영어, 스페인어 , 포루투갈 , 독일어, 프랑스, 중국어, 러시아, 이탈리아, 네덜란드(덧치) , 스웨덴어, 터키어,  덴마크 danish, 노르웨이, 필란드어, 그리스, 헝가리 , 히브리, 루마니아
// 우크라이나어, 아이슬란드, 불가리아, 
//실패 : 태국어 몇글자 폰트, 아랍어 몇글자 폰트, 폴란드어, 체코어, 리투아니안어(s위에 ^) , 라트비안 (c위에 ^) , 슬로베니아(c위에 ^), 크로아티아 (c위에 ^)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//5 14 15 
//5 2 16

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

#include "u8g2_korea_kang4.h"

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
  }
  if (accumTimeForScroll >= scrollStartDelayTime)
     scrollWithInterval(scrollDelay);
  else{
    accumTimeForScroll += deltaTime;
  }
  previousMillis = currentMillis;
  
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

// // 입력된 문자가 중국어 문자인지 여부를 반환하는 함수
// bool isChinese(char c) {
//     return (c >= -24389 && c <= -20167); // 중국어 문자의 유니코드 범위
// }

// // 입력된 문자가 한글 문자인지 여부를 반환하는 함수
// bool isKorean(char c) {
//     return ((c >= -52268 && c <= -49324) || (c >= -8400 && c <= -8179)); // 한글 문자의 유니코드 범위
// }

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
  const uint8_t *FONT_CHINA = u8g2_font_wqy14_t_gb2312a; 
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
         u8g2.setFont(FONT_CHINA); //중국어 4040자 133,898바이트
        break;
    case 6: // Arabic
        u8g2.setFont(u8g2_font_cu12_t_arabic); 

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
    case 33: // Estonian
        u8g2.setFont(FONT_STANDARD); 
        break;
    case 41: // Indonesian
        u8g2.setFont(FONT_EUROPE); 
        break;
    default:
        u8g2.setFont(FONT_STANDARD); 
        break;
  }
}
