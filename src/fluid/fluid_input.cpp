/*****************************************************************************

The input interface provides support for processing input messages.  The InputMsg structure is passed for each incoming
message that is detected.

   local in = input.subscribe(JTYPE_MOVEMENT, SurfaceID, 0, function(SurfaceID, Message)

   end)

   in.unsubscribe()

To get keyboard feedback:

   local in = input.keyboard(SurfaceID, function(Input, SurfaceID, Flags, Value)

   end)

   in.unsubscribe()

For drag and drop operations, data can be requested from a source as follows:

   input.requestItem(SourceID, Item, DataType, function(Items)

   end)

*****************************************************************************/

#define PRV_SCRIPT
#define PRV_FLUID
#define PRV_FLUID_MODULE
#include <parasol/main.h>
#include <parasol/modules/surface.h>
#include <parasol/modules/display.h>
#include <parasol/modules/fluid.h>
#include <inttypes.h>

extern "C" {
 #include "lauxlib.h"
 #include "lj_obj.h"
}

#include "hashes.h"
#include "defs.h"

static int input_unsubscribe(lua_State *Lua);
static void focus_event(lua_State *, evFocus *, LONG);
static void key_event(struct finput *, evKey *, LONG);

//****************************************************************************
// Any Read accesses to the object will pass through here.

static int input_index(lua_State *Lua)
{
   parasol::Log log;
   auto input = (struct finput *)luaL_checkudata(Lua, 1, "Fluid.input");

   if (input) {
      CSTRING field;
      if (!(field = luaL_checkstring(Lua, 2))) return 0;

      log.trace("input.index(#%d, %s)", input->SurfaceID, field);

      switch (StrHash(field, 0)) {
         case HASH_UNSUBSCRIBE:
            lua_pushvalue(Lua, 1); // Duplicate the interface reference
            lua_pushcclosure(Lua, input_unsubscribe, 1);
            return 1;

         default:
            luaL_error(Lua, "Unknown field reference '%s'", field);
      }
   }
   return 0;
}

//****************************************************************************
// Usage: input = input.keyboard(SurfaceID, Function)

static int input_keyboard(lua_State *Lua)
{
   parasol::Log log("input.keyboard");
   auto prv = (prvFluid *)Lua->Script->Head.ChildPrivate;

   OBJECTID object_id;
   struct object *obj;
   if ((obj = (struct object *)luaL_checkudata(Lua, 1, "Fluid.obj"))) object_id = obj->ObjectID;
   else object_id = lua_tointeger(Lua, 1);

   if ((object_id) and (GetClassID(object_id) != ID_SURFACE)) luaL_argerror(Lua, 1, "Surface object required.");

   LONG function_type = lua_type(Lua, 2);
   if ((function_type IS LUA_TFUNCTION) or (function_type IS LUA_TSTRING));
   else {
      luaL_argerror(Lua, 2, "Function reference required.");
      return 0;
   }

   log.traceBranch("Surface: %d", object_id);

   BYTE sub_keyevent = FALSE;
   if (object_id) {
      if (!prv->FocusEventHandle) { // Monitor the focus state of the target surface with a global function.
         FUNCTION callback;
         SET_FUNCTION_STDC(callback, (APTR)&focus_event);
         SubscribeEvent(EVID_GUI_SURFACE_FOCUS, &callback, Lua, &prv->FocusEventHandle);
      }

      objSurface *surface;
      if (!AccessObject(object_id, 5000, &surface)) {
         if (surface->Flags & RNF_HAS_FOCUS) sub_keyevent = TRUE;
         ReleaseObject(surface);
      }
      else {
         luaL_error(Lua, "Failed to access surface #%d.", object_id);
         return 0;
      }
   }
   else sub_keyevent = TRUE; // Global subscription independent of any surface.

   auto input = (struct finput *)lua_newuserdata(Lua, sizeof(struct finput));
   if (input) {
      luaL_getmetatable(Lua, "Fluid.input");
      lua_setmetatable(Lua, -2);

      APTR event = NULL;
      if (sub_keyevent) {
         FUNCTION callback;
         SET_FUNCTION_STDC(callback, (APTR)&key_event);
         SubscribeEvent(EVID_IO_KEYBOARD_KEYPRESS, &callback, input, &event);
      }

      input->Script    = Lua->Script;
      input->SurfaceID = object_id;
      input->KeyEvent  = event;
      if (function_type IS LUA_TFUNCTION) {
         lua_pushvalue(Lua, 2);
         input->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }
      else {
         lua_getglobal(Lua, (STRING)lua_tostring(Lua, 2));
         input->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }

      lua_pushvalue(Lua, lua_gettop(Lua)); // Take a copy of the Fluid.input object
      input->InputObject = luaL_ref(Lua, LUA_REGISTRYINDEX);
      input->Mode = FIM_KEYBOARD;
      input->Next = prv->InputList;
      prv->InputList = input;
      return 1;
   }
   else {
      luaL_error(Lua, "Failed to create Fluid.input object.");
      return 0;
   }
}

//****************************************************************************
// Usage: req = input.requestItem(Source, Item, DataType, ReceiptFunction)
//
// Request an item of data from an existing object that can provision data.  Used to support drag and drop operations.

static int input_request_item(lua_State *Lua)
{
   auto prv = (prvFluid *)Lua->Script->Head.ChildPrivate;

   if (!lua_isfunction(Lua, 4)) {
      luaL_argerror(Lua, 4, "Function expected.");
      return 0;
   }

   struct datarequest *request;
   if (!AllocMemory(sizeof(struct datarequest), MEM_NO_CLEAR|MEM_DATA, &request, NULL)) {
      struct object *obj;
      OBJECTID source_id;
      ERROR error;

      if ((obj = (struct object *)get_meta(Lua, 1, "Fluid.obj"))) {
         source_id = obj->ObjectID;
      }
      else if (!(source_id = lua_tonumber(Lua, 1))) {
         luaL_argerror(Lua, 1, "Invalid object reference");
         return 0;
      }

      LONG item = lua_tonumber(Lua, 2);

      LONG datatype;
      if (lua_isstring(Lua, 3)) {
         CSTRING dt = lua_tostring(Lua, 3);
         if (!StrMatch("text", dt))              datatype = DATA_TEXT;
         else if (!StrMatch("raw", dt))          datatype = DATA_RAW;
         else if (!StrMatch("device_input", dt)) datatype = DATA_DEVICE_INPUT;
         else if (!StrMatch("xml", dt))          datatype = DATA_XML;
         else if (!StrMatch("audio", dt))        datatype = DATA_AUDIO;
         else if (!StrMatch("record", dt))       datatype = DATA_RECORD;
         else if (!StrMatch("image", dt))        datatype = DATA_IMAGE;
         else if (!StrMatch("request", dt))      datatype = DATA_REQUEST;
         else if (!StrMatch("receipt", dt))      datatype = DATA_RECEIPT;
         else if (!StrMatch("file", dt))         datatype = DATA_FILE;
         else if (!StrMatch("content", dt))      datatype = DATA_CONTENT;
         else if (!StrMatch("input_ready", dt))  datatype = DATA_INPUT_READY;
         else {
            luaL_argerror(Lua, 3, "Unrecognised datatype");
            return 0;
         }
      }
      else if ((datatype = lua_tonumber(Lua, 3)) <= 0) {
         luaL_argerror(Lua, 3, "Datatype invalid");
         return 0;
      }

      request->SourceID = source_id;

      LONG function_type = lua_type(Lua, 4);
      if (function_type IS LUA_TFUNCTION) {
         lua_pushvalue(Lua, 4);
         request->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }
      else if (function_type IS LUA_TSTRING) {
         lua_getglobal(Lua, (STRING)lua_tostring(Lua, 4));
         request->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }

      request->TimeCreated = PreciseTime();
      request->Next = prv->Requests;
      prv->Requests = request;

      struct dcRequest dcr;
      dcr.Item          = item;
      dcr.Preference[0] = datatype;
      dcr.Preference[1] = 0;

      struct acDataFeed dc = {
         .ObjectID = Lua->Script->Head.UniqueID,
         .Datatype = DATA_REQUEST,
         .Buffer   = &dcr,
         .Size     = sizeof(dcr)
      };

      {
         // The source will return a DATA_RECEIPT for the items that we've asked for (see the DataFeed action).
         parasol::Log log("input.request_item");
         log.branch("");
         error = ActionMsg(AC_DataFeed, source_id, &dc);
      }

      if (error) luaL_error(Lua, "Failed to request item %d from source #%d: %s", item, source_id, GetErrorMsg(error));
   }
   else luaL_error(Lua, "Failed to create table.");

   return 0;
}

//****************************************************************************
// Usage: input = input.subscribe(MaskFlags (JTYPE), SurfaceID (Optional), DeviceID (Optional), Function)
//
// This functionality is a wrapper for the gfxSubscribeInput() function.  Due to the fact that individual subscriptions
// cannot be tracked as a resource, we have to subscribe to all surfaces and manipulate the event mask universally.
// This situation could be improved if gfxSubscribeInput() uniquely tracked subscriptions, e.g. with a unique ID
// and gfxUnsubscribeInput() used that ID for releasing each subscription.

static int input_subscribe(lua_State *Lua)
{
   parasol::Log log("input.subscribe");
   auto prv = (prvFluid *)Lua->Script->Head.ChildPrivate;

   LONG mask = lua_tointeger(Lua, 1); // Optional

   OBJECTID object_id;
   struct object *object;
   if ((object = (struct object *)get_meta(Lua, 2, "Fluid.obj"))) object_id = object->ObjectID;
   else object_id = lua_tointeger(Lua, 2);

   LONG device_id = lua_tointeger(Lua, 3); // Optional

   LONG function_type = lua_type(Lua, 4);
   if ((function_type IS LUA_TFUNCTION) or (function_type IS LUA_TSTRING));
   else {
      luaL_argerror(Lua, 4, "Function reference required.");
      return 0;
   }

   ERROR error;
   if (!modDisplay) {
      parasol::SwitchContext context(modFluid);
      if ((error = LoadModule("display", MODVERSION_DISPLAY, &modDisplay, &DisplayBase))) {
         luaL_error(Lua, "Failed to load display module.");
         return 0;
      }
   }

   LONG existing_mask = 0;
   for (auto input=prv->InputList; input; input=input->Next) {
      existing_mask |= input->Mask;
   }

   log.msg("Surface: %d, Mask: $%.8x, Device: %d", object_id, mask, device_id);

   struct finput *input;
   if ((input = (struct finput *)lua_newuserdata(Lua, sizeof(struct finput)))) {
      luaL_getmetatable(Lua, "Fluid.input");
      lua_setmetatable(Lua, -2);

      input->SurfaceID = object_id;

      if (function_type IS LUA_TFUNCTION) {
         lua_pushvalue(Lua, 4);
         input->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }
      else {
         lua_getglobal(Lua, (STRING)lua_tostring(Lua, 1));
         input->Callback = luaL_ref(Lua, LUA_REGISTRYINDEX);
      }

      lua_pushvalue(Lua, lua_gettop(Lua)); // Take a copy of the Fluid.input object
      input->InputObject = luaL_ref(Lua, LUA_REGISTRYINDEX);
      input->KeyEvent    = NULL;
      input->Mask        = mask;
      input->Mode        = FIM_DEVICE;
      input->Next        = prv->InputList;
      prv->InputList = input;

      if ((~existing_mask) & mask) {
         if (existing_mask) gfxUnsubscribeInput(0);
         if ((error = gfxSubscribeInput(0, existing_mask | mask, device_id))) goto failed;
      }

      return 1;
   }

failed:
   luaL_error(Lua, "Failed to initialise input subscription.");
   return 0;
}

//****************************************************************************
// Usage: error = input.unsubscribe(SurfaceID)

static int input_unsubscribe(lua_State *Lua)
{
   auto input = (struct finput *)get_meta(Lua, lua_upvalueindex(1), "Fluid.input");
   if (!input) {
      luaL_argerror(Lua, 1, "Expected input interface.");
      return 0;
   }

   parasol::Log log("input.unsubscribe");
   log.traceBranch("");

   if (input->InputObject) { luaL_unref(Lua, LUA_REGISTRYINDEX, input->InputObject); input->InputObject = 0; }
   if (input->Callback)    { luaL_unref(Lua, LUA_REGISTRYINDEX, input->Callback); input->Callback = 0; }
   if (input->KeyEvent)    { UnsubscribeEvent(input->KeyEvent); input->KeyEvent = NULL; }

   input->Script = NULL;
   input->Mode   = 0;
   return 0;
}

//****************************************************************************
// Input garbage collecter.

static int input_destruct(lua_State *Lua)
{
   parasol::Log log("input.destroy");

   auto input = (struct finput *)lua_touserdata(Lua, 1);
   if (input) {
      log.traceBranch("Surface: %d, CallbackRef: %d, KeyEvent: %p", input->SurfaceID, input->Callback, input->KeyEvent);

      if (input->SurfaceID) {
         // NB: If a keyboard subscription was created, the Display module may not be present/necessary.
         if (modDisplay) gfxUnsubscribeInput(input->SurfaceID);
         input->SurfaceID = 0;
      }

      if (input->InputObject) { luaL_unref(Lua, LUA_REGISTRYINDEX, input->InputObject); input->InputObject = 0; }
      if (input->Callback) { luaL_unref(Lua, LUA_REGISTRYINDEX, input->Callback); input->Callback = 0; }
      if (input->KeyEvent) { UnsubscribeEvent(input->KeyEvent); input->KeyEvent = NULL; }

      if (Lua->Script) { // Remove from the chain.
         auto prv = (prvFluid *)Lua->Script->Head.ChildPrivate;
         if (prv->InputList IS input) prv->InputList = input->Next;
         else {
            auto list = prv->InputList;
            while (list) {
               if (list->Next IS input) {
                  list->Next = input->Next;
                  break;
               }
               list = list->Next;
            }
         }
      }
   }

   return 0;
}

//****************************************************************************
// Key events should only be received when a monitored surface has the focus.

static void key_event(struct finput *Input, evKey *Event, LONG Size)
{
   parasol::Log log("input.key_event");
   objScript *script = Input->Script;
   auto prv = (prvFluid *)script->Head.ChildPrivate;

   if ((!script) or (!prv)) {
      log.trace("Input->Script undefined.");
      return;
   }

   log.traceBranch("Incoming keyboard input");

   LONG depth = GetResource(RES_LOG_DEPTH); // Required because thrown errors cause the debugger to lose its step position
   LONG top = lua_gettop(prv->Lua);
   lua_rawgeti(prv->Lua, LUA_REGISTRYINDEX, Input->Callback); // Get the function reference in Lua and place it on the stack
   lua_rawgeti(prv->Lua, LUA_REGISTRYINDEX, Input->InputObject); // Arg: Input Object
   lua_pushinteger(prv->Lua, Input->SurfaceID);  // Arg: Surface (if applicable)
   lua_pushinteger(prv->Lua, Event->Qualifiers); // Arg: Key Flags
   lua_pushinteger(prv->Lua, Event->Code);       // Arg: Key Value
   lua_pushinteger(prv->Lua, Event->Unicode);    // Arg: Unicode character

   if (lua_pcall(prv->Lua, 5, 0, 0)) {
      process_error(script, "Keyboard event callback");
   }

   lua_settop(prv->Lua, top);
   SetResource(RES_LOG_DEPTH, depth);

   log.traceBranch("Collecting garbage.");
   lua_gc(prv->Lua, LUA_GCCOLLECT, 0);
}

//****************************************************************************
// This is a global function for monitoring the focus of surfaces that we want to filter on for keyboard input.

static void focus_event(lua_State *Lua, evFocus *Event, LONG Size)
{
   parasol::Log log(__FUNCTION__);
   auto prv = (prvFluid *)Lua->Script->Head.ChildPrivate;
   objScript *script = Lua->Script;

   if ((!script) or (!prv)) {
      log.trace("Script undefined.");
      return;
   }

   log.traceBranch("Incoming focus event.");

   for (auto input=prv->InputList; input; input=input->Next) {
      if (input->Mode != FIM_KEYBOARD) continue;
      if (input->KeyEvent) continue;

      for (LONG i=0; i < Event->TotalWithFocus; i++) {
         if (input->SurfaceID IS Event->FocusList[i]) {
            FUNCTION callback;
            SET_FUNCTION_STDC(callback, (APTR)&key_event);
            log.trace("Focus notification received for key events on surface #%d.", input->SurfaceID);
            SubscribeEvent(EVID_IO_KEYBOARD_KEYPRESS, &callback, input, &input->KeyEvent);
            break;
         }
      }
   }

   for (auto input=prv->InputList; input; input=input->Next) {
      if (input->Mode != FIM_KEYBOARD) continue;
      if (!input->KeyEvent) continue;

      for (LONG i=0; i < Event->TotalLostFocus; i++) {
         if (input->SurfaceID IS Event->FocusList[Event->TotalWithFocus+i]) {
            log.trace("Lost focus notification received for key events on surface #%d.", input->SurfaceID);
            UnsubscribeEvent(input->KeyEvent);
            input->KeyEvent = NULL;
            break;
         }
      }
   }
}

//****************************************************************************

static int input_tostring(lua_State *Lua)
{
   auto input = (struct finput *)lua_touserdata(Lua, 1);
   if (input) lua_pushfstring(Lua, "Input handler for surface #%d", input->SurfaceID);
   else lua_pushstring(Lua, "?");
   return 1;
}

//****************************************************************************

void register_input_class(lua_State *Lua)
{
   static const struct luaL_reg inputlib_functions[] = {
      { "subscribe",   input_subscribe },
      { "keyboard",    input_keyboard },
      { "requestItem", input_request_item },
      { NULL, NULL }
   };

   static const struct luaL_reg inputlib_methods[] = {
      { "__gc",       input_destruct },
      { "__tostring", input_tostring },
      { "__index",    input_index },
      { NULL, NULL }
   };

   parasol::Log log(__FUNCTION__);
   log.trace("Registering input interface.");

   luaL_newmetatable(Lua, "Fluid.input");
   lua_pushstring(Lua, "__index");
   lua_pushvalue(Lua, -2);  // pushes the metatable created earlier
   lua_settable(Lua, -3);   // metatable.__index = metatable

   luaL_openlib(Lua, NULL, inputlib_methods, 0);
   luaL_openlib(Lua, "input", inputlib_functions, 0);
}