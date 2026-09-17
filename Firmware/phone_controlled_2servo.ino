#include <WiFi.h>
#include <WebServer.h>

// --- Configuration ---
const char* ssid = "My_RC_Car";
const char* password = "password123";

WebServer server(80);

// --- Motor Pin Setup ---
const int m1Pin1 = 25; const int m1Pin2 = 26;
const int m2Pin1 = 12; const int m2Pin2 = 13;
const int m3Pin1 = 4;  const int m3Pin2 = 5;
const int m4Pin1 = 16; const int m4Pin2 = 17;

// --- Servo Pin Setup (From your schematic) ---
const int servo1Pin = 27; // Pin 10 on your J7 connector
const int servo2Pin = 32; // Pin 11 on your J7 connector

// --- PWM Settings ---
// Motors
const int motorFreq = 30000;
const int motorRes = 8;
// Servos (Must be 50Hz)
const int servoFreq = 50;
const int servoRes = 14; // Higher resolution for smooth movement

// PWM Channels (0-15 available)
// Motors use 0-7
const int c1a=0, c1b=1; 
const int c2a=2, c2b=3; 
const int c3a=4, c3b=5; 
const int c4a=6, c4b=7;
// Servos use 8-9
const int cServo1 = 8;
const int cServo2 = 9;

// --- HTML Interface ---
const char* htmlPage = R"html(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<style>
  body { 
    background-color: #222; margin: 0; 
    display: flex; flex-direction: column; align-items: center; 
    font-family: sans-serif; color: white;
    height: 100vh; overflow: hidden;
  }
  h3 { margin: 10px 0 5px 0; pointer-events: none; }
  
  /* Joystick Styles */
  #joystick-zone {
    width: 260px; height: 260px;
    background: radial-gradient(circle, #444 0%, #222 60%);
    border: 2px solid #555; border-radius: 50%;
    position: relative; touch-action: none; margin-bottom: 20px;
  }
  #knob {
    width: 70px; height: 70px;
    background: #4CAF50; border-radius: 50%;
    position: absolute; top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    box-shadow: 0 0 15px rgba(0,0,0,0.5); pointer-events: none;
  }

  /* Slider Styles */
  .slider-container {
    width: 80%; max-width: 400px;
    background: #333; padding: 10px; margin: 5px;
    border-radius: 10px; display: flex; align-items: center;
  }
  .slider-label { width: 70px; font-weight: bold; font-size: 14px; }
  input[type=range] { flex-grow: 1; height: 30px; }
</style>
</head>
<body>

  <div class="slider-container">
    <div class="slider-label">Servo 1</div>
    <input type="range" min="0" max="180" value="90" oninput="sendServo(1, this.value)">
  </div>
  <div class="slider-container">
    <div class="slider-label">Servo 2</div>
    <input type="range" min="0" max="180" value="90" oninput="sendServo(2, this.value)">
  </div>

  <h3>Drive</h3>
  <div id="joystick-zone">
    <div id="knob"></div>
  </div>

<script>
  // Joystick Logic
  const zone = document.getElementById('joystick-zone');
  const knob = document.getElementById('knob');
  const maxDist = 90; 
  let rect = zone.getBoundingClientRect();
  let dragging = false;

  zone.addEventListener('touchstart', startMove);
  zone.addEventListener('touchmove', moveKnob);
  zone.addEventListener('touchend', endMove);
  zone.addEventListener('mousedown', (e) => { dragging=true; startMove(e); });
  window.addEventListener('mousemove', (e) => { if(dragging) moveKnob(e); });
  window.addEventListener('mouseup', () => { if(dragging) { dragging=false; endMove(); }});

  function startMove(e) { e.preventDefault(); rect = zone.getBoundingClientRect(); }
  
  function moveKnob(e) {
    e.preventDefault();
    let clientX = e.touches ? e.touches[0].clientX : e.clientX;
    let clientY = e.touches ? e.touches[0].clientY : e.clientY;
    let x = clientX - (rect.left + rect.width/2);
    let y = clientY - (rect.top + rect.height/2);
    
    let dist = Math.sqrt(x*x + y*y);
    if (dist > maxDist) { x = (x/dist)*maxDist; y = (y/dist)*maxDist; }
    
    knob.style.transform = `translate(calc(-50% + ${x}px), calc(-50% + ${y}px))`;
    
    let normX = Math.round((x / maxDist) * 255);
    let normY = Math.round((y / maxDist) * 255) * -1;
    sendDrive(normX, normY);
  }

  function endMove() {
    knob.style.transform = `translate(-50%, -50%)`;
    sendDrive(0, 0);
  }

  let lastDrive = 0;
  function sendDrive(x, y) {
    let now = Date.now();
    if (now - lastDrive > 50 || (x===0 && y===0)) {
      lastDrive = now;
      fetch(`/drive?x=${x}&y=${y}`).catch(e=>{});
    }
  }

  // Servo Logic
  function sendServo(id, val) {
    fetch(`/servo?id=${id}&val=${val}`).catch(e=>{});
  }
</script>
</body>
</html>
)html";

void setup() {
  Serial.begin(115200);

  // --- Motor Setup ---
  ledcSetup(c1a, motorFreq, motorRes); ledcAttachPin(m1Pin1, c1a);
  ledcSetup(c1b, motorFreq, motorRes); ledcAttachPin(m1Pin2, c1b);
  ledcSetup(c2a, motorFreq, motorRes); ledcAttachPin(m2Pin1, c2a);
  ledcSetup(c2b, motorFreq, motorRes); ledcAttachPin(m2Pin2, c2b);
  ledcSetup(c3a, motorFreq, motorRes); ledcAttachPin(m3Pin1, c3a);
  ledcSetup(c3b, motorFreq, motorRes); ledcAttachPin(m3Pin2, c3b);
  ledcSetup(c4a, motorFreq, motorRes); ledcAttachPin(m4Pin1, c4a);
  ledcSetup(c4b, motorFreq, motorRes); ledcAttachPin(m4Pin2, c4b);

  // --- Servo Setup ---
  ledcSetup(cServo1, servoFreq, servoRes); 
  ledcAttachPin(servo1Pin, cServo1);
  
  ledcSetup(cServo2, servoFreq, servoRes); 
  ledcAttachPin(servo2Pin, cServo2);

  // Set default position (90 degrees)
  writeServo(cServo1, 90);
  writeServo(cServo2, 90);

  // Wi-Fi
  WiFi.softAP(ssid, password);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // Server
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/drive", handleDrive);
  server.on("/servo", handleServo);
  server.begin();
}

void loop() {
  server.handleClient();
}

// --- Servo Helper Function ---
// Calculates the correct PWM duty cycle for a servo at 50Hz
void writeServo(int channel, int angle) {
  // Map angle (0-180) to pulse width (500us - 2400us)
  int microsec = map(angle, 0, 180, 500, 2400);
  // Calculate Duty Cycle for 14-bit resolution (2^14 = 16384)
  // Period at 50Hz is 20000 microseconds
  int duty = (microsec * 16384) / 20000; 
  ledcWrite(channel, duty);
}

void handleServo() {
  if (server.hasArg("id") && server.hasArg("val")) {
    int id = server.arg("id").toInt();
    int val = server.arg("val").toInt();
    
    if (id == 1) writeServo(cServo1, val);
    if (id == 2) writeServo(cServo2, val);
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Error");
  }
}

// --- Motor Logic ---
void handleDrive() {
  if (server.hasArg("x") && server.hasArg("y")) {
    int x = server.arg("x").toInt(); 
    int y = server.arg("y").toInt(); 
    int left = constrain(y + x, -255, 255);
    int right = constrain(y - x, -255, 255);

    setMotorGroup(c1a, c1b, c3a, c3b, left);
    setMotorGroup(c2a, c2b, c4a, c4b, right);
    server.send(200, "text/plain", "OK");
  }
}

void setMotorGroup(int fA, int fB, int rA, int rB, int spd) {
  int pwm = abs(spd);
  if (spd > 0) {
    ledcWrite(fA, pwm); ledcWrite(fB, 0); ledcWrite(rA, pwm); ledcWrite(rB, 0);
  } else if (spd < 0) {
    ledcWrite(fA, 0); ledcWrite(fB, pwm); ledcWrite(rA, 0); ledcWrite(rB, pwm);
  } else {
    ledcWrite(fA, 0); ledcWrite(fB, 0); ledcWrite(rA, 0); ledcWrite(rB, 0);
  }
}
