/*
 * $Id: wstdio.c,v 1.1 2005-09-18 22:05:38 dhmunro Exp $
 * pl_stdinit, pl_stdout, pl_stdin for MS Windows
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "playw.h"
#include "play/std.h"

int (*_pl_w_stdinit)(void(**)(char*,long), void(**)(char*,long))= 0;
int _pl_w_no_mdi = 0;

static void (*w_on_stdin)(char *input_line)= 0;
static void w_formout(char *line, void (*func)(char *, long));

static void (*w_stdout)(char *line, long len)= 0;
static void (*w_stderr)(char *line, long len)= 0;

int console_mode = 0;

void
pl_stdinit(void (*on_stdin)(char *input_line))
{
  char *mdi = pl_getenv("NO_MDI");
  if (mdi && mdi[0] && (mdi[0]!='0' || mdi[1])) _pl_w_no_mdi |= 1;
  if (!_pl_w_no_mdi && !_pl_w_stdinit) _pl_w_no_mdi = 1, AllocConsole();
  if (!_pl_w_no_mdi || con_stdinit(&w_stdout, &w_stderr)) {
    _pl_w_stdinit(&w_stdout, &w_stderr);
  } else if (_pl_w_main_window) {
    /* without this, actual first window created does not show properly */
    ShowWindow(_pl_w_main_window, SW_SHOWNORMAL);
    ShowWindow(_pl_w_main_window, SW_HIDE);
  }
  w_on_stdin = on_stdin;
}

void pl_stdout(char *output_line)
{
  if (!w_stdout) w_stdout = con_stdout;
  w_formout(output_line, w_stdout);
}

void pl_stderr(char *output_line)
{
  if (!w_stderr) w_stderr = con_stderr;
  w_formout(output_line, w_stderr);
}

char *_pl_w_sendbuf(long len)
{
  /* console app: called by worker to get buffer to hold stdin
   * gui app: called by boss to get buffer to hold input line
   *          called by worker _pl_w_sendbuf(-1) to retrieve boss buffer
   * therefore must use raw Windows memory manager routines */
  static char *buf = 0;
  if (len >= 0) {
    HANDLE heap = GetProcessHeap();
    if (len <= 0x100) len = 0x100;
    else len = ((len-1)&0x3ff) + 0x100;
    buf = buf? HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS, buf, len+1) :
              HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, len+1);
  }
  return buf;
}

void
_pl_w_deliver(char *buf)
{
  int cr, i, j;
  for (i=j=0 ; buf[j] ; j=i) {
    for (; buf[i] ; i++) if (buf[i]=='\r' || buf[i]=='\n') break;
    cr = (buf[i]=='\r');
    buf[i++] = '\0';
    if (cr && buf[i]=='\n') i++;
    /* deliver one line at a time, not including newline */
    if (w_on_stdin) w_on_stdin(buf+j);
  }
}

static void
w_formout(char *line, void (*func)(char *, long))
{
  if (pl_signalling == PL_SIG_NONE && line) {
    static char *buf = 0;
    HANDLE heap = GetProcessHeap();
    long len = 256, j = 0, i = 0;
    char c = line[i];
    if (!buf)
      buf = HeapAlloc(heap, HEAP_GENERATE_EXCEPTIONS, 258);
    do {
      if (j >= len) {
        buf = HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS, buf, 2*len+2);
        len *= 2;
      }
      if (line[i]!='\n' || c=='\r' || console_mode) {
        if (line[i] == '\a') {
          i++;
          pl_feep(0);
          continue;
        } else if (console_mode && line[i]=='\r' && line[i+1]=='\n') {
          i++;
          continue;
        }
      } else {
        buf[j++] = '\r';
      }
      buf[j++] = c = line[i++];
    } while (c);
    func(buf, j-1);
    if (len > 256)
      buf = HeapReAlloc(heap, HEAP_GENERATE_EXCEPTIONS, buf, 258);
  }
}
