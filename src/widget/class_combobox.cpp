/*****************************************************************************

The source code of the Parasol project is made publicly available under the
terms described in the LICENSE.TXT file that is distributed with this package.
Please refer to it for further information on licensing.

******************************************************************************

-CLASS-
ComboBox: The ComboBox class manages the display and interaction of user combo boxes.

The ComboBox class is used to create combo boxes, also known as 'drop-down menus' in application interfaces.  A
combobox typically looks like a text entry area, but features a button positioned to the right-hand side of the gadget.
Clicking on the button will pop-up a menu that the user can use to select a pre-defined menu item.  Clicking on one of
those items will paste the item text into the combobox.

A crucial feature of the combobox is the drop-down menu.  The combobox uses the @Menu class to support its menu
construction. To add items to the drop-down menu, you need to pass instructions to it using XML.  You can learn more
about the XML specification in the @Menu class manual.

When the user selects a combobox item, you may need to respond with an action.  You can do this by initialising child
objects to the combobox. These will be executed when the combobox is activated.  When programming, you can also
subscribe to the combobox's Activate action and write a customised response routine.

The id of the most recently selected menu item can be retrieved from the #SelectedID field.

To make modifications to the menu after initialisation, read the #Menu field and manipulate it directly.

-END-

*****************************************************************************/

#define PRV_COMBOBOX
#define PRV_WIDGET_MODULE
#include <parasol/modules/widget.h>
#include <parasol/modules/display.h>
#include <parasol/modules/font.h>
#include "defs.h"

static OBJECTPTR clCombobox = NULL;

//****************************************************************************

static void style_trigger(objComboBox *Self, LONG Style)
{
   if (Self->prvStyleTrigger.Type IS CALL_SCRIPT) {
      OBJECTPTR script;
      if ((script = Self->prvStyleTrigger.Script.Script)) {
         const ScriptArg args[] = {
            { "ComboBox", FD_OBJECTPTR, { .Address = Self } },
            { "Style", FD_LONG,         { .Long = Style } }
         };
         scCallback(script, Self->prvStyleTrigger.Script.ProcedureID, args, ARRAYSIZE(args), NULL);
      }
   }
}

//****************************************************************************

static ERROR COMBOBOX_ActionNotify(objComboBox *Self, struct acActionNotify *Args)
{
   if (Args->Error != ERR_Okay) return ERR_Okay;

   if (Args->ActionID IS AC_Free) {
      if ((Self->Feedback.Type IS CALL_SCRIPT) and (Self->Feedback.Script.Script->UniqueID IS Args->ObjectID)) {
         Self->Feedback.Type = CALL_NONE;
      }
   }
   else return ERR_NoSupport;

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Clear: Clears the content of the combobox list box.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Clear(objComboBox *Self, APTR Void)
{
   return acClear(Self->Menu);
}

/*****************************************************************************
-ACTION-
Disable: Turns the combobox off.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Disable(objComboBox *Self, APTR Void)
{
   Self->Flags |= CMF_DISABLED;
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Enable: Turns the combobox back on if it has previously been disabled.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Enable(objComboBox *Self, APTR Void)
{
   Self->Flags &= ~CMF_DISABLED;
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Focus: Sets the focus on the combobox.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Focus(objComboBox *Self, APTR Void)
{
   return acFocus(Self->Viewport);
}

//****************************************************************************

static ERROR COMBOBOX_Free(objComboBox *Self, APTR Void)
{
   if (Self->TextInput) { acFree(Self->TextInput); Self->TextInput = NULL; }
   if (Self->Menu)      { acFree(Self->Menu); Self->Menu = NULL; }
   if (Self->Viewport)  { acFree(Self->Viewport); Self->Viewport = NULL; }
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Hide: Removes the combobox from the display.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Hide(objComboBox *Self, APTR Void)
{
   Self->Flags |= CMF_HIDE;
   return acHide(Self->Viewport);
}

//****************************************************************************

static ERROR COMBOBOX_Init(objComboBox *Self, APTR Void)
{
   if (!Self->ParentViewport) { // Find our parent viewport
      OBJECTID owner_id;
      for (owner_id=GetOwner(Self); (owner_id); owner_id=GetOwnerID(owner_id)) {
          if (GetClassID(owner_id) IS ID_VECTOR) {
             Self->ParentViewport = (objVector *)GetObjectPtr(owner_id);
             if ((Self->ParentViewport->Head.SubID != ID_VECTORVIEWPORT) and
                 (Self->ParentViewport->Head.SubID != ID_VECTORSCENE)) return ERR_UnsupportedOwner;
             else break;
          }
      }
      if (!owner_id) return ERR_UnsupportedOwner;
   }

   Self->Viewport->Parent = &Self->ParentViewport->Head;

   if (Self->Flags & CMF_HIDE) Self->Viewport->Visibility = VIS_HIDDEN;

   if (!acInit(Self->Viewport)) {
      if (drwApplyStyleGraphics(Self, Self->Viewport->Head.UniqueID, NULL, NULL)) {
         return ERR_Failed; // Graphics styling is required.
      }

      //Self->Viewport->Flags |= VF_GRAB_FOCUS;
   }
   else return ERR_Init;

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
MoveToBack: Moves the combobox behind its siblings.
-END-
*****************************************************************************/

static ERROR COMBOBOX_MoveToBack(objComboBox *Self, APTR Void)
{
   return acMoveToBack(Self->Viewport);
}

/*****************************************************************************
-ACTION-
MoveToFront: Moves the combobox in front of its siblings.
-END-
*****************************************************************************/

static ERROR COMBOBOX_MoveToFront(objComboBox *Self, APTR Void)
{
   return acMoveToFront(Self->Viewport);
}

//****************************************************************************

static ERROR COMBOBOX_NewObject(objComboBox *Self, APTR Void)
{
   if (!NewObject(ID_VECTORVIEWPORT, NF_INTEGRAL, &Self->Viewport)) {
      if (!NewObject(ID_VECTORTEXT, NF_INTEGRAL, &Self->TextInput)) {
         SetOwner(Self->TextInput, Self->Viewport);
         if (!NewObject(ID_MENU, NF_INTEGRAL, &Self->Menu)) {
            SetString(Self->TextInput, FID_Face, glWidgetFace);
            drwApplyStyleValues(Self, NULL);
            return ERR_Okay;
         }
         else return ERR_NewObject;
      }
      else return ERR_NewObject;
   }
   else return ERR_NewObject;
}

/*****************************************************************************
-ACTION-
Redimension: Changes the size and position of the combobox.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Redimension(objComboBox *Self, struct acRedimension *Args)
{
   return Action(AC_Redimension, Self->Viewport, Args);
}

/*****************************************************************************
-ACTION-
Resize: Alters the size of the combobox.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Resize(objComboBox *Self, struct acResize *Args)
{
   return Action(AC_Resize, Self->Viewport, Args);
}

/*****************************************************************************
-ACTION-
SetVar: Arguments can be passed through to the combobox menu via unlisted fields.
-END-
*****************************************************************************/

static ERROR COMBOBOX_SetVar(objComboBox *Self, struct acSetVar *Args)
{
   return Action(AC_SetVar, Self->Menu, Args);
}

/*****************************************************************************
-ACTION-
Show: Puts the combobox on display.
-END-
*****************************************************************************/

static ERROR COMBOBOX_Show(objComboBox *Self, APTR Void)
{
   return acShow(Self->Viewport);
}

/*****************************************************************************

-FIELD-
Align: Manages the alignment of a combobox surface within its container.

The position of a combobox object can be abstractly defined with alignment instructions by setting this field.  The
alignment feature takes precedence over values in coordinate fields such as #X and #Y.

*****************************************************************************/

static ERROR SET_Align(objComboBox *Self, LONG Value)
{
   //Self->Viewport->Align = Value;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Bottom: The bottom coordinate of the combobox (Y + Height).

*****************************************************************************/

static ERROR GET_Bottom(objComboBox *Self, LONG *Value)
{
   DOUBLE y, height;
   if (!GetFields(Self->Viewport, FID_Y|TDOUBLE, &y, FID_Height|TDOUBLE, &height, TAGEND)) {
      *Value = F2T(y + height);
      return ERR_Okay;
   }
   else return ERR_GetField;
}

/*****************************************************************************

-FIELD-
Disable: Disables the combobox on initialisation.

The combobox can be disabled on initialisation by setting this field to TRUE.  If you need to disable the combobox
after it has been activated, it is preferred that you use the Disable action.

To enable the combobox after it has been disabled, use the Enable action.

*****************************************************************************/

static ERROR GET_Disable(objComboBox *Self, LONG *Value)
{
   if (Self->Flags & CMF_DISABLED) *Value = TRUE;
   else *Value = FALSE;
   return ERR_Okay;
}

static ERROR SET_Disable(objComboBox *Self, LONG Value)
{
   if (Value IS TRUE) return acDisable(Self);
   else return acEnable(Self);
}

/*****************************************************************************

-FIELD-
Feedback: Provides instant feedback when a user interacts with the Combobox.

Set the Feedback field with a callback function in order to receive instant feedback when user interaction occurs.  The
function prototype is `routine(*ComboBox)`

*****************************************************************************/

static ERROR GET_Feedback(objComboBox *Self, FUNCTION **Value)
{
   if (Self->Feedback.Type != CALL_NONE) {
      *Value = &Self->Feedback;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_Feedback(objComboBox *Self, FUNCTION *Value)
{
   if (Value) {
      if (Self->Feedback.Type IS CALL_SCRIPT) UnsubscribeAction(Self->Feedback.Script.Script, AC_Free);
      Self->Feedback = *Value;
      if (Self->Feedback.Type IS CALL_SCRIPT) SubscribeAction(Self->Feedback.Script.Script, AC_Free);
   }
   else Self->Feedback.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Flags: Optional flags may be defined here.

-FIELD-
Height: Defines the height of the combobox.

An combobox can be given a fixed or relative height by setting this field to the desired value.  To set a relative
height, use the FD_PERCENT flag when setting the field.

*****************************************************************************/

static ERROR GET_Height(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_Height, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_Height, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_Height(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_Height, Value);
}

/*****************************************************************************

-FIELD-
Label: The label is a string displayed to the left of the combobox area.

A label can be drawn next to the combobox area by setting the Label field.  The label should be a short, descriptive
string of one or two words.  It is common practice for the label to be followed with a colon character.

*****************************************************************************/

static ERROR GET_Label(objComboBox *Self, STRING *Value)
{
   *Value = Self->Label;
   return ERR_Okay;
}

static ERROR SET_Label(objComboBox *Self, CSTRING Value)
{
   if (Value) StrCopy(StrTranslateText(Value), Self->Label, sizeof(Self->Label));
   else Self->Label[0] = 0;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
LabelWidth: A set-width for the label area of the combobox may be defined here.

If you set a label for the combobox, the width of the label area is automatically calculated according to the width of
the label string.  You may override this behaviour by setting a value in the LabelWidth field.

-FIELD-
Menu: Provides direct access to the drop-down menu.

The drop-down menu that is used for the combobox can be accessed directly through this field.  You may find this useful
for manipulating the content of the drop-down menu following initialisation of the combobox.

-FIELD-
Right: The right-most coordinate of the combobox (X + Width).

*****************************************************************************/

static ERROR GET_Right(objComboBox *Self, LONG *Value)
{
   DOUBLE x, width;
   if (!GetFields(Self->Viewport, FID_X|TDOUBLE, &x, FID_Width|TDOUBLE, &width, TAGEND)) {
      *Value = F2T(x + width);
      return ERR_Okay;
   }
   else return ERR_GetField;
}

/*****************************************************************************

-FIELD-
SelectedID: Returns the menu ID of the selected combobox item.

This field returns the menu ID of the selected combobox item.  This requires that an id is set for each configured menu
item (the 'id' attribute).

Menu ID's are not guaranteed to be unique.  It is your responsibility to assign ID's and ensure that they are unique to
prevent an ID from matching multiple items.

If the combobox text does not reflect one of the available menu items, then the returned value will be -1.  If the
selected menu item has no identifier, the default return value is 0.

*****************************************************************************/

static ERROR GET_SelectedID(objComboBox *Self, LONG *Value)
{
   *Value = -1;

   STRING str;
   if (!GetString(Self->TextInput, FID_String, &str)) {
      for (auto item=Self->Menu->Items; item; item=item->Next) {
         if (!StrMatch(str, item->Text)) {
            *Value = item->ID;
            return ERR_Okay;
         }
      }
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
String: The text that is to be printed inside the combobox.

The text string to display in the combobox is declared in this field.  The string must be in UTF-8
format and may not contain line feeds.  This field can be read field at any time to determine what the user has entered in
the combobox.

If the string is changed after initialisation, the combobox will be redrawn to show the updated text.  No feedback
notification will be sent as a result of updating this field manually.

*****************************************************************************/

static ERROR GET_String(objComboBox *Self, STRING *Value)
{
   STRING str;
   if (!GetString(Self->TextInput, FID_String, &str)) {
      *Value = str;
      return ERR_Okay;
   }
   else return ERR_FieldNotSet;
}

static ERROR SET_String(objComboBox *Self, CSTRING Value)
{
   // Do nothing if the string will remain unchanged

   STRING original;
   if ((!GetString(Self->TextInput, FID_String, &original)) and (original)) {
      if (!StrMatch(original, Value)) return ERR_Okay;
   }

   if (!SetString(Self->TextInput, FID_String, Value)) {
      if (Self->Head.Flags & NF_INITIALISED) style_trigger(Self, STYLE_CONTENT);
      return ERR_Okay;
   }
   else return ERR_Failed;
}

/*****************************************************************************

-FIELD-
Surface: The surface that will contain the combobox graphic.

The surface that will contain the combobox graphic is set here.  If this field is not set prior to initialisation, the
combobox will attempt to scan for the correct surface by analysing its parents until it finds a suitable candidate.

-FIELD-
StyleTrigger: Requires a callback for reporting changes that can affect graphics styling.

This field is reserved for use by the style code that is managing the widget graphics.

*****************************************************************************/

static ERROR SET_StyleTrigger(objComboBox *Self, FUNCTION *Value)
{
   if (Value) {
      if (Self->prvStyleTrigger.Type IS CALL_SCRIPT) UnsubscribeAction(Self->prvStyleTrigger.Script.Script, AC_Free);
      Self->prvStyleTrigger = *Value;
      if (Self->prvStyleTrigger.Type IS CALL_SCRIPT) SubscribeAction(Self->prvStyleTrigger.Script.Script, AC_Free);
   }
   else Self->prvStyleTrigger.Type = CALL_NONE;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
TabFocus: Set this field to a TabFocus object to register the combobox in a tab-list.

The TabFocus field provides a convenient way of adding the combobox to a @TabFocus object, so that it can be
focussed on via the tab key.  Simply set this field to the ID of the @TabFocus object that is managing the
tab-list for the application window.

*****************************************************************************/

static ERROR SET_TabFocus(objComboBox *Self, OBJECTPTR Value)
{
   if ((Value) and (Value->ClassID IS ID_TABFOCUS)) {
      tabAddObject(Value, Self->Viewport->Head.UniqueID);
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
TextInput: The text control object created for the combobox is referenced here.

The text object that is referenced here manages the display and editing of text inside the combobox area.
Characteristics of the text object can be defined prior to initialisation, although we recommend that this be done from
the combobox style definition.

The face and point size of the text is set by the widget styling code and therefore cannot be changed through the
TextInput object directly.

-FIELD-
Width: Defines the width of a combobox.

A combobox can be given a fixed or relative width by setting this field to the desired value.  To set a relative width,
use the FD_PERCENT flag when setting the field.

*****************************************************************************/

static ERROR GET_Width(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_Width, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_Width, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_Width(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_Width, Value);
}

/*****************************************************************************

-FIELD-
X: The horizontal position of a combobox.

The horizontal position of a combobox can be set to an absolute or relative coordinate by writing a value to the X
field.  To set a relative/percentage based value, you must use the FD_PERCENT flag or the value will be interpreted as
fixed.  Negative values are permitted.

*****************************************************************************/

static ERROR GET_X(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_X, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_X, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_X(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_X, Value);
}

/*****************************************************************************

-FIELD-
XOffset: The horizontal offset of a combobox.

The XOffset has a dual purpose depending on whether or not it is set in conjunction with an X coordinate or a Width
based field.

If set in conjunction with an X coordinate then the combobox will be drawn from that X coordinate up to the width of
the container, minus the value given in the XOffset.  This means that the width of the ComboBox is dynamically
calculated in relation to the width of the container.

If the XOffset field is set in conjunction with a fixed or relative width then the combobox will be drawn at an X
coordinate calculated from the formula `X = ContainerWidth - ComboBoxWidth - XOffset`.

*****************************************************************************/

static ERROR GET_XOffset(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_XOffset, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_XOffset, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_XOffset(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_XOffset, Value);
}

/*****************************************************************************

-FIELD-
Y: The vertical position of a combobox.

The vertical position of a ComboBox can be set to an absolute or relative coordinate by writing a value to the Y
field.  To set a relative/percentage based value, you must use the FD_PERCENT flag or the value will be interpreted as
fixed.  Negative values are permitted.

*****************************************************************************/

static ERROR GET_Y(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_Y, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_Y, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_Y(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_Y, Value);
}

/*****************************************************************************

-FIELD-
YOffset: The vertical offset of a combobox.

The YOffset has a dual purpose depending on whether or not it is set in conjunction with a Y coordinate or a Height
based field.

If set in conjunction with a Y coordinate then the combobox will be drawn from that Y coordinate up to the height of
the container, minus the value given in the YOffset.  This means that the height of the combobox is dynamically
calculated in relation to the height of the container.

If the YOffset field is set in conjunction with a fixed or relative height then the combobox will be drawn at a Y
coordinate calculated from the formula `Y = ContainerHeight - ComboBoxHeight - YOffset`.
-END-

*****************************************************************************/

static ERROR GET_YOffset(objComboBox *Self, Variable *Value)
{
   if (Value->Type & FD_DOUBLE) return GetDouble(Self->Viewport, FID_YOffset, &Value->Double);
   else if (Value->Type & FD_LARGE) return GetLarge(Self->Viewport, FID_YOffset, &Value->Large);
   else return ERR_FieldTypeMismatch;
}

static ERROR SET_YOffset(objComboBox *Self, Variable *Value)
{
   return SetVariable(Self->Viewport, FID_YOffset, Value);
}

//****************************************************************************

#include "class_combobox_def.c"

static const FieldDef Align[] = {
   { "Right",    ALIGN_RIGHT    }, { "Left",       ALIGN_LEFT    },
   { "Bottom",   ALIGN_BOTTOM   }, { "Top",        ALIGN_TOP     },
   { "Center",   ALIGN_CENTER   }, { "Middle",     ALIGN_MIDDLE  },
   { "Vertical", ALIGN_VERTICAL }, { "Horizontal", ALIGN_HORIZONTAL },
   { NULL, 0 }
};

static const FieldArray clFields[] = {
   { "TextInput",      FDF_INTEGRAL|FDF_R,   0, NULL, NULL },
   { "Menu",           FDF_INTEGRAL|FDF_R,   0, NULL, NULL },
   { "Viewport",       FDF_OBJECT|FDF_R,     ID_VECTORVIEWPORT, NULL, NULL },
   { "ParentViewport", FDF_OBJECT|FDF_RI,    ID_VECTORVIEWPORT, NULL, NULL },
   { "Flags",          FDF_LONGFLAGS|FDF_RW, (MAXINT)&clComboBoxFlags, NULL, NULL },
   { "LabelWidth",     FDF_LONG|FDF_RI,      0, NULL, NULL },
   // Virtual fields
   { "Align",         FDF_VIRTUAL|FDF_LONGFLAGS|FDF_I,    (MAXINT)&Align,  NULL, (APTR)SET_Align },
   { "Bottom",        FDF_VIRTUAL|FDF_LONG|FDF_R,         0, (APTR)GET_Bottom, NULL },
   { "Disable",       FDF_VIRTUAL|FDF_LONG|FDF_RW,        0, (APTR)GET_Disable, (APTR)SET_Disable },
   { "Feedback",      FDF_VIRTUAL|FDF_FUNCTIONPTR|FDF_RW, 0, (APTR)GET_Feedback, (APTR)SET_Feedback },
   { "Label",         FDF_VIRTUAL|FDF_STRING|FDF_RW,      0, (APTR)GET_Label, (APTR)SET_Label },
   { "Right",         FDF_VIRTUAL|FDF_LONG|FDF_R,         0, (APTR)GET_Right, NULL },
   { "SelectedID",    FDF_VIRTUAL|FDF_LONG|FDF_R,         0, (APTR)GET_SelectedID, NULL },
   { "String",        FDF_VIRTUAL|FDF_STRING|FDF_RW,      0, (APTR)GET_String, (APTR)SET_String },
   { "StyleTrigger",  FDF_VIRTUAL|FDF_FUNCTIONPTR|FDF_W,  0, NULL, (APTR)SET_StyleTrigger },
   { "TabFocus",      FDF_VIRTUAL|FDF_OBJECT|FDF_I,       ID_TABFOCUS, NULL, (APTR)SET_TabFocus },
   { "Text",          FDF_SYNONYM|FDF_VIRTUAL|FDF_STRING|FDF_RW, 0, (APTR)GET_String, (APTR)SET_String },
   // Variable Fields
   { "Height",  FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_Height,  (APTR)SET_Height },
   { "Width",   FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_Width,   (APTR)SET_Width },
   { "X",       FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_X,       (APTR)SET_X },
   { "XOffset", FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_XOffset, (APTR)SET_XOffset },
   { "Y",       FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_Y,       (APTR)SET_Y },
   { "YOffset", FDF_VIRTUAL|FDF_VARIABLE|FDF_DOUBLE|FDF_PERCENTAGE|FDF_RW, 0, (APTR)GET_YOffset, (APTR)SET_YOffset },
   END_FIELD
};

//****************************************************************************

ERROR init_combobox(void)
{
   return CreateObject(ID_METACLASS, 0, &clCombobox,
      FID_ClassVersion|TFLOAT, VER_COMBOBOX,
      FID_Name|TSTRING,   "ComboBox",
      FID_Category|TLONG, CCF_GUI,
      FID_Flags|TLONG,    CLF_PROMOTE_INTEGRAL,
      FID_Actions|TPTR,   clComboBoxActions,
      FID_Fields|TARRAY,  clFields,
      FID_Size|TLONG,     sizeof(objComboBox),
      FID_Path|TSTR,      MOD_PATH,
      TAGEND);
}

void free_combobox(void)
{
   if (clCombobox) { acFree(clCombobox); clCombobox = NULL; }
}
