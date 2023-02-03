//공통
#include "BluetoothSerial.h"
#include <SPI.h>
#include <Wire.h>
//U8G2
#include <U8g2lib.h>
#include <Arduino.h>
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

//Bluetooth
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

int previousLangCode = -1;

void setup() {
  Serial.begin(115200);

  //Initialize U8G2 
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setFontDirection(0);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_korean1);
  Message("U8G2 START!!");

  //Initialize Bluetooth
  SerialBT.begin("ESP32hyehwa"); //Bluetooth device name
  Serial.println("블루투스 시작됨");
}


void loop() {
  if (Serial.available()) {
    SerialBT.write(Serial.read());
    Serial.println(Serial.readString());
  }
  
  if (SerialBT.available()) {
    String fullMsg = SerialBT.readString();
    Serial.println(fullMsg);

    //메세지 형식 => 12:Hello World; 
    //오류시 12:Hello World;12:Hello World;12:Hello World 이런식의 메세지 출력됨.
    //오류방지로 첫 ; 까지만 먼저 읽기
    String beforeFullMsg = Split(fullMsg, ';', 0);

    /////// 국가코드 READ 후 국가코드에 맞는 폰트를 설정한다 /////// 
    int langCodeInt = Split(beforeFullMsg, ':', 0).toInt(); 
    if(previousLangCode != langCodeInt)
    {
      ChangeUTF(langCodeInt);
      previousLangCode = langCodeInt;
    }

    /////// 메세지 READ 후 출력한다 ///////
    String finalMsg = Split(beforeFullMsg, ':', 1);
    Message(finalMsg);
  }
  delay(20);
}

void ChangeUTF(int langCodeInt)
{
  /*
  ko = 1, ja = 2,  en = 0,  es = 3, fr = 4,  de = 5,
  pt = 6,  it = 7, vi = 8, th = 9, ru = 10, 중국 간체 11, 대만 번체 12
  */
 
  switch(langCodeInt)
  {
    default: 
    // 기본값
      u8g2.setFont(u8g2_font_unifont_t_korean1); 
      break;
    case 0:
    //영어 
      u8g2.setFont(u8g2_font_unifont_t_korean1); 
      break;
    case 1:
    //한국 
      u8g2.setFont(u8g2_font_unifont_t_korean1); 
      break;
    case 2:
    //일본
      u8g2.setFont(u8g2_font_b10_t_japanese1);
      break;
    case 3:
    //스페인
      break;
    case 4:
    //프랑스
      break;
    case 5: 
    // 독일
  //    u8g2.setFont(u8g2_font_unifont_t_deutsche);
      break;
    case 6: 
    // 포르투갈
  //    u8g2.setFont(u8g2_font_unifont_t_Portugiesisch1);
      break;
    case 7: 
    // 이탈리아
      break;
    case 8: 
    // 베트남
      u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
      break;
    case 9: 
    // 태국
      u8g2.setFont(u8g2_font_etl14thai_t); 
      break;
    case 10: 
    // 러시아
      break;
    case 11: 
      u8g2.setFont(u8g2_font_unifont_t_chinese1);
    // 중국(간체)
      break;
    case 12: 
      u8g2.setFont(u8g2_font_unifont_t_chinese2);
    // 대만(번체)
      break;
  }
}
      

String Split(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Message(String msg){

  u8g2.clearBuffer();
  int lineLength = 15;
  int len = msg.length();
  int lines = len / lineLength + 1;
  int height = 10;
  for (int i = 0; i < lines; i++) {
  int end = min((i + 1) * lineLength, len);
  String line = msg.substring(i * lineLength, end);
  u8g2.drawStr(0, height, line.c_str());
  height += 10;
  }

  u8g2.sendBuffer();
}






//ETC

//u8g2.setFont(u8g2_font_unifont_h_utf8);
//u8g2.setFont(u8g2_font_unifont_t_korean1); 
//u8g2.setFont(u8g2_font_unifont_t_korean1); 
//u8g2.setFont(u8g2_font_b10_t_japanese1);
//u8g2.setFont(u8g2_font_unifont_h_utf8); 
//u8g2.setFont(u8g2_font_unifont_t_korean_NanumGothicCoding_16);
//u8g2.setFont(u8g2_font_unifont_t_chinese2); 
//u8g2.setFont(u8g2_font_b10_t_japanese1); 
//u8g2.setFont(u8g2_font_unifont_t_cjk);
//u8g2.setFont(u8g2_font_unifont_t_arabic);//아랍어
//u8g2.setFont(u8g2_font_unifont_t_polish);//폴란드어
//u8g2.setFont(u8g2_font_unifont_t_vietnamese1);//베트남어
//u8g2.setFont(u8g2_font_unifont_t_deutsche;//독일어
//u8g2.setFont(u8g2_font_unifont_t_Portugiesisch1);
//포르투칼어u8g2_font_unifont_tf

//u8g2.setFont(u8g2_font_unifont_te);
//u8g2.setFont(u8g2_font_unifont_t_latin);
//u8g2.setFont(u8g2_font_unifont_t_extended);
//u8g2.setFont(u8g2_font_unifont_t_greek);
//u8g2.setFont(u8g2_font_unifont_t_cyrillic);
//u8g2.setFont(u8g2_font_unifont_t_hebrew);
//u8g2.setFont(u8g2_font_unifont_t_bengali);
//u8g2.setFont(u8g2_font_unifont_t_tibetan0;
//u8g2.setFont(u8g2_font_unifont_t_urdu):
//u8g2.setFont(u8g2_font_unifont_t_devanagari)
//u8g2.setFont(u8g2_font_unifont_t_symbols);
//u8g2.setFont(u8g2_font_unifont_t_emticons);
//u8g2.setFont(u8g2_font_unifont_t_animals);
//u8g2.setFont(u8g2_font_unifont_t_chinese1);
//u8g2.setFont(u8g2_font_unifont_t_chinese2);
//u8g2.setFont(u8g2_font_unifont_t_chinese3);
//u8g2.setFont(u8g2_font_unifont_t_korean1);
//u8g2.setFont(u8g2_font_unifont_t_korean2);
//u8g2.setFont(u8g2_font_unifont_t_korean3);
//u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
//u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
//u8g2.setFont(u8g2_font_unifont_t_cjk);


// u8g2.setCursor(0, 32);

//u8g2.print("Hello World!");
//u8g2.print("مرحبا، انا سعيد لرؤيتك");
//u8g2.print("你好, 世界!");
// u8g2.print("어서오세요,반갑습니다");
//u8g2.print("こんにちは世界!");
//u8g2.print("Cześć miło");//폴란드어
//u8g2.print("xin chào");//베트남어
// u8g2.print("Hola mucho gusto");스페인어
//u8g2.print("Schön dich kennenzulernen");//독일어
//u8g2.print("Prazer em conhecê-la");//포르투칼어
//u8g2.print("Piacere di conoscerti");//이탈리아

//   u8g2.sendBuffer();
  