// Auto-generated by idl-c.fluid

static const struct FieldDef clScrollFlags[] = {
   { "Fixed", 0x00000040 },
   { "Reverse", 0x00000008 },
   { "Vertical", 0x00000004 },
   { "Relative", 0x00000010 },
   { "AutoActivate", 0x00000080 },
   { "AutoHide", 0x00000001 },
   { "Invisible", 0x00000200 },
   { "Horizontal", 0x00000002 },
   { "Slider", 0x00000020 },
   { "Message", 0x00000100 },
   { NULL, 0 }
};

static const struct FieldDef clScrollAxis[] = {
   { "X", 0x00000000 },
   { "Y", 0x00000001 },
   { "Z", 0x00000002 },
   { NULL, 0 }
};

FDEF maUpdateScroll[] = { { "PageSize", FD_LONG }, { "ViewSize", FD_LONG }, { "Position", FD_LONG }, { "Unit", FD_LONG }, { 0, 0 } };
FDEF maAddScrollButton[] = { { "Surface", FD_OBJECTID }, { "Direction", FD_LONG }, { 0, 0 } };

static const struct MethodArray clScrollMethods[] = {
   { -1, (APTR)SCROLL_UpdateScroll, "UpdateScroll", maUpdateScroll, sizeof(struct scUpdateScroll) },
   { -2, (APTR)SCROLL_AddScrollButton, "AddScrollButton", maAddScrollButton, sizeof(struct scAddScrollButton) },
   { 0, 0, 0, 0, 0 }
};

static const struct ActionArray clScrollActions[] = {
   { AC_ActionNotify, (APTR)SCROLL_ActionNotify },
   { AC_Activate, (APTR)SCROLL_Activate },
   { AC_Free, (APTR)SCROLL_Free },
   { AC_Hide, (APTR)SCROLL_Hide },
   { AC_Init, (APTR)SCROLL_Init },
   { AC_NewObject, (APTR)SCROLL_NewObject },
   { AC_Show, (APTR)SCROLL_Show },
   { 0, 0 }
};

