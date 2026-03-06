#ifndef MEMORYSTATS_H
#define MEMORYSTATS_H
#include <Arduino.h>

String getHeapInternalString();
String getPsramString();
void MemoryStats_printSnapshot(const char* tag);

#endif