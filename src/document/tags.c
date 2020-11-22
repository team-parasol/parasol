
static void check_para_attrib(objDocument *Self, STRING Attrib, STRING Value, struct escParagraph *esc);

//****************************************************************************

static void check_para_attrib(objDocument *Self, STRING Attrib, STRING Value, struct escParagraph *esc)
{
   if (!StrMatch(Attrib, "anchor")) {
      Self->Style.StyleChange = TRUE;
      Self->Style.FontStyle.Options |= FSO_ANCHOR;
   }
   else if (!StrMatch(Attrib, "leading")) {
      if (esc) {
         esc->LeadingRatio = StrToFloat(Value);
         if (esc->LeadingRatio < MIN_LEADING) esc->LeadingRatio = MIN_LEADING;
         else if (esc->LeadingRatio > MAX_LEADING) esc->LeadingRatio = MAX_LEADING;
      }
   }
   else if (!StrMatch(Attrib, "nowrap")) {
      Self->Style.StyleChange = TRUE;
      Self->Style.FontStyle.Options |= FSO_NO_WRAP;
   }
   else if (!StrMatch(Attrib, "kerning")) {  // REQUIRES CODE AND DOCUMENTATION

   }
   else if (!StrMatch(Attrib, "lineheight")) { // REQUIRES CODE AND DOCUMENTATION
      // Line height is expressed as a ratio - 1.0 is standard, 1.5 would be an extra half, 0.5 would squash the text by half.

      //Self->Style.LineHeightRatio = StrToFloat(Tag->Attrib[i].Value);
      //if (Self->Style.LineHeightRatio < MIN_LINEHEIGHT) Self->Style.LineHeightRatio = MIN_LINEHEIGHT;
   }
   else if (!StrMatch(Attrib, "trim")) {
      esc->Trim = TRUE;
   }
   else if (!StrMatch(Attrib, "vspacing")) {
      // Vertical spacing between embedded paragraphs.  Ratio is expressed as a measure of the *default* lineheight (not the height of the
      // last line of the paragraph).  E.g. 1.5 is one and a half times the standard lineheight.  The default is 1.0.

      if (esc) {
         esc->VSpacing = StrToFloat(Value);
         if (esc->VSpacing < MIN_VSPACING) esc->VSpacing = MIN_VSPACING;
         else if (esc->VSpacing > MAX_VSPACING) esc->VSpacing = MAX_VSPACING;
      }
   }
}

//****************************************************************************

static void trim_preformat(objDocument *Self, LONG *Index)
{
   LONG i;

   if ((i = *Index) > 0) {
      PREV_CHAR(Self->Stream, i);

      while (i > 0) {
         if (Self->Stream[i] IS '\n') {
         }
         else if (Self->Stream[i] IS CTRL_CODE) {
         }
         else break; // Content encountered

         PREV_CHAR(Self->Stream, i);
      }

      NEXT_CHAR(Self->Stream, i);

      Self->StreamLen -= *Index - i;
      *Index = i;
   }
}

/*****************************************************************************
** Internal: saved_style_check()
**
** This function is used to manage hierarchical styling:
**
** + Save Font Style
**   + Execute child tags
** + Restore Font Style
**
** If the last style that comes out of parse_tag() does not match the
** style stored in SaveStatus, we need to record a style change.
*/

static void saved_style_check(objDocument *Self, struct style_status *SaveStatus)
{
   UBYTE font = Self->Style.FontChange;
   UBYTE style = Self->Style.StyleChange;

   if (SaveStatus->FontStyle.Index != Self->Style.FontStyle.Index) font = TRUE;

   if ((SaveStatus->FontStyle.Options != Self->Style.FontStyle.Options) OR
       (((ULONG *)&SaveStatus->FontStyle.Colour)[0] != ((ULONG *)&Self->Style.FontStyle.Colour)[0])) {
      style = TRUE;
   }

   if ((font) OR (style)) {
      // Restore the style that we had before processing the children

      CopyMemory(SaveStatus, &Self->Style, sizeof(struct style_status));

      // Reapply the fontstate and stylestate information

      Self->Style.FontChange  = font;
      Self->Style.StyleChange = style;
   }
}

//****************************************************************************
// Advances the cursor.  It is only possible to advance positively on either axis.

static void tag_advance(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct escAdvance advance;

   ClearMemory(&advance, sizeof(advance));

   // Advances the cursor to a new position

   advance.X = StrToInt(XMLATTRIB(Tag, "x"));
   advance.Y = StrToInt(XMLATTRIB(Tag, "y"));

   if (advance.X < 0) advance.X = 0;
   else if (advance.X > 4000) advance.X = 4000;

   if (advance.Y < 0) advance.Y = 0;
   else if (advance.Y > 4000) advance.Y = 4000;

   insert_escape(Self, Index, ESC_ADVANCE, &advance, sizeof(advance));
}

//****************************************************************************
// NB: If a <body> tag contains any children, it is treated as a template and must contain an <inject/> tag so that
// the XML insertion point is known.

static void tag_body(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   #define MAX_BODY_MARGIN 500
   LONG i;

   // Body tag needs to be placed before any content

   for (i=1; i < Tag->TotalAttrib; i++) {
      ULONG hash_attrib = StrHash(Tag->Attrib[i].Name, 0);
      if (hash_attrib IS HASH_link) {
         StrToColour(Tag->Attrib[i].Value, &Self->LinkColour);
      }
      else if (hash_attrib IS HASH_vlink) {
         StrToColour(Tag->Attrib[i].Value, &Self->VLinkColour);
      }
      else if (hash_attrib IS HASH_selectcolour) { // Colour to use when a link is selected (using the tab key to get to a link will select it)
         StrToColour(Tag->Attrib[i].Value, &Self->SelectColour);
      }
      else if (hash_attrib IS HASH_leftmargin) {
         Self->LeftMargin = StrToInt(Tag->Attrib[i].Value);
         if (Self->LeftMargin < 0) Self->LeftMargin = 0;
         else if (Self->LeftMargin > MAX_BODY_MARGIN) Self->LeftMargin = MAX_BODY_MARGIN;
      }
      else if (hash_attrib IS HASH_rightmargin) {
         Self->RightMargin = StrToInt(Tag->Attrib[i].Value);
         if (Self->RightMargin < 0) Self->RightMargin = 0;
         else if (Self->RightMargin > MAX_BODY_MARGIN) Self->RightMargin = MAX_BODY_MARGIN;
      }
      else if (hash_attrib IS HASH_topmargin) {
         Self->TopMargin = StrToInt(Tag->Attrib[i].Value);
         if (Self->TopMargin < 0) Self->TopMargin = 0;
         else if (Self->TopMargin > MAX_BODY_MARGIN) Self->TopMargin = MAX_BODY_MARGIN;
      }
      else if (hash_attrib IS HASH_bottommargin) {
         Self->BottomMargin = StrToInt(Tag->Attrib[i].Value);
         if (Self->BottomMargin < 0) Self->BottomMargin = 0;
         else if (Self->BottomMargin > MAX_BODY_MARGIN) Self->BottomMargin = MAX_BODY_MARGIN;
      }
      else if (hash_attrib IS HASH_margins) {
         Self->LeftMargin   = StrToInt(Tag->Attrib[i].Value);
         if (Self->LeftMargin < 0) Self->LeftMargin = 0;
         else if (Self->LeftMargin > MAX_BODY_MARGIN) Self->LeftMargin = MAX_BODY_MARGIN;
         Self->RightMargin  = Self->LeftMargin;
         Self->TopMargin    = Self->LeftMargin;
         Self->BottomMargin = Self->LeftMargin;
      }
      else if (hash_attrib IS HASH_lineheight) {
         Self->LineHeight = StrToInt(Tag->Attrib[i].Value);
         if (Self->LineHeight < 4) Self->LineHeight = 4;
         else if (Self->LineHeight > 100) Self->LineHeight = 100;
      }
      else if ((hash_attrib IS HASH_pagewidth) OR
               (hash_attrib IS HASH_width)) {
         WORD j;

         Self->PageWidth = StrToFloat(Tag->Attrib[i].Value);
         if (Self->PageWidth < 1) Self->PageWidth = 1;
         else if (Self->PageWidth > 6000) Self->PageWidth = 6000;

         Self->RelPageWidth = FALSE;
         for (j=0; Tag->Attrib[i].Value[j]; j++) {
             if (Tag->Attrib[i].Value[j] IS '%') {
                Self->RelPageWidth = TRUE;
                break;
             }
         }
         LogF("insert_child:","Page width forced to %.0f%s.", Self->PageWidth, Self->RelPageWidth ? "%%" : "");
      }
      else if (hash_attrib IS HASH_colour) { // Background colour
         StrToColour(Tag->Attrib[i].Value, &Self->Background);
      }
      else if ((hash_attrib IS HASH_face) OR (hash_attrib IS HASH_fontface)) {
         SET_FontFace(Self, Tag->Attrib[i].Value);
      }
      else if (hash_attrib IS HASH_fontsize) { // Default font point size
         Self->FontSize = StrToInt(Tag->Attrib[i].Value);
      }
      else if (hash_attrib IS HASH_fontcolour) { // Default font colour
         StrToColour(Tag->Attrib[i].Value, &Self->FontColour);
      }
      else LogF("@insert_child()","Style attribute %s=%s not supported.", Tag->Attrib[i].Name, Tag->Attrib[i].Value);
   }

   StrCopy(Self->FontFace, Self->Style.Face, sizeof(Self->Style.Face));
   Self->Style.FontStyle.Index   = create_font(Self->FontFace, "Regular", Self->FontSize);
   Self->Style.FontStyle.Options = 0;
   Self->Style.FontStyle.Colour  = Self->FontColour;
   Self->Style.Point         = Self->FontSize;
   Self->Style.FontChange    = TRUE;
   Self->Style.StyleChange   = TRUE;

   Self->BodyTag = Child;
}

//****************************************************************************
// In background mode, all objects are targetted to the view surface rather than the page surface.

static void tag_background(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->BkgdGfx++;
   parse_tag(Self, XML, Child, Index, 0);
   Self->BkgdGfx--;
}

//****************************************************************************

static void tag_bold(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (!(Self->Style.FontStyle.Options & FSO_BOLD)) {
      struct style_status savestatus = Self->Style; // Save the current style
      Self->Style.FontChange = TRUE; // Bold fonts are typically a different typeset
      Self->Style.FontStyle.Options |= FSO_BOLD;
      parse_tag(Self, XML, Child, Index, 0);
      saved_style_check(Self, &savestatus);
   }
   else parse_tag(Self, XML, Child, Index, Flags & (~FILTER_ALL));
}

//****************************************************************************

static void tag_br(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   insert_text(Self, Index, "\n", 1, FSO_PREFORMAT);
   Self->NoWhitespace = TRUE;
}

/*****************************************************************************
** Internal: tag_cache()
**
** Use caching to create objects that will persist between document refreshes
** and page changes (so long as said page resides within the same document source).
** The following code illustrates how to create a persistent XML object:
**
** <if not exists="[xml192]">
**   <cache>
**     <xml name="xml192"/>
**   </cache>
** </if>
**
** The object is removed when the document object is destroyed, or the document
** source is changed.
**
** NOTE: Another valid method of caching an object is to use a persistent script.
*/

static void tag_cache(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->ObjectCache++;
   parse_tag(Self, XML, Child, Index, 0);
   Self->ObjectCache--;
}

/*****************************************************************************
** Internal: tag_call()
**
** Use this instruction to call a function during the parsing of the document.
**
** The only argument required by this tag is 'function'.  All following attributes are treated as arguments that are
** passed to the called procedure (note that arguments are passed in the order in which they appear).
**
** Global arguments can be set against the script object itself if the argument is prefixed with an underscore.
**
** To call a function that isn't in the default script, simply specify the name of the target script before the
** function name, split with a dot, e.g. "script.function".
**
** <call function="[script].function" arg1="" arg2="" _global=""/>
*/

static void tag_call(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i;

   OBJECTPTR script = Self->DefaultScript;

   STRING function = NULL;
   if (Tag->TotalAttrib > 1) {
      if (!StrMatch("function", Tag->Attrib[1].Name)) {
         function = Tag->Attrib[1].Value;
         for (i=0; (function[i]) AND (function[i] IS '.'); i++);
         if (function[i] IS '.') {
            function[i] = 0;
            FindPrivateObject(function, &script);
            function[i] = '.';
            function = function + i;
         }
      }
   }

   if (!function) {
      LogErrorMsg("The first attribute to <call/> must be a function reference.");
      Self->Error = ERR_Syntax;
      return;
   }

   if (!script) {
      LogErrorMsg("No script in this document for a requested <call/>.");
      Self->Error = ERR_Failed;
      return;
   }

   FMSG("tag_call()","Calling script #%d function '%s'", GetUniqueID(script), function);

   if (Tag->TotalAttrib > 2) {
      LONG index;
      struct ScriptArg args[40];

      index = 0;
      for (i=2; (i < Tag->TotalAttrib) AND (index < ARRAYSIZE(args)); i++) {
         if (Tag->Attrib[i].Name[0] IS '_') {
            // Global variable setting
            acSetVar(script,Tag->Attrib[i].Name+1, Tag->Attrib[i].Value);
         }
         else {
            args[index].Name    = Tag->Attrib[i].Name;
            if (args[index].Name[0] IS '@') args[index].Name++;
            args[index].Type    = FD_STRING;
            args[index].Address = Tag->Attrib[i].Value;
            index++;
         }
      }

      scExec(script, function, args, index);
   }
   else scExec(script, function, NULL, 0);

   LOGRETURN();

   // Check for a result and print it

   CSTRING *results;
   LONG size;
   if ((!GetFieldArray(script, FID_Results, &results, &size)) AND (size > 0)) {
      objXML *xmlinc;
      if (!CreateObject(ID_XML, 0, &xmlinc,
            FID_Statement|TSTR, results[0],
            FID_Flags|TLONG,    XMF_PARSE_HTML|XMF_STRIP_HEADERS,
            TAGEND)) {

         parse_tag(Self, xmlinc, xmlinc->Tags[0], Index, Flags);

         // Add the created XML object to the document rather than destroying it

         add_resource_id(Self, xmlinc->Head.UniqueID, RT_OBJECT_TEMP);
      }
      FreeResource(results);
   }
}

//****************************************************************************

static void tag_caps(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (!(Self->Style.FontStyle.Options & FSO_CAPS)) {
      struct style_status savestatus;
      savestatus = Self->Style; // Save the current style
      Self->Style.StyleChange = TRUE;
      Self->Style.FontStyle.Options |= FSO_CAPS;
      parse_tag(Self, XML, Tag->Child, Index, 0);
      saved_style_check(Self, &savestatus);
   }
   else parse_tag(Self, XML, Tag->Child, Index, Flags);
}

//****************************************************************************

static void tag_debug(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i;

   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("msg", Tag->Attrib[i].Name)) {
         LogF("@DocMsg:", "%s", Tag->Attrib[i].Value); // Using %s rather than a direct reference to msg to prevent formatting interpretation
      }
   }
}

//****************************************************************************
// Use div to structure the document in a similar way to paragraphs.  Its main
// difference is that it avoids the declaration of paragraph start and end points.

static void tag_div(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct style_status savestatus;
   LONG i;

   savestatus = Self->Style;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("align", Tag->Attrib[i].Name)) {
         if ((!StrMatch(Tag->Attrib[i].Value, "center")) OR (!StrMatch(Tag->Attrib[i].Value, "horizontal"))) {
            Self->Style.StyleChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ALIGN_CENTER;
         }
         else if (!StrMatch(Tag->Attrib[i].Value, "right")) {
            Self->Style.StyleChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ALIGN_RIGHT;
         }
         else LogErrorMsg("Alignment type '%s' not supported.", Tag->Attrib[i].Value);
      }
      else check_para_attrib(Self, Tag->Attrib[i].Name, Tag->Attrib[i].Value, 0);
   }

   parse_tag(Self, XML, Child, Index, 0);
   saved_style_check(Self, &savestatus);
}

//****************************************************************************
// Creates a new edit definition.  These are stored in a linked list.  Edit definitions are used by referring to them
// by name in table cells.

static void tag_editdef(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct DocEdit edit;
   STRING onchange, onenter, onexit;
   LONG i, totalargs, bufsize;

   totalargs = 0;
   bufsize   = 0;
   onchange  = NULL;
   onenter   = NULL;
   onexit    = NULL;

   ClearMemory(&edit, sizeof(edit));
   edit.MaxChars = -1;
   edit.LineBreaks = FALSE;

   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("maxchars", Tag->Attrib[i].Name)) {
         edit.MaxChars = StrToInt(Tag->Attrib[i].Value);
         if (edit.MaxChars < 0) edit.MaxChars = -1;
      }
      else if (!StrMatch("name", Tag->Attrib[i].Name)) {
         edit.NameHash = StrHash(Tag->Attrib[i].Value, 0);
      }
      else if (!StrMatch("selectcolour", Tag->Attrib[i].Name)) {


      }
      else if (!StrMatch("linebreaks", Tag->Attrib[i].Name)) {
         edit.LineBreaks = StrToInt(Tag->Attrib[i].Value);
      }
      else if (!StrMatch("editfonts", Tag->Attrib[i].Name)) {

      }
      else if (!StrMatch("editimages", Tag->Attrib[i].Name)) {

      }
      else if (!StrMatch("edittables", Tag->Attrib[i].Name)) {

      }
      else if (!StrMatch("editall", Tag->Attrib[i].Name)) {

      }
      else if (!StrMatch("onchange", Tag->Attrib[i].Name)) {
         if ((!onchange) AND (Tag->Attrib[i].Value[0])) {
            bufsize += StrLength(Tag->Attrib[i].Value) + 1;
            onchange = Tag->Attrib[i].Value;
         }
      }
      else if (!StrMatch("onexit", Tag->Attrib[i].Name)) {
         if ((!onexit) AND (Tag->Attrib[i].Value[0])) {
            bufsize += StrLength(Tag->Attrib[i].Value) + 1;
            onexit = Tag->Attrib[i].Value;
         }
      }
      else if (!StrMatch("onenter", Tag->Attrib[i].Name)) {
         if ((!onenter) AND (Tag->Attrib[i].Value[0])) {
            bufsize += StrLength(Tag->Attrib[i].Value) + 1;
            onenter = Tag->Attrib[i].Value;
         }
      }
      else if (Tag->Attrib[i].Name[0] IS '@') {
         if (totalargs < 32) {
            totalargs++;
            bufsize += StrLength(Tag->Attrib[i].Name) - 1 + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
      else if (Tag->Attrib[i].Name[0] IS '_') {
         if (totalargs < 32) {
            totalargs++;
            bufsize += StrLength(Tag->Attrib[i].Name) + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
   }

   if (bufsize > 4096) {
      Self->Error = ERR_BufferOverflow;
      return;
   }

   struct DocEdit *editptr, *last;
   LONG offset;

   if (!AllocMemory(sizeof(struct DocEdit)+bufsize, MEM_NO_CLEAR, &editptr, NULL)) {
      CopyMemory(&edit, editptr, sizeof(struct DocEdit));
      if (bufsize > 0) {
         offset = sizeof(struct DocEdit);

         if (onchange) {
            editptr->OnChange = offset;
            offset += StrCopy(onchange, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
         }

         if (onexit) {
            editptr->OnExit = offset;
            offset += StrCopy(onexit, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
         }

         if (onenter) {
            editptr->OnEnter = offset;
            offset += StrCopy(onenter, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
         }

         if (totalargs) {
            LONG count;

            editptr->TotalArgs = totalargs;
            editptr->Args = offset;

            count = 0;
            for (i=1; (i < Tag->TotalAttrib) AND (count < totalargs); i++) {
               if (Tag->Attrib[i].Name[0] IS '@') {
                  count++;
                  offset += StrCopy(Tag->Attrib[i].Name+1, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
                  offset += StrCopy(Tag->Attrib[i].Value, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
               }
               else if (Tag->Attrib[i].Name[0] IS '_') {
                  count++;
                  offset += StrCopy(Tag->Attrib[i].Name, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
                  offset += StrCopy(Tag->Attrib[i].Value, ((UBYTE *)editptr)+offset, COPY_ALL) + 1;
               }
            }
         }
      }

      if (Self->EditDefs) {
         for (last=Self->EditDefs; last->Next; last=last->Next);
         last->Next = editptr;
      }
      else Self->EditDefs = editptr;
   }
   else Self->Error = ERR_AllocMemory;
}

//****************************************************************************
// This very simple tag tells the parser that the object or link that immediately follows the focus element should
// have the initial focus when the user interacts with the document.  Commonly used for things such as input boxes.
//
// If the focus tag encapsulates any content, it will be processed in the same way as if it were to immediately follow
// the closing tag.
//
// Note that for hyperlinks, the 'select' attribute can also be used as a convenient means to assign focus.

static void tag_focus(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->FocusIndex = Self->TabIndex;
}

//****************************************************************************

static void tag_footer(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->FooterTag = Child;
}

//****************************************************************************

static void tag_header(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->HeaderTag = Child;
}

/*****************************************************************************
** Internal: tag_indent()
**
** Indent document block.  The extent of the indentation can be customised in
** the Units value.
*/

static void tag_indent(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct escParagraph esc;
   LONG i, j;

   ClearMemory(&esc, sizeof(esc));

   esc.Indent = DEFAULT_INDENT;
   esc.VSpacing = 1.0;
   esc.LeadingRatio = 1.0;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch(Tag->Attrib[i].Name, "units")) {
         esc.Indent = StrToInt(Tag->Attrib[i].Name);
         if (esc.Indent < 0) esc.Indent = 0;
         for (j=0; Tag->Attrib[i].Name[j]; j++) {
            if (Tag->Attrib[i].Name[j] IS '%') { esc.Relative = TRUE; break; }
         }
      }
      else check_para_attrib(Self, Tag->Attrib[i].Name, Tag->Attrib[i].Value, &esc);
   }

   insert_paragraph_start(Self, Index, &esc);

      parse_tag(Self, XML, Child, Index, 0);

   insert_paragraph_end(Self, Index);
}

//****************************************************************************
// Use of <meta> for custom information is allowed and is ignored by the parser.

static void tag_head(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct XMLTag *scan;

   // The head contains information about the document

   for (scan=Tag->Child; scan; scan=scan->Next) {
      // Anything allocated here needs to be freed in unload_doc()
      if (!StrMatch("title", scan->Attrib->Name)) {
         if ((scan->Child) AND (!scan->Child->Attrib->Name)) {
            if (Self->Title) FreeResource(Self->Title);
            Self->Title = StrClone(scan->Child->Attrib->Value);
         }
      }
      else if (!StrMatch("author", scan->Attrib->Name)) {
         if ((scan->Child) AND (!scan->Child->Attrib->Name)) {
            if (Self->Author) FreeResource(Self->Author);
            Self->Author = StrClone(scan->Child->Attrib->Value);
         }
      }
      else if (!StrMatch("copyright", scan->Attrib->Name)) {
         if ((scan->Child) AND (!scan->Child->Attrib->Name)) {
            if (Self->Copyright) FreeResource(Self->Copyright);
            Self->Copyright = StrClone(scan->Child->Attrib->Value);
         }
      }
      else if (!StrMatch("keywords", scan->Attrib->Name)) {
         if ((scan->Child) AND (!scan->Child->Attrib->Name)) {
            if (Self->Keywords) FreeResource(Self->Keywords);
            Self->Keywords = StrClone(scan->Child->Attrib->Value);
         }
      }
      else if (!StrMatch("description", scan->Attrib->Name)) {
         if ((scan->Child) AND (!scan->Child->Attrib->Name)) {
            if (Self->Description) FreeResource(Self->Description);
            Self->Description = StrClone(scan->Child->Attrib->Value);
         }
      }
   }
}

//****************************************************************************
// Include XML from another RIPPLE file.

static void tag_include(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   objXML *xmlinc;
   STRING src;

   if ((src = XMLATTRIB(Tag, "src"))) {
      if (!CreateObject(ID_XML, NF_INTEGRAL, &xmlinc,
            FID_Path|TSTR,   src,
            FID_Flags|TLONG, XMF_PARSE_HTML|XMF_STRIP_HEADERS,
            TAGEND)) {

         parse_tag(Self, xmlinc, xmlinc->Tags[0], Index, Flags);

         add_resource_id(Self, xmlinc->Head.UniqueID, RT_OBJECT_TEMP);
      }
      else LogErrorMsg("Failed to include '%s'", src);
   }
   else LogErrorMsg("<include> directive missing required 'src' element.");
}

//****************************************************************************

static void tag_parse(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   // The value attribute will contain XML.  We will parse the XML as if it were part of the document source.  This feature
   // is typically used when pulling XML information out of an object field.

   objXML *xmlinc;
   STRING tagname;

   if (Tag->TotalAttrib > 1) {
      tagname = Tag->Attrib[1].Name;
      if (*tagname IS '$') tagname++;

      if (!StrMatch("value", tagname)) {
         StrCopy(Tag->Attrib[1].Value, Self->Temp, Self->TempSize);

         FMSG("~tag_parse()","Parsing string value as XML...");

         if (!CreateObject(ID_XML, NF_INTEGRAL, &xmlinc,
               FID_Statement|TSTR, Self->Temp,
               FID_Flags|TLONG,    XMF_PARSE_HTML|XMF_STRIP_HEADERS,
               TAGEND)) {

            parse_tag(Self, xmlinc, xmlinc->Tags[0], Index, Flags);

            // Add the created XML object to the document rather than destroying it

            add_resource_id(Self, xmlinc->Head.UniqueID, RT_OBJECT_TEMP);
         }

         LOGRETURN();
      }
   }
}

//****************************************************************************
// Indexes set bookmarks that can be used for quick-scrolling to document sections.  They can also be used to mark
// sections of content that may require run-time modification.
//
// <index name="News">
//   <p>Something in here.</p>
// </index>
//
// If the name attribute is not specified, an attempt will be made to derive the name from the first immediate string
// of the index' content, e.g:
//
//   <index>News</>
//
// The developer can use indexes to bookmark areas of code that are of interest.  The FindIndex() method is used for
// this purpose.

static void tag_index(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i;
   ULONG name;
   UBYTE visible;

   name = 0;
   visible = TRUE;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("name", Tag->Attrib[i].Name)) {
         name = StrHash(Tag->Attrib[i].Value, 0);
      }
      else if (!StrMatch("hide", Tag->Attrib[i].Name)) {
         visible = FALSE;
      }
      else LogErrorMsg("<index> unsupported attribute '%s'", Tag->Attrib[i].Name);
   }

   if (!name) {
      if ((Child) AND (Child->Attrib)) {
         if ((!Child->Attrib->Name) AND (Child->Attrib->Value)) name = StrHash(Child->Attrib->Value, 0);
      }
   }

   // This style check ensures that the font style is up to date before the start of the index.
   // This is important if the developer wants to insert content at the start of the index,
   // as that content should have the attributes of the current font style.

   style_check(Self, Index);

   if (name) {
      struct escIndex esc;
      struct escIndexEnd end;

      esc.NameHash = name;
      esc.ID       = Self->UniqueID++;
      esc.Y        = 0;
      esc.Visible  = visible;
      if (Self->Invisible) esc.ParentVisible = FALSE;
      else esc.ParentVisible = TRUE;

      insert_escape(Self, Index, ESC_INDEX_START, &esc, sizeof(esc));

      if (Child) {
         if (!visible) Self->Invisible++;
         parse_tag(Self, XML, Child, Index, 0);
         if (!visible) Self->Invisible--;
      }

      end.ID = esc.ID;
      insert_escape(Self, Index, ESC_INDEX_END, &end, sizeof(end));
   }
   else if (Child) parse_tag(Self, XML, Child, Index, 0);
}

//****************************************************************************
// If calling a function with 'onclick', all arguments must be identified with the @ prefix.  Parameters will be
// passed to the function in the order in which they are given.  Global values can be set against the document
// object itself, if a parameter is prefixed with an underscore.
//
// Script objects can be specifically referenced when calling a function, e.g. "myscript.function".  If no script
// object is referenced, then it is assumed that the default script contains the function.
//
// <a href="http://" onclick="function" colour="rgb" @arg1="" @arg2="" _global=""/>
//
// Dummy links that specify neither an href or onclick value can be useful in embedded documents if the
// EventCallback feature is used.

static void tag_link(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   escLink link;
   link.Type  = 0;
   link.Args  = 0;
   link.PointerMotion = 0;

   LONG argsize    = 0;
   LONG buffersize = sizeof(link);
   STRING href     = NULL;
   STRING function = NULL;
   STRING colour   = NULL;
   BYTE   select   = FALSE;
   STRING hint     = NULL;
   STRING pointermotion = NULL;

   LONG i;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if ((!link.Type) AND (!StrMatch("href", Tag->Attrib[i].Name))) {
         if ((href = Tag->Attrib[i].Value)) {
            link.Type = LINK_HREF;
            buffersize += StrLength(href) + 1;
         }
      }
      else if ((!link.Type) AND (!StrMatch("onclick", Tag->Attrib[i].Name))) { // Function to execute on click
         if ((function = Tag->Attrib[i].Value)) {
            link.Type = LINK_FUNCTION;
            buffersize += StrLength(function) + 1;
         }
      }
      else if ((!StrMatch("hint", Tag->Attrib[i].Name)) AND
               (!StrMatch("title", Tag->Attrib[i].Name))) { // 'title' is the http equivalent of our 'hint'
         LogMsg("No support for <a> hints yet.");
         hint = Tag->Attrib[i].Value;
      }
      else if (!StrMatch("colour", Tag->Attrib[i].Name)) {
         colour = Tag->Attrib[i].Value;
      }
      else if (!StrMatch("pointermotion", Tag->Attrib[i].Name)) { // Function to execute on pointer motion
         if ((pointermotion = Tag->Attrib[i].Value)) {
            buffersize += StrLength(pointermotion) + 1;
         }
      }
      else if (Tag->Attrib[i].Name[0] IS '@') {
         if ((link.Args < 64) AND (argsize < 4096)) {
            link.Args++;
            argsize += StrLength(Tag->Attrib[i].Name) - 1 + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
      else if (Tag->Attrib[i].Name[0] IS '_') {
         if ((link.Args < 64) AND (argsize < 4096)) {
            link.Args++;
            argsize += StrLength(Tag->Attrib[i].Name) + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
      else if (!StrMatch("select", Tag->Attrib[i].Name)) {
         select = TRUE;
      }
      else LogErrorMsg("<a|link> unsupported attribute '%s'", Tag->Attrib[i].Name);
   }

   buffersize += argsize;
   UBYTE buffer[buffersize];

   if ((link.Type) OR (Tag->Child)) {
      link.ID = ++Self->LinkID;
      link.Align = Self->Style.FontStyle.Options;

      LONG pos = sizeof(link);
      if (link.Type IS LINK_FUNCTION) {
         pos += StrCopy(function, buffer + pos, COPY_ALL) + 1;
      }
      else pos += StrCopy(href ? href : "", buffer + pos, COPY_ALL) + 1;

      LONG count = 0;
      for (i=1; i < Tag->TotalAttrib; i++) {
         if ((Tag->Attrib[i].Name) AND (Tag->Attrib[i].Name[0] IS '@')) {
            count++;
            pos += StrCopy(Tag->Attrib[i].Name+1, buffer+pos, COPY_ALL) + 1;
            pos += StrCopy(Tag->Attrib[i].Value, buffer+pos, COPY_ALL) + 1;
            if (count >= link.Args) break;
         }
         else if ((Tag->Attrib[i].Name) AND (Tag->Attrib[i].Name[0] IS '_')) {
            count++;
            pos += StrCopy(Tag->Attrib[i].Name, buffer+pos, COPY_ALL) + 1;
            pos += StrCopy(Tag->Attrib[i].Value, buffer+pos, COPY_ALL) + 1;
            if (count >= link.Args) break;
         }
      }
      link.Args = count;

      if (pointermotion) {
         link.PointerMotion = pos;
         pos += StrCopy(pointermotion, buffer + pos, COPY_ALL) + 1;
      }

      CopyMemory(&link, buffer, sizeof(link));

      insert_escape(Self, Index, ESC_LINK, buffer, buffersize);

      struct style_status savestatus = Self->Style;

      Self->Style.StyleChange    = TRUE;
      Self->Style.FontStyle.Options |= FSO_UNDERLINE;
      Self->Style.FontStyle.Colour   = Self->LinkColour;

      if (colour) StrToColour(colour, &Self->Style.FontStyle.Colour);

      parse_tag(Self, XML, Tag->Child, Index, 0);

      saved_style_check(Self, &savestatus);

      insert_escape(Self, Index, ESC_LINK_END, 0, 0);

      // This style check will forcibly revert the font back to whatever it was rather than waiting for new content to result in
      // a change.  The reason why we want to do this is to make it easier to manage run-time insertion of new content.  For
      // instance if the user enters text on a new line following an <h1> heading, the user's expectation would be for the new
      // text to be in the format of the body's font and not the <h1> font.

      style_check(Self, Index);

      // Links are added to the list of tab-able points

      i = add_tabfocus(Self, TT_LINK, link.ID);
      if (select) Self->FocusIndex = i;
   }
   else parse_tag(Self, XML, Tag->Child, Index, Flags & (~FILTER_ALL));
}

//****************************************************************************

#define LIST_BUFFER_SIZE 80

static void tag_list(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct escList esc, *savelist;
   LONG i;
   UBYTE buffer[LIST_BUFFER_SIZE];

   ClearMemory(&esc, sizeof(esc));

   esc.Colour      = Self->Style.FontStyle.Colour; // Default colour matches the current font colour
   esc.Start       = 1;
   esc.VSpacing    = 0.5;
   esc.Type        = LT_BULLET;
   esc.BlockIndent = BULLET_WIDTH; // Indenting for child items
   esc.ItemIndent  = BULLET_WIDTH; // Indenting from the item graphic - applies to bullet style only
   esc.OrderInsert = 0;
   esc.ItemNum     = esc.Start;
   esc.Buffer      = buffer;
   buffer[0] = 0;

   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("colour", Tag->Attrib[i].Name)) {
         StrToColour(Tag->Attrib[i].Value, &esc.Colour);
      }
      else if (!StrMatch("indent", Tag->Attrib[i].Name)) {
         // Affects the indenting to apply to child items.
         esc.BlockIndent = StrToInt(Tag->Attrib[i].Value);
      }
      else if (!StrMatch("vspacing", Tag->Attrib[i].Name)) {
         esc.VSpacing = StrToFloat(Tag->Attrib[i].Value);
         if (esc.VSpacing < 0) esc.VSpacing = 0;
      }
      else if (!StrMatch("type", Tag->Attrib[i].Name)) {
         if (!StrMatch("bullet", Tag->Attrib[i].Value)) {
            esc.Type = LT_BULLET;
         }
         else if (!StrMatch("ordered", Tag->Attrib[i].Value)) {
            esc.Type = LT_ORDERED;
            esc.ItemIndent = 0;
         }
         else if (!StrMatch("custom", Tag->Attrib[i].Value)) {
            esc.Type = LT_CUSTOM;
            esc.ItemIndent = 0;
         }
      }
   }

   style_check(Self, Index); // Font changes must take place prior to the list for correct bullet point alignment

   // Note: Paragraphs are not inserted because <li> does this

   insert_escape(Self, Index, ESC_LIST_START, &esc, sizeof(esc));

   savelist = Self->Style.List;
   Self->Style.List = &esc;

      if (Child) parse_tag(Self, XML, Child, Index, 0);

   Self->Style.List = savelist;

   insert_escape(Self, Index, ESC_LIST_END, 0, 0);

   Self->NoWhitespace = TRUE;
}

//****************************************************************************
// Also see check_para_attrib() for paragraph attributes.

static void tag_paragraph(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct style_status savestatus;
   LONG i;
   struct escParagraph esc;

   ClearMemory(&esc, sizeof(esc));

   esc.VSpacing = 1.0;
//   esc.LeadingRatio = 1.0;

   savestatus = Self->Style;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("align", Tag->Attrib[i].Name)) {
         if ((!StrMatch(Tag->Attrib[i].Value, "center")) OR (!StrMatch(Tag->Attrib[i].Value, "horizontal"))) {
            Self->Style.StyleChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ALIGN_CENTER;
         }
         else if (!StrMatch(Tag->Attrib[i].Value, "right")) {
            Self->Style.StyleChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ALIGN_RIGHT;
         }
         else LogErrorMsg("Alignment type '%s' not supported.", Tag->Attrib[i].Value);
      }
      else check_para_attrib(Self, Tag->Attrib[i].Name, Tag->Attrib[i].Value, &esc);
   }

   insert_escape(Self, Index, ESC_PARAGRAPH_START, &esc, sizeof(esc));
   if (esc.Trim) Self->NoWhitespace = TRUE; // No whitespace
   else Self->NoWhitespace = FALSE; // Allow whitespace

   parse_tag(Self, XML, Child, Index, 0);
   saved_style_check(Self, &savestatus);

   insert_paragraph_end(Self, Index);
   Self->NoWhitespace = TRUE;

   // This style check will forcibly revert the font back to whatever it was rather than waiting for new content to
   // result in a change.  The reason why we want to do this is to make it easier to manage run-time insertion of new
   // content.  For instance if the user enters text on a new line following an <h1> heading, the user's
   // expectation would be for the new text to be in the format of the body's font and not the <h1> font.

   style_check(Self, Index);
}

//****************************************************************************

static void tag_print(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   // Copy the content from the value attribute into the document stream.  If used inside an object, the data is sent
   // to that object as XML.

   if (Tag->TotalAttrib > 1) {
      STRING tagname = Tag->Attrib[1].Name;
      if (*tagname IS '$') tagname++;

      if (!StrMatch("value", tagname)) {
         if (Self->CurrentObject) {
            acDataText(Self->CurrentObject, Tag->Attrib[1].Value);
         }
         else {
            StrCopy(Tag->Attrib[1].Value, Self->Temp, Self->TempSize);
            insert_text(Self, Index, Self->Temp, StrLength(Self->Temp), (Self->Style.FontStyle.Options & FSO_PREFORMAT) ? TRUE : FALSE);
         }
      }
      else if (!StrMatch("src", Tag->Attrib[1].Name)) {
         // This option is only supported in unrestricted mode
         if (Self->Flags & DCF_UNRESTRICTED) {
            struct CacheFile *cache;
            if (!LoadFile(Tag->Attrib[1].Value, 0, &cache)) {
               insert_text(Self, Index, cache->Data, cache->Size, (Self->Style.FontStyle.Options & FSO_PREFORMAT) ? TRUE : FALSE);
               UnloadFile(cache);
            }
         }
         else LogErrorMsg("Cannot <print src.../> unless in unrestricted mode.");
      }
   }
}

//****************************************************************************
// Sets the attributes of an object.  NOTE: For security reasons, this feature is limited to objects that are children
// of the document object.
//
//   <set object="" fields .../>
//
//   <set arg=value .../>
//
// Note: XML validity could be improved restricting the set tag so that args were set as <set arg="argname"
// value="value"/>, however apart from being more convoluted, this would also result in more syntactic cruft as each
// arg setting would require its own set element.

static void tag_set(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i;

   if (Tag->TotalAttrib > 1) {
      if (!StrMatch("object", Tag->Attrib[1].Name)) {
         OBJECTID objectid;
         if (!FastFindObject(Tag->Attrib[1].Value, 0, &objectid, 1, 0)) {
            if (valid_objectid(Self, objectid) IS TRUE) {
               OBJECTPTR object;
               if (!AccessObject(objectid, 3000, &object)) {
                  for (i=2; i < Tag->TotalAttrib; i++) {
                     FMSG("tag_set:","#%d %s = '%s'", objectid, Tag->Attrib[i].Name, Tag->Attrib[i].Value);
                     if (Tag->Attrib[i].Name[0] IS '@') {
                        SetFieldEval(object, Tag->Attrib[i].Name+1, Tag->Attrib[i].Value);
                     }
                     else SetFieldEval(object, Tag->Attrib[i].Name, Tag->Attrib[i].Value);
                  }
                  ReleaseObject(object);
               }
            }
         }
      }
      else {
         // Set document arguments
         for (i=1; i < Tag->TotalAttrib; i++) {
            if (Tag->Attrib[i].Name[0] IS '@') {
               acSetVar(Self, Tag->Attrib[i].Name+1, Tag->Attrib[i].Value);
            }
            else acSetVar(Self, Tag->Attrib[i].Name, Tag->Attrib[i].Value);
         }
      }
   }
}

//****************************************************************************

static void tag_template(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   // Templates can be used to create custom tags.
   //
   // <template name="customimage">
   //   <image src="" background="#f0f0f0"/>
   // </template>

   if (!Self->InTemplate) {
      add_template(Self, XML, Tag);
   }
}

//****************************************************************************
// Used to send XML data to an embedded object.
//
// NOTE: If no child tags or content is inside the XML string, or if attributes are attached to the XML tag, then the
// user is trying to create a new XML object (under the Data category), not the XML reserved word.

static void tag_xml(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   tag_xml_content(Self, XML, Tag, PXF_ARGS);
}

static void tag_xmlraw(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   tag_xml_content(Self, XML, Tag, 0);
}

static void tag_xmltranslate(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   tag_xml_content(Self, XML, Tag, PXF_TRANSLATE|PXF_ARGS);
}

//****************************************************************************
// For use the by tag_xml*() range of functions only.

static void tag_xml_content(objDocument *Self, objXML *XML, struct XMLTag *Tag, WORD Flags)
{
   struct MemInfo meminfo;
   OBJECTPTR target;
   STRING xmlstr, str;
   LONG i, b_revert;
   UBYTE e_revert, s_revert;

   if (!Tag->Child) return;

   if (Tag->Index >= XML->TagCount) {
      LogErrorMsg("Illegal tag index %d >= %d", Tag->Index, XML->TagCount);
      return;
   }

   STRING buffer = Self->Buffer + Self->BufferIndex;
   LONG size = Self->BufferSize - Self->BufferIndex;

   if ((str = XMLATTRIB(Tag, "object"))) {
      FindPrivateObject(str, &target);
      if (valid_object(Self, target) IS FALSE) {
         LOGRETURN();
         return;
      }
   }
   else target = Self->CurrentObject;

   Tag = Tag->Child;

   LAYOUT("~tag_xml()","XML: %d, Tag: %d/%d, Target: %d", XML->Head.UniqueID, Tag->Index, XML->TagCount, target->UniqueID);

   if (!target) {
      LogErrorMsg("<xml> used without a valid object reference to receive the XML.");
      LAYOUT_LOGRETURN();
      return;
   }

   b_revert = Self->BufferIndex;
   s_revert = Self->ArgIndex;
   e_revert = 0;

   if (Flags & (PXF_ARGS|PXF_TRANSLATE)) {
      LAYOUT("tag_xml","Converting args from tag %d.", Tag->Index);
      for (i=Tag->Index; XML->Tags[Tag->Index]->Branch <= XML->Tags[i]->Branch; i++) {
         convert_xml_args(Self, XML->Tags[i]->Attrib, XML->Tags[i]->TotalAttrib);
      }
      e_revert = Self->ArgIndex;
   }

   LAYOUT("tag_xml:","Getting string.");

   if (!xmlGetString(XML, Tag->Index, XMF_INCLUDE_SIBLINGS, &xmlstr)) {
      if (Flags & PXF_TRANSLATE) {
         LAYOUT("tag_xml:","Translating...");
         if ((!MemoryPtrInfo(xmlstr, &meminfo)) AND
             (meminfo.Size > size)) {
            acDataXML(target, xmlstr);
         }
         else {
            StrCopy(xmlstr, buffer, size);
            if (Flags & PXF_TRANSLATE) safe_translate(buffer, size, SEF_STRICT|SEF_IGNORE_QUOTES);
            acDataXML(target, buffer);
         }
      }
      else acDataXML(target, xmlstr);

      FreeResource(xmlstr);
   }

   if (Flags & (PXF_ARGS|PXF_TRANSLATE)) {
      LAYOUT("tag_xml:","Reverting attributes.");
      if (e_revert > s_revert) {
         while (e_revert > s_revert) {
            e_revert--;
            Self->VArg[e_revert].Attrib[0] = Self->VArg[e_revert].String;
         }
      }
   }

   Self->BufferIndex = b_revert;
   Self->ArgIndex = s_revert;

   LAYOUT_LOGRETURN();
}

//****************************************************************************

static void tag_font(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct style_status savestatus = Self->Style; // Save the current style
   UBYTE preformat = FALSE;
   LONG flags = 0;

   LONG i;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("colour", Tag->Attrib[i].Name)) {
         Self->Style.StyleChange = TRUE;
         StrToColour(Tag->Attrib[i].Value, &Self->Style.FontStyle.Colour);
      }
      else if (!StrMatch("face", Tag->Attrib[i].Name)) {
         STRING str;
         LONG j;

         Self->Style.FontChange = TRUE;

         str = Tag->Attrib[i].Value;
         j = 0;
         while ((*str) AND (j < sizeof(Self->Style.Face))) {
            if (*str IS ':') {
               // Point size follows

               str++;
               Self->Style.Point = StrToInt(str);
               while ((*str) AND (*str != ':')) str++;
               if (*str IS ':') {
                  // Style follows

                  str++;
                  if (!StrMatch("bold", str)) {
                     Self->Style.FontChange = TRUE;
                     Self->Style.FontStyle.Options |= FSO_BOLD;
                  }
                  else if (!StrMatch("italic", str)) {
                     Self->Style.FontChange = TRUE;
                     Self->Style.FontStyle.Options |= FSO_ITALIC;
                  }
                  else if (!StrMatch("bold italic", str)) {
                     Self->Style.FontChange = TRUE;
                     Self->Style.FontStyle.Options |= FSO_BOLD|FSO_ITALIC;
                  }
               }
               break;
            }
            else Self->Style.Face[j++] = *str++;
         }
         Self->Style.Face[j] = 0;

         StrCopy(Tag->Attrib[i].Value, Self->Style.Face, sizeof(Self->Style.Face));
      }
      else if (!StrMatch("size", Tag->Attrib[i].Name)) {
         Self->Style.FontChange = TRUE;
         Self->Style.Point = StrToFloat(Tag->Attrib[i].Value);
      }
      else if (!StrMatch("style", Tag->Attrib[i].Name)) {
         if (!StrMatch("bold", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_BOLD;
         }
         else if (!StrMatch("italic", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ITALIC;
         }
         else if (!StrMatch("bold italic", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_BOLD|FSO_ITALIC;
         }
      }
      else if (!StrMatch("preformat", Tag->Attrib[i].Name)) {
         Self->Style.StyleChange = TRUE;
         Self->Style.FontStyle.Options |= FSO_PREFORMAT;
         preformat = TRUE;
         flags |= IPF_STRIPFEEDS;
      }
   }

   parse_tag(Self, XML, Child, Index, flags);

   saved_style_check(Self, &savestatus);

   if (preformat) trim_preformat(Self, Index);
}

//****************************************************************************

static void tag_object(objDocument *Self, CSTRING pagetarget, LONG class_id, struct XMLTag *Template,
  objXML *XML, struct XMLTag *Tag, struct XMLTag *child, LONG *Index,
  LONG Flags, UBYTE *s_revert, UBYTE *e_revert, LONG *b_revert)
{
   struct XMLTag *scan;
   struct Field *field;
   STRING content, argname;
   OBJECTID object_id;
   OBJECTPTR object;
   FIELD field_id;
   LONG i;
   BYTE customised;

   // NF_INTEGRAL is only set when the object is owned by the document

   if (NewLockedObject(class_id, (Self->CurrentObject) ? 0 : NF_INTEGRAL, &object, &object_id)) {
      LogErrorMsg("Failed to create object of class #%d.", class_id);
      return;
   }

   LogBranch("Processing %s object from document tag, owner #%d.", object->Class->ClassName, Self->CurrentObject ? Self->CurrentObject->UniqueID : -1);

   // If the class supports the LayoutStyle field, set it with current style information.

   if ((field = FindField(object, FID_LayoutStyle, NULL))) {
      if (field->Flags & FDF_SYSTEM) set_object_style(Self, object);
   }

   Self->DrawIntercept++;

   // Setup the callback interception so that we can control the order in which objects draw their graphics to the surface.

   if (Self->CurrentObject) {
      SetLong(object, FID_Owner, Self->CurrentObject->UniqueID);
   }
   else if (pagetarget) {
      field_id = StrHash(pagetarget, 0);
      if (Self->BkgdGfx) {
         SetLong(object, field_id, Self->ViewID);
      }
      else SetLong(object, field_id, Self->PageID);
   }

   for (i=1; i < Tag->TotalAttrib; i++) {
      argname = Tag->Attrib[i].Name;
      while (*argname IS '$') argname++;
      if (!Tag->Attrib[i].Value) SetFieldEval(object, argname, "1");
      else SetFieldEval(object, argname, Tag->Attrib[i].Value);
   }

   // Check for the 'data' tag which can be used to send data feed information prior to initialisation.
   //
   // <data type="text">Content</data>
   // <data type="xml" template="TemplateName"/>
   // <data type="xml" object="[xmlobj]"/>
   // <data type="xml">Content</data>

   customised = FALSE;
   if (Tag->Child) {
      STRING type, src, template;

      for (scan=Tag->Child; scan; scan=scan->Next) {
         if (StrMatch("data", scan->Attrib->Name)) continue;

         PTR_RESTORE_ARGS();

         PTR_SAVE_ARGS(scan);

         if (!(type = XMLATTRIB(scan, "type"))) type = "text";

         if (!StrMatch("text", type)) {
            if (!scan->Child) continue;
            if (!xmlGetContent(XML, scan->Child->Index, Self->Temp, Self->TempSize)) {
               acDataText(object, Self->Temp);
            }
         }
         else if (!StrMatch("xml", type)) {
            customised = TRUE;

            if ((template = XMLATTRIB(scan, "template"))) {
               struct XMLTag *tmp;

               for (tmp=Self->Templates->Tags[0]; tmp; tmp=tmp->Next) {
                  for (i=0; i < tmp->TotalAttrib; i++) {
                     if ((!StrMatch("Name", tmp->Attrib[i].Name)) AND
                         (!StrMatch(template, tmp->Attrib[i].Value))) {
                        if (!xmlGetString(Self->Templates, tmp->Child->Index, XMF_INCLUDE_SIBLINGS|XMF_STRIP_CDATA, &content)) {
                           acDataXML(object, content);
                           FreeResource(content);
                        }

                        break;
                     }
                  }
               }
            }
            else if ((src = XMLATTRIB(scan, "object"))) {
               OBJECTID objectid;

               if (!FastFindObject(src, 0, &objectid, 1, 0)) {
                  if ((objectid) AND (valid_objectid(Self, objectid))) {
                     objXML *objxml;
                     if (!AccessObject(objectid, 3000, &objxml)) {
                        if (objxml->Head.ClassID IS ID_XML) {
                           if (!xmlGetString(objxml, 0, XMF_INCLUDE_SIBLINGS|XMF_STRIP_CDATA, &content)) {
                              acDataXML(object, content);
                              FreeResource(content);
                           }
                        }
                        else LogErrorMsg("Cannot extract XML data from a non-XML object.");
                        ReleaseObject(objxml);
                     }
                  }
                  else LogErrorMsg("Invalid object reference '%s'", src);
               }
               else LogErrorMsg("Unable to find object '%s'", src);
            }
            else {
               if (!scan->Child) continue;
               if (!xmlGetString(XML, scan->Child->Index, XMF_INCLUDE_SIBLINGS|XMF_STRIP_CDATA, &content)) {
                  acDataXML(object, content);
                  FreeResource(content);
               }
            }
         }
         else LogErrorMsg("Unsupported data type '%s'", type);
      }
   }

   // Feeds are applied to invoked objects, whereby the object's class name matches a feed.

   if ((!customised) AND (Template)) {
      if (!xmlGetString(Self->Templates, Template->Child->Index, XMF_INCLUDE_SIBLINGS|XMF_STRIP_CDATA, &content)) {
         acDataXML(object, content);
         FreeResource(content);
      }
   }

   if (!acInit(object)) {
      struct escObject escobj;
      OBJECTPTR prevobject;

      if (Self->Invisible) acHide(object); // Hide the object if it's in an invisible section

      ClearMemory(&escobj, sizeof(escobj));

      if (FindField(object, FID_Layout, NULL)) escobj.Graphical = TRUE;
      else if (FindField(object, FID_LayoutSurface, NULL)) escobj.Graphical = TRUE;
      else escobj.Graphical = FALSE;

      // Child tags are processed as normal, but are applied with respect to the object.  Any tags that reflect
      // document content are passed to the object as XML.

      if (Tag->Child) {
         FMSG("~","Processing child tags for object #%d.", object_id);
            prevobject = Self->CurrentObject;
            Self->CurrentObject = object;
            parse_tag(Self, XML, Tag->Child, Index, Flags & (~FILTER_ALL));
            Self->CurrentObject = prevobject;
         LOGRETURN();
      }

      if (child != Tag->Child) {
         FMSG("~","Processing further child tags for object #%d.", object_id);
            prevobject = Self->CurrentObject;
            Self->CurrentObject = object;
            parse_tag(Self, XML, child, Index, Flags & (~FILTER_ALL));
            Self->CurrentObject = prevobject;
         LOGRETURN();
      }

      // The object can self-destruct in ClosingTag(), so check that it still exists before inserting it into the text stream.

      if (!CheckObjectExists(object_id, NULL)) {
         if (Self->BkgdGfx) {
            struct docresource *resource;
            resource = add_resource_id(Self, object_id, RT_OBJECT_UNLOAD);
            if (resource) resource->ClassID = class_id;
         }
         else {
            objLayout *layout;

            escobj.ObjectID = object->UniqueID;
            escobj.ClassID = object->ClassID;
            escobj.Embedded = FALSE;
            if (Self->CurrentObject) escobj.Owned = TRUE;

            // By default objects are assumed to be in the background (thus not embedded as part of the text stream).
            // This section is intended to confirm the graphical state of the object.

            if ((FindField(object, FID_Layout, NULL)) AND (!GetPointer(object, FID_Layout, &layout))) {
               if (layout->Layout & (LAYOUT_BACKGROUND|LAYOUT_FOREGROUND));
               else if (layout->Layout & LAYOUT_EMBEDDED) escobj.Embedded = TRUE;
            }
            else escobj.Embedded = TRUE; // If the layout object is not present, the object is managing its own graphics and likely is embedded (button, combobox, checkbox etc are like this)

            style_check(Self, Index);
            insert_escape(Self, Index, ESC_OBJECT, &escobj, sizeof(escobj));

            struct docresource *resource;
            if (Self->ObjectCache) {
               switch (object->ClassID) {
                  // The following class types can be cached
                  case ID_XML:
                  case ID_FILE:
                  case ID_CONFIG:
                  case ID_COMPRESSION:
                  case ID_SCRIPT:
                  case ID_RSS:
                     resource = add_resource_id(Self, object_id, RT_PERSISTENT_OBJECT);
                     break;

                  // The following class types use their own internal caching system

                  default:
                     LogErrorMsg("Cannot cache object of class type '%s'", ResolveClassID(object->ClassID));
                  case ID_IMAGE:
                     resource = add_resource_id(Self, object_id, RT_OBJECT_UNLOAD);
                     break;
               }
            }
            else resource = add_resource_id(Self, object_id, RT_OBJECT_UNLOAD);

            if (resource) resource->ClassID = class_id;

            // If the object is embedded in the text stream, we will allow whitespace to immediately follow the object.

            if (escobj.Embedded) Self->NoWhitespace = FALSE;

            // Add the object to the tab-list if it is in our list of classes that support keyboard input.

            LONG classes[] = { ID_INPUT, ID_CHECKBOX, ID_COMBOBOX, ID_VIEW };

            for (i=0; i < ARRAYSIZE(classes); i++) {
               if (classes[i] IS class_id) {
                  add_tabfocus(Self, TT_OBJECT, object_id);
                  break;
               }
            }
         }
      }
      else FMSG("insert_child:","Object %d self-destructed.", object_id);
   }
   else {
      acFree(object);
      LogErrorMsg("Failed to initialise object of class $%.8x", class_id);
   }

next: // Used by PTR_SAVE_ARGS()

   Self->DrawIntercept--;

   if (object) ReleaseObject(object);

   LogReturn();
}

//****************************************************************************

static void tag_pre(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
//   insert_paragraph_start(Self, Index, NULL);

   if (!(Self->Style.FontStyle.Options & FSO_PREFORMAT)) {
      struct style_status savestatus;
      savestatus = Self->Style;
      Self->Style.StyleChange = TRUE;
      Self->Style.FontStyle.Options |= FSO_PREFORMAT;
      parse_tag(Self, XML, Child, Index, IPF_STRIPFEEDS);
      saved_style_check(Self, &savestatus);
   }
   else parse_tag(Self, XML, Child, Index, IPF_STRIPFEEDS);

   trim_preformat(Self, Index);

//   insert_paragraph_end(Self, Index);
//   Self->NoWhitespace = TRUE;
}

//****************************************************************************
// By default, a script will be activated when the parser encounters it in the document.  If the script returns a
// result string, that result is assumed to be valid XML and is processed by the parser as such.
//
// If the script contains functions, those functions can be called at any time, either during the parsing process or
// when the document is displayed.
//
// The first script encountered by the parser will serve as the default source for all function calls.  If you need to
// call functions in other scripts then you need to access them by name - e.g. 'myscript.function()'.
//
// Only the first section of content enclosed within the <script> tag (CDATA) is accepted by the script parser.

static void tag_script(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   OBJECTPTR script;
   objXML *xmlinc;
   LONG i;
   STRING tagname;
   ERROR error;

   STRING type = "fluid";
   STRING src = NULL;
   STRING cachefile = NULL;
   STRING name = NULL;
   BYTE defaultscript = FALSE;
   BYTE persistent = FALSE;

   for (i=1; i < Tag->TotalAttrib; i++) {
      tagname = Tag->Attrib[i].Name;
      if (*tagname IS '$') tagname++;
      if (*tagname IS '@') continue; // Variables are set later

      if (!StrMatch("type", tagname)) {
         type = Tag->Attrib[i].Value;
      }
      else if (!StrMatch("persistent", tagname)) {
         // A script that is marked as persistent will survive refreshes
         persistent = TRUE;
      }
      else if (!StrMatch("src", tagname)) {
         if (safe_file_path(Self, Tag->Attrib[i].Value)) {
            src = Tag->Attrib[i].Value;
         }
         else {
            LogErrorMsg("Security violation - cannot set script src to: %s", Tag->Attrib[i].Value);
            return;
         }
      }
      else if (!StrMatch("cachefile", tagname)) {
         // Currently the security risk of specifying a cache file is that you could overwrite files on the user's PC,
         // so for the time being this requires unrestricted mode.

         if (Self->Flags & DCF_UNRESTRICTED) {
            cachefile = Tag->Attrib[i].Value;
         }
         else {
            LogErrorMsg("Security violation - cannot set script cachefile to: %s", Tag->Attrib[i].Value);
            return;
         }
      }
      else if (!StrMatch("name", tagname)) {
         name = Tag->Attrib[i].Value;
      }
      else if (!StrMatch("postprocess", tagname)) {
         LogErrorMsg("--- PostProcess mode for scripts is obsolete - please use the PageProcessed event trigger or call an initialisation function directly ---");
      }
      else if (!StrMatch("default", tagname)) {
         defaultscript = TRUE;
      }
      else if (!StrMatch("external", tagname)) {
         // Reference an external script as the default for function calls
         if (Self->Flags & DCF_UNRESTRICTED) {
            if (!FindPrivateObject(Tag->Attrib[i].Value, &Self->DefaultScript)) {
               return;
            }
            else {
               LogErrorMsg("Failed to find external script '%s'", Tag->Attrib[i].Value);
               return;
            }
         }
         else {
            LogErrorMsg("Security violation - cannot reference external script '%s'", Tag->Attrib[i].Value);
            return;
         }
      }
   }

   if ((persistent) AND (!name)) name = "mainscript";

   if (!src) {
      if ((!Tag->Child) OR (Tag->Child->Attrib->Name) OR (!Tag->Child->Attrib->Value)) {
         // Ignore if script holds no content
         LogErrorMsg("<script/> tag does not contain content.");
         return;
      }
   }

   // If the script is persistent and already exists in the resource cache, do nothing further.

   if (persistent) {
      struct docresource *resource;
      for (resource=Self->Resources; resource; resource=resource->Next) {
         if (resource->Type IS RT_PERSISTENT_SCRIPT) {
            script = GetObjectPtr(resource->ObjectID);
            if (!StrMatch(name, GetName(script))) {
               LogF("tag_script","Persistent script discovered.");
               if ((!Self->DefaultScript) OR (defaultscript)) Self->DefaultScript = script;
               return;
            }
         }
      }
   }

   if (!StrMatch("fluid", type)) {
      error = NewObject(ID_FLUID, NF_INTEGRAL, &script);
   }
   else {
      error = ERR_NoSupport;
      LogErrorMsg("Unsupported script type '%s'", type);
   }

   if (!error) {
      if (name) SetName(script, name);

      if (src) SetString(script, FID_Path, src);
      else {
         if (!xmlGetContent(XML, Tag->Index, Self->Temp, Self->TempSize)) {
            SetString(script, FID_Statement, Self->Temp);
         }
         else {
            acFree(script);
            return;
         }
      }

      if (cachefile) SetField(script, FID_CacheFile, cachefile);

      // Object references are to be limited in scope to the Document object

      //SetLong(script, FID_ObjectScope, Self->Head.UniqueID);

      // Pass custom arguments in the script tag

      for (i=1; i < Tag->TotalAttrib; i++) {
         tagname = Tag->Attrib[i].Name;
         if (*tagname IS '$') tagname++;
         if (*tagname IS '@') {
            acSetVar(script, tagname+1, Tag->Attrib[i].Value);
         }
      }

      if (!(error = acInit(script))) {
         // Pass document arguments to the script

         struct KeyStore *vs;
         if (!GetPointer(script, FID_Variables, &vs)) {
            VarCopy(Self->Vars, vs);
            VarCopy(Self->Params, vs);
         }

         if (!(error = acActivate(script))) {
            // Persistent scripts survive refreshes.

            add_resource_id(Self, script->UniqueID, (persistent) ? RT_PERSISTENT_SCRIPT : RT_OBJECT_UNLOAD_DELAY);

            if ((!Self->DefaultScript) OR (defaultscript)) {
               LogMsg("Script #%d is the default script for this document.", script->UniqueID);
               Self->DefaultScript = script;
            }

            // Any results returned from the script are processed as XML

            CSTRING *results;
            LONG size;
            if ((!GetFieldArray(script, FID_Results, &results, &size)) AND (size > 0)) {
               if (!CreateObject(ID_XML, 0, &xmlinc,
                     FID_Statement|TSTR, results[0],
                     FID_Flags|TLONG,    XMF_PARSE_HTML|XMF_STRIP_HEADERS,
                     TAGEND)) {

                  parse_tag(Self, xmlinc, xmlinc->Tags[0], Index, Flags);

                  // Add the created XML object to the document rather than destroying it

                  add_resource_id(Self, xmlinc->Head.UniqueID, RT_OBJECT_TEMP);
               }
            }
         }
         else acFree(script);
      }
      else acFree(script);
   }
}

//****************************************************************************
// Similar to <font/>, but the original font state is never saved and restored.

static void tag_setfont(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i;

   for (i=1; i < Tag->TotalAttrib; i++) {
      ULONG hash_attrib = StrHash(Tag->Attrib[i].Name, 0);
      if (hash_attrib IS HASH_colour) {
         Self->Style.StyleChange = TRUE;
         StrToColour(Tag->Attrib[i].Value, &Self->Style.FontStyle.Colour);
      }
      else if (hash_attrib IS HASH_face) {
         Self->Style.FontChange = TRUE;
         StrCopy(Tag->Attrib[i].Value, Self->Style.Face, sizeof(Self->Style.Face));
      }
      else if (hash_attrib IS HASH_size) {
         Self->Style.FontChange = TRUE;
         Self->Style.Point = StrToFloat(Tag->Attrib[i].Value);
      }
      else if (hash_attrib IS HASH_style) {
         if (!StrMatch("bold", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_BOLD;
         }
         else if (!StrMatch("italic", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_ITALIC;
         }
         else if (!StrMatch("bold italic", Tag->Attrib[i].Value)) {
            Self->Style.FontChange = TRUE;
            Self->Style.FontStyle.Options |= FSO_BOLD|FSO_ITALIC;
         }
      }
      else if (hash_attrib IS HASH_preformat) {
         Self->Style.StyleChange = TRUE;
         Self->Style.FontStyle.Options |= FSO_PREFORMAT;
      }
   }

   //style_check(Self, Index);
}

//****************************************************************************

static void tag_setmargins(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   escSetMargins margins;
   LONG i;

   ClearMemory(&margins, sizeof(margins));

   margins.Top    = 0x7fff;
   margins.Left   = 0x7fff;
   margins.Right  = 0x7fff;
   margins.Bottom = 0x7fff;

   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("top", Tag->Attrib[i].Name)) {
         margins.Top = StrToInt(Tag->Attrib[i].Value);
         if (margins.Top < -4000) margins.Top = -4000;
         else if (margins.Top > 4000) margins.Top = 4000;
      }
      else if (!StrMatch("bottom", Tag->Attrib[i].Name)) {
         margins.Bottom = StrToInt(Tag->Attrib[i].Value);
         if (margins.Bottom < -4000) margins.Bottom = -4000;
         else if (margins.Bottom > 4000) margins.Bottom = 4000;
      }
      else if (!StrMatch("right", Tag->Attrib[i].Name)) {
         margins.Right = StrToInt(Tag->Attrib[i].Value);
         if (margins.Right < -4000) margins.Right = -4000;
         else if (margins.Right > 4000) margins.Right = 4000;
      }
      else if (!StrMatch("left", Tag->Attrib[i].Name)) {
         margins.Left = StrToInt(Tag->Attrib[i].Value);
         if (margins.Left < -4000) margins.Left = -4000;
         else if (margins.Left > 4000) margins.Left = 4000;
      }
      else if (!StrMatch("all", Tag->Attrib[i].Name)) {
         LONG value;
         value = StrToInt(Tag->Attrib[i].Value);
         if (value < -4000) value = -4000;
         else if (value > 4000) value = 4000;
         margins.Left = margins.Top = margins.Right = margins.Bottom = value;
      }
   }

   insert_escape(Self, Index, ESC_SETMARGINS, &margins, sizeof(margins));
}

//****************************************************************************

static void tag_savestyle(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   //style_check(Self, Index);
   Self->RestoreStyle = Self->Style; // Save the current style
}

//****************************************************************************

static void tag_restorestyle(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   Self->Style = Self->RestoreStyle; // Restore the saved style
   Self->Style.FontChange = TRUE;
   //style_check(Self, Index);
}

//****************************************************************************

static void tag_italic(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (!(Self->Style.FontStyle.Options & FSO_ITALIC)) {
      struct style_status savestatus;
      savestatus = Self->Style;
      Self->Style.FontChange = TRUE; // Italic fonts are typically a different typeset
      Self->Style.FontStyle.Options |= FSO_ITALIC;
      parse_tag(Self, XML, Child, Index, 0);
      saved_style_check(Self, &savestatus);
   }
   else parse_tag(Self, XML, Child, Index, 0);
}

//****************************************************************************

static void tag_li(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (!Self->Style.List) {
      LogErrorMsg("<li> not used inside a <list> tag.");
      return;
   }

   struct escParagraph para;
   ClearMemory(&para, sizeof(para));
   para.ListItem     = TRUE;
   para.LeadingRatio = 0;
   para.VSpacing     = Self->Style.List->VSpacing;
   para.BlockIndent  = Self->Style.List->BlockIndent;
   para.ItemIndent   = Self->Style.List->ItemIndent;

   STRING value = NULL;

   LONG i;
   for (i=1; i < Tag->TotalAttrib; i++) {
      STRING tagname = Tag->Attrib[i].Name;
      if (*tagname IS '$') tagname++;

      if (!StrMatch("value", tagname)) {
         value = Tag->Attrib[i].Value;
      }
      else if (!StrMatch("leading", tagname)) {
         para.LeadingRatio = StrToFloat(Tag->Attrib[i].Value);
         if (para.LeadingRatio < MIN_LEADING) para.LeadingRatio = MIN_LEADING;
         else if (para.LeadingRatio > MAX_LEADING) para.LeadingRatio = MAX_LEADING;
      }
      else if (!StrMatch("vspacing", tagname)) {
         para.VSpacing = StrToFloat(Tag->Attrib[i].Value);
         if (para.VSpacing < MIN_LEADING) para.VSpacing = MIN_LEADING;
         else if (para.VSpacing > MAX_VSPACING) para.VSpacing = MAX_VSPACING;
      }
   }

   if ((Self->Style.List->Type IS LT_CUSTOM) AND (value) AND (*value)) {
      style_check(Self, Index); // Font changes must take place prior to the printing of custom string items

      LONG len = sizeof(escParagraph) + StrLength(value) + 1;
      UBYTE buffer[len];

      para.CustomString = TRUE;

      CopyMemory(&para, buffer, sizeof(para));
      StrCopy(value, (STRING)(buffer + sizeof(escParagraph)), COPY_ALL);

      insert_escape(Self, Index, ESC_PARAGRAPH_START, buffer, len);

         parse_tag(Self, XML, Child, Index, 0);

      insert_paragraph_end(Self, Index);
   }
   else if (Self->Style.List->Type IS LT_ORDERED) {
      style_check(Self, Index); // Font changes must take place prior to the printing of custom string items

      i = IntToStr(Self->Style.List->ItemNum, Self->Style.List->Buffer + Self->Style.List->OrderInsert, LIST_BUFFER_SIZE - Self->Style.List->OrderInsert - 1);
      i += Self->Style.List->OrderInsert;
      if (i < LIST_BUFFER_SIZE - 2) {
         Self->Style.List->Buffer[i++] = '.';
         Self->Style.List->Buffer[i] = 0;
      }

      LONG save_insert = Self->Style.List->OrderInsert;
      Self->Style.List->OrderInsert = i;

      LONG save_item = Self->Style.List->ItemNum;
      Self->Style.List->ItemNum = 1;

      LONG len = sizeof(escParagraph) + StrLength(Self->Style.List->Buffer) + 1;
      UBYTE buffer[len];

      para.CustomString = TRUE;
      CopyMemory(&para, buffer, sizeof(para));
      StrCopy(Self->Style.List->Buffer, (STRING)(buffer + sizeof(escParagraph)), COPY_ALL);

      insert_escape(Self, Index, ESC_PARAGRAPH_START, buffer, len);
         parse_tag(Self, XML, Child, Index, 0);
      insert_paragraph_end(Self, Index);

      Self->Style.List->OrderInsert = save_insert;
      Self->Style.List->ItemNum = save_item + 1;
   }
   else {
      insert_paragraph_start(Self, Index, &para);
         parse_tag(Self, XML, Child, Index, 0);
      insert_paragraph_end(Self, Index);
   }
}

//****************************************************************************

static void tag_underline(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (!(Self->Style.FontStyle.Options & FSO_UNDERLINE)) {
      struct style_status savestatus;
      savestatus = Self->Style;
      Self->Style.StyleChange = TRUE;
      Self->Style.FontStyle.Options |= FSO_UNDERLINE;
      parse_tag(Self, XML, Child, Index, 0);
      saved_style_check(Self, &savestatus);
   }
   else parse_tag(Self, XML, Child, Index, Flags & (~FILTER_ALL));
}

//****************************************************************************

static void tag_repeat(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG loopstart = 0;
   LONG loopend = 0;
   LONG count = 0;
   LONG step  = 0;
   STRING indexname = NULL;

   LONG i;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("start", Tag->Attrib[i].Name)) {
         loopstart = StrToInt(Tag->Attrib[i].Value);
         if (loopstart < 0) loopstart = 0;
      }
      else if (!StrMatch("count", Tag->Attrib[i].Name)) {
         count = StrToInt(Tag->Attrib[i].Value);
         if (count < 0) {
            LogErrorMsg("Invalid count value of %d", count);
            return;
         }
      }
      else if (!StrMatch("end", Tag->Attrib[i].Name)) {
         loopend = StrToInt(Tag->Attrib[i].Value) + 1;
      }
      else if (!StrMatch("step", Tag->Attrib[i].Name)) {
         step = StrToInt(Tag->Attrib[i].Value);
      }
      else if (!StrMatch("index", Tag->Attrib[i].Name)) {
         // If an index name is specified, the programmer will need to refer to it as [@indexname] and [%index] will
         // remain unchanged from any parent repeat loop.

         indexname = Tag->Attrib[i].Value;
      }
   }

   if (!step) {
      if (loopend < loopstart) step = -1;
      else step = 1;
   }

   // Validation - ensure that it will be possible for the repeat loop to execute correctly without the chance of
   // infinite looping.
   //
   // If the user set both count and end attributes, the count attribute will be given the priority here.

   if (count > 0) loopend = loopstart + (count * step);

   if (step > 0) {
      if (loopend < loopstart) step = -step;
   }
   else if (loopend > loopstart) step = -step;

   FMSG("~insert_child:","Performing a repeat loop (start: %d, end: %d, step: %d).", loopstart, loopend, step);

   LONG saveindex = Self->LoopIndex;

   while (loopstart < loopend) {
      if (!indexname) {
         Self->LoopIndex = loopstart;
      }
      else {
         UBYTE intstr[20];
         IntToStr(loopstart, intstr, sizeof(intstr));
         SetVar(Self, indexname, intstr);
      }

      struct XMLTag *xmlchild = Tag->Child;
      parse_tag(Self, XML, xmlchild, Index, Flags);

      loopstart += step;
   }

   if (!indexname) Self->LoopIndex = saveindex;

   LOGRETURN();
   FMSG("insert_child:","Repeat loop ends.");
}

//****************************************************************************
// <table columns="10%,90%" width="100" height="100" colour="#808080">
//  <row><cell>Activate<brk/>This activates the object.</cell></row>
//  <row><cell span="2">Reset</cell></row>
// </table>
//
// <table width="100" height="100" colour="#808080">
//  <cell>Activate</cell><cell>This activates the object.</cell>
//  <cell colspan="2">Reset</cell>
// </table>
//
// Columns:      The minimum width of each column in the table.
// Width/Height: Minimum width and height of the table.
// Colour:       Background colour for the table.
// Border:       Border colour for the table (see thickness).
// Thickness:    Thickness of the border colour.
//
// The only acceptable child tags inside a <table> section are row, brk and cell tags.  Command tags are acceptable
// (repeat, if statements, etc).  The table byte code is typically generated as ESC_TABLE_START, ESC_ROW, ESC_CELL...,
// ESC_ROW_END, ESC_TABLE_END.

static void tag_table(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct escTable start, *table;
   struct process_table var, *savevar;
   STRING columns, *list;
   LONG table_index, i, j, k;

   ClearMemory(&start, sizeof(start));
   start.MinWidth  = 1;
   start.MinHeight = 1;

   columns = NULL;
   for (i=1; i < Tag->TotalAttrib; i++) {
      ULONG hash_attrib = StrHash(Tag->Attrib[i].Name, 0);
      if (hash_attrib IS HASH_columns) {
         // Column preferences are processed only when the end of the table marker has been reached.
         columns = Tag->Attrib[i].Value;
      }
      else if (hash_attrib IS HASH_width) {
         start.MinWidth = StrToInt(Tag->Attrib[i].Value);
         start.WidthPercent = FALSE;
         for (j=0; Tag->Attrib[i].Value[j]; j++) {
            if (Tag->Attrib[i].Value[j] IS '%') {
               start.WidthPercent = TRUE;
               break;
            }
         }
         if (start.MinWidth < 1) start.MinWidth = 1;
         else if (start.MinWidth > 10000) start.MinWidth = 10000;
      }
      else if (hash_attrib IS HASH_height) {
         start.MinHeight = StrToInt(Tag->Attrib[i].Value);
         for (j=0; Tag->Attrib[i].Value[j]; j++) {
            if (Tag->Attrib[i].Value[j] IS '%') {
               start.HeightPercent = TRUE;
               break;
            }
         }
         if (start.MinHeight < 1) start.MinHeight = 1;
         else if (start.MinHeight > 10000) start.MinHeight = 10000;
      }
      else if (hash_attrib IS HASH_colour) {
         StrToColour(Tag->Attrib[i].Value, &start.Colour);
      }
      else if (hash_attrib IS HASH_border) {
         StrToColour(Tag->Attrib[i].Value, &start.Highlight);
         StrToColour(Tag->Attrib[i].Value, &start.Shadow);
         if (start.Thickness < 1) start.Thickness = 1;
      }
      else if (hash_attrib IS HASH_highlight) {
         StrToColour(Tag->Attrib[i].Value, &start.Highlight);
         if (start.Thickness < 1) start.Thickness = 1;
      }
      else if (hash_attrib IS HASH_shadow) {
         StrToColour(Tag->Attrib[i].Value, &start.Shadow);
         if (start.Thickness < 1) start.Thickness = 1;
      }
      else if (hash_attrib IS HASH_spacing) { // Spacing between the cells
         start.CellVSpacing = StrToInt(Tag->Attrib[i].Value);
         if (start.CellVSpacing < 0) start.CellVSpacing = 0;
         else if (start.CellVSpacing > 200) start.CellVSpacing = 200;
         start.CellHSpacing = start.CellVSpacing;
      }
      else if (hash_attrib IS HASH_thin) { // Thin tables do not have spacing (defined by 'spacing' or 'hspacing') on the sides
         start.Thin = TRUE;
      }
      else if (hash_attrib IS HASH_vspacing) { // Spacing between the cells
         start.CellVSpacing = StrToInt(Tag->Attrib[i].Value);
         if (start.CellVSpacing < 0) start.CellVSpacing = 0;
         else if (start.CellVSpacing > 200) start.CellVSpacing = 200;
      }
      else if (hash_attrib IS HASH_hspacing) { // Spacing between the cells
         start.CellHSpacing = StrToInt(Tag->Attrib[i].Value);
         if (start.CellHSpacing < 0) start.CellHSpacing = 0;
         else if (start.CellHSpacing > 200) start.CellHSpacing = 200;
      }
      else if ((hash_attrib IS HASH_margins) OR (hash_attrib IS HASH_padding)) { // Padding inside the cells
         start.CellPadding = StrToInt(Tag->Attrib[i].Value);
         if (start.CellPadding < 0) start.CellPadding = 0;
         else if (start.CellPadding > 200) start.CellPadding = 200;
      }
      else if (hash_attrib IS HASH_thickness) {
         j = StrToInt(Tag->Attrib[i].Value);
         if (j < 0) j = 0;
         else if (j > 255) j = 255;
         start.Thickness = j;
      }
   }

   table_index = *Index;
   insert_escape(Self, Index, ESC_TABLE_START, &start, sizeof(start));

   savevar = Self->Style.Table;
   Self->Style.Table = &var;
   Self->Style.Table->escTable = &start;

      parse_tag(Self, XML, Tag->Child, Index, IPF_NOCONTENT|FILTER_TABLE);

   Self->Style.Table = savevar;

   table = (struct escTable *)(Self->Stream + table_index + ESC_LEN_START);
   CopyMemory(&start, table, sizeof(start));

   if (!AllocMemory(sizeof(struct tablecol) * table->TotalColumns, MEM_DATA, &table->Columns, NULL)) {
      if (columns) {
         // The columns value, if supplied is arranged as a CSV list of column widths

         char coltemp[1024];

         StrCopy(columns, coltemp, sizeof(coltemp));
         if ((list = StrBuildArray(coltemp, 0, 0, SBF_CSV))) {
            for (i=0; (i < table->TotalColumns) AND (list[i]); i++) {
               table->Columns[i].PresetWidth = StrToInt(list[i]);
               for (k=0; list[i][k]; k++) {
                  if (list[i][k] IS '%') table->Columns[i].PresetWidth |= 0x8000;
               }
            }

            if (i < table->TotalColumns) LogF("@tag_table","Warning - columns attribute '%s' did not define %d columns.", columns, table->TotalColumns);

            FreeResource(list);
         }
         else LogF("@tag_table","Failed to build array from columns value: %s", columns);
      }

      add_resource_ptr(Self, table->Columns, RT_MEMORY);
   }

   insert_escape(Self, Index, ESC_TABLE_END, 0, 0);
   //style_check(Self, Index);
   //Self->Style.StyleChange = FALSE;

   Self->NoWhitespace = TRUE; // Setting this to TRUE will prevent the possibility of blank spaces immediately following the table.
}

//****************************************************************************

static void tag_row(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct escRow escrow;
   LONG i;

   if (!Self->Style.Table) {
      LogErrorMsg("<row> not defined inside <table> section.");
      Self->Error = ERR_InvalidData;
      return;
   }

   ClearMemory(&escrow, sizeof(escrow));
   escrow.Stack     = NULL;
   escrow.RowHeight = 0;
   escrow.MinHeight = 0;
   escrow.Colour.Alpha = 0;
   escrow.Shadow.Alpha = 0;
   escrow.Highlight.Alpha = 0;

   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("height", Tag->Attrib[i].Name)) {
         escrow.MinHeight = StrToInt(Tag->Attrib[i].Value);
         if (escrow.MinHeight < 0) escrow.MinHeight = 0;
         else if (escrow.MinHeight > 4000) escrow.MinHeight = 4000;
      }
      else if (!StrMatch("colour", Tag->Attrib[i].Name))    StrToColour(Tag->Attrib[i].Value, &escrow.Colour);
      else if (!StrMatch("highlight", Tag->Attrib[i].Name)) StrToColour(Tag->Attrib[i].Value, &escrow.Highlight);
      else if (!StrMatch("shadow", Tag->Attrib[i].Name))    StrToColour(Tag->Attrib[i].Value, &escrow.Shadow);
      else if (!StrMatch("border", Tag->Attrib[i].Name)) {
         StrToColour(Tag->Attrib[i].Value, &escrow.Highlight);
         escrow.Shadow = escrow.Highlight;
      }
   }

   insert_escape(Self, Index, ESC_ROW, &escrow, sizeof(escrow));
   Self->Style.Table->escTable->Rows++;
   Self->Style.Table->RowCol = 0;

   if (Child) {
      parse_tag(Self, XML, Child, Index, IPF_NOCONTENT|FILTER_ROW);
   }

   insert_escape(Self, Index, ESC_ROW_END, NULL, 0);

   if (Self->Style.Table->RowCol > Self->Style.Table->escTable->TotalColumns) {
      Self->Style.Table->escTable->TotalColumns = Self->Style.Table->RowCol;
   }
}

//****************************************************************************

static void tag_cell(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   struct {
      struct escCell cell;
      UBYTE buffer[200];
   } esc;
   struct style_status savestatus;
   LONG cell_index, i, offset, totalargs, argsize;
   BYTE preformat, select;
   static UBYTE edit_recurse = 0;

   if (!Self->Style.Table) {
      LogErrorMsg("<cell> not defined inside <table> section.");
      Self->Error = ERR_InvalidData;
      return;
   }

   ClearMemory(&esc.cell, sizeof(esc.cell));
   esc.cell.ColSpan   = 1;
   esc.cell.RowSpan   = 1;
   esc.cell.Column    = Self->Style.Table->RowCol;
   esc.cell.CellID    = Self->UniqueID++;

   offset = 0;
   totalargs = 0;
   argsize = 0;
   select = FALSE;

   for (i=1; i < Tag->TotalAttrib; i++) {
      ULONG hash_attrib = StrHash(Tag->Attrib[i].Name, 0);
      if (hash_attrib IS HASH_colspan) {
         esc.cell.ColSpan = StrToInt(Tag->Attrib[i].Value);
         if (esc.cell.ColSpan < 1) esc.cell.ColSpan = 1;
         else if (esc.cell.ColSpan > 1000) esc.cell.ColSpan = 1000;
      }
      else if (hash_attrib IS HASH_rowspan) {
         esc.cell.RowSpan = StrToInt(Tag->Attrib[i].Value);
         if (esc.cell.RowSpan < 1) esc.cell.RowSpan = 1;
         else if (esc.cell.RowSpan > 1000) esc.cell.RowSpan = 1000;
      }
      else if (hash_attrib IS HASH_edit) {
         if (edit_recurse) {
            LogErrorMsg("Edit cells cannot be embedded recursively.");
            Self->Error = ERR_Recursion;
            return;
         }
         esc.cell.EditHash = StrHash(Tag->Attrib[i].Value, 0);

         // Check that the given name matches to an actual edit definition

         struct DocEdit *def;
         for (def=Self->EditDefs; def; def=def->Next) {
            if (def->NameHash IS esc.cell.EditHash) break;
         }
         if (!def) {
            LogErrorMsg("Edit definition '%s' does not exist.", Tag->Attrib[i].Value);
            esc.cell.EditHash = 0;
         }
      }
      else if (hash_attrib IS HASH_select) {
         select = TRUE;
      }
      else if (hash_attrib IS HASH_colour) {
         StrToColour(Tag->Attrib[i].Value, &esc.cell.Colour);
      }
      else if (hash_attrib IS HASH_highlight) {
         StrToColour(Tag->Attrib[i].Value, &esc.cell.Highlight);
      }
      else if (hash_attrib IS HASH_shadow) {
         StrToColour(Tag->Attrib[i].Value, &esc.cell.Shadow);
      }
      else if (hash_attrib IS HASH_border) {
         StrToColour(Tag->Attrib[i].Value, &esc.cell.Highlight);
         StrToColour(Tag->Attrib[i].Value, &esc.cell.Shadow);
      }
      else if (hash_attrib IS HASH_nowrap) {
         Self->Style.StyleChange = TRUE;
         Self->Style.FontStyle.Options |= FSO_NO_WRAP;
      }
      else if (hash_attrib IS HASH_onclick) {
         if ((Tag->Attrib[i].Value) AND (Tag->Attrib[i].Value[0]) AND (!esc.cell.OnClick)) {
            LONG len = StrLength(Tag->Attrib[i].Value) + 1;
            if (len < sizeof(esc.buffer)-offset) {
               esc.cell.OnClick = sizeof(esc.cell) + offset;
               CopyMemory(Tag->Attrib[i].Value, esc.buffer+offset, len);
               offset += len;
            }
            else {
               Self->Error = ERR_BufferOverflow;
               edit_recurse--;
               return;
            }
         }
      }
      else if (Tag->Attrib[i].Name[0] IS '@') {
         if ((totalargs < 32) AND (argsize < 4096)) {
            totalargs++;
            argsize += StrLength(Tag->Attrib[i].Name) - 1 + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
      else if (Tag->Attrib[i].Name[0] IS '_') {
         if ((totalargs < 32) AND (argsize < 4096)) {
            totalargs++;
            argsize += StrLength(Tag->Attrib[i].Name) + StrLength(Tag->Attrib[i].Value) + 2;
         }
         else LogErrorMsg("No of args or arg size limit exceeded in a <a|link>.");
      }
   }

   if (esc.cell.EditHash) edit_recurse++;

   // Edit sections enforce preformatting, which means that all whitespace entered by the user
   // will be taken into account.  The following check sets FSO_PREFORMAT if it hasn't been set already.

   cell_index = *Index;

   if (totalargs) {
      UBYTE buffer[sizeof(esc.cell) + offset + argsize];
      LONG pos, count;

      esc.cell.TotalArgs = totalargs;
      esc.cell.Args = sizeof(esc.cell) + offset;
      CopyMemory(&esc.cell, buffer, sizeof(esc.cell) + offset);

      pos = sizeof(esc.cell) + offset;
      count = 0;
      for (i=1; i < Tag->TotalAttrib; i++) {
         if ((Tag->Attrib[i].Name) AND (Tag->Attrib[i].Name[0] IS '@')) {
            count++;
            pos += StrCopy(Tag->Attrib[i].Name+1, buffer+pos, COPY_ALL) + 1;
            pos += StrCopy(Tag->Attrib[i].Value, buffer+pos, COPY_ALL) + 1;
            if (count >= totalargs) break;
         }
         else if ((Tag->Attrib[i].Name) AND (Tag->Attrib[i].Name[0] IS '_')) {
            count++;
            pos += StrCopy(Tag->Attrib[i].Name, buffer+pos, COPY_ALL) + 1;
            pos += StrCopy(Tag->Attrib[i].Value, buffer+pos, COPY_ALL) + 1;
            if (count >= totalargs) break;
         }
      }

      insert_escape(Self, Index, ESC_CELL, buffer, pos);
   }
   else insert_escape(Self, Index, ESC_CELL, &esc, sizeof(esc.cell)+offset);

   if (Child) {
      Self->NoWhitespace = TRUE; // Reset whitespace flag: FALSE allows whitespace at the start of the cell, TRUE prevents whitespace

      if ((esc.cell.EditHash) AND (!(Self->Style.FontStyle.Options & FSO_PREFORMAT))) {
         savestatus = Self->Style;
         Self->Style.StyleChange = TRUE;
         Self->Style.FontStyle.Options |= FSO_PREFORMAT;
         preformat = TRUE;
      }
      else preformat = FALSE;

      parse_tag(Self, XML, Child, Index, Flags & (~(IPF_NOCONTENT|FILTER_ALL)));

      if (preformat) saved_style_check(Self, &savestatus);
   }

   Self->Style.Table->RowCol += esc.cell.ColSpan;

   struct escCellEnd esccell_end;
   esccell_end.CellID = esc.cell.CellID;
   insert_escape(Self, Index, ESC_CELL_END, &esccell_end, sizeof(esccell_end));

   if (esc.cell.EditHash) {
      // Links are added to the list of tabbable points

      LONG tab = add_tabfocus(Self, TT_EDIT, esc.cell.CellID);
      if (select) Self->FocusIndex = tab;
   }

   if (esc.cell.EditHash) edit_recurse--;
}

//****************************************************************************
// This instruction can only be used from within a template.

static void tag_inject(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   if (Self->InTemplate) {
      if (Self->InjectTag) {
         parse_tag(Self, Self->InjectXML, Self->InjectTag, Index, Flags);
      }
   }
   else LogErrorMsg("<inject/> request detected but not used inside a template.");
}

//****************************************************************************
// No response is required for page tags, but we can check for validity.

static void tag_page(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   STRING name, str;

   if ((name = str = XMLATTRIB(Tag, "name"))) {
      while (*str) {
         if (((*str >= 'A') AND (*str <= 'Z')) OR
             ((*str >= 'a') AND (*str <= 'z')) OR
             ((*str >= '0') AND (*str <= '9'))) {
            // Character is valid
         }
         else {
            LogErrorMsg("Page has an invalid name of '%s'.  Character support is limited to [A-Z,a-z,0-9].", name);
            break;
         }
         str++;
      }
   }
}

/*****************************************************************************
** Usage: <trigger event="resize" function="script.function"/>
*/

static void tag_trigger(objDocument *Self, objXML *XML, struct XMLTag *Tag, struct XMLTag *Child, LONG *Index, LONG Flags)
{
   LONG i, trigger_code;
   STRING event;
   ULONG event_hash;
   OBJECTPTR script;
   CSTRING function_name;
   LARGE function_id;

   event = NULL;
   event_hash = 0;
   function_name = NULL;
   for (i=1; i < Tag->TotalAttrib; i++) {
      if (!StrMatch("event", Tag->Attrib[i].Name)) {
         event = Tag->Attrib[i].Value;
         event_hash = StrHash(Tag->Attrib[i].Value, 0);
      }
      else if (!StrMatch("function", Tag->Attrib[i].Name)) {
         function_name = Tag->Attrib[i].Value;
      }
   }

   if ((event_hash) AND (function_name)) {

      // These are described in the documentation for the AddListener method

      switch(event_hash) {
         case HASH_AfterLayout:       trigger_code = DRT_AFTER_LAYOUT; break;
         case HASH_BeforeLayout:      trigger_code = DRT_BEFORE_LAYOUT; break;
         case HASH_UserClick:         trigger_code = DRT_USER_CLICK; break;
         case HASH_UserClickRelease:  trigger_code = DRT_USER_CLICK_RELEASE; break;
         case HASH_UserMovement:      trigger_code = DRT_USER_MOVEMENT; break;
         case HASH_Refresh:           trigger_code = DRT_REFRESH; break;
         case HASH_GotFocus:          trigger_code = DRT_GOT_FOCUS; break;
         case HASH_LostFocus:         trigger_code = DRT_LOST_FOCUS; break;
         case HASH_LeavingPage:       trigger_code = DRT_LEAVING_PAGE; break;
         case HASH_PageProcessed:     trigger_code = DRT_PAGE_PROCESSED; break;
         default:
            LogErrorMsg("Trigger event '%s' for function '%s' is not recognised.", event, function_name);
            return;
      }

      // Get the script

      if (!extract_script(Self, function_name, &script, &function_name, NULL)) {
         if (!scGetProcedureID(script, function_name, &function_id)) {
            struct DocTrigger *trigger;
            if (!AllocMemory(sizeof(struct DocTrigger), MEM_DATA|MEM_NO_CLEAR, &trigger, NULL)) {
               SET_FUNCTION_SCRIPT(trigger->Function, script, function_id);
               trigger->Next = Self->Triggers[trigger_code];
               Self->Triggers[trigger_code] = trigger;
            }
            else FuncError(ERR_AllocMemory);
         }
         else LogF("@tag_trigger","Unable to resolve '%s' in script #%d to a function ID (the procedure may not exist)", function_name, script->UniqueID);
      }
      else LogF("@tag_trigger","The script for '%s' is not available - check if it is declared prior to the trigger tag.", function_name);
   }
}

//****************************************************************************

static void insert_paragraph_start(objDocument *Self, LONG *Index, escParagraph *Esc)
{
   struct escParagraph var;

   if (!Esc) {
      ClearMemory(&var, sizeof(var));
      Esc = &var;
   }

   insert_escape(Self, Index, ESC_PARAGRAPH_START, Esc, sizeof(escParagraph));
}

//****************************************************************************
// This function inserts a paragraph into a text stream, with the addition of some checking to ensure that multiple
// line breaks are avoided.

static void insert_paragraph_end(objDocument *Self, LONG *Index)
{
   insert_escape(Self, Index, ESC_PARAGRAPH_END, NULL, 0);
   Self->NoWhitespace = TRUE; // TRUE: Prevents whitespace
}

//****************************************************************************
// TAG_OBJECTOK: Indicates that the tag can be used inside an object section, e.g. <image>.<this_tag_ok/>..</image>
// FILTER_TABLE: The tag is restricted to use within <table> sections.
// FILTER_ROW:   The tag is restricted to use within <row> sections.

static struct tagroutine glTags[] = {
   // Place content related tags in this section (tags that affect text, the page layout etc)
   { HASH_a,             tag_link,         TAG_CHILDREN|TAG_CONTENT },
   { HASH_link,          tag_link,         TAG_CHILDREN|TAG_CONTENT },
   { HASH_blockquote,    tag_indent,       TAG_CHILDREN|TAG_CONTENT|TAG_PARAGRAPH },
   { HASH_b,             tag_bold,         TAG_CHILDREN|TAG_CONTENT },
   { HASH_caps,          tag_caps,         TAG_CHILDREN|TAG_CONTENT },
   { HASH_div,           tag_div,          TAG_CHILDREN|TAG_CONTENT|TAG_PARAGRAPH },
   { HASH_p,             tag_paragraph,    TAG_CHILDREN|TAG_CONTENT|TAG_PARAGRAPH },
   { HASH_font,          tag_font,         TAG_CHILDREN|TAG_CONTENT },
   { HASH_i,             tag_italic,       TAG_CHILDREN|TAG_CONTENT },
   { HASH_li,            tag_li,           TAG_CHILDREN|TAG_CONTENT },
   { HASH_pre,           tag_pre,          TAG_CHILDREN|TAG_CONTENT },
   { HASH_indent,        tag_indent,       TAG_CHILDREN|TAG_CONTENT|TAG_PARAGRAPH },
   { HASH_u,             tag_underline,    TAG_CHILDREN|TAG_CONTENT },
   { HASH_list,          tag_list,         TAG_CHILDREN|TAG_CONTENT|TAG_PARAGRAPH },
   { HASH_advance,       tag_advance,      TAG_CONTENT },
   { HASH_br,            tag_br,           TAG_CONTENT },
   // Conditional command tags
   { HASH_else,          NULL,             TAG_CONDITIONAL },
   { HASH_elseif,        NULL,             TAG_CONDITIONAL },
   { HASH_repeat,        tag_repeat,       TAG_CHILDREN|TAG_CONDITIONAL },
   // Special instructions
   { HASH_cache,         tag_cache,        TAG_INSTRUCTION },
   { HASH_call,          tag_call,         TAG_INSTRUCTION },
   { HASH_debug,         tag_debug,        TAG_INSTRUCTION },
   { HASH_focus,         tag_focus,        TAG_INSTRUCTION|TAG_OBJECTOK },
   { HASH_include,       tag_include,      TAG_INSTRUCTION|TAG_OBJECTOK },
   { HASH_print,         tag_print,        TAG_INSTRUCTION|TAG_OBJECTOK },
   { HASH_parse,         tag_parse,        TAG_INSTRUCTION|TAG_OBJECTOK },
   { HASH_set,           tag_set,          TAG_INSTRUCTION|TAG_OBJECTOK },
   { HASH_trigger,       tag_trigger,      TAG_INSTRUCTION },
   // Root level tags
   { HASH_page,          tag_page,         TAG_CHILDREN|TAG_ROOT },
   // Others
   { HASH_background,    tag_background,   0 },
   { HASH_data,          NULL,             0 },
   { HASH_editdef,       tag_editdef,      0 },
   { HASH_footer,        tag_footer,       0 },
   { HASH_head,          tag_head,         0 }, // Synonym for info
   { HASH_header,        tag_header,       0 },
   { HASH_info,          tag_head,         0 },
   { HASH_inject,        tag_inject,       TAG_OBJECTOK },
   { HASH_row,           tag_row,          TAG_CHILDREN|FILTER_TABLE },
   { HASH_cell,          tag_cell,         TAG_PARAGRAPH|FILTER_ROW },
   { HASH_table,         tag_table,        TAG_CHILDREN },
   { HASH_td,            tag_cell,         TAG_CHILDREN|FILTER_ROW },
   { HASH_tr,            tag_row,          TAG_CHILDREN },
   { HASH_body,          tag_body,         0 },
   { HASH_index,         tag_index,        0 },
   { HASH_setmargins,    tag_setmargins,   TAG_OBJECTOK },
   { HASH_setfont,       tag_setfont,      TAG_OBJECTOK },
   { HASH_restorestyle,  tag_restorestyle, TAG_OBJECTOK },
   { HASH_savestyle,     tag_savestyle,    TAG_OBJECTOK },
   { HASH_script,        tag_script,       0 },
   { HASH_template,      tag_template,     0 },
   { HASH_xml,           tag_xml,          TAG_OBJECTOK },
   { HASH_xml_raw,       tag_xmlraw,       TAG_OBJECTOK },
   { HASH_xml_translate, tag_xmltranslate, TAG_OBJECTOK },
   { 0, NULL, 0 }
};