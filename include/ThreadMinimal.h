//=================================================================================================================================
//
//  ThreadMinimal.h
//
//      Declarations for various POSIX threads wrappers.
//
//  COLUMNS 132 TABSTOP 4 SPACE-FILL
//
//=================================================================================================================================

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

//---------------------------------------------------------------------------------------------------------------------------------
//
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <list>
#include <queue>
#include <string>

#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

//---------------------------------------------------------------------------------------------------------------------------------
// default stack - best to specify your own value for constructor
#define THREAD_DEFAULT_STACK    (4 * 1024 * 1024)

namespace libthrocket
{

//---------------------------------------------------------------------------------------------------------------------------------
//
class Lockable
{
public:
                                Lockable() {}
    virtual                     ~Lockable() {}

    virtual void                lock()                              =   0;
    virtual void                unlock()                            =   0;

                                // disallow default construction / copy constructors
                                //Lockable();
                                Lockable(const Lockable &);
    void                        operator=(const Lockable &);
};

//---------------------------------------------------------------------------------------------------------------------------------
// instantiate this as an auto variable with a pointer to a valid Lockable and when
// this will create a scope-controlled critical section.  i.e. when the scope of
// the auto variable (the Lock) is pop'd, the lock will automatically be released.
class Unlock;
class Lock
{
friend class Unlock;
public:
                                Lock(Lockable & m) :
                                    mm(m)
                                { mm.lock(); }
                                Lock(Lockable * pm) :
                                    mm(*pm)
                                { mm.lock(); }
    virtual                     ~Lock()
                                { mm.unlock(); }

private:
    Lockable                  & mm;

                                // disallow default construction / copy constructors
                                Lock();
                                Lock(const Lock &);
    void                        operator=(const Lock &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class Unlock
{
public:
                                Unlock(Lock & l)    :
                                    m_Lock(l)
                                { m_Lock.mm.unlock(); }
    virtual                     ~Unlock()
                                { m_Lock.mm.lock(); }

private:
    Lock                      & m_Lock;

                                // disallow default construction / copy constructors
                                Unlock();
                                Unlock(const Unlock &);
    void                        operator=(const Unlock &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class Mutex                 :   public Lockable
{
public:
                                Mutex()
                                {
                                    int rc = pthread_mutex_init(&m_Mutex, NULL);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_mutex_init: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    virtual                     ~Mutex()
                                { 
                                    int rc = pthread_mutex_destroy(&m_Mutex);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_mutex_destroy: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }

    virtual void                lock()
                                {
                                    int rc = pthread_mutex_lock(&m_Mutex);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_mutex_lock: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    virtual void                unlock()
                                {
                                    int rc = pthread_mutex_unlock(&m_Mutex);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_mutex_unlock: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }

    friend                      class Condition;

private:
    pthread_mutex_t             m_Mutex;

                                // disallow default construction / copy constructors
                                //Mutex();
                                Mutex(const Mutex &);
    void                        operator=(const Mutex &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class Condition
{
public:
                                Condition()
                                {
                                    int rc = pthread_cond_init(&m_Cond, NULL);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_cond_init: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
                                ~Condition()
                                {
                                    int rc = pthread_cond_destroy(&m_Cond);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_cond_destroy: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }

                                enum eBlockReturns
                                {
                                    eBlockSignalled =    0,
                                    eBlockTimedOut  =    1
                                };

    void                        signal()                                                        //< wake up blocked thread
                                {
                                    int rc = pthread_cond_signal(&m_Cond);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_cond_signal: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    void                        block(Mutex * pMutex)                                           //< indefinite
                                {
                                    int rc = pthread_cond_wait(&m_Cond, &(pMutex->m_Mutex));
                                    if (rc == 0) return;
                                    std::cerr << "pthread_cond_wait: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    enum eBlockReturns          blockTimed(Mutex * pMutex, const struct timespec * pAbstime)    //< absolute time
                                {
                                    int rc = pthread_cond_timedwait(&m_Cond, &(pMutex->m_Mutex), pAbstime);
                                    if (rc == ETIMEDOUT) return eBlockTimedOut;
                                    if (rc == 0)         return eBlockSignalled;
                                    std::cerr << "pthread_cond_timedwait: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    enum eBlockReturns          blockTimed(Mutex * pMutex, uint32_t u32USec)                    //< interval
                                {
                                    struct timeval              tv;
                                    gettimeofday(&tv, NULL);
                                    tv.tv_sec  += u32USec / 1000000;
                                    tv.tv_usec += u32USec % 1000000;
                                    struct timespec             ts;
                                    if (tv.tv_usec >= 1000000)
                                    {
                                        ts.tv_sec  = tv.tv_sec + 1;
                                        ts.tv_nsec = (tv.tv_usec - 1000000) * 1000;
                                    }
                                    else
                                    {
                                        ts.tv_sec  = tv.tv_sec;
                                        ts.tv_nsec = tv.tv_usec * 1000;
                                    }
                                
                                    return blockTimed(pMutex, &ts);
                                }
                                                                                                // and again for Mutex references
    void                        block(Mutex & mutex)                                            //< indefinite
                                { block(&mutex); }
    enum eBlockReturns          blockTimed(Mutex & mutex, const struct timespec * pAbstime)     //< absolute time
                                { return blockTimed(&mutex, pAbstime); }
    enum eBlockReturns          blockTimed(Mutex & mutex, uint32_t uSec)                        //< interval
                                { return blockTimed(&mutex, uSec); }

private:
   pthread_cond_t               m_Cond;

                                // disallow default construction / copy constructors
                                //Condition();
                                Condition(const Condition &);
    void                        operator=(const Condition &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class ThreadQueue;

class ThreadMessage
{
public:
                                ThreadMessage() :
                                    m_q_respond(NULL)
                                {
                                    m_buf    = NULL;
                                    clear();
                                }
    virtual                     ~ThreadMessage()
                                {
                                    clear();
                                }

    virtual void                clear()
                                {
                                    m_ID = 0;
                                    m_q_respond = NULL;
                                    m_bool[0] = false;
                                    m_bool[1] = false;
                                    m_str[0].clear();
                                    m_str[1].clear();
                                    m_int[0] = 0;
                                    m_int[1] = 0;
                                    m_dbl[0] = 0.0;
                                    m_dbl[1] = 0.0;
                                    if (m_buf)
                                        delete [] m_buf;
                                    m_buf = NULL;
                                    m_buf_len = 0;
                                }

    virtual void                SetID(uint32_t ID)
                                { m_ID = ID; }
    virtual uint32_t            GetID()
                                { return m_ID; }
    virtual void                SetBool(size_t idx, bool b)
                                { m_bool[idx] = b; }
    virtual bool                GetBool(size_t idx)
                                { return m_bool[idx]; }
    virtual void                SetStr(size_t idx, const std::string & s)
                                { m_str[idx] = s; }
    virtual void                SwapStr(size_t idx, std::string & s)
                                { m_str[idx].swap(s); }
    virtual std::string &       GetStr(size_t idx)
                                { return m_str[idx]; }
    virtual void                SetInt(size_t idx, size_t i)
                                { m_int[idx] = i; }
    virtual size_t              GetInt(size_t idx)
                                { return m_int[idx]; }
    virtual void                SetFlt(size_t idx, float d)
                                { m_flt[idx] = d; }
    virtual float               GetFlt(size_t idx)
                                { return m_flt[idx]; }
    virtual void                SetDbl(size_t idx, double d)
                                { m_dbl[idx] = d; }
    virtual double              GetDbl(size_t idx)
                                { return m_dbl[idx]; }
    virtual void                SetBuf(const uint8_t * p, size_t l)
                                { m_buf = new uint8_t [l]; memcpy(m_buf, p, l); m_buf_len = l; }
    virtual const uint8_t     * GetBuf()
                                { return m_buf; }
    virtual size_t              GetBufLen()
                                { return m_buf_len; }
    virtual void                SetQRespond(ThreadQueue * q)
                                { m_q_respond = q; }
    virtual ThreadQueue       * GetQRespond()
                                { return m_q_respond; }

protected:
    ThreadQueue               * m_q_respond;
    uint32_t                    m_ID;
    bool                        m_bool[2];
    std::string                 m_str[2];
    size_t                      m_int[2];
    float                       m_flt[2];
    double                      m_dbl[2];
    uint8_t                   * m_buf;
    size_t                      m_buf_len;

private:

                                // disallow default construction / copy constructors
                                //ThreadMessage();
                                ThreadMessage(const ThreadMessage &);
    void                        operator=(const ThreadMessage &);

};

//---------------------------------------------------------------------------------------------------------------------------------
//
template<typename T> class ThreadMessagePtr : public ThreadMessage
{
public:
                                ThreadMessagePtr()  :
                                    m_p(NULL)
                                {}
                                ThreadMessagePtr(T * p) :
                                    m_p(p)
                                {}
    virtual                     ~ThreadMessagePtr()
                                {
                                    if (m_p)
                                        delete m_p;
                                }

    virtual void                SetPtr(T * p)
                                { m_p = p; }
    virtual T *                 GetPtr()
                                { return m_p; }

private:
    T *                         m_p;

                                // disallow default construction / copy constructors
                                //ThreadMessagePtr();
                                ThreadMessagePtr(const ThreadMessagePtr &);
    void                        operator=(const ThreadMessagePtr &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
template<typename T> class ThreadMessageArr : public ThreadMessage
{
public:
                                ThreadMessageArr()  :
                                    m_p(NULL)
                                {}
                                ThreadMessageArr(T * p) :
                                    m_p(p)
                                {}
    virtual                     ~ThreadMessageArr()
                                {
                                    if (m_p)
                                        delete [] m_p;
                                }

    virtual void                SetArr(T * p)
                                { m_p = p; }
    virtual T *                 GetArr()
                                { return m_p; }

private:
    T *                         m_p;

                                // disallow default construction / copy constructors
                                //ThreadMessageArr();
                                ThreadMessageArr(const ThreadMessageArr &);
    void                        operator=(const ThreadMessageArr &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class ThreadQueue
{
public:
                                ThreadQueue()
                                {}
    virtual                     ~ThreadQueue()
                                {
                                    while (m_q.size() > 0)
                                    {
                                        ThreadMessage * msg = get();
                                        delete msg;
                                    }
                                }

    virtual void                put(ThreadMessage * msg)
                                {
                                    Lock l(m_mutex);
                                    m_q.push(msg);
                                    m_cond.signal();
                                }
                                // non-blocking message queue read
                                // returns NULL if queue is empty
    virtual ThreadMessage     * getNonBlocking()
                                {
                                    Lock l(m_mutex);
                                    if (m_q.size() > 0)
                                    {
                                        ThreadMessage * msg = m_q.front();
                                        m_q.pop();
                                        return msg;
                                    }
                                    return NULL;
                                }
                                // blocking message queue read
    virtual ThreadMessage     * get()
                                {
                                    Lock l(m_mutex);
                                    while (1)
                                    {
                                        if (m_q.size() > 0)
                                        {
                                            ThreadMessage * msg = m_q.front();
                                            m_q.pop();
                                            return msg;
                                        }
                                        m_cond.block(m_mutex);
                                    }
                                }
                                // timed message queue read
                                // u32USec == 0 means DO-NOT-BLOCK
    virtual ThreadMessage     * getTimed(uint32_t u32USec = 0)
                                {
                                    Lock l(m_mutex);
                                    while (1)
                                    {
                                        if (m_q.size() > 0)
                                        {
                                            ThreadMessage * msg = m_q.front();
                                            m_q.pop();
                                            return msg;
                                        }
                                        // we have to wait
                                        if (!u32USec)
                                            return NULL;
                                        // timed block.
                                        // timeout means return NULL ThreadMessage
                                        if (m_cond.blockTimed(m_mutex, u32USec) == Condition::eBlockTimedOut)
                                            return NULL;
                                    }
                                }
                                // absolute-timed message queue read
    virtual ThreadMessage     * getTimed(struct timespec * pts)
                                {
                                    Lock l(m_mutex);
                                    while (1)
                                    {
                                        if (m_q.size() > 0)
                                        {
                                            ThreadMessage * msg = m_q.front();
                                            m_q.pop();
                                            return msg;
                                        }
                                        // timed block.
                                        if (m_cond.blockTimed(m_mutex, pts) == Condition::eBlockTimedOut)
                                            return NULL;
                                    }
                                }
                                // obtain size of queue
    virtual size_t              size()
                                {
                                    Lock l(m_mutex);
                                    return m_q.size();
                                }

private:
    Mutex                       m_mutex;
    Condition                   m_cond;
    std::queue<ThreadMessage *> m_q;

                                // disallow default construction / copy constructors
                                //ThreadQueue();
                                ThreadQueue(const ThreadQueue &);
    void                        operator=(const ThreadQueue &);
};

//---------------------------------------------------------------------------------------------------------------------------------
// subclass this
class Thread
{
public:
                                Thread(uint32_t u32StackSize = THREAD_DEFAULT_STACK)    :
                                    mu32StackSize(u32StackSize)
                                {
                                    Reset();
                                    // XXX - jbates - NOTE: stack size is not placed in mAttr!!!
                                    int rc = pthread_attr_init(&mAttr);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_attr_init: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }
    virtual                     ~Thread()
                                {
                                    int rc = pthread_attr_destroy(&mAttr);
                                    if (rc == 0) return;
                                    std::cerr << "pthread_attr_destroy: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }

    void                        Reset()
                                {
                                    mbStarted       = false;
                                    mbStillRunning  = false;
                                    mbStopRequested = false;
                                    mbJoined        = false;
                                }

    void                        go()
                                {
                                    {
                                        Lock l(mCSLocal);
                                        assert(mbStarted == false);
                                        assert(mbStillRunning == false);
                                        assert(mbStopRequested == false);
                                        assert(mbJoined == false);
                                        Reset();
                                        int rc = pthread_create(&mID, &mAttr, ProcWrapThread, this);
                                        if (rc != 0)
                                        {
                                            std::cerr << "pthread_create: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                            abort();
                                        }
                                        while (1)
                                        {
                                            mStarted.block(mCSLocal);
                                            if (mbStarted)
                                                break;
                                        }
                                    }
                                }
    void                        wait()
                                {
                                    {
                                        Lock l(mCSLocal);
                                        assert(mbStarted == true);
                                        // assert(mbStillRunning == ?
                                        // assert(mbStopRequested == ?
                                        assert(mbJoined == false);
                                    }
                                    int rc = pthread_join(mID, NULL);
                                    {
                                        Lock l(mCSLocal);
                                        mbJoined = true;
                                    }
                                    if (rc == 0) return;
                                    std::cerr << "pthread_join: rc: " << rc << " " << std::strerror(rc) << std::endl;
                                    abort();
                                }

    pthread_t                   GetID()
                                { Lock l(mCSLocal); return mID; }
    bool                        GetStarted()
                                { Lock l(mCSLocal); return mbStarted; }
    bool                        GetStillRunning()
                                { Lock l(mCSLocal); return mbStillRunning; }
    void                        SetStopRequested()
                                { Lock l(mCSLocal); mbStopRequested = true; }
    bool                        GetStopRequested()
                                { Lock l(mCSLocal); return mbStopRequested; }

    void                        Queue(ThreadMessage * m)
                                { mQ.put(m); }

protected:

    ThreadQueue                 mQ;

    ThreadMessage             * DeQueueNonBlocking()
                                { return mQ.getNonBlocking(); }
    ThreadMessage             * DeQueue()
                                { return mQ.get(); }
    ThreadMessage             * DeQueueTimed(uint32_t u32USec)
                                { return mQ.getTimed(u32USec); }
    ThreadMessage             * DeQueueTimed(struct timespec * pts)
                                { return mQ.getTimed(pts); }

                                // implement this
    virtual void                Run() = 0;

private:
    pthread_t                   mID;
    Mutex                       mCSLocal;
    Condition                   mStarted;
    uint32_t                    mu32StackSize;
    volatile bool               mbStarted;
    volatile bool               mbStillRunning;
    volatile bool               mbStopRequested;
    volatile bool               mbJoined;

    static void               * ProcWrapThread(void * pvArg);
    pthread_attr_t              mAttr;

                                // disallow default construction / copy constructors
                                //Thread();
                                Thread(const Thread &);
    void                        operator=(const Thread &);
};

//---------------------------------------------------------------------------------------------------------------------------------
//
class ThreadMother
{
public:
    virtual size_t              GetNumChildren()
                                { Lock l(m_lockThreadMother); return m_childrenThreadMother.size(); }

                                ThreadMother() {}
    virtual                     ~ThreadMother() {}

    virtual Thread *            ChildBirth(Thread * t)
                                {
                                    Lock l(m_lockThreadMother);
                                    return LockedChildBirth(t);
                                }

    virtual void                Infanticide();
    virtual void                ReapChildren();
    virtual void                SendAllChildren(ThreadMessage * m);

protected:
    virtual Thread *            LockedChildBirth(Thread * t)
                                { m_childrenThreadMother.push_back(t); return t; }

    virtual Mutex             & GetMutexChildren()
                                { return m_lockThreadMother; }
    virtual std::list<Thread*>& GetListChildren()
                                { return m_childrenThreadMother; }

private:
    Mutex                       m_lockThreadMother;
    std::list<Thread *>         m_childrenThreadMother;

                                // disallow default construction / copy constructors
                                //ThreadMother();
                                ThreadMother(const ThreadMother &);
    void                        operator=(const Thread &);
};

};  // namespace libthrocket

//=================================================================================================================================
