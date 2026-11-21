#include "esp_http_server.h"
#include "Arduino.h"
#include <ArduinoJson.h>

double x;
double y;
bool led;

/* HTML Page */
const char page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Drone Control</title>
  <style>
    body { font-family:sans-serif; background:#111; color:#eee; display:flex; flex-direction:column; align-items:center; }
    #stream { width:320px; height:240px; border:2px solid #333; margin:20px; }
    #controls { display:flex; flex-direction:column; align-items:center; gap:20px; }
    #joystick {
      width:200px;
      height:200px;
      background:#222;
      border-radius:50%;
      position:relative;
      touch-action:none;
    }
    #knob {
      width:60px;
      height:60px;
      background:pink;
      border-radius:50%;
      position:absolute;
      left:50%;
      top:50%;
      transform:translate(-50%,-50%);
      pointer-events:none;
    }
    button { padding:15px 30px; font-size:18px; border-radius:10px; cursor:pointer; }
  </style>
</head>
<body>
  <h2>ESP32 Drone Control Interface</h2>

  <iframe id="stream" frameborder="0"></iframe>

  <div id="controls">
    <div id="joystick"><div id="knob"></div></div>
    <button id="ledBtn">LED</button>
    <p>Joystick: <span id="joy-coords">x:0, y:0</span></p>
    <p>LED: <span id="led-state">OFF</span></p>
  </div>

<script>
document.getElementById("stream").src =
  `http://${window.location.hostname}:81/stream`;

function sendControl(data) {
  fetch("/joystick", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data)
  });
}

const joystick = document.getElementById("joystick");
const knob = document.getElementById("knob");
const joyCoords = document.getElementById("joy-coords");

let dragging = false;

function handleMove(clientX, clientY) {
  const rect = joystick.getBoundingClientRect();
  const cx = rect.width / 2;
  const cy = rect.height / 2;

  let ox = clientX - rect.left - cx;
  let oy = clientY - rect.top - cy;

  const radius = cx - knob.offsetWidth / 2;
  const dist = Math.hypot(ox, oy);

  if (dist > radius) {
    const s = radius / dist;
    ox *= s;
    oy *= s;
  }

  knob.style.left = `${cx + ox}px`;
  knob.style.top  = `${cy + oy}px`;

  const nx = (ox / radius).toFixed(2);
  const ny = (-oy / radius).toFixed(2);

  joyCoords.textContent = `x:${nx}, y:${ny}`;
  sendControl({ x: nx * 100, y: ny * 100 });
}

function resetJoystick() {
  dragging = false;
  knob.style.left = "50%";
  knob.style.top  = "50%";
  joyCoords.textContent = "x:0, y:0";
  sendControl({ x: 0, y: 0 });
}

/* Mouse */
joystick.addEventListener("mousedown", () => dragging = true);
document.addEventListener("mousemove", e => {
  if (dragging) handleMove(e.clientX, e.clientY);
});
document.addEventListener("mouseup", resetJoystick);

/* Touch */
joystick.addEventListener("touchstart", e => {
  dragging = true;
  handleMove(e.touches[0].clientX, e.touches[0].clientY);
});
document.addEventListener("touchmove", e => {
  if (dragging) handleMove(e.touches[0].clientX, e.touches[0].clientY);
});
document.addEventListener("touchend", resetJoystick);

/* LED */
const ledBtn = document.getElementById("ledBtn");
const ledState = document.getElementById("led-state");
ledBtn.onclick = () => {
  fetch("/led").then(r => r.text()).then(t => {
    ledState.textContent = t.includes("ON") ? "ON" : "OFF";
  });
};
</script>
</body>
</html>
)rawliteral";

/* Handlers */
esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, page, strlen(page));
}

esp_err_t joystick_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) return httpd_resp_send_500(req);
  buf[ret] = '\0';

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, buf)) return httpd_resp_send_500(req);

  if (doc.containsKey("x")) x = doc["x"];
  if (doc.containsKey("y")) y = doc["y"];

  return httpd_resp_send(req, "OK", 2);
}

esp_err_t led_handler(httpd_req_t *req) {
  led = !led;
  return httpd_resp_send(req, led ? "LED ON" : "LED OFF", HTTPD_RESP_USE_STRLEN);
}