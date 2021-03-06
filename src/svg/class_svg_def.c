// Auto-generated by idl-c.fluid

static const struct FieldDef clSVGFlags[] = {
   { "Alpha", 0x00000002 },
   { "Autoscale", 0x00000001 },
   { NULL, 0 }
};

FDEF maRender[] = { { "Bitmap", FD_OBJECTPTR }, { "X", FD_LONG }, { "Y", FD_LONG }, { "Width", FD_LONG }, { "Height", FD_LONG }, { 0, 0 } };

static const struct MethodArray clSVGMethods[] = {
   { -1, (APTR)SVG_Render, "Render", maRender, sizeof(struct svgRender) },
   { 0, 0, 0, 0, 0 }
};

static const struct ActionArray clSVGActions[] = {
   { AC_Activate, (APTR)SVG_Activate },
   { AC_DataFeed, (APTR)SVG_DataFeed },
   { AC_Deactivate, (APTR)SVG_Deactivate },
   { AC_Free, (APTR)SVG_Free },
   { AC_Init, (APTR)SVG_Init },
   { AC_NewObject, (APTR)SVG_NewObject },
   { AC_SaveImage, (APTR)SVG_SaveImage },
   { AC_SaveToObject, (APTR)SVG_SaveToObject },
   { 0, 0 }
};

