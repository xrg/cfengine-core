/*
   Copyright (C) CFEngine AS

   This file is part of CFEngine 3 - written and maintained by CFEngine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <rlist.h>

#include <files_names.h>
#include <conversion.h>
#include <expand.h>
#include <matching.h>
#include <scope.h>
#include <fncall.h>
#include <string_lib.h>
#include <mutex.h>
#include <misc_lib.h>
#include <assoc.h>
#include <eval_context.h>
#include <json.h>


static Rlist *RlistPrependRval(Rlist **start, Rval rval);

RvalType DataTypeToRvalType(DataType datatype)
{
    switch (datatype)
    {
    case CF_DATA_TYPE_BODY:
    case CF_DATA_TYPE_BUNDLE:
    case CF_DATA_TYPE_CONTEXT:
    case CF_DATA_TYPE_COUNTER:
    case CF_DATA_TYPE_INT:
    case CF_DATA_TYPE_INT_RANGE:
    case CF_DATA_TYPE_OPTION:
    case CF_DATA_TYPE_REAL:
    case CF_DATA_TYPE_REAL_RANGE:
    case CF_DATA_TYPE_STRING:
        return RVAL_TYPE_SCALAR;

    case CF_DATA_TYPE_CONTEXT_LIST:
    case CF_DATA_TYPE_INT_LIST:
    case CF_DATA_TYPE_OPTION_LIST:
    case CF_DATA_TYPE_REAL_LIST:
    case CF_DATA_TYPE_STRING_LIST:
        return RVAL_TYPE_LIST;

    case CF_DATA_TYPE_CONTAINER:
        return RVAL_TYPE_CONTAINER;

    case CF_DATA_TYPE_NONE:
        return RVAL_TYPE_NOPROMISEE;
    }

    ProgrammingError("DataTypeToRvalType, unhandled");
}

char *RlistScalarValue(const Rlist *rlist)
{
    if (rlist->val.type != RVAL_TYPE_SCALAR)
    {
        ProgrammingError("Rlist value contains type %c instead of expected scalar", rlist->val.type);
    }

    return rlist->val.item;
}

/*******************************************************************/

FnCall *RlistFnCallValue(const Rlist *rlist)
{
    if (rlist->val.type != RVAL_TYPE_FNCALL)
    {
        ProgrammingError("Rlist value contains type %c instead of expected FnCall", rlist->val.type);
    }

    return rlist->val.item;
}

/*******************************************************************/

Rlist *RlistRlistValue(const Rlist *rlist)
{
    if (rlist->val.type != RVAL_TYPE_LIST)
    {
        ProgrammingError("Rlist value contains type %c instead of expected List", rlist->val.type);
    }

    return rlist->val.item;
}

/*******************************************************************/

char *RvalScalarValue(Rval rval)
{
    if (rval.type != RVAL_TYPE_SCALAR)
    {
        ProgrammingError("Internal error: Rval contains type %c instead of expected scalar", rval.type);
    }

    return rval.item;
}

/*******************************************************************/

FnCall *RvalFnCallValue(Rval rval)
{
    if (rval.type != RVAL_TYPE_FNCALL)
    {
        ProgrammingError("Rval contains type %c instead of expected FnCall", rval.type);
    }

    return rval.item;
}

/*******************************************************************/

Rlist *RvalRlistValue(Rval rval)
{
    if (rval.type != RVAL_TYPE_LIST)
    {
        ProgrammingError("Rval contain type %c instead of expected List", rval.type);
    }

    return rval.item;
}

/*******************************************************************/

JsonElement *RvalContainerValue(Rval rval)
{
    if (rval.type != RVAL_TYPE_CONTAINER)
    {
        ProgrammingError("Rval contain type %c instead of expected container", rval.type);
    }

    return rval.item;
}


const char *RvalTypeToString(RvalType type)
{
    switch (type)
    {
    case RVAL_TYPE_CONTAINER:
        return "data";
    case RVAL_TYPE_FNCALL:
        return "call";
    case RVAL_TYPE_LIST:
        return "list";
    case RVAL_TYPE_NOPROMISEE:
        return "null";
    case RVAL_TYPE_SCALAR:
        return "scalar";
    }

    assert(false && "never reach");
    return NULL;
}

Rlist *RlistKeyIn(Rlist *list, const char *key)
{
    for (Rlist *rp = list; rp != NULL; rp = rp->next)
    {
        if (rp->val.type == RVAL_TYPE_SCALAR &&
            strcmp(RlistScalarValue(rp), key) == 0)
        {
            return rp;
        }
    }

    return NULL;
}

/*******************************************************************/

bool RlistMatchesRegexRlist(const Rlist *list, const Rlist *search)
/*
   Returns true if "list" contains all the regular expressions in
   "search".  Non-scalars in "list" and "search" are skipped.
*/
{
    for (const Rlist *rp = search; rp != NULL; rp = rp->next)
    {
        if (rp->val.type == RVAL_TYPE_SCALAR &&
            // check for the current element in the search list
            !RlistMatchesRegex(list, RlistScalarValue(search)))
        {
            return false;
        }
    }

    return true;
}

bool RlistMatchesRegex(const Rlist *list, const char *regex)
/*
   Returns true if any of the "list" of strings matches "regex".
   Non-scalars in "list" are skipped.
*/
{
    if (regex == NULL || list == NULL)
    {
        return false;
    }

    for (const Rlist *rp = list; rp != NULL; rp = rp->next)
    {
        if (rp->val.type == RVAL_TYPE_SCALAR &&
            StringMatchFull(regex, RlistScalarValue(rp)))
        {
            return true;
        }
    }

    return false;
}

// TODO: cf_null lists should not be needed
bool RlistIsNullList(const Rlist *list)
{
    for (const Rlist *rp = list; rp; rp = rp->next)
    {
        if (rp->val.type != RVAL_TYPE_SCALAR)
        {
            return false;
        }

        if (strcmp(CF_NULL_VALUE, RlistScalarValue(rp)) != 0)
        {
            return false;
        }
    }

    return true;
}

bool RlistIsInListOfRegex(const Rlist *list, const char *str)
/*
   Returns true if any of the "list" of regular expressions matches "str".
   Non-scalars in "list" are skipped.
*/
{
    if (str == NULL || list == NULL)
    {
        return false;
    }

    for (const Rlist *rp = list; rp != NULL; rp = rp->next)
    {
        if (rp->val.type == RVAL_TYPE_SCALAR &&
            StringMatchFull(RlistScalarValue(rp), str))
        {
            return true;
        }
    }

    return false;
}

/*******************************************************************/

static Rval RvalCopyScalar(Rval rval)
{
    assert(rval.type == RVAL_TYPE_SCALAR);
    const char * src = rval.item ? rval.item : "";

    return (Rval) {xstrdup(src), RVAL_TYPE_SCALAR};
}

Rlist *RlistAppendRval(Rlist **start, Rval rval)
{
    Rlist *rp = xmalloc(sizeof(Rlist));

    if (*start == NULL)
    {
        *start = rp;
    }
    else
    {
        Rlist *lp = *start;
        while (lp->next != NULL)
        {
            lp = lp->next;
        }

        lp->next = rp;
    }

    rp->val = rval;

    ThreadLock(cft_lock);

    rp->next = NULL;

    ThreadUnlock(cft_lock);

    return rp;
}

Rval RvalNew(const void *item, RvalType type)
{
    switch (type)
    {
    case RVAL_TYPE_SCALAR:
        return (Rval) { xstrdup(item), RVAL_TYPE_SCALAR };

    case RVAL_TYPE_FNCALL:
        return (Rval) { FnCallCopy(item), RVAL_TYPE_FNCALL };

    case RVAL_TYPE_LIST:
        return (Rval) { RlistCopy(item), RVAL_TYPE_LIST };

    case RVAL_TYPE_CONTAINER:
        return (Rval) { JsonCopy(item), RVAL_TYPE_CONTAINER };

    case RVAL_TYPE_NOPROMISEE:
        return ((Rval) {NULL, type});
    }

    assert(false);
    return ((Rval) { NULL, RVAL_TYPE_NOPROMISEE });
}

Rval RvalCopy(Rval rval)
{
    return RvalNew(rval.item, rval.type);
}

/*******************************************************************/

Rlist *RlistCopy(const Rlist *rp)
{
    Rlist *start = NULL;

    while (rp != NULL)
    {
        RlistAppendRval(&start, RvalCopy(rp->val));
        rp = rp->next;
    }

    return start;
}

/*******************************************************************/

void RlistDestroy(Rlist *rl)
/* Delete an rlist and all its references */
{
    while (rl != NULL)
    {
        Rlist *next = rl->next;

        if (rl->val.item)
        {
            RvalDestroy(rl->val);
        }

        free(rl);
        rl = next;
    }
}

/*******************************************************************/

Rlist *RlistAppendScalarIdemp(Rlist **start, const char *scalar)
{
    if (RlistKeyIn(*start, scalar))
    {
        return NULL;
    }

    return RlistAppendScalar(start, scalar);
}

Rlist *RlistPrependScalarIdemp(Rlist **start, const char *scalar)
{
    if (RlistKeyIn(*start, scalar))
    {
        return NULL;
    }

    return RlistPrepend(start, scalar, RVAL_TYPE_SCALAR);
}

Rlist *RlistAppendScalar(Rlist **start, const char *scalar)
{
    return RlistAppendRval(start, RvalCopyScalar((Rval) { (char *)scalar, RVAL_TYPE_SCALAR }));
}

Rlist *RlistAppend(Rlist **start, const void *item, RvalType type)
{
    Rlist *lp = *start;

    switch (type)
    {
    case RVAL_TYPE_SCALAR:
        return RlistAppendScalar(start, item);

    case RVAL_TYPE_FNCALL:
        break;

    case RVAL_TYPE_LIST:
        for (const Rlist *rp = item; rp; rp = rp->next)
        {
            lp = RlistAppendRval(start, RvalCopy(rp->val));
        }

        return lp;

    default:
        Log(LOG_LEVEL_DEBUG, "Cannot append %c to rval-list '%s'", type, (char *) item);
        return NULL;
    }

    Rlist *rp = xmalloc(sizeof(Rlist));

    if (*start == NULL)
    {
        *start = rp;
    }
    else
    {
        for (lp = *start; lp->next != NULL; lp = lp->next)
        {
        }

        lp->next = rp;
    }

    rp->val = RvalCopy((Rval) {(void *) item, type});

    ThreadLock(cft_lock);

    rp->next = NULL;

    ThreadUnlock(cft_lock);

    return rp;
}

/*******************************************************************/

static Rlist *RlistPrependRval(Rlist **start, Rval rval)
{
    Rlist *rp = xmalloc(sizeof(Rlist));

    rp->next = *start;
    rp->val = rval;
    ThreadLock(cft_lock);
    *start = rp;
    ThreadUnlock(cft_lock);
    return rp;
}

Rlist *RlistPrepend(Rlist **start, const void *item, RvalType type)
{
    switch (type)
    {
    case RVAL_TYPE_LIST:
        {
            Rlist *lp = NULL;
            for (const Rlist *rp = item; rp; rp = rp->next)
            {
                lp = RlistPrependRval(start, RvalCopy(rp->val));
            }
            return lp;
        }

    case RVAL_TYPE_SCALAR:
    case RVAL_TYPE_FNCALL:
    case RVAL_TYPE_CONTAINER:
    case RVAL_TYPE_NOPROMISEE:
        return RlistPrependRval(start, RvalNew(item, type));
    }

    assert(false);
    return NULL;
}

/*******************************************************************/

int RlistLen(const Rlist *start)
{
    int count = 0;

    for (const Rlist *rp = start; rp != NULL; rp = rp->next)
    {
        count++;
    }

    return count;
}

/*******************************************************************/

Rlist *RlistParseShown(const char *string)
{
    Rlist *newlist = NULL, *splitlist, *rp;
    char value[CF_MAXVARSIZE];

/* Parse a string representation generated by ShowList and turn back into Rlist */

    splitlist = RlistFromSplitString(string, ',');

    for (rp = splitlist; rp != NULL; rp = rp->next)
    {
        sscanf(RlistScalarValue(rp), "%*[{ '\"]%255[^'\"]", value);
        RlistAppendScalar(&newlist, value);
    }

    RlistDestroy(splitlist);
    return newlist;
}

/*******************************************************************/

typedef enum
{
    ST_OPENED,
    ST_PRECLOSED,
    ST_CLOSED,
    ST_IO,
    ST_ELM1,
    ST_ELM2,
    ST_END1,
    ST_END2,
    ST_SEP,
    ST_ERROR
} state;

#define CLASS_BLANK(x)  (((x)==' ')||((x)=='\t'))
#define CLASS_START1(x) (((x)=='\'')) 
#define CLASS_START2(x) (((x)=='"'))
#define CLASS_END1(x)   ((CLASS_START1(x)))
#define CLASS_END2(x)   ((CLASS_START2(x)))
#define CLASS_BRA1(x)   (((x)=='{'))
#define CLASS_BRA2(x)   (((x)=='}'))
#define CLASS_SEP(x)    (((x)==','))
#define CLASS_EOL(x)    (((x)=='\0'))

#define CLASS_ANY0(x) ((!CLASS_BLANK(x))&&(!CLASS_BRA1(x)))
#define CLASS_ANY1(x) ((!CLASS_BLANK(x))&&(!CLASS_START1(x))&&(!CLASS_START2(x)))
#define CLASS_ANY2(x) ((!CLASS_END1(x)))
#define CLASS_ANY3(x) ((!CLASS_END2(x)))
#define CLASS_ANY4(x) ((!CLASS_BLANK(x))&&(!CLASS_SEP(x))&&(!CLASS_BRA2(x)))
#define CLASS_ANY5(x) ((!CLASS_BLANK(x))&&(!CLASS_SEP(x))&&(!CLASS_BRA2(x)))
#define CLASS_ANY6(x) ((!CLASS_BLANK(x))&&(!CLASS_START2(x))&&(!CLASS_START2(x)))
#define CLASS_ANY7(x) ((!CLASS_BLANK(x))&&(!CLASS_EOL(x)))

/**
 @brief parse elements in a list passed through use_module
 
 @param[in] str: is the string to parse
 @param[out] newlist: rlist of elements found

 @retval 0: successful > 0: failed
 */
static int LaunchParsingMachine(const char *str, Rlist **newlist)
{
    const char *s = str;
    state current_state = ST_OPENED;
    int ret;

    Buffer *buf = BufferNewWithCapacity(CF_MAXVARSIZE);

    assert(newlist);

    while (current_state != ST_CLOSED && *s)
    {
        switch(current_state)
        {
            case ST_ERROR:
                Log(LOG_LEVEL_ERR, "Parsing error : Malformed string");
                ret = 1;
                goto clean;
            case ST_OPENED:
                if (CLASS_BLANK(*s))
                {
                    current_state = ST_OPENED;
                }
                else if (CLASS_BRA1(*s)) 
                {
                    current_state = ST_IO;
                }
                else if (CLASS_ANY0(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            case ST_IO:
                if (CLASS_BLANK(*s))
                {
                    current_state = ST_IO;
                }
                else if (CLASS_START1(*s))
                {
                    BufferClear(buf);
                    current_state = ST_ELM1;
                }
                else if (CLASS_START2(*s))
                {
                    BufferClear(buf);
                    current_state = ST_ELM2;
                }
                else if (CLASS_ANY1(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            case ST_ELM1:
                if (CLASS_END1(*s))
                {
                    RlistAppendScalar(newlist, BufferData(buf));
                    BufferClear(buf);
                    current_state = ST_END1;
                }
                else if (CLASS_ANY2(*s))
                {
                    BufferAppendChar(buf, *s);
                    current_state = ST_ELM1;
                }
                s++;
                break;
            case ST_ELM2:
                if (CLASS_END2(*s))
                {
                    RlistAppendScalar(newlist, BufferData(buf));
                    BufferClear(buf);
                    current_state = ST_END2;
                }
                else if (CLASS_ANY3(*s))
                {
                    BufferAppendChar(buf, *s);
                    current_state = ST_ELM2;
                }
                s++;
                break;
            case ST_END1:
                if (CLASS_SEP(*s))
                {
                    current_state = ST_SEP;
                }
                else if (CLASS_BRA2(*s))
                {
                    current_state = ST_PRECLOSED;
                }
                else if (CLASS_BLANK(*s))
                {
                    current_state = ST_END1;
                }
                else if (CLASS_ANY4(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            case ST_END2:
                if (CLASS_SEP(*s))
                {
                    current_state = ST_SEP;
                }
                else if (CLASS_BRA2(*s))
                {
                    current_state = ST_PRECLOSED;
                }
                else if (CLASS_BLANK(*s))
                {
                    current_state = ST_END2;
                }
                else if (CLASS_ANY5(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            case ST_SEP:
                if (CLASS_BLANK(*s))
                {
                    current_state = ST_SEP;
                }
                else if (CLASS_START1(*s))
                {
                    current_state = ST_ELM1;
                }
                else if (CLASS_START2(*s))
                {
                    current_state = ST_ELM2;
                }
                else if (CLASS_ANY6(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            case ST_PRECLOSED:
                if (CLASS_BLANK(*s))
                {
                    current_state = ST_PRECLOSED;
                }
                else if (CLASS_EOL(*s))
                {
                    current_state = ST_CLOSED;
                }
                else if (CLASS_ANY7(*s))
                {
                    current_state = ST_ERROR;
                }
                s++;
                break;
            default:
                Log(LOG_LEVEL_ERR, "Parsing logic error: unknown state");
                ret = 2;
                goto clean;
                break;
        }
    }

    if (current_state != ST_CLOSED && current_state != ST_PRECLOSED )
    {
        Log(LOG_LEVEL_ERR, "Parsing error : Malformed string (unexpected end of input)");
        ret = 3;
        goto clean;
    }

    BufferDestroy(buf);
    return 0;

clean:
    BufferDestroy(buf);
    RlistDestroy(*newlist);
    assert(ret != 0);
    return ret;
}

Rlist *RlistParseString(const char *string)
{
    Rlist *newlist = NULL;
    if (LaunchParsingMachine(string, &newlist))
    {
        return NULL;
    }

    return newlist;
}

/*******************************************************************/

void RvalDestroy(Rval rval)
{
    if (rval.item == NULL)
    {
        return;
    }

    switch (rval.type)
    {
    case RVAL_TYPE_SCALAR:
        ThreadLock(cft_lock);
        free(RvalScalarValue(rval));
        ThreadUnlock(cft_lock);
        return;

    case RVAL_TYPE_LIST:
        RlistDestroy(RvalRlistValue(rval));
        return;

    case RVAL_TYPE_FNCALL:
        FnCallDestroy(RvalFnCallValue(rval));
        break;

    case RVAL_TYPE_CONTAINER:
        JsonDestroy(RvalContainerValue(rval));
        break;

    case RVAL_TYPE_NOPROMISEE:
        return;
    }
}

/*********************************************************************/

void RlistDestroyEntry(Rlist **liststart, Rlist *entry)
{
    if (entry != NULL)
    {
        if (entry->val.item)
        {
            free(entry->val.item);
        }

        Rlist *sp = entry->next;

        if (entry == *liststart)
        {
            *liststart = sp;
        }
        else
        {
            Rlist *rp = *liststart;
            while (rp->next != entry)
            {
                rp = rp->next;
            }

            assert(rp && rp->next == entry);
            rp->next = sp;
        }

        free(entry);
    }
}

/*******************************************************************/

/*
 * Copies from <from> to <to>, writing up to <len> bytes, stopping
 * before the first <sep>.
 *
 * \<sep> is not counted as the separator, but copied to <to> as <sep>.
 * Any other escape sequences are not supported.
 *
 * Returns the number of bytes read out of from; this may be more than
 * the number written into to (which is at most len, including the
 * terminating '\0').
 */
static int SubStrnCopyChr(char *to, const char *from, int len, char sep)
{
    char *sto = to;
    int count = 0;
    assert(from && from[0]);

    for (const char *sp = from; sto - to < len - 1 && sp[0] != '\0'; sp++)
    {
        if (sp[0] == '\\' && sp[1] == sep)
        {
            *sto++ = *++sp;
            count += 2;
        }
        else if (sp[0] == sep)
        {
            break;
        }
        else
        {
            *sto++ = sp[0];
            count++;
        }
    }
    assert(sto - to < len);
    *sto = '\0';

    assert(count <= strlen(from));
    return count;
}

Rlist *RlistFromSplitString(const char *string, char sep)
/* Splits a string on a separator - e.g. "," - into a linked list of
 * separate items.  Supports escaping separators - e.g. "\," isn't a
 * separator, it contributes a simple "," in a list entry. */
{
    if (string == NULL)
    {
        return NULL;
    }

    Rlist *liststart = NULL;
    char node[CF_MAXVARSIZE];

    for (const char *sp = string; *sp != '\0';)
    {
        sp += SubStrnCopyChr(node, sp, CF_MAXVARSIZE, sep);
        assert(sp - string <= strlen(string));
        if (*sp)
        {
            assert(*sp == sep && (sp == string || sp[-1] != '\\'));
            sp++;
        }

        RlistAppendScalar(&liststart, node);
    }

    return liststart;
}

/*******************************************************************/

Rlist *RlistFromSplitRegex(const char *string, const char *regex, size_t max_entries, bool allow_blanks)
{
    assert(string);
    if (!string)
    {
        return NULL;
    }

    const char *sp = string;
    size_t entry_count = 0;
    int start = 0;
    int end = 0;
    Rlist *result = NULL;
    Buffer *buffer = BufferNewWithCapacity(CF_MAXVARSIZE);

    while ((entry_count < max_entries) && StringMatch(regex, sp, &start, &end))
    {
        if (end == 0)
        {
            break;
        }

        BufferClear(buffer);
        BufferAppend(buffer, sp, start);

        if (allow_blanks || BufferSize(buffer) > 0)
        {
            RlistAppendScalar(&result, BufferData(buffer));
            entry_count++;
        }

        sp += end;
    }

    if (entry_count < max_entries)
    {
        BufferClear(buffer);
        size_t remaining = strlen(sp);
        BufferAppend(buffer, sp, remaining);

        if ((allow_blanks && sp != string) || BufferSize(buffer) > 0)
        {
            RlistAppendScalar(&result, BufferData(buffer));
        }
    }

    BufferDestroy(buffer);

    return result;
}

/*******************************************************************/
/*
 * Splits string on regex, returns a list of (at most max) fragments.
 *
 * NOTE: in contrast with RlistFromSplitRegex() this one will produce at most max number of elements;
 *       last element will contain everything that lefts from original string (we use everything after
 *       the (max-1)-th separator as the final list element, including any separators that may be embedded in it)
 */
Rlist *RlistFromRegexSplitNoOverflow(const char *string, const char *regex, int max)
{
    Rlist *liststart = NULL;
    char node[CF_MAXVARSIZE];
    int start, end;
    int count = 0;

    assert(max > 0); // ensured by FnCallStringSplit() before calling us
    assert(string != NULL); // ensured by FnCallStringSplit() before calling us

    const char *sp = string;
    // We will avoid compiling regex multiple times.
    pcre *pattern = CompileRegex(regex);

    if (pattern == NULL)
    {
        Log(LOG_LEVEL_DEBUG, "Error compiling regex from '%s'", regex);
        return NULL;
    }

    while (count < max - 1 &&
           StringMatchWithPrecompiledRegex(pattern, sp, &start, &end))
    {
        assert(start < CF_MAXVARSIZE);
        memcpy(node, sp, start);
        node[start] = '\0';
        RlistAppendScalar(&liststart, node);
        count++;

        sp += end;
    }

    assert(count < max);
    RlistAppendScalar(&liststart, sp);

    pcre_free(pattern);

    return liststart;
}

Rlist *RlistLast(Rlist *start)
{
    if (start == NULL)
    {
        return NULL;
    }
    Rlist *rp = start;
    while (rp->next != NULL)
    {
        rp = rp->next;
    }
    return rp;
}

void RlistFilter(Rlist **list,
                 bool (*KeepPredicate)(void *, void *), void *predicate_user_data,
                 void (*DestroyItem)(void *))
{
    assert(KeepPredicate);
    Rlist *start = *list, *prev = NULL, *next;

    for (Rlist *rp = start; rp; rp = next)
    {
        next = rp->next;
        if (KeepPredicate(RlistScalarValue(rp), predicate_user_data))
        {
            prev = rp;
        }
        else
        {
            if (prev)
            {
                prev->next = next;
            }
            else
            {
                assert(rp == *list);
                *list = next;
            }

            if (DestroyItem)
            {
                DestroyItem(rp->val.item);
                rp->val.item = NULL;
            }

            rp->next = NULL;
            RlistDestroy(rp);
        }
    }
}

void RlistReverse(Rlist **list)
{
    Rlist *prev = NULL;
    while (*list)
    {
        Rlist *tmp = *list;
        *list = (*list)->next;
        tmp->next = prev;
        prev = tmp;
    }
    *list = prev;
}

void RlistWrite(Writer *writer, const Rlist *list)
{
    WriterWrite(writer, " {");

    for (const Rlist *rp = list; rp != NULL; rp = rp->next)
    {
        WriterWriteChar(writer, '\'');
        RvalWrite(writer, rp->val);
        WriterWriteChar(writer, '\'');

        if (rp->next != NULL)
        {
            WriterWriteChar(writer, ',');
        }
    }

    WriterWriteChar(writer, '}');
}

/* Note: only single quotes are escaped, as they are used in RlistWrite to
   delimit strings. If double quotes would be escaped, they would be mangled by
   RlistParseShown */

static void ScalarWrite(Writer *w, const char *s)
{
    for (; *s; s++)
    {
        if (*s == '\'')
        {
            WriterWriteChar(w, '\\');
        }
        WriterWriteChar(w, *s);
    }
}

void RvalWrite(Writer *writer, Rval rval)
{
    RvalWriteParts(writer, rval.item, rval.type);
}

void RvalWriteParts(Writer *writer, const void* item, RvalType type)
{
    if (item == NULL)
    {
        return;
    }

    switch (type)
    {
    case RVAL_TYPE_SCALAR:
        ScalarWrite(writer, item);
        break;

    case RVAL_TYPE_LIST:
        RlistWrite(writer, item);
        break;

    case RVAL_TYPE_FNCALL:
        FnCallWrite(writer, item);
        break;

    case RVAL_TYPE_NOPROMISEE:
        WriterWrite(writer, "(no-one)");
        break;

    case RVAL_TYPE_CONTAINER:
        JsonWrite(writer, item, 0);
        break;
    }
}

void RlistShow(FILE *fp, const Rlist *list)
{
    Writer *w = FileWriter(fp);
    RlistWrite(w, list);
    FileWriterDetach(w);
}

void RvalShow(FILE *fp, Rval rval)
{
    Writer *w = FileWriter(fp);
    RvalWrite(w, rval);
    FileWriterDetach(w);
}

unsigned RvalHash(Rval rval, unsigned seed, unsigned max)
{
    switch (rval.type)
    {
    case RVAL_TYPE_SCALAR:
        return StringHash(RvalScalarValue(rval), seed, max);
    case RVAL_TYPE_FNCALL:
        return FnCallHash(RvalFnCallValue(rval), seed, max);
    case RVAL_TYPE_LIST:
        return RlistHash(RvalRlistValue(rval), seed, max);
    case RVAL_TYPE_NOPROMISEE:
        return (seed + 1) % max;
    default:
        ProgrammingError("Unhandled case in switch: %d", rval.type);
    }
}

unsigned RlistHash(const Rlist *list, unsigned seed, unsigned max)
{
    unsigned hash = seed;
    for (const Rlist *rp = list; rp; rp = rp->next)
    {
        hash = RvalHash(rp->val, hash, max);
    }
    return hash;
}


static JsonElement *FnCallToJson(const FnCall *fp)
{
    assert(fp);

    JsonElement *object = JsonObjectCreate(3);

    JsonObjectAppendString(object, "name", fp->name);
    JsonObjectAppendString(object, "type", "function-call");

    JsonElement *argsArray = JsonArrayCreate(5);

    for (Rlist *rp = fp->args; rp != NULL; rp = rp->next)
    {
        switch (rp->val.type)
        {
        case RVAL_TYPE_SCALAR:
            JsonArrayAppendString(argsArray, RlistScalarValue(rp));
            break;

        case RVAL_TYPE_FNCALL:
            JsonArrayAppendObject(argsArray, FnCallToJson(RlistFnCallValue(rp)));
            break;

        default:
            assert(false && "Unknown argument type");
            break;
        }
    }
    JsonObjectAppendArray(object, "arguments", argsArray);

    return object;
}

static JsonElement *RlistToJson(Rlist *list)
{
    JsonElement *array = JsonArrayCreate(RlistLen(list));

    for (Rlist *rp = list; rp; rp = rp->next)
    {
        switch (rp->val.type)
        {
        case RVAL_TYPE_SCALAR:
            JsonArrayAppendString(array, RlistScalarValue(rp));
            break;

        case RVAL_TYPE_LIST:
            JsonArrayAppendArray(array, RlistToJson(RlistRlistValue(rp)));
            break;

        case RVAL_TYPE_FNCALL:
            JsonArrayAppendObject(array, FnCallToJson(RlistFnCallValue(rp)));
            break;

        default:
            assert(false && "Unsupported item type in rlist");
            break;
        }
    }

    return array;
}

JsonElement *RvalToJson(Rval rval)
{
    assert(rval.item);

    switch (rval.type)
    {
    case RVAL_TYPE_SCALAR:
        return JsonStringCreate(RvalScalarValue(rval));
    case RVAL_TYPE_LIST:
        return RlistToJson(RvalRlistValue(rval));
    case RVAL_TYPE_FNCALL:
        return FnCallToJson(RvalFnCallValue(rval));
    case RVAL_TYPE_CONTAINER:
        return JsonCopy(RvalContainerValue(rval));
    case RVAL_TYPE_NOPROMISEE:
        assert(false);
        return JsonObjectCreate(1);
    }

    assert(false);
    return NULL;
}

void RlistFlatten(EvalContext *ctx, Rlist **list)
{
    Rlist *prev = NULL, *next;
    for (Rlist *rp = *list; rp != NULL; rp = next)
    {
        next = rp->next;
        if (rp->val.type != RVAL_TYPE_SCALAR)
        {
            prev = rp;
            continue;
        }

        char naked[CF_BUFSIZE] = "";
        if (IsNakedVar(RlistScalarValue(rp), '@'))
        {
            GetNaked(naked, RlistScalarValue(rp));

            if (!IsExpandable(naked))
            {
                VarRef *ref = VarRefParse(naked);
                DataType value_type = CF_DATA_TYPE_NONE;
                const void *value = EvalContextVariableGet(ctx, ref, &value_type);
                VarRefDestroy(ref);

                if (value)
                {
                    switch (DataTypeToRvalType(value_type))
                    {
                    case RVAL_TYPE_LIST:
                        {
                            RlistDestroyEntry(list, rp);

                            for (const Rlist *srp = value; srp != NULL; srp = srp->next)
                            {
                                Rlist *nrp = xmalloc(sizeof(Rlist));
                                nrp->val = RvalCopy(srp->val);
                                nrp->next = next;

                                if (prev)
                                {
                                    prev->next = nrp;
                                }
                                else
                                {
                                    *list = nrp;
                                }

                                prev = nrp;
                            }
                        }
                        continue;

                    default:
                        Log(LOG_LEVEL_WARNING, "Attempted to dereference variable '%s' using @ but variable did not resolve to a list",
                            RlistScalarValue(rp));
                        break;
                    }
                }
            }
        }

        prev = rp;
    }
}
