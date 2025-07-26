#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <string>

ESP8266WebServer server(80); // Port 80 = HTTP
DNSServer dns;

constexpr std::string_view WebPage = R"html(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<meta charset='utf-8'><title>Hladinoměr</title>
</head>
<style>
body {
    margin: 0;
    padding: 0;
}

h1 {text-align:center;}

.form {
    position:relative;    
    max-width: 14rem;
    margin:auto;
}

table.hl {
    width: 100%;
    font-size: 1.7rem;
    max-width: 15rem;
    margin: auto;    
}

table.hl th {text-align:right;}
table.hl td:nth-child(2) {text-align:right;}
    
table.scan {
    border: 1px solid;
    border-collapse: collapse;
    margin-top: 1rem;
    width: 100%;    
}

table.scan th {
    background-color: #ccc;
    padding: 0.2rem;
    border-right: 1px dotted;
}

table.scan td {
    padding: 0.2rem;
    border-right: 1px dotted;
}

.form label {
    display:flex;
    position: relative;
    margin: 0.2rem;
}

.form label > span {
    width:40%;
}
.form label > *:last-child {
    flex-grow:1;
    flex-shrink:1;
}

input[type=number] {
    width: 5rem;
    text-align: center;
}
input[type=text] {
    text-align:center;
    width: 5rem;
}
.form .buttons {
    text-align:right;
}
table.scan tr {
    cursor: pointer;
}
table.scan tr td:last-child {
    text-align:right;
}
    
</style>
<script type="text/javascript">


function fetchData() {
    fetch("/data").then(x=>x.json()).then(data=>{
        var hladina = document.getElementById("hladina");
        var trend = document.getElementById("trend");
        hladina.textContent = data.level;
        trend.textContent = data.trend;
    });
    setTimeout(fetchData,1000);
}

function start() {
    setTimeout(fetchData,1000);
}

function showhide() {
    const p = this.parentElement;
    const nx1 = p.nextElementSibling;
    nx1.hidden = !nx1.hidden;
    
}

function doScan() {
    const p = this.parentElement;
    const nx1 = p.nextElementSibling;
    nx1.hidden = !nx1.hidden;
    if (!nx1.hidden) {

        function runScan() {
            if (nx1.hidden) return ;
            fetch("/scan").then(x=>x.json())
                .then(data=>{
                    if (data.length == 0) return;
                    let sr = document.getElementById("scan_result");
                    sr.innerHTML="";
                    data.sort((a,b)=>b.RSSI - a.RSSI).forEach(x=>{
                        let r = document.createElement("tr");
                        let d1 = document.createElement("td");
                        let d2 = document.createElement("td");
                        let perc =  2 * (x.RSSI + 100);
                        if (perc > 100) perc = 100;
                        if (perc < 1) perc = 1;
                        
                        d1.textContent = x.SSID;
                        d2.textContent = perc+" %";
                        r.appendChild(d1);
                        r.appendChild(d2);
                        sr.appendChild(r);
                        r.onclick = ()=>{
                            document.getElementById("ssid").value = x.SSID;
                        };
                    });
                    setTimeout(runScan,5000);
                });
            
        }

        runScan();
    }
}


</script>
<body onload="start()">
<h1>Hladinoměr</h1>
<table class="hl">
<tr><th>Hladina:</th><td id="hladina"></td><td>cm</td></tr>
<tr><th>Trend:</th><td id="trend"></td><td>cm/h</td></tr>
</table>
<hr>
    <label>
        <input type="checkbox" onchange="showhide.call(this)"> Kalibrace
    </label>
    <div hidden="hidden" class="form">
        <label><span>Nulový bod</span>
            <input type="number" id="nullpoint">
        </label>
        <div class="buttons"><button>Nastavit</button></div>
    </div>            
<hr>
    <label>
        <input type="checkbox" onchange="doScan.call(this)"> Nastav WiFi
    </label>
    <div hidden="hidden" class="form">
        <form method="POST" action="/savewifi">
            <label><span>SSID:</span>
                <input name="ssid" type="text"  id="ssid">            
            </label>
            <label><span>Heslo:</span>
                <input name="pass" type="text" id="pass">            
            </label>
                <div class="buttons"><input type="submit" value="Nastavit"></div>
        </form>
        <table class="scan">
            <thead>
                <tr><th>SSID</th><th>Signal</th></tr>
            </thead>
            <tbody id="scan_result">
                <tr>
                    <td colspan="2">(Vyhledávám sítě)</td>
                </tr>
            </tbody>
        </table>
    </div>            
</body>
</html>
)html";


float getLevel() {
  // Např. čtení z čidla
  return 123.4;
}

float getTrend() {
  // Např. výpočet trendu
  return -0.5;
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

String  origin_ssid;
unsigned long next_tick = 0;
unsigned int error_ticks = 0;
bool ap_mode = false;


void setup()
{
  Serial.begin(115200);
  Serial.println();

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
  server.begin();  
}

void startCaptivePortal() {
  ap_mode = true;
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Hladinometr_AP");
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

void loop() {
   dns.processNextRequest();
   server.handleClient();
  if (millis()>next_tick) {
    next_tick = millis() + 1000;
    int status = WiFi.status();
    if (status == WL_CONNECTED) {
      error_ticks = 0;
    } else {
      ++error_ticks;
      if (error_ticks > 10) {
          error_ticks = 0;
          if (!ap_mode) {
              startCaptivePortal();
          } else {
              tryReconnect();
        }
      }
    }
    Serial.printf("Connection status: %d, IP %s\n", WiFi.status(), WiFi.localIP().toString().c_str());
  }
}
