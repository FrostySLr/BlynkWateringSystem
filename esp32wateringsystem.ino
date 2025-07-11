#define BLYNK_TEMPLATE_ID "TMPL6n-KrlWlu"
#define BLYNK_TEMPLATE_NAME "Automated Watering Plan"
//Include the library files
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFiClient.h>
#include "DHT.h"
#include <BlynkSimpleEsp32.h>
#define moisture 33 // pin 33
#define relay 12 // pin 12
#define temperature 32 // pin 33
#define trig 27 // pin 27
#define echo 14 // pin 14

//Initialize the LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Initialize the DHT Temperature & Humidity Sensor
DHT dht(temperature, DHT11);
// Store the saved water tank level
int blynkDistance = 0; 
int valueMoisture = 0;
int conditionSwitch = 0;

BlynkTimer timer;
// Enter your Auth token
char auth[] = "";

//Enter your WIFI SSID and password
char ssid[] = "";
char pass[] = "";
//Max level (current level - 100 = Real CM)
int MaxLevel = 19; 

void setup() {
  // Debug console
  lcd.clear();
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(relay, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  lcd.setCursor(1, 0);
  lcd.print("System Loading");
  digitalWrite(relay, LOW);
  for (int a = 0; a <= 15; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(200);
  }
  lcd.clear();
}

//Get the ultrasonic sensor values
void Moisture() {
  valueMoisture = analogRead(moisture);
  valueMoisture = ( 100 - ( (valueMoisture/4095.00) * 100 ) ); //Raw data value of the sensor is ranging between 0 - 4095, this line converts that to percentage (%)
  Blynk.virtualWrite(V0, valueMoisture);
  Serial.println(valueMoisture);
  lcd.setCursor(0, 0);
  lcd.print("Moisture:");
  lcd.print(valueMoisture);
  lcd.print(" ");
}
// Temprature   
void TempHumid(){
  int humi = dht.readHumidity();
  float temp = dht.readTemperature();
  Blynk.virtualWrite(V2, temp);
  Blynk.virtualWrite(V3, humi);
  lcd.setCursor(0, 1);
  lcd.print("HUM:");
  lcd.print(humi);
  lcd.print("%");
  lcd.print("T:");
  lcd.print(temp);
  lcd.print("C");
  delay(1000);
}

void ultrasonic() {
  digitalWrite(trig, LOW);
  delayMicroseconds(4);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long t = pulseIn(echo, HIGH);
  int distance = t / 29 / 2;
  // T = berapa lama itu bounce back ke sensor nya
  // 

  blynkDistance = (distance - MaxLevel) * -1;
  if (distance <= MaxLevel) {
    Blynk.virtualWrite(V4, blynkDistance); 
  } else {
    Blynk.virtualWrite(V4, 0);
  }
}
//
void automation(){
  // function 1 - jika tangki air masih ada -> akan mengecek moisture dari tanaman tersebut -> menyala jika kering / mati jika lembab 
  if (blynkDistance > 5) 
    {
      // Check moisture value jika moisture value diatas 54 akan mengubah value condition switch nya jadi 1 (Mati)
      if (valueMoisture >= 54){
            conditionSwitch = 1;
          }   
      if (conditionSwitch == 0) 
      {
          Serial.print("condition 1:1");
          Blynk.virtualWrite(V1, 1);
          digitalWrite(relay, HIGH);
          lcd.setCursor(13, 0);
          lcd.print(" ");
          lcd.print("ON");  
      }
// Condition switch bergantung dengan moisture value 
      else if(conditionSwitch == 1){
          Serial.print("condition 1:2");
          Blynk.virtualWrite(V1, 0);
          digitalWrite(relay, LOW);
          lcd.setCursor(13, 0);
          lcd.print("OFF");  
          if(valueMoisture <= 17){ // Jika moisture value nya dibawah 17, akan menyalakan pump.
            conditionSwitch = 0; // Condition 1 (Mati) Condition 0 (Menyala)
          }         
      }     
    }

    // function 2 - jika tangki air masih ada -> akan mengecek moisture dari tanaman tersebut -> menyala jika kering / mati jika lembab / tambahan alert jika tangki air sudah mulai habis
  if (blynkDistance == 5)
    {
      Blynk.logEvent("low_water", String("Air mulai habis!, mohon di isi secepatnya"));
    	if (conditionSwitch == 0) 
      {
        Serial.print("condition 2:1");
        Blynk.virtualWrite(V1, 1);
        digitalWrite(relay, HIGH);
        lcd.setCursor(13, 0);
        lcd.print(" ");
        lcd.print("ON");
        // jika moisture value diatas 54 akan mengubah value condition switch nya jadi 1 (Mati)
        if(valueMoisture >= 54){
            conditionSwitch = 1;
          }       
      }
      else if (conditionSwitch == 1) 
      {
        Serial.print("condition 2:2");
        Blynk.virtualWrite(V1, 0);
        digitalWrite(relay, LOW);
        lcd.setCursor(13, 0);
        lcd.print("OFF"); 
        if(valueMoisture <= 17){ // Jika moisture value nya dibawah 17, akan menyalakan pump.
            conditionSwitch = 0; // Condition 1 (Mati) Condition 0 (Menyala)
          }                 
      }	
    }
  if (blynkDistance < 1)
    {
      Serial.print("condition 3:1");
      Blynk.logEvent("empty_water", String("Air Sudah Habis!, mohon di isi"));
      int relayStateNOW = digitalRead(relay);
      if(relayStateNOW == HIGH){
        Blynk.virtualWrite(V1, 0);
        digitalWrite(relay, LOW);
        lcd.setCursor(13, 0);
        lcd.print("OFF");
      }
    }
}

// function button pada blynk
BLYNK_WRITE(V1) {
  bool Relay = param.asInt();
  if (Relay == 0) {
    // mematikan pump
    digitalWrite(relay, LOW);
    lcd.setCursor(13, 0);
    lcd.print("OFF");
  } else {
    // menyalakan pump
    digitalWrite(relay, HIGH);
    lcd.setCursor(13, 0);
    lcd.print(" ");
    lcd.print("ON");
    delay(10000); // Delay 10 detik ketika manual button di pencet.
  }
}

// main program loop
void loop() {
  Moisture();
  TempHumid();
  ultrasonic();
  automation();
  Blynk.run();//Run the Blynk library
  delay(200);

}