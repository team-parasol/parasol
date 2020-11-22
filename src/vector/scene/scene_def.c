// Auto-generated by idl-c.fluid

static const struct FieldDef clVectorSceneFlags[] = {
   { "RenderTime", 0x00000002 },
   { "BitmapSized", 0x00000001 },
   { "Resize", 0x00000004 },
   { NULL, 0 }
};

static const struct FieldDef clVectorSceneSampleMethod[] = {
   { "Auto", 0x00000000 },
   { "Neighbour", 0x00000001 },
   { "Bilinear", 0x00000002 },
   { "Bicubic", 0x00000003 },
   { "Spline16", 0x00000004 },
   { "Kaiser", 0x00000005 },
   { "Quadric", 0x00000006 },
   { "Gaussian", 0x00000007 },
   { "Bessel", 0x00000008 },
   { "Mitchell", 0x00000009 },
   { "Sinc3", 0x0000000a },
   { "Lanczos3", 0x0000000b },
   { "Blackman3", 0x0000000c },
   { "Sinc8", 0x0000000d },
   { "Lanczos8", 0x0000000e },
   { "Blackman8", 0x0000000f },
   { NULL, 0 }
};

FDEF maAddDef[] = { { "Name", FD_STR }, { "Def", FD_OBJECTPTR }, { 0, 0 } };
FDEF maSearchByID[] = { { "ID", FD_LONG }, { "Result", FD_OBJECTPTR|FD_RESULT }, { 0, 0 } };
FDEF maFindDef[] = { { "Name", FD_STR }, { "Def", FD_OBJECTPTR|FD_RESULT }, { 0, 0 } };

static const struct MethodArray clVectorSceneMethods[] = {
   { -1, (APTR)VECTORSCENE_AddDef, "AddDef", maAddDef, sizeof(struct scAddDef) },
   { -2, (APTR)VECTORSCENE_SearchByID, "SearchByID", maSearchByID, sizeof(struct scSearchByID) },
   { -3, (APTR)VECTORSCENE_FindDef, "FindDef", maFindDef, sizeof(struct scFindDef) },
   { 0, 0, 0, 0, 0 }
};

static const struct ActionArray clVectorSceneActions[] = {
   { AC_Draw, (APTR)VECTORSCENE_Draw },
   { AC_Free, (APTR)VECTORSCENE_Free },
   { AC_Init, (APTR)VECTORSCENE_Init },
   { AC_NewObject, (APTR)VECTORSCENE_NewObject },
   { AC_Redimension, (APTR)VECTORSCENE_Redimension },
   { AC_Reset, (APTR)VECTORSCENE_Reset },
   { AC_Resize, (APTR)VECTORSCENE_Resize },
   { 0, 0 }
};
