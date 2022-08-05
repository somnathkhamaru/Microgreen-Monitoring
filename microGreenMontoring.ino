#include <Adafruit_GFX.h>      // include Adafruit graphics library
#include <Adafruit_ST7735.h>   // include Adafruit ST7735 TFT library
#include <Adafruit_ADS1015.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DHT.h"
// Replace with your network credentials
const char *ssid     = "soms";
const char *password = "sunny007";

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
String formattedTime = "";
//Month names
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
Adafruit_ADS1015 ads;

// ST7735 TFT module connections
#define TFT_RST   D4     // TFT RST pin is connected to NodeMCU pin D4 (GPIO2)
#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D3 (GPIO0)
#define TFT_DC    D8    // TFT DC  pin is connected to NodeMCU pin D4 (GPIO4)
const int analogInPin = A0;
//  DHT sensor type you're using!
#define DHTTYPE DHT11
// DHT Sensor
const int DHTPin = 12;
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);
const int RedLedPin = 16;
const int GreenLedPin = 3;
const int BuzzerPin = 1;
// Temporary variables
static char celsiusTemp[7];
static char fahrenheitTemp[7];
static char humidityTemp[7];

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
float  sensorValue = 0;
float oldSensorValue = 0;
float  sensorValueMois = 0;
float oldSensorValueMois = 0;
float  sensorValueHum = 0;
float oldSensorValueHum = 0;
float  sensorValueTemp = 0;
float oldSensorValueTemp = 0;
float moistureThreshold = 10;
float maxMoistureThreshold = 20;
float minLightThreshold = 5;
int textSize = 1;
int setTimerM = 0;
int setTimerL = 0;
unsigned long startTimeM;
unsigned long startTimeL;
unsigned long thresholdMillis = 60000;
String LMA = "Lo-Soil Moisture Alert !!";
String NMA = "No Lo-Soil Moisture Alert";
String LLA = "Lo-Ambient Light Alert !!";
String NLA = "No Lo-Ambient Light Alert";

void setup(void) {
  Serial.begin(9600);
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  digitalWrite(RedLedPin, LOW);
  digitalWrite(GreenLedPin, LOW);
  digitalWrite(BuzzerPin, LOW);
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  ads.setGain(GAIN_ONE);
  ads.begin();
  dht.begin();
  tft.setSPISpeed(40000000);
  tft.setRotation(1);
  tft.fillScreen(ST7735_WHITE);
  delay(100);
  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Initialize a NTPClient to get time
  timeClient.begin();
  timeClient.setTimeOffset(19800);
  displayMessage(5, 115, LMA, NMA);
  displayMessage(5, 105, LLA, NLA);
}
void getDateTime()
{
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  String oldTime = formattedTime;
  formattedTime = timeClient.getFormattedTime();
  //  Serial.print("Formatted Time: ");
  //  Serial.println(formattedTime);
  //Get a time structure
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;
  String currentMonthName = months[currentMonth - 1];
  int currentYear = ptm->tm_year + 1900;
  //Print complete date:
  String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
  //  Serial.print("Current date: ");
  //  Serial.println(currentDate);
  //  Serial.println("");
  displayData(5, 70 , 90, 120 , currentDate, currentDate, "Date = ", "");
  displayData(5, 85 , 90, 120 , oldTime, formattedTime, "Time = ", "");
}
void getTempHumidity()
{
  oldSensorValueHum = sensorValueHum;
  oldSensorValueTemp = sensorValueTemp;
  float h = dht.readHumidity();

  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    displayData(5, 40 , 90, 120 , String(oldSensorValueHum, 2), String(sensorValueHum, 2), "Hum Error->", "Check!!");
    displayData(5, 55 , 90, 120 , String(oldSensorValueTemp, 2), String(sensorValueTemp, 2), "Temp Error->", "Check!");
    Serial.println("Failed to read from DHT sensor!");
    strcpy(celsiusTemp, "Failed");
    strcpy(fahrenheitTemp, "Failed");
    strcpy(humidityTemp, "Failed");
  }
  else {

    dtostrf(h, 6, 2, humidityTemp);
    sensorValueHum = h;
    sensorValueTemp = t;
    // You can delete the following Serial.print's, it's just for debugging purposes
    displayData(5, 40 , 90, 120 , String(oldSensorValueHum, 2), String(sensorValueHum, 2), "Humidity = ", " %");
    displayData(5, 55 , 90, 120 , String(oldSensorValueTemp, 2), String(sensorValueTemp, 2), "Temperature = ", " *C");
    //      Serial.print("Humidity: ");
    //      Serial.print(h);
    //      Serial.print(" %\t Temperature: ");
    //      Serial.print(t);
    //      Serial.print(" *C ");
    delay(1000);
  }
}
void displayData(int c1 , int c2 , int c3, int c4 , String oldVal, String newVal, String type, String unit)
{
  tft.setCursor(c1, c2);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(textSize);
  tft.print(type);
  tft.setCursor(c3, c2);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(textSize);
  tft.println(oldVal);
  tft.setCursor(c4, c2);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(textSize);
  tft.println(unit);
  tft.setCursor(c1, c2);
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(textSize);
  tft.print(type);
  tft.setCursor(c3, c2);
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(textSize);
  tft.println(newVal);
  tft.setCursor(c4, c2);
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(textSize);
  tft.println(unit);
}
void displayMessage(int c1 , int c2 , String oldVal, String newVal)
{
  tft.setCursor(c1, c2);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(textSize);
  tft.println(oldVal);
  tft.setCursor(c1, c2);
  tft.setTextColor(ST7735_BLUE);
  tft.setTextSize(textSize);
  tft.println(newVal);
}
void getLDRData()
{

  sensorValue = ads.readADC_SingleEnded(0);
  // print the readings in the Serial Monitor
  float lightPercent = (sensorValue / 1369) * 100;
  displayData(5, 10 , 90, 125, String(oldSensorValue, 2), String(lightPercent, 2), "Light = ", " %");
  oldSensorValue = lightPercent;
  if (lightPercent < minLightThreshold)
  {

    digitalWrite(RedLedPin, HIGH);
    if (setTimerL == 0)
    {
      startTimeL = millis();
      displayMessage(5, 105, NLA, LLA);
      setTimerL = 1;
    }
    else
    {
      if ((millis() - startTimeL) >= thresholdMillis)
      {
        digitalWrite(BuzzerPin, HIGH);
      }

    }

  }
  else
  {
    if (setTimerM == 0 && setTimerL == 0)
    {
      digitalWrite(RedLedPin, LOW);
    }
    setTimerL = 0;
  }
  delay(1000);
  // print the readings in the Serial Monitor
  //  Serial.print("sensor = ");
  //  Serial.println(sensorValue);
}
void getMoistureData()
{
  sensorValueMois = ads.readADC_SingleEnded(1);
  float moisturePercent = ((1385 - sensorValueMois) / 1385) * 100;
  // print the readings in the Serial Monitor
  displayData(5, 25 , 90, 125 , String(oldSensorValueMois, 2), String(moisturePercent, 2), "Moisture = ", " %");
  oldSensorValueMois = moisturePercent;
  if (moisturePercent < moistureThreshold)
  {
    digitalWrite(RedLedPin, HIGH);
    if (setTimerM == 0)
    {
      startTimeM = millis();
      displayMessage(5, 115, NMA, LMA);
      setTimerM = 1;
    }
    else
    {
      if ((millis() - startTimeM) >= thresholdMillis)
      {
        digitalWrite(BuzzerPin, HIGH);
      }
    }
  }
  else
  {
    if (setTimerM == 0 && setTimerL == 0)
    {
      digitalWrite(RedLedPin, LOW);
    }
    setTimerM = 0;
  }
  if (moisturePercent > maxMoistureThreshold)
  {
    digitalWrite(GreenLedPin, HIGH);
  }
  else
  {
    digitalWrite(GreenLedPin, LOW);
  }
  delay(1000);
  // print the readings in the Serial Monitor
  //Serial.print("Moisture sensor = ");
  //Serial.println(sensorValueMois);
}
void loop() {
  getDateTime();
  getLDRData();
  getMoistureData();
  getTempHumidity();
  if (setTimerM == 0 && setTimerL == 0)
  {
    displayMessage(5, 115, LMA, NMA);
    displayMessage(5, 105, LLA, NLA);
    digitalWrite(BuzzerPin, LOW);
  }
}
