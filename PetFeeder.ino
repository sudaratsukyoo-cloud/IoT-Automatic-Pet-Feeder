#define BLYNK_TEMPLATE_ID "TMPL62kDSntMx"
#define BLYNK_TEMPLATE_NAME "Feed System"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#define IR_SENSOR D1
#define SS_PIN D8
#define RST_PIN D3

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <SPI.h>
#include <MFRC522.h>

char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

Servo myservo;
BlynkTimer timer;
WidgetRTC rtc;
MFRC522 rfid(SS_PIN, RST_PIN);

int startHour1, startMinute1;
int startHour2, startMinute2;

bool fedMorning = false;
bool fedEvening = false;

int feedCount = 0;

// ===== ให้อาหาร =====
void feedFood()
{
  // ===== เช็กจำนวนครั้ง =====
  if(feedCount >= 100)
  {
    Serial.println("Feed limit reached");

    Blynk.virtualWrite(V3, "วันนี้ให้อาหารครบ 3 ครั้งแล้ว");

    Blynk.logEvent("feed_limit", "วันนี้ให้อาหารครบ 3 ครั้งแล้ว");

    return;
  }

  // ===== เพิ่มจำนวนครั้ง =====
  feedCount++;

  Serial.print("Feed Count = ");
  Serial.println(feedCount);

  Serial.println("Feeding...");

  Blynk.virtualWrite(V3,
  "กำลังให้อาหาร\nครั้งที่ " + String(feedCount));

  // เปิดช่องอาหาร
  myservo.write(140);
  delay(500);

  // ปิดกลับ
  myservo.write(0);
  delay(500);

  Serial.println("Feed Done");

  Blynk.virtualWrite(V3,
  "ให้อาหารแล้ว\nรวม " + String(feedCount) + " ครั้ง");
}

// ===== ปุ่ม Manual =====
BLYNK_WRITE(V0)
{
  if(param.asInt() == 1)
  {
    feedFood();
  }
}

// ===== เวลาอาหารเช้า =====
BLYNK_WRITE(V1)
{
  TimeInputParam t(param);

  if(t.hasStartTime())
  {
    startHour1 = t.getStartHour();
    startMinute1 = t.getStartMinute();
  }
}

// ===== เวลาอาหารเย็น =====
BLYNK_WRITE(V2)
{
  TimeInputParam t(param);

  if(t.hasStartTime())
  {
    startHour2 = t.getStartHour();
    startMinute2 = t.getStartMinute();
  }
}

// ===== เช็กเวลา =====
void checkFeedingTime()
{
  int currentHour = hour();
  int currentMinute = minute();

  // รอบเช้า
  if(currentHour == startHour1 &&
     currentMinute == startMinute1 &&
     !fedMorning)
  {
    feedFood();
    fedMorning = true;
  }

  // รอบเย็น
  if(currentHour == startHour2 &&
     currentMinute == startMinute2 &&
     !fedEvening)
  {
    feedFood();
    fedEvening = true;
  }

  // รีเซ็ตทุกวันตอนเที่ยงคืน
if(currentHour == 0 && currentMinute == 0)
{
  fedMorning = false;
  fedEvening = false;

  feedCount = 0;

  Serial.println("Feed count reset");
}
}

void checkFoodLevel()
{
  int sensorValue = digitalRead(IR_SENSOR);

  Serial.print("IR = ");
  Serial.println(sensorValue);

  // HIGH = ไม่เจออาหาร
  if(sensorValue == HIGH)
  {
    Serial.println("Food LOW");

    Blynk.virtualWrite(V4, "อาหารใกล้หมดแล้ว!!");

    Blynk.logEvent("food_low", "อาหารใกล้หมดแล้ว!!");
  }
  else
  {
    Serial.println("Food OK");

    Blynk.virtualWrite(V4, "อาหารปกติ");
  }
}

void checkRFID()
{
  // ไม่มีการ์ด
  if (!rfid.PICC_IsNewCardPresent())
  {
    return;
  }

  // อ่านไม่ได้
  if (!rfid.PICC_ReadCardSerial())
  {
    return;
  }

  Serial.print("Card UID: ");

  String uid = "";

  for (byte i = 0; i < rfid.uid.size; i++)
  {
    uid += String(rfid.uid.uidByte[i], HEX);
  }

  Serial.println(uid);

  // ===== ใส่ UID ของการ์ดตรงนี้ =====
  if(uid == "7e73b36")
  {
    Serial.println("Access Granted");

    Blynk.virtualWrite(V3, "RFID ให้อาหาร");

    feedFood();

    Blynk.logEvent("feed_alert", "มีการให้อาหารผ่าน RFID");
  }
  else
  {
    Serial.println("Access Denied");

    Blynk.logEvent("wrong_card", "พบบัตรที่ไม่ได้รับอนุญาต");
  }

  rfid.PICC_HaltA();
}

void setup()
{
  Serial.begin(9600);

  SPI.begin();
  rfid.PCD_Init();

  myservo.attach(D4);

  pinMode(IR_SENSOR, INPUT);

  // ตำแหน่งเริ่มต้น
  myservo.write(0);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  rtc.begin();

  // เช็กทุก 30 วินาที
  timer.setInterval(30000L, checkFeedingTime);
  timer.setInterval(5000L, checkFoodLevel);
}

void loop()
{
if (WiFi.status() != WL_CONNECTED)
{
  WiFi.begin(ssid, pass);
}

  Blynk.run();
  timer.run();

  checkRFID();
}