#undef MOD_IDL
#define MOD_IDL "s.InputEvent:pNext:InputEvent,dValue,xTimestamp,lRecipientID,lOverID,lAbsX,lAbsY,lX,lY,lDeviceID,uwType,uwFlags,uwMask\ns.dcRequest:lItem,cPreference[4]\ns.dcAudio:lSize,lFormat\ns.dcKeyEntry:lFlags,lValue,xTimestamp,lUnicode\ns.dcDeviceInput:dValue,xTimestamp,lDeviceID,lFlags,uwType,uwUnused\ns.DateTime:lYear,lMonth,lDay,lHour,lMinute,lSecond,lTimeZone\ns.HSV:dHue,dSaturation,dValue\ns.FRGB:fRed,fGreen,fBlue,fAlpha\ns.DRGB:dRed,dGreen,dBlue,dAlpha\ns.RGB8:ucRed,ucGreen,ucBlue,ucAlpha\ns.RGB16:uwRed,uwGreen,uwBlue,uwAlpha\ns.RGB32:ulRed,ulGreen,ulBlue,ulAlpha\ns.RGBPalette:lAmtColours,eCol:RGB8[256]\ns.ColourFormat:ucRedShift,ucGreenShift,ucBlueShift,ucAlphaShift,ucRedMask,ucGreenMask,ucBlueMask,ucAlphaMask,ucRedPos,ucGreenPos,ucBluePos,ucAlphaPos,ucBitsPerPixel\ns.ClipRectangle:lLeft,lRight,lBottom,lTop\ns.ObjectSignal:oObject\ns.rkBase64Decode:ucStep,ucPlainChar,ucBit\ns.rkBase64Encode:ucStep,ucResult,lStepCount\ns.FeedSubscription:lSubscriberID,lMessagePortMID,ulClassID\ns.FunctionField:sName,ulType\ns.Function:pAddress,sName,pArgs:FunctionField\ns.FieldArray:sName,ulFlags,mArg,pGetField,pSetField\ns.FieldDef:sName,lValue\ns.SystemState:sErrorMessages,sRootPath,sSystemPath,sModulePath,sPlatform,lConsoleFD,lCoreVersion,lCoreRevision,lInstanceID,lTotalErrorMessages,lStage\ns.Variable:ulType,lUnused,xLarge,dDouble,pPointer\ns.ActionArray:lActionCode,pRoutine\ns.MemoryLocks:lMemoryID,wLocks\ns.ActionTable:ulHash,lSize,sName,pArgs:FunctionField\ns.ChildEntry:lObjectID,ulClassID\ns.ListTasks:lProcessID,lTaskID,lWaitingProcessID,lWaitingMemoryID,lWaitingTime,lMessageID,lOutputID,lSemaphore,lInstanceID,lTotalMemoryLocks,lModalID,pMemoryLocks:MemoryLocks\ns.FDTable:lFD,pRoutine,pData,lFlags\ns.Message:xTime,lUniqueID,lType,lSize\ns.MemInfo:pStart,lObjectID,lSize,wAccessCount,wFlags,lMemoryID,lLockID,lTaskID,lHandle\ns.KeyStore:pMutex,pData,lTableSize,lTotal,lFlags\ns.CompressionFeedback:lFeedbackID,lIndex,sPath,sDest,xProgress,xOriginalSize,xCompressedSize,wYear,wMonth,wDay,wHour,wMinute,wSecond\ns.CompressedItem:xOriginalSize,xCompressedSize,pNext:CompressedItem,sPath,pTags:KeyStore,lPermissions,lUserID,lGroupID,lOthersID,lFlags,eCreated:DateTime,eModified:DateTime\ns.FileInfo:xSize,xTimeStamp,pNext:FileInfo,sName,pTags:KeyStore,lFlags,lPermissions,lUserID,lGroupID,eCreated:DateTime,eModified:DateTime\ns.DirInfo:pInfo:FileInfo\ns.FileFeedback:xSize,xPosition,sPath,sDest,lFeedbackID,cReserved[32]\ns.Field:mArg,pGetValue,pSetValue,pWriteValue,sName,ulFieldID,uwOffset,uwIndex,ulFlags\nc.ALIGN:BOTTOM=0x20,LEFT=0x1,HORIZONTAL=0x4,TOP=0x10,MIDDLE=0xc,VERTICAL=0x8,CENTER=0xc,RIGHT=0x2\nc.JET:RIGHT_BUMPER_1=0x13,LEFT_BUMPER_2=0x12,ENTERED_SURFACE=0x21,DISPLAY_EDGE=0x27,TRIGGER_LEFT=0xd,DIGITAL_X=0x1,ANALOG_Y=0x16,WHEEL_TILT=0x1c,BUTTON_7=0x9,BUTTON_8=0xa,LMB=0x3,BUTTON_SELECT=0x10,MMB=0x5,RIGHT_BUMPER_2=0x14,ANALOG2_Y=0x19,WHEEL=0x1b,BUTTON_9=0xb,DEVICE_TILT_Z=0x26,BUTTON_2=0x4,ANALOG_Z=0x17,LEFT_BUMPER_1=0x11,ANALOG2_X=0x18,END=0x28,BUTTON_10=0xc,DEVICE_TILT_Y=0x25,BUTTON_4=0x6,BUTTON_START=0xf,DEVICE_TILT_X=0x24,BUTTON_1=0x3,PRESSURE=0x23,LEFT_SURFACE=0x22,ABS_Y=0x20,ABS_X=0x1f,RMB=0x4,ANALOG_X=0x15,BUTTON_5=0x7,PEN_TILT_HORIZONTAL=0x1e,PEN_TILT_VERTICAL=0x1d,DIGITAL_Y=0x2,BUTTON_3=0x5,BUTTON_6=0x8,TRIGGER_RIGHT=0xe,ANALOG2_Z=0x1a\nc.FDB:DECOMPRESS_OBJECT=0x4,REMOVE_FILE=0x3,DECOMPRESS_FILE=0x1,COMPRESS_FILE=0x2\nc.FOF:INCLUDE_SHARED=0x2,SMART_NAMES=0x1\nc.VLF:ERROR=0x2,WARNING=0x4,DEBUG=0x80,EXTAPI=0x40,BRANCH=0x1,INFO=0x10,API=0x20,TRACE=0x100,CRITICAL=0x8,FUNCTION=0x200\nc.TSTATE:RUNNING=0x0,PAUSED=0x1,TERMINATED=0x3,STOPPING=0x2\nc.STF:SCAN_SELECTION=0x4,BACKWARDS=0x8,MOVE_CURSOR=0x2,WRAP=0x20,EXPRESSION=0x10,CASE=0x1\nc.MOF:LINK_LIBRARY=0x1,STATIC=0x2,SYSTEM_PROBE=0x4\nc.NETMSG:END=0x1,START=0x0\nc.EDGE:LEFT=0x2,TOP_RIGHT=0x20,BOTTOM_LEFT=0x40,RIGHT=0x4,BOTTOM_RIGHT=0x80,BOTTOM=0x8,TOP=0x1,TOP_LEFT=0x10,ALL=0xff\nc.SEEK:START=0x0,END=0x2,CURRENT=0x1\nc.MHF:STRUCTURE=0x2,DEFAULT=0x2,NULL=0x1,STATIC=0x4\nc.CNF:STRIP_QUOTES=0x1,OPTIONAL_FILES=0x4,NEW=0x8,AUTO_SAVE=0x2\nc.EVG:CLASS=0xb,IO=0x6,DISPLAY=0x5,ANDROID=0xd,NETWORK=0x2,AUDIO=0x8,END=0xe,HARDWARE=0x7,POWER=0xa,USER=0x9,FILESYSTEM=0x1,APP=0xc,GUI=0x4,SYSTEM=0x3\nc.MTF:X=0x1,Y=0x2,RELATIVE=0x10,Z=0x4,ANIM=0x8\nc.SCF:DEBUG=0x2,EXIT_ON_ERROR=0x1\nc.LTF:CURRENT_PROCESS=0x1\nc.KQ:CONTROL=0x18,ALTGR=0x40,L_COMMAND=0x80,L_CTRL=0x8,INSTRUCTION_KEYS=0x78,R_ALT=0x40,RELEASED=0x800,QUALIFIERS=0x1fb,L_CONTROL=0x8,L_ALT=0x20,WIN_CONTROL=0x20000,NUM_PAD=0x200,INFO=0x3c04,R_CTRL=0x10,CTRL=0x18,DEAD_KEY=0x10000,CAPS_LOCK=0x4,NUM_LOCK=0x8000,R_COMMAND=0x100,ALT=0x60,L_SHIFT=0x1,PRESSED=0x1000,COMMAND=0x180,REPEAT=0x400,SHIFT=0x3,NOT_PRINTABLE=0x2000,SCR_LOCK=0x4000,R_CONTROL=0x10,R_SHIFT=0x2\nc.SBF:NO_DUPLICATES=0x1,SORT=0x2,CSV=0x10,DESC=0x8,CASE=0x4\nc.STT:STRING=0x4,FLOAT=0x2,NUMBER=0x1,HEX=0x3\nc.FFR:ABORT=0x2,SKIP=0x1,CONTINUE=0x0,OKAY=0x0\nc.IDTYPE:FUNCTION=0x3,GLOBAL=0x2,MESSAGE=0x1\nc.THF:AUTO_FREE=0x1,MSG_HANDLER=0x2\nc.DEVICE:TEMPORARY=0x800,MEMORY=0x1000,REMOVABLE=0x20,SCANNER=0x400,MODEM=0x2000,WRITE=0x10,PRINTER_3D=0x8000,READ=0x8,COMPACT_DISC=0x1,PRINTER=0x200,TAPE=0x100,SCANNER_3D=0x10000,NETWORK=0x80,HARD_DISK=0x2,USB=0x4000,REMOVEABLE=0x20,SOFTWARE=0x40,FLOPPY_DISK=0x4\nc.LAYOUT:LEFT=0x2,FOREGROUND=0x10,TIGHT=0x1,IGNORE_CURSOR=0x80,RIGHT=0x4,LOCK=0x40,TILE=0x100,SQUARE=0x0,BACKGROUND=0x8,WIDE=0x6,EMBEDDED=0x20\nc.CLIPMODE:COPY=0x2,PASTE=0x4,CUT=0x1\nc.MOVE:LEFT=0x4,ALL=0xf,DOWN=0x1,UP=0x2,RIGHT=0x8\nc.DATA:RAW=0x2,CONTENT=0xb,AUDIO=0x5,INPUT_READY=0xc,IMAGE=0x7,REQUEST=0x8,RECEIPT=0x9,FILE=0xa,XML=0x4,RECORD=0x6,DEVICE_INPUT=0x3,TEXT=0x1\nc.AST:Path=0x1,FLAGS=0x3,NAME=0x2,Icon=0x4,Comment=0x5,Flags=0x3,DEVICE_PATH=0x8,Name=0x2,ID=0x9,PATH=0x1,Label=0x6,DevicePath=0x8,LABEL=0x6,ICON=0x4,Device=0x7,DEVICE=0x7,COMMENT=0x5\nc.SEF:KEEP_ESCAPE=0x4,NO_SCRIPT=0x8,IGNORE_QUOTES=0x2,STRICT=0x1\nc.TOI:LOCAL_CACHE=0x0,ANDROID_CLASS=0x3,ANDROID_ENV=0x2,LOCAL_STORAGE=0x1,ANDROID_ASSETMGR=0x4\nc.MSF:ADDRESS=0x10,MESSAGE_ID=0x20,WAIT=0x1,ADD=0x8,NO_DUPLICATE=0x4,UPDATE=0x2\nc.STR:WILDCARD=0x4,MATCH_LEN=0x2,MATCH_CASE=0x1,CASE=0x1\nc.COPY:ALL=0x7fffffff\nc.NSF:LOCAL=0x2,DELAY=0x8,FORCE_DELAY=0x8,OTHER_TASKS=0x4,EXCLUSIVE=0x1,LOCAL_TASK=0x2\nc.RDF:LINK=0x40,ARCHIVE=0x2000,QUALIFIED=0x200,QUALIFY=0x200,OPENDIR=0x4000,VIRTUAL=0x400,READ_ALL=0x1f,READ_ONLY=0x1000,SIZE=0x1,PERMISSIONS=0x4,FILES=0x8,STREAM=0x800,DATE=0x2,FOLDER=0x10,VOLUME=0x20,HIDDEN=0x100,FILE=0x8,TAGS=0x80,TIME=0x2,FOLDERS=0x10\nc.DRL:LEFT=0x3,SOUTH_EAST=0x6,NORTH=0x0,RIGHT=0x2,NORTH_EAST=0x4,DOWN=0x1,SOUTH=0x1,EAST=0x2,WEST=0x3,NORTH_WEST=0x5,UP=0x0,SOUTH_WEST=0x7\nc.FD:PRIVATE=0x10000,SYSTEM=0x10000,TAGS=0x400,LONG=0x40000000,PTRSIZE=0x80,STRING=0x800000,VARTAGS=0x40,FLOAT=0x10000000,RESOURCE=0x2000,DOUBLE=0x80000000,BUFFER=0x200,FUNCTION=0x2000000,ERROR=0x800,BUFSIZE=0x80,BYTE=0x1000000,INIT=0x400,RI=0x500,VOID=0x0,PTR_LONGRESULT=0x48000100,I=0x400,PTR_LARGERESULT=0xc000100,PTRBUFFER=0x8000200,SYNONYM=0x20000,RW=0x300,W=0x200,R=0x100,DOUBLERESULT=0x80000100,FUNCTIONPTR=0xa000000,STR=0x800000,FLAGS=0x40,LONGRESULT=0x40000100,PTRRESULT=0x8000100,STRRESULT=0x800100,WORD=0x400000,OBJECT=0x1,VIRTUAL=0x8,ARRAYSIZE=0x80,INTEGRAL=0x2,OBJECTID=0x40000001,UNSIGNED=0x40000,LARGERESULT=0x4000100,PTR=0x8000000,CUSTOM=0x8000,OBJECTPTR=0x8000001,READ=0x100,RGB=0x80000,PTR_DOUBLERESULT=0x88000100,REQUIRED=0x4,STRUCT=0x10,ARRAY=0x1000,PERCENTAGE=0x200000,LARGE=0x4000000,VARIABLE=0x20000000,RESULT=0x100,ALLOC=0x20,VOLATILE=0x0,LOOKUP=0x80,POINTER=0x8000000,WRITE=0x200\nc.LOC:FOLDER=0x1,DIRECTORY=0x1,FILE=0x3,VOLUME=0x2\nc.ERF:Notified=0x40000000,Delay=0x20000000\nc.ERR:ResolvePath=0x64,WrongVersion=0x13,Failed=0xd,CoreVersion=0x26,NoMethods=0x49,NoAction=0x1b,OwnerNeedsBitmap=0x25,SystemCall=0x6b,Terminate=0x9,NegativeSubClassID=0x28,LockRequired=0x9b,GetSurfaceInfo=0x7d,NullArgs=0x8d,Redimension=0x71,MissingPath=0x4c,Query=0x2e,ExecViolation=0x8f,Continue=0x5,Resize=0x70,InUse=0xc,File=0xe,HostUnreachable=0x85,ExamineFailed=0x19,SystemCorrupt=0x23,UnsupportedField=0x5d,Refresh=0x69,VirtualVolume=0x58,False=0x1,NoPermission=0x22,Memory=0x1d,Disconnected=0x86,LockFailed=0x18,True=0x0,CreateObject=0x65,ObjectExists=0x6e,AccessSemaphore=0x73,NotLocked=0x4d,Init=0x21,AllocSemaphore=0x72,IllegalMethodID=0x36,DoNotExpunge=0x30,UnsupportedOwner=0x52,AccessObject=0x53,NoStats=0x1f,ExpectedFile=0x6f,WrongType=0xa8,ResourceExists=0x68,Loop=0x62,FileDoesNotExist=0x12,InitModule=0x11,OpenFile=0x76,EmptyString=0x6d,ServiceUnavailable=0x99,Recursion=0x90,IllegalActionAttempt=0x39,Lock=0x18,Finished=0x7e,SmallMask=0x6c,LostOwner=0x2f,ModuleMissingName=0x40,NoData=0x15,GetField=0x56,GlobalInstanceLocked=0xa4,Retry=0x7,ModuleInitFailed=0x3c,AlreadyLocked=0x9c,Args=0x14,UnrecognisedFieldType=0x5b,CardReaderUnavailable=0x9f,Exception=0xa3,InvalidData=0xf,ResolveSymbol=0xb4,LockMutex=0xaa,SystemLocked=0x3f,ObtainMethod=0x2c,CreateResource=0xb2,Obsolete=0xb1,OutOfSpace=0x7c,ExpectedFolder=0xae,NoSupport=0x1c,NetworkUnreachable=0x84,ObjectCorrupt=0x50,MarkedForDeletion=0x35,OutOfBounds=0x5f,NeedWidthHeight=0x27,InputOutput=0x94,Immutable=0xaf,Compression=0xad,ConnectionRefused=0x83,Decompression=0xac,ModuleOpenFailed=0x38,NotInitialised=0x67,EOF=0x7e,AccessMemory=0x4b,ReallocMemory=0x61,AllocMutex=0xa9,AllocMemory=0x54,WrongObjectType=0x8e,Read=0x16,Busy=0x8b,FieldTypeMismatch=0x5a,Permissions=0x22,OutsideMainThread=0xa6,Exists=0x7a,Deactivated=0x9a,OpenGL=0xa5,InvalidReference=0xa2,InvalidHTTPResponse=0xa1,ProxySSLTunnel=0xa0,LostClass=0x1a,NothingDone=0x4,DeadLock=0x3e,NoMediaInserted=0x9e,CardReaderUnknown=0x9d,Locked=0x9c,BadOwner=0x52,InvalidValue=0x98,Skip=0x6,WouldBlock=0x93,Seek=0x60,InvalidHandle=0x96,Write=0x17,LoadModule=0x95,IdenticalPaths=0x79,InvalidDimension=0x59,Security=0x97,NoFieldAccess=0x57,DataSize=0x8a,ArrayFull=0x2d,UnbalancedXML=0x92,NewObject=0x55,IllegalAddress=0x91,EndOfFile=0x7e,MissingClassName=0x2a,WrongClass=0x8e,InvalidObject=0x8e,ExceptionThreshold=0x9,FieldNotSet=0x44,StatementUnsatisfied=0x4f,InvalidURI=0x82,HostNotFound=0x81,SchemaViolation=0x89,FileNotFound=0x12,IntegrityViolation=0x88,TaskStillExists=0x87,ConnectionAborted=0x8c,AddClass=0x41,BadState=0x80,LowCapacity=0x20,Activate=0x42,StringFormat=0x7f,Syntax=0x7f,OutOfData=0x7e,SanityFailure=0x7b,DoesNotExist=0x78,ModuleMissingInit=0x3b,DoubleInit=0x43,NoMatchingObject=0x4a,Search=0x10,ListChildren=0x6a,EntryMissingHeader=0x3a,OwnerPassThrough=0x51,FileWriteFlag=0x47,FileReadFlag=0x46,ReadFileToBuffer=0xb0,NoSearchResult=0x4e,FieldSearch=0x32,InvalidPath=0x33,MemoryCorrupt=0x31,NoPointer=0xb,OutOfRange=0x2b,MissingClass=0x45,DeleteFile=0x75,MemoryDoesNotExist=0x3d,DirEmpty=0x8,NoMemory=0xa,ConstraintViolation=0x88,Mismatch=0x5e,TimeOut=0x1e,ReadOnly=0x77,SetVolume=0xab,PermissionDenied=0x22,Okay=0x0,LimitedSuccess=0x2,MemoryInfo=0x66,ExclusiveDenied=0x53,Draw=0x48,UseSubClass=0xa7,BufferOverflow=0x5c,END=0xb5,NotPossible=0xb3,IllegalActionID=0x37,NotFound=0x10,SetField=0x34,NeedOwner=0x24,FileExists=0x63,CreateFile=0x74,Cancelled=0x3,NegativeClassID=0x29\nc.K:LENS_FOCUS=0x8c,S=0x13,FORWARD=0x90,F5=0x50,F11=0x56,R_ALT=0x49,EQUALS=0x27,F7=0x52,U=0x15,R_CONTROL=0x42,F16=0x5b,PAGE_UP=0x70,Y=0x19,BACK_SLASH=0x2f,F9=0x54,THREE=0x1d,VOLUME_DOWN=0x85,STAR=0x93,BACK=0x86,NEXT=0x8e,NP_ENTER=0x7e,DELETE=0x6d,CAMERA=0x89,REWIND=0x91,ZERO=0x24,NP_DOT=0x3f,B=0x2,INSERT=0x75,F15=0x5a,NP_8=0x39,SCR_LOCK=0x64,NP_2=0x33,NP_0=0x31,SELECT=0x73,H=0x8,F=0x6,PERIOD=0x2d,NP_6=0x37,NP_4=0x35,NP_DECIMAL=0x3f,L=0xc,L_ALT=0x48,COMMA=0x2c,F20=0x82,FIVE=0x1f,R_COMMAND=0x4b,SPACE=0x30,F18=0x80,CALL=0x87,F2=0x4d,F4=0x4f,SYSRQ=0x7f,NP_MULTIPLY=0x3b,R=0x12,F19=0x81,F6=0x51,T=0x14,DOT=0x2d,UP=0x60,REDO=0x77,NP_PLUS_MINUS=0x5e,CANCEL=0x7a,REVERSE_QUOTE=0x25,X=0x18,ENTER=0x6b,V=0x16,PRT_SCR=0x7d,VOLUME_UP=0x84,L_COMMAND=0x4a,R_SQUARE=0x29,Z=0x1a,AT=0x8a,LESS_GREATER=0x5f,NP_DIVIDE=0x40,EXECUTE=0x74,L_SQUARE=0x28,PAUSE=0x65,BACKSPACE=0x69,RIGHT=0x62,A=0x1,PLAY=0x95,Q=0x11,TAB=0x6a,F17=0x5c,SEVEN=0x21,SLEEP=0x67,PLUS=0x8b,C=0x3,MUTE=0x92,MACRO=0x5d,PREVIOUS=0x8f,F14=0x59,F13=0x58,R_SHIFT=0x45,HOME=0x6f,K=0xb,E=0x5,D=0x4,STOP=0x8d,MINUS=0x26,BREAK=0x7b,F1=0x4c,NP_3=0x34,NP_9=0x3a,G=0x7,POWER=0x68,L_CONTROL=0x41,NP_1=0x32,CAPS_LOCK=0x46,NUM_LOCK=0x7c,UNDO=0x76,END_CALL=0x88,L_SHIFT=0x44,I=0x9,CLEAR=0x6e,POUND=0x94,APOSTROPHE=0x2b,NP_5=0x36,N=0xe,MENU=0x78,NP_7=0x38,SIX=0x20,LEFT=0x63,SEMI_COLON=0x2a,P=0x10,F8=0x53,NP_PLUS=0x3c,PAGE_DOWN=0x71,ESCAPE=0x6c,LIST_END=0x96,M=0xd,ONE=0x1b,SLASH=0x2e,HELP=0x43,WIN_CONTROL=0x83,WAKE=0x66,NP_MINUS=0x3e,NP_SEPARATOR=0x3d,O=0xf,PRINT=0x47,NP_BAR=0x3d,TWO=0x1c,EIGHT=0x22,END=0x72,F10=0x55,FIND=0x79,J=0xa,FOUR=0x1e,F12=0x57,DOWN=0x61,F3=0x4e,W=0x17,NINE=0x23\nc.KSF:THREAD_SAFE=0x2,UNTRACKED=0x4,INTERNAL=0x10,CASE=0x1,AUTO_REMOVE=0x8\nc.RFD:REMOVE=0x8,ALLOW_RECURSION=0x20,ALWAYS_CALL=0x100,WRITE=0x1,SOCKET=0x40,READ=0x4,EXCEPT=0x2,STOP_RECURSE=0x10,RECALL=0x80\nc.CLF:XML_CONTENT=0x10,SHARED_ONLY=0x1,PUBLIC_OBJECTS=0x8,SHARED_OBJECTS=0x1,PROMOTE_INTEGRAL=0x4,NO_OWNERSHIP=0x20,PRIVATE_ONLY=0x2\nc.RP:SYSTEM_PATH=0x2,MODULE_PATH=0x1,ROOT_PATH=0x3\nc.CF:ZLIB=0x2,DEFLATE=0x3,GZIP=0x1\nc.CMF:READ_ONLY=0x8,NEW=0x2,APPLY_SECURITY=0x20,NO_LINKS=0x10,CREATE_FILE=0x4,PASSWORD=0x1\nc.PERMIT:USER_EXEC=0x4,GROUP_WRITE=0x20,GROUP=0xf0,GROUP_DELETE=0x80,ALL_EXEC=0x444,OTHERS_EXEC=0x400,OTHERS=0xf00,ALL_WRITE=0x222,OTHERS_READ=0x100,OTHERS_DELETE=0x800,EVERYONE_EXEC=0x444,GROUP_READ=0x10,GROUP_EXEC=0x40,ARCHIVE=0x2000,OTHERS_WRITE=0x200,WRITE=0x2,USER=0xf,READ=0x1,OFFLINE=0x40000,INHERIT=0x20000,PASSWORD=0x4000,DELETE=0x8,ALL_READ=0x111,EVERYONE_READ=0x111,NETWORK=0x80000,EVERYONE_READWRITE=0x333,EVERYONE_WRITE=0x222,USER_WRITE=0x2,EVERYONE_DELETE=0x888,EVERYONE_ACCESS=0xfff,GROUPID=0x10000,HIDDEN=0x1000,EXEC=0x4,ALL_DELETE=0x888,USERID=0x8000,USER_READ=0x1\nc.NF:HAS_SHARED_RESOURCES=0x4000,NAME=0x80000000,INTEGRAL=0x10,UNTRACKED=0x1,NO_TRACK=0x1,FREE_MARK=0x200,UNIQUE=0x40000000,TIMER_SUB=0x80,PUBLIC=0x2,RECLASSED=0x800,SHARED=0x2,INITIALISED=0x8,PRIVATE=0x0,SIGNALLED=0x2000,UNLOCK_FREE=0x20,MESSAGE=0x1000,CREATE_OBJECT=0x100,FOREIGN_OWNER=0x4,FREE=0x40,NEW_OBJECT=0x400\nc.RES:CORE_IDL=0x1f,EXCEPTION_HANDLER=0x15,KEY_STATE=0x1e,FREE_SWAP=0x1d,CONSOLE_FD=0x2,PRIVILEGED=0x8,PRIVILEGED_USER=0x7,LOG_DEPTH=0x10,TOTAL_MEMORY=0x18,SHARED_BLOCKS=0x1b,FREE_MEMORY=0x1c,CPU_SPEED=0x1a,GLOBAL_INSTANCE=0x3,PARENT_CONTEXT=0xa,TOTAL_SWAP=0x19,NET_PROCESSING=0x16,USER_ID=0x5,PROCESS_STATE=0x17,DISPLAY_DRIVER=0x6,MAX_PROCESSES=0xf,OPEN_INFO=0x14,SHARED_CONTROL=0x4,TASK_LIST=0xe,MESSAGE_QUEUE=0x1,TASK_CONTROL=0xd,THREAD_ID=0x12,CURRENT_MSG=0x13,RANDOM_SEED=0x9,TOTAL_SHARED_MEMORY=0xc,JNI_ENV=0x11,LOG_LEVEL=0xb\nc.VAS:WATCH_PATH=0x8,IDENTIFY_FILE=0xc,CASE_SENSITIVE=0xf,MAKE_DIR=0xd,RENAME=0x4,CLOSE_DIR=0x6,DEREGISTER=0x1,DELETE=0x3,GET_INFO=0xa,SCAN_DIR=0x2,READ_LINK=0x10,OPEN_DIR=0x5,CREATE_LINK=0x11,TEST_PATH=0x7,SAME_FILE=0xe,GET_DEVICE_INFO=0xb,IGNORE_FILE=0x9\nc.MSGID:SET_FIELD=0x62,COMMAND=0x65,DEBUG=0x5f,GET_FIELD=0x61,ACTION=0x63,CORE_END=0x64,EXPOSE=0x64,WAIT_FOR_OBJECTS=0x5a,ACTION_RESULT=0x60,VALIDATE_PROCESS=0x5d,EVENT=0x5e,BREAK=0x66,QUIT=0x3e8,THREAD_CALLBACK=0x5c,THREAD_ACTION=0x5b\nc.JTYPE:DRAG_ITEM=0x800,DRAGGED=0x4,SECONDARY=0x1,DIGITAL=0x10,MOVEMENT=0x100,ANALOG=0x20,EXT_MOVEMENT=0x40,ANCHORED=0x2,REPEATED=0x400,DBL_CLICK=0x200,FEEDBACK=0x8,BUTTON=0x80\nc.MAX:NAME_LEN=0x20\nc.SUB:WARN_EXISTS=0x7fffffff,FAIL_EXISTS=0x7ffffffe\nc.AC:Move=0x15,Draw=0xa,SaveImage=0x21,NewChild=0x18,Show=0x26,MoveToPoint=0x32,Free=0xd,Deactivate=0x9,Activate=0x2,ActionNotify=0x1,SaveToObject=0x22,Signal=0x38,DataFeed=0x8,Rename=0x1e,SelectArea=0x37,AccessObject=0x3,Sort=0x35,LostFocus=0x14,Focus=0xc,Refresh=0x2e,Read=0x1d,Enable=0x30,GetVar=0xf,MoveToFront=0x17,Clear=0x4,Disable=0x2f,CopyData=0x7,Query=0x1c,ReleaseObject=0xe,MoveToBack=0x16,Reset=0x1f,ScrollToPoint=0x33,Redimension=0x31,Custom=0x34,Resize=0x20,SaveSettings=0x36,Clipboard=0x2d,OwnerDestroyed=0x6,SetField=0x2c,Scroll=0x23,Redo=0x1b,Seek=0x24,Prev=0x2a,Write=0x2b,FreeWarning=0x5,Next=0x29,Hide=0x11,END=0x39,Undo=0x27,Flush=0xb,Unlock=0x28,DragDrop=0x10,NewObject=0x1a,Init=0x12,SetVar=0x25,NewOwner=0x19,Lock=0x13\nc.CCF:IO=0x40,SYSTEM=0x80,MISC=0x800,NETWORK=0x1000,AUDIO=0x200,EFFECT=0x4,DRAWABLE=0x2,MULTIMEDIA=0x2000,DATA=0x400,TOOL=0x100,FILESYSTEM=0x8,COMMAND=0x1,GUI=0x20,GRAPHICS=0x10\nc.STP:Y=0x2,X=0x1,Z=0x4,ANIM=0x8\nc.IDF:SECTION=0x1,IGNORE_HOST=0x4,HOST=0x2\nc.VOLUME:SYSTEM=0x10,HIDDEN=0x4,PRIORITY=0x2,SAVE=0x8,REPLACE=0x1\nc.MEM:NO_BLOCK=0x2000,FIXED=0x4000,READ_WRITE=0x30000,OBJECT=0x200,WRITE=0x20000,NO_BLOCKING=0x2000,CALLER=0x800000,UNTRACKED=0x80,MANAGED=0x8000,NO_LOCK=0x400,TASK=0x200000,DELETE=0x1000,SHARED=0x1,RESERVED=0x80000,DATA=0x0,PUBLIC=0x1,AUDIO=0x8,EXCLUSIVE=0x800,NO_POOL=0x20,READ=0x10000,NO_CLEAR=0x40000,CODE=0x10,HIDDEN=0x100000,TMP_LOCK=0x40,STRING=0x100,TEXTURE=0x4,VIDEO=0x2\nc.ALF:SHARED=0x1,RECURSIVE=0x2\nc.SMF:NON_BLOCKING=0x1,NO_BLOCKING=0x1,EXISTS=0x2\nc.RSF:PATH=0x10,NO_FILE_CHECK=0x1,APPROXIMATE=0x4,CHECK_VIRTUAL=0x2,CASE_SENSITIVE=0x20,NO_DEEP_SCAN=0x8\nc.FDL:FEEDBACK=0x1\nc.MFF:MOVED=0x10,UNMOUNT=0x100,ATTRIB=0x20,DEEP=0x1000,CLOSED=0x80,FILE=0x400,FOLDER=0x200,WRITE=0x2,SELF=0x800,OPENED=0x40,RENAME=0x10,READ=0x1,CREATE=0x4,DELETE=0x8,MODIFY=0x2\nc.PMF:SYSTEM_NO_BREAK=0x1\nc.OPF:DATE=0x4,MAX_DEPTH=0x40,PRIVILEGED=0x4000,SYSTEM_PATH=0x8000,SCAN_MODULES=0x40000,JUMPTABLE=0x20,SHOW_MEMORY=0x100,SHOW_PUBLIC_MEM=0x200000,GLOBAL_INSTANCE=0x80000,CORE_VERSION=0x10,AUTHOR=0x8,COPYRIGHT=0x2,DETAIL=0x80,NAME=0x1,ROOT_PATH=0x20000,COMPILED_AGAINST=0x2000,ARGS=0x800,MODULE_PATH=0x10000,OPTIONS=0x100000,SHOW_IO=0x200,SHOW_ERRORS=0x400,ERROR=0x1000\nc.ACF:ALL_USERS=0x1\nc.TSF:PIPE=0x400,DUMMY=0x2,SHELL=0x20,DEBUG=0x40,WAIT=0x4,QUIET=0x80,RESET_PATH=0x8,FOREIGN=0x1,ATTACHED=0x200,DETACHED=0x100,PRIVILEGED=0x10\nc.FDT:MODIFIED=0x0,ARCHIVED=0x3,ACCESSED=0x2,CREATED=0x1\nc.DMF:RELATIVE_HEIGHT=0x400,FIXED_X=0x4,RELATIVE_CENTER_Y=0x80000,Y_OFFSET=0xa0,FIXED_RADIUS_Y=0x2000000,RELATIVE_X=0x1,X_OFFSET=0x50,RELATIVE_CENTER_X=0x40000,RELATIVE_WIDTH=0x800,HORIZONTAL_FLAGS=0xa55,WIDTH=0xa00,X=0x5,HEIGHT_FLAGS=0x5a0,RELATIVE_X_OFFSET=0x10,STATUS_CHANGE_H=0x400000,RELATIVE_DEPTH=0x2000,RELATIVE_RADIUS_Y=0x1000000,FIXED_Z=0x4000,STATUS_CHANGE=0xc00000,FIXED_DEPTH=0x1000,VERTICAL_FLAGS=0x5aa,Y=0xa,FIXED_CENTER_Y=0x200000,FIXED_RADIUS=0x2020000,RELATIVE_RADIUS=0x1010000,FIXED_Y_OFFSET=0x80,WIDTH_FLAGS=0xa50,FIXED_HEIGHT=0x100,HEIGHT=0x500,RELATIVE_Y_OFFSET=0x20,FIXED_X_OFFSET=0x40,STATUS_CHANGE_V=0x800000,FIXED_Y=0x8,RELATIVE_Y=0x2,FIXED_WIDTH=0x200,RELATIVE_Z=0x8000,FIXED_RADIUS_X=0x20000,RELATIVE_RADIUS_X=0x10000,FIXED_CENTER_X=0x100000\nc.LDF:CHECK_EXISTS=0x1\nc.FBK:MOVE_FILE=0x1,DELETE_FILE=0x3,COPY_FILE=0x2\nc.SEM:GET_VAL=0x1,GET_DATA_DOUBLE=0x6,GET_DATA_LARGE=0x5,SET_DATA_PTR=0x7,SET_DATA_LONG=0x8,SET_DATA_LARGE=0x9,GET_DATA_PTR=0x3,GET_DATA_LONG=0x4,SET_DATA_DOUBLE=0xa,GET_COUNTER=0x2\nc.FL:LINK=0x20,EXCLUDE_FOLDERS=0x2000,DEVICE=0x400,APPROXIMATE=0x10,EXCLUDE_FILES=0x1000,RESET_DATE=0x200,STREAM=0x800,WRITE=0x1,DIRECTORY=0x8,NEW=0x2,FILE=0x100,READ=0x4,BUFFER=0x40,FOLDER=0x8,LOOP=0x80\nc.PTR:SIZING=0xc,CROSSHAIR=0xa,END=0x19,DEFAULT=0x1,CUSTOM=0x17,PAINTBRUSH=0x14,SIZE_LEFT=0x6,SIZE_BOTTOM_LEFT=0x2,SPLIT_VERTICAL=0xd,TEXT=0x13,HAND_RIGHT=0x12,SIZE_TOP_LEFT=0x4,SLEEP=0xb,SIZE_TOP_RIGHT=0x5,NO_CHANGE=0x0,MAGNIFIER=0xf,INVISIBLE=0x16,SPLIT_HORIZONTAL=0xe,SIZE_TOP=0x8,STOP=0x15,HAND_LEFT=0x11,SIZE_RIGHT=0x7,SIZE_BOTTOM_RIGHT=0x3,DRAGGABLE=0x18,HAND=0x10,SIZE_BOTTOM=0x9\n"
