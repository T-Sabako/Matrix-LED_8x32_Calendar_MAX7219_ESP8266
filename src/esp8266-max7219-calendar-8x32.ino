#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "GF4x8p.h"  // 4x8 font+week(漢字)
#include "SF3x6p.h"  // 3x5 font

const char *Day[] = {"\xc5", "\xbf", "\xc0", "\xc1", "\xc2", "\xc3", "\xc4"};
int dayOfWeek;

//Wi-Fi
#define ssid "hogeid"
#define password "hogepw"
String newHostname = "Calendar8x32LED";

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;

//NTP client
int timezone = 9;
char ntp_server1[20] = "ntp.nict.jp";
char ntp_server2[20] = "ntp.jst.mfeed.ad.jp";
char ntp_server3[20] = "time.aws.com";
int dst = 0;

//MAX7219 Setting
#define MAX_DEVICES 4 // eight modules
#define CLK_PIN   14
#define DATA_PIN  13
#define CS_PIN    15

#define SPEED_TIME  25  //Small numbers are faster. Zero is the fastest.
#define PAUSE_TIME_LL 56000 //56s for month/day long pause
#define PAUSE_TIME_L  3000 //3s for month/day long pause
#define PAUSE_TIME_M  2000 //2s for month/day long pause
#define PAUSE_TIME_S  1000 //1s for month/day short pause

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

char bufY[5], bufD[9], bufW[4], bufT[6], bufS[3];

// Hardware SPI connection
//MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void initWiFi() {
  WiFi.hostname(newHostname.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi sucessfully.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi, trying to connect...");
  WiFi.disconnect();
  WiFi.hostname(newHostname.c_str());
  WiFi.begin(ssid, password);
}

void updateTime() {
  time_t now = time(nullptr);
  struct tm* newtime = localtime(&now);
  
  dayOfWeek = String(newtime->tm_wday).toInt();

  strftime(bufT, sizeof(bufT), "%R", newtime); 
  strftime(bufY, sizeof(bufY), "%Y", newtime);
  strftime(bufD, sizeof(bufD), "%m/%d", newtime);
  //strftime(bufW, sizeof(bufW), "%a", newtime);
  snprintf(bufW, sizeof(bufW), "%s", Day[dayOfWeek]);
  strftime(bufS, sizeof(bufS), "%S", newtime);

}

void setup() {
  Serial.begin(115200);

  //Register event handlers
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);
   
  initWiFi();
  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI());
  Serial.printf("hostname: %s\n", WiFi.hostname().c_str());

  configTime(timezone * 3600, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("Waiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(500);
  }

  P.begin(2); // 4 zones
  P.setZone(0, 1, 3);  //1110 <- Zone 0  for Date display
  P.setZone(1, 0, 0);  //0001 <- Zone 1  for WeekDay display

  P.setFont(0,GF4x8p);
  P.setFont(1,GF4x8p);

  P.setIntensity(0);  //LED明るさ(0-15)

  updateTime();

}

void loop() {
  static uint8_t  dsw = 0;

  P.displayAnimate();
  updateTime();

  //Zone 1 Day of the Week 
  if (P.getZoneStatus(1)) {
    P.displayZoneText(1, bufW, PA_CENTER, SPEED_TIME, 0, PA_PRINT, PA_NO_EFFECT);
    P.displayReset(1);
  }

  //Zone 0 Date
  if (P.getZoneStatus(0)) {
    //P.displayZoneText(2, bufD, PA_CENTER, SPEED_TIME, 0, PA_PRINT, PA_NO_EFFECT);
    //P.displayReset(2);
    switch (dsw) {
      case 0: // Time
        P.displayZoneText(0, bufT, PA_CENTER, SPEED_TIME, PAUSE_TIME_L, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
        dsw++;
        break;
      case 1: // month/day
        P.displayZoneText(0, bufD, PA_CENTER, SPEED_TIME, PAUSE_TIME_LL, PA_SCROLL_DOWN, PA_SCROLL_DOWN);
        dsw = 0;
        break;
      default:
        break;
    }
    P.displayReset(0);
  }

  delay(50); //Don't remove this delay, and don't make it too small
}