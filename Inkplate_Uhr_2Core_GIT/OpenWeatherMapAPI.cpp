#include <Arduino.h>
#include <WiFi.h>

#include "OpenWeatherMapAPI.h"


const char *owmApiHost = "api.openweathermap.org";

const uint64_t timeout = 5000; // [ms]


void OpenWeatherMapAPI::initWifi(const char* ssid, const char* password)
{
  Serial.println("Connecting to " + String(ssid));
  int counter = millis();
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED  && (millis() - counter) <= 10000) { }
  Serial.println("Connected with IP address " + WiFi.localIP().toString());
};

void OpenWeatherMapAPI::closeWifi()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF); 
}

String OpenWeatherMapAPI::getResponseForecast(String query, String appId)
{
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(owmApiHost, httpPort)) {
    Serial.println("Connection failed");
    return "";
  }

  String path = "/data/2.5/forecast?q=" + query + "&appid=" + appId;
  client.print(
    "GET " + path + " HTTP/1.1\r\n" +
    "Host: " + owmApiHost + "\r\n" +
    "Connection: close\r\n" +
    "Pragma: no-cache\r\n" +
    "Cache-Control: no-cache\r\n" +
    "User-Agent: ESP32\r\n" +
    "Accept: text/html,application/json\r\n\r\n");

  uint64_t startMillis = millis();
  while (client.available() == 0) {
    if (millis() - startMillis > timeout) {
      Serial.println("Client timeout");
      client.stop();
      return "";
    }
  }

  String resHeader = "", resBody = "";
  bool receivingHeader = true;
  while (client.available()) 
  {
    String line = client.readStringUntil('\r');
    if (line.length() == 1 && resBody.length() == 0) 
    {
      receivingHeader = false;
      continue;
    }
    if (receivingHeader) 
    {
      resHeader += line;
    }
    else 
    {
      resBody += line;
    }
  }

  client.stop();

  //Serial.print("Data received successfully (request body size: " + String(resBody.length()) + " bytes)");
  //Serial.println(resBody);
  return resBody;
};

String OpenWeatherMapAPI::getResponseWeather(String query, String appId)
{
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(owmApiHost, httpPort)) {
    Serial.println("Connection failed");
    return "";
  }

  String path = "/data/2.5/weather?q=" + query + "&appid=" + appId;
  client.print(
    "GET " + path + " HTTP/1.1\r\n" +
    "Host: " + owmApiHost + "\r\n" +
    "Connection: close\r\n" +
    "Pragma: no-cache\r\n" +
    "Cache-Control: no-cache\r\n" +
    "User-Agent: ESP32\r\n" +
    "Accept: text/html,application/json\r\n\r\n");

  uint64_t startMillis = millis();
  while (client.available() == 0) {
    if (millis() - startMillis > timeout) {
      Serial.println("Client timeout");
      client.stop();
      return "";
    }
  }

  String resHeader = "", resBody = "";
  bool receivingHeader = true;
  
  while (client.available()) 
  {
    String line = client.readStringUntil('\r');
    if (line.length() == 1 && resBody.length() == 0) 
    {
      receivingHeader = false;
      continue;
    }
    if (receivingHeader) 
    {
      resHeader += line;
    }
    else 
    {
      resBody += line;
    }
  }

  client.stop();

  //Serial.print("Data received successfully (request body size: " + String(resBody.length()) + " bytes)");
  //Serial.println(resBody);
    
  return resBody;
};
