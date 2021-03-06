/*****************************************************************************

Functions that are internal to the Core.

*****************************************************************************/

#ifdef __unix__
   #include <unistd.h>
   #include <signal.h>
   #include <sys/ipc.h>
   #ifndef __ANDROID__
      #include <sys/shm.h>
      #include <sys/sem.h>
      #include <sys/msg.h>
   #endif
   #include <sys/types.h>
   #include <sys/wait.h>
   #include <string.h>
   #include <errno.h>
   #include <signal.h>
#endif

#include "defs.h"

using namespace parasol;

//****************************************************************************
// If a function has reason to believe that a process has crashed or is failing to unlock memory blocks, it can call
// the validate_process() function to help clean up the system.

ERROR validate_process(LONG ProcessID)
{
   parasol::Log log(__FUNCTION__);
   OBJECTID task_id;
   LONG i;
   BYTE broadcastpid;
   static LONG glValidating = 0;

   log.function("PID: %d", ProcessID);

   if (glValidating) return ERR_Okay;
   if (glValidateProcessID IS ProcessID) glValidateProcessID = 0;
   if ((ProcessID IS glProcessID) or (!ProcessID)) return ERR_Okay;

   // Determine whether or not the process is alive

   #ifdef _WIN32
      // On Windows we don't check if the process is alive because validation can often occur during the final shutdown
      // phase of the other process.
   #elif __unix__
      if ((kill(ProcessID, 0) IS -1) and (errno IS ESRCH));
      else return ERR_Okay;
   #else
      log.error("This platform does not support validate_process()");
      return ERR_Okay;
   #endif

   task_id = 0;
   broadcastpid = FALSE;
   if (!LOCK_PROCESS_TABLE(4000)) {
      for (i=0; i < MAX_TASKS; i++) {
         if (shTasks[i].ProcessID IS ProcessID) {
            task_id = shTasks[i].TaskID;
            ClearMemory(shTasks+i, sizeof(struct TaskList));
            broadcastpid = TRUE;
            break;
         }
      }

      UNLOCK_PROCESS_TABLE();
   }

    if (broadcastpid) {
      // Broadcast a system.task.removed signal.  Foreign processes are broadcast if we were responsible for launching them (i.e. the process is referenced in the task list).

      evTaskRemoved task_removed = { GetEventID(EVG_SYSTEM, "task", "removed"), task_id, ProcessID };
      BroadcastEvent(&task_removed, sizeof(task_removed));
   }

   if (!task_id) {
      log.msg("No task slot for process %d - handled by someone else?", ProcessID);
      return ERR_False;
   }

   log.branch("Process %d / task #%d no longer exists, validating...", ProcessID, task_id);

   glValidating = ProcessID;

   remove_public_locks(ProcessID);
   free_public_resources(task_id);

   // Wake up foreign tasks that are waiting on the crashed process.  Very straight-forward as we can do a broadcast.

   #ifdef __unix__
      if (!LOCK_PUBLIC_MEMORY(1000)) {
         pthread_cond_broadcast(&glSharedControl->PublicLocks[PL_PUBLICMEM].Cond);
         UNLOCK_PUBLIC_MEMORY();
      }

      if (!LOCK_SEMAPHORES(1000)) {
         pthread_cond_broadcast(&glSharedControl->PublicLocks[PL_SEMAPHORES].Cond);
         UNLOCK_SEMAPHORES();
      }
   #endif

   log.msg("Validation complete.");
   glValidating = 0;
   return ERR_False; // Return ERR_False to indicate that the task was not healthy
}

/*****************************************************************************

copy_args: Used for turning argument structures into sendable messages.

This function searches an argument structure for pointer and string types.  If it encounters them, it attempts to
convert them to a format that can be passed to other memory spaces.

A PTR|RESULT followed by a PTRSIZE indicates that the user has to supply a buffer to the function.  It is assumed that
the function will fill the buffer with data, which means that a result set has to be returned to the caller.  Example:

<pre>
Read(Bytes (FD_LONG), Buffer (FD_PTRRESULT), BufferSize (FD_PTRSIZE), &BytesRead (FD_LONGRESULT));
</pre>

A standard PTR followed by a PTRSIZE indicates that the user has to supply a buffer to the function.  It is assumed
that this is one-way traffic only, and the function will not fill the buffer with data.  Example:

<pre>
Write(Bytes (FD_LONG, Buffer (FD_PTR), BufferSize (FD_PTRSIZE), &BytesWritten (FD_LONGRESULT));
</pre>

If the function will return a memory block of its own, it must return the block as a MEMORYID, not a PTR.  The
allocation must be made using the object's MemFlags, as the action messaging functions will change between
public|untracked and private memory flags as necessary.  Example:

  Read(Bytes (FD_LONG), &BufferMID (FD_LONGRESULT), &BufferSize (FD_LONGRESULT));

*****************************************************************************/

/***
NOTE: THIS ROUTINE REQUIRES BETTER SUPPORT FOR MESSAGING AC_ACTIONNOTIFY ACTIONS, AS THE ACTIONNOTIFY STRUCTURE
CONTAINS AN ACTION STRUCTURE POINTER.  THIS SITUATION COULD BE SOLVED BY INTRODUCING AN 'FD_ACTION' FLAG FOR THE
ACTIONNOTIFY ARGS ARRAY IN DATA.C, WHICH WE COULD DETECT and DEAL WITH IN THIS ROUTINE.
***/

ERROR copy_args(const struct FunctionField *Args, LONG ArgsSize, BYTE *ArgsBuffer, BYTE *Buffer, LONG BufferSize,
                    LONG *NewSize, WORD *WaitResult, CSTRING ActionName)
{
   parasol::Log log("CopyArguments");
   BYTE *src, *data;
   MEMORYID memoryid;
   LONG memsize, i, j, len, pos, offset;
   ERROR error;
   STRING str;

   if ((!Args) or (!ArgsBuffer) or (!Buffer)) return log.warning(ERR_NullArgs);

   *WaitResult = FALSE;

   for (i=0; (i < ArgsSize); i++) { // Copy the arguments to the buffer
      if (i >= BufferSize) return log.warning(ERR_BufferOverflow);
      Buffer[i] = ArgsBuffer[i];
   }

   pos = 0;
   offset = ArgsSize;
   for (i=0; Args[i].Name; i++) {
      // If the current byte position in the argument structure exceeds the size of that structure, break immediately.

      if (pos >= ArgsSize) {
         log.error("Invalid action definition for \"%s\".  Amount of arguments exceeds limit of %d bytes.", ActionName, ArgsSize);
         break;
      }

      // Process the argument depending on its type

      if (Args[i].Type & FD_STR) { // Copy the string and make sure that there is enough space for it to fit inside the buffer.
         j = offset;
         if ((str = ((STRING *)(ArgsBuffer + pos))[0])) {
            for (len=0; (str[len]) and (offset < BufferSize); len++) {
               Buffer[offset++] = str[len];
            }
            if (offset < BufferSize) {
               Buffer[offset++] = 0;
               ((LONG *)(Buffer + pos))[0] = j; // Replace the pointer with a translatable offset
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else ((LONG *)(Buffer + pos))[0] = 0;

         pos += sizeof(STRING);
      }
      else if (Args[i].Type & FD_PTR) {
         if (Args[i].Type & (FD_LONG|FD_PTRSIZE)) { // Pointer to long.
            if ((size_t)offset < (BufferSize - sizeof(LONG))) {
               ((LONG *)Buffer)[offset] = ((LONG *)(ArgsBuffer + pos))[0];
               ((LONG *)(Buffer + pos))[0] = offset;
               offset += sizeof(LONG);
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) { // Pointer to large/double
            if ((size_t)offset < (BufferSize - sizeof(LARGE))) {
               ((LARGE *)Buffer)[offset] = ((LARGE *)(ArgsBuffer + pos))[0];
               ((LONG *)(Buffer + pos))[0] = offset;
               offset += sizeof(LARGE);
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else {
            // There are two types of pointer references:

            // 1. Receive Pointers
            //    If FD_RESULT is used, this indicates that there is a result to be stored in a buffer setup by the
            //    caller.  The size of this is determined by a following FD_PTRSIZE.

            // 2. Send Pointers
            //    Standard FD_PTR types must be followed by an FD_PTRSIZE that indicates the amount of data that needs
            //    to be passed to the other task.  A public memory block is allocated and filled with data for this
            //    particular type.

            if (!(Args[i+1].Type & FD_PTRSIZE)) {
               // If no PTRSIZE is specified, send a warning
               log.warning("Warning: Argument \"%s\" is not followed up with a PTRSIZE definition.", Args[i].Name);
               ((APTR *)(Buffer + pos))[0] = NULL;
            }
            else {
               memsize = ((LONG *)(ArgsBuffer + pos + sizeof(APTR)))[0];
               if (memsize > 0) {
                  if (Args[i].Type & FD_RESULT) {
                     // "Receive" pointer type: Prepare a public buffer so that we can accept a result

                     if (!AllocMemory(memsize, MEM_PUBLIC|MEM_NO_CLEAR, NULL, &memoryid)) {
                        ((LONG *)(Buffer + pos))[0] = memoryid;
                     }
                     else { error = ERR_AllocMemory; goto looperror; }
                  }
                  else {
                     // "Send" pointer type: Prepare the argument structure for sending data to the other task

                     if ((src = ((BYTE **)(ArgsBuffer + pos))[0])) { // Get the data source pointer
                        if (memsize > MSG_MAXARGSIZE) {
                           // For large data areas, we need to allocate them as public memory blocks

                           if (!AllocMemory(memsize, MEM_PUBLIC|MEM_NO_CLEAR, (void **)&data, &memoryid)) {
                              ((LONG *)(Buffer + pos))[0] = memoryid;
                              CopyMemory(src, data, memsize);
                              ReleaseMemoryID(memoryid);
                           }
                           else { error = ERR_AllocMemory; goto looperror; }
                        }
                        else {
                           ((LONG *)(Buffer + pos))[0] = offset; // Record the offset at which we are going to write the data
                           for (len=0; (len < memsize); len++) {
                              if (offset >= BufferSize) {
                                 error = ERR_BufferOverflow;
                                 goto looperror;
                              }
                              else Buffer[offset++] = src[len];
                           }
                        }
                     }
                     else ((LONG *)(Buffer + pos))[0] = 0;
                  }
               }
               else ((LONG *)(Buffer + pos))[0] = 0;
            }
         }
         pos += sizeof(APTR);
      }
      else if (Args[i].Type & (FD_LONG|FD_PTRSIZE)) pos += sizeof(LONG);
      else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) pos += sizeof(LARGE);
      else log.warning("Bad type definition for argument \"%s\".", Args[i].Name);

      if (Args[i].Type & FD_RESULT) {
         // No extra action is taken when FD_RESULT is used in conjunction with data types (e.g. FD_RESULT|FD_LONG).
         // These types can only be fixed up when the message returns.

         *WaitResult = TRUE;
      }
   }

   *NewSize = offset;
   return ERR_Okay;

looperror:
   // When an error occurs inside the loop, we back-track through the position at where the error occurred and free any
   // memory allocations that have been made.

   return log.warning(error);
}

/*****************************************************************************
** This version of copy_args() is for thread-based execution.  Used by ActionThread() in lib_actions.c
*/

ERROR local_copy_args(const struct FunctionField *Args, LONG ArgsSize, BYTE *ArgsBuffer, BYTE *Buffer, LONG BufferSize,
                    LONG *NewSize, CSTRING ActionName)
{
   parasol::Log log("CopyArguments");
   BYTE *src, *data;
   LONG memsize, j, len;
   ERROR error;
   STRING str;

   if ((!Args) or (!ArgsBuffer) or (!Buffer)) return log.warning(ERR_NullArgs);

   for (LONG i=0; (i < ArgsSize); i++) { // Copy the arguments to the buffer
      if (i >= BufferSize) return log.warning(ERR_BufferOverflow);
      Buffer[i] = ArgsBuffer[i];
   }

   LONG pos = 0;
   LONG offset = ArgsSize;
   for (LONG i=0; Args[i].Name; i++) {
      // If the current byte position in the argument structure exceeds the size of that structure, break immediately.

      if (pos >= ArgsSize) {
         log.error("Invalid action definition for \"%s\".  Amount of arguments exceeds limit of %d bytes.", ActionName, ArgsSize);
         break;
      }

      // Process the argument depending on its type

      if (Args[i].Type & FD_STR) { // Copy the string and make sure that there is enough space for it to fit inside the buffer.
         j = offset;
         if ((str = ((STRING *)(ArgsBuffer + pos))[0])) {
            for (len=0; (str[len]) and (offset < BufferSize); len++) Buffer[offset++] = str[len];
            if (offset < BufferSize) {
               Buffer[offset++] = 0;
               ((STRING *)(Buffer + pos))[0] = str;
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else ((STRING *)(Buffer + pos))[0] = NULL;

         pos += sizeof(STRING);
      }
      else if (Args[i].Type & FD_PTR) {
         if (Args[i].Type & (FD_LONG|FD_PTRSIZE)) { // Pointer to long.
            if ((size_t)offset < (BufferSize - sizeof(LONG))) {
               ((LONG *)Buffer)[offset] = ((LONG *)(ArgsBuffer + pos))[0];
               ((APTR *)(Buffer + pos))[0] = ArgsBuffer + offset;
               offset += sizeof(LONG);
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) { // Pointer to large/double
            if ((size_t)offset < (BufferSize - sizeof(LARGE))) {
               ((LARGE *)Buffer)[offset] = ((LARGE *)(ArgsBuffer + pos))[0];
               ((APTR *)(Buffer + pos))[0] = ArgsBuffer + offset;
               offset += sizeof(LARGE);
            }
            else { error = ERR_BufferOverflow; goto looperror; }
         }
         else {
            // There are two types of pointer references:

            // 1. Receive Pointers
            //    If FD_RESULT is used, this indicates that there is a result to be stored in a buffer setup by the
            //    caller.  The size of this is determined by a following FD_PTRSIZE.

            // 2. Send Pointers
            //    Standard FD_PTR types must be followed by an FD_PTRSIZE that indicates the amount of data that needs
            //    to be passed to the other task.  A public memory block is allocated and filled with data for this
            //    particular type.

            if (!(Args[i+1].Type & FD_PTRSIZE)) {
               // If no PTRSIZE is specified, send a warning
               log.warning("Warning: Argument \"%s\" is not followed up with a PTRSIZE definition.", Args[i].Name);
               ((APTR *)(Buffer + pos))[0] = NULL;
            }
            else {
               memsize = ((LONG *)(ArgsBuffer + pos + sizeof(APTR)))[0];
               if (memsize > 0) {
                  if (Args[i].Type & FD_RESULT) { // "Receive" pointer type: Prepare a buffer so that we can accept a result
                     APTR mem;
                     if (!AllocMemory(memsize, MEM_NO_CLEAR, &mem, NULL)) {
                        ((APTR *)(Buffer + pos))[0] = mem;
                     }
                     else { error = ERR_AllocMemory; goto looperror; }
                  }
                  else {
                     // "Send" pointer type: Prepare the argument structure for sending data to the other task
                     if ((src = ((BYTE **)(ArgsBuffer + pos))[0])) { // Get the data source pointer
                        if (memsize > MSG_MAXARGSIZE) {
                           // For large data areas, we need to allocate them as public memory blocks
                           if (!AllocMemory(memsize, MEM_NO_CLEAR, (void **)&data, NULL)) {
                              ((APTR *)(Buffer + pos))[0] = data;
                              CopyMemory(src, data, memsize);
                           }
                           else { error = ERR_AllocMemory; goto looperror; }
                        }
                        else {
                           ((APTR *)(Buffer + pos))[0] = ArgsBuffer + offset; // Record the address at which we are going to write the data
                           for (LONG len=0; len < memsize; len++) {
                              if (offset >= BufferSize) {
                                 error = ERR_BufferOverflow;
                                 goto looperror;
                              }
                              else Buffer[offset++] = src[len];
                           }
                        }
                     }
                     else ((LONG *)(Buffer + pos))[0] = 0;
                  }
               }
               else ((LONG *)(Buffer + pos))[0] = 0;
            }
         }
         pos += sizeof(APTR);
      }
      else if (Args[i].Type & (FD_LONG|FD_PTRSIZE)) pos += sizeof(LONG);
      else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) pos += sizeof(LARGE);
      else log.warning("Bad type definition for argument \"%s\".", Args[i].Name);
   }

   *NewSize = offset;
   return ERR_Okay;

looperror:
   // When an error occurs inside the loop, we back-track through the position at where the error occurred and free any
   // memory allocations that have been made.

   return log.warning(error);
}

//****************************************************************************
// This version of free_ptr_args() is for thread-based execution.  Used by thread_action() in lib_actions.c

void local_free_args(APTR Parameters, const struct FunctionField *Args)
{
   LONG pos = 0;
   for (LONG i=0; Args[i].Name; i++) {
      if ((Args[i].Type & FD_PTR) and (Args[i+1].Type & FD_PTRSIZE)) {
         LONG size = ((LONG *)((BYTE *)Parameters + pos + sizeof(APTR)))[0];
         if ((Args[i].Type & FD_RESULT) or (size > MSG_MAXARGSIZE)) {
            APTR pointer;
            if ((pointer = ((APTR *)((BYTE *)Parameters + pos))[0])) {
               ((APTR *)((BYTE *)Parameters + pos))[0] = 0;
               FreeResource(pointer);
            }
         }
         pos += sizeof(APTR);
      }
      else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) pos += sizeof(LARGE);
      else pos += sizeof(LONG);
   }
}

/*****************************************************************************
** Resolves pointers and strings within an ActionMessage structure.
*/

ERROR resolve_args(APTR Parameters, const struct FunctionField *Args)
{
   parasol::Log log(__FUNCTION__);
   LONG i;
   ERROR error;
   BYTE *Buffer = (BYTE *)Parameters;
   LONG pos = 0;
   for (i=0; Args[i].Name; i++) {
      if (Args[i].Type & FD_STR) {
         // Replace the offset with a pointer
         if (((LONG *)(Buffer + pos))[0]) {
            ((STRING *)(Buffer + pos))[0] = Buffer + ((LONG *)(Buffer + pos))[0];
         }
         pos += sizeof(STRING);
      }
      else if ((Args[i].Type & FD_PTR) and (Args[i+1].Type & FD_PTRSIZE)) {
         LONG size = ((LONG *)(Buffer + pos + sizeof(APTR)))[0];
         if ((Args[i].Type & FD_RESULT) or (size > MSG_MAXARGSIZE)) {
            // Gain exclusive access to the public memory block that was allocated for this argument, and store the pointer to it.
            // The memory block will need to be released by the routine that called our function.

            MEMORYID mid = ((MEMORYID *)(Buffer + pos))[0];
            if (mid < 0) {
               APTR memory;
               if (!AccessMemory(mid, MEM_READ_WRITE, 3000, &memory)) {
                  ((APTR *)(Buffer + pos))[0] = memory;
               }
               else {
                  log.warning("Failed to gain access to memory block #%d.", mid);
                  error = ERR_AccessMemory;
                  goto looperror;
               }
            }
            else if (mid > 0) {
               log.warning("Bad memory ID #%d for arg \"%s\", not a public allocation.", mid, Args[i].Name);
               error = ERR_AccessMemory;
               goto looperror;
            }
         }
         else {
            if (((LONG *)(Buffer + pos))[0] > 0) {
               ((APTR *)(Buffer + pos))[0] = Buffer + ((LONG *)(Buffer + pos))[0];
            }
            else ((APTR *)(Buffer + pos))[0] = NULL;
         }
         pos += sizeof(APTR);
      }
      else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) pos += sizeof(LARGE);
      else pos += sizeof(LONG);
   }
   return ERR_Okay;

looperror:
   // On failure we must back-track through the array looking for pointers that we have already gained access to, and release them before returning.

   return error;
}

/*****************************************************************************
** Frees any allocations made in an argument structure.  This function is designed to work with pointer addresses, that
** have been exclusively accessed - not offsets.
*/

ERROR free_ptr_args(APTR Parameters, const struct FunctionField *Args, WORD ReleaseOnly)
{
   BYTE *Buffer = (BYTE *)Parameters;
   LONG pos = 0;
   for (LONG i=0; Args[i].Name; i++) {
      if ((Args[i].Type & FD_PTR) and (Args[i+1].Type & FD_PTRSIZE)) {
         LONG size = ((LONG *)(Buffer + pos + sizeof(APTR)))[0];
         if ((Args[i].Type & FD_RESULT) or (size > MSG_MAXARGSIZE)) {
            APTR ptr;
            if ((ptr = ((APTR *)(Buffer + pos))[0])) {
               ((APTR *)(Buffer + pos))[0] = 0;
               MEMORYID mid = ReleaseMemory(ptr);
               if (ReleaseOnly != TRUE) FreeResourceID(mid);
            }
         }
         pos += sizeof(APTR);
      }
      else if (Args[i].Type & (FD_DOUBLE|FD_LARGE)) pos += sizeof(LARGE);
      else pos += sizeof(LONG);
   }
   return ERR_Okay;
}

//****************************************************************************

void fix_core_table(struct CoreBase *CoreBase, FLOAT Version)
{
   LONG version, revision;
   version = (LONG)Version;
   revision = ((DOUBLE)Version - (DOUBLE)version + 0.000001) * 10.0;
/*
   if ((version < 5) or ((version IS 5) and (revision < 4))) {
      CoreBase->_SubscribeTimer = (APTR)SubscribeTimer_P54;
   }
*/
}

/*****************************************************************************
** This internal routine is executed periodically if this is the master task.
*/

ERROR critical_janitor(OBJECTID SubscriberID, LONG Elapsed, LONG TotalElapsed)
{
#ifdef __unix__
   parasol::Log log(__FUNCTION__);
   LONG diff, limit;

   log.traceBranch("");

   // Check if any process has locked out any *system critical* memory blocks for an unacceptable amount of time.  System critical blocks are in the
   // range of -1000 to -2000.  Processes that lock these memory blocks excessively are subject to being killed.

   for (LONG task=0; task < MAX_TASKS; task++) {
      LONG murder = 0;

      if (!LOCK_PUBLIC_MEMORY(5000)) {
         LARGE time = (PreciseTime() / 1000LL);
         for (LONG i=glSharedControl->NextBlock-1; i >= 0; i--) {
            if ((glSharedBlocks[i].AccessCount > 0) and (glSharedBlocks[i].ProcessLockID) and
                (glSharedBlocks[i].MemoryID <= -1000) and (glSharedBlocks[i].MemoryID > -2000)) {

               if (glSharedBlocks[i].AccessTime) diff = time - glSharedBlocks[i].AccessTime;
               else diff = 0;

               if (glSharedBlocks[i].MemoryID IS glSharedControl->SurfacesMID) limit = 3000;
               else limit = 3000;

               if (diff > limit) {
                  log.warning("Killing process %d for holding onto block %d for %d usec.", glSharedBlocks[i].ProcessLockID, glSharedBlocks[i].MemoryID, diff);
                  murder = glSharedBlocks[i].ProcessLockID;
                  break;
               }
            }
         }
         UNLOCK_PUBLIC_MEMORY();
      }

      if (murder) {
         // Send a safe kill signal, then followup with the most severe signal if the process didn't listen.

         log.trace("Killing process %d", murder);
         kill(murder, SIGUSR1); // Diagnosis signal
         PrintDiagnosis(murder, 0);
         kill(murder, SIGHUP); // Safe kill signal
         WaitTime(0,-200000);
         kill(murder, SIGKILL); // Force kill signal
      }
   }
#endif

   return ERR_Okay;
}

//****************************************************************************

TaskList * find_process(LONG ProcessID)
{
   for (LONG i=0; i < MAX_TASKS; i++) {
      if (ProcessID IS shTasks[i].ProcessID) return shTasks+i;
   }
   return NULL;
}

//****************************************************************************

ERROR process_janitor(OBJECTID SubscriberID, LONG Elapsed, LONG TotalElapsed)
{
#ifdef __unix__
   parasol::Log log(__FUNCTION__);

   // Call waitpid() to check for zombie processes first.  This covers all processes within our own context, so our child processes, children of those children etc.

   // However, it can be 'blocked' from certain processes, e.g. those started from ZTerm.  Such processes are discovered in the second search routine.

   LONG childprocess, status;
   while ((childprocess = waitpid(-1, &status, WNOHANG)) > 0) {
      log.warning("Zombie process #%d discovered.", childprocess);

      TaskList *task;
      if ((task = find_process(childprocess))) {
         task->ReturnCode = WEXITSTATUS(status);
         task->Returned = TRUE;
         validate_process(childprocess);
      }
   }

   if (glMasterTask) {
      // Check all registered processes to see which ones are alive.  This routine can manage all processes, although exhibits
      // some problems with zombies, hence the earlier waitpid() routine to clean up such processes.

      for (LONG i=0; i < MAX_TASKS; i++) {
         if (shTasks[i].ProcessID) {
            if ((kill(shTasks[i].ProcessID, 0) IS -1) and (errno IS ESRCH)) {
               validate_process(shTasks[i].ProcessID);
            }
         }
      }
   }

#elif _WIN32
   for (LONG i=0; i < MAX_TASKS; i++) {
      if ((shTasks[i].ProcessID) and (shTasks[i].ProcessID != glProcessID)) {
         if (!winCheckProcessExists(shTasks[i].ProcessID)) {
            validate_process(shTasks[i].ProcessID);
         }
      }
   }
#endif

   return ERR_Okay;
}

//**********************************************************************
// Returns a unique ID for the active thread.  The ID has no relationship with the host operating system.

static THREADVAR LONG tlUniqueThreadID = 0;

LONG get_thread_id(void)
{
   if (tlUniqueThreadID) return tlUniqueThreadID;
   tlUniqueThreadID = __sync_add_and_fetch(&glSharedControl->ThreadIDCount, 1);
   return tlUniqueThreadID;
}
