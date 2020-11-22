/*****************************************************************************

-CLASS-
Pointer: Used to support mouse pointers.

The Pointer class provides the computer user with a means of interacting with the graphical interface.  Traditionally
the pointer is controlled by an attached mouse device, but the use of a keyboard, joystick or touch-screen is a
possibility.  The Pointer class operates in 3 dimensions (X, Y, Z) and the Z axis is typically controlled by a mouse
wheel.

A pointer object should usually be created at boot-up unless you don't want a pointer in your system (e.g. for
keyboard-only input).  Not creating a pointer will also mean that the system will not support user-click and
user-movement management, unless you have some other means of achieving this. It is recommended that when creating a
pointer at boot-up you give it a name of "SystemPointer".  This is a system-wide standard that makes it easier for
other objects to find the pointing device.  If you give it a different name then the pointer will effectively be hidden
from other objects.

The Pointer class manages action-events such as UserMovement, UserClick and UserClickRelease.  These actions are called
and passed to other objects on the desktop as appropriate.

-END-

*****************************************************************************/

static const struct FieldDef CursorLookup[] = {
   { "None",            0 },
   { "Default",         PTR_DEFAULT },             // Values start from 1 and go up
   { "SizeBottomLeft",  PTR_SIZE_BOTTOM_LEFT },
   { "SizeBottomRight", PTR_SIZE_BOTTOM_RIGHT },
   { "SizeTopLeft",     PTR_SIZE_TOP_LEFT },
   { "SizeTopRight",    PTR_SIZE_TOP_RIGHT },
   { "SizeLeft",        PTR_SIZE_LEFT },
   { "SizeRight",       PTR_SIZE_RIGHT },
   { "SizeTop",         PTR_SIZE_TOP },
   { "SizeBottom",      PTR_SIZE_BOTTOM },
   { "Crosshair",       PTR_CROSSHAIR },
   { "Sleep",           PTR_SLEEP },
   { "Sizing",          PTR_SIZING },
   { "SplitVertical",   PTR_SPLIT_VERTICAL },
   { "SplitHorizontal", PTR_SPLIT_HORIZONTAL },
   { "Magnifier",       PTR_MAGNIFIER },
   { "Hand",            PTR_HAND },
   { "HandLeft",        PTR_HAND_LEFT },
   { "HandRight",       PTR_HAND_RIGHT },
   { "Text",            PTR_TEXT },
   { "Paintbrush",      PTR_PAINTBRUSH },
   { "Stop",            PTR_STOP },
   { "Invisible",       PTR_INVISIBLE },
   { "Custom",          PTR_CUSTOM },
   { "Dragable",        PTR_DRAGGABLE },
   { NULL, 0 }
};

static ERROR GET_ButtonOrder(objPointer *, STRING *);
static ERROR GET_ButtonState(objPointer *, LONG *);

static ERROR SET_ButtonOrder(objPointer *, STRING);
static ERROR SET_MaxSpeed(objPointer *, LONG);
static ERROR PTR_SET_X(objPointer *, LONG);
static ERROR PTR_SET_Y(objPointer *, LONG);

static const struct FieldArray clPointerFields[];
static const struct MethodArray clPointerMethods[];
static const struct ActionArray clPointerActions[];

#ifdef _WIN32
static ERROR PTR_SetWinCursor(objPointer *, struct ptrSetWinCursor *);
static struct FunctionField mthSetWinCursor[]  = { { "Cursor", FD_LONG }, { NULL, 0 } };
#endif

#ifdef __xwindows__
static ERROR PTR_GrabX11Pointer(objPointer *, struct ptrGrabX11Pointer *);
static ERROR PTR_UngrabX11Pointer(objPointer *, APTR);
static struct FunctionField mthGrabX11Pointer[] = { { "Surface", FD_LONG }, { NULL, 0 } };
#endif

static LONG glDefaultSpeed = 160;
static FLOAT glDefaultAcceleration = 0.8;
static TIMER glRepeatTimer = 0;
OBJECTID glOverTaskID = 0; // Task that owns the surface that the cursor is positioned over

static ERROR repeat_timer(objPointer *, LARGE, LARGE);
static void set_pointer_defaults(objPointer *);
static WORD examine_chain(objPointer *, WORD, struct SurfaceControl *, WORD);
static BYTE get_over_object(objPointer *);
static void process_ptr_button(objPointer *, struct dcDeviceInput *);
static void process_ptr_movement(objPointer *, struct dcDeviceInput *);
static void process_ptr_wheel(objPointer *, struct dcDeviceInput *);
static void send_inputmsg(struct InputMsg *input, struct InputSubscription *List);

//****************************************************************************

INLINE void call_userinput(STRING Debug, struct InputMsg *input, LONG Flags, OBJECTID RecipientID, OBJECTID OverID,
   LONG AbsX, LONG AbsY, LONG OverX, LONG OverY)
{
   struct InputSubscription *list;

   if ((glSharedControl->InputMID) AND (!AccessMemory(glSharedControl->InputMID, MEM_READ, 1000, &list))) {
      //FMSG("call_userinput()","Type: %s, Value: %.2f, Recipient: %d, Over: %d, %dx%d %s", (input->Type < JET_END) ? glInputNames[input->Type] : (STRING)"", input->Value, RecipientID, OverID, AbsX, AbsY, Debug);

      input->Mask        = glInputType[input->Type].Mask;
      input->Flags       = glInputType[input->Type].Flags | Flags;
      input->RecipientID = RecipientID;
      input->OverID      = OverID;
      input->AbsX        = AbsX;
      input->AbsY        = AbsY;
      input->X           = OverX;
      input->Y           = OverY;
      send_inputmsg(input, list);
      ReleaseMemory(list);
   }
}

//****************************************************************************

static void send_inputmsg(struct InputMsg *input, struct InputSubscription *List)
{
   struct acDataFeed dc_inputready;
   struct dcDisplayInputReady inputready;
   LONG i, msgindex;
   ERROR error;

   // Store the message in the input message queue

   inputready.NextIndex   = glInput->IndexCounter++;
   dc_inputready.ObjectID = glPointerID;
   dc_inputready.DataType = DATA_INPUT_READY;
   dc_inputready.Buffer   = &inputready;
   dc_inputready.Size     = sizeof(inputready);

   msgindex = inputready.NextIndex & INPUT_MASK;
   CopyMemory(input, glInput->Msgs+msgindex, sizeof(struct InputMsg));

   for (i=0; i < glSharedControl->InputTotal; i++) {
      if ((!List[i].SurfaceID) OR (List[i].SurfaceID IS input->RecipientID)) {
         if ((List[i].Mask & input->Mask) != input->Mask) continue;

         //FMSG("send_inputmsg:","Object #%d, Surface #%d vs #%d, Mask: $%.8x vs $%.8x, MsgSent %d, Port %d", List[i].SubscriberID, List[i].SurfaceID, input->RecipientID, List[i].Mask, input->Mask, List[i].MsgSent, List[i].MsgPort);

         // Send an input ready message if the subscriber is known to be up to date with its messages.

         List[i].LastIndex = glInput->IndexCounter;

         if (List[i].MsgSent IS FALSE) {
            inputready.SubIndex = i;
            error = ActionMsgPort(AC_DataFeed, List[i].SubscriberID, &dc_inputready, List[i].MsgPort, -1);
            List[i].MsgSent = TRUE;

            if (error IS ERR_NoMatchingObject) {
               // Remove the subscriber if its message port no longer exists

               LogF("@send_inputmsg","Subscriber #%d invalid, removing from input subscription array.", List[i].SubscriberID);

               if (i+1 < glSharedControl->InputTotal) {
                  CopyMemory(List+i+1, List+i, sizeof(struct InputSubscription) * (glSharedControl->InputTotal - i - 1));
               }
               else ClearMemory(List+i, sizeof(List[i]));

               i--; // Offset the subsequent i++ of the for loop
               glSharedControl->InputTotal--;
            }
         }
      }
   }
}

//****************************************************************************

static ERROR create_pointer_class(void)
{
#ifdef __snap__
   struct utsname syslinux;
   WORD version, release;
   STRING str;

   // In Linux kernel version 2.6.x+, the mouse management system was changed so that all mouse communication is
   // expected to go through /dev/input/mice.  It is not possible to communicate directly with mouse devices over
   // /dev/input/mice, because it's an abstract service.

   if (!uname(&syslinux)) {
      str = syslinux.release;
      version = StrToInt(str);
      while ((*str >= '0') AND (*str <= '9')) str++;
      while ((*str) AND ((*str < '0') OR (*str > '9'))) str++;
      release = StrToInt(str);

      if (((version IS 2) AND (release >= 6)) OR (version >= 3)) {
         glModernInput = TRUE;
      }
      else glModernInput = FALSE;
   }

   if (glModernInput) LogMsg("Modern input system will be used for mouse communication.");
   else LogMsg("Old-style input system will be used for direct mouse communication.");
#endif

   return(CreateObject(ID_METACLASS, 0, &clPointer,
      FID_BaseClassID|TLONG,   ID_POINTER,
      FID_ClassVersion|TFLOAT, VER_POINTER,
      FID_Name|TSTRING,   "Pointer",
      FID_Category|TLONG, CCF_GRAPHICS,
      FID_Actions|TPTR,   clPointerActions,
      FID_Methods|TARRAY, clPointerMethods,
      FID_Fields|TARRAY,  clPointerFields,
      FID_Size|TLONG,     sizeof(objPointer),
      FID_Flags|TLONG,    CLF_SHARED_ONLY,
      FID_Path|TSTR,      MOD_PATH,
      TAGEND));
}

/*****************************************************************************

*****************************************************************************/

#ifdef _WIN32
static ERROR PTR_SetWinCursor(objPointer *Self, struct ptrSetWinCursor *Args)
{
   winSetCursor(GetWinCursor(Args->Cursor));
   Self->CursorID = Args->Cursor;
   return ERR_Okay;
}
#endif

/*****************************************************************************
** Private action used to grab the window cursor under X11.  Can only be executed by the task that owns the pointer.
*/

#ifdef __xwindows__
static ERROR PTR_GrabX11Pointer(objPointer *Self, struct ptrGrabX11Pointer *Args)
{
   APTR xwin;
   OBJECTPTR surface;

   if (AccessObject(Self->SurfaceID, 5000, &surface) IS ERR_Okay) {
      GetPointer(surface, FID_WindowHandle, &xwin);
      ReleaseObject(surface);

      if (xwin) XGrabPointer(XDisplay, (Window)xwin, True, 0, GrabModeAsync, GrabModeAsync, (Window)xwin, None, CurrentTime);
   }

   return ERR_Okay;
}

static ERROR PTR_UngrabX11Pointer(objPointer *Self, APTR Void)
{
   XUngrabPointer(XDisplay, CurrentTime);
   return ERR_Okay;
}
#endif

/*****************************************************************************

-ACTION-
DataFeed: This action can be used to send fake input to a pointer object.

Fake input can be sent to a pointer object with the DATA_DEVICE_INPUT data type, as if the user was using the mouse.
The data will be interpreted no differently to genuine user input from hardware.

Note that if a button click is used in a device input message, the client must follow up with the equivalent release
flag for that button.

-END-

*****************************************************************************/

static ERROR PTR_DataFeed(objPointer *Self, struct acDataFeed *Args)
{
   if (!Args) return PostError(ERR_NullArgs);

   if (Args->DataType IS DATA_DEVICE_INPUT) {
      struct dcDeviceInput *input;
      LONG i;

      if ((input = (struct dcDeviceInput *)Args->Buffer)) {
         for (i=0; i < ARRAYSIZE(Self->Buttons); i++) {
            if ((Self->Buttons[i].LastClicked) AND (CheckObjectIDExists(Self->Buttons[i].LastClicked) != ERR_Okay)) Self->Buttons[i].LastClicked = 0;
         }

         for (i=sizeof(struct dcDeviceInput); i <= Args->Size; i+=sizeof(struct dcDeviceInput), input++) {
            if ((input->Type < 1) OR (input->Type >= JET_END)) continue;

            input->Flags = glInputType[input->Type].Flags;

            //FMSG("~","Incoming Input: %s, Value: %.2f, Flags: $%.8x, Time: " PF64(), (input->Type < JET_END) ? glInputNames[input->Type] : (STRING)"", input->Value, input->Flags, input->Timestamp);

            if (input->Type IS JET_WHEEL) process_ptr_wheel(Self, input);
            else if (input->Flags & JTYPE_BUTTON) process_ptr_button(Self, input);
            else process_ptr_movement(Self, input);

            //LOGRETURN();
         }
      }
   }
   else {
      return PostError(ERR_WrongType);
   }

   return ERR_Okay;
}

//****************************************************************************

static void process_ptr_button(objPointer *Self, struct dcDeviceInput *Input)
{
   struct InputMsg userinput;
   OBJECTID modal_id, target;
   LONG absx, absy, buttonflag, bi, uiflags;

   ClearMemory(&userinput, sizeof(userinput));
   userinput.Value     = Input->Value;
   userinput.Timestamp = Input->Timestamp;
   userinput.Type      = Input->Type;
   userinput.Flags     = Input->Flags;
   userinput.DeviceID  = Input->DeviceID;

   if (!userinput.Timestamp) userinput.Timestamp = PreciseTime();

   uiflags = 0;

   if ((userinput.Type >= JET_BUTTON_1) AND (userinput.Type <= JET_BUTTON_10)) {
      bi = userinput.Type - JET_BUTTON_1;
      buttonflag = Self->ButtonOrderFlags[bi];
   }
   else {
      // This subroutine is used when the button is not one of the regular 1-10 available button types

      call_userinput("IrregularButton", &userinput, uiflags, Self->OverObjectID, Self->OverObjectID,
         Self->X, Self->Y, Self->OverX, Self->OverY);
      return;
   }

   if (userinput.Value <= 0) {
      // Button released.  Button releases are always reported relative to the object that received the original button press.
      // The surface immediately below the pointer does not receive any information about the release.

      FMSG("process_button:","Button %d released.", bi);

      // Restore the cursor to its default state if cursor release flags have been met

      if ((Self->CursorRelease & buttonflag) AND (Self->CursorOwnerID)) {
         gfxRestoreCursor(PTR_DEFAULT, NULL);
      }

      if (Self->Buttons[bi].LastClicked) {
         if (!GetSurfaceAbs(Self->Buttons[bi].LastClicked, &absx, &absy, 0, 0)) {
            uiflags |= Self->DragSourceID ? JTYPE_DRAG_ITEM : 0;

            if ((ABS(Self->X - Self->LastReleaseX) > Self->ClickSlop) OR
                (ABS(Self->Y - Self->LastReleaseY) > Self->ClickSlop)) {
               uiflags |= JTYPE_DRAGGED;
            }

            if (Self->Buttons[bi].DblClick) {
               if (!(uiflags & JTYPE_DRAGGED)) uiflags |= JTYPE_DBL_CLICK;
            }

            call_userinput("ButtonRelease-LastClicked", &userinput, uiflags, Self->Buttons[bi].LastClicked, Self->OverObjectID,
               Self->X, Self->Y, Self->X - absx, Self->Y - absy); // OverX/Y is reported relative to the click-held surface
         }
         Self->Buttons[bi].LastClicked = 0;
      }

      Self->LastReleaseX = Self->X;
      Self->LastReleaseY = Self->Y;
   }

   // Check for a modal surface.  The modal_id variable is set if a modal surface is active and the pointer is not
   // positioned over that surface (or its children).  The modal_id is therefore zero if the pointer is over the modal
   // surface, or if no modal surface is defined.

   if (glOverTaskID) {
      modal_id = drwGetModalSurface(glOverTaskID);

      if (modal_id) {
         if (modal_id IS Self->OverObjectID) {
            modal_id = 0;
         }
         else {
            // Check if the OverObject is one of the children of modal_id.

            ERROR error;
            error = drwCheckIfChild(modal_id, Self->OverObjectID);
            if ((error IS ERR_True) OR (error IS ERR_LimitedSuccess)) modal_id = 0;
         }
      }
   }
   else modal_id = 0;

   // Button Press Handler

   if (userinput.Value > 0) {
      FMSG("process_button:","Button %d depressed @ " PF64() " Coords: %dx%d", bi, userinput.Timestamp, Self->X, Self->Y);

      //if ((modal_id) AND (modal_id != Self->OverObjectID)) {
      //   LogF("~","Surface %d is modal, button click on %d cancelled.", modal_id, Self->OverObjectID);
      //   DelayMsg(AC_MoveToFront, modal_id, NULL);
      //   DelayMsg(AC_Focus, modal_id, NULL);
      //   LogReturn();
      //}

      //if (!modal_id) {
         // Before performing the click, we first check that there are no objects waiting for click-releases in the
         // designated fields.  If there are, we send them UserClickRelease() actions to retain system integrity.

         if (Self->Buttons[bi].LastClicked) {
            LogErrorMsg("Did not receive a release for button %d on surface #%d.", bi, Self->Buttons[bi].LastClicked);

            call_userinput("ButtonPress-ForceRelease", &userinput, uiflags, Self->Buttons[bi].LastClicked, Self->OverObjectID,
               Self->X, Self->Y, Self->OverX, Self->OverY);
         }

         if (((DOUBLE)(userinput.Timestamp - Self->Buttons[bi].LastClickTime)) / 1000000.0 < Self->DoubleClick) {
            FMSG("process_button:","Double click detected (under %.2fs)", Self->DoubleClick);
            Self->Buttons[bi].DblClick = TRUE;
            uiflags |= JTYPE_DBL_CLICK;
         }
         else Self->Buttons[bi].DblClick = FALSE;

         Self->Buttons[bi].LastClicked   = Self->OverObjectID;
         Self->Buttons[bi].LastClickTime = userinput.Timestamp;

         Self->LastClickX = Self->X;
         Self->LastClickY = Self->Y;

         // If a modal surface is active for the current process, the button press is reported to the modal surface only.

         if ((modal_id) AND (modal_id != Self->OverObjectID)) {
            target = modal_id;
         }
         else target = Self->OverObjectID;

         DelayMsg(AC_Focus, target, NULL);

         call_userinput("ButtonPress", &userinput, uiflags, target, Self->OverObjectID,
            Self->X, Self->Y, Self->OverX, Self->OverY);
      //}

      FUNCTION callback;
      SET_FUNCTION_STDC(callback, &repeat_timer);
      SubscribeTimer(0.02, &callback, &glRepeatTimer); // Use a timer subscription so that repeat button clicks can be supported (the interval indicates the rate of the repeat)
   }

   if ((Self->DragSourceID) AND (!Self->Buttons[bi].LastClicked)) {
      // Drag and drop has been released.  Inform the destination surface of the item's release.

      if (Self->DragSurface) {
         acHideID(Self->DragSurface);
         Self->DragSurface = 0;
      }

      if (!modal_id) {
         acDragDropID(Self->OverObjectID, Self->DragSourceID, Self->DragItem, Self->DragData);
      }

      Self->DragItem = 0;
      Self->DragSourceID = 0;
   }
}

//****************************************************************************

static void process_ptr_wheel(objPointer *Self, struct dcDeviceInput *Input)
{
   struct InputSubscription *subs;
   LONG i;

   if ((glSharedControl->InputMID) AND (!AccessMemory(glSharedControl->InputMID, MEM_READ, 1000, &subs))) {
      struct InputMsg msg;
      msg.Type        = JET_WHEEL;
      msg.Flags       = JTYPE_ANALOG|JTYPE_EXT_MOVEMENT | Input->Flags;
      msg.Mask        = JTYPE_EXT_MOVEMENT;
      msg.Value       = Input->Value;
      msg.Timestamp   = Input->Timestamp;
      msg.DeviceID    = Input->DeviceID;
      msg.RecipientID = Self->OverObjectID;
      msg.OverID      = Self->OverObjectID;
      msg.AbsX        = Self->X;
      msg.AbsY        = Self->Y;
      msg.X           = Self->OverX;
      msg.Y           = Self->OverY;
      send_inputmsg(&msg, subs);
      ReleaseMemory(subs);
   }

   // Convert wheel mouse usage into scroll messages

   DOUBLE scrollrate = 0;
   DOUBLE wheel = Input->Value;
   if (wheel > 0) {
      for (i=1; i <= wheel; i++) scrollrate += Self->WheelSpeed * i;
   }
   else {
      wheel = -wheel;
      for (i=1; i <= wheel; i++) scrollrate -= Self->WheelSpeed * i;
   }

   struct acScroll scroll;
   scroll.XChange = 0;
   scroll.YChange = scrollrate / 100; //(wheel * Self->WheelSpeed) / 100;
   scroll.ZChange = 0;
   ActionMsg(AC_Scroll, Self->OverObjectID, &scroll);
}

//****************************************************************************

static void process_ptr_movement(objPointer *Self, struct dcDeviceInput *Input)
{
   struct InputMsg userinput;
   LONG absx, absy;

   ClearMemory(&userinput, sizeof(userinput));
   userinput.Value     = Input->Value;
   userinput.Timestamp = Input->Timestamp;
   userinput.Type      = Input->Type;
   userinput.Flags     = Input->Flags;
   userinput.DeviceID  = Input->DeviceID;

   if (!userinput.Timestamp) userinput.Timestamp = PreciseTime();

   // All X/Y movement passed through the pointer object must be expressed in absolute coordinates.

   if ((userinput.Type IS JET_DIGITAL_X) OR (userinput.Type IS JET_ANALOG_X)) {
      userinput.Type  = JET_ABS_X;
      userinput.Value += Self->X;
   }
   else if ((userinput.Type IS JET_DIGITAL_Y) OR (userinput.Type IS JET_ANALOG_Y)) {
      userinput.Type = JET_ABS_Y;
      userinput.Value += Self->Y;
   }

   BYTE moved = FALSE;
   LONG current_x = Self->X;
   LONG current_y = Self->Y;
   switch (userinput.Type) {
      case JET_ABS_X: current_x = F2T(userinput.Value); if (current_x != Self->X) moved = TRUE; break;
      case JET_ABS_Y: current_y = F2T(userinput.Value); if (current_y != Self->Y) moved = TRUE; break;
   }

   if (moved IS FALSE) {
      // Check if the surface that we're over has changed due to hide, show or movement of surfaces in the display.

      if (get_over_object(Self)) moved = TRUE;
   }

   if (moved) {
      // Movement handling.  Pointer coordinates are managed here on the basis that they are 'global', i.e. in a hosted
      // environment the coordinates are relative to the top-left of the host display.  Anchoring is enabled by calling
      // LockCursor().  Typically this support is not available on hosted environments because we can't guarantee that
      // the pointer is locked.

      if (Self->AnchorID) {
         if (CheckObjectIDExists(Self->AnchorID) != ERR_Okay) {
            Self->AnchorID = 0;
         }
      }

      #ifdef __snap__

         LONG xchange = current_x - Self->X;
         LONG ychange = current_y - Self->Y;

         if (Self->RestrictID) {
            LONG width, height;

            // Pointer cannot leave the surface that it is restricted to

            if (!GetSurfaceAbs(Self->RestrictID, &absx, &absy, &width, &height)) {
               if (current_x < absx) current_x = absx;
               if (current_y < absy) current_y = absy;
               if (current_x > (absx + width - 1))  current_x = absx + width - 1;
               if (current_y > (absy + height - 1)) current_y = absy + height - 1;
            }
         }
         else {
            // Pointer cannot leave the display boundaries
            if (current_x < 0) current_x = 0;
            if (current_y < 0) current_y = 0;
            if (current_x > glSNAP->VideoMode.XResolution - 1) current_x = glSNAP->VideoMode.XResolution - 1;
            if (current_y > glSNAP->VideoMode.YResolution - 1) current_y = glSNAP->VideoMode.YResolution - 1;
         }

         if (Self->AnchorID) {
            // NOTE: If the pointer is moving diagonally, the anchor surface will receive two separate messages,
            // one for the vertical movement and another for the horizontal.  Therefore if the anchored object
            // is going to consolidate the messages, it will need to take this into account and not rely on AbsX/AbsY
            // representing a movement overview.

            call_userinput("Movement-Anchored", &userinput, JTYPE_ANCHORED, Self->AnchorID, Self->AnchorID, current_x, current_y, xchange, ychange);
         }
         else {
            acMoveToPoint(Self, current_x, current_y, 0, MTF_X|MTF_Y); // NB: This function will update the OverObject field for us
         }

      #else
         LONG xchange = current_x - Self->X;
         LONG ychange = current_y - Self->Y;

         Self->X = current_x;
         Self->Y = current_y;

         if (Self->AnchorID) {
            // When anchoring is enabled we send a movement message signal to the anchored object.  NOTE: In hosted
            // environments we cannot maintain a true anchor since the pointer is out of our control, but we still must
            // perform the necessary notification.

            call_userinput("Movement-Anchored", &userinput, 0, Self->AnchorID, Self->AnchorID, current_x, current_y, xchange, ychange);
         }
         else {
            struct acMoveToPoint moveto = { Self->X, Self->Y, 0, MTF_X|MTF_Y };
            NotifySubscribers(Self, AC_MoveToPoint, &moveto, NULL, ERR_Okay);

            // Recalculate the OverObject due to cursor movement

            get_over_object(Self);
         }

      #endif

      if (Self->AnchorID) {
         // Do nothing as only the anchor surface receives a message (see earlier)
      }
      else if (Self->Buttons[0].LastClicked) {
         // This routine is used when the user is holding down the left mouse button (indicated by LastClicked).  The X/Y
         // coordinates are worked out in relation to the clicked object by climbing the Surface object hierarchy.

         if (Self->DragSurface) {
            LONG sx, sy;

            sx = Self->X + DRAG_XOFFSET;
            sy = Self->Y + DRAG_YOFFSET;
            if (Self->DragParent) {
               LONG absx, absy;
               if (!drwGetSurfaceCoords(Self->DragParent, NULL, NULL, &absx, &absy, NULL, NULL)) {
                  sx -= absx;
                  sy -= absy;
               }
            }

            acMoveToPointID(Self->DragSurface, sx, sy, 0, MTF_X|MTF_Y);
         }

         if (!GetSurfaceAbs(Self->Buttons[0].LastClicked, &absx, &absy, 0, 0)) {
            LONG uiflags = Self->DragSourceID ? JTYPE_DRAG_ITEM : 0;

            // Send the movement message to the last clicked object

            call_userinput("Movement-LastClicked", &userinput, uiflags, Self->Buttons[0].LastClicked, Self->OverObjectID,
               Self->X, Self->Y, Self->X - absx, Self->Y - absy); // OverX/Y reported relative to the click-held surface

            get_over_object(Self);

            // The surface directly under the pointer also needs notification - important for the view to highlight
            // folders during drag and drop for example.

            // JTYPE_SECONDARY indicates to the receiver of the input message that it is not the primary recipient.

            if (Self->Buttons[0].LastClicked != Self->OverObjectID) {
               call_userinput("Movement-LastClicked", &userinput, uiflags|JTYPE_SECONDARY, Self->OverObjectID, Self->OverObjectID,
                  Self->X, Self->Y, Self->OverX, Self->OverY);
            }

         }
         else {
            LogErrorMsg("Failed to get info for surface #%d.", Self->Buttons[0].LastClicked);
            Self->Buttons[0].LastClicked = 0;
         }
      }
      else {
         if (Self->OverObjectID) {
            call_userinput("OverObject", &userinput, 0, Self->OverObjectID, Self->OverObjectID,
               Self->X, Self->Y, Self->OverX, Self->OverY);
         }

         // If the surface that we're over has changed, send a message to the previous surface to tell it that the
         // pointer has moved for one final time.

         if ((Self->LastSurfaceID) AND (Self->LastSurfaceID != Self->OverObjectID)) {
            call_userinput("Movement-PrevSurface", &userinput, 0, Self->LastSurfaceID, Self->OverObjectID,
               Self->X, Self->Y, Self->OverX, Self->OverY);
         }
      }

      Self->LastSurfaceID = Self->OverObjectID; // Reset the LastSurfaceID
   }

   // If a release object has been specified and the cursor is not positioned over it, call the RestoreCursor method.

   if ((Self->CursorReleaseID) AND (Self->CursorReleaseID != Self->OverObjectID)) {
      gfxRestoreCursor(PTR_DEFAULT, NULL);
   }
}

/*****************************************************************************
** Event: ptr_user_login()
*/

static void ptr_user_login(objPointer *Self, APTR Info, LONG Size)
{
   set_pointer_defaults(Self);
}

//****************************************************************************

static ERROR PTR_Free(objPointer *Self, APTR Void)
{
   acHide(Self);

   if (Self->BitmapID) { acFreeID(Self->BitmapID); Self->BitmapID = 0; }

   if (Self->UserLoginHandle) {
      UnsubscribeEvent(Self->UserLoginHandle);
      Self->UserLoginHandle = NULL;
   }
/*
   OBJECTPTR object;
   if ((Self->SurfaceID) AND (!AccessObject(Self->SurfaceID, 5000, &object))) {
      UnsubscribeFeed(object);
      ReleaseObject(object);
   }
*/
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Hide: Hides the pointer from the display.
-END-
*****************************************************************************/

static ERROR PTR_Hide(objPointer *Self, APTR Void)
{
   LogAction(NULL);

   #ifdef __xwindows__
/*
      APTR xwin;
      OBJECTPTR surface;

      if (AccessObject(Self->SurfaceID, 5000, &surface) IS ERR_Okay) {
         GetPointer(surface, FID_WindowHandle, &xwin);
         XDefineCursor(XDisplay, (Window)xwin, GetX11Cursor(Self->CursorID));
         ReleaseObject(surface);
      }
*/
   #elif _WIN32
      winShowCursor(0);
   #endif

   Self->Flags &= ~PF_VISIBLE;
   return ERR_Okay;
}

//****************************************************************************

static ERROR PTR_Init(objPointer *Self, APTR Void)
{
   objBitmap *bitmap;

   if (!modSurface) {
      OBJECTPTR context;

      // The Surface module has to be tracked back to our task because it will open the display module (this causes a
      // resource deadlock as the system can't establish which module to destroy first during expunge).  Also note that
      // the module is terminated through resource tracking, we don't free it during our CMDExpunge() sequence for
      // system integrity reasons.

      context = SetContext(CurrentTask());

         if (LoadModule("surface", MODVERSION_SURFACE, &modSurface, &SurfaceBase) != ERR_Okay) {
            return ERR_InitModule;
         }

      SetContext(context);
   }

   // Find the Surface object that we are associated with.  Note that it is okay if no surface is available at this
   // stage, but the host system must have a mechanism for setting the Surface field at a later stage or else
   // GetOverObject will not function.

   if (!Self->SurfaceID) {
      Self->SurfaceID = GetOwner(Self);
      while ((Self->SurfaceID) AND (GetClassID(Self->SurfaceID) != ID_SURFACE)) {
         Self->SurfaceID = GetOwnerID(Self->SurfaceID);
      }

      if (!Self->SurfaceID) FastFindObject("SystemSurface", NULL, &Self->SurfaceID, 1, NULL);
   }

   // Allocate a custom cursor bitmap

   ERROR error;
   if (!NewLockedObject(ID_BITMAP, NF_INTEGRAL|Self->Head.Flags, &bitmap, &Self->BitmapID)) {
      SetFields(bitmap,
         FID_Name|TSTR,           "CustomCursor",
         FID_Width|TLONG,         MAX_CURSOR_WIDTH,
         FID_Height|TLONG,        MAX_CURSOR_HEIGHT,
         FID_BitsPerPixel|TLONG,  32,
         FID_BytesPerPixel|TLONG, 4,
         FID_Flags|TLONG,         BMF_ALPHA_CHANNEL,
         TAGEND);
      if (!acInit(bitmap)) error = ERR_Okay;
      else {
         acFree(bitmap);
         Self->BitmapID = 0;
         error = ERR_Init;
      }

      ReleaseObject(bitmap);
   }
   else error = ERR_NewObject;

   if (error) return PostError(error);

   if (Self->MaxSpeed < 1) Self->MaxSpeed = 10;
   if (Self->Speed < 1)    Self->Speed    = 150;

#ifdef __snap__
   init_mouse_driver();
#endif

   FUNCTION call;
   SET_FUNCTION_STDC(call, ptr_user_login);
   SubscribeEvent(EVID_USER_STATUS_LOGIN, &call, Self, &Self->UserLoginHandle);

   // 2016-07: Commented out this feed subscription as it had no purpose in MS Windows and unlikely that it's used for
   // other platforms.
/*
   if (Self->SurfaceID) {
      objSurface *surface;
      if (!AccessObject(Self->SurfaceID, 5000, &surface)) {
         SubscribeFeed(surface);
         SetLong(surface, FID_Flags, surface->Flags);
         ReleaseObject(surface);
      }
   }
*/
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
Move: Moves the position of a pointer to a new location.

Calling the Move action allows you to move the position of the pointer to a new location instantly.  This has the effect
of bypassing the normal set of routines for pointer movement (i.e. no UserMovement signals will be sent to applications
to indicate the change).
-END-

*****************************************************************************/

static ERROR PTR_Move(objPointer *Self, struct acMove *Args)
{
   if (!Args) return PostError(ERR_Args);

   if ((!Args->XChange) AND (!Args->YChange)) return ERR_Okay;

   return acMoveToPoint(Self, Self->X + Args->XChange, Self->Y + Args->YChange, 0, MTF_X|MTF_Y);

/*
#ifdef __xwindows__
   OBJECTPTR surface;

   if (AccessObject(Self->SurfaceID, 3000, &surface) IS ERR_Okay) {
      APTR xwin;

      Self->X += Args->XChange;
      Self->Y += Args->YChange;
      if (Self->X < 0) Self->X = 0;
      if (Self->Y < 0) Self->Y = 0;

      GetPointer(surface, FID_WindowHandle, &xwin);
      XWarpPointer(XDisplay, None, (Window)xwin, 0, 0, 0, 0, Self->X, Self->Y);
      Self->HostX = Self->X;
      Self->HostY = Self->Y;
      ReleaseObject(surface);
   }
#elif __snap__
   if (Self->Flags & PF_SOFTWARE) {
      struct acMoveToPoint moveto;

      Self->X += Args->XChange;
      Self->Y += Args->YChange;
      if (Self->X < 0) Self->X = 0;
      if (Self->Y < 0) Self->Y = 0;
      if (Self->X > glNucleus->VideoMode.XResolution - 1) Self->X = glNucleus->VideoMode.XResolution - 1;
      if (Self->Y > glNucleus->VideoMode.YResolution - 1) Self->Y = glNucleus->VideoMode.YResolution - 1;

      moveto.X = Self->X - Self->Cursors[Self->CursorID].HotX;
      moveto.Y = Self->Y - Self->Cursors[Self->CursorID].HotY;
      moveto.Z = 0;
      moveto.Flags  = MTF_X|MTF_Y;
      ActionMsg(AC_MoveToPoint, Self->CursorSurfaceID, &moveto);
   }
#elif _WIN32
   OBJECTPTR surface;

   if (AccessObject(Self->SurfaceID, 3000, &surface) IS ERR_Okay) {
      Self->X += Args->XChange;
      Self->Y += Args->YChange;
      if (Self->X < 0) Self->X = 0;
      if (Self->Y < 0) Self->Y = 0;

      winSetCursorPos(Self->X, Self->Y);
      Self->HostX = Self->X;
      Self->HostY = Self->Y;
      ReleaseObject(surface);
   }
#endif
*/
   return ERR_Okay;
}

/*****************************************************************************

-ACTION-
MoveToPoint: Moves the position of a pointer to a new location.

Calling the MoveToPoint action allows you to move the position of the pointer to a new location instantly.  This has the
effect of bypassing the normal set of routines for pointer movement (i.e. no UserMovement signals will be sent to
applications to indicate the change).

You may subscribe to this action if you would like to listen for changes to the pointer's screen position.
-END-

*****************************************************************************/

static ERROR PTR_MoveToPoint(objPointer *Self, struct acMoveToPoint *Args)
{
   if (!Args) return PostError(ERR_NullArgs)|ERF_Notified;
/*
   if ((!(Args->Flags & MTF_X)) OR ((Args->Flags & MTF_X) AND (Self->X IS Args->X))) {
      if ((!(Args->Flags & MTF_Y)) OR ((Args->Flags & MTF_Y) AND (Self->Y IS Args->Y))) {
         return ERR_Okay|ERF_Notified;
      }
   }
*/
#ifdef __xwindows__
   OBJECTPTR surface;

   if (!AccessObject(Self->SurfaceID, 3000, &surface)) {
      APTR xwin;

      if (!GetPointer(surface, FID_WindowHandle, &xwin)) {
         if (Args->Flags & MTF_X) Self->X = Args->X;
         if (Args->Flags & MTF_Y) Self->Y = Args->Y;
         if (Self->X < 0) Self->X = 0;
         if (Self->Y < 0) Self->Y = 0;

         XWarpPointer(XDisplay, None, (Window)xwin, 0, 0, 0, 0, Self->X, Self->Y);
         Self->HostX = Self->X;
         Self->HostY = Self->Y;
      }
      ReleaseObject(surface);
   }
#elif __snap__
   if (Self->Flags & PF_SOFTWARE) {
      struct acMoveToPoint moveto;
      OBJECTPTR surface;

      if (Args->Flags & MTF_X) Self->X = Args->X;
      if (Args->Flags & MTF_Y) Self->Y = Args->Y;
      if (Self->X < 0) Self->X = 0;
      if (Self->Y < 0) Self->Y = 0;
      if (Self->X > glSNAP->VideoMode.XResolution - 1) Self->X = glSNAP->VideoMode.XResolution - 1;
      if (Self->Y > glSNAP->VideoMode.YResolution - 1) Self->Y = glSNAP->VideoMode.YResolution - 1;

      moveto.X = Self->X - Self->Cursors[Self->CursorID].HotX;
      moveto.Y = Self->Y - Self->Cursors[Self->CursorID].HotY;
      moveto.ZCoord = NULL;
      moveto.Flags  = MTF_X|MTF_Y;
      if (AccessObject(Self->CursorSurfaceID, 3000, &surface) IS ERR_Okay) {
         Action(AC_MoveToPoint, surface, &moveto);
         ReleaseObject(surface);
      }
      else ActionMsg(AC_MoveToPoint, Self->CursorSurfaceID, &moveto);
   }
#elif _WIN32
   OBJECTPTR surface;

   if (!AccessObject(Self->SurfaceID, 3000, &surface)) {
      if (Args->Flags & MTF_X) Self->X = Args->X;
      if (Args->Flags & MTF_Y) Self->Y = Args->Y;
      if (Self->X < 0) Self->X = 0;
      if (Self->Y < 0) Self->Y = 0;

      winSetCursorPos(Self->X, Self->Y);
      Self->HostX = Self->X;
      Self->HostY = Self->Y;
      ReleaseObject(surface);
   }
#endif

   // Determine the surface object that we are currently positioned over.  If it has set a cursor image, switch to it if the pointer is not locked.

   get_over_object(Self);

   // Customised notification (ensures that both X and Y coordinates are reported).

   struct acMoveToPoint moveto = { Self->X, Self->Y, 0, MTF_X|MTF_Y };
   NotifySubscribers(Self, AC_MoveToPoint, &moveto, NULL, ERR_Okay);

   return ERR_Okay|ERF_Notified;
}

//****************************************************************************

static ERROR PTR_NewObject(objPointer *Self, APTR Void)
{
#ifdef __snap__
   StrCopy("AutoDetect", Self->Device, sizeof(Self->Device));
#endif

   Self->CursorID = PTR_DEFAULT;
   Self->ClickSlop = 2;
   set_pointer_defaults(Self);

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Refresh: Refreshes the pointer's cursor status.
-END-
*****************************************************************************/

static ERROR PTR_Refresh(objPointer *Self, APTR Void)
{
   // Calling OverObject will refresh the cursor image from the underlying surface object.  Incidentally, the point of
   // all this is to satisfy the Surface class' need to have the pointer refreshed if a surface's cursor ID is changed.

   get_over_object(Self);
   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Reset: Resets the pointer settings back to the default.
-END-
*****************************************************************************/

static ERROR PTR_Reset(objPointer *Self, APTR Void)
{
   LogAction(NULL);

   Self->Speed        = 150;
   Self->Acceleration = 0.50;
   Self->DoubleClick  = 0.30;
   Self->MaxSpeed     = 100;
   Self->WheelSpeed   = DEFAULT_WHEELSPEED;

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
SaveToObject: Saves the current pointer settings to another object.
-END-
*****************************************************************************/

static ERROR PTR_SaveToObject(objPointer *Self, struct acSaveToObject *Args)
{
   OBJECTPTR config;

   if ((!Args) OR (!Args->DestID)) return PostError(ERR_NullArgs);

   if (!CreateObject(ID_CONFIG, NF_INTEGRAL, &config,
         TAGEND)) {

      char buffer[30];
      StrFormat(buffer, sizeof(buffer), "%f", Self->Speed);
      cfgWriteValue(config, "POINTER", "Speed", buffer);

      StrFormat(buffer, sizeof(buffer), "%f", Self->Acceleration);
      cfgWriteValue(config, "POINTER", "Acceleration", buffer);

      StrFormat(buffer, sizeof(buffer), "%f", Self->DoubleClick);
      cfgWriteValue(config, "POINTER", "DoubleClick", buffer);

      StrFormat(buffer, sizeof(buffer), "%d", Self->MaxSpeed);
      cfgWriteValue(config, "POINTER", "MaxSpeed", buffer);

      StrFormat(buffer, sizeof(buffer), "%f", Self->WheelSpeed);
      cfgWriteValue(config, "POINTER", "WheelSpeed", buffer);

      cfgWriteValue(config, "POINTER", "ButtonOrder", Self->ButtonOrder);

      acSaveToObject(config, Args->DestID, 0);

      acFree(config);
   }

   return ERR_Okay;
}

/*****************************************************************************
-ACTION-
Show: Shows the pointer if it is not already on the display.
-END-
*****************************************************************************/

static ERROR PTR_Show(objPointer *Self, APTR Void)
{
   LogBranch("");

   #ifdef __xwindows__
/*
      APTR xwin;
      OBJECTPTR surface;

      if (AccessObject(Self->SurfaceID, 5000, &surface) IS ERR_Okay) {
         GetPointer(surface, FID_WindowHandle, &xwin);
         XDefineCursor(XDisplay, (Window)xwin, GetX11Cursor(Self->CursorID));
         ReleaseObject(surface);
      }
*/
 #elif _WIN32

      winShowCursor(1);
   #endif

   Self->Flags |= PF_VISIBLE;

   LogReturn();
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Acceleration: The rate of acceleration for relative pointer movement.

This field affects the rate of acceleration as the pointer is moved across the display.  It is recommended that this
field is never set manually, as the user will need to determine the best acceleration level through trial and error in
the user preferences program.

This field is not relevant in a hosted environment.

-FIELD-
Anchor: Can refer to a surface that the pointer has been anchored to.

If the pointer is anchored to a surface through ~SetCursor(), this field will refer to the surface that holds the
anchor.

-FIELD-
Bitmap: Refers to bitmap in which custom cursor images can be drawn.

The pointer graphic can be changed to a custom image if the PTR_CUSTOM #CursorID type is defined and an image is
drawn to the @Bitmap object referenced by this field.

-FIELD-
ButtonOrder: Defines the order in which mouse buttons are interpreted.

This field defines the order of interpretation of the mouse buttons when they are pressed.  This allows a right handed
device to have its buttons remapped to mimic a left-handed device for instance.

The default button order is defined as "123456789AB".  The left, right and middle mouse buttons are defined as 1, 2 and
3 respectively.  The rest of the buttons are assigned by the device, preferably starting from the left of the device and
moving clockwise.

It is legal for buttons to be referenced more than once, for instance a setting of "111" will force the middle and right
mouse buttons to translate to the left mouse button.

Changes to this field will have an immediate impact on the pointing device's behaviour.

*****************************************************************************/

static ERROR GET_ButtonOrder(objPointer *Self, STRING *Value)
{
   *Value = Self->ButtonOrder;
   return ERR_Okay;
}

static ERROR SET_ButtonOrder(objPointer *Self, STRING Value)
{
   WORD i, j;

   LogMsg("%s", Value);

   if (!Value) return ERR_Okay;

   // Assign the buttons

   for (i=0; (Value[i]) AND (i < sizeof(Self->ButtonOrder)-1); i++) Self->ButtonOrder[i] = Value[i];
   Self->ButtonOrder[sizeof(Self->ButtonOrder)-1] = 0;

   // Eliminate any invalid buttons

   for (i=0; Self->ButtonOrder[i]; i++) {
      if (((Self->ButtonOrder[i] >= '1') AND (Self->ButtonOrder[i] <= '9')) OR
          ((Self->ButtonOrder[i] >= 'A') AND (Self->ButtonOrder[i] <= 'Z'))) {
      }
      else Self->ButtonOrder[i] = ' ';
   }

   // Reduce the length of the button list if there are gaps

   j = 0;
   for (i=0; Self->ButtonOrder[i]; i++) {
      if (Self->ButtonOrder[i] != ' ') {
         Self->ButtonOrder[j++] = Self->ButtonOrder[i];
      }
   }

   while (j < sizeof(Self->ButtonOrder)) Self->ButtonOrder[j++] = 0; // Clear any left-over bytes

   // Convert the button indexes into their relevant flags

   for (i=0; i < sizeof(Self->ButtonOrder); i++) {
      if ((Self->ButtonOrder[i] >= '1') AND (Self->ButtonOrder[i] <= '9')) j = Self->ButtonOrder[i] - '1';
      else if ((Self->ButtonOrder[i] >= 'A') AND (Self->ButtonOrder[i] <= 'Z')) j = Self->ButtonOrder[i] - 'A' + 9;
      else j = 0;
      Self->ButtonOrderFlags[i] = 1<<j;
   }

   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ButtonState: Indicates the current button-press state.

You can read this field at any time to get an indication of the buttons that are currently being held by the user.  The
flags returned by this field are JD_LMB, JD_RMB and JD_MMB indicating left, right and middle mouse buttons respectively.

*****************************************************************************/

static ERROR GET_ButtonState(objPointer *Self, LONG *Value)
{
   LONG i;
   LONG state = 0;
   for (i=0; i < ARRAYSIZE(Self->Buttons); i++) {
      if (Self->Buttons[i].LastClicked) state |= 1<<i;
   }

   *Value = state;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
ClickSlop: A leniency value that assists in determining if the user intended to click or drag.

The ClickSlop value defines the allowable pixel distance between two clicks for them to be considered a double-click
(or a drag operation if they exceed the distance).

-FIELD-
CursorID: Sets the user's cursor image, selected from the pre-defined graphics bank.

-FIELD-
CursorOwner: The current owner of the cursor, as defined by SetCursor().

If the pointer is currently owned by an object, this field will refer to that object ID.  Pointer ownership is managed
by the ~SetCursor() function.

-FIELD-
DoubleClick: The maximum interval between two clicks for a double click to be recognised.

A double-click is recognised when two separate clicks occur within a pre-determined time frame.  The length of that
time frame is determined in the DoubleClick field and is measured in seconds.  The recommended interval is 0.3 seconds,
although the user can store his own preference in the pointer configuration file.

-FIELD-
DragItem: The currently dragged item, as defined by StartCursorDrag().

When the pointer is in drag-mode, the custom item number that was defined in the initial call to StartCursorDrag() will
be defined here.  At all other times this field will be set to zero.

-FIELD-
DragSource: The object managing the current drag operation, as defined by StartCursorDrag().

When the pointer is in drag-mode, the object that is managing the source data will be referenced in this field.  At all
other times this field will be set to zero.

Item dragging is managed by the StartCursorDrag() function.

-FIELD-
Flags: Optional flags.
Lookup: PF

-FIELD-
Input: Declares the I/O object to read movement from.

By default a pointer will read its input directly from the mouse port.  However it may be convenient for the pointer to
receive its information from elsewhere, in which case you can set this field to point to a different input object.  The
object that you use <i>must</i> be able to send joyport information over data channels.

-FIELD-
MaxSpeed: Restricts the maximum speed of a pointer's movement.

The maximum speed at which the pointer can move per frame is specified in this field.  This field is provided to help
the user for times where the pointer may be moving to fast (for example if the hardware driver is interpreting the mouse
movement at larger offsets than what is normal).  You can also set the value to 1 if a digital simulation is required.

*****************************************************************************/

static ERROR SET_MaxSpeed(objPointer *Self, LONG Value)
{
   if (Value < 2) Self->MaxSpeed = 2;
   else if (Value > 200) Self->MaxSpeed = 200;
   else Self->MaxSpeed = Value;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
OverObject: Readable field that gives the ID of the object under the pointer.

This field returns a reference to the object directly under the pointer's hot-spot.  NULL can be returned if there is
no surface object under the pointer.

-FIELD-
OverX: The horizontal position of the Pointer with respect to the object underneath the hot-spot.

The OverX field provides other classes with a means of finding out exactly where the pointer is positioned over their
display area.  For example, if a user click occurs on an Image and it is necessary to find out what coordinates where
affected, the OverX and OverY fields can be polled to determine the exact position of the user click.

-FIELD-
OverY: The vertical position of the Pointer with respect to the object underneath the hot-spot.

The OverY field provides other classes with a means of finding out exactly where the pointer is positioned over their
display area.  For example, if a user click occurs on an Image and it is necessary to find out what coordinates where
affected, the OverX and OverY fields can be polled to determine the exact position of the user click.

-FIELD-
OverZ: The position of the Pointer within an object.

This special field applies to 3D interfaces only.  It reflects the position of the pointer within 3-Dimensional
displays, by returning its coordinate along the Z axis.

-FIELD-
Restrict: Refers to a surface when the pointer is restricted.

If the pointer has been restricted to a surface through SetCursor(), this field refers to the ID of that surface.  If
the pointer is not restricted, this field is set to zero.

-FIELD-
Speed: Speed multiplier for Pointer movement.

The speed at which the pointer moves can be adjusted with this field.  To lower the speed, use a value between 0 and
100%.  To increase the speed, use a value between 100 and 1000%.  The Speed of the Pointer is complemented by the
MaxSpeed field, which restricts the maximum amount of pixels that a Pointer can move each time the input device is
polled.

-FIELD-
Surface: The top-most surface that is under the pointer's hot spot.

The surface that is directly under the pointer's hot spot is referenced by this field.  It is automatically updated
whenever the position of the pointer changes or a new surface appears under the pointer.

-FIELD-
WheelSpeed: Defines a multiplier to be applied to the mouse wheel.

This field defines a multiplier that is applied to values coming from the mouse wheel.  A setting of 1.0 leaves the
wheel speed unaltered, while a setting of 2.0 would double the regular speed.

-FIELD-
X: The horizontal position of the pointer within its parent display.

*****************************************************************************/

static ERROR PTR_SET_X(objPointer *Self, LONG Value)
{
   if (Self->Head.Flags & NF_INITIALISED) acMoveToPoint(Self, Value, 0, 0, MTF_X);
   else Self->X = Value;
   return ERR_Okay;
}

/*****************************************************************************

-FIELD-
Y: The vertical position of the pointer within its parent display.
-END-

*****************************************************************************/

static ERROR PTR_SET_Y(objPointer *Self, LONG Value)
{
   if (Self->Head.Flags & NF_INITIALISED) acMoveToPoint(Self, 0, Value, 0, MTF_Y);
   else Self->Y = Value;
   return ERR_Okay;
}

//****************************************************************************

static void set_pointer_defaults(objPointer *Self)
{
   OBJECTPTR config;
   DOUBLE speed, acceleration, doubleclick, maxspeed, wheelspeed;
   CSTRING buttonorder;

   if (CreateObject(ID_CONFIG, 0, &config,
         FID_Path|TSTR, "user:config/pointer.cfg",
         TAGEND) IS ERR_Okay) {

      if (cfgReadFloat(config, "POINTER", "Speed", &speed) != ERR_Okay)               speed         = glDefaultSpeed;
      if (cfgReadFloat(config, "POINTER", "Acceleration", &acceleration) != ERR_Okay) acceleration  = glDefaultAcceleration;
      if (cfgReadFloat(config, "POINTER", "MaxSpeed", &maxspeed) != ERR_Okay)         maxspeed      = 100;
      if (cfgReadFloat(config, "POINTER", "WheelSpeed", &wheelspeed) != ERR_Okay)     wheelspeed    = DEFAULT_WHEELSPEED;
      if (cfgReadFloat(config, "POINTER", "DoubleClick", &doubleclick) != ERR_Okay)   doubleclick   = 0.36;
      if (cfgReadValue(config, "POINTER", "ButtonOrder", &buttonorder) != ERR_Okay)   buttonorder   = "123456789ABCDEF";

      SetFields(Self, FID_Speed|TDOUBLE,        speed,
                      FID_Acceleration|TDOUBLE, acceleration,
                      FID_MaxSpeed|TDOUBLE,     maxspeed,
                      FID_WheelSpeed|TFLOAT,    wheelspeed,
                      FID_DoubleClick|TDOUBLE,  doubleclick,
                      FID_ButtonOrder|TSTRING,  buttonorder,
                      TAGEND);

      acFree(config);
   }
}

//****************************************************************************

static BYTE get_over_object(objPointer *Self)
{
   struct SurfaceControl *ctl;

   if ((Self->SurfaceID) AND (CheckObjectIDExists(Self->SurfaceID) != ERR_Okay)) Self->SurfaceID = 0;

   if (!glSharedControl->SurfacesMID) return FALSE;

   ERROR error = AccessMemory(glSharedControl->SurfacesMID, MEM_READ, 20, &ctl);
   //list = drwAccessList(ARF_READ|ARF_NO_DELAY, &size);

   BYTE changed = FALSE;
   if (!error) {
      // Find the surface that the pointer resides in (usually SystemSurface @ index 0)

      LONG index;
      struct SurfaceList *list = (APTR)ctl + ctl->ArrayIndex;
      if (!Self->SurfaceID) {
         Self->SurfaceID = list[0].SurfaceID;
         index = 0;
      }
      else for (index=0; (index < ctl->Total) AND (list[index].SurfaceID != Self->SurfaceID); index++);

      LONG listend = ctl->Total;

      LONG i = examine_chain(Self, index, ctl, listend);

      OBJECTID li_objectid = list[i].SurfaceID;
      LONG li_left = list[i].Left;
      LONG li_top  = list[i].Top;
      LONG cursor_image = list[i].Cursor; // Preferred cursor ID
      glOverTaskID = list[i].TaskID;   // Task that owns the surface
      ReleaseMemory(ctl);

      if (Self->OverObjectID != li_objectid) {
         struct InputSubscription *subs;

         FMSG("~get_overobject:","OverObject changing from #%d to #%d.  InputMID: %d", Self->OverObjectID, li_objectid, glSharedControl->InputMID);

         changed = TRUE;

         if ((glSharedControl->InputMID) AND (!AccessMemory(glSharedControl->InputMID, MEM_READ, 500, &subs))) {
            struct InputMsg input;
            input.Type       = JET_LEFT_SURFACE;
            input.Flags      = JTYPE_FEEDBACK;
            input.Mask       = JTYPE_FEEDBACK;
            input.Value      = Self->OverObjectID;
            input.Timestamp  = PreciseTime();
            input.DeviceID   = Self->Head.UniqueID;
            input.RecipientID = Self->OverObjectID; // Recipient is the surface we are leaving
            input.OverID     = li_objectid; // New surface (entering)
            input.AbsX       = Self->X;
            input.AbsY       = Self->Y;
            input.X          = Self->X - li_left;
            input.Y          = Self->Y - li_top;
            send_inputmsg(&input, subs);

            input.Type       = JET_ENTERED_SURFACE;
            input.Value      = li_objectid;
            input.RecipientID = li_objectid; // Recipient is the surface we are entering
            send_inputmsg(&input, subs);

            ReleaseMemory(subs);
         }
         Self->OverObjectID = li_objectid;

         LOGRETURN();
      }

      Self->OverX = Self->X - li_left;
      Self->OverY = Self->Y - li_top;

      //drwReleaseList(ARF_READ);

      if (cursor_image) {
         if (cursor_image != Self->CursorID) gfxSetCursor(NULL, NULL, cursor_image, NULL, NULL);
      }
      else if ((Self->CursorID != PTR_DEFAULT) AND (!Self->CursorOwnerID)) {
         // Restore the pointer to the default image if the cursor isn't locked
         gfxSetCursor(NULL, NULL, PTR_DEFAULT, NULL, NULL);
      }
   }
   else FMSG("get_over_object()","Process failed to access the Surface List.");

   return changed;
}

//****************************************************************************

static WORD examine_chain(objPointer *Self, WORD Index, struct SurfaceControl *Ctl, WORD ListEnd)
{
   // NB: The reason why we traverse backwards is because we want to catch the front-most objects first.

   struct SurfaceList *list = (struct SurfaceList *)((APTR)Ctl + Ctl->ArrayIndex);
   OBJECTID objectid = list[Index].SurfaceID;
   LONG x = Self->X;
   LONG y = Self->Y;
   LONG i;
   for (i=ListEnd-1; i >= 0; i--) {
      if ((list[i].ParentID IS objectid) AND (list[i].Flags & RNF_VISIBLE)) {
         if ((x >= list[i].Left) AND (x < list[i].Right) AND (y >= list[i].Top) AND (y < list[i].Bottom)) {
            for (ListEnd=i+1; list[ListEnd].Level > list[i].Level; ListEnd++); // Recalculate the ListEnd (optimisation)
            return examine_chain(Self, i, Ctl, ListEnd);
         }
      }
   }

   return Index;
}

//****************************************************************************
// This timer is used for handling repeat-clicks.

static ERROR repeat_timer(objPointer *Self, LARGE Elapsed, LARGE Unused)
{
   if (!glSharedControl->InputMID) return ERR_Terminate;

   // The subscription is automatically removed if no buttons are held down

   BYTE unsub;
   struct InputSubscription *subs;
   if (!AccessMemory(glSharedControl->InputMID, MEM_READ, 500, &subs)) {
      unsub = TRUE;
      LONG i;
      for (i=0; i < ARRAYSIZE(Self->Buttons); i++) {
         if (Self->Buttons[i].LastClicked) {
            LARGE time = PreciseTime();
            if (Self->Buttons[i].LastClickTime + 300000LL <= time) {
               struct InputMsg input;
               ClearMemory(&input, sizeof(input));

               LONG absx, absy;
               if (Self->Buttons[i].LastClicked IS Self->OverObjectID) {
                  input.X = Self->OverX;
                  input.Y = Self->OverY;
               }
               else if (!GetSurfaceAbs(Self->Buttons[i].LastClicked, &absx, &absy, 0, 0)) {
                  input.X = Self->X - absx;
                  input.Y = Self->Y - absy;
               }
               else {
                  input.X = Self->OverX;
                  input.Y = Self->OverY;
               }

               input.Type       = JET_BUTTON_1 + i;
               input.Mask       = JTYPE_BUTTON|JTYPE_REPEATED;
               input.Flags      = JTYPE_BUTTON|JTYPE_REPEATED;
               input.Value      = 1.0; // Self->Buttons[i].LastValue
               input.Timestamp  = time;
               input.DeviceID   = 0;
               input.RecipientID = Self->Buttons[i].LastClicked;
               input.OverID     = Self->OverObjectID;
               input.AbsX       = Self->X;
               input.AbsY       = Self->Y;

               send_inputmsg(&input, subs);
            }

            unsub = FALSE;
         }
      }
      ReleaseMemory(subs);
   }
   else return PostError(ERR_AccessMemory);

   if (unsub) return ERR_Terminate;
   else return ERR_Okay;
}

//****************************************************************************

#ifdef __snap__
// Mouse driver initialisation

static ERROR init_mouse_driver(void) {
   OBJECTPTR config;
   STRING str;
   WORD port;
   ERROR error;
   LONG i;

   if (CreateObject(ID_CONFIG, NF_INTEGRAL, &config,
         FID_Path|TSTR, "config:hardware/drivers.cfg",
         TAGEND) IS ERR_Okay) {

      if (cfgReadValue(config, "MOUSE", "Device", &str) IS ERR_Okay) StrCopy(str, Self->Device, COPY_ALL);

      if (cfgReadValue(config, "MOUSE", "Driver", &str) IS ERR_Okay) {
         for (i=0; glMouseTypes[i].Name; i++) {
            if (!StrMatch(str, glMouseTypes[i].ShortName)) {
               glDriverIndex = i;
               break;
            }
         }
      }

      acFree(config);
   }

   LogMsg("Using mouse driver \"%s\" and device \"%s\"", glMouseTypes[glDriverIndex].Name, Self->Device);

   if (!StrMatch("AutoDetect", Self->Device)) {
      // If auto-detect is used, open both the PS/2 and USB ports and monitor them in parallel.  On some Linux systems, /dev/mouse
      // could be an authentic device, so use that if it is set.  From kernel 2.6.9, /dev/input/mice is a software device that represents everything.

      // NOTE: OPENING THESE DEVICES TYPICALLY REQUIRES SUPERUSER PRIVILEGES.

      SetResource(RES_PRIVILEGEDUSER, TRUE);

      if (glModernInput) {
         LogMsg("Using modern input mode on /dev/input/mice.");
         if ((glPorts[PORT_CUSTOM].FD = open("/dev/input/mice", O_RDWR)) != -1) {
            glPorts[PORT_CUSTOM].Device = "/dev/input/mice";
            glDriverIndex = MODERN_DRIVER; // Force EXPS/2 when talking to /dev/input/mice
         }
         else glModernInput = FALSE;
      }

      if (glModernInput IS FALSE) {
         // If /dev/mouse is active, it reflects a symbolic link to the user's preferred mouse device.

         LogMsg("Using auto-detect mode (PS/2, USB).");

         if ((glPorts[PORT_CUSTOM].FD = open("/dev/mouse", O_RDWR)) != -1) {
            glPorts[PORT_CUSTOM].Device = "/dev/mouse";
         }
         else {
            glPorts[PORT_PS2].FD = open(glPorts[PORT_PS2].Device, O_RDWR);
            glPorts[PORT_USB].FD = open(glPorts[PORT_USB].Device, O_RDWR);
         }
      }

      SetResource(RES_PRIVILEGEDUSER, FALSE);
   }
   else {
      glModernInput = FALSE;
      if ((glPorts[PORT_CUSTOM].FD = open(Self->Device, O_RDWR)) != -1) {
         glPorts[PORT_CUSTOM].Device = "Custom";
      }
      else {
         LogF("!","Failed to open mouse device \"%s\".", Self->Device);
         return ERR_Failed;
      }
   }

   // Set all open ports to non-blocking and subscribe to the FD's

   for (port=0; port < ARRAYSIZE(glPorts); port++) {
      if (glPorts[port].FD != -1) {
         LogMsg("Setting open port %s, device %s to non-blocking.", glPorts[port].Name, glPorts[port].Device);
         fcntl(glPorts[port].FD, F_SETFL, fcntl(glPorts[port].FD, F_GETFL)|O_NONBLOCK);
         flush_mouse(port);
         RegisterFD(glPorts[port].FD, RFD_READ, &read_mouseport, (APTR)Self->Head.UniqueID);
      }
   }

   // Initialise the mouse now if the driver is already selected

   if (glDriverIndex) {
      LogBranch("Using pre-selected driver \"%s\" on all ports.", glMouseTypes[glDriverIndex].ShortName);

      for (port=0; port < ARRAYSIZE(glPorts); port++) {
         if (glPorts[port].FD != -1) {
            glPorts[port].Driver = glDriverIndex;
            if (init_mouse(port) != ERR_Okay) {
               LogErrorMsg("Mouse protocol failure: %s.", glMouseTypes[glPorts[port].Driver].ShortName);
            }
         }
      }

      LogReturn();
   }

   // Allocate the surface for software based cursor images

   if (!NewLockedObject(ID_SURFACE, NF_INTEGRAL|Self->Head.Flags, &surface, &Self->CursorSurfaceID)) {
      SetFields(surface,
         FID_Name|TSTR,    "Pointer",
         FID_Parent|TLONG, Self->SurfaceID,
         FID_Owner|TLONG,  Self->Head.UniqueID,
         FID_X|TLONG,      -64,
         FID_Y|TLONG,      -64,
         FID_Width|TLONG,  MAX_CURSOR_WIDTH,
         FID_Height|TLONG, MAX_CURSOR_HEIGHT,
         FID_Flags|TLONG,  RNF_CURSOR|RNF_PRECOPY|RNF_COMPOSITE,
         TAGEND);
      if (!acInit(surface)) {
         drwAddCallback(surface, &DrawPointer);
      }
      else { acFree(surface); Self->CursorSurfaceID = 0; }

      ReleaseObject(surface);
   }
   else return PostError(ERR_NewObject);

   return ERR_Okay;
}
#endif

//****************************************************************************

static const struct ActionArray clPointerActions[] = {
   { AC_DataFeed,     PTR_DataFeed },
   { AC_Free,         PTR_Free },
   { AC_Hide,         PTR_Hide },
   { AC_Init,         PTR_Init },
   { AC_Move,         PTR_Move },
   { AC_MoveToPoint,  PTR_MoveToPoint },
   { AC_NewObject,    PTR_NewObject },
   { AC_Refresh,      PTR_Refresh },
   { AC_Reset,        PTR_Reset },
   { AC_SaveToObject, PTR_SaveToObject },
   { AC_Show,         PTR_Show },
   { 0, NULL }
};

static const struct FieldDef clPointerFlags[] = {
   { "Visible",  PF_VISIBLE },
   { NULL, 0 }
};

static const struct FunctionField mthSetCursor[]     = { { "Surface", FD_LONG }, { "Flags", FD_LONG }, { "Cursor", FD_LONG }, { "Name", FD_STRING }, { "Owner", FD_LONG }, { "PreviousCursor", FD_LONG|FD_RESULT }, { NULL, 0 } };
static const struct FunctionField mthRestoreCursor[] = { { "Cursor", FD_LONG }, { "Owner", FD_LONG }, { NULL, 0 } };

static const struct MethodArray clPointerMethods[] = {
   // Private methods
#ifdef _WIN32
   { MT_PtrSetWinCursor,    PTR_SetWinCursor,   "SetWinCursor",   mthSetWinCursor,  sizeof(struct ptrSetWinCursor) },
#endif
#ifdef __xwindows__
   { MT_PtrGrabX11Pointer,   PTR_GrabX11Pointer,   "GrabX11Pointer",   mthGrabX11Pointer, sizeof(struct ptrGrabX11Pointer) },
   { MT_PtrUngrabX11Pointer, PTR_UngrabX11Pointer, "UngrabX11Pointer", NULL, 0 },
#endif
   { 0, NULL, NULL, NULL, 0 }
};

static const struct FieldArray clPointerFields[] = {
   { "Speed",        FDF_DOUBLE|FDF_RW,   0, NULL, NULL },
   { "Acceleration", FDF_DOUBLE|FDF_RW,   0, NULL, NULL },
   { "DoubleClick",  FDF_DOUBLE|FDF_RW,   0, NULL, NULL },
   { "WheelSpeed",   FDF_DOUBLE|FDF_RW,   0, NULL, NULL },
   { "X",            FDF_LONG|FDF_RW,     0, NULL, PTR_SET_X },
   { "Y",            FDF_LONG|FDF_RW,     0, NULL, PTR_SET_Y },
   { "MaxSpeed",     FDF_LONG|FDF_RW,     0, NULL, SET_MaxSpeed },
   { "OverX",        FDF_LONG|FDF_R,      0, NULL, NULL },
   { "OverY",        FDF_LONG|FDF_R,      0, NULL, NULL },
   { "OverZ",        FDF_LONG|FDF_R,      0, NULL, NULL },
   { "Input",        FDF_OBJECTID|FDF_RW, 0, NULL, NULL },
   { "Surface",      FDF_OBJECTID|FDF_RW, ID_SURFACE, NULL, NULL },
   { "Anchor",       FDF_OBJECTID|FDF_R,  ID_SURFACE, NULL, NULL },
   { "CursorID",     FDF_LONG|FDF_LOOKUP|FDF_RI, (MAXINT)&CursorLookup, NULL, NULL },
   { "CursorOwner",  FDF_OBJECTID|FDF_RW,  0, NULL, NULL },
   { "Flags",        FDF_LONGFLAGS|FDF_RI, (MAXINT)&clPointerFlags, NULL, NULL },
   { "Restrict",     FDF_OBJECTID|FDF_R,   ID_SURFACE, NULL, NULL },
   { "HostX",        FDF_LONG|FDF_R|FDF_SYSTEM, 0, NULL, NULL },
   { "HostY",        FDF_LONG|FDF_R|FDF_SYSTEM, 0, NULL, NULL },
   { "Bitmap",       FDF_OBJECTID|FDF_R,   ID_BITMAP, NULL, NULL },
   { "DragSource",   FDF_OBJECTID|FDF_R,   0, NULL, NULL },
   { "DragItem",     FDF_LONG|FDF_R,       0, NULL, NULL },
   { "OverObject",   FDF_OBJECTID|FDF_R,   0, NULL, NULL },
   { "ClickSlop",    FDF_LONG|FDF_RW,      0, NULL, NULL },
   // Virtual Fields
   { "ButtonState",  FDF_LONG|FDF_R,     0, GET_ButtonState, NULL },
   { "ButtonOrder",  FDF_STRING|FDF_RW,  0, GET_ButtonOrder, SET_ButtonOrder },
   END_FIELD
};
