//============================================================================================================================= 132
//
//  Socket.h
//
//      Roll up all good socket programming wisdom gleaned over the years into C++ classes.
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

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#ifdef WIN32
typedef uint32_t in_addr_t;
#else   // WIN32
	#include <arpa/inet.h>
	//#include <netinet/in.h>
    #include <sys/socket.h>
	#include <sys/time.h>
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET (-1)
#endif  // WIN32

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
#include "Exception.h"
#include "ThreadMinimal.h"

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
DECLARE_LIBTHROCKET_EXCEPTION_CLASS(libthrocket,Socket)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Sys)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Bind)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Init)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Param)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Connect)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Socket,Timeout)

DECLARE_LIBTHROCKET_EXCEPTION_CLASS(libthrocket,Resolv)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Resolv,Lookup)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,Resolv,Family)

DECLARE_LIBTHROCKET_EXCEPTION_CLASS(libthrocket,HTTPClient)
DECLARE_LIBTHROCKET_EXCEPTION_SUBCLASS(libthrocket,HTTPClient,Send)

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
// handshake/transfer directions
#define SOCKET_HANDSHAKE_CLIENT true
#define SOCKET_HANDSHAKE_SERVER false
#define SOCKET_TRANSFER_RECV    true
#define SOCKET_TRANSFER_SEND    false

namespace libthrocket
{

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
const std::string Resolv(const std::string& strHostname);

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class Socket
{
    public:
                                Socket
                                (
                                    int                 nSocket,
                                    int                 nSocketType,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    m_nSocket(nSocket),
                                    m_nSocketType(nSocketType),
                                    m_i64RecvTimeout(i64RecvTimeout),
                                    m_i64SendTimeout(i64SendTimeout)
                                { Init(); }

                                Socket
                                (
                                    int                 nSocketType,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    m_nSocket(INVALID_SOCKET),
                                    m_nSocketType(nSocketType),
                                    m_i64RecvTimeout(i64RecvTimeout),
                                    m_i64SendTimeout(i64SendTimeout)
                                { Init(); }

        virtual                 ~Socket();

        static void             GlobalInit();
        virtual void            Init();

        virtual void            Close()
                                { libthrocket::Lock l(&m_CSLocal); LockedClose(); }
        virtual void            Wait(bool bWantRead, bool bWantWrite, int64_t i64Timeout)
                                { libthrocket::Lock l(&m_CSLocal); LockedWait(bWantRead, bWantWrite, i64Timeout); }

        virtual void            SetRecvTimeout(int64_t i64RecvTimeout);
        virtual int64_t         GetRecvTimeout()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetRecvTimeout(); }

        virtual void            SetSendTimeout(int64_t i64SendTimeout);
        virtual int64_t         GetSendTimeout()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetSendTimeout(); }

        virtual void            SetFD(int nSocket);
        virtual int             GetFD()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetFD(); }

        void                    Select(bool bWantRead, bool bWantWrite, int64_t i64Timeout);

        void                    SetNonBlocking()
                                { libthrocket::Lock l(&m_CSLocal); LockedSetNonBlocking(); }
        void                    SetBlocking(bool bBlocking = true)
                                { libthrocket::Lock l(&m_CSLocal); LockedSetBlocking(bBlocking); }

        static const std::string GetHostname();
        static const std::string PollReventsString(short revents);

    protected:

        libthrocket::Mutex             m_CSLocal;
        int                     m_nSocket;
        int                     m_nSocketType;
        int64_t                 m_i64RecvTimeout;
        int64_t                 m_i64SendTimeout;

        virtual void            LockedClose();
        virtual void            LockedWait(bool bWantRead, bool bWantWrite, int64_t i64Timeout);
        virtual int64_t         LockedGetRecvTimeout() const
                                { return m_i64RecvTimeout; }
        virtual int64_t         LockedGetSendTimeout() const
                                { return m_i64SendTimeout; }
        virtual int             LockedGetFD() const
                                { return m_nSocket; }
        virtual void            LockedSetNonBlocking();
        virtual void            LockedSetBlocking(bool bBlocking);
        virtual const std::string LockedGetPeerAddrString()
                                { return ""; }

        #ifdef WIN32

            WSAEVENT            m_hWSAEvent;
            static libthrocket::Mutex  m_csWSAStartup;
            static bool         m_bWSAStartup;

        #endif  // WIN32

                                // disallow default construction / copy constructors
                                Socket();
                                Socket(const Socket &);
        void                    operator=(const Socket &);
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class InetSocket            :   public Socket
{
    public:
                                InetSocket
                                (
                                    int                 nSocket,
                                    int                 nSocketType,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    Socket(nSocket, nSocketType, i64RecvTimeout, i64SendTimeout)
                                {}
                                
                                InetSocket
                                (
                                    int                 nSocketType,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    Socket(nSocketType, i64RecvTimeout, i64SendTimeout)
                                {}
        virtual                 ~InetSocket()
                                {}

        static const std::string IPAddrString(uint32_t u32IPAddr);
        static const std::string AddrString(uint32_t u32IPAddr, uint16_t u16Port = 0);
        static const std::string AddrString(const std::string& strIPAddr, uint16_t u16Port = 0);
		static bool				ValidIPv4Addr(const std::string & strIPAddr)
								{ return inet_addr(strIPAddr.c_str()) != (in_addr_t) -1; }
		static bool				ValidIPv4Port(int32_t i32Port)
								{ return i32Port >= 0 && i32Port <= 0xFFFF; }

        virtual void            Open()
                                { libthrocket::Lock l(&m_CSLocal); LockedOpen(); }
        virtual void            Bind(const std::string& strIPAddr, uint16_t u16Port, int nListenLen = 0)
                                { libthrocket::Lock l(&m_CSLocal); LockedBind(strIPAddr, u16Port, nListenLen); }

        virtual uint16_t        GetDecodedLocalPort()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetDecodedLocalPort(); }
        virtual const std::string GetLocalIPString()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetLocalIPString(); }
        virtual const std::string GetLocalAddrString()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetLocalAddrString(); }

    protected:

        virtual void            LockedOpen();
        virtual void            LockedBind(const std::string& strIPAddr, uint16_t u16Port, int nListenLen = 0);
        virtual void            LockedReuseAddr();

        virtual uint32_t        LockedGetEncodedLocalIP();
        virtual uint16_t        LockedGetDecodedLocalPort();
        virtual const std::string LockedGetLocalIPString();
        virtual const std::string LockedGetLocalPortString();
        virtual const std::string LockedGetLocalAddrString();

    private:

        std::string             m_strIPAddr;
        uint16_t                m_u16Port;

                                // disallow default construction / copy constructors
                                InetSocket();
                                InetSocket(const InetSocket &);
        void                    operator=(const InetSocket &);
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class UDPSocket             :   public InetSocket
{
    public:
                                UDPSocket
                                (
                                    int                 nSocket,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    InetSocket(nSocket, SOCK_DGRAM, i64RecvTimeout, i64SendTimeout)
                                {}
                                
                                UDPSocket
                                (
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    InetSocket(SOCK_DGRAM, i64RecvTimeout, i64SendTimeout)
                                {}
        virtual                 ~UDPSocket()
                                {}

        virtual uint32_t        Send(const std::string& strIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedSend(strIPAddr, u16Port, pu8Bytes, u32Bytes); }
        virtual uint32_t        Recv(std::string& strIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedRecv(strIPAddr, u16Port, pu8Bytes, u32Bytes); }
        virtual uint32_t        Send(in_addr_t inaIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedSend(inaIPAddr, u16Port, (uint8_t*) pu8Bytes, u32Bytes); }
        virtual uint32_t        Recv(in_addr_t& inaIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedRecv(inaIPAddr, u16Port, pu8Bytes, u32Bytes); }

        virtual void            Broadcast() // call this if you're going to be doing mcast or bcast Send()s
                                { libthrocket::Lock l(&m_CSLocal); LockedBroadcast(); }


    protected:

        virtual uint32_t        LockedSend(const std::string& strIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes);
        virtual uint32_t        LockedRecv(std::string& strIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes);
        virtual uint32_t        LockedSend(in_addr_t inaIPAddr, uint16_t u16Port, const uint8_t* pu8Bytes, uint32_t u32Bytes);
        virtual uint32_t        LockedRecv(in_addr_t& inaIPAddr, uint16_t& u16Port, uint8_t* pu8Bytes, uint32_t u32Bytes);

        virtual void            LockedBroadcast();

    private:
                                // disallow default construction / copy constructors
                                UDPSocket();
                                UDPSocket(const UDPSocket &);
        void                    operator=(const UDPSocket &);
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class TCPSocket             :   public InetSocket
{
    public:
                                TCPSocket
                                (
                                    int                 nSocket,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    InetSocket(nSocket, SOCK_STREAM, i64RecvTimeout, i64SendTimeout),
                                    m_bConnected(false)
                                {}
                                
                                TCPSocket
                                (
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    InetSocket(SOCK_STREAM, i64RecvTimeout, i64SendTimeout),
                                    m_bConnected(false)
                                {}
        virtual                 ~TCPSocket()
                                {}

        virtual void            Connect(const std::string& strIPAddr, uint16_t u16Port)
                                { libthrocket::Lock l(&m_CSLocal); LockedConnect(strIPAddr, u16Port); }
        virtual void            Disconnect()
                                { libthrocket::Lock l(&m_CSLocal); LockedDisconnect(); }

        virtual uint32_t        Send(const uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedTransfer(SOCKET_TRANSFER_SEND, (uint8_t*) pu8Bytes, u32Bytes, false/*bShort*/); }
        virtual uint32_t        Recv(uint8_t* pu8Bytes, uint32_t u32Bytes, bool bShort = false)
                                { libthrocket::Lock l(&m_CSLocal); return LockedTransfer(SOCKET_TRANSFER_RECV, pu8Bytes, u32Bytes, bShort); }
        virtual uint32_t        RecvAll(uint8_t* pu8Bytes, uint32_t u32Bytes)
                                { libthrocket::Lock l(&m_CSLocal); return LockedRecvAll(pu8Bytes, u32Bytes); }

        virtual uint16_t        GetDecodedPeerPort()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetDecodedPeerPort(); }
        virtual const std::string GetPeerIPString()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetPeerIPString(); }
        virtual const std::string GetPeerAddrString()
                                { libthrocket::Lock l(&m_CSLocal); return LockedGetPeerAddrString(); }
        virtual void            NoNagle()
                                { libthrocket::Lock l(&m_CSLocal); LockedNoNagle(); }

        virtual bool            IsConnected() const
                                { return m_nSocket != -1 && m_bConnected; }

    protected:

        virtual void            LockedConnect(const std::string& strIPAddr, uint16_t u16Port);
        virtual void            LockedDisconnect()
                                { m_bConnected = false; Socket::LockedClose(); }

        virtual uint32_t        LockedTransfer(bool bDirection, uint8_t* pu8Bytes, uint32_t u32Bytes, bool bShort);
        virtual uint32_t        LockedRecvAll(uint8_t* pu8Bytes, uint32_t u32Bytes);

        virtual uint32_t        LockedGetEncodedPeerIP();
        virtual uint16_t        LockedGetDecodedPeerPort();
        virtual const std::string LockedGetPeerIPString();
        virtual const std::string LockedGetPeerPortString();
        virtual const std::string LockedGetPeerAddrString();
        virtual void            LockedNoNagle();

    private:

        bool                    m_bConnected;

                                // disallow default construction / copy constructors
                                TCPSocket();
                                TCPSocket(const TCPSocket &);
        void                    operator=(const TCPSocket &);
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class TCPAcceptSocket		:	public InetSocket
{
	public:
								TCPAcceptSocket
                                (
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    InetSocket(SOCK_STREAM, i64RecvTimeout, i64SendTimeout)
                                {}
		virtual					~TCPAcceptSocket()
								{}

		virtual TCPSocket *		Accept(int64_t i64AcceptTimeout)
								{ libthrocket::Lock l(&m_CSLocal); return LockedAccept(i64AcceptTimeout); }						

	protected:

		virtual TCPSocket *		LockedAccept(int64_t i64AcceptTimeout);

    private:
                                // disallow default construction / copy constructors
                                TCPAcceptSocket();
                                TCPAcceptSocket(const TCPAcceptSocket &);
        void                    operator=(const TCPAcceptSocket &);
};

//------------------------------=-----------------------=---------------------------------------------------------------------- 132
//
class HTTPClient            :   public TCPSocket
{
    public:
                                HTTPClient
                                (
                                    const std::string&  strIPAddr,
                                    int16_t             u16Port,
                                    int64_t             i64RecvTimeout,
                                    int64_t             i64SendTimeout
                                )   :
                                    TCPSocket(i64RecvTimeout, i64SendTimeout),
                                    m_strIPAddr(strIPAddr),
                                    m_u16Port(u16Port)
                                {}
        virtual                 ~HTTPClient()
                                {}


        virtual void            Connect()
                                { TCPSocket::Connect(m_strIPAddr, m_u16Port); }

        virtual uint32_t        Request
                                (
                                    const uint8_t*      pu8Request,
                                    uint32_t            u32RequestLen,
                                    uint8_t*            pu8Response,
                                    uint32_t            u32ResponseLen
                                );

    private:

        std::string             m_strIPAddr;
        uint16_t                m_u16Port;

                                // disallow default construction / copy constructors
                                HTTPClient();
                                HTTPClient(const HTTPClient &);
        void                    operator=(const HTTPClient &);
};

};  // namespace libthrocket

//============================================================================================================================= 132
