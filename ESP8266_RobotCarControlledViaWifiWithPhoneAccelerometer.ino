/*
 * ESP8266 Robot Car controlled via WiFi with phone accelerometer sensor.
 * Written by Fajar Ru <kzofajar@gmail.com> 13/06/2018
 *
 * This code does not come with the websocket client.
 * You need to create a websocket client that send phone accelerometer data
 * in realtime (or periodically) to ESP8266 websocket server.
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

/*
 * --------------------------------------------------
 * SYSTEM
 * --------------------------------------------------
*/

// WiFi SSID data for Station Mode
const char *ssid = "Hizzely";
const char *password = "87654321";

// Pins configuration
const int led = 2; // default ESP8266 blue LED.
const int pinPutihPWM = 5; // left motor power
const int pinPutih1 = 14; // left motor direction +
const int pinPutih2 = 12; // left motor direction -
const int pinHitamPWM = 4; // right motor power
const int pinHitam1 = 13; // right motor direction +
const int pinHitam2 = 15; // right motor direction -

void ledBlink(int count, int del)
{
    for (int z = 0; z < count; z++)
    {
        digitalWrite(led, HIGH);
        delay(del);
        digitalWrite(led, LOW);
    }
}

/*
 * --------------------------------------------------
 * MOTOR CONTROLLER
 * --------------------------------------------------
*/

void motorPower(int signal)
{
    if (signal == 1)
    {
        digitalWrite(pinPutihPWM, HIGH);
        digitalWrite(pinHitamPWM, HIGH);
    }
    else
    {
        digitalWrite(pinPutihPWM, LOW);
        digitalWrite(pinHitamPWM, LOW);
    }
}

void motorControl(int action)
{
    switch (action)
    {
      case 0:
        Serial.println("Called motorControl.forward");
        // Motor 1&2 Direction: Forward
        digitalWrite(pinPutih1, HIGH);
        digitalWrite(pinPutih2, LOW);
        digitalWrite(pinHitam1, HIGH);
        digitalWrite(pinHitam2, LOW);
        // Turn on Motors
        motorPower(1);
        break;
      case 1:
        Serial.println("Called motorControl.backward");
        // Motor 1&2 Direction: Backwards
        digitalWrite(pinPutih1, LOW);
        digitalWrite(pinPutih2, HIGH);
        digitalWrite(pinHitam1, LOW);
        digitalWrite(pinHitam2, HIGH);
        // Turn on Motors
        motorPower(1);
        break;
      case 2:
        Serial.println("Called motorControl.turnLeft");
        // Motor 1 Direction: Backwards
        digitalWrite(pinPutih1, LOW);
        digitalWrite(pinPutih2, HIGH);
        // Motor 2 Direction: Forward
        digitalWrite(pinHitam1, HIGH);
        digitalWrite(pinHitam2, LOW);
        // Turn on Motors
        motorPower(1);
        break;
      case 3:
        Serial.println("Called motorControl.turnRight");
        // Motor 1 Direction: Forward
        digitalWrite(pinPutih1, HIGH);
        digitalWrite(pinPutih2, LOW);
        // Motor 2 Direction: Backwards
        digitalWrite(pinHitam1, LOW);
        digitalWrite(pinHitam2, HIGH);
        // Turn on Motors
        motorPower(1);
        break;
      default:
        motorPower(0);
        break;
    }
}

void demoTest()
{
    // forward
    motorControl(1);
    delay(2000);
    // backward
    motorControl(2);
    delay(2000);
    // left
    motorControl(3);
    delay(2000);
    // right
    motorControl(4);
    delay(2000);
    // stop
    motorPower(0);
    delay(2000);
}

/*
 * --------------------------------------------------
 * WEB SOCKET
 * --------------------------------------------------
*/

// New web socket instance
char *wsData, *wsEndPointer;
long int wsAccelX, wsAccelY, wsAccelZ;
const int webSocketPort = 8080;
WebSocketsServer webSocket = WebSocketsServer(webSocketPort);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
      case WStype_DISCONNECTED:
        Serial.print("WebSocket: Client ID ");
        Serial.print(num);
        Serial.println(" Disconnected!");
        break;
      case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.print("WebSocket: New Client!");
        Serial.printf("[%u] Connected from %d.%d.%d.%d (%s)\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        webSocket.sendTXT(num, "Connected!");
        ledBlink(2, 100);
      }
        break;
      case WStype_TEXT:
        Serial.print("WebSocket: New Data from Client ID ");
        Serial.println(num);

		/*
		 * This is where the received phone accelerometer data will be parsed
		 * and used to control the motors.
		*/
        wsData = (char *) payload;
        wsAccelX = strtol (wsData, &wsEndPointer, 10);
        wsAccelY = strtol (wsEndPointer, &wsEndPointer, 10);
        wsAccelZ = strtol (wsEndPointer, &wsEndPointer, 10);

        Serial.print("X - ");
        Serial.println(wsAccelX);
        Serial.print("Y - ");
        Serial.println(wsAccelY);
        Serial.print("Z - ");
        Serial.println(wsAccelZ);
		
		/*
		 * Here you can set the acceptable accelerometer sensor value
		 * in order to triggering the motors controller.
		*/
		
		// stationary mode, your phone on flat surface
        if (wsAccelX > -40 && wsAccelX < 40 && wsAccelY > -80 && wsAccelY < 80)
        {
            motorPower(0);
            Serial.println("X is > -40 && < 40: Motors off");
        }
		// left control, phone must be tilted to left
        else if (wsAccelX >= 40)
        {
            motorControl(2);
            Serial.println("X is >= 40: Motors turn left");
        }
		// right control, phone must be tilted to right
        else if (wsAccelX <= -40)
        {
            motorControl(3);
            Serial.println("X is <= -40: Motors turn right");
        }
		// forward control, phone must be tilted to front
        else if (wsAccelY >= 80)
        {
            motorControl(0);
            Serial.println("Y is >= 80: Motors move forward");
        }
		// backward control, phone must be tilted to back
        else if (wsAccelY <= -80)
        {
            motorControl(1);
            Serial.println("Y is <= -80: Motors move backward");
        }

        ledBlink(1, 100);
        break;
    }
}

/*
 * --------------------------------------------------
 * HTTP SERVER
 * --------------------------------------------------
*/

// New web server instance
const int httpServerPort = 80;
ESP8266WebServer server(httpServerPort);

void handleRoot()
{
    server.send(200, "text/plain", "Hello there, Hizzely here from ESP8266!");
}

void handleNotFound()
{
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

void setup(void)
{
    // Start Serial Communication
    Serial.begin(115200);
    Serial.println("ESP Device Setup");
    pinMode(led, OUTPUT);

    // Set Network Connection
    Serial.print("Setting up network");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // -- Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        ledBlink(1, 50);
        delay(200);
    }

    // -- Display network diagnostics data
    digitalWrite(led, LOW); // <- turn on LED when wifi is connected.
    Serial.println("");
    Serial.print("- Connected to ");
    Serial.println(ssid);
    Serial.print("- Signal Strength ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("- Router IP ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("- Device IP ");
    Serial.println(WiFi.localIP());
    Serial.print("- Device MAC ");
    Serial.println(WiFi.macAddress());

    // Set HTTP Server Route
    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.print("- HTTP server started on Port ");
    Serial.println(httpServerPort);

    // Set Websocket
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);
    Serial.print("- Web Socket started on Port ");
    Serial.println(webSocketPort);

    // Set Motor Controller Pin
    pinMode(pinPutihPWM, OUTPUT); // PWM Putih
    pinMode(pinPutih1, OUTPUT);   // -|
    pinMode(pinPutih2, OUTPUT);   // -|
    pinMode(pinHitamPWM, OUTPUT); // PWM Hitam
    pinMode(pinHitam1, OUTPUT);   // --|
    pinMode(pinHitam2, OUTPUT);   // --|
}

void loop(void)
{
    webSocket.loop();
    server.handleClient();
}
