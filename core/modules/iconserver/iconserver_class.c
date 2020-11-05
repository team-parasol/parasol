/*****************************************************************************

-CLASS-
IconServer: Manages the icon database.

The IconServer class provides an icon management and image resizing service for all applications.  It provides this
service through the icons: assignment.  Icons are requested from the server by accessing them through the virtual
filesystem, making access to the icon server 'invisible'.  Direct contact with the icon server only needs to be made
when requesting a new theme, or to change the default icon size.

Icon files can be accessed from the icon server using the following file path formats:

<pre>
icons:filename
icons:category/filename
icons:category/filename(size)
</pre>

The second format, where the category and filename is specified is recommended for normal usage.  The size of the icon
can be requested by entering a numeral in brackets at the end of the location.  For instance
'icons:programs/filemanager(64)' will generate a 64x64 pixel version of the filemanager icon.  The minimum size for an
icon is 12 pixels and the maximum size is 256 pixels.
-END-

*****************************************************************************/

static ERROR resolve_icon_assign(objIconServer *, STRING, STRING, LONG);

static ERROR add_iconserver(void)
{
   if (CreateObject(ID_METACLASS, 0, &clIconServer,
         FID_ClassVersion|TFLOAT, VER_IconServer,
         FID_Name|TSTR,      "IconServer",
         FID_Category|TLONG, CCF_SYSTEM,
         FID_Actions|TPTR,   clActions,
         FID_Fields|TARRAY,  clFields,
         FID_Flags|TLONG,    CLF_SHARED_ONLY,
         FID_Size|TLONG,     sizeof(objIconServer),
         FID_Path|TSTR,      MOD_PATH,
         TAGEND) != ERR_Okay) {
      return PostError(ERR_CreateObject);
   }

   return ERR_Okay;
}

//****************************************************************************

static ERROR ICON_AccessObject(objIconServer *Self, APTR Void)
{
   Self->ResolvePath = &resolve_icon_assign;
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Clear: Removes all cached images generated by the icon server.
-END-
*****************************************************************************/

static ERROR ICON_Clear(objIconServer *Self, APTR Void)
{
   LogBranch(NULL);

   char buffer[256];
   StrFormat(buffer, sizeof(buffer), "%s%s/", glIconPath, Self->prvTheme);

   struct DirInfo *dir;
   if (!OpenDir(buffer, RDF_FOLDERS, &dir)) {
      while (!ScanDir(dir)) {
         StrFormat(buffer, sizeof(buffer), "temp:iconcache/%s/%s/", Self->prvTheme, dir->Info->Name);
         DeleteFile(buffer, NULL);
      }

      FreeResource(dir);
   }
   else LogMsg("No image directories in theme location \"%s\".", buffer);

   LogReturn();
   return ERR_Okay;
}

//****************************************************************************

static ERROR ICON_Free(objIconServer *Self, APTR Void)
{
   return ERR_Okay;
}

//****************************************************************************

static ERROR ICON_Init(objIconServer *Self, APTR Void)
{
   // Check that the theme directory exists.  Switch to the default directory if it does not.

   LogMsg("Checking for theme \"%s\".", Self->prvTheme);

   LONG type;
   char buffer[100];
   StrFormat(buffer, sizeof(buffer), "%s%s/", glIconPath, Self->prvTheme);
   if ((AnalysePath(buffer, &type) != ERR_Okay) OR (type != LOC_DIRECTORY)) {
      LogMsg("Bad theme \"%s\", switching to default.", Self->prvTheme);
      StrCopy("Default", Self->prvTheme, sizeof(Self->prvTheme));
   }

   // Open the iconserver.data file from the user's icon theme directory.  If the iconserver.data file does not exist,
   // we'll rebuild the file by scanning the icon directories.

   if (!load_icon_db(Self)) return ERR_Okay;
   if (!acRefresh(Self)) return ERR_Okay;
   return ERR_Okay;
}

//****************************************************************************

static ERROR ICON_NewObject(objIconServer *Self, APTR Void)
{
   StrCopy("Default", Self->prvTheme, sizeof(Self->prvTheme));
   Self->FixedSize = 0;
   Self->IconRatio = 7.0;
   Self->ResolvePath = &resolve_icon_assign;
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Refresh: Regenerates the icon database.
-END-
*****************************************************************************/

static ERROR ICON_Refresh(objIconServer *Self, APTR Void)
{

   // Regenerate the icon database.  This involves scanning each directory in the theme location and querying them
   // through the Picture class (so that we can obtain size information).
   //
   // The database stores icons in this format:
   //
   // <LONG:Code> <LONG:Length> <UBYTE[]:Category>
   //   <LONG:Code> <LONG:Length> <UBYTE[]:IconFile>
   //   <LONG:Code> <LONG:Length> <UBYTE[]:IconFile>
   //   ...
   // <LONG:Code> <LONG:Length> <UBYTE[]:Category>
   //   <LONG:Code> <LONG:Length> <UBYTE[]:IconFile>
   //   <LONG:Code> <LONG:Length> <UBYTE[]:IconFile>
   //   ...

   LogBranch("Regenerating the icon database...");

   UBYTE buffer[256], *data;
   StrFormat(buffer, sizeof(buffer), "temp:iconcache/%s/", Self->prvTheme);
   CreateFolder(buffer, PERMIT_READ|PERMIT_WRITE);

   StrFormat(buffer, sizeof(buffer), "temp:iconcache/%s/iconserver.data", Self->prvTheme);

   OBJECTPTR output;
   if (!CreateObject(ID_FILE, NF_INTEGRAL, &output,
         FID_Path|TSTR,         buffer,
         FID_Flags|TLONG,       FL_NEW|FL_WRITE,
         FID_Permissions|TLONG, PERMIT_READ|PERMIT_WRITE,
         TAGEND)) {

      StrFormat(buffer, sizeof(buffer), "%s%s/", glIconPath, Self->prvTheme);

      struct DirInfo *dir;
      if (!OpenDir(buffer, RDF_FOLDERS, &dir)) {
         while (!ScanDir(dir)) {
            data = buffer;
            ((LONG *)data)[0] = CODE_CATEGORY;
            data += sizeof(LONG);

            WORD len;
            for (len=0; dir->Info->Name[len]; len++);
            len++;
            len = ALIGN32(len);

            ((LONG *)data)[0] = sizeof(LONG) + sizeof(LONG) + len;
            data += sizeof(LONG);
            data += StrCopy(dir->Info->Name, data, sizeof(buffer));
            *data++ = 0; *data++ = 0; *data++ = 0; *data++ = 0;

            acWrite(output, buffer, sizeof(LONG) + sizeof(LONG) + len, 0);

            write_icon_category(Self, output, dir->Info->Name);
         }

         // Write the end-of-database marker

         data = buffer;
         ((LONG *)data)[0] = CODE_END;
         acWrite(output, buffer, sizeof(LONG), 0);

         FreeResource(dir);
      }
      else LogMsg("No image directories in theme location \"%s\".", buffer);

      acFree(output);
   }

   if (load_icon_db(Self) != ERR_Okay) {
      LogReturn();
      return ERR_Failed;
   }

   LogReturn();
   return ERR_Okay;
}

//****************************************************************************

static ERROR ICON_ReleaseObject(objIconServer *Self, APTR Void)
{
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Cache: Switch off the cache by writing a FALSE value to this field.

By default, all generated icons are cached on the hard drive for high speed loading whenever an icon is requested at
a previously generated size.  This can potentially be a problem when developing new icon sets or adjusting the icon
database, as you may want to regenerate each icon from scratch.  Turning off the cache will typically fix such
problems.

-FIELD-
CacheSize: Indicates the total size of the icon cache in bytes.

This field can be read at any time to determine the total number of bytes in use by the icon cache.  The result value
is generated dynamically through analysis of the icon directory structure.  Only the active icon theme will be analysed.

*****************************************************************************/

static ERROR GET_CacheSize(objIconServer *Self, LONG *Value)
{
   LogBranch(NULL);

   char buffer[256];
   StrFormat(buffer, sizeof(buffer), "%s%s/", glIconPath, Self->prvTheme);

   *Value = 0;

   struct DirInfo *dir, *subdir;
   if (!OpenDir(buffer, RDF_FOLDER, &dir)) {
      while (!ScanDir(dir)) {
         StrFormat(buffer, sizeof(buffer), "temp:iconcache/%s/%s/", Self->prvTheme, dir->Info->Name);

         if (!OpenDir(buffer, RDF_FILE|RDF_SIZE, &subdir)) {
            while (!ScanDir(subdir)) *Value += subdir->Info->Size;
            FreeResource(subdir);
         }
      }
      FreeResource(dir);
   }

   LogReturn();

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
FixedSize: Enforces a fixed icon size.

Setting the FixedSize field will enforce fixed icon sizing (measured in pixels), which overrides the dynamic sizing
normally enabled by #IconSize.  Fixed sizing can be switched off by setting this field to zero.

-FIELD-
IconSize: The default size of each icon generated by the server can be changed here.

By default, the iconserver will generate icons at a size of 32 pixels when no specific size has been requested.  You
may set this field to increase or decrease the default icon size.  The minimum and maximum values are 12 and 256
pixels respectively.

*****************************************************************************/

static ERROR GET_IconSize(objIconServer *Self, LONG *Value)
{
   if (Self->FixedSize >= 12) {
      *Value = Self->FixedSize;
   }
   else {
      if (Self->IconRatio < 2) Self->IconRatio = 2;

      SURFACEINFO *info;
      if (!drwGetSurfaceInfo(0, &info)) {
         if (info->Width < info->Height) *Value = ((DOUBLE)info->Width) * Self->IconRatio / 100.0;
         else *Value = ((DOUBLE)info->Height) * Self->IconRatio / 100.0;
         if (*Value < MIN_SIZE) *Value = MIN_SIZE;
      }
      else *Value = DEFAULT_SIZE;
   }

   return ERR_Okay;
}

static ERROR SET_IconSize(objIconServer *Self, LONG Value)
{
   if (!Value) {
      // Turn off fixed size, make sure ratios are enabled
      Self->FixedSize = 0;
      if (Self->IconRatio < 2) Self->IconRatio = 2;
   }
   else if (Value >= MIN_SIZE) {
      Self->FixedSize = Value;
      if (Self->FixedSize < MIN_SIZE) Self->FixedSize = MIN_SIZE;
      else if (Self->FixedSize > MAX_SIZE) Self->FixedSize = MAX_SIZE;
  }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
IconRatio: Setting this field enables dynamic icon sizing that is relative to the display size.

The IconRatio field can be set if you wish to enable dynamic icon sizing.  This field is set as a percentage, typically
around 7%, which would generate icons at 7% of the shortest dimension of the current display .

The minimum ratio is 2% and the maximum is 15%.  When you write to this field, fixed icon sizes will no longer be
the default.  You can switch back to fixed icon sizes if you write a value to the IconSize field.  Fixed size icons
can also be requested at any time on an individual basis by passing a size to the icons: assignment when loading
icons.

Regardless of the ratio setting, the maximum and minimum pixel size of any icon is always 12 and 256 pixels
respectively.

*****************************************************************************/

static ERROR SET_IconRatio(objIconServer *Self, DOUBLE Value)
{
   Self->IconRatio = Value;

   if (Self->IconRatio < 2.0) Self->IconRatio = 2.0;
   else if (Self->IconRatio > 15.0) Self->IconRatio = 15.0;

   LogMsg("Icon size ratio changed to %.2f%%.", Self->IconRatio);
   return ERR_Okay;
}

/*****************************************************************************
-FIELD-
Theme: The theme used by an iconserver object is set here.

The theme currently in use by the iconserver is set here.  You can alter the theme at any time by writing the name of
the new theme to this field.  The name must be represented in the 'system:icons/' directory tree or the theme will
revert back to the default setting.  When you change the theme, the icon database will be refreshed.  This may take
some time depending on the number of icons to process.

*****************************************************************************/

static ERROR GET_Theme(objIconServer *Self, STRING *Value)
{
   if (Self->prvTheme[0]) *Value = Self->prvTheme;
   else *Value = NULL;
   return ERR_Okay;
}

static ERROR SET_Theme(objIconServer *Self, CSTRING Value)
{
   if ((Value) AND (*Value)) StrCopy(Value, Self->prvTheme, sizeof(Self->prvTheme));
   else StrCopy("Default", Self->prvTheme, sizeof(Self->prvTheme));

   if (!(Self->Head.Flags & NF_INITIALISED)) return ERR_Okay;

   // Check that the theme directory exists.  Switch to the default directory if it does not.

   LONG type;
   char buffer[256];
   StrFormat(buffer, sizeof(buffer), "%s%s/", glIconPath, Self->prvTheme);
   if ((AnalysePath(buffer, &type) != ERR_Okay) OR (type != LOC_DIRECTORY)) {
      LogErrorMsg("The requested theme \"%s\" does not exist.", Self->prvTheme);
      StrCopy("Default", Self->prvTheme, sizeof(Self->prvTheme));
   }

   // Open the iconserver.data file from the user's icon theme directory.  If the iconserver.data file does not exist,
   // we'll rebuild the file by scanning the icon directories.

   if (!load_icon_db(Self)) return ERR_Okay;
   if (!acRefresh(Self)) return ERR_Okay;
   return ERR_Failed;
}

/*****************************************************************************
-FIELD-
VolatileIcons: If TRUE, the icon database will not be checked for cached icon images.

Setting the VolatileIcons field to TRUE will turn off support for the icon database.  Icons will be scaled on the fly
and saved to disk, even if a cached version of that icon already exists.
-END-
*****************************************************************************/

static ERROR load_icon_db(objIconServer *Self)
{
   if (glDatabase) { FreeResource(glDatabase); glDatabase = NULL; }

   char buffer[256];
   StrFormat(buffer, sizeof(buffer), "temp:iconcache/%s/iconserver.data", Self->prvTheme);

   OBJECTPTR file;
   ERROR error = ERR_Okay;
   if (!CreateObject(ID_FILE, NF_INTEGRAL, &file,
         FID_Path|TSTR,   buffer,
         FID_Flags|TLONG, FL_READ,
         TAGEND)) {
      if ((!GetLong(file, FID_Size, &glDataSize)) AND (glDataSize > 0)) {
         if (!AllocMemory(glDataSize, MEM_UNTRACKED|MEM_DATA|MEM_NO_CLEAR, &glDatabase, NULL)) {
            acRead(file, glDatabase, glDataSize, 0);
         }
         else error = PostError(ERR_AllocMemory);
      }
      else error = PostError(ERR_NoData);

      acFree(file);
   }
   else error = ERR_CreateObject;

   return error;
}

//****************************************************************************

static void write_icon_category(objIconServer *Self, OBJECTPTR Output, STRING Category)
{
   char buffer[256];
   StrFormat(buffer, sizeof(buffer), "%s%s/%s/", glIconPath, Self->prvTheme, Category);

   struct DirInfo *dir;
   if (!OpenDir(buffer, RDF_FILE, &dir)) {
      while (!ScanDir(dir)) {
         struct FileInfo *scan = dir->Info;

         UBYTE *data = buffer;
         ((LONG *)data)[0] = CODE_ICON;
         data += sizeof(LONG);

         WORD len, j;
         for (len=0; scan->Name[len]; len++);

         for (j=len; (j > 0) AND (scan->Name[j] != '.'); j--);
         if (scan->Name[j] IS '.') {
            len = j;
            scan->Name[j] = 0;
         }
         len++;
         len = (len + 3) & (~3);

         ((LONG *)data)[0] = sizeof(LONG) + sizeof(LONG) + len;

         data += sizeof(LONG);
         data += StrCopy(scan->Name, data, COPY_ALL);
         *data++ = 0; *data++ = 0; *data++ = 0; *data++ = 0;

         struct acWrite write = {
            .Buffer = buffer,
            .Length = sizeof(LONG) + sizeof(LONG) + len
         };
         Action(AC_Write, Output, &write);
      }

      FreeResource(dir);
   }
}

/*****************************************************************************
** This function is called by the file system when it needs to resolve the icons: assignment.  It is also used to
** check for the existence of files.
**
** Icons can be accessed in the following formats:
**
**    icons:filename
**    icons:category/filename
**    icons:category/filename(size)
**
** If overlaying a foreground onto an icon, this format is acceptable:
**
**    icons:category/filename(size);icons:category/filename
**
** Only the first size will be accepted when overlaying a foreground onto an icon.
*/

static ERROR resolve_icon_assign(objIconServer *Self, STRING Path, STRING Buffer, LONG BufferSize)
{
   UBYTE category[BUFFERSIZE], icon[BUFFERSIZE], ovcategory[BUFFERSIZE], ovicon[BUFFERSIZE]; // NB: path extraction code assumes these are all the same array size
   LONG i;

   FMSG("~resolve_icon()","%s", Path);

   // If the location ends with a trailing slash, then a directory has actually been requested and we should simply
   // resolve the path to its absolute location (important for browsing in the File Manager for example).

   for (i=0; Path[i]; i++);
   if ((Path[i-1] IS '/') OR (Path[i-1] IS '\\') OR (Path[i-1] IS ':')) {
      StrFormat(Buffer, BufferSize, "%s%s/%s", glIconPath, Self->prvTheme, Path + 6);
      STEP();
      return ERR_Okay;
   }

   // Extract the icon name, theme and size

   LONG size;
   if ((size = Self->FixedSize) < 12) {
      SURFACEINFO *info;
      if (!drwGetSurfaceInfo(0, &info)) {
         if (info->Width < info->Height) size = F2T(((DOUBLE)info->Width) * Self->IconRatio / 100.0);
         else size = F2T(((DOUBLE)info->Height) * Self->IconRatio / 100.0);
      }
   }

   ERROR error;
   if ((error = extract_icon(size, Path, category, icon, ovcategory, ovicon, &size)) != ERR_Okay) { STEP(); return error; }
   if ((error = find_icon_category(category, icon)) != ERR_Okay) { STEP(); return error; }

   FMSG("resolve_icon","Resolved %s to icon %s/%s, overlay %s/%s, size %d", Path, category, icon, ovcategory, ovicon, size);

   if (ovcategory[0]) {
      if (find_icon_category(ovcategory, ovicon) != ERR_Okay) ovcategory[0] = 0;
      FMSG("resolve_icon","Overlay category '%s', icon '%s'", ovcategory, ovicon);
   }

   // Try to open the icon at its cache location.  If it is there, then we will use the cached version.  We also do some
   // date checking here - if the date/time on the original is newer than the date on the cache, we'll need to
   // regenerate the cached image.

   if (Self->VolatileIcons IS FALSE) {
      if (ovcategory[0]) StrFormat(Buffer, BufferSize, "temp:iconcache/%s/%s/%s_%s_%s%d", Self->prvTheme, category, icon, ovcategory, ovicon, size);
      else StrFormat(Buffer, BufferSize, "temp:iconcache/%s/%s/%s%d", Self->prvTheme, category, icon, size);

      MSG("Attempting to open icon from cache location %s", Buffer);

      objFile *cache_file;
      if (!CreateObject(ID_FILE, NF_INTEGRAL, &cache_file,
            FID_Path|TSTR, Buffer,
            TAGEND)) {

         LARGE cache_ts, src_ts;
         GetLarge(cache_file, FID_TimeStamp, &cache_ts);
         acFree(cache_file);

         // Check the date/time of the original icon source
         char original[256];
         StrFormat(original, sizeof(original), "%s%s/%s/%s.png", glIconPath, Self->prvTheme, category, icon);

         objFile *src;
         if (!CreateObject(ID_FILE, NF_INTEGRAL, &src,
               FID_Path|TSTR, original,
               TAGEND)) {
            GetLarge(src, FID_TimeStamp, &src_ts);
            acFree(src);

            if (src_ts < cache_ts) {
               STEP();
               return ERR_Okay; // Use the existing cache file
            }
            else LogF("resolve_icon","Regenerating new cache file for icon \"%s/%s\".", category, icon);
         }
      }
   }

   // Load the original icon, resize it to the requested size and write the resulting picture file to the cache.  Then resolve the assignment so that it points to the cached file.

   LogF("resolve_icon","Resizing icon to new pixel size: %d", size);

   StrFormat(Buffer, BufferSize, "%s%s/%s/%s", glIconPath, Self->prvTheme, category, icon);

   objPicture *picture;
   if (CreateObject(ID_PICTURE, NF_INTEGRAL, &picture,
         FID_Path|TSTR,   Buffer,
         FID_Flags|TLONG, PCF_FORCE_ALPHA_32,
         TAGEND) != ERR_Okay) {
      LogF("@resolve_icon","Failed to open file \"%s\".", Buffer);
      STEP();
      return ERR_CreateObject;
   }

   // Initialise the destination picture that we are going to use for resizing

   objPicture *resizepic;
   if (NewObject(ID_PICTURE, NF_INTEGRAL, &resizepic) != ERR_Okay) {
      acFree(picture);
      STEP();
      return ERR_NewObject;
   }

   SetName(resizepic, "picResizedIcon");

   DOUBLE sizeratio;
   if (picture->Bitmap->Width > picture->Bitmap->Height) sizeratio = (DOUBLE)size / (DOUBLE)picture->Bitmap->Width;
   else sizeratio = (DOUBLE)size / (DOUBLE)picture->Bitmap->Height;

   resizepic->Flags |= PCF_NEW | (picture->Flags & (PCF_MASK|PCF_ALPHA));
   resizepic->Bitmap->Flags |= picture->Bitmap->Flags & (BMF_ALPHA_CHANNEL|BMF_INVERSE_ALPHA);
   resizepic->Bitmap->BitsPerPixel = picture->Bitmap->BitsPerPixel;
   resizepic->Bitmap->Width  = ((DOUBLE)picture->Bitmap->Width)  * sizeratio;
   resizepic->Bitmap->Height = ((DOUBLE)picture->Bitmap->Height) * sizeratio;

   if (!acInit(resizepic)) {
      // Stretch the source into the destination

      gfxCopyStretch(picture->Bitmap, resizepic->Bitmap, CSTF_BILINEAR|CSTF_FILTER_SOURCE, 0, 0,
         picture->Bitmap->Width, picture->Bitmap->Height, 0, 0, resizepic->Bitmap->Width, resizepic->Bitmap->Height);

      if ((picture->Mask) AND (resizepic->Mask)) {
         gfxCopyStretch(picture->Mask, resizepic->Mask, CSTF_BILINEAR|CSTF_FILTER_SOURCE, -1, -1,
            picture->Mask->Width + 2, picture->Mask->Height + 2, 0, 0, resizepic->Mask->Width, resizepic->Mask->Height);
      }
   }
   else {
      LogF("@resolve_icon","Failed to initialise the resize picture space.");
      acFree(resizepic);
      acFree(picture);
      STEP();
      return ERR_Init;
   }

   acFree(picture);

   // Apply underlay/overlay filters

   apply_filter(resizepic->Bitmap, NULL, category, icon, NULL);

   // Generate the path that we will save the icon to

   StrFormat(Buffer, BufferSize, "temp:iconcache/%s/%s/%s%d", Self->prvTheme, category, icon, size);

   // Load an overlay on top of the icon if requested

   if ((ovcategory[0]) AND (ovicon[0])) {
      FMSG("resolve_icon","Loading overlay %s/%s", ovcategory, ovicon);

      char overlay[256];
      StrFormat(overlay, sizeof(overlay), "%s%s/%s/%s", glIconPath, Self->prvTheme, ovcategory, ovicon);

      if (!CreateObject(ID_PICTURE, NF_INTEGRAL, &picture,
            FID_Path|TSTR,   overlay,
            FID_Flags|TLONG, PCF_FORCE_ALPHA_32,
            TAGEND)) {

         // Stretch the source into the destination

         objBitmap *temp;
         if (!CreateObject(ID_BITMAP, NF_INTEGRAL, &temp,
               FID_Width|TLONG,        resizepic->Bitmap->Width,
               FID_Height|TLONG,       resizepic->Bitmap->Height,
               FID_BitsPerPixel|TLONG, 32,
               FID_Flags|TLONG,        BMF_ALPHA_CHANNEL,
               TAGEND)) {
            gfxCopyStretch(picture->Bitmap, temp, CSTF_BILINEAR|CSTF_FILTER_SOURCE, 0, 0,
               picture->Bitmap->Width, picture->Bitmap->Height, 0, 0, temp->Width, temp->Height);
            gfxCopyArea(temp, resizepic->Bitmap, BAF_BLEND, 0, 0, temp->Width, temp->Height, 0, 0);
            StrFormat(Buffer, BufferSize, "temp:iconcache/%s/%s/%s_%s_%s%d", Self->prvTheme, category, icon, ovcategory, ovicon, size);
            acFree(temp);
         }

         acFree(picture);
      }
   }

   // Save the new picture

   OBJECTPTR file;
   if (!CreateObject(ID_FILE, NF_INTEGRAL, &file,
         FID_Path|TSTR,   Buffer,
         FID_Flags|TLONG, FL_NEW|FL_WRITE,
         FID_Permissions|TLONG, PERMIT_READ|PERMIT_WRITE,
         TAGEND)) {
      acSaveToObject(resizepic, file->UniqueID, 0);
      acFree(file);
   }

   FMSG("resolve_icon","Path resolved to '%s' from '%s'", Buffer, Path);

   acFree(resizepic);
   STEP();
   return ERR_Okay;
}

//****************************************************************************

static const struct FieldArray clFields[] = {
   { "IconRatio",     FDF_DOUBLE|FDF_RW,  0, NULL, SET_IconRatio },
   { "ResolvePath",   FDF_POINTER|FDF_SYSTEM|FDF_RW, 0, NULL, NULL },
   { "FixedSize",     FDF_LONG|FDF_RW,    0, NULL, SET_IconSize }, // When reading the FixedSize field, the preferred size is returned (which can be 0 if fixed sizing is off)
   { "VolatileIcons", FDF_LONG|FDF_RW,    0, NULL, NULL },
   // Virtual fields
   { "CacheSize",     FDF_LONG|FDF_R,     0, GET_CacheSize, NULL },
   { "IconSize",      FDF_LONG|FDF_RW,    0, GET_IconSize, SET_IconSize },
   { "Theme",         FDF_STRING|FDF_RW,  0, GET_Theme, SET_Theme },
    END_FIELD
};

static const struct ActionArray clActions[] = {
   { AC_AccessObject,  ICON_AccessObject },
   { AC_Clear,         ICON_Clear },
   { AC_Free,          ICON_Free },
   { AC_Init,          ICON_Init },
   { AC_NewObject,     ICON_NewObject },
   { AC_Refresh,       ICON_Refresh },
   { AC_ReleaseObject, ICON_ReleaseObject },
   { 0, NULL }
};
