#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// Network credentials
  char* u = "octopuss shooter";
  char* p = "12345689";//atleast nine digits

// Pin definitions
#define TRIG_PIN 14   // GPIO14 (D5 on some boards)
#define PIG_PIN 12    // GPIO12 (D6 on some boards)
#define SERVO_PIN 15   // GPIO9 (D9 on some boards)

Servo yaw;

// Create WebSocket and HTTP servers
WebSocketsServer websocket = WebSocketsServer(81);
ESP8266WebServer server(80);

// Global variables to store the parsed values
int range = 90;
bool checkbox1Checked = false;
bool checkbox2Checked = false;

void managepins(int range1, bool checkbox1, bool checkbox2) {
    digitalWrite(PIG_PIN, checkbox2 ? HIGH : LOW);
    if (checkbox2) {
        digitalWrite(TRIG_PIN, checkbox1 ? HIGH : LOW);
    }
    yaw.write(range1);
}

void parseMessage(String msg) {
    int rangeIndex = msg.indexOf("range:");
    int rangeEndIndex = msg.indexOf(";", rangeIndex);
    String rangeValue = msg.substring(rangeIndex + 6, rangeEndIndex);
    range = rangeValue.toInt();

    int checkbox1Index = msg.indexOf("checkbox1:");
    int checkbox1EndIndex = msg.indexOf(";", checkbox1Index);
    String checkbox1Value = msg.substring(checkbox1Index + 10, checkbox1EndIndex);
    checkbox1Checked = (checkbox1Value == "true");

    int checkbox2Index = msg.indexOf("checkbox2:");
    String checkbox2Value = msg.substring(checkbox2Index + 10);
    checkbox2Checked = (checkbox2Value == "true");

    Serial.print("Range: ");
    Serial.println(range);
    Serial.print("Checkbox1: ");
    Serial.println(checkbox1Checked ? "Checked" : "Unchecked");
    Serial.print("Checkbox2: ");
    Serial.println(checkbox2Checked ? "Checked" : "Unchecked");
}

const char* webpage = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>God Ex Machina</title>
    <style>
        body {
            background-color: rgb(42, 36, 36);
            color: white;
            font-family: Arial, sans-serif;
        }
        .range-slider {
            margin: 10px;
            color: blueviolet;
            width: 100%;
            height: 40px;
            accent-color: rgb(66, 29, 150);
        }
        .beans {
            align-content: center;
        }
        input.checkbox1 {
            width: 100px;
            height: 40px;
            accent-color: rgb(232, 239, 26);
        }
        input.checkbox2 {
            width: 80px;
            height: 40px;
            accent-color: red;
        }
        .names {
            display: flex;
            margin-left: 25px;
        }
    </style>
</head>
<body>
    <input type="range" min="0" value="90" max="180" class="range-slider" id="rangeSlider">
    <div class="names">
        <h2>spin____</h2>
        <h2>trigger</h2>
    </div>
    <div class="beans">
        <input type="checkbox" class="checkbox1" id="checkbox1">
        <input type="checkbox" class="checkbox2" id="checkbox2">
    </div>

    <script>
        const ws = new WebSocket('ws://' + location.hostname + ':81');

        ws.onopen = () => {
            console.log('WebSocket connection established');
        };

        ws.onmessage = (event) => {
            console.log('Message from server', event.data);
        };

        ws.onclose = () => {
            console.log('WebSocket connection closed');
        };

        function sendValues() {
            const rangeValue = document.getElementById('rangeSlider').value;
            const checkbox1Checked = document.getElementById('checkbox1').checked;
            const checkbox2Checked = document.getElementById('checkbox2').checked;

            const message = `range:${rangeValue};checkbox1:${checkbox1Checked};checkbox2:${checkbox2Checked}`;
            ws.send(message);
        }

        // Add event listeners to call sendValues on any input change
        document.getElementById('rangeSlider').addEventListener('input', sendValues);
        document.getElementById('checkbox1').addEventListener('change', sendValues);
        document.getElementById('checkbox2').addEventListener('change', sendValues);
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    server.send(200, "text/html", webpage);
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        Serial.print("Received message: ");
        Serial.println(msg);
        parseMessage(msg);
        managepins(range, checkbox1Checked, checkbox2Checked);
    }
}

void setup() {
    Serial.begin(9600);
    WiFi.mode(WIFI_AP); 
    WiFi.softAP(u, p);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
    
    pinMode(PIG_PIN, OUTPUT);
    pinMode(TRIG_PIN, OUTPUT);
    yaw.attach(SERVO_PIN);
    
    websocket.begin();
    websocket.onEvent(onWebSocketEvent);

    server.on("/", HTTP_GET, handleRoot);
    server.begin();
}

void loop() {
    server.handleClient();
    websocket.loop();
}

