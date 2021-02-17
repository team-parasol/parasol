// Auto-generated by idl-c.fluid

static const struct FieldDef clSurfaceFlags[] = {
   { "FullScreen", 0x02000000 },
   { "PostComposite", 0x01000000 },
   { "InitOnly", 0x06583981 },
   { "FastResize", 0x00000080 },
   { "AspectRatio", 0x08000000 },
   { "NoPrecomposite", 0x01000000 },
   { "ReadOnly", 0x00050240 },
   { "GrabFocus", 0x00000020 },
   { "Video", 0x00002000 },
   { "PervasiveCopy", 0x00100000 },
   { "Sticky", 0x00000010 },
   { "Precopy", 0x00001000 },
   { "Host", 0x00000800 },
   { "NoVertical", 0x00008000 },
   { "NoHorizontal", 0x00004000 },
   { "AutoQuit", 0x00000400 },
   { "Transparent", 0x00000001 },
   { "FixedBuffer", 0x00080000 },
   { "IgnoreFocus", 0x04000000 },
   { "WriteOnly", 0x00002000 },
   { "Visible", 0x00000008 },
   { "NoFocus", 0x00200000 },
   { "FixedDepth", 0x00400000 },
   { "Composite", 0x01000000 },
   { "HasFocus", 0x00000040 },
   { "StickToBack", 0x00000002 },
   { "Disabled", 0x00000100 },
   { "AfterCopy", 0x00040000 },
   { "Cursor", 0x00010000 },
   { "ScrollContent", 0x00020000 },
   { "Pointer", 0x00010000 },
   { "Volatile", 0x00051000 },
   { "TotalRedraw", 0x00800000 },
   { "Region", 0x00000200 },
   { "StickToFront", 0x00000004 },
   { NULL, 0 }
};

static const struct FieldDef clSurfaceAlign[] = {
   { "Bottom", 0x00000020 },
   { "Left", 0x00000001 },
   { "Horizontal", 0x00000004 },
   { "Top", 0x00000010 },
   { "Middle", 0x0000000c },
   { "Vertical", 0x00000008 },
   { "Center", 0x0000000c },
   { "Right", 0x00000002 },
   { NULL, 0 }
};

static const struct FieldDef clSurfaceDimensions[] = {
   { "RelativeHeight", 0x00000400 },
   { "FixedX", 0x00000004 },
   { "RelativeCenterY", 0x00080000 },
   { "YOffset", 0x000000a0 },
   { "FixedRadiusY", 0x02000000 },
   { "RelativeX", 0x00000001 },
   { "XOffset", 0x00000050 },
   { "RelativeCenterX", 0x00040000 },
   { "RelativeWidth", 0x00000800 },
   { "HorizontalFlags", 0x00000a55 },
   { "Width", 0x00000a00 },
   { "X", 0x00000005 },
   { "HeightFlags", 0x000005a0 },
   { "RelativeXOffset", 0x00000010 },
   { "StatusChangeH", 0x00400000 },
   { "RelativeDepth", 0x00002000 },
   { "RelativeRadiusY", 0x01000000 },
   { "FixedZ", 0x00004000 },
   { "StatusChange", 0x00c00000 },
   { "FixedDepth", 0x00001000 },
   { "VerticalFlags", 0x000005aa },
   { "Y", 0x0000000a },
   { "FixedCenterY", 0x00200000 },
   { "FixedRadius", 0x02020000 },
   { "RelativeRadius", 0x01010000 },
   { "FixedYOffset", 0x00000080 },
   { "WidthFlags", 0x00000a50 },
   { "FixedHeight", 0x00000100 },
   { "Height", 0x00000500 },
   { "RelativeYOffset", 0x00000020 },
   { "FixedXOffset", 0x00000040 },
   { "StatusChangeV", 0x00800000 },
   { "FixedY", 0x00000008 },
   { "RelativeY", 0x00000002 },
   { "FixedWidth", 0x00000200 },
   { "RelativeZ", 0x00008000 },
   { "FixedRadiusX", 0x00020000 },
   { "RelativeRadiusX", 0x00010000 },
   { "FixedCenterX", 0x00100000 },
   { NULL, 0 }
};

static const struct FieldDef clSurfaceDragStatus[] = {
   { "None", 0x00000000 },
   { "Anchor", 0x00000001 },
   { "Normal", 0x00000002 },
   { NULL, 0 }
};

static const struct FieldDef clSurfaceCursor[] = {
   { "NoChange", 0x00000000 },
   { "Default", 0x00000001 },
   { "SizeBottomLeft", 0x00000002 },
   { "SizeBottomRight", 0x00000003 },
   { "SizeTopLeft", 0x00000004 },
   { "SizeTopRight", 0x00000005 },
   { "SizeLeft", 0x00000006 },
   { "SizeRight", 0x00000007 },
   { "SizeTop", 0x00000008 },
   { "SizeBottom", 0x00000009 },
   { "Crosshair", 0x0000000a },
   { "Sleep", 0x0000000b },
   { "Sizing", 0x0000000c },
   { "SplitVertical", 0x0000000d },
   { "SplitHorizontal", 0x0000000e },
   { "Magnifier", 0x0000000f },
   { "Hand", 0x00000010 },
   { "HandLeft", 0x00000011 },
   { "HandRight", 0x00000012 },
   { "Text", 0x00000013 },
   { "Paintbrush", 0x00000014 },
   { "Stop", 0x00000015 },
   { "Invisible", 0x00000016 },
   { "Custom", 0x00000017 },
   { "Draggable", 0x00000018 },
   { NULL, 0 }
};

FDEF maInheritedFocus[] = { { "FocusID", FD_OBJECTID }, { "Flags", FD_LONG }, { 0, 0 } };
FDEF maExpose[] = { { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "Flags", FD_LONG }, { 0, 0 } };
FDEF maInvalidateRegion[] = { { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { 0, 0 } };
FDEF maSetDisplay[] = { { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { "InsideWidth", FD_LONG }, { "InsideHeight", FD_LONG }, { "BitsPerPixel", FD_LONG }, { "RefreshRate", FD_DOUBLE }, { "Flags", FD_LONG }, { 0, 0 } };
FDEF maSetOpacity[] = { { "Value", FD_DOUBLE }, { "Adjustment", FD_DOUBLE }, { 0, 0 } };
FDEF maAddCallback[] = { { "Callback", FD_FUNCTIONPTR }, { 0, 0 } };
FDEF maResetDimensions[] = { { "X", FD_DOUBLE }, { "Y", FD_DOUBLE }, { "XOffset", FD_DOUBLE }, { "YOffset", FD_DOUBLE }, { "Width", FD_DOUBLE }, { "Height", FD_DOUBLE }, { "Dimensions", FD_LONG }, { 0, 0 } };
FDEF maRemoveCallback[] = { { "Callback", FD_FUNCTIONPTR }, { 0, 0 } };

static const struct MethodArray clSurfaceMethods[] = {
   { -1, (APTR)SURFACE_InheritedFocus, "InheritedFocus", maInheritedFocus, sizeof(struct drwInheritedFocus) },
   { -2, (APTR)SURFACE_Expose, "Expose", maExpose, sizeof(struct drwExpose) },
   { -3, (APTR)SURFACE_InvalidateRegion, "InvalidateRegion", maInvalidateRegion, sizeof(struct drwInvalidateRegion) },
   { -4, (APTR)SURFACE_SetDisplay, "SetDisplay", maSetDisplay, sizeof(struct drwSetDisplay) },
   { -5, (APTR)SURFACE_SetOpacity, "SetOpacity", maSetOpacity, sizeof(struct drwSetOpacity) },
   { -6, (APTR)SURFACE_AddCallback, "AddCallback", maAddCallback, sizeof(struct drwAddCallback) },
   { -7, (APTR)SURFACE_Minimise, "Minimise", 0, 0 },
   { -8, (APTR)SURFACE_ResetDimensions, "ResetDimensions", maResetDimensions, sizeof(struct drwResetDimensions) },
   { -9, (APTR)SURFACE_RemoveCallback, "RemoveCallback", maRemoveCallback, sizeof(struct drwRemoveCallback) },
   { 0, 0, 0, 0, 0 }
};

static const struct ActionArray clSurfaceActions[] = {
   { AC_ActionNotify, (APTR)SURFACE_ActionNotify },
   { AC_Activate, (APTR)SURFACE_Activate },
   { AC_Disable, (APTR)SURFACE_Disable },
   { AC_Draw, (APTR)SURFACE_Draw },
   { AC_Enable, (APTR)SURFACE_Enable },
   { AC_Focus, (APTR)SURFACE_Focus },
   { AC_Free, (APTR)SURFACE_Free },
   { AC_Hide, (APTR)SURFACE_Hide },
   { AC_Init, (APTR)SURFACE_Init },
   { AC_LostFocus, (APTR)SURFACE_LostFocus },
   { AC_Move, (APTR)SURFACE_Move },
   { AC_MoveToBack, (APTR)SURFACE_MoveToBack },
   { AC_MoveToFront, (APTR)SURFACE_MoveToFront },
   { AC_MoveToPoint, (APTR)SURFACE_MoveToPoint },
   { AC_NewObject, (APTR)SURFACE_NewObject },
   { AC_NewOwner, (APTR)SURFACE_NewOwner },
   { AC_Redimension, (APTR)SURFACE_Redimension },
   { AC_ReleaseObject, (APTR)SURFACE_ReleaseObject },
   { AC_Resize, (APTR)SURFACE_Resize },
   { AC_SaveImage, (APTR)SURFACE_SaveImage },
   { AC_Scroll, (APTR)SURFACE_Scroll },
   { AC_ScrollToPoint, (APTR)SURFACE_ScrollToPoint },
   { AC_Show, (APTR)SURFACE_Show },
   { 0, 0 }
};

