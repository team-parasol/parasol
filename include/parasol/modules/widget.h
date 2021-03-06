#ifndef MODULES_WIDGET
#define MODULES_WIDGET 1

// Name:      widget.h
// Copyright: Paul Manias 2003-2020
// Generator: idl-c

#ifndef MAIN_H
#include <parasol/main.h>
#endif

#define MODVERSION_WIDGET (1)

#ifndef MODULES_FONT_H
#include <parasol/modules/font.h>
#endif

#ifndef MODULES_PICTURE_H
#include <parasol/modules/picture.h>
#endif

#ifndef MODULES_VECTOR_H
#include <parasol/modules/vector.h>
#endif

#ifndef MODULES_XML_H
#include <parasol/modules/xml.h>
#endif

#ifndef MODULES_DISPLAY_H
#include <parasol/modules/display.h>
#endif

#ifndef MODULES_DOCUMENT_H
#include <parasol/modules/document.h>
#endif

struct WidgetBase {
   ERROR (*_CreateIcon)(CSTRING, CSTRING, CSTRING, LONG, struct rkBitmap **);
};

#ifndef PRV_WIDGET_MODULE
#define widgetCreateIcon(...) (WidgetBase->_CreateIcon)(__VA_ARGS__)
#endif

// Scrollbar flags.

#define SBF_HIDE 0x00000001
#define SBF_CONSTANT 0x00000002
#define SBF_RELATIVE 0x00000004
#define SBF_SLIDER 0x00000008
#define SBF_NO_INTERSECT 0x00000010

// ComboBox flags.

#define CMF_HIDE 0x00000001
#define CMF_DISABLED 0x00000002
#define CMF_EDIT 0x00000004
#define CMF_SHOW_ICONS 0x00000008
#define CMF_LIMIT_TO_LIST 0x00000010
#define CMF_AUTO_COMPLETE 0x00000020
#define CMF_NO_TRANSLATION 0x00000040

// CheckBox flags.

#define CBF_DISABLED 0x00000001
#define CBF_HIDE 0x00000002

// Button flags.

#define BTF_HIDE 0x00000001
#define BTF_DISABLED 0x00000002
#define BTF_NO_GFX 0x00000004
#define BTF_PULSE 0x00000008

// Flags for the Input class.

#define INF_HIDE 0x00000001
#define INF_DISABLED 0x00000002
#define INF_COMMANDLINE 0x00000004
#define INF_SELECT_TEXT 0x00000008
#define INF_SECRET 0x00000010
#define INF_ENTER_TAB 0x00000020

// Flags for the TabFocus class.

#define TF_LOCAL_FOCUS 0x00000001
#define TF_CHILD_FOCUS 0x00000002
#define TF_LIMIT_TO_LIST 0x00000004

// Scroll flags.

#define SCF_AUTO_HIDE 0x00000001
#define SCF_HORIZONTAL 0x00000002
#define SCF_VERTICAL 0x00000004
#define SCF_REVERSE 0x00000008
#define SCF_RELATIVE 0x00000010
#define SCF_SLIDER 0x00000020
#define SCF_FIXED 0x00000040
#define SCF_AUTO_ACTIVATE 0x00000080
#define SCF_MESSAGE 0x00000100
#define SCF_INVISIBLE 0x00000200

// Flags for the Text class.

#define TXF_EDIT 0x00000001
#define TXF_AUTO_CLEAR 0x00000002
#define TXF_OVERWRITE 0x00000004
#define TXF_TAB_ENTRY 0x00000008
#define TXF_WORDWRAP 0x00000010
#define TXF_HISTORY 0x00000020
#define TXF_VARIABLE 0x00000040
#define TXF_FORCE_CAPS 0x00000080
#define TXF_GLOBAL_EDITING 0x00000100
#define TXF_AREA_SELECTED 0x00000200
#define TXF_NO_SYS_KEYS 0x00000400
#define TXF_MULTI_SELECT 0x00000800
#define TXF_SINGLE_SELECT 0x00001000
#define TXF_DISABLED 0x00002000
#define TXF_BACKGROUND 0x00004000
#define TXF_ENTER_TAB 0x00008000
#define TXF_PRESERVE_BKGD 0x00010000
#define TXF_SECRET 0x00020000
#define TXF_PASSWORD 0x00020000
#define TXF_TAB_KEY 0x00040000
#define TXF_AUTO_SELECT 0x00080000
#define TXF_STR_TRANSLATE 0x00100000
#define TXF_STRETCH 0x00200000
#define TXF_COMMANDLINE 0x00000023
#define TXF_SELECT 0x00001800

// Activation flags

#define ACF_RELEASE 0x00000001
#define ACF_SINGLE_CLICK 0x00000002
#define ACF_DOUBLE_CLICK 0x00000004
#define ACF_MOVEMENT 0x00000008
#define ACF_SENSITIVE 0x0000000a

// Image flags

#define IMF_ENLARGE 0x00000001
#define IMF_SHRINK 0x00000002
#define IMF_NO_BLEND 0x00000004
#define IMF_STICKY 0x00000008
#define IMF_NO_FAIL 0x00000010
#define IMF_11_RATIO 0x00000020
#define IMF_FIT 0x00000040
#define IMF_FIXED_SIZE 0x00000080
#define IMF_NO_DRAW 0x00000100
#define IMF_SCALABLE 0x00000200
#define IMF_FIXED 0x00000080
#define IMF_SOLID_BLEND 0x00000004
#define IMF_STRETCH 0x00000003

// Flags for the Menu class.

#define MNF_SHOW_IMAGES 0x00000001
#define MNF_SHOW_KEYS 0x00000002
#define MNF_EXT_COLUMN 0x00000004
#define MNF_POPUP 0x00000008
#define MNF_REVERSE_X 0x00000010
#define MNF_REVERSE_Y 0x00000020
#define MNF_NO_HIDE 0x00000040
#define MNF_SORT 0x00000080
#define MNF_IGNORE_FOCUS 0x00000100
#define MNF_PRESERVE_BKGD 0x00000200
#define MNF_NO_TRANSLATION 0x00000400
#define MNF_CACHE 0x00000800
#define MNF_POINTER_PLACEMENT 0x00001000
#define MNF_SHOW_ICONS 0x00000001
#define MNF_POINTER_XY 0x00001000

// Flags for MenuItems.

#define MIF_DISABLED 0x00000001
#define MIF_BREAK 0x00000002
#define MIF_EXTENSION 0x00000004
#define MIF_CATEGORISE 0x00000008
#define MIF_NO_KEY_RESPONSE 0x00000010
#define MIF_KEY_REPEAT 0x00000020
#define MIF_SORT 0x00000040
#define MIF_OPTION 0x00000080
#define MIF_SELECTED 0x00000100
#define MIF_TOGGLE 0x00000200

#define BHS_OUTSIDE 0
#define BHS_ENTERED 1
#define BHS_INSIDE 2

#define MENUFADE_FADE_IN 1
#define MENUFADE_FADE_OUT 2

// Axis options.  This determines the axis that is signalled when the slider is moved (note that it is feasible for a horizontal scrollbar to signal the Y axis, if this is desired by the client)

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

// Scroll directions.

#define SD_NEGATIVE 1
#define SD_UP 1
#define SD_LEFT 1
#define SD_POSITIVE 2
#define SD_DOWN 2
#define SD_RIGHT 2

// Direction options

#define SO_HORIZONTAL 1
#define SO_VERTICAL 2

// Image class definition

#define VER_IMAGE (1.000000)

typedef struct rkImage {
   OBJECT_HEADER
   struct rkLayout * Layout;
   STRING Hint;                 // Display this hint during user hover.
   LONG   Frame;
   LONG   Flags;                // Optional flags.
   struct RGB8 Mask;
   struct RGB8 Background;      // Background colour to use behind the image.
   LONG   FrameRate;            // The maximum frame-rate for displaying animated images.

#ifdef PRV_IMAGE
   objPicture *Picture;
   objBitmap  *Bitmap;      // Image bitmap.  May originate from the picture, may not
   objBitmap  *RawBitmap;   // Original bitmap image, compressed
   STRING RenderString;
   STRING Path;
   struct SurfaceCoords Surface;
   TIMER  FrameTimer;
   WORD   Opacity;
   char   IconFilter[24];
   char   IconTheme[24];
  
#endif
} objImage;

// Scrollbar class definition

#define VER_SCROLLBAR (1.000000)

typedef struct rkScrollbar {
   OBJECT_HEADER
   DOUBLE   Opacity;            // Opacity setting
   OBJECTID RegionID;           // Surface representing the scrollbar
   OBJECTID SurfaceID;          // The surface target for the scrollbar graphic
   OBJECTID SliderID;           // Surface for the slider
   LONG     Flags;              // Special options
   struct rkScroll * Scroll;    // Reference to the Scroll object
   LONG     Direction;          // Orientation of the scrollbar
   LONG     Breadth;            // Width of the scrollbar
   OBJECTID IntersectID;        // Intersecting scrollbar

#ifdef PRV_SCROLLBAR
   OBJECTID  ScrollID;
   char      Template[120];
   OBJECTPTR Script;
  
#endif
} objScrollbar;

struct ScrollButton {
   OBJECTID ButtonID;
   BYTE     Direction;
   LONG     InputHandle;
};

// Scroll class definition

#define VER_SCROLL (1.000000)

typedef struct rkScroll {
   OBJECT_HEADER
   DOUBLE   Position;     // Page offset.  For internal scroll management issues this field is in floating point, but should only be set by the user as an integer
   OBJECTID ObjectID;     // Object to receive the scroll action
   OBJECTID SliderID;     // Surface that represents the slider within the scrollbar gadget
   OBJECTID IntersectID;  // Intersecting scrollbar, if relevant
   OBJECTID MonitorID;    // Surface to monitor for extra scroll commands (e.g. from the mouse wheel)
   OBJECTID ViewID;       // Surface that forms the view containing the page (optional - for surface based scrolling)
   OBJECTID PageID;       // Surface that forms the page (optional - for surface based scrolling)
   LONG     Unit;         // Amount of units to move when buttons are pressed
   LONG     Flags;        // Optional flags
   LONG     PageSize;     // Size of the page (typically larger than the view)
   LONG     ViewSize;     // Size of the view (contains the page)
   LONG     StartMargin;  // Prevents the slider from moving beyond the bounds of its parent surface.
   LONG     EndMargin;    // Prevents the slider from moving beyond the bounds of its parent surface.
   LONG     SliderSize;   // The width or height of the slider, depending on orientation.
   LONG     Axis;         // The axis that is signalled when the slider is moved.

#ifdef PRV_SCROLL
   LARGE    LastUpdate;        // Timestamp of the most recent UpdateScroll() call
   OBJECTID ScrollbarID;       // Surface that represents the whole of the scrollbar
   OBJECTID IntersectSurface;  // Surface that represents the intersecting scrollbar, if there is one
   LONG     BarSize;           // Indicates the amount of pixel space within the scrollbar area
   LONG     PrevCoord;
   LONG     SliderX, SliderY;
   LONG     ObscuredView;
   LONG     SurfaceWidth, SurfaceHeight;
   LONG     Offset;
   LONG     InputHandle;
   char     Field[32];
   UBYTE    RecursionBlock;
   BYTE     PostIntersect;     // Set to TRUE if the Intersect field was set post-initialisation (we are not expected to adjust our scrollbar size)
   struct ScrollButton Buttons[8];
   FUNCTION Feedback;
  
#endif
} objScroll;

// Scroll methods

#define MT_ScUpdateScroll -1
#define MT_ScAddScrollButton -2

struct scUpdateScroll { LONG PageSize; LONG ViewSize; LONG Position; LONG Unit;  };
struct scAddScrollButton { OBJECTID SurfaceID; LONG Direction;  };

INLINE ERROR scUpdateScroll(APTR Ob, LONG PageSize, LONG ViewSize, LONG Position, LONG Unit) {
   struct scUpdateScroll args = { PageSize, ViewSize, Position, Unit };
   return(Action(MT_ScUpdateScroll, (OBJECTPTR)Ob, &args));
}

INLINE ERROR scAddScrollButton(APTR Ob, OBJECTID SurfaceID, LONG Direction) {
   struct scAddScrollButton args = { SurfaceID, Direction };
   return(Action(MT_ScAddScrollButton, (OBJECTPTR)Ob, &args));
}


// Text class definition

#define VER_TEXT (1.000000)

typedef struct rkText {
   OBJECT_HEADER
   struct rkLayout * Layout;    // Layout manager
   struct rkFont * Font;        // Font to use for the text
   OBJECTID VScrollID;          // Reference to a vertical scroll bar
   OBJECTID HScrollID;          // References to a horizontal scroll bar
   OBJECTID TabFocusID;         // If the tab key is pressed, a focus can be sent to this object
   OBJECTID FocusID;            // Refers to the object that will be monitored for user focussing.
   LONG     CursorColumn;       // Cursor column
   LONG     CursorRow;          // Cursor row / Line selection
   LONG     Flags;              // Special flags
   LONG     AmtLines;           // Amount of strings currently in the array
   LONG     SelectRow;          // Row that the current text selection starts from
   LONG     SelectColumn;       // Column that the current text selection starts from
   LONG     Frame;              // The frame for which the text will be visible.
   LONG     HistorySize;        // Size of the history buffer (measured in array elements)
   LONG     LineLimit;          // Limit the number of lines to the value in this field
   LONG     CharLimit;          // Limit the number of chars allowed in each line of the text object
   struct RGB8 Highlight;       // Default colour for text highlighting
   struct RGB8 Background;      // Colour for text background
   struct RGB8 CursorColour;    // The colour of the cursor

#ifdef PRV_TEXT
   struct TextLine *Array;      // Pointer to an array of lines
   struct TextHistory *History; // History of text entries
   struct rkXML  *XML;
   struct rkFile *FileStream;
   APTR   prvKeyEvent;
   FUNCTION ValidateInput;
   FUNCTION Activated;
   STRING StringBuffer;
   STRING Location;             // Optional file to load on Init()
   DOUBLE RelSize;              // Relative point size (a percentage relative to the height of the container)
   LONG   CurrentFrame;
   LONG   MaxLines;          // Total number of lines available in the array
   LONG   XPosition;         // Horizontal scrolling offset
   LONG   YPosition;         // Vertical scrolling offset
   LONG   CursorChar;
   LONG   CursorSavePos;
   LONG   InputHandle;
   TIMER  CursorTimer;
   WORD   TotalFrames;
   WORD   ClickHeld;
   WORD   HistoryPos;
   WORD   CursorWidth;
   UWORD  CursorFlash;     // If set to 1, cursor is invisible.  This field is managed by the Timer action
   WORD   CursorEndColumn;
   UBYTE  PointerLocked;
   UBYTE  State;
   BYTE   Tag;
   BYTE   NoUpdate;
   BYTE   NoCursor;
   BYTE   Active:1;
  
#endif
} objText;

// Text methods

#define MT_TxtAddLine -1
#define MT_TxtDeleteLine -2
#define MT_TxtGetLine -3
#define MT_TxtReplaceLine -4
#define MT_TxtSelectArea -5
#define MT_TxtSetFont -6

struct txtAddLine { LONG Line; CSTRING String; LONG Length;  };
struct txtDeleteLine { LONG Line;  };
struct txtGetLine { LONG Line; STRING Buffer; LONG Length;  };
struct txtReplaceLine { LONG Line; CSTRING String; LONG Length;  };
struct txtSelectArea { LONG Row; LONG Column; LONG EndRow; LONG EndColumn;  };
struct txtSetFont { CSTRING Face;  };

INLINE ERROR txtAddLine(APTR Ob, LONG Line, CSTRING String, LONG Length) {
   struct txtAddLine args = { Line, String, Length };
   return(Action(MT_TxtAddLine, (OBJECTPTR)Ob, &args));
}

INLINE ERROR txtDeleteLine(APTR Ob, LONG Line) {
   struct txtDeleteLine args = { Line };
   return(Action(MT_TxtDeleteLine, (OBJECTPTR)Ob, &args));
}

INLINE ERROR txtGetLine(APTR Ob, LONG Line, STRING Buffer, LONG Length) {
   struct txtGetLine args = { Line, Buffer, Length };
   return(Action(MT_TxtGetLine, (OBJECTPTR)Ob, &args));
}

INLINE ERROR txtReplaceLine(APTR Ob, LONG Line, CSTRING String, LONG Length) {
   struct txtReplaceLine args = { Line, String, Length };
   return(Action(MT_TxtReplaceLine, (OBJECTPTR)Ob, &args));
}

INLINE ERROR txtSelectArea(APTR Ob, LONG Row, LONG Column, LONG EndRow, LONG EndColumn) {
   struct txtSelectArea args = { Row, Column, EndRow, EndColumn };
   return(Action(MT_TxtSelectArea, (OBJECTPTR)Ob, &args));
}

INLINE ERROR txtSetFont(APTR Ob, CSTRING Face) {
   struct txtSetFont args = { Face };
   return(Action(MT_TxtSetFont, (OBJECTPTR)Ob, &args));
}


// TabFocus class definition

#define VER_TABFOCUS (1.000000)

typedef struct rkTabFocus {
   OBJECT_HEADER
   OBJECTID SurfaceID;  // The surface to monitor for the primary focus when managing the tab key
   LONG     Total;      // Total number of objects on the tab list
   LONG     Flags;      // Optional flags

#ifdef PRV_TABFOCUS
   struct {
      OBJECTID ObjectID;
      OBJECTID SurfaceID;
   } TabList[50];         // List of objects to be managed by the tab key
   APTR prvKeyEvent;
   WORD Index;            // Current focus index
   OBJECTID CurrentFocus;
   BYTE Reverse:1;
  
#endif
} objTabFocus;

// TabFocus methods

#define MT_TabAddObject -1
#define MT_TabInsertObject -2
#define MT_TabRemoveObject -3
#define MT_TabSetObject -4

struct tabAddObject { OBJECTID ObjectID;  };
struct tabInsertObject { LONG Index; OBJECTID ObjectID;  };
struct tabRemoveObject { OBJECTID ObjectID;  };
struct tabSetObject { LONG Index; OBJECTID ObjectID;  };

INLINE ERROR tabAddObject(APTR Ob, OBJECTID ObjectID) {
   struct tabAddObject args = { ObjectID };
   return(Action(MT_TabAddObject, (OBJECTPTR)Ob, &args));
}

INLINE ERROR tabInsertObject(APTR Ob, LONG Index, OBJECTID ObjectID) {
   struct tabInsertObject args = { Index, ObjectID };
   return(Action(MT_TabInsertObject, (OBJECTPTR)Ob, &args));
}

INLINE ERROR tabRemoveObject(APTR Ob, OBJECTID ObjectID) {
   struct tabRemoveObject args = { ObjectID };
   return(Action(MT_TabRemoveObject, (OBJECTPTR)Ob, &args));
}

INLINE ERROR tabSetObject(APTR Ob, LONG Index, OBJECTID ObjectID) {
   struct tabSetObject args = { Index, ObjectID };
   return(Action(MT_TabSetObject, (OBJECTPTR)Ob, &args));
}


// CheckBox class definition

#define VER_CHECKBOX (1.000000)

typedef struct rkCheckBox {
   OBJECT_HEADER
   OBJECTID RegionID;  // Surface region created by the checkbox object
   OBJECTID SurfaceID; // The surface target for the checkbox widget
   LONG     Flags;     // Special options
   LONG     LabelWidth; // The fixed pixel width allocated for drawing the label string.
   LONG     Value;     // Either TRUE or FALSE
   LONG     Align;     // Alignment flags

#ifdef PRV_CHECKBOX
   FUNCTION Feedback;
   APTR   prvKeyEvent;
   BYTE   Active;
   char   Label[48];       // Label to display alongside the checkbox
  
#endif
} objCheckBox;

// Button class definition

#define VER_BUTTON (1.000000)

typedef struct rkButton {
   OBJECT_HEADER
   STRING   Hint;      // Applies a hint to a button, which can be displayed as a tool-tip.
   STRING   Icon;      // Name of an icon to display in the button.
   OBJECTID RegionID;  // Surface region created by the button object
   OBJECTID SurfaceID; // The surface target for the button graphic
   LONG     Flags;     // Special options
   LONG     Clicked;   // TRUE if the button has been clicked and reverts to FALSE when the user releases the button.
   LONG     HoverState; // User hover state indicator

#ifdef PRV_BUTTON
   FUNCTION Feedback;
   char String[40];            // String to display inside the button
   struct rkDocument *Document;
   APTR   prvKeyEvent;      // For subscribing to keyboard events
   STRING Onclick;          // Available in Document mode only, references the function to be called when clicked
   UBYTE  Active;           // For recursion management.
   LONG   ClickX, ClickY;   // X/Y coordinate most recently clicked
   LONG   InputHandle;
  
#endif
} objButton;

// Resize class definition

#define VER_RESIZE (1.000000)

typedef struct rkResize {
   OBJECT_HEADER
   struct rkLayout * Layout;    // Layout manager
   OBJECTID ObjectID;           // Object that is to be resized
   LONG     Button;             // Determines what button is used for resizing
   LONG     Direction;          // Direction flags (horizontal/vertical)
   LONG     Border;             // Border flags can be used to monitor up to 8 separate areas at once
   LONG     BorderSize;         // Determines the size of the border edge

#ifdef PRV_RESIZE
   LONG  OriginalWidth, OriginalHeight;
   LONG  OriginalX, OriginalY;
   LONG  OriginalAbsX, OriginalAbsY;
   LONG  prvAnchorX, prvAnchorY;
   LONG  InputHandle;
   WORD  CursorSet;
   WORD  State;
   BYTE  prvAnchored;
  
#endif
} objResize;

// Input class definition

#define VER_INPUT (1.000000)

typedef struct rkInput {
   OBJECT_HEADER
   struct rkText * TextInput;    // Text control object - for the input area
   OBJECTID RegionID;            // Surface region created by the input object
   OBJECTID SurfaceID;           // The surface target for the input graphic
   LONG     Flags;               // Special options
   LONG     LabelWidth;          // Width of the input box label area
   LONG     InputWidth;          // Width of the input area

#ifdef PRV_INPUT
   char    prvLabel[48];
   char    prvPostLabel[48];
   ULONG    prvLastStringHash;
   FUNCTION prvFeedback;
   UBYTE    prvActive:1;
   UBYTE    prvStringReset:1;
  
#endif
} objInput;

// ComboBox class definition

#define VER_COMBOBOX (1.000000)

typedef struct rkComboBox {
   OBJECT_HEADER
   struct rkText * TextInput;    // Text control object - for the combobox area
   struct rkMenu * Menu;         // Menu control object
   OBJECTID RegionID;            // Surface region created by the combobox object
   OBJECTID SurfaceID;           // The surface target for the combobox graphic
   LONG     Flags;               // Special options
   LONG     LabelWidth;          // Width of the combobox label area

#ifdef PRV_COMBOBOX
   UBYTE Active:1;
   char  Label[48];
   FUNCTION Feedback;
  
#endif
} objComboBox;

// Menu class definition

#define VER_MENU (1.000000)

typedef struct rkMenu {
   OBJECT_HEADER
   DOUBLE   HoverDelay;          // Optional hover feature for the Monitor field.
   DOUBLE   AutoExpand;          // Auto-expand delay, measured in seconds.
   DOUBLE   FadeDelay;           // The length of time allocated to special effects like fading.  Value in seconds
   struct rkMenuItem * Items;    // Items listed in the menu
   struct rkFont * Font;         // The font being used in the menu
   STRING   Style;               // Name of a menu graphics style to apply.
   OBJECTID TargetID;            // Target for the surface (e.g. desktop)
   OBJECTID ParentID;            // Parent menu if this is a child of a master menu
   OBJECTID RelativeID;          // Relative surface that should be used for coordinate origins
   OBJECTID KeyMonitorID;        // Surface to monitor for key presses
   OBJECTID MenuSurfaceID;       // Surface for this menu
   OBJECTID MonitorID;           // Surface to monitor for mouse clicks
   LONG     Flags;               // Optional flags
   LONG     VSpacing;            // Amount of spacing between each menu item
   LONG     BreakHeight;         // The amount of height to give to menu-break graphics
   LONG     FixedWidth;          // Predetermined fixed-width, often used for things like combo-boxes
   LONG     LeftMargin;          // Left hand margin inside the menu
   LONG     RightMargin;         // Right hand margin inside the menu
   LONG     TopMargin;           // Top margin inside the menu
   LONG     BottomMargin;        // Bottom margin inside the menu
   LONG     HighlightLM;         // Highlight rectangle left margin
   LONG     HighlightRM;         // Highlight rectangle right margin
   LONG     ItemHeight;          // Minimum allowable height for text based menu items
   LONG     ImageSize;           // Size of the image column.  Icons will be generated to fit this size
   LONG     LineLimit;           // Maximum number of displayed lines before the scrollbar kicks in
   LONG     BorderSize;          // Size of the border at the menu edges
   LONG     SelectionIndex;      // The index of the most recent item to be executed.  If zero, no item has been executed.
   struct RGB8 FontColour;
   struct RGB8 FontHighlight;
   struct RGB8 Highlight;
   struct RGB8 HighlightBorder;
   LONG     ImageGap;            // Gap between the image column and text
   LONG     KeyGap;              // Gap between the text and key columns
   LONG     ExtensionGap;        // Gap between the extension column and the text or key column
   LONG     TextWidth;           // Width of the widest text string
   LONG     KeyWidth;            // Width of the widest key string

#ifdef PRV_MENU
 PRV_MENU_FIELDS 
#endif
} objMenu;

// Menu methods

#define MT_MnSwitch -1
#define MT_MnSelectItem -2
#define MT_MnGetItem -3

struct mnSwitch { LONG TimeLapse;  };
struct mnSelectItem { LONG ID; LONG State;  };
struct mnGetItem { LONG ID; struct rkMenuItem * Item;  };

INLINE ERROR mnSwitch(APTR Ob, LONG TimeLapse) {
   struct mnSwitch args = { TimeLapse };
   return(Action(MT_MnSwitch, (OBJECTPTR)Ob, &args));
}

INLINE ERROR mnSelectItem(APTR Ob, LONG ID, LONG State) {
   struct mnSelectItem args = { ID, State };
   return(Action(MT_MnSelectItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR mnGetItem(APTR Ob, LONG ID, struct rkMenuItem ** Item) {
   struct mnGetItem args = { ID, 0 };
   ERROR error = Action(MT_MnGetItem, (OBJECTPTR)Ob, &args);
   if (Item) *Item = args.Item;
   return(error);
}


// MenuItem class definition

#define VER_MENUITEM (1.000000)

typedef struct rkMenuItem {
   OBJECT_HEADER
   struct rkMenuItem * Prev;    // Previous menu item in chain
   struct rkMenuItem * Next;    // Next menu item in chain
   struct rkBitmap * Bitmap;    // Icon to display in the menu item
   struct rkMenu * SubMenu;     // If the item refers to a sub-menu, it will be pointed to here
   STRING Path;                 // Location of a menu definition file if this item is a menu extension
   STRING Name;                 // Internal name of the menu item (user specific, helpful for searching purposes and unique identification)
   STRING Text;                 // Text to print in the menu item
   LONG   Flags;                // Optional flags
   LONG   Key;                  // Key value for this item
   LONG   Qualifiers;           // Qualifier key(s) for this item
   LONG   Index;                // Item index.  Follows the order of the items as they are listed in the menu
   LONG   Group;                // Grouping, relevant for checkmarking menu items
   LONG   ID;                   // User-defined unique identifier ('id' attribute)
   LONG   Height;               // Height of the item
   struct RGB8 Colour;          // Font colour
   struct RGB8 Background;      // Background colour

#ifdef PRV_MENUITEM
 PRV_MENUITEM_FIELDS 
#endif
} objMenuItem;

// View flags.

#define VWF_MULTI_SELECT 0x00000001
#define VWF_NO_ICONS 0x00000002
#define VWF_SENSITIVE 0x00000004
#define VWF_NO_SORTING 0x00000008
#define VWF_NO_SELECT 0x00000010
#define VWF_WIDTH_RESET 0x00000020
#define VWF_NO_SELECT_JMP 0x00000040
#define VWF_NOTIFY_ON_CLEAR 0x00000080
#define VWF_AUTO_DESELECT 0x00000100
#define VWF_DRAG_DROP 0x00000200
#define VWF_USER_DRAG 0x00000400

// List options

#define VIEW_LIST 0
#define VIEW_ICON 1
#define VIEW_TREE 2
#define VIEW_COLUMN 3
#define VIEW_LONG_LIST 4
#define VIEW_GROUP_TREE 5
#define VIEW_DOCUMENT 6
#define VIEW_COLUMN_TREE 7

// Graphics options (normally defined by the template)

#define VGF_OUTLINE_TITLE 0x00000001
#define VGF_STRIPES 0x00000002
#define VGF_LINE_BREAKS 0x00000004
#define VGF_GROUP_SHADOW 0x00000008
#define VGF_ALT_GROUP 0x00000010
#define VGF_BRANCHES 0x00000020
#define VGF_HAIRLINES 0x00000040
#define VGF_NO_BORDER 0x00000080
#define VGF_DRAW_TABLE 0x00000100

// Flags for SelectCallback

#define SLF_SELECTED 0x00000001
#define SLF_ACTIVE 0x00000002
#define SLF_INVERTED 0x00000004
#define SLF_MOVED 0x00000008
#define SLF_MULTIPLE 0x00000010
#define SLF_CLICK 0x00000020
#define SLF_KEYPRESS 0x00000040
#define SLF_MANUAL 0x00000080

// View class definition

#define VER_VIEW (1.000000)

typedef struct rkView {
   OBJECT_HEADER
   struct rkLayout * Layout;        // Layout manager
   struct rkXML * XML;              // The XML object contains all view information
   struct rkFont * Font;            // Font object
   STRING    ColumnString;          // String defining the column headers
   OBJECTPTR ContextMenu;           // Reference to a context menu when the user right-clicks on an item
   OBJECTPTR VScroll;               // Reference to a vertical scroll bar
   OBJECTPTR HScroll;               // References to a horizontal scroll bar
   struct rkDocument * Document;    // Reference to a document object for use in DOCUMENT mode
   STRING    GroupFace;             // The font to use for group header text
   STRING    ItemNames;             // Valid item names must be listed here using a wildcard list
   STRING    TextAttrib;            // Identifies the XML attribute that will be used for drawing item text
   OBJECTID  FocusID;               // The surface to monitor for the focus
   OBJECTID  DragSourceID;          // Source object to use for drag and drop operations
   LONG      Flags;                 // Special flags
   LONG      Style;                 // Style of view
   LONG      HSpacing;              // Horizontal spacing between each item when arranged across the page
   LONG      VSpacing;              // Vertical spacing between each item when arranged down the page
   LONG      SelectedTag;           // The most recently selected tag [monitorable]
   LONG      ActiveTag;             // The most recently clicked or activated tag (not necessarily selected)
   LONG      HighlightTag;          // Current user-highlighted item
   LONG      MaxItemWidth;          // Maximum item width, in pixels (applies to certain view styles)
   LONG      ButtonThickness;       // Thickness of column button borders when in COLUMN mode
   LONG      IconSize;              // Default icon size
   LONG      GfxFlags;              // Graphics flags (VGF)
   LONG      DragItemCount;         // The number of items being dragged
   LONG      TotalItems;            // Total number of items listed in the view
   LONG      GroupHeight;           // Defines a fixed height for group headers
   struct RGB8 ButtonBackground;    // Colour of column button background
   struct RGB8 ButtonHighlight;     // Colour of column button highlight
   struct RGB8 ButtonShadow;        // Colour of column button shadow
   struct RGB8 ColHighlight;        // Highlight colour on mouse over.
   struct RGB8 ColSelect;           // Colour to use for selected items
   struct RGB8 ColItem;             // Default colour to apply to new items
   struct RGB8 ColHairline;         // Colour of column hairlines
   struct RGB8 ColSelectHairline;   // Colour of hairlines when an item is selected
   struct RGB8 ColBackground;       // Base-colour of group-tree background
   struct RGB8 ColTitleFont;        // Colour of group title font
   struct RGB8 ColSelectFont;       // Font colour for selected items
   struct RGB8 ColBkgdHighlight;    // Background highlight colour on mouse over
   struct RGB8 ColBorder;           // Colour for the surrounding border, if any
   struct RGB8 ColButtonFont;       // Colour of column button font
   struct RGB8 ColAltBackground;    // Alternative line colour for the background
   struct RGB8 ColGroupShade;       // Shading to apply to the end of the item view when in GROUPTREE mode
   struct RGB8 ColBranch;           // Colour of tree branches

#ifdef PRV_VIEW
   LONG *DragItems;              // Array of tag ID's for items being dragged
   objBitmap *GroupBitmap;       // Group header bitmap
   objBitmap *SelectBitmap;      // Group selection bitmap
   objBitmap *ExpandBitmap;      // Bitmap to use for opening tree items
   objBitmap *CollapseBitmap;    // Bitmap to use for closing tree items
   objFont   *GroupFont;         // Font allocated for group headers
   struct view_col *Columns;       // Column information
   struct view_col *ColumnResize;  // Set if a column is being resized
   objScrollbar *VScrollbar, *HScrollbar;
   objVectorGradient *Shadow;
   FUNCTION ExpandCallback;
   FUNCTION SelectCallback;
   FUNCTION CellClick;
   LONG     *SelectedTags;
   STRING   BkgdXML;
   STRING   GroupHeaderXML;
   STRING   GroupSelectXML;
   STRING   Selections;
   STRING   NodeStrings;       // String buffer for all XML->Tag->Node->String pointers
   APTR     prvKeyEvent;
   OBJECTID DragSurface;
   OBJECTID GroupSurfaceID;
   LONG   InputHandle;
   LONG   NSIndex;           // Node string index
   LONG   NSSize;            // Size of the NodeStrings buffer
   LONG   XPos, YPos;        // Scrolling offset
   LONG   ClickX, ClickY;
   LONG   ClickIndex;
   LONG   PageWidth, PageHeight; // Width/Height of the scrollable page area
   LONG   SelectionIndex;    // Selection index, used to assist initialisation process only
   WORD   LineHeight;        // Pixel height of each line
   WORD   IconWidth;         // Maximum pixel width/height of icons
   WORD   ColumnHeight;      // Height of column buttons when in column mode
   WORD   TreeIndex;
   WORD   HBarHeight;
   BYTE   DateFormat[28];    // Preferred date format
   char   Selection[120];    // Currently selected item
   char   IconFilter[32];    // Preferred icon filter.
   char   IconTheme[32];     // Preferred icon theme.  If not defined, the default is used.
   BYTE   Sort[4];           // Sort indexes to each column (4 max) from most to least important.  A negative index means that the sort is descending
   char   VarDefault[16];
   BYTE   PointerLocked;     // Set to a cursor value if the pointer is locked
   UWORD  HBarVisible:1;
   UWORD  VBarVisible:1;
   UWORD  ClickHeld:1;
   UWORD  SelectHairline:1;  // Set to TRUE if SelectHairline has been defined
   UWORD  Deselect:1;
   UWORD  ActiveDrag:1;      // Set to TRUE when a LMB has occurred and it is possible to initiate a drag and drop operation
   UWORD  SelectingItems:1;  // Set to TRUE if the LMB has been clicked on an item to select/deselect it
   UWORD  RedrawDue:1;       // Set to TRUE to indicate that a Draw action message has been placed on the message queue for delayed execution
  
#endif
} objView;

// View methods

#define MT_viewSelectAll -3
#define MT_viewInvertSelection -4
#define MT_viewSortColumn -5
#define MT_viewSortColumnIndex -6
#define MT_viewSelectItem -7
#define MT_viewSetItem -8
#define MT_viewRemoveItem -9
#define MT_viewRemoveTag -10
#define MT_viewSelectNone -11
#define MT_viewRaiseItem -12
#define MT_viewLowerItem -13
#define MT_viewCloseBranch -14
#define MT_viewOpenBranch -15
#define MT_viewInsertItem -16
#define MT_viewRevealItem -17
#define MT_viewItemDimensions -18

struct viewSortColumn { CSTRING Column; LONG Descending;  };
struct viewSortColumnIndex { LONG Column; LONG Descending;  };
struct viewSelectItem { CSTRING XPath;  };
struct viewSetItem { CSTRING XPath; LONG TagIndex; CSTRING Tag; CSTRING Attrib; CSTRING Value;  };
struct viewRemoveItem { CSTRING XPath; LONG TagIndex;  };
struct viewRemoveTag { LONG TagIndex; LONG Total;  };
struct viewRaiseItem { CSTRING XPath; LONG TagIndex;  };
struct viewLowerItem { CSTRING XPath; LONG TagIndex;  };
struct viewCloseBranch { CSTRING XPath; LONG TagIndex;  };
struct viewOpenBranch { CSTRING XPath; LONG TagIndex; LONG Parents;  };
struct viewInsertItem { CSTRING XPath; LONG TagIndex; LONG Insert; CSTRING XML;  };
struct viewRevealItem { LONG TagIndex;  };
struct viewItemDimensions { LONG TagIndex; LONG X; LONG Y; LONG Width; LONG Height;  };

#define viewSelectAll(obj) Action(MT_viewSelectAll,(obj),0)

#define viewInvertSelection(obj) Action(MT_viewInvertSelection,(obj),0)

INLINE ERROR viewSortColumn(APTR Ob, CSTRING Column, LONG Descending) {
   struct viewSortColumn args = { Column, Descending };
   return(Action(MT_viewSortColumn, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewSortColumnIndex(APTR Ob, LONG Column, LONG Descending) {
   struct viewSortColumnIndex args = { Column, Descending };
   return(Action(MT_viewSortColumnIndex, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewSelectItem(APTR Ob, CSTRING XPath) {
   struct viewSelectItem args = { XPath };
   return(Action(MT_viewSelectItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewSetItem(APTR Ob, CSTRING XPath, LONG TagIndex, CSTRING Tag, CSTRING Attrib, CSTRING Value) {
   struct viewSetItem args = { XPath, TagIndex, Tag, Attrib, Value };
   return(Action(MT_viewSetItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewRemoveItem(APTR Ob, CSTRING XPath, LONG TagIndex) {
   struct viewRemoveItem args = { XPath, TagIndex };
   return(Action(MT_viewRemoveItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewRemoveTag(APTR Ob, LONG TagIndex, LONG Total) {
   struct viewRemoveTag args = { TagIndex, Total };
   return(Action(MT_viewRemoveTag, (OBJECTPTR)Ob, &args));
}

#define viewSelectNone(obj) Action(MT_viewSelectNone,(obj),0)

INLINE ERROR viewRaiseItem(APTR Ob, CSTRING XPath, LONG TagIndex) {
   struct viewRaiseItem args = { XPath, TagIndex };
   return(Action(MT_viewRaiseItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewLowerItem(APTR Ob, CSTRING XPath, LONG TagIndex) {
   struct viewLowerItem args = { XPath, TagIndex };
   return(Action(MT_viewLowerItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewCloseBranch(APTR Ob, CSTRING XPath, LONG TagIndex) {
   struct viewCloseBranch args = { XPath, TagIndex };
   return(Action(MT_viewCloseBranch, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewOpenBranch(APTR Ob, CSTRING XPath, LONG TagIndex, LONG Parents) {
   struct viewOpenBranch args = { XPath, TagIndex, Parents };
   return(Action(MT_viewOpenBranch, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewInsertItem(APTR Ob, CSTRING XPath, LONG TagIndex, LONG Insert, CSTRING XML) {
   struct viewInsertItem args = { XPath, TagIndex, Insert, XML };
   return(Action(MT_viewInsertItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewRevealItem(APTR Ob, LONG TagIndex) {
   struct viewRevealItem args = { TagIndex };
   return(Action(MT_viewRevealItem, (OBJECTPTR)Ob, &args));
}

INLINE ERROR viewItemDimensions(APTR Ob, LONG TagIndex, LONG * X, LONG * Y, LONG * Width, LONG * Height) {
   struct viewItemDimensions args = { TagIndex, 0, 0, 0, 0 };
   ERROR error = Action(MT_viewItemDimensions, (OBJECTPTR)Ob, &args);
   if (X) *X = args.X;
   if (Y) *Y = args.Y;
   if (Width) *Width = args.Width;
   if (Height) *Height = args.Height;
   return(error);
}


#define CT_DATA 0
#define CT_AUDIO 1
#define CT_IMAGE 2
#define CT_FILE 3
#define CT_OBJECT 4
#define CT_TEXT 5
#define CT_END 6

// Clipboard types

#define CLIPTYPE_DATA 0x00000001
#define CLIPTYPE_AUDIO 0x00000002
#define CLIPTYPE_IMAGE 0x00000004
#define CLIPTYPE_FILE 0x00000008
#define CLIPTYPE_OBJECT 0x00000010
#define CLIPTYPE_TEXT 0x00000020

// Clipboard flags

#define CLF_DRAG_DROP 0x00000001
#define CLF_HOST 0x00000002

#define CEF_DELETE 0x00000001
#define CEF_EXTEND 0x00000002

// Clipboard class definition

#define VER_CLIPBOARD (1.000000)

typedef struct rkClipboard {
   OBJECT_HEADER
   LONG     Flags;      // Optional flags
   MEMORYID ClusterID;  // Identifies the data cluster (item grouping) that the clipboard will work with

#ifdef PRV_CLIPBOARD
   FUNCTION RequestHandler;
   BYTE     ClusterAllocated:1;
  
#endif
} objClipboard;

// Clipboard methods

#define MT_ClipAddFile -1
#define MT_ClipAddObject -2
#define MT_ClipAddObjects -3
#define MT_ClipGetFiles -4
#define MT_ClipAddText -5
#define MT_ClipRemove -6

struct clipAddFile { LONG Datatype; CSTRING Path; LONG Flags;  };
struct clipAddObject { LONG Datatype; OBJECTID ObjectID; LONG Flags;  };
struct clipAddObjects { LONG Datatype; OBJECTID * Objects; LONG Flags;  };
struct clipGetFiles { LONG Datatype; LONG Index; CSTRING * Files; LONG Flags;  };
struct clipAddText { CSTRING String;  };
struct clipRemove { LONG Datatype;  };

INLINE ERROR clipAddFile(APTR Ob, LONG Datatype, CSTRING Path, LONG Flags) {
   struct clipAddFile args = { Datatype, Path, Flags };
   return(Action(MT_ClipAddFile, (OBJECTPTR)Ob, &args));
}

INLINE ERROR clipAddObject(APTR Ob, LONG Datatype, OBJECTID ObjectID, LONG Flags) {
   struct clipAddObject args = { Datatype, ObjectID, Flags };
   return(Action(MT_ClipAddObject, (OBJECTPTR)Ob, &args));
}

INLINE ERROR clipAddObjects(APTR Ob, LONG Datatype, OBJECTID * Objects, LONG Flags) {
   struct clipAddObjects args = { Datatype, Objects, Flags };
   return(Action(MT_ClipAddObjects, (OBJECTPTR)Ob, &args));
}

INLINE ERROR clipGetFiles(APTR Ob, LONG * Datatype, LONG Index, CSTRING ** Files, LONG * Flags) {
   struct clipGetFiles args = { 0, Index, 0, 0 };
   ERROR error = Action(MT_ClipGetFiles, (OBJECTPTR)Ob, &args);
   if (Datatype) *Datatype = args.Datatype;
   if (Files) *Files = args.Files;
   if (Flags) *Flags = args.Flags;
   return(error);
}

INLINE ERROR clipAddText(APTR Ob, CSTRING String) {
   struct clipAddText args = { String };
   return(Action(MT_ClipAddText, (OBJECTPTR)Ob, &args));
}

INLINE ERROR clipRemove(APTR Ob, LONG Datatype) {
   struct clipRemove args = { Datatype };
   return(Action(MT_ClipRemove, (OBJECTPTR)Ob, &args));
}


#endif
