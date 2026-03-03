#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "WebServerManager.h"     // gives us extern AsyncWebServer server;

bool isNetworkConnected();
void setupNetwork();

#endif