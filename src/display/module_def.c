// Auto-generated by idl-c.fluid

#ifdef  __cplusplus
extern "C" {
#endif

static ERROR gfxGetDisplayInfo(OBJECTID Display, struct DisplayInfoV3 ** Info);
static LONG gfxGetDisplayType();
static ERROR gfxSetCursor(OBJECTID Surface, LONG Flags, LONG Cursor, CSTRING Name, OBJECTID Owner);
static ERROR gfxRestoreCursor(LONG Cursor, OBJECTID Owner);
static ERROR gfxGetCursorPos(LONG * X, LONG * Y);
static ERROR gfxSetCursorPos(LONG X, LONG Y);
static ERROR gfxGetRelativeCursorPos(OBJECTID Surface, LONG * X, LONG * Y);
static ERROR gfxGetCursorInfo(struct CursorInfo * Info, LONG Size);
static ERROR gfxSetCustomCursor(OBJECTID Surface, LONG Flags, struct rkBitmap * Bitmap, LONG HotX, LONG HotY, OBJECTID Owner);
static struct rkPointer * gfxAccessPointer();
static ERROR gfxScanDisplayModes(CSTRING Filter, struct DisplayInfoV3 * Info, LONG Size);
static ERROR gfxLockCursor(OBJECTID Surface);
static ERROR gfxUnlockCursor(OBJECTID Surface);
static ERROR gfxSetHostOption(LONG Option, LARGE Value);
static ERROR gfxStartCursorDrag(OBJECTID Source, LONG Item, CSTRING Datatypes, OBJECTID Surface);
static ERROR gfxCopySurface(struct BitmapSurfaceV2 * Surface, struct rkBitmap * Bitmap, LONG Flags, LONG X, LONG Y, LONG Width, LONG Height, LONG XDest, LONG YDest);
static void gfxSync(struct rkBitmap * Bitmap);
static ERROR gfxResample(struct rkBitmap * Bitmap, struct ColourFormat * ColourFormat);
static void gfxGetColourFormat(struct ColourFormat * Format, LONG BitsPerPixel, LONG RedMask, LONG GreenMask, LONG BlueMask, LONG AlphaMask);
static ERROR gfxCopyArea(struct rkBitmap * Bitmap, struct rkBitmap * Dest, LONG Flags, LONG X, LONG Y, LONG Width, LONG Height, LONG XDest, LONG YDest);
static ERROR gfxCopyStretch(struct rkBitmap * Bitmap, struct rkBitmap * Dest, LONG Flags, LONG X, LONG Y, LONG Width, LONG Height, LONG XDest, LONG YDest, LONG DestWidth, LONG DestHeight);
static void gfxReadRGBPixel(struct rkBitmap * Bitmap, LONG X, LONG Y, struct RGB8 ** RGB);
static ULONG gfxReadPixel(struct rkBitmap * Bitmap, LONG X, LONG Y);
static void gfxDrawRGBPixel(struct rkBitmap * Bitmap, LONG X, LONG Y, struct RGB8 * RGB);
static void gfxDrawPixel(struct rkBitmap * Bitmap, LONG X, LONG Y, ULONG Colour);
static void gfxDrawLine(struct rkBitmap * Bitmap, LONG X, LONG Y, LONG XEnd, LONG YEnd, ULONG Colour);
static void gfxDrawRectangle(struct rkBitmap * Bitmap, LONG X, LONG Y, LONG Width, LONG Height, ULONG Colour, LONG Flags);
static void gfxFlipBitmap(struct rkBitmap * Bitmap, LONG Orientation);
static void gfxSetClipRegion(struct rkBitmap * Bitmap, LONG Number, LONG Left, LONG Top, LONG Right, LONG Bottom, LONG Terminate);
static ERROR gfxCompress(struct rkBitmap * Bitmap, LONG Level);
static ERROR gfxDecompress(struct rkBitmap * Bitmap, LONG RetainData);
static ERROR gfxFlood(struct rkBitmap * Bitmap, LONG X, LONG Y, ULONG Colour);
static void gfxDrawEllipse(struct rkBitmap * Bitmap, LONG X, LONG Y, LONG Width, LONG Height, ULONG Colour, LONG Fill);
static ERROR gfxSubscribeInput(FUNCTION * Callback, OBJECTID SurfaceFilter, LONG Mask, OBJECTID DeviceFilter, LONG * Handle);
static ERROR gfxUnsubscribeInput(LONG Handle);
static CSTRING gfxGetInputTypeName(LONG Type);
static DOUBLE gfxScaleToDPI(DOUBLE Value);

#ifdef  __cplusplus
}
#endif
#ifndef FDEF
#define FDEF static const struct FunctionField
#endif

FDEF argsAccessPointer[] = { { "Object", FD_OBJECTPTR }, { 0, 0 } };
FDEF argsCompress[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "Level", FD_LONG }, { 0, 0 } };
FDEF argsCopyArea[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "Dest", FD_OBJECTPTR }, { "Flags", FD_LONG }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "XDest", FD_LONG }, { "YDest", FD_LONG }, { 0, 0 } };
FDEF argsCopyStretch[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "Dest", FD_OBJECTPTR }, { "Flags", FD_LONG }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "XDest", FD_LONG }, { "YDest", FD_LONG }, { "DestWidth", FD_LONG }, { "DestHeight", FD_LONG }, { 0, 0 } };
FDEF argsCopySurface[] = { { "Error", FD_LONG|FD_ERROR }, { "BitmapSurface:Surface", FD_PTR|FD_STRUCT }, { "Bitmap", FD_OBJECTPTR }, { "Flags", FD_LONG }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "XDest", FD_LONG }, { "YDest", FD_LONG }, { 0, 0 } };
FDEF argsDecompress[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "RetainData", FD_LONG }, { 0, 0 } };
FDEF argsDrawEllipse[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "Colour", FD_LONG }, { "Fill", FD_LONG }, { 0, 0 } };
FDEF argsDrawLine[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "XEnd", FD_LONG }, { "YEnd", FD_LONG }, { "Colour", FD_LONG }, { 0, 0 } };
FDEF argsDrawPixel[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_LONG }, { 0, 0 } };
FDEF argsDrawRGBPixel[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "RGB8:RGB", FD_PTR|FD_STRUCT }, { 0, 0 } };
FDEF argsDrawRectangle[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "Colour", FD_LONG }, { "Flags", FD_LONG }, { 0, 0 } };
FDEF argsFlipBitmap[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "Orientation", FD_LONG }, { 0, 0 } };
FDEF argsFlood[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Colour", FD_LONG }, { 0, 0 } };
FDEF argsGetColourFormat[] = { { "Void", FD_VOID }, { "ColourFormat:Format", FD_PTR|FD_STRUCT }, { "BitsPerPixel", FD_LONG }, { "RedMask", FD_LONG }, { "GreenMask", FD_LONG }, { "BlueMask", FD_LONG }, { "AlphaMask", FD_LONG }, { 0, 0 } };
FDEF argsGetCursorInfo[] = { { "Error", FD_LONG|FD_ERROR }, { "CursorInfo:Info", FD_PTR|FD_STRUCT }, { "Size", FD_LONG|FD_BUFSIZE }, { 0, 0 } };
FDEF argsGetCursorPos[] = { { "Error", FD_LONG|FD_ERROR }, { "X", FD_LONG|FD_RESULT }, { "Y", FD_LONG|FD_RESULT }, { 0, 0 } };
FDEF argsGetDisplayInfo[] = { { "Error", FD_LONG|FD_ERROR }, { "Display", FD_OBJECTID }, { "DisplayInfo:Info", FD_PTR|FD_STRUCT|FD_RESULT }, { 0, 0 } };
FDEF argsGetDisplayType[] = { { "Result", FD_LONG }, { 0, 0 } };
FDEF argsGetInputTypeName[] = { { "Result", FD_STR }, { "Type", FD_LONG }, { 0, 0 } };
FDEF argsGetRelativeCursorPos[] = { { "Error", FD_LONG|FD_ERROR }, { "Surface", FD_OBJECTID }, { "X", FD_LONG|FD_RESULT }, { "Y", FD_LONG|FD_RESULT }, { 0, 0 } };
FDEF argsLockCursor[] = { { "Error", FD_LONG|FD_ERROR }, { "Surface", FD_OBJECTID }, { 0, 0 } };
FDEF argsReadPixel[] = { { "Result", FD_LONG }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { 0, 0 } };
FDEF argsReadRGBPixel[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "RGB8:RGB", FD_PTR|FD_STRUCT|FD_RESULT }, { 0, 0 } };
FDEF argsResample[] = { { "Error", FD_LONG|FD_ERROR }, { "Bitmap", FD_OBJECTPTR }, { "ColourFormat:ColourFormat", FD_PTR|FD_STRUCT }, { 0, 0 } };
FDEF argsRestoreCursor[] = { { "Error", FD_LONG|FD_ERROR }, { "Cursor", FD_LONG }, { "Owner", FD_OBJECTID }, { 0, 0 } };
FDEF argsScaleToDPI[] = { { "Result", FD_DOUBLE }, { "Value", FD_DOUBLE }, { 0, 0 } };
FDEF argsScanDisplayModes[] = { { "Error", FD_LONG|FD_ERROR }, { "Filter", FD_STR }, { "DisplayInfo:Info", FD_PTR|FD_STRUCT }, { "Size", FD_LONG|FD_BUFSIZE }, { 0, 0 } };
FDEF argsSetClipRegion[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { "Number", FD_LONG }, { "Left", FD_LONG }, { "Top", FD_LONG }, { "Right", FD_LONG }, { "Bottom", FD_LONG }, { "Terminate", FD_LONG }, { 0, 0 } };
FDEF argsSetCursor[] = { { "Error", FD_LONG|FD_ERROR }, { "Surface", FD_OBJECTID }, { "Flags", FD_LONG }, { "Cursor", FD_LONG }, { "Name", FD_STR }, { "Owner", FD_OBJECTID }, { 0, 0 } };
FDEF argsSetCursorPos[] = { { "Error", FD_LONG|FD_ERROR }, { "X", FD_LONG }, { "Y", FD_LONG }, { 0, 0 } };
FDEF argsSetCustomCursor[] = { { "Error", FD_LONG|FD_ERROR }, { "Surface", FD_OBJECTID }, { "Flags", FD_LONG }, { "Bitmap", FD_OBJECTPTR }, { "HotX", FD_LONG }, { "HotY", FD_LONG }, { "Owner", FD_OBJECTID }, { 0, 0 } };
FDEF argsSetHostOption[] = { { "Error", FD_LONG|FD_ERROR }, { "Option", FD_LONG }, { "Value", FD_LARGE }, { 0, 0 } };
FDEF argsStartCursorDrag[] = { { "Error", FD_LONG|FD_ERROR }, { "Source", FD_OBJECTID }, { "Item", FD_LONG }, { "Datatypes", FD_STR }, { "Surface", FD_OBJECTID }, { 0, 0 } };
FDEF argsSubscribeInput[] = { { "Error", FD_LONG|FD_ERROR }, { "Callback", FD_FUNCTIONPTR }, { "SurfaceFilter", FD_OBJECTID }, { "Mask", FD_LONG }, { "DeviceFilter", FD_OBJECTID }, { "Handle", FD_LONG|FD_RESULT }, { 0, 0 } };
FDEF argsSync[] = { { "Void", FD_VOID }, { "Bitmap", FD_OBJECTPTR }, { 0, 0 } };
FDEF argsUnlockCursor[] = { { "Error", FD_LONG|FD_ERROR }, { "Surface", FD_OBJECTID }, { 0, 0 } };
FDEF argsUnsubscribeInput[] = { { "Error", FD_LONG|FD_ERROR }, { "Handle", FD_LONG }, { 0, 0 } };

const struct Function glFunctions[] = {
   { (APTR)gfxGetDisplayInfo, "GetDisplayInfo", argsGetDisplayInfo },
   { (APTR)gfxGetDisplayType, "GetDisplayType", argsGetDisplayType },
   { (APTR)gfxSetCursor, "SetCursor", argsSetCursor },
   { (APTR)gfxRestoreCursor, "RestoreCursor", argsRestoreCursor },
   { (APTR)gfxGetCursorPos, "GetCursorPos", argsGetCursorPos },
   { (APTR)gfxSetCursorPos, "SetCursorPos", argsSetCursorPos },
   { (APTR)gfxGetRelativeCursorPos, "GetRelativeCursorPos", argsGetRelativeCursorPos },
   { (APTR)gfxGetCursorInfo, "GetCursorInfo", argsGetCursorInfo },
   { (APTR)gfxSetCustomCursor, "SetCustomCursor", argsSetCustomCursor },
   { (APTR)gfxAccessPointer, "AccessPointer", argsAccessPointer },
   { (APTR)gfxScanDisplayModes, "ScanDisplayModes", argsScanDisplayModes },
   { (APTR)gfxLockCursor, "LockCursor", argsLockCursor },
   { (APTR)gfxUnlockCursor, "UnlockCursor", argsUnlockCursor },
   { (APTR)gfxSetHostOption, "SetHostOption", argsSetHostOption },
   { (APTR)gfxStartCursorDrag, "StartCursorDrag", argsStartCursorDrag },
   { (APTR)gfxCopySurface, "CopySurface", argsCopySurface },
   { (APTR)gfxSync, "Sync", argsSync },
   { (APTR)gfxResample, "Resample", argsResample },
   { (APTR)gfxGetColourFormat, "GetColourFormat", argsGetColourFormat },
   { (APTR)gfxCopyArea, "CopyArea", argsCopyArea },
   { (APTR)gfxCopyStretch, "CopyStretch", argsCopyStretch },
   { (APTR)gfxReadRGBPixel, "ReadRGBPixel", argsReadRGBPixel },
   { (APTR)gfxReadPixel, "ReadPixel", argsReadPixel },
   { (APTR)gfxDrawRGBPixel, "DrawRGBPixel", argsDrawRGBPixel },
   { (APTR)gfxDrawPixel, "DrawPixel", argsDrawPixel },
   { (APTR)gfxDrawLine, "DrawLine", argsDrawLine },
   { (APTR)gfxDrawRectangle, "DrawRectangle", argsDrawRectangle },
   { (APTR)gfxFlipBitmap, "FlipBitmap", argsFlipBitmap },
   { (APTR)gfxSetClipRegion, "SetClipRegion", argsSetClipRegion },
   { (APTR)gfxCompress, "Compress", argsCompress },
   { (APTR)gfxDecompress, "Decompress", argsDecompress },
   { (APTR)gfxFlood, "Flood", argsFlood },
   { (APTR)gfxDrawEllipse, "DrawEllipse", argsDrawEllipse },
   { (APTR)gfxSubscribeInput, "SubscribeInput", argsSubscribeInput },
   { (APTR)gfxUnsubscribeInput, "UnsubscribeInput", argsUnsubscribeInput },
   { (APTR)gfxGetInputTypeName, "GetInputTypeName", argsGetInputTypeName },
   { (APTR)gfxScaleToDPI, "ScaleToDPI", argsScaleToDPI },
   { NULL, NULL, NULL }
};

