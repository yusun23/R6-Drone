#ifndef INTERFACE_H
#define INTERFACE_H

#include "esp_http_server.h"

// ===== Handlers =====
esp_err_t index_handler(httpd_req_t *req);
esp_err_t joystick_handler(httpd_req_t *req);
esp_err_t led_handler(httpd_req_t *req);

// ===== Shared control variables =====
extern double x;      // Joystick X
extern double y;      // Joystick Y
extern bool led;   // LED state

#endif
