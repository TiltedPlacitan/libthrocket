//============================================================================================================================= 132
//
//  AlarmDebugLog.cc
//
//      A simple logging class suitable for system and debug logging.
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

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#include "AlarmDebugLog.h"

namespace libthrocket
{

bool     ADL_syslog_opened  = false;
char *   ADL_syslog_ident   = const_cast<char *>("not-set");
uint64_t ADL_debug_mask     = 0;
uint8_t  ADL_debug_level    = 0;

void LOGSYSLOG(const char * level, const char * fmt, va_list args)
{
    ADL_syslog_open();
    char log_line[max_log_line + 1];
    int count = vsnprintf(log_line, max_log_line, fmt, args);
    if (count > (int) max_log_line) // ERROR: buffer overflow would have occurred
    {
        syslog(0, "Avoided buffer overflow in syslog message generation");
        return;
    }

    std::stringstream ss;
    ss << level << ": ";
    ss << log_line;
    ss << '\n';
    syslog(LOG_INFO, "%s", ss.str().c_str());
}

};
