/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
Bitmap: Manages bitmap graphics and provides drawing functionality.

The Bitmap class provides a way of describing an area of memory that an application can draw to, and/or display if the
data is held in video memory.  Bitmaps are used in the handling of @Display and @Picture objects, and form the backbone
of Parasol's graphics functionality.  The Bitmap class supports everything from basic graphics primitives to masking and
alpha blending features.

To create a new bitmap object, you need to specify its #Width and #Height at a minimum.  Preferably, you should also
know how many colours you want to use and whether the bitmap data should be held in standard memory (for CPU based
reading and writing) or video memory (for hardware based drawing).  After creating a bitmap you can use a number of
available drawing methods for the purpose of image management.  Please note that these methods are designed to be
called under exclusive conditions, and it is not recommended that you call methods on a bitmap using the message
system.

By default, the CPU can only be used to read and write data directly to or from a bitmap when it is held in standard
memory (this is the default type).  If the BLIT or VIDEO flags are specified in the #DataFlags field then the
CPU cannot access this memory, unless you specifically request it.  To do this, use the #Lock() and #Unlock() actions
to temporarily gain read/write access to a bitmap.

If you require complex drawing functionality that is not available in the Bitmap class, please use the functionality
provided by the Vector module.

To save the image of a bitmap, you can either copy its image to a @Picture object, or you can use the SaveImage
action to save the data in PCX format.  Raw data can also be processed through a bitmap by using the Read and Write
actions.
-END-

*****************************************************************************/

#define PRV_BITMAP
#define PRV_DISPLAY

#ifdef _WIN32
#define DLLCALL // __declspec(dllimport)
#define WINAPI  __stdcall

DLLCALL LONG WINAPI SetPixelV(APTR, LONG, LONG, LONG);
DLLCALL LONG WINAPI SetPixel(APTR, LONG, LONG, LONG);
DLLCALL LONG WINAPI GetPixel(APTR, LONG, LONG);
#endif

static LONG CalculatePixelRoutines(objBitmap *);

//****************************************************************************
// Pixel and pen based functions.

// Video Pixel Routines

#ifdef _WIN32

static void  VideoDrawPixel(objBitmap *, LONG, LONG, ULONG);
static void  VideoDrawRGBPixel(objBitmap *, LONG, LONG, RGB8 *);
static void  VideoDrawRGBIndex(objBitmap *, UBYTE *, RGB8 *);
static ULONG VideoReadPixel(objBitmap *, LONG, LONG);
static void  VideoReadRGBPixel(objBitmap *, LONG, LONG, RGB8 *);
static void  VideoReadRGBIndex(objBitmap *, UBYTE *, RGB8 *);

#else

static void VideoDrawPixel32(objBitmap *, LONG, LONG, ULONG);
static void VideoDrawPixel24(objBitmap *, LONG, LONG, ULONG);
static void VideoDrawPixel16(objBitmap *, LONG, LONG, ULONG);
static void VideoDrawPixel8(objBitmap *,  LONG, LONG, ULONG);

static void VideoDrawRGBPixel32(objBitmap *, LONG, LONG, RGB8 *);
static void VideoDrawRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void VideoDrawRGBPixel16(objBitmap *, LONG, LONG, RGB8 *);
static void VideoDrawRGBPixel8(objBitmap *,  LONG, LONG, RGB8 *);

static void VideoDrawRGBIndex32(objBitmap *, ULONG *, RGB8 *);
static void VideoDrawRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void VideoDrawRGBIndex16(objBitmap *, UWORD *, RGB8 *);
static void VideoDrawRGBIndex8(objBitmap *,  UBYTE *, RGB8 *);

static ULONG VideoReadPixel32(objBitmap *, LONG, LONG);
static ULONG VideoReadPixel24(objBitmap *, LONG, LONG);
static ULONG VideoReadPixel16(objBitmap *, LONG, LONG);
static ULONG VideoReadPixel8(objBitmap *,  LONG, LONG);

static void VideoReadRGBPixel32(objBitmap *, LONG, LONG, RGB8 *);
static void VideoReadRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void VideoReadRGBPixel16(objBitmap *, LONG, LONG, RGB8 *);
static void VideoReadRGBPixel8(objBitmap *,  LONG, LONG, RGB8 *);

static void VideoReadRGBIndex32(objBitmap *, ULONG *, RGB8 *);
static void VideoReadRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void VideoReadRGBIndex16(objBitmap *, UWORD *, RGB8 *);
static void VideoReadRGBIndex8(objBitmap *,  UBYTE *, RGB8 *);

#endif

// Memory Pixel Routines

static void MemDrawPixel32(objBitmap *, LONG, LONG, ULONG);
static void MemDrawLSBPixel24(objBitmap *, LONG, LONG, ULONG);
static void MemDrawMSBPixel24(objBitmap *, LONG, LONG, ULONG);
static void MemDrawPixel16(objBitmap *, LONG, LONG, ULONG);
static void MemDrawPixel8(objBitmap *, LONG, LONG, ULONG);

static ULONG MemReadPixel32(objBitmap *, LONG, LONG);
static ULONG MemReadLSBPixel24(objBitmap *, LONG, LONG);
static ULONG MemReadMSBPixel24(objBitmap *, LONG, LONG);
static ULONG MemReadPixel16(objBitmap *, LONG, LONG);
static ULONG MemReadPixel8(objBitmap *, LONG, LONG);

static void MemDrawRGBPixel32(objBitmap *, LONG, LONG, RGB8 *);
static void MemDrawLSBRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void MemDrawMSBRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void MemDrawRGBPixel16(objBitmap *, LONG, LONG, RGB8 *);
static void MemDrawRGBPixel8(objBitmap *, LONG, LONG, RGB8 *);

static void MemDrawRGBIndex32(objBitmap *, ULONG *, RGB8 *);
static void MemDrawLSBRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void MemDrawMSBRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void MemDrawRGBIndex16(objBitmap *, UWORD *, RGB8 *);
static void MemDrawRGBIndex8(objBitmap *, UBYTE *, RGB8 *);

static void MemReadRGBPixel32(objBitmap *, LONG, LONG, RGB8 *);
static void MemReadLSBRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void MemReadMSBRGBPixel24(objBitmap *, LONG, LONG, RGB8 *);
static void MemReadRGBPixel16(objBitmap *, LONG, LONG, RGB8 *);
static void MemReadRGBPixel8(objBitmap *, LONG, LONG, RGB8 *);

static void MemReadRGBIndex32(objBitmap *, ULONG *, RGB8 *);
static void MemReadLSBRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void MemReadMSBRGBIndex24(objBitmap *, UBYTE *, RGB8 *);
static void MemReadRGBIndex16(objBitmap *, UWORD *, RGB8 *);
static void MemReadRGBIndex8(objBitmap *, UBYTE *, RGB8 *);

static void MemReadRGBPixelPlanar(objBitmap *, LONG, LONG, RGB8 *);
static void MemReadRGBIndexPlanar(objBitmap *, UBYTE *, RGB8 *);
static void MemDrawPixelPlanar(objBitmap *, LONG, LONG, ULONG);
static ULONG MemReadPixelPlanar(objBitmap *, LONG, LONG);

static void DrawRGBPixelPlanar(objBitmap *, LONG X, LONG Y, RGB8 *);

//****************************************************************************

static ERROR GET_Handle(objBitmap *, APTR *);

static ERROR SET_Bkgd(objBitmap *, RGB8 *);
static ERROR SET_BkgdIndex(objBitmap *, LONG);
static ERROR SET_Trans(objBitmap *, RGB8 *);
static ERROR SET_TransIndex(objBitmap *, LONG);
static ERROR SET_Data(objBitmap *, UBYTE *);
static ERROR SET_Handle(objBitmap *, APTR);
static ERROR SET_Palette(objBitmap *, RGBPalette *);

static const FieldDef clDataFlags[] = {
   { "Public",  MEM_PUBLIC  }, { "Video",   MEM_VIDEO   },
   { "Blit",    MEM_TEXTURE }, { "NoClear", MEM_NO_CLEAR },
   { "Data",    NULL        },
   { NULL, 0 }
};

FDEF argsDrawUCPixel[]  = { { "Void", FD_VOID  }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_LONG }, { NULL, 0 } };
FDEF argsDrawUCRPixel[] = { { "Void", FD_VOID  }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_PTR|FD_RGB }, { NULL, 0 } };
FDEF argsReadUCPixel[]  = { { "Value", FD_LONG }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_PTR|FD_RESULT|FD_RGB }, { NULL, 0 } };
FDEF argsReadUCRPixel[] = { { "Void", FD_VOID  }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_PTR|FD_RESULT|FD_RGB }, { NULL, 0 } };
FDEF argsDrawUCRIndex[] = { { "Void", FD_VOID  }, { "Bitmap", FD_OBJECTPTR }, { "Data", FD_PTR }, { "Colour", FD_PTR|FD_RGB }, { NULL, 0 } };
FDEF argsReadUCRIndex[] = { { "Void", FD_VOID  }, { "Bitmap", FD_OBJECTPTR }, { "Data", FD_PTR }, { "Colour", FD_PTR|FD_RGB|FD_RESULT }, { NULL, 0 } };

//****************************************************************************
// Synopsis: RGBToValue(RGB8 *, RGBPalette *)
// Formula:  Score = Abs(BB1 - BB2) + Abs(GG1 - GG2) + Abs(RR1 - RR2)
//           The closer the score is to zero, the better the colour match.

static ULONG RGBToValue(RGB8 *RGB, RGBPalette *Palette)
{
   LONG BestMatch  = 0x7fffffff; // Highest possible value
   ULONG best = 0;
   WORD mred   = RGB->Red;
   WORD mgreen = RGB->Green;
   WORD mblue  = RGB->Blue;

   WORD i;
   for (i=Palette->AmtColours-1; i > 0; i--) {
      LONG Match = mred - Palette->Col[i].Red; // R1 - R2
      if (Match < 0) Match = -Match; // Abs(R1 - R2)

      WORD g = mgreen - Palette->Col[i].Green;
      if (g < 0) Match -= g; else Match += g;

      WORD b = mblue - Palette->Col[i].Blue;
      if (b < 0) Match -= b; else Match += b;

      if (Match < BestMatch) {
         if (!Match) return i;
         BestMatch  = Match;
         best = i;
      }
   }

   return best;
}

//****************************************************************************

static ERROR BITMAP_AccessObject(objBitmap *Self, APTR Void)
{
   if (Self->Head.Flags & NF_INITIALISED) CalculatePixelRoutines(Self);

   Self->Palette      = &Self->prvPaletteArray;
   Self->ColourFormat = &Self->prvColourFormat;

   #ifdef __xwindows__
      if (Self->x11.XShmImage IS TRUE) {
         Self->x11.ximage.obdata = (char *)&Self->x11.ShmInfo;
      }

      if ((Self->Flags & BMF_X11_DGA) and (glDGAAvailable)) {
         Self->Data = (UBYTE *)glDGAVideo;
      }
   #endif

   if (Self->DataMID) {
      if (AccessMemory(Self->DataMID, MEM_READ_WRITE|MEM_NO_BLOCKING, 1000, (APTR *)&Self->Data) != ERR_Okay) {
         return ERR_AccessMemory;
      }
      #ifdef __xwindows__
         Self->x11.ximage.data = (char *)Self->Data;
         Self->x11.ShmInfo.shmaddr = (char *)Self->Data;
      #endif
   }

   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Clear: Clears a bitmap's image to a colour of black.

Clearing a bitmap wipes away its graphical contents by drawing a blank area over its existing graphics.  The colour of
the blank area is determined by the #BkgdRGB field.  To clear a bitmap to a different colour, use the #DrawRectangle()
method instead.

If the bitmap supports alpha blending, the alpha blend bits will be reset to 'clear' status.

*****************************************************************************/

static ERROR BITMAP_Clear(objBitmap *Self, APTR Void)
{
#ifdef _GLES_
   if (Self->DataFlags & MEM_VIDEO) {
      if (!lock_graphics_active(__func__)) {
         glClearColorx(Self->BkgdRGB.Red, Self->BkgdRGB.Green, Self->BkgdRGB.Blue, 255);
         glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
         unlock_graphics();
         return ERR_Okay;
      }
      else return ERR_LockFailed;
   }
#endif

   LONG opacity = Self->Opacity;
   Self->Opacity = 255;
   gfxDrawRectangle(Self, 0, 0, Self->Width, Self->Height, Self->BkgdIndex, BAF_FILL);
   Self->Opacity = opacity;
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Compress: Compresses bitmap data to save memory.

A bitmap can be compressed with the CompressBitmap method to save memory when the bitmap is not in use.  This is useful
if a large bitmap needs to be stored in memory and it is anticipated that the bitmap will be used infrequently.

Once a bitmap is compressed, its image data is invalid.  Any attempt to access the bitmap's image data will likely
result in a memory access fault.  The image data will remain invalid until the #Decompress() method is
called to restore the bitmap to its original state.

The BMF_COMPRESSED bit will be set in the #Flags field after a successful call to this function to indicate that the
bitmap is compressed.

-INPUT-
int Level: Level of compression.  Zero uses a default setting (recommended), the maximum is 10.

-ERRORS-
Okay
NullArgs
AllocMemory
ReallocMemory
CreateObject: A Compression object could not be created.
-END-

*****************************************************************************/

static ERROR BITMAP_Compress(objBitmap *Self, struct bmpCompress *Args)
{
   parasol::Log log;

   if (!Args) return log.warning(ERR_NullArgs);

   if (Self->DataFlags & (MEM_VIDEO|MEM_TEXTURE)) {
      log.warning("Cannot compress video bitmaps.");
      return ERR_Failed;
   }

   if (Self->Size < 8192) return ERR_Okay;

   log.traceBranch("");

   if (Self->prvCompressMID) {
      // If the original compression object still exists, all we are going to do is free up the raw bitmap data.

      if ((Self->DataMID) and (Self->prvAFlags & BF_DATA)) {
         if (Self->Data) { ReleaseMemoryID(Self->DataMID); Self->Data = NULL; }
         FreeResourceID(Self->DataMID);
         Self->DataMID = 0;
      }

      return ERR_Okay;
   }

   ERROR error = ERR_Okay;
   if (!glCompress) {
      if (CreateObject(ID_COMPRESSION, NULL, &glCompress, TAGEND) != ERR_Okay) {
         return log.warning(ERR_CreateObject);
      }
      SetOwner(glCompress, glModule);
   }

   APTR buffer;
   if (!AllocMemory(Self->Size, MEM_NO_CLEAR, &buffer, NULL)) {
      struct cmpCompressBuffer cbuf;
      cbuf.Input      = Self->Data;
      cbuf.InputSize  = Self->Size;
      cbuf.Output     = buffer;
      cbuf.OutputSize = Self->Size;
      if (!Action(MT_CmpCompressBuffer, glCompress, &cbuf)) {
         APTR data;
         if (!AllocMemory(cbuf.Result, MEM_NO_CLEAR|Self->Head.MemFlags, &data, &Self->prvCompressMID)) {
            CopyMemory(buffer, data, cbuf.Result);
            FreeResource(buffer);
            ReleaseMemoryID(Self->prvCompressMID);
         }
         else error = ERR_ReallocMemory;
      }
      else error = ERR_Failed;
   }
   else error = ERR_AllocMemory;

   if (!error) { // Free the original data
      if ((Self->DataMID) and (Self->prvAFlags & BF_DATA)) {
         if (Self->Data) { ReleaseMemoryID(Self->DataMID); Self->Data = NULL; }
         FreeResourceID(Self->DataMID);
         Self->DataMID = 0;
      }

      Self->Flags |= BMF_COMPRESSED;
   }

   return error;
}

/*****************************************************************************

-METHOD-
CopyArea: Copies a rectangular area from one bitmap to another.

This method is a proxy for ~Display.CopyArea().

-INPUT-
obj(Bitmap) DestBitmap: Pointer to the destination bitmap.
int Flags:  Special flags.
int X: The horizontal position of the area to be copied.
int Y: The vertical position of the area to be copied.
int Width:  The width of the area.
int Height: The height of the area.
int XDest:  The horizontal position to copy the area to.
int YDest:  The vertical position to copy the area to.

-ERRORS-
Okay
NullArgs
Mismatch: The destination bitmap is not a close enough match to the source bitmap in order to perform the operation.

*****************************************************************************/

static ERROR BITMAP_CopyArea(objBitmap *Self, struct bmpCopyArea *Args)
{
   if (Args) return gfxCopyArea(Self, Args->DestBitmap, Args->Flags, Args->X, Args->Y, Args->Width, Args->Height, Args->XDest, Args->YDest);
   else return ERR_NullArgs;
}

/*****************************************************************************

-METHOD-
CopyStretch: Copies a rectangular area from one bitmap to another with stretching.

This method is a proxy for ~Display.CopyStretch().

-INPUT-
obj(Bitmap) DestBitmap: Pointer to the destination bitmap.
int Flags:  Special flags.
int X: The horizontal position of the area to be copied.
int Y: The vertical position of the area to be copied.
int Width:  The width of the source area.
int Height: The height of the source area.
int XDest:  The horizontal position to copy the area to.
int YDest:  The vertical position to copy the area to.
int DestWidth:  The width to use for the destination area.
int DestHeight: The height to use for the destination area.

-ERRORS-
Okay:
Args:     The DestBitmap argument was not specified.
Mismatch: The destination bitmap is not a close enough match to the source bitmap in order to perform the copy.

*****************************************************************************/

static ERROR BITMAP_CopyStretch(objBitmap *Self, struct bmpCopyStretch *Args)
{
   return gfxCopyStretch(Self, Args->DestBitmap, Args->Flags, Args->X, Args->Y, Args->Width, Args->Height, Args->XDest, Args->YDest, Args->DestWidth, Args->DestHeight);
}

/*****************************************************************************

-METHOD-
Decompress: Decompresses a compressed bitmap.

The Decompress method is used to restore a compressed bitmap to its original state.  If the bitmap is not
compressed, the method does nothing.

By default the original compression data will be destroyed, however it can be retained by setting the RetainData
argument to TRUE.  Retaining the data will allow it to be decompressed on consecutive occasions.  Because both the raw
and compressed image data will be held in memory, the next call to #Compress() will remove the raw image data from
memory while retaining the original compressed data without starting a re-compression process.

-INPUT-
int RetainData: Retains the compression data if TRUE.

-ERRORS-
Okay
AllocMemory: Insufficient memory in recreating the bitmap data buffer.

*****************************************************************************/

static ERROR BITMAP_Decompress(objBitmap *Self, struct bmpDecompress *Args)
{
   parasol::Log log;
   struct cmpDecompressBuffer dbuf;
   APTR data;
   ERROR error;

   if (!Self->prvCompressMID) return ERR_Okay;

   log.msg(VLF_BRANCH|VLF_EXTAPI, "Size: %d, Retain: %d", Self->Size, (Args) ? Args->RetainData : FALSE);

   // Note: If the decompression fails, we'll keep the bitmap data in memory in order to stop code from failing if it
   // accesses the Data address following attempted decompression.

   if (!Self->Data) {
      if (!AllocMemory(Self->Size, MEM_NO_BLOCKING|MEM_NO_POOL|MEM_NO_CLEAR|Self->Head.MemFlags|Self->DataFlags, &Self->Data, &Self->DataMID)) {
         Self->prvAFlags |= BF_DATA;
      }
      else return log.warning(ERR_AllocMemory);
   }

   if (!glCompress) {
      if (CreateObject(ID_COMPRESSION, NULL, &glCompress, TAGEND) != ERR_Okay) {
         return log.warning(ERR_CreateObject);
      }
      SetOwner(glCompress, glModule);
   }

   if (!(error = AccessMemory(Self->prvCompressMID, MEM_READ, 1000, &data))) {
      dbuf.Input      = data;
      dbuf.Output     = Self->Data;
      dbuf.OutputSize = Self->Size;
      error = Action(MT_CmpDecompressBuffer, glCompress, &dbuf);
      if (error IS ERR_BufferOverflow) error = ERR_Okay;
      ReleaseMemoryID(Self->prvCompressMID);
   }

   if ((Args) and (Args->RetainData IS TRUE)) {
      // Keep the source compression data
   }
   else {
      // Remove the source compression data
      FreeResourceID(Self->prvCompressMID);
      Self->prvCompressMID = NULL;
      Self->Flags &= ~BMF_COMPRESSED;
   }

   return error;
}

/*****************************************************************************

-ACTION-
CopyData: Copies bitmap image data to other bitmaps with colour remapping enabled.

This action will copy the image of the bitmap to any other initialised bitmap that you specify.  Support for copying
the image data to other object class types is not provided.

This action features automatic clipping and remapping, for occasions where the bitmaps do not match up in size or colour.

*****************************************************************************/

static ERROR BITMAP_CopyData(objBitmap *Self, struct acCopyData *Args)
{
   parasol::Log log;
   objBitmap *Dest;
   LONG MaxHeight;

   if ((!Args) or (!Args->DestID)) return log.warning(ERR_NullArgs);

   if (!AccessObject(Args->DestID, 3000, &Dest)) {
      if ((Dest->Head.ClassID != ID_BITMAP)) {
         ReleaseObject(Dest);
         return log.warning(ERR_Args);
      }

      if (Self->Height > Dest->Height) MaxHeight = Dest->Height;
      else MaxHeight = Self->Height;

      if (Self->Width >= Dest->Width) { // Source is wider or equal to the destination
         gfxCopyArea(Self, Dest, NULL, 0, 0, Dest->Width, MaxHeight, 0, 0);
      }
      else { // The destination is wider than the source.  Cpoy the source first, then clear the exposed region on the right.
         gfxCopyArea(Self, Dest, NULL, 0, 0, Self->Width, MaxHeight, 0, 0);
         gfxDrawRectangle(Dest, Self->Width, 0, Dest->Width - Self->Width, MaxHeight, Dest->BkgdIndex, BAF_FILL);
      }

      // If the destination height is greater, we will need to clear the pixels trailing at the bottom.

      if (Self->Height < Dest->Height) {
         gfxDrawRectangle(Dest, 0, Self->Height, Dest->Width, Dest->Height - Self->Height, Dest->BkgdIndex, BAF_FILL);
      }

      ReleaseObject(Dest);
      return ERR_Okay;
   }
   else return log.warning(ERR_ExclusiveDenied);
}

/*****************************************************************************

-ACTION-
Draw: Clears a bitmap's image to its assigned background colour.

*****************************************************************************/

static ERROR BITMAP_Draw(objBitmap *Self, APTR Void)
{
   gfxDrawRectangle(Self, 0, 0, Self->Width, Self->Height, Self->BkgdIndex, BAF_FILL);
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
DrawLine: Draws a line to a bitmap.

This method is a proxy for ~Display.DrawLine().

-INPUT-
int X: Horizontal start coordinate.
int Y: Vertical start coordinate.
int XEnd:   Horizontal end coordinate.
int YEnd:   Vertical end coordinate.
uint Colour: The pixel colour to use for drawing the line.

-ERRORS-
Okay:
NullArgs:

*****************************************************************************/

static ERROR BITMAP_DrawLine(objBitmap *Self, struct bmpDrawLine *Args)
{
   if (Args) gfxDrawLine(Self, Args->X, Args->Y, Args->XEnd, Args->YEnd, Args->Colour);
   else return ERR_NullArgs;
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
DrawRectangle: Draws rectangles, both filled and unfilled.

This method draws both filled and unfilled rectangles.  The rectangle is drawn to the target bitmap at position (X, Y)
with dimensions determined by the specified Width and Height.  If the Fill argument is set to TRUE then the rectangle
will be filled, otherwise the rectangle's outline will be drawn.  The colour of the rectangle is determined by the
pixel value in the Colour argument.

-INPUT-
int X: The left-most coordinate of the rectangle.
int Y: The top-most coordinate of the rectangle.
int Width:  The width of the rectangle.
int Height: The height of the rectangle.
uint Colour: The colour index to use for the rectangle.
int(BAF) Flags:  Supports BAF_FILL and BAF_BLEND.

-ERRORS-
Okay
Args

*****************************************************************************/

static ERROR BITMAP_DrawRectangle(objBitmap *Self, struct bmpDrawRectangle *Args)
{
   if (!Args) return ERR_NullArgs;
   gfxDrawRectangle(Self, Args->X, Args->Y, Args->Width, Args->Height, Args->Colour, Args->Flags);
   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Flip: Flips a bitmap around the horizontal or vertical axis.

This method is used to flip bitmap images on their horizontal or vertical axis.

-INPUT-
int(FLIP) Orientation: Set to either FLIP_HORIZONTAL or FLIP_VERTICAL.

-ERRORS-
Okay
Args
NullArgs

*****************************************************************************/

static ERROR BITMAP_Flip(objBitmap *Self, struct bmpFlip *Args)
{
   parasol::Log log;

   if (!Args) return log.warning(ERR_NullArgs);

   // NB: A faster way to flip a Bitmap would be to use CopyArea() to do the transfer in strips, but would require a
   // temporary memory area to hold the information.

   if (Args->Orientation IS FLIP_HORIZONTAL) {
      if (!LockSurface(Self, SURFACE_READWRITE)) {
         for (LONG y=0; y < Self->Height/2; y++) {
            for (LONG x=0; x < Self->Width; x++) {
               LONG c1 = Self->ReadUCPixel(Self, x, Self->Height - y - 1);
               LONG c2 = Self->ReadUCPixel(Self, x, y);
               Self->DrawUCPixel(Self, x, y, c1);
               Self->DrawUCPixel(Self, x, Self->Height - y - 1, c2);
            }
         }
         UnlockSurface(Self);
      }
   }
   else if (Args->Orientation IS FLIP_VERTICAL) {
      if (!LockSurface(Self, SURFACE_READWRITE)) {
         // Palette based Bitmap
         for (LONG x=0; x < Self->Width/2; x++) {
            for (LONG y=0; y < Self->Height; y++) {
               LONG c1 = Self->ReadUCPixel(Self, Self->Width - x - 1, y); // Right pixel
               LONG c2 = Self->ReadUCPixel(Self, x, y); // Left pixel
               Self->DrawUCPixel(Self, Self->Width - x - 1, y, c2);
               Self->DrawUCPixel(Self, x, y, c1);
            }
         }
         UnlockSurface(Self);
      }
   }
   else return log.warning(ERR_Args);

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
Flood: Fills a bitmap area with a specific colour.

This method performs a flood-fill operation on a bitmap.  It requires an X and Y value that will target a pixel to
initiate the flood-fill operation.  The colour value indicated in RGB will be used to change the targeted pixel and all
adjacent pixels that share the targeted pixel's colour.

The speed of the algorithm is wholly dependent on the amount of pixels that need to be filled, although hardware
support may be used for filling if it is available.

-INPUT-
int X: The horizontal point to start the flood fill.
int Y: The vertical point to start the flood fill.
uint Colour: The colour index to use for the fill.

-ERRORS-
Okay
NullArgs

*****************************************************************************/

static void flood_fill(objBitmap *, LONG, LONG, ULONG, ULONG);

static ERROR BITMAP_Flood(objBitmap *Self, struct bmpFlood *Args)
{
   parasol::Log log;

   if (!Args) return log.warning(ERR_NullArgs);

   if ((Args->X >= Self->Clip.Left) and (Args->X < Self->Clip.Right) AND
       (Args->Y >= Self->Clip.Top) and (Args->Y < Self->Clip.Bottom)) {

      ULONG background = Self->ReadUCPixel(Self, Args->X, Args->Y);;

      if (background != Args->Colour) {
         flood_fill(Self, Args->X, Args->Y, Args->Colour, background);
      }
   }

   return ERR_Okay;
}

static void flood_fill(objBitmap *Self, LONG X, LONG Y, ULONG FillColour, ULONG Background)
{
   if ((X >= Self->Clip.Left) and (X < Self->Clip.Right) AND
       (Y >= Self->Clip.Top) and (Y < Self->Clip.Bottom)) {

      ULONG background = Self->ReadUCPixel(Self, X, Y);

      if (background IS Background) {
         Self->DrawUCPixel(Self, X, Y, FillColour);
         flood_fill(Self, X + 1, Y, FillColour, Background);
         flood_fill(Self, X - 1, Y, FillColour, Background);
         flood_fill(Self, X, Y + 1, FillColour, Background);
         flood_fill(Self, X, Y - 1, FillColour, Background);
      }
   }
}

/******************************************************************************
-ACTION-
Flush: Flushes pending graphics operations and returns when the accelerator is idle.

The Flush action is provided for you to ensure that your graphics operations are synchronised with the graphics
accelerator.  Synchronisation is essential prior to drawing to the bitmap with the CPU.  Failure to synchronise may
result in corruption in the bitmap's graphics display.

You do not have to use this function if you stick to using the graphics functions that are provided in the Bitmap class.
-END-
******************************************************************************/

static ERROR BITMAP_Flush(objBitmap *Self, APTR Void)
{
#ifdef _GLES_
   if (!lock_graphics_active(__func__)) {
      glFlush();
      unlock_graphics();
   }
#endif
   return ERR_Okay;
}

//****************************************************************************

static ERROR BITMAP_Free(objBitmap *Self, APTR Void)
{
   #ifdef __xwindows__
      if (Self->x11.XShmImage IS TRUE) {
         // Tell the X11 server to detach from the memory block
         XShmDetach(XDisplay, &Self->x11.ShmInfo);
         Self->x11.XShmImage = FALSE;
      }
   #endif

   if ((Self->DataMID) and (Self->prvAFlags & BF_DATA)) {
      if (Self->Data) { ReleaseMemoryID(Self->DataMID); Self->Data = NULL; }
      FreeResourceID(Self->DataMID);
      Self->DataMID = 0;
   }

   if (Self->prvCompressMID) { FreeResourceID(Self->prvCompressMID); Self->prvCompressMID = 0; }

   if (Self->ResolutionChangeHandle) {
      UnsubscribeEvent(Self->ResolutionChangeHandle);
      Self->ResolutionChangeHandle = NULL;
   }

   #ifdef __xwindows__
      if (Self->x11.drawable) {
         if (XDisplay) XFreePixmap(XDisplay, Self->x11.drawable);
         Self->x11.drawable = NULL;
      }

      if (Self->x11.readable) {
         XDestroyImage(Self->x11.readable);
         Self->x11.readable = NULL;
      }
   #endif

   #ifdef _WIN32
      if (Self->win.Drawable) {
         winDeleteDC(Self->win.Drawable);
         Self->win.Drawable = NULL;
      }
   #endif

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
GetColour: Converts Red, Green, Blue components into a single colour value.

The GetColour method is used to convert Red, Green and Blue colour components into a single colour index that can be
used for directly writing colours to the bitmap.  The result is returned in the Colour parameter.

-INPUT-
int Red:    Red component from 0 - 255.
int Green:  Green component from 0 - 255.
int Blue:   Blue component value from 0 - 255.
int Alpha:  Alpha component value from 0 - 255.
&uint Colour: The resulting colour value will be returned here.

-ERRORS-
Okay
NullArgs
-END-

*****************************************************************************/

static ERROR BITMAP_GetColour(objBitmap *Self, struct bmpGetColour *Args)
{
   if (!Args) return ERR_NullArgs;

   if (Self->BitsPerPixel > 8) {
      Args->Colour = PackPixelA(Self, Args->Red, Args->Green, Args->Blue, Args->Alpha);
   }
   else {
      struct RGB8 rgb;
      rgb.Red   = Args->Red;
      rgb.Green = Args->Green;
      rgb.Blue  = Args->Blue;
      rgb.Alpha = Args->Alpha;
      Args->Colour = RGBToValue(&rgb, Self->Palette);
   }

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Init: Initialises a bitmap.

This action will initialise a bitmap object so that it is ready for use.  If the bitmap #Data field has not
been specified, a memory block will be allocated and placed in this field.  The type of memory that is allocated is
dependent on the bitmap #DataFlags field.  If you have not specified a memory type, you will get a default of
MEM_DATA.  For a display compatible bitmap use MEM_VIDEO.  If you just want to store a bitmap in fast writeable memory,
use MEM_TEXTURE.

This action will not work unless you have defined the #Width and #Height fields of the bitmap at a minimum.
-END-
*****************************************************************************/

static ERROR BITMAP_Init(objBitmap *Self, APTR Void)
{
   parasol::Log log;
   #ifdef __xwindows__
      WORD alignment;
   #endif
   ERROR error;

   if (acQuery(Self) != ERR_Okay) return log.warning(ERR_Query);

   log.branch("Size: %dx%d @ %d bit, %d bytes, Mem: $%.8x, Flags: $%.8x", Self->Width, Self->Height, Self->BitsPerPixel, Self->BytesPerPixel, Self->DataFlags, Self->Flags);

   if (Self->Clip.Left < 0) Self->Clip.Left = 0;
   if (Self->Clip.Top < 0)  Self->Clip.Top  = 0;
   if ((Self->Clip.Right > Self->Width)  or (Self->Clip.Right < 1)) Self->Clip.Right = Self->Width;
   if ((Self->Clip.Bottom > Self->Height) or (Self->Clip.Bottom < 1)) Self->Clip.Bottom = Self->Height;

   // If the Bitmap is 15 or 16 bit, make corrections to the background values

   if (Self->BitsPerPixel IS 16) {
      Self->TransRGB.Red   &= 0xf8;
      Self->TransRGB.Green &= 0xfc;
      Self->TransRGB.Blue  &= 0xf8;

      Self->BkgdRGB.Red   &= 0xf8;
      Self->BkgdRGB.Green &= 0xfc;
      Self->BkgdRGB.Blue  &= 0xf8;
   }
   else if (Self->BitsPerPixel IS 15) {
      Self->TransRGB.Red   &= 0xf8;
      Self->TransRGB.Green &= 0xf8;
      Self->TransRGB.Blue  &= 0xf8;

      Self->BkgdRGB.Red   &= 0xf8;
      Self->BkgdRGB.Green &= 0xf8;
      Self->BkgdRGB.Blue  &= 0xf8;
   }

#ifdef __xwindows__

   Self->DataFlags &= ~MEM_TEXTURE; // Blitter memory not available in X11

   if ((!Self->DataMID) and (!Self->Data)) {
      if (!(Self->Flags & BMF_NO_DATA)) {
         Self->DataFlags &= ~MEM_VIDEO; // Video memory not available for allocation in X11 (may be set to identify X11 windows only)

         if (!Self->Size) {
            log.warning("The Bitmap has no Size (there is a dimensional error).");
            return ERR_FieldNotSet;
         }

         //if (Self->DataFlags & MEM_VIDEO) {
         //   Self->x11.drawable = XCreatePixmap(XDisplay, DefaultRootWindow(XDisplay), Self->Width, Self->Height, Self->BitsPerPixel);
         //   if (!Self->x11.drawable) {
         //      log.warning("X11 failed to allocate a Pixmap for the video based Bitmap.");
         //      return ERR_Failed;
         //   }
         //}
         //else {

            if (Self->x11.XShmImage IS FALSE) {
               log.msg("Allocating a memory based XImage.");
               if (!AllocMemory(Self->Size, MEM_NO_BLOCKING|MEM_NO_POOL|MEM_NO_CLEAR|Self->Head.MemFlags|Self->DataFlags, &Self->Data, &Self->DataMID)) {
                  Self->prvAFlags |= BF_DATA;

                  if (Self->LineWidth & 0x0001) alignment = 8;
                  else if (Self->LineWidth & 0x0002) alignment = 16;
                  else alignment = 32;

                  if (!glHeadless) {
                     Self->x11.ximage.width            = Self->Width;  // Image width
                     Self->x11.ximage.height           = Self->Height; // Image height
                     Self->x11.ximage.xoffset          = 0;            // Number of pixels offset in X direction
                     Self->x11.ximage.format           = ZPixmap;      // XYBitmap, XYPixmap, ZPixmap
                     Self->x11.ximage.data             = (char *)Self->Data; // Pointer to image data
                     if (glX11ShmImage) Self->x11.ximage.obdata = (char *)&Self->x11.ShmInfo; // Magic pointer for the XShm extension
                     Self->x11.ximage.byte_order       = 0;            // LSBFirst / MSBFirst
                     Self->x11.ximage.bitmap_unit      = alignment;    // Quant. of scanline - 8, 16, 32
                     Self->x11.ximage.bitmap_bit_order = 0;            // LSBFirst / MSBFirst
                     Self->x11.ximage.bitmap_pad       = alignment;    // 8, 16, 32, either XY or Zpixmap
                     if (Self->BitsPerPixel IS 32) Self->x11.ximage.depth = 24;
                     else Self->x11.ximage.depth = Self->BitsPerPixel;            // Actual bits per pixel
                     Self->x11.ximage.bytes_per_line   = Self->LineWidth;         // Accelerator to next line
                     Self->x11.ximage.bits_per_pixel   = Self->BytesPerPixel * 8; // Bits per pixel-group
                     Self->x11.ximage.red_mask         = 0;
                     Self->x11.ximage.green_mask       = 0;
                     Self->x11.ximage.blue_mask        = 0;

                     XInitImage(&Self->x11.ximage);

                     // If the XShm extension is available, try using it.  Using XShm allows the
                     // X11 server to copy image memory straight to the display rather than
                     // having it messaged.

                     if (glX11ShmImage) {
                        struct MemInfo meminfo;

                        if ((!MemoryIDInfo(Self->DataMID, &meminfo)) and (meminfo.Handle)) {
                           Self->x11.ShmInfo.shmid    = meminfo.Handle;
                           Self->x11.ShmInfo.readOnly = False;
                           Self->x11.ShmInfo.shmaddr  = (char *)Self->Data;

                           // Attach the memory block to the X11 server

                           if (XShmAttach(XDisplay, &Self->x11.ShmInfo)) {
                              Self->x11.XShmImage = TRUE;
                              XSync(XDisplay, TRUE);
                           }
                           else log.warning("XShmAttach() failed.");
                        }
                     }
                  }
               }
               else return log.warning(ERR_AllocMemory);
            }
         //}
      }
   }

   if (!glHeadless) XSync(XDisplay, False);

#elif _WIN32

   Self->DataFlags &= ~MEM_TEXTURE; // Video buffer memory not available in Win32

   if ((!Self->DataMID) and (!Self->Data)) {
      if (!(Self->Flags & BMF_NO_DATA)) {
         if (!Self->Size) {
            log.warning("The Bitmap has no Size (there is a dimensional error).");
            return ERR_FieldNotSet;
         }

         if (Self->DataFlags & MEM_VIDEO) {
            Self->prvAFlags |= BF_WINVIDEO;
            if ((Self->win.Drawable = winCreateCompatibleDC())) {
            }
            else return log.warning(ERR_SystemCall);
         }
         else if (!AllocMemory(Self->Size, MEM_NO_BLOCKING|MEM_NO_POOL|MEM_NO_CLEAR|Self->Head.MemFlags|Self->DataFlags, &Self->Data, &Self->DataMID)) {
            Self->prvAFlags |= BF_DATA;
         }
         else return log.warning(ERR_AllocMemory);
      }
      else {
         if (Self->DataFlags & MEM_VIDEO) Self->prvAFlags |= BF_WINVIDEO;
      }
   }

#elif __snap__

   // Bitmaps that are placed in video buffers need to beware of resolution changes.  If a resolution switch is about
   // to occur, the bitmap needs to be pushed to system RAM because the video memory will be cleared.

   if (Self->DataFlags & MEM_TEXTURE) {
      SubscribeEvent(EVID_DISPLAY_RESOLUTION_CHANGE, &resolution_change, Self, &Self->ResolutionChangeHandle);
   }

#elif _GLES_
   // MEM_VIDEO + BMF_NO_DATA: The bitmap represents the OpenGL display.  No data area will be allocated as direct access to the OpenGL video frame buffer is not possible.
   // MEM_VIDEO: Not currently used as a means of allocating a particular type of OpenGL buffer.
   // MEM_TEXTURE:  The bitmap is to be used as an OpenGL texture or off-screen buffer.  The bitmap content is temporary - i.e. the content can be dumped by the graphics driver if the video display changes.
   // MEM_DATA:  The bitmap resides in regular CPU accessible memory.

   if ((!Self->DataMID) and (!Self->Data)) {
      if (!(Self->Flags & BMF_NO_DATA)) {
         if (Self->Size <= 0) {
            log.warning("The Bitmap has no Size (there is a dimensional error).");
            return ERR_FieldNotSet;
         }

         if (Self->DataFlags & MEM_VIDEO) {
            // Do nothing - the bitmap merely represents the video display and does not hold content.
         }
         else if (Self->DataFlags & MEM_TEXTURE) {
            // Blittable bitmaps are fast, but their content is temporary.  It is not possible to use the CPU on this
            // bitmap type - the developer should use MEM_DATA if that is desired.

            log.warning("Support for MEM_TEXTURE not included yet.");
            return ERR_NoSupport;
         }
         else if (!AllocMemory(Self->Size, Self->Head.MemFlags|Self->DataFlags|MEM_NO_BLOCKING|MEM_NO_POOL|MEM_NO_CLEAR, &Self->Data, &Self->DataMID)) {
            Self->prvAFlags |= BF_DATA;
         }
         else return ERR_AllocMemory;
      }
   }

   if (Self->DataFlags & (MEM_VIDEO|MEM_TEXTURE)) Self->Flags |= BMF_2DACCELERATED;

#else
   #error Platform requires memory allocation routines for the Bitmap class.
#endif

   // Determine the correct pixel format for the bitmap

#ifdef __xwindows__

   if ((!glHeadless) and (Self->x11.drawable)) {
      XVisualInfo visual, *info;
      LONG items;
      visual.bits_per_rgb = Self->BytesPerPixel * 8;
      if ((info = XGetVisualInfo(XDisplay, VisualBitsPerRGBMask, &visual, &items))) {
         gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, info->red_mask, info->green_mask, info->blue_mask, 0xff000000);
         XFree(info);
      }
      else gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);
   }
   else gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, Self->x11.ximage.red_mask, Self->x11.ximage.green_mask, Self->x11.ximage.blue_mask, 0xff000000);

#elif _WIN32

   if (Self->DataFlags & MEM_VIDEO) {
      LONG red, green, blue, alpha;

      if (winGetPixelFormat(&red, &green, &blue, &alpha) IS ERR_Okay) {
         gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, red, green, blue, alpha);
      }
      else gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);
   }
   else gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);

#elif _GLES_

   if (Self->BitsPerPixel >= 24) gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0x0000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
   else if (Self->BitsPerPixel IS 16) gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0xf800, 0x07e0, 0x001f, 0x0000);
   else if (Self->BitsPerPixel IS 15) gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0x7c00, 0x03e0, 0x001f, 0x0000);
   else gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);

#else

   gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);

#endif

   if ((error = CalculatePixelRoutines(Self)) != ERR_Okay) return error;

   if (Self->BitsPerPixel > 8) {
      Self->TransIndex = (((Self->TransRGB.Red   >> Self->prvColourFormat.RedShift)   & Self->prvColourFormat.RedMask)   << Self->prvColourFormat.RedPos) |
                         (((Self->TransRGB.Green >> Self->prvColourFormat.GreenShift) & Self->prvColourFormat.GreenMask) << Self->prvColourFormat.GreenPos) |
                         (((Self->TransRGB.Blue  >> Self->prvColourFormat.BlueShift)  & Self->prvColourFormat.BlueMask)  << Self->prvColourFormat.BluePos) |
                         (((255 >> Self->prvColourFormat.AlphaShift) & Self->prvColourFormat.AlphaMask) << Self->prvColourFormat.AlphaPos);

      Self->BkgdIndex = (((Self->BkgdRGB.Red   >> Self->prvColourFormat.RedShift)   & Self->prvColourFormat.RedMask)   << Self->prvColourFormat.RedPos) |
                        (((Self->BkgdRGB.Green >> Self->prvColourFormat.GreenShift) & Self->prvColourFormat.GreenMask) << Self->prvColourFormat.GreenPos) |
                        (((Self->BkgdRGB.Blue  >> Self->prvColourFormat.BlueShift)  & Self->prvColourFormat.BlueMask)  << Self->prvColourFormat.BluePos) |
                        (((255 >> Self->prvColourFormat.AlphaShift) & Self->prvColourFormat.AlphaMask) << Self->prvColourFormat.AlphaPos);
   }

   if ((!(Self->Flags & BMF_NO_DATA)) and (Self->Flags & BMF_CLEAR)) {
      log.msg("Clearing Bitmap...");
      acClear(Self);
   }

   // Sanitise the Flags field

   if (Self->BitsPerPixel < 32) Self->Flags &= ~BMF_ALPHA_CHANNEL;
   Self->Flags &= ~BMF_NO_DATA;

   //log.msg("Red: %.2x/%d/%d , Green: %.2x/%d/%d",
   //   Self->prvColourFormat.RedMask,   Self->prvColourFormat.RedShift,   Self->prvColourFormat.RedPos,
   //   Self->prvColourFormat.GreenMask, Self->prvColourFormat.GreenShift, Self->prvColourFormat.GreenPos);

   //log.msg("Blue: %.2x/%d/%d , Alpha: %.2x/%d/%d",
   //   Self->prvColourFormat.BlueMask,  Self->prvColourFormat.BlueShift,  Self->prvColourFormat.BluePos,
   //   Self->prvColourFormat.AlphaMask, Self->prvColourFormat.AlphaShift, Self->prvColourFormat.AlphaPos);

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Lock: Locks the bitmap surface so that you can manipulate the graphics directly.
-END-
*****************************************************************************/

static ERROR BITMAP_Lock(objBitmap *Self, APTR Void)
{
#ifdef __xwindows__
   if (Self->x11.drawable) {
      WORD alignment;
      LONG size, bpp;

      // If there is an existing readable area, try to reuse it if possible

      if (Self->x11.readable) {
         if ((Self->x11.readable->width >= Self->Width) and (Self->x11.readable->height >= Self->Height)) {
            XGetSubImage(XDisplay, Self->x11.drawable, Self->XOffset + Self->Clip.Left,
               Self->YOffset + Self->Clip.Top, Self->Clip.Right - Self->Clip.Left,
               Self->Clip.Bottom - Self->Clip.Top, 0xffffffff, ZPixmap, Self->x11.readable,
               Self->XOffset + Self->Clip.Left, Self->YOffset + Self->Clip.Top);
            return ERR_Okay;
         }
         else XDestroyImage(Self->x11.readable);
      }

      // Generate a fresh XImage from the current drawable

      if (Self->LineWidth & 0x0001) alignment = 8;
      else if (Self->LineWidth & 0x0002) alignment = 16;
      else alignment = 32;

      if (Self->Type IS BMP_PLANAR) {
         size = Self->ByteWidth * Self->Height * Self->BitsPerPixel;
      }
      else size = Self->ByteWidth * Self->Height;

      Self->Data = (UBYTE *)malloc(size);

      if ((bpp = Self->BitsPerPixel) IS 32) bpp = 24;

      if ((Self->x11.readable = XCreateImage(XDisplay, CopyFromParent, bpp,
           ZPixmap, 0, (char *)Self->Data, Self->Width, Self->Height, alignment, Self->ByteWidth))) {
         XGetSubImage(XDisplay, Self->x11.drawable, Self->XOffset + Self->Clip.Left,
            Self->YOffset + Self->Clip.Top, Self->Clip.Right - Self->Clip.Left,
            Self->Clip.Bottom - Self->Clip.Top, 0xffffffff, ZPixmap, Self->x11.readable,
            Self->XOffset + Self->Clip.Left, Self->YOffset + Self->Clip.Top);
      }
      else return ERR_Failed;
   }

   return ERR_Okay;

#else

   return LockSurface(Self, SURFACE_READWRITE);

#endif
}

//****************************************************************************

static ERROR BITMAP_NewObject(objBitmap *Self, APTR Void)
{
   #define CBANK 5
   RGB8 *RGB;
   LONG i, j;

   Self->Palette      = &Self->prvPaletteArray;
   Self->ColourFormat = &Self->prvColourFormat;

   Self->Opacity = 255;

   // Generate the standard colour palette

   Self->Palette = &Self->prvPaletteArray;
   Self->Palette->AmtColours = 256;

   RGB = Self->Palette->Col;
   RGB++; // Skip the black pixel at the start

   for (i=0; i < 6; i++) {
      for (j=0; j < CBANK; j++) {
         RGB[(i*CBANK) + j].Red   = (i * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 0;
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
      }
   }

   for (i=6; i < 12; i++) {
      for (j=0; j < 5; j++) {
         RGB[(i*CBANK) + j].Red   = ((i-6) * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 51;
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
      }
   }

   for (i=12; i < 18; i++) {
      for (j=0; j < 5; j++) {
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
         RGB[(i*CBANK) + j].Red   = ((i-12) * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 102;
      }
   }

   for (i=18; i < 24; i++) {
      for (j=0; j < 5; j++) {
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
         RGB[(i*CBANK) + j].Red   = ((i-18) * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 153;
      }
   }

   for (i=24; i < 30; i++) {
      for (j=0; j < 5; j++) {
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
         RGB[(i*CBANK) + j].Red   = ((i-24) * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 204;
      }
   }

   for (i=30; i < 36; i++) {
      for (j=0; j < 5; j++) {
         RGB[(i*CBANK) + j].Blue  = (j + 1) * 255/CBANK;
         RGB[(i*CBANK) + j].Red   = ((i-30) * 255/CBANK);
         RGB[(i*CBANK) + j].Green = 255;
      }
   }

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Query: Fills a bitmap with pre-initialised/default values prior to initialisation.

This action will pre-initialise a bitmap object so that all the fields are filled out with default values.  It stops
short of allocating the bitmap's memory.

For this action to work properly you must have defined the Width and Height fields of the bitmap before making the
Query.  This function is intelligent enough to fill out the fields based on the information you have given it, e.g. if
you set the #BytesPerPixel field to 2 then it will determine that the bitmap is a 16 bit, 64k colour bitmap.
-END-
*****************************************************************************/

static ERROR BITMAP_Query(objBitmap *Self, APTR Void)
{
   parasol::Log log;
   objDisplay *display;
   OBJECTID display_id;
   LONG i;

   log.msg(VLF_BRANCH|VLF_EXTAPI, "Bitmap: %p, Depth: %d, Width: %d, Height: %d", Self, Self->BitsPerPixel, Self->Width, Self->Height);

   if ((Self->Width <= 0) or (Self->Height <= 0)) {
      return log.warning(ERR_InvalidDimension);
   }

   #ifdef _GLES_
      if (Self->DataFlags & MEM_TEXTURE) {
         // OpenGL requires bitmap textures to be a power of 2.

         LONG new_width = nearestPower(Self->Width);
         LONG new_height = nearestPower(Self->Height);

         if (new_width != Self->Width) {
            LogMsg("Extending bitmap width from %d to %d for OpenGL.", Self->Width, new_width);
            Self->Width = new_width;
         }

         if (new_height != Self->Height) {
            LogMsg("Extending bitmap height from %d to %d for OpenGL.", Self->Height, new_height);
            Self->Height = new_height;
         }
      }
   #endif

   // If the BMF_MASK flag is set then the programmer wants to use the Bitmap object as a 1 or 8-bit mask.

   if (Self->Flags & BMF_MASK) {
      if ((!Self->BitsPerPixel) and (!Self->AmtColours)) {
         Self->BitsPerPixel = 1;
         Self->AmtColours = 2;
         Self->Type = BMP_PLANAR;
      }
      else if (Self->AmtColours >= 256) {
         Self->AmtColours = 256;
         Self->Type = BMP_CHUNKY;
         // Change the palette to grey scale for alpha channel masks
         for (i=0; i < 256; i++) {
            Self->Palette->Col[i].Red   = i;
            Self->Palette->Col[i].Green = i;
            Self->Palette->Col[i].Blue  = i;
         }
      }
      Self->BytesPerPixel = 1;
   }

   // If no type has been set, use the type that is native to the system that Parasol is running on.

   if (!Self->Type) Self->Type = BMP_CHUNKY;

   if (Self->BitsPerPixel) {
      switch(Self->BitsPerPixel) {
         case 1:  Self->BytesPerPixel = 1; Self->AmtColours = 2; Self->Type = BMP_PLANAR; break;
         case 2:  Self->BytesPerPixel = 1; Self->AmtColours = 4; break;
         case 8:  Self->BytesPerPixel = 1; Self->AmtColours = 256; break;
         case 15: Self->BytesPerPixel = 2; Self->AmtColours = 32768; break;
         case 16: Self->BytesPerPixel = 2; Self->AmtColours = 65536; break;
         case 24: Self->BytesPerPixel = 3; Self->AmtColours = 16777216; break;
         case 32: Self->BytesPerPixel = 4; Self->AmtColours = 16777216; break;
      }
   }
   else if (Self->BytesPerPixel) {
      switch(Self->BytesPerPixel) {
         case 1:  Self->BitsPerPixel  = 8;  Self->AmtColours = 256; break;
         case 2:  Self->BitsPerPixel  = 16; Self->AmtColours = 65536; break;
         case 3:  Self->BitsPerPixel  = 24; Self->AmtColours = 16777216; break;
         case 4:  Self->BitsPerPixel  = 32; Self->AmtColours = 16777216; break;
         default: Self->BytesPerPixel = 1;  Self->BitsPerPixel = 8; Self->AmtColours = 256;
      }
   }

   // Ensure values for BitsPerPixel, AmtColours, BytesPerPixel are correct

   if (!Self->AmtColours) {
      if (Self->BitsPerPixel) {
         if (Self->BitsPerPixel <= 24) {
            Self->AmtColours = 1<<Self->BitsPerPixel;
            if (Self->AmtColours <= 256) Self->BytesPerPixel = 1;
            else if (Self->AmtColours <= 65536) Self->BytesPerPixel = 2;
            else Self->BytesPerPixel = 3;
         }
         else {
            Self->AmtColours = 16777216;
            Self->BytesPerPixel = 4;
         }
      }
      else {
         Self->AmtColours    = 16777216;
         Self->BitsPerPixel  = 32;
         Self->BytesPerPixel = 4;
#if 1
         LONG count = 1;
         if (!FindObject("SystemDisplay", ID_DISPLAY, FOF_INCLUDE_SHARED, &display_id, &count)) {
            if (!AccessObject(display_id, 3000, &display)) {
               Self->AmtColours    = display->Bitmap->AmtColours;
               Self->BytesPerPixel = display->Bitmap->BytesPerPixel;
               Self->BitsPerPixel  = display->Bitmap->BitsPerPixel;
               ReleaseObject(display);
            }
         }
#else
         DISPLAYINFO info;
         if (!get_display_info(0, &info)) {
            Self->AmtColours    = info.AmtColours;
            Self->BytesPerPixel = info.BytesPerPixel;
            Self->BitsPerPixel  = info.BitsPerPixel;
         }
#endif
      }
   }

   // Calculate ByteWidth, make sure it's word aligned

   if (Self->Type IS BMP_PLANAR) {
      Self->ByteWidth = (Self->Width + 7) / 8;
   }
   else Self->ByteWidth = Self->Width * Self->BytesPerPixel;

   // Initialise the line and plane module fields

   Self->LineWidth = Self->ByteWidth;
   Self->LineWidth = AlignLong(Self->LineWidth);
   Self->PlaneMod = Self->LineWidth * Self->Height;

#ifdef __xwindows__

   // If we have Direct Graphics Access, use the DGA values rather than our generic calculations for bitmap parameters.

   if ((Self->DataFlags & MEM_VIDEO) and (Self->x11.drawable)) {
      log.trace("LineWidth: %d, PixelLine: %d, BankSize: %d", Self->LineWidth, glDGAPixelsPerLine, glDGABankSize);
      if ((glDGAAvailable) and (glDGAPixelsPerLine)) {
         Self->LineWidth = glDGAPixelsPerLine * Self->BytesPerPixel;
         Self->PlaneMod = Self->LineWidth;
      }
   }

#endif

#ifdef _GLES_
   if ((Self->BitsPerPixel IS 8) and (Self->Flags & BMF_MASK)) Self->prvGLPixel = GL_ALPHA;
   else if (Self->BitsPerPixel <= 24) Self->prvGLPixel = GL_RGB;
   else Self->prvGLPixel = GL_RGBA;

   if (Self->BitsPerPixel IS 32) Self->prvGLFormat = GL_UNSIGNED_BYTE;
   else if (Self->BitsPerPixel IS 24) Self->prvGLFormat = GL_UNSIGNED_BYTE;
   else if (Self->BitsPerPixel <= 16) Self->prvGLFormat = GL_UNSIGNED_SHORT_5_6_5;
   else Self->prvGLFormat = GL_UNSIGNED_BYTE;
#endif

   // Calculate the total size of the bitmap

   if (Self->Type IS BMP_PLANAR) {
      Self->Size = Self->LineWidth * Self->Height * Self->BitsPerPixel;
   }
   else Self->Size = Self->LineWidth * Self->Height;

   Self->Flags |= BMF_QUERIED;
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Read: Reads raw image data from a bitmap object.
-END-
*****************************************************************************/

static ERROR BITMAP_Read(objBitmap *Self, struct acRead *Args)
{
   if (!Self->Data) return ERR_NoData;
   if ((!Args) or (!Args->Buffer)) return ERR_NullArgs;

   LONG len = Args->Length;
   if (Self->Position + len > Self->Size) len = Self->Size - Self->Position;
   CopyMemory(Self->Data + Self->Position, Args->Buffer, len);
   Self->Position += len;
   Args->Result = len;
   return ERR_Okay;
}

//****************************************************************************

static ERROR BITMAP_ReleaseObject(objBitmap *Self, APTR Void)
{
#ifdef __xwindows__
   XSync(XDisplay, False);
#endif

   if ((Self->Data) and (Self->DataMID)) { ReleaseMemoryID(Self->DataMID); Self->Data = NULL; }

#ifdef __xwindows__
   if (Self->x11.readable) { XDestroyImage(Self->x11.readable); Self->x11.readable = NULL; }
#endif

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Resize: Resizes a bitmap object's dimensions.

Resizing a bitmap allows you to change its dimensions in width and height, as well as depth.  The image data will be
retained during the resize, although cropping will occur if any of the dimensions are reduced.  Reducing the depth of
the bitmap will degrade the bitmap's quality.

Bitmap resizing can be time consuming due to the retention of the image data.  Processing time can be significantly
reduced by setting the VOLATILE flag in the bitmap #Flags field.
-END-
*****************************************************************************/

static ERROR BITMAP_Resize(objBitmap *Self, struct acResize *Args)
{
   parasol::Log log;
   MEMORYID datamid;
   LONG width, height, bytewidth, bpp, amtcolours, size;

   if (!Args) return log.warning(ERR_NullArgs);

   // Calculate new Bitmap values

   LONG origwidth  = Self->Width;
   LONG origheight = Self->Height;
   LONG origbpp    = Self->BitsPerPixel;

   if (Args->Width > 0) width = (LONG)Args->Width;
   else width = Self->Width;

   if (Args->Height > 0) height = (LONG)Args->Height;
   else height = Self->Height;

   if ((Args->Depth > 0) and (!(Self->Flags & BMF_FIXED_DEPTH))) bpp = (LONG)Args->Depth;
   else bpp = Self->BitsPerPixel;

   // If the NEVER_SHRINK option is set, the width and height may not be set to anything less than what is current.

   if (Self->Flags & BMF_NEVER_SHRINK) {
      if (width < Self->Width) width = Self->Width;
      if (height < Self->Height) height = Self->Height;
   }

   // Return if there is no change in the bitmap size

   if ((Self->Width IS width) and (Self->Height IS height) and (Self->BitsPerPixel IS bpp)) {
      return ERR_Okay|ERF_Notified;
   }

   // Calculate type-dependent values

   WORD bytesperpixel;
   switch(bpp) {
      case 1:  bytesperpixel = 1; amtcolours = 2; break;
      case 8:  bytesperpixel = 1; amtcolours = 256; break;
      case 15: bytesperpixel = 2; amtcolours = 32768; break;
      case 16: bytesperpixel = 2; amtcolours = 65536; break;
      case 24: bytesperpixel = 3; amtcolours = 16777216; break;
      case 32: bytesperpixel = 4; amtcolours = 16777216; break;
      default: bytesperpixel = bpp / 8;
               amtcolours = 1<<bpp;
   }

   if (Self->Type IS BMP_PLANAR) bytewidth = (width + (width % 16))/8;
   else bytewidth = width * bytesperpixel;

   LONG linewidth = AlignLong(bytewidth);
   LONG planemod = bytewidth * height;

   if (Self->Type IS BMP_PLANAR) size = linewidth * height * bpp;
   else size = linewidth * height;

   if (GetClassID(Self->Head.OwnerID) IS ID_DISPLAY) goto setfields;

#ifdef __xwindows__

   //if (Self->x11.drawable) {
   //   if ((drawable = XCreatePixmap(XDisplay, DefaultRootWindow(XDisplay), width, height, bpp))) {
   //      XCopyArea(XDisplay, Self->x11.drawable, drawable, glXGC, 0, 0, Self->Width, Self->Height, 0, 0);
   //      XFreePixmap(XDisplay, Self->x11.drawable);
   //      Self->x11.drawable = drawable;
   //   }
   //   else return log.warning(ERR_AllocMemory);
   //   goto setfields;
   //}

#elif _WIN32
   if (Self->prvAFlags & BF_WINVIDEO) return ERR_NoSupport;
#endif

   UBYTE *data;
   if (Self->Flags & BMF_NO_DATA);
   else if ((Self->Data) and (Self->prvAFlags & BF_DATA)) {
      if (!AllocMemory(size, MEM_NO_BLOCKING|MEM_NO_POOL|Self->Head.MemFlags|Self->DataFlags|MEM_NO_CLEAR, &data, &datamid)) {
         if (Self->DataMID) {
            ReleaseMemoryID(Self->DataMID);
            FreeResourceID(Self->DataMID);
         }
         Self->Data = data;
         Self->DataMID = datamid;
      }
      else return log.warning(ERR_AllocMemory);
   }
   else return log.warning(ERR_FieldNotSet);

setfields:
   Self->Width         = width;
   Self->Height        = height;
   Self->Size          = size;
   Self->BitsPerPixel  = bpp;
   Self->AmtColours    = amtcolours;
   Self->BytesPerPixel = bytesperpixel;
   Self->ByteWidth     = bytewidth;
   Self->LineWidth     = linewidth;
   Self->PlaneMod      = planemod;
   Self->Clip.Left      = 0;
   Self->Clip.Top       = 0;
   Self->Clip.Right     = width;
   Self->Clip.Bottom    = height;

#ifdef __xwindows__
   WORD alignment;
   if (Self->x11.XShmImage) {
      Self->x11.XShmImage = FALSE; // Set to FALSE in case we fail (will drop through to standard XImage support)
      XShmDetach(XDisplay, &Self->x11.ShmInfo);  // Remove the previous attachment
      XSync(XDisplay, False);

      MemInfo meminfo;
      if ((!MemoryIDInfo(datamid, &meminfo)) and (meminfo.Handle)) {
         ClearMemory(&Self->x11.ShmInfo, sizeof(Self->x11.ShmInfo));
         Self->x11.ShmInfo.shmid    = meminfo.Handle;
         Self->x11.ShmInfo.readOnly = False;
         Self->x11.ShmInfo.shmaddr  = (char *)data;
         if (XShmAttach(XDisplay, &Self->x11.ShmInfo)) {
            if (Self->LineWidth & 0x0001) alignment = 8;
            else if (Self->LineWidth & 0x0002) alignment = 16;
            else alignment = 32;

            ClearMemory(&Self->x11.ximage, sizeof(Self->x11.ximage));

            Self->x11.ximage.width            = Self->Width;  // Image width
            Self->x11.ximage.height           = Self->Height; // Image height
            Self->x11.ximage.format           = ZPixmap;      // XYBitmap, XYPixmap, ZPixmap
            Self->x11.ximage.data             = (char *)Self->Data; // Pointer to image data
            Self->x11.ximage.obdata           = (char *)&Self->x11.ShmInfo;
            Self->x11.ximage.bitmap_unit      = alignment;    // Quant. of scanline - 8, 16, 32
            Self->x11.ximage.bitmap_pad       = alignment;    // 8, 16, 32, either XY or Zpixmap
            if (Self->BitsPerPixel IS 32) Self->x11.ximage.depth = 24;
            else Self->x11.ximage.depth = Self->BitsPerPixel;            // Actual bits per pixel
            Self->x11.ximage.bytes_per_line   = Self->LineWidth;         // Accelerator to next line
            Self->x11.ximage.bits_per_pixel   = Self->BytesPerPixel * 8; // Bits per pixel-group

            XInitImage(&Self->x11.ximage);
            Self->x11.XShmImage = TRUE;
         }
      }
   }

   if ((!Self->x11.drawable) and (Self->x11.XShmImage != TRUE)) {
      if (Self->LineWidth & 0x0001) alignment = 8;
      else if (Self->LineWidth & 0x0002) alignment = 16;
      else alignment = 32;

      ClearMemory(&Self->x11.ximage, sizeof(XImage));

      Self->x11.ximage.width            = Self->Width;  // Image width
      Self->x11.ximage.height           = Self->Height; // Image height
      Self->x11.ximage.format           = ZPixmap;      // XYBitmap, XYPixmap, ZPixmap
      Self->x11.ximage.data             = (char *)Self->Data; // Pointer to image data
      Self->x11.ximage.bitmap_unit      = alignment;    // Quant. of scanline - 8, 16, 32
      Self->x11.ximage.bitmap_pad       = alignment;    // 8, 16, 32, either XY or Zpixmap
      if (Self->BitsPerPixel IS 32) Self->x11.ximage.depth = 24;
      else Self->x11.ximage.depth = Self->BitsPerPixel;      // Actual bits per pixel
      Self->x11.ximage.bytes_per_line   = Self->LineWidth;         // Accelerator to next line
      Self->x11.ximage.bits_per_pixel   = Self->BytesPerPixel * 8; // Bits per pixel-group

      XInitImage(&Self->x11.ximage);
   }

#endif

   if (origbpp != Self->BitsPerPixel) {
      gfxGetColourFormat(Self->ColourFormat, Self->BitsPerPixel, 0, 0, 0, 0);
   }

   CalculatePixelRoutines(Self);

   if (Self->Flags & BMF_CLEAR) {
      if (Self->Width > origwidth) gfxDrawRectangle(Self, origwidth, 0, Self->Width - origwidth, origheight, bmpGetColourRGB(Self, &Self->BkgdRGB), BAF_FILL);
      if (Self->Height > origheight) gfxDrawRectangle(Self, 0, origheight, Self->Width, Self->Height - origheight, bmpGetColourRGB(Self, &Self->BkgdRGB), BAF_FILL);
   }

#ifdef __xwindows__
   XSync(XDisplay, False);
#endif
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
SaveImage: Saves a bitmap's image to a data object of your choosing in PCX format.
-END-
*****************************************************************************/

static ERROR BITMAP_SaveImage(objBitmap *Self, struct acSaveImage *Args)
{
   parasol::Log log;
   struct {
      BYTE Signature;
      BYTE Version;
      BYTE Encoding;
      BYTE BitsPixel;
      WORD XMin, YMin;
      WORD XMax, YMax;
      WORD XRes, YRes;
      UBYTE palette[48];
      BYTE Reserved;
      BYTE NumPlanes;
      WORD BytesLine;
      WORD PalType;
      UBYTE dummy[58];
   } pcx;
   RGB8 rgb;
   OBJECTPTR dest;
   UBYTE *buffer, lastpixel, newpixel, counter;
   LONG i, j, p, size;

   if ((!Args) or (!Args->DestID)) return log.warning(ERR_NullArgs);

   log.branch("Save To #%d", Args->DestID);

   // Create PCX Header

   ClearMemory(&pcx, sizeof(pcx));
   pcx.Signature = 10;       // ZSoft PCX-files
   pcx.Version   = 5;        // Version
   pcx.Encoding  = 1;        // Run Length Encoding=ON
   pcx.XMin      = 0;
   pcx.YMin      = 0;
   pcx.BitsPixel = 8;
   pcx.BytesLine = Self->Width;
   pcx.XMax      = Self->Width - 1;
   pcx.YMax      = Self->Height - 1;
   pcx.XRes      = Self->Width;
   pcx.YRes      = Self->Height;
   pcx.PalType   = 1;
   if (Self->AmtColours <= 256) pcx.NumPlanes = 1;
   else pcx.NumPlanes = 3;

   size = Self->Width * Self->Height * pcx.NumPlanes;
   if (!AllocMemory(size, MEM_DATA|MEM_NO_CLEAR, &buffer, NULL)) {
      if (!AccessObject(Args->DestID, 3000, &dest)) {
         acWrite(dest, &pcx, sizeof(pcx), NULL);

         LONG dp = 0;
         for (i=0; i <= (Self->Height - 1); i++) {
            if (pcx.NumPlanes IS 1) {
               // Save as a 256 colour image
               lastpixel = Self->ReadUCPixel(Self, 0, i);
               counter = 1;
               for (j=1; j <= Self->Width;j++) {
                  newpixel = Self->ReadUCPixel(Self, j, i);

                  if ((newpixel IS lastpixel) and (j != Self->Width - 1) and (counter <= 62)) {
                     counter++;
                  }
                  else {
                     if (!((counter IS 1) and (lastpixel < 192))) {
                        buffer[dp++] = 192 + counter;
                     }
                     buffer[dp++] = lastpixel;
                     lastpixel = newpixel;
                     counter = 1;
                  }

                  if (dp >= (size - 10)) {
                     FreeResource(buffer);
                     ReleaseObject(dest);
                     return log.warning(ERR_BufferOverflow);
                  }
               }
            }
            else {
               // Save as a true colour image
               for (p=0; p < 3; p++) {
                  // No encoding
                  if (pcx.Encoding IS 0) {
                     for (j=0; j < Self->Width; j++) {
                        Self->ReadUCRPixel(Self, j, i, &rgb);
                        switch(p) {
                           case 0:  buffer[dp++] = rgb.Red;   break;
                           case 1:  buffer[dp++] = rgb.Green; break;
                           default: buffer[dp++] = rgb.Blue;
                        }
                     }
                  }
                  else {
                     // Encoding on
                     Self->ReadUCRPixel(Self, 0, i, &rgb);
                     switch(p) {
                        case 0:  lastpixel = rgb.Red;   break;
                        case 1:  lastpixel = rgb.Green; break;
                        default: lastpixel = rgb.Blue;
                     }
                     counter = 1;

                     for (j=1; j < Self->Width; j++) {
                        Self->ReadUCRPixel(Self, j, i, &rgb);
                        switch(p) {
                           case 0:  newpixel = rgb.Red;   break;
                           case 1:  newpixel = rgb.Green; break;
                           default: newpixel = rgb.Blue;
                        }

                        if (newpixel IS lastpixel) {
                           counter++;
                           if (counter IS 63) {
                              buffer[dp++] = 0xc0 | counter;
                              buffer[dp++] = lastpixel;
                              counter = 0;
                           }
                        }
                        else {
                           if ((counter IS 1) and (0xc0 != (0xc0 & lastpixel))) {
                              buffer[dp++] = lastpixel;
                           }
                           else if (counter) {
                              buffer[dp++] = 0xc0 | counter;
                              buffer[dp++] = lastpixel;
                           }
                           lastpixel = newpixel;
                           counter = 1;
                        }
                     }

                     // Finish line if necessary

                     if ((counter IS 1) and (0xc0 != (0xc0 & lastpixel))) {
                        buffer[dp++] = lastpixel;
                     }
                     else if (counter) {
                        buffer[dp++] = 0xc0 | counter;
                        buffer[dp++] = lastpixel;
                     }
                  }
               }
            }
         }

         acWrite(dest, buffer, dp, NULL);
         FreeResource(buffer);

         // Setup palette

         if (Self->AmtColours <= 256) {
            UBYTE palette[(256 * 3) + 1];
            LONG j = 0;
            palette[j++] = 12;          // Palette identifier
            for (LONG i=0; i < 256; i++) {
               palette[j++] = Self->Palette->Col[i].Red;
               palette[j++] = Self->Palette->Col[i].Green;
               palette[j++] = Self->Palette->Col[i].Blue;
            }

            acWrite(dest, palette, sizeof(palette), NULL);
         }

         ReleaseObject(dest);
         return ERR_Okay;
      }
      else {
         FreeResource(buffer);
         return ERR_AccessObject;
      }
   }
   else return ERR_AllocMemory;
}

/*****************************************************************************
-ACTION-
Seek: Changes the current byte position for read/write operations.

*****************************************************************************/

static ERROR BITMAP_Seek(objBitmap *Self, struct acSeek *Args)
{
   if (Args->Position IS SEEK_START) Self->Position = (LONG)Args->Offset;
   else if (Args->Position IS SEEK_END) Self->Position = (LONG)(Self->Size - Args->Offset);
   else if (Args->Position IS SEEK_CURRENT) Self->Position = (LONG)(Self->Position + Args->Offset);
   else return ERR_Args;

   if (Self->Position > Self->Size) Self->Position = Self->Size;
   else if (Self->Position < 0) Self->Position = 0;

   return ERR_Okay;
}

/*****************************************************************************

-METHOD-
SetClipRegion: Sets a clipping region for a bitmap object.

This method is a proxy for ~Display.SetClipRegion().

-INPUT-
int Number:    The number of the clip region to set.
int Left:      The horizontal start of the clip region.
int Top:       The vertical start of the clip region.
int Right:     The right-most edge of the clip region.
int Bottom:    The bottom-most edge of the clip region.
int Terminate: Set to TRUE if this is the last clip region in the list, otherwise FALSE.

-ERRORS-
Okay
NullArgs

*****************************************************************************/

static ERROR BITMAP_SetClipRegion(objBitmap *Self, struct bmpSetClipRegion *Args)
{
   if (!Args) return ERR_NullArgs;

   gfxSetClipRegion(Self, Args->Number, Args->Left, Args->Top, Args->Right, Args->Bottom, Args->Terminate);
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Unlock: Unlocks the bitmap surface once direct access is no longer required.

*****************************************************************************/

static ERROR BITMAP_Unlock(objBitmap *Self, APTR Void)
{
#ifndef __xwindows__
   UnlockSurface(Self);
#endif
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Write: Writes raw image data to a bitmap object.
-END-
*****************************************************************************/

static ERROR BITMAP_Write(objBitmap *Self, struct acWrite *Args)
{
   if (Self->Data) {
      BYTE *Data = (BYTE *)Self->Data + Self->Position;
      LONG amt_bytes = 0;
      while (Args->Length > 0) {
         Data[amt_bytes] = ((UBYTE *)Args->Buffer)[amt_bytes];
         Args->Length--;
         amt_bytes++;
      }
      Self->Position += amt_bytes;
      return ERR_Okay;
   }
   else return ERR_NoData;
}

/*****************************************************************************

-FIELD-
AmtColours: The maximum number of displayable colours.

-FIELD-
BitsPerPixel: The number of bits per pixel

The BitsPerPixel field clarifies exactly how many bits are being used to manage each pixel on the display.  This
includes any 'special' bits that are in use, e.g. alpha-channel bits.

-FIELD-
Bkgd: The bitmap's background colour is defined here in RGB format.

The default background colour for a bitmap is black.  To change it, set this field with the new RGB colour.  The
background colour is used in operations that require a default colour, such as when clearing the bitmap.

The #BkgdIndex will be updated as a result of setting this field.

*****************************************************************************/

static ERROR SET_Bkgd(objBitmap *Self, RGB8 *Value)
{
   Self->BkgdRGB = *Value;

   if (Self->BitsPerPixel > 8) {
      Self->BkgdIndex = (((Self->BkgdRGB.Red   >>Self->prvColourFormat.RedShift)   & Self->prvColourFormat.RedMask)   << Self->prvColourFormat.RedPos) |
                         (((Self->BkgdRGB.Green>>Self->prvColourFormat.GreenShift) & Self->prvColourFormat.GreenMask) << Self->prvColourFormat.GreenPos) |
                         (((Self->BkgdRGB.Blue >>Self->prvColourFormat.BlueShift)  & Self->prvColourFormat.BlueMask)  << Self->prvColourFormat.BluePos) |
                         (((Self->BkgdRGB.Alpha>>Self->prvColourFormat.AlphaShift) & Self->prvColourFormat.AlphaMask) << Self->prvColourFormat.AlphaPos);
   }
   else Self->BkgdIndex = RGBToValue(&Self->BkgdRGB, Self->Palette);
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
BkgdIndex: The bitmap's background colour is defined here as a colour index.

The bitmap's background colour is defined in this field as a colour index.  It is recommended that the #Bkgd
field is used for altering the bitmap background unless efficiency requires that the colour index is calculated and set
directly.

*****************************************************************************/

static ERROR SET_BkgdIndex(objBitmap *Self, LONG Index)
{
   if ((Index < 0) or (Index > 255)) return ERR_OutOfRange;
   Self->BkgdIndex = Index;
   Self->BkgdRGB   = Self->Palette->Col[Self->BkgdIndex];
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
BytesPerPixel: The number of bytes per pixel.

If you need to find out how many bytes are involved in the makeup of each pixel you will need to read this field.  The
maximum number of bytes you can typically expect is 4 and the minimum is 1.  If a planar bitmap is being used then you
should refer to the #BitsPerPixel field, which should yield more useful information.

-FIELD-
ByteWidth: The width of the bitmap, in bytes.

The ByteWidth of the bitmap is calculated directly from the bitmap's #Width and #Type settings. Under no circumstances
should you attempt to calculate this value in advance, as it is heavily dependent on the bitmap's #Type.

The formulas used to calculate the value of this field are:

<pre>
Planar      = Width/8
Chunky/8    = Width
Chunky/15   = Width * 2
Chunky/16   = Width * 2
Chunky/24   = Width * 3
Chunky/32   = Width * 4
</pre>

If you would like to know the total byte width per line including any padded bytes that may lie at the end of each line,
please refer to the #LineMod field.

-FIELD-
ClipBottom: The bottom-most edge of  bitmap's clipping region.

During the initialisation of a bitmap, a default clipping region will be created that matches the bitmap's dimensions.
Clipping regions define the area under which graphics can be drawn to a bitmap.  This particular field reflects the
bottom-most edge of all clipping regions that have been set or altered through the #SetClipRegion() method.

-FIELD-
ClipLeft: The left-most edge of a bitmap's clipping region.

During the initialisation of a bitmap, a default clipping region will be created that matches the bitmap's dimensions.
Clipping regions define the area under which graphics can be drawn to a bitmap.  This particular field reflects the
left-most edge of all clipping regions that have been set or altered through the #SetClipRegion() method.

-FIELD-
ClipRight: The right-most edge of a bitmap's clipping region.

During the initialisation of a bitmap, a default clipping region will be created that matches the bitmap's dimensions.
Clipping regions define the area under which graphics can be drawn to a bitmap.  This particular field reflects the
right-most edge of all clipping regions that have been set or altered through the #SetClipRegion() method.

-FIELD-
ClipTop: The top-most edge of a bitmap's clipping region.

During the initialisation of a bitmap, a default clipping region will be created that matches the bitmap's dimensions.
Clipping regions define the area under which graphics can be drawn to a bitmap.  This particular field reflects the
top-most edge of all clipping regions that have been set or altered through the #SetClipRegion() method.

-FIELD-
Clip: Defines the bitmap's clipping region.

*****************************************************************************/

static ERROR GET_Clip(objBitmap *Self, ClipRectangle **Value)
{
   *Value = &Self->Clip;
   return ERR_Okay;
}

static ERROR SET_Clip(objBitmap *Self, ClipRectangle *Value)
{
   Self->Clip = *Value;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ColourFormat: Describes the colour format used to construct each bitmap pixel.

The ColourFormat field points to a structure that defines the colour format used to construct each bitmap pixel.  It
only applies to bitmaps that use 2-bytes per colour value or better.  The structure consists of the following fields:

<struct lookup="ColourFormat">
<field type="UBYTE" name="RedShift">Right shift value for the red component (applies only to 15/16 bit formats for eliminating redundant bits).</>
<field type="UBYTE" name="BlueShift">Right shift value for the blue component.</>
<field type="UBYTE" name="GreenShift">Right shift value for the green component.</>
<field type="UBYTE" name="AlphaShift">Right shift value for the alpha component.</>
<field type="UBYTE" name="RedMask">The unshifted mask value for the red component (ranges from 0x00 to 0xff).</>
<field type="UBYTE" name="GreenMask">The unshifted mask value for the green component.</>
<field type="UBYTE" name="BlueMask">The unshifted mask value for the blue component.</>
<field type="UBYTE" name="AlphaMask">The unshifted mask value for the alpha component.</>
<field type="UBYTE" name="RedPos">Left shift/positional value for the red component.</>
<field type="UBYTE" name="GreenPos">Left shift/positional value for the green component.</>
<field type="UBYTE" name="BluePos">Left shift/positional value for the blue component.</>
<field type="UBYTE" name="AlphaPos">Left shift/positional value for the alpha component.</>
<field type="UBYTE" name="BitsPerPixel"> Number of bits per pixel for this format.</>
</>

The following C macros can called on any bitmap in order to build colour values from individual RGB components:

<pre>
PackPixel(Bitmap, Red, Green, Blue)
PackPixelA(Bitmap, Red, Green, Blue, Alpha)
PackAlpha(Bitmap, Alpha)
PackPixelRGB(Bitmap, struct RGB8 *RGB)
PackPixelRGBA(Bitmap, struct RGB8 *RGB)
</pre>

The following C macros are optimised versions of the above that are limited to 24 and 32-bit bitmaps:

<pre>
PackPixelWB(Bitmap, Red, Green, Blue)
PackPixelWBA(Bitmap, Red, Green, Blue, Alpha)
</pre>

The following C macros can be used to unpack individual colour components from any colour value read from the bitmap:

<pre>
UnpackRed(Bitmap, Colour)
UnpackGreen(Bitmap, Colour)
UnpackBlue(Bitmap, Colour)
UnpackAlpha(Bitmap, Colour)
</pre>

-FIELD-
Data: Pointer to a bitmap's data area.

This field points directly to the start of a bitmap's data area.  Allocating your own bitmap memory is acceptable
if creating a bitmap that is not based on video memory.  However, it is usually a better idea for the
initialisation process to allocate the correct amount of memory for you by not interfering with this field.

*****************************************************************************/

ERROR SET_Data(objBitmap *Self, UBYTE *Data)
{
   // This code allows us to calculate the correct memory flags and also set the pixel drawing functions accordingly
   // (i.e. functions to draw to video memory are different to drawing to normal memory).

   if (Self->Data != Data) {
      Self->Data = Data;

      if (!Self->DataFlags) {
         MemInfo info;
         if (MemoryPtrInfo(Data, &info) != ERR_Okay) {
            parasol::Log log;
            log.warning("Could not obtain flags from address %p.", Data);
         }
         else if (Self->DataFlags != info.Flags) {
            Self->DataFlags = info.Flags;
            if (Self->Head.Flags & NF_INITIALISED) CalculatePixelRoutines(Self);
         }
      }
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
DataFlags: Defines the memory flags to use in allocating a bitmap's data area.

This field determines the type of memory that will be allocated for the #Data field during the initialisation process.
This field accepts the MEM_DATA, MEM_VIDEO and MEM_TEXTURE memory flags.

Please note that video based bitmaps may be faster than data bitmaps for certain applications, but the content is typically
read-only.  Under normal circumstances it is not possible to use the pixel reading functions, or read from the
bitmap #Data field directly with these bitmap types.  To circumvent this problem please use the #Lock() action
to enable read access when you require it.

-FIELD-
DrawUCPixel: Points to a C function that draws pixels to the bitmap using colour indexes.

This field points to an internal C function that can be used for drawing pixels to the bitmap.  It is intended that the
function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks will
be performed (meaning it is possible to supply invalid coordinates that would result in a segfault).

The prototype of the DrawUCPixel function is `Function(*Bitmap, LONG X, LONG Y, ULONG Colour)`.

The new pixel value must be defined in the Colour parameter.

-FIELD-
DrawUCRIndex: Points to a C function that draws pixels to the bitmap in RGB format.

This field points to an internal C function that can be used for drawing pixels to the bitmap.  It is intended that
the function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks
will be performed (meaning it is possible to supply an invalid address that would result in a segfault).

The prototype of the DrawUCRIndex function is `Function(*Bitmap, UBYTE *Data, RGB8 *RGB)`.

The Data parameter must point to a location within the Bitmap's graphical address space. The new pixel value must be
defined in the RGB parameter.

Note that a colour indexing equivalent of this function is not available in the Bitmap class - this is because it is
more efficient to index the Bitmap's #Data field directly.

-FIELD-
DrawUCRPixel: Points to a C function that draws pixels to the bitmap in RGB format.

This field points to an internal C function that can be used for drawing pixels to the bitmap.  It is intended that the
function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks will
be performed (meaning it is possible to supply invalid coordinates that would result in a segfault).

The prototype of the DrawUCRPixel function is `Function(*Bitmap, LONG X, LONG Y, RGB8 *RGB)`.

The new pixel value must be defined in the RGB parameter.

-FIELD-
Flags: Optional flags.

-FIELD-
Handle: Private. Platform dependent field for referencing video memory.
-END-

*****************************************************************************/

static ERROR GET_Handle(objBitmap *Self, APTR *Value)
{
#ifdef _WIN32
   *Value = (APTR)Self->win.Drawable;
   return ERR_Okay;
#elif __xwindows__
   *Value = (APTR)Self->x11.drawable;
   return ERR_Okay;
#else
   return ERR_NoSupport;
#endif
}

static ERROR SET_Handle(objBitmap *Self, APTR Value)
{
   // Note: The only area of the system allowed to set this field are the Display/Surface classes for video management.

#ifdef _WIN32
   Self->win.Drawable = Value;
   return ERR_Okay;
#elif __xwindows__
   Self->x11.drawable = (MAXINT)Value;
   return ERR_Okay;
#else
   return ERR_NoSupport;
#endif
}

/*****************************************************************************

-FIELD-
Height: The height of the bitmap, in pixels.

-FIELD-
LineMod: The length of each bitmap line in bytes, including alignment.

-FIELD-
Opacity: Determines the translucency setting to use in drawing operations.

Some drawing operations support the concept of applying an opacity rating to create translucent graphics.  By adjusting
the opacity rating, you can affect the level of translucency that is applied when executing certain graphics operations.

Methods that support opacity should document the fact that they support the feature.  By default the opacity rating is
set to 255 to turn off translucency effects.  Lowering the value will increase the level of translucency when drawing
graphics.

-FIELD-
Palette: Points to a bitmap's colour palette.

A palette is an array of containing colour values in standard RGB format ($RRGGBB).  The first value must have a
header ID of ID_PALETTE, followed by the amount of values in the array. Following this is the actual list itself -
colour 0, then colour 1 and so on. There is no termination signal at the end of the list.

The following example is for a 32 colour palette:

<pre>
RGBPalette Palette = {
  ID_PALETTE, VER_PALETTE, 32,
  {{ 0x00,0x00,0x00 }, { 0x10,0x10,0x10 }, { 0x17,0x17,0x17 }, { 0x20,0x20,0x20 },
   { 0x27,0x27,0x27 }, { 0x30,0x30,0x30 }, { 0x37,0x37,0x37 }, { 0x40,0x40,0x40 },
   { 0x47,0x47,0x47 }, { 0x50,0x50,0x50 }, { 0x57,0x57,0x57 }, { 0x60,0x60,0x60 },
   { 0x67,0x67,0x67 }, { 0x70,0x70,0x70 }, { 0x77,0x77,0x77 }, { 0x80,0x80,0x80 },
   { 0x87,0x87,0x87 }, { 0x90,0x90,0x90 }, { 0x97,0x97,0x97 }, { 0xa0,0xa0,0xa0 },
   { 0xa7,0xa7,0xa7 }, { 0xb0,0xb0,0xb0 }, { 0xb7,0xb7,0xb7 }, { 0xc0,0xc0,0xc0 },
   { 0xc7,0xc7,0xc7 }, { 0xd0,0xd0,0xd0 }, { 0xd7,0xd7,0xd7 }, { 0xe0,0xe0,0xe0 },
   { 0xe0,0xe0,0xe0 }, { 0xf0,0xf0,0xf0 }, { 0xf7,0xf7,0xf7 }, { 0xff,0xff,0xff }
   }
};
</pre>

Palettes are created for all bitmap types, including RGB based bitmaps above 8-bit colour.  This is because a number of
drawing functions require a palette table for conversion between the bitmap types.

Although the array is dynamic, parent objects such as the Display need to be notified if you want a palette's colours
to be propagated to the video display.

*****************************************************************************/

ERROR SET_Palette(objBitmap *Self, RGBPalette *SrcPalette)
{
   parasol::Log log;

   // The objective here is to copy the given source palette to the bitmap's palette.  To see how the hook is set up,
   // refer to the bitmap's object definition structure that is compiled into the module.

   if (!SrcPalette) return ERR_Okay;

   if (SrcPalette->AmtColours <= 256) {
      if (!Self->Palette) {
         if (AllocMemory(sizeof(RGBPalette), MEM_NO_CLEAR, &Self->Palette, NULL) != ERR_Okay) {
            log.warning(ERR_AllocMemory);
         }
      }

      Self->Palette->AmtColours = SrcPalette->AmtColours;
      WORD i = SrcPalette->AmtColours-1;
      while (i > 0) {
         Self->Palette->Col[i] = SrcPalette->Col[i];
         i--;
      }
      return ERR_Okay;
   }
   else {
      log.warning("Corruption in Palette at %p.", SrcPalette);
      return ERR_ObjectCorrupt;
   }
}

/*****************************************************************************

-FIELD-
PlaneMod: The differential between each bitmap plane.

This field specifies the distance (in bytes) between each bitplane.  For non-planar types like CHUNKY, this field will
actually reflect the total size of the bitmap.  The calculation used for PLANAR types is `ByteWidth * Height`.

-FIELD-
Position: The current read/write data position.

This field reflects the current byte position for reading and writing raw data to and from a bitmap object.  If you
need to change the current byte position, use the Seek action.

-FIELD-
ReadUCRIndex: Points to a C function that reads pixels from the bitmap in RGB format.

This field points to an internal C function that can be used for reading pixels from the bitmap.  It is intended that
the function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks
will be performed (meaning it is possible to supply an invalid address that would result in a segfault).

The prototype of the ReadUCRIndex function is `Function(*Bitmap, UBYTE *Data, RGB8 *RGB)`.

The Data parameter must point to a location within the Bitmap's graphical address space. The pixel value will be
returned in the RGB parameter.

Note that a colour indexing equivalent of this function is not available in the Bitmap class - this is because it is
more efficient to index the Bitmap's #Data field directly.

-FIELD-
ReadUCPixel: Points to a C function that reads pixels from the bitmap in colour index format.

This field points to an internal C function that can be used for reading pixels from the bitmap.  It is intended that
the function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks
will be performed (meaning it is possible to supply invalid X/Y coordinates that would result in a segfault).

The prototype of the ReadUCPixel function is `Function(*Bitmap, LONG X, LONG Y, LONG *Index)`.

The pixel value will be returned in the Index parameter.

-FIELD-
ReadUCRPixel: Points to a C function that reads pixels from the bitmap in RGB format.

This field points to an internal C function that can be used for reading pixels from the bitmap.  It is intended that
the function is only ever called by C programs and that caution is exercised by the programmer, as no clipping checks
will be performed (meaning it is possible to supply invalid X/Y coordinates that would result in a segfault).

The prototype of the ReadUCRPixel function is `Function(*Bitmap, LONG X, LONG Y, RGB8 *RGB)`.

The pixel value will be returned in the RGB parameter.  It should be noted that as this function converts the pixel
value into RGB format, #ReadUCPixel or #ReadUCRIndex should be used as faster alternatives if the
pixel value does not need to be de-constructed into its RGB components.

-FIELD-
Size: The total size of the bitmap, in bytes.

-FIELD-
TransRGB: The transparent colour of the bitmap, in RGB format.

The transparent colour of the bitmap is defined here.  Colours in the bitmap that match this value will not be copied
during drawing operations.

NOTE: This field should never be set if the bitmap utilises alpha transparency.

*****************************************************************************/

static ERROR SET_Trans(objBitmap *Self, RGB8 *Value)
{
   Self->TransRGB = *Value;

   if (Self->BitsPerPixel > 8) {
      Self->TransIndex = (((Self->TransRGB.Red  >>Self->prvColourFormat.RedShift)   & Self->prvColourFormat.RedMask)   << Self->prvColourFormat.RedPos) |
                         (((Self->TransRGB.Green>>Self->prvColourFormat.GreenShift) & Self->prvColourFormat.GreenMask) << Self->prvColourFormat.GreenPos) |
                         (((Self->TransRGB.Blue >>Self->prvColourFormat.BlueShift)  & Self->prvColourFormat.BlueMask)  << Self->prvColourFormat.BluePos) |
                         (((Self->TransRGB.Alpha>>Self->prvColourFormat.AlphaShift) & Self->prvColourFormat.AlphaMask) << Self->prvColourFormat.AlphaPos);
   }
   else Self->TransIndex = RGBToValue(&Self->TransRGB, Self->Palette);

   if (!(Self->DataFlags & MEM_VIDEO)) Self->Flags |= BMF_TRANSPARENT;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
TransIndex: The transparent colour of the bitmap, represented as an index.

The transparent colour of the bitmap is defined here.  Colours in the bitmap that match this value will not be copied
during graphics operations.  It is recommended that the #TransRGB field is used for altering the bitmap
transparency unless efficiency requires that the transparency is set directly.

NOTE: This field should never be set if the bitmap utilises alpha transparency.

*****************************************************************************/

static ERROR SET_TransIndex(objBitmap *Self, LONG Index)
{
   if ((Index < 0) or (Index > 255)) return ERR_OutOfRange;

   Self->TransIndex = Index;
   Self->TransRGB   = Self->Palette->Col[Self->TransIndex];

   if (!(Self->DataFlags & MEM_VIDEO)) Self->Flags |= BMF_TRANSPARENT;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Type: Defines the data type of the bitmap.

This field defines the graphics data type - either PLANAR (required for 1-bit bitmaps) or CHUNKY (the default).

-FIELD-
Width: The width of the bitmap, in pixels.

-FIELD-
XOffset: Private. Provided for surface/video drawing purposes - considered too advanced for standard use.

-FIELD-
YOffset: Private. Provided for surface/video drawing purposes - considered too advanced for standard use.
-END-

*****************************************************************************/

//****************************************************************************

static ERROR CalculatePixelRoutines(objBitmap *Self)
{
   parasol::Log log;

   if (Self->Type IS BMP_PLANAR) {
      Self->ReadUCPixel  = MemReadPixelPlanar;
      Self->ReadUCRPixel = MemReadRGBPixelPlanar;
      Self->ReadUCRIndex = MemReadRGBIndexPlanar;
      Self->DrawUCPixel  = MemDrawPixelPlanar;
      Self->DrawUCRPixel = DrawRGBPixelPlanar;
      Self->DrawUCRIndex = NULL;
      return ERR_Okay;
   }

   if (Self->Type != BMP_CHUNKY) {
      log.warning("Unsupported Bitmap->Type %d.", Self->Type);
      return ERR_Failed;
   }

#ifdef _WIN32

   if (Self->prvAFlags & BF_WINVIDEO) {
      Self->ReadUCPixel  = &VideoReadPixel;
      Self->ReadUCRPixel = &VideoReadRGBPixel;
      Self->ReadUCRIndex = &VideoReadRGBIndex;
      Self->DrawUCPixel  = &VideoDrawPixel;
      Self->DrawUCRPixel = &VideoDrawRGBPixel;
      Self->DrawUCRIndex = &VideoDrawRGBIndex;
      return ERR_Okay;
   }

#else

   if (Self->DataFlags & (MEM_VIDEO|MEM_TEXTURE)) {
      switch(Self->BytesPerPixel) {
         case 1:
            Self->ReadUCPixel  = &VideoReadPixel8;
            Self->ReadUCRPixel = &VideoReadRGBPixel8;
            Self->ReadUCRIndex = &VideoReadRGBIndex8;
            Self->DrawUCPixel  = &VideoDrawPixel8;
            Self->DrawUCRPixel = &VideoDrawRGBPixel8;
            Self->DrawUCRIndex = &VideoDrawRGBIndex8;
            break;

         case 2:
            Self->ReadUCPixel  = &VideoReadPixel16;
            Self->ReadUCRPixel = &VideoReadRGBPixel16;
            Self->ReadUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))&VideoReadRGBIndex16;
            Self->DrawUCPixel  = &VideoDrawPixel16;
            Self->DrawUCRPixel = &VideoDrawRGBPixel16;
            Self->DrawUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))&VideoDrawRGBIndex16;
            break;

         case 3:
            Self->ReadUCPixel  = &VideoReadPixel24;
            Self->ReadUCRPixel = &VideoReadRGBPixel24;
            Self->ReadUCRIndex = &VideoReadRGBIndex24;
            Self->DrawUCPixel  = &VideoDrawPixel24;
            Self->DrawUCRPixel = &VideoDrawRGBPixel24;
            Self->DrawUCRIndex = &VideoDrawRGBIndex24;
            break;

         case 4:
            Self->ReadUCPixel  = &VideoReadPixel32;
            Self->ReadUCRPixel = &VideoReadRGBPixel32;
            Self->ReadUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))&VideoReadRGBIndex32;
            Self->DrawUCPixel  = &VideoDrawPixel32;
            Self->DrawUCRPixel = &VideoDrawRGBPixel32;
            Self->DrawUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))&VideoDrawRGBIndex32;
            break;

         default:
            log.warning("Unsupported Bitmap->BytesPerPixel %d.", Self->BytesPerPixel);
            return ERR_Failed;
      }
      return ERR_Okay;
   }
#endif

   switch(Self->BytesPerPixel) {
      case 1:
        Self->ReadUCPixel  = MemReadPixel8;
        Self->ReadUCRPixel = MemReadRGBPixel8;
        Self->ReadUCRIndex = MemReadRGBIndex8;
        Self->DrawUCPixel  = MemDrawPixel8;
        Self->DrawUCRPixel = MemDrawRGBPixel8;
        Self->DrawUCRIndex = MemDrawRGBIndex8;
        break;

      case 2:
         Self->ReadUCPixel  = MemReadPixel16;
         Self->ReadUCRPixel = MemReadRGBPixel16;
         Self->ReadUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))MemReadRGBIndex16;
         Self->DrawUCPixel  = MemDrawPixel16;
         Self->DrawUCRPixel = MemDrawRGBPixel16;
         Self->DrawUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))MemDrawRGBIndex16;
         break;

      case 3:
         if (Self->prvColourFormat.RedPos IS 16) {
            Self->ReadUCPixel  = MemReadLSBPixel24;
            Self->ReadUCRPixel = MemReadLSBRGBPixel24;
            Self->ReadUCRIndex = MemReadLSBRGBIndex24;
            Self->DrawUCPixel  = MemDrawLSBPixel24;
            Self->DrawUCRPixel = MemDrawLSBRGBPixel24;
            Self->DrawUCRIndex = MemDrawLSBRGBIndex24;
         }
         else {
            Self->ReadUCPixel  = MemReadMSBPixel24;
            Self->ReadUCRPixel = MemReadMSBRGBPixel24;
            Self->ReadUCRIndex = MemReadMSBRGBIndex24;
            Self->DrawUCPixel  = MemDrawMSBPixel24;
            Self->DrawUCRPixel = MemDrawMSBRGBPixel24;
            Self->DrawUCRIndex = MemDrawMSBRGBIndex24;
         }
         break;

      case 4:
         Self->ReadUCPixel  = MemReadPixel32;
         Self->ReadUCRPixel = MemReadRGBPixel32;
         Self->ReadUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))MemReadRGBIndex32;
         Self->DrawUCPixel  = MemDrawPixel32;
         Self->DrawUCRPixel = MemDrawRGBPixel32;
         Self->DrawUCRIndex = (void (*)(rkBitmap *, UBYTE *, RGB8 *))MemDrawRGBIndex32;
         break;

      default:
        log.warning("Unsupported Bitmap->BytesPerPixel %d.", Self->BytesPerPixel);
        return ERR_Failed;
   }

   return ERR_Okay;
}

//****************************************************************************

#include "lib_mempixels.cpp"

#ifdef __xwindows__
#include "x11/lib_pixels.cpp"
#endif

#ifdef _WIN32
#include "win32/lib_pixels.cpp"
#endif

#ifdef __ANDROID__
#include "android/lib_pixels.cpp"
#endif

#include "class_bitmap_def.c"

static const FieldArray clBitmapFields[] = {
   { "Palette",       FDF_POINTER|FDF_RW,           0, NULL, (APTR)SET_Palette },
   { "ColourFormat",  FDF_POINTER|FDF_STRUCT|FDF_R, (MAXINT)"ColourFormat", NULL, NULL },
   { "DrawUCPixel",   FDF_POINTER|FDF_R,            (MAXINT)&argsDrawUCPixel, NULL, NULL },
   { "DrawUCRPixel",  FDF_POINTER|FDF_R,            (MAXINT)&argsDrawUCRPixel, NULL, NULL },
   { "ReadUCPixel",   FDF_POINTER|FDF_R,            (MAXINT)&argsReadUCPixel, NULL, NULL },
   { "ReadUCRPixel",  FDF_POINTER|FDF_R,            (MAXINT)&argsReadUCRPixel, NULL, NULL },
   { "ReadUCRIndex",  FDF_POINTER|FDF_R,            (MAXINT)&argsReadUCRIndex, NULL, NULL },
   { "DrawUCRIndex",  FDF_POINTER|FDF_R,            (MAXINT)&argsDrawUCRIndex, NULL, NULL },
   { "Data",          FDF_POINTER|FDF_RI,           0, NULL, (APTR)SET_Data },
   { "Width",         FDF_LONG|FDF_RI,              0, NULL, NULL },
   { "ByteWidth",     FDF_LONG|FDF_R,               0, NULL, NULL },
   { "Height",        FDF_LONG|FDF_RI,              0, NULL, NULL },
   { "Type",          FDF_LONG|FDF_RI|FDF_LOOKUP,   (MAXINT)&clBitmapType, NULL, NULL },
   { "LineMod",       FDF_LONG|FDF_R,               0, NULL, NULL },
   { "PlaneMod",      FDF_LONG|FDF_R,               0, NULL, NULL },
   { "ClipLeft",      FDF_LONG|FDF_RW,              0, NULL, NULL },
   { "ClipRight",     FDF_LONG|FDF_RW,              0, NULL, NULL },
   { "ClipBottom",    FDF_LONG|FDF_RW,              0, NULL, NULL },
   { "ClipTop",       FDF_LONG|FDF_RW,              0, NULL, NULL },
   { "Size",          FDF_LONG|FDF_R,               0, NULL, NULL },
   { "DataFlags",     FDF_LONGFLAGS|FDF_RI,         (MAXINT)&clDataFlags, NULL, NULL },
   { "AmtColours",    FDF_LONG|FDF_RI,              0, NULL, NULL },
   { "Flags",         FDF_LONGFLAGS|FDF_RI,         (MAXINT)&clBitmapFlags, NULL, NULL },
   { "TransIndex",    FDF_LONG|FDF_RW,              0, NULL, (APTR)SET_TransIndex },
   { "BytesPerPixel", FDF_LONG|FDF_RI,              0, NULL, NULL },
   { "BitsPerPixel",  FDF_LONG|FDF_RI,              0, NULL, NULL },
   { "Position",      FDF_LONG|FDF_R,               0, NULL, NULL },
   { "XOffset",       FDF_LONG|FDF_SYSTEM|FDF_RW,   0, NULL, NULL },
   { "YOffset",       FDF_LONG|FDF_SYSTEM|FDF_RW,   0, NULL, NULL },
   { "Opacity",       FDF_LONG|FDF_RW,              0, NULL, NULL },
   { "DataID",        FDF_LONG|FDF_SYSTEM|FDF_R,    0, NULL, NULL },
   { "TransRGB",      FDF_RGB|FDF_RW,               0, NULL, (APTR)SET_Trans },
   { "Bkgd",          FDF_RGB|FDF_RW,               0, NULL, (APTR)SET_Bkgd },
   { "BkgdIndex",     FDF_LONG|FDF_RW,              0, NULL, (APTR)SET_BkgdIndex },
   // Virtual fields
   { "Clip",          FDF_POINTER|FDF_STRUCT|FDF_RW, 0, (APTR)GET_Clip, (APTR)SET_Clip },
   { "Handle",        FDF_POINTER|FDF_SYSTEM|FDF_RW, 0, (APTR)GET_Handle, (APTR)SET_Handle },
   END_FIELD
};

//****************************************************************************

static ERROR create_bitmap_class(void)
{
   return(CreateObject(ID_METACLASS, 0, &BitmapClass,
      FID_ClassVersion|TFLOAT, VER_BITMAP,
      FID_Name|TSTR,      "Bitmap",
      FID_Category|TLONG, CCF_GRAPHICS,
      FID_Actions|TPTR,   clBitmapActions,
      FID_Methods|TARRAY, clBitmapMethods,
      FID_Fields|TARRAY,  clBitmapFields,
      FID_Size|TLONG,     sizeof(objBitmap),
      FID_Path|TSTR,      MOD_PATH,
      TAGEND));
}

