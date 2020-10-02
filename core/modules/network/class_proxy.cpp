/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
Proxy: Manages user settings for proxy servers.

The proxy server class provides a global management service for a user's proxy servers.  You can alter proxy settings
manually or present the user with a dialog box to edit and create new proxies.  Scanning functions are also provided
with filtering, allowing you to scan for proxies that should be used with the user's network connection.

Proxy objects are designed to work similarly to database recordsets. Creating a new proxy object will allow you to
create a new proxy record if all required fields are set and the object is saved.

Searching through the records with the #Find() and #FindNext() methods will move the recordset through
each entry the proxy database.  You may change existing values of any proxy and then save the changes by calling the
#SaveSettings() action.
-END-

*****************************************************************************/

#define PRV_PROXY

#ifdef _WIN32
#define HKEY_PROXY "\\HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\"
#endif

static objConfig *glConfig = NULL; // NOT THREAD SAFE

static ERROR find_proxy(objProxy *);
static void clear_values(objProxy *);
static ERROR get_record(objProxy *);

/*
static void free_proxy(void)
{
 if (glConfig) { acFree(glConfig); glConfig = NULL; }
}
*/

/*****************************************************************************

-METHOD-
Delete: Removes a proxy from the database.

Call the Delete method to remove a proxy from the system.  The proxy will be permanently removed from the proxy
database on the success of this function.

-ERRORS-
Okay: Proxy deleted.
-END-

*****************************************************************************/

static ERROR PROXY_Delete(objProxy *Self, APTR Void)
{
   if ((!Self->Section[0]) OR (!Self->Record)) return PostError(ERR_Failed);

   LogBranch(NULL);

   if (Self->Host) {
      #ifdef _WIN32

      #endif
   }

   if (glConfig) { acFree(glConfig); glConfig = NULL; }

   if (!CreateObject(ID_CONFIG, NF_UNTRACKED, &glConfig,
         FID_Path|TSTR,  "user:config/network/proxies.cfg",
         TAGEND)) {
      cfgDeleteSection(glConfig, Self->Section);
      acSaveSettings(glConfig);
   }

   LogBack();
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Disable: Marks a proxy as disabled.

Calling the Disable action will mark the proxy as disabled.  Disabled proxies remain in the system but are ignored by
programs that scan the database for active proxies.

The change will not come into effect until the proxy record is saved.

*****************************************************************************/

static ERROR PROXY_Disable(objProxy *Self, APTR Void)
{
   Self->Enabled = FALSE;
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Enable: Enables a proxy.

Calling the Enable action will mark the proxy as enabled.  The change will not come into effect until the proxy record
is saved.

*****************************************************************************/

static ERROR PROXY_Enable(objProxy *Self, APTR Void)
{
   Self->Enabled = TRUE;
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Find: Search for a proxy that matches a set of filters.

The following example searches for all proxies available for use on port 80 (HTTP).

<pre>
if (!(CreateObject(ID_PROXY, 0, &proxy, TAGEND))) {
   if (!prxFind(proxy, 80)) {
      do {
         ...
      } while (!prxFindNext(proxy));
      FreeResource(proxy);
   }
   acFree(proxy);
}
</pre>

-INPUT-
int Port: The port number  to access.  If zero, all proxies will be returned if you perform a looped search.
int Enabled: Set to TRUE to return only enabled proxies, FALSE for disabled proxies or -1 for all proxies.

-ERRORS-
Okay: A proxy was discovered.
NoSearchResult: No matching proxy was discovered.
-END-

*****************************************************************************/

static ERROR PROXY_Find(objProxy *Self, struct prxFind *Args)
{
   MSG("Port: %d, Enabled: %d", (Args) ? Args->Port : 0, (Args) ? Args->Enabled : -1);

   // Remove the previous cache of the proxy database

   if (glConfig) { acFree(glConfig); glConfig = NULL; }

   // Load the current proxy database into the cache

   if (!CreateObject(ID_CONFIG, NF_UNTRACKED, &glConfig,
         FID_Path|TSTR, "user:config/network/proxies.cfg",
         TAGEND)) {

      #ifdef _WIN32

         CSTRING override, servers, section;
         OBJECTPTR task;
         BYTE bypass;
         LONG port, total, j, i, index, lastsection, enabled;
         CSTRING name, value;

         // Remove any existing host proxy settings

         section = NULL;
         for (i=0; i < glConfig->AmtEntries; i++) {
            if (!StrMatch(section, glConfig->Entries[i].Section)) continue;

            section = glConfig->Entries[i].Section;
            lastsection = i;
            do {
               if (!StrMatch("Host", glConfig->Entries[i].Key)) {
                  if (!cfgDeleteSection(glConfig, glConfig->Entries[i].Section)) {
                     i = lastsection;
                     break;
                  }
               }
               i++;
            } while ((i < glConfig->AmtEntries) AND (!StrMatch(section, glConfig->Entries[i].Section)));
         }

         // Add the host system's current proxy settings

         bypass = FALSE;
         task = CurrentTask();
         if (!taskGetEnv(task, HKEY_PROXY "ProxyEnable", &value)) {
            enabled = StrToInt(value);

               // If ProxyOverride is set and == <local> then you should bypass the proxy for local addresses.

               if (!taskGetEnv(task, HKEY_PROXY "ProxyOverride", &override)) {
                  if (!StrMatch("<local>", override)) {
                     bypass = TRUE;
                  }
               }

               if ((!taskGetEnv(task, HKEY_PROXY "ProxyServer", &servers)) AND (servers[0])) {
                  LogMsg("Host has defined default proxies: %s", servers);

                  total = 0;
                  port  = 0;
                  name  = NULL;
                  i     = 0;
                  index = 0;
                  while (TRUE) {
                     if (!StrCompare("ftp=", servers+i, 4, 0)) {
                        name = "Windows FTP";
                        port = 21;
                        index = i + 4;
                     }
                     else if (!StrCompare("http=", servers+i, 5, 0)) {
                        name = "Windows HTTP";
                        port = 80;
                        index = i + 5;
                     }
                     else if (!StrCompare("https=", servers+i, 6, 0)) {
                        name = "Windows HTTPS";
                        port = 443;
                        index = i + 6;
                     }
                     else {
                        // If a string is set with no equals, it will be a global proxy server.  Looks like "something.com:80"

                        for (j=i; servers[j] AND (servers[j] != ':'); j++) {
                           if (servers[j] IS '=') break;
                        }

                        if (servers[j] IS ':') {
                           index = i;
                           name = "Windows";
                           port = 0; // Proxy applies to all ports
                        }
                        else {
                           name = NULL;
                           port = -1;
                        }
                     }

                     if ((name) AND (port != -1)) {
                        LONG id, serverport;
                        char section[32];
                        char server[80];

                        id = 0;
                        cfgReadInt(glConfig, "ID", "Value", &id);
                        id = id + 1;
                        cfgWriteInt(glConfig, "ID", "Value", id);

                        IntToStr(id, section, sizeof(section));

                        size_t s;
                        for (s=0; servers[index+s] AND (servers[index+s] != ':') AND (s < sizeof(server)-1); s++) server[s] = servers[index+s];
                        server[s] = 0;

                        if (servers[index+s] IS ':') {
                           serverport = StrToInt(servers + index + s + 1);

                           MSG("Discovered proxy server %s, port %d", server, serverport);

                           cfgWriteValue(glConfig, section, "Name", name);
                           cfgWriteValue(glConfig, section, "Server", server);

                           if (enabled > 0) {
                              cfgWriteInt(glConfig, section, "Enabled", enabled);
                           }
                           else cfgWriteInt(glConfig, section, "Enabled", enabled);

                           cfgWriteInt(glConfig, section, "Port", port);
                           cfgWriteInt(glConfig, section, "ServerPort", serverport);
                           cfgWriteInt(glConfig, section, "Host", 1); // Indicate that this proxy originates from host OS settings

                           i = index + s;
                        }
                     }

                     while ((servers[i]) AND (servers[i] != ';')) i++;

                     if (!servers[i]) break;
                     i++;
                  }
               }
         }
         else LogMsg("Host does not have proxies enabled (registry setting: %s)", HKEY_PROXY);

      #endif

      if (Args) {
         if (Args->Port > 0) IntToStr(Args->Port, Self->FindPort, sizeof(Self->FindPort));
         else Self->FindPort[0] = 0;
         Self->FindEnabled = Args->Enabled;
      }
      else {
         Self->FindPort[0] = 0;
         Self->FindEnabled = -1;
      }

      Self->Section[0] = 0;

      return find_proxy(Self);
   }
   else return ERR_AccessObject;
}

/*****************************************************************************

-METHOD-
FindNext: Continues an initiated search.

This method continues searches that have been initiated by the #Find() method. If a proxy is found that matches
the filter, ERR_Okay is returned and the details of the proxy object will reflect the data of the discovered record.
ERR_NoSearchResult is returned if there are no more matching proxies.

-ERRORS-
Okay: A proxy was discovered.
NoSearchResult: No matching proxy was discovered.
-END-

*****************************************************************************/

static ERROR PROXY_FindNext(objProxy *Self, APTR Void)
{
   if (!Self->Find) return ERR_NoSearchResult; // Ensure that Find() was used to initiate a search

   return find_proxy(Self);
}

//****************************************************************************

static ERROR find_proxy(objProxy *Self)
{
   LONG i, j, num, id;
   CSTRING str;
   UBYTE match;

   clear_values(Self);

   if (!glConfig) {
      MSG("Global config not loaded.");
      return ERR_NoSearchResult;
   }

   if (!Self->Find) { // This is the start of the search
      Self->Find = TRUE;
   }

   if (Self->Section[0]) { // Find the current section/record
      MSG("Finding current record.");

      for (i=0; i < glConfig->AmtEntries; i++) {
         if (!StrMatch(Self->Section, glConfig->Entries[i].Section)) break;
      }

      // Move to the next section/record

      MSG("Moving to next record.");

      for (; i < glConfig->AmtEntries; i++) {
         if (StrMatch(Self->Section, glConfig->Entries[i].Section) != ERR_Okay) break;
      }
   }
   else i = 0;

   MSG("Finding next proxy in %d config entries.  Port: '%s', Enabled: %d", glConfig->AmtEntries, Self->FindPort, Self->FindEnabled);

   while (i < glConfig->AmtEntries) {
      match = TRUE;

      MSG("Section: %s", glConfig->Entries[i].Section);

      id = StrToInt(glConfig->Entries[i].Section);
      if (id > 0) {
         if (Self->FindPort[0]) {
            // Does the port match for this proxy?

            if (!cfgReadValue(glConfig, glConfig->Entries[i].Section, "Port", &str)) {
               if ((str[0] IS '0') AND (!str[1])) {
                  // Port is set to 'All' (0) so the match is automatic.
               }
               else {
                  if (StrCompare(str, Self->FindPort, 0, STR_WILDCARD) != ERR_Okay) {
                     MSG("Port '%s' doesn't match requested port '%s'", str, Self->FindPort);
                     match = FALSE;
                  }
               }
            }
         }

         if ((match) AND (Self->FindEnabled != -1)) {
            // Does the enabled status match for this proxy?

            if (!cfgReadInt(glConfig, glConfig->Entries[i].Section, "Enabled", &num)) {
               if (Self->FindEnabled != num) {
                  MSG("Enabled state of %d does not match requested state %d.", num, Self->FindEnabled);
                  match = FALSE;
               }
            }
         }

         if ((match) AND (!cfgReadValue(glConfig, glConfig->Entries[i].Section, "NetworkFilter", &str))) {
            // Do any of the currently connected networks match with the filter?

            LogErrorMsg("Network filters not supported yet.");

         }

         if ((match) AND (!cfgReadValue(glConfig, glConfig->Entries[i].Section, "GatewayFilter", &str))) {
            // Do any connected gateways match with the filter?

            LogErrorMsg("Gateway filters not supported yet.");

         }

         if (match) {
            MSG("Found a matching proxy.");

            StrCopy(glConfig->Entries[i].Section, Self->Section, sizeof(Self->Section));
            return get_record(Self);
         }
      }

      // Go to the next section

      j = i++;
      while (i < glConfig->AmtEntries) {
         if (StrMatch(glConfig->Entries[j].Section, glConfig->Entries[i].Section) != ERR_Okay) break;
         i++;
      }
   }

   MSG("No proxy matched.");

   Self->Find = FALSE;
   return ERR_NoSearchResult;
}

//****************************************************************************

static ERROR PROXY_Free(objProxy *Self, APTR Void)
{
   clear_values(Self);
   return ERR_Okay;
}

//****************************************************************************

static ERROR PROXY_Init(objProxy *Self, APTR Void)
{
   return ERR_Okay;
}

//****************************************************************************

static ERROR PROXY_NewObject(objProxy *Self, APTR Void)
{
   Self->Section[0] = 0;
   Self->Enabled = TRUE;
   Self->Port = 80;
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
SaveSettings: Permanently saves user configurable settings for a proxy.

This action saves a user's settings for a proxy. Saving the proxy settings will make them available to the user on
subsequent logins.

Settings are saved to the user's local account under `user:config/network/proxies.cfg`.  It is possible for the
administrator to define proxy settings as the default for all users by copying the `proxies.cfg` file to the
`system:users/default/config/network/` folder.
-END-

*****************************************************************************/

static ERROR PROXY_SaveSettings(objProxy *Self, APTR Void)
{
   objConfig *config;
   objFile *file;
   ERROR error;

   if ((!Self->Server) OR (!Self->ServerPort)) return PostError(ERR_FieldNotSet);

   LogBranch("Host: %d", Self->Host);

   if (Self->Host) {
      // Proxy is hosted

      #ifdef _WIN32
         OBJECTPTR task;

         task = CurrentTask();

         if (Self->Enabled) {
            taskSetEnv(task, HKEY_PROXY "ProxyEnable", "1");
         }
         else taskSetEnv(task, HKEY_PROXY "ProxyEnable", "0");

         if ((!Self->Server) OR (!Self->Server[0])) {
            MSG("Clearing proxy server value.");

            taskSetEnv(task, HKEY_PROXY "ProxyServer", "");
         }
         else if (Self->Port IS 0) {
            // Proxy is for all ports

            char buffer[120];

            StrFormat(buffer, sizeof(buffer), "%s:%d", Self->Server, Self->ServerPort);

            MSG("Changing all-port proxy to: %s", buffer);

            taskSetEnv(task, HKEY_PROXY "ProxyServer", buffer);
         }
         else {
            char buffer[120];
            CSTRING portname;
            STRING newlist;
            LONG index, end, len;

            portname = NULL;
            switch(Self->Port) {
               case 21: portname = "ftp"; break;
               case 80: portname = "http"; break;
               case 443: portname = "https"; break;
            }

            if (portname) {
               CSTRING servers;
               char server_buffer[200];
               taskGetEnv(task, HKEY_PROXY "ProxyServer", &servers);
               if (!servers) servers = "";
               StrCopy(servers, server_buffer, sizeof(server_buffer));

               StrFormat(buffer, sizeof(buffer), "%s=", portname);
               if ((index = StrSearch(buffer, server_buffer, 0)) != -1) { // Entry already exists - remove it first
                  for (end=index; server_buffer[end]; end++) {
                     if (server_buffer[end] IS ';') {
                        end++;
                        break;
                     }
                  }

                  StrShrink(server_buffer, index, end-index);
               }

               // Add the entry to the end of the string list

               len = StrFormat(buffer, sizeof(buffer), "%s=%s:%d", portname, Self->Server, Self->ServerPort);
               end = StrLength(server_buffer);
               if (!AllocMemory(end + len + 2, MEM_STRING|MEM_NO_CLEAR, &newlist, NULL)) {
                  if (end > 0) {
                     CopyMemory(server_buffer, newlist, end);
                     newlist[end++] = ';';
                  }

                  CopyMemory(buffer, newlist+end, len+1);

                  // Save the new proxy list

                  taskSetEnv(task, HKEY_PROXY "ProxyServer", newlist);

                  FreeResource(newlist);
               }
            }
            else LogErrorMsg("Windows' host proxy settings do not support port %d", Self->Port);
         }

      #endif

      LogBack();
      return ERR_Okay;
   }

   if (!CreateObject(ID_CONFIG, 0, &config,
         FID_Path|TSTR,  "user:config/network/proxies.cfg",
         TAGEND)) {

      if (Self->Section[0]) {
         cfgDeleteSection(config, Self->Section);
      }
      else {
         // This is a new proxy

         LONG id;

         id = 0;
         cfgReadInt(config, "ID", "Value", &id);
         id = id + 1;
         cfgWriteInt(config, "ID", "Value", id);

         IntToStr(id, Self->Section, sizeof(Self->Section));
         Self->Record = id;
      }

      cfgWriteInt(config, Self->Section,   "Port",          Self->Port);
      cfgWriteValue(config, Self->Section, "NetworkFilter", Self->NetworkFilter);
      cfgWriteValue(config, Self->Section, "GatewayFilter", Self->GatewayFilter);
      cfgWriteValue(config, Self->Section, "Username",      Self->Username);
      cfgWriteValue(config, Self->Section, "Password",      Self->Password);
      cfgWriteValue(config, Self->Section, "Name",          Self->ProxyName);
      cfgWriteValue(config, Self->Section, "Server",        Self->Server);
      cfgWriteInt(config, Self->Section,   "ServerPort",    Self->ServerPort);
      cfgWriteInt(config, Self->Section,   "Enabled",       Self->Enabled);

      if (!CreateObject(ID_FILE, 0, &file,
            FID_Path|TSTR,  "user:config/network/proxies.cfg",
            FID_Permissions|TLONG, PERMIT_USER_READ|PERMIT_USER_WRITE,
            FID_Flags|TLONG,       FL_NEW|FL_WRITE,
            TAGEND)) {

         error = acSaveToObject(config, file->Head.UniqueID, 0);
         acFree(file);
      }
      else error = ERR_CreateObject;

      acFree(config);
   }
   else error = ERR_CreateObject;

   LogBack();
   return error;
}

/****************************************************************************

-FIELD-
GatewayFilter: The IP address of the gateway that the proxy is limited to.

The GatewayFilter defines the IP address of the gateway that this proxy is limited to. It is intended to limit the
results of searches performed by the #Find() method.

****************************************************************************/

static ERROR SET_GatewayFilter(objProxy *Self, CSTRING Value)
{
   if (Self->GatewayFilter) { FreeResource(Self->GatewayFilter); Self->GatewayFilter = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->GatewayFilter = StrClone(Value))) {
         return ERR_AllocMemory;
      }
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
Host: If TRUE, the proxy settings are derived from the host operating system's default settings.

If Host is set to TRUE, the proxy settings are derived from the host operating system's default settings.  Hosted
proxies are treated differently to user proxies - they have priority, and any changes are applied directly to the host
system rather than the user's configuration.

-FIELD-
Port: Defines the ports supported by this proxy.

The Port defines the port that the proxy server is supporting, e.g. port 80 for HTTP.

****************************************************************************/

static ERROR SET_Port(objProxy *Self, LONG Value)
{
   if (Value >= 0) {
      Self->Port = Value;
      return ERR_Okay;
   }
   else return ERR_OutOfRange;
}

/****************************************************************************

-FIELD-
NetworkFilter: The name of the network that the proxy is limited to.

The NetworkFilter defines the name of the network that this proxy is limited to. It is intended to limit the results of
searches performed by the #Find() method.

This filter must not be set if the proxy needs to work on an unnamed network.

****************************************************************************/

static ERROR SET_NetworkFilter(objProxy *Self, CSTRING Value)
{
   if (Self->NetworkFilter) { FreeResource(Self->NetworkFilter); Self->NetworkFilter = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->NetworkFilter = StrClone(Value))) return ERR_AllocMemory;
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
Username: The username to use when authenticating against the proxy server.

If the proxy requires authentication, the user name may be set here to enable an automated authentication process. If
the username is not set, a dialog will be required to prompt the user for the user name before communicating with the
proxy server.

****************************************************************************/

static ERROR SET_Username(objProxy *Self, CSTRING Value)
{
   if (Self->Username) { FreeResource(Self->Username); Self->Username = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->Username = StrClone(Value))) return ERR_AllocMemory;
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
Password: The password to use when authenticating against the proxy server.

If the proxy requires authentication, the user password may be set here to enSable an automated authentication process.
If the password is not set, a dialog will need to be used to prompt the user for the password before communicating with
the proxy.

****************************************************************************/

static ERROR SET_Password(objProxy *Self, CSTRING Value)
{
   if (Self->Password) { FreeResource(Self->Password); Self->Password = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->Password = StrClone(Value))) return ERR_AllocMemory;
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
ProxyName: A human readable name for the proxy server entry.

A proxy can be given a human readable name by setting this field.

****************************************************************************/

static ERROR SET_ProxyName(objProxy *Self, CSTRING Value)
{
   if (Self->ProxyName) { FreeResource(Self->ProxyName); Self->ProxyName = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->ProxyName = StrClone(Value))) return ERR_AllocMemory;
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
Server: The destination address of the proxy server - may be an IP address or resolvable domain name.

The domain name or IP address of the proxy server must be defined here.

****************************************************************************/

static ERROR SET_Server(objProxy *Self, CSTRING Value)
{
   if (Self->Server) { FreeResource(Self->Server); Self->Server = NULL; }

   if ((Value) AND (Value[0])) {
      if (!(Self->Server = StrClone(Value))) return ERR_AllocMemory;
   }

   return ERR_Okay;
}

/****************************************************************************

-FIELD-
ServerPort: The port that is used for proxy server communication.

The port used to communicate with the proxy server must be defined here.

****************************************************************************/

static ERROR SET_ServerPort(objProxy *Self, LONG Value)
{
   if ((Value > 0) AND (Value <= 65536)) {
      Self->ServerPort = Value;
      return ERR_Okay;
   }
   else return PostError(ERR_OutOfRange);
}

/****************************************************************************

-FIELD-
Enabled: All proxies are enabled by default until this field is set to FALSE.

To disable a proxy, set this field to FALSE or call the #Disable() action.  This prevents the proxy from being
discovered in searches unless

****************************************************************************/

static ERROR SET_Enabled(objProxy *Self, LONG Value)
{
   if (Value) Self->Enabled = TRUE;
   else Self->Enabled = FALSE;
   return ERR_Okay;
}

/****************************************************************************

-FIELD-
Record: The unique ID of the current proxy record.

The Record is set to the unique ID of the current proxy record.  If no record is indexed then the Record is set to
zero.

If you set the Record manually, the proxy object will attempt to lookup that record.  ERR_Okay will be returned if the
record is found and all record fields will be updated to reflect the data of that proxy.
-END-

****************************************************************************/

static ERROR SET_Record(objProxy *Self, LONG Value)
{
   clear_values(Self);
   IntToStr(Value, Self->Section, sizeof(Self->Section));
   return get_record(Self);
}

/****************************************************************************
** The section field must be set to the record that you want before you call this function.
**
** Also not that you must have called clear_values() at some point before this function.
*/

static ERROR get_record(objProxy *Self)
{
   CSTRING str;

   FMSG("get_record()","Section: %s", Self->Section);

   Self->Record = StrToInt(Self->Section);

   if (!cfgReadValue(glConfig, Self->Section, "Server", &str))   {
      Self->Server = StrClone(str);
      if (!cfgReadValue(glConfig, Self->Section, "NetworkFilter", &str)) Self->NetworkFilter = StrClone(str);
      if (!cfgReadValue(glConfig, Self->Section, "GatewayFilter", &str)) Self->GatewayFilter = StrClone(str);
      if (!cfgReadValue(glConfig, Self->Section, "Username", &str))      Self->Username = StrClone(str);
      if (!cfgReadValue(glConfig, Self->Section, "Password", &str))      Self->Password = StrClone(str);
      if (!cfgReadValue(glConfig, Self->Section, "Name", &str))          Self->ProxyName = StrClone(str);
      if (!cfgReadInt(glConfig, Self->Section, "Port", &Self->Port));
      if (!cfgReadInt(glConfig, Self->Section, "ServerPort", &Self->ServerPort));
      if (!cfgReadInt(glConfig, Self->Section, "Enabled", &Self->Enabled));
      if (!cfgReadInt(glConfig, Self->Section, "Host", &Self->Host));
      return ERR_Okay;
   }
   else return PostError(ERR_NotFound);
}

/***************************************************************************/

static void clear_values(objProxy *Self)
{
   FMSG("clear_values()","");

   Self->Record     = 0;
   Self->Port       = 0;
   Self->Enabled    = 0;
   Self->ServerPort = 0;
   Self->Host       = 0;
   if (Self->NetworkFilter) { FreeResource(Self->NetworkFilter); Self->NetworkFilter = NULL; }
   if (Self->GatewayFilter) { FreeResource(Self->GatewayFilter); Self->GatewayFilter = NULL; }
   if (Self->Username)      { FreeResource(Self->Username);      Self->Username      = NULL; }
   if (Self->Password)      { FreeResource(Self->Password);      Self->Password      = NULL; }
   if (Self->ProxyName)     { FreeResource(Self->ProxyName);     Self->ProxyName     = NULL; }
   if (Self->Server)        { FreeResource(Self->Server);        Self->Server        = NULL; }
}

//***************************************************************************

static const struct FieldDef clPorts[] = {
   { "FTP-Data",  20 },
   { "FTP",       21 },
   { "SSH",       22 },
   { "Telnet",    23 },
   { "SMTP",      25 },
   { "RSFTP",     26 },
   { "HTTP",      80 },
   { "SFTP",      115 },
   { "SQL",       118 },
   { "IRC",       194 },
   { "LDAP",      389 },
   { "HTTPS",     443 },
   { "FTPS",      990 },
   { "TelnetSSL", 992 },
   { "All",       0 },   // All ports
   { NULL, 0 }
};

static const struct FieldArray clFields[] = {
   { "NetworkFilter", FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_NetworkFilter },
   { "GatewayFilter", FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_GatewayFilter },
   { "Username",      FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_Username },
   { "Password",      FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_Password },
   { "ProxyName",     FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_ProxyName },
   { "Server",        FDF_STRING|FDF_RW, 0, NULL, (APTR)SET_Server },
   { "Port",          FDF_LONG|FDF_LOOKUP|FDF_RW,   (MAXINT)&clPorts, NULL, (APTR)SET_Port },
   { "ServerPort",    FDF_LONG|FDF_RW,   0, NULL, (APTR)SET_ServerPort },
   { "Enabled",       FDF_LONG|FDF_RW,   0, NULL, (APTR)SET_Enabled },
   { "Record",        FDF_LONG|FDF_RW,   0, NULL, (APTR)SET_Record },
   END_FIELD
};

static const struct ActionArray clActions[] = {
   { AC_Disable,      (APTR)PROXY_Disable },
   { AC_Enable,       (APTR)PROXY_Enable },
   { AC_Free,         (APTR)PROXY_Free },
   { AC_Init,         (APTR)PROXY_Init },
   { AC_NewObject,    (APTR)PROXY_NewObject },
   { AC_SaveSettings, (APTR)PROXY_SaveSettings },
   { 0, 0 }
};

static const struct FunctionField argsFind[] = { { "Port", FD_LONG }, { "Enabled", FD_LONG }, { NULL, 0 } };

static const struct MethodArray clMethods[] = {
   { MT_PrxDelete,    (APTR)PROXY_Delete,   "Delete",   NULL, 0 },
   { MT_PrxFind,      (APTR)PROXY_Find,     "Find",     argsFind, sizeof(struct prxFind) },
   { MT_PrxFindNext,  (APTR)PROXY_FindNext, "FindNext", NULL, 0 },
   { 0, NULL, NULL, NULL, 0 }
};

//****************************************************************************

ERROR init_proxy(void)
{
   return(CreateObject(ID_METACLASS, 0, &clProxy,
      FID_ClassVersion|TFLOAT, VER_PROXY,
      FID_Name|TSTR,   "Proxy",
      FID_Category|TLONG, CCF_NETWORK,
      FID_Actions|TPTR,   clActions,
      FID_Methods|TARRAY, clMethods,
      FID_Fields|TARRAY,  clFields,
      FID_Size|TLONG,     sizeof(objProxy),
      FID_Path|TSTR,      MOD_PATH,
      TAGEND));
}