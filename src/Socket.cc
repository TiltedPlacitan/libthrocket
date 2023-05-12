//============================================================================================================================= 132
//
//  Socket.cc
//
//      Roll up all good socket programming wisdom gleaned over the years into C++ classes.
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
#include "Socket.h"

using namespace std;

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
inline int64_t TimeuS64(struct timeval* ptv = NULL)
{
    struct timeval tvNow;
    int64_t        t64MicroTime;

    if (ptv == NULL)
    {
        ptv = &tvNow;
        ::gettimeofday(ptv, NULL);
    }
    t64MicroTime = ptv->tv_sec;
    t64MicroTime *= 1000000L;
    t64MicroTime += ptv->tv_usec;

    return t64MicroTime;
}
//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#ifdef WIN32

#define EINPROGRESS     WSAEWOULDBLOCK

typedef int socklen_t;

typedef int (FAR PASCAL *tTransferProc)(int, void*, int, int);

bool                            Socket::m_bWSAStartup   =   false;
libthrocket::Mutex                     Socket::m_csWSAStartup;

#else   // WIN32

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>

#define GetLastError() errno
#define closesocket(s) close(s)
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)

typedef int (*tTransferProc)(int, void*, int, int);

#endif  // WIN32

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#ifdef WIN32

static const string
libthrocket::SocketErrorString(int nErr)
{
    char                        acMsg[128];
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, nErr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) acMsg, sizeof(acMsg), NULL );
    return acMsg;
}

#else   // WIN32

static const string
SocketErrorString(int nErr)
{
    return strerror(nErr);
}

#endif  // WIN32

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
Resolv(const string& strHostname)
{
    // if an IP address cannot be parsed, assume we already have a hostname
    string                      strIPAddr               =   strHostname;
    static libthrocket::Mutex          csResolv;
    uint32_t                    u32IPAddr;
    struct hostent*             phent;

    libthrocket::Lock                  l(&csResolv);

    u32IPAddr = inet_addr(strHostname.c_str());
    if (u32IPAddr == (uint32_t) -1)
    {
        phent = gethostbyname(strHostname.c_str());
        if (phent != NULL)
        {
            if (phent->h_length == sizeof(u32IPAddr))
            {
                memcpy(&u32IPAddr, phent->h_addr, sizeof(u32IPAddr));
                strIPAddr = libthrocket::InetSocket::AddrString(u32IPAddr, 0);
            } else
            {
                int             nSaveErrno              =   GetLastError();
                throw libthrocket::ResolvFamilyException(LIBTHROCKET_THROWN_BY, "gethostbyname " + strHostname + " " +
                                            std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
            }
        } else
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::ResolvLookupException(LIBTHROCKET_THROWN_BY, "gethostbyname " + strHostname + " " +
                                        std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
        }
    } else
    {
        // NADA
    }

   return strIPAddr;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::GlobalInit()
{
    #ifdef WIN32

        libthrocket::Lock              l(&m_csWSAStartup);
        WSADATA                     wsadata;

        if (m_bWSAStartup == false)
        {
            if (WSAStartup (MAKEWORD (2, 2), &wsadata) != 0)
            {
                int             nSaveErrno              =   GetLastError();
                throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "WSAStartup " + std::to_string(nSaveErrno) +
                                                          " (" + SocketErrorString(nSaveErrno) + ")");
            }
            m_bWSAStartup = true;
        }

    #endif  // WIN32
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::Init()
{
    GlobalInit();

    #ifdef WIN32

        m_hWSAEvent = WSACreateEvent();
        if (m_hWSAEvent == WSA_INVALID_EVENT)
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "WSACreateEvent " + std::to_string(nSaveErrno) + " (" +
                                                      SocketErrorString(nSaveErrno) + ")");
        }

    #endif  // WIN32
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
libthrocket::Socket::~Socket()
{
    #ifdef WIN32

        WSACloseEvent(m_hWSAEvent);

    #endif  // WIN32

    LockedClose();
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
// static: get the hostname that this computer thinks it is
const string
libthrocket::Socket::GetHostname()
{
    static libthrocket::Mutex          CSHostname;
    static string               strHostname;
    libthrocket::Lock                  l(&CSHostname);

    if (strHostname.length() == 0)
    {
        std::unique_ptr<char[]> up_hostname(new char[HOST_NAME_MAX + 1]);
        gethostname(up_hostname.get(), HOST_NAME_MAX);
        up_hostname.get()[HOST_NAME_MAX] = '\0';
        strHostname = string(up_hostname.get());
    }
    return strHostname;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//  Create a human-readable representation of a poll() revents bitmask
#ifdef WIN32

const string
libthrocket::Socket::PollReventsString(short revents)
{
    return "";
}

#else   // WIN32

const string
libthrocket::Socket::PollReventsString(short revents)
{
    string                      strEvents;
    short                       sMysteryEvents;

    strEvents = "";

    if ((revents & POLLIN)      == POLLIN)      { strEvents += " IN";   }
    if ((revents & POLLPRI)     == POLLPRI)     { strEvents += " PRI";  }
    if ((revents & POLLOUT)     == POLLOUT)     { strEvents += " OUT";  }
    if ((revents & POLLERR)     == POLLERR)     { strEvents += " ERR";  }
    if ((revents & POLLHUP)     == POLLHUP)     { strEvents += " HUP";  }
    if ((revents & POLLNVAL)    == POLLNVAL)    { strEvents += " NVAL"; }

    sMysteryEvents = revents & ~(POLLIN | POLLPRI | POLLOUT | POLLERR | POLLHUP | POLLNVAL);

    if (sMysteryEvents != 0)
    {
        LOGWARNING("mystery events %04X", sMysteryEvents);
    }

    return strEvents;
}

#endif  // WIN32

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::SetSendTimeout(int64_t i64Timeout)
{
    libthrocket::Lock                  l(&m_CSLocal);

    if (i64Timeout < 0)
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, string("i64Timeout ") + std::to_string(i64Timeout));

    m_i64SendTimeout = i64Timeout;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::SetRecvTimeout(int64_t i64Timeout)
{
    libthrocket::Lock                  l(&m_CSLocal);

    if (i64Timeout < 0)
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, string("i64Timeout ") + std::to_string(i64Timeout));

    m_i64RecvTimeout = i64Timeout;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::Select(bool bWantRead, bool bWantWrite, int64_t i64Timeout)
{
    if (i64Timeout < 1)
        return;

    struct pollfd               pfd[1];
    int64_t                     i64TimeBegin            =   TimeuS64();
    int64_t                     i64Latency              =   0;
    int64_t                     i64TimeRemaining        =   i64Timeout;
    do
    {
        pfd[0].fd = m_nSocket;
        pfd[0].events = POLLNVAL;
        if (bWantRead)
            pfd[0].events |= POLLIN; // | POLLPRI | POLLRDHUP;
        if (bWantWrite)
            pfd[0].events |= POLLOUT; // | POLLHUP;

        int                     nRC;
        nRC = poll(pfd, 1, i64TimeRemaining / 1000);
        i64Latency = TimeuS64() - i64TimeBegin;

        if (nRC > 0)
        {
            break;

        } else if (nRC == -1)
        {
            int                     nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "select: (" + LockedGetPeerAddrString() + ") " +
                                        std::to_string(i64Latency) + "/" + std::to_string(i64Timeout) + " uS " +
                                        std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");

        } else if (nRC == 0)
        {
            if (i64Latency >= i64Timeout)
                throw libthrocket::SocketTimeoutException(LIBTHROCKET_THROWN_BY, "select: (" + LockedGetPeerAddrString() + ") " +
                                        std::to_string(i64Latency) + "/" + std::to_string(i64Timeout) + " uS timeout");

            LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
                "SCK> wait: %d (%21s) %ld/%ld SHORT TIMEOUT", 
                m_nSocket, LockedGetPeerAddrString().c_str(), i64Latency, i64Timeout);

            i64TimeRemaining = i64Timeout - i64Latency;

        } else
        {
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "select: (" + LockedGetPeerAddrString() + ") " +
                                        std::to_string(i64Latency) + "/" + std::to_string(i64Timeout) + " wild " +
                                        std::to_string(nRC));
        }

    } while (i64Latency < i64Timeout);

    bool                        bCheckErr               =   true;
    if (bWantRead && (pfd[0].revents & POLLIN) != 0)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
            "SCK> wait: %d (%21s) %ld/%ld read ready", 
            m_nSocket, LockedGetPeerAddrString().c_str(), i64Latency, i64Timeout);
        // reading from an error'd socket may be OK
        bCheckErr = false;
    }

    if (bWantWrite && (pfd[0].revents & POLLOUT) != 0)
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
            "SCK> wait: %d (%21s) %ld/%ld write ready",
            m_nSocket, LockedGetPeerAddrString().c_str(),
            i64Latency, i64Timeout);
        // do not let someone have the idea that they can write to an error'd socket
        //bCheckErr = false;
    }

    if (bCheckErr != false && (pfd[0].revents & POLLERR) != 0)
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "select: (" + LockedGetPeerAddrString() + ") " +
                                        std::to_string(i64Latency) + "/" + std::to_string(i64Timeout) + " uS socket error");
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::SetFD(int nSocket)
{
    if (m_nSocket != INVALID_SOCKET)
        throw libthrocket::SocketInitException(LIBTHROCKET_THROWN_BY, "m_nSocket != INVALID_SOCKET");

    m_nSocket = nSocket;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::LockedClose()
{
    if (m_nSocket != INVALID_SOCKET)
    {
        // log first here
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
            "SCK> clos: %d", 
            m_nSocket);

        closesocket(m_nSocket);
        m_nSocket = INVALID_SOCKET;
    }
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::Socket::LockedWait(bool bWantRead, bool bWantWrite, int64_t i64Timeout)
{
    if (bWantRead == false && bWantWrite == false)
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, "don't want read or write");
    if (i64Timeout < 0)
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, "negatimeout");

    if (i64Timeout < 0)
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, "negatimeout");

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK> wait: %d (%21s)%s%s TO %ld uS",
        LockedGetFD(), LockedGetPeerAddrString().c_str(),
        bWantRead  != false ? " read"  : "",
        bWantWrite != false ? " write" : "",
        i64Timeout);

    if (i64Timeout > 0)
    {
        // we are supposed to be called locked
        // we do not wish to block other threads when we're just blocking on select
        m_CSLocal.unlock();
        {
            try
            {
                Select(bWantRead, bWantWrite, i64Timeout);
            }
            catch (const libthrocket::Exception & e)
            {
                m_CSLocal.lock();
                throw;
            }
        }
        m_CSLocal.lock();
    }
}

//-------------------------------3-----------------------3--------------------------------------------------------------------- 132
//
void
libthrocket::Socket::LockedSetNonBlocking()
{
#ifdef WIN32

    if (WSAEventSelect(m_nSocket, m_hWSAEvent, FD_READ | FD_WRITE | FD_CLOSE) != 0)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "WSAEventSelect " + std::to_string(nSaveErrno) +
                                                  " (" + SocketErrorString(nSaveErrno) + ")");
    }

#else   // WIN32

    int                         nFlags;

    nFlags = fcntl(m_nSocket, F_GETFL);
    if (nFlags != -1)
    {
        nFlags |= O_NONBLOCK;
        if (fcntl(m_nSocket, F_SETFL, nFlags) == -1)
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "fcntl set " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
        }
    } else
    {
        int                 nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "fcntl get " + std::to_string(nSaveErrno) +
                                                  " (" + SocketErrorString(nSaveErrno) + ")");
    }

#endif

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK>       %d (%21s) set non-blocking",
        LockedGetFD(), LockedGetPeerAddrString().c_str());
}

//-------------------------------3-----------------------3--------------------------------------------------------------------- 132
//
void
libthrocket::Socket::LockedSetBlocking(bool bBlocking)
{
#ifdef WIN32

    throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "LockedSetBlocking not implemented on WIN32");

#else
    if (bBlocking != false)
    {
        int                     nFlags;

        nFlags = fcntl(m_nSocket, F_GETFL);
        if (nFlags != -1)
        {
            nFlags &= ~O_NONBLOCK;
            if (fcntl(m_nSocket, F_SETFL, nFlags) == -1)
            {
                int             nSaveErrno              =   GetLastError();
                throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "fcntl set " + std::to_string(nSaveErrno) +
                                                          " (" + SocketErrorString(nSaveErrno) + ")");
            }
        } else
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "fcntl get " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
        }
    } else
    {
        LockedSetNonBlocking();
    }
#endif  // WIN32

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK>       %d (%21s) set blocking",
        LockedGetFD(), LockedGetPeerAddrString().c_str());
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::InetSocket::LockedOpen()
{
    if (m_nSocket == INVALID_SOCKET)
    {
        m_nSocket = socket(PF_INET, m_nSocketType, 0);
        if (m_nSocket == INVALID_SOCKET)
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "socket " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
        }
    }

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK> open: %d (%21s) got fd",
        LockedGetFD(), LockedGetPeerAddrString().c_str());
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::InetSocket::LockedBind(const string& strIPAddr, uint16_t u16Port, int nListenLen)
{
    LockedOpen();

    if (m_strIPAddr == "")
    {
        LockedReuseAddr();

        struct sockaddr_in      sin;
        sin.sin_family      =   PF_INET;
        sin.sin_port        =   htons(u16Port);
        sin.sin_addr.s_addr =   inet_addr(strIPAddr.c_str());
        if (bind(m_nSocket, (struct sockaddr*) &sin, sizeof(sin)) != 0)
        {
            int                 nSaveErrno              =   GetLastError();
            closesocket(m_nSocket);
            throw libthrocket::SocketBindException(LIBTHROCKET_THROWN_BY, "bind " + AddrString(strIPAddr, u16Port) + " " +
                                      std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
        }

        struct sockaddr         sin_bound;
        socklen_t               slen                    =   sizeof(sin_bound);
        if (getsockname(m_nSocket, &sin_bound, &slen) != 0)
        {
            int                 nSaveErrno              =   GetLastError();
            closesocket(m_nSocket);
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "getsockname " + AddrString(strIPAddr, u16Port) + " " +
                                     std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
        }

        if (m_nSocketType == SOCK_STREAM && listen(m_nSocket, nListenLen) != 0)
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY,    "listen " + AddrString(strIPAddr, u16Port) + " " +
                                                   std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ')');
        }

        m_strIPAddr =   strIPAddr;
        //m_u16Port =   u16Port;
        m_u16Port = ntohs(((struct sockaddr_in*) &sin_bound)->sin_port);
    }

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK> bind: %d (%21s) bound",
        LockedGetFD(), LockedGetLocalAddrString().c_str());
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
// use on listen socket to immediately release port binding upon close
void
libthrocket::InetSocket::LockedReuseAddr()
{
    #ifdef WIN32

        // WIN32 doesn't seem to need this

    #else   // WIN32
        int                         nEnabled                =   1;

        if (setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, &nEnabled, sizeof(nEnabled)) != 0)
        {
            int                     nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "setsockopt FD " + std::to_string(m_nSocket) + " SO_REUSEADDR " +
                                    std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
        }
    #endif  // WIN32

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK>       %d (%21s) reuse addr",
        LockedGetFD(), LockedGetPeerAddrString().c_str());
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::IPAddrString(uint32_t u32IPAddr)
{
    uint32_t                    hl;
    string                      strAddr;

    hl = htonl(u32IPAddr);
    strAddr  = std::to_string((hl & 0xFF000000) >> 24);
    strAddr += '.';
    strAddr += std::to_string((hl & 0x00FF0000) >> 16);
    strAddr += '.';
    strAddr += std::to_string((hl & 0x0000FF00) >>  8);
    strAddr += '.';
    strAddr += std::to_string((hl & 0x000000FF));

    return strAddr;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::AddrString(uint32_t u32IPAddr, uint16_t u16Port)
{
    string                      strAddr;
    strAddr = IPAddrString(u32IPAddr);
    if (u16Port != 0)
    {
        strAddr += ':';
        strAddr += std::to_string(u16Port);
    }
    return strAddr;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::AddrString(const string& strIPAddr, uint16_t u16Port)
{
    return strIPAddr + ":" + std::to_string(u16Port);
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::InetSocket::LockedGetEncodedLocalIP()
{
    struct sockaddr_in          sin;
    socklen_t                   slen                    =   sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    if (getsockname(m_nSocket, (struct sockaddr*) &sin, &slen) == -1)
        return 0;
    return sin.sin_addr.s_addr;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::LockedGetLocalIPString()
{
    string                      strIP                   =   AddrString(LockedGetEncodedLocalIP());
    return strIP;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint16_t
libthrocket::InetSocket::LockedGetDecodedLocalPort()
{
    struct sockaddr_in          sin;
    socklen_t                   slen                    =   sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    if (getsockname(m_nSocket, (struct sockaddr*) &sin, &slen) == -1)
        return 0;
    return ntohs(sin.sin_port);
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::LockedGetLocalPortString()
{
    uint16_t                    u16Port                 =   LockedGetDecodedLocalPort();
    string                      strPort                 =   std::to_string(u16Port);
    return strPort;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::InetSocket::LockedGetLocalAddrString()
{
    return LockedGetLocalIPString() + ':' + LockedGetLocalPortString();
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::UDPSocket::LockedSend(in_addr_t inaIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes)
{
    int                         nRC                     =   0;
    bool                        bWantRead               =   false;
    bool                        bWantWrite              =   true;

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "UDP> send: %d (%21s) %u bytes TO %ld uS", 
        LockedGetFD(), AddrString(inaIPAddr, u16Port).c_str(), u32Bytes, LockedGetSendTimeout());

    LockedWait(bWantRead, bWantWrite, m_i64SendTimeout);

    struct sockaddr_in          sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = inaIPAddr;
    sin.sin_port = htons(u16Port);
    #ifdef WIN32
        nRC = sendto(LockedGetFD(), (char*) pu8Bytes, u32Bytes, 0, (struct sockaddr*) &sin, sizeof(sin));
    #else   // WIN32
        nRC = sendto(LockedGetFD(),         pu8Bytes, u32Bytes, 0, (struct sockaddr*) &sin, sizeof(sin));
    #endif  // WIN32

    if (nRC == -1)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "send: " + std::to_string(LockedGetFD()) + " " + 
                                                  AddrString(inaIPAddr, u16Port) + " " + std::to_string(nSaveErrno) +
                                                  " (" + SocketErrorString(nSaveErrno) + ")");

    } else if ((uint32_t) nRC != u32Bytes)
    {
        LOGWARNING("UDP> send: %d (%21s) %u bytes send mismatch %d bytes", 
            LockedGetFD(), AddrString(inaIPAddr, u16Port).c_str(), u32Bytes, nRC);

    } else
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
            "UDP> send: %d (%21s) %d bytes", 
            LockedGetFD(), AddrString(inaIPAddr, u16Port).c_str(), nRC);
    }

    return (uint32_t) nRC;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::UDPSocket::LockedSend(const string& strIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes)
{
    return LockedSend(inet_addr(strIPAddr.c_str()), u16Port, pu8Bytes, u32Bytes);
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::UDPSocket::LockedRecv(in_addr_t& inaIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes)
{
    int                         nRC                     =   0;
    bool                        bWantRead               =   true;
    bool                        bWantWrite              =   false;

    inaIPAddr = 0;
    u16Port = 0;

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "UDP> recv: %d (%21s) %u bytes TO %ld uS", 
        LockedGetFD(), LockedGetLocalAddrString().c_str(), u32Bytes, LockedGetSendTimeout());

    LockedWait(bWantRead, bWantWrite, m_i64RecvTimeout);

    struct sockaddr_in          sin;
    memset(&sin, 0, sizeof(sin));
    socklen_t                   slen                    =   sizeof(sin);
    #ifdef WIN32
        nRC = recvfrom(LockedGetFD(), (char*) pu8Bytes, u32Bytes, 0, (struct sockaddr*) &sin, &slen);
    #else   // WIN32
        nRC = recvfrom(LockedGetFD(),         pu8Bytes, u32Bytes, 0, (struct sockaddr*) &sin, &slen);
    #endif  // WIN32

    if (nRC == -1)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "recv: " + std::to_string(LockedGetFD()) + " " + 
                                                  LockedGetLocalAddrString() + " " + std::to_string(nSaveErrno) +
                                                  " (" + SocketErrorString(nSaveErrno) + ")");

    } else
    {
        LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
            "UDP> recv: %d (%21s) %d bytes", 
            LockedGetFD(), LockedGetLocalAddrString().c_str(), nRC);
    }

    inaIPAddr = sin.sin_addr.s_addr;
    u16Port = htons(sin.sin_port);

    return (uint32_t) nRC;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::UDPSocket::LockedRecv(string& strIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes)
{
    in_addr_t                   inaIPAddr;
    uint32_t                    u32RC                   =   LockedRecv(inaIPAddr, u16Port, pu8Bytes, u32Bytes);
    strIPAddr = AddrString(inaIPAddr);
    return u32RC;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::UDPSocket::LockedBroadcast()
{
    int                         nEnabled                =   1;

#ifdef WIN32

    if (setsockopt(m_nSocket, SOL_SOCKET, SO_BROADCAST, (const char*) &nEnabled, sizeof (nEnabled)) != 0)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "setsockopt FD " + std::to_string(m_nSocket) + " SO_BROADCAST " +
                                                  std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
    }

#else   // WIN32

    if (setsockopt(m_nSocket, SOL_SOCKET, SO_BROADCAST, &nEnabled, sizeof (nEnabled)) != 0)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "setsockopt FD " + std::to_string(m_nSocket) + " SO_BROADCAST " + 
                                                  std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
    }

#endif  // WIN32
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::TCPSocket::LockedNoNagle()
{
    int                         nEnabled                =   1;

#ifdef WIN32

    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, (const char*) &nEnabled, sizeof (nEnabled)) != 0)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "setsockopt FD " + std::to_string(m_nSocket) + " TCP_NODELAY " +
                                                  std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
    }

#else   // WIN32

    if (setsockopt(m_nSocket, IPPROTO_TCP, TCP_NODELAY, &nEnabled, sizeof (nEnabled)) != 0)
    {
        int                     nSaveErrno              =   GetLastError();
        throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, "setsockopt FD " + std::to_string(m_nSocket) + " TCP_NODELAY " + 
                                                  std::to_string(nSaveErrno) + " (" + SocketErrorString(nSaveErrno) + ")");
    }

#endif  // WIN32
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::TCPSocket::LockedGetEncodedPeerIP()
{
    struct sockaddr_in          sin;
    socklen_t                   slen                    =   sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    if (getpeername(m_nSocket, (struct sockaddr*) &sin, &slen) == -1)
        return 0;
    return sin.sin_addr.s_addr;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint16_t
libthrocket::TCPSocket::LockedGetDecodedPeerPort()
{
    struct sockaddr_in          sin;
    socklen_t                   slen                    =   sizeof(sin);
    memset(&sin, 0, sizeof(sin));
    if (getpeername(m_nSocket, (struct sockaddr*) &sin, &slen) == -1)
        return 0;
    return ntohs(sin.sin_port);
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::TCPSocket::LockedGetPeerIPString()
{
    string                      strIP                   =   AddrString(LockedGetEncodedPeerIP());
    return strIP;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::TCPSocket::LockedGetPeerPortString()
{
    uint16_t                    u16Port                 =   LockedGetDecodedPeerPort();
    string                      strPort                 =   std::to_string(u16Port);
    return strPort;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const string
libthrocket::TCPSocket::LockedGetPeerAddrString()
{
    return LockedGetPeerIPString() + ':' + LockedGetPeerPortString();
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
void
libthrocket::TCPSocket::LockedConnect
(
    const string&               strIPAddr,
    uint16_t                    u16Port
)
{
    if (m_nSocket != INVALID_SOCKET)
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "m_nSocket != INVALID_SOCKET");

    m_nSocket = socket(PF_INET, SOCK_STREAM, 0);
    if (m_nSocket == INVALID_SOCKET)
    {
        int                     nSaveErrno;
        nSaveErrno = GetLastError();
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "socket: " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
    }

    try
    {
        LockedSetNonBlocking();
    }
    catch (const libthrocket::Exception & e)
    {
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, e.GetDetail());
    }

    struct sockaddr_in          sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = inet_addr(strIPAddr.c_str());
    sin.sin_port = htons(u16Port);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "TCP> conn: %d (%21s)", 
        LockedGetFD(), AddrString(sin.sin_addr.s_addr, htons(sin.sin_port)).c_str());

    if (connect(m_nSocket, (struct sockaddr*) &sin, sizeof(sin)) == SOCKET_ERROR)
    {
        int                     nSaveErrno;
        nSaveErrno = GetLastError();
        if (nSaveErrno != EINPROGRESS)
            throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "connect: " + std::to_string(nSaveErrno) +
                                                          " (" + SocketErrorString(nSaveErrno) + ")");
    }

    // non-blocking async connect completion
    try
    {
        Select(false/*bWantRead*/, true/*bWantWrite*/, m_i64SendTimeout);
        m_bConnected = true;
    }
    catch (const libthrocket::Exception & e)
    {
        // just throw?
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, e.GetDetail());
    }

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "TCP> conn: %d (%21s) success", 
        LockedGetFD(), LockedGetPeerAddrString().c_str());
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::TCPSocket::LockedTransfer(bool bDirection, uint8_t* pu8Bytes, uint32_t u32Bytes, bool bShort)
{
    int                         nRC                     =   0;
    const char*                 pcFunc                  =   NULL;
    tTransferProc               pfFunc                  =   NULL;
    bool                        bWantRead               =   false;
    bool                        bWantWrite              =   false;
    uint32_t                    u32BytesTransferred     =   0;
    int64_t                     i64Now;
    int64_t                     i64Timeout;
    int64_t                     i64Expire;

    if (bDirection == SOCKET_TRANSFER_SEND)
    {
        pcFunc = (const char*) "send";
        pfFunc = (tTransferProc) send;
        bWantRead  = false;
        bWantWrite = true;
        i64Timeout = LockedGetSendTimeout();

    } else if (bDirection == SOCKET_TRANSFER_RECV)
    {
        pcFunc = (const char*) "recv";
        pfFunc = (tTransferProc) recv;
        bWantRead  = true;
        bWantWrite = false;
        i64Timeout = LockedGetRecvTimeout();

    } else
    {
        throw libthrocket::SocketParamException(LIBTHROCKET_THROWN_BY, "invalid transfer type");
    }

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "TCP> %s: %d (%21s) %u bytes TO %ld uS", 
        pcFunc, LockedGetFD(), LockedGetPeerAddrString().c_str(), u32Bytes, i64Timeout);

    i64Now = TimeuS64();
    i64Expire = i64Now + i64Timeout;
    while (1)
    {
        #ifdef WIN32
        #else   // WIN32
            if (bDirection == SOCKET_TRANSFER_RECV && WOULDDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH) != false)
            {
                int                 nCount;
                if (ioctl(LockedGetFD(), FIONREAD, &nCount) == 0)
                {
                    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
                        "TCP> recv: %d (%21s) FIONREAD %d", 
                        LockedGetFD(), LockedGetPeerAddrString().c_str(), nCount);
                }
            }
        #endif

        LockedWait(bWantRead, bWantWrite, i64Expire - i64Now);

        nRC = pfFunc(LockedGetFD(), pu8Bytes, u32Bytes, 0);

        if (nRC == 0)
        {
            break;

        } else if (nRC < 1)
        {
            int                 nSaveErrno              =   GetLastError();
            throw libthrocket::SocketSysException(LIBTHROCKET_THROWN_BY, string(pcFunc) + " " + std::to_string(LockedGetFD()) + " " + 
                                                      LockedGetPeerAddrString() + " " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");

        } else
        {
            LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
                "TCP> %s: %d (%21s) %d bytes", 
                pcFunc, LockedGetFD(), LockedGetPeerAddrString().c_str(), nRC);
            u32Bytes            -=  nRC;
            u32BytesTransferred +=  nRC;
            pu8Bytes            +=  nRC;
        }

        if (bShort || u32Bytes < 1)
            break;

        i64Now = TimeuS64();
        if (i64Now >= i64Expire)
            throw libthrocket::SocketTimeoutException(LIBTHROCKET_THROWN_BY, string(pcFunc) + " " + std::to_string(LockedGetFD()) + " " + 
                                                    LockedGetPeerAddrString() + " timeout");
    }

    return u32BytesTransferred;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::TCPSocket::LockedRecvAll(uint8_t* pu8Bytes, uint32_t u32Bytes)
{
    uint32_t                    u32BytesRecvd           =   0;
    uint32_t                    u32BytesTotal           =   0;

    while (u32Bytes > 0)
    {
        u32BytesRecvd = LockedTransfer(SOCKET_TRANSFER_RECV, pu8Bytes, u32Bytes, false/*bShort*/);
        if (u32BytesRecvd == 0)
            break;
        pu8Bytes      += u32BytesRecvd;
        u32Bytes      -= u32BytesRecvd;
        u32BytesTotal += u32BytesRecvd;
    }

    return u32BytesTotal;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
libthrocket::TCPSocket *
libthrocket::TCPAcceptSocket::LockedAccept(int64_t i64AcceptTimeout)
{
    fd_set                      fdsRead;
    FD_ZERO(&fdsRead);
    FD_SET(m_nSocket, &fdsRead);
    fd_set                      fdsWrite;
    FD_ZERO(&fdsWrite);
    FD_SET(m_nSocket, &fdsWrite);
    fd_set                      fdsExcept;
    FD_ZERO(&fdsExcept);
    FD_SET(m_nSocket, &fdsExcept);
    struct timeval              tv;
    tv.tv_sec  = (int32_t) (i64AcceptTimeout / (1000 * 1000));
    tv.tv_usec = (int32_t) (i64AcceptTimeout % (1000 * 1000));
    int                     nRC;
    nRC = select(m_nSocket + 1, &fdsRead, &fdsWrite, &fdsExcept, &tv);
    if (nRC == 0)
        throw libthrocket::SocketTimeoutException(LIBTHROCKET_THROWN_BY, "select");
    if (nRC == -1)
    {
        int nSaveErrno = GetLastError();
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "select: " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
    }
    if (!FD_ISSET(m_nSocket, &fdsRead))
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "select: listening socket is not readable");

    struct sockaddr saddr;
    memset(&saddr, 0, sizeof(saddr));
    socklen_t addrlen = sizeof(saddr);
    int nFD = accept(m_nSocket, &saddr, &addrlen);
    if (nFD == -1)
    {
        int nSaveErrno = GetLastError();
        throw libthrocket::SocketConnectException(LIBTHROCKET_THROWN_BY, "accept: " + std::to_string(nSaveErrno) +
                                                      " (" + SocketErrorString(nSaveErrno) + ")");
    }
    TCPSocket * pSock = new TCPSocket(nFD, m_i64RecvTimeout, m_i64SendTimeout);

    LOGDEBUG(ADL_DMSK_SCK, ADL_DLVL_HIGH,
        "SCK> acpt: %d (%21s)",
        pSock->GetFD(), pSock->GetPeerAddrString().c_str());

    return pSock;
}

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
uint32_t
libthrocket::HTTPClient::Request
(
    const uint8_t*      pu8Request,
    uint32_t            u32RequestLen,
    uint8_t*            pu8Response,
    uint32_t            u32ResponseLen
)
{
    try
    {
        Send(pu8Request, u32RequestLen);
    }
    catch (const libthrocket::Exception & e)
    {
        throw libthrocket::HTTPClientSendException(LIBTHROCKET_THROWN_BY, e.GetDetail());
    }

    return RecvAll(pu8Response, u32ResponseLen);
}

//============================================================================================================================= 132
