#ifndef OpenWeatherMapAPI_h
#define OpenWeatherMapAPI_h

class OpenWeatherMapAPI
{
public:
	static void initWifi(const char* ssid, const char* password);
  static void closeWifi();
	static String getResponseForecast(String query, String appId);
  static String getResponseWeather(String query, String appId);
};

#endif
