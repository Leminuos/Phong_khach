// Khai báo thư viện
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <DHT.h>

// Khai báo các chân phòng khách                                    
#define pinLedPK 22       // Khai báo đèn phòng khách
#define pinFanPK 24       // Khai báo quạt phòng khách
#define buttonLedPK 26    // Khai báo nút nhấn đóng mở đèn phòng khách
#define buttonFanPK 28    // Khai báo nút nhấn đóng mở quạt phòng khách
#define pinBuzzer 30      // Khai báo còi
#define pinDHT 32          // Khai báo cảm biến nhiệt độ, độ ẩm
#define CBCD1  34          // Khai báo cảm biến chuyển động

// Khai báo các chân phòng bếp
#define pinLedPB 23        // Khai báo đèn phòng bếp
#define pinFanPB 25        // Khai báo quạt phòng bếp
#define buttonLedPB 27     // Khai báo nút nhấn đóng mở đèn phòng bếp
#define buttonFanPB 29     // Khai báo nút nhấn đóng mở quạt phòng bếp
#define CBGAS 31           // Khai báo chân cảm biến khí gas
#define CBND A0            // Khai báo chân cảm biến nhiệt độ

// Khai báo các chân sân sau

#define pinLedSS 36         // Khai báo đèn sân sau
#define buttonLedSS 38      // Khai báo nút nhấn đóng mở đèn sân sau
#define pinServo 40         // Khai báo động cơ dây phơi
#define buttonDayPhoi 42    // Khai báo nút nhấn đóng mở dây phơi
#define CBCD2 44            // Khai báo cảm biến chuyển động
#define CBMUA 46            // Khai báo cảm biến mưa
#define CBAS 48             // Khai báo cảm biến ánh sáng

const char DHTTYPE = DHT11;             // Loại cảm biến nhiệt độ, độ ẩm dùng là dht11

// Khai báo các biến truyền/nhận dữ liệu
String sendString = "";                     // Dữ liệu gửi
String inputString = "";                    // Dữ liệu nhận
boolean stringComplete = false;                // Kiểm tra dữ liệu đã truyển hết chưa

// Trạng thái các thiết bị trong nhà
boolean stateLedPK = 0, stateFanPK = 0, stateDoor = 0 ;                  // Phòng khách
boolean stateLedPN = 0, stateAirPN = 0;                                  // Phòng ngủ
boolean stateLedPB = 0, stateFanPB = 0;                                  // Phòng bếp
boolean stateLedSS = 0;                                                  // Sân sau

// Khai báo các biến Management System
boolean stateGas = 0, stateFire = 0;    // Trạng thái khí gas và cháy nhà 
boolean dayPhoiAuto = 0;                // Chế độ dây phơi tự động           
int tempPB;                             // Biến dùng lưu trữ nhiệt độ phòng bếp
float temp, humi;                       // Biến dùng lưu trữ nhiệt độ, độ ẩm phòng khách  

// Khai báo các biến tạm
byte pos = 0;                           // Biến dùng lưu trữ góc quay Servo
unsigned long previousTime;             // Lưu thời gian trước đó

// Khai báo các đối tượng 
LiquidCrystal lcd(49, 47, 45, 43, 41, 39); // các chân theo thứ tự RS, E, D4, D5, D6, D7
Servo dayPhoi;
DHT dht(pinDHT, DHTTYPE);
byte degree[8] = {                         // Ký tự đặc biệt "Độ C" trên màn hình LCD
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

void handleOnOff();                        // Hàm bật/ tắt các thiết bị
void updateTempHumi();                     // Hàm cập nhật nhiệt độ, độ ẩm
void settingDayPhoi();                     // Hàm cài đặt dây phơi ở chế độ tự động hay nút nhấn

void setup() {
  // Kết nối serial  với tốc độ truyền dữ liệu 9600
  Serial.begin(9600);                      
  Serial1.begin(9600);                                        
  // Đảm bảo bộ đệm được xoá trước khi truyền nhận dữ liệu
  Serial.flush();                          
  Serial1.flush();
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.createChar(1, degree);   // Lệch tạo ký tự đặc biệt
  dht.begin();

  // Cấu hình chân vào ra phòng khách
  pinMode(CBCD1, INPUT);
  pinMode(pinLedPK, OUTPUT);
  pinMode(pinFanPK, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);
  pinMode(buttonLedPK, INPUT_PULLUP);
  pinMode(buttonFanPK, INPUT_PULLUP);
  digitalWrite(pinLedPK, HIGH);            // Ban đầu đèn tắt
  digitalWrite(pinFanPK, HIGH);            // Ban đầu quạt tắt
  digitalWrite(pinBuzzer, LOW);

  // Cấu hình chân vào ra phòng bếp
  pinMode(CBND, INPUT);
  pinMode(CBGAS, INPUT);
  pinMode(pinLedPB, OUTPUT);
  pinMode(pinFanPB, OUTPUT);
  pinMode(buttonLedPB, INPUT_PULLUP);
  pinMode(buttonFanPK, INPUT_PULLUP);
  digitalWrite(pinLedPB, HIGH);
  digitalWrite(pinFanPB, HIGH);

  // Cấu hình chân vào ra sân sau
  pinMode(CBMUA, INPUT);
  pinMode(CBCD2, INPUT);
  pinMode(CBAS, INPUT);
  pinMode(pinLedSS, OUTPUT);
  pinMode(buttonLedSS, INPUT_PULLUP);
  pinMode(buttonDayPhoi, INPUT_PULLUP);
  digitalWrite(pinLedSS, HIGH);           
  dayPhoi.attach(pinServo);
  dayPhoi.write(pos);
}

void loop() {
  while(Serial.available() > 0){
    char inChar = (char) Serial.read();
    inputString += inChar;
    if(inChar == '\n'){
      stringComplete = true;
    }
    if(stringComplete){
      //Serial.print("Data nhận:");
      //Serial.println(inputString);
      //=======================================================
      // Phòng khách
      if(inputString.indexOf("L1PK") >= 0){
        //Serial.println("Đèn phòng khách bật!");
        digitalWrite(pinLedPK, 0);
        stateLedPK = 1;
      }
      else if(inputString.indexOf("L0PK") >= 0){
        //Serial.println("Đèn phòng khách tắt!");
        digitalWrite(pinLedPK, 1);
        stateLedPK = 0;
      }
      if(inputString.indexOf("F1PK") >= 0){
        //Serial.println("Quạt phòng khách bật!");
        digitalWrite(pinFanPK, 0);
        stateFanPK = 1;
      }
      else if(inputString.indexOf("F0PK") >= 0){
        //Serial.println("Quạt phòng khách tắt!");
        digitalWrite(pinFanPK, 1);
        stateFanPK = 0;
      }
      //===================================================
      // Phòng bếp
      if(inputString.indexOf("L1PB") >= 0){
        //Serial.println("Đèn phòng bếp bật!");
        digitalWrite(pinLedPB, 0);
        stateLedPB = 1;
      }
      else if(inputString.indexOf("L0PB") >= 0){
        //Serial.println("Đèn phòng bếp tắt!");
        digitalWrite(pinLedPK, 1);
        stateLedPK = 0;
      }
      if(inputString.indexOf("F1PN") >= 0){
        //Serial.println("Quạt phòng bếp bật!");
        digitalWrite(pinFanPB, 0);
        stateFanPB = 1;
      }
      else if(inputString.indexOf("F0PB") >= 0){
        //Serial.println("Quạt phòng bếp tắt!");
        digitalWrite(pinFanPB, 1);
        stateFanPB = 0;
      }
      //=====================================================
      // Phòng ngủ
      if(inputString.indexOf("L1PN") >= 0){
        //Serial.println("Đèn phòng ngủ bật!");
        stateLedPN = 1;
        sendString = "L" + String(stateLedPN) + "PN";
        Serial1.println(sendString);
      }
      else if(inputString.indexOf("L0PN") >= 0){
        //Serial.println("Đèn phòng ngủ tắt!");
        stateLedPN = 0;
        sendString = "L" + String(stateLedPN) + "PN";
        Serial1.println(sendString);
      }
      if(inputString.indexOf("A1PN") >= 0){
        //Serial.println("Điều hoà phòng ngủ bật!");
        stateAirPN = 1;
        sendString = "A" + String(stateAirPN) + "PN";
        Serial1.println(sendString);
      }
      else if(inputString.indexOf("A0PN") >= 0){
        //Serial.println("Điều hoà phòng ngủ tắt!");
        stateAirPN = 0;
        sendString = "A" + String(stateAirPN) + "PN";
        Serial1.println(sendString);
      }
      inputString = "";
      stringComplete = false;
    }
  }
  while(Serial1.available() > 0){
    char inChar = (char) Serial1.read();
    inputString += inChar;
    if(inChar == '\n'){
      stringComplete = true;
    }
    if(stringComplete){
      //Serial.print("Data nhận:");
      //Serial.println(inputString);
      //================================================
      if(inputString.indexOf("L1PN") >= 0){
        //Serial.println("Đèn phòng ngủ bật!");
        stateLedPN = 1;
        Serial.println("L" + String(stateLedPN) + "PN");
      }
      else if(inputString.indexOf("L0PN") >= 0){
        //Serial.println("Đèn phòng ngủ tắt!");
        stateLedPN = 0;
        Serial.println("L" + String(stateLedPN) + "PN");
      }
      if(inputString.indexOf("A1PN") >= 0){
        //Serial.println("Điều hoà phòng ngủ bật!");
        stateAirPN = 1;
        Serial.println("A" + String(stateAirPN) + "PN");
      }
      else if(inputString.indexOf("A0PN") >= 0){
        //Serial.println("Điều hoà phòng ngủ tắt!");
        stateAirPN = 0;
        Serial.println("A" + String(stateAirPN) + "PN");
      }
      inputString = "";
      stringComplete = false;
    }
  }
  handleOnOff();
  if(millis() - previousTime > 2000){  // Sau 2s sẽ gửi dữ liệu 
    updateTempHumi();
    sendString =  "H" + String(humi) + "%" + "T" + String(temp) + "C" + "B" + String(tempPB) + "C";
    Serial.println(sendString);
    previousTime = millis();
  }
  settingDayPhoi();
}
void handleOnOff(){
  // Bật/tắt đèn phòng khách
  if (digitalRead(buttonLedPK) == 0){
    delay(200);                                          // Ổn định nút nhấn 
    digitalWrite(pinLedPK, stateLedPK);                  // Bật/ tắt đèn
    stateLedPK =! stateLedPK;                            // Trạng thái hiện tại
    Serial.println("L" + String(stateLedPK) + "PK");     // Gửi trạng thái đèn tới ESP8266
  }
  // Bât/ tắt quạt phòng khách
  if (digitalRead(buttonFanPK) == 0){
    delay(200);
    digitalWrite(pinFanPK, stateFanPK);
    stateFanPK =! stateFanPK;
    Serial.println("F" + String(stateFanPK) + "PK");
  }
  // Bật/tắt đèn phòng bếp
  if (digitalRead(buttonLedPB) == 0){
    delay(200);
    digitalWrite(pinLedPB, stateLedPB);
    stateLedPB =! stateLedPB;
    Serial.println("L" + String(stateLedPB) + "PB");
  }
  // Bât/ tắt quạt phòng bếp
  if (digitalRead(buttonFanPB) == 0){
    delay(200);
    digitalWrite(pinFanPB, stateFanPB);
    stateFanPB =! stateFanPB;
    Serial.println("F" + String(stateFanPB) + "PB");
  }
  // Bật/ tắt đèn sân sau
  if (digitalRead(buttonLedSS) == 0){
    delay(200);
    digitalWrite(pinLedSS, stateLedSS);
    stateLedSS =! stateLedSS;
  }
}

void updateTempHumi(){
  tempPB = 5.0 * (analogRead(CBND)) * 100.0 / 1023.0;
  humi = dht.readHumidity();
  temp = dht.readTemperature();
  if (isnan(humi) || isnan(temp)) {
    lcd.print("ERROR");
    return;
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humi: ");
  lcd.setCursor(6, 0);
  lcd.print(humi);
  lcd.setCursor(11, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.setCursor(6, 1);
  lcd.print(temp);
  lcd.setCursor(11, 1);
  lcd.write(1);
  lcd.setCursor(12, 1);
  lcd.print("C");
}
void writeServo(byte pos){
  if(millis() - previousTime >= 15){ // Đợi 15ms cho servo quay đến góc đó rồi tới bước tiếp theo
    dayPhoi.write(pos);              // Xuất góc vào servo
  }
}
void settingDayPhoi(){
  if (dayPhoiAuto == 1) { 
    if((digitalRead(CBMUA) == 1) && (pos == 0)) // không mưa
    {
      for(pos = 0; pos < 180; pos += 1) // cho servo quay từ 0->179 độ
      {                                  // mỗi bước của vòng lặp tăng 1 độ
        writeServo(pos);
      }
    }
    if((digitalRead(CBMUA) == 0) && (pos == 180)) // co mua
    {
      for(pos = 180; pos >= 1; pos -= 1) // cho servo quay từ 179-->0 độ
      {
        writeServo(pos);
      }
    }
  }
  if (digitalRead(buttonDayPhoi) == 0) // Chế độ dây phơi bằng nút nhấn
  {
    if (pos == 180){
      for(pos = 180; pos > 0; pos--)    // cho servo quay từ 0->179 độ
      {
        writeServo(pos);                               
      }
    }
    if(pos == 0){
      for(pos = 0; pos < 180; pos++) // cho servo quay từ 179-->0 độ
      {
        writeServo(pos);
      }
    }
  }
}