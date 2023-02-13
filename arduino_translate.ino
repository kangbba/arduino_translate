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

  Message(0, "Device Started");
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
void loop() {
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
     
      Message(0, "Device Connected");

  }

  bool recentMessageExist = nowCallback != previousCallback;
  if(recentMessageExist)
  {
    if (recentMessage.length() > 0 && recentMessage.indexOf(":") != -1 && recentMessage.indexOf(";") != -1) 
    {
      previousCallback = nowCallback;
      int langCode;
      String someMsg;
      parseLangCodeAndMessage(recentMessage, langCode, someMsg);
      Serial.print("langCode ");
      Serial.print(langCode);
      Serial.println();
      Serial.print("someMsg ");
      Serial.print(someMsg);
      Serial.println();
      Message(langCode, someMsg);
    //  
    //  delay(100);
  //    
    } 
    else 
    {
      Serial.println("Invalid input format. It should be in the format 'langcode:someMsg;'");
    }
  }
  delay(10);
}
void parseLangCodeAndMessage(String input, int &langCode, String &someMsg) {
  int separatorIndex = input.indexOf(":");
  langCode = input.substring(0, separatorIndex).toInt();
  someMsg = input.substring(separatorIndex + 1, input.indexOf(";"));
}

int deviceWidth = 128 - 10;
int fontHeight = 16;
int charWidth = 16;
void Message(int langCode, String msg)
{
    u8g2.clearBuffer();
    ChangeUTF(langCode);

    int padding = 0;
    int cursorX = 0;

    u8g2.setCursor(padding, 15);
    for (int i = 0; i < msg.length(); i++) {
      char currentChar = msg.charAt(i);

      String charString(currentChar);
      ChangeFontSize(langCode, charString);
     // int charWidth = u8g2.getUTF8Width(charString.c_str());
      if (cursorX + charWidth > deviceWidth ) {
          if (currentChar == ' ') continue;  // 공백이면 출력 생략
          cursorX = padding;
          u8g2.setCursor(cursorX, u8g2.getCursorY() + fontHeight);
      }
      u8g2.print(currentChar);
      cursorX += charWidth;
    }
    u8g2.sendBuffer();
}

void ChangeFontSize(int langCode, String charString)
{
  switch(langCode)
  {
    default:
      charWidth = u8g2.getUTF8Width(charString.c_str());
      fontHeight = u8g2.getMaxCharHeight();
      break;
    case 12: // Korean
      charWidth = 16;
      fontHeight = 16;
    case 5: // Chinese
      charWidth = 4;
      fontHeight = 16;
      break;    
  }
}

void ChangeUTF(int langCodeInt)
{
  switch (langCodeInt) {
    case 1: // English
        u8g2.setFont(u8g2_font_unifont_t_korean2);
        break;
    case 2: // Spanish
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 3: // French
        u8g2.setFont(u8g2_font_8x13_tr);   
        break;
    case 4: // German
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 5: // Chinese
        u8g2.setFont(u8g2_font_wqy14_t_gb2312a); //중국어 4040자 133,898바이트
        break;
    case 6: // Arabic
        u8g2.setFont(u8g2_font_unifont_t_arabic);
        break;
    case 7: // Russian
        u8g2.setFont(u8g2_font_unifont_t_cyrillic); 
        break;
    case 8: // Portuguese
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 9: // Italian
        u8g2.setFont(u8g2_font_8x13_tr); 
        break;
    case 10: // Japanese
        u8g2.setFont(u8g2_font_b16_t_japanese2);
        break;
    case 11: // Dutch
        break;
    case 12: // Korean
        u8g2.setFont(u8g2_font_unifont_t_korean1);
        break;
    case 13: // Swedish
        break;
    case 14: // Turkish
        break;
    case 15: // Polish
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
