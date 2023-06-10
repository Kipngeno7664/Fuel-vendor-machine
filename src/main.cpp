#include <Arduino.h>

#include <I2CKeypad.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>

WiFiClient client;
HTTPClient httpClient;
float randomNumber, randomNumber2, randomNumber3;

// #include <Keypad_I2C.h> // I2C Keypad library by Joe Young https://github.com/joeyoung/arduino_keypads
#include <LiquidCrystal_I2C.h> // I2C LCD Library by Francisco Malpartida https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/Home

#define lcd_addr 0x27    // I2C address of typical I2C LCD Backpack
#define keypad_addr 0x20 // I2C a (A0-A1-A2 dip sddress of I2C Expander modulewitch to off position)
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char *WIFI_SSID = "Kevin";
const char *WIFI_PASSWORD = "kevin2021";
const char *URL = "http://192.168.43.18/payment/dbwrite.php";
String sendval, sendval2, postData;
const int FLOW_SENSOR_PIN = 14; // The pin for the flow rate sensor
const int RELAY_PIN = 2;

float enteredAmount;
float fuelDispensed = 0;
float fuelToDispense = 0;
unsigned long pulseCount = 0;
bool flag = false;
String phoneNumber;
float volume;
// const float FLOW_RATE_CALIBRATION_FACTOR = 450;
float totalFuel = 500; // max fuel capacity of the storage tank in liters

const float unitPrice = 100; // price per liter

// Define the keypad pins
const byte ROWS = 4;
const byte COLS = 3;
// char keys[ROWS][COLS] = {
//     {'1', '2', '3'},
//     {'4', '5', '6'},
//     {'7', '8', '9'},
//     {'*', '0', '#'}};

// Keypad pins connected to the I2C -Expander pins P0-P6
byte rowPins[ROWS] = {0, 1, 2, 3}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6};    // connect to the column pinouts of the keypad

// Create instance of the Keypad name I2C_Keypad and using the PCF8574 chip
I2CKeyPad keyPad(keypad_addr);

// I2CKeypad I2C_Keypad(keypad_addr);
IRAM_ATTR void increase()
{
  pulseCount++;
}
char keys[14] = "123456789*0#";
void setup()
{
  Serial.begin(115200); // i2c_lcd.begin (16,2); //  our LCD is a 16x2, change for your LCD if needed
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected");
  lcd.begin(16, 2);
  Wire.begin();
  Wire.setClock(400000);
  I2C_Keypad.begin();
  I2C_Keypad.loadKeymap(keys);

  lcd.backlight();
  lcd.init();
  pinMode(RELAY_PIN, OUTPUT);

  pinMode(FLOW_SENSOR_PIN, INPUT);
  lcd.setCursor(0, 0);
  lcd.print("ENTER AMOUNT: ");
  // LCD Backlight ON
  // i2c_lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  // i2c_lcd.setBacklight(HIGH);

  // lcd.clear(); // Clear the LCD screen

  // lcd.backlight();

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), increase, RISING);
}
void loop()
{

  // Put value of pressed key on keypad in key variable
  // char key = I2C_Keypad.getKey();

  if (I2C_Keypad.isPressed())
  {
    int key = I2C_Keypad.getChar();
    if (key >= '0' && key <= '9' && flag == false)
    {

      enteredAmount = enteredAmount * 10 + (key - '0');
      lcd.setCursor(0, 0);
      lcd.print("Amount: ");
      lcd.print(enteredAmount);
    }
    else if (key >= '0' && key <= '9' && flag == true)
    {

      phoneNumber = phoneNumber + key;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("PHONE: ");
      lcd.setCursor(0, 1);
      lcd.print(phoneNumber);
    }
    else if (key == '*')
    {

      lcd.clear();
      enteredAmount = 0;
      lcd.print("ENTER AMOUNT: ");
    }
    else if (key == '#' && flag == false)
    { // User has entered the amount, turn on the pump
      flag = true;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter Phone");
      lcd.setCursor(0, 1);
    }
    else if (key == '#' && flag == true)
    {
      fuelToDispense = enteredAmount / unitPrice;
      lcd.clear();
      sendval = String(phoneNumber);
      sendval2 = String(enteredAmount);
      lcd.setCursor(0, 0);
      lcd.print("Dispense: ");
      lcd.setCursor(0, 1);
      lcd.print(fuelToDispense);
      if (fuelToDispense > 1)
      {
        lcd.setCursor(10, 1);
        lcd.print("Litres");
      }
      else
      {
        lcd.setCursor(11, 1);
        lcd.print("Litre");
      }
      // delay(2000);
      String data = "sendval=" + sendval + "&sendval2=" + sendval2;
      // String data = "sendval=333&sendval2=20";

      httpClient.begin(client, URL);
      httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpClient.POST(data);
      String content = httpClient.getString();
      httpClient.end();

      // Serial.println(content);
      delay(20000);
      digitalWrite(RELAY_PIN, HIGH);
    }
  }

  if (digitalRead(RELAY_PIN) == HIGH)
  { // Pump is on, measure the flow rate

    pulseCount = 0;
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), increase, RISING);
    delay(5000);
    detachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN));
    volume = 2.663 * pulseCount;
    fuelDispensed += volume / 1000 * 30; // Convert mL to L
    Serial.println(volume);
    lcd.setCursor(0, 1);
    lcd.print("Dispensed: ");
    lcd.print(fuelDispensed);
    if (fuelDispensed >= fuelToDispense)
    { // Pump has dispensed the requested amount of fuel, turn it off
      digitalWrite(RELAY_PIN, LOW);
      lcd.clear();
      fuelDispensed = 0;
      fuelToDispense = 0;
      enteredAmount = 0;
      flag = false;
      lcd.print("ENTER AMOUNT: ");
    }
  }
}
