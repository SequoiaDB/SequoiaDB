/* linenoise.c -- guerrilla line editing library against the idea that a
 * line editing lib needs to be 20,000 lines of C code.
 *
 * You can find the latest source code at:
 *
 *   http://github.com/antirez/linenoise
 *
 * Does a number of crazy assumptions that happen to be true in 99.9999% of
 * the 2010 UNIX computers around.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2013, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ------------------------------------------------------------------------
 *
 * References:
 * - http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - http://www.3waylabs.com/nw/WWW/products/wizcon/vt220.html
 *
 * Todo list:
 * - Filter bogus Ctrl+<char> combinations.
 * - Win32 support
 *
 * List of escape sequences used by this program, we do everything just
 * with three sequences. In order to be so cheap we may have some
 * flickering effect with some slow terminal, but the lesser sequences
 * the more compatible.
 *
 * CHA (Cursor Horizontal Absolute)
 *    Sequence: ESC [ n G
 *    Effect: moves cursor to column n
 *
 * EL (Erase Line)
 *    Sequence: ESC [ n K
 *    Effect: if n is 0 or missing, clear from cursor to end of line
 *    Effect: if n is 1, clear from beginning of line to cursor
 *    Effect: if n is 2, clear entire line
 *
 * CUF (CUrsor Forward)
 *    Sequence: ESC [ n C
 *    Effect: moves cursor forward of n chars
 *
 * When multi line mode is enabled, we also use an additional escape
 * sequence. However multi line editing is disabled by default.
 *
 * CUU (Cursor Up)
 *    Sequence: ESC [ n A
 *    Effect: moves cursor up of n chars.
 *
 * CUD (Cursor Down)
 *    Sequence: ESC [ n B
 *    Effect: moves cursor down of n chars.
 *
 * The following are used to clear the screen: ESC [ H ESC [ 2 J
 * This is actually composed of two sequences:
 *
 * cursorhome
 *    Sequence: ESC [ H
 *    Effect: moves the cursor to upper left corner
 *
 * ED2 (Clear entire screen)
 *    Sequence: ESC [ 2 J
 *    Effect: clear the whole screen
 *
 */

#ifdef _UNICODE
#undef _UNICODE
#endif

#ifdef UNICODE
#undef UNICODE
#endif

#include "core.hpp"

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>


#include "linenoise.h"
#include "pdTrace.hpp"
#include "utilTrace.hpp"


#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define snprintf _snprintf
#define read(x,y,z) _read(x,y,z)
#define write(x,y,z) _write(x,y,z)
#define isatty(x) _isatty(x)
#define strdup(x) _strdup(x)
#endif

#define REDIS_NOTUSED(V) ((void) V)

#define LINENOISE_DEFAULT_HISTORY_MAX_LEN 300
#define LINENOISE_MAX_LINE 4096
static char *unsupported_term[] = {"dumb","cons25","emacs",NULL};
static linenoiseCompletionCallback *completionCallback = NULL;

static void setDisplayAttribute( bool enhancedDisplay, struct abuf *ab );

#ifndef _WIN32
static struct termios orig_termios; /* In order to restore at exit.*/
#endif
static int rawmode = 0; /* For atexit() function to check if restore is needed*/
static int mlmode = 1;  /* Multi line mode. Default is single line. */
static int atexit_registered = 0; /* Register atexit just 1 time. */
static int history_max_len = LINENOISE_DEFAULT_HISTORY_MAX_LEN;
int history_len = 0;
static char **history = NULL;

/*
* echo Char = 0, echo original content
* echo Char != 0, echo corresponding ASCII character
*/
static char echoChar = 0;
static int echoOn = 1;

/* The linenoiseState structure represents the state during line editing.
 * We pass this state to functions implementing specific editing
 * functionalities. */
struct linenoiseState
{
    int ifd;            /* Terminal stdin file descriptor. */
    int ofd;            /* Terminal stdout file descriptor. */
    char *buf;          /* Edited line buffer. */
    size_t buflen;      /* Edited line buffer size. */
    const char *prompt; /* Prompt to display. */
    size_t plen;        /* Prompt length. */
    size_t pos;         /* Current cursor position. */
    size_t oldpos;      /* Previous refresh cursor position. */
    size_t len;         /* Current edited line length. */
    size_t cols;        /* Number of columns in terminal. */
    size_t maxrows;     /* Maximum num of rows used so far (multiline mode) */
    int history_index;  /* The history index we are currently editing. */
    bool remove_col;   /* Whether the refresh operation is remove colour or not */
};

typedef int ( *CharacterDispatchRoutine ) ( struct linenoiseState *, char ) ;

struct CharacterDispatch
{
   unsigned int len ;                    // length of the chars list
   const char *chars ;                   // chars to test
   CharacterDispatchRoutine *dispatch ;  // array of routines to call
} ;

static int doDispatch( struct linenoiseState *l, char c,
                       CharacterDispatch &dispatchTable )
{
   int ret = 0 ;
   for ( unsigned int i = 0 ; i < dispatchTable.len ; ++i )
   {
      if ( dispatchTable.chars[i] == c )
      {
         ret = dispatchTable.dispatch[i] ( l, c ) ;
      }
   }
   return ret ;
}

enum KEY_ACTION
{
   KEY_NULL = 0,       /* NULL */
   CTRL_A = 1,         /* Ctrl+a */
   CTRL_B = 2,         /* Ctrl-b */
   CTRL_C = 3,         /* Ctrl-c */
   CTRL_D = 4,         /* Ctrl-d */
   CTRL_E = 5,         /* Ctrl-e */
   CTRL_F = 6,         /* Ctrl-f */
   CTRL_G = 7,         /* Ctrl-g */
   CTRL_H = 8,         /* Ctrl-h */
   CTRL_I = 9,         /* Ctrl+i */ // not use
   TAB = 9,            /* Tab */
   CTRL_J = 10,        /* Ctrl+j */ // not use
   CTRL_K = 11,        /* Ctrl+k */
   CTRL_L = 12,        /* Ctrl+l */
   CTRL_M = 13,        /* Ctrl+m */ // not use
   ENTER = 13,         /* Enter */
   CTRL_N = 14,        /* Ctrl-n */
   CTRL_O = 15,        /* Ctrl-o */ // not use
   CTRL_P = 16,        /* Ctrl-p */
   CTRL_Q = 17,        /* Ctrl-q */ // not use
   CTRL_R = 18,        /* Ctrl-r */
   CTRL_S = 19,        /* Ctrl-s */ // not use
   CTRL_T = 20,        /* Ctrl-t */
   CTRL_U = 21,        /* Ctrl+u */
   CTRL_V = 22,        /* Ctrl+v */ // not use
   CTRL_W = 23,        /* Ctrl+w */
   CTRL_X = 24,        /* Ctrl+x */ // not use
   CTRL_Y = 25,        /* Ctrl+y */ // not use
   CTRL_Z = 26,        /* Ctrl+z */ // not use
   ESC = 27,           /* Escape */
   BACKSPACE =  127    /* Backspace */
};

static void linenoiseAtExit(void);

#ifdef _WIN32
#ifndef STDIN_FILENO
  #define STDIN_FILENO (_fileno(stdin))
  #define STDOUT_FILENO (_fileno(stdout))
#endif

HANDLE hOut;
HANDLE hIn;
DWORD consolemode ;

static BOOLEAN s_initDisplayAttr       = FALSE ;
static WORD    s_oldDisplayAttribute   = -1 ;

//PD_TRACE_DECLARE_FUNCTION ( SDB_WIN32READ, "win32read" )
static int win32read( char * c, BOOLEAN * withCtrl )
{
    PD_TRACE_ENTRY ( SDB_WIN32READ );
    int ret = -1 ;
    DWORD foo;
    INPUT_RECORD b;
    KEY_EVENT_RECORD e;
    BOOLEAN getCtrl = FALSE ;

    while (1)
    {
        if (!ReadConsoleInput(hIn, &b, 1, &foo))
        {
           ret = 0;
           goto error;
        }
        if (!foo)
        {
           ret = 0;
           goto error;
        }

        if (b.EventType == KEY_EVENT && b.Event.KeyEvent.bKeyDown)
        {
            e = b.Event.KeyEvent;
            *c = b.Event.KeyEvent.uChar.AsciiChar;

            //if (e.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
            //{
                /* Alt+key ignored */
            //} else
            if (e.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                /* Ctrl+Key */
                switch (*c)
                {
                    case CTRL_A: // ctrl+a, move to beginning of line
                    case CTRL_B: // ctrl+b, left_arrow
                    case CTRL_C: // ctrl+c, cancel or quit
                    case CTRL_D: // ctrl+d, remove char at right of cursor
                    case CTRL_E: // ctrl+e, move to end of line
                    case CTRL_F: // ctrl+f, right_arrow
                    case CTRL_G: // ctrl+g, stop reverse search
                    case CTRL_H: // ctrl+h, backspace
                    case CTRL_K: // ctrl+k, delete from current to the end of line
                    case CTRL_L: // ctrl+l, clear the screen
                    case CTRL_M:
                    case CTRL_N: // ctrl+n, down_arrow
                    case CTRL_P: // ctrl+p, up_arrow
                    case CTRL_R: // ctrl+r, reverse search
                    case CTRL_T: // ctrl+t, swap the char at the cursor and the one before
                    case CTRL_U: // ctrl+u, delete the whole line
                    case CTRL_W:
                        ret = 1;
                        goto done;
                    default:
                        getCtrl = TRUE ;
                        switch ( e.wVirtualKeyCode )
                        {
                            case VK_LEFT :   /* left */
                                *c = 2 ;
                                ret = 1 ;
                                goto done ;
                            case VK_RIGHT :  /* right */
                                *c = 6 ;
                                ret = 1 ;
                                goto done ;
                            default :
                                break ;
                        }
                        /* Other Ctrl+KEYs ignored */
                        break;
                }
            }
            else
            {
                switch (e.wVirtualKeyCode)
                {
                    case VK_ESCAPE: /* ignore - send ctrl-c, will return -1 */
                        *c = 27;
                        ret = 1;
                        goto done;
                    case VK_RETURN:  /* enter */
                        *c = 13;
                        ret = 1;
                        goto done;
                    case VK_LEFT:   /* left */
                        *c = 2;
                        ret = 1;
                        goto done;
                    case VK_RIGHT: /* right */
                        *c = 6;
                        ret = 1;
                        goto done;
                    case VK_UP:   /* up */
                        *c = 16;
                        ret = 1;
                        goto done;
                    case VK_DOWN:  /* down */
                        *c = 14;
                        ret = 1;
                        goto done;
                    case VK_HOME:
                        *c = 1;
                        ret = 1;
                        goto done;
                    case VK_END:
                        *c = 5;
                        ret = 1;
                        goto done;
                    case VK_BACK:
                        *c = 8;
                        ret = 1;
                        goto done;
                    case VK_DELETE:
                        *c = 127;
                        ret = 1;
                        goto done;
                    default:
                        if (*c)
                        {
                            ret = 1;
                            goto done;
                        }
                }
            }
        }
    }
done:
    if ( NULL != withCtrl )
    {
       ( *withCtrl ) = getCtrl ;
    }
    PD_TRACE_EXIT ( SDB_WIN32READ ) ;
    return ret; /* Makes compiler happy */
error:
   goto done;
}
#endif

void setEchoOn()
{
   echoOn = 1;
}

void setEchoOff()
{
   echoOn = 0;
}

void setEchoChar(char c)
{
   echoChar = c;
}

int linenoiseRead( struct linenoiseState * l, char * c,
                   BOOLEAN * withCtrl = NULL )
{
   int nread ;
#ifdef _WIN32
   nread = win32read( c, withCtrl ) ;
#else
   nread = read( l->ifd, c, 1 ) ;
#endif
   return nread ;
}

int linenoiseHistoryAdd(const char *line);
static void refreshLine(struct linenoiseState *l);
static void refreshLinePrompt ( struct linenoiseState * l,
                                const char * prompt ) ;

/* Debugging macro. */
#if 0
FILE *lndebug_fp = NULL;
#define lndebug(...) \
    do { \
        if (lndebug_fp == NULL) { \
            lndebug_fp = fopen("/tmp/lndebug.txt","a"); \
            fprintf(lndebug_fp, \
            "[%d %d %d] p: %d, rows: %d, rpos: %d, max: %d, oldmax: %d\n", \
            (int)l->len,(int)l->pos,(int)l->oldpos,plen,rows,rpos, \
            (int)l->maxrows,old_rows); \
        } \
        fprintf(lndebug_fp, ", " __VA_ARGS__); \
        fflush(lndebug_fp); \
    } while (0)
#else
#define lndebug(fmt, ...)
#endif

/* ======================= Low level terminal handling ====================== */

/* Set if to use or not the multi line mode. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNSETMULTILINE, "linenoiseSetMultiLine" )
void linenoiseSetMultiLine(int ml)
{
    PD_TRACE_ENTRY ( SDB_LNSETMULTILINE );
    mlmode = ml;
    PD_TRACE_EXIT ( SDB_LNSETMULTILINE );
}

/* Return true if the terminal name is in the list of terminals we know are
 * not able to understand basic escape sequences. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_ISUNSUPPORTTERM, "isUnsupportedTerm" )
static int isUnsupportedTerm(void)
{
   PD_TRACE_ENTRY ( SDB_ISUNSUPPORTTERM );
   int ret = 0;
#ifndef _WIN32
    char *term = getenv("TERM");
    int j;

    if (term == NULL)
    {
       ret = 0;
       goto done;
    }
    for (j = 0; unsupported_term[j]; j++)
        if (!strcasecmp(term,unsupported_term[j]))
        {
            ret = 1;
            goto done;
        }
#else
   goto done; // remove warming
#endif
done:
    PD_TRACE_EXIT ( SDB_ISUNSUPPORTTERM );
    return ret;
}

/* Raw mode: 1960 magic shit. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_ENABLERAWMODE, "enableRawMode" )
static int enableRawMode(int fd)
{
   PD_TRACE_ENTRY ( SDB_ENABLERAWMODE );
   int ret = 0;
#ifndef _WIN32
    struct termios raw;

    if (!isatty(STDIN_FILENO)) goto error;
    if (!atexit_registered)
    {
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig_termios) == -1) goto error;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer.
     * We want read to return every single byte, without timeout. */
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSADRAIN,&raw) < 0) goto error;
    rawmode = 1;
#else
    REDIS_NOTUSED(fd);

    if (!atexit_registered)
    {
        /* Init windows console handles only once */
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut==INVALID_HANDLE_VALUE) goto error;

        if (!GetConsoleMode(hOut, &consolemode))
        {
            CloseHandle(hOut);
            ret = -1;
            goto error;
        }

        hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE)
        {
            CloseHandle(hOut);
            ret = -1;
            goto error;
        }

        GetConsoleMode(hIn, &consolemode);
        SetConsoleMode(hIn, 0);

        /* Cleanup them at exit */
        atexit(linenoiseAtExit);
        atexit_registered = 1;
    }

    rawmode = 1;
#endif
done:
    PD_TRACE_EXIT ( SDB_ENABLERAWMODE );
    return ret;
error:
    errno = ENOTTY;
    goto done;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_DISABLERAWMODE, "disableRawMode" )
static void disableRawMode(int fd)
{
   PD_TRACE_ENTRY ( SDB_DISABLERAWMODE );
#ifdef _WIN32
    REDIS_NOTUSED(fd);
    rawmode = 0;
#else
    /* Don't even check the return value as it's too late. */
    if (rawmode && tcsetattr(fd,TCSADRAIN,&orig_termios) != -1)
        rawmode = 0;
#endif
    PD_TRACE_EXIT ( SDB_DISABLERAWMODE );
}

/* Use the ESC [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_GETCURSORPOSITION, "getCursorPosition" )
static int getCursorPosition(int ifd, int ofd)
{
    PD_TRACE_ENTRY ( SDB_GETCURSORPOSITION );
    char buf[32] = { 0 } ;
    int cols = 0, rows = 0 ;
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4)
    {
        cols = -1;
        goto error;
    }

    /* Read the response: ESC [ rows ; cols R */
    while (i < sizeof(buf)-1)
    {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[')
    {
        cols = -1;
        goto error;
    }
    if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2)
    {
        cols = -1;
        goto error;
    }
done:
    PD_TRACE_EXIT ( SDB_GETCURSORPOSITION );
    return cols;
error:
   goto done;
}

/* Try to get the number of columns in the current terminal, or assume 80
 * if it fails. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_GETCOLUMNS, "getColumns" )
static int getColumns(int ifd, int ofd)
{
    PD_TRACE_ENTRY ( SDB_GETCOLUMNS );
    int ret = 80 ;

#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO b ;
    if (!GetConsoleScreenBufferInfo(hOut, &b))
    {
        goto error ;
    }
    ret = b.dwSize.X ;

#else
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        /* ioctl() failed. Try to query the terminal itself. */
        int start = 0 , cols = 0 ;

        /* Get the initial position so we can restore it later. */
        start = getCursorPosition(ifd,ofd);
        if (start == -1) goto error;

        /* Go to right margin and get position. */
        if (write(ofd,"\x1b[999C",6) != 6) goto error;
        cols = getCursorPosition(ifd,ofd);
        if (cols == -1) goto error;

        /* Restore position. */
        if (cols > start)
        {
            char seq[32];
            snprintf(seq,32,"\x1b[%dD",cols-start);
            if (write(ofd,seq,strlen(seq)) == -1)
            {
                /* Can't recover... */
            }
        }
        ret = cols;
    }
    else
    {
        ret = ws.ws_col;
    }

#endif // _WIN32

done:
    PD_TRACE_EXIT ( SDB_GETCOLUMNS );
    return ret ;
error:
    goto done ;
}

/* Clear the screen. Used to handle ctrl+l */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNCLEARSCREEN, "linenoiseClearScreen" )
void linenoiseClearScreen(void)
{
    PD_TRACE_ENTRY ( SDB_LNCLEARSCREEN );
#ifdef _WIN32
    system("cls");
#else
    if (write(STDOUT_FILENO,"\x1b[H\x1b[2J",7) <= 0)
    {
        /* nothing to do, just to avoid warning. */
    }
#endif
    PD_TRACE_EXIT ( SDB_LNCLEARSCREEN );
}

/* Beep, used for completion when there is nothing to complete or when all
 * the choices were already shown. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNBEEP, "linenoiseBeep" )
static void linenoiseBeep(void)
{
    PD_TRACE_ENTRY ( SDB_LNBEEP );
    fprintf(stderr, "\x7");
    fflush(stderr);
    PD_TRACE_EXIT ( SDB_LNBEEP );
}

/* ============================== Completion ================================ */

/* Free a list of completion option populated by linenoiseAddCompletion(). */
//PD_TRACE_DECLARE_FUNCTION ( SDB_FREECOMPLETIONS, "freeCompletions" )
static void freeCompletions(linenoiseCompletions *lc)
{
    PD_TRACE_ENTRY ( SDB_FREECOMPLETIONS );
    size_t i;
    for (i = 0; i < lc->len; i++)
        free(lc->cvec[i]);
    if (lc->cvec != NULL)
        free(lc->cvec);
    PD_TRACE_EXIT ( SDB_FREECOMPLETIONS );
}

/* This is an helper function for linenoiseEdit() and is called when the
 * user types the <tab> key in order to complete the string currently in the
 * input.
 *
 * The state of the editing is encapsulated into the pointed linenoiseState
 * structure as described in the structure definition. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_COMPLETELINE, "completeLine" )
static int completeLine(struct linenoiseState *ls)
{
    PD_TRACE_ENTRY ( SDB_COMPLETELINE );
    linenoiseCompletions lc = { 0, 0, NULL, NULL };
    int nread, nwritten;
    int ret = 0;
    char c = 0;

    completionCallback(ls->buf,&lc);
    if (lc.len == 0)
    {
        linenoiseBeep();
    }
    else if ( 1== lc.len || lc.fill )
    {
        char *newStr = NULL;
        if ( 1 == lc.len )
        {
            newStr = lc.cvec[0];
            c = 32;
        }
        else
        {
            newStr = lc.fill;
        }
        nwritten = snprintf( ls->buf, ls->buflen, "%s", newStr );
        ls->len = nwritten;
        ls->pos = nwritten;
        refreshLine(ls);
    }
    else
    {
        size_t stop = 0;

        while(!stop)
        {
            nread = linenoiseRead( ls, &c ) ;
            if (nread <= 0)
            {
                freeCompletions(&lc);
                ret = -1 ;
                goto error ;
            }

            switch(c)
            {
                case 9: /* tab */
                   {
                    unsigned int index = 0;
                    char tmpBuf[1] = {0};
                    struct linenoiseState saved = *ls;
                    printf("\n");
                    ls->len = 0;
                    ls->pos = 0;
                    ls->buf = tmpBuf;
                    ls->prompt = "";

                    refreshLine(ls);

                    ls->len = saved.len;
                    ls->pos = saved.pos;
                    ls->buf = saved.buf;
                    ls->prompt =saved.prompt;
                    while (index < lc.len -1 )
                    {
                        printf( "%-s\t", lc.cvec[index] );
                        index++;
                    }
                    printf( "%s\n", lc.cvec[index] );
                    fflush(stdout);

                    refreshLine(ls);
                    break;
                   }
                case 27: /* escape */
                    /* Re-show original buffer */
                    stop = 1;
                    break;
                default:
                    /* Update buffer and return */
                    stop = 1;
                    break;
             }
        }
    }

    ret = (int)c ;
done:
    freeCompletions(&lc);
    PD_TRACE_EXIT ( SDB_COMPLETELINE );
    return ret; /* Return last read character */
error:
   goto done;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYCLEAR, "linenoiseHistoryClear" )
void linenoiseHistoryClear(void)
{
    PD_TRACE_ENTRY ( SDB_LNHISTORYCLEAR );
    memset( history, 0, (sizeof(char*)*(history_max_len)) ) ;
    history_len = 0 ;
    PD_TRACE_EXIT ( SDB_LNHISTORYCLEAR );
}

/* Register a callback function to be called for tab-completion. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNSETCPLCALLBACK, "linenoiseSetCompletionCallback" )
void linenoiseSetCompletionCallback(linenoiseCompletionCallback *fn)
{
    PD_TRACE_ENTRY ( SDB_LNSETCPLCALLBACK );
    completionCallback = fn;
    PD_TRACE_EXIT ( SDB_LNSETCPLCALLBACK );
}

/* This function is used by the callback function registered by the user
 * in order to add completion options given the input string when the
 * user typed <tab>. See the example.c source code for a very easy to
 * understand example. */
 //PD_TRACE_DECLARE_FUNCTION ( SDB_LNADDCPL, "linenoiseAddCompletion" )
void linenoiseAddCompletion(linenoiseCompletions *lc, const char *str)
{
    PD_TRACE_ENTRY ( SDB_LNADDCPL );
    size_t len = strlen(str);
    char *copy, **cvec;

    copy = (char *)malloc(len+1);
    if (copy == NULL)
    {
       goto error;
    }
    memcpy(copy,str,len+1);
    cvec = (char**)realloc(lc->cvec,sizeof(char*)*(lc->len+1));
    if (cvec == NULL)
    {
        free(copy);
        goto done;
    }
    lc->cvec = cvec;
    lc->cvec[lc->len++] = copy;
done:
    PD_TRACE_EXIT ( SDB_LNADDCPL );
    return;
error:
    goto done;
}

/* =========================== Line editing ================================= */

/* We define a very simple "append buffer" structure, that is an heap
 * allocated string where we can append to. This is useful in order to
 * write all the escape sequences in a buffer and flush them to the standard
 * output in a single call, to avoid flickering effects. */
struct abuf
{
    char *b;
    int len;
};

 //PD_TRACE_DECLARE_FUNCTION ( SDB_ABINIT, "abInit" )
static void abInit(struct abuf *ab)
{
    PD_TRACE_ENTRY ( SDB_ABINIT );
    ab->b = NULL;
    ab->len = 0;
    PD_TRACE_EXIT ( SDB_ABINIT );
}

 //PD_TRACE_DECLARE_FUNCTION ( SDB_ABAPPEND, "abAppend" )
static void abAppend(struct abuf *ab, const char *s, int len)
{
    PD_TRACE_ENTRY ( SDB_ABAPPEND );
    char *newone = (char *)realloc(ab->b,ab->len+len);

    if (newone == NULL) return;

    memcpy(newone+ab->len,s,len);
    ab->b = newone;
    ab->len += len;
    PD_TRACE_EXIT ( SDB_ABAPPEND );
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_ABFREE, "abFree" )
static void abFree(struct abuf *ab)
{
    PD_TRACE_ENTRY ( SDB_ABFREE );
    free(ab->b);
    PD_TRACE_EXIT ( SDB_ABFREE );
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_CALCHIGHLIGHTPOS, "calcHighLightPos" )
static int calcHighLightPos( struct linenoiseState *l )
{
    PD_TRACE_ENTRY ( SDB_CALCHIGHLIGHTPOS );
    int len = l->len ;
    int pos = l->pos ;
    char *buffer = l->buf ;
    int highlight_pos = -1 ;

    // find out the place to high light
    if ( ( !l->remove_col ) && ( pos < len ) )
    {
       int direction = 0 ;
       if ( strchr( "}])", buffer[pos] ) )
          direction = -1 ;
       else if ( strchr( "{[(", buffer[pos] ) )
          direction = 1 ;

       if ( direction )
       {
          int flag = direction ;
          for ( int i = pos + direction ; i >= 0 && i < len ; i += direction )
          {
             if ( strchr( "}])", buffer[i] ) )
                --flag ;
             else if ( strchr( "{[(", buffer[i] ) )
                ++flag ;

             if ( flag == 0 )
             {
                highlight_pos = i ;
                break ;
             }
          }
       }
    }

    l->remove_col = false ;
    PD_TRACE_EXIT ( SDB_CALCHIGHLIGHTPOS );
    return highlight_pos ;
}

#ifdef _WINDOWS

//PD_TRACE_DECLARE_FUNCTION ( SDB_SETDISPLAYATTR, "setDisplayAttribute" )
static int setDisplayAttribute( bool enhancedDisplay,
                                COORD dwCoord,
                                int length )
{
    PD_TRACE_ENTRY ( SDB_SETDISPLAYATTR );
    WORD attr = 0 ;
    DWORD lp = 0 ;
    WORD *pAttrs = &attr ;
    int ret = -1 ;

    if ( enhancedDisplay )
    {
        CONSOLE_SCREEN_BUFFER_INFO inf ;
        GetConsoleScreenBufferInfo( hOut, &inf ) ;
        s_oldDisplayAttribute = inf.wAttributes ;
        s_initDisplayAttr     = TRUE ;

        BYTE oldLowByte = s_oldDisplayAttribute & 0xFF ;
        BYTE newLowByte = 0 ;
        switch ( oldLowByte )
        {
        case 0x07:
            // most similar to xterm appearance
            newLowByte = FOREGROUND_BLUE | FOREGROUND_GREEN ;
            break;
        case 0x70:
            newLowByte = BACKGROUND_BLUE | BACKGROUND_INTENSITY ;
            break;
        default:
            newLowByte = oldLowByte ^ 0xFF;     // default to inverse video
            break ;
        }
        attr = ( s_oldDisplayAttribute & 0xFF00 ) | newLowByte ;
    }
    else
    {
        attr = s_oldDisplayAttribute ;
    }

    if ( FALSE == s_initDisplayAttr )
    {
        ret = 0;
        goto error;
    }

    if ( length > 1 )
    {
        pAttrs = new WORD[ length ] ;
        if ( !pAttrs )
        {
            ret = -1;
            goto error;
        }
        for ( int i = 0 ; i < length ; ++i )
        {
            pAttrs[ i ] = attr ;
        }
    }

    if ( WriteConsoleOutputAttribute( hOut, pAttrs, length, dwCoord, &lp ) )
    {
        ret = 0 ;
    }
    if ( pAttrs && pAttrs != &attr )
    {
      delete pAttrs ;
    }
done:
    PD_TRACE_EXIT ( SDB_SETDISPLAYATTR );
    return ret ;
error:
    goto done;
}

#endif // _WINDOWS

/* Single line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_REFRESHSINGLELINE, "refreshSingleLine" )
static void refreshSingleLine(struct linenoiseState *l, const char *prompt)
{
    PD_TRACE_ENTRY ( SDB_REFRESHSINGLELINE );
    char seq[64] = { 0 } ;
    l->cols = getColumns( STDIN_FILENO, STDOUT_FILENO ) ;
    size_t plen = strlen( prompt ) ;
    int fd = l->ofd ;
    char *buf = l->buf ;
    size_t len = l->len ;
    size_t pos = l->pos ;
    struct abuf ab ;
    BOOLEAN moveLeft = l->remove_col ? TRUE : FALSE ;
    int highlightPos = calcHighLightPos( l ) ;

    if ( plen >= l->cols )
    {
       // not enough room
       len = 0 ;
       plen = l->cols ;
    }

    while( ( plen+pos ) >= l->cols && len > 0 )
    {
        buf++;
        len--;
        pos--;

        if ( highlightPos >= 0 )
        {
            --highlightPos ;
        }
    }
    while ( plen+len > l->cols && len > 0 )
    {
        len--;
    }

#ifndef _WIN32
    REDIS_NOTUSED( moveLeft ) ;
    abInit(&ab) ;
    /* Cursor to left edge */
    snprintf(seq,64,"\x1b[0G");
    abAppend(&ab,seq,strlen(seq));
    /* Write the prompt and the current buffer content */
    abAppend(&ab,prompt,plen);

    if ( -1 == highlightPos )
    {
       if ( len > 0 )
       {
          abAppend(&ab,buf,len);
       }
    }
    else
    {
       if ( len > 0 )
       {
          abAppend( &ab, l->buf, highlightPos ) ;
          setDisplayAttribute( true, &ab ) ;
          abAppend( &ab, l->buf + highlightPos, 1 ) ;
          setDisplayAttribute( false, &ab ) ;
          abAppend( &ab, l->buf + highlightPos + 1, len - highlightPos - 1 ) ;
       }
    }
    /* Erase to right */
    snprintf(seq,64,"\x1b[0K");
    abAppend(&ab,seq,strlen(seq));
    /* Move cursor to original position. */
    snprintf(seq,64,"\x1b[0G\x1b[%dC", (int)(pos+plen));
    abAppend(&ab,seq,strlen(seq));
    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    abFree(&ab);
#else
    REDIS_NOTUSED( seq ) ;
    REDIS_NOTUSED( fd ) ;
    REDIS_NOTUSED( ab ) ;

    DWORD pl, w ;
    CONSOLE_SCREEN_BUFFER_INFO b ;
    COORD coord ;

    /* Get buffer console info */
    if (!GetConsoleScreenBufferInfo(hOut, &b)) return;
    /* Erase Line */
    coord.X = 0;
    coord.Y = b.dwCursorPosition.Y ;
    setDisplayAttribute( false, coord, b.dwSize.X ) ;
    FillConsoleOutputCharacterA( hOut, ' ', b.dwSize.X, coord, &w ) ;

    if ( l->pos == 0 || moveLeft )
    {
      SetConsoleCursorPosition( hOut, coord ) ;
    }

    /* Write prompt */
    WriteConsoleOutputCharacter( hOut, prompt, plen, coord, &pl ) ;

    coord.X = plen ;

    /* set high light display */
    if ( highlightPos != -1 )
    {
        COORD highCoord ;
        highCoord.X = highlightPos + plen ;
        highCoord.Y = coord.Y ;
        setDisplayAttribute( true, highCoord, 1 ) ;
    }

    /* Write content */
    WriteConsoleOutputCharacter( hOut, buf, len, coord, &pl ) ;
    if ( !moveLeft )
    {
        /* Set Cursor */
        coord.X = (int)( pos + plen ) ;
        SetConsoleCursorPosition( hOut, coord ) ;
    }

#endif
    PD_TRACE_EXIT ( SDB_REFRESHSINGLELINE );
}

/* Multi line low level line refresh.
 *
 * Rewrite the currently edited line accordingly to the buffer content,
 * cursor position, and number of columns of the terminal. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_REFRESHMULTILINE, "refreshMultiLine" )
static void refreshMultiLine(struct linenoiseState *l, const char *prompt)
{
    PD_TRACE_ENTRY ( SDB_REFRESHMULTILINE );
    char seq[64] = { 0 } ;
    int cur_column = 0 ;
    int old_cols = l->cols ;
    l->cols = getColumns( STDIN_FILENO, STDOUT_FILENO ) ;
    int plen = strlen( prompt ) ;
    /* rows used by current buf. */
    int rows = ( plen+l->len+l->cols - 1 ) / l->cols ;
    /* cursor relative row. for Y */
    int rpos = ( plen+l->oldpos+old_cols ) /old_cols ;
    /* rpos after refresh. */
    int rpos2 = ( plen + l->pos + l->cols ) / l->cols ;
    int old_rows = l->maxrows ;
    size_t pos = l->pos ;
    int fd = l->ofd ;
    struct abuf ab ;
    BOOLEAN moveLeft = l->remove_col ? TRUE : FALSE ;
    int highlight_pos = calcHighLightPos( l ) ;

    if ( !echoOn )
    {
        return ;
    }

    /* Update maxrows if needed. */
    if ( rows > (int)l->maxrows ) l->maxrows = rows ;

#ifndef _WIN32

    /* First step: clear all the lines used before. To do so start by
     * going to the last row. */
    abInit(&ab);
    if ( old_rows - rpos > 0 )
    {
        lndebug("go down %d", old_rows - rpos ) ;
        snprintf( seq,64,"\x1b[%dB", old_rows - rpos ) ;
        abAppend( &ab,seq,strlen(seq) ) ;
    }

    /* Now for every row clear it, go up. */
    for (int j = 0; j < old_rows-1 ; j++)
    {
        lndebug("clear+up");
        snprintf(seq,64,"\x1b[0G\x1b[0K\x1b[1A");
        abAppend(&ab,seq,strlen(seq));
    }

    /* Clean the top line. */
    lndebug("clear");
    snprintf(seq,64,"\x1b[0G\x1b[0K");
    abAppend(&ab,seq,strlen(seq));

    /* Write the prompt and the current buffer content */
    abAppend(&ab,prompt,plen);

    if ( !echoOn )
    {
      /// not append
    }
    else if ( echoChar != 0 )
    {
       for ( unsigned int i = 0 ; i < l->len ; ++i )
       {
          abAppend( &ab, &echoChar, 1 ) ;
       }
    }
    else
    {
       if ( -1 == highlight_pos )
       {
           abAppend(&ab,l->buf,l->len ) ;
       }
       else
       {
           abAppend( &ab, l->buf, highlight_pos ) ;
           setDisplayAttribute( true, &ab ) ;
           abAppend( &ab, l->buf + highlight_pos, 1 ) ;
           setDisplayAttribute( false, &ab ) ;
           abAppend( &ab, l->buf + highlight_pos + 1, l->len - highlight_pos - 1 ) ;
       }
    }

    /* If we are at the very end of the screen with our prompt, we need to
     * emit a newline and move the prompt to the first column. */
    if ( pos && pos == l->len && ( pos + plen ) % l->cols == 0 )
    {
        lndebug("<newline>");
        abAppend(&ab,"\n",1);
        snprintf(seq,64,"\x1b[0G");
        abAppend(&ab,seq,strlen(seq));
        rows++ ;
        if (rows > (int)l->maxrows)
        {
            l->maxrows = rows ;
        }
    }

    /* Move cursor to right position. */
    lndebug("rpos2 %d", rpos2) ;

    /* Go up till we reach the expected positon. */
    if ( rows - rpos2 > 0 && !moveLeft )
    {
        lndebug("go-up %d", rows-rpos2);
        snprintf(seq,64,"\x1b[%dA", rows-rpos2);
        abAppend(&ab,seq,strlen(seq));
    }

    /* Set column. */
    if ( moveLeft )
    {
        cur_column = 1 + plen % (int)l->cols ;
    }
    else
    {
        if ( !echoOn )
        {
            cur_column = 1+((plen) % (int)l->cols) ;
        }
        else
        {
            cur_column = 1+((plen+(int)pos) % (int)l->cols) ;
        }
    }
    lndebug("set col %d", cur_column ) ;
    snprintf(seq,64,"\x1b[%dG", cur_column ) ;
    abAppend(&ab,seq,strlen(seq));

    lndebug("\n") ;
    l->oldpos = pos ;

    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    abFree(&ab);

    PD_TRACE_EXIT ( SDB_REFRESHMULTILINE );
    return;
#else
    REDIS_NOTUSED( seq ) ;
    REDIS_NOTUSED( fd ) ;
    REDIS_NOTUSED( ab ) ;

    int writeLen = 0 ;
    DWORD pl = 0, w = 0 ;
    CONSOLE_SCREEN_BUFFER_INFO b ;
    COORD coord ;
    SHORT y = 0 ;

    SMALL_RECT srcScrollRect ;
    CHAR_INFO chiFill ;
    COORD coordDest ;

    if ( !GetConsoleScreenBufferInfo( hOut, &b ) )
    {
        goto error ;
    }

    srcScrollRect.Top = 1 ;
    srcScrollRect.Bottom = b.dwSize.Y - 1 ;
    srcScrollRect.Left = 0 ;
    srcScrollRect.Right = b.dwSize.X - 1 ;

    chiFill.Attributes = b.wAttributes ;
    chiFill.Char.AsciiChar = ' ' ;

    coordDest.X = 0 ;
    coordDest.Y = 0 ;

    /* Clear the contents from the last line up to the top */
    coord.X = 0 ;

    if( b.dwCursorPosition.Y )
    {
      y = b.dwCursorPosition.Y - rpos + 1 ;
    }
    else
    {
      /* If the screen has been cleaned up,
       *  need to refresh the content from the first line.
       */
      y = 0 ;
    }
    for ( int i = 0 ; i < old_rows - 1 ; ++i )
    {
        // in windows, we need to minus 1, because (X, Y) start from (0, 0)
        coord.Y = y + old_rows - 1 - i ;
        setDisplayAttribute( false, coord, b.dwSize.X ) ;
        FillConsoleOutputCharacterA( hOut, ' ', b.dwSize.X, coord, &w ) ;
    }

    // clear the top line
    coord.Y = y ;
    setDisplayAttribute( false, coord, b.dwSize.X ) ;
    FillConsoleOutputCharacterA( hOut, ' ', b.dwSize.X, coord, &w ) ;

    if ( pos == 0 ||
         ( plen + (int)pos ) % (int)l->cols == 0 ||
         moveLeft )
    {
        // Move cursor to the left edge
        SetConsoleCursorPosition( hOut, coord ) ;
    }
    int beginY = coord.Y ;

    /* Write prompt */
    int writeLineNum = 0 ;
    writeLen = 0 ;
    while ( writeLen < plen )
    {
        writeLineNum = plen - writeLen > l->cols ? l->cols : plen - writeLen ;
        if ( !WriteConsoleOutputCharacter( hOut, prompt + writeLen,
                                           writeLineNum, coord, &pl ) )
        {
            goto error ;
        }
        writeLen += writeLineNum ;
        if ( writeLineNum == l->cols )
        {
            ++coord.Y ;
            coord.X = 0 ;
            if ( coord.Y == b.dwSize.Y )
            {
               ScrollConsoleScreenBuffer( hOut, &srcScrollRect, NULL,
                                          coordDest, &chiFill ) ;
               --coord.Y ;
               --y ;
               --beginY ;
               setDisplayAttribute( false, coord, b.dwSize.X ) ;
               FillConsoleOutputCharacterA( hOut, ' ', b.dwSize.X, coord, &w ) ;
            }
        }
        else
        {
            coord.X = writeLineNum % l->cols ;
        }
    }

    /* Write content */
    writeLen = 0 ;
    while ( writeLen < l->len )
    {
        writeLineNum = ( l->len - writeLen > l->cols - coord.X ) ?
                       l->cols - coord.X :
                       l->len - writeLen ;
        if ( !WriteConsoleOutputCharacter( hOut, l->buf + writeLen,
                                           writeLineNum, coord, &pl ) )
        {
            goto error;
        }
        writeLen += writeLineNum ;
        if ( writeLineNum == l->cols - coord.X )
        {
            ++coord.Y ;
            coord.X = 0 ;
            if ( coord.Y == b.dwSize.Y )
            {
               ScrollConsoleScreenBuffer( hOut, &srcScrollRect, NULL,
                                          coordDest, &chiFill ) ;
               --coord.Y ;
               --y ;
               --beginY ;
               setDisplayAttribute( false, coord, b.dwSize.X ) ;
               FillConsoleOutputCharacterA( hOut, ' ', b.dwSize.X, coord, &w ) ;
            }
        }
        else
        {
            coord.X = ( coord.X + writeLineNum ) % l->cols ;
        }
    }

    /* set highlight display */
    if ( -1 != highlight_pos )
    {
        COORD highCoord ;
        highCoord.X = ( highlight_pos + plen ) % l->cols ;
        highCoord.Y = beginY - 1 + ( plen + highlight_pos + l->cols ) / l->cols ;
        setDisplayAttribute( true, highCoord, 1 ) ;
    }

    /* If we are at the very end of the screen with our prompt, we need to
     * emit a newline and move the prompt to the first column. */
    if ( pos && pos == l->len && ( pos+plen ) % l->cols == 0 )
    {
        rows++ ;
        if ( rows > (int)l->maxrows )
        {
            l->maxrows = rows ;
        }
    }

    /* After display the contents, we should put the cursor to the right
     * place */
    if ( !moveLeft )
    {
        coord.Y = y + rpos2 - 1 ;
        // In windows, (X, Y) coordinate start from top left corner (0, 0)
        // X = 0 is the first column
        coord.X = ( plen + (int)pos ) % (int)l->cols ;
    }
    else
    {
        coord.X = plen % (int)l->cols ;
    }
    SetConsoleCursorPosition( hOut, coord ) ;

    /* record the position for next refresh */
    l->oldpos = pos ;

done:
    PD_TRACE_EXIT ( SDB_REFRESHMULTILINE );
    return;
error:
    goto done;
#endif
}

/* Calls the two low level functions refreshSingleLine() or
 * refreshMultiLine() according to the selected mode. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_REFRESHLINE, "refreshLine" )
static void refreshLine(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_REFRESHLINE ) ;
    refreshLinePrompt( l, l->prompt ) ;
    PD_TRACE_EXIT ( SDB_REFRESHLINE ) ;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_REFRESHLINEPROMPT, "refreshLinePrompt" )
static void refreshLinePrompt ( struct linenoiseState * l, const char * prompt )
{
   PD_TRACE_ENTRY( SDB_REFRESHLINEPROMPT ) ;

   if ( mlmode )
   {
       refreshMultiLine( l, prompt ) ;
   }
   else
   {
       refreshSingleLine( l, prompt ) ;
   }

   PD_TRACE_EXIT( SDB_REFRESHLINEPROMPT ) ;
}

/* Insert the character 'c' at cursor current position.
 *
 * On error writing to the terminal -1 is returned, otherwise 0. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITINSERT, "linenoiseEditInsert" )
int linenoiseEditInsert(struct linenoiseState *l, char c)
{
    PD_TRACE_ENTRY ( SDB_LNEDITINSERT );

#ifndef _WINDOWS
    int ret = 0 ;
#endif // _WINDOWS

    if (l->len < l->buflen)
    {
        if (l->len == l->pos)
        {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';

#ifndef _WINDOWS
            if ((!mlmode && l->plen+l->len < l->cols) )
            {
                /* Avoid a full update of the line in the
                 * trivial case. */
                if ( write( l->ofd, &c, 1 ) == -1 )
                {
                    ret = -1;
                    goto error;
                }
            }
            else
            {
#endif // _WINDOWS
                refreshLine(l) ;
#ifndef _WINDOWS
            }
#endif // _WINDOWS
        }
        else
        {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }

#ifndef _WINDOWS
done:
    PD_TRACE_EXIT ( SDB_LNEDITINSERT );
    return ret;
error:
    goto done;
#else
    return 0 ;
#endif // _WINDOWS
}

/* Move cursor on the left. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVELEFT, "linenoiseEditMoveLeft" )
void linenoiseEditMoveLeft(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITMOVELEFT );
    if (l->pos > 0)
    {
        l->pos--;
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITMOVELEFT );
}

/* Move cursor on the right. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVERIGHT, "linenoiseEditMoveRight" )
void linenoiseEditMoveRight(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITMOVERIGHT );
    if (l->pos != l->len)
    {
        l->pos++;
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITMOVERIGHT );
}

/* Move cursor to the last word. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVELASTWORD, "linenoiseEditMoveLastWord" )
void linenoiseEditMoveLastWord( struct linenoiseState *l )
{
   PD_TRACE_ENTRY ( SDB_LNEDITMOVELASTWORD ) ;
   char * buf = l->buf ;
   if ( l->pos > 0 && isalnum( buf[l->pos] ) && ( ( !isalnum( buf[l->pos-1] ) ) ) )
   {
      l->pos-- ;
   }
   while ( l->pos > 0 && !isalnum( buf[l->pos] ) )
   {
      l->pos-- ;
   }
   while ( l->pos > 0 && isalnum( buf[l->pos] ) )
   {
      l->pos-- ;
   }
   if ( l->pos != 0 )
   {
      l->pos++ ;
   }
   refreshLine( l ) ;
   PD_TRACE_EXIT ( SDB_LNEDITMOVELASTWORD ) ;
}

/* Move cursor to the next word. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVENEXTWORD, "linenoiseEditMoveNextWord" )
void linenoiseEditMoveNextWord( struct linenoiseState *l )
{
   PD_TRACE_ENTRY ( SDB_LNEDITMOVENEXTWORD) ;
   char * buf = l->buf ;
   while ( l->pos != l->len && !isalnum( buf[l->pos] ) )
   {
      l->pos++ ;
   }
   while ( l->pos != l->len && isalnum( buf[l->pos] ) )
   {
      l->pos++ ;
   }
   refreshLine( l ) ;
   PD_TRACE_EXIT ( SDB_LNEDITMOVENEXTWORD ) ;
}

/* Move cursor to the start of the line. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVEHOME, "linenoiseEditMoveHome" )
void linenoiseEditMoveHome(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITMOVEHOME );
    if (l->pos != 0)
    {
        l->pos = 0;
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITMOVEHOME );
}

/* Move cursor to the end of the line. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITMOVEEND, "linenoiseEditMoveEnd" )
void linenoiseEditMoveEnd(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITMOVEEND );
    if (l->pos != l->len)
    {
        l->pos = l->len;
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITMOVEEND );
}

/* Substitute the currently edited line with the next or previous history
 * entry as specified by 'dir'. */
#define LINENOISE_HISTORY_NEXT 0
#define LINENOISE_HISTORY_PREV 1
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITHISTORYNEXT, "linenoiseEditHistoryNext" )
void linenoiseEditHistoryNext(struct linenoiseState *l, int dir)
{
    PD_TRACE_ENTRY ( SDB_LNEDITHISTORYNEXT );
    if (history_len > 1)
    {
        /* Update the current history entry before to
         * overwrite it with the next one. */
        free(history[history_len - 1 - l->history_index]);
        history[history_len - 1 - l->history_index] = strdup(l->buf);
        /* Show the new entry */
        l->history_index += (dir == LINENOISE_HISTORY_PREV) ? 1 : -1;
        if (l->history_index < 0)
        {
            l->history_index = 0;
            goto done;
        }
        else if (l->history_index >= history_len)
        {
            l->history_index = history_len-1;
            goto done;
        }
        strncpy(l->buf,history[history_len - 1 - l->history_index],l->buflen);
        l->buf[l->buflen-1] = '\0';
        l->len = l->pos = strlen(l->buf);
        refreshLine(l);
    }
done:
    PD_TRACE_EXIT ( SDB_LNEDITHISTORYNEXT );
    return;
}

/* Delete the character at the right of the cursor without altering the cursor
 * position. Basically this is what happens with the "Delete" keyboard key. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITDELETE, "linenoiseEditDelete" )
void linenoiseEditDelete(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITDELETE );
    if (l->len > 0 && l->pos < l->len)
    {
        memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITDELETE );
}

/* Backspace implementation. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITBACKSPACE, "linenoiseEditBackspace" )
void linenoiseEditBackspace(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITBACKSPACE );
    if (l->pos > 0 && l->len > 0)
    {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
    PD_TRACE_EXIT ( SDB_LNEDITBACKSPACE );
}

/* Delete the previosu word, maintaining the cursor at the start of the
 * current word. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDITDELPREVWORD, "linenoiseEditDeletePrevWord" )
void linenoiseEditDeletePrevWord(struct linenoiseState *l)
{
    PD_TRACE_ENTRY ( SDB_LNEDITDELPREVWORD );
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
    PD_TRACE_EXIT ( SDB_LNEDITDELPREVWORD );
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_LNREVINCSEARCH, "linenoiseReverseIncrementalSearch" )
int linenoiseReverseIncrementalSearch ( struct linenoiseState * l )
{
   PD_TRACE_ENTRY( SDB_LNREVINCSEARCH ) ;

   char search_buf[ LINENOISE_MAX_LINE ] ;
   char search_prompt[ LINENOISE_MAX_LINE ] ;
   int search_len = 0 ;
   int search_pos = history_len - 1 - l->history_index ;
   int search_dir = -1 ;
   char * prompt = NULL ;
   int has_match = 1 ;

   // backup of current input
   char * buf = NULL ;
   {
      size_t len = 1 + strlen( l->buf ) ;
      buf = (char *)malloc( len ) ;
      if ( NULL == buf )
      {
         return 0 ;
      }
      memcpy( buf, l->buf, len ) ;
   }

   search_buf[ 0 ] = 0 ;

   while ( TRUE )
   {
      if ( !has_match )
      {
         prompt = "(failed-reverse-i-search)`%s': " ;
      }
      else
      {
         prompt = "(reverse-i-search)`%s': " ;
      }
      if ( !snprintf( search_prompt, sizeof( search_prompt ), prompt,
                      search_buf ) )
      {
         linenoiseBeep() ;
         break ;
      }
      else
      {
         // crop
         search_prompt[ sizeof( search_prompt ) - 1 ] = 0 ;
      }

      l->pos = 0 ;
      refreshLinePrompt( l, search_prompt ) ;

      char c ;
      int new_char = 0 ;
      int nread = 0 ;

      nread = linenoiseRead( l, &c ) ;
      if ( nread <= 0 )
      {
         l->pos = l->len = snprintf( l->buf, l->buflen, "%s", buf ) ;
         l->buf[ l->buflen - 1 ] = 0 ;
         refreshLine( l ) ;
         free( buf ) ;
         return 0 ;
      }

      switch ( c )
      {
         case BACKSPACE :
         case CTRL_H :
         {
            if ( search_len > 0 )
            {
               search_buf[ --search_len ] = 0 ;
            }
            else
            {
               linenoiseBeep() ;
            }
            break ;
         }
         case CTRL_N :
         case CTRL_R :
         {
            if ( search_pos >= history_len )
            {
               // restart
               search_pos = history_len - 1 ;
            }
            else if ( search_pos > 0 )
            {
               // move next
               search_pos -- ;
            }
            search_dir = -1 ;
            break ;
         }
         case CTRL_P :
         {
            if ( search_pos < 0 )
            {
               // restart
               search_pos = 0 ;
            }
            else if ( search_pos < history_len - 1 )
            {
               // move previous
               search_pos ++ ;
            }
            search_dir = 1 ;
            break ;
         }
         case CTRL_C :
         {
            // cancel searching
            l->remove_col = true ;
            l->pos = l->len ;
            refreshLine( l ) ;
            free( buf ) ;
            return -1 ;
         }
         case ESC :
         case CTRL_G :
         {
            // exit searching and return current line
            // but in escape sequence
            refreshLine( l ) ;
            free( buf ) ;
            return ESC == c ? -2 : 0 ;
         }
         case ENTER :
         {
            // found history and apply
            l->remove_col = true ;
            l->pos = l->len ;
            refreshLine( l ) ;
            history_len-- ;
            free( history[ history_len ] );
            free( buf ) ;
            return (int)l->len ;
         }
         case CTRL_A :
         case CTRL_B :
         case CTRL_D :
         case CTRL_E :
         case CTRL_F :
         case CTRL_I :
         case CTRL_J :
         case CTRL_K :
         case CTRL_L :
         case CTRL_O :
         case CTRL_Q :
         case CTRL_S :
         case CTRL_T :
         case CTRL_U :
         case CTRL_V :
         case CTRL_W :
         case CTRL_X :
         case CTRL_Y :
         case CTRL_Z :
         {
            // exit searching and return current line
            refreshLine( l ) ;
            free( buf ) ;
            return 0 ;
         }
         default :
         {
            if ( c >= ' ' )
            {
               new_char = 1 ;
               search_buf[ search_len ] = c ;
               search_buf[ ++search_len ] = 0 ;
            }
            break ;
         }
      }

      has_match = 0 ;

      if ( strlen( search_buf ) > 0 )
      {
         for ( ;
               search_pos >= 0 && search_pos < history_len ;
               search_pos += search_dir )
         {
            if ( strstr( history[ search_pos ], search_buf ) )
            {
               has_match = 1 ;
               l->len = snprintf( l->buf, l->buflen, "%s",
                                  history[ search_pos ] ) ;
               l->history_index = history_len - 1 - search_pos ;
               break ;
            }
         }

         if ( !has_match )
         {
            linenoiseBeep() ;
            // forbid writes if the line is too long
            if ( search_len > 0 && new_char &&
                 search_len + 1 >= (int)( sizeof( search_buf ) ) )
            {
               search_buf[ --search_len ] = 0 ;
            }
         }
      }
   }

   free( buf ) ;

   PD_TRACE_EXIT( SDB_LNREVINCSEARCH ) ;

   return 0 ;
}

// ESC [ 1 ; 5 C
static int ctrlRightArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveNextWord( l ) ;
   return 0 ;
}

// ESC [ 1 ; 5 D
static int ctrlLeftArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveLastWord( l ) ;
   return 0 ;
}

// ESC [ 1 ; 5 <more stuff>
static CharacterDispatchRoutine escLeftBracket1Semicolon3or5Routines[] =
{
   ctrlRightArrowKeyRoutine,
   ctrlLeftArrowKeyRoutine
} ;

static CharacterDispatch escLeftBracket1Semicolon3or5Dispatch =
{
   2, "CD", escLeftBracket1Semicolon3or5Routines
} ;

// ESC [ 1 ; <more stuff>
static int escLeftBracket1Semicolon5Routine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ?
          -1 : doDispatch( l, c, escLeftBracket1Semicolon3or5Dispatch ) ;
}

static CharacterDispatchRoutine escLeftBracket1SemicolonRoutines[] = {
   escLeftBracket1Semicolon5Routine
} ;

static CharacterDispatch escLeftBracket1SemicolonDispatch = {
   1, "5", escLeftBracket1SemicolonRoutines
} ;

// ESC O H ( or ESC [ 1 ~ )
static int homeKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveHome( l ) ;
   return 0 ;
}

// ESC O F ( or ESC [ 4 ~ )
static int endKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveEnd( l ) ;
   return 0 ;
}

// Handle ESC O <more stuff>
static CharacterDispatchRoutine escORoutines[] = {
   homeKeyRoutine,
   endKeyRoutine
} ;

static CharacterDispatch escODispatch =
{
   2, "HF", escORoutines
} ;

// ESC [ 3 ~
static int deleteKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditDelete( l ) ;
   return 0 ;
}

// ESC [ 1 <more stuff>
static int escLeftBracket1SemicolonRoutine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escLeftBracket1SemicolonDispatch ) ;
}

static CharacterDispatchRoutine escLeftBracket1Routines[] =
{
   homeKeyRoutine,
   escLeftBracket1SemicolonRoutine
} ;

static CharacterDispatch escLeftBracket1Dispatch =
{
   2, "~;", escLeftBracket1Routines
} ;

// ESC [ 3 <more stuff>
static CharacterDispatchRoutine escLeftBracket3Routines[] =
{
   deleteKeyRoutine
} ;

static CharacterDispatch escLeftBracket3Dispatch =
{
   1, "~", escLeftBracket3Routines
} ;

// ESC [ 4 <more stuff>
static CharacterDispatchRoutine escLeftBracket4Routines[] =
{
   endKeyRoutine
} ;

static CharacterDispatch escLeftBracket4Dispatch =
{
   1, "~", escLeftBracket4Routines
} ;

// ESC [ A
static int upArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditHistoryNext( l, LINENOISE_HISTORY_PREV ) ;
   return 0;
}

// ESC [ B
static int downArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditHistoryNext( l, LINENOISE_HISTORY_NEXT ) ;
   return 0 ;
}

// ESC [ C
static int rightArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveRight( l ) ;
   return 0 ;
}

// ESC [ D
static int leftArrowKeyRoutine( struct linenoiseState *l, char c )
{
   linenoiseEditMoveLeft( l ) ;
   return 0 ;
}

// ESC [ <digit>
static int escLeftBracket1Routine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escLeftBracket1Dispatch ) ;
}

static int escLeftBracket3Routine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escLeftBracket3Dispatch ) ;
}

static int escLeftBracket4Routine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escLeftBracket4Dispatch ) ;
}

// Handle ESC [ <more stuff>
static CharacterDispatchRoutine escLeftBracketRoutines[] = {
   upArrowKeyRoutine,
   downArrowKeyRoutine,
   rightArrowKeyRoutine,
   leftArrowKeyRoutine,
   escLeftBracket1Routine,
   escLeftBracket3Routine,
   escLeftBracket4Routine
} ;

static CharacterDispatch escLeftBracketDispatch =
{
   7, "ABCD134", escLeftBracketRoutines
} ;

// ESC dispatch
static int escLeftBracketRoutine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escLeftBracketDispatch ) ;
}

static int escORoutine( struct linenoiseState *l, char c )
{
   int nread  = linenoiseRead( l, &c ) ;
   return nread <= 0 ? -1 : doDispatch( l, c, escODispatch ) ;
}

// Handle ESC [ or O
static CharacterDispatchRoutine escRoutines[] =
{
   escLeftBracketRoutine, escORoutine
} ;

static CharacterDispatch escDispatch =
{
   2, "[O", escRoutines
} ;

/* This function is the core of the line editing capability of linenoise.
 * It expects 'fd' to be already in "raw mode" so that every key pressed
 * will be returned ASAP to read().
 *
 * The resulting string is put into 'buf' when the user type enter, or
 * when ctrl+d is typed.
 *
 * The function returns the length of the current buffer. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNEDIT, "linenoiseEdit" )
static int linenoiseEdit( int stdin_fd, int stdout_fd, char *buf,
                          size_t buflen, const char *prompt)
{
    PD_TRACE_ENTRY ( SDB_LNEDIT );
    int ret = 0;
    struct linenoiseState l;

    /* Populate the linenoise state that we pass to functions implementing
     * specific editing functionalities. */
    l.ifd = stdin_fd;
    l.ofd = stdout_fd;
    l.buf = buf;
    l.buflen = buflen;
    l.prompt = prompt;
    l.plen = strlen(prompt);
    l.oldpos = 0;
    l.pos = 0 ;
    l.len = 0;
    l.cols = getColumns( stdin_fd, stdout_fd ) ;
    l.maxrows = 0 ;
    l.history_index = 0 ;
    l.remove_col = false ;
#ifdef _WIN32
    DWORD foo ;
#endif

    /* Buffer starts empty. */
    l.buf[0] = '\0';
    l.buflen--; /* Make sure there is always space for the nulterm */

    // for escape sequence
    bool insideEseSeq = false ;

    /* The latest history entry is always our current buffer, that
     * initially is just an empty string. */
    linenoiseHistoryAdd("");

#ifdef _WIN32
    if(!WriteConsole(hOut, prompt, l.plen, &foo, NULL ) )
    {
        ret = -1;
        goto error;
    }
#else
    if (write(l.ofd,prompt,l.plen) == -1)
    {
        ret = -1;
        goto error;
    }
#endif

    while(1)
    {
        BOOLEAN withCtrl = FALSE ;
        char c = KEY_NULL ;
        if ( linenoiseRead( &l, &c, &withCtrl ) <= 0 )
        {
            ret = l.len;
            goto error;
        }

        // handle escape sequence
        // 1. [~0 ...
        // 2. OA ...

        if( insideEseSeq )
        {
            int r = doDispatch( &l, c, escDispatch ) ;
            if ( r == -1 )
            {
                ret = l.len ;
                goto error ;
            }
            insideEseSeq = false ;
            continue ;
        }

        /* Only autocomplete when the callback is set. It returns < 0 when
         * there was an error reading from fd. Otherwise it will return the
         * character that should be handled next. */
        if (c == TAB && completionCallback != NULL)
        {
            if( !linenoiseIsStdinEmpty() )
            {
               continue ;
            }
            c = completeLine(&l);
            /* Return on errors */
            if (c < 0)
            {
                ret = l.len;
                goto error;
            }
            /* Read next character when 0 */
            if (c == 0) continue;
        }

        switch(c)
        {
        case ENTER:    /* enter */
            // remove colour
            l.remove_col = true ;
            refreshLine( &l ) ;
            history_len--;
            free(history[history_len]);
            ret = (int)l.len;
            goto done;
        case CTRL_C:  /* ctrl-c */
            // remove colour
            l.remove_col = true ;
            refreshLine( &l ) ;
            errno = (l.len == 0 && 0 == strncmp(l.prompt, "> ", strlen("> ")))
                     ? EAGAIN : ECANCELED ;
            ret = -1;
            goto done;
        case BACKSPACE: /* backspace in linux and delete in windows */
#ifdef _WIN32
            /* delete in _WIN32*/
            /* win32read() will send 127 for DEL and 8 for BS and Ctrl-H */
            if (l.len > 0)
            {
                linenoiseEditDelete(&l);
            }
            else
            {
                history_len--;
                free(history[history_len]);
                ret = -1;
                goto error;
            }
            break;
#endif
        case CTRL_H:     /* ctrl-h */
            linenoiseEditBackspace(&l);
            break;
        case CTRL_D:     /* ctrl-d, remove char at right of cursor, or of the
                       line is empty, act as end-of-file. */
            if (l.len > 0)
            {
                linenoiseEditDelete(&l);
            }
            else
            {
                history_len--;
                free(history[history_len]);
                ret = -1;
                goto error;
            }
            break;
        case CTRL_T:    /* ctrl-t, swaps current character with previous. */
            if (l.pos > 0 && l.pos < l.len)
            {
                int aux = l.buf[l.pos-1];
                l.buf[l.pos-1] = l.buf[l.pos];
                l.buf[l.pos] = aux;
                if (l.pos != l.len-1) l.pos++;
                refreshLine(&l);
            }
            break;
        case CTRL_B:     /* ctrl-b */
            if ( withCtrl )
            {
                linenoiseEditMoveLastWord( &l ) ;
            }
            else
            {
                linenoiseEditMoveLeft( &l ) ;
            }
            break;
        case CTRL_F:     /* ctrl-f */
            if ( withCtrl )
            {
                linenoiseEditMoveNextWord( &l ) ;
            }
            else
            {
                linenoiseEditMoveRight(&l);
            }
            break;
        case CTRL_P:    /* ctrl-p */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_PREV);
            break;
        case CTRL_N:    /* ctrl-n */
            linenoiseEditHistoryNext(&l, LINENOISE_HISTORY_NEXT);
            break;
        case ESC:    /* escape sequence */
            insideEseSeq = true ;
            break ;
        case CTRL_U: /* Ctrl+u, delete the whole line. */
            l.buf[0] = '\0';
            l.pos = l.len = 0;
            refreshLine(&l);
            break;
        case CTRL_K: /* Ctrl+k, delete from current to end of line. */
            l.buf[l.pos] = '\0';
            l.len = l.pos;
            refreshLine(&l);
            break;
        case CTRL_A: /* Ctrl+a, go to the start of the line */
            linenoiseEditMoveHome(&l);
            break;
        case CTRL_E: /* ctrl+e, go to the end of the line */
            linenoiseEditMoveEnd(&l);
            break;
        case CTRL_L: /* ctrl+l, clear screen */
            linenoiseClearScreen();
            refreshLine(&l);
            break;
        case CTRL_W: /* ctrl+w, delete previous word */
            linenoiseEditDeletePrevWord(&l);
            break;
        case CTRL_R: /* Ctrl-r */
        {
           int res = linenoiseReverseIncrementalSearch( &l ) ;
           if ( res > 0 )
           {
              ret = res ;
              goto done ;
           }
           else if ( res == -1 )
           {
              // cancel finding
              errno = ECANCELED ;
              ret = -1 ;
              goto done ;
           }
           else if ( res == -2 )
           {
              // escape
              insideEseSeq = 1 ;
           }
           break ;
        }
        case CTRL_G: /* Ctrl-g */ // not use
        case CTRL_J: /* Ctrl+j */ // not use
        case CTRL_O: /* Ctrl-o */ // not use
        case CTRL_Q: /* Ctrl-q */ // not use
        case CTRL_S: /* Ctrl-s */ // not use
        case CTRL_V: /* Ctrl+v */ // not use
        case CTRL_X: /* Ctrl+w */ // not use
        case CTRL_Y: /* Ctrl+w */ // not use
        case CTRL_Z: /* Ctrl+w */ // not use
            break;
        default:
            if (linenoiseEditInsert(&l,c))
            {
                ret = -1;
                goto error;
            }
            break;
        }

    }
    ret = l.len;
done:
    PD_TRACE_EXIT ( SDB_LNEDIT );
    return ret;
error:
    goto done;
}

/* This special mode is used by linenoise in order to print scan codes
 * on screen for debugging / development purposes. It is implemented
 * by the linenoise_example program using the --keycodes option. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNPRINTKEYCODES, "linenoisePrintKeyCodes" )
void linenoisePrintKeyCodes(void)
{
    PD_TRACE_ENTRY ( SDB_LNPRINTKEYCODES );
    char quit[4];

    printf("Linenoise key codes debugging mode.\n"
            "Press keys to see scan codes. Type 'quit' at any time to exit.\n");
    if (enableRawMode(STDIN_FILENO) == -1)
    {
        goto error;
    }
    memset(quit,' ',4);
    while(1)
    {
        char c;
        int nread;

        nread = read(STDIN_FILENO,&c,1);
        if (nread <= 0) continue;
        memmove(quit,quit+1,sizeof(quit)-1); /* shift string to left. */
        quit[sizeof(quit)-1] = c; /* Insert current char on the right. */
        if (memcmp(quit,"quit",sizeof(quit)) == 0) break;

        printf("'%c' %02x (%d) (type quit to exit)\n",
            isprint(c) ? c : '?', (int)c, (int)c);
        printf("\x1b[0G"); /* Go left edge manually, we are in raw mode. */
        fflush(stdout);
    }
    disableRawMode(STDIN_FILENO);
done:
    PD_TRACE_EXIT ( SDB_LNPRINTKEYCODES );
    return;
error:
    goto done;
}

/* This function calls the line editing function linenoiseEdit() using
 * the STDIN file descriptor set in raw mode. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNRAW, "linenoiseRaw" )
static int linenoiseRaw(char *buf, size_t buflen, const char *prompt)
{
    PD_TRACE_ENTRY ( SDB_LNRAW );
    int ret = 0;
    int count;

    if (buflen == 0)
    {
        errno = EINVAL;
        ret = -1;
        goto error;
    }
    if (!isatty(STDIN_FILENO))
    {
        ossPrintf ( "%s\n", prompt ) ;

        /* Not a tty: read from file / pipe. */
        if (fgets(buf, buflen, stdin) == NULL )
        {
            ret = -1;
            goto error;
        }
        count = strlen(buf);
        if (count && buf[count-1] == '\n') {
            count--;
            buf[count] = '\0';
        }
    }
    else
    {
        /* Interactive editing. */
        if (enableRawMode(STDIN_FILENO) == -1)
        {
            ret = -1;
            goto error;
        }
        count = linenoiseEdit(STDIN_FILENO, STDOUT_FILENO, buf, buflen, prompt);
        disableRawMode(STDIN_FILENO);
        printf("\n");
    }
    ret = count;
done:
    PD_TRACE_EXIT ( SDB_LNRAW );
    return ret;
error:
    goto done;
}

/* The high level function that is the main API of the linenoise library.
 * This function checks if the terminal has basic capabilities, just checking
 * for a blacklist of stupid terminals, and later either calls the line
 * editing function or uses dummy fgets() so that you will be able to type
 * something even in the most desperate of the conditions. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LN, "linenoise" )
char *linenoise(const char *prompt)
{
    PD_TRACE_ENTRY ( SDB_LN );
    char buf[LINENOISE_MAX_LINE];
    char *ret = NULL;
    int count;

    if (isUnsupportedTerm())
    {
        size_t len;

        printf("%s",prompt);
        fflush(stdout);
        if (fgets(buf,LINENOISE_MAX_LINE,stdin) == NULL) goto error;
        len = strlen(buf);
        while(len && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        {
            len--;
            buf[len] = '\0';
        }
        ret = strdup(buf);
    }
    else
    {
        count = linenoiseRaw(buf,LINENOISE_MAX_LINE,prompt);
        if (count == -1)
        {
           goto error;
        }
        ret = strdup(buf);
    }
done:
    PD_TRACE_EXIT ( SDB_LN );
    return ret;
error:
    goto done;
}

/* ================================ History ================================= */

/* Free the history, but does not reset it. Only used when we have to
 * exit() to avoid memory leaks are reported by valgrind & co. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_FREEHISTORYINLINENOISE, "freeHistory" )
static void freeHistory(void)
{
    PD_TRACE_ENTRY ( SDB_FREEHISTORYINLINENOISE );
    if (history)
    {
        int j;
        for (j = 0; j < history_len; j++)
            free(history[j]);
        free(history);
    }
    PD_TRACE_EXIT ( SDB_FREEHISTORYINLINENOISE );
}

/* At exit we'll try to fix the terminal to the initial conditions. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNNOISEATEXT, "linenoiseAtExit" )
static void linenoiseAtExit(void) {
    PD_TRACE_ENTRY ( SDB_LNNOISEATEXT );
#ifdef _WIN32
    SetConsoleMode(hIn, consolemode);
    CloseHandle(hOut);
    CloseHandle(hIn);
#else
    disableRawMode(STDIN_FILENO);
#endif
    freeHistory();
    PD_TRACE_EXIT ( SDB_LNNOISEATEXT );
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYGET, "linenoiseHistoryGet" )
const char* linenoiseHistoryGet(int pos)
{
   PD_TRACE_ENTRY ( SDB_LNHISTORYGET );
   char *ret = NULL;
   if ( !history || pos >= history_len || pos < 0 )
   {
       ret = NULL;
       goto error;
   }
   ret = history[pos];
done:
   PD_TRACE_EXIT ( SDB_LNHISTORYGET );
   return ret ;
error:
   goto done;
}

/* This is the API call to add a new entry in the linenoise history.
 * It uses a fixed array of char pointers that are shifted (memmoved)
 * when the history max length is reached in order to remove the older
 * entry and make room for the new one, so it is not exactly suitable for huge
 * histories, but will work well for a few hundred of entries.
 *
 * Using a circular buffer is smarter, but a bit more complex to handle. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYADD, "linenoiseHistoryAdd" )
int linenoiseHistoryAdd(const char *line)
{
    PD_TRACE_ENTRY ( SDB_LNHISTORYADD );
    char *linecopy;
    int ret = 0;
    if (history_max_len == 0)
    {
       ret = 0;
       goto done;
    }
    /* Initialization on first call. */
    if (history == NULL)
    {
        history = (char **)malloc(sizeof(char*)*history_max_len);
        if (history == NULL)
        {
           ret = 0;
           goto error;
        }
        memset(history,0,(sizeof(char*)*history_max_len));
    }

    /* Don't add duplicated lines. */
    if (history_len && !strcmp(history[history_len-1], line))
    {
       ret = 0 ;
       goto done;
    }

    /* Add an heap allocated copy of the line in the history.
     * If we reached the max length, remove the older line. */
    linecopy = strdup(line);
    if (!linecopy)
    {
         ret = 0;
         goto error;
    }
    if (history_len == history_max_len)
    {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    ret = 1;
done:
    PD_TRACE_EXIT ( SDB_LNHISTORYADD );
    return ret;
error:
   goto done;
}

/* Set the maximum length for the history. This function can be called even
 * if there is already some history, the function will make sure to retain
 * just the latest 'len' elements if the new history length value is smaller
 * than the amount of items already inside the history. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYSETMAXLEN, "linenoiseHistorySetMaxLen" )
int linenoiseHistorySetMaxLen(int len)
{
    PD_TRACE_ENTRY ( SDB_LNHISTORYSETMAXLEN );
    char **newone;
    int ret = 0 ;
    if (len < 1)
    {
        ret = 0;
        goto error;
    }
    if (history)
    {
        int tocopy = history_len;

        newone = (char **)malloc(sizeof(char*)*len);
        if (newone == NULL)
        {
            ret = 0;
            goto error;
        }

        /* If we can't copy everything, free the elements we'll not use. */
        if (len < tocopy)
        {
            int j;
            for (j = 0; j < tocopy-len; j++) free(history[j]);
            tocopy = len;
        }
        memset(newone,0,sizeof(char*)*len);
        memcpy(newone,history+(history_len-tocopy), sizeof(char*)*tocopy);
        free(history);
        history = newone;
    }
    history_max_len = len;
    if (history_len > history_max_len)
        history_len = history_max_len;
    ret = 1;
done:
    PD_TRACE_EXIT ( SDB_LNHISTORYSETMAXLEN );
    return ret;
error:
    goto done;
}

/* Save the history in the specified file. On success 0 is returned
 * otherwise -1 is returned. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYSAVE, "linenoiseHistorySave" )
int linenoiseHistorySave(const char *filename)
{
PD_TRACE_ENTRY ( SDB_LNHISTORYSAVE );
int ret = 0;
#ifdef _WIN32
    FILE *fp = fopen(filename,"wb");
#else
    FILE *fp = fopen(filename, "w");
#endif
    int j;

    if (fp == NULL)
    {
        ret = -1;
        goto error;
    }
    for (j = 0; j < history_len; j++)
        fprintf(fp,"%s\n",history[j]);
    fclose(fp);
done:
    PD_TRACE_EXIT ( SDB_LNHISTORYSAVE );
    return ret;
error:
    goto done;
}

/* Load the history from the specified file. If the file does not exist
 * zero is returned and no operation is performed.
 *
 * If the file exists and the operation succeeded 0 is returned, otherwise
 * on error -1 is returned. */
//PD_TRACE_DECLARE_FUNCTION ( SDB_LNHISTORYLOAD, "linenoiseHistoryLoad" )
int linenoiseHistoryLoad(const char *filename)
{
    PD_TRACE_ENTRY ( SDB_LNHISTORYLOAD );
    int ret = 0 ;
    FILE *fp = fopen(filename,"r");
    char buf[LINENOISE_MAX_LINE];

    if (fp == NULL)
    {
        ret = -1;
        goto error ;
    }

    while (fgets(buf,LINENOISE_MAX_LINE,fp) != NULL)
    {
        char *p;

        p = strchr(buf,'\r');
        if (!p) p = strchr(buf,'\n');
        if (p) *p = '\0';
        linenoiseHistoryAdd(buf);
    }
    fclose(fp);
done:
    PD_TRACE_EXIT ( SDB_LNHISTORYLOAD );
    return ret;
error:
    goto done;
}

//PD_TRACE_DECLARE_FUNCTION ( SDB_SETDISPLAYATTRIBUTE, "setDisplayAttribute" )
static void setDisplayAttribute( bool enhancedDisplay, struct abuf *ab )
{
    PD_TRACE_ENTRY ( SDB_SETDISPLAYATTRIBUTE );
#ifdef _WIN32
    if ( enhancedDisplay )
    {
        CONSOLE_SCREEN_BUFFER_INFO inf;
        GetConsoleScreenBufferInfo( hOut, &inf );
        s_oldDisplayAttribute = inf.wAttributes;
        BYTE oldLowByte = s_oldDisplayAttribute & 0xFF;
        BYTE newLowByte;
        switch ( oldLowByte ) {
        case 0x07:
            //newLowByte = FOREGROUND_BLUE | FOREGROUND_INTENSITY;  // too dim
            //newLowByte = FOREGROUND_BLUE;                         // even dimmer
            newLowByte = FOREGROUND_BLUE | FOREGROUND_GREEN;        // most similar to xterm appearance
            break;
        case 0x70:
            newLowByte = BACKGROUND_BLUE | BACKGROUND_INTENSITY;
            break;
        default:
            newLowByte = oldLowByte ^ 0xFF;     // default to inverse video
            break;
        }
        inf.wAttributes = ( inf.wAttributes & 0xFF00 ) | newLowByte;
        SetConsoleTextAttribute( hOut, inf.wAttributes );
    }
    else
    {
        SetConsoleTextAttribute( hOut, s_oldDisplayAttribute );
    }
#else
    if ( enhancedDisplay )
    {
         abAppend( ab, "\x1b[1;34m", 7 ) ;
    }
    else
    {
         abAppend( ab, "\x1b[0m", 4 ) ;
    }
#endif
   PD_TRACE_EXIT ( SDB_SETDISPLAYATTRIBUTE );
}

void linenoiseClearInputBuffer( void )
{
#ifdef _WIN32
   FlushConsoleInputBuffer( hIn ) ;
#else
   tcflush( STDIN_FILENO, TCIOFLUSH ) ;
#endif
}

int linenoiseIsStdinEmpty()
{
   int ret = 1 ;
#ifdef _WIN32
   DWORD len = 0 ;
   if( GetNumberOfConsoleInputEvents( hIn, &len ) )
   {
      if( len > 0 )
      {
         ret = 0 ;
      }
   }
   else
   {
      goto error ;
   }
#else
   int len = 0 ;
   int hasEnableRawMode = rawmode ;

   if( !hasEnableRawMode )
   {
      if( 0 != enableRawMode( STDIN_FILENO ) )
      {
         goto error ;
      }
   }
   if( !ioctl( STDIN_FILENO, FIONREAD, &len ) )
   {
      if( len > 0 )
      {
         ret = 0 ;
      }
   }
   if( !hasEnableRawMode )
   {
      disableRawMode( STDIN_FILENO ) ;
   }
#endif
done:
   return ret ;
error:
   ret = 1 ;
   goto done ;
}
