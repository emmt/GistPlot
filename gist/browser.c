/*
 * $Id: browser.c,v 1.1 2005-09-18 22:04:26 dhmunro Exp $
 * Main for GIST CGM viewer
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include <string.h>
#include "play/std.h"
#include "play/io.h"

#include "gist/xbasic.h"
#include "gist/cgm.h"

/* gp_open_cgm, gp_read_cgm, gp_cgm_relative, gp_cgm_landscape, gp_cgm_bg0fg1 in gist/cgmin.h */
#include "gist/cgmin.h"
/* GpEPSEngine declared in gist/eps.h */
#include "gist/eps.h"

extern int cgmScaleToFit;
extern int gp_do_escapes;
extern char* gp_argv0; /* in gread.c */

/* List of commands in alphabetical order, 0 terminated */
static char *commandList[]= {
  "cgm", "display", "draw", "eps", "free", "help", "info", "open",
  "ps", "quit", "send", 0 };

/* Corresponding functions */
static int MakeCGM(int help);
static int MakeX(int help);
static int Draw(int help);
static int EPS(int help);
static int FreeAny(int help);
static int Help(int help);
static int Info(int help);
static int OpenIn(int help);
static int MakePS(int help);
static int Quit(int help);
static int Send(int help);
static int Special(int help);  /* must correspond to 0 in commandList */
static int nPrefix, cSuffix;   /* arguments for Special */
typedef int (*Command)(int help);
static Command CommandList[]= {
  &MakeCGM, &MakeX, &Draw, &EPS, &FreeAny, &Help, &Info, &OpenIn,
  &MakePS, &Quit, &Send, &Special };
static int GetCommand(char *token);

int gp_cgm_is_batch= 0;

int warningCount= 0;
int no_warnings= 0;

static int CreateCGM(int device, char *name);
static int CreatePS(int device, char *name);
static int CreateX(int device, char *name);

static int GetPageGroup(char *token);
static int CheckEOL(char *command);
static void DrawSend(int ds, char *token);
static int GetDeviceList(int ds, char *command);
static int FindDevice(void);
static int SaveName(int device, char *name);
static void DoSpecial(int nPrefix, int cSuffix);

static int xPrefix= 0;
#ifndef NO_XLIB
static void HandleExpose(gp_engine_t *engine, gd_drawing_t *drauing, int xy[]);
static void HandleOther(gp_engine_t *engine, int k, int md);
#endif
static int HelpAndExit(void);
static int MessageAndExit(char *msg);

extern int on_idle(void);
extern int on_quit(void);
extern void on_stdin(char *line);
static int did_startup = 0;
extern int do_startup(void);

int defaultDPI;

char *outNames[8];
int outLengths[8];
int outDraw[8], outSend[8], outMark[8];

int nPageGroups= 0;
int mPage[32], nPage[32], sPage[32];
int n_active_groups = 0;

static int nOut, noDisplay, x_only;
static char *inName;

int
pl_on_launch(int argc, char *argv[])
{
  int i, j;
  char *arg;

  nOut= 0;
  for (i=0 ; i<8 ; i++) {
    outNames[i]= 0;
    gp_cgm_out_engines[i]= 0;
    outDraw[i]= outSend[i]= outLengths[i]= 0;
  }
  noDisplay= gp_cgm_is_batch= x_only= 0;
  defaultDPI= 100;
  inName= 0;

  pl_quitter(&on_quit);
  pl_idler(&on_idle);
  pl_stdinit(&on_stdin);
  gp_stdout = pl_stdout;

  for (i=1 ; i<argc ; i++) {
    arg= argv[i];

    if (arg[0]=='-') {
      int fileType= -1;
      arg++;
      if (strcmp(arg, "cgm")==0) {
        fileType= 0;
        i++;
        if (i>=argc) return MessageAndExit("Missing file or display name");
        arg= argv[i];
      } else if (strcmp(arg, "ps")==0) {
        fileType= 1;
        i++;
        if (i>=argc) return MessageAndExit("Missing file or display name");
        arg= argv[i];
      } else if (strcmp(arg, "in")==0) {
        i++;
        if (i>=argc) return MessageAndExit("Missing file or display name");
        if (inName) return HelpAndExit();
        else inName= argv[i];
      } else if (strcmp(arg, "display")==0 || strcmp(arg, "d")==0) {
        fileType= 2;
        i++;
        if (i>=argc) return MessageAndExit("Missing file or display name");
        arg= argv[i];
      } else if (strcmp(arg, "f")==0) {
        gp_cgm_is_batch= 1;
        fileType= 1;
        arg= "*stdout*";
      } else if (strcmp(arg, "nd")==0) noDisplay= 1;
      else if (strcmp(arg, "b")==0) gp_cgm_is_batch= 1;
      else if (strcmp(arg, "nowarn")==0) no_warnings= 1;
      else if (strcmp(arg, "geometry")==0) {
#ifndef NO_XLIB
        char *suffix;
        int w=0,h=0;
        i++;
        if (i>=argc) MessageAndExit("Missing geometry");
        arg = argv[i];
        w = (int)strtol(arg, &suffix, 10);
        if (suffix!=arg && *suffix=='x') {
          arg = suffix+1;
          h = (int)strtol(arg, &suffix, 10);
        }
        if (w < 10 || h < 10) MessageAndExit("Invalid geometry");
        gx_75dpi_width = gx_100dpi_width = w;
        gx75height = gx100height = h;
#else
        MessageAndExit("-geometry illegal, no interactive graphics");
#endif
      } else if (strcmp(arg, "75")==0) defaultDPI= 75;
      else if (strcmp(arg, "100")==0) defaultDPI= 100;
      else if (strcmp(arg, "gks")==0) {
#ifndef NO_XLIB
        gx_75dpi_width= gx75height= 600;     /* 8x8 X window size */
        gx_100dpi_width= gx100height= 800;   /* 8x8 X window size */
#endif
        cgmScaleToFit= 1;               /* 8x8 PostScript plotting area */
        gp_do_escapes= 0;
      } else if (strcmp(arg, "x")==0) x_only= 1;
      else if (strcmp(arg, "gks")==0) {
        cgmScaleToFit= 1;
        gp_do_escapes= 0;
      }
      else if (strcmp(arg, "fmbug")==0) gp_eps_fm_bug= 1;
      else if (strcmp(arg, "gp_cgm_bg0fg1")==0) gp_cgm_bg0fg1= 1;
      else if (strcmp(arg, "esc0")==0) gp_do_escapes= 0;
      else if (strcmp(arg, "esc1")==0) gp_do_escapes= 1;
      else return HelpAndExit();

      if (fileType>=0) {
        if (nOut>=8)
          return MessageAndExit("At most 8 output files/displays allowed");
        for (j=0 ; j<nOut ; j++) if (strcmp(outNames[j], arg)==0)
          return MessageAndExit("Duplicate output filenames not allowed");
        outNames[nOut]= arg;
        gp_cgm_out_types[nOut]= fileType;
        nOut++;
      }

    } else if (arg[0]<='9' && arg[0]>='0') {
      if (GetPageGroup(arg)) return MessageAndExit("Try again");

    } else if (!inName) {
      inName= arg;

    } else {
      return HelpAndExit();
    }
  }

  if (inName && gp_open_cgm(inName)) inName= 0;

  if (gp_cgm_is_batch) {
    if (!inName)
      return MessageAndExit("Must specify an input CGM file to use -b or -f");
    if (!nOut)
      return MessageAndExit("Must specify some output file to use -b");
    noDisplay= 1;
  }

  /* Create CGM and PostScript engines */
  for (i=0 ; i<nOut ; i++) {
    if (gp_cgm_out_types[i]==0) CreateCGM(i, outNames[i]);
    else if (gp_cgm_out_types[i]==1) CreatePS(i, outNames[i]);
  }

  /* If page list specified, do implied send command */
  if (gp_cgm_is_batch && nPageGroups<=0) {
    mPage[0]= 1;
    nPage[0]= 32767;
    sPage[0]= 1;
    nPageGroups= 1;
  }
  if (nPageGroups>0) {
    for (i=0 ; i<8 ; i++) {
      if (!outSend[i]) gp_deactivate(gp_cgm_out_engines[i]);
      if (outSend[i] && !gp_activate(gp_cgm_out_engines[i])) n_active_groups++;
    }
  }

#ifndef NO_XLIB
  gp_initializer(&argc, argv);
#else
  gp_argv0 = argv != NULL ? argv[0] : NULL;
#endif

  return 0;
}

int
do_startup(void)
{
  int i;
  did_startup = 1;

  if (!noDisplay) {
    int noX= 1;
    for (i=0 ; i<nOut ; i++) if (gp_cgm_out_types[i]==2) {
      if (!CreateX(i, outNames[i])) noX= 0;
    }
    if (noX && nOut<8) {
      if (!CreateX(nOut, 0)) {
        nOut++;
        noX= 0;
      }
    }
    noDisplay= noX;
  }
  if (noDisplay && x_only)
    return MessageAndExit("Must be an X display to use -x");
  if (x_only && !inName)
    return MessageAndExit("Must specify an input CGM file to use -x");

  if (n_active_groups)
    gp_read_cgm(mPage, nPage, sPage, nPageGroups);
  return 0;
}

static int need_prompt = 1;

int
on_idle(void)
{
  int flag;
  if (!did_startup && do_startup()) return 0;
  if (need_prompt && !x_only && !gp_cgm_is_batch) pl_stdout("gist> ");
  need_prompt = 0;
  flag = gp_catalog_cgm();
  if (!flag && gp_cgm_is_batch) pl_quit();
  return flag;
}

int
on_quit(void)
{
  int i;
  for (i=0 ; i<8 ; i++) {
    if (gp_cgm_out_engines[i]) {
      gp_deactivate(gp_cgm_out_engines[i]);
      gp_kill_engine(gp_cgm_out_engines[i]);
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------ */

static int HelpAndExit(void)
{
  pl_stderr("Usage:   gist [cgminput] [options] [page list]\n\n");
  pl_stderr("   options:  -in cgminput  (open cgminput for input)\n");
  pl_stderr("             -cgm cgmout   (open cgmout for output)\n");
  pl_stderr("             -ps psout     (open psout for output)\n");
  pl_stderr("             -display host:n.m  (connect to X server)\n");
  pl_stderr("             -75  (default to 75 dpi)\n");
  pl_stderr("             -100 (default to 100 dpi)\n");
  pl_stderr("             -gks (default to 8x8 inches)\n");
  pl_stderr("             -geometry WxH (initial window size in pixels)\n");
  pl_stderr("             -nd  (not even default X server)\n");
  pl_stderr("             -b   (batch mode, implies -nd)\n");
  pl_stderr("             -x   (graphics window only, no keyboard)\n");
  pl_stderr("             -nowarn (only one warning message printed)\n");
  pl_stderr("             -f   (PostScript to stdout, implies -b)\n");
  pl_stderr("             -fmbug (if EPS files are for FrameMaker)\n");
  pl_stderr("             -gp_cgm_bg0fg1 (force color 0 to bg, 1 to fg)\n");
  pl_stderr("             -esc0 (skip ^_! escapes-assumed if -gks)\n");
  pl_stderr("             -esc1 (handle ^_! text escapes-default)\n");
  pl_stderr("   page list:  [page group] [page group] ...\n");
  pl_stderr("   page group:  n     (output page n)\n");
  pl_stderr("   page group:  n-m   (output pages n->m inclusive)\n");
  pl_stderr("   page group:  n-m-s (output every s-th page, n-m)\n");
  pl_quit();
  return 1;
}

static int MessageAndExit(char *msg)
{
  pl_stderr("gist: ");
  pl_stderr(msg);
  pl_stderr("\ngist: try    gist -help    for usage\n");
  pl_quit();
  return 1;
}

void gp_cgm_warning(char *general, char *particular)
{
  if (++warningCount > 5) return;
  if (no_warnings) {
    if (no_warnings==1)
      pl_stderr("gist: (WARNING) rerun without -nowarn to see warnings");
    no_warnings= 2;
    return;
  }
  pl_stderr("gist: (WARNING) ");
  pl_stderr(general);
  pl_stderr(particular);
  pl_stderr("\n");
}

/* ------------------------------------------------------------------------ */

static int GetCommand(char *token)
{
  char **command= commandList;
  char *cmd;
  char *now= token;
  int quitOK= 0;

  if (!now) return -1;

  /* Quit has two synonyms, ? is a synonym for help, and f, b, and g
     are special command forms.  */
  if (strcmp(token, "exit")==0 || strcmp(token, "end")==0 ||
      strcmp(token, "quit")==0) {
    /* Quit and its synonyms cannot be abbreviated */
    now= "quit";
    quitOK= 1;
  } else if (strcmp(token, "?")==0) now= "help";
  else if (strcmp(token, "f")==0) now= "1f";
  else if (strcmp(token, "b")==0) now= "1b";
  else if (strcmp(token, "g")==0) now= "1g";
  else if (strcmp(token, "s")==0) now= "send";

  /* Check for nf, nb, ng */
  if (*now<='9' && *now>='0') {
    char *suffix;
    nPrefix= (int)strtol(now, &suffix, 10);
    cSuffix= *suffix;
    if ((cSuffix=='f' || cSuffix=='b' || cSuffix=='g') &&
        *(suffix+1)=='\0') {
      while (*command) command++;
      return command-commandList;
    }
  }

  cmd= *command;
  while (cmd && *now>*cmd) cmd= *(++command);
  while (cmd && *now==*cmd) {
    while (*cmd && *now==*cmd) { now++;  cmd++; }
    if (!*now) break;  /* token matches cmd */
    now= token;
    cmd= *(++command);
  }
  if (!cmd || *now) {
    pl_stderr("gist: Unknown command: ");
    pl_stderr(token);
    pl_stderr("\n      Type help for help.\n");
    return -1;
  }

  if (*cmd) {
    now= token;
    cmd= *(command+1);
    if (cmd && *now++==*cmd++) {
      while (*cmd && *now==*cmd) { now++;  cmd++; }
      if (!*now) {
        pl_stderr("gist: Ambiguous command: ");
        pl_stderr(token);
        pl_stderr("\n      Type help for help.\n");
        return -1;
      }
    }
  }

  if (strcmp(*command, "quit")==0 && !quitOK) {
    pl_stderr("gist: The quit command cannot be abbreviated.\n");
    return -1;
  }

  return command-commandList;
}

static int GetPageGroup(char *token)
{
  int n, m, s;
  char *suffix, *tok;

  s= 1;
  n= m= (int)strtol(token, &suffix, 10);
  if (suffix!=token && *suffix=='-') {
    tok= suffix+1;
    n= (int)strtol(tok, &suffix, 10);
    if (suffix!=tok && *suffix=='-') {
      tok= suffix+1;
      s= (int)strtol(tok, &suffix, 10);
      if (suffix==tok) suffix= tok-1;  /* this is an error */
    }
  }

  if (*suffix) {
    pl_stderr("gist: (SYNTAX) ");
    pl_stderr(token);
    pl_stderr(" is not a legal page number group\n");
    return 1;
  }

  if (nPageGroups>=32) {
    pl_stderr("gist: (SYNTAX) too many page number groups (32 max)\n");
    return 1;
  }

  mPage[nPageGroups]= m;
  nPage[nPageGroups]= n;
  sPage[nPageGroups]= s;
  nPageGroups++;
  return 0;
}

static int CheckEOL(char *command)
{
  if (strtok(0, " \t\n")) {
    pl_stderr("gist: (SYNTAX) garbage after ");
    pl_stderr(command);
    pl_stderr(" command\n");
    return 1;
  } else {
    return 0;
  }
}

static int Help(int help)
{
  int cmd;

  if (help) {
    pl_stderr("gist: help command syntax:\n     help [command]\n");
    pl_stderr("  Print help message (for given command).\n");
    return 0;
  }

  cmd= GetCommand(strtok(0, " \t\n"));

  if (cmd<0) {
    int len;
    char line[80], **command= commandList;
    pl_stderr("gist: Input Syntax:\n     command [arg1] [arg2] ...\n");
    strcpy(line, "  Available commands are:  ");
    len= 27;
    for (;;) {
      for (;;) {
        if (len+strlen(*command) > 72) {
          strcpy(line+len, "\n");
          pl_stderr(line);
          break;
        }
        strcpy(line+len, *command);
        len+= strlen(*command++);
        if (!*command) {
          strcpy(line+len, "\n");
          pl_stderr(line);
          break;
        }
        strcpy(line+len, ", ");
        len+= 2;
      }
      if (!*command) break;
      strcpy(line, "     ");
      len= 5;
    }
    pl_stderr("  The quit command has two synonyms:  exit, end\n");
    pl_stderr("  Any command except quit may be abbreviated by\n");
    pl_stderr("  omitting characters from the right.\n");
    pl_stderr("  The help command explains specific syntax, e.g.:\n");
    pl_stderr("     help cgm\n");
    pl_stderr("  describes the syntax of the cgm command.\n");
    pl_stderr("  Five commands can be typed to a gist X window:\n");
    pl_stderr("     nf   - forward n pages and draw (default 1)\n");
    pl_stderr("     nb   - backward n pages and draw (default 1)\n");
    pl_stderr("     ng   - go to page n and draw (default 1)\n");
    pl_stderr("     s    - send current page\n");
    pl_stderr("     q    - quit\n");

  } else {
    CommandList[cmd](1);
  }

  return 0;
}

static char line[256];

void
on_stdin(char *lin)
{
  int cmd;

  line[0] = '\0';
  strncat(line, lin, 255);
  cmd= GetCommand(strtok(line, " \t\n"));

  warningCount= xPrefix= 0;
  if (cmd>=0 && CommandList[cmd](0))
    pl_quit();

  need_prompt = 1;
}

static int Quit(int help)
{
  if (help) {
    pl_stderr("gist: quit command syntax:\n     quit\n");
    pl_stderr("  Finish and close any output files, then exit.\n");
    pl_stderr("  Synonyms:  exit, end   (no abbreviations allowed)\n");
    return 0;
  }
  return 1;
}

/* ------------------------------------------------------------------------ */

static int SaveName(int device, char *name)
{
  int len;

  if (name == outNames[device]) return 0;

  len= name? (int)strlen(name) : 0;
  if (len>outLengths[device]) {
    if (outLengths[device]>0) pl_free(outNames[device]);
    outNames[device]= (char *)pl_malloc(len+1);
    if (!outNames[device]) {
      pl_stderr("gist: (SEVERE) memory manager failed in SaveName\n");
      outLengths[device]= 0;
      return 1;
    }
    outLengths[device]= len;
  }

  if (name) strcpy(outNames[device], name);
  else if (outLengths[device]>0) outNames[device][0]= '\0';
  else outNames[device]= 0;
  return 0;
}

static int CreateCGM(int device, char *name)
{
  if (SaveName(device, name)) return 1;

  gp_cgm_out_engines[device]=
    gp_new_cgm_engine("CGM Viewer", gp_cgm_landscape, 0, name);
  if (!gp_cgm_out_engines[device]) {
    gp_cgm_warning(gp_error, "");
    gp_cgm_warning("Unable to create CGM engine ", name);
    return 1;
  }

  gp_cgm_out_types[device]= 0;
  outDraw[device]= 0;
  outSend[device]= 1;

  return 0;
}

static int CreatePS(int device, char *name)
{
  if (SaveName(device, name)) return 1;

  gp_cgm_out_engines[device]=
    gp_new_ps_engine("CGM Viewer", gp_cgm_landscape, 0, name);
  if (!gp_cgm_out_engines[device]) {
    gp_cgm_warning(gp_error, "");
    gp_cgm_warning("Unable to create PostScript engine ", name);
    return 1;
  }

  gp_cgm_out_types[device]= 1;
  outDraw[device]= 0;
  outSend[device]= 1;

  return 0;
}

static int CreateX(int device, char *name)
{
#ifndef NO_XLIB
  if (SaveName(device, name)) return 1;

  gp_input_hint = 1;
  gp_cgm_out_engines[device]=
    gp_new_bx_engine("CGM Viewer", gp_cgm_landscape, defaultDPI, name);
  if (!gp_cgm_out_engines[device]) {
    gp_cgm_warning(gp_error, "");
    gp_cgm_warning("Unable to open X server ", name? name : "(default)");
    return 1;
  }

  gp_cgm_out_types[device]= 2;
  outDraw[device]= 1;
  outSend[device]= 0;

  gx_input(gp_cgm_out_engines[device], &HandleExpose, 0, 0, &HandleOther);

  return 0;
#else
  return 1;
#endif
}

/* ------------------------------------------------------------------------ */

static int OpenIn(int help)
{
  char *token;

  if (help) {
    pl_stderr("gist: open command syntax:\n     open cgminput\n");
    pl_stderr("  Closes the current CGM input file, then opens cgminput.\n");
    pl_stderr("  Only a Gist-compliant binary CGM file is legal.\n");
    pl_stderr("  The cgminput may be the first file of a family.\n");
    pl_stderr("  Subsequent page numbers refer to this input file.\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (!token) {
    pl_stderr("gist: (SYNTAX) cgminput name missing in open command\n");
    return 0;
  }
  if (CheckEOL("open")) return 0;

  if (no_warnings) no_warnings= 1;  /* one warning per file family */
  gp_open_cgm(token);
  return 0;
}

static int FindDevice(void)
{
  int i;
  for (i=0 ; i<8 ; i++) if (!gp_cgm_out_engines[i]) break;
  if (i>=8)
    gp_cgm_warning("8 devices already open for output, command ignored", "");
  return i;
}

static int MakeCGM(int help)
{
  char *token, *cgmout;
  long size= 0;
  int device;

  if (help) {
    pl_stderr("gist: cgm command syntax:\n     cgm cgmout [size]\n");
    pl_stderr("  Opens a CGM file cgmout for output.\n");
    pl_stderr("  The size (default 1000000) is the maximum size of a\n");
    pl_stderr("  single file in the output family, in bytes.\n");
    pl_stderr("  Subsequent send commands will write to cgmout,\n");
    pl_stderr("  unless the send to list is modified (see send).\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (!token) {
    pl_stderr("gist: (SYNTAX) cgmout name missing in cgm command\n");
    return 0;
  }
  cgmout= token;
  token= strtok(0, " \t\n");
  if (token) {
    char *suffix;
    size= strtol(token, &suffix, 0);
    if (*suffix) {
      pl_stderr("gist: (SYNTAX) size unintelligble in cgm command\n");
      return 0;
    }
    if (CheckEOL("cgm")) return 0;
  }

  device= FindDevice();
  if (device>=8) return 0;

  if (!CreateCGM(device, cgmout) &&
      size>0) ((gp_cgm_engine_t *)gp_cgm_out_engines[device])->fileSize= size;

  return 0;
}

static int MakePS(int help)
{
  char *token;
  int device;

  if (help) {
    pl_stderr("gist: ps command syntax:\n     ps psout\n");
    pl_stderr("  Opens a PostScript file psout for output.\n");
    pl_stderr("  Subsequent send commands will write to psout,\n");
    pl_stderr("  unless the send to list is modified (see send).\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (!token) {
    pl_stderr("gist: (SYNTAX) psout name missing in ps command\n");
    return 0;
  }
  if (CheckEOL("ps")) return 0;

  device= FindDevice();
  if (device>=8) return 0;

  CreatePS(device, token);

  return 0;
}

static int MakeX(int help)
{
  char *token, *server;
  int dpi, device, defDPI;

  if (help) {
    pl_stderr("gist: display command syntax:\n     ");
    pl_stderr("display host:server.screen [dpi]\n");
    pl_stderr("  Connects to the specified X server.\n");
    pl_stderr("  Subsequent draw commands will write to server,\n");
    pl_stderr("  unless the draw to list is modified (see draw).\n");
    pl_stderr("  If specified, 40<=dpi<=200 (default 100).\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (!token) {
    pl_stderr("gist: (SYNTAX) cgmoutput name missing in cgm command\n");
    return 0;
  }
  server= token;
  token= strtok(0, " \t\n");
  if (token) {
    char *suffix;
    dpi= (int)strtol(token, &suffix, 0);
    if (*suffix) {
      pl_stderr("gist: (SYNTAX) dpi unintelligble in display command\n");
      return 0;
    }
    if (dpi<40 && dpi>200) {
      pl_stderr(
        "gist: (SYNTAX) dpi not between 40 and 200 in display command\n");
      return 0;
    }
    if (CheckEOL("display")) return 0;
  } else {
    dpi= 100;
  }

  device= FindDevice();
  if (device>=8) return 0;

  defDPI= defaultDPI;
  defaultDPI= dpi;
  CreateX(device, server);
  defaultDPI= defDPI;

  return 0;
}

static int EPS(int help)
{
  char *token;
  int device;

  if (help) {
    pl_stderr("gist: eps command syntax:\n     eps epsout\n");
    pl_stderr("  Open an Encapsulated PostScript file epsout, write\n");
    pl_stderr("  the current page to it, then close epsout.\n");
    pl_stderr("  (Note that an EPS file can have only a single page.)\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (!token) {
    pl_stderr("gist: (SYNTAX) epsout name missing in eps command\n");
    return 0;
  }
  if (CheckEOL("eps")) return 0;

  device= FindDevice();
  if (device>=8) return 0;

  device= FindDevice();
  if (device>=8) return 0;

  gp_cgm_out_engines[device]=
    gp_new_ps_engine("CGM Viewer", gp_cgm_landscape, 0, "_tmp.eps");
  if (!gp_cgm_out_engines[device]) {
    gp_cgm_warning(gp_error, "");
    gp_cgm_warning("Unable to create PostScript engine ", token);
    return 0;
  }

  gp_preempt(gp_cgm_out_engines[device]);

  nPage[0]= mPage[0]= gp_cgm_relative(0);
  sPage[0]= 1;
  nPageGroups= 1;

  /* First read does PS part, second computes EPS preview */
  if (!gp_read_cgm(mPage, nPage, sPage, nPageGroups)) {
    gp_preempt(0);
    gp_cgm_out_engines[device]= gp_eps_preview(gp_cgm_out_engines[device], token);
    if (gp_cgm_out_engines[device]) {
      gp_preempt(gp_cgm_out_engines[device]);
      gp_read_cgm(mPage, nPage, sPage, nPageGroups);
    } else {
      gp_cgm_warning("memory manager failed creating EPS engine ", token);
      return 0;
    }
  }

  if (gp_cgm_out_engines[device]) {
    gp_preempt(0);

    gp_kill_engine(gp_cgm_out_engines[device]);
    gp_cgm_out_engines[device]= 0;
  }

  return 0;
}

static int FreeAny(int help)
{
  int i;

  if (help) {
    pl_stderr("gist: free command syntax:\n     free [device# ...]\n");
    pl_stderr("  Finish and close the device#(s).  If none given,\n");
    pl_stderr("  frees all send devices,\n");
    pl_stderr("  (Use the info command to describe current device numbers.)\n");
    return 0;
  }

  if (GetDeviceList(1, "free")) return 0;

  for (i=0 ; i<8 ; i++) {
    if (outMark[i]) {
      gp_kill_engine(gp_cgm_out_engines[i]);
      gp_cgm_out_engines[i]= 0;
    }
  }

  return 0;
}

static char *yorn[2]= { "No  ", "Yes " };
static char *tname[3]= { "CGM", "PS ", "X  " };

extern void gp_cgm_info(void);  /* cgmin.c */

static int Info(int help)
{
  int i;

  if (help) {
    pl_stderr("gist: info command syntax:\n     info\n");
    pl_stderr("  Print descriptions of all current output files.\n");
    return 0;
  }

  pl_stdout("\n");
  gp_cgm_info();
  pl_stdout("Number Draw Send Type   Name\n");
  for (i=0 ; i<8 ; i++) {
    if (gp_cgm_out_engines[i]) {
      char msg[80];
      sprintf(msg, "%3d    %s %s %s  ", i, yorn[outDraw[i]], yorn[outSend[i]],
             tname[gp_cgm_out_types[i]]);
      pl_stdout(msg);
      pl_stdout(outNames[i]? outNames[i] : "");
      pl_stdout("\n");
    }
  }
  pl_stdout("\n");

  return 0;
}

/* ------------------------------------------------------------------------ */

static int Draw(int help)
{
  int i, n;
  char *token;

  if (help) {
    pl_stderr("gist: draw command syntax:\n     draw [page list]\n");
    pl_stderr(
    "  Copy the page(s) (default current page) from the current CGM input\n");
    pl_stderr("  to all display output devices.\n");
    pl_stderr("  By default, these are all X windows.\n");
    pl_stderr("  Use alternate syntax:\n     draw to [device#1 ...]\n");
    pl_stderr("  to specify a particular list of devices to be used\n");
    pl_stderr("  by the draw command.  Without any device numbers,\n");
    pl_stderr("  draw to restores the default list of devices.\n");
    pl_stderr("  (Use the info command to describe current device numbers.)\n");
    pl_stderr("  Page list syntax:   group1 [group2 ...]\n");
    pl_stderr("  Page group syntax:   n   - just page n\n");
    pl_stderr("                     m-n   - pages n thru m\n");
    pl_stderr("                   m-n-s   - pages n thru m, step s\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (token && strcmp(token, "to")==0) {
    DrawSend(1, token);
    return 0;
  }

  n= 0;
  for (i=0 ; i<8 ; i++) {
    if (!outDraw[i]) gp_deactivate(gp_cgm_out_engines[i]);
    if (outDraw[i] && !gp_activate(gp_cgm_out_engines[i])) n++;
  }

  if (!n && (i= FindDevice())<8) {
    if (!CreateX(i, 0) && !gp_activate(gp_cgm_out_engines[i])) n++;
  }

  if (n) DrawSend(0, token);
  else gp_cgm_warning("no devices active for draw command", "");
  return 0;
}

static int Send(int help)
{
  int i, n;
  char *token;

  if (help) {
    pl_stderr("gist: send command syntax:\n     send [page list]\n");
    pl_stderr(
    "  Copy the page(s) (default current page) from the current CGM input\n");
    pl_stderr("  to all display output devices.\n");
    pl_stderr("  By default, these are all X windows.\n");
    pl_stderr("  Use alternate syntax:\n     send to [device#1] ...\n");
    pl_stderr("  to specify a particular list of devices to be used\n");
    pl_stderr("  by the send command.  Without any device numbers,\n");
    pl_stderr("  send to restores the default list of devices.\n");
    pl_stderr("  (Use the info command to describe current device numbers.)\n");
    pl_stderr("  Page list syntax:   group1 [group2 ...]\n");
    pl_stderr("  Page group syntax:   n   - just page n\n");
    pl_stderr("                     m-n   - pages n thru m\n");
    pl_stderr("                   m-n-s   - pages n thru m, step s\n");
    return 0;
  }

  token= strtok(0, " \t\n");
  if (token && strcmp(token, "to")==0) {
    DrawSend(1, token);
    return 0;
  }

  n= 0;
  for (i=0 ; i<8 ; i++) {
    if (!outSend[i]) gp_deactivate(gp_cgm_out_engines[i]);
    if (outSend[i] && !gp_activate(gp_cgm_out_engines[i])) n++;
  }

  if (n) DrawSend(1, token);
  else gp_cgm_warning("no devices active for send command", "");
  return 0;
}

static int GetDeviceList(int ds, char *command)
{
  int device;
  char *token= strtok(0, " \t\n");

  if (token) {
    char *suffix;
    for (device=0 ; device<8 ; device++) outMark[device]= 0;
    do {
      device= (int)strtol(token, &suffix, 10);
      if (*suffix || device<0 || device>7) {
        pl_stderr("gist: (SYNTAX) ");
        pl_stderr(token);
        pl_stderr(" not a legal device# in ");
        pl_stderr(command);
        pl_stderr(" command\n");
        return 1;
      }
      if (!gp_cgm_out_engines[device])
        gp_cgm_warning("ignoring non-existent device# ", token);
      else
        outMark[device]= 1;
    } while ((token= strtok(0, " \t\n")));
    if (ds==0) {
      for (device=0 ; device<8 ; device++) if (outMark[device]) break;
      if (device>=8) {
        gp_cgm_warning(command, " command with no legal devices, no action taken");
        return 1;
      }
    }

  } else if (ds==0) {
    for (device=0 ; device<8 ; device++) outMark[device]= 0;
  } else {
    for (device=0 ; device<8 ; device++) outMark[device]= outSend[device];
  }

  return 0;
}

static char *dsName[]= { "draw to", "send to" };

static void DrawSend(int ds, char *token)
{
  nPageGroups= 0;

  if (token && strcmp(token, "to")==0) {
    /* draw to  or send to  merely resets outDraw or outSend list */
    int i, n= 0;
    int *outDS= ds? outSend : outDraw;
    if (GetDeviceList(0, dsName[ds])) return;
    for (i=0 ; i<8 ; i++) if ((outDS[i]= outMark[i])) n++;
    if (!n) for (i=0 ; i<8 ; i++)
      outDS[i]= gp_cgm_out_engines[i]? (ds? gp_cgm_out_types[i]<2 : gp_cgm_out_types[i]==2) : 0;
    return;

  } else if (token) {
    do {
      if (GetPageGroup(token)) return;
    } while ((token= strtok(0, " \t\n")));
  } else {
    nPage[0]= mPage[0]= gp_cgm_relative(0);
    sPage[0]= 1;
    nPageGroups= 1;
  }

  gp_read_cgm(mPage, nPage, sPage, nPageGroups);
}

static int Special(int help)
{
  if (help) {
    char msg[80];
    sprintf(msg, "gist: n%c command syntax:\n     n%c\n",
            cSuffix, cSuffix);
    pl_stderr(msg);
    if (cSuffix=='f')
      pl_stderr("  Forward n (default 1) pages, then draw\n");
    else if (cSuffix=='b')
      pl_stderr("  Backward n (default 1) pages, then draw\n");
    else
      pl_stderr("  Go to page n (default 1), then draw\n");
    return 0;
  }

  if (CheckEOL("nf, nb, or ng")) return 0;

  DoSpecial(nPrefix, cSuffix);
  return 0;
}

static void DoSpecial(int nPrefix, int cSuffix)
{
  int i, n;

  n= 0;
  for (i=0 ; i<8 ; i++) {
    if (!outDraw[i]) gp_deactivate(gp_cgm_out_engines[i]);
    if (outDraw[i] && !gp_activate(gp_cgm_out_engines[i])) n++;
  }

  if (cSuffix=='f') mPage[0]= gp_cgm_relative(nPrefix);
  else if (cSuffix=='b') mPage[0]= gp_cgm_relative(-nPrefix);
  else mPage[0]= nPrefix;
  nPage[0]= mPage[0];
  sPage[0]= 1;
  nPageGroups= 1;

  if (n) gp_read_cgm(mPage, nPage, sPage, nPageGroups);
  else gp_cgm_warning("no devices active for nf, nb, or ng command", "");
}

/* ------------------------------------------------------------------------ */

#ifndef NO_XLIB
/* ARGSUSED */
static void HandleExpose(gp_engine_t *engine, gd_drawing_t *drauing, int xy[])
{
  gp_x_engine_t *xEngine= gp_x_engine(engine);

  if (!xEngine) return;

  /* Redraw current picture on this engine only */

  gp_preempt(engine);

  nPage[0]= mPage[0]= gp_cgm_relative(0);
  sPage[0]= 1;
  nPageGroups= 1;

  gp_read_cgm(mPage, nPage, sPage, nPageGroups);

  gp_preempt(0);
  gx_expose(engine, drauing, xy);
}

static int cSuffices[]= { 'f', 'b', 'g' };

/* ARGSUSED */
static void HandleOther(gp_engine_t *engine, int k, int md)
{
  gp_x_engine_t *xEngine= gp_x_engine(engine);
  int go, bad;

  if (!xEngine) return;

  go= bad= 0;

  if (k == '0') xPrefix= 10*xPrefix;
  else if (k == '1') xPrefix= 10*xPrefix+1;
  else if (k == '2') xPrefix= 10*xPrefix+2;
  else if (k == '3') xPrefix= 10*xPrefix+3;
  else if (k == '4') xPrefix= 10*xPrefix+4;
  else if (k == '5') xPrefix= 10*xPrefix+5;
  else if (k == '6') xPrefix= 10*xPrefix+6;
  else if (k == '7') xPrefix= 10*xPrefix+7;
  else if (k == '8') xPrefix= 10*xPrefix+8;
  else if (k == '9') xPrefix= 10*xPrefix+9;
  else if (k=='f' || k=='F' || (k=='+' && (md&PL_KEYPAD))) go= 1;
  else if (k=='b' || k=='B' || (k=='-' && (md&PL_KEYPAD))) go= 2;
  else if (k=='g' || k=='G' || k=='\r') go= 3;
  else if (k=='s' || k=='S' || (k=='=' && (md&PL_KEYPAD))) go= 4;
  else if (k=='q' || k=='Q') go= 5;
  else bad= 1;

  if ((go==4||go==5) && xPrefix!=0) bad= 1;
  if (go && !bad) {
    if (go<4) {
      if (xPrefix==0) xPrefix= 1;
      DoSpecial(xPrefix, cSuffices[go-1]);
    } else if (go==4) {
      int i, n= 0;
      for (i=0 ; i<8 ; i++) {
        if (!outSend[i]) gp_deactivate(gp_cgm_out_engines[i]);
        if (outSend[i] && !gp_activate(gp_cgm_out_engines[i])) n++;
      }

      nPage[0]= mPage[0]= gp_cgm_relative(0);
      sPage[0]= 1;
      nPageGroups= 1;

      if (n) gp_read_cgm(mPage, nPage, sPage, nPageGroups);
      else gp_cgm_warning("no devices active for send command", "");
    } else if (go==5) {
      pl_quit();
    }
    xPrefix= 0;
    warningCount= 0;
  } else if (bad) {
    pl_feep(xEngine->win);
    xPrefix= 0;
  }
}
#endif

/* ------------------------------------------------------------------------ */
