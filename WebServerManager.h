#pragma once
#include <Arduino.h>




extern AsyncWebServer server;
extern AsyncWebSocket ws;          // if you use websockets

void WebServerManager_begin();