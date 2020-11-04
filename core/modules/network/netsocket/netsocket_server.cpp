
//****************************************************************************
// Called when a socket handle detects a new client wanting to connect to it.

static void server_client_connect(SOCKET_HANDLE FD, objNetSocket *Self)
{
   UBYTE ip[8];
   SOCKET_HANDLE clientfd;

   FMSG("~socket_connect()","FD: %d", FD);

   OBJECTPTR context = SetContext(Self);

   if (Self->IPV6) {
#ifdef __linux__

      #ifdef ENABLE_SSL_ACCEPT
         // NB: I think we can just use a standard accept() and then wait
         // for it to communicate via SSL or create an SSL connection object
         // for it???  See BIO_s_accept() documentation for other ideas.

         if (Self->SSL) {
            LONG result;
            if ((result = xtSSL_accept(Self->SSL)) != 1) {
               LogErrorMsg("SSL_accept: %s", xtERR_error_string(xtSSL_get_error(Self->SSL, result), NULL));
            }

            LogErrorMsg("No support for retrieving IPV6 address and client handle yet.");

            clientfd = NOHANDLE;

         }
         else {
            struct sockaddr_in6 addr;
            socklen_t len = sizeof(addr);
            clientfd = accept(FD, (struct sockaddr *)&addr, &len);
            if (clientfd IS NOHANDLE) { STEP(); return; }
            ip[0] = addr.sin6_addr.s6_addr[0];
            ip[1] = addr.sin6_addr.s6_addr[1];
            ip[2] = addr.sin6_addr.s6_addr[2];
            ip[3] = addr.sin6_addr.s6_addr[3];
            ip[4] = addr.sin6_addr.s6_addr[4];
            ip[5] = addr.sin6_addr.s6_addr[5];
            ip[6] = addr.sin6_addr.s6_addr[6];
            ip[7] = addr.sin6_addr.s6_addr[7];
         }
      #else
         struct sockaddr_in6 addr;
         socklen_t len = sizeof(addr);
         clientfd = accept(FD, (struct sockaddr *)&addr, &len);
         if (clientfd IS NOHANDLE) { STEP(); return; }
         ip[0] = addr.sin6_addr.s6_addr[0];
         ip[1] = addr.sin6_addr.s6_addr[1];
         ip[2] = addr.sin6_addr.s6_addr[2];
         ip[3] = addr.sin6_addr.s6_addr[3];
         ip[4] = addr.sin6_addr.s6_addr[4];
         ip[5] = addr.sin6_addr.s6_addr[5];
         ip[6] = addr.sin6_addr.s6_addr[6];
         ip[7] = addr.sin6_addr.s6_addr[7];
      #endif
#else
      LogF("@socket_connect","IPV6 not supported yet.");
      SetContext(context);
      STEP();
      return;
#endif
   }
   else {
      struct sockaddr_in addr;

      #ifdef __linux__
         socklen_t len = sizeof(addr);
         clientfd = accept(FD, (struct sockaddr *)&addr, &len);
      #elif _WIN32
         LONG len = sizeof(addr);
         clientfd = win_accept(Self, FD, (struct sockaddr *)&addr, &len);
      #endif

      if (clientfd IS NOHANDLE) {
         LogF("@server_connect","accept() failed to return an FD.");
         SetContext(context);
         STEP();
         return;
      }

      ip[0] = addr.sin_addr.s_addr;
      ip[1] = addr.sin_addr.s_addr>>8;
      ip[2] = addr.sin_addr.s_addr>>16;
      ip[3] = addr.sin_addr.s_addr>>24;
      ip[4] = 0;
      ip[5] = 0;
      ip[6] = 0;
      ip[7] = 0;
   }

   if (Self->TotalClients >= Self->ClientLimit) {
      CLOSESOCKET(clientfd);
      PostError(ERR_ArrayFull);
      SetContext(context);
      STEP();
      return;
   }

   // Check if this IP address already has a client structure from an earlier socket connection.
   // (One NetClient represents a single IP address; Multiple ClientSockets can connect from that IP address)

   struct rkNetClient *client_ip;
   for (client_ip=Self->Clients; client_ip; client_ip=client_ip->Next) {
      if (((LARGE *)&ip)[0] IS ((LARGE *)&client_ip->IP)[0]) break;
   }

   if (!client_ip) {
      if (AllocMemory(sizeof(struct rkNetClient), MEM_DATA, &client_ip, NULL) != ERR_Okay) {
         CLOSESOCKET(clientfd);
         SetContext(context);
         STEP();
         return;
      }

      client_ip->NetSocket = Self;
      ((LARGE *)&client_ip->IP)[0] = ((LARGE *)&ip)[0];
      client_ip->TotalSockets = 0;
      Self->TotalClients++;

      if (!Self->Clients) Self->Clients = client_ip;
      else {
         if (Self->LastClient) Self->LastClient->Next = client_ip;
         if (Self->Clients) Self->Clients->Prev = Self->LastClient;
      }
      Self->LastClient = client_ip;
   }

   if (!(Self->Flags & NSF_MULTI_CONNECT)) { // Check if the IP is already registered and alive
      if (client_ip->Sockets) {
         // Check if the client is alive by writing to it.  If the client is dead, remove it and continue with the new connection.

         LogF("socket_connect","Preventing second connection attempt from IP %d.%d.%d.%d\n", client_ip->IP[0], client_ip->IP[1], client_ip->IP[2], client_ip->IP[3]);
         CLOSESOCKET(clientfd);
         SetContext(context);
         STEP();
         return;
      }
   }

   // Socket Management

   objClientSocket *client_socket;
   if (!NewObject(ID_CLIENTSOCKET, 0, &client_socket)) {
      client_socket->Handle = clientfd;
      client_socket->Client = client_ip;
      acInit(client_socket);
   }
   else {
      CLOSESOCKET(clientfd);
      if (!client_ip->Sockets) free_client(Self, client_ip);
      SetContext(context);
      STEP();
      return;
   }

   if (Self->Feedback.Type IS CALL_STDC) {
      void (*routine)(objNetSocket *, objClientSocket *, LONG);
      OBJECTPTR context = SetContext(Self->Feedback.StdC.Context);
         routine = reinterpret_cast<void (*)(objNetSocket *, objClientSocket *, LONG)>(Self->Feedback.StdC.Routine);
         routine(Self, client_socket, NTC_CONNECTED);
      SetContext(context);
   }
   else if (Self->Feedback.Type IS CALL_SCRIPT) {
      const struct ScriptArg args[] = {
         { "NetSocket",    FD_OBJECTPTR, { .Address = Self } },
         { "ClientSocket", FD_OBJECTPTR, { .Address = client_socket } },
         { "State",        FD_LONG,      { .Long = NTC_CONNECTED } }
      };

      OBJECTPTR script;
      if ((script = Self->Feedback.Script.Script)) {
         scCallback(script, Self->Feedback.Script.ProcedureID, args, ARRAYSIZE(args));
      }
   }

   FMSG("socket_connect","Total clients: %d", Self->TotalClients);
   SetContext(context);
   STEP();
}

/*****************************************************************************
** If the socket is a server and has data queued against a client, this routine is called.
*/

static void server_client_outgoing(SOCKET_HANDLE FD, objClientSocket *Socket)
{
   if ((!Socket) OR (!Socket->Client)) {
      FMSG("@server_outgoing()","No Socket or Socket->Client.");
      return;
   }

   objNetSocket *Self = Socket->Client->NetSocket;

   if (Self->Terminating) return;

   FMSG("~server_outgoing()","%d", FD);

   Self->InUse++;

   ERROR error = ERR_Okay;

   // Send out remaining queued data before getting new data to send

   if (Socket->WriteQueue.Buffer) {
      while (Socket->WriteQueue.Buffer) {
         LONG len = Socket->WriteQueue.Length - Socket->WriteQueue.Index;
         if (len > glMaxWriteLen) len = glMaxWriteLen;

         if (len > 0) {
            error = SEND(Self, FD, (BYTE *)Socket->WriteQueue.Buffer + Socket->WriteQueue.Index, &len, 0);

            if ((error) OR (!len)) break;

            FMSG("server_outgoing","[NetSocket:%d] Sent %d of %d bytes remaining on the queue.", Self->Head.UniqueID, len, Socket->WriteQueue.Length-Socket->WriteQueue.Index);

            Socket->WriteQueue.Index += len;
         }

         if (Socket->WriteQueue.Index >= Socket->WriteQueue.Length) {
            FMSG("server_outgoing","Terminating the write queue (pos %d/%d).", Socket->WriteQueue.Index, Socket->WriteQueue.Length);
            FreeResource(Socket->WriteQueue.Buffer);
            Socket->WriteQueue.Buffer = NULL;
            Socket->WriteQueue.Index  = 0;
            Socket->WriteQueue.Length = 0;
            break;
         }
      }
   }

   // Before feeding new data into the queue, the current buffer must be empty.

   if ((!Socket->WriteQueue.Buffer) OR (Socket->WriteQueue.Index >= Socket->WriteQueue.Length)) {
      if (Socket->Outgoing.Type != CALL_NONE) {
         Self->CurrentSocket = Socket;

            if (Socket->Outgoing.Type IS CALL_STDC) {
               ERROR (*routine)(objNetSocket *, objClientSocket *);
               routine = reinterpret_cast<ERROR (*)(objNetSocket *, objClientSocket *)>(Socket->Outgoing.StdC.Routine);
               OBJECTPTR context = SetContext(Socket->Outgoing.StdC.Context);
                  error = routine(Self, Socket);
               SetContext(context);
            }
            else if (Socket->Outgoing.Type IS CALL_SCRIPT) {
               OBJECTPTR script;
               const struct ScriptArg args[] = {
                  { "NetSocket", FD_OBJECTPTR, { .Address = Self } },
                  { "Socket",    FD_OBJECTPTR, { .Address = Socket } }
               };

               if ((script = Socket->Outgoing.Script.Script)) {
                  if (!scCallback(script, Socket->Outgoing.Script.ProcedureID, args, ARRAYSIZE(args))) {
                     GetLong(script, FID_Error, &error);
                  }
                  else error = ERR_Terminate; // Fatal error in attempting to execute the procedure
               }
            }

            if (error) Socket->Outgoing.Type = CALL_NONE;

         Self->CurrentSocket = NULL;
      }

      // If the write queue is empty and all data has been retrieved, we can remove the FD-Write registration so that
      // we don't tax the system resources.

      if (!Socket->WriteQueue.Buffer) {
         FMSG("server_outgoing","[NetSocket:%d] Write-queue listening on FD %d will now stop.", Self->Head.UniqueID, FD);
         RegisterFD((HOSTHANDLE)(MAXINT)FD, RFD_REMOVE|RFD_WRITE|RFD_SOCKET, NULL, NULL);
         #ifdef _WIN32
         win_socketstate(FD, -1, 0);
         #endif
      }
   }
   else FMSG("server_outgoing","Outgoing buffer is not empty.");

   Self->InUse--;

   STEP();
}

/*****************************************************************************
** Terminates the connection to the client and removes associated resources.
*/

static void free_client(objNetSocket *Self, struct rkNetClient *Client)
{
   static THREADVAR BYTE recursive = 0;

   if (!Client) return;
   if (recursive) return;

   recursive++;

   LogF("~free_client()","%d:%d:%d:%d, Sockets: %d", Client->IP[0], Client->IP[1], Client->IP[2], Client->IP[3], Client->TotalSockets);

   // Free all sockets related to this client

   while (Client->Sockets) {
      objClientSocket *current_socket = Client->Sockets;
      free_client_socket(Self, Client->Sockets, TRUE);
      if (Client->Sockets IS current_socket) {
         LogF("@free_client","Resource management error detected in Client->Sockets");
         break;
      }
   }

   if (Client->Prev) {
      Client->Prev->Next = Client->Next;
      if (Client->Next) Client->Next->Prev = Client->Prev;
   }
   else {
      Self->Clients = Client->Next;
      if ((Self->Clients) AND (Self->Clients->Next)) Self->Clients->Next->Prev = NULL;
   }

   FreeResource(Client);

   Self->TotalClients--;

   recursive--;
   LogBack();
}

/*****************************************************************************
** Terminates the connection to the client and removes associated resources.
*/

static void free_client_socket(objNetSocket *Socket, objClientSocket *ClientSocket, BYTE Signal)
{
   if (!ClientSocket) return;

   LogF("~free_client_socket()","Handle: %d, NetSocket: %d, ClientSocket: %d", ClientSocket->SocketHandle, Socket->Head.UniqueID, ClientSocket->Head.UniqueID);

   if ((Signal) AND (Socket->Feedback.Type)) {
      if (Socket->Feedback.Type IS CALL_STDC) {
         void (*routine)(objNetSocket *, objClientSocket *, LONG);
         OBJECTPTR context = SetContext(Socket->Feedback.StdC.Context);
            routine = reinterpret_cast<void (*)(objNetSocket *, objClientSocket *, LONG)>(Socket->Feedback.StdC.Routine);
            routine(Socket, ClientSocket, NTC_DISCONNECTED);
         SetContext(context);
      }
      else if (Socket->Feedback.Type IS CALL_SCRIPT) {
         const struct ScriptArg args[] = {
            { "NetSocket",    FD_OBJECTPTR, { .Address = Socket } },
            { "ClientSocket", FD_OBJECTPTR, { .Address = ClientSocket } },
            { "State",        FD_LONG,      { .Long = NTC_DISCONNECTED } }
         };

         OBJECTPTR script;
         if ((script = Socket->Feedback.Script.Script)) {
            scCallback(script, Socket->Feedback.Script.ProcedureID, args, ARRAYSIZE(args));
         }
      }
   }

   acFree(ClientSocket);

   LogBack();
}
