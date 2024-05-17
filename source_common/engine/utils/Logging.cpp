///------------------------------------------------------------------------------------------------
///  Logging.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 25/04/2024
///------------------------------------------------------------------------------------------------

#include <mutex>

#include <engine/utils/Logging.h>
#include <engine/utils/Date.h>

///------------------------------------------------------------------------------------------------

static std::mutex sLoggingMutex;

///------------------------------------------------------------------------------------------------

namespace logging
{

///------------------------------------------------------------------------------------------------

void Log(const LogType logType, const char* message, ...)
{
    std::lock_guard<std::mutex> loggingGuard(sLoggingMutex);
    switch(logType)
    {
        case LogType::INFO: printf("[INFO] "); break;
        case LogType::WARNING: printf("[WARNING] "); break;
        case LogType::ERROR: printf("[ERROR] "); break;
    }
    
    va_list args;
    va_start (args, message);
    vprintf (message, args);
    va_end (args);
    
    printf("\n");
    
    fflush(stdout);
}

///------------------------------------------------------------------------------------------------

}

///------------------------------------------------------------------------------------------------
