//-----------------------------------------------------------------------------
// Torque Game Engine 
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

//!!!!!!!!! NOTE !!!!!!!!!
// Since the platform layers are currently separate, the Mac implementation takes
// extreme advantage of knowing that all values are already in 'network order', and
// that a byte array of an ip address is the same byte order as a U32 ip address.


#include "Platform/types.h"
#if defined(TORQUE_OS_MAC_OSX)
//#include <OpenTransportProtocol.h>
//#include <OpenTransportProviders.h>
#define IP_BROADCAST   kIP_BROADCAST
#define IP_REUSEADDR   kIP_REUSEADDR
#else
#include <OpenTptInternet.h>
#endif

#include "PlatformMacCarb/platformMacCarb.h"
#include "Platform/platform.h"
#include "Platform/event.h"
#include "console/console.h"
#include "Platform/gameInterface.h"
#include "Core/fileStream.h"



struct Connection
{
   NetSocket socket;
   S32 state;
   S32 prevState;
   bool listenConnection;
   Connection *nextConnection;
   Connection *nextInTable;
// HANDLE connectThreadHandle;
};

struct Status
{
   bool mSignal;
   OSErr mErr;
   void *mData;
   Status()
   { 
      mSignal = false;
      mErr = kOTNoError;
      mData = NULL;
   }
};


// main control vars.
static Net::Error getLastError();
static S32 defaultPort = 28000;
static S32 netPort = 0;


static U32 nextConnectionId = 1;
static U32 nextAcceptId = 1000000000;
static Connection *connectionList = NULL;
enum {
   ConnectionTableSize = 256,
   ConnectionTableMask = 0xFF
};
static Connection *connectionTable[ConnectionTableSize] = { 0, };


// OpenTransport init controls.
static U32 OTReferenceCount = 0;


//======================================================================
// NEW MAC GLOBALS
//======================================================================
// structures we want/need to hang around.
static EndpointRef mainEndpoint = NULL;
static InetAddress mainPortAddr, destPortAddr;
static TBind tbRequest, tbBound;
static TUnitData tSend, tRecv;
static OTFlags tFlags = 0L;


// error holders.
static OSErr macErr = kOTNoError;
static const OSStatus kStatusFine = 0L;
static OSStatus macStatus = kStatusFine;
static OTResult macResult = kOTNoError;

#define HOSTFROMBYTES(a)   (*((InetHost*)(a)))
static const InetHost broadcastIP = 0xFFFFFFFF;
static const InetHost loopbackIP = 0x7F000001; // 127.0.0.1


//======================================================================
static pascal void EventProc( void *_pSession, OTEventCode event, OTResult result, void *cookie )
{
   Status *status = (Status *) _pSession;

   switch ( event )
   {
      case T_DATA:
         //pSess->m_dataToRead = 1;
         break;
   
      case T_BINDCOMPLETE:
         status->mErr = (OSErr) result;
         status->mSignal = true;
         break;   

      case T_OPENCOMPLETE:
         status->mErr = (OSErr) result;
         status->mData = cookie;  // the EndPoint
         status->mSignal = true;
         break;
   }
}



//======================================================================
bool Net::init()
{
   OSStatus err;
   if ( OTReferenceCount == 0 )
   {
      err = InitOpenTransportInContext(kInitOTForApplicationMask, NULL);
      if (err != kOTNoError)
         return false;
   }

#if 0   // this helps validate the byte ordering of host ips in a byte array.
   static InetHost testloop = 0;
   static U8 test2[4];
   OTInetStringToHost("127.0.2.1", &testloop);
   OTInetStringToHost("127.0.2.1", (InetHost*)test2);
#endif

   // successfully opened Open Transport, inncrement the reference count      
   OTReferenceCount++;
   return true;
}


//======================================================================
void Net::shutdown()
{
   while(connectionList)
      Net::closeConnectTo(connectionList->socket);

   // make sure that we shut down the main open port.      
   closePort();
   if ( OTReferenceCount <= 0 )
      return;
   
   // close Open Transport if there are no more references to it
   OTReferenceCount--;   
   if ( OTReferenceCount == 0 )
      CloseOpenTransportInContext(NULL);   
}


//======================================================================
static void netToIPSocketAddress(const NetAddress *address, InetAddress *inAddr)
{
   dMemset(inAddr, 0, sizeof(InetAddress));
   inAddr->fAddressType = AF_INET;
   inAddr->fHost = HOSTFROMBYTES(address->netNum);
   inAddr->fPort = address->port;
}


//======================================================================
static void IPSocketToNetAddress(const InetAddress *inAddr, NetAddress *address)
{
   address->type = NetAddress::IPAddress;
   address->port = inAddr->fPort;
   HOSTFROMBYTES(address->netNum) = inAddr->fHost;
}


//======================================================================
bool Net::openPort(S32 port)
{
   // check if we have an endpoint already.
   if (mainEndpoint != NULL) // just unbind it - no need to close & recreate...
      OTUnbind(mainEndpoint);
   else // need to create a new endpoint.
   {
      mainEndpoint = OTOpenEndpointInContext(OTCreateConfiguration(kUDPName), 0L, NULL, &macStatus, NULL);
      if (mainEndpoint == NULL)
      {
         // !!!!!!TBD add error case.
         // macStatus may need to be converted...
         return false;
      }
   }
   
   // then, bind the endpoint to the specified address.

   // first, clear the bind value struct.
   tbBound.qlen = 0;
   tbBound.addr.maxlen = 0;
   tbBound.addr.len = 0;
   tbBound.addr.buf = NULL;

   // then, setup the bind request struct to point to a port addr struct.
   tbRequest.qlen = 4;
   tbRequest.addr.maxlen = sizeof(InetAddress);   
   tbRequest.addr.len = sizeof(InetAddress);
   tbRequest.addr.buf = (U8*)(&mainPortAddr);

   // then, fill the port address structure for the given port.
   // this will bind to whichever net device it first finds.  !!!!!TBD????
   OTInitInetAddress(&mainPortAddr, port, kOTAnyInetAddress);

   // then do the bind.  this is sync, obv.
   macStatus = OTBind(mainEndpoint, &tbRequest, &tbBound);   
   if (macStatus != kStatusFine)
   {
      return false;
   }

   // now that we're bound, need to 'negotiate' for capabilities.
   U8 omBuffer[kOTFourByteOptionSize];
   TOptMgmt   omRequest;
   TOptMgmt   omReply;
   // alias the buffer for cleaner structure access.
   TOption *omOpts = (TOption*)omBuffer;

   // set up the request and reply structures.
   omRequest.flags = T_NEGOTIATE;
   omRequest.opt.len = kOTFourByteOptionSize;
   omRequest.opt.buf   = omBuffer;

   omReply.opt.maxlen = kOTFourByteOptionSize;
   omReply.opt.buf = omBuffer;

   // main thing we need is BROADCAST cap.   
   omOpts->len = kOTFourByteOptionSize;
   omOpts->status = 0;
   omOpts->level = INET_IP; // we're dealing with IP protocols.
   omOpts->name = IP_BROADCAST; // we want BROADCAST.
   *(U32*)(omOpts->value) = TRUE; // we want broadcast to be ENABLED.

   macStatus = OTOptionManagement(mainEndpoint, &omRequest, &omReply);
   if(macStatus != kStatusFine)
   {
      // need to look deeper to see what really happened.
      if(omOpts->status != T_SUCCESS)
      {
         // !!!!!TBD convert/handle the error!
      }

      return false;
   }

   // we also want REUSE_ADDR
   omOpts->len = kOTFourByteOptionSize;
   omOpts->status = 0;
   omOpts->level = INET_IP; // we're dealing with IP protocols.
   omOpts->name = IP_REUSEADDR; // we want REUSE_ADDR.
   *(U32*)(omOpts->value) = TRUE; // we want broadcast to be ENABLED.

   macStatus = OTOptionManagement(mainEndpoint, &omRequest, &omReply);
   if(macStatus != kStatusFine)
   {
      // need to look deeper to see what really happened.
      if(omOpts->status != T_SUCCESS)
      {
         // !!!!!TBD convert/handle the error!
      }

      return false;
   }

   // we're ready.  hold onto assigned portnum in a global...   
   netPort = port;
   
   return true;
}


//======================================================================
void Net::closePort()
{
   // check if we have an endpoint.
   if (mainEndpoint != NULL)
   {
      // for now, unbind it
      OTUnbind(mainEndpoint);
      netPort = 0;
      
      // we'll try this for nicer cleanup...
      OTCloseProvider(mainEndpoint);
      mainEndpoint = NULL;
   }
}


//======================================================================
Net::Error Net::sendto(const NetAddress *address, const U8 *buffer, S32 bufferSize)
{
   if(Game->isJournalReading())
      return NoError;

   if (mainEndpoint == NULL)
      return NoError; // !!!!!TBD????

   // clear any send struct fields that should be clean...
   tSend.opt.len = 0;

   // setup the send buffer.
   tSend.udata.maxlen = bufferSize;
   tSend.udata.len = bufferSize;
   tSend.udata.buf = (U8*)buffer;

   // setup the dest addr struct.
   tSend.addr.maxlen = sizeof(InetAddress);
   tSend.addr.len = sizeof(InetAddress);
   tSend.addr.buf = (U8*)(&mainPortAddr);

   // fill in the dest address.
   netToIPSocketAddress(address, &mainPortAddr);

   // then send it.      
   macResult = OTSndUData(mainEndpoint, &tSend);
   if(macResult < 0)
   {
      //!!!!!!TBD handle error.
      return UnknownError;
   }

   return NoError;
}


//======================================================================
static PacketReceiveEvent receiveEvent; // why create on stack each time...
//======================================================================
void Net::process()
{
   S32 bytesRead;
   
   if (mainEndpoint==NULL)
      return;

   macResult = kOTNoError;
   
   while(1)
   {
      bytesRead = 0L;
      
      // clear any send struct fields that should be clean...
      tRecv.opt.len = 0;

      // setup the recv buffer to point to the receive event packet..
      tRecv.udata.maxlen = MaxPacketDataSize;
      tRecv.udata.len = MaxPacketDataSize;
      tRecv.udata.buf = receiveEvent.data;

      // setup the dest addr struct.
      tRecv.addr.maxlen = sizeof(InetAddress);
      tRecv.addr.len = sizeof(InetAddress);
      tRecv.addr.buf = (U8*)(&destPortAddr);

      tFlags = 0L;
      macResult = OTRcvUData(mainEndpoint, &tRecv, &tFlags);
      if (macResult == kOTNoError)
         bytesRead = tRecv.udata.len;
      else
      if (macResult < 0)
      {
         if (macResult == kOTNoDataErr)
            break; // clean return.
         else
         {
            // !!!!TBD error handling.
            break;
         }
      }
//      else // !!!!TBD????? what to do with positive error?
//      {
//      }


      IPSocketToNetAddress(&destPortAddr, &receiveEvent.sourceAddress);
      
      // check for loopback.
      NetAddress &na = receiveEvent.sourceAddress;
      if (na.type == NetAddress::IPAddress &&
            na.netNum[0] == 127 &&
            na.netNum[1] == 0 &&
            na.netNum[2] == 0 &&
            na.netNum[3] == 1 &&
            na.port == netPort)
         continue;

      // validate that we read something, else loop again.
      if (bytesRead <= 0)
         continue;

      // post the received data.
      receiveEvent.size = PacketReceiveEventHeaderSize + bytesRead;
      Game->postEvent(receiveEvent);
   }
   
   // !!!!!!TBD
   // may need to convert macResult into another variable
   // for referencing error later on...
}


// THE NEXT SET OF METHODS ARE FOR HANDLING TCP-STREAM PORTS.


//======================================================================
//DWORD WINAPI connectThreadFunction(LPVOID param)
//{
//   Connection *con = (Connection *) param;
//   con;
//   return 0;
//}


//======================================================================
// the mac uses 'connections' to track the equivalent of PC sockets.
//======================================================================
static Connection *newConnection(NetSocket id)
{
   // first, alloc a new Conn object, and hook it into list/table properly.
   Connection *conn = new Connection;
   conn->nextConnection = connectionList;
   connectionList = conn;
   conn->nextInTable = connectionTable[id & ConnectionTableMask];
   connectionTable[id & ConnectionTableMask] = conn;

   conn->socket = id;
   conn->listenConnection = false;
   return conn;
}


//======================================================================
NetSocket Net::openListenPort(U16 port)
{
   if(Game->isJournalReading())
   {
      U32 ret;
      Game->journalRead(&ret);
      Connection *conn = newConnection(ret);
      return ret;
   }

   NetSocket sock = openSocket();
   Connection *conn = newConnection(sock);
   bind(sock, port);
   listen(sock, 4);
   conn->listenConnection = true; // until listen does this...
   setBlocking(sock, false);

   if(Game->isJournalWriting())
      Game->journalWrite(sock);

   return sock;
}


//======================================================================
NetSocket Net::openConnectTo(const char *stringAddress)
{
   if(Game->isJournalReading())
   {
      U32 ret;
      Game->journalRead(&ret);
      Connection *conn = newConnection(ret);
      return ret;
   }
      
   NetSocket sock = openSocket();
   Connection *conn = newConnection(sock);
   
   conn->prevState = ConnectedNotifyEvent::DNSResolved;
   conn->state = ConnectedNotifyEvent::DNSResolved;
   
   NetAddress netaddr;
   connect(sock, &netaddr);
   setBlocking(sock, false);
   
//threadHandle = CreateThread(NULL, 0, connectThreadFunction, (LPVOID) conn, 0, &threadId);
//CloseHandle(threadHandle);

   conn->state = ConnectedNotifyEvent::Connected;
   return sock;
}


#if later
//======================================================================
void Net::processConnected()
{
   Connection **walk = &connectionList;
   Connection *con;
   while((con = *walk) != NULL)
   {
      bool del = false;
      if(con->listenConnection)
      {
         NetSocket newSocket;
         ConnectedAcceptEvent event;
         newSocket = accept(con->socket, &event.address);

         if(newSocket != InvalidSocket)
         {
            Connection *nc = newConnection(newSocket);
            nc->prevState = ConnectedNotifyEvent::Connected;
            nc->state = ConnectedNotifyEvent::Connected;
            setBlocking(newSocket, false);
            
            event.portTag = con->socket;
            event.connectionTag = newSocket;
            Game->postEvent(event);
         }
         walk = &con->nextConnection;
      }
      else
      {
         if(con->state != con->prevState)
         {
            ConnectedNotifyEvent event;
            event.tag = con->socket;
            event.state = con->state;
            Game->postEvent(event);
            con->prevState = con->state;
         }
         if(con->state == ConnectedNotifyEvent::Connected)
         {
            ConnectedReceiveEvent event;
            Net::Error err;
            S32 bytesRead;
            event.tag = con->socket;
         
            do {
//                err = recv(con->socket, event.data, MaxPacketDataSize, &bytesRead);
               if(err == NoError && bytesRead != 0)
               {
                  event.size = ConnectedReceiveEventHeaderSize + bytesRead;
                  Game->postEvent(event);
               }
               else if(err != WouldBlock)
               {
                  // bad news... this disconnected
                  ConnectedNotifyEvent event;
                  event.tag = con->socket;
                  event.state = ConnectedNotifyEvent::Disconnected; 
                  Game->postEvent(event);
                  del = true;
               }
            }
            while(err == NoError && bytesRead != 0);
         }
         if(del)
         {
            *walk = con->nextConnection;
            closeSocket(con->socket);
            for(Connection **tbWalk = &connectionTable[con->tag & ConnectionTableMask]; *tbWalk != NULL; tbWalk = &(*tbWalk)->nextInTable)
            {
               Connection *dc = *tbWalk;
               if(dc->tag == con->tag)
               {
                  *tbWalk = dc->nextInTable;
                  break;
               }
            }
            delete con;
         }
         else
            walk = &con->nextConnection;
      }
   }
}
#endif


//======================================================================
void Net::closeConnectTo(NetSocket sock)
{
   Connection **walk;

   if(!Game->isJournalReading()) // if reading, skip this, but do next loop.
   for(walk = &connectionList; *walk != NULL; walk = &(*walk)->nextConnection)
   {
      Connection *con = *walk;
      if(con->socket == sock)
      {
         *walk = con->nextConnection;
         closeSocket(sock);
         break;
      }
   }
   
   // if reading, still need to flush the connection object we created until we don't create during read.
   for(walk = &connectionTable[sock & ConnectionTableMask]; *walk != NULL; walk = &(*walk)->nextInTable)
   {
      Connection *con = *walk;
      if(con->socket == sock)
      {
         *walk = con->nextInTable;
         delete con;
         return;
      }
   }
}


//======================================================================
Net::Error Net::sendtoSocket(NetSocket socket, const U8 *buffer, int bufferSize)
{
   if(Game->isJournalReading())
   {
      U32 e;
      Game->journalRead(&e);
      
      return (Net::Error) e;
   }
   Net::Error e = send(socket, buffer, bufferSize);
   if(Game->isJournalWriting())
      Game->journalWrite(U32(e));
   return e;
}

                  
//======================================================================
NetSocket Net::openSocket()
{
   return nextConnectionId++; // it'll wrap eventually!!!!
   // !!!!TBD -- CHECK FOR THIS ID IN THE LIST BEFORE RETURNING IT,
   // SO WE DON'T RETURN AN ALREADY-IN-USE SOCKET ID.
}


//======================================================================
Net::Error Net::closeSocket(NetSocket socket)
{
    if(socket != InvalidSocket)
    {
#if later
       if(!closesocket(socket))
          return NoError;
       else
#endif
          return getLastError();
    }
    else
       return NotASocket;
}


//======================================================================
Net::Error Net::connect(NetSocket socket, const NetAddress *address)
{
   if(address->type != NetAddress::IPAddress)
       return WrongProtocolType;
#if later
    SOCKADDR_IN socketAddress;
    netToIPSocketAddress(address, &socketAddress);
    if(!::connect(socket, (PSOCKADDR) &socketAddress, sizeof(socketAddress)))
       return NoError;
#endif
   return getLastError();
}


//======================================================================
Net::Error Net::listen(NetSocket socket, S32 backlog)
{
//   conn->listenConnection = true;
//   if(!::listen(socket, backlog))
//      return NoError;
   return getLastError();
}


//======================================================================
NetSocket Net::accept(NetSocket acceptSocket, NetAddress *remoteAddress)
{
#if later
   SOCKADDR_IN socketAddress;
   S32 addrLen = sizeof(socketAddress);
   
   SOCKET retVal = ::accept(acceptSocket, (PSOCKADDR) &socketAddress, &addrLen);
   if(retVal != INVALID_SOCKET)
   {
      IPSocketToNetAddress(&socketAddress, remoteAddress);
      return retVal;
   }
#endif
   return InvalidSocket;
}


//======================================================================
Net::Error Net::bind(NetSocket socket, U16 port)
{
#if later
   S32 error;
   
   SOCKADDR_IN socketAddress;
   dMemset((char *)&socketAddress, 0, sizeof(socketAddress));
   socketAddress.sin_family = AF_INET;
   // It's entirely possible that there are two NIC cards.
   // We let the user specify which one the server runs on.

   // thanks to [TPG]P1aGu3 for the name
   const char* serverIP = Con::getVariable( "Pref::Net::BindAddress" );
   // serverIP is guaranteed to be non-0.
   AssertFatal( serverIP, "serverIP is NULL!" );

   if( serverIP[0] != '\0' ) {
      // we're not empty
      socketAddress.sin_addr.s_addr = inet_addr( serverIP );

      if( socketAddress.sin_addr.s_addr != INADDR_NONE ) {
    Con::printf( "Binding server port to %s", serverIP );
      } else {
    Con::warnf( ConsoleLogEntry::General,
           "inet_addr() failed for %s while binding!",
           serverIP );
    socketAddress.sin_addr.s_addr = INADDR_ANY;
      }

   } else {
      Con::printf( "Binding server port to default IP" );
      socketAddress.sin_addr.s_addr = INADDR_ANY;
   }

   socketAddress.sin_port = htons(port);
   error = ::bind(socket, (PSOCKADDR) &socketAddress, sizeof(socketAddress));

   if(!error)
      return NoError;
#endif
   return getLastError();
}


//======================================================================
Net::Error Net::setBufferSize(NetSocket socket, S32 bufferSize)
{
   socket, bufferSize;
//    S32 error;
//    error = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *) &bufferSize, sizeof(bufferSize));
//    if(!error)
//       error = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *) &bufferSize, sizeof(bufferSize));
//    if(!error)
//       return NoError;
   return getLastError();
}


//======================================================================
Net::Error Net::setBroadcast(NetSocket socket, bool broadcast)
{
   socket, broadcast;
//    S32 bc = broadcast;
//    S32 error = setsockopt(socket, SOL_SOCKET, SO_BROADCAST, (char*)&bc, sizeof(bc));
//    if(!error)
//       return NoError;
   return getLastError();   
}


//======================================================================
Net::Error Net::setBlocking(NetSocket socket, bool blockingIO)
{
   socket, blockingIO;
//    DWORD notblock = !blockingIO;
//    S32 error = ioctlsocket(socket, FIONBIO, &notblock);
//    if(!error)
//       return NoError;
   return getLastError();   
}


//======================================================================
Net::Error Net::send(NetSocket socket, const U8 *buffer, S32 bufferSize)
{
   Net::Error e;
//    e = ::send(socket, (const char*)buffer, bufferSize, 0);

   return getLastError();
//   return e;
}


//======================================================================
Net::Error Net::recv(NetSocket socket, U8 *buffer, S32 bufferSize, S32 *bytesRead)
{
   socket, buffer, bufferSize, bytesRead;
//    *bytesRead = ::recv(socket, (char*)buffer, bufferSize, 0);
//    if(*bytesRead == SOCKET_ERROR)
//       return getLastError();
   return NoError;
}


//======================================================================
bool Net::compareAddresses(const NetAddress *a1, const NetAddress *a2)
{
   if(a1->type != a2->type)
      return false;
   if(*((U32 *)a1->netNum) != *((U32 *)a2->netNum))
      return false;

/* this was for IPX only.  !!!!tbd
   if(a1->type == NetAddress::IPAddress)
      return true;
   for(S32 i = 0; i < 6; i++)
      if(a1->nodeNum[i] != a2->nodeNum[i])
         return false;
*/
   return true;
}


//======================================================================
bool Net::stringToAddress(const char *addressString, NetAddress *address)
{
   char sRemote[256]; // temporary space so we can muck with the inString.
   char *sPort;
   
   // clear destination.
   dMemset((void*)address, 0, sizeof(NetAddress));
   
   // assume IP if it doesn't have ipx: at the front.
   if (dStrnicmp(addressString, "ipx:", 4))
   {
      if (!dStrnicmp(addressString, "ip:", 3))
         addressString += 3;  // eat off the ip:
      
      if (dStrlen(addressString) > 255) // waaay too long.
         return false;

      dStrcpy(sRemote, addressString);
      
      sPort = dStrchr(sRemote, ':');
      if (sPort) // if non-null...
         *sPort++ = 0; // null the : and set ahead past the :
         
      // first, look for broadcast.
      if(!dStricmp(sRemote, "broadcast"))
      {
         *((InetHost*)address->netNum) = broadcastIP;
      }
      else
      {
         // try to do a simple string convert, in hopes it's a dotted-numeral address.
         OTInetStringToHost(sRemote, ((InetHost*)(address->netNum)));
         
         // if host numbers are all zero, safe bet that we need to do DNS lookup...
         if(*((InetHost*)address->netNum) == 0)
         {
/* THIS WAS OLD CODE... NEED TO VERIFY OR REWRITE!!!!TBD */
// dhc - 9.01 - minor tweaks to try and get this dns crap running again.  BUT NOT REALLY ERROR CHECKING!!!!!TBD
            U8 dnsBuffer[1024];
            TLookupRequest dnsRequest;
            TLookupReply dnsReply;
            MapperRef dns;
            OSStatus oterr;
            
            // create a mapper for DNS lookup
            dns = OTOpenMapperInContext(OTCreateConfiguration(kDNRName), 0, &macStatus, NULL);

            // set the mapper to be sync and blocking, so we don't need callbacks
            // !!!!TBD change to async handling of this!
            if (macStatus == kOTNoError)
               oterr = OTSetSynchronous(dns);
            if (macStatus == kOTNoError)
               oterr = OTSetBlocking(dns);

            // then, if we haven't yet hit an error, do the DNS lookup.
            if (macStatus == kOTNoError)
            {
               dMemset((void*)&dnsRequest, 0, sizeof(TLookupRequest));
               dnsRequest.maxcnt = 1; // just looking for a single IP
               dnsRequest.timeout = 5000; // # ms before we timeout.
               dnsRequest.name.buf = (unsigned char *)sRemote;
               dnsRequest.name.len = dStrlen(sRemote);

               dMemset((char *)&dnsReply, 0, sizeof(TLookupReply));
               dnsReply.names.maxlen = 1024;
               dnsReply.names.buf = dnsBuffer;
            
               macStatus = OTLookupName(dns, &dnsRequest, &dnsReply);
               if (macStatus == kOTNoError)
               {
                  if (!dnsReply.rspcount)
                  {
                     // !!!!TBD handle error of not found.  hopefully, macStatus was set already.
                  }
                  else // copy over the result.
                  {
                     *((InetHost*)address->netNum) = ((InetAddress*)(((TLookupBuffer*)dnsBuffer)->fAddressBuffer))->fHost;
                  }
               }
            }

            // HERE we handle ALL drop through error cases.
            // this way, it's handled in one place, and cleanup
            // is simpler as well...
            if (macStatus != kOTNoError)
            {
               //!!!!TBD -- handle the error by setting global lookup val.
            }
            
            // close down dns if valid.
            if (dns != kOTInvalidMapperRef)
            {
               macStatus = OTCloseProvider(dns);
               if (macStatus != kOTNoError)
               {
                  // !!!! NOT SURE.  we don't want to necessarily blow away
                  // the failure above with this one.
                  //!!!!TBD -- handle the error by setting global lookup val.
               }
            }

            if (macStatus != kOTNoError)
               return false;
         }
      }

      if (sPort)
         address->port = dAtoi(sPort);
      else
         address->port = defaultPort;

      address->type = NetAddress::IPAddress;

      return true;
   }

   return false;
}


//======================================================================
void Net::addressToString(const NetAddress *address, char addressString[256])
{
   addressString[0] = 0; // clear string...
   
   if(address->type == NetAddress::IPAddress)
   {
      if (HOSTFROMBYTES(address->netNum) == broadcastIP)
         dSprintf(addressString, 256, "IP:Broadcast:%d", address->port);
      else
         dSprintf(addressString, 256, "IP:%d.%d.%d.%d:%d",
                  address->netNum[0], address->netNum[1], address->netNum[2], address->netNum[3],
                  address->port);
   }
}


//======================================================================
Net::Error getLastError()
{
   return Net::UnknownError;
//    S32 err = WSAGetLastError();
//    switch(err)
//    {
//       case WSAEWOULDBLOCK:
//          return Net::WouldBlock;
//       default:
//          return Net::UnknownError;
//    }
}
