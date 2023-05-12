//============================================================================================================================= 132
//
//  Exception.h
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

#pragma once

//#include "StackTrace.h"

#include <string>

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
namespace libthrocket
{
class Exception
{
    public:
                                    Exception(const std::string& strClass, const std::string& strThrownBy = "", const std::string& strDetail = "") :
                                        m_strClass(strClass),
                                        m_strThrownBy(strThrownBy),
                                        m_strDetail(strDetail)
                                    {
                                        //StackTrace s;
                                        //m_strStackTrace = s.ToString();
                                    }
                                    Exception(const Exception& e)   :
                                        m_strClass(e.GetClass()),
                                        m_strThrownBy(e.GetThrownBy()),
                                        m_strDetail(e.GetDetail())
                                    {
                                        //StackTrace s;
                                        //m_strStackTrace = s.ToString();
                                    }

        virtual                     ~Exception()
                                    {}

        virtual const std::string & GetClass() const
                                    { return m_strClass; }
        virtual void                SetClass(const std::string & s)
                                    { m_strClass = s; }
        virtual void                ClearClass()
                                    { m_strClass = ""; }
        virtual const std::string & GetThrownBy() const
                                    { return m_strThrownBy; }
        virtual void                SetThrownBy(const std::string & s)
                                    { m_strThrownBy = s; }
        virtual void                ClearThrownBy()
                                    { m_strThrownBy = ""; }
        virtual const std::string & GetDetail() const
                                    { return m_strDetail; }
        virtual void                SetDetail(const std::string & s)
                                    { m_strDetail = s; }
        virtual void                ClearDetail()
                                    { m_strDetail = ""; }
        virtual const std::string & GetStackTrace() const
                                    { return m_strStackTrace; }
        virtual void                SetStackTrace(const std::string & s)
                                    { m_strStackTrace = s; }
        virtual void                ClearStackTrace()
                                    { m_strStackTrace = ""; }

                                    // send down the file and line of the code that caught the exception
        void                        LogCritical(const std::string& strCaughtBy) const;
        void                        LogError(const std::string& strCaughtBy) const;
        void                        LogWarning(const std::string& strCaughtBy) const;
        void                        LogState(const std::string& strCaughtBy) const;
        void                        LogTrace(const std::string& strCaughtBy) const;
        void                        Console(const std::string& strCaughtBy) const;

    private:

        std::string                 m_strClass;
        std::string                 m_strThrownBy;
        std::string                 m_strDetail;
        std::string                 m_strStackTrace;

                                    // disallow default construction / copy constructors
                                    Exception();
                                    //Exception(const Exception &);
    void                            operator=(const Exception &);

};
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#define DECLARE_LIBTHROCKET_EXCEPTION_CLASS(ns,t)                                                                          \
namespace ns                                                                                                        \
{                                                                                                                   \
class t##Exception          :   public libthrocket::Exception                                                              \
{                                                                                                                   \
    public:                                                                                                         \
        t##Exception(const std::string& strClass, const std::string& strThrownBy, const std::string& strDetail) :   \
            Exception(std::string(#ns) + "::" + strClass + "Exception", strThrownBy, strDetail)                     \
        {}                                                                                                          \
        t##Exception(const std::string& strThrownBy, const std::string& strDetail = "")    :                        \
            Exception(std::string(#ns) + "::" + __FUNCTION__, strThrownBy, strDetail)                               \
        {}                                                                                                          \
        virtual ~t##Exception()                                                                                     \
        {}                                                                                                          \
};                                                                                                                  \
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#define DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(ns,t,st)                                                                    \
namespace ns                                                                                                        \
{                                                                                                                   \
class t##st##Exception  :   public ns::t##Exception                                                                 \
{                                                                                                                   \
    public:                                                                                                         \
        t##st##Exception(const std::string& strThrownBy, const std::string& strDetail = "")    :                    \
            t##Exception(#t#st, strThrownBy, strDetail)                                                             \
        {}                                                                                                          \
        virtual ~t##st##Exception()                                                                                 \
        {}                                                                                                          \
};                                                                                                                  \
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
namespace libthrocket
{
class StatusCodeException : public libthrocket::Exception
{
public:
                    StatusCodeException(const std::string & strClass, const std::string & strThrownBy,
                                        int nStatusCode, const std::string & strDetail) :
                        Exception(strClass, strThrownBy, strDetail),
                        m_nStatusCode(nStatusCode)
                    {}

    int             GetStatusCode() const
                    { return m_nStatusCode; }

private:
    int             m_nStatusCode;
};
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#define DECLARE_LIBTHROCKET_STATUS_CODE_EXCEPTION_SUBCLASS(ns,t)                                                           \
namespace ns                                                                                                        \
{                                                                                                                   \
class t##Exception          :   public libthrocket::StatusCodeException                                                    \
{                                                                                                                   \
    public:                                                                                                         \
        t##Exception(const std::string& strClass, const std::string& strThrownBy,                                   \
                     int nStatusCode, const std::string& strDetail)    :                                            \
            libthrocket::StatusCodeException(std::string(#ns) + "::" + strClass, strThrownBy, nStatusCode, strDetail)      \
        {}                                                                                                          \
        t##Exception(const std::string& strThrownBy, int nStatusCode, const std::string& strDetail = "")    :       \
            libthrocket::StatusCodeException(std::string(#ns) + "::" + __FUNCTION__, strThrownBy, nStatusCode, strDetail)  \
        {}                                                                                                          \
};                                                                                                                  \
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#define LIBTHROCKET_THROWN_BY (std::string(__FUNCTION__) + "@" + std::string(__FILE__) + ":" + std::to_string(__LINE__))
#define LIBTHROCKET_CAUGHT_BY (std::string(__FUNCTION__) + "@" + std::string(__FILE__) + ":" + std::to_string(__LINE__))

//============================================================================================================================= 132
