#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h> /* ANSI C header file */
#include <syslog.h> /* for syslog() */


static void err_doit(int, int, const char *, va_list);
/* Nonfatal error related to system call
 * Print message and return */
static void err_ret(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}
/* Fatal error related to system call
 * Print message and terminate */
static void err_sys(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}
/* Fatal error related to system call
 * Print message, dump core, and terminate */
static void err_dump(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, LOG_ERR, fmt, ap);
    va_end(ap);
    abort(); /* dump core and terminate */
    exit(1); /* shouldn’t get here */
}
/* Nonfatal error unrelated to system call
 * Print message and return */
static void err_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, LOG_INFO, fmt, ap);
    va_end(ap);
    return;
}
/* Fatal error unrelated to system call
 * Print message and terminate */
static void err_quit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, LOG_ERR, fmt, ap);
    va_end(ap);
    exit(1);
}
/* Print message and return to caller
 * Caller specifies "errnoflag" and "level" */
static void
err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
    #define MAXLINE 1024
    int errno_save, n;
    char buf[MAXLINE + 1];
    errno_save = errno; /* value caller might want printed */
#ifdef HAVE_VSNPRINTF
    vsnprintf(buf, MAXLINE, fmt, ap); /* safe */
#else
    vsprintf(buf, fmt, ap); /* not safe */
#endif
    n = strlen(buf);
    if (errnoflag)
        snprintf(buf + n, MAXLINE - n, ": err [%d], errMsg [%s]", errno_save, strerror(errno_save));
    strcat(buf, "\n");
    if (0)
    {
        //syslog(level, buf);
    }
    else
    {
        fflush(stdout); /* in case stdout and stderr are the same */
        fputs(buf, stderr);
        fflush(stderr);
    }
    return;
}