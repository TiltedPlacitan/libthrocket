//============================================================================================================================= 132
//
//  Exception.cc
//
//      A simple exception class intended for widespread inheritance.
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
#include "Exception.h"

using namespace std;

namespace libthrocket
{

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::LogCritical(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        LOGCRITICAL("\n============================================================"
                    "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                    "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        LOGCRITICAL("\n============================================================"
                    "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                    "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str());
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::LogError(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        LOGERROR("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        LOGERROR("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str());
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::LogWarning(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        LOGWARNING("\n============================================================"
                   "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                   "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        LOGWARNING("\n============================================================"
                   "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                   "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str());
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::LogState(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        LOGSTATE("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        LOGSTATE("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str());
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::LogTrace(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        LOGTRACE("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        LOGTRACE("\n============================================================"
                 "\n\n%s:\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                 "============================================================\n",
            m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(), m_strDetail.c_str());
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
Exception::Console(const string& strCaughtBy) const
{
    if (m_strStackTrace.length())
    {
        fprintf(stderr, 
                "\n============================================================"
                "\n\n %s\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\nSTACK:\n\n%s\n\n"
                "============================================================\n",
                m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(),
                m_strDetail.c_str(), m_strStackTrace.c_str());
    }
    else
    {
        fprintf(stderr, 
                "\n============================================================"
                "\n\n%s\n\nCAUGHT: %s\n\nTHROWN: %s\n\nDETAIL:\n\n%s\n\n"
                "============================================================\n",
                m_strClass.c_str(), strCaughtBy.c_str(), m_strThrownBy.c_str(),
                m_strDetail.c_str());
    }
}

};  // namespace libthrocket

//============================================================================================================================= 132
