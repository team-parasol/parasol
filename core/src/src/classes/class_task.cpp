/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
Task: System processes are managed by the Task class.

Tasks, also known as processes, form the basis of process execution in an operating system.  By creating a task object,
it is possible to execute a program from within the host system.  Programs that are compliant with Parasol may also
reveal additional meta information such as #Author and #Copyright strings in the task object.

To execute a compiled program, set the #Location field to point to the executable file before initialising the
task.  Arguments can be passed to the executable by setting the #Parameters field.  Once the task object is
successfully initialised, use the #Activate() action to run the executable.  If the file executes successfully,
a new task object is spawned separately to represent the executable (which means it is safe to destroy your task object
immediately afterwards).  If the #Activate() action returns with ERR_Okay then the executable program was run
successfully.

To find the task object that represents the active process, use the ~CurrentTask() function to quickly retrieve it.

To send messages to another task, you need to know its #MessageQueue ID so that ~SendMessage() can be used.  A simple
way to initiate interprocess communication is to pass your MessageQueue ID to the other task as a parameter.

-END-

*****************************************************************************/

#define PRV_TASK

#ifdef __CYGWIN__
#undef __unix__
#endif

#ifdef __unix__
 #include <unistd.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <fcntl.h>
 #include <time.h>
 #include <errno.h>
 #include <sys/resource.h>
 #include <sys/utsname.h>
 #include <sys/poll.h>
 #include <sys/time.h>
 #include <sys/wait.h>
#endif

#ifdef _WIN32
 #ifdef __CYGWIN__
  #include <unistd.h>
 #else
  #include <direct.h>
 #endif
 #include <stdio.h>
#endif

#include "../defs.h"
#include <parasol/main.h>

extern "C" void CloseCore(void);

static ERROR InterceptedAction(objTask *, APTR);

#ifdef __unix__

static void task_stdout(HOSTHANDLE FD, APTR);
static void task_stderr(HOSTHANDLE FD, APTR);

#elif _WIN32

#define DLLCALL // __declspec(dllimport)
#define WINAPI  __stdcall
#define HKEY_CLASSES_ROOT	   (0x80000000)
#define HKEY_CURRENT_USER	   (0x80000001)
#define HKEY_LOCAL_MACHINE	   (0x80000002)
#define HKEY_USERS	         (0x80000003)
#define HKEY_PERFORMANCE_DATA	(0x80000004)
#define HKEY_CURRENT_CONFIG	(0x80000005)
#define HKEY_DYN_DATA	      (0x80000006)

#define REG_DWORD 4
#define REG_DWORD_BIG_ENDIAN 5
#define REG_QWORD 11
#define REG_SZ 1

#define KEY_READ  0x20019
#define KEY_WRITE 0x20006

#define MAX_PATH 260
#define INVALID_HANDLE_VALUE (void *)(-1)

extern "C" DLLCALL LONG WINAPI RegOpenKeyExA(LONG,CSTRING,LONG,LONG,APTR *);
extern "C" DLLCALL LONG WINAPI RegQueryValueExA(APTR,CSTRING,LONG *,LONG *,BYTE *,LONG *);
extern "C" DLLCALL LONG WINAPI RegSetValueExA(APTR hKey, CSTRING lpValueName, LONG Reserved, LONG dwType, const void *lpData, LONG cbData);

static LONG glProcessBreak = 0;
#endif

static ERROR GET_LaunchPath(objTask *, STRING *);

static ERROR TASK_Activate(objTask *, APTR);
static ERROR TASK_Free(objTask *, APTR);
static ERROR TASK_GetEnv(objTask *, struct taskGetEnv *);
static ERROR TASK_GetVar(objTask *, struct acGetVar *);
static ERROR TASK_Init(objTask *, APTR);
static ERROR TASK_NewObject(objTask *, APTR);
static ERROR TASK_ReleaseObject(objTask *, APTR);
static ERROR TASK_SetEnv(objTask *, struct taskSetEnv *);
static ERROR TASK_SetVar(objTask *, struct acSetVar *);
static ERROR TASK_Write(objTask *, struct acWrite *);

static ERROR TASK_AddArgument(objTask *, struct taskAddArgument *);
static ERROR TASK_CloseInstance(objTask *, APTR);
static ERROR TASK_Expunge(objTask *, APTR);
static ERROR TASK_Quit(objTask *, APTR);

static const FieldDef clFlags[] = {
   { "Foreign",    TSF_FOREIGN },
   { "Dummy",      TSF_DUMMY },
   { "Wait",       TSF_WAIT },
   { "Shell",      TSF_SHELL },
   { "ResetPath",  TSF_RESET_PATH },
   { "Privileged", TSF_PRIVILEGED },
   { "Debug",      TSF_DEBUG },
   { "Quiet",      TSF_QUIET },
   { "Attached",   TSF_ATTACHED },
   { "Detached",   TSF_DETACHED },
   { "Pipe",       TSF_PIPE },
   { NULL, 0 }
};

static const ActionArray clActions[] = {
   { AC_Activate,      (APTR)TASK_Activate },
   { AC_Free,          (APTR)TASK_Free },
   { AC_GetVar,        (APTR)TASK_GetVar },
   { AC_NewObject,     (APTR)TASK_NewObject },
   { AC_ReleaseObject, (APTR)TASK_ReleaseObject },
   { AC_SetVar,        (APTR)TASK_SetVar },
   { AC_Init,          (APTR)TASK_Init },
   { AC_Write,         (APTR)TASK_Write },
   // The following actions are program dependent
   { AC_ActionNotify,  (APTR)InterceptedAction },
   { AC_Clear,         (APTR)InterceptedAction },
   { AC_Custom,        (APTR)InterceptedAction },
   { AC_DataFeed,      (APTR)InterceptedAction },
   { AC_Deactivate,    (APTR)InterceptedAction },
   { AC_Disable,       (APTR)InterceptedAction },
   { AC_Draw,          (APTR)InterceptedAction },
   { AC_Enable,        (APTR)InterceptedAction },
   { AC_Flush,         (APTR)InterceptedAction },
   { AC_Focus,         (APTR)InterceptedAction },
   { AC_Hide,          (APTR)InterceptedAction },
   { AC_Lock,          (APTR)InterceptedAction },
   { AC_LostFocus,     (APTR)InterceptedAction },
   { AC_Move,          (APTR)InterceptedAction },
   { AC_MoveToBack,    (APTR)InterceptedAction },
   { AC_MoveToFront,   (APTR)InterceptedAction },
   { AC_MoveToPoint,   (APTR)InterceptedAction },
   { AC_NewChild,      (APTR)InterceptedAction },
   { AC_NewOwner,      (APTR)InterceptedAction },
   { AC_Query,         (APTR)InterceptedAction },
   { AC_Read,          (APTR)InterceptedAction },
   { AC_Redimension,   (APTR)InterceptedAction },
   { AC_Rename,        (APTR)InterceptedAction },
   { AC_Reset,         (APTR)InterceptedAction },
   { AC_Resize,        (APTR)InterceptedAction },
   { AC_SaveImage,     (APTR)InterceptedAction },
   { AC_SaveToObject,  (APTR)InterceptedAction },
   { AC_Scroll,        (APTR)InterceptedAction },
   { AC_ScrollToPoint, (APTR)InterceptedAction },
   { AC_Seek,          (APTR)InterceptedAction },
   { AC_Show,          (APTR)InterceptedAction },
   { AC_Unlock,        (APTR)InterceptedAction },
   { AC_Clipboard,     (APTR)InterceptedAction },
   { AC_Refresh,       (APTR)InterceptedAction },
   { AC_Sort,          (APTR)InterceptedAction },
   { AC_SaveSettings,  (APTR)InterceptedAction },
   { AC_SelectArea,    (APTR)InterceptedAction },
   { AC_Undo,          (APTR)InterceptedAction },
   { AC_Redo,          (APTR)InterceptedAction },
   { AC_DragDrop,      (APTR)InterceptedAction },
   { 0, NULL }
};

#include "class_task_def.c"

//****************************************************************************

static void task_stdinput_callback(HOSTHANDLE FD, void *Task)
{
   auto Self = (objTask *)Task;
   char buffer[4096];
   ERROR error;

#ifdef _WIN32
   LONG bytes_read;
   LONG result = winReadStdInput(FD, buffer, sizeof(buffer)-1, &bytes_read);
   if (!result) {
      error = ERR_Okay;
   }
   else if (result IS 1) {
      return;
   }
   else if (result IS -2) {
      error = ERR_Finished;
      RegisterFD(winGetStdInput(), RFD_READ|RFD_REMOVE, &task_stdinput_callback, Self);
   }
   else {
      error = ERR_Failed;
      return;
   }
#else
   LONG bytes_read = read(fileno(stdin), buffer, sizeof(buffer)-1);
   if (bytes_read >= 0) error = ERR_Okay;
   else error = ERR_Finished;
#endif

   buffer[bytes_read] = 0;

   if (Self->InputCallback.Type IS CALL_STDC) {
      auto routine = (void (*)(objTask *, APTR, LONG, ERROR))Self->InputCallback.StdC.Routine;
      routine(Self, buffer, bytes_read, error);
   }
   else if (Self->InputCallback.Type IS CALL_SCRIPT) {
      OBJECTPTR script;
      if ((script = Self->InputCallback.Script.Script)) {
         const ScriptArg args[] = {
            { "Task",       FD_OBJECTPTR,       { .Address = Self } },
            { "Buffer",     FD_PTRBUFFER,       { .Address = buffer } },
            { "BufferSize", FD_LONG|FD_BUFSIZE, { .Long = bytes_read } },
            { "Status",     FD_ERROR,           { .Long = error } }
         };
         scCallback(script, Self->InputCallback.Script.ProcedureID, args, ARRAYSIZE(args));
      }
   }
}

#ifdef __unix__
static void check_incoming(objTask *Self)
{
   struct pollfd fd;

   if (Self->InFD != -1) {
      fd.fd = Self->InFD;
      fd.events = POLLIN;
      if ((poll(&fd, 1, 0) > 0) AND (fd.revents & POLLIN)) {
         task_stdout(Self->InFD, Self);
      }
   }

   if (Self->ErrFD != -1) {
      fd.fd = Self->ErrFD;
      fd.events = POLLIN;
      if ((poll(&fd, 1, 0) > 0) AND (fd.revents & POLLIN)) {
         task_stderr(Self->ErrFD, Self);
      }
   }
}
#endif

//****************************************************************************
// Data output from the executed process is passed via data channels to the object specified in Task->OutputID, and/or
// sent to a callback function.

#ifdef __unix__
static void task_stdout(HOSTHANDLE FD, APTR Task)
{
   static UBYTE recursive = 0;

   if (recursive) return;

   recursive++;

   LONG len;
   char buffer[2048];
   if ((len = read(FD, buffer, sizeof(buffer)-1)) > 0) {
      buffer[len] = 0;

      auto task = (objTask *)Task;
      if (task->OutputCallback.Type IS CALL_STDC) {
         auto routine = (void (*)(objTask *, APTR, LONG))task->OutputCallback.StdC.Routine;
         routine(task, buffer, len);
      }
      else if (task->OutputCallback.Type IS CALL_SCRIPT) {
         OBJECTPTR script;
         if ((script = task->OutputCallback.Script.Script)) {
            const ScriptArg args[] = {
               { "Task",       FD_OBJECTPTR,       { .Address = Task } },
               { "Buffer",     FD_PTRBUFFER,       { .Address = buffer } },
               { "BufferSize", FD_LONG|FD_BUFSIZE, { .Long = len } }
            };
            scCallback(script, task->OutputCallback.Script.ProcedureID, args, ARRAYSIZE(args));
         }
      }
   }
   recursive--;
}

static void task_stderr(HOSTHANDLE FD, APTR Task)
{
   char buffer[2048];
   LONG len;
   static UBYTE recursive = 0;

   if (recursive) return;

   recursive++;
   if ((len = read(FD, buffer, sizeof(buffer)-1)) > 0) {
      buffer[len] = 0;

      auto task = (objTask *)Task;
      if (task->ErrorCallback.Type) {
         if (task->ErrorCallback.Type IS CALL_STDC) {
            auto routine = (void (*)(objTask *, APTR, LONG))task->ErrorCallback.StdC.Routine;
            routine(task, buffer, len);
         }
         else if (task->ErrorCallback.Type IS CALL_SCRIPT) {
            OBJECTPTR script;
            if ((script = task->ErrorCallback.Script.Script)) {
               const ScriptArg args[] = {
                  { "Task", FD_OBJECTPTR,       { .Address = Task } },
                  { "Data", FD_PTRBUFFER,       { .Address = buffer } },
                  { "Size", FD_LONG|FD_BUFSIZE, { .Long = len } }
               };
               scCallback(script, task->ErrorCallback.Script.ProcedureID, args, ARRAYSIZE(args));
            }
         }
      }
   }
   recursive--;
}
#endif

//****************************************************************************
// task_incoming_stdout() and task_incoming_stderr() are callbacks that are activated when data is incoming from a
// process that we've launched.

#ifdef _WIN32
static void output_callback(objTask *Task, FUNCTION *Callback, APTR Buffer, LONG Size)
{
   if (Callback->Type IS CALL_STDC) {
      auto routine = (void (*)(objTask *, APTR, LONG))Callback->StdC.Routine;
      routine(Task, Buffer, Size);
   }
   else if (Callback->Type IS CALL_SCRIPT) {
      OBJECTPTR script;
      if ((script = Callback->Script.Script)) {
         const ScriptArg args[] = {
            { "Task", FD_OBJECTPTR,       { .Address = Task } },
            { "Data", FD_PTRBUFFER,       { .Address = Buffer } },
            { "Size", FD_LONG|FD_BUFSIZE, { .Long = Size } }
         };
         scCallback(script, Callback->Script.ProcedureID, args, ARRAYSIZE(args));
      }
   }
}

static void task_incoming_stdout(WINHANDLE Handle, objTask *Task)
{
   parasol::Log log(__FUNCTION__);
   static UBYTE recursive = 0;

   if (recursive) return;
   if (!Task->Platform) return;

   log.traceBranch("");

   char buffer[4096];
   LONG size = sizeof(buffer) - 1;
   winResetStdOut(Task->Platform, buffer, &size);

   if (size > 0) {
      recursive = 1;
      buffer[size] = 0;
      output_callback(Task, &Task->OutputCallback, buffer, size);
      recursive = 0;
   }
}

static void task_incoming_stderr(WINHANDLE Handle, objTask *Task)
{
   parasol::Log log(__FUNCTION__);
   static UBYTE recursive = 0;

   if (recursive) return;
   if (!Task->Platform) return;

   log.traceBranch("");

   char buffer[4096];
   LONG size = sizeof(buffer) - 1;
   winResetStdErr(Task->Platform, buffer, &size);

   if (size > 0) {
      recursive = 1;
      buffer[size] = 0;
      output_callback(Task, &Task->ErrorCallback, buffer, size);
      recursive = 0;
   }
}

//****************************************************************************
// These functions arrange for callbacks to be made whenever one of our process-connected pipes receives data.

extern "C" void task_register_stdout(objTask *Task, WINHANDLE Handle)
{
   parasol::Log log(__FUNCTION__);
   log.traceBranch("Handle: %d", (LONG)(MAXINT)Handle);
   RegisterFD(Handle, RFD_READ, (void (*)(void *, void *))&task_incoming_stdout, Task);
}

extern "C" void task_register_stderr(objTask *Task, WINHANDLE Handle)
{
   parasol::Log log(__FUNCTION__);
   log.traceBranch("Handle: %d", (LONG)(MAXINT)Handle);
   RegisterFD(Handle, RFD_READ, (void (*)(void *, void *))&task_incoming_stderr, Task);
}

//****************************************************************************

extern "C" void task_deregister_incoming(WINHANDLE Handle)
{
   RegisterFD(Handle, RFD_REMOVE|RFD_READ|RFD_WRITE|RFD_EXCEPT, NULL, NULL);
}
#endif

//****************************************************************************

static ERROR msg_getfield(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   parasol::Log log("ProcessMessages");
   log.warning("Support for GetField messages not available.");
   return ERR_Okay;
}

//****************************************************************************

static ERROR msg_setfield(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   parasol::Log log("ProcessMessages");
   log.warning("Support for SetField messages not available.");
   return ERR_Okay;
}

//****************************************************************************

static ERROR msg_actionresult(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   parasol::Log log("ProcessMessages");
   log.warning("Support for ActionResult messages not available.");
   return ERR_Okay;
}

//****************************************************************************

static CSTRING action_id_name(LONG ActionID)
{
   static char idname[20];
   if ((ActionID > 0) AND (ActionID < AC_END)) {
      return ActionTable[ActionID].Name;
   }
   else {
      IntToStr(ActionID, idname, sizeof(idname));
      return idname;
   }
}

//****************************************************************************

static ERROR msg_action(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   parasol::Log log("ProcessMessages");
   ActionMessage *action;

   if (!(action = (ActionMessage *)Message)) {
      log.warning("No data attached to MSGID_ACTION message.");
      return ERR_Okay;
   }

   #ifdef DBG_INCOMING
      log.function("Executing action %s on object #%d, Data: %p, Size: %d, Args: %d", action_id_name(action->ActionID), action->ObjectID, Message, MsgSize, action->SendArgs);
   #endif

   if ((action->ObjectID) AND (action->ActionID)) {
      OBJECTPTR obj;
      ERROR error;
      if (!(error = AccessObject(action->ObjectID, 5000, &obj))) {
         if (action->SendArgs IS FALSE) {
            obj->Flags |= NF_MESSAGE;
            action->Error = Action(action->ActionID, obj, NULL);
            obj->Flags &= ~NF_MESSAGE;
            ReleaseObject(obj);
         }
         else {
            const FunctionField *fields;
            if (action->ActionID > 0) fields = ActionTable[action->ActionID].Args;
            else {
               rkMetaClass *objclass = (rkMetaClass *)obj->Class;
               if (objclass->Base) objclass = objclass->Base;

               if (objclass->Methods) {
                  fields = objclass->Methods[-action->ActionID].Args;
               }
               else {
                  log.warning("No method table for object #%d, class %d", obj->UniqueID, obj->ClassID);
                  fields = NULL;
                  ReleaseObject(obj);
               }
            }

            // Use resolve_args() to process the args structure back into something readable

            if (fields) {
               if (!resolve_args(action+1, fields)) {
                  obj->Flags |= NF_MESSAGE;
                  action->Error = Action(action->ActionID, obj, action+1);
                  obj->Flags &= ~NF_MESSAGE;
                  ReleaseObject(obj);

                  if ((action->ReturnResult IS TRUE) AND (action->ReturnMessage)) {
                     SendMessage(action->ReturnMessage, MSGID_ACTION_RESULT, 0, action, MsgSize);
                  }

                  free_ptr_args(action+1, fields, FALSE);
               }
               else {
                  log.warning("Failed to resolve arguments for action %s.", action_id_name(action->ActionID));
                  if ((action->ReturnResult IS TRUE) AND (action->ReturnMessage)) {
                     action->Error = ERR_Args;
                     SendMessage(action->ReturnMessage, MSGID_ACTION_RESULT, 0, action, MsgSize);
                  }
                  ReleaseObject(obj);
               }
            }
         }
      }
      else {
         if ((error != ERR_NoMatchingObject) AND (error != ERR_MarkedForDeletion)) {
            if (action->ActionID > 0) log.warning("Could not gain access to object %d to execute action %s.", action->ObjectID, action_id_name(action->ActionID));
            else log.warning("Could not gain access to object %d to execute method %d.", action->ObjectID, action->ActionID);
         }
         else {
            if (action->ActionID IS AC_ActionNotify) {
               struct acActionNotify *notify = (struct acActionNotify *)(action + 1);

               log.debug("ActionNotify(%d, %s) from object %d cancelled, object does not exist.", action->ObjectID, action_id_name(notify->ActionID), notify->ObjectID);

               // This helpful little subroutine will remove the redundant subscription from the calling object

               OBJECTPTR object;
               if ((notify->ObjectID) AND (!AccessObject(notify->ObjectID, 3000, &object))) {
                  UnsubscribeActionByID(object, 0, action->ObjectID);
                  ReleaseObject(object);
               }
            }
            else log.debug("Action %s cancelled, object #%d does not exist or marked for deletion.", action_id_name(action->ActionID), action->ObjectID);
         }
      }
   }
   else log.warning("Action message %s specifies an object ID of #%d.", action_id_name(action->ActionID), action->ObjectID);

   return ERR_Okay;
}

//****************************************************************************
// Internal debug message found.  Internal debug messages are used for diagnosing things that are in local memory to
// the task (programs like Inspector cannot access such areas).

static ERROR msg_debug(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   parasol::Log log("Debug");
   DebugMessage *debug;
   LONG j;

   if (!(debug = (DebugMessage *)Message)) return ERR_Okay;

   if (debug->DebugID IS 1) {
      log.error("Index   Address   MemoryID    Locks");
      ThreadLock lock(TL_MEMORY_PAGES, 4000);
      if (lock.granted()) {
         for (LONG i=0; i < glTotalPages; i++) {
            if ((glMemoryPages[i].Address) OR (glMemoryPages[i].MemoryID)) {
               for (j=0; j < glTotalPages; j++) {
                  if ((j != i) AND (glMemoryPages[j].Address IS glMemoryPages[i].Address)) break;
               }
               if (j < glTotalPages) log.error("%.3d:   %p     %8d%10d [DUPLICATE WITH %d]", i, glMemoryPages[i].Address, glMemoryPages[i].MemoryID, glMemoryPages[i].AccessCount, j);
               else log.error("%.3d:   %p     %8d%10d", i, glMemoryPages[i].Address, glMemoryPages[i].MemoryID, glMemoryPages[i].AccessCount);
            }
         }
      }
   }

   return ERR_Okay;
}

//****************************************************************************

static ERROR msg_validate_process(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   ValidateMessage *data;

   if (!(data = (ValidateMessage *)Message)) return ERR_Okay;
   validate_process(data->ProcessID);
   return ERR_Okay;
}

//****************************************************************************

static ERROR msg_quit(APTR Custom, LONG MsgID, LONG MsgType, APTR Message, LONG MsgSize)
{
   glTaskState = TSTATE_STOPPING;
   return ERR_Okay;
}

//****************************************************************************
// This function is called when a WIN32 process that we launched has been terminated.
//
// For the linux equivalent, refer to internal.c validate_processID().

#ifdef _WIN32
static void task_process_end(WINHANDLE FD, objTask *Task)
{
   parasol::Log log(__FUNCTION__);

   winGetExitCodeProcess(Task->Platform, &Task->ReturnCode);
   if (Task->ReturnCode != 259) {
      Task->ReturnCodeSet = TRUE;
      log.branch("Process " PF64() " ended, return code: %d.", (LARGE)FD, Task->ReturnCode);
   }
   else log.branch("Process " PF64() " signalled exit too early.", (LARGE)FD);

   if (Task->Platform) {
      char buffer[4096];
      LONG size;

      // Process remaining data

      do {
         size = sizeof(buffer);
         if ((!winReadStd(Task->Platform, TSTD_OUT, buffer, &size)) AND (size)) {
            log.msg("Processing %d remaining bytes on stdout.", size);
            output_callback(Task, &Task->OutputCallback, buffer, size);
         }
         else break;
      } while (size IS sizeof(buffer));

      do {
         size = sizeof(buffer);
         if ((!winReadStd(Task->Platform, TSTD_ERR, buffer, &size)) AND (size)) {
            log.msg("Processing %d remaining bytes on stderr.", size);
            output_callback(Task, &Task->ErrorCallback, buffer, size);
         }
         else break;
      } while (size IS sizeof(buffer));

      winFreeProcess(Task->Platform);
      Task->Platform = NULL;
   }
   else winCloseHandle(FD); // winFreeProcess() normally does this with Process->Handle

   // Call ExitCallback, if specified

   if (Task->ExitCallback.Type IS CALL_STDC) {
      auto routine = (void (*)(objTask *))Task->ExitCallback.StdC.Routine;
      routine(Task);
   }
   else if (Task->ExitCallback.Type IS CALL_SCRIPT) {
      OBJECTPTR script;
      if ((script = Task->ExitCallback.Script.Script)) {
         const ScriptArg args[] = { { "Task", FD_OBJECTPTR, { .Address = Task } } };
         scCallback(script, Task->ExitCallback.Script.ProcedureID, args, ARRAYSIZE(args));
      }
   }

   // Post an event for the task's closure

   evTaskRemoved task_removed = { EVID_SYSTEM_TASK_REMOVED, Task->Head.UniqueID, Task->ProcessID };
   BroadcastEvent(&task_removed, sizeof(task_removed));

   // Send a break if we're waiting for this process to end

   if ((Task->Flags & TSF_WAIT) AND (Task->TimeOut > 0)) SendMessage(0, glProcessBreak, 0, NULL, 0);
}
#endif

//****************************************************************************

#ifdef _WIN32
extern "C" void register_process_pipes(objTask *Self, WINHANDLE ProcessHandle)
{
   parasol::Log log;
   log.traceBranch("Process: %d", (LONG)(MAXINT)ProcessHandle);
   RegisterFD(ProcessHandle, RFD_READ, (void (*)(void *, void *))&task_process_end, Self);
}

extern "C" void deregister_process_pipes(objTask *Self, WINHANDLE ProcessHandle)
{
   parasol::Log log;
   log.traceBranch("Process: %d", (LONG)(MAXINT)ProcessHandle);
   if (ProcessHandle) RegisterFD(ProcessHandle, RFD_REMOVE|RFD_READ|RFD_WRITE|RFD_EXCEPT, NULL, NULL);
}
#endif

//****************************************************************************
// Action interception routine.

static ERROR InterceptedAction(objTask *Self, APTR Args)
{
   if (Self->Actions[tlContext->Action].PerformAction) {
      return Self->Actions[tlContext->Action].PerformAction((OBJECTPTR)Self, Args);
   }
   else return ERR_NoSupport;
}

/*****************************************************************************

-ACTION-
Activate: Activating a task object will execute it.

Activating a task results in the execution of the file referenced in the #Location field.

On successful execution, the ProcessID will refer to the ID of the executed process.  This ID is compatible with the
hosting platform's unique process numbers.

If the WAIT flag is specified, this action will not return until the executed process has returned or the
#TimeOut (if specified) has expired.  Messages are processed as normal during this time, ensuring that your
process remains responsive while waiting.

The process' return code can be read from the #ReturnCode field after the process has completed its execution.

In Microsoft Windows, output can be redirected to a file if the redirection symbol is used to direct output in one of
the task arguments.  For instance `&gt;C:\output.txt` will redirect both stderr and stdout to
`c:\output.txt`.  The use of `1&gt;` to redirect stdout and `2&gt;` to redirect stderr
independently of each other is also acceptable.

When running a DOS program in Microsoft Windows, the SHELL flag can be set in the #Flags field to prevent the
DOS window from appearing.  The DOS window will also be hidden if the stdout or stderr pipes are redirected.

-ERRORS-
Okay
FieldNotSet: The Location field has not been set.
Failed
TimeOut:     Can be returned if the WAIT flag is used.  Indicates that the process was launched, but the timeout expired before the process returned.
-END-

*****************************************************************************/

static ERROR TASK_Activate(objTask *Self, APTR Void)
{
   parasol::Log log;
   LONG i, j, k;
   char buffer[1000];
   STRING path, *args;
   ERROR error;
   #ifdef _WIN32
      char launchdir[500];
      STRING redirect_stdout, redirect_stderr;
      BYTE hide_output;
      LONG winerror;
   #endif
   #ifdef __unix__
      LONG pid, argcount;
      BYTE privileged, shell;
   #endif

   Self->ReturnCodeSet = FALSE;

   // If this is a dummy task object then it is being used during the initialisation sequence, so do nothing.

   if (Self->Flags & TSF_DUMMY) return ERR_Okay;

   if (Self->Flags & TSF_FOREIGN) Self->Flags |= TSF_SHELL;

   if (!Self->Location) return log.warning(ERR_MissingPath);

#ifdef _WIN32
   //struct taskAddArgument add;
   //StrFormat(buffer, sizeof(buffer), "--instance %d", glInstanceID);
   //add.Argument = buffer;
   //Action(MT_AddTaskArgument, Self, &add);

   // Determine the launch folder

   launchdir[0] = 0;
   if ((!GET_LaunchPath(Self, &path)) AND (path)) {
      if (!ResolvePath(path, RSF_APPROXIMATE|RSF_PATH, &path)) {
         for (i=0; (path[i]) AND ((size_t)i < sizeof(launchdir)-1); i++) launchdir[i] = path[i];
         launchdir[i] = 0;
         FreeResource(path);
      }
      else {
         for (i=0; (path[i]) AND ((size_t)i < sizeof(launchdir)-1); i++) launchdir[i] = path[i];
         launchdir[i] = 0;
      }
   }
   else if (Self->Flags & TSF_RESET_PATH) {
      if (!ResolvePath(Self->Location, RSF_APPROXIMATE|RSF_PATH, &path)) {
         for (i=0; (path[i]) AND ((size_t)i < sizeof(launchdir)-1); i++) launchdir[i] = path[i];
         FreeResource(path);
      }
      else for (i=0; (Self->Location[i]) AND ((size_t)i < sizeof(launchdir)-1); i++) launchdir[i] = Self->Location[i];

      while ((i > 0) AND (launchdir[i] != '\\')) i--;
      launchdir[i] = 0;
   }

   // Resolve the location of the executable (may contain a volume) and copy it to the command line buffer.

   i = 0;
   buffer[i++] = '"';
   if (!ResolvePath(Self->Location, RSF_APPROXIMATE|RSF_PATH, &path)) {
      for (j=0; (path[j]) AND ((size_t)i < sizeof(buffer)-1); i++,j++) {
         if (path[j] IS '/') buffer[i] = '\\';
         else buffer[i] = path[j];
      }
      FreeResource(path);
   }
   else {
      for (j=0; (Self->Location[j]) AND ((size_t)i < sizeof(buffer)-1); i++,j++) {
         if (Self->Location[j] IS '/') buffer[i] = '\\';
         else buffer[i] = Self->Location[j];
      }
   }
   buffer[i++] = '"';

   // Following the executable path are any arguments that have been used

   redirect_stdout = NULL;
   redirect_stderr = NULL;
   hide_output = FALSE;

   if (!GetField((OBJECTPTR)Self, FID_Parameters|TPTR, &args)) {
      for (j=0; args[j]; j++) {
         if (args[j][0] IS '>') {
            // Redirection argument detected

            if (!ResolvePath(args[j] + 1, RSF_NO_FILE_CHECK, &redirect_stdout)) {
               redirect_stderr = redirect_stdout;
            }

            log.msg("StdOut/Err redirected to %s", redirect_stdout);

            hide_output = TRUE;
            continue;
         }
         else if ((args[j][0] IS '2') AND (args[j][1] IS '>')) {
            log.msg("StdErr redirected to %s", args[j] + 2);
            ResolvePath(args[j] + 2, RSF_NO_FILE_CHECK, &redirect_stderr);
            hide_output = TRUE;
            continue;
         }
         else if ((args[j][0] IS '1') AND (args[j][1] IS '>')) {
            log.msg("StdOut redirected to %s", args[j] + 2);
            ResolvePath(args[j] + 2, RSF_NO_FILE_CHECK, &redirect_stdout);
            hide_output = TRUE;
            continue;
         }

         buffer[i++] = ' ';

         // Check if the argument contains spaces - if so, we need to encapsulate it within quotes.  Otherwise, just
         // copy it as normal.

         for (k=0; (args[j][k]) AND (args[j][k] != ' '); k++);

         if (args[j][k] IS ' ') {
            buffer[i++] = '"';
            for (k=0; args[j][k]; k++) buffer[i++] = args[j][k];
            buffer[i++] = '"';
         }
         else for (k=0; args[j][k]; k++) buffer[i++] = args[j][k];
      }
   }
   buffer[i] = 0;

   // Convert single quotes into double quotes

   BYTE whitespace = TRUE;
   for (i=0; buffer[i]; i++) {
      if (whitespace) {
         if (buffer[i] IS '"') {
            // Skip everything inside double quotes
            i++;
            while ((buffer[i]) AND (buffer[i] != '"')) i++;
            if (!buffer[i]) break;
            whitespace = FALSE;
            continue;
         }
         else if (buffer[i] IS '\'') {
            for (j=i+1; buffer[j]; j++) {
               if (buffer[j] IS '\'') {
                  if (buffer[j+1] <= 0x20) {
                     buffer[i] = '"';
                     buffer[j] = '"';
                  }
                  i = j;
                  break;
               }
               else if (buffer[j] IS '"') break;
            }
         }
      }

      if (buffer[i] <= 0x20) whitespace = TRUE;
      else whitespace = FALSE;
   }

   log.trace("Exec: %s", buffer);

   // Hide window if this is designated a shell program (i.e. hide the DOS window).
   // NB: If you hide a non-shell program, this usually results in the first GUI window that pops up being hidden.

   if (Self->Flags & TSF_SHELL) hide_output = TRUE;

   // Determine whether this new process will be a member of the parent process' group.  This can be forced with the TSF_DETACHED/ATTACHED flags,
   // otherwise it will be determined automatically according to the status of our current task.

   BYTE group;

   if (Self->Flags & TSF_ATTACHED) group = TRUE;
   else if (Self->Flags & TSF_DETACHED) group = FALSE;
   else {
      if (glMasterTask) group = TRUE;
      else group = FALSE;
   }

   LONG internal_redirect = 0;
   if (Self->OutputCallback.Type) internal_redirect |= TSTD_OUT;
   if (Self->ErrorCallback.Type) internal_redirect |= TSTD_ERR;
   if (Self->Flags & TSF_PIPE) internal_redirect |= TSTD_IN;

   if (!(winerror = winLaunchProcess(Self, buffer, (launchdir[0] != 0) ? launchdir : 0, group,
         internal_redirect, &Self->Platform, hide_output, redirect_stdout, redirect_stderr, &Self->ProcessID))) {

      error = ERR_Okay;
      if ((Self->Flags & TSF_WAIT) AND (Self->TimeOut > 0)) {
         log.msg("Waiting for process to exit.  TimeOut: %.2f sec", Self->TimeOut);

         //if (!glProcessBreak) glProcessBreak = AllocateID(IDTYPE_MESSAGE);
         glProcessBreak = MSGID_BREAK;

         ProcessMessages(0, -1);

         winGetExitCodeProcess(Self->Platform, &Self->ReturnCode);
         if (Self->ReturnCode != 259) Self->ReturnCodeSet = TRUE;
      }
   }
   else {
      char msg[300];
      winFormatMessage(winerror, msg, sizeof(msg));
      log.warning("Launch Error: %s", msg);
      error = ERR_Failed;
   }

   if (redirect_stderr IS redirect_stdout) redirect_stderr = NULL;
   if (redirect_stdout) FreeResource(redirect_stdout);
   if (redirect_stderr) FreeResource(redirect_stderr);

   return error;

#elif __unix__

   // Add a 'cd' command so that the application starts in its own folder

   path = NULL;
   GET_LaunchPath(Self, &path);

   i = 0;
   if ((Self->Flags & TSF_RESET_PATH) OR (path)) {
      Self->Flags |= TSF_SHELL;

      buffer[i++] = 'c';
      buffer[i++] = 'd';
      buffer[i++] = ' ';

      if (!path) path = Self->Location;
      if (!ResolvePath(path, RSF_APPROXIMATE|RSF_PATH, &path)) {
         for (j=0; (path[j]) AND ((size_t)i < sizeof(buffer)-1);) buffer[i++] = path[j++];
         FreeResource(path);
      }
      else {
         for (j=0; (path[j]) AND ((size_t)i < sizeof(buffer)-1);) buffer[i++] = path[j++];
      }

      while ((i > 0) AND (buffer[i-1] != '/')) i--;
      if (i > 0) {
         buffer[i++] = ';';
         buffer[i++] = ' ';
      }
   }

   // Resolve the location of the executable (may contain an volume) and copy it to the command line buffer.

   if (!ResolvePath(Self->Location, RSF_APPROXIMATE|RSF_PATH, &path)) {
      for (j=0; (path[j]) AND ((size_t)i < sizeof(buffer)-1);) buffer[i++] = path[j++];
      buffer[i] = 0;
      FreeResource(path);
   }
   else {
      for (j=0; (Self->Location[j]) AND ((size_t)i < sizeof(buffer)-1);) buffer[i++] = Self->Location[j++];
      buffer[i] = 0;
   }

   argcount = 0;
   if (!GetField((OBJECTPTR)Self, FID_Parameters|TPTR, &args)) {
      for (argcount=0; args[argcount]; argcount++);
   }

   STRING argslist[argcount+2];
   LONG bufend;

   bufend = i;

   // Following the executable path are any arguments that have been used. NOTE: This isn't needed if TSF_SHELL is used,
   // however it is extremely useful in the debug printout to see what is being executed.

   if (!GetField((OBJECTPTR)Self, FID_Parameters|TPTR, &args)) {
      for (j=0; args[j]; j++) {
         buffer[i++] = ' ';

         // Check if the argument contains spaces - if so, we need to encapsulate it within quotes.  Otherwise, just
         // copy it as normal.

         for (k=0; (args[j][k]) AND (args[j][k] != ' '); k++);

         if (args[j][k] IS ' ') {
            buffer[i++] = '"';
            for (k=0; args[j][k]; k++) buffer[i++] = args[j][k];
            buffer[i++] = '"';
         }
         else for (k=0; args[j][k]; k++) buffer[i++] = args[j][k];
      }
      buffer[i] = 0;
   }

   // Convert single quotes into double quotes

   for (i=0; buffer[i]; i++) if (buffer[i] IS '\'') buffer[i] = '"';

   log.warning(buffer);

   // If we're not going to run in shell mode, create an argument list for passing to the program.

   if (!(Self->Flags & TSF_SHELL)) {
      buffer[bufend] = 0;

      argslist[0] = buffer;
      for (i=0; i < argcount; i++) {
         argslist[i+1] = args[i];
      }
      argslist[i+1] = 0;

      if (Self->Flags & TSF_DEBUG) {
         for (i=1; argslist[i]; i++) {
            log.msg("Arg %d: %s", i, argslist[i]);
         }
      }
   }

   LONG outpipe[2],errpipe[2];
   LONG out_errfd, in_errfd, out_fd, in_fd, input_fd;

   out_errfd = -1;
   out_fd    = -1;
   in_errfd  = -1;
   in_fd     = -1;

   input_fd = open("/dev/null", O_RDONLY); // Input is always NULL, we don't want the child process reading from our own stdin stream

   if (Self->OutputCallback.Type) {
      log.trace("Output will be sent to callback.");
      if (!pipe(outpipe)) {
         out_fd = outpipe[1]; // for writing
         in_fd  = outpipe[0]; // for reading
      }
      else {
         log.warning("Failed to create pipe: %s", strerror(errno));
         if (input_fd != -1) close(input_fd);
         if (out_fd != -1)   close(out_fd);
         return ERR_Failed;
      }
   }

   if ((out_fd IS -1) AND (Self->Flags & TSF_QUIET)) {
      log.msg("Output will go to NULL");
      out_fd = open("/dev/null", O_RDONLY);
   }

   if (Self->ErrorCallback.Type) {
      log.trace("Error output will be sent to a callback.");
      if (!pipe(errpipe)) {
         out_errfd = errpipe[1];
         in_errfd  = errpipe[0];
      }
      else {
         log.warning("Failed to create pipe: %s", strerror(errno));
         if (input_fd != -1) close(input_fd);
         if (out_fd != -1)   close(out_fd);
         return ERR_Failed;
      }
   }

   if ((out_errfd IS -1) AND (TSF_QUIET)) {
      out_errfd = open("/dev/null", O_RDONLY);
   }

   // Fork a new task.  Remember that forking produces an exact duplicate of the process that made the fork.

   privileged = (Self->Flags & TSF_PRIVILEGED) ? 1 : 0;
   shell = (Self->Flags & TSF_SHELL) ? 1 : 0;

   if (LOCK_PROCESS_TABLE(4000) != ERR_Okay) {
      if (input_fd != -1)  close(input_fd);
      if (out_fd != -1)    close(out_fd);
      if (out_errfd != -1) close(out_errfd);
      if (in_fd != -1)     close(in_fd);
      if (in_errfd != -1)  close(in_errfd);
      return log.warning(ERR_SystemLocked);
   }

   pid = fork();

   if (pid IS -1) {
      UNLOCK_PROCESS_TABLE();
      if (input_fd != -1)  close(input_fd);
      if (out_fd != -1)    close(out_fd);
      if (out_errfd != -1) close(out_errfd);
      if (in_fd != -1)     close(in_fd);
      if (in_errfd != -1)  close(in_errfd);
      log.warning("Failed in an attempt to fork().");
      return ERR_Failed;
   }

   if (pid) {
      // The following code is executed by the initiating process thread

      log.msg("Created new process %d.  Shell: %d", pid, shell);

      Self->ProcessID = pid; // Record the native process ID

      // Preallocate a task slot for the newly running task.  This allows us to communicate a few things to the new
      // task, such as who the parent is and where data should be output to.

      for (i=0; (i < MAX_TASKS) AND (shTasks[i].ProcessID); i++);

      if (i < MAX_TASKS) {
         shTasks[i].ProcessID    = pid;
         shTasks[i].ParentID     = glCurrentTaskID;
         shTasks[i].CreationTime = PreciseTime() / 1000LL;
         shTasks[i].InstanceID   = glInstanceID;
      }

      UNLOCK_PROCESS_TABLE();

      if (in_fd != -1) {
         RegisterFD(in_fd, RFD_READ, &task_stdout, Self);
         Self->InFD = in_fd;
         close(out_fd);
      }

      if (in_errfd != -1) {
         RegisterFD(in_errfd, RFD_READ, &task_stderr, Self);
         Self->ErrFD = in_errfd;
         close(out_errfd);
      }

      // input_fd has no relevance to the parent process

      if (input_fd != -1) {
         close(input_fd);
         input_fd = -1;
      }

      error = ERR_Okay;
      if (Self->Flags & TSF_WAIT) {
         log.msg("Waiting for process to turn into a zombie.");

         // Wait for the child process to turn into a zombie.  NB: A parent process or our own child handler may
         // potentially pick this up but that's fine as waitpid() will just fail with -1 in that case.

         LONG status = 0;
         LARGE ticks = PreciseTime() + F2I(Self->TimeOut * 1000000);
         while (!waitpid(pid, &status, WNOHANG)) {
            ProcessMessages(0, 20);

            if ((Self->TimeOut) AND (PreciseTime() >= ticks)) {
               error = log.warning(ERR_TimeOut);
               break;
            }
         }

         // Find out what error code was returned

         if (WIFEXITED(status)) {
            Self->ReturnCode = (BYTE)WEXITSTATUS(status);
            Self->ReturnCodeSet = TRUE;
         }
      }

      check_incoming(Self);

      return error;
   }

   // The following code is executed by the newly forked process. Using execl() is the easiest way to clean up after a
   // fork because it will replace the process image, which means we don't have to worry about freeing memory and the
   // like.

   if (input_fd != -1) { // stdin
      close(0);
      dup2(input_fd, 0);
      close(input_fd);
   }

   // Duplicate our parent's output FD's for stdout and stderr

   if (out_fd != -1) { // stdout
      close(1);
      dup2(out_fd, 1);
      close(out_fd);
   }

   if (out_errfd != -1) { // stdin
      close(2);
      dup2(out_errfd, 2);
      close(out_errfd);
   }

   // Close the read-only end of the pipe as it's not relevant to the forked process.

   if (in_fd != -1)    close(in_fd);
   if (in_errfd != -1) close(in_errfd);

   if (!privileged) { // Drop privileges so that the program runs as normal
      seteuid(glUID);
      setegid(glGID);
      setuid(glUID);
      setgid(glGID);
   }

   if (shell) { // For some reason, bash terminates the argument list if it encounters a # symbol, so we'll strip those out.
      for (j=0,i=0; buffer[i]; i++) {
         if (buffer[i] != '#') buffer[j++] = buffer[i];
      }
      buffer[j] = 0;

      execl("/bin/sh", "sh", "-c", buffer, (char *)NULL);
   }
   else execv(buffer, (char * const *)&argslist);

   exit(EXIT_FAILURE);
#endif
}

/*****************************************************************************

-METHOD-
AddArgument: Adds new arguments to the Parameters field.

This method provides a simple way of adding new arguments to the #Parameters field.  Provide the value of the
new argument to add it to the end of the list.  If the string is surrounded by quotes, they will be removed
automatically.

-INPUT-
cstr Argument: The argument string that you want to add.

-ERRORS-
Okay
Args
GetField:    The Parameters field could not be retrieved.
AllocMemory: Memory for the new Parameters could not be allocated.

*****************************************************************************/

static ERROR TASK_AddArgument(objTask *Self, struct taskAddArgument *Args)
{
   parasol::Log log;

   if ((!Args) OR (!Args->Argument) OR (!*Args->Argument)) return log.warning(ERR_NullArgs);

   if (!Self->ParametersMID) {
      CSTRING array[2];
      array[0] = Args->Argument;
      return SetArray((OBJECTPTR)Self, FID_Parameters|TSTR, array, 1);
   }

   if (!Self->Parameters) {
      CSTRING *args;
      if (GetField((OBJECTPTR)Self, FID_Parameters|TPTR, &args) != ERR_Okay) {
         return log.warning(ERR_GetField);
      }
      Self->Parameters = args;
   }

   // Calculate the new size of the argument block

   LONG total, len;
   for (len=0; Args->Argument[len]; len++);
   len++;

   MEMORYID argsmid;
   CSTRING *args;
   if (!AllocMemory(Self->ParametersSize + sizeof(STRING) + len, Self->Head.MemFlags|MEM_NO_CLEAR, (void **)&args, &argsmid)) {
      Self->ParametersSize += sizeof(STRING) + len;

      for (total=0; Self->Parameters[total]; total++);

      STRING str = (STRING)(args + total + 2);

      // Copy the old arguments across to the new array

      LONG i, j;
      for (i=0; Self->Parameters[i]; i++) {
         args[i] = str;
         for (j=0; Self->Parameters[i][j]; j++) str[j] = Self->Parameters[i][j];
         str[j++] = 0;
         str += j;
      }

      // Add the new argument.  Notice that we strip enclosing quotes if necessary.

      args[i++] = str;
      args[i]   = NULL;

      CSTRING src = Args->Argument;
      if (*src IS '"') {
         src++;
         while ((*src) AND (*src != '"')) *str++ = *src++;
      }
      else if (*src IS '\'') {
         src++;
         while ((*src) AND (*src != '\'')) *str++ = *src++;
      }
      else while (*src) *str++ = *src++;
      *str = 0;

      ReleaseMemoryID(Self->ParametersMID);
      FreeResourceID(Self->ParametersMID);

      Self->Parameters    = args;
      Self->ParametersMID = argsmid;
      return ERR_Okay;
   }
   else return log.warning(ERR_AllocMemory);
}

/*****************************************************************************

-METHOD-
CloseInstance: Sends a quit message to all tasks running in the current instance.

This method will close all tasks that are running in the current instance by sending them a quit message.  This
includes the process that is making the method call.

-ERRORS-
Okay

*****************************************************************************/

static ERROR TASK_CloseInstance(objTask *Self, APTR Void)
{
   for (LONG i=0; i < MAX_TASKS; i++) {
      if (shTasks[i].TaskID) SendMessage(shTasks[i].MessageID, MSGID_QUIT, 0, NULL, 0);
   }
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Expunge: Forces a Task to expunge unused code.

The Expunge method releases all loaded libraries that are no longer in use by the active process.

If the Expunge method is called on the System Task, it will message all Tasks to perform the expunge sequence.
The System Task object can be found by searching for the "SystemTask" object.

-ERRORS-
Okay

*****************************************************************************/

static ERROR TASK_Expunge(objTask *Self, APTR Void)
{
   if (Self->Head.UniqueID IS SystemTaskID) {
      parasol::ScopedSysLock lock(PL_PROCESSES, 4000);
      if (lock.granted()) {
         for (LONG i=0; i < MAX_TASKS; i++) {
            if ((shTasks[i].TaskID) AND (shTasks[i].TaskID != Self->Head.UniqueID)) {
               ActionMsg(MT_TaskExpunge, shTasks[i].TaskID, NULL, 0, 0);
            }
         }
      }
   }
   else Expunge(FALSE);

   return ERR_Okay;
}

//****************************************************************************

static ERROR TASK_Free(objTask *Self, APTR Void)
{
   parasol::Log log;

#ifdef __unix__
   check_incoming(Self);

   if (Self->InFD != -1) {
      RegisterFD(Self->InFD, RFD_REMOVE, NULL, NULL);
      close(Self->InFD);
      Self->InFD = -1;
   }

   if (Self->ErrFD != -1) {
      RegisterFD(Self->ErrFD, RFD_REMOVE, NULL, NULL);
      close(Self->ErrFD);
      Self->ErrFD = -1;
   }

   if (Self->InputCallback.Type) RegisterFD(fileno(stdin), RFD_READ|RFD_REMOVE, &task_stdinput_callback, Self);
#endif

#ifdef _WIN32
   if (Self->Env) { FreeResource(Self->Env); Self->Env = NULL; }
   if (Self->Platform) { winFreeProcess(Self->Platform); Self->Platform = NULL; }
   if (Self->InputCallback.Type) RegisterFD(winGetStdInput(), RFD_READ|RFD_REMOVE, &task_stdinput_callback, Self);
#endif

   for (LONG i=0; Self->Fields[i]; i++) { FreeResource(Self->Fields[i]); Self->Fields[i] = NULL; }

   // Free allocations

   if (Self->LaunchPath)  { ReleaseMemoryID(Self->LaunchPathMID);  Self->LaunchPath  = NULL; }
   if (Self->Location)    { ReleaseMemoryID(Self->LocationMID);    Self->Location    = NULL; }
   if (Self->Path)        { ReleaseMemoryID(Self->PathMID);        Self->Path        = NULL; }
   if (Self->ProcessPath) { ReleaseMemoryID(Self->ProcessPathMID); Self->ProcessPath = NULL; }
   if (Self->Parameters)  { ReleaseMemoryID(Self->ParametersMID);  Self->Parameters  = NULL; }
   if (Self->Copyright)   { ReleaseMemoryID(Self->CopyrightMID);   Self->Copyright   = NULL; }

   if (Self->LaunchPathMID)  { FreeResourceID(Self->LaunchPathMID);  Self->LaunchPathMID  = 0; }
   if (Self->LocationMID)    { FreeResourceID(Self->LocationMID);    Self->LocationMID    = 0; }
   if (Self->PathMID)        { FreeResourceID(Self->PathMID);        Self->PathMID        = 0; }
   if (Self->ProcessPathMID) { FreeResourceID(Self->ProcessPathMID); Self->ProcessPathMID = 0; }
   if (Self->ParametersMID)  { FreeResourceID(Self->ParametersMID);  Self->ParametersMID  = 0; }
   if (Self->CopyrightMID)   { FreeResourceID(Self->CopyrightMID);   Self->CopyrightMID   = 0; }
   if (Self->MessageMID)     { FreeResourceID(Self->MessageMID);     Self->MessageMID     = 0; }

   if (Self->MsgAction)          { FreeResource(Self->MsgAction);          Self->MsgAction          = NULL; }
   if (Self->MsgGetField)        { FreeResource(Self->MsgGetField);        Self->MsgGetField        = NULL; }
   if (Self->MsgSetField)        { FreeResource(Self->MsgSetField);        Self->MsgSetField        = NULL; }
   if (Self->MsgActionResult)    { FreeResource(Self->MsgActionResult);    Self->MsgActionResult    = NULL; }
   if (Self->MsgDebug)           { FreeResource(Self->MsgDebug);           Self->MsgDebug           = NULL; }
   if (Self->MsgValidateProcess) { FreeResource(Self->MsgValidateProcess); Self->MsgValidateProcess = NULL; }
   if (Self->MsgQuit)            { FreeResource(Self->MsgQuit);            Self->MsgQuit            = NULL; }
   if (Self->MsgEvent)           { FreeResource(Self->MsgEvent);           Self->MsgEvent           = NULL; }
   if (Self->MsgThreadCallback)  { FreeResource(Self->MsgThreadCallback);  Self->MsgThreadCallback  = NULL; }
   if (Self->MsgThreadAction)    { FreeResource(Self->MsgThreadAction);    Self->MsgThreadAction    = NULL; }

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
GetEnv: Retrieves environment variables for the active process.

On platforms that support environment variables, GetEnv() returns the value of the environment variable matching the
Name string.  If there is no matching variable, ERR_DoesNotExist is returned.

In Windows, it is possible to look up registry keys if the string starts with one of the following (in all other
cases, the system's environment variables are queried):

<pre>
\HKEY_LOCAL_MACHINE\
\HKEY_CURRENT_USER\
\HKEY_CLASSES_ROOT\
\HKEY_USERS\
</pre>

Here is a valid example for reading the 'Parasol' key value `\HKEY_CURRENT_USER\Software\Parasol`

Caution: If your programming language uses backslash as an escape character (true for Fluid developers), remember to
use double-backslashes as the key value separator in your Name string.

-INPUT-
cstr Name:  The name of the environment variable to retrieve.
&cstr Value: The value of the environment variable is returned in this parameter.

-ERRORS-
Okay
Args
DoesNotExist: The environment variable is undefined.
NoSupport: The platform does not support environment variables.
-END-

*****************************************************************************/

static ERROR TASK_GetEnv(objTask *Self, struct taskGetEnv *Args)
{
   parasol::Log log;

   if ((!Args) OR (!Args->Name)) return log.warning(ERR_NullArgs);

#ifdef _WIN32

   #define ENV_SIZE 4096
   LONG len;

   Args->Value = NULL;

   if (glCurrentTask != Self) return ERR_Failed;

   if (!Self->Env) {
      if (AllocMemory(ENV_SIZE, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (APTR *)&Self->Env, NULL) != ERR_Okay) {
         return ERR_AllocMemory;
      }
   }

   if (Args->Name[0] IS '\\') {
      struct {
         ULONG ID;
         CSTRING HKey;
      } keys[] = {
         { HKEY_LOCAL_MACHINE,  "\\HKEY_LOCAL_MACHINE\\" },
         { HKEY_CURRENT_USER,   "\\HKEY_CURRENT_USER\\" },
         { HKEY_CLASSES_ROOT,   "\\HKEY_CLASSES_ROOT\\" },
         { HKEY_USERS,          "\\HKEY_USERS\\" },
         { 0, 0 }
      };

      for (LONG ki=0; ki < ARRAYSIZE(keys); ki++) {
         if (!StrCompare(keys[ki].HKey, Args->Name, 0, 0)) {
            CSTRING str = Args->Name + StrLength(keys[ki].HKey); // str = Parasol\Something
            len = StrLength(str); // End of string

            while (len > 0) {
               if (str[len] IS '\\') break;
               len--;
            }

            if (len > 0) {
               char path[len];
               CopyMemory(str, path, len);
               path[len] = 0;

               APTR keyhandle;
               if (!RegOpenKeyExA(keys[ki].ID, path, 0, KEY_READ, &keyhandle)) {
                  LONG type;

                  LONG envlen = ENV_SIZE;
                  if (!RegQueryValueExA(keyhandle, str+len+1, 0, &type, Self->Env, &envlen)) {
                     // Numerical registry types can be converted into strings

                     switch(type) {
                        case REG_DWORD: IntToStr(((LONG *)Self->Env)[0], Self->Env, ENV_SIZE); break;
                        case REG_DWORD_BIG_ENDIAN: IntToStr(((LONG *)Self->Env)[0], Self->Env, ENV_SIZE); break; // Not quite right... we should convert the endianness first.
                        case REG_QWORD: IntToStr(((LARGE *)Self->Env)[0], Self->Env, ENV_SIZE); break;
                     }

                     Args->Value = Self->Env;
                  }
                  winCloseHandle(keyhandle);
               }

               if (Args->Value) return ERR_Okay;
               else return ERR_DoesNotExist;
            }
            else return log.warning(ERR_Syntax);
         }
      }
   }

   len = winGetEnv(Args->Name, Self->Env, ENV_SIZE);
   if (!len) return ERR_DoesNotExist;
   if (len >= ENV_SIZE) return log.warning(ERR_BufferOverflow);

   Args->Value = Self->Env;
   return ERR_Okay;

#elif __unix__
   if ((Args->Value = getenv(Args->Name))) {
      return ERR_Okay;
   }
   else return ERR_DoesNotExist;
#else
   #warn Write support for GetEnv()
   return ERR_NoSupport;
#endif
}

/*****************************************************************************

-METHOD-
SetEnv: Sets environment variables for the active process.

On platforms that support environment variables, SetEnv() is used for defining values for named variables.  A Name and
accompanying Value string are required.  If the Value is NULL, the environment variable is removed if it already exists.

In Windows, it is possible to set registry keys if the string starts with one of the following (in all other cases, the
system's environment variables are queried):

<pre>
\HKEY_LOCAL_MACHINE\
\HKEY_CURRENT_USER\
\HKEY_CLASSES_ROOT\
\HKEY_USERS\
</pre>

When setting a registry key, the function will always set the Value as a string type unless the key already exists.  If
the existing key value is a number such as DWORD or QWORD, then the Value will be converted to an integer before the
key is set.

-INPUT-
cstr Name:  The name of the environment variable to set.
cstr Value: The value to assign to the environment variable.

-ERRORS-
Okay
Args
NoSupport: The platform does not support environment variables.
-END-

*****************************************************************************/

static ERROR TASK_SetEnv(objTask *Self, struct taskSetEnv *Args)
{
   parasol::Log log;

   if ((!Args) OR (!Args->Name)) return log.warning(ERR_NullArgs);

#ifdef _WIN32

   if (Args->Name[0] IS '\\') {
      LONG i, ki, len;
      const struct {
         ULONG ID;
         CSTRING HKey;
      } keys[] = {
         { HKEY_LOCAL_MACHINE,  "\\HKEY_LOCAL_MACHINE\\" },
         { HKEY_CURRENT_USER,   "\\HKEY_CURRENT_USER\\" },
         { HKEY_CLASSES_ROOT,   "\\HKEY_CLASSES_ROOT\\" },
         { HKEY_USERS,          "\\HKEY_USERS\\" },
         { 0, 0 }
      };

      log.msg("Registry: %s = %s", Args->Name, Args->Value);

      for (ki=0; ki < ARRAYSIZE(keys); ki++) {
         if (!StrCompare(keys[ki].HKey, Args->Name, 0, 0)) {
            for (len=0; keys[ki].HKey[len]; len++);

            CSTRING str = Args->Name + len;     // str = Parasol\Something
            for (len=0; str[len]; len++); // End of string

            while (len > 0) {
               if (str[len] IS '\\') break;
               len--;
            }

            if (len > 0) {
               char path[len];
               for (i=0; i < len; i++) path[i] = str[i];
               path[i] = 0;

               APTR keyhandle;
               if (!RegOpenKeyExA(keys[ki].ID, path, 0, KEY_READ|KEY_WRITE, &keyhandle)) {
                  LONG type;

                  if (!RegQueryValueExA(keyhandle, str+i+1, 0, &type, NULL, NULL)) {
                     // Numerical registry types can be converted into strings

                     switch(type) {
                        case REG_DWORD: {
                           LONG int32 = StrToInt(Args->Value);
                           RegSetValueExA(keyhandle, str+i+1, 0, REG_DWORD, &int32, sizeof(int32));
                           break;
                        }

                        case REG_QWORD: {
                           LARGE int64 = StrToInt(Args->Value);
                           RegSetValueExA(keyhandle, str+i+1, 0, REG_QWORD, &int64, sizeof(int64));
                           break;
                        }

                        default: {
                           RegSetValueExA(keyhandle, str+i+1, 0, REG_SZ, Args->Value, StrLength(Args->Value)+1);
                        }
                     }
                  }
                  else RegSetValueExA(keyhandle, str+i+1, 0, REG_SZ, Args->Value, StrLength(Args->Value)+1);

                  winCloseHandle(keyhandle);
               }

               return ERR_Okay;
            }
            else return log.warning(ERR_Syntax);
         }
      }

      return log.warning(ERR_Failed);
   }
   else {
      winSetEnv(Args->Name, Args->Value);
      return ERR_Okay;
   }

#elif __unix__

   if (Args->Value) setenv(Args->Name, Args->Value, 1);
   else unsetenv(Args->Name);
   return ERR_Okay;

#else

   #warn Write support for SetEnv()
   return ERR_NoSupport;

#endif
}

/*****************************************************************************
-ACTION-
GetVar: Retrieves variable field values.
-END-
*****************************************************************************/

static ERROR TASK_GetVar(objTask *Self, struct acGetVar *Args)
{
   parasol::Log log;
   LONG j;

   if ((!Args) OR (!Args->Buffer)) return log.warning(ERR_NullArgs);

   for (LONG i=0; Self->Fields[i]; i++) {
      if (!StrCompare(Args->Field, Self->Fields[i], 0, STR_MATCH_LEN)) {
         CSTRING fieldvalue;
         for (fieldvalue=Self->Fields[i]; *fieldvalue; fieldvalue++);
         fieldvalue++;

         for (j=0; (fieldvalue[j]) AND (j < Args->Size-1); j++) Args->Buffer[j] = fieldvalue[j];
         Args->Buffer[j++] = 0;

         if (j >= Args->Size) return ERR_BufferOverflow;
         else return ERR_Okay;
      }
   }

   log.warning("The variable \"%s\" does not exist.", Args->Field);

   return ERR_Okay;
}

//****************************************************************************

static ERROR TASK_Init(objTask *Self, APTR Void)
{
   parasol::Log log;
   MessageHeader *msgblock;
   LONG i, len;

   if (Self->Head.UniqueID IS SystemTaskID) {
      // Perform the following if this is the System Task
      Self->ProcessID = 0;
   }
   else if ((!glCurrentTaskID) OR (glCurrentTaskID IS SystemTaskID)) {
      // Perform the following if this is a Task representing the current process

      Self->ProcessID = glProcessID;

      glCurrentTaskID = Self->Head.UniqueID;
      glCurrentTask   = Self;

      // Allocate the message block for this Task

      if (!AllocMemory(sizeof(MessageHeader), MEM_PUBLIC, (void **)&msgblock, &glTaskMessageMID)) {
         Self->MessageMID = glTaskMessageMID;
         msgblock->TaskIndex = glTaskEntry->Index;
         ReleaseMemoryID(glTaskMessageMID);
      }
      else return ERR_AllocMemory;

      // Refer to the task object ID in the system list

      if (!LOCK_PROCESS_TABLE(4000)) {
         glTaskEntry->TaskID = Self->Head.UniqueID;
         glTaskEntry->MessageID = glTaskMessageMID;
         UNLOCK_PROCESS_TABLE();
      }

#ifdef _WIN32
      char buffer[300];
      if (winGetExeDirectory(sizeof(buffer), buffer)) {
         len = StrLength(buffer);
         while ((len > 1) AND (buffer[len-1] != '/') AND (buffer[len-1] != '\\') AND (buffer[len-1] != ':')) len--;
         if (!AllocMemory(len+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->ProcessPath, &Self->ProcessPathMID)) {
            for (i=0; i < len; i++) Self->ProcessPath[i] = buffer[i];
            Self->ProcessPath[i] = 0;
         }
      }

      if ((len = winGetCurrentDirectory(sizeof(buffer), buffer))) {
         if (!AllocMemory(len+2, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Path, &Self->PathMID)) {
            for (i=0; i < len; i++) Self->Path[i] = buffer[i];
            if (Self->Path[i-1] != '\\') Self->Path[i++] = '\\';
            Self->Path[i] = 0;
         }
      }

#elif __unix__

         char buffer[256], procfile[50];

         // This method of path retrieval only works on Linux (most types of Unix don't provide any support for this).

         LONG pos = 0;
         for (i=0; "/proc/"[i]; i++) procfile[pos++] = "/proc/"[i];
         pos += IntToStr(glProcessID, procfile+pos, 20);
         for (i=0; "/exe"[i]; i++) procfile[pos++] = "/exe"[i];
         procfile[pos] = 0;

         buffer[0] = 0;
         if ((i = readlink(procfile, buffer, sizeof(buffer)-1)) > 0) {
            buffer[i] = 0;
            while (i > 0) { // Strip the process name
               if (buffer[i] IS '/') {
                  buffer[i+1] = 0;
                  break;
               }
               i--;
            }

            for (len=0; buffer[len]; len++);
            while ((len > 1) AND (buffer[len-1] != '/') AND (buffer[len-1] != '\\') AND (buffer[len-1] != ':')) len--;
            if (!AllocMemory(len+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->ProcessPath, &Self->ProcessPathMID)) {
               for (i=0; i < len; i++) Self->ProcessPath[i] = buffer[i];
               Self->ProcessPath[i] = 0;
            }
         }

         if (!Self->PathMID) { // Set the working folder
            if (getcwd(buffer, sizeof(buffer))) {
               for (len=0; buffer[len]; len++);
               if (!AllocMemory(len+2, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Path, &Self->PathMID)) {
                  for (i=0; buffer[i]; i++) Self->Path[i] = buffer[i];
                  Self->Path[i++] = '/';
                  Self->Path[i] = 0;
               }
            }
         }
#endif

      // Initialise message handlers so that the task can process messages.

      FUNCTION call;
      call.Type = CALL_STDC;
      call.StdC.Routine = (APTR)msg_action;
      AddMsgHandler(NULL, MSGID_ACTION, &call, &Self->MsgAction);

      call.StdC.Routine = (APTR)msg_getfield;
      AddMsgHandler(NULL, MSGID_GET_FIELD, &call, &Self->MsgGetField);

      call.StdC.Routine = (APTR)msg_setfield;
      AddMsgHandler(NULL, MSGID_SET_FIELD, &call, &Self->MsgSetField);

      call.StdC.Routine = (APTR)msg_actionresult;
      AddMsgHandler(NULL, MSGID_ACTION_RESULT, &call, &Self->MsgActionResult);

      call.StdC.Routine = (APTR)msg_debug;
      AddMsgHandler(NULL, MSGID_DEBUG, &call, &Self->MsgDebug);

      call.StdC.Routine = (APTR)msg_validate_process;
      AddMsgHandler(NULL, MSGID_VALIDATE_PROCESS, &call, &Self->MsgValidateProcess);

      call.StdC.Routine = (APTR)msg_quit;
      AddMsgHandler(NULL, MSGID_QUIT, &call, &Self->MsgQuit);

      call.StdC.Routine = (APTR)msg_event; // lib_events.c
      AddMsgHandler(NULL, MSGID_EVENT, &call, &Self->MsgEvent);

      call.StdC.Routine = (APTR)msg_threadcallback; // class_thread.c
      AddMsgHandler(NULL, MSGID_THREAD_CALLBACK, &call, &Self->MsgThreadCallback);

      call.StdC.Routine = (APTR)msg_threadaction; // class_thread.c
      AddMsgHandler(NULL, MSGID_THREAD_ACTION, &call, &Self->MsgThreadAction);


      log.msg("Process Path: %s", Self->ProcessPath);
      log.msg("Working Path: %s", Self->Path);
   }
   else if (Self->ProcessID) {
      // The process ID has been preset - this means that the task could represent a link to an existing Parasol
      // process, or to a foreign process.

      for (i=0; i < MAX_TASKS; i++) {
         if ((shTasks[i].TaskID) AND (shTasks[i].ProcessID IS Self->ProcessID)) {
            log.msg("Connected process %d to task %d, message port %d.", Self->ProcessID, shTasks[i].TaskID, shTasks[i].MessageID);
            Self->MessageMID = shTasks[i].MessageID;
            break;
         }
      }

      if (i >= MAX_TASKS) Self->Flags |= TSF_FOREIGN;
   }

   return ERR_Okay;
}

/*****************************************************************************
** Task: NewObject
*/

static ERROR TASK_NewObject(objTask *Self, APTR Void)
{
#ifdef __unix__
   Self->InFD = -1;
   Self->ErrFD = -1;
#endif
   Self->TimeOut = 60 * 60 * 24;
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Quit: Sends a quit message to a task.

The Quit method can be used as a convenient way of sending a task a quit message.  This will normally result in the
destruction of the task, so long as it is still functioning correctly and has been coded to respond to the
MSGID_QUIT message type.  It is legal for a task to ignore a quit request if it is programmed to stay alive.  A task
can be killed outright with the Free action.

-ERRORS-
Okay
-END-

*****************************************************************************/

static ERROR TASK_Quit(objTask *Self, APTR Void)
{
   parasol::Log log;

   if ((Self->ProcessID) AND (Self->ProcessID != glProcessID)) {
      log.msg("Terminating foreign process %d", Self->ProcessID);

      #ifdef __unix__
         kill(Self->ProcessID, SIGHUP); // Safe kill signal - this actually results in that process generating an internal MSGID_QUIT message
      #elif _WIN32
         winTerminateApp(Self->ProcessID, 1000);
      #else
         #warning Add code to kill foreign processes.
      #endif
   }
   else if (Self->MessageMID) {
      log.branch("Sending quit message to queue %d.", Self->MessageMID);
      if (!SendMessage(Self->MessageMID, MSGID_QUIT, 0, NULL, 0)) {
         return ERR_Okay;
      }
   }
   else log.warning("Task is not linked to a message queue or process.");

   return ERR_Okay;
}

//****************************************************************************

static ERROR TASK_ReleaseObject(objTask *Self, APTR Void)
{
   if (Self->LaunchPath)  { ReleaseMemoryID(Self->LaunchPathMID);  Self->LaunchPath  = NULL; }
   if (Self->Location)    { ReleaseMemoryID(Self->LocationMID);    Self->Location    = NULL; }
   if (Self->Parameters)  { ReleaseMemoryID(Self->ParametersMID);  Self->Parameters  = NULL; }
   if (Self->Copyright)   { ReleaseMemoryID(Self->CopyrightMID);   Self->Copyright   = NULL; }
   if (Self->Path)        { ReleaseMemoryID(Self->PathMID);        Self->Path        = NULL; }
   if (Self->ProcessPath) { ReleaseMemoryID(Self->ProcessPathMID); Self->ProcessPath = NULL; }
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
SetVar: Variable fields are supported for the general storage of program variables.
-END-
*****************************************************************************/

static ERROR TASK_SetVar(objTask *Self, struct acSetVar *Args)
{
   parasol::Log log;

   if ((!Args) OR (!Args->Field) OR (!Args->Value)) return ERR_NullArgs;

   // Find the insertion point

   LONG i;
   for (i=0; Self->Fields[i]; i++) {
      if (!StrMatch(Args->Field, Self->Fields[i])) break;
   }

   if (i < ARRAYSIZE(Self->Fields) - 1) {
      STRING field;
      if (!AllocMemory(StrLength(Args->Field) + StrLength(Args->Value) + 2,
            MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&field, NULL)) {

         LONG pos = StrCopy(Args->Field, field, COPY_ALL) + 1;
         StrCopy(Args->Value, field + pos, COPY_ALL);

         if (Self->Fields[i]) FreeResource(Self->Fields[i]);
         Self->Fields[i] = field;

         return ERR_Okay;
      }
      else return log.warning(ERR_AllocMemory);
   }
   else return log.warning(ERR_ArrayFull);
}

/*****************************************************************************

-ACTION-
Write: Send raw data to a launched process' stdin descriptor.

If a process is successfully launched with the PIPE set in #Flags, data can be sent to its stdin pipe by calling the
Write action.  Setting the Buffer parameter to NULL will result in the pipe being closed (this will signal to the
process that no more data is incoming).

*****************************************************************************/

static ERROR TASK_Write(objTask *Task, struct acWrite *Args)
{
   parasol::Log log;

   if (!Args) return log.warning(ERR_NullArgs);

#ifdef _WIN32
   LONG winerror;
   if (Task->Platform) {
      if (!(winerror = winWriteStd(Task->Platform, Args->Buffer, Args->Length))) {
         return ERR_Okay;
      }
      else return log.warning(ERR_Write);
   }
   else return log.warning(ERR_Failed);
#else
   return log.warning(ERR_NoSupport);
#endif
}

/*****************************************************************************

-FIELD-
Actions: Used to gain direct access to a task's actions.

This field provides direct access to the actions of a task.  You can use it in the development of an executable program
to hook into the Core action system.  This allows you to create a program that blends in seamlessly with the
system's object oriented design.  In some cases this is a necessity, for example, use of some functions will require you
to hook into the ActionNotify action.

The Actions field itself points to a list of action routines that are arranged into a lookup table, sorted by action ID.
You can hook into an action simply by writing to its index in the table with a pointer to the routine that you want to
use for that action.  For example:

<pre>
if (!AccessObject(CurrentTask(), 5000, &task)) {
   GetPointer(task, FID_Actions, &amp;actions);
   actions[AC_ActionNotify] = PROGRAM_ActionNotify;
   ReleaseObject(task);
}
</pre>

The synopsis of the routines that you use for hooking into the action list must match
`ERROR PROGRAM_ActionNotify(*Task, APTR Args)`.

It is recommended that you refer to the Action Support Guide before hooking into any action that you have not written
code for before.

*****************************************************************************/

static ERROR GET_Actions(objTask *Self, struct ActionEntry **Value)
{
   *Value = Self->Actions;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Args: Command line arguments (string format).

This field allows command line arguments to be set using a single string, whereby each value is separated by whitespace.
The string will be disassembled and the arguments will be available to read from the #Parameters field.

If an argument needs to include whitespace, use double-quotes to encapsulate the value.

*****************************************************************************/

static ERROR SET_Args(objTask *Self, CSTRING Value)
{
   if ((!Value) OR (!*Value)) return ERR_Okay;

   while (*Value) {
      while (*Value <= 0x20) Value++; // Skip whitespace

      if (*Value) { // Extract the argument
         char buffer[400];
         LONG i;
         for (i=0; (*Value) AND (*Value > 0x20) AND ((size_t)i < sizeof(buffer)-1);) {
            if (*Value IS '"') {
               Value++;
               while (((size_t)i < sizeof(buffer)-1) AND (*Value) AND (*Value != '"')) {
                  buffer[i++] = *Value++;
               }
               if (*Value IS '"') Value++;
            }
            else buffer[i++] = *Value++;
         }
         buffer[i] = 0;

         if (*Value) while (*Value > 0x20) Value++;

         struct taskAddArgument add = { .Argument = buffer };
         Action(MT_TaskAddArgument, &Self->Head, &add);
      }
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Parameters: Command line arguments (list format).

Command line arguments for a program can be defined here in list format, whereby each argument is an independent
string.  The list must be terminated with a NULL pointer.

To illustrate, the following command-line string:

<pre>1&gt; YourProgram PREFS MyPrefs -file "documents:readme.txt"</pre>

Would be represented as follows:

<pre>
CSTRING Args[] = {
   "PREFS",
   "MyPrefs",
   "-file",
   "documents:readme.txt",
   NULL
};
</pre>

*****************************************************************************/

static ERROR GET_Parameters(objTask *Self, CSTRING **Value, LONG *Elements)
{
   parasol::Log log;

   if (Self->Parameters) {
      *Value = Self->Parameters;
      *Elements = 0;
      return ERR_Okay;
   }
   else if (!Self->ParametersMID) {
      log.msg("No arguments to return.");
      *Value = NULL;
      *Elements = 0;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->ParametersMID, MEM_READ, 2000, (void **)&Self->Parameters)) {
      *Value = Self->Parameters;
      // Resolve string addresses

      LONG i;
      for (i=0; Self->Parameters[i]; i++);
      *Elements = i;

      CSTRING args = (CSTRING)(Self->Parameters + i + 1);

      for (i=0; Self->Parameters[i]; i++) {
         Self->Parameters[i] = args;
         while (*args) args++;
         args++;
      }

      return ERR_Okay;
   }
   else {
      *Value = NULL;
      *Elements = 0;
      return log.warning(ERR_AccessMemory);
   }
}

static ERROR SET_Parameters(objTask *Self, CSTRING *Value, LONG Elements)
{
   parasol::Log log;

   if (Self->Parameters)    { ReleaseMemoryID(Self->ParametersMID); Self->Parameters = NULL; }
   if (Self->ParametersMID) { FreeResourceID(Self->ParametersMID);    Self->ParametersMID = 0; }

   if (Value) {
      // Calculate the size of the argument array and strings tacked onto the end

      LONG i, j;
      Self->ParametersSize = sizeof(STRING); // Null-terminated array entry
      for (j=0; j < Elements; j++) {
         if (!Value[j]) { Elements = j; break; }
         Self->ParametersSize += sizeof(STRING); // Array entry
         for (i=0; Value[j][i]; i++) Self->ParametersSize++; // String length
         Self->ParametersSize++; // String null terminator
      }

      if (!AllocMemory(Self->ParametersSize, MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Parameters, &Self->ParametersMID)) {
         STRING args = (STRING)(Self->Parameters + j + 1);
         for (j=0; j < Elements; j++) {
            Self->Parameters[j] = args;
            for (i=0; Value[j][i]; i++) args[i] = Value[j][i];
            args[i++] = 0;
            args += i;
         }
         Self->Parameters[j] = 0;
      }
      else return log.warning(ERR_AllocMemory);
   }
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Author: Describes the person that wrote the program.

This field gives information about the author of the program/task. If the author is not determinable from the
#Location, this field will usually be set to NULL.

*****************************************************************************/

static ERROR GET_Author(objTask *Self, STRING *Value)
{
   *Value = Self->Author;
   return ERR_Okay;
}

static ERROR SET_Author(objTask *Self, CSTRING Value)
{
   StrCopy(Value, Self->Author, sizeof(Self->Author));
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Copyright: Copyright/licensing details.

*****************************************************************************/

static ERROR GET_Copyright(objTask *Self, STRING *Value)
{
   parasol::Log log;

   if (Self->Copyright) {
      *Value = Self->Copyright;
      return ERR_Okay;
   }
   else if (!Self->CopyrightMID) {
      *Value = NULL;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->CopyrightMID, MEM_READ, 2000, (void **)&Self->Copyright)) {
      *Value = Self->Copyright;
      return ERR_Okay;
   }
   else {
      *Value = NULL;
      return log.warning(ERR_AccessMemory);
   }
}

static ERROR SET_Copyright(objTask *Self, CSTRING Value)
{
   parasol::Log log;

   if (Self->Copyright)    { ReleaseMemoryID(Self->CopyrightMID);   Self->Copyright = NULL; }
   if (Self->CopyrightMID) { FreeResourceID(Self->CopyrightMID); Self->CopyrightMID = 0; }

   if ((Value) AND (*Value)) {
      LONG len = StrLength(Value);
      if (!AllocMemory(len+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Copyright, &Self->CopyrightMID)) {
         CopyMemory(Value, Self->Copyright, len+1);
      }
      else return log.warning(ERR_AllocMemory);
   }
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Date: The date that the program was last updated or compiled.

The Date usually specifies the date on which the program was compiled for public release. It is up to the developer of
the program to set this string correctly and keep it current.

The correct specification for this string is `Day Month Year` or `Month Year` as in the following
examples:

<pre>
"14 February 1998"
"6 May 1997"
"January 2000"
</pre>

Please do not use shorthand dates such as `14/2/98`.  Do not include the time or any other information besides that
which is outlined here.

****************************************************************************/

static ERROR GET_Date(objTask *Self, STRING *Value)
{
   *Value = Self->Date;
   return ERR_Okay;
}

static ERROR SET_Date(objTask *Self, CSTRING Value)
{
   StrCopy(Value, Self->Date, sizeof(Self->Date));
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ExitCallback: The callback is activated when the process is terminated.

The ExitCallback field can be set with a function reference that will be called when the executed process is
terminated.  The callback must follow the synopsis `Function(*Task)`.

Please keep in mind that if the Task is freed when the process is still executing, the ExitCallback routine will not be
called on termination because the Task object no longer exists for the control of the process.

*****************************************************************************/

static ERROR GET_ExitCallback(objTask *Self, FUNCTION **Value)
{
   if (Self->ExitCallback.Type != CALL_NONE) {
      *Value = &Self->ExitCallback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_ExitCallback(objTask *Self, FUNCTION *Value)
{
   if (Value) Self->ExitCallback = *Value;
   else Self->ExitCallback.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ErrorCallback: This callback returns incoming data from STDERR.

The ErrorCallback field can be set with a function reference that will be called when an active process sends data via
STDERR.  The callback must follow the synopsis `Function(*Task, APTR Data, LONG Size)`

The information read from STDERR will be returned in the Data pointer and the byte-length of the data will be
indicated by the Size.  The data pointer is temporary and will be invalid once the callback function has returned.

*****************************************************************************/

static ERROR GET_ErrorCallback(objTask *Self, FUNCTION **Value)
{
   if (Self->ErrorCallback.Type != CALL_NONE) {
      *Value = &Self->ErrorCallback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_ErrorCallback(objTask *Self, FUNCTION *Value)
{
   if (Value) Self->ErrorCallback = *Value;
   else Self->ErrorCallback.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
InputCallback: This callback returns incoming data from STDIN.

The InputCallback field is available for use only when the Task object represents the current process.
The referenced function will be called when process receives data from STDIN.  The callback must follow the
synopsis `Function(*Task, APTR Data, LONG Size, ERROR Status)`

The information read from STDOUT will be returned in the Data pointer and the byte-length of the data will be indicated
by the Size.  The data buffer is temporary and will be invalid once the callback function has returned.

A status of ERR_Finished is sent if the stdinput handle has been closed.

*****************************************************************************/

static ERROR GET_InputCallback(objTask *Self, FUNCTION **Value)
{
   if (Self->InputCallback.Type != CALL_NONE) {
      *Value = &Self->InputCallback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_InputCallback(objTask *Self, FUNCTION *Value)
{
   if (Self != glCurrentTask) return ERR_Failed;

   if (Value) {
      ERROR error;
      #ifdef __unix__
      fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL) | O_NONBLOCK);
      if (!(error = RegisterFD(fileno(stdin), RFD_READ, &task_stdinput_callback, Self))) {
      #elif _WIN32
      if (!(error = RegisterFD(winGetStdInput(), RFD_READ, &task_stdinput_callback, Self))) {
      #endif
         Self->InputCallback = *Value;
      }
      else return error;
   }
   else {
      #ifdef _WIN32
      if (Self->InputCallback.Type) RegisterFD(winGetStdInput(), RFD_READ|RFD_REMOVE, &task_stdinput_callback, Self);
      #else
      if (Self->InputCallback.Type) RegisterFD(fileno(stdin), RFD_READ|RFD_REMOVE, &task_stdinput_callback, Self);
      #endif
      Self->InputCallback.Type = CALL_NONE;
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
OutputCallback: This callback returns incoming data from STDOUT.

The OutputCallback field can be set with a function reference that will be called when an active process sends data via
STDOUT.  The callback must follow the synopsis `Function(*Task, APTR Data, LONG Size)`

The information read from STDOUT will be returned in the Data pointer and the byte-length of the data will be indicated
by the Size.  The data pointer is temporary and will be invalid once the callback function has returned.

*****************************************************************************/

static ERROR GET_OutputCallback(objTask *Self, FUNCTION **Value)
{
   if (Self->OutputCallback.Type != CALL_NONE) {
      *Value = &Self->OutputCallback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_OutputCallback(objTask *Self, FUNCTION *Value)
{
   if (Value) Self->OutputCallback = *Value;
   else Self->OutputCallback.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Flags: Optional flags.
Lookup: TSF

-FIELD-
LaunchPath: Launched executables will start in the path specified here.

Use the LaunchPath field to specify the folder that a launched executable will start in when the task object is
activated.  This will override all other path options, such as the RESET_PATH flag.

*****************************************************************************/

static ERROR GET_LaunchPath(objTask *Self, STRING *Value)
{
   parasol::Log log;

   if (Self->LaunchPath) {
      *Value = Self->LaunchPath;
      return ERR_Okay;
   }
   else if (!Self->LaunchPathMID) {
      *Value = NULL;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->LaunchPathMID, MEM_READ, 2000, (void **)&Self->LaunchPath)) {
      *Value = Self->LaunchPath;
      return ERR_Okay;
   }
   else {
      *Value = NULL;
      return log.warning(ERR_AccessMemory);
   }
}

static ERROR SET_LaunchPath(objTask *Self, CSTRING Value)
{
   parasol::Log log;

   if (Self->LaunchPath)    { ReleaseMemoryID(Self->LaunchPathMID);   Self->LaunchPath = NULL; }
   if (Self->LaunchPathMID) { FreeResourceID(Self->LaunchPathMID); Self->LaunchPathMID = 0; }

   if ((Value) AND (*Value)) {
      LONG i;
      for (i=0; Value[i]; i++);
      if (!AllocMemory(i+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->LaunchPath, &Self->LaunchPathMID)) {
         CopyMemory(Value, Self->LaunchPath, i+1);
      }
      else return log.warning(ERR_AllocMemory);
   }
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Location: Location of an executable file to launch.

When a task object is activated, the Location field will be checked for a valid filename.  If the path is valid, the
executable code will be loaded from this source.  The source must be in an executable format recognised by the
native platform.

Leading spaces will be ignored by the string parser.  The Location string can be enclosed with quotes, in which case
only the quoted portion of the string will be used as the source path.

*****************************************************************************/

static ERROR GET_Location(objTask *Self, STRING *Value)
{
   parasol::Log log;

   if (Self->Location) {
      *Value = Self->Location;
      return ERR_Okay;
   }
   else if (!Self->LocationMID) {
      *Value = NULL;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->LocationMID, MEM_READ, 2000, (void **)&Self->Location)) {
      *Value = Self->Location;
      return ERR_Okay;
   }
   else {
      *Value = NULL;
      return log.warning(ERR_AccessMemory);
   }
}

static ERROR SET_Location(objTask *Self, CSTRING Value)
{
   parasol::Log log;

   if (Self->Location)    { ReleaseMemoryID(Self->LocationMID);   Self->Location = NULL; }
   if (Self->LocationMID) { FreeResourceID(Self->LocationMID); Self->LocationMID = 0; }

   if ((Value) AND (*Value)) {
      LONG i;
      for (i=0; Value[i]; i++);
      if (!AllocMemory(i+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Location, &Self->LocationMID)) {
         while ((*Value) AND (*Value <= 0x20)) Value++;
         if (*Value IS '"') {
            Value++;
            i = 0;
            while ((*Value) AND (*Value != '"')) Self->Location[i++] = *Value++;
         }
         else for (i=0; *Value; i++) Self->Location[i] = *Value++;
         Self->Location[i] = 0;
      }
      else return log.warning(ERR_AllocMemory);
   }
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Instance: The instance ID that the process belongs to.

All tasks that use the Parasol API belong to a run-time instance that can host multiple processes.  Tasks that share
the same instance ID can easily communicate with each other, while those that do not will be in a separate namespace.

It is not possible to change the instance ID once the process has started.  New processes can be assigned an instance
ID on creation with the `--instance` commandline argument.  By default, any new process will share the same
instance ID as its creator.

*****************************************************************************/

static ERROR GET_Instance(objTask *Self, LONG *Value)
{
   *Value = glInstanceID;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
MessageQueue: Read this field to acquire a task's message queue ID.

If you need the ID of a task's message queue, read this field to obtain it.  Once you have a task's message queue ID,
you can use it to send messages to the task.  For information on messaging, refer to the ~GetMessage(),
~ProcessMessages() and ~SendMessage() functions.

*****************************************************************************/

static ERROR GET_MessageQueue(objTask *Task, MEMORYID *Value)
{
   if ((*Value = glTaskMessageMID)) return ERR_Okay;
   else return ERR_NoData;
}

/*****************************************************************************

-FIELD-
Name: Name of the task.

This field specifies the name of the Task or program that has been initialised. It is up to the developer of the
program to set the Name which will appear in this field.  If there is no name for the task then the system may
assign a randomly generated name.

*****************************************************************************/

static ERROR GET_Name(objTask *Self, STRING *Value)
{
   *Value = Self->Name;
   return ERR_Okay;
}

static ERROR SET_Name(objTask *Self, CSTRING Value)
{
   StrCopy(Value, Self->Name, sizeof(Self->Name));
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
TimeOut: Limits the amount of time to wait for a launched process to return.

This field can be set in conjunction with the WAIT flag to define the time limit when waiting for a launched
process to return.  The time out is defined in seconds.

-FIELD-
ProcessID: Reflects the process ID when an executable is launched.

If a task object launches an executable file via Activate(), the ProcessID will be set to the 'pid' that was assigned
to the new process by the host system.  At all other times the ProcessID is set to zero.

-FIELD-
Path: The current working folder of the active process.

The Path specifies the 'working folder' that determines where files are loaded from when an absolute path is not
otherwise specified for file access.  Initially the working folder is usually set to the folder of the parent
process, such as that of a terminal shell.

The working folder can be changed at any time by updating the Path with a new folder location.

*****************************************************************************/

static ERROR GET_Path(objTask *Self, STRING *Value)
{
   parasol::Log log;

   if (Self->Path) {
      *Value = Self->Path;
      return ERR_Okay;
   }
   else if (!Self->PathMID) {
      *Value = NULL;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->PathMID, MEM_READ, 2000, (void **)&Self->Path)) {
      *Value = Self->Path;
      return ERR_Okay;
   }
   else {
      *Value = NULL;
      return log.warning(ERR_AccessMemory);
   }
}

static ERROR SET_Path(objTask *Self, CSTRING Value)
{
   parasol::Log log;

   if (Self->Path)    { ReleaseMemoryID(Self->PathMID); Self->Path = NULL; }
   if (Self->PathMID) { FreeResourceID(Self->PathMID); Self->PathMID = 0; }

   log.msg("New Path: %s", Value);

   if ((Value) AND (*Value)) {
      LONG len, j;
      for (len=0; Value[len]; len++);
      while ((len > 1) AND (Value[len-1] != '/') AND (Value[len-1] != '\\') AND (Value[len-1] != ':')) len--;
      if (!AllocMemory(len+1, MEM_STRING|MEM_NO_CLEAR|Self->Head.MemFlags, (void **)&Self->Path, &Self->PathMID)) {
         for (j=0; j < len; j++) Self->Path[j] = Value[j];
         Self->Path[j] = 0;

#ifdef __unix__
         STRING path;
         if (!ResolvePath(Self->Path, RSF_NO_FILE_CHECK, &path)) {
            int result = chdir(path);
            if (result) log.warning("Failed to switch current path to: %s", path);
            FreeResource(path);

            if (result) return ERR_InvalidPath;
         }
         else log.warning("Failed to resolve path \"%s\"", Self->Path);
#elif _WIN32
         STRING path;
         if (!ResolvePath(Self->Path, RSF_NO_FILE_CHECK|RSF_PATH, &path)) {
            LONG result = chdir(path);
            if (result) log.warning("Failed to switch current path to: %s", path);
            FreeResource(path);

            if (result) return ERR_InvalidPath;
         }
         else return log.warning(ERR_InvalidPath);
#else
#warn Support required for changing the current path.
#endif
      }
      else return log.warning(ERR_AllocMemory);
   }
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ProcessPath: The path of the executable that is associated with the task.

The ProcessPath is set to the path of the executable file that is associated with the task.  It is managed internally
and cannot be altered.

In Microsoft Windows it is not always possible to determine the origins of an executable, in which case the
ProcessPath is set to the working folder in use at the time the process was launched.

*****************************************************************************/

static ERROR GET_ProcessPath(objTask *Self, CSTRING *Value)
{
   parasol::Log log;

   if (Self->ProcessPath) {
      *Value = Self->ProcessPath;
      return ERR_Okay;
   }
   else if (!Self->ProcessPathMID) {
      *Value = NULL;
      return ERR_FieldNotSet;
   }
   else if (!AccessMemory(Self->ProcessPathMID, MEM_READ, 2000, (void **)&Self->ProcessPath)) {
      *Value = Self->ProcessPath;
      return ERR_Okay;
   }
   else {
      *Value = NULL;
      return log.warning(ERR_AccessMemory);
   }
}

/*****************************************************************************

-FIELD-
Priority: The task priority in relation to other tasks is be defined here.

Set the Priority field to change the priority of the process associated with the task.  The default value for all
processes is zero.  High positive values will give the process more CPU time while negative values will yield
CPU time to other active processes.

Note that depending on the platform, there may be limits as to whether one process can change the priority level
of a foreign process.  Other factors such as the scheduler used by the host system should be considered in the
effect of prioritisation.

*****************************************************************************/

static ERROR SET_Priority(objTask *Self, LONG Value)
{
#ifdef __unix__
   LONG priority, unused;
   priority = -getpriority(PRIO_PROCESS, 0);
   unused = nice(-(Value - priority));
#endif
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ReturnCode: The task's return code can be retrieved following execution.

Once a process has completed execution then its return code can be read from this field.  If process is still running,
the error code ERR_TaskStillExists will be returned.

-ERRORS-
Okay
TaskStillExists: The task is still running and has no return code at this stage.
DoesNotExist: The task is yet to be successfully launched with the Activate action.

*****************************************************************************/

static ERROR GET_ReturnCode(objTask *Self, LONG *Value)
{
   parasol::Log log;

   if (Self->ReturnCodeSet) {
      *Value = Self->ReturnCode;
      return ERR_Okay;
   }

   if (!Self->ProcessID) {
      log.msg("Task hasn't been launched yet.");
      return ERR_DoesNotExist;
   }

#ifdef __unix__
   // Please note that ProcessMessages() will typically kill zombie processes.  This means waitpid() may not return the
   // status (although it will inform us that the task no longer exists).  This issue is resolved by getting
   // ProcessMessages() to set the ReturnCode field when it detects zombie tasks.

   LONG status = 0;
   LONG result = waitpid(Self->ProcessID, &status, WNOHANG);

   if ((result IS -1) OR (result IS Self->ProcessID)) {
      // The process has exited.  Find out what error code was returned and pass it as the result.

      if (WIFEXITED(status)) {
         Self->ReturnCode = (BYTE)WEXITSTATUS(status);
         Self->ReturnCodeSet = TRUE;
      }

      *Value = Self->ReturnCode;
      return ERR_Okay;
   }
   else return ERR_TaskStillExists;

#elif _WIN32

   winGetExitCodeProcess(Self->Platform, &Self->ReturnCode);
   if (Self->ReturnCode IS 259) return ERR_TaskStillExists;
   else {
      Self->ReturnCodeSet = TRUE;
      *Value = Self->ReturnCode;
      return ERR_Okay;
   }

#else

   return ERR_NoSupport;

#endif
}

static ERROR SET_ReturnCode(objTask *Self, LONG Value)
{
   Self->ReturnCodeSet = TRUE;
   Self->ReturnCode = Value;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Short: A short description of the process' purpose.

This field allows for the specification of a short description for the process. The description should be under 80
characters (one sentence).  The description is typically useful for occasions where the user is debugging the
system or trying to get a quick overview of the processes that are currently running.
-END-

*****************************************************************************/

static ERROR GET_Short(objTask *Self, CSTRING *Value)
{
   *Value = Self->Short;
   return ERR_Okay;
}

static ERROR SET_Short(objTask *Self, CSTRING Value)
{
   StrCopy(Value, Self->Short, sizeof(Self->Short));
   return ERR_Okay;
}

//****************************************************************************

static const FieldArray clFields[] = {
   { "TimeOut",         FDF_DOUBLE|FDF_RW,     0, NULL, NULL },
   { "Flags",           FDF_LONGFLAGS|FDF_RI, (MAXINT)&clFlags, NULL, NULL },
   { "ReturnCode",      FDF_LONG|FDF_RW,      0, (APTR)GET_ReturnCode, (APTR)SET_ReturnCode },
   { "ProcessID",       FDF_LONG|FDF_RI,      0, NULL, NULL },
   // Virtual fields
   { "Actions",        FDF_POINTER|FDF_R,   0, (APTR)GET_Actions,          NULL },
   { "Args",           FDF_STRING|FDF_W,    0, NULL,                       (APTR)SET_Args },
   { "Parameters",     FDF_ARRAY|FDF_STRING|FDF_RW, 0, (APTR)GET_Parameters, (APTR)SET_Parameters },
   { "Author",         FDF_STRING|FDF_RW,   0, (APTR)GET_Author,           (APTR)SET_Author },
   { "Copyright",      FDF_STRING|FDF_RW,   0, (APTR)GET_Copyright,        (APTR)SET_Copyright },
   { "Date",           FDF_STRING|FDF_RW,   0, (APTR)GET_Date,             (APTR)SET_Date },
   { "ErrorCallback",  FDF_FUNCTIONPTR|FDF_RI, 0, (APTR)GET_ErrorCallback, (APTR)SET_ErrorCallback }, // STDERR
   { "ExitCallback",   FDF_FUNCTIONPTR|FDF_RW, 0, (APTR)GET_ExitCallback,  (APTR)SET_ExitCallback },
   { "Instance",       FDF_LONG|FDF_R,      0, (APTR)GET_Instance,         NULL },
   { "InputCallback",  FDF_FUNCTIONPTR|FDF_RW, 0, (APTR)GET_InputCallback, (APTR)SET_InputCallback }, // STDIN
   { "LaunchPath",     FDF_STRING|FDF_RW,   0, (APTR)GET_LaunchPath,       (APTR)SET_LaunchPath },
   { "Location",       FDF_STRING|FDF_RW,   0, (APTR)GET_Location,         (APTR)SET_Location },
   { "MessageQueue",   FDF_LONG|FDF_R,      0, (APTR)GET_MessageQueue,     NULL },
   { "Name",           FDF_STRING|FDF_RW,   0, (APTR)GET_Name,             (APTR)SET_Name },
   { "OutputCallback", FDF_FUNCTIONPTR|FDF_RI, 0, (APTR)GET_OutputCallback, (APTR)SET_OutputCallback }, // STDOUT
   { "Path",           FDF_STRING|FDF_RW,   0, (APTR)GET_Path,              (APTR)SET_Path },
   { "ProcessPath",    FDF_STRING|FDF_R,    0, (APTR)GET_ProcessPath,       NULL },
   { "Priority",       FDF_LONG|FDF_W,      0, NULL,                        (APTR)SET_Priority },
   { "Short",          FDF_STRING|FDF_RW,   0, (APTR)GET_Short,             (APTR)SET_Short },
   // Synonyms
   { "Src",            FDF_SYNONYM|FDF_STRING|FDF_RW, 0, (APTR)GET_Location, (APTR)SET_Location },
   { "ArgsList",       FDF_ARRAY|FDF_STRING|FDF_SYSTEM|FDF_RW, 0, (APTR)GET_Parameters, (APTR)SET_Parameters }, // OBSOLETE
   END_FIELD
};

//****************************************************************************

extern "C" ERROR add_task_class(void)
{
   parasol::Log log(__FUNCTION__);

   log.branch("");

   if (!NewPrivateObject(ID_METACLASS, 0, (OBJECTPTR *)&TaskClass)) {
      if (!SetFields((OBJECTPTR)TaskClass,
            FID_ClassVersion|TFLOAT,  VER_TASK,
            FID_Name|TSTRING,         "Task",
            FID_Category|TLONG,       CCF_SYSTEM,
            FID_FileExtension|TSTR,   "*.exe|*.bat|*.com",
            FID_FileDescription|TSTR, "Executable File",
            FID_FileHeader|TSTR,      "[0:$4d5a]|[0:$7f454c46]",
            FID_Actions|TPTR,         clActions,
            FID_Methods|TARRAY,       clTaskMethods,
            FID_Fields|TARRAY,        clFields,
            FID_Size|TLONG,           sizeof(objTask),
            FID_Path|TSTR,            "modules:core",
            TAGEND)) {
         return acInit(&TaskClass->Head);
      }
      else return ERR_SetField;
   }
   else return ERR_NewObject;
}