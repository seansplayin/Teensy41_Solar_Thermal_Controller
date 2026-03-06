#ifndef LOGGING_H
#define LOGGING_H


void logPumpEvent(int pumpIndex, const String& event);
void listAllFiles();
void readAndPrintLogFile(const String& filename);
void aggregatePumptoDailyLogs(int pumpIndex);
void aggregateDailyToMonthlyLogs(int pumpIndex);
void aggregateMonthlyToYearlyLogs(int pumpIndex);
void performLogAggregation();
void checkTimeAndAct(); 
unsigned long extractRuntimeFromLogLine(String line);
unsigned long extractTimestamp(const String& line);
void logMessage(const String& message);


extern QueueHandle_t logQueue;

void logPumpEvent(uint8_t pumpIndex, bool isStart, const DateTime &ts);

#endif // LOGGING_H