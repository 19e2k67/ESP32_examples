// 2.8寸版本
#include <ArduinoJson.h> //请使用ArduinoJson V6版本，V5版本会导致编译失败
#include <TimeLib.h>
#include <Preferences.h>
Preferences preferences;
String PrefSSID, PrefPassword;
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include "font/ZdyLwFont_20.h"
#include "font/FxLED_32.h"
#include "img/main_img/main_img.h"
#include "img/temperature.h"
#include "img/humidity.h"
#include "img/watch_top.h"
#include "img/watch_bottom.h"
#include "img/start_gif.h"
#include "img/weather_code_jpg.h"
#include "weather_code_jpg/d00.h"
#include "img/taikongren/i0.h"
#include "img/taikongren/i1.h"
#include "img/taikongren/i2.h"
#include "img/taikongren/i3.h"
#include "img/taikongren/i4.h"
#include "img/taikongren/i5.h"
#include "img/taikongren/i6.h"
#include "img/taikongren/i7.h"
#include "img/taikongren/i8.h"
#include "img/taikongren/i9.h"
#include "img/setWiFi_img.h"
#include "img\Weather_Warning_Icon.h"
#include "img\Gif\ziji.h"
#include "img\Gif\dagu.h"
#include "img\Gif\zzzzzzz.h"
#include "src/SetWiFi.h" //Web配网
#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>
#include <TJpg_Decoder.h>
/***********************功能参数配置**********************************/
#define SerialBaud 115200 // 串口波特率

bool AutoBright = true;	  // 自动亮度控制 true - 打开 false - 关闭
byte setNTPSyncTime = 20; // 设置NTP时间同步频率，10分钟同步一次
byte setWeatherTime = 30; // 设置天气数据更新频率，30分钟更新一次
/*城市列表见CityCode文件夹内的cityCode.js文件*/
/*cityCode可在Web配网页面修改，这里可以不用管了*/
String cityCode = ""; // 手动修改天气城市代码，若为空则自动获取
/*示例1：String cityCode = "101250111";//雨花区*/
/*示例2：String cityCode = "";//自动获取*/

// 群主调试用，DIY玩家请将Boards = 1，不要问为什么！！！
byte Boards = 1;
byte GL5528, Button;

// 新增参数

// 底部动画选择
unsigned int dbgif = 1;

// 哔哩哔哩粉丝模块
String UID = "385784682";				// B站用户UID数字号
int following = 0;						// 关注数
int follower = 0;						// 粉丝数

void getCityWeater();
void getCityCode();
void setupPins();
void weaterData(String *cityDZ, String *dataSK, String *dataFC, String *dataSuggest, String *dataWarn1);
void scrollTxt(int pos);
void ButtonScrollTxt(int pos);
String hourMinute();
String week();
String monthDay();
// 获取b站粉丝数量————————————————————————//

void get_Bstation_follow()
{
	// 创建TCP连接
	HTTPClient httpClient;
	httpClient.begin("https://api.bilibili.com/x/relation/stat?vmid=" + UID + "&jsonp=jsonp");
	// URL请求地址
	// B站粉丝数链接，注意修改UID
	// 发送网络请求
	// client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host2 + "\r\n" + "Connection: close\r\n\r\n");

	httpClient.GET();
	String response = httpClient.getString();
	Serial.println("响应数据:");
	Serial.println(response);
	// 获得json格式的数据
	JsonDocument doc;
	deserializeJson(doc, response);
	// JsonObject data = doc["data"];
	// following = data["following"]; // 59
	// follower = data["follower"];   // 411

	following = doc["data"]["following"].as<int>();
	follower = doc["data"]["follower"].as<int>();
	Serial.printf("关注: %d\n", following);
	Serial.printf("粉丝: %d\n", follower);
}

// 新增按键配置引脚

#define enterbtn 36 // 进入键
#define backbtn 39	// 返回键
#define upbtn 34	// 向上键
#define downbtn 35	// 向下键

#define homebtn 33 // 主页键
#define fmqsw 16   // 蜂鸣器开关键

// 新增信号线
#define SDOIN 19  // 屏幕SDO线，备用，用于多层渲染，暂时用不上
#define DATAIN 17 // DHT11温湿度传感器数据线，暂时用不上，有了传感器再加

// 蜂鸣器引脚及启动关闭功能
#define PIN_BUZZER 25

// 初始化新增的引脚

// 手动关闭蜂鸣器功能

void fmqswRun()
{
	if (digitalRead(fmqsw) == HIGH)
	{
		digitalWrite(PIN_BUZZER, LOW);
	}
}

// 模拟闹铃功能,i控制次数,默认响铃1分钟

void alarmrun()

{

	for (int i = 0; i <= 60; i++)
	{
		digitalWrite(PIN_BUZZER, HIGH);
		delay(500);
		digitalWrite(PIN_BUZZER, LOW);
		delay(500);
	}
}

void alarmoff()

{
	digitalWrite(PIN_BUZZER, LOW);
}

/********************************************************************/

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite clk = TFT_eSprite(&tft);

unsigned int Gif_Mode = 1;
bool guanggao_Flag = false;
uint32_t targetTime = 0;
byte omm = 99;
boolean initial = 1;
byte xcolon = 0;
unsigned int colour = 0;

uint16_t bgColor = 0xFFFF;

// NTP服务器
static const char ntpServerName[] = "ntp6.aliyun.com";
const int timeZone = 8; // 东八区

WiFiUDP Udp;
unsigned int localPort = 8000;

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
String num2str(int digits);
void sendNTPpacket(IPAddress &address);

int weatherCode = 99;
bool getCityWeaterFlag = false;
bool getCityCodeFlag = false;

unsigned long t1 = 0, t2 = 0;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
	if (y >= tft.height())
		return 0;
	tft.pushImage(x, y, w, h, bitmap);
	// Return 1 to decode next block
	return 1;
}

byte loadNum = 6;
void loading(byte delayTime, byte NUM)
{
	clk.setColorDepth(8);
	clk.createSprite(200, 50);
	clk.fillSprite(0x0000);
	clk.loadFont(ZdyLwFont_20); // 加载font/ZdyLwFont_20字体
	clk.drawRoundRect(0, 0, 200, 16, 8, 0xFFFF);
	clk.fillRoundRect(3, 3, loadNum, 10, 5, 0xFFFF);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_WHITE, 0x0000);
	clk.drawString("正在连接 " + PrefSSID + " ...", 100, 40, 2);
	clk.pushSprite(20, 110);
	clk.deleteSprite();
	loadNum += NUM;
	if (loadNum >= 194)
	{
		loadNum = 194;
	}
	delay(delayTime);
	clk.unloadFont(); // 释放加载字体资源
}

//----------------------------------------------------------//

// 显示wifi连接失败，并重新进入配网模式
void displayConnectWifiFalse()
{
	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);
	TJpgDec.drawJpg(0, 0, wififalse, sizeof(wififalse));
	delay(5000);
}

unsigned long oldTime_1 = 0;
int imgNum_1 = 0;
int connectTimes = 0;
int lightValue = 0, backLight_hour = 0;

int Filter_Value;

long __tstamp;
char m[2] = {'0', '\0'};
boolean checkMillis(int m)
{
	if (millis() - __tstamp > m)
	{
		__tstamp = millis();
		return true;
	}
	else
	{
		return false;
	}
}

// 强制门户Web配网
bool setWiFi_Flag = false;
void setWiFi()
{
	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);
	TJpgDec.drawJpg(0, 0, setWiFi_img, sizeof(setWiFi_img));

	initBasic();
	initSoftAP();
	initWebServer();
	initDNS();
	while (setWiFi_Flag == false)
	{
		server.handleClient();
		dnsServer.processNextRequest();
		if (WiFi.status() == WL_CONNECTED)
		{
			server.stop();
			setWiFi_Flag = true;
		}
	}
}

// 新增主页功能页面,待添加

void homepage()
{
}

time_t prevDisplay = 0; // 显示时间
unsigned long weaterTime = 0;

float v1 = 2.0;
int time123 = 0;

#define FILTER_N 20
int Filter()
{
	int i;
	int filter_sum = 0;
	int filter_max, filter_min;
	int filter_buf[FILTER_N];
	for (i = 0; i < FILTER_N; i++)
	{
		filter_buf[i] = analogRead(GL5528);
		delay(1);
	}
	filter_max = filter_buf[0];
	filter_min = filter_buf[0];
	filter_sum = filter_buf[0];
	for (i = FILTER_N - 1; i > 0; i--)
	{
		if (filter_buf[i] > filter_max)
			filter_max = filter_buf[i];
		else if (filter_buf[i] < filter_min)
			filter_min = filter_buf[i];
		filter_sum = filter_sum + filter_buf[i];
		filter_buf[i] = filter_buf[i - 1];
	}
	i = FILTER_N - 2;
	filter_sum = filter_sum - filter_max - filter_min + i / 2; // +i/2 的目的是为了四舍五入
	filter_sum = filter_sum / i;
	return filter_sum;
}

unsigned long wdsdTime = 0;
byte wdsdValue = 0;
String wendu = "", shidu = "";

unsigned long wifiTimes = 0;

void weatherWarning()
{ // 间隔5秒切换显示温度和湿度，该数据为气象站获取的室外参数
	if (millis() - wdsdTime > 5000)
	{
		wdsdValue = wdsdValue + 1;
		// Serial.println("wdsdValue0" + String(wdsdValue));
		clk.setColorDepth(8);
		clk.loadFont(ZdyLwFont_20);
		switch (wdsdValue)
		{
		case 1:
			// Serial.println("wdsdValue1" + String(wdsdValue));
			TJpgDec.drawJpg(165, 171, temperature, sizeof(temperature)); // 温度图标
			for (int i = 20; i > 0; i--)
			{
				clk.createSprite(50, 32);
				clk.fillSprite(bgColor);
				clk.setTextDatum(CC_DATUM);
				clk.setTextColor(TFT_BLACK, bgColor);
				clk.drawString(wendu + "℃", 25, i + 16);
				clk.pushSprite(188, 168);
				clk.deleteSprite();
				vTaskDelay(3);
			}
			break;
		case 2:
			// Serial.println("wdsdValue2" + String(wdsdValue));
			TJpgDec.drawJpg(165, 171, humidity, sizeof(humidity)); // 湿度图标
			for (int i = 20; i > 0; i--)
			{
				clk.createSprite(50, 32);
				clk.fillSprite(bgColor);
				clk.setTextDatum(CC_DATUM);
				clk.setTextColor(TFT_BLACK, bgColor);
				clk.drawString(shidu, 25, i + 16);
				clk.pushSprite(188, 168);
				clk.deleteSprite();
				vTaskDelay(3);
			}
			wdsdValue = 0;
			break;
		}
		wdsdTime = millis();
		clk.unloadFont();
	}
}

void smartConfigWIFI()
{
	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);
	TJpgDec.drawJpg(0, 0, wifi_config, sizeof(wifi_config)); // 显示微信配网图片
	WiFi.mode(WIFI_AP_STA);
	delay(100);
	WiFi.beginSmartConfig();
	Serial.println("配网中.");
	while (!WiFi.smartConfigDone())
	{
		delay(500);
		Serial.print(".");
	}

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	preferences.begin("wifi", false);
	preferences.putString("ssid", WiFi.SSID());
	preferences.putString("password", WiFi.psk());
	preferences.end();

	Serial.println("配网完成，正在重启...");
	delay(2000);
	ESP.restart(); // 重启ESP32
}

// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode()
{
	int OldConnectionTimes = millis(), NewConnectionTimes = 0;
	// 创建 HTTPClient 对象
	HTTPClient httpClient;
	while (getCityCodeFlag == false)
	{
		String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());

		// 配置请求地址。此处也可以不使用端口号和PATH而单纯的
		httpClient.begin(URL);

		// 设置请求头中的User-Agent
		httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
		httpClient.addHeader("Referer", "http://www.weather.com.cn/");

		// 启动连接并发送HTTP请求
		int httpCode = httpClient.GET();
		// Serial.print("Send GET request to URL: ");
		// Serial.println(URL);
		Serial.println("数据请求中...");

		// 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
		if (httpCode == HTTP_CODE_OK)
		{
			String str = httpClient.getString();
			// Serial.println(str);
			int aa = str.indexOf("id=");
			if (aa > -1)
			{
				cityCode = str.substring(aa + 4, aa + 4 + 9);
				// Serial.println(cityCode);
				Serial.println("获取城市代码成功");
				getCityCodeFlag = true;
				getCityWeater();
			}
			else
			{
				Serial.println("获取城市代码失败，正在重新获取...");
			}
		}
		else
		{
			Serial.print("请求城市代码错误：");
			Serial.println(String(httpCode) + "正在重新获取...");
		}
		// 连接时长超过5秒，直接重启重新连接
		NewConnectionTimes = millis();
		if ((NewConnectionTimes - OldConnectionTimes) >= 5000)
		{
			ESP.restart(); // 重启ESP32
		}
	}
	// 关闭ESP8266与服务器连接
	httpClient.end();
}

// 获取城市天气
/*
 * 101250111 - 雨花区
 * 101250106 - 长沙县
 * 101250101 - 长沙
 */
bool warn_2 = false;
int Warn_Number1 = 0, Warn_Value1 = 0, Warn_Number2 = 0, Warn_Value2 = 0, Warn_Flag = 1;
void getCityWeater()
{
	int OldConnectionTimes = millis(), NewConnectionTimes = 0;
	// cityCode = "101250106";
	HTTPClient httpClient;
	while (getCityWeaterFlag == false)
	{
		String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now());
		// 创建 HTTPClient 对象

		httpClient.begin(URL);

		// 设置请求头中的User-Agent
		httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
		httpClient.addHeader("Referer", "http://www.weather.com.cn/");

		// 启动连接并发送HTTP请求
		int httpCode = httpClient.GET();
		Serial.println("正在获取天气数据");
		Serial.println(URL);

		// 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
		if (httpCode == HTTP_CODE_OK)
		{

			String str = httpClient.getString();
			// Serial.println(str);

			int indexStart = str.indexOf("weatherinfo\":");
			int indexEnd = str.indexOf("};var alarmDZ");
			String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
			// Serial.println(jsonCityDZ);

			// 气象预警不同时间会发布不同的预警信息，只会显示最新的一个，显示多个也只是显示最新时间的前一个预警，没必要了
			indexStart = str.indexOf("alarmDZ ={\"w\":[");
			indexEnd = str.indexOf("]};var dataSK");
			String jsonDataWarn1 = str.substring(indexStart + 15, indexEnd);
			// Serial.println("1="+jsonDataWarn1);
			if (jsonDataWarn1.length() >= 40)
			{
				Warn_Flag = 1;
			}
			else
			{
				Warn_Flag = 0;
			}

			indexStart = str.indexOf("dataSK =");
			indexEnd = str.indexOf(";var dataZS");
			String jsonDataSK = str.substring(indexStart + 8, indexEnd);
			// Serial.println(jsonDataSK);

			indexStart = str.indexOf("\"f\":[");
			indexEnd = str.indexOf(",{\"fa");
			String jsonFC = str.substring(indexStart + 5, indexEnd);
			// Serial.println(jsonFC);

			indexStart = str.indexOf(";var dataZS ={\"zs\":");
			indexEnd = str.indexOf(",\"cn\":\"长沙\"};var fc =");
			String jsonSuggest = str.substring(indexStart + 19, indexEnd);
			// Serial.println(jsonSuggest);

			weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC, &jsonSuggest, &jsonDataWarn1);
			Serial.println("天气数据获取成功");
			getCityWeaterFlag = true;
		}
		else
		{
			Serial.print("请求城市天气错误：");
			Serial.println(String(httpCode) + " 正在重新获取...");
		}
		// 连接时长超过5秒，直接重启重新连接
		NewConnectionTimes = millis();
		if ((NewConnectionTimes - OldConnectionTimes) >= 3000)
		{
			// ESP.restart(); //重启ESP32
			break;
		}
	}
	// 关闭ESP8266与服务器连接
	httpClient.end();
}

void getLunarCalendar()
{
	HTTPClient httpClient;
	String URL = "https://api.xlongwei.com/service/datetime/convert.json";
	// 创建 HTTPClient 对象

	httpClient.begin(URL);

	// 设置请求头中的User-Agent
	httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
	httpClient.addHeader("Referer", "http://www.weather.com.cn/");

	// 启动连接并发送HTTP请求
	int httpCode = httpClient.GET();
	Serial.println("正在获取天气数据");
	Serial.println(URL);

	// 如果服务器响应OK则从服务器获取响应体信息并通过串口输出
	if (httpCode == HTTP_CODE_OK)
	{
		String str = httpClient.getString();
		Serial.println("农历" + str);
	}
}

String scrollText[6];
String ButtonScrollText[8];
// int scrollTextWidth = 0;
// 天气信息写到屏幕上
void weaterData(String *cityDZ, String *dataSK, String *dataFC, String *dataSuggest, String *dataWarn1)
{

	JsonDocument doc;
	deserializeJson(doc, *dataSK);
	JsonObject sk = doc.as<JsonObject>();

	// TFT_eSprite clkb = TFT_eSprite(&tft);

	/***绘制相关文字***/
	clk.setColorDepth(8);
	clk.loadFont(ZdyLwFont_20); // 加载font/ZdyLwFont_20字体
	wendu = sk["temp"].as<String>();
	shidu = sk["SD"].as<String>();

	// 城市名称
	clk.createSprite(88, 32); // 88,32
	clk.fillSprite(bgColor);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_BLACK, bgColor);
	// clk.drawString(sk["cityname"].as<String>()+"区",44,18);
	clk.drawString(sk["cityname"].as<String>(), 44, 18);
	clk.pushSprite(151, 1);
	clk.deleteSprite();

	// PM2.5空气指数
	uint16_t pm25BgColor; // 优
	String aqiTxt;
	int pm25V = sk["aqi"];
	// Serial.println("pm25V:" + String(pm25V));
	if (pm25V >= 301)
	{
		pm25BgColor = tft.color565(255, 36, 0); // 重度
		aqiTxt = "严重";
	}
	else if (pm25V >= 201 & pm25V <= 300)
	{
		pm25BgColor = tft.color565(136, 11, 32); // 重度
		aqiTxt = "重度";
	}
	else if (pm25V >= 151 & pm25V <= 200)
	{
		pm25BgColor = tft.color565(186, 55, 121); // 中度
		aqiTxt = "中度";
	}
	else if (pm25V >= 101 & pm25V <= 160)
	{
		pm25BgColor = tft.color565(242, 159, 57); // 轻
		aqiTxt = "轻度";
	}
	else if (pm25V >= 51 & pm25V <= 100)
	{
		pm25BgColor = tft.color565(247, 219, 100); // 良
		aqiTxt = "良";
	}
	else if (pm25V >= 0 & pm25V <= 50)
	{
		pm25BgColor = tft.color565(156, 202, 127); // 优
		aqiTxt = "优";
	}
	clk.createSprite(50, 24);
	clk.fillSprite(bgColor);
	clk.fillRoundRect(0, 0, 50, 24, 4, pm25BgColor);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(0xFFFF);
	clk.drawString(aqiTxt, 25, 14);
	clk.pushSprite(5, 140);
	clk.deleteSprite();

	// 左上角滚动字幕
	// 解析第二段JSON
	scrollText[0] = "实时天气 " + sk["weather"].as<String>();
	scrollText[1] = "空气质量 " + aqiTxt;

	scrollText[2] = "风向 " + sk["WD"].as<String>() + sk["WS"].as<String>();

	deserializeJson(doc, *cityDZ);
	JsonObject dz = doc.as<JsonObject>();
	scrollText[3] = "今日 " + dz["weather"].as<String>();

	// 显示天气图标
	String weatherCodeText = dz["weathercode"].as<String>();
	weatherCode = weatherCodeText.substring(1, weatherCodeText.length() + 1).toInt();
	// Serial.println(weatherCode);
	switch (weatherCode)
	{
	case 0:
		TJpgDec.drawJpg(10, 105, d00_40X30, sizeof(d00_40X30));
		break;
	case 1:
		TJpgDec.drawJpg(10, 105, d01_40X30, sizeof(d01_40X30));
		break;
	case 2:
		TJpgDec.drawJpg(10, 105, d02_40X30, sizeof(d02_40X30));
		break;
	case 3:
		TJpgDec.drawJpg(10, 105, d03_40X30, sizeof(d03_40X30));
		break;
	case 4:
		TJpgDec.drawJpg(10, 105, d04_40X30, sizeof(d04_40X30));
		break;
	case 5:
		TJpgDec.drawJpg(10, 105, d05_40X30, sizeof(d05_40X30));
		break;
	case 6:
		TJpgDec.drawJpg(10, 105, d06_40X30, sizeof(d06_40X30));
		break;
	case 7:
		TJpgDec.drawJpg(10, 105, d07_40X30, sizeof(d07_40X30));
		break;
	case 8:
		TJpgDec.drawJpg(10, 105, d08_40X30, sizeof(d08_40X30));
		break;
	case 9:
		TJpgDec.drawJpg(10, 105, d09_40X30, sizeof(d09_40X30));
		break;
	case 10:
		TJpgDec.drawJpg(10, 105, d10_40X30, sizeof(d10_40X30));
		break;
	case 11:
		TJpgDec.drawJpg(10, 105, d11_40X30, sizeof(d11_40X30));
		break;
	case 12:
		TJpgDec.drawJpg(10, 105, d12_40X30, sizeof(d12_40X30));
		break;
	case 13:
		TJpgDec.drawJpg(10, 105, d13_40X30, sizeof(d13_40X30));
		break;
	case 14:
		TJpgDec.drawJpg(10, 105, d14_40X30, sizeof(d14_40X30));
		break;
	case 15:
		TJpgDec.drawJpg(10, 105, d15_40X30, sizeof(d15_40X30));
		break;
	case 16:
		TJpgDec.drawJpg(10, 105, d16_40X30, sizeof(d16_40X30));
		break;
	case 17:
		TJpgDec.drawJpg(10, 105, d17_40X30, sizeof(d17_40X30));
		break;
	case 18:
		TJpgDec.drawJpg(10, 105, d18_40X30, sizeof(d18_40X30));
		break;
	case 19:
		TJpgDec.drawJpg(10, 105, d19_40X30, sizeof(d19_40X30));
		break;
	case 20:
		TJpgDec.drawJpg(10, 105, d20_40X30, sizeof(d20_40X30));
		break;
	case 21:
		TJpgDec.drawJpg(10, 105, d21_40X30, sizeof(d21_40X30));
		break;
	case 22:
		TJpgDec.drawJpg(10, 105, d22_40X30, sizeof(d22_40X30));
		break;
	case 23:
		TJpgDec.drawJpg(10, 105, d23_40X30, sizeof(d23_40X30));
		break;
	case 24:
		TJpgDec.drawJpg(10, 105, d24_40X30, sizeof(d24_40X30));
		break;
	case 25:
		TJpgDec.drawJpg(10, 105, d25_40X30, sizeof(d25_40X30));
		break;
	case 26:
		TJpgDec.drawJpg(10, 105, d26_40X30, sizeof(d26_40X30));
		break;
	case 27:
		TJpgDec.drawJpg(10, 105, d27_40X30, sizeof(d27_40X30));
		break;
	case 28:
		TJpgDec.drawJpg(10, 105, d28_40X30, sizeof(d28_40X30));
		break;
	case 29:
		TJpgDec.drawJpg(10, 105, d29_40X30, sizeof(d29_40X30));
		break;
	case 30:
		TJpgDec.drawJpg(10, 105, d30_40X30, sizeof(d30_40X30));
		break;
	case 31:
		TJpgDec.drawJpg(10, 105, d31_40X30, sizeof(d31_40X30));
		break;
	case 32:
		TJpgDec.drawJpg(10, 105, d32_40X30, sizeof(d32_40X30));
		break;
	case 33:
		TJpgDec.drawJpg(10, 105, d33_40X30, sizeof(d33_40X30));
		break;
	case 49:
		TJpgDec.drawJpg(10, 105, d49_40X30, sizeof(d49_40X30));
		break;
	case 53:
		TJpgDec.drawJpg(10, 105, d53_40X30, sizeof(d53_40X30));
		break;
	case 54:
		TJpgDec.drawJpg(10, 105, d54_40X30, sizeof(d54_40X30));
		break;
	case 55:
		TJpgDec.drawJpg(10, 105, d55_40X30, sizeof(d55_40X30));
		break;
	case 56:
		TJpgDec.drawJpg(10, 105, d56_40X30, sizeof(d56_40X30));
		break;
	case 57:
		TJpgDec.drawJpg(10, 105, d57_40X30, sizeof(d57_40X30));
		break;
	case 58:
		TJpgDec.drawJpg(10, 105, d58_40X30, sizeof(d58_40X30));
		break;
	case 301:
		TJpgDec.drawJpg(10, 105, d301_40X30, sizeof(d301_40X30));
		break;
	case 302:
		TJpgDec.drawJpg(10, 105, d302_40X30, sizeof(d302_40X30));
		break;
	default:
		break;
	}

	deserializeJson(doc, *dataFC);
	JsonObject fc = doc.as<JsonObject>();

	scrollText[4] = "最低温度 " + fc["fd"].as<String>() + "℃";
	scrollText[5] = "最高温度 " + fc["fc"].as<String>() + "℃";

	// scrollText[6] = "PM2.5 "+sk["aqi"].as<String>();

	// Serial.println(scrollText[0]);
	clk.unloadFont(); // 释放加载字体资源

	deserializeJson(doc, *dataSuggest);
	JsonObject dataSuggestJson = doc.as<JsonObject>();
	ButtonScrollText[0] = dataSuggestJson["lk_name"].as<String>() + " " + dataSuggestJson["lk_hint"].as<String>();
	ButtonScrollText[1] = dataSuggestJson["cl_name"].as<String>() + " " + dataSuggestJson["cl_hint"].as<String>();
	ButtonScrollText[2] = dataSuggestJson["uv_name"].as<String>() + " " + dataSuggestJson["uv_hint"].as<String>();
	ButtonScrollText[3] = dataSuggestJson["ct_name"].as<String>() + " " + dataSuggestJson["ct_hint"].as<String>();
	ButtonScrollText[4] = dataSuggestJson["gm_name"].as<String>() + " " + dataSuggestJson["gm_hint"].as<String>();
	ButtonScrollText[5] = dataSuggestJson["ys_name"].as<String>() + " " + dataSuggestJson["ys_hint"].as<String>();
	// ButtonScrollText[6] = dataSuggestJson["gz_name"].as<String>() + " " + dataSuggestJson["gz_hint"].as<String>();
	// ButtonScrollText[6] = dataSuggestJson["cl_name"].as<String>() + " " + dataSuggestJson["cl_hint"].as<String>();
	ButtonScrollText[6] = dataSuggestJson["pl_name"].as<String>() + " " + dataSuggestJson["pl_hint"].as<String>();
	ButtonScrollText[7] = dataSuggestJson["co_name"].as<String>() + " " + dataSuggestJson["co_hint"].as<String>();

	deserializeJson(doc, *dataWarn1);
	JsonObject dataWarnjson1 = doc.as<JsonObject>();
	Warn_Number1 = dataWarnjson1["w4"].as<int>();
	Warn_Value1 = dataWarnjson1["w6"].as<int>();
	// Serial.println(dataWarnjson);
	Serial.println("气象预警编号1：" + String(Warn_Number1) + " 等级1：" + String(Warn_Value1));
	/*switch(Warn_Number) { // 处理图片太TM烦躁了，而且显示的效果也特别差，而且预警代码一直没找到文档
	  case 7: //高温
		if(Warn_Value == 1) {TJpgDec.drawJpg(175,110,gaowen_lan, sizeof(gaowen_lan));break;}
		if(Warn_Value == 2) {TJpgDec.drawJpg(175,110,gaowen_huang, sizeof(gaowen_huang));break;}
		if(Warn_Value == 3) {TJpgDec.drawJpg(175,110,gaowen_cheng, sizeof(gaowen_cheng));break;}
		if(Warn_Value == 4) {TJpgDec.drawJpg(175,110,gaowen_hong, sizeof(gaowen_hong));break;}
	  //case 9: //雷电
	  //case 0: //雷暴大风
	  //case 2: //暴雨
	  default:TJpgDec.drawJpg(175,110,BG_Color, sizeof(BG_Color));Serial.println("NULL");break;
	}*/
	uint16_t weatherWarnBgColor1;
	switch (Warn_Value1)
	{ // 这等级把我搞蒙了，一会蓝色是0，一会又变成1
	// 填充颜色
	case 1:
		weatherWarnBgColor1 = tft.color565(0, 128, 255);
		break; // 蓝色
	case 2:
		weatherWarnBgColor1 = tft.color565(255, 204, 51);
		break; // 黄色
	case 3:
		weatherWarnBgColor1 = tft.color565(255, 153, 0);
		break; // 橙色
	case 4:
		weatherWarnBgColor1 = tft.color565(255, 0, 0);
		break; // 红色
	default:
		Serial.println("NULL");
		break;
	}
	// 多个气象预警显示，有空了再更新
	// if(Warn_Flag == 1) {
	if (dataWarnjson1["w5"].as<String>() != "null")
	{
		clk.loadFont(ZdyLwFont_20);
		clk.createSprite(90, 24);
		clk.fillSprite(bgColor);
		clk.fillRoundRect(0, 0, 90, 24, 5, weatherWarnBgColor1);
		clk.setTextDatum(CC_DATUM);
		clk.setTextColor(TFT_WHITE);
		clk.drawString(dataWarnjson1["w5"].as<String>(), 45, 14);
		// clk.drawString("预 警",45,45);
		clk.pushSprite(145, 140);
		clk.deleteSprite();
		clk.unloadFont();
		clk.unloadFont();
	}
}

/*
<select id="kind" name="kind">
		<option value="" class="talarm">请选择预警种类</option>
		<option value="01">台风预警</option>
		<option value="02">暴雨预警</option>
		<option value="03">暴雪预警</option>
		<option value="04">寒潮预警</option>
		<option value="05">大风预警</option>
		<option value="06">沙尘暴预警</option>
		<option value="07">高温预警</option>
		<option value="08">干旱预警</option>
		<option value="09">雷电预警</option>
		<option value="10">冰雹预警</option>
		<option value="11">霜冻预警</option>
		<option value="12">大雾预警</option>
		<option value="13">霾预警</option>
		<option value="14">道路结冰预警</option>

		<option value="51">海上大雾预警</option>
		<option value="52">雷暴大风预警</option>
		<option value="53">持续低温预警</option>
		<option value="54">浓浮尘预警</option>
		<option value="55">龙卷风预警</option>
		<option value="56">低温冻害预警</option>
		<option value="57">海上大风预警</option>
		<option value="58">低温雨雪冰冻预警</option>
		<option value="59">强对流预警</option>
		<option value="60">臭氧预警</option>
		<option value="61">大雪预警</option>
		<option value="62">强降雨预警</option>
		<option value="63">强降温预警</option>
		<option value="64">雪灾预警</option>
		<option value="65">森林（草原）火险预警</option>
		<option value="66">雷暴预警</option>
		<option value="67">严寒预警</option>
		<option value="68">沙尘预警</option>
		<option value="69">海上雷雨大风预警</option>
		<option value="70">海上雷电预警</option>
		<option value="71">海上台风预警</option>
		<option value="72">低温预警</option>


		<option value="91">寒冷预警</option>
		<option value="92">灰霾预警</option>
		<option value="93">雷雨大风预警</option>
		<option value="94">森林火险预警</option>
		<option value="95">降温预警</option>
		<option value="96">道路冰雪预警</option>
		<option value="97">干热风预警</option>
		<option value="98">空气重污染预警</option>
		<option value="99">冰冻预警</option>

	  </select>
	  <select id="grade" name="grade">
		<option value="" class="calarm">请选择预警等级</option>
		<option value="04">红色</option>
		<option value="03">橙色</option>
		<option value="02">黄色</option>
		<option value="01">蓝色</option>
		<option value="05">未知</option>
	  </select>
*/

int currentIndex = 0;
int prevTime = 0;
TFT_eSprite clkb = TFT_eSprite(&tft);

void scrollBanner()
{
	if (millis() - prevTime > 3500)
	{ // 3.5秒切换一次

		if (scrollText[currentIndex])
		{

			clkb.setColorDepth(8);
			clkb.loadFont(ZdyLwFont_20);

			for (int pos = 20; pos > 0; pos--)
			{
				scrollTxt(pos);
			}

			clkb.deleteSprite();
			clkb.unloadFont();

			if (currentIndex >= 5)
			{
				currentIndex = 0; // 回第一个
			}
			else
			{
				currentIndex += 1; // 准备切换到下一个
			}

			// Serial.println(currentIndex);
		}
		prevTime = millis();
	}
}

void scrollTxt(int pos)
{
	clkb.createSprite(148, 24);
	clkb.fillSprite(bgColor);
	clkb.setTextWrap(false);
	clkb.setTextDatum(CC_DATUM);
	clkb.setTextColor(TFT_BLACK, bgColor);
	clkb.drawString(scrollText[currentIndex], 74, pos + 14);
	clkb.pushSprite(2, 4);
}

/**
 *底部生活信息滚动显示
 */

byte ButtoncurrentIndex = 0;
unsigned long ButtonprevTime = 0;
TFT_eSprite clkbb = TFT_eSprite(&tft);

void ButtonscrollBanner()
{
	if (millis() - ButtonprevTime > 5000)
	{ // 5秒切换一次

		if (ButtonScrollText[ButtoncurrentIndex])
		{
			clkbb.loadFont(ZdyLwFont_20);

			for (int pos = 20; pos > 0; pos--)
			{
				ButtonScrollTxt(pos);
			}

			clkbb.deleteSprite();
			clkbb.unloadFont();

			if (ButtoncurrentIndex >= 7)
			{
				ButtoncurrentIndex = 0; // 回第一个
			}
			else
			{
				ButtoncurrentIndex += 1; // 准备切换到下一个
			}

			// Serial.println(ButtoncurrentIndex);
		}
		ButtonprevTime = millis();
	}
}

void ButtonScrollTxt(int pos)
{
	// clkbb.loadFont(ZdyLwFont_20);
	clkbb.createSprite(240, 40);
	clkbb.fillSprite(bgColor);
	clkbb.setTextDatum(CC_DATUM);
	clkbb.setTextColor(TFT_BLACK, bgColor);
	clkbb.drawString(ButtonScrollText[ButtoncurrentIndex], 120, pos + 20);
	clkbb.pushSprite(0, 201);
	// clkbb.deleteSprite();
	// clkbb.unloadFont(); //释放加载字体资源
}

unsigned long oldTime = 0, imgNum = 1;
void imgDisplay()
{
	int x, y = 94, dt;
	switch (Warn_Flag)
	{ // 如果有气象预警信息，图标自动左移
	case 0:
		x = 90;
		break;
	case 1:
		if (Gif_Mode == 4)
		{
			x = 65;
			y = 85;
		}
		else if (Gif_Mode == 3)
		{
			x = 70;
			y = 94;
		}
		else if (Gif_Mode == 2)
		{
			x = 70;
			y = 86;
		}
		else if (Gif_Mode == 1)
		{
			x = 60;
			y = 94;
		}
		break;
	}
	switch (Gif_Mode)
	{ // 修改动画的播放速度
	case 1:
		dt = 100;
		break;
	case 2:
		dt = 50;
		break;
	case 3:
		dt = 100;
		break;
	case 4:
		dt = 100;
		break;
	}
	if (millis() - oldTime >= dt)
	{
		imgNum = imgNum + 1;
		oldTime = millis();
	}
	if (Gif_Mode == 1)
	{ // 动画-龙猫转圈
		switch (imgNum)
		{

		case 1:
			TJpgDec.drawJpg(x, y, img_0, sizeof(img_0));
			break;
		case 2:
			TJpgDec.drawJpg(x, y, img_1, sizeof(img_1));
			break;
		case 3:
			TJpgDec.drawJpg(x, y, img_2, sizeof(img_2));
			break;
		case 4:
			TJpgDec.drawJpg(x, y, img_3, sizeof(img_3));
			break;
		case 5:
			TJpgDec.drawJpg(x, y, img_4, sizeof(img_4));
			break;
		case 6:
			TJpgDec.drawJpg(x, y, img_5, sizeof(img_5));
			break;
		case 7:
			TJpgDec.drawJpg(x, y, img_6, sizeof(img_6));
			break;
		case 8:
			TJpgDec.drawJpg(x, y, img_7, sizeof(img_7));
			break;
		case 9:
			TJpgDec.drawJpg(x, y, img_8, sizeof(img_8));
			break;
		case 10:
			TJpgDec.drawJpg(x, y, img_9, sizeof(img_9));
			break;
		case 11:
			TJpgDec.drawJpg(x, y, img_10, sizeof(img_10));
			break;
		case 12:
			TJpgDec.drawJpg(x, y, img_11, sizeof(img_11));
			break;
		case 13:
			TJpgDec.drawJpg(x, y, img_12, sizeof(img_12));
			break;
		case 14:
			TJpgDec.drawJpg(x, y, img_13, sizeof(img_13));
			break;
		case 15:
			TJpgDec.drawJpg(x, y, img_14, sizeof(img_14));
			break;
		case 16:
			TJpgDec.drawJpg(x, y, img_15, sizeof(img_15));
			break;
		case 17:
			TJpgDec.drawJpg(x, y, img_16, sizeof(img_16));
			break;
		case 18:
			TJpgDec.drawJpg(x, y, img_17, sizeof(img_17));
			break;
		case 19:
			TJpgDec.drawJpg(x, y, img_18, sizeof(img_18));
			break;
		case 20:
			TJpgDec.drawJpg(x, y, img_19, sizeof(img_19));
			break;
		case 21:
			TJpgDec.drawJpg(x, y, img_20, sizeof(img_20));
			break;
		case 22:
			TJpgDec.drawJpg(x, y, img_21, sizeof(img_21));
			break;
		case 23:
			TJpgDec.drawJpg(x, y, img_22, sizeof(img_22));
			break;
		case 24:
			TJpgDec.drawJpg(x, y, img_23, sizeof(img_23));
			break;
		case 25:
			TJpgDec.drawJpg(x, y, img_24, sizeof(img_24));
			break;
		case 26:
			TJpgDec.drawJpg(x, y, img_25, sizeof(img_25));
			break;
		case 27:
			TJpgDec.drawJpg(x, y, img_26, sizeof(img_26));
			break;
		case 28:
			TJpgDec.drawJpg(x, y, img_27, sizeof(img_27));
			break;
		case 29:
			TJpgDec.drawJpg(x, y, img_28, sizeof(img_28));
			break;
		case 30:
			TJpgDec.drawJpg(x, y, img_29, sizeof(img_29));
			break;
		case 31:
			TJpgDec.drawJpg(x, y, img_30, sizeof(img_30));
			break;
		case 32:
			TJpgDec.drawJpg(x, y, img_31, sizeof(img_31));
			break;
		case 33:
			TJpgDec.drawJpg(x, y, img_32, sizeof(img_32));
			break;
		case 34:
			TJpgDec.drawJpg(x, y, img_33, sizeof(img_33));
			break;
		case 35:
			TJpgDec.drawJpg(x, y, img_34, sizeof(img_34));
			break;
		case 36:
			TJpgDec.drawJpg(x, y, img_35, sizeof(img_35));
			break;
		case 37:
			TJpgDec.drawJpg(x, y, img_36, sizeof(img_36));
			break;
		case 38:
			TJpgDec.drawJpg(x, y, img_37, sizeof(img_37));
			break;
		case 39:
			TJpgDec.drawJpg(x, y, img_38, sizeof(img_38));
			break;
		case 40:
			TJpgDec.drawJpg(x, y, img_39, sizeof(img_39));
			break;
		case 41:
			TJpgDec.drawJpg(x, y, img_40, sizeof(img_40));
			break;
		case 42:
			TJpgDec.drawJpg(x, y, img_41, sizeof(img_41));
			break;
		case 43:
			TJpgDec.drawJpg(x, y, img_42, sizeof(img_42));
			break;
		case 44:
			TJpgDec.drawJpg(x, y, img_43, sizeof(img_43));
			break;
		case 45:
			TJpgDec.drawJpg(x, y, img_44, sizeof(img_44));
			break;
		case 46:
			TJpgDec.drawJpg(x, y, img_45, sizeof(img_45));
			break;
		case 47:
			TJpgDec.drawJpg(x, y, img_46, sizeof(img_46));
			break;
		case 48:
			TJpgDec.drawJpg(x, y, img_47, sizeof(img_47));
			break;
		case 49:
			TJpgDec.drawJpg(x, y, img_48, sizeof(img_48));
			break;
		case 50:
			TJpgDec.drawJpg(x, y, img_49, sizeof(img_49));
			break;
		case 51:
			TJpgDec.drawJpg(x, y, img_50, sizeof(img_50));
			break;
		case 52:
			TJpgDec.drawJpg(x, y, img_51, sizeof(img_51));
			break;
		case 53:
			TJpgDec.drawJpg(x, y, img_52, sizeof(img_52));
			break;
		case 54:
			TJpgDec.drawJpg(x, y, img_53, sizeof(img_53));
			break;
		case 55:
			TJpgDec.drawJpg(x, y, img_54, sizeof(img_54));
			break;
		case 56:
			TJpgDec.drawJpg(x, y, img_55, sizeof(img_55));
			break;
		case 57:
			TJpgDec.drawJpg(x, y, img_56, sizeof(img_56));
			break;
		case 58:
			TJpgDec.drawJpg(x, y, img_57, sizeof(img_57));
			break;
		case 59:
			TJpgDec.drawJpg(x, y, img_58, sizeof(img_58));
			break;
		case 60:
			TJpgDec.drawJpg(x, y, img_59, sizeof(img_59));
			break;
		case 61:
			TJpgDec.drawJpg(x, y, img_60, sizeof(img_60));
			break;
		case 62:
			TJpgDec.drawJpg(x, y, img_61, sizeof(img_61));
			break;
		case 63:
			TJpgDec.drawJpg(x, y, img_62, sizeof(img_62));
			break;
		case 64:
			TJpgDec.drawJpg(x, y, img_63, sizeof(img_63));
			break;
		case 65:
			TJpgDec.drawJpg(x, y, img_64, sizeof(img_64));
			break;
		case 66:
			TJpgDec.drawJpg(x, y, img_65, sizeof(img_65));
			break;
		case 67:
			TJpgDec.drawJpg(x, y, img_66, sizeof(img_66));
			break;
		case 68:
			TJpgDec.drawJpg(x, y, img_67, sizeof(img_67));
			break;
		case 69:
			TJpgDec.drawJpg(x, y, img_68, sizeof(img_68));
			break;
		case 70:
			TJpgDec.drawJpg(x, y, img_69, sizeof(img_69));
			break;
		case 71:
			TJpgDec.drawJpg(x, y, img_70, sizeof(img_70));
			break;
		case 72:
			TJpgDec.drawJpg(x, y, img_71, sizeof(img_71));
			break;
		case 73:
			TJpgDec.drawJpg(x, y, img_72, sizeof(img_72));
			break;
		case 74:
			TJpgDec.drawJpg(x, y, img_73, sizeof(img_73));
			break;
		case 75:
			TJpgDec.drawJpg(x, y, img_74, sizeof(img_74));
			break;
		case 76:
			TJpgDec.drawJpg(x, y, img_75, sizeof(img_75));
			break;
		case 77:
			TJpgDec.drawJpg(x, y, img_76, sizeof(img_76));
			break;
		case 78:
			TJpgDec.drawJpg(x, y, img_77, sizeof(img_77));
			break;
		case 79:
			TJpgDec.drawJpg(x, y, img_78, sizeof(img_78));
			break;
		case 80:
			TJpgDec.drawJpg(x, y, img_79, sizeof(img_79));
			imgNum = 1;
			break;
		}
	}
	else if (Gif_Mode == 3)
	{ // 动画-太空人
		switch (imgNum)
		{
		case 1:
			TJpgDec.drawJpg(x, y, i0, sizeof(i0));
			break;
		case 2:
			TJpgDec.drawJpg(x, y, i1, sizeof(i1));
			break;
		case 3:
			TJpgDec.drawJpg(x, y, i2, sizeof(i2));
			break;
		case 4:
			TJpgDec.drawJpg(x, y, i3, sizeof(i3));
			break;
		case 5:
			TJpgDec.drawJpg(x, y, i4, sizeof(i4));
			break;
		case 6:
			TJpgDec.drawJpg(x, y, i5, sizeof(i5));
			break;
		case 7:
			TJpgDec.drawJpg(x, y, i6, sizeof(i6));
			break;
		case 8:
			TJpgDec.drawJpg(x, y, i7, sizeof(i7));
			break;
		case 9:
			TJpgDec.drawJpg(x, y, i8, sizeof(i8));
			break;
		case 10:
			TJpgDec.drawJpg(x, y, i9, sizeof(i9));
			imgNum = 1;
			break;
		}
	}
	else if (Gif_Mode == 2)
	{ // 动画-耍杂技
		y = 86;
		switch (imgNum)
		{
		case 1:
			TJpgDec.drawJpg(x, y, ziji_01_61x80, sizeof(ziji_01_61x80));
			break;
		case 2:
			TJpgDec.drawJpg(x, y, ziji_02_61x80, sizeof(ziji_02_61x80));
			break;
		case 3:
			TJpgDec.drawJpg(x, y, ziji_03_61x80, sizeof(ziji_03_61x80));
			break;
		case 4:
			TJpgDec.drawJpg(x, y, ziji_04_61x80, sizeof(ziji_04_61x80));
			break;
		case 5:
			TJpgDec.drawJpg(x, y, ziji_05_61x80, sizeof(ziji_05_61x80));
			break;
		case 6:
			TJpgDec.drawJpg(x, y, ziji_06_61x80, sizeof(ziji_06_61x80));
			break;
		case 7:
			TJpgDec.drawJpg(x, y, ziji_07_61x80, sizeof(ziji_07_61x80));
			break;
		case 8:
			TJpgDec.drawJpg(x, y, ziji_08_61x80, sizeof(ziji_08_61x80));
			break;
		case 9:
			TJpgDec.drawJpg(x, y, ziji_09_61x80, sizeof(ziji_09_61x80));
			break;
		case 10:
			TJpgDec.drawJpg(x, y, ziji_10_61x80, sizeof(ziji_10_61x80));
			break;
		case 11:
			TJpgDec.drawJpg(x, y, ziji_11_61x80, sizeof(ziji_11_61x80));
			break;
		case 12:
			TJpgDec.drawJpg(x, y, ziji_12_61x80, sizeof(ziji_12_61x80));
			break;
		case 13:
			TJpgDec.drawJpg(x, y, ziji_13_61x80, sizeof(ziji_13_61x80));
			break;
		case 14:
			TJpgDec.drawJpg(x, y, ziji_14_61x80, sizeof(ziji_14_61x80));
			break;
		case 15:
			TJpgDec.drawJpg(x, y, ziji_15_61x80, sizeof(ziji_15_61x80));
			break;
		case 16:
			TJpgDec.drawJpg(x, y, ziji_16_61x80, sizeof(ziji_16_61x80));
			break;
		case 17:
			TJpgDec.drawJpg(x, y, ziji_17_61x80, sizeof(ziji_17_61x80));
			break;
		case 18:
			TJpgDec.drawJpg(x, y, ziji_18_61x80, sizeof(ziji_18_61x80));
			break;
		case 19:
			TJpgDec.drawJpg(x, y, ziji_19_61x80, sizeof(ziji_19_61x80));
			break;
		case 20:
			TJpgDec.drawJpg(x, y, ziji_20_61x80, sizeof(ziji_20_61x80));
			break;
		case 21:
			TJpgDec.drawJpg(x, y, ziji_21_61x80, sizeof(ziji_21_61x80));
			break;
		case 22:
			TJpgDec.drawJpg(x, y, ziji_22_61x80, sizeof(ziji_22_61x80));
			break;
		case 23:
			TJpgDec.drawJpg(x, y, ziji_23_61x80, sizeof(ziji_23_61x80));
			break;
		case 24:
			TJpgDec.drawJpg(x, y, ziji_24_61x80, sizeof(ziji_24_61x80));
			break;
		case 25:
			TJpgDec.drawJpg(x, y, ziji_25_61x80, sizeof(ziji_25_61x80));
			break;
		case 26:
			TJpgDec.drawJpg(x, y, ziji_26_61x80, sizeof(ziji_26_61x80));
			break;
		case 27:
			TJpgDec.drawJpg(x, y, ziji_27_61x80, sizeof(ziji_27_61x80));
			break;
		case 28:
			TJpgDec.drawJpg(x, y, ziji_28_61x80, sizeof(ziji_28_61x80));
			break;
		case 29:
			TJpgDec.drawJpg(x, y, ziji_29_61x80, sizeof(ziji_29_61x80));
			break;
		case 30:
			TJpgDec.drawJpg(x, y, ziji_30_61x80, sizeof(ziji_30_61x80));
			break;
		case 31:
			TJpgDec.drawJpg(x, y, ziji_31_61x80, sizeof(ziji_31_61x80));
			break;
		case 32:
			TJpgDec.drawJpg(x, y, ziji_32_61x80, sizeof(ziji_32_61x80));
			break;
		case 33:
			TJpgDec.drawJpg(x, y, ziji_33_61x80, sizeof(ziji_33_61x80));
			break;
		case 34:
			TJpgDec.drawJpg(x, y, ziji_34_61x80, sizeof(ziji_34_61x80));
			break;
		case 35:
			TJpgDec.drawJpg(x, y, ziji_35_61x80, sizeof(ziji_35_61x80));
			break;
		case 36:
			TJpgDec.drawJpg(x, y, ziji_36_61x80, sizeof(ziji_36_61x80));
			imgNum = 0;
			break;
			// case 37:TJpgDec.drawJpg(x,y,ziji_37_61x80, sizeof(ziji_37_61x80));imgNum = 0;break;
		}
	}
	else if (Gif_Mode == 4)
	{ // 动画-龙猫睡觉
		y = 85;
		switch (imgNum)
		{
		case 1:
			TJpgDec.drawJpg(x, y, zzzzzzz_01_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 2:
			TJpgDec.drawJpg(x, y, zzzzzzz_02_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 3:
			TJpgDec.drawJpg(x, y, zzzzzzz_03_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 4:
			TJpgDec.drawJpg(x, y, zzzzzzz_04_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 5:
			TJpgDec.drawJpg(x, y, zzzzzzz_05_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 6:
			TJpgDec.drawJpg(x, y, zzzzzzz_06_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 7:
			TJpgDec.drawJpg(x, y, zzzzzzz_07_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 8:
			TJpgDec.drawJpg(x, y, zzzzzzz_08_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 9:
			TJpgDec.drawJpg(x, y, zzzzzzz_09_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 10:
			TJpgDec.drawJpg(x, y, zzzzzzz_10_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 11:
			TJpgDec.drawJpg(x, y, zzzzzzz_11_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 12:
			TJpgDec.drawJpg(x, y, zzzzzzz_12_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 13:
			TJpgDec.drawJpg(x, y, zzzzzzz_13_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 14:
			TJpgDec.drawJpg(x, y, zzzzzzz_14_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 15:
			TJpgDec.drawJpg(x, y, zzzzzzz_15_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 16:
			TJpgDec.drawJpg(x, y, zzzzzzz_16_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 17:
			TJpgDec.drawJpg(x, y, zzzzzzz_17_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 18:
			TJpgDec.drawJpg(x, y, zzzzzzz_18_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 19:
			TJpgDec.drawJpg(x, y, zzzzzzz_19_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 20:
			TJpgDec.drawJpg(x, y, zzzzzzz_20_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 21:
			TJpgDec.drawJpg(x, y, zzzzzzz_21_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 22:
			TJpgDec.drawJpg(x, y, zzzzzzz_22_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 23:
			TJpgDec.drawJpg(x, y, zzzzzzz_23_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 24:
			TJpgDec.drawJpg(x, y, zzzzzzz_24_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 25:
			TJpgDec.drawJpg(x, y, zzzzzzz_25_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 26:
			TJpgDec.drawJpg(x, y, zzzzzzz_26_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 27:
			TJpgDec.drawJpg(x, y, zzzzzzz_27_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 28:
			TJpgDec.drawJpg(x, y, zzzzzzz_28_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 29:
			TJpgDec.drawJpg(x, y, zzzzzzz_29_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 30:
			TJpgDec.drawJpg(x, y, zzzzzzz_30_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 31:
			TJpgDec.drawJpg(x, y, zzzzzzz_31_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 33:
			TJpgDec.drawJpg(x, y, zzzzzzz_32_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 34:
			TJpgDec.drawJpg(x, y, zzzzzzz_33_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 35:
			TJpgDec.drawJpg(x, y, zzzzzzz_34_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 36:
			TJpgDec.drawJpg(x, y, zzzzzzz_35_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 37:
			TJpgDec.drawJpg(x, y, zzzzzzz_36_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 38:
			TJpgDec.drawJpg(x, y, zzzzzzz_37_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 39:
			TJpgDec.drawJpg(x, y, zzzzzzz_38_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 40:
			TJpgDec.drawJpg(x, y, zzzzzzz_39_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 41:
			TJpgDec.drawJpg(x, y, zzzzzzz_40_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 42:
			TJpgDec.drawJpg(x, y, zzzzzzz_41_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 43:
			TJpgDec.drawJpg(x, y, zzzzzzz_42_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 44:
			TJpgDec.drawJpg(x, y, zzzzzzz_43_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 45:
			TJpgDec.drawJpg(x, y, zzzzzzz_45_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 46:
			TJpgDec.drawJpg(x, y, zzzzzzz_46_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 47:
			TJpgDec.drawJpg(x, y, zzzzzzz_47_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 48:
			TJpgDec.drawJpg(x, y, zzzzzzz_48_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 49:
			TJpgDec.drawJpg(x, y, zzzzzzz_49_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 50:
			TJpgDec.drawJpg(x, y, zzzzzzz_50_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 51:
			TJpgDec.drawJpg(x, y, zzzzzzz_51_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 52:
			TJpgDec.drawJpg(x, y, zzzzzzz_52_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 53:
			TJpgDec.drawJpg(x, y, zzzzzzz_53_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 54:
			TJpgDec.drawJpg(x, y, zzzzzzz_54_71x80, sizeof(zzzzzzz_01_71x80));
			break;
		case 55:
			TJpgDec.drawJpg(x, y, zzzzzzz_55_71x80, sizeof(zzzzzzz_55_71x80));
			imgNum = 0;
			break;
			// case 56: TJpgDec.drawJpg(x,y,zzzzzzz_56_71x80, sizeof(zzzzzzz_56_71x80));imgNum=0;break;
		}
	}
}

// 新增底部动画，可以自己调整内容

unsigned long oldTimenew = 0, imgNumnew = 1;
void imgDisplaynew()
{
	int xnew = 10, ynew = 242, dtnew = 50;
	if (millis() - oldTimenew >= dtnew)
	{
		imgNumnew = imgNumnew + 1;
		oldTimenew = millis();
	}
	if (dbgif == 1)
	{
		switch (imgNumnew)
		{
		case 1:
			TJpgDec.drawJpg(xnew, ynew, ziji_01_61x80, sizeof(ziji_01_61x80));
			break;
		case 2:
			TJpgDec.drawJpg(xnew, ynew, ziji_02_61x80, sizeof(ziji_02_61x80));
			break;
		case 3:
			TJpgDec.drawJpg(xnew, ynew, ziji_03_61x80, sizeof(ziji_03_61x80));
			break;
		case 4:
			TJpgDec.drawJpg(xnew, ynew, ziji_04_61x80, sizeof(ziji_04_61x80));
			break;
		case 5:
			TJpgDec.drawJpg(xnew, ynew, ziji_05_61x80, sizeof(ziji_05_61x80));
			break;
		case 6:
			TJpgDec.drawJpg(xnew, ynew, ziji_06_61x80, sizeof(ziji_06_61x80));
			break;
		case 7:
			TJpgDec.drawJpg(xnew, ynew, ziji_07_61x80, sizeof(ziji_07_61x80));
			break;
		case 8:
			TJpgDec.drawJpg(xnew, ynew, ziji_08_61x80, sizeof(ziji_08_61x80));
			break;
		case 9:
			TJpgDec.drawJpg(xnew, ynew, ziji_09_61x80, sizeof(ziji_09_61x80));
			break;
		case 10:
			TJpgDec.drawJpg(xnew, ynew, ziji_10_61x80, sizeof(ziji_10_61x80));
			break;
		case 11:
			TJpgDec.drawJpg(xnew, ynew, ziji_11_61x80, sizeof(ziji_11_61x80));
			break;
		case 12:
			TJpgDec.drawJpg(xnew, ynew, ziji_12_61x80, sizeof(ziji_12_61x80));
			break;
		case 13:
			TJpgDec.drawJpg(xnew, ynew, ziji_13_61x80, sizeof(ziji_13_61x80));
			break;
		case 14:
			TJpgDec.drawJpg(xnew, ynew, ziji_14_61x80, sizeof(ziji_14_61x80));
			break;
		case 15:
			TJpgDec.drawJpg(xnew, ynew, ziji_15_61x80, sizeof(ziji_15_61x80));
			break;
		case 16:
			TJpgDec.drawJpg(xnew, ynew, ziji_16_61x80, sizeof(ziji_16_61x80));
			break;
		case 17:
			TJpgDec.drawJpg(xnew, ynew, ziji_17_61x80, sizeof(ziji_17_61x80));
			break;
		case 18:
			TJpgDec.drawJpg(xnew, ynew, ziji_18_61x80, sizeof(ziji_18_61x80));
			break;
		case 19:
			TJpgDec.drawJpg(xnew, ynew, ziji_19_61x80, sizeof(ziji_19_61x80));
			break;
		case 20:
			TJpgDec.drawJpg(xnew, ynew, ziji_20_61x80, sizeof(ziji_20_61x80));
			break;
		case 21:
			TJpgDec.drawJpg(xnew, ynew, ziji_21_61x80, sizeof(ziji_21_61x80));
			break;
		case 22:
			TJpgDec.drawJpg(xnew, ynew, ziji_22_61x80, sizeof(ziji_22_61x80));
			break;
		case 23:
			TJpgDec.drawJpg(xnew, ynew, ziji_23_61x80, sizeof(ziji_23_61x80));
			break;
		case 24:
			TJpgDec.drawJpg(xnew, ynew, ziji_24_61x80, sizeof(ziji_24_61x80));
			break;
		case 25:
			TJpgDec.drawJpg(xnew, ynew, ziji_25_61x80, sizeof(ziji_25_61x80));
			break;
		case 26:
			TJpgDec.drawJpg(xnew, ynew, ziji_26_61x80, sizeof(ziji_26_61x80));
			break;
		case 27:
			TJpgDec.drawJpg(xnew, ynew, ziji_27_61x80, sizeof(ziji_27_61x80));
			break;
		case 28:
			TJpgDec.drawJpg(xnew, ynew, ziji_28_61x80, sizeof(ziji_28_61x80));
			break;
		case 29:
			TJpgDec.drawJpg(xnew, ynew, ziji_29_61x80, sizeof(ziji_29_61x80));
			break;
		case 30:
			TJpgDec.drawJpg(xnew, ynew, ziji_30_61x80, sizeof(ziji_30_61x80));
			break;
		case 31:
			TJpgDec.drawJpg(xnew, ynew, ziji_31_61x80, sizeof(ziji_31_61x80));
			break;
		case 32:
			TJpgDec.drawJpg(xnew, ynew, ziji_32_61x80, sizeof(ziji_32_61x80));
			break;
		case 33:
			TJpgDec.drawJpg(xnew, ynew, ziji_33_61x80, sizeof(ziji_33_61x80));
			break;
		case 34:
			TJpgDec.drawJpg(xnew, ynew, ziji_34_61x80, sizeof(ziji_34_61x80));
			break;
		case 35:
			TJpgDec.drawJpg(xnew, ynew, ziji_35_61x80, sizeof(ziji_35_61x80));
			break;
		case 36:
			TJpgDec.drawJpg(xnew, ynew, ziji_36_61x80, sizeof(ziji_36_61x80));
			imgNumnew = 0;
			break;
		}
	}
}

//------------------------------------------------------------------------------------------------

void digitalClockDisplay()
{

	clk.setColorDepth(8);

	/***中间时间区***/
	// 时分
	clk.createSprite(140, 48);
	clk.fillSprite(bgColor);
	// clk.loadFont(FxLED_48);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_BLACK, bgColor);
	clk.drawString(hourMinute(), 70, 24, 7); // 绘制时和分
	// clk.unloadFont();
	clk.pushSprite(28, 40);
	clk.deleteSprite();

	// 秒
	clk.createSprite(40, 28);
	clk.fillSprite(bgColor);

	clk.loadFont(FxLED_32);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_BLACK, bgColor);
	clk.drawString(num2str(second()), 20, 12);

	clk.unloadFont();
	clk.pushSprite(170, 55);
	clk.deleteSprite();
	/***中间时间区***/

	/***底部***/
	clk.loadFont(ZdyLwFont_20);
	clk.createSprite(58, 32);
	clk.fillSprite(bgColor);

	// 星期
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_BLACK, bgColor);
	clk.drawString(week(), 29, 16);
	clk.pushSprite(1, 168);
	clk.deleteSprite();

	// 月日
	clk.createSprite(98, 32);
	clk.fillSprite(bgColor);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_BLACK, bgColor);
	clk.drawString(monthDay(), 49, 16);
	clk.pushSprite(61, 168);
	clk.deleteSprite();

	clk.unloadFont();
	/***底部***/
}

// 星期
String week()
{
	String wk[7] = {"日", "一", "二", "三", "四", "五", "六"};
	String s = "周" + wk[weekday() - 1];
	return s;
}

// 月日
String monthDay()
{
	String s = String(month());
	s = s + "月" + day() + "日";
	return s;
}
// 时分
String hourMinute()
{
	String s = num2str(hour());
	backLight_hour = s.toInt();
	s = s + ":" + num2str(minute());
	return s;
}

String num2str(int digits)
{
	String s = "";
	if (digits < 10)
		s = s + "0";
	s = s + digits;
	return s;
}

void printDigits(int digits)
{
	Serial.print(":");
	if (digits < 10)
		Serial.print('0');
	Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48;		// NTP时间在消息的前48字节中
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
	IPAddress ntpServerIP; // NTP server's ip address

	while (Udp.parsePacket() > 0)
		; // discard any previously received packets
	// Serial.println("Transmit NTP Request");
	//  get a random server from the pool
	WiFi.hostByName(ntpServerName, ntpServerIP);
	// Serial.print(ntpServerName);
	// Serial.print(": ");
	// Serial.println(ntpServerIP);
	sendNTPpacket(ntpServerIP);
	uint32_t beginWait = millis();
	while (millis() - beginWait < 1500)
	{
		int size = Udp.parsePacket();
		if (size >= NTP_PACKET_SIZE)
		{
			Serial.println("可以呀，小伙子，NTP同步成功啦！！！");
			Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
			unsigned long secsSince1900;
			// convert four bytes starting at location 40 to a long integer
			secsSince1900 = (unsigned long)packetBuffer[40] << 24;
			secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
			secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
			secsSince1900 |= (unsigned long)packetBuffer[43];
			// Serial.println(secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
			return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
		}
	}
	// ESP.restart(); //时间获取失败直接重启
	Serial.println("NTP同步失败，别气馁，下次会成功的...");
	return 0; // 无法获取时间时返回0
}

// 向NTP服务器发送请求
void sendNTPpacket(IPAddress &address)
{
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011; // LI, Version, Mode
	packetBuffer[1] = 0;		  // Stratum, or type of clock
	packetBuffer[2] = 6;		  // Polling Interval
	packetBuffer[3] = 0xEC;		  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12] = 49;
	packetBuffer[13] = 0x4E;
	packetBuffer[14] = 49;
	packetBuffer[15] = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	Udp.beginPacket(address, 123); // NTP requests are to port 123
	Udp.write(packetBuffer, NTP_PACKET_SIZE);
	Udp.endPacket();
}
void setupPins()
{
	pinMode(enterbtn, INPUT);
	pinMode(backbtn, INPUT);
	pinMode(upbtn, INPUT);
	pinMode(downbtn, INPUT);
	pinMode(homebtn, INPUT);
	pinMode(fmqsw, INPUT);
	pinMode(SDOIN, INPUT);
	pinMode(DATAIN, INPUT);

	pinMode(PIN_BUZZER, OUTPUT);
	digitalWrite(PIN_BUZZER, LOW);
}
void setup()
{
	setupPins(); // 初始化新增的按键及引脚

	if (Boards == 1)
	{				 // ST7789
		GL5528 = 32; // 光敏电阻引脚
		Button = 4;	 // 按键引脚
	}
	else if (Boards == 2)
	{				 // ILI 9488
		GL5528 = 35; // 光敏电阻引脚
		Button = 18; // 按键引脚
	}

	EEPROM.begin(8);
	guanggao_Flag = EEPROM.read(0);
	tft.init();
	// 设置屏幕显示的旋转角度，参数为：0, 1, 2, 3
	// 分别代表 0°、90°、180°、270°
	tft.setRotation(0);

	Serial.begin(SerialBaud);
	pinMode(Button, INPUT); // 配网按钮接GPIO-4
	pinMode(GL5528, INPUT); // 光敏电阻
	randomSeed(analogRead(GL5528));
	ledcSetup(0, 5000, 8);
	ledcAttachPin(22, 0);
	ledcWrite(0, 150);

	// 首次使用自动进入配网模式,读取NVS存储空间内的ssid、password和citycode
	preferences.begin("wifi", false);
	PrefSSID = preferences.getString("ssid", "none");
	PrefPassword = preferences.getString("password", "none");
	cityCode = preferences.getString("citycode", "none");
	preferences.end();
	if (PrefSSID == "none")
	{
		// smartConfigWIFI();
		setWiFi();
	}

	int buttonStateTime = 0;

	while (digitalRead(Button) == HIGH)
	{
		buttonStateTime = millis();
		clk.loadFont(ZdyLwFont_20);
		clk.createSprite(240, 80);
		clk.setTextDatum(CC_DATUM);
		clk.setTextColor(TFT_WHITE, bgColor);
		if (buttonStateTime >= 15500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " ON/OFF广告", 120, 40);
		}
		else if (buttonStateTime >= 12500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " 配网模式", 120, 40);
		}
		else if (buttonStateTime >= 9500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " 动画:睡觉", 120, 40);
		}
		else if (buttonStateTime >= 6500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " 动画:太空人", 120, 40);
		}
		else if (buttonStateTime >= 3500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " 动画:耍杂技", 120, 40);
		}
		else if (buttonStateTime >= 500)
		{
			clk.drawString("" + String(millis() / 1000) + "秒" + " 动画:转圈", 120, 40);
		}
		clk.pushSprite(0, 80);
		clk.deleteSprite();
		clk.unloadFont(); // 释放加载字体资源
	}

	if (buttonStateTime >= 15500)
	{ // 打开/关闭启动广告
		guanggao_Flag = !guanggao_Flag;
		EEPROM.write(0, guanggao_Flag);
		EEPROM.commit();
		// Serial.println(EEPROM.read(0));
		clk.loadFont(ZdyLwFont_20);
		clk.createSprite(240, 80);
		clk.setTextDatum(CC_DATUM);
		clk.setTextColor(TFT_WHITE, bgColor);
		if (guanggao_Flag)
		{
			clk.drawString("启动广告已打开", 120, 40);
		}
		else
		{
			clk.drawString("启动广告已关闭", 120, 40);
		}
		clk.pushSprite(0, 80);
		clk.deleteSprite();
		clk.unloadFont(); // 释放加载字体资源
		delay(3000);
	}
	else if (buttonStateTime >= 12500)
	{ // 配网模式
		// smartConfigWIFI();
		setWiFi();
	}
	else if (buttonStateTime >= 9500)
	{ // 动画-龙猫打鼓
		EEPROM.write(1, 4);
		EEPROM.commit();
	}
	else if (buttonStateTime >= 6500)
	{ // 动画-太空人
		EEPROM.write(1, 3);
		EEPROM.commit();
	}
	else if (buttonStateTime >= 3500)
	{ // 动画-龙猫耍杂技
		EEPROM.write(1, 2);
		EEPROM.commit();
	}
	else if (buttonStateTime >= 500)
	{ // 动画-龙猫转圈
		EEPROM.write(1, 1);
		EEPROM.commit();
	}

	Gif_Mode = EEPROM.read(1);
	// Gif_Mode = 4;

	// 广告页
	guanggao_Flag = false;
	if (guanggao_Flag == true)
	{
		TJpgDec.setJpgScale(1);
		TJpgDec.setSwapBytes(true);
		TJpgDec.setCallback(tft_output);
		TJpgDec.drawJpg(0, 0, guanggao, sizeof(guanggao));
		clk.loadFont(ZdyLwFont_20);
		clk.createSprite(40, 30);
		for (int i = 10; i >= 1; i--)
		{
			clk.fillSprite(bgColor);
			clk.setTextDatum(CC_DATUM);
			clk.setTextColor(TFT_BLACK, bgColor);
			clk.drawString(String(i) + "秒", 20, 15);
			clk.pushSprite(200, 210);
			delay(1000);
		}
		clk.deleteSprite();
		clk.unloadFont(); // 释放加载字体资源
	}

	tft.fillScreen(0x0000);
	delay(100);
	tft.setTextColor(TFT_BLACK, bgColor);

	targetTime = millis() + 1000;

	Serial.println("正在连接" + PrefSSID + " ...");
	WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());
	// WiFi.begin("CKTN", "18900744765");
	while (WiFi.status() != WL_CONNECTED)
	{
		for (byte n = 0; n < 10; n++)
		{
			loading(100, 1);
			connectTimes++;
			if (connectTimes >= 190)
			{ // 进度条即将结束时还未连接成功，则提示wifi连接失败，自动进入配网模式
				connectTimes = 0;
				displayConnectWifiFalse();
				// smartConfigWIFI();
				setWiFi();
			}
		}
	}
	while (loadNum < 194 & connectTimes <= 189)
	{ // 让动画走完
		loading(0, 5);
		connectTimes = 0;
	}

	Serial.print("本地IP： ");
	Serial.println(WiFi.localIP());
	// Serial.println("启动UDP");
	Udp.begin(localPort);
	// Serial.print("端口号: ");
	// Serial.println(Udp.localPort());
	// Serial.println("等待同步...");
	setSyncProvider(getNtpTime);
	setSyncInterval(setNTPSyncTime * 60); // NTP网络同步频率，单位秒。

	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);
	int x = 0, y = 0, dt = 50, xyz = 1; // x\y=图片显示坐标，dt=单帧切换时间，xyz=gif整体播放的次数
	while (imgNum_1 <= 19 & xyz >= 0)
	{
		if (millis() - oldTime_1 >= dt)
		{
			imgNum_1 = imgNum_1 + 1;
			oldTime_1 = millis();
		}
		// wifi连接成功后的动画显示
		/*switch(imgNum_1) {
		  case 1: TJpgDec.drawJpg(x,y,start_0, sizeof(start_0));break;
		  case 2: TJpgDec.drawJpg(x,y,start_1, sizeof(start_1));break;
		  case 3: TJpgDec.drawJpg(x,y,start_2, sizeof(start_2));break;
		  case 4: TJpgDec.drawJpg(x,y,start_3, sizeof(start_3));break;
		  case 5: TJpgDec.drawJpg(x,y,start_4, sizeof(start_4));break;
		  case 6: TJpgDec.drawJpg(x,y,start_5, sizeof(start_5));break;
		  case 7: TJpgDec.drawJpg(x,y,start_6, sizeof(start_6));break;
		  case 8: TJpgDec.drawJpg(x,y,start_7, sizeof(start_7));break;
		  case 9: TJpgDec.drawJpg(x,y,start_8, sizeof(start_8));break;
		  case 10: TJpgDec.drawJpg(x,y,start_9, sizeof(start_9));break;
		  case 11: TJpgDec.drawJpg(x,y,start_10, sizeof(start_10));break;
		  case 12: TJpgDec.drawJpg(x,y,start_11, sizeof(start_11));break;
		  case 13: TJpgDec.drawJpg(x,y,start_12, sizeof(start_12));break;
		  case 14: TJpgDec.drawJpg(x,y,start_13, sizeof(start_13));break;
		  case 15: TJpgDec.drawJpg(x,y,start_14, sizeof(start_14));break;
		  case 16: TJpgDec.drawJpg(x,y,start_15, sizeof(start_15));break;
		  case 17: TJpgDec.drawJpg(x,y,start_16, sizeof(start_16));break;
		  case 18: TJpgDec.drawJpg(x,y,start_17, sizeof(start_17));break;
		  case 19: TJpgDec.drawJpg(x,y,start_18, sizeof(start_18));imgNum_1 = 1;xyz--;break;
		} */
	}
	delay(200);
	// Serial.println("赶紧扫码加QQ群，或手动搜索QQ群号：531732157");
	// TJpgDec.drawJpg(x, y, start_end, sizeof(start_end)); // 显示QQ扫码加群二维码
	// delay(3000);

	// TJpgDec.drawJpg(0,0,watchtop, sizeof(watchtop)); //顶部图片显示 240*20
	// TJpgDec.drawJpg(0,220,watchbottom, sizeof(watchbottom)); //底部图片显示 240*20

	// 绘制一个视口
	// tft.setViewport(0, 20, 240, 240);
	tft.fillScreen(0x0000);
	tft.fillRoundRect(0, 0, 240, 320, 0, bgColor); // 实心矩形
	// tft.resetViewport();

	// 绘制线框
	tft.drawFastHLine(0, 0, 240, TFT_BLACK);
	// tft.drawFastHLine(0,220,240,TFT_BLACK);

	tft.drawFastHLine(0, 34, 240, TFT_BLACK);
	tft.drawFastHLine(0, 200, 240, TFT_BLACK);

	tft.drawFastVLine(150, 0, 34, TFT_BLACK);

	tft.drawFastHLine(0, 166, 240, TFT_BLACK);

	tft.drawFastVLine(60, 166, 34, TFT_BLACK);
	tft.drawFastVLine(160, 166, 34, TFT_BLACK);

	tft.drawFastHLine(0, 241, 240, TFT_BLACK);
	tft.drawFastVLine(80, 241, 80, TFT_BLACK);
	//  获取b站粉丝数量
	get_Bstation_follow();

	// 粉丝数量展示界面

	clk.createSprite(120, 34);
	clk.fillSprite(0XFFFF);
	clk.loadFont(ZdyLwFont_20);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_PURPLE, 0XFFFF);
	clk.drawString("关注数:" + String(following), 60, 17);
	clk.pushSprite(82, 245);
	clk.deleteSprite();
	clk.unloadFont(); // 释放加载字体资源

	clk.createSprite(120, 34);
	clk.fillSprite(0XFFFF);
	clk.loadFont(ZdyLwFont_20);
	clk.setTextDatum(CC_DATUM);
	clk.setTextColor(TFT_PURPLE, 0XFFFF);
	clk.drawString("粉丝数:" + String(follower), 60, 17);
	clk.pushSprite(82, 280);
	clk.deleteSprite();
	clk.unloadFont(); // 释放加载字体资源

	if (cityCode.length() >= 8)
	{
		// Serial.println("手动设置cityCode");
		getCityWeater(); // 获取天气数据
	}
	else
	{
		// Serial.println("自动设置cityCode");
		getCityCode(); // 获取城市代码
	}
	// getLunarCalendar();
}
void loop()
{

	fmqswRun();

	if (digitalRead(homebtn) == HIGH)
	{
		delay(500);
		homepage(); // 主页功能
	}

	t1 = millis();

	switch (AutoBright)
	{ // 屏幕背光控制
	case true:
		Filter_Value = Filter();
		ledcWrite(0, map(Filter_Value, 0, 4095, 0, 255));
		break;
	case false:
		ledcWrite(0, 150);
		break;
	}
	// Filter_Value = Filter();
	// ledcWrite(0,map(Filter_Value,0,4095,0,255));
	// ledcWrite(0,150);

	if (digitalRead(Button) == HIGH)
	{
		delay(500);
		ESP.restart(); // 重启ESP32
	}

	if (now() != prevDisplay)
	{
		prevDisplay = now();
		digitalClockDisplay();
	}

	// 更新时，网络环境差的情况下，屏幕会有短暂停止刷新过程，网络环境好，该过程不明显，很难看出差别
	if ((millis() - weaterTime) > (setWeatherTime * 60000))
	{ // 30分钟更新一次天气
		getCityWeaterFlag = false;
		getCityCodeFlag = false;
		weaterTime = millis();
		getCityWeater();
	}
	scrollBanner();
	ButtonscrollBanner();
	imgDisplay();
	imgDisplaynew();
	weatherWarning();

	// wifi断开重启重连
	if (millis() - wifiTimes >= 60000)
	{
		wifiTimes = millis();
		if (WiFi.status() != WL_CONNECTED)
		{
			Serial.println("哦豁，断网咯，正在为你重启...");
			ESP.restart();
		}
	}

	// 串口打印一个循环所运行的时间，该数值越小越好
	t2 = millis();
	// Serial.println("RunTimed:" + String(t2-t1) + "ms");
	t1 = 0;
	t2 = 0;
}
