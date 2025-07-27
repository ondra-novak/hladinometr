#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <SoftwareSerial.h>
#include <U8g2lib.h>
#include <string>
#include "web.hpp"
#include "utils.hpp"



U8G2_ST7567_OS12864_F_4W_SW_SPI display(U8G2_R2,D3,D5,D1,D2,D0);
ESP8266WebServer server(80); // Port 80 = HTTP
DNSServer dns;
String  origin_ssid;
unsigned long next_tick = 0;
unsigned int error_ticks = 0;
bool ap_mode = false;
CidloRead cidlo(D6, D7);
TrendCalc<1024> trendCalc;
Configuration conf = {};
constexpr auto apName = std::string_view ("Hladinomer_AP");


int getLevel() {
    return conf.zero_point - cidlo.get_value();
}

int getTrend() {    
    float t1 = trendCalc.slope(conf.trend_secs);    
    return static_cast<int>(t1 * 3600); 
}

void handleRoot() {

    server.send(200, "text/html",WebPage.data(), WebPage.size());
}

void handleData() {
  server.send(200, "application/json", "{\"level\": " + String(getLevel()) + ", \"trend\": " + String(getTrend())+"}");
}

void handleScanWifi() {
    String result = "[";
    
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      String ssidFound = WiFi.SSID(i);
      int rssi =  WiFi.RSSI(i);
      if (ssidFound.indexOf('"') != -1) continue;
      if (result.length()>2) result+=",";
      result += "{\"SSID\": \""+ssidFound+"\", \"RSSI\": "+String(rssi)+"}";    
    }
    result +="]";
    WiFi.scanDelete(); // nezapomenout
    server.send(200, "application/json", result);
}

void handleCaptivePortalRequest() {
    server.sendHeader("Location", "http://"+WiFi.softAPIP().toString()+"/", true);
  server.send(302, "text/plain", "");
}

void handleSaveWifi() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    if (ssid.length() > 0) {
        server.send(200, "text/html;charset=utf-8", "<html><body><h2>Nastavení uloženo. Zařízení se restartuje...</h2></body></html>");
        auto stop = millis()+1000;
        while (millis()<stop) server.handleClient();

        WiFi.persistent(true);           // Uložení do flash
        WiFi.mode(WIFI_STA);             // Přepnutí do STA
        WiFi.begin(ssid.c_str(), pass.c_str());  

        delay(1000);                      // Nechme odeslat odpověď
        ESP.restart();                    // Restart zařízení
    } else {
        server.send(400, "text/html;charset=utf-8", "<html><body><h2>SSID chybí</h2></body></html>");
    }
}

void handleConfigGet() {
    server.send(200,"application/json","{\"zeropt\":"+String(conf.zero_point)+",\"trend_sec\":"+String(conf.trend_secs)+"}");
}
void handleConfigSave() {
    String zeropt = server.arg("zeropt");
    String trendsec = server.arg("trend_sec");
    if (zeropt.length() > 0 && trendsec.length() > 0) {
        int zpt = zeropt.toInt();
        int tsec = trendsec.toInt();
        if (zpt > 0 && tsec > 0) {
            conf.zero_point = zpt;
            conf.trend_secs = tsec;
            trendCalc.reset();
            server.send(202,"text/plain","Accepted");
            return;
        }
    } 
    server.send(400,"text/plain","Bad request");
}



void setup()
{
  Serial.begin(115200);
  cidlo.begin();
  display.begin();


 WiFi.mode(WIFI_STA);
//  WiFi.persistent(true);
  WiFi.begin();

  origin_ssid =  WiFi.SSID();
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/scan", handleScanWifi);
  server.on("/generate_204", handleCaptivePortalRequest);
  server.on("/hotspot-detect.html", handleCaptivePortalRequest);
  server.on("/savewifi",HTTP_POST, handleSaveWifi);
  server.on("/config", HTTP_GET, handleConfigGet);
  server.on("/config", HTTP_POST, handleConfigSave);
  server.begin();  
  next_tick = millis()+3000;
}

void startCaptivePortal() {
  ap_mode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(std::string(apName).c_str());
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP adresa: ");
  Serial.println(IP);
  dns.start(53, "*", WiFi.softAPIP()); // DNS přesměrování všech dotazů
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
}

void startSTA() {  
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  Serial.println("Přepínám zpět do STA režimu...");
  ap_mode = false;
}

void tryReconnect() {
    Serial.println("Reconnect...");
    int n = wifi_softap_get_station_num();
    if (n > 0) {
      Serial.println("No reconnect, client is associated");
      return;
    }
    if (origin_ssid.isEmpty()) {
      Serial.println("No reconnect, no prior ssid");
      return;
    }
    n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++) {
      String ssidFound = WiFi.SSID(i);
      if (ssidFound == origin_ssid) {
        Serial.println("Známá síť '" + ssidFound + "' nalezena. Zkouším připojení...");
        WiFi.scanDelete(); // uvolnění paměti
        startSTA();        // pokus o přepnutí
        return;
      }
    }
    WiFi.scanDelete(); // nezapomenout
    Serial.println("Zatím žádná známá síť v dosahu.");
}

void updateDisplay() {    
    display.clearBuffer();
    display.setFont(u8g2_font_logisoso32_tf);
    char buff[20];
    int val = getLevel()/10;
    if (val < 0) snprintf(buff,20,"< 0");
    else snprintf(buff, 20, "%d.%02d", val/100, val % 100);
    int w = display.getStrWidth(buff);
    display.drawStr(90-w, 64, buff);

    display.setFont(u8g2_font_logisoso16_tf);
    display.drawStr(90, 64, "m");
    display.setFont(u8g2_font_6x13_tf);
    if (ap_mode) {
        display.drawStr(0,13,std::string(apName).c_str());
        display.drawStr(0,26,WiFi.softAPIP().toString().c_str());

    } else {        
        display.drawStr(0,13,WiFi.SSID().c_str());
        display.drawStr(0,26,WiFi.localIP().toString().c_str());
    }

    display.sendBuffer();
    display.setContrast(0);

}

void loop() {
   dns.processNextRequest();
   server.handleClient();
   cidlo.read();

  if (millis()>next_tick) {

    int v = conf.zero_point - cidlo.get_value();
    trendCalc.add(v);

    next_tick = millis() + 1000;
    int status = WiFi.status();
    if (status == WL_CONNECTED) {
        error_ticks = 0;
    } else {
        ++error_ticks;
        if (error_ticks > 10) {
            Serial.println(error_ticks);
            error_ticks = 0;
              if (!ap_mode) {
                startCaptivePortal();
            } else {
                tryReconnect();
            }
        }
    }
    updateDisplay();
  }
}
