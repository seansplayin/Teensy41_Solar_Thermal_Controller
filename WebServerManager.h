#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H
#include <Arduino.h>
#include <AsyncWebServer_Teensy41.h>


extern AsyncWebServer server;
extern AsyncWebSocket ws;

// Add these prototypes (fixes undeclared functions)
void WebServerManager_begin();
String getContentType(const String& path);

#endif