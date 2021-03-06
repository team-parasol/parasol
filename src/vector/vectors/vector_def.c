// Auto-generated by idl-c.fluid

static const struct FieldDef clVectorVisibility[] = {
   { "Hidden", 0x00000000 },
   { "Visible", 0x00000001 },
   { "Collapse", 0x00000002 },
   { "Inherit", 0x00000003 },
   { NULL, 0 }
};

FDEF maPush[] = { { "Position", FD_LONG }, { 0, 0 } };
FDEF maTracePath[] = { { "Callback", FD_FUNCTIONPTR }, { 0, 0 } };
FDEF maGetBoundary[] = { { "Flags", FD_LONG }, { "X", FD_DOUBLE|FD_RESULT }, { "Y", FD_DOUBLE|FD_RESULT }, { "Width", FD_DOUBLE|FD_RESULT }, { "Height", FD_DOUBLE|FD_RESULT }, { 0, 0 } };
FDEF maRotate[] = { { "Angle", FD_DOUBLE }, { "CenterX", FD_DOUBLE }, { "CenterY", FD_DOUBLE }, { 0, 0 } };
FDEF maTransform[] = { { "Transform", FD_STR }, { 0, 0 } };
FDEF maApplyMatrix[] = { { "A", FD_DOUBLE }, { "B", FD_DOUBLE }, { "C", FD_DOUBLE }, { "D", FD_DOUBLE }, { "E", FD_DOUBLE }, { "F", FD_DOUBLE }, { 0, 0 } };
FDEF maTranslate[] = { { "X", FD_DOUBLE }, { "Y", FD_DOUBLE }, { 0, 0 } };
FDEF maScale[] = { { "X", FD_DOUBLE }, { "Y", FD_DOUBLE }, { 0, 0 } };
FDEF maSkew[] = { { "X", FD_DOUBLE }, { "Y", FD_DOUBLE }, { 0, 0 } };
FDEF maPointInPath[] = { { "X", FD_DOUBLE }, { "Y", FD_DOUBLE }, { 0, 0 } };
FDEF maGetTransform[] = { { "Type", FD_LONG }, { "VectorTransform:Transform", FD_PTR|FD_STRUCT|FD_RESULT }, { 0, 0 } };

static const struct MethodArray clVectorMethods[] = {
   { -1, (APTR)VECTOR_Push, "Push", maPush, sizeof(struct vecPush) },
   { -2, (APTR)VECTOR_TracePath, "TracePath", maTracePath, sizeof(struct vecTracePath) },
   { -3, (APTR)VECTOR_GetBoundary, "GetBoundary", maGetBoundary, sizeof(struct vecGetBoundary) },
   { -4, (APTR)VECTOR_Rotate, "Rotate", maRotate, sizeof(struct vecRotate) },
   { -5, (APTR)VECTOR_Transform, "Transform", maTransform, sizeof(struct vecTransform) },
   { -6, (APTR)VECTOR_ApplyMatrix, "ApplyMatrix", maApplyMatrix, sizeof(struct vecApplyMatrix) },
   { -7, (APTR)VECTOR_Translate, "Translate", maTranslate, sizeof(struct vecTranslate) },
   { -8, (APTR)VECTOR_Scale, "Scale", maScale, sizeof(struct vecScale) },
   { -9, (APTR)VECTOR_Skew, "Skew", maSkew, sizeof(struct vecSkew) },
   { -10, (APTR)VECTOR_PointInPath, "PointInPath", maPointInPath, sizeof(struct vecPointInPath) },
   { -11, (APTR)VECTOR_ClearTransforms, "ClearTransforms", 0, 0 },
   { -12, (APTR)VECTOR_GetTransform, "GetTransform", maGetTransform, sizeof(struct vecGetTransform) },
   { 0, 0, 0, 0, 0 }
};

static const struct ActionArray clVectorActions[] = {
   { AC_ActionNotify, (APTR)VECTOR_ActionNotify },
   { AC_Free, (APTR)VECTOR_Free },
   { AC_Init, (APTR)VECTOR_Init },
   { AC_NewObject, (APTR)VECTOR_NewObject },
   { AC_NewOwner, (APTR)VECTOR_NewOwner },
   { AC_Reset, (APTR)VECTOR_Reset },
   { 0, 0 }
};

