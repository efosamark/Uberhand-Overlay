#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <time.h>
#include <chrono>

const char* logPath = "sdmc:/config/uberhand/log.txt";

char logTimeBuffer[std::size("yyyy-mm-dd hh:mm:ss")]{ 0 };

__attribute__((__format__(__printf__, 1, 2)))
void log(const char* format, ...) noexcept {
    FILE* logFile = fopen(logPath, "a");
    if (logFile == nullptr) {
        fprintf(stderr, "Failed to open log file: %s", logPath);
        return;
    }
    u64 currentTime;
    timeGetCurrentTime(TimeType_NetworkSystemClock, &currentTime);
    tm* localTime = std::localtime((std::time_t*)(&currentTime));
    std::strftime(logTimeBuffer, sizeof(logTimeBuffer), "%F %T", localTime);

    fputc('[', logFile);
    fputs(logTimeBuffer, logFile);
    fputc(']', logFile);
    fputc(' ', logFile);
    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);
    fputc('\n', logFile);
    fclose(logFile);
}