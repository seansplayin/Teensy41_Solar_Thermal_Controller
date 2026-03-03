#pragma once
#include <Arduino.h>

// === ONLY PLACE WE INCLUDE THE HEAVY TEENSY ASYNC HEADERS ===
#include <AsyncWebServer_Teensy41.h>

extern AsyncWebServer server;
extern AsyncWebSocket ws;          // if you use websockets

void WebServerManager_begin();