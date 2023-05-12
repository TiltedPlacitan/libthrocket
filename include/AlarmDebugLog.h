//============================================================================================================================= 132
//
//  AlarmDebugLog.h
//
//      Simple facility for logging events to syslog and debug to stderr.
//
//      u16Port is always host-byte-order (as it should not need to be converted to be readable in printf)...
//      inaIPAddr is always network-byte-order (as it needs to be converted to be readable in printf)...
//
//  COLUMNS 132 TABSTOP 4 SPACE-FILL
//
//============================================================================================================================= 132

/* ============================================================================

Copyright 1998-2022 Jack Bates

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

============================================================================ */

#pragma once

#include <syslog.h>

#include <cstdio>
#include <cstdint>
#include <cstdarg>

#include <sstream>

//============================================================================================================================= 132
// SYSLOG EVENTS
namespace libthrocket
{

const size_t    max_log_line = 1024;

extern bool     ADL_syslog_opened;
extern char *   ADL_syslog_ident;
extern uint64_t ADL_debug_mask;
extern uint8_t  ADL_debug_level;

inline void ADL_syslog_open()
{
    if (!libthrocket::ADL_syslog_opened)
        openlog(ADL_syslog_ident, LOG_CONS | LOG_PID, LOG_DAEMON);
    ADL_syslog_opened = true;
}

extern void LOGSYSLOG(const char * level, const char * fmt, va_list args);

inline void LOGCRITICAL(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("CRITICAL", fmt, args);
}

inline void LOGERROR(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("ERROR", fmt, args);
}

inline void LOGWARNING(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("WARNING", fmt, args);
}

inline void LOGINFO(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("INFO", fmt, args);
}

inline void LOGSTATE(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("STATE", fmt, args);
}

inline void LOGTRACE(const char * fmt...)
{
    va_list args;
    va_start(args, fmt);
    LOGSYSLOG("TRACE", fmt, args);
}

};

//============================================================================================================================= 132
// STDERR DEBUGGING
#define ADL_DLVL_NONE ( 0)
#define ADL_DLVL_LOW  ( 4)
#define ADL_DLVL_MED  ( 8)
#define ADL_DLVL_HIGH (16)

#define ADL_DMSK_NONE ((uint64_t) 0x0000000000000000)
#define ADL_DMSK_SCK  ((uint64_t) 0x0000000000000001)
#define ADL_DMSK_ALL  ((uint64_t) 0xFFFFFFFFFFFFFFFF)

#define WOULDDEBUG(mask, level)                                                     \
    (mask & libthrocket::ADL_debug_mask && level >= libthrocket::ADL_debug_level)
#define LOGDEBUG(mask, level, args...)                                              \
    if (WOULDDEBUG(mask, level))                                                    \
    {    fprintf(stderr, "DEBUG: "); fprintf(stderr, args); fprintf(stderr, "\n"); }

//============================================================================================================================= 132
