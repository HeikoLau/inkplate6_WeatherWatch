// Next 3 lines are a precaution, you can ignore those, and the example would also work without them
#ifndef ARDUINO_ESP32_DEV
#error "Wrong board selection for this example, please select Inkplate 6 in the boards menu."
#endif

#include "Inkplate.h"            //Include Inkplate library to the sketch
Inkplate display(INKPLATE_1BIT); // Create an object on Inkplate library and also set library into 1-bit mode (BW)

#include <WiFi.h>
#include "time.h"
#include "Roboto_Medium_48_mod.h";
#include "Roboto_Light_36_mod.h";
#include "Roboto_Condensed_24.h";
#include "DSEG7_NT_Bold_128.h";
#include "OpenWeatherMapAPI.h"
#include "ArduinoJson.h";
#include "icons.h"

char* ssid     = "your superior wlan";
char* password = "1234567890";

const char query[] = "Hinterposemuckel,de"; // location,country
const char apiKey[] = ""; // get your own apikey @ api.openweathermap.org


const char* ntpServer = "192.168.188.1";
//const char* ntpServer = "pool.ntp.org";
//const long  gmtOffset_sec = 3600;
//const int   daylightOffset_sec = 3600;
const uint16_t koordX[] = {392, 421, 450, 479, 506, 532, 557, 579, 600, 619, 634, 648, 658, 666, 670, 672, 670, 666, 658, 648, 634, 619, 600, 579, 557, 532, 506, 479, 450, 421, 392, 363, 334, 305, 278, 252, 227, 205, 184, 165, 150, 136, 126, 118, 114, 112, 114, 118, 126, 136, 150, 165, 184, 205, 227, 252, 278, 305, 334, 363};
const uint16_t koordY[] = {12, 14, 18, 26, 36, 50, 65, 84, 105, 127, 152, 178, 205, 234, 263, 292, 321, 350, 379, 406, 432, 457, 479, 500, 519, 534, 548, 558, 566, 570, 572, 570, 566, 558, 548, 534, 519, 500, 479, 457, 432, 406, 379, 350, 321, 292, 263, 234, 205, 178, 152, 127, 105, 84, 65, 50, 36, 26, 18, 14};

char timeString[8];
char timesecString[3];
char dayString[11];
char monthString[10];
char dateDayString[5];
char yearString[6];

int offsetDay = 0; //für Platzierung des Tages
int offsetDate = 0; //für Platzierung des Datums
int sec = 0;
int diffsec = 0;
int minute = 0;
int hour = 0;
int counter = 0;

bool nextSec = 0;
//bool nextMinute = 1;
bool nextHour = 1;
bool wifi = 1;
bool forceGetData = 0;
bool responseData = 0;

//wettervariablen
char weather_0_main[16];
char weather_0_description[16];
char stadt[16];
float main_temp;
int main_pressure;
int weather_0_id;

long list_item_dt_0[4];
int list_item_main_temp[4];
int list_item_main_pressure[4];
int list_item_weather_0_id[4];
char list_item_weather_0_main[4][16];
char list_item_weather_0_description[4][16];

TaskHandle_t Task1;
TaskHandle_t Task2;

void setup() {
  Serial.begin(115200);
  Serial.println(xPortGetCoreID());

  connect2NTP(0, 0); //gmtOffset, daylightOffset

  display.begin();        // Init Inkplate library (you should call this function ONLY ONCE)
  display.clearDisplay(); // Clear frame buffer of display
  display.display();      // Put clear image on display
  display.setTextColor(BLACK, WHITE); // Set text color to be black and background color to be white

  OpenWeatherMapAPI::initWifi(ssid, password);

  String weather = OpenWeatherMapAPI::getResponseWeather(query, apiKey);
  String forecast = OpenWeatherMapAPI::getResponseForecast(query, apiKey);

  OpenWeatherMapAPI::closeWifi();

  const char* bufWeather = weather.c_str();
  const char* bufForecast = forecast.c_str();

  parseWeather(bufWeather);
  parseForecast(bufForecast);

  Serial.println(stadt);
  drawInitSecCircle();

  drawForecast();

  xTaskCreatePinnedToCore(
    taskCode0,   /* Task function. */
    "Task1",     /* name of task. */
    12000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task1,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */

  delay(500);

  xTaskCreatePinnedToCore(
    taskCode1,   /* Task function. */
    "Task2",     /* name of task. */
    12000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    &Task2,      /* Task handle to keep track of created task */
    1);          /* pin task to core 1 */

  delay(500);
}

//Funktion für Core 0, für Uhr
void taskCode0( void * pvParameters ) {
  Serial.print("Task0 running on core ");
  Serial.println(xPortGetCoreID());

  while (true)
  {
    getCurrentTime();

    if (nextSec)
    {
      drawWatch();

      if (hour == 5 && minute == 1)
      {
        if (nextHour)
        {
          display.clearDisplay(); // Clear frame buffer of display
          display.display();      // Put clear image on display
          Serial.println("Wenns 5Uhr ist");

          drawForecast();

          nextHour = 0;
        }
      }
      else
      {
        nextHour = 1;
        delay(150);
      }
    }

    if (responseData)
    {
      drawForecast();
      responseData = 0;
    }

    checkTouchpads();

    delay(50);
  }
}

//Funktion für Core 1, für WLAN-Kran
void taskCode1( void * pvParameters ) {
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());

  int updateMin = minute / 5;
  updateMin = updateMin * 5 + 5;

  if (updateMin == 60)
  {
    updateMin = 0;
  }
  Serial.print("updateZeit");
  Serial.println(updateMin);

  while (true)
  {

    //if (nextSec)
    {
      if (minute == updateMin)
      {
        OpenWeatherMapAPI::initWifi(ssid, password);
        String weather = OpenWeatherMapAPI::getResponseWeather(query, apiKey);
        OpenWeatherMapAPI::closeWifi();
        const char* bufWeather = weather.c_str();

        parseWeather(bufWeather);

        if (updateMin > 50)
        {
          updateMin = 0;
        }
        else
        {
          updateMin = updateMin + 5;
          Serial.print("updateZeit");
          Serial.println(updateMin);
        }
      }

      if (hour == 5)
      {
        if (nextHour)
        {
          OpenWeatherMapAPI::initWifi(ssid, password);
          String forecast = OpenWeatherMapAPI::getResponseForecast(query, apiKey);
          OpenWeatherMapAPI::closeWifi();
          const char* bufForecast = forecast.c_str();
          parseForecast(bufForecast);

          connect2NTP(0, 0);

          nextHour = 0;
        }
      }
      else
      {
        nextHour = 1;
        delay(150);
      }

      if (forceGetData)
      {
        Serial.println("Update");
        connect2NTP(0, 0); //gmtOffset, daylightOffset
        OpenWeatherMapAPI::initWifi(ssid, password);

        String weather = OpenWeatherMapAPI::getResponseWeather(query, apiKey);
        String forecast = OpenWeatherMapAPI::getResponseForecast(query, apiKey);

        OpenWeatherMapAPI::closeWifi();

        const char* bufWeather = weather.c_str();
        const char* bufForecast = forecast.c_str();

        parseWeather(bufWeather);
        parseForecast(bufForecast);

        forceGetData = 0;
        responseData = 1;
      }
    }

    delay(2000);
  }
}

void loop() {

}

void checkTouchpads()
{
  if (display.readTouchpad(PAD1)) //Reset ESP
  {
    display.setCursor(294, 330);
    display.print("R");
    display.partialUpdate(false, true);

    delay(3000);
    ESP.restart();
  }

  if (display.readTouchpad(PAD2)) //Update alles
  {
    display.setCursor(294, 330);
    display.print("U");
    display.partialUpdate(false, true);

    forceGetData = 1;
  }

  if (display.readTouchpad(PAD3)) // Clear Diplay
  {
    display.setCursor(294, 330);
    display.print("C");
    display.partialUpdate(false, true);

    delay(3000);
    display.clearDisplay(); // Clear frame buffer of display
    display.display();      // Put clear image on display
    Serial.println("Wenn getouched");

    drawInitSecCircle();
    drawForecast();

  }
}

void drawInitSecCircle()
{
  for (int i = 0; i < 60; i++)
  {
    display.drawCircle(koordX[i] - 80, koordY[i] + 8, 8, BLACK); //koordX + 8 - 90
  }

  display.partialUpdate(false, true);
}

void drawSecCircle()
{
  int min_2 = minute / 2;
  for (int i = 0; i < diffsec + 1; i++)
  {
    if (min_2 * 2 == minute)
    {
      display.fillCircle(koordX[sec - i] - 80, koordY[sec - i] + 8, 8, BLACK); //koordX + 8 - 90
    }
    else
    {
      display.fillCircle(koordX[sec - i] - 80, koordY[sec - i] + 8, 7, WHITE);
    }
  }
}

void drawWatch()
{
  display.fillCircle(312, 300, 270, WHITE);
  display.setFont(&DSEG7_NT_Bold_128);
  display.setCursor(84, 315);                                  // Set position of the text
  display.print(";;:;;");
  display.setCursor(84, 315);                                  // Set position of the text
  display.print(timeString);

  display.setFont(&Roboto_Medium_48_mod);
  drawSecCircle();
  display.setCursor(404 - offsetDay, 120);                     // Set position of the text
  display.print(dayString);
  display.setCursor(404 - offsetDate, 170);                    // Set position of the text
  display.print(dateDayString);
  display.print(monthString);

  drawIcon(weather_0_id, 265, 450, 100, 100);

  display.setFont(&Roboto_Light_36_mod);
  display.setCursor(279, 75);                                  // Set position of the text
  display.print(yearString);
  display.setCursor(254, 360);                                  // Set position of the text
  display.print(stadt);
  display.setCursor(144, 400);                                  // Set position of the text
  display.print("Temperatur: ");
  display.print(main_temp);
  display.print(" *C");
  display.setCursor(144, 440);                                  // Set position of the text
  display.print("Luftdruck: ");
  display.print(main_pressure);
  display.print(" hPa");

  display.partialUpdate(false, true);

  Serial.println(main_temp);
}

void drawForecast()
{
  display.fillRect(670, 2, 128, 596, WHITE);

  drawIcon(list_item_weather_0_id[0], 680, 1, 100, 100);
  drawIcon(list_item_weather_0_id[1], 680, 151, 100, 100);
  drawIcon(list_item_weather_0_id[2], 680, 301, 100, 100);
  drawIcon(list_item_weather_0_id[3], 680, 451, 100, 100);

  display.setFont(&Roboto_Condensed_24);
  display.setCursor(690, 123);                                  // Set position of the text
  display.print(list_item_main_temp[0]);
  display.print(" *C");
  display.setCursor(690, 273);                                  // Set position of the text
  display.print(list_item_main_temp[1]);
  display.print(" *C");
  display.setCursor(690, 423);                                  // Set position of the text
  display.print(list_item_main_temp[2]);
  display.print(" *C");
  display.setCursor(690, 573);                                  // Set position of the text
  display.print(list_item_main_temp[3]);
  display.print(" *C");

  display.setCursor(690, 145);                                  // Set position of the text
  display.print(list_item_main_pressure[0]);
  display.print(" hPa");
  display.setCursor(690, 295);                                  // Set position of the text
  display.print(list_item_main_pressure[1]);
  display.print(" hPa");
  display.setCursor(690, 445);                                  // Set position of the text
  display.print(list_item_main_pressure[2]);
  display.print(" hPa");
  display.setCursor(690, 595);                                  // Set position of the text
  display.print(list_item_main_pressure[3]);
  display.print(" hPa");

  display.partialUpdate(false, true);
}

void getCurrentTime() {
  struct tm timeinfo;
  wifi = 1;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    wifi = 0;
    //return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  if (sec |= timeinfo.tm_sec)
  {
    nextSec = 1;
    if (sec > timeinfo.tm_sec)
    {
      diffsec = timeinfo.tm_sec;
    }
    else
    {
      diffsec = timeinfo.tm_sec - sec;
    }
    sec = timeinfo.tm_sec;
    minute = timeinfo.tm_min;
    hour = timeinfo.tm_hour;
    strftime(timeString, 8, "%H:%M", &timeinfo);
    strftime(timesecString, 3, "%S", &timeinfo);
    strftime(dayString, 11, "%A", &timeinfo);
    changeDay();
    strftime(monthString, 10, "%B", &timeinfo);
    changeMonth();
    strftime(dateDayString, 5, "%d. ", &timeinfo);
    if (wifi)
    {
      strftime(yearString, 6, "%Y", &timeinfo);
    }
    else
    {
      strcpy(yearString, "XXXX");
    }

    //counter++;
  }
  else
  {
    nextSec = 0;
  }
}

void changeDay()
{
  if (dayString[0] == 'M')
  {
    offsetDay = 179;
    strcpy(dayString, "Montag");
  }
  else if (dayString[0] == 'T')
  {
    if (dayString[1] == 'u')
    {
      offsetDay = 194;
      strcpy(dayString, "Dienstag");
    }
    else
    {
      offsetDay = 227;
      strcpy(dayString, "Donnerstag");
    }
  }
  else if (dayString[0] == 'W')
  {
    offsetDay = 200;
    strcpy(dayString, "Mittwoch");
  }
  else if (dayString[0] == 'F')
  {
    offsetDay = 174;
    strcpy(dayString, "Freitag");
  }
  else if (dayString[0] == 'S')
  {
    if (dayString[1] == 'a')
    {
      offsetDay = 193;
      strcpy(dayString, "Samstag");
    }
    else
    {
      offsetDay = 187;
      strcpy(dayString, "Sonntag");
    }
  }
}

void changeMonth()
{
  if (monthString[0] == 'F')
  {
    offsetDate = 230;
    strcpy(monthString, "Februar");
  }
  else if (monthString[0] == 'J')
  {
    if (monthString[1] == 'a')
    {
      offsetDate = 202;
      strcpy(monthString, "Januar");
    }
    else if (monthString[2] == 'n')
    {
      offsetDate = 188;
      strcpy(monthString, "Juni");
    }
    else
    {
      offsetDate = 179;
      strcpy(monthString, "Juli");
    }
  }
  else if (monthString[0] == 'S')
  {
    offsetDate = 268;
    strcpy(monthString, "September");
  }
  else if (monthString[0] == 'O')
  {
    offsetDate = 232;
    strcpy(monthString, "Oktober");
  }
  else if (monthString[0] == 'M')
  {
    if (monthString[2] == 'r')
    {
      offsetDate = 196;
      strcpy(monthString, "M^rz");
    }
    else
    {
      offsetDate = 179;
      strcpy(monthString, "Mai");
    }
  }
  else if (monthString[0] == 'A')
  {
    if (monthString[1] == 'p')
    {
      offsetDate = 194;
      strcpy(monthString, "April");
    }
    else
    {
      offsetDate = 202;
      strcpy(monthString, "August");
    }
  }
  else if (monthString[0] == 'N')
  {
    offsetDate = 260;
    strcpy(monthString, "November");
  }
  else if (monthString[0] == 'D')
  {
    offsetDate = 258;
    strcpy(monthString, "Dezember");
  }
}

void connect2NTP(long gmtOffset_sec, int daylightOffset_sec)
{
  // Connect to Wi-Fi
  counter = millis();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && (millis() - counter) <= 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected.");
    wifi = 1;
  }
  else
  {
    Serial.println("");
    Serial.println("WiFi not connected.");
    wifi = 0;
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1); //Timezone für Berlin
  tzset();

  getCurrentTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void parseWeather(const char* charWeather)
{
  StaticJsonDocument<144> filter;

  JsonObject filter_weather_0 = filter["weather"].createNestedObject();
  filter_weather_0["id"] = true;
  filter_weather_0["main"] = true;
  filter_weather_0["description"] = true;

  JsonObject filter_main = filter.createNestedObject("main");
  filter_main["temp"] = true;
  filter_main["pressure"] = true;
  filter["name"] = true;

  StaticJsonDocument<256> doc;

  DeserializationError error = deserializeJson(doc, charWeather, 700, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject weather_0 = doc["weather"][0];
  weather_0_id = weather_0["id"]; // 800
  strcpy(weather_0_main, doc["weather"][0]["main"].as<const char *>()); //"Clouds"
  strcpy(weather_0_description, doc["weather"][0]["description"].as<const char *>()); //"few clouds"

  main_temp = doc["main"]["temp"]; // 268.04
  main_temp = (main_temp - 273.15) * 10;
  int tmp = main_temp;
  main_temp = tmp / 10.0;
  main_pressure = doc["main"]["pressure"]; // 1014

  strcpy(stadt, "H_rlitz");
}

char* check4Umlauts(char* ort)
{
  Serial.println("und nun");
  Serial.println(ort);
  for (int i = 0; i < 16; i++)
  {
    Serial.println(ort[i]);
    if (ort[i] == 'ö')
    {
      ort[i] = '_';
    }
    else if (ort[i] == 'ä')
    {
      ort[i] = '^';
    }
    else if (ort[i] == 'ü')
    {
      ort[i] = '`';
    }
  }
  return ort;
}


void parseForecast(const char* charForecast)
{
  StaticJsonDocument<208> filter;

  JsonObject filter_list_0 = filter["list"].createNestedObject();
  filter_list_0["dt"] = true;

  JsonObject filter_list_0_main = filter_list_0.createNestedObject("main");
  filter_list_0_main["temp"] = true;
  filter_list_0_main["pressure"] = true;

  JsonObject filter_list_0_weather_0 = filter_list_0["weather"].createNestedObject();
  filter_list_0_weather_0["id"] = true;
  filter_list_0_weather_0["main"] = true;
  filter_list_0_weather_0["description"] = true;

  JsonObject filter_main = filter.createNestedObject("main");
  filter_main["temp"] = true;
  filter_main["pressure"] = true;

  DynamicJsonDocument doc(10000);

  DeserializationError error = deserializeJson(doc, charForecast, 20000, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  list_item_weather_0_id[0] = doc["list"].as<JsonArray>()[10]["dt"]; //dritter Zeitstempel
  list_item_weather_0_id[1] = doc["list"].as<JsonArray>()[18]["dt"]; //dritter Zeitstempel
  list_item_weather_0_id[2] = doc["list"].as<JsonArray>()[26]["dt"]; //dritter Zeitstempel
  list_item_weather_0_id[3] = doc["list"].as<JsonArray>()[34]["dt"]; //dritter Zeitstempel

  float temptmp = doc["list"].as<JsonArray>()[10]["main"]["temp"];
  list_item_main_temp[0] = (temptmp + 0.5) - 273.15;
  temptmp = doc["list"].as<JsonArray>()[18]["main"]["temp"];
  list_item_main_temp[1] = (temptmp + 0.5) - 273.15;
  temptmp = doc["list"].as<JsonArray>()[26]["main"]["temp"];
  list_item_main_temp[2] = (temptmp + 0.5) - 273.15;
  temptmp = doc["list"].as<JsonArray>()[34]["main"]["temp"];
  list_item_main_temp[3] = (temptmp + 0.5) - 273.15;

  list_item_main_pressure[0] = doc["list"].as<JsonArray>()[10]["main"]["pressure"];
  list_item_main_pressure[1] = doc["list"].as<JsonArray>()[18]["main"]["pressure"];
  list_item_main_pressure[2] = doc["list"].as<JsonArray>()[26]["main"]["pressure"];
  list_item_main_pressure[3] = doc["list"].as<JsonArray>()[34]["main"]["pressure"];

  list_item_weather_0_id[0] = doc["list"].as<JsonArray>()[10]["weather"][0]["id"];
  list_item_weather_0_id[1] = doc["list"].as<JsonArray>()[18]["weather"][0]["id"];
  list_item_weather_0_id[2] = doc["list"].as<JsonArray>()[26]["weather"][0]["id"];
  list_item_weather_0_id[3] = doc["list"].as<JsonArray>()[34]["weather"][0]["id"];
}

void drawIcon(int id, int x, int y, int w, int h)
{
  Serial.println(id);
  if (id / 100 == 2)
  {
    display.drawImage(Gewitter_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id / 100 == 3)
  {
    display.drawImage(Niesel_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id / 100 == 5)
  {
    display.drawImage(Regen_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id / 100 == 6)
  {
    display.drawImage(Schnee_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id == 800)
  {
    display.drawImage(Sonne_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id == 801)
  {
    display.drawImage(SonneWolke_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id == 802)
  {
    display.drawImage(WolkeMittel_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else if (id == 803 || id == 804)
  {
    display.drawImage(WolkeDunkel_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
  else
  {
    display.drawImage(Nebel_1bit, x, y, w, h,
                      BLACK); // Arguments are: array variable name, start X, start Y, size X, size Y, color
  }
}
