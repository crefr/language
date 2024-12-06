#ifndef LOGGER_INCLUDED
#define LOGGER_INCLUDED

#include <wchar.h>

/// @brief different levels of logging, IT IS NECESSARY TO WRITE THEM IN ASCENDING ORDER
enum loglevels{LOG_RELEASE, LOG_DEBUG, LOG_DEBUG_PLUS};

/// @brief enum with logging modes
typedef enum
{
    LOG_HTML,
    LOG_TEXT
} log_mode_t;

#define LOGPRINTWITHTIME(loglevel, ...)          \
        do{                                      \
                logPrintTime(loglevel);          \
                logPrint(loglevel, __VA_ARGS__); \
        }while(0)

#define LOGPRINT(loglevel, ...)                  \
        do{                                      \
                logPrint(loglevel, __VA_ARGS__); \
        }while(0)

#define LOGPRINTERROR(loglevel, ...)             \
        do{                                      \
                logPrintTime(loglevel);          \
                logPrint(loglevel, __VA_ARGS__); \
                logExit();                       \
        }while(0)

#define PRINTFANDLOG(loglevel, ...)              \
        do{                                      \
                logPrint(loglevel, __VA_ARGS__); \
                printf(__VA_ARGS__);             \
                printf("\n");                    \
        }while(0)

/// @brief starts logging, initialises inside vars, opens file
int  logStart(const char * logfilename, enum loglevels loglevel, log_mode_t mode);

/// @brief formats and prints string to log file
void logPrint(enum loglevels loglevel, const char * fmt, ...);

/// @brief formats and prints wide char string to log file
void wlogPrint(enum loglevels loglevel, const wchar_t * log_str, ...);

/// @brief prints current time to log file
void logPrintTime(enum loglevels loglevel);

/// @brief ends logging, closes file
void logExit(void);

/// @brief cancels buffering in log file
void logCancelBuffer();

/// @brief returns current logging level
enum loglevels logGetLevel();


#endif
