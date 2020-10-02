/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
NetSocket: Manages network connections via TCP/IP sockets.

The NetSocket class provides a simple way of using TCP/IP sockets.  Connections from a single client to the server and
from the server to multiple clients are supported.  SSL functionality is also integrated.

The NetSocket class is designed only to cater for asynchronous (non-blocking) communication. This functionality is
achieved primarily through callback fields - connection alerts are managed by #Feedback, incoming data is
received through #Incoming and readiness for outgoing data is supported by #Outgoing.

<header>Client-Server Connections</>

After a connection has been established, data may be written to the server uses any of the following methods:

<list type="unsorted">
<li>Write directly to the server at any time using either the #Write() action or #WriteMsg() method.</li>
<li>Subscribe to the socket by referring to a routine in the #Outgoing field.  The routine will be called to
initially fill the internal write buffer, thereafter it will be called whenever the buffer is empty.</li>
</ul>

It is possible to write to a NetSocket object before the connection to a server is established.  Doing so will buffer
the data in the socket until the connection with the server has been initiated, at which point the data will be
immediately sent.

<header>Server-Client Connections</>

To accept incoming client connections, create a NetSocket object with the SERVER flag set and define the
#Port value on which to listen for new clients.  If you wish to allow multiple connections from a single
client IP address, you should also set the MULTICONNECT flag.

When a new connection is detected, the #Feedback function will be called as `Feedback(objNetSocket *NetSocket, OBJECTPTR Context, LONG Operation)`

The NetSocket parameter refers to the original NetSocket object, the Context refers to the object that set the Feedback
field and the Operation value will be set to NTC_CONNECTED.  Information about the new connection can be read from the
#CurrentSocket field.  If a client disconnects, the Feedback function will be called in the same manner but
with an Operation value of NTC_DISCONNECTED.

Information on all active connections can be read from the #Clients field.  This contains a linked list of IP
addresses and their connections to the server port.

To send data to a client, set the #CurrentSocket field to reference the target client and then call the
#Write() action.

All data that is received from client sockets will be passed to the #Incoming feedback routine.  The
#CurrentSocket field will refer to the socket that has received the data.
-END-

*****************************************************************************/

// The MaxWriteLen cannot exceed the size of the network queue on the host platform, otherwise all send attempts will
// return 'could block' error codes.  Note that when using SSL, the write length is an SSL library imposition.

static LONG glMaxWriteLen = 16 * 1024;

//****************************************************************************
// Prototypes for internal methods

#ifdef __linux__
static void client_connect(HOSTHANDLE, APTR);
#endif

static void client_server_incoming(SOCKET_HANDLE, struct rkNetSocket *);
static void client_server_outgoing(SOCKET_HANDLE, struct rkNetSocket *);
static void free_client(objNetSocket *, struct rkNetClient *);
static void free_client_socket(objNetSocket *, objClientSocket *, BYTE);
static void server_client_incoming(SOCKET_HANDLE, objClientSocket *);
static void server_client_outgoing(SOCKET_HANDLE, objClientSocket *);
static void server_client_connect(SOCKET_HANDLE, objNetSocket *);
static void free_socket(objNetSocket *);
static ERROR write_queue(objNetSocket *, struct NetQueue *, CPTR, LONG);

//****************************************************************************

static ERROR NETSOCKET_ActionNotify(objNetSocket *Self, struct acActionNotify *Args)
{
   if (!Args) return ERR_NullArgs;

   if (Args->ActionID IS AC_Free) {
      if ((Self->Feedback.Type IS CALL_SCRIPT) AND (Self->Feedback.Script.Script->UniqueID IS Args->ObjectID)) {
         Self->Feedback.Type = CALL_NONE;
      }
      else if ((Self->Incoming.Type IS CALL_SCRIPT) AND (Self->Incoming.Script.Script->UniqueID IS Args->ObjectID)) {
         Self->Incoming.Type = CALL_NONE;
      }
      else if ((Self->Outgoing.Type IS CALL_SCRIPT) AND (Self->Outgoing.Script.Script->UniqueID IS Args->ObjectID)) {
         Self->Outgoing.Type = CALL_NONE;
      }
   }

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Connect: Connects a NetSocket to an address

This method initiates the connection process with a target Address.  The address to connect to can be specified either
as a domain name, in which case the domain name is first resolved to an IP address, or the address can be specified in
standard IP notation.

This method is non-blocking.  It will return immediately and the connection will be resolved once the server responds
to the connection request or an error occurs.  Client code should subscribe to the State field to respond to changes to
the connection state.

Pre-Condition: Must be in a connection state of NTC_DISCONNECTED

Post-Condition: If this method returns ERR_Okay, will be in state NTC_CONNECTING.

-INPUT-
cstr Address: String containing either a domain name (e.g. "www.google.com") or an IP address (e.g. "123.123.123.123")
int Port: Remote port to connect to.

-ERRORS-
Okay: The NetSocket connecting process was successfully started.
Args: Address was NULL, or Port was not in the required range.
BadState: The NetSocket was not in the state NTC_DISCONNECTED.
HostNotFound: Host name resolution failed.
Failed: The connect failed for some other reason.
-END-

*****************************************************************************/

static void connect_name_resolved(LARGE, ERROR, CSTRING HostName, CSTRING Aliases,
   LONG TotalAliases, struct IPAddress *IPs, LONG TotalIPs);

static ERROR NETSOCKET_Connect(objNetSocket *Self, struct nsConnect *Args)
{
   struct IPAddress server_ip;

   if ((!Args) OR (!Args->Address) OR (Args->Port <= 0) OR (Args->Port >= 65536)) return PostError(ERR_Args);

   if (Self->Flags & NSF_SERVER) return ERR_Failed;

   if (!Self->SocketHandle) return PostError(ERR_NotInitialised);

   if (Self->State != NTC_DISCONNECTED) {
      LogErrorMsg("Attempt to connect when socket is not in disconnected state");
      return PostError(ERR_BadState);
   }

   LogBranch("Address: %s, Port: %d", Args->Address, Args->Port);

   if (Args->Address != Self->Address) {
      if (Self->Address) FreeResource(Self->Address);
      Self->Address = StrClone(Args->Address);
   }
   Self->Port = Args->Port;

   if (!netStrToAddress(Self->Address, &server_ip)) { // The address is an IP string, no resolution is necessary
      connect_name_resolved((MAXINT)Self, ERR_Okay, NULL, NULL, 0, &server_ip, 1);
   }
   else { // Assume address is a domain name, so try and do a DNS resolution
      LogMsg("Attempting to resolve domain name '%s'...", Self->Address);

      FUNCTION callback;
      SET_FUNCTION_STDC(callback, (APTR)connect_name_resolved);
      if (netResolveName(Self->Address, 0, &callback, (MAXINT)Self) != ERR_Okay) {
         return LogBackError(0, Self->Error = ERR_HostNotFound);
      }
   }

   LogBack();
   return ERR_Okay;
}

//****************************************************************************
// This function is called on completion of nsResolveName().

static void connect_name_resolved(LARGE ClientData, ERROR Error, CSTRING HostName, CSTRING Aliases,
   LONG TotalAliases, struct IPAddress *IPs, LONG TotalIPs)
{
   objNetSocket *Self = (objNetSocket *)(MAXINT)ClientData;
   struct sockaddr_in server_address;

   if (Error != ERR_Okay) {
      LogF("@connect_name_resolved()","DNS resolution failed: %s", GetErrorMsg(Error));
      return;
   }

   LogF("connect_name_resolved()","Received callback on DNS resolution.  Handle: %d", Self->SocketHandle);

   // Start connect()

   ClearMemory(&server_address, sizeof(struct sockaddr_in));
   server_address.sin_family = AF_INET;
   server_address.sin_port = netHostToShort((UWORD)Self->Port);
   server_address.sin_addr.s_addr = netHostToLong(IPs->Data[0]);

#ifdef __linux__
   LONG result = connect(Self->SocketHandle, (struct sockaddr *)&server_address, sizeof(server_address));

   if (result IS -1) {
      if (errno IS EINPROGRESS) {
         FMSG("connect_name_resolved","Connection in progress...");
      }
      else if ((errno IS EWOULDBLOCK) OR (errno IS EAGAIN)) {
         FMSG("connect_name_resolved","connect() attempt would block or need to try again.");
      }
      else {
         LogF("@connect_name_resolved","Connect() failed: %s", strerror(errno));
         Self->Error = ERR_Failed;
         SetLong(Self, FID_State, NTC_DISCONNECTED);
         return;
      }

      SetLong(Self, FID_State,  NTC_CONNECTING);
      RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_READ|RFD_SOCKET, (APTR)&client_server_incoming, Self);

      // The write queue will be signalled once the connection process is completed.

      RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_WRITE|RFD_SOCKET, &client_connect, Self);
   }
   else {
      FMSG("connect_name_resolved","connect() successful.");

      SetLong(Self, FID_State, NTC_CONNECTED);
      RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_READ|RFD_SOCKET, (APTR)&client_server_incoming, Self);
   }

#elif _WIN32
   if ((Self->Error = win_connect(Self->SocketHandle, (struct sockaddr *)&server_address, sizeof(server_address)))) {
      LogErrorMsg("connect() failed: %s", GetErrorMsg(Self->Error));
      return;
   }

   SetLong(Self, FID_State, NTC_CONNECTING); // Connection isn't complete yet - see win32_netresponse() code for NTE_CONNECT
#endif
}

//****************************************************************************
// Action: DataFeed

static ERROR NETSOCKET_DataFeed(objNetSocket *Self, struct acDataFeed *Args)
{
   if (!Args) return PostError(ERR_NullArgs);

   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Disable: Disables sending and receiving on the socket.

This method will stop all sending and receiving of data over the socket.

-ERRORS-
Okay
Failed: Shutdown operation failed.

*****************************************************************************/

static ERROR NETSOCKET_Disable(objNetSocket *Self, APTR Void)
{
   MSG(NULL);

#ifdef __linux__
   LONG result = shutdown(Self->SocketHandle, 2);
#elif _WIN32
   LONG result = win_shutdown(Self->SocketHandle, 2);
#endif

   if (result) { // Zero is success on both platforms
      LogErrorMsg("shutdown() failed.");
      return ERR_Failed;
   }

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
DisconnectClient: Disconnects all sockets connected to a specific client IP.

For server sockets with a client connection, this method will terminate all socket connections made to a client and
frees any resources allocated to it.  If #Feedback is defined, a DISCONNECTED state message will also be issued
for each socket connection.

If only one socket connection needs to be disconnected, please use #DisconnectSocket().

-INPUT-
obj(NetClient) Client: The client to be disconnected.

-ERRORS-
Okay
NullArgs
-END-

*****************************************************************************/

static ERROR NETSOCKET_DisconnectClient(objNetSocket *Self, struct nsDisconnectClient *Args)
{
   if ((!Args) OR (!Args->Client)) return PostError(ERR_NullArgs);

   free_client(Self, Args->Client);

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
DisconnectSocket: Disconnects a socket that is connected to a specific client IP.

This method will disconnect a socket connection for a given client.  If #Feedback is defined, a DISCONNECTED
state message will also be issued.

-INPUT-
obj(ClientSocket) Socket: The client socket to be disconnected.

-ERRORS-
Okay
NullArgs
-END-

*****************************************************************************/

static ERROR NETSOCKET_DisconnectSocket(objNetSocket *Self, struct nsDisconnectSocket *Args)
{
   if ((!Args) OR (!Args->Socket)) return PostError(ERR_NullArgs);

   free_client_socket(Self, Args->Socket, TRUE);

   return ERR_Okay;
}

//****************************************************************************
// Action: Free

static ERROR NETSOCKET_Free(objNetSocket *Self, APTR Void)
{
#ifdef ENABLE_SSL
   sslDisconnect(Self);
#endif

   if (Self->Address) { FreeResource(Self->Address); Self->Address = NULL; }

   free_socket(Self);

   // Remove all client resources

   while (Self->Clients) free_client(Self, Self->Clients);

   return ERR_Okay;
}

//****************************************************************************
// Action: FreeWarning
//
// If a netsocket object is about to be freed, ensure that we are not using the netsocket object in one of our message
// handlers.  We can still delay the free request in any case.

static ERROR NETSOCKET_FreeWarning(objNetSocket *Self, APTR Void)
{
   if (Self->InUse) {
      if (!Self->Terminating) { // Check terminating state to prevent flooding of the message queue
         LogMsg("NetSocket in use, cannot free yet (request delayed).");
         Self->Terminating = TRUE;
         DelayMsg(AC_Free, Self->Head.UniqueID, NULL);
      }
      return ERR_InUse;
   }
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
GetLocalIPAddress: Returns the IP address that the socket is locally bound to.

This method performs the POSIX equivalent of getsockname().  It returns the current address to which the NetSocket
is bound.

-INPUT-
struct(*IPAddress) Address:  Pointer to an IPAddress structure which will be set to the result of the query if successful.

-ERRORS-
Okay
NullArgs
Failed
-END-

*****************************************************************************/

static ERROR NETSOCKET_GetLocalIPAddress(objNetSocket *Self, struct nsGetLocalIPAddress *Args)
{
   MSG(NULL);

   if ((!Args) OR (!Args->Address)) return PostError(ERR_NullArgs);

   struct sockaddr_in addr;
   LONG addr_length = sizeof(addr);

   LONG result;
#ifdef __linux__
   result = getsockname(Self->SocketHandle, (struct sockaddr *)&addr, &addr_length);
#elif _WIN32
   result = win_getsockname(Self->SocketHandle, (struct sockaddr *)&addr, &addr_length);
#endif

   if (!result) {
      Args->Address->Data[0] = netLongToHost(addr.sin_addr.s_addr);
      Args->Address->Data[1] = 0;
      Args->Address->Data[2] = 0;
      Args->Address->Data[3] = 0;
      Args->Address->Type = IPADDR_V4;
      return ERR_Okay;
   }
   else return PostError(ERR_Failed);
}

//****************************************************************************
// Action: Init()

static ERROR NETSOCKET_Init(objNetSocket *Self, APTR Void)
{
   ERROR error;

   if (Self->SocketHandle != (SOCKET_HANDLE)-1) return ERR_Okay; // The socket has been pre-configured by the developer

#ifdef ENABLE_SSL
   // Initialise the SSL structures (do not perform any connection yet).  Notice how the NSF_SSL flag is used to check
   // for an SSL request - however after this point any SSL checks must be made on the presence of a value in the SSL
   // field.

   if (Self->Flags & NSF_SSL) {
      if ((error = sslInit()) != ERR_Okay) return error;
      if ((error = sslSetup(Self)) != ERR_Okay) return error;
   }
#endif

   // Create the underlying socket

#ifdef __linux__

   //if ((Self->SocketHandle = socket(PF_INET6, SOCK_STREAM, 0)) IS NOHANDLE) {
      if ((Self->SocketHandle = socket(PF_INET, SOCK_STREAM, 0)) IS NOHANDLE) {
         LogErrorMsg("socket() %s", strerror(errno));
         return ERR_Failed;
      }
   //}
   //else Self->IPV6 = TRUE;

   // Put the socket into non-blocking mode, this is required when registering it as an FD and also prevents connect()
   // calls from going to sleep.

   #if 0
      // Was there any reason to use ioctl() when we have fcntl()???
      ULONG non_blocking = 1;
      LONG result = ioctl(Self->SocketHandle, FIONBIO, &non_blocking);
      if (result) return PostError(ERR_Failed);
   #else
      if (fcntl(Self->SocketHandle, F_SETFL, fcntl(Self->SocketHandle, F_GETFL) | O_NONBLOCK)) return PostError(ERR_Failed);
   #endif

   // Set the send timeout so that connect() will timeout after a reasonable time

   //LONG timeout = 30000;
   //result = setsockopt(Self->SocketHandle, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
   //assert(result IS 0);

#elif _WIN32

   Self->SocketHandle = win_socket(Self, TRUE, FALSE);
   if (Self->SocketHandle IS NOHANDLE) return ERR_Failed;

#endif

#ifdef ENABLE_SSL

   if (Self->SSL) sslLinkSocket(Self); // Link the socket handle to the SSL object

#endif

   if (Self->Flags & NSF_SERVER) {
      if (!Self->Port) return PostError(ERR_FieldNotSet);

      if (Self->IPV6) {
         #ifdef __linux__
            // IPV6

            struct sockaddr_in6 addr;

            ClearMemory(&addr, sizeof(addr));
            addr.sin6_family = AF_INET6;
            addr.sin6_port   = netHostToShort(Self->Port); // Must be passed in in network byte order
            addr.sin6_addr   = in6addr_any;   // Must be passed in in network byte order

            LONG result;
            LONG value = 1;
            setsockopt(Self->SocketHandle, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

            if ((result = bind(Self->SocketHandle, (struct sockaddr *)&addr, sizeof(addr))) != -1) {
               listen(Self->SocketHandle, Self->Backlog);
               RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_READ|RFD_SOCKET, &server_client_connect, Self);
               return ERR_Okay;
            }
            else if (result IS EADDRINUSE) return PostError(ERR_InUse);
            else return PostError(ERR_Failed);
         #else
            return ERR_NoSupport;
         #endif
      }
      else {
         // IPV4
         struct sockaddr_in addr;
         ClearMemory(&addr, sizeof(addr));
         addr.sin_family = AF_INET;
         addr.sin_port   = netHostToShort(Self->Port); // Must be passed in in network byte order
         addr.sin_addr.s_addr   = INADDR_ANY;   // Must be passed in in network byte order

         #ifdef __linux__
            LONG result;
            LONG value = 1;
            setsockopt(Self->SocketHandle, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

            if ((result = bind(Self->SocketHandle, (struct sockaddr *)&addr, sizeof(addr))) != -1) {
               listen(Self->SocketHandle, Self->Backlog);
               RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_READ|RFD_SOCKET, &server_client_connect, Self);
               return ERR_Okay;
            }
            else if (result IS EADDRINUSE) return PostError(ERR_InUse);
            else return PostError(ERR_Failed);
         #elif _WIN32
            if (!(error = win_bind(Self->SocketHandle, (struct sockaddr *)&addr, sizeof(addr)))) {
               if (!(error = win_listen(Self->SocketHandle, Self->Backlog))) {
                  return ERR_Okay;
               }
               else return PostError(error);
            }
            else return PostError(error);
         #endif
      }
   }
   else if ((Self->Address) AND (Self->Port > 0)) {
      if ((error = nsConnect(Self, Self->Address, Self->Port)) != ERR_Okay) {
         return error;
      }
      else return ERR_Okay;
   }
   else return ERR_Okay;
}

//****************************************************************************
// Action: NewObject

static ERROR NETSOCKET_NewObject(objNetSocket *Self, APTR Void)
{
   Self->SocketHandle = NOHANDLE;
   Self->Error        = ERR_Okay;
   Self->Backlog      = 10;
   Self->State        = NTC_DISCONNECTED;
   Self->MsgLimit     = 1024768;
   Self->ClientLimit  = 1024;
   #ifdef _WIN32
      Self->WriteSocket = NULL;
      Self->ReadSocket = NULL;
   #endif
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Read: Read information from the socket.

The Read action will read incoming data from the socket and write it to the provided buffer.  If the socket connection
is safe, success will always be returned by this action regardless of whether or not data was available.  Almost all
other return codes indicate permanent failure, and the socket connection will be closed when the action returns.

-ERRORS-
Okay: Read successful (if no data was on the socket, success is still indicated).
NullArgs
Disconnected: The socket connection is closed.
Failed: A permanent failure has occurred and socket has been closed.

*****************************************************************************/

static ERROR NETSOCKET_Read(objNetSocket *Self, struct acRead *Args)
{
   if ((!Args) OR (!Args->Buffer)) return PostError(ERR_NullArgs);

   if (Self->Flags & NSF_SERVER) {
      // Read from a client (CurrentClient must be set)

      objClientSocket *csocket;
      if (!(csocket = Self->CurrentSocket)) return PostError(ERR_Disconnected);

      csocket->ReadCalled = TRUE;

      if (!Args->Length) { Args->Result = 0; return ERR_Okay; }

      ERROR error = RECEIVE(Self, csocket->Handle, Args->Buffer, Args->Length, 0, &Args->Result);

      if (error != ERR_Okay) {
         LogBranch("Freeing socket, error '%s'", GetErrorMsg(error));
         free_client_socket(Self, csocket, TRUE);
         LogBack();
      }
      return error;
   }
   else {
      // Read from the server that we're connected to

      if (Self->SocketHandle IS NOHANDLE) return PostError(ERR_Disconnected);

      Self->ReadCalled = TRUE;

      if (!Args->Length) { Args->Result = 0; return ERR_Okay; }

      ERROR error = RECEIVE(Self, Self->SocketHandle, Args->Buffer, Args->Length, 0, &Args->Result);

      if (error != ERR_Okay) {
         LogBranch("Freeing socket, error '%s'", GetErrorMsg(error));
         free_socket(Self);
         LogBack();
      }

      return error;
   }
}

/*****************************************************************************

-METHOD-
ReadMsg: Read a message from the socket.

This method reads messages that have been sent to the socket using Parasol Message Protocols.  Any message sent with
the WriteMsg method will conform to this protocol, thus simplifying message transfers between programs based on the
core platform at either point of the network link.

This method never returns a successful error code unless an entire message has been received from the sender.

-INPUT-
&ptr Message: A pointer to the message buffer will be placed here if a message has been received.
&int Length: The length of the message is returned here.
&int Progress: The number of bytes that have been read for the incoming message.
&int CRC: Indicates the CRC value that the message is expected to match.

-ERRORS-
Okay: A complete message has been read and indicated in the result parameters.
Args
NullArgs
LimitedSuccess: Some data has arrived, but the entire message is incomplete.  The length of the incoming message may be indicated in the Length parameter.
NoData: No new data was found for the socket.
BadData: The message header or tail was invalid, or the message length exceeded internally imposed limits.
AllocMemory: A message buffer could not be allocated.

*****************************************************************************/

static ERROR NETSOCKET_ReadMsg(objNetSocket *Self, struct nsReadMsg *Args)
{
   if (!Args) return PostError(ERR_NullArgs);

   FMSG("~","Reading message.");

   Args->Message = NULL;
   Args->Length  = 0;
   Args->CRC     = 0;
   Args->Progress = 0;

   struct NetQueue *queue;
   if (Self->Flags & NSF_SERVER) {
      objClientSocket *csocket;
      if (!(csocket = Self->CurrentSocket)) {
         STEP();
         return PostError(ERR_FieldNotSet);
      }
      queue = &csocket->ReadQueue;
   }
   else queue = &Self->ReadQueue;

   if (!queue->Buffer) {
      queue->Length = 2048;
      if (AllocMemory(queue->Length, MEM_NO_CLEAR, &queue->Buffer, NULL) != ERR_Okay) {
         STEP();
         return ERR_AllocMemory;
      }
   }

   LONG msglen, result, magic;
   ULONG total_length;
   ERROR error;

   if (queue->Index >= sizeof(struct NetMsg)) { // The complete message header has been received
      msglen = be32_cpu(((struct NetMsg *)queue->Buffer)->Length);
      total_length = sizeof(struct NetMsg) + msglen + 1 + sizeof(struct NetMsgEnd);
   }
   else { // The message header has not been read yet
      if (!(error = acRead(Self, (BYTE *)queue->Buffer + queue->Index, sizeof(struct NetMsg) - queue->Index, &result))) {
         queue->Index += result;

         if (queue->Index >= sizeof(struct NetMsg)) {
            // We have the message header
            magic  = be32_cpu(((struct NetMsg *)queue->Buffer)->Magic);
            msglen = be32_cpu(((struct NetMsg *)queue->Buffer)->Length);

            if (magic != NETMSG_MAGIC) {
               LogErrorMsg("Incoming message does not have the magic header (received $%.8x).", magic);
               queue->Index = 0;
               STEP();
               return ERR_InvalidData;
            }
            else if (msglen > NETMSG_SIZE_LIMIT) {
               LogErrorMsg("Incoming message of %d ($%.8x) bytes exceeds message limit.", msglen, msglen);
               queue->Index = 0;
               STEP();
               return ERR_InvalidData;
            }

            total_length = sizeof(struct NetMsg) + msglen + 1 + sizeof(struct NetMsgEnd);

            // Check if the queue buffer needs to be extended

            if (total_length > queue->Length) {
               MSG("Extending queue length from %d to %d", queue->Length, total_length);
               APTR buffer;
               if (!AllocMemory(total_length, MEM_NO_CLEAR, &buffer, NULL)) {
                  if (queue->Buffer) {
                     CopyMemory(queue->Buffer, buffer, queue->Index);
                     FreeResource(queue->Buffer);
                  }
                  queue->Buffer = buffer;
                  queue->Length = total_length;
               }
               else {
                  STEP();
                  return PostError(ERR_AllocMemory);
               }
            }
         }
         else {
            MSG("Succeeded in reading partial message header only (%d bytes).", result);
            STEP();
            return ERR_LimitedSuccess;
         }
      }
      else {
         MSG("Read() failed, error '%s'", GetErrorMsg(error));
         STEP();
         return ERR_LimitedSuccess;
      }
   }

   struct NetMsgEnd *msgend;
   Args->Message = (BYTE *)queue->Buffer + sizeof(struct NetMsg);
   Args->Length = msglen;

   //MSG("Current message is %d bytes long (raw len: %d), progress is %d bytes.", msglen, total_length, queue->Index);

   if (!(error = acRead(Self, (char *)queue->Buffer + queue->Index, total_length - queue->Index, &result))) {
      queue->Index += result;
      Args->Progress = queue->Index - sizeof(struct NetMsg) - sizeof(struct NetMsgEnd);
      if (Args->Progress < 0) Args->Progress = 0;

      // If the entire message has been read, we can report success to the user

      if (queue->Index >= total_length) {
         msgend = (struct NetMsgEnd *)((BYTE *)queue->Buffer + sizeof(struct NetMsg) + msglen + 1);
         magic = be32_cpu(msgend->Magic);
         queue->Index   = 0;
         Args->Progress = Args->Length;
         Args->CRC      = be32_cpu(msgend->CRC);

         MSG("The entire message of %d bytes has been received.", msglen);

         if (NETMSG_MAGIC_TAIL != magic) {
            LogErrorMsg("Incoming message has an invalid tail of $%.8x, CRC $%.8x.", magic, Args->CRC);
            STEP();
            return ERR_InvalidData;
         }

         STEP();
         return ERR_Okay;
      }
      else {
         STEP();
         return ERR_LimitedSuccess;
      }
   }
   else {
      LogErrorMsg("Failed to read %d bytes off the socket, error %d.", total_length - queue->Index, error);
      queue->Index = 0;
      STEP();
      return error;
   }
}

/*****************************************************************************

-ACTION-
Write: Writes data to the socket.

Writing data to a socket will send raw data to the remote client or server.  Write connections are buffered, so any
data overflow generated in a call to this action will be buffered into a software queue.  Resource limits placed on the
software queue are governed by the #MsgLimit field setting.

In server mode, the #CurrentSocket will designate the client that will receive the data.  If this field is not
set, ERR_FieldNotSet will be returned.

It is also possible to write to a socket in advance of any connection being made. The netsocket will queue the data
and automatically send it once the first connection has been made.

*****************************************************************************/

static ERROR NETSOCKET_Write(objNetSocket *Self, struct acWrite *Args)
{
   LONG len;
   objClientSocket *csocket;
   ERROR error;

   if (!Args) return ERR_NullArgs;

   Args->Result = 0;

   if (Self->Flags & NSF_SERVER) {
      // Write to a client (CurrentSocket must be set)

      if (!(csocket = Self->CurrentSocket)) {
         LogErrorMsg("CurrentSocket field not defined.");
         return ERR_FieldNotSet;
      }

      MSG("Writing %d bytes to client %d.", Args->Length, csocket->Handle);

      if (csocket->Handle IS NOHANDLE) {
         write_queue(Self, &csocket->WriteQueue, Args->Buffer, Args->Length);
         return ERR_Okay;
      }

      len = Args->Length;
      error = SEND(Self, csocket->Handle, Args->Buffer, &len, 0);

      if ((error) OR (len < Args->Length)) {
         // DataSize: The buffer is too big and will have to be transferred in chunks
         // BufferOverflow: The send buffer is full and the write will have to be buffered

         MSG("Buffer overflow, queuing data.");
         if ((error IS ERR_DataSize) OR (error IS ERR_BufferOverflow) OR (len > 0))  {
            write_queue(Self, &csocket->WriteQueue, (BYTE *)Args->Buffer + len, Args->Length - len);
            #ifdef __linux__
               RegisterFD((HOSTHANDLE)csocket->Handle, RFD_WRITE|RFD_SOCKET, &server_client_outgoing, csocket);
            #elif _WIN32
               win_socketstate(csocket->Handle, -1, TRUE);
               Self->WriteClientSocket = &server_client_outgoing;
            #endif
         }
      }

      Args->Result = Args->Length;
   }
   else {
      // Write to the server

      if ((Self->SocketHandle IS NOHANDLE) OR (Self->State != NTC_CONNECTED)) {
         // Queue the write prior to server connection

         MSG("Writing %d bytes to server (queued for connection).", Args->Length);
         write_queue(Self, &Self->WriteQueue, Args->Buffer, Args->Length);
         return ERR_Okay;
      }

      // Note that if a write queue has been setup, there is no way that we can write to the server until the queue has
      // been exhausted.  Thus we have add more data to the queue if it already exists.

      if (!Self->WriteQueue.Buffer) {
         len = Args->Length;
         error = SEND(Self, Self->SocketHandle, Args->Buffer, &len, 0);
      }
      else {
         len = 0;
         error = ERR_BufferOverflow;
      }

      if ((error) OR (len < Args->Length)) {
         if (error) MSG("SEND() Error: '%s', queuing %d/%d bytes for transfer...", GetErrorMsg(error), Args->Length - len, Args->Length);
         else MSG("Queuing %d of %d remaining bytes for transfer...", Args->Length - len, Args->Length);
         if ((error IS ERR_DataSize) OR (error IS ERR_BufferOverflow) OR (len > 0))  {
            write_queue(Self, &Self->WriteQueue, (BYTE *)Args->Buffer + len, Args->Length - len);
            #ifdef __linux__
               RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_WRITE|RFD_SOCKET, (APTR)&client_server_outgoing, Self);
            #elif _WIN32
               win_socketstate(Self->SocketHandle, -1, TRUE);
               Self->WriteSocket = &client_server_outgoing;
            #endif
         }
      }
      else MSG("Successfully wrote all %d bytes to the server.", Args->Length);

      Args->Result = Args->Length;
   }

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
WriteMsg: Writes a message to the socket.

Messages can be written to sockets with the WriteMsg method and read back by the receiver with #ReadMsg().  The
message data is sent through the #Write() action, so the standard process will apply (the message will be
queued and does not block if buffers are full).

-INPUT-
buf(ptr) Message: Pointer to the message to send.
bufsize Length: The length of the message.

-ERRORS-
Okay
Args
OutOfRange

*****************************************************************************/

static ERROR NETSOCKET_WriteMsg(objNetSocket *Self, struct nsWriteMsg *Args)
{
   if ((!Args) OR (!Args->Message) OR (Args->Length < 1)) return PostError(ERR_Args);
   if ((Args->Length < 1) OR (Args->Length > NETMSG_SIZE_LIMIT)) return PostError(ERR_OutOfRange);

   FMSG("~","Message: %p, Length: %d", Args->Message, Args->Length);

   struct NetMsg msg;
   msg.Magic = cpu_be32(NETMSG_MAGIC);
   msg.Length = cpu_be32(Args->Length);
   acWrite(Self, &msg, sizeof(msg), NULL);

   acWrite(Self, Args->Message, Args->Length, NULL);

   UBYTE endbuffer[sizeof(struct NetMsgEnd) + 1];
   struct NetMsgEnd *end = (struct NetMsgEnd *)(endbuffer + 1);
   endbuffer[0] = 0; // This null terminator helps with message parsing
   end->Magic = cpu_be32((ULONG)NETMSG_MAGIC_TAIL);
   end->CRC   = cpu_be32(GenCRC32(0, Args->Message, Args->Length));
   acWrite(Self, &endbuffer, sizeof(endbuffer), NULL);

   STEP();
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Address: An IP address or domain name to connect to.

If this field is set with an IP address or domain name prior to initialisation, an attempt to connect to that location
will be made when the NetSocket is initialised.  Post-initialisation this field cannot be set by the client, however
calls to #Connect() will result in it being updated so that it always reflects the named address of the current
connection.

*****************************************************************************/

static ERROR SET_Address(objNetSocket *Self, CSTRING Value)
{
   if (Self->Address) { FreeResource(Self->Address); Self->Address = NULL; }
   if (Value) Self->Address = StrClone(Value);
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Backlog: The maximum number of connections that can be queued against the socket.

Incoming connections to NetSocket objects are queued until they are answered by the object.  As there is a limit to the
number of connections that can be queued, you can adjust the backlog by writing this field.  The default setting is 10
connections.

If the backlog is exceeded, subsequent connections to the socket will typically be met with a connection refused error.

-FIELD-
ClientLimit: The maximum number of clients that can be connected to a server socket.

-FIELD-
Clients: For server sockets, lists all clients connected to the server.

-FIELD-
CurrentClient: Relevant for server sockets, indicates the target client socket for communications.

For server sockets, the CurrentClient field must refer to a target ClientSocket in order to perform operations such as
#Read() and #Write().

-FIELD-
Error: Information about the last error that occurred during a NetSocket operation

This field describes the last error that occurred during a NetSocket operation:

In the case where a NetSocket object enters the NTC_DISCONNECTED state from the NTC_CONNECTED state, this field can be
used to determine how a TCP connection was closed.

<types type="Error">
<type name="ERR_Okay">The connection was closed gracefully.  All data sent by the peer has been received.</>
<type name="ERR_Disconnected">The connection was broken in a non-graceful fashion. Data may be lost.</>
<type name="ERR_TimeOut">The connect operation timed out.</>
<type name="ERR_ConnectionRefused">The connection was refused by the remote host.  Note: This error will not occur on Windows, and instead the Error field will be set to ERR_Failed.</>
<type name="ERR_NetworkUnreachable">The network was unreachable.  Note: This error will not occur on Windows, and instead the Error field will be set to ERR_Failed.</>
<type name="ERR_HostUnreachable">No path to host was found.  Note: This error will not occur on Windows, and instead the Error field will be set to ERR_Failed.</>
<type name="ERR_Failed">An unspecified error occurred.</>
</>

-FIELD-
Feedback: A callback trigger for when the state of the NetSocket is changed.

Refer to a custom function in this field and it will be called whenever the #State of the socket (such as
connection or disconnection) changes.

The function must be in the format `Function(*NetSocket, OBJECTPTR Context, LONG State)`

The NetSocket parameter will refer to the NetSocket object to which the Feedback function is subscribed.  The Context
refers to the object that set the Feedback field and the State reflects the new value in the #State field.

*****************************************************************************/

static ERROR GET_Feedback(objNetSocket *Self, FUNCTION **Value)
{
   if (Self->Feedback.Type != CALL_NONE) {
      *Value = &Self->Feedback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_Feedback(objNetSocket *Self, FUNCTION *Value)
{
   if (Value) {
      if (Self->Feedback.Type IS CALL_SCRIPT) UnsubscribeAction(Self->Feedback.Script.Script, AC_Free);
      Self->Feedback = *Value;
      if (Self->Feedback.Type IS CALL_SCRIPT) SubscribeAction(Self->Feedback.Script.Script, AC_Free);
   }
   else Self->Feedback.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Flags: Optional flags.

-FIELD-
Incoming: Callback that is triggered when the socket receives data.

The Incoming field can be set with a custom function that will be called whenever the socket receives data.  The
function must follow this definition: `ERROR Incoming(*NetSocket, OBJECTPTR Context)`

The NetSocket parameter refers to the NetSocket object.  The Context refers to the object that set the Incoming field.

To get the data from the socket, call the #Read() action. Reading at least some of the data from the socket is
compulsory - if the function does not do this, then the data will be cleared from the socket when the function returns.
If your callback function returns ERR_Terminate, then the Incoming field will be cleared so that your function will no
longer be called.  All other error codes are ignored.

*****************************************************************************/

static ERROR GET_Incoming(objNetSocket *Self, FUNCTION **Value)
{
   if (Self->Incoming.Type != CALL_NONE) {
      *Value = &Self->Incoming;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_Incoming(objNetSocket *Self, FUNCTION *Value)
{
   if (Value) {
      if (Self->Incoming.Type IS CALL_SCRIPT) UnsubscribeAction(Self->Incoming.Script.Script, AC_Free);
      Self->Incoming = *Value;
      if (Self->Incoming.Type IS CALL_SCRIPT) SubscribeAction(Self->Incoming.Script.Script, AC_Free);
   }
   else Self->Incoming.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Outgoing: Callback that is triggered when the socket is ready to send data.

The Outgoing field can be set with a custom function that will be called whenever the socket is ready to send data.
The function must be in the format `ERROR Outgoing(*NetSocket, OBJECTPTR Context)`

The NetSocket parameter refers to the NetSocket object.  The Context refers to the object that set the Outgoing field.

To send data to the NetSocket object, call the #Write() action.  If your callback function returns
ERR_Terminate, then the Outgoing field will be cleared so that your function will no longer be called.  All other error
codes are ignored.

*****************************************************************************/

static ERROR GET_Outgoing(objNetSocket *Self, FUNCTION **Value)
{
   if (Self->Flags & NSF_SERVER) {
      objClientSocket *csocket;

      if (!(csocket = Self->CurrentSocket)) return PostError(ERR_Disconnected);

      if (csocket->Outgoing.Type != CALL_NONE) {
         *Value = &csocket->Outgoing;
         return ERR_Okay;
      }
      else return ERR_FieldNotSet;
   }
   else {
      if (Self->Outgoing.Type != CALL_NONE) {
         *Value = &Self->Outgoing;
         return ERR_Okay;
      }
      else return ERR_FieldNotSet;
   }
}

static ERROR SET_Outgoing(objNetSocket *Self, FUNCTION *Value)
{
   if (Self->Flags & NSF_SERVER) {
      objClientSocket *csocket;

      if (!(csocket = Self->CurrentSocket)) return PostError(ERR_Disconnected);

      if (Value) {
         if (csocket->Outgoing.Type IS CALL_SCRIPT) UnsubscribeAction(csocket->Outgoing.Script.Script, AC_Free);
         csocket->Outgoing = *Value;
         if (csocket->Outgoing.Type IS CALL_SCRIPT) SubscribeAction(csocket->Outgoing.Script.Script, AC_Free);

         if (Self->Head.Flags & NF_INITIALISED) {
            if (csocket->Handle != NOHANDLE) {
               #ifdef __linux__
                  RegisterFD((HOSTHANDLE)csocket->Handle, RFD_WRITE|RFD_SOCKET, &server_client_outgoing, csocket);
               #elif _WIN32
                  win_socketstate(csocket->Handle, -1, TRUE);
                  Self->WriteClientSocket = &server_client_outgoing;
               #endif
            }
            else LogErrorMsg("Will not listen for socket-writes (no socket handle on client).");
         }
         else return PostError(ERR_Disconnected);
      }
      else csocket->Outgoing.Type = CALL_NONE;
   }
   else {
      if (Self->Outgoing.Type IS CALL_SCRIPT) UnsubscribeAction(Self->Outgoing.Script.Script, AC_Free);
      Self->Outgoing = *Value;
      if (Self->Outgoing.Type IS CALL_SCRIPT) SubscribeAction(Self->Outgoing.Script.Script, AC_Free);

      if (Self->Head.Flags & NF_INITIALISED) {
         if ((Self->SocketHandle != NOHANDLE) AND (Self->State IS NTC_CONNECTED)) {
            // Setting the Outgoing field after connectivity is established will put the socket into streamed write mode.

            #ifdef __linux__
               RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_WRITE|RFD_SOCKET, (APTR)&client_server_outgoing, Self);
            #elif _WIN32
               win_socketstate(Self->SocketHandle, -1, TRUE);
               Self->WriteSocket = &client_server_outgoing;
            #endif
         }
         else MSG("Will not listen for socket-writes (no socket handle, or state %d != NTC_CONNECTED).", Self->State);
      }
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
MsgLimit: Limits the size of incoming and outgoing messages.

This field limits the size of incoming and outgoing message queues (each socket connection receives two queues assigned
to both incoming and outgoing messages).  The size is defined in bytes.  Sending or receiving messages that overflow
the queue results in the connection being terminated with an error.

The default setting is 1 megabyte.

-FIELD-
OutQueueSize: The number of bytes on the socket's outgoing queue.

*****************************************************************************/

static ERROR GET_OutQueueSize(objNetSocket *Self, LONG *Value)
{
   *Value = Self->WriteQueue.Length;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Port: The port number to use for initiating a connection.

-FIELD-
SocketHandle: Platform specific reference to the network socket handle.

*****************************************************************************/

static ERROR GET_SocketHandle(objNetSocket *Self, APTR *Value)
{
   *Value = (APTR)(MAXINT)Self->SocketHandle;
   return ERR_Okay;
}

static ERROR SET_SocketHandle(objNetSocket *Self, APTR Value)
{
   // The user can set SocketHandle prior to initialisation in order to create a NetSocket object that is linked to a
   // socket created from outside the core platform code base.

   Self->SocketHandle = (SOCKET_HANDLE)(MAXINT)Value;
   Self->ExternalSocket = TRUE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
State: The current connection state of the netsocket object.

*****************************************************************************/

static ERROR SET_State(objNetSocket *Self, LONG Value)
{
   if (Value != Self->State) {
      if (Self->Flags & NSF_DEBUG) LogMsg("State changed from %d to %d", Self->State, Value);

      #ifdef ENABLE_SSL
      if ((Self->State IS NTC_CONNECTING_SSL) AND (Value IS NTC_CONNECTED)) {
         // SSL connection has just been established

         if (SSL_get_verify_result(Self->SSL) != X509_V_OK) {
             // Handle the failed verification
             MSG("SSL certification was not validated.");
         }
         else MSG("SSL certification is valid.");
      }
      #endif

      Self->State = Value;

      if (Self->Feedback.Type) {
         FMSG("~","Reporting state change to subscriber, operation %d.", Self->State);

         if (Self->Feedback.Type IS CALL_STDC) {
            void (*routine)(objNetSocket *, LONG);
            OBJECTPTR context = SetContext(Self->Feedback.StdC.Context);
               routine = reinterpret_cast<void (*)(objNetSocket *, LONG)>(Self->Feedback.StdC.Routine);
               routine(Self, Self->State);
            SetContext(context);
         }
         else if (Self->Feedback.Type IS CALL_SCRIPT) {
            OBJECTPTR script;
            if ((script = Self->Feedback.Script.Script)) {
               const struct ScriptArg args[] = {
                  { "NetSocket", FD_OBJECTPTR, { .Address = Self } },
                  { "State",     FD_LONG,      { .Long = Self->State } }
               };
               scCallback(script, Self->Feedback.Script.ProcedureID, args, ARRAYSIZE(args));
            }
         }

         STEP();
      }

      if ((Self->State IS NTC_CONNECTED) AND ((Self->WriteQueue.Buffer) OR (Self->Outgoing.Type != CALL_NONE))) {
         MSG("Sending queued data to server on connection.");
         #ifdef __linux__
            RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_WRITE|RFD_SOCKET, (APTR)&client_server_outgoing, Self);
         #elif _WIN32
            win_socketstate(Self->SocketHandle, -1, TRUE);
            Self->WriteSocket = &client_server_outgoing;
         #endif
      }
   }

   SetResourcePtr(RES_EXCEPTION_HANDLER, NULL); // Stop winsock from fooling with our exception handler

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
TotalClients: Indicates the total number of clients currently connected to the socket (if in server mode).

In server mode, the netsocket will maintain a count of the total number of clients currently connected to the socket.
You can read the total number of connections from this field.

In client mode, this field is always set to zero.

-FIELD-
UserData: A user-defined pointer that can be useful in action notify events.

When some client code has subscribed to a method, such as MT_NsOConnect, and when the client code receives a
MT_NsOConnect event through ActionNotify, it can be helpful to have a user data pointer in order to retrieve some
application defined data structure.  Typically this field is set at object creation to point to some user data
structure.  For them on it can be used in ActionNotify to retrieve the structure.

-FIELD-
ValidCert: Indicates certificate validity if the socket is encrypted with a certificate.

After an encrypted connection has been made to a server, the ValidCert field can be used to determine the validity of
the server's certificate.

If encrypted communication is not supported, ERR_NoSupport is returned.  If the certificate is valid or the connection
is not encrypted, a value of zero is returned to indicate that the connection is valid.
-END-

*****************************************************************************/

static ERROR GET_ValidCert(objNetSocket *Self, LONG *Value)
{
#ifdef ENABLE_SSL
   if ((Self->SSL) AND (Self->State IS NTC_CONNECTED)) {
      *Value = SSL_get_verify_result(Self->SSL);
   }
   else *Value = 0;

   return ERR_Okay;
#else
   return ERR_NoSupport;
#endif
}

//****************************************************************************

static void free_socket(objNetSocket *Self)
{
   LogF("~free_socket()","Handle: %d", Self->SocketHandle);

   if (Self->SocketHandle != NOHANDLE) {
      FMSG("free_socket","Deregistering socket.");
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
      DeregisterFD((HOSTHANDLE)Self->SocketHandle);
#pragma GCC diagnostic warning "-Wint-to-pointer-cast"

      if (!Self->ExternalSocket) { CLOSESOCKET(Self->SocketHandle); Self->SocketHandle = NOHANDLE; }
   }

   #ifdef _WIN32
      Self->WriteSocket = NULL;
      Self->ReadSocket  = NULL;
   #endif

   FMSG("free_socket","Freeing I/O buffer queues.");

   if (Self->WriteQueue.Buffer) { FreeResource(Self->WriteQueue.Buffer); Self->WriteQueue.Buffer = NULL; }
   if (Self->ReadQueue.Buffer) { FreeResource(Self->ReadQueue.Buffer); Self->ReadQueue.Buffer = NULL; }

   if (!(Self->Head.Flags & NF_FREE)) {
      if (Self->State != NTC_DISCONNECTED) {
         FMSG("~free_socket:","Changing state to disconnected.");
         SetLong(Self, FID_State, NTC_DISCONNECTED);
         STEP();
      }
   }

   FMSG("free_socket","Resetting exception handler.");

   SetResourcePtr(RES_EXCEPTION_HANDLER, NULL); // Stop winsock from fooling with our exception handler

   LogBack();
}

//****************************************************************************

static ERROR write_queue(objNetSocket *Self, struct NetQueue *Queue, CPTR Message, LONG Length)
{
   FMSG("~write_queue()","Queuing a socket message of %d bytes.", Length);

   if (Queue->Buffer) {
      if (Queue->Index >= (ULONG)Queue->Length) {
         FMSG("write_queue","Terminating the current buffer (emptied).");
         FreeResource(Queue->Buffer);
         Queue->Buffer = NULL;
      }
   }

   if (Queue->Buffer) {
      // Add more information to an existing queue

      if (Queue->Length + Length > (ULONG)Self->MsgLimit) {
         FMSG("write_queue:","Cannot buffer message of %d bytes - it will overflow the MsgLimit.", Length);
         STEP();
         return ERR_BufferOverflow;
      }

      FMSG("write_queue","Extending current buffer to %d bytes.", Queue->Length + Length);

      if (Queue->Index) {
         // Compact the existing data if some of it has been sent
         CopyMemory((BYTE *)Queue->Buffer + Queue->Index, Queue->Buffer, Queue->Length - Queue->Index);
         Queue->Length -= Queue->Index;
         Queue->Index = 0;
      }

      // Adjust the buffer size

      if (!ReallocMemory(Queue->Buffer, Queue->Length + Length, &Queue->Buffer, NULL)) {
         CopyMemory(Message, (BYTE *)Queue->Buffer + Queue->Length, Length);
         Queue->Length += Length;
      }
      else {
         STEP();
         return ERR_ReallocMemory;
      }
   }
   else if (!AllocMemory(Length, MEM_NO_CLEAR, &Queue->Buffer, NULL)) {
      FMSG("write_queue","Allocated new buffer of %d bytes.", Length);
      Queue->Index = 0;
      Queue->Length = Length;
      CopyMemory(Message, Queue->Buffer, Length);
   }
   else {
      STEP();
      return PostError(ERR_AllocMemory);
   }

   STEP();
   return ERR_Okay;
}

// This function is called from winsockwrappers.c whenever a network event occurs on a NetSocket.  Callbacks
// set against the NetSocket object will send/receive data on the socket.
//
// Recursion typically occurs on calls to ProcessMessages() during incoming and outgoing data transmissions.  This is
// not important if the same transmission message is being repeated, but does require careful management if, for
// example, a disconnection were to occur during a read/write operation for example.  Using DataFeeds is a more
// reliable method of managing recursion problems, but burdens the message queue.

#ifdef _WIN32
void win32_netresponse(struct rkNetSocket *Self, SOCKET_HANDLE Socket, LONG Message, ERROR Error)
{
   #ifdef DEBUG
   static const CSTRING msg[] = {
      "None",
      "Write",
      "Read",
      "Accept",
      "Connect",
      "Close"
   };
   #endif

   FMSG("~win32_netresponse()","[%d:%p], %s, Error %d, InUse: %d, WinRecursion: %d", Self->Head.UniqueID, Self, msg[Message], Error, Self->InUse, Self->WinRecursion);

   Self->InUse++;

   if (Message IS NTE_READ) {
      if (Error) LogErrorMsg("Socket failed on incoming data, error %d.", Error);

      #ifdef NO_NETRECURSION
         if (Self->WinRecursion) {
            FMSG("win32_netsponse","Recursion detected (read request)");
            Self->InUse--;
            STEP();
            return;
         }
         else {
            Self->WinRecursion++;
            if (Self->Flags & NSF_SERVER) {
               struct rkNetClient *client;
               objClientSocket *clientsocket;
               for (client=Self->Clients; client; client=client->Next) {
                  for (clientsocket=client->Sockets; clientsocket; clientsocket=clientsocket->Next) {
                     if (clientsocket->Handle IS Socket) {
                        server_client_incoming(Socket, clientsocket);
                        break;
                     }
                  }
                  if (clientsocket) break;
               }
            }
            else if (Self->ReadSocket) Self->ReadSocket(0, Self);
            else client_server_incoming(0, Self);
            Self->WinRecursion--;
         }
      #else
         if (Self->ReadSocket) Self->ReadSocket(0, Self);
         else client_server_incoming(0, Self);
      #endif
   }
   else if (Message IS NTE_WRITE) {
      if (Error) LogErrorMsg("Socket failed on outgoing data, error %d.", Error);

      #ifdef NO_NETRECURSION
         if (Self->WinRecursion) {
            FMSG("win32_netsponse","Recursion detected (write request)");
            Self->InUse--;
            STEP();
            return;
         }
         else {
            Self->WinRecursion++;
            if (Self->Flags & NSF_SERVER) {
               struct rkNetClient *client;
               objClientSocket *clientsocket;
               for (client=Self->Clients; client; client=client->Next) {
                  for (clientsocket=client->Sockets; clientsocket; clientsocket=clientsocket->Next) {
                     if (clientsocket->Handle IS Socket) {
                        server_client_outgoing(Socket, clientsocket);
                        break;
                     }
                  }
                  if (clientsocket) break;
               }
            }
            else if (Self->WriteSocket) Self->WriteSocket(0, Self);
            else client_server_outgoing(0, Self);
            Self->WinRecursion--;
         }
      #else
         if (Self->WriteSocket) Self->WriteSocket(0, Self);
         else client_server_outgoing(0, Self);
      #endif
   }
   else if (Message IS NTE_CLOSE) {
      LogF("win32_netresponse","Socket closed by server, error %d.", Error);
      if (Self->State != NTC_DISCONNECTED) SetLong(Self, FID_State, NTC_DISCONNECTED);
      free_socket(Self);
   }
   else if (Message IS NTE_ACCEPT) {
      FMSG("win32_netresponse","Accept message received for new client.");
      server_client_connect(Self->SocketHandle, Self);
   }
   else if (Message IS NTE_CONNECT) {
      if (Error IS ERR_Okay) {
         LogF("win32_netresponse","Connection to server granted.");

         #ifdef ENABLE_SSL
            if (Self->SSL) {
               // Perform the SSL handshake

               FMSG("~server_connect","Attempting SSL handshake.");

                  sslConnect(Self);

               STEP();

               if (Self->State IS NTC_CONNECTING_SSL) {
                  //RegisterFD((HOSTHANDLE)Self->SocketHandle, RFD_READ|RFD_SOCKET, &client_server_incoming, Self);
               }
            }
            else SetLong(Self, FID_State, NTC_CONNECTED);
         #else
            SetLong(Self, FID_State, NTC_CONNECTED);
         #endif
      }
      else {
         LogF("win32_netresponse","Connection state changed, error: %s", GetErrorMsg(Error));
         Self->Error = Error;
         SetLong(Self, FID_State, NTC_DISCONNECTED);
      }
   }

   Self->InUse--;
   STEP();
}
#endif

//****************************************************************************

static const struct FieldDef clValidCert[] = {
   { "Okay",                          SCV_OK },                                 // The operation was successful.
   { "UnableToGetIssuerCert",         SCV_UNABLE_TO_GET_ISSUER_CERT },          // unable to get issuer certificate the issuer certificate could not be found: this occurs if the issuer certificate of an untrusted certificate cannot be found.
   { "UnableToGetCRL",                SCV_UNABLE_TO_GET_CRL },                  // unable to get certificate CRL the CRL of a certificate could not be found. Unused.
   { "UnableToDecryptCertSignature",  SCV_UNABLE_TO_DECRYPT_CERT_SIGNATURE },   // unable to decrypt certificate's signature the certificate signature could not be decrypted. This means that the actual signature value could not be determined rather than it not matching the expected value, this is only meaningful for RSA keys.
   { "UnableToDecryptCRLSignature",   SCV_UNABLE_TO_DECRYPT_CRL_SIGNATURE },    // unable to decrypt CRL's signature the CRL signature could not be decrypted: this means that the actual signature value could not be determined rather than it not matching the expected value. Unused.
   { "UnableToDecodeIssuerPublicKey", SCV_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY }, // unable to decode issuer public key the public key in the certificate SubjectPublicKeyInfo could not be read.
   { "CertSignatureFailure",          SCV_CERT_SIGNATURE_FAILURE },             // certificate signature failure the signature of the certificate is invalid.
   { "CRLSignuatreFailure",           SCV_CRL_SIGNATURE_FAILURE },              // CRL signature failure the signature of the certificate is invalid. Unused.
   { "CertNotYetValid",               SCV_CERT_NOT_YET_VALID },                 // certificate is not yet valid the certificate is not yet valid: the notBefore date is after the current time.
   { "CertHasExpired",                SCV_CERT_HAS_EXPIRED },                   // certificate has expired the certificate has expired: that is the notAfter date is before the current time.
   { "CRLNotYetValid",                SCV_CRL_NOT_YET_VALID },                  // CRL is not yet valid the CRL is not yet valid. Unused.
   { "CRLHasExpired",                 SCV_CRL_HAS_EXPIRED },                    // CRL has expired the CRL has expired. Unused.
   { "ErrorInCertNotBeforeField",     SCV_ERROR_IN_CERT_NOT_BEFORE_FIELD },     // format error in certificate's notBefore field the certificate notBefore field contains an invalid time.
   { "ErrorInCertNotAfterField",      SCV_ERROR_IN_CERT_NOT_AFTER_FIELD },      // format error in certificate's notAfter field the certificate notAfter field contains an invalid time.
   { "ErrorInCRLLastUpdateField",     SCV_ERROR_IN_CRL_LAST_UPDATE_FIELD },     // format error in CRL's lastUpdate field the CRL lastUpdate field contains an invalid time. Unused.
   { "ErrorInCRLNextUpdateField",     SCV_ERROR_IN_CRL_NEXT_UPDATE_FIELD },     // format error in CRL's nextUpdate field the CRL nextUpdate field contains an invalid time. Unused.
   { "OutOfMemory",                   SCV_OUT_OF_MEM },                         // out of memory an error occurred trying to allocate memory. This should never happen.
   { "DepthZeroSelfSignedCert",       SCV_DEPTH_ZERO_SELF_SIGNED_CERT },        // self signed certificate the passed certificate is self signed and the same certificate cannot be found in the list of trusted certificates.
   { "SelfSignedCertInChain",         SCV_SELF_SIGNED_CERT_IN_CHAIN },          // self signed certificate in certificate chain the certificate chain could be built up using the untrusted certificates but the root could not be found locally.
   { "UnableToGetIssuerCertLocally",  SCV_UNABLE_TO_GET_ISSUER_CERT_LOCALLY },  // unable to get local issuer certificate the issuer certificate of a locally looked up certificate could not be found. This normally means the list of trusted certificates is not complete.
   { "UnableToVertifyLeafSignature",  SCV_UNABLE_TO_VERIFY_LEAF_SIGNATURE },    // unable to verify the first certificate no signatures could be verified because the chain contains only one certificate and it is not self signed.
   { "CertChainTooLong",              SCV_CERT_CHAIN_TOO_LONG },                // certificate chain too long the certificate chain length is greater than the supplied maximum depth. Unused.
   { "CertRevoked",                   SCV_CERT_REVOKED },                       // certificate revoked the certificate has been revoked. Unused.
   { "InvalidCA",                     SCV_INVALID_CA },                         // invalid CA certificate a CA certificate is invalid. Either it is not a CA or its extensions are not consistent with the supplied purpose.
   { "PathLengthExceeded",            SCV_PATH_LENGTH_EXCEEDED },               // path length constraint exceeded the basicConstraints pathlength parameter has been exceeded.
   { "InvalidPurpose",                SCV_INVALID_PURPOSE },                    // unsupported certificate purpose the supplied certificate cannot be used for the specified purpose.
   { "CertUntrusted",                 SCV_CERT_UNTRUSTED },                     // certificate not trusted the root CA is not marked as trusted for the specified purpose.
   { "CertRejected",                  SCV_CERT_REJECTED },                      // certificate rejected the root CA is marked to reject the specified purpose.
   { "SubjectIssuerMismatch",         SCV_SUBJECT_ISSUER_MISMATCH },            // subject issuer mismatch the current candidate issuer certificate was rejected because its subject name did not match the issuer name of the current certificate. Only dis-played when the -issuer_checks option is set.
   { "KeyMismatch",                   SCV_AKID_SKID_MISMATCH },                 // authority and subject key identifier mismatch the current candidate issuer certificate was rejected because its subject key identifier was present and did not match the authority key identifier current certificate. Only displayed when the -issuer_checks option is set.
   { "KeyIssuerSerialMismatch",       SCV_AKID_ISSUER_SERIAL_MISMATCH },        // authority and issuer serial number mismatch the current candidate issuer certificate was rejected because its issuer name and serial number was present and did not match the authority key identifier of the current certificate. Only displayed when the -issuer_checks option is set.
   { "KeyUsageNoCertSign",            SCV_KEYUSAGE_NO_CERTSIGN },               // key usage does not include certificate signing the current candidate issuer certificate was rejected because its keyUsage extension does not permit certificate signing.
   { "ApplicationVerification",       SCV_APPLICATION_VERIFICATION },           // application verification failure an application specific error. Unused.
   { 0, 0 },
};

#include "netsocket_def.c"

static const struct FieldArray clSocketFields[] = {
   { "Clients",          FDF_POINTER|FDF_STRUCT|FDF_R, (MAXINT)&"NetClient", NULL, NULL },
   { "CurrentClient",    FDF_OBJECT|FDF_RW,    ID_CLIENTSOCKET, NULL, NULL },
   { "UserData",         FDF_POINTER|FDF_RW,   0, NULL, NULL },
   { "Address",          FDF_STRING|FDF_RI,    0, NULL, (APTR)SET_Address },
   { "State",            FDF_LONG|FDF_LOOKUP|FDF_RW, (MAXINT)&clNetSocketState, NULL, (APTR)SET_State },
   { "Error",            FDF_LONG|FDF_R,       0, NULL, NULL },
   { "Port",             FDF_LONG|FDF_RI,      0, NULL, NULL },
   { "Flags",            FDF_LONGFLAGS|FDF_RW, (MAXINT)&clNetSocketFlags, NULL, NULL },
   { "TotalClients",     FDF_LONG|FDF_R,       0, NULL, NULL },
   { "Backlog",          FDF_LONG|FDF_RI,      0, NULL, NULL },
   { "ClientLimit",      FDF_LONG|FDF_RW,      0, NULL, NULL },
   { "MsgLimit",         FDF_LONG|FDF_RI,      0, NULL, NULL },
   // Virtual fields
   { "SocketHandle",     FDF_POINTER|FDF_RI,      0, (APTR)GET_SocketHandle, (APTR)SET_SocketHandle },
   { "Feedback",         FDF_FUNCTIONPTR|FDF_RW,  0, (APTR)GET_Feedback, (APTR)SET_Feedback },
   { "Incoming",         FDF_FUNCTIONPTR|FDF_RW,  0, (APTR)GET_Incoming, (APTR)SET_Incoming },
   { "Outgoing",         FDF_FUNCTIONPTR|FDF_RW,  0, (APTR)GET_Outgoing, (APTR)SET_Outgoing },
   { "OutQueueSize",     FDF_LONG|FDF_R,          0, (APTR)GET_OutQueueSize },
   { "ValidCert",        FDF_LONG|FDF_LOOKUP,     (MAXINT)&clValidCert, (APTR)GET_ValidCert, NULL },
   END_FIELD
};

#include "netsocket_server.cpp"
#include "netsocket_client.cpp"

//****************************************************************************

static ERROR add_netsocket(void)
{
   if (CreateObject(ID_METACLASS, 0, &clNetSocket,
      FID_ClassVersion|TFLOAT, VER_NETSOCKET,
      FID_Name|TSTRING,   "NetSocket",
      FID_Category|TLONG, CCF_NETWORK,
      FID_Actions|TPTR,   clNetSocketActions,
      FID_Methods|TARRAY, clNetSocketMethods,
      FID_Fields|TARRAY,  clSocketFields,
      FID_Size|TLONG,     sizeof(objNetSocket),
      FID_Path|TSTR,      MOD_PATH,
      TAGEND) != ERR_Okay) return ERR_CreateObject;

   return ERR_Okay;
}