#include <string>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

/*
 * --------------------------------------------------
 * GLOBAL VARIABLES
 * 
 */
const char* wifiSsid = "Haze";
const char* wifiPass = "Bismillah";

ESP8266WebServer webServer(80); 
WebSocketsServer webSocket(81);

String serialData;

/*
 * --------------------------------------------------
 * CUSTOM FUNCTIONS
 * 
 */

void ledBlink(int pin, int del = 500, int repeat = 1) {
  for (int i = 0; i < 1; ++i ) {
    digitalWrite(pin, HIGH);
    delay(del);
    digitalWrite(pin, LOW);
    delay(del);  
  }
}

/*
 * --------------------------------------------------
 * HANDLER VARIABLES
 * 
 */

void webSocketHandler(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("Websocket Client [%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("Websocket Client [%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      break;
  }
}

void httpGetIndex() {
  webServer.send(200, "text/html", "<!DOCTYPE html><html lang='en'><head> <meta charset='UTF-8'> <meta name='viewport' content='width=device-width, initial-scale=1.0'> <meta http-equiv='X-UA-Compatible' content='ie=edge'> <title>Kelompok 1 Hardsoft / Smart Parking Monitoring</title> <style>html{font-family: 'Segoe UI', Arial, Helvetica, sans-serif;}body{background: #f7f7f7; margin: 0 !important; padding: 0 !important;}.container-flex{display: flex; flex-direction: row; justify-content: center; align-items: flex-start;}.child-flex{padding: 12px; margin-left: 12px; margin-right: 12px; border: 1px solid #ddd; border-radius: 5px;}.box{display: flex; width: 250px; min-height: 400px; background-color: #eee; justify-content: center;}.vehicle-miniature{background-image: url('https://raw.githubusercontent.com/hizzely/testrepo/master/car.png'); background-size: cover; background-position: center; margin-top:0px; width: 130px; height: 185px; border-radius: 5px; transition: .4s;}</style></head><body> <div class='header-back'> <h2 style='text-align: center; background-color: #ddd; margin-top:0; padding: 24px;'>IoT-based Smart Parking Monitor <br/><small>&copy; 2019-2020 Kelompok 1</small></h2> </div><div class='container-flex'> <div class='child-flex'> <div class='box'> <div id='vehicle1' class='vehicle-miniature'></div></div><div style='text-align: center;'> <h3 id='vehicle1-title'>Lane 1 (<span id='vehicle1-status'>PARKED</span>)</h3> <p id='vehicle1-pos'>Approx. Distance: <span id='vehicle1-distance'>0</span>cm</p></div></div><div id='lane2' class='child-flex'> <div class='box'> <div id='vehicle2' class='vehicle-miniature'></div></div><div style='text-align: center;'> <h3 id='vehicle2-title'>Lane 2 (<span id='vehicle2-status'>PARKED</span>)</h3> <p id='vehicle2-pos'>Approx. Distance: <span id='vehicle2-distance'>0</span>cm</p></div></div></div><script>const HOST=location.host,VIS_LANE_MAX=215;let state=[{elementId:'vehicle2',state:null,stateText:null,distance:null,data:null},{elementId:'vehicle1',state:null,stateText:null,distance:null,data:null}];function map(e,t,a,n,s){return(e-t)*(s-n)/(a-t)+n}function parseMessage(e){let t=e.split('|');return t.forEach((e,a)=>{t[a]=e.split(';')}),t}function updateViz(e){state.forEach((t,a)=>{switch(t.state=parseInt(e[a][0]),t.distance=Math.floor(parseInt(e[a][1])),t.data=map(t.distance,1,15,VIS_LANE_MAX,0),t.data<0?t.data=0:t.data>215&&(t.data=215),t.distance<0&&(t.distance=0),t.state){case 0:t.stateText='Empty';break;case 1:t.stateText='Parking';break;case 2:t.stateText='Idle';break;case 3:t.stateText='5 secs to park';break;case 4:t.stateText='Parked';break;case 5:t.stateText='Picked Up/Unpaid'}document.getElementById(`${t.elementId}-status`).textContent=t.stateText,document.getElementById(`${t.elementId}-pos`).style.opacity=t.distance>14?0:1,document.getElementById(`${t.elementId}-distance`).textContent=t.distance,document.getElementById(t.elementId).style.opacity=t.distance>14?0:1,document.getElementById(t.elementId).style.transform=`translateY(${t.data}px)`})}let socket=new WebSocket('ws://'+HOST+':81');socket.onmessage=function(e){updateViz(parseMessage(e.data))}; </script></body></html>");
}


/*
 * --------------------------------------------------
 * SETUP FUNCTIONS
 * 
 */

void setupWifi() {
  Serial.println(F("Setting up Wifi Access Point..."));
  WiFi.begin(wifiSsid, wifiPass);

  while (WiFi.status() != WL_CONNECTED) {
    ledBlink(LED_BUILTIN, 100, 10);
  }
  
  Serial.print(F("IP Address: "));
  Serial.println(WiFi.localIP());
}

void setupWebServer() {
  webServer.on("/", httpGetIndex);
  webServer.begin();
  Serial.println(F("HTTP server started"));
}

void setupWebSocket() {
  webSocket.begin();
  webSocket.onEvent(webSocketHandler);
  Serial.println(F("WebSocket server started"));
}


/*
 * --------------------------------------------------
 * MAIN ROUTINE
 * 
 */
 
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  while (!Serial) { ; }

  setupWifi();
  setupWebSocket();
  setupWebServer();
  
  Serial.println(F("NodeMCU Ready"));
}

void loop() {
  webSocket.loop();
  webServer.handleClient();

  if (Serial.available() > 0) {
    serialData = Serial.readStringUntil('\n');
    webSocket.broadcastTXT(serialData);
  }
}
