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

#include <promise_logging.h>

#include <logging.h>
#include <logging_priv.h>
#include <misc_lib.h>
#include <string_lib.h>
#include <eval_context.h>
#include <ring_buffer.h>

typedef struct
{
    const EvalContext *eval_context;
    int promise_level;

    char *stack_path;

    RingBuffer *messages;
} PromiseLoggingContext;

static LogLevel AdjustLogLevel(LogLevel base, LogLevel adjust)
{
    if (adjust == -1)
    {
        return base;
    }
    else
    {
        return MAX(base, adjust);
    }
}

static LogLevel StringToLogLevel(const char *value)
{
    if (value)
    {
        if (!strcmp(value, "verbose"))
        {
            return LOG_LEVEL_VERBOSE;
        }
        if (!strcmp(value, "inform"))
        {
            return LOG_LEVEL_INFO;
        }
        if (!strcmp(value, "error"))
        {
            return LOG_LEVEL_NOTICE; /* Error level includes warnings and notices */
        }
    }
    return -1;
}

static LogLevel GetLevelForPromise(const Promise *pp, const char *attr_name)
{
    return StringToLogLevel(PromiseGetConstraintAsRval(pp, attr_name, RVAL_TYPE_SCALAR));
}

static LogLevel CalculateLogLevel(const Promise *pp)
{
    LogLevel log_level = LogGetGlobalLevel();

    if (pp)
    {
        log_level = AdjustLogLevel(log_level, GetLevelForPromise(pp, "log_level"));
    }

    /* Disable system log for dry-runs */
    if (DONTDO)
    {
        log_level = LOG_LEVEL_NOTHING;
    }

    return log_level;
}

static LogLevel CalculateReportLevel(const Promise *pp)
{
    LogLevel report_level = LogGetGlobalLevel();

    if (pp)
    {
        report_level = AdjustLogLevel(report_level, GetLevelForPromise(pp, "report_level"));
    }

    return report_level;
}

static char *LogHook(LoggingPrivContext *pctx, const char *message)
{
    PromiseLoggingContext *plctx = pctx->param;

    if (plctx->promise_level > 0)
    {
        RingBufferAppend(plctx->messages, xstrdup(message));
        return StringConcatenate(3, plctx->stack_path, ": ", message);
    }
    else
    {
        return xstrdup(message);
    }
}

void PromiseLoggingInit(const EvalContext *eval_context, size_t max_messages)
{
    LoggingPrivContext *pctx = LoggingPrivGetContext();

    if (pctx != NULL)
    {
        ProgrammingError("Promise logging: Still bound to another EvalContext");
    }

    PromiseLoggingContext *plctx = xcalloc(1, sizeof(PromiseLoggingContext));
    plctx->eval_context = eval_context;
    plctx->messages = RingBufferNew(max_messages, NULL, free);

    pctx = xcalloc(1, sizeof(LoggingPrivContext));
    pctx->param = plctx;
    pctx->log_hook = &LogHook;

    LoggingPrivSetContext(pctx);
}

void PromiseLoggingPromiseEnter(const EvalContext *eval_context, const Promise *pp)
{
    LoggingPrivContext *pctx = LoggingPrivGetContext();

    if (pctx == NULL)
    {
        ProgrammingError("Promise logging: Unable to enter promise, not bound to EvalContext");
    }

    PromiseLoggingContext *plctx = pctx->param;

    if (plctx->eval_context != eval_context)
    {
        ProgrammingError("Promise logging: Unable to enter promise, bound to EvalContext different from passed one");
    }

    if (EvalContextStackCurrentPromise(eval_context) != pp)
    {
        /*
         * FIXME: There are still cases where promise passed here is not on top of stack
         */
        /* ProgrammingError("Logging: Attempt to set promise not on top of stack as current"); */
    }

    plctx->promise_level++;
    assert(plctx->promise_level == 1);
    plctx->stack_path = EvalContextStackPath(eval_context);
    RingBufferClear(plctx->messages);
    LoggingPrivSetLevels(CalculateLogLevel(pp), CalculateReportLevel(pp));
}

void PromiseLoggingPromiseFinish(const EvalContext *eval_context, const Promise *pp)
{
    LoggingPrivContext *pctx = LoggingPrivGetContext();

    if (pctx == NULL)
    {
        ProgrammingError("Promise logging: Unable to finish promise, not bound to EvalContext");
    }

    PromiseLoggingContext *plctx = pctx->param;

    if (plctx->eval_context != eval_context)
    {
        ProgrammingError("Promise logging: Unable to finish promise, bound to EvalContext different from passed one");
    }

    if (EvalContextStackCurrentPromise(eval_context) != pp)
    {
        /*
         * FIXME: There are still cases where promise passed here is not on top of stack
         */
        /* ProgrammingError("Logging: Attempt to finish promise not on top of stack"); */
    }

    plctx->promise_level--;
    assert(plctx->promise_level == 0);
    free(plctx->stack_path);
    plctx->stack_path = NULL;

    LoggingPrivSetLevels(LogGetGlobalLevel(), LogGetGlobalLevel());
}

const RingBuffer *PromiseLoggingMessages(const EvalContext *ctx)
{
    LoggingPrivContext *pctx = LoggingPrivGetContext();

    if (pctx == NULL)
    {
        ProgrammingError("Promise logging: Unable to finish promise, not bound to EvalContext");
    }

    PromiseLoggingContext *plctx = pctx->param;

    if (plctx->eval_context != ctx)
    {
        ProgrammingError("Promise logging: Unable to finish promise, bound to EvalContext different from passed one");
    }

    return plctx->messages;
}

void PromiseLoggingFinish(const EvalContext *eval_context)
{
    LoggingPrivContext *pctx = LoggingPrivGetContext();

    if (pctx == NULL)
    {
        ProgrammingError("Promise logging: Unable to finish, PromiseLoggingInit was not called before");
    }

    PromiseLoggingContext *plctx = pctx->param;

    if (plctx->eval_context != eval_context)
    {
        ProgrammingError("Promise logging: Unable to finish, passed EvalContext does not correspond to current one");
    }

    if (plctx->promise_level > 0)
    {
        ProgrammingError("Promise logging: Unable to finish, promise is still active");
    }

    RingBufferDestroy(plctx->messages);
    LoggingPrivSetContext(NULL);

    free(plctx);
    free(pctx);
}
