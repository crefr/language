#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "logger.h"

static enum loglevels LOGlevel = LOG_RELEASE;
static FILE * LOGfile = NULL;

int logStart(const char * logfilename, enum loglevels loglevel, log_mode_t mode)
{
    LOGlevel = loglevel;
    // LOGfile = fopen(logfilename, "a+");
    LOGfile = fopen(logfilename, "w");
    if (LOGfile == NULL){
        printf("}}} logger ERROR: cannot open logfile\n");
        return 0;
    }
    if (mode == LOG_HTML)
        logPrint(LOG_RELEASE, "<pre>\n");
    logPrint(LOG_RELEASE, "\n{-----------STARTED-----------}\n");
    return 1;
}

void logPrint(enum loglevels loglevel, const char * fmt, ...)
{
    if (loglevel <= LOGlevel){
        //logPrintTime();
        va_list va = {};
        va_start(va, fmt);
        vfprintf(LOGfile, fmt, va);
        //fprintf(LOGfile, "\n");
        va_end(va);
    }
}

void logPrintTime(enum loglevels loglevel)
{
    if (loglevel <= LOGlevel){
        time_t time_0= time(NULL);
        struct tm *calctime = localtime(&time_0);

        const size_t timestrlen = 100;
        char timestr[timestrlen] = {};

        strftime(timestr, timestrlen, "[%d.%m.%G %H:%M:%S] ", calctime);
        fprintf(LOGfile, "%s", timestr);
    }
}

void logExit()
{
    logPrint(LOG_RELEASE, "{-----------ENDING------------}\n");
    fclose(LOGfile);
}

void logCancelBuffer()
{
    setbuf(LOGfile, NULL);
}

enum loglevels logGetLevel()
{
    return LOGlevel;
}
