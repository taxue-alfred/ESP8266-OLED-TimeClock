#include <ArduinoJson.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include "MyRealTimeClock.h"
#include <DHT.h>
#include <U8g2lib.h>
#include <ESP8266HTTPClient.h>

//重写NTPclient获取时间格式函数，从00:00:00改为00:00
class NTPClient_son : public NTPClient
{
private:
  UDP *_udp;
  bool _udpSetup = false;
  const char *_poolServerName = "pool.ntp.org"; // Default time server
  int _port = NTP_DEFAULT_LOCAL_PORT;
  long _timeOffset = 0;
  unsigned long _updateInterval = 60000; // In ms
  unsigned long _currentEpoc = 0;        // In s
  unsigned long _lastUpdate = 0;         // In ms
  byte _packetBuffer[NTP_PACKET_SIZE];

public:
  //显式基类构造函数调用
  NTPClient_son(UDP &udp, const char *poolServerName, long timeOffset, unsigned long updateInterval) : NTPClient(udp, poolServerName, timeOffset, updateInterval){};
  String getFormattedTime() const
  {
    unsigned long rawTime = this->getEpochTime();
    unsigned long hours = (rawTime % 86400L) / 3600;
    String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

    unsigned long minutes = (rawTime % 3600) / 60;
    String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

    unsigned long seconds = rawTime % 60;
    String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

    return hoursStr + ":" + minuteStr;
  }
};

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/14, /* data=*/12, /* reset=*/U8X8_PIN_NONE);
DHT dht(5, DHT11);
MyRealTimeClock myRTC(4, 13, 15);
const char *ssid = "SSID"; // WiFi名称
const char *password = "PASS"; // WiFi密码
WiFiUDP ntpUDP;
WiFiClient client;
HTTPClient http;
//获取NTP时间:且设置时域为＋8:00
NTPClient_son timeClient(ntpUDP, "ntp1.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);

int humidity;
int temperature;
int return_code = 69;
unsigned long previousMillis = 0;
const long interval = 70;
unsigned long previousMillis_weather = 0;
const long interval_weather = 18000;

void RTC_information()
{
  //设置RTC秒分时，星期，月日年，后续考虑加入NTP
  myRTC.setDS1302Time(00, 29, 12, 6, 06, 06, 2020); //设置日期
}

int weather_get()
{
  http.begin(client, "http://api.seniverse.com/v3/weather/now.json?key=SHDzRbT_57w0_3POY&location=ip&language=zh-Hans&unit=c");
  int httpCode = http.GET();
  //获取HTTP返回的字符串
  if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
  {
    String payload = http.getString();
    Serial.print("Http Code is:");
    Serial.println(httpCode);

    //---------------json解析---------------
    const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 220;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, payload);
    JsonObject results_0 = doc["results"][0];
    JsonObject results_0_now = results_0["now"];
    const int results_0_now_code = results_0_now["code"]; // 天气代码，一个天气代码对应一个天气情况
    //---------------json解析完毕---------------助手链接：https://arduinojson.org/v6/assistant/
    //条件判断
    if (results_0_now_code == 0)
    {
      return 69;
    }else if (results_0_now_code == 1)
    {
      return 66;
    }else if (results_0_now_code == 4|5|6|7|8)
    {
      return 65;
    }else if (results_0_now_code == 9)
    {
      return 64;
    }else if (results_0_now_code == 10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25)
    {
      return 67;
    }else
    {
      return 68;
    }
  }
  else
  {
    httpCode = http.GET();
    Serial.println("Code Error,regetting...");
    http.end();
    return 68; //天气信息获取错误将会显示星星图标
  }
}

//屏幕显示函数
void screen_show(String time_info, int temperature, int humidity, int weather_code)
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_logisoso38_tn);
    u8g2.drawStr(5, 62, time_info.c_str());
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    u8g2.drawGlyph(0, 16, weather_code);//显示天气图标
    u8g2.setFont(u8g2_font_profont22_mf);
    u8g2.setCursor(38, 16);
    u8g2.print(temperature);
    u8g2.drawStr(65, 16, "C");
    u8g2.setCursor(88, 16);
    u8g2.print(humidity);
    u8g2.drawStr(113, 16, "%");
  } while (u8g2.nextPage());
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  //检查WIFI连接状态
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  //初始化NTP
  timeClient.begin();
  //初始化屏幕显示库
  u8g2.begin();
  //初始化温度模块
  dht.begin();
  //调用RTC函数
  //  RTC_information();
}

void loop()
{
  //RTC部分
  myRTC.updateTime();
  //NTP部分
  timeClient.update();
  delay(1000);
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    //获取温湿度信息并且储存到变量
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    currentMillis = millis();
    if (currentMillis - previousMillis_weather >= interval_weather){
      previousMillis_weather = currentMillis;
      //天气部分
      return_code = weather_get();
    };
  };
  //屏幕显示部分
  screen_show(timeClient.getFormattedTime(), temperature, humidity, return_code);
  //测试部分
  Serial.print("Code is: ");
  Serial.println(return_code);
  Serial.println(timeClient.getFormattedTime());
}
