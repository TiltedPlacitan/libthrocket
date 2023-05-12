//============================================================================================================================= 132
//
//  ThreadMinimal.cc
//
//      Implementations for various POSIX threads wrappers.
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

#include <cassert>
#include <iostream>

#include "AlarmDebugLog.h"
#include "Exception.h"
#include "ThreadMinimal.h"

using namespace std;

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
// friend of Thread
// expects to be called with a pointer to a Thread as arg
void * libthrocket::Thread::ProcWrapThread(void * pvArg)
{
    libthrocket::Thread * pThread = (Thread *) pvArg;

    {
        libthrocket::Lock l((pThread->mCSLocal));
        pThread->mbStarted      = true;
        pThread->mbStillRunning = true;
        pThread->mStarted.signal();
    }

    try
    {
        // run the routine
        // you SHOULD be catching your exceptions
        // ...but if you don't we catch everything we know about below...
        pThread->Run();
    }
    catch (const libthrocket::Exception & e)
    {
        e.Console(LIBTHROCKET_CAUGHT_BY);
    }
    catch (const std::exception & e)
    {
        cerr << "ProcWrapThread: std::exception: " << e.what() << endl;
    }
    catch (...)
    {
        cerr << "ProcWrapThread: wild exception" << endl;
    }

    {
        Lock l((pThread->mCSLocal));
        pThread->mbStillRunning = false;
    }

    return NULL;
}

// --------------------------------------------------------------------------------------------------------------------------------
//
void libthrocket::ThreadMother::ReapChildren()
{
    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: entry", __PRETTY_FUNCTION__);

    libthrocket::Lock l(m_lockThreadMother);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: locked", __PRETTY_FUNCTION__);

    std::list<libthrocket::Thread *> keep_me;
    std::list<libthrocket::Thread *> delete_me;
    std::list<libthrocket::Thread *>::iterator iter;
    for (iter = m_childrenThreadMother.begin(); iter != m_childrenThreadMother.end(); iter++)
    {
        if (!(*iter)->GetStillRunning())
        {
            delete_me.push_back(*iter);
        }
        else
        {
            keep_me.push_back(*iter);
        }
    }

    for (iter = delete_me.begin(); iter != delete_me.end(); iter++)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p wait", *iter);
        (*iter)->wait();
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p delt", *iter);
        delete *iter;
    }

    m_childrenThreadMother.swap(keep_me);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: exit", __PRETTY_FUNCTION__);
}

// --------------------------------------------------------------------------------------------------------------------------------
//
void libthrocket::ThreadMother::Infanticide()
{
    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: entry", __PRETTY_FUNCTION__);

    libthrocket::Lock l(m_lockThreadMother);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: locked", __PRETTY_FUNCTION__);

    std::list<libthrocket::Thread *>::iterator iter;
    // request stop to all child threads.
    for (iter = m_childrenThreadMother.begin(); iter != m_childrenThreadMother.end(); iter++)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p stop", *iter);
        (*iter)->SetStopRequested();
    }
    // reap all child threads.
    for (iter = m_childrenThreadMother.begin(); iter != m_childrenThreadMother.end(); iter++)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p wait", *iter);
        (*iter)->wait();
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p delt", *iter);
        delete *iter;
    }

    m_childrenThreadMother.clear();

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: exit", __PRETTY_FUNCTION__);
}

// --------------------------------------------------------------------------------------------------------------------------------
// Synchronously/iteratively send a message to all child threads without requiring that the message be copied...
void libthrocket::ThreadMother::SendAllChildren(libthrocket::ThreadMessage * m)
{
    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: entry", __PRETTY_FUNCTION__);

    libthrocket::Lock l(m_lockThreadMother);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: locked", __PRETTY_FUNCTION__);

    libthrocket::ThreadQueue q_notify;
    m->SetQRespond(&q_notify);

    std::list<libthrocket::Thread *>::iterator iter;
    // request stop to all child threads.
    for (iter = m_childrenThreadMother.begin(); iter != m_childrenThreadMother.end(); iter++)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "thr %p iter", *iter);
        (*iter)->Queue(m);
        q_notify.get();
        
    }

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH, "%s: exit", __PRETTY_FUNCTION__);
}

//============================================================================================================================= 132
