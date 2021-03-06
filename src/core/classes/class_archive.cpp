/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
FileArchive: Creates simple read-only volumes backed by compressed archives.

The FileArchive class makes it possible to create virtual file system volumes that are based on compressed file
archives.  It is not necessary for client programs to instantiate a FileArchive to make use of this functionality.
Instead, create a @Compression object that declares a Path to the source archive file and set an
ArchiveName for reference.  In the example below, also take note of the use of NF_UNTRACKED to prevent the
@Compression object from being automatically collected when it goes out of scope:

<pre>
CreateObject(ID_COMPRESSION, NF_UNTRACKED, &archive,
   FID_Path|TSTR,        "user:documents/myfile.zip",
   FID_ArchiveName|TSTR, "myfiles",
   TAGEND);
</pre>

With the Compression object in place, opening files within the archive is as simple as using the correct path
reference.  The format is `archive:ArchiveName/path/to/file.ext` and the example below illustrates:

`obj.new('file', { path='archive:myfiles/readme.txt', flags='!READ' })`

-END-

*****************************************************************************/

#include "zlib.h"

#ifndef __system__
#define __system__
#endif

#define PRV_FILE
#define PRV_COMPRESSION

#include "../defs.h"
#include <parasol/main.h>

using namespace parasol;

#define LEN_ARCHIVE 8 // "archive:" length

struct prvFileArchive {
   ZipFile  Info;
   z_stream Stream;
   objFile  *FileStream;
   objCompression *Archive;
   UBYTE    InputBuffer[SIZE_COMPRESSION_BUFFER];
   UBYTE    OutputBuffer[SIZE_COMPRESSION_BUFFER];
   UBYTE    *ReadPtr;      // Current position within OutputBuffer
   LONG     InputLength;
   bool     Inflating;
};

static std::unordered_map<ULONG, objCompression *> glArchives;

static ERROR close_folder(DirInfo *);
static ERROR open_folder(DirInfo *);
static ERROR get_info(CSTRING, FileInfo *, LONG);
static ERROR scan_folder(DirInfo *);
static ERROR test_path(STRING, LONG, LONG *);

//****************************************************************************
// Return the portion of the string that follows the last discovered '/' or '\'

INLINE CSTRING name_from_path(CSTRING Path)
{
   for (LONG i=0; Path[i]; i++) {
      if ((Path[i] IS '/') or (Path[i] IS '\\')) {
         Path = Path + i + 1;
         i = -1;
      }
   }
   return Path;
}

//****************************************************************************

static void reset_state(objFile *Self)
{
   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;

   if (prv->Inflating) { inflateEnd(&prv->Stream); prv->Inflating = false; }

   prv->Stream.avail_in = 0;
   prv->ReadPtr = NULL;
   Self->Position = 0;
}

//****************************************************************************

static ERROR seek_to_item(objFile *Self)
{
   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;
   ZipFile *item = &prv->Info;

   acSeekStart(prv->FileStream, item->Offset + HEAD_EXTRALEN);
   prv->ReadPtr = NULL;

   UWORD extra_len = read_word(prv->FileStream);
   ULONG stream_start = item->Offset + HEAD_LENGTH + item->NameLen + extra_len;
   if (acSeekStart(prv->FileStream, stream_start) != ERR_Okay) return ERR_Seek;

   if (item->CompressedSize > 0) {
      Self->Flags |= FL_FILE;

      if (item->DeflateMethod IS 0) { // The file is stored rather than compressed
         Self->Size = item->CompressedSize;
         return ERR_Okay;
      }
      else if ((item->DeflateMethod IS 8) and (!inflateInit2(&prv->Stream, -MAX_WBITS))) {
         prv->Inflating = true;
         Self->Size = item->OriginalSize;
         return ERR_Okay;
      }
      else return ERR_Failed;
   }
   else { // Folder or empty file
      if (item->IsFolder) Self->Flags |= FL_FOLDER;
      else Self->Flags |= FL_FILE;
      Self->Size = 0;
      return ERR_Okay;
   }
}

//****************************************************************************
// Insert a new compression object as an archive.

extern void add_archive(objCompression *Compression)
{
   glArchives[Compression->ArchiveHash] = Compression;
}

//****************************************************************************

extern void remove_archive(objCompression *Compression)
{
   glArchives.erase(Compression->ArchiveHash);
}

//****************************************************************************
// Return the archive referenced by 'archive:[NAME]/...'

extern objCompression * find_archive(CSTRING Path, std::string &FilePath)
{
   parasol::Log log(__FUNCTION__);

   if (!Path) return NULL;

   // Compute the hash of the referenced archive.

   CSTRING path = Path + LEN_ARCHIVE;
   ULONG hash = 5381;
   char c;
   while ((c = *path++) and (c != '/') and (c != '\\')) {
      if ((c >= 'A') and (c <= 'Z')) hash = (hash<<5) + hash + c - 'A' + 'a';
      else hash = (hash<<5) + hash + c;
   }

   if (glArchives.contains(hash)) {
      FilePath.assign(path);
      return glArchives[hash];
   }
   else {
      log.warning("No match for path '%s'", Path);
      return NULL;
   }
}

//****************************************************************************

static ERROR ARCHIVE_Activate(objFile *Self, APTR Void)
{
   parasol::Log log;

   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;

   if (!prv->Archive) return log.warning(ERR_SystemCorrupt);

   if (prv->FileStream) return ERR_Okay; // Already activated

   log.msg("Allocating file stream for item %s", prv->Info.Name);

   if (!CreateObject(ID_FILE, NF_INTEGRAL, (OBJECTPTR *)&prv->FileStream,
         FID_Name|TSTR,   "ArchiveFileStream",
         FID_Path|TSTR,   prv->Archive->Path,
         FID_Flags|TLONG, FL_READ,
         TAGEND)) {

      ERROR error = seek_to_item(Self);
      if (error) log.warning(error);
      return error;
   }
   else return ERR_File;
}

//****************************************************************************

static ERROR ARCHIVE_Free(objFile *Self, APTR Void)
{
   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;

   if (prv) {
      if (prv->FileStream) { acFree(&prv->FileStream->Head); prv->FileStream = NULL; }
      if (prv->Inflating)  { inflateEnd(&prv->Stream); prv->Inflating = false; }
   }

   return ERR_Okay;
}

//****************************************************************************

static ERROR ARCHIVE_Init(objFile *Self, APTR Void)
{
   parasol::Log log;

   if (!Self->Path) return ERR_FieldNotSet;

   if (StrCompare("archive:", Self->Path, LEN_ARCHIVE, 0) != ERR_Okay) return ERR_NoSupport;

   if (Self->Flags & (FL_NEW|FL_WRITE)) return log.warning(ERR_ReadOnly);

   ERROR error = ERR_Search;
   if (!AllocMemory(sizeof(prvFileArchive), Self->Head.MemFlags, &Self->Head.ChildPrivate, NULL)) {
      if (Self->Path[StrLength(Self->Path)-1] IS ':') { // Nothing is referenced
         return ERR_Okay;
      }
      else {
         std::string file_path;

         auto prv = (prvFileArchive *)(Self->Head.ChildPrivate);
         prv->Archive = find_archive(Self->Path, file_path);

         if (prv->Archive) {
            // TODO: This is a slow scan and could be improved if a hashed directory structure was
            // generated during add_archive() first; then we could perform a quick lookup here.

            ZipFile *item;
            for (item=prv->Archive->prvFiles; item; item=(ZipFile *)item->Next) {
               if (!StrCompare(file_path.c_str(), item->Name, 0, STR_CASE|STR_MATCH_LEN)) break;
            }

            if ((!item) and (Self->Flags & FL_APPROXIMATE)) {
               file_path.append(".*");
               for (item=prv->Archive->prvFiles; item; item=(ZipFile *)item->Next) {
                  if (!StrCompare(file_path.c_str(), item->Name, 0, STR_WILDCARD)) break;
               }
            }

            if (item) {
               ((prvFileArchive *)(Self->Head.ChildPrivate))->Info = *item;
               if (!(error = acActivate((OBJECTPTR)Self))) {
                  error = acQuery((OBJECTPTR)Self);
               }
            }
         }
      }
   }
   else error = ERR_AllocMemory;

   if (error) {
      if (Self->Head.ChildPrivate) { FreeResource(Self->Head.ChildPrivate); Self->Head.ChildPrivate = NULL; }
   }

   return error;
}

//****************************************************************************

static ERROR ARCHIVE_Query(objFile *Self, APTR Void)
{
   auto prv = (prvFileArchive *)(Self->Head.ChildPrivate);

   // Activate the source if this hasn't been done already.

   ERROR error;
   if (!prv->FileStream) {
      error = acActivate((OBJECTPTR)Self);
      if (error) return error;
   }

   // If security flags are present, convert them to file system permissions.

   ZipFile *item = &prv->Info;
   if (item->Flags & ZIP_SECURITY) {
      LONG permissions = 0;
      if (item->Flags & ZIP_UEXEC) permissions |= PERMIT_USER_EXEC;
      if (item->Flags & ZIP_GEXEC) permissions |= PERMIT_GROUP_EXEC;
      if (item->Flags & ZIP_OEXEC) permissions |= PERMIT_OTHERS_EXEC;

      if (item->Flags & ZIP_UREAD) permissions |= PERMIT_USER_READ;
      if (item->Flags & ZIP_GREAD) permissions |= PERMIT_GROUP_READ;
      if (item->Flags & ZIP_OREAD) permissions |= PERMIT_OTHERS_READ;

      if (item->Flags & ZIP_UWRITE) permissions |= PERMIT_USER_WRITE;
      if (item->Flags & ZIP_GWRITE) permissions |= PERMIT_GROUP_WRITE;
      if (item->Flags & ZIP_OWRITE) permissions |= PERMIT_OTHERS_WRITE;

      Self->Permissions = permissions;
   }

   return ERR_Okay;
}

//****************************************************************************

static ERROR ARCHIVE_Read(objFile *Self, struct acRead *Args)
{
   parasol::Log log;

   if ((!Args) or (!Args->Buffer)) return log.warning(ERR_NullArgs);
   else if (Args->Length == 0) return ERR_Okay;
   else if (Args->Length < 0) return ERR_OutOfRange;

   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;

   if (prv->Info.DeflateMethod IS 0) {
      ERROR error = acRead(prv->FileStream, Args->Buffer, Args->Length, &Args->Result);
      if (!error) Self->Position += Args->Result;
      return error;
   }
   else {
      Args->Result = 0;

      auto zf = &prv->Info;

      //log.trace("Decompressing %d bytes to %d, buffer size %d", zf->CompressedSize, zf->OriginalSize, Args->Length);

      if ((prv->Inflating) and (!prv->Stream.avail_in)) { // Initial setup
         struct acRead read = {
            .Buffer = prv->InputBuffer,
            .Length = (zf->CompressedSize < SIZE_COMPRESSION_BUFFER) ? (LONG)zf->CompressedSize : SIZE_COMPRESSION_BUFFER
         };

         if (Action(AC_Read, &prv->FileStream->Head, &read)) return ERR_Read;
         if (read.Result <= 0) return ERR_Read;

         prv->ReadPtr          = prv->OutputBuffer;
         prv->InputLength      = zf->CompressedSize - read.Result;
         prv->Stream.next_in   = prv->InputBuffer;
         prv->Stream.avail_in  = read.Result;
         prv->Stream.next_out  = prv->OutputBuffer;
         prv->Stream.avail_out = SIZE_COMPRESSION_BUFFER;
      }

      while (true) {
         // Output any buffered data to the client first
         if (prv->ReadPtr < (UBYTE *)prv->Stream.next_out) {
            LONG len = (LONG)(prv->Stream.next_out - (Bytef *)prv->ReadPtr);
            if (len > Args->Length) len = Args->Length;
            CopyMemory(prv->ReadPtr, (char *)Args->Buffer + Args->Result, len);
            prv->ReadPtr   += len;
            Args->Result   += len;
            Self->Position += len;
         }

         // Stop if necessary

         if (prv->Stream.total_out IS zf->OriginalSize) break; // All data decompressed
         if (Args->Result >= Args->Length) return ERR_Okay;
         if (!prv->Inflating) return ERR_Okay;

         // Reset the output buffer and decompress more data

         prv->Stream.next_out  = prv->OutputBuffer;
         prv->Stream.avail_out = SIZE_COMPRESSION_BUFFER;

         LONG result = inflate(&prv->Stream, (prv->Stream.avail_in) ? Z_SYNC_FLUSH : Z_FINISH);

         prv->ReadPtr = prv->OutputBuffer;

         if ((result) and (result != Z_STREAM_END)) {
            return convert_zip_error(&prv->Stream, result);
         }

         // Read more data from the source if necessary

         if ((prv->Stream.avail_in <= 0) and (prv->InputLength > 0) and (result != Z_STREAM_END)) {
            struct acRead read = { .Buffer = prv->InputBuffer };

            if (prv->InputLength < SIZE_COMPRESSION_BUFFER) read.Length = prv->InputLength;
            else read.Length = SIZE_COMPRESSION_BUFFER;

            if (Action(AC_Read, &prv->FileStream->Head, &read)) return ERR_Read;
            if (read.Result <= 0) return ERR_Read;

            prv->InputLength -= read.Result;
            prv->Stream.next_in  = prv->InputBuffer;
            prv->Stream.avail_in = read.Result;
         }
      }

      if (prv->Inflating) {
         inflateEnd(&prv->Stream);
         prv->Inflating = false;
      }

      return ERR_Okay;
   }
}

//****************************************************************************

static ERROR ARCHIVE_Seek(objFile *Self, struct acSeek *Args)
{
   parasol::Log log;
   LARGE pos;

   log.traceBranch("Seek to offset %.2f from seek position %d", Args->Offset, Args->Position);

   if (Args->Position IS SEEK_START) pos = F2T(Args->Offset);
   else if (Args->Position IS SEEK_END) pos = Self->Size - F2T(Args->Offset);
   else if (Args->Position IS SEEK_CURRENT) pos = Self->Position + F2T(Args->Offset);
   else return log.warning(ERR_Args);

   if (pos < 0) return log.warning(ERR_OutOfRange);

   if (pos < Self->Position) { // The position must be reset to zero if we need to backtrack
      reset_state(Self);

      ERROR error = seek_to_item(Self);
      if (error) return log.warning(error);
   }

   UBYTE buffer[2048];
   while (Self->Position < pos) {
      struct acRead read = { .Buffer = buffer, .Length = (LONG)(pos - Self->Position) };
      if ((size_t)read.Length > sizeof(buffer)) read.Length = sizeof(buffer);
      if (Action(AC_Read, (OBJECTPTR)Self, &read)) return ERR_Decompression;
   }

   return ERR_Okay;
}

//****************************************************************************

static ERROR ARCHIVE_Write(objFile *Self, struct acWrite *Args)
{
   parasol::Log log;
   return log.warning(ERR_NoSupport);
}

//****************************************************************************

static ERROR ARCHIVE_GET_Size(objFile *Self, LARGE *Value)
{
   auto prv = (prvFileArchive *)Self->Head.ChildPrivate;
   if (prv) {
      *Value = prv->Info.OriginalSize;
      return ERR_Okay;
   }
   else return ERR_NotInitialised;
}

//****************************************************************************
// Open the archive: volume for scanning.

static ERROR open_folder(DirInfo *Dir)
{
   std::string file_path;
   Dir->prvIndex = 0;
   Dir->prvTotal = 0;
   Dir->prvHandle = find_archive(Dir->prvResolvedPath, file_path);
   if (!Dir->prvHandle) return ERR_DoesNotExist;
   return ERR_Okay;
}

//****************************************************************************
// Scan the next entry in the folder.

static ERROR scan_folder(DirInfo *Dir)
{
   parasol::Log log(__FUNCTION__);

   // Retrieve the file path, skipping the "archive:name/" part.

   CSTRING path = Dir->prvResolvedPath + LEN_ARCHIVE + 1;
   while ((*path) and (*path != '/') and (*path != '\\')) path++;
   if ((*path IS '/') or (*path IS '\\')) path++;

   log.traceBranch("Path: \"%s\", Flags: $%.8x", path, Dir->prvFlags);

   auto archive = (objCompression *)Dir->prvHandle;

   ZipFile *zf = archive->prvFiles;
   if (Dir->prvIndexPtr) zf = (ZipFile *)Dir->prvIndexPtr;

   for (; zf; zf=(ZipFile *)zf->Next) {
      if (*path) {
         if (StrCompare(path, zf->Name, 0, 0) != ERR_Okay) continue;
      }

      // Single folders will appear as 'ABCDEF/'
      // Single files will appear as 'ABCDEF.ABC' (no slash)

      LONG name_len = StrLength(zf->Name);
      LONG path_len = StrLength(path);

      if (name_len <= path_len) continue;

      // Is this item in a sub-folder?  If so, ignore it.

      {
         LONG i;
         for (i=path_len; (zf->Name[i]) and (zf->Name[i] != '/') and (zf->Name[i] != '\\'); i++);
         if (zf->Name[i]) continue;
      }

      if ((Dir->prvFlags & RDF_FILE) and (!zf->IsFolder)) {

         if (Dir->prvFlags & RDF_PERMISSIONS) {
            Dir->Info->Flags |= RDF_PERMISSIONS;
            Dir->Info->Permissions = PERMIT_READ|PERMIT_GROUP_READ|PERMIT_OTHERS_READ;
         }

         if (Dir->prvFlags & RDF_SIZE) {
            Dir->Info->Flags |= RDF_SIZE;
            Dir->Info->Size = zf->OriginalSize;
         }

         if (Dir->prvFlags & RDF_DATE) {
            Dir->Info->Flags |= RDF_DATE;
            Dir->Info->Modified.Year   = zf->Year;
            Dir->Info->Modified.Month  = zf->Month;
            Dir->Info->Modified.Day    = zf->Day;
            Dir->Info->Modified.Hour   = zf->Hour;
            Dir->Info->Modified.Minute = zf->Minute;
            Dir->Info->Modified.Second = 0;
         }

         Dir->Info->Flags |= RDF_FILE;
         StrCopy(name_from_path(zf->Name), Dir->Info->Name, MAX_FILENAME);

         Dir->prvIndexPtr = zf->Next;
         Dir->prvTotal++;
         return ERR_Okay;
      }

      if ((Dir->prvFlags & RDF_FOLDER) and (zf->IsFolder)) {
         Dir->Info->Flags |= RDF_FOLDER;

         LONG i = StrCopy(name_from_path(zf->Name), Dir->Info->Name, MAX_FILENAME-2);

         if (Dir->prvFlags & RDF_QUALIFY) {
            Dir->Info->Name[i++] = '/';
            Dir->Info->Name[i++] = 0;
         }

         if (Dir->prvFlags & RDF_PERMISSIONS) {
            Dir->Info->Flags |= RDF_PERMISSIONS;
            Dir->Info->Permissions = PERMIT_READ|PERMIT_GROUP_READ|PERMIT_OTHERS_READ;
         }

         Dir->prvIndexPtr = zf->Next;
         Dir->prvTotal++;
         return ERR_Okay;
      }
   }

   return ERR_DirEmpty;
}

//****************************************************************************

static ERROR close_folder(DirInfo *Dir)
{
   return ERR_Okay;
}

//****************************************************************************

static ERROR get_info(CSTRING Path, FileInfo *Info, LONG InfoSize)
{
   parasol::Log log(__FUNCTION__);
   CompressedItem *item;
   objCompression *cmp;
   std::string file_path;
   ERROR error;

   log.traceBranch("%s", Path);

   if ((cmp = find_archive(Path, file_path))) {
      struct cmpFind find = { .Path=file_path.c_str(), .Flags=STR_CASE|STR_MATCH_LEN };
      if ((error = Action(MT_CmpFind, &cmp->Head, &find))) return error;
      item = find.Item;
   }
   else return ERR_DoesNotExist;

   Info->Size = item->OriginalSize;
   Info->Flags = 0;
   Info->Created = item->Created;
   Info->Modified = item->Modified;

   if (item->Flags & FL_FOLDER) Info->Flags |= RDF_FOLDER;
   else Info->Flags |= RDF_FILE|RDF_SIZE;

   // Extract the file name

   LONG len = StrLength(Path);
   LONG i = len;
   if ((Path[i-1] IS '/') or (Path[i-1] IS '\\')) i--;
   while ((i > 0) and (Path[i-1] != '/') and (Path[i-1] != '\\') and (Path[i-1] != ':')) i--;
   i = StrCopy(Path + i, Info->Name, MAX_FILENAME-2);

   if (Info->Flags & RDF_FOLDER) {
      if (Info->Name[i-1] IS '\\') Info->Name[i-1] = '/';
      else if (Info->Name[i-1] != '/') {
         Info->Name[i++] = '/';
         Info->Name[i] = 0;
      }
   }

   Info->Permissions = item->Permissions;
   Info->UserID      = item->UserID;
   Info->GroupID     = item->GroupID;
   Info->Tags        = NULL;
   return ERR_Okay;
}

//****************************************************************************
// Test an archive: location.

static ERROR test_path(STRING Path, LONG Flags, LONG *Type)
{
   parasol::Log log(__FUNCTION__);

   log.traceBranch("%s", Path);

   std::string file_path;
   objCompression *cmp;
   if (!(cmp = find_archive(Path, file_path))) return ERR_DoesNotExist;

   if (file_path.empty()) {
      *Type = LOC_VOLUME;
      return ERR_Okay;
   }

   CompressedItem *item;
   ERROR error = cmpFind(cmp, file_path.c_str(), STR_CASE|STR_MATCH_LEN, &item);

   if ((error) and (Flags & RSF_APPROXIMATE)) {
      file_path.append(".*");
      if (!(error = cmpFind(cmp, file_path.c_str(), STR_CASE|STR_WILDCARD, &item))) {
         // Point the path to the discovered item
         LONG i;
         for (i=0; (Path[i] != '/') and (Path[i]); i++);
         if (Path[i] IS '/') StrCopy(item->Path, Path + i + 1, MAX_FILENAME);
      }
   }

   if (error) {
      log.trace("cmpFind() did not find %s, %s", file_path.c_str(), GetErrorMsg(error));

      if (error IS ERR_Search) return ERR_DoesNotExist;
      else return error;
   }

   if (item->Flags & FL_FOLDER) *Type = LOC_FOLDER;
   else *Type = LOC_FILE;

   return ERR_Okay;
}

//****************************************************************************

static const ActionArray clArchiveActions[] = {
   { AC_Activate, (APTR)ARCHIVE_Activate },
   { AC_Free,     (APTR)ARCHIVE_Free },
   { AC_Init,     (APTR)ARCHIVE_Init },
   { AC_Query,    (APTR)ARCHIVE_Query },
   { AC_Read,     (APTR)ARCHIVE_Read },
   { AC_Seek,     (APTR)ARCHIVE_Seek },
   { AC_Write,    (APTR)ARCHIVE_Write },
   { 0, NULL }
};

static const MethodArray clArchiveMethods[] = {
   { 0, NULL, NULL, NULL, 0 }
};

static const struct FieldArray clArchiveFields[] = {
   { "Size", FDF_LARGE|FDF_R, 0, (APTR)ARCHIVE_GET_Size, NULL },
    END_FIELD
};

//****************************************************************************

extern "C" ERROR add_archive_class(void)
{
   return CreateObject(ID_METACLASS, 0, (OBJECTPTR *)&glArchiveClass,
      FID_BaseClassID|TLONG, ID_FILE,
      FID_SubClassID|TLONG,  ID_FILEARCHIVE,
      FID_Name|TSTRING,      "FileArchive",
      FID_Actions|TPTR,      clArchiveActions,
      FID_Methods|TARRAY,    clArchiveMethods,
      FID_Fields|TARRAY,     clArchiveFields,
      FID_Path|TSTR,         "modules:core",
      TAGEND);
}

//****************************************************************************

extern "C" ERROR create_archive_volume(void)
{
   return VirtualVolume("archive",
      VAS_OPEN_DIR,  &open_folder,
      VAS_SCAN_DIR,  &scan_folder,
      VAS_CLOSE_DIR, &close_folder,
      VAS_TEST_PATH, &test_path,
      VAS_GET_INFO,  &get_info,
      0);
}
