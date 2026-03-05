#ifndef WEBSERVER_MANAGER_H
#define WEBSERVER_MANAGER_H

#define CORE_TEENSY  // Add this here to fix the library error across all including files

#include <AsyncWebServer_Teensy41.h>
#include <Teensy41_AsyncTCP.h>

extern AsyncWebServer server;
extern AsyncWebSocket ws;

// Add these prototypes (fixes undeclared functions)
void WebServerManager_begin();
String getContentType(const String& path);

#endif