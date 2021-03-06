// Auto-generated by idl-c.fluid

#ifdef  __cplusplus
extern "C" {
#endif

ERROR AccessMemory(MEMORYID Memory, LONG Flags, LONG MilliSeconds, APTR * Result);
ERROR Action(LONG Action, OBJECTPTR Object, APTR Parameters);
void ActionList(struct ActionTable ** Actions, LONG * Size);
ERROR ActionMsg(LONG Action, OBJECTID Object, APTR Args, MEMORYID MessageID, CLASSID ClassID);
ERROR ActionTags(LONG Action, OBJECTPTR Object, ...);
CSTRING ResolveClassID(CLASSID ID);
LONG AllocateID(LONG Type);
ERROR AllocMemory(LONG Size, LONG Flags, APTR * Address, MEMORYID * ID);
ERROR AccessObject(OBJECTID Object, LONG MilliSeconds, OBJECTPTR * Result);
ERROR ListTasks(LONG Flags, struct ListTasks ** List);
ERROR CheckAction(OBJECTPTR Object, LONG Action);
ERROR CheckMemoryExists(MEMORYID ID);
ERROR CheckObjectExists(OBJECTID Object, CSTRING Name);
ERROR CloneMemory(APTR Address, LONG Flags, APTR * NewAddress, MEMORYID * NewID);
ERROR CreateObject(LARGE ClassID, LONG Flags, OBJECTPTR * Object, ...);
OBJECTPTR CurrentContext();
ERROR GetFieldArray(OBJECTPTR Object, FIELD Field, APTR * Result, LONG * Elements);
LONG AdjustLogLevel(LONG Adjust);
void LogF(CSTRING Header, CSTRING Message, ...);
ERROR FindObject(CSTRING Name, CLASSID ClassID, LONG Flags, OBJECTID * Array, LONG * Count);
struct rkMetaClass * FindClass(CLASSID ClassID);
ERROR ReleaseObject(OBJECTPTR Object);
ERROR FreeResource(const void * Address);
ERROR FreeResourceID(MEMORYID ID);
CLASSID GetClassID(OBJECTID Object);
OBJECTID GetOwnerID(OBJECTID Object);
ERROR GetField(OBJECTPTR Object, FIELD Field, APTR Result);
ERROR GetFieldVariable(OBJECTPTR Object, CSTRING Field, STRING Buffer, LONG Size);
ERROR GetFields(OBJECTPTR Object, ...);
CSTRING GetName(OBJECTPTR Object);
ERROR ListChildren(OBJECTID Object, LONG IncludeShared, struct ChildEntry * List, LONG * Count);
ERROR StrBase64Decode(struct rkBase64Decode * State, CSTRING Input, LONG InputSize, APTR Output, LONG * Written);
ERROR RegisterFD(HOSTHANDLE FD, LONG Flags, void (*Routine)(HOSTHANDLE, APTR) , APTR Data);
ERROR ManageAction(LONG Action, APTR Routine);
ERROR MemoryIDInfo(MEMORYID ID, struct MemInfo * MemInfo, LONG Size);
ERROR MemoryPtrInfo(APTR Address, struct MemInfo * MemInfo, LONG Size);
ERROR NewObject(LARGE ClassID, LONG Flags, OBJECTPTR * Object);
LONG NotifySubscribers(OBJECTPTR Object, LONG Action, APTR Args, LONG Flags, ERROR Error);
ERROR StrReadLocale(CSTRING Key, CSTRING * Value);
APTR GetMemAddress(MEMORYID ID);
ERROR ProcessMessages(LONG Flags, LONG TimeOut);
LONG RandomNumber(LONG Range);
ERROR ReallocMemory(APTR Memory, LONG Size, APTR * Address, MEMORYID * ID);
ERROR GetMessage(MEMORYID Queue, LONG Type, LONG Flags, APTR Buffer, LONG Size);
MEMORYID ReleaseMemory(APTR Address);
CLASSID ResolveClassName(CSTRING Name);
void SelfDestruct();
ERROR SendMessage(MEMORYID Queue, LONG Type, LONG Flags, APTR Data, LONG Size);
ERROR SetOwner(OBJECTPTR Object, OBJECTPTR Owner);
OBJECTPTR SetContext(OBJECTPTR Object);
ERROR SetField(OBJECTPTR Object, FIELD Field, ...);
ERROR SetFields(OBJECTPTR Object, ...);
ERROR SetFieldEval(OBJECTPTR Object, CSTRING Field, CSTRING Value);
ERROR SetName(OBJECTPTR Object, CSTRING Name);
void LogReturn();
ERROR StrCompare(CSTRING String1, CSTRING String2, LONG Length, LONG Flags);
ERROR SubscribeAction(OBJECTPTR Object, LONG Action);
ERROR SubscribeFeed(OBJECTPTR Object);
ERROR SubscribeEvent(LARGE Event, FUNCTION * Callback, APTR Custom, APTR * Handle);
ERROR SubscribeTimer(DOUBLE Interval, FUNCTION * Callback, APTR * Subscription);
ERROR UpdateTimer(APTR Subscription, DOUBLE Interval);
ERROR UnsubscribeAction(OBJECTPTR Object, LONG Action);
ERROR UnsubscribeFeed(OBJECTPTR Object);
void UnsubscribeEvent(APTR Event);
ERROR BroadcastEvent(APTR Event, LONG EventSize);
void WaitTime(LONG Seconds, LONG MicroSeconds);
LARGE GetEventID(LONG Group, CSTRING SubGroup, CSTRING Event);
ULONG GenCRC32(ULONG CRC, APTR Data, ULONG Length);
LARGE GetResource(LONG Resource);
LARGE SetResource(LONG Resource, LARGE Value);
ERROR ScanMessages(APTR Queue, LONG * Index, LONG Type, APTR Buffer, LONG Size);
ERROR SysLock(LONG Index, LONG MilliSeconds);
ERROR SysUnlock(LONG Index);
ERROR CopyMemory(const void * Src, APTR Dest, LONG Size);
ERROR ClearMemory(APTR Memory, LONG Length);
ERROR SubscribeActionTags(OBJECTPTR Object, ...);
void PrintDiagnosis(LONG Process, LONG Signal);
ERROR AssociateCmd(CSTRING Path, CSTRING Mode, LONG Flags, CSTRING Command);
ERROR UpdateMessage(APTR Queue, LONG Message, LONG Type, APTR Data, LONG Size);
ERROR AddMsgHandler(APTR Custom, LONG MsgType, FUNCTION * Routine, struct MsgHandler ** Handle);
ERROR FindPrivateObject(CSTRING Name, OBJECTPTR * Object);
LARGE PreciseTime();
ERROR SetFieldsID(OBJECTID Object, ...);
OBJECTPTR GetObjectPtr(OBJECTID Object);
struct Field * FindField(OBJECTPTR Object, ULONG FieldID, OBJECTPTR * Source);
struct Field * CurrentField();
LONG GetMsgPort(OBJECTID Object);
CSTRING GetErrorMsg(ERROR Error);
struct Message * GetActionMsg();
ERROR FuncError(CSTRING Header, ERROR Error);
ERROR SetArray(OBJECTPTR Object, FIELD Field, APTR Array, LONG Elements);
ERROR ReleaseMemoryID(MEMORYID MemoryID);
ERROR AccessPrivateObject(OBJECTPTR Object, LONG MilliSeconds);
void ReleasePrivateObject(OBJECTPTR Object);
ERROR AllocMutex(LONG Flags, APTR * Result);
void FreeMutex(APTR Mutex);
ERROR LockMutex(APTR Mutex, LONG MilliSeconds);
void UnlockMutex(APTR Mutex);
ERROR ActionThread(LONG Action, OBJECTPTR Object, APTR Args, FUNCTION * Callback, LONG Key);
MEMORYID GetFeedList(OBJECTPTR Object);
ERROR AllocSharedMutex(CSTRING Name, APTR * Mutex);
void FreeSharedMutex(APTR Mutex);
ERROR LockSharedMutex(APTR Mutex, LONG MilliSeconds);
void UnlockSharedMutex(APTR Mutex);
LONG StrShrink(STRING String, LONG Offset, LONG TotalBytes);
LONG StrExpand(STRING String, LONG Offset, LONG TotalChars);
ERROR StrInsert(CSTRING Insert, STRING Buffer, LONG Size, LONG Offset, LONG ReplaceChars);
LONG StrSearch(CSTRING Keyword, CSTRING String, LONG Flags);
void StrUpper(STRING String);
void StrLower(STRING String);
void StrCapitalise(STRING String);
LONG StrLineLength(CSTRING String);
CSTRING StrNextLine(CSTRING String);
ERROR StrReplace(CSTRING Src, CSTRING Keyword, CSTRING Replacement, STRING * Result, LONG Flags);
ERROR StrEvaluate(STRING Buffer, LONG Size, LONG Flags, OBJECTID Owner);
ERROR StrCalculate(CSTRING String, DOUBLE * Result, STRING Buffer, LONG Size);
DOUBLE StrToFloat(CSTRING String);
LONG StrCopy(CSTRING Src, STRING Dest, LONG Length);
STRING StrClone(CSTRING String);
LONG StrLength(CSTRING String);
LARGE StrToInt(CSTRING String);
ERROR StrSort(CSTRING * List, LONG Flags);
STRING * StrBuildArray(STRING List, LONG Size, LONG Total, LONG Flags);
LONG UTF8CharOffset(CSTRING String, LONG Offset);
LONG UTF8Length(CSTRING String);
LONG UTF8OffsetToChar(CSTRING String, LONG Offset);
LONG UTF8PrevLength(CSTRING String, LONG Offset);
LONG UTF8CharLength(CSTRING String);
ULONG UTF8ReadValue(CSTRING String, LONG * Length);
LONG UTF8WriteValue(LONG Value, STRING Buffer, LONG Size);
LONG StrFormat(STRING Buffer, LONG Size, CSTRING Format, ...);
ERROR StrFormatDate(STRING Buffer, LONG Size, CSTRING Format, struct DateTime * Time);
ERROR StrToColour(CSTRING Colour, struct RGB8 * RGB);
LONG StrDatatype(CSTRING String);
LONG CharCopy(CSTRING Source, STRING Dest, LONG Length);
LARGE StrToHex(CSTRING String);
CSTRING StrTranslateText(CSTRING String);
LONG StrTranslateRefresh();
LONG StrSortCompare(CSTRING String1, CSTRING String2);
ERROR StrReadDate(CSTRING String, struct DateTime * DateTime);
LONG StrEvalConditional(CSTRING Statement);
LONG UTF8Copy(CSTRING Src, STRING Dest, LONG Chars, LONG Size);
LONG StrBase64Encode(const void * Input, LONG InputSize, STRING Output, LONG OutputSize);
ERROR VarSetString(struct KeyStore * Store, CSTRING Key, CSTRING Value);
CSTRING VarGetString(struct KeyStore * Store, CSTRING Key);
ERROR VarCopy(struct KeyStore * Source, struct KeyStore * Dest);
ULONG StrHash(CSTRING String, LONG CaseSensitive);
CSTRING UTF8ValidEncoding(CSTRING String, CSTRING Encoding);
ERROR AnalysePath(CSTRING Path, LONG * Type);
ERROR CreateFolder(CSTRING Path, LONG Permissions);
ERROR MoveFile(CSTRING Source, CSTRING Dest, FUNCTION * Callback);
ERROR ResolvePath(CSTRING Path, LONG Flags, STRING * Result);
ERROR SetVolume(LARGE,...);
ERROR DeleteVolume(CSTRING Name);
ERROR VirtualVolume(CSTRING Name, ...);
ERROR CopyFile(CSTRING Source, CSTRING Dest, FUNCTION * Callback);
ERROR SetDocView(CSTRING Path, CSTRING Document);
CSTRING GetDocView(CSTRING Path);
ERROR DeleteFile(CSTRING Path, FUNCTION * Callback);
ERROR WaitForObjects(LONG Flags, LONG TimeOut, struct ObjectSignal * ObjectSignals);
ERROR SaveObjectToFile(OBJECTPTR Object, CSTRING Path, LONG Permissions);
ERROR OpenDir(CSTRING Path, LONG Flags, struct DirInfo ** Info);
ERROR ScanDir(struct DirInfo * Info);
ERROR IdentifyFile(CSTRING Path, CSTRING Mode, LONG Flags, CLASSID * Class, CLASSID * SubClass, STRING * Command);
ERROR TranslateCmdRef(CSTRING String, STRING * Command);
ERROR CreateLink(CSTRING From, CSTRING To);
void VarUnlock(struct KeyStore * Store);
void SetDefaultPermissions(LONG User, LONG Group, LONG Permissions);
CSTRING ResolveUserID(LONG User);
CSTRING ResolveGroupID(LONG Group);
ERROR ReadFileToBuffer(CSTRING Path, APTR Buffer, LONG BufferSize, LONG * Result);
ERROR LoadFile(CSTRING Path, LONG Flags, struct CacheFile ** Cache);
void UnloadFile(struct CacheFile * Cache);
ERROR AddInfoTag(struct FileInfo * Info, CSTRING Name, CSTRING Value);
ERROR SaveImageToFile(OBJECTPTR Object, CSTRING Path, CLASSID Class, LONG Permissions);
ERROR CompareFilePaths(CSTRING PathA, CSTRING PathB);
const struct SystemState * GetSystemState();
ERROR SetResourcePath(LONG PathType, CSTRING Path);
OBJECTPTR CurrentTask();
struct KeyStore * VarNew(LONG InitialSize, LONG Flags);
APTR VarSet(struct KeyStore * Store, CSTRING Key, APTR Data, LONG Size);
ERROR VarGet(struct KeyStore * Store, CSTRING Name, APTR * Data, LONG * Size);
ERROR KeySet(struct KeyStore * Store, ULONG Key, const void * Data, LONG Size);
ERROR KeyGet(struct KeyStore * Store, ULONG Key, APTR * Data, LONG * Size);
ERROR VarIterate(struct KeyStore * Store, CSTRING Index, CSTRING * Key, APTR * Data, LONG * Size);
ERROR NewLockedObject(LARGE ClassID, LONG Flags, OBJECTPTR * Object, OBJECTID * ID, CSTRING Name);
ERROR KeyIterate(struct KeyStore * Store, ULONG Index, ULONG * Key, APTR * Data, LONG * Size);
ERROR VarSetSized(struct KeyStore * Store, CSTRING Key, LONG Size, APTR * Data, LONG * DataSize);
ERROR VarLock(struct KeyStore * Store, LONG Timeout);
void VLogF(LONG Flags, CSTRING Header, CSTRING Message, va_list Args);
ERROR WakeProcess(LONG ProcessID);

#ifdef  __cplusplus
}
#endif
