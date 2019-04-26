/*
 * $Id: test2d.c,v 1.1 2005-09-18 22:05:43 dhmunro Exp $
 * test code to exercise play features
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

/* set these to 1/0 during development to turn on/off
 * exercising the corresponding play features */
#define TRY_STDINIT   1
#define TRY_HANDLER   1
#define TRY_GUI       1

#define TRY_PSTDIO    1

/* TRY_GRAPHICS prerequisites:
 *      pl_connect, pl_window, pl_text, pl_lines, and pl_fill
 * -implement those functions first
 * -some features (e.g.- rotated text) need not be implemented initially */
#define TRY_GRAPHICS  1
#define TRY_RECT      1
#define TRY_ELLIPSE   1
#define TRY_DOTS      1
#define TRY_SEGMENTS  1
#define TRY_NDXCELL   1
#define TRY_RGBCELL   1

#define TRY_PALETTE   1
#define TRY_CLIP      1
#define TRY_CURSOR    1
#define TRY_RESIZE    1

#define TRY_CLIPBOARD 1

#define TRY_OFFSCREEN 1
#define TRY_MENU      1
#define TRY_METAFILE  1

#define TRY_RGBREAD   1

/* #include "config.h" -- really must work without this */

#include "play2.h"
#include "play/std.h"
#include "play/io.h"
#include <stdio.h>
#include <string.h>

#if TRY_HANDLER
#include <signal.h>
#ifdef SIGBUS
# define MY_SIGBUS SIGBUS
#else
# define MY_SIGBUS 0
#endif
#ifdef SIGPIPE
# define MY_SIGPIPE SIGPIPE
#else
# define MY_SIGPIPE 0
#endif
static int sig_table[] = {
  0, 0, SIGINT, SIGFPE, SIGSEGV, SIGILL, MY_SIGBUS, MY_SIGPIPE };
static char *sig_name[] = {
  "PL_SIG_NONE", "PL_SIG_SOFT", "PL_SIG_INT", "PL_SIG_FPE", "PL_SIG_SEGV",
  "PL_SIG_ILL", "PL_SIG_BUS", "PL_SIG_IO", "PL_SIG_OTHER" };
#endif

extern int on_idle(void);
extern void on_alarm(void *context);
static void idle_act(char *args);
static void set_act(char *args);
static void clr_act(char *args);

extern int on_quit(void);

#if TRY_STDINIT
extern void on_stdin(char *input_line);
static void help_act(char *args);
#endif

#if TRY_PSTDIO
static void ls_act(char *args);
static void cd_act(char *args);
static void pwd_act(char *args);
static void mv_act(char *args);
static void rm_act(char *args);
static void mkdir_act(char *args);
static void rmdir_act(char *args);
#endif

#if TRY_HANDLER
extern void on_exception(int signal, char *errmsg);
static void raise_act(char *args);
static void abort_act(char *args);
#endif

#if TRY_GRAPHICS
extern int phints;
extern pl_scr_t *scr, *scr2, *scremote;
pl_scr_t *scr, *scr2, *scremote;
extern pl_win_t *win1, *win2, *win3, *win4;
pl_win_t *win1, *win2, *win3, *win4, *offscr;
extern void dis_graphics(void);
extern void simple_create(pl_win_t **winp);
extern void simple_draw(pl_scr_t *scr, pl_win_t *w, int x0, int y0);
extern void advanced_draw(pl_win_t *w);
extern void seg_draw(pl_win_t *w, int x0, int y0, int x1, int y1);
extern void box_draw(pl_win_t *w, int x0, int y0, int x1, int y1, int fillit);
extern void tri_draw(pl_win_t *w, int x0,int y0, int x1,int y1, int x2,int y2,
                     int fillit);
extern void curve_draw(pl_win_t *w, int x0, int y0, int width, int height);
static void feep_act(char *args);
static void redraw_act(char *args);
static void private_act(char *args);
static void rgb_act(char *args);
extern void hilite(pl_win_t *w, int x0, int y0, int x1, int y1, int onoff);
static void make_rainbow(int pn, pl_col_t *colors);
# if TRY_RESIZE
static void resize_act(char *args);
# endif
# if TRY_CLIPBOARD
static void paste_act(char *args);
# endif
static int hcour, bcour, wcour;
# if TRY_GUI
extern void on_focus(void *c,int in);
extern void on_key(void *c,int k,int md);
extern void on_click(void *c,int b,int md,int x,int y,
                     unsigned long ms);
extern void on_motion(void *c,int md,int x,int y);
extern void on_expose(void *c, int *xy);
extern void on_deselect(void *c);
extern void on_destroy(void *c);
extern void on_resize(void *c,int w,int h);
static int xfoc, xkey, xclick, xmotion, ygui, xhi0, yhi0, xhi1, yhi1, hion;
extern void on_panic(pl_scr_t *screen);
# endif
# if TRY_METAFILE
static void meta_act(char *args);
# endif
# if TRY_PALETTE
static void pal_act(char *args);
static int pal_number = 1;
static void reset_palette(void);
# endif
# if TRY_RGBCELL
static void rgbcell_act(char *args);
# endif
# if TRY_OFFSCREEN
extern void on_animate(void *context);
# endif
#endif
static int rgbcell_on = 0;

static int app_argc = 0;
static char **app_argv = 0;
static int app_quit = 0;

static int prompt_issued = 0;

int
on_quit(void)
{
#if TRY_GRAPHICS
  dis_graphics();
#endif
#if TRY_STDINIT
  sprintf(pl_wkspc.c, "\non_quit called, returning %d\n", app_quit);
  pl_stdout(pl_wkspc.c);
#endif
  return app_quit;
}

#if TRY_STDINIT
static void
quit_act(char *args)
{
  app_quit = (int)strtol(args, (char **)0, 0);
  pl_quit();
}

struct command {
  char *name;
  void (*action)(char *args);
  char *desc;
} commands[] = {
  { "help", &help_act, "help\n" },
  { "quit", &quit_act, "quit [value]\n" },
  { "idle", &idle_act, "idle [ncalls]\n" },
  { "set", &set_act, "set alarm_secs\n" },
  { "clr", &clr_act, "clr [alarm_number]\n" },
#if TRY_HANDLER
  { "raise", &raise_act, "raise [signal_number]\n" },
  { "abort", &abort_act, "abort\n" },
#endif
#if TRY_PSTDIO
  { "ls", &ls_act, "ls [path]\n" },
  { "cd", &cd_act, "cd [path]\n" },
  { "pwd", &pwd_act, "pwd\n" },
  { "mv", &mv_act, "mv old_path new_path\n" },
  { "rm", &rm_act, "rm path\n" },
  { "mkdir", &mkdir_act, "mkdir path\n" },
  { "rmdir", &rmdir_act, "rmdir path\n" },
#endif
#if TRY_GRAPHICS
  { "feep", &feep_act, "feep\n" },
  { "redraw", &redraw_act, "redraw\n" },
  { "private", &private_act, "private\n" },
  { "rgb", &rgb_act, "rgb\n" },
# if TRY_RESIZE
  { "resize", &resize_act, "resize width height\n" },
# endif
# if TRY_CLIPBOARD
  { "paste", &paste_act, "paste\n" },
# endif
# if TRY_METAFILE
  { "meta", &meta_act, "meta filename\n" },
# endif
# if TRY_PALETTE
  { "palette", &pal_act, "palette\n" },
# endif
# if TRY_RGBCELL
  { "rgbcell", &rgbcell_act, "rgbcell\n" },
# endif
#endif
  { (char*)0, (void (*)(char*))0 }
};

/* ARGSUSED */
static void
help_act(char *args)
{
  int i;
  for (i=0 ; commands[i].desc ; i++)
    pl_stdout(commands[i].desc);
}

void
on_stdin(char *input_line)
{
  struct command *cmnd;
  int n = strspn(input_line, " \t\n\r");
  input_line += n;
  n = strcspn(input_line, " \t\n\r");
  prompt_issued = 0;
  if (n) {
    for (cmnd=commands ; cmnd->name ; cmnd++) {
      if (strncmp(cmnd->name, input_line, n)) continue;
      if (!cmnd->action) break;
      strcpy(pl_wkspc.c, "doing: ");
      strncat(pl_wkspc.c, input_line, n);
      strcat(pl_wkspc.c, "\n");
      pl_stdout(pl_wkspc.c);
      input_line += n;
      input_line += strspn(input_line, " \t\n\r");
      cmnd->action(input_line);
      return;
    }

    strcpy(pl_wkspc.c, "\ntest2d command not recognized: ");
    strncat(pl_wkspc.c, input_line, n);
    strcat(pl_wkspc.c, "\n");
    pl_stderr(pl_wkspc.c);
  }
}
#endif

#if TRY_HANDLER

void
on_exception(int signal, char *errmsg)
{
  pl_qclear();

#if TRY_STDINIT
  if (signal<0) signal = 0;
  else if (signal>PL_SIG_OTHER) signal = PL_SIG_OTHER;
  sprintf(pl_wkspc.c, "test2d received signal %s\n", sig_name[signal]);
  pl_stdout(pl_wkspc.c);
  if (errmsg) {
    sprintf(pl_wkspc.c, "  with errmsg = %s\n", errmsg);
    pl_stdout(pl_wkspc.c);
  }
  prompt_issued = 0;
#endif
}

static void
raise_act(char *args)
{
  char *after = args;
  int n = args[0]? (int)strtol(args, &after, 0) : 0;
#if TRY_STDINIT
  if (after==args) {
    while (args[-1]=='\n' || args[-1]=='\r' ||
           args[-1]==' ' || args[-1]=='\t') args--;
    n = (args[-1]-'e');  /* this should be non-optimizable n=0 */
    sprintf(pl_wkspc.c, "SIGFPE handling broken: 1.0/%d = %g\n",
            n, 1.0/(double)n);
    pl_stdout(pl_wkspc.c);
    return;
  }
#endif
  if (n<0) n = 0;
  else if (n>=PL_SIG_OTHER) n = 1001;
  else n = sig_table[n];
  raise(n);
}

static void
abort_act(char *args)
{
  pl_abort();
}
#endif

static unsigned int app_ncalls = 0;
static int panic_count = 0;

int
pl_on_launch(int argc, char *argv[])
{
  app_argc = argc;
  app_argv = argv;
  pl_quitter(&on_quit);
  pl_idler(&on_idle);
#if TRY_STDINIT
  pl_stdinit(&on_stdin);
  pl_stdout("test2d initialized stdio\n");
#endif
#if TRY_HANDLER
  pl_handler(&on_exception);
#endif
#if TRY_GUI
  pl_gui(&on_expose, &on_destroy, &on_resize, &on_focus,
        &on_key, &on_click, &on_motion, &on_deselect, &on_panic);
#endif
  return 0;
}

int
on_idle(void)
{
  if (app_ncalls) {
#if TRY_STDINIT
    pl_stdout("on_idle called after idle command\n");
#endif
    app_ncalls--;
    prompt_issued = (app_ncalls!=0);
  }
#if TRY_GRAPHICS
  if (!scr && (panic_count<3)) {
    int sw, sh;
    scr = pl_connect(0);
    simple_create(&win1);
    win2 = pl_window(scr, 400, 400, "test2d advanced", PL_BG,
                   phints | PL_NORESIZE | PL_NOKEY | PL_NOMOTION, &win2);
    pl_sshape(scr, &sw, &sh);
# if TRY_STDINIT
    sprintf(pl_wkspc.c, "test2d: screen shape %d X %d pixels\n", sw, sh);
    pl_stdout(pl_wkspc.c);
    prompt_issued = 0;
# endif
  }
#endif
#if TRY_STDINIT
  if (!prompt_issued) {
    pl_stdout("idle> ");
    prompt_issued = 1;
  }
#endif
  return (app_ncalls!=0);
}

static struct alarm_data {
  double timeout, set_at, rang_at;
  int state;
} alarm_list[5];

void
on_alarm(void *context)
{
  struct alarm_data *alarm = context;
  int n = alarm-alarm_list;
  if (alarm->state==-1) {
    alarm->state = -2;
#if TRY_STDINIT
    sprintf(pl_wkspc.c, "test2d: ERROR cancelled alarm #%d rang\n", n);
    pl_stderr(pl_wkspc.c);
#endif
  } else {
    alarm->rang_at = pl_wall_secs();
    alarm->state = 0;
#if TRY_STDINIT
    sprintf(pl_wkspc.c,
            "test2d: alarm #%d rang after %g secs (requested %g)\n", n,
            alarm->rang_at-alarm->set_at, alarm->timeout);
    pl_stdout(pl_wkspc.c);
#endif
  }
  prompt_issued = 0;
}

static void
idle_act(char *args)
{
  app_ncalls = (unsigned int)strtol(args, (char **)0, 0);
  if (app_ncalls<1) app_ncalls = 1;
  if (app_ncalls>5) app_ncalls = 5;
}

static void
set_act(char *args)
{
  double t = strtod(args, (char **)0);
  int n;
  for (n=0 ; n<5 ; n++) if (alarm_list[n].state<=0) break;
  if (n>=5) {
#if TRY_STDINIT
    pl_stdout("test2d: (no free alarms, set ignored)\n");
#endif
    return;
  }
  alarm_list[n].timeout = t>0.? t : 0.;
  alarm_list[n].state = 1;
  alarm_list[n].set_at = pl_wall_secs();
  alarm_list[n].rang_at = 0.0;
  pl_set_alarm(t, &on_alarm, &alarm_list[n]);
#if TRY_STDINIT
  sprintf(pl_wkspc.c,
          "test2d: set alarm #%d for %g secs\n", n, alarm_list[n].timeout);
  pl_stdout(pl_wkspc.c);
#endif
}

static void
clr_act(char *args)
{
  int n = (int)strtol(args, (char **)0, 0);
  if (!args[0])
    for (n=0 ; n<5 ; n++) if (alarm_list[n].state==1) break;
  if (n<0 || n>=5) {
    pl_stdout("test2d: (no such alarm, clr ignored)\n");
    return;
  }
  pl_clr_alarm((n%2)? &on_alarm : 0, &alarm_list[n]);
  alarm_list[n].state = -1;
#if TRY_STDINIT
  sprintf(pl_wkspc.c, "test2d: clr alarm #%d\n", n);
  pl_stdout(pl_wkspc.c);
#endif
}

#if TRY_PSTDIO
static void
ls_act(char *args)
{
  pl_dir_t *dir;
  char *dot = ".";
  char *path = strtok(args, " \t\r\n");
  if (!path) path = dot;
  dir = pl_dopen(path);
  if (dir) {
    pl_file_t *f;
    int is_dir;
    char *name = pl_dnext(dir, &is_dir);
    long len = strlen(path);
    char *dirname = pl_strncat(path, (len<1 || path[len-1]!='/')? "/":"", 0);
    for ( ; name ; name=pl_dnext(dir, &is_dir)) {
      if (!is_dir) {
        strcpy(pl_wkspc.c, dirname);
        strncat(pl_wkspc.c, name, strlen(name));
        f = pl_fopen(pl_wkspc.c, "rb");
        if (f) {
          len = pl_fsize(f);
          pl_fclose(f);
        } else {
          len = -1;
        }
        sprintf(pl_wkspc.c, "%s %-20s %ld\n", "FIL", name, len);
      } else {
        sprintf(pl_wkspc.c, "%s %s\n", "DIR", name);
      }
      pl_stdout(pl_wkspc.c);
    }
    pl_free(dirname);
    pl_dclose(dir);
  } else {
    pl_stdout("test2d: pl_dopen cant find directory\n");
  }
}

static void
cd_act(char *args)
{
  char *home = pl_getenv("HOME");
  char *path = strtok(args, " \t\r\n");
  if (!path) path = home;
  if (!path) pl_stdout("test2d: pl_getenv cant find HOME\n");
  else if (pl_chdir(path)) pl_stdout("test2d: pl_chdir failed\n");
}

static void
pwd_act(char *args)
{
  char *path = pl_getcwd();
  if (!path) pl_stdout("test2d: pl_getcwd failed\n");
  else {
    pl_stdout(path);
    pl_stdout("\n");
  }
}

static void
mv_act(char *args)
{
  char *path1 = strtok(args, " \t\r\n");
  char *path2 = strtok((char *)0, " \t\r\n");
  if (!path1 || !path2)
    pl_stdout("test2d: syntax: mv old_path new_path\n");
  else if (pl_rename(path1, path2))
    pl_stdout("test2d: pl_rename failed\n");
}

static void
rm_act(char *args)
{
  char *path = strtok(args, " \t\r\n");
  if (!path) pl_stdout("test2d: syntax: rm path\n");
  else if (pl_remove(path)) pl_stdout("test2d: pl_remove failed\n");
}

static void
mkdir_act(char *args)
{
  char *path = strtok(args, " \t\r\n");
  if (!path) pl_stdout("test2d: syntax: mkdir path\n");
  else if (pl_mkdir(path)) pl_stdout("test2d: pl_mkdir failed\n");
}

static void
rmdir_act(char *args)
{
  char *path = strtok(args, " \t\r\n");
  if (!path) pl_stdout("test2d: syntax: rmdir path\n");
  else if (pl_rmdir(path)) pl_stdout("test2d: pl_rmdir failed\n");
}
#endif

#if TRY_GRAPHICS
void
dis_graphics(void)
{
  if (win4) pl_destroy(win4);
  if (win3) pl_destroy(win3);
  if (win2) pl_destroy(win2);
  if (win1) pl_destroy(win1);
  if (offscr) pl_destroy(offscr);
  if (scremote) pl_disconnect(scremote);
  if (scr2) pl_disconnect(scr2);
  if (scr) pl_disconnect(scr);
}

int phints = 0;

#define SIMDX 450
#define SIMDY 450

static int w0 = SIMDX;
static int h0 = SIMDY;
static int yoffscr, xsub, ysub, doffscr, animating, xpal, ypal;
static int win3_hi, win3_flag, win4_hi;

void
simple_create(pl_win_t **winp)
{
  *winp = pl_window(scr, SIMDX, SIMDY, "test2d window", PL_BG, phints, winp);
# if !TRY_GUI
  simple_draw(scr, *winp, w0, h0);
# endif
}

# if TRY_OFFSCREEN
void
on_animate(void *context)
{
  pl_win_t *w = context;
  pl_clear(offscr);
  pl_color(offscr, PL_FG);
  pl_pen(offscr, 1, PL_SOLID);
  box_draw(offscr, xsub, yoffscr+ysub, xsub+15, yoffscr+ysub+15, 0);
  pl_bitblt(w, 0, 16, offscr, xsub, ysub+16, xsub+16, ysub+h0-16);
  if (animating) pl_set_alarm(0.05, &on_animate, context);
  xsub += 1;
  if (xsub>=16) xsub = 0;
  ysub += 1;
  if (ysub>=16) ysub = 0;
  if (!doffscr) {
    yoffscr += 4;
    if (yoffscr>h0-32) {
      yoffscr = h0-32;
      doffscr = 1;
    }
  } else {
    yoffscr -= 4;
    if (yoffscr<16) {
      yoffscr = 16;
      doffscr = 0;
    }
  }
}
# endif

void
simple_draw(pl_scr_t *scr, pl_win_t *w, int x0, int y0)
{
  int fontb;
  int fonth = pl_txheight(scr, PL_GUI_FONT, 12, &fontb);
  int ytxt = 2*fonth;
  int dx, dx1, dx2;

  pl_clear(w);
  pl_color(w, PL_FG);
  pl_pen(w, 1, PL_SOLID);
  pl_font(w, PL_GUI_FONT, 12, 0);

  /* draw 16x16 boxes in each corner (tests origin and size) */
  box_draw(w,     0,     0,   15,   15, 0);
  box_draw(w,     0, y0-16,   15, y0-1, 0);
  box_draw(w, x0-16,     0, x0-1,   15, 0);
  box_draw(w, x0-16, y0-16, x0-1, y0-1, 0);
  pl_text(w, 16, ytxt, "Corner boxes show four sides?", 29);
  ytxt += 2*fonth;

# if TRY_OFFSCREEN
  if (offscr) {
    xsub = ysub = 0;
    yoffscr = 16;
    doffscr = 0;
    pl_clr_alarm(&on_animate, (void *)0);
    animating = 0;
    on_animate(w);
    pl_color(w, PL_FG);
    pl_pen(w, 1, PL_SOLID);
  }
# endif

  /* string alignment test */
  dx = pl_txwidth(scr, "String aligned within boxes?", 28, PL_GUI_FONT, 12);
  box_draw(w, 16, ytxt-fontb+fonth, 16+dx, ytxt-fontb, 0);
  dx1 = pl_txwidth(scr, "String ", 7, PL_GUI_FONT, 12);
  dx2 = pl_txwidth(scr, "aligned withinGARBAGE", 14, PL_GUI_FONT, 12);
  box_draw(w, 16+dx1, ytxt-fontb, 16+dx1+dx2, ytxt, 0);
  pl_text(w, 16, ytxt, "String aligned within boxes?GARBAGE", 28);
  ytxt += 2*fonth;

  /* fill alignment test */
  /* alignment ticks */
  seg_draw(w, x0-40,  9, x0-40, 19);
  seg_draw(w, x0-40, 61, x0-40, 71);
  seg_draw(w, x0- 9, 40, x0-19, 40);
  seg_draw(w, x0-61, 40, x0-71, 40);
  /* upper right corner */
  box_draw(w, x0-19, 30, x0-29, 40, 1);
  box_draw(w, x0-39, 30, x0-29, 20, 1);
  tri_draw(w, x0-29, 20, x0-29, 30, x0-19, 30, 1);
  tri_draw(w, x0-29, 40, x0-39, 30, x0-39, 40, 1);
  /* lower right, y --> 81-y */
  box_draw(w, x0-19, 51, x0-29, 41, 1);
  box_draw(w, x0-39, 51, x0-29, 61, 1);
  tri_draw(w, x0-29, 61, x0-29, 51, x0-19, 51, 1);
  tri_draw(w, x0-29, 41, x0-39, 51, x0-39, 41, 1);
  /* lower left, x0-x --> 81-(x0-x) */
  box_draw(w, x0-60, 51, x0-50, 41, 1);
  box_draw(w, x0-40, 51, x0-50, 61, 1);
  tri_draw(w, x0-50, 61, x0-50, 51, x0-60, 51, 1);
  tri_draw(w, x0-50, 41, x0-40, 51, x0-40, 41, 1);
  /* upper left, y --> 81-y */
  box_draw(w, x0-60, 30, x0-50, 40, 1);
  box_draw(w, x0-40, 30, x0-50, 20, 1);
  tri_draw(w, x0-50, 20, x0-50, 30, x0-60, 30, 1);
  tri_draw(w, x0-50, 40, x0-40, 30, x0-40, 40, 1);
  pl_text(w, 16, ytxt, "Fills aligned (UR corner)?", 26);
  ytxt += 2*fonth;
  pl_flush(w);

# if TRY_DOTS
  {
    int x[4], y[4];
    x[0] = x0-40; x[1] = x0-40; x[2] = x0-61; x[3] = x0-21;
    y[0] = 80; y[1] = 120; y[2] = 100; y[3] = 100;
    pl_i_pnts(w, x, y, 4);
    pl_dots(w);
  }
  seg_draw(w, x0-40,  82, x0-40, 118);
  seg_draw(w, x0-59, 100, x0-23, 100);
  pl_text(w, 16, ytxt, "Dots aligned (.+.)?", 19);
  ytxt += 2*fonth;
# endif

# if TRY_SEGMENTS
  {
    int x[4], y[4];
    x[0] = x0-40; x[1] = x0-40; x[2] = x0-45; x[3] = x0-35;
    y[0] = 140; y[1] = 150; y[2] = 145; y[3] = 145;
    pl_i_pnts(w, x, y, 4);
    pl_segments(w);
  }
  seg_draw(w, x0-40, 130, x0-40, 138);
  seg_draw(w, x0-40, 152, x0-40, 160);
  seg_draw(w, x0-33, 145, x0-25, 145);
  seg_draw(w, x0-47, 145, x0-55, 145);
  pl_text(w, 16, ytxt, "Segments aligned (-+-)?", 23);
  ytxt += 2*fonth;
# endif

# if TRY_ELLIPSE
  box_draw(w, x0-61, 170, x0-42, 189, 0);
  box_draw(w, x0-40, 170, x0-21, 189, 0);
  pl_ellipse(w, x0-59, 172, x0-44, 187, 0);
  pl_ellipse(w, x0-38, 172, x0-23, 187, 1);
  pl_text(w, 16, ytxt, "Circs/rects OK (1 pixel gaps)?", 30);
  ytxt += 2*fonth;
# endif

# if TRY_RECT
  box_draw(w, x0-61, 200, x0-21, 230, 0);
  pl_pen(w, 4, PL_SOLID | PL_SQUARE);
  pl_rect(w, x0-57, 204, x0-24, 227, 1);
  pl_pen(w, 3, PL_SOLID | PL_SQUARE);
  pl_rect(w, x0-53, 208, x0-29, 222, 1);
  pl_pen(w, 1, PL_SOLID);
  pl_rect(w, x0-50, 211, x0-32, 219, 1);
  pl_rect(w, x0-48, 213, x0-33, 218, 0);
# endif

  ytxt -= fonth;
  if (ytxt<190) ytxt = 190;
  /* top set of curves tests thin linestyles */
  curve_draw(w, 20, ytxt, -50, -50);
  pl_pen(w, 1, PL_DASH);
  curve_draw(w, 40, ytxt, 50, 50);
  pl_pen(w, 1, PL_DOT);
  curve_draw(w, 110, ytxt, -50, 50);
  pl_pen(w, 1, PL_DASHDOT);
  curve_draw(w, 130, ytxt, 50, -50);
  pl_pen(w, 1, PL_DASHDOTDOT);
  curve_draw(w, 200, ytxt, -50, -50);
  pl_pen(w, 1, PL_SOLID);
  curve_draw(w, 220, ytxt, 50, 50);

# if TRY_CLIP
  pl_clip(w, 315,ytxt+15, 345,ytxt+45);
  box_draw(w, 300,ytxt, 330,ytxt+30, 1);
  box_draw(w, 330,ytxt+30, 360,ytxt+60, 1);
  pl_clip(w, 0,0, 0,0);
  box_draw(w, 313,ytxt+13, 346,ytxt+46, 0);
# endif
  ytxt += 70;

  /* telescope segments test line widths, should be centered on y=ytxt */
  seg_draw(w, 20, ytxt, 60, ytxt);
  pl_pen(w, 2, PL_SOLID);
  seg_draw(w, 60, ytxt, 100, ytxt);
  pl_pen(w, 3, PL_SOLID);
  seg_draw(w, 100, ytxt, 140, ytxt);
  pl_pen(w, 4, PL_SOLID);
  seg_draw(w, 140, ytxt, 180, ytxt);
  pl_pen(w, 5, PL_SOLID);
  seg_draw(w, 180, ytxt, 220, ytxt);
  pl_pen(w, 6, PL_SOLID);
  seg_draw(w, 220, ytxt, 260, ytxt);
  pl_pen(w, 7, PL_SOLID);
  seg_draw(w, 260, ytxt, 300, ytxt);
  pl_pen(w, 8, PL_SOLID);
  seg_draw(w, 300, ytxt, 340, ytxt);
  pl_pen(w, 9, PL_SOLID);
  seg_draw(w, 340, ytxt, 380, ytxt);
  pl_pen(w, 5, PL_SOLID);
  ytxt += 20;

  /* lower set of curves test thick linestyles */
  curve_draw(w, 20, ytxt, -50, -50);
  pl_pen(w, 5, PL_DASH);
  curve_draw(w, 40, ytxt, 50, 50);
  pl_pen(w, 5, PL_DOT);
  curve_draw(w, 110, ytxt, -50, 50);
  pl_pen(w, 5, PL_DASHDOT);
  curve_draw(w, 130, ytxt, 50, -50);
  pl_pen(w, 5, PL_DASHDOTDOT);
  curve_draw(w, 200, ytxt, -50, -50);
  pl_pen(w, 5, PL_SOLID);
  curve_draw(w, 220, ytxt, 50, 50);

  /* triangles test square joins --
   * upper one should have sharp points, no notch at closure,
   * lowere one should have rounded corners */
  pl_pen(w, 5, PL_SOLID | PL_SQUARE);
  tri_draw(w, 300,ytxt, 340,ytxt, 300,ytxt+40, 0);
  pl_pen(w, 5, PL_SOLID);
  tri_draw(w, 350,ytxt+50, 350,ytxt+10, 310,ytxt+50, 0);
  ytxt += 60+fonth;

  hcour = pl_txheight(scr, PL_COURIER | PL_BOLD, 14, &bcour);
  wcour = pl_txwidth(scr, "M", 1, PL_COURIER | PL_BOLD, 14);
# if TRY_GUI
  ygui = ytxt;
  ytxt = ygui+fonth;
  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  pl_text(w, 20, ytxt, "on_focus", 8);
  xfoc = 20+3*wcour;
  pl_text(w, xfoc+7*wcour, ytxt, "on_key", 6);
  xkey = xfoc+6*wcour;
  pl_text(w, xkey+10*wcour, ytxt, "on_click", 8);
  xclick = xkey+11*wcour;
  yhi0 = ytxt-bcour;
  yhi1 = yhi0+hcour;
  xhi0 = xclick-wcour;
  xhi1 = xhi0 + pl_txwidth(scr, "on_click", 8, PL_COURIER | PL_BOLD, 14);
  hion = 0;
  pl_text(w, xclick+9*wcour, ytxt, "on_motion", 9);
  xmotion = xclick+9*wcour;
# endif
}

void
seg_draw(pl_win_t *w, int x0, int y0, int x1, int y1)
{
  int x[2], y[2];
  x[0] = x0;  x[1] = x1;  y[0] = y0;  y[1] = y1;
  pl_i_pnts(w, x, y, 2);
  pl_lines(w);
}

void
box_draw(pl_win_t *w, int x0, int y0, int x1, int y1, int fillit)
{
  int x[5], y[5];
  x[0] = x0;  x[1] = x1;  x[2] = x1;  x[3] = x0;  x[4] = x0;
  y[0] = y0;  y[1] = y0;  y[2] = y1;  y[3] = y1;  y[4] = y0;
  pl_i_pnts(w, x, y, 5);
  if (!fillit) pl_lines(w);
  else         pl_fill(w, 2);
}

void
tri_draw(pl_win_t *w, int x0,int y0, int x1,int y1, int x2,int y2, int fillit)
{
  int x[4], y[4];
  x[0] = x0;  x[1] = x1;  x[2] = x2;  x[3] = x0;
  y[0] = y0;  y[1] = y1;  y[2] = y2;  y[3] = y0;
  pl_i_pnts(w, x, y, 4);
  if (!fillit) pl_lines(w);
  else         pl_fill(w, 2);
}

extern double cv[6];
double cv[6] = { 1., 1., 0.896575, 0.517638, 0.267949, 0. };

void
curve_draw(pl_win_t *w, int x0, int y0, int width, int height)
{
  int x[6], y[6], i;
  if (width<0) x0 -= width;
  if (height<0) y0 -= height;
  for (i=0 ; i<6 ; i++) {
    x[i] = x0 + (int)(cv[i]*width);
    y[i] = y0 + (int)(cv[5-i]*height);
  }
  pl_i_pnts(w, x, y, 6);
  pl_lines(w);
}

void
advanced_draw(pl_win_t *w)
{
  int x0, y0;
  int xtxt = 20;
  int ytxt = 10;
  int base, height, width;
  int i, j, ii, jj;

  pl_clear(win2);
# if TRY_PALETTE
  reset_palette();
# endif
  pl_color(w, PL_FG);

  /* cant tell what order simple_draw and advanced_draw run... */
  hcour = pl_txheight(scr, PL_COURIER | PL_BOLD, 14, &bcour);
  wcour = pl_txwidth(scr, "M", 1, PL_COURIER | PL_BOLD, 14);

  ytxt += pl_txheight(scr, PL_COURIER, 14, &base);
  pl_font(w, PL_COURIER, 14, 0);
  pl_text(w, xtxt,ytxt, "Courier 14", 10);
  xtxt += pl_txwidth(scr, "Courier 14", 10, PL_COURIER, 14);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  pl_text(w, xtxt,ytxt, " Bold", 5);
  xtxt += pl_txwidth(scr, " Bold", 5, PL_COURIER | PL_BOLD, 14);
  pl_font(w, PL_COURIER | PL_ITALIC, 14, 0);
  pl_text(w, xtxt,ytxt, " Italic", 7);
  xtxt += pl_txwidth(scr, " Italic", 7, PL_COURIER | PL_ITALIC, 14);
  pl_font(w, PL_COURIER | PL_BOLD | PL_ITALIC, 14, 0);
  pl_text(w, xtxt,ytxt, " Bold-Italic", 12);

  ytxt += 6 + pl_txheight(scr, PL_HELVETICA, 10, &base);
  xtxt = 20;
  pl_font(w, PL_HELVETICA, 10, 0);
  pl_text(w, xtxt,ytxt, "Helvetica 10", 12);
  xtxt += pl_txwidth(scr, "Helvetica 10", 12, PL_HELVETICA, 10);
  pl_font(w, PL_HELVETICA | PL_BOLD, 10, 0);
  pl_text(w, xtxt,ytxt, " Bold", 5);
  xtxt += pl_txwidth(scr, " Bold", 5, PL_HELVETICA | PL_BOLD, 10);
  pl_font(w, PL_HELVETICA | PL_ITALIC, 10, 0);
  pl_text(w, xtxt,ytxt, " Italic", 7);
  xtxt += pl_txwidth(scr, " Italic", 7, PL_HELVETICA | PL_ITALIC, 10);
  pl_font(w, PL_HELVETICA | PL_BOLD | PL_ITALIC, 10, 0);
  pl_text(w, xtxt,ytxt, " Bold-Italic", 12);

  ytxt += 6 + pl_txheight(scr, PL_TIMES, 12, &base);
  xtxt = 20;
  pl_font(w, PL_TIMES, 12, 0);
  pl_text(w, xtxt,ytxt, "Times 12", 8);
  xtxt += pl_txwidth(scr, "Times 12", 8, PL_TIMES, 12);
  pl_font(w, PL_TIMES | PL_BOLD, 12, 0);
  pl_text(w, xtxt,ytxt, " Bold", 5);
  xtxt += pl_txwidth(scr, " Bold", 5, PL_TIMES | PL_BOLD, 12);
  pl_font(w, PL_TIMES | PL_ITALIC, 12, 0);
  pl_text(w, xtxt,ytxt, " Italic", 7);
  xtxt += pl_txwidth(scr, " Italic", 7, PL_TIMES | PL_ITALIC, 12);
  pl_font(w, PL_TIMES | PL_BOLD | PL_ITALIC, 12, 0);
  pl_text(w, xtxt,ytxt, " Bold-Italic", 12);

  ytxt += 6 + pl_txheight(scr, PL_NEWCENTURY, 18, &base);
  xtxt = 20;
  pl_font(w, PL_NEWCENTURY, 18, 0);
  pl_text(w, xtxt,ytxt, "Newcentury 18", 13);
  xtxt += pl_txwidth(scr, "Newcentury 18", 13, PL_NEWCENTURY, 18);
  pl_font(w, PL_NEWCENTURY | PL_BOLD, 18, 0);
  pl_text(w, xtxt,ytxt, " Bold", 5);
  xtxt += pl_txwidth(scr, " Bold", 5, PL_NEWCENTURY | PL_BOLD, 18);
  pl_font(w, PL_NEWCENTURY | PL_ITALIC, 18, 0);
  pl_text(w, xtxt,ytxt, " Italic", 7);
  xtxt += pl_txwidth(scr, " Italic", 7, PL_NEWCENTURY | PL_ITALIC, 18);
  pl_font(w, PL_NEWCENTURY | PL_BOLD | PL_ITALIC, 18, 0);
  pl_text(w, xtxt,ytxt, " Bold-Italic", 12);

  ytxt += 6 + pl_txheight(scr, PL_SYMBOL, 14, &base);
  xtxt = 20;
  pl_font(w, PL_SYMBOL, 14, 0);
  pl_text(w, xtxt,ytxt, "Symbol 14", 9);
  xtxt += pl_txwidth(scr, "Symbol 14", 9, PL_SYMBOL, 14);

  xtxt += 20;
  pl_pen(w, 1, PL_SOLID);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  pl_text(w, xtxt,ytxt, "Square", 6);
  height = pl_txheight(scr, PL_COURIER | PL_BOLD, 14, &base);
  width = pl_txwidth(scr, "Square", 6, PL_COURIER | PL_BOLD, 14);
  x0 = xtxt;
  y0 = ytxt -= base;
  base = height-base;  /* descent */
  box_draw(w, xtxt,ytxt, xtxt+width,ytxt+height, 0);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 3);
  pl_text(w, xtxt+width+base,ytxt, "Square", 6);
  box_draw(w, xtxt+width,ytxt, xtxt+width+height,ytxt+width, 0);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 2);
  pl_text(w, xtxt+width+height,ytxt+width+base, "Square", 6);
  box_draw(w, xtxt+width+height,ytxt+width, xtxt+height,ytxt+width+height, 0);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 1);
  pl_text(w, xtxt+height-base,ytxt+width+height, "Square", 6);
  box_draw(w, xtxt+height,ytxt+width+height, xtxt,ytxt+height, 0);

  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  xtxt += width+3*height;
  width /= 6;
  pl_color(w, PL_WHITE);
  box_draw(w, xtxt, ytxt, xtxt+16*width, ytxt+6*height, 1);
  pl_color(w, PL_BLACK);
  pl_text(w, xtxt+width,ytxt+height-base, "black", 5);
  box_draw(w, xtxt, ytxt+height, xtxt+7*width, ytxt+2*height, 1);
  pl_color(w, PL_WHITE);
  pl_text(w, xtxt+width,ytxt+2*height-base, "white", 5);
  pl_color(w, PL_RED);
  pl_text(w, xtxt+8*width,ytxt+height-base, "red", 3);
  pl_color(w, PL_GREEN);
  pl_text(w, xtxt+8*width,ytxt+2*height-base, "green", 5);
  pl_color(w, PL_BLUE);
  pl_text(w, xtxt+8*width,ytxt+3*height-base, "blue", 4);
  pl_color(w, PL_CYAN);
  pl_text(w, xtxt+8*width,ytxt+4*height-base, "cyan", 4);
  pl_color(w, PL_MAGENTA);
  pl_text(w, xtxt+8*width,ytxt+5*height-base, "magenta", 7);
  pl_color(w, PL_YELLOW);
  pl_text(w, xtxt+8*width,ytxt+6*height-base, "yellow", 6);
  pl_color(w, PL_BG);
  box_draw(w, xtxt, ytxt+2*height, xtxt+7*width, ytxt+4*height, 1);
  pl_color(w, PL_FG);
  pl_text(w, xtxt+width,ytxt+3*height-base, "fg", 2);
  pl_text(w, xtxt+width,ytxt+4*height-base, "xorfg", 5);
  pl_color(w, PL_XOR);
  box_draw(w, xtxt, ytxt+3*height, xtxt+7*width, ytxt+4*height, 1);
  ytxt += 6*height + 4;

# if TRY_NDXCELL
  {
    unsigned char cells[1200];
    pl_col_t fgpix, bgpix, pix;
    for (j=0 ; j<400 ; j+=80)
      for (i=0 ; i<20 ; i+=4)
        for (jj=0 ; jj<80 ; jj+=20)
          for (ii=0 ; ii<4 ; ii++)
            cells[j+i+jj+ii] = (unsigned char)(PL_BG - (i/4+j/16)%10);
    pl_color(w, PL_FG);
    xtxt = 20;
    box_draw(w, xtxt-1, ytxt-1, xtxt+20, ytxt+20, 0);
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+20,ytxt+20);
    xtxt += 30;
    pl_color(w, PL_FG);
    box_draw(w, xtxt-1, ytxt-1, xtxt+10, ytxt+10, 0);
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+10,ytxt+10);
    xtxt += 20;
    pl_color(w, PL_FG);
    box_draw(w, xtxt-1, ytxt-1, xtxt+40, ytxt+40, 0);
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+40,ytxt+40);
    xtxt += 50;
    pl_color(w, PL_FG);
    box_draw(w, xtxt-1, ytxt-1, xtxt+10, ytxt+40, 0);
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+10,ytxt+40);
    xtxt += 20;
    pl_color(w, PL_FG);
    box_draw(w, xtxt-1, ytxt-1, xtxt+40, ytxt+10, 0);
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+40,ytxt+10);

    xtxt += 50;
    for (jj=0 ; jj<25 ; jj+=5)
      for (ii=0 ; ii<5 ; ii++)
        cells[jj+ii] = (unsigned char)(PL_BG - (ii+jj)%10);
    box_draw(w, xtxt-1, ytxt-1, xtxt+40, ytxt+40, 0);
    pl_ndx_cell(w, cells, 5,5, xtxt,ytxt,xtxt+40,ytxt+40);

#  if TRY_RGBREAD
    pl_rgb_read(w, cells, x0,y0, x0+20,y0+20);
    fgpix = PL_RGB(cells[0],cells[1],cells[2]);
    bgpix = PL_RGB(cells[1197],cells[1198],cells[1199]);
    for (j=0 ; j<400 ; j+=20)
      for (i=0 ; i<20 ; i++) {
        pix = PL_RGB(cells[3*(i+j)],cells[3*(i+j)+1],cells[3*(i+j)+2]);
        if (pix==fgpix) cells[i+j] = PL_FG;
        else if (pix==bgpix) cells[i+j] = PL_BG;
        else cells[i+j] = PL_RED;
      }
    xtxt += 50;
    pl_ndx_cell(w, cells, 20,20, xtxt,ytxt,xtxt+20,ytxt+20);
#  endif

    ytxt += 50;
  }
# endif

# if TRY_PALETTE
  pl_color(w, PL_FG);
  xtxt = 20;
  pl_text(w, xtxt,ytxt+height-base, "palette:", 8);
  xpal = xtxt + 8*width + 10;
  ypal = ytxt;
  pl_color(win2, PL_BG);
  box_draw(win2, xpal-3*wcour,ypal+2*hcour-bcour,
           xpal-2*wcour,ypal+3*hcour-bcour, 1);
  pl_color(win2, PL_FG);
  pl_font(win2, PL_COURIER | PL_BOLD, 14, 0);
  {
    char palno[8];
    palno[0] = pal_number+'0';
    palno[1] = '\0';
    pl_text(win2, xpal-3*wcour,ypal+2*hcour, palno, 1);
    for (i=0 ; i<200 ; i++) {
      pl_color(win2, i);
      box_draw(win2, xpal+i,ypal, xpal+i+1,ypal+40, 1);
    }
  }
# endif

#if TRY_RGBCELL
  rgbcell_on = !rgbcell_on;
  rgbcell_act(0);
#endif
}

# if TRY_PALETTE
static void
reset_palette(void)
{
  int i;
  pl_col_t colors[200];
  if (!win2) return;
  if (pal_number==0) {
    pl_palette(win2, colors, 0);
  } else {
    make_rainbow(pal_number, colors);
    pl_palette(win2, colors, 200);
    for (i=0 ; i<200 ; i++) colors[i] = 0;  /* check for no effect */
  }
}
# endif

static void
make_rainbow(int pn, pl_col_t *colors)
{
  long i, j, k;
  for (i=0 ; i<200 ; i++) {
    if (pn==1) {
      j = (255*i)/199;
      colors[i] = PL_RGB(j,j,j);
    } else if (pn==2) {
      if (i<=33) {
        j = (251*(33-i))/66;
        if (j>127) k=(255*(255-j))/j, j=255;
        else k=255, j=(255*j)/(255-j);
        colors[i] = PL_RGB(k,0,j);
      } else if (i<=100) {
        j = (251*(100-i))/66;
        if (j>127) k=(255*(255-j))/j, j=255;
        else k=255, j=(255*j)/(255-j);
        colors[i] = PL_RGB(j,k,0);
      } else if (i<=167) {
        j = (251*(167-i))/66;
        if (j>127) k=(255*(255-j))/j, j=255;
        else k=255, j=(255*j)/(255-j);
        colors[i] = PL_RGB(0,j,k);
      } else {
        j = (251*(233-i))/66;
        if (j>127) k=(255*(255-j))/j, j=255;
        else k=255, j=(255*j)/(255-j);
        colors[i] = PL_RGB(k,0,j);
      }
    }
  }
}

static void
feep_act(char *args)
{
  if (win1) pl_feep(win1);
}

static void
redraw_act(char *args)
{
  if (win1) {
    pl_clear(win1);
    simple_draw(scr, win1, w0, h0);
  } else {
    win1 = pl_window(scr, w0, h0, "test2d window", PL_BG, phints, &win1);
  }
  if (!win2) {
    int hints = phints | PL_NORESIZE | PL_NOKEY | PL_NOMOTION;
    if (rgbcell_on) hints |= PL_RGBMODEL;
    win2 = pl_window(scr, 400, 400, "test2d advanced", PL_BG, hints, &win2);
  } else {
    advanced_draw(win2);
  }
}

static void
private_act(char *args)
{
  phints ^= PL_PRIVMAP;
  if (win2) {
    int hints = phints | PL_NORESIZE | PL_NOKEY | PL_NOMOTION;
    pl_win_t *w = win2;
    win2 = 0;
    pl_destroy(w);
    if (rgbcell_on) hints |= PL_RGBMODEL;
    win2 = pl_window(scr, 400, 400, "test2d advanced", PL_BG, hints, &win2);
  }
# if TRY_STDINIT
  sprintf(pl_wkspc.c, "test2d: using %s colormap\n",
          (phints&PL_PRIVMAP)? "private" : "shared");
  pl_stdout(pl_wkspc.c);
# endif
}

static void
rgb_act(char *args)
{
  phints ^= PL_RGBMODEL;
  if (win2) {
    pl_win_t *w = win2;
    win2 = 0;
    pl_destroy(w);
    win2 = pl_window(scr, 400, 400, "test2d advanced", PL_BG,
                   phints | PL_NORESIZE | PL_NOKEY | PL_NOMOTION, &win2);
  }
# if TRY_STDINIT
  sprintf(pl_wkspc.c, "test2d: using %s colormap\n",
          (phints&PL_RGBMODEL)? "5-9-5 true" : "pseudo");
  pl_stdout(pl_wkspc.c);
# endif
}

# if TRY_RESIZE
static void
resize_act(char *args)
{
  char *arg2 = args;
  int w = (int)strtol(args, &arg2, 0);
  int h = (int)strtol(arg2, (char **)0, 0);
  if (win3) return;
  if (win1) pl_raise(win1);
  if (win1 && w>50 && w<1000 && h>50 && w<1000) {
    pl_resize(win1, w, h);
    if (w!=w0 || h!=h0) {
#  if TRY_OFFSCREEN
      if (offscr) {
        pl_destroy(offscr);
        offscr = pl_offscreen(win1, 32, h);
      }
#  endif
      simple_draw(scr, win1, w, h);
      w0 = w;
      h0 = h;
    }
  }
#  if TRY_STDINIT
  sprintf(pl_wkspc.c, "test2d: %d pixels X %d pixels\n", w, h);
  pl_stdout(pl_wkspc.c);
#  endif
}
# endif

# if TRY_CLIPBOARD
static void
paste_act(char *args)
{
  if (!win1) return;
#  if TRY_STDINIT
  pl_stdout("paste got:");
  pl_stdout(pl_spaste(win1));
  pl_stdout("\n");
#  endif
}
# endif

# if TRY_METAFILE
static void
meta_act(char *args)
{
  char *path = strtok(args, " \t\r\n");
  if (path && win1) {
    pl_win_t *meta = pl_metafile(win1, path, 0, 0, w0, h0, 0);
    if (meta) {
      simple_draw(scr, meta, w0, h0);
      pl_destroy(meta);
#  if TRY_STDINIT
      sprintf(pl_wkspc.c, "test2d: wrote %s\n", path);
      pl_stdout(pl_wkspc.c);
#  endif
    } else {
#  if TRY_STDINIT
      pl_stdout("test2d: pl_metafile() not implemented\n");
#  endif
    }
  } else {
#  if TRY_STDINIT
    pl_stdout("test2d: meta needs pathname and basic window\n");
#  endif
  }
}
# endif

# if TRY_PALETTE
static void
pal_act(char *args)
{
  pal_number++;
  if (pal_number>2) pal_number = 0;
  advanced_draw(win2);
}
# endif

# if TRY_RGBCELL
static void
rgbcell_act(char *args)
{
  rgbcell_on = !rgbcell_on;
  if (win2 && rgbcell_on) {
    int i, j, k;
    unsigned char *rgbs = pl_malloc(3*200*50);
    pl_col_t colors[200];
    make_rainbow(2, colors);
    for (i=j=0 ; i<600 ; i+=3,j++) {
      rgbs[i  ] = (unsigned char)PL_R(colors[j]);
      rgbs[i+1] = (unsigned char)PL_G(colors[j]);
      rgbs[i+2] = (unsigned char)PL_B(colors[j]);
    }
    for (j=600,k=49 ; j<30000 ; j+=600,k--)
      for (i=0 ; i<600 ; i+=3) {
        rgbs[j+i  ] = (k*(int)rgbs[i  ])/50;
        rgbs[j+i+1] = (k*(int)rgbs[i+1])/50;
        rgbs[j+i+2] = (k*(int)rgbs[i+2])/50;
      }
    pl_rgb_cell(win2, rgbs, 200, 50, xpal,ypal+50, xpal+200,ypal+100);
    pl_free(rgbs);
  } else if (win2) {
    pl_color(win2, PL_BG);
    box_draw(win2, xpal,ypal+50, xpal+200,ypal+100, 1);
  }
}
# endif
#endif

#if TRY_GUI
void
on_focus(void *c,int in)
{
  pl_win_t *w = *(pl_win_t**)c;
  if (w!=win1 || win3 || !w) return;
  pl_color(w, PL_BG);
  box_draw(w, xfoc, ygui-bcour+hcour, xfoc+3*wcour, ygui-bcour, 1);
  pl_color(w, PL_FG);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  if (in) pl_text(w, xfoc, ygui, "IN", 2);
  else pl_text(w, xfoc, ygui, "OUT", 3);
}

void
on_key(void *c,int k,int md)
{
  char txt[8];
  pl_win_t *w = *(pl_win_t**)c;
  if (w!=win1 || win3) return;
  pl_color(w, PL_BG);
  box_draw(w, xkey, ygui-bcour+hcour, xkey+8*wcour, ygui-bcour, 1);
  pl_color(w, PL_FG);
  pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
  txt[0] = (md&PL_SHIFT)? 'S' : ' ';
  txt[1] = (md&PL_CONTROL)? 'C' : ' ';
  txt[2] = (md&PL_META)? 'M' : ' ';
  txt[3] = (md&PL_ALT)? 'A' : ' ';
  txt[4] = (md&PL_COMPOSE)? '+' : ' ';
  txt[5] = (md&PL_KEYPAD)? '#' : ' ';
  if (k=='\177') {
    txt[6] = '^';
    txt[7] = '?';
  } else if (k<PL_LEFT) {
    txt[6] = (k<' ')? '^' : ' ';
    txt[7] = (k<' ')? k+'@' : k;
  } else if (k<=PL_F12) {
    txt[6] = 'F';
    if (k==PL_LEFT) txt[7] = '<';
    else if (k==PL_RIGHT) txt[7] = '>';
    else if (k==PL_UP) txt[7] = '^';
    else if (k==PL_DOWN) txt[7] = 'V';
    else if (k==PL_PGUP) txt[7] = 'U';
    else if (k==PL_PGDN) txt[7] = 'D';
    else if (k==PL_HOME) txt[7] = 'H';
    else if (k==PL_END) txt[7] = 'E';
    else if (k==PL_INSERT) txt[7] = 'I';
    else if (k<PL_F10) txt[7] = (k-PL_F0)+'0';
    else txt[7] = (k-PL_F10)+'a';
  } else {
    txt[6] = '?';
    txt[7] = '?';
  }
  pl_text(w, xkey, ygui, txt, 8);
}

static unsigned long multi_ms = 0;

void
on_click(void *c,int b,int md,int x,int y, unsigned long ms)
{
  char txt[64];
  pl_win_t *w = *(pl_win_t**)c;
  int down = (md&(1<<(b+2)))==0;
  if (w==win1 && !win3) {
    unsigned long dms = multi_ms? ms-multi_ms : 0;
    if (down) multi_ms = ms;
    pl_color(w, PL_BG);
    box_draw(w, xkey, ygui-bcour+hcour, xkey+8*wcour, ygui-bcour, 1);
    box_draw(w, xclick, ygui-bcour+hcour, xclick+7*wcour, ygui-bcour, 1);
    pl_color(w, PL_FG);
    pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
    txt[0] = (md&PL_SHIFT)? 'S' : ' ';
    txt[1] = (md&PL_CONTROL)? 'C' : ' ';
    txt[2] = (md&PL_META)? 'M' : ' ';
    txt[3] = (md&PL_ALT)? 'A' : ' ';
    txt[4] = (md&PL_COMPOSE)? '+' : ' ';
    txt[5] = (md&PL_KEYPAD)? '#' : ' ';
    txt[6] = '-';
    txt[7] = '>';
    pl_text(w, xkey, ygui, txt, 8);
    sprintf(txt, "%03o %03o", md&0370, 1<<(b+2));
    pl_text(w, xclick, ygui, txt, 7);
    if (down && dms && dms<250 && x>=xhi0 && x<xhi1 && y>=yhi0 && y<yhi1) {
      pl_color(w, PL_XOR);
      box_draw(w, xhi0, yhi0, xhi1, yhi1, 1);
      hion = !hion;
# if TRY_CLIPBOARD
      if (hion) pl_scopy(w, "on_click", 8);
      else pl_scopy(w, (char *)0, 0);
# endif
    }
# if TRY_OFFSCREEN
    if (y<=ygui && x<16 && !down) {
      animating = !animating;
      if (animating) pl_set_alarm(0.05, &on_animate, w);
      else pl_clr_alarm(&on_animate, (void *)0);
    }
# endif
# if TRY_MENU
    if (!win3 && down && dms && dms<250 && y<ygui) {
      int x0, y0;
      pl_winloc(w, &x0, &y0);
      win3 = pl_menu(scr, 12*wcour, 6*hcour, x0+x, y0+y, PL_GRAYA, &win3);
      win3_hi = win3_flag = win4_hi = 0;
    }
# endif
  } else if (win3) {
    if (down || win3_flag) {
      pl_win_t *w3 = win3;
      if (win3_hi!=2) {  /* win4 already destroyed */
        win3 = 0;
        pl_destroy(w3);
# if TRY_STDINIT
        sprintf(pl_wkspc.c, "test2d: menu item %d\n", win3_hi);
        pl_stdout(pl_wkspc.c);
        prompt_issued = 0;
# endif
      } else {
        pl_win_t *w4 = win4;
        int skip = 0;
        if (!win4_hi) {
          if (w!=win3) {  /* get x,y relative to win3 */
            int x0, y0;
            pl_winloc(w, &x0, &y0);
            x += x0;
            y += y0;
            pl_winloc(win3, &x0, &y0);
            x -= x0;
            y -= y0;
          }
          skip = (x>=0 && x<12*wcour && y>=0 && y<6*hcour);
        }
        if (!skip) {
          win4 = win3 = 0;
          pl_destroy(w4);
          pl_destroy(w3);
# if TRY_STDINIT
          sprintf(pl_wkspc.c, "test2d: submenu item %d\n", win4_hi);
          pl_stdout(pl_wkspc.c);
          prompt_issued = 0;
# endif
        }
      }
    }
  }
}

static int cur_cursor = 0;

void
on_motion(void *c,int md,int x,int y)
{
  char txt[64];
  pl_win_t *w = *(pl_win_t**)c;
  if (w==win1 && !win3) {
    pl_color(w, PL_BG);
    box_draw(w, xmotion, ygui-bcour+hcour, xmotion+12*wcour, ygui-bcour, 1);
    pl_color(w, PL_FG);
    pl_font(w, PL_COURIER | PL_BOLD, 14, 0);
    sprintf(txt, "% 4d,% 4d  ", x, y);
    pl_text(w, xmotion, ygui, txt, strlen(txt));
# if TRY_CURSOR
    {
      int i;
      if (y > yhi1) {
        i = ((PL_NONE+1)*x)/w0;
        if (i<0) i = 0;
        if (i>PL_NONE) i = PL_NONE;
      } else {
        i = PL_SELECT;
      }
      if (i!=cur_cursor) {
        cur_cursor = i;
        pl_cursor(w, i);
      }
    }
# endif
  } else if (win3) {
    int in_menu = (x>=4 && x<12*wcour-4 && y>=4 && y<6*hcour-4);
    int old3hi = win3_hi;
    int old4hi = win4_hi;
    in_menu = in_menu && (w==win3 || w==win4);
    if (!in_menu) {
      int x0, y0;
      pl_winloc(w, &x0, &y0);
      x0 += x;
      y0 += y;
      if (w!=win3) {
        pl_winloc(win3, &x, &y);
        x = x0 - x;
        y = y0 - y;
        in_menu = (x>=4 && x<12*wcour-4 && y>=4 && y<6*hcour-4);
        if (in_menu) w = win3;
      }
      if (win4 && w!=win4) {
        pl_winloc(win4, &x, &y);
        x = x0 - x;
        y = y0 - y;
        in_menu = (x>=4 && x<12*wcour-4 && y>=4 && y<6*hcour-4);
        if (in_menu) w = win4;
      }
    }
    if (in_menu) {
      int *pwin_hi = (w==win3)? &win3_hi : &win4_hi;
      if (y<2*hcour)      *pwin_hi = 1;
      else if (y<4*hcour) *pwin_hi = 2;
      else                *pwin_hi = 3;
    } else {
      if (!win4) win3_hi = 0;
      win4_hi = 0;
    }
    if (win3_hi!=old3hi) {
      if (old3hi) hilite(win3, 0, 2*(old3hi-1)*hcour,
                         12*wcour, 2*old3hi*hcour, 0);
      if (win3_hi) hilite(win3, 0, 2*(win3_hi-1)*hcour,
                          12*wcour, 2*win3_hi*hcour, 1);
    }
    if (win4 && win4_hi!=old4hi) {
      if (old4hi) hilite(win4, 0, 2*(old4hi-1)*hcour,
                         12*wcour, 2*old4hi*hcour, 0);
      if (win4_hi) hilite(win4, 0, 2*(win4_hi-1)*hcour,
                          12*wcour, 2*win4_hi*hcour, 1);
    }
    if (!win3_flag && win3_hi) win3_flag = 1;
    if (win3_hi==2) {
# if TRY_MENU
      if (!win4) {
        int x0, y0;
        pl_winloc(win3, &x0, &y0);
        x0 += 12*wcour;
        y0 += 2*hcour;
        win4 = pl_menu(scr, 12*wcour, 6*hcour, x0, y0, PL_GRAYA, &win4);
        win4_hi = 0;
      }
# endif
    } else if (win4) {
      pl_win_t *w4 = win4;
      win4 = 0;
      pl_destroy(w4);
    }
  }
}

void
hilite(pl_win_t *w, int x0, int y0, int x1, int y1, int onoff)
{
  pl_color(w, onoff? PL_WHITE : PL_GRAYA);
  box_draw(w, x0, y0+4, x0+4, y1, 1);
  box_draw(w, x0, y0, x1, y0+4, 1);
  if (onoff) pl_color(w, PL_BLACK);
  box_draw(w, x0+4, y1-4, x1, y1, 1);
  box_draw(w, x1-4, y0+4, x1, y1-4, 1);
  if (onoff) {
    tri_draw(w, x0, y1, x0+4, y1, x0+4, y1-4, 1);
    tri_draw(w, x1-4, y0+4, x1, y0+4, x1, y0, 1);
  }
}

void
on_expose(void *c, int *xy)
{
  pl_win_t **w = c;
  if (w == &win1) {
# if TRY_OFFSCREEN
    if (!offscr) offscr = pl_offscreen(*w, 32, h0);
# endif
    simple_draw(scr, *w, w0, h0);
  } else if (w == &win2) {
    advanced_draw(*w);
  } else if (w == &win3) {
    pl_win_t *ww = *w;
    pl_color(ww, PL_BLACK);
    pl_font(ww, PL_COURIER | PL_BOLD, 14, 0);
    pl_text(ww, wcour, 2*hcour-bcour, "item no. 1", 10);
    pl_text(ww, wcour, 4*hcour-bcour, "sub menu >", 10);
    pl_text(ww, wcour, 6*hcour-bcour, "item no. 3", 10);
    win3_hi = 0;
  } else if (w == &win4) {
    pl_win_t *ww = *w;
    pl_color(ww, PL_BLACK);
    pl_font(ww, PL_COURIER | PL_BOLD, 14, 0);
    pl_text(ww, wcour, 2*hcour-bcour, "subitem #1", 10);
    pl_text(ww, wcour, 4*hcour-bcour, "subitem #2", 10);
    pl_text(ww, wcour, 6*hcour-bcour, "subitem #3", 10);
    win4_hi = 0;
  }
}

void
on_deselect(void *c)
{
  if (hion) {
    pl_win_t *w = *(pl_win_t **)c;
    if (w != win1) return;
    pl_color(w, PL_XOR);
    box_draw(w, xhi0, yhi0, xhi1, yhi1, 1);
    hion = 0;
# if TRY_STDINIT
    pl_stdout("test2d: on_deselect called");
    pl_stdout("\n");
    prompt_issued = 0;
# endif
  }
}

void
on_destroy(void *c)
{
  pl_win_t *w = *(pl_win_t **)c;
  int in_menu = (w==win4 || w==win3);
  if (w==win4) win4 = 0;
  else if (w==win3) win3 = 0;
  else if (w==win2) win2 = 0;
  else if (w==win1) win1 = 0;
# if TRY_STDINIT
  if (!in_menu) {
    pl_stdout("test2d: on_destroy called\n");
    prompt_issued = 0;
  }
# endif
}

void
on_resize(void *c,int w,int h)
{
  if (w!=w0 || h!=h0) {
    pl_win_t **win = c;
    if (win != &win1) return;
# if TRY_OFFSCREEN
    if (offscr) {
      pl_destroy(offscr);
      offscr = pl_offscreen(*win, 32, h);
    }
# endif
    if (w>0 && h>0) {
      simple_draw(scr, *win, w, h);
      w0 = w;
      h0 = h;
    } else {
      w0 = h0 = 1;
    }
  }
}

static char *scr_name[] = {
  "<nil bug>", "<unrecognized bug>", "scr", "scr2", "scremote" };

void
on_panic(pl_scr_t *screen)
{
  int scrn = 1;
  if (screen) {
    if (screen==scr) {
      scrn = 2;
      scr = 0;
      panic_count++;
    } else if (screen==scr2) {
      scrn = 3;
      scr2 = 0;
    } else if (screen==scremote) {
      scrn = 4;
      scremote = 0;
    }
    scrn = 1;
  }
# if TRY_STDINIT
  sprintf(pl_wkspc.c, "test2d: on_panic called on screen %s\n",
          scr_name[scrn]);
  pl_stdout(pl_wkspc.c);
  prompt_issued = 0;
# endif
}
#endif
