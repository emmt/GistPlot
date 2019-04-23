/*
 * $Id: gread.c,v 1.2 2009-10-19 04:37:51 dhmunro Exp $
 * Define gd_drawing_t gread read routine for GIST
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "gist2.h"
#include "gist/private.h"
#include "play/io.h"
#include "play/std.h"
#include "play2.h"
#include <errno.h>

#ifndef GISTPATH
#define GISTPATH "~/gist:~/Gist:/usr/local/lib/gist"
#endif
char *gp_default_path= GISTPATH;

/* ------------------------------------------------------------------------ */

#include <string.h>

struct GsysRead {
  char *legend;
  gp_box_t viewport;
  ga_tick_style_t ticks;
} modelSystem, tempSystem;

struct GlegRead {
  gp_real_t x, y, dx, dy;
  gp_text_attribs_t textStyle;
  int nchars, nlines, nwrap;
} modelLegends;

static char *FormGistPath(void);
static char *FormFullName(char *gistPath, const char *name);
static void FormatError(pl_file_t *fp, const char *name, const char *id);
static int SnarfColor(char *token);
static int SnarfRGB(char *token, pl_col_t *cell);
static int SnarfGray(pl_col_t *cell, int lookingFor4);
static char *WhiteSkip(char *input);
static char *DelimitRead(char *input, int *closed, int nlOK);
static char *ColRead(char *input, pl_col_t *dest);
static char *IntRead(char *input, int *dest);
static char *RealRead(char *input, gp_real_t *dest);
static char *StringRead(char *input, char **dest);
static char *MemberRead(char *input, char **member);
static char *ArrayRead(char *input, gp_real_t *dest, int narray);
static char *LineRead(char *input, gp_line_attribs_t *dest);
static char *TextRead(char *input, gp_text_attribs_t *dest);
static char *AxisRead(char *input, ga_axis_style_t *dest);
static char *TickRead(char *input, ga_tick_style_t *dest);
static char *SystemRead(char *input, struct GsysRead *dest);
static char *LegendsRead(char *input, struct GlegRead *dest);

/* ------------------------------------------------------------------------ */
/* A palette file (.gp) or style file (.gs) will be found if it:

   1. Is in the current working directory.
   2. Is the first file of its name encountered in any of the directories
      named in the GISTPATH environment variable.
   3. Ditto for the GISTPATH default string built in at compile time.
      Note that the environment variable is in addition to the compile
      time variable, not in place of it.

   The path name list should consist of directory names (with or without
   trailing '/'), separated by ':' with no intervening whitespace.  The
   symbol '~', if it is the first symbol of a directory name, will be
   expanded to the value of the HOME environment variable, but other
   environment variable expansions are not recognized.

   If the given filename begins with '/' the path search is
   not done.  '~' is NOT recognized in the given filename.
 */

extern char *g_argv0;
char *g_argv0 = 0;

static char *scratch = 0;
static char *gist_path = 0;

char *
gp_set_path(char *gpath)
{
  if (gpath) {
    char *p = gist_path;
    gist_path = pl_strcpy(gpath);
    if (p) pl_free(p);
  } else {
    FormGistPath();
  }
  return gist_path;
}

static char *FormGistPath(void)
{
  if (!gist_path) {
    char *gistPath = getenv("GISTPATH");
    int len = gistPath? strlen(gistPath) : 0;
    int len0 = g_argv0? strlen(g_argv0) : 0;
    int lend = gp_default_path? strlen(gp_default_path) : 0;
    char *place;

    /* Get enough scratch space to hold
       the concatenation of the GISTPATH environment variable and the
       GISTPATH compile-time option, and a fallback computed from argv[0] */
    gist_path = pl_malloc(len+len0+lend+4);
    if (!gist_path) return 0;

    place = gist_path;
    if (gistPath) {
      strcpy(place, gistPath);
      place += len;
      *(place++) = ':';
    }
    strcpy(place, gp_default_path);
    place += lend;
    /* back up to sibling of directory containing executable */
    for (len=len0-1 ; len>0 ; len--) if (g_argv0[len]=='/') break;
    for (len-- ; len>0 ; len--) if (g_argv0[len]=='/') break;
    if (len > 0) {
      /* tack /g/ sibling of executable directory onto path */
      *(place++) = ':';
      strncpy(place, g_argv0, ++len);
      place += len;
      strcpy(place, "g");
    }
  }

  scratch = pl_malloc(1028);
  if (!scratch) return 0;
  return gist_path;
}

static char *FormFullName(char *gistPath, const char *name)
{
  int nlen= strlen(name);
  int len, elen;
  char *now= scratch;

  for (;;) {
    /* Skip past any components of the GISTPATH which result in impossibly
       long path names */
    do len= strcspn(gistPath, ":"); while (!len);
    /* handle MS Windows drive letters */
    if (len==1 && gistPath[1]==':' &&
        ((gistPath[0]>='A' && gistPath[0]<='Z') ||
         (gistPath[0]>='a' && gistPath[0]<='z')))
      len = 2+strcspn(gistPath+2, ":");
    if (!len) break;
    elen= len;

    now= scratch;
    if (gistPath[0]=='~') {
      /* Get name of home directory from HOME environment variable */
      char *home= getenv("HOME");
      int hlen;
      if (home && (hlen= strlen(home))<1024) {
        strcpy(now, home);
        now+= hlen;
        gistPath++;
        len--;
        elen+= hlen-1;
      }
    }

    if (elen+nlen<1023) break;

    gistPath+= len+1;
  }

  if (len) {
    strncpy(now, gistPath, len);
    now+= len;
    if (now[-1]!='/') *now++= '/';
    strcpy(now, name);
  } else {
    scratch[0]= '\0';
  }

  return gistPath+len + strspn(gistPath+len, ":");
}

pl_file_t *gp_open(const char *name)
{
  pl_file_t* f;
  if (name == NULL) {
    return NULL;
  }
  f = pl_fopen(name, "r");
  if (f == NULL && name[0] != '/') {
    /* Try to find relative file names somewhere on GISTPATH or, failing
       that, in the default directory specified at compile time.  */
    char* gistPath= FormGistPath();
    if (gistPath != NULL) {
      do {
        gistPath = FormFullName(gistPath, name);
        f = pl_fopen(scratch, "r");
      } while (f == NULL && gistPath[0] != '\0');
      pl_free(scratch);
    }
  }
  if (f == NULL) {
    strcpy(gp_error, "unable to open file ");
    strncat(gp_error, name, 100);
  }
  return f;
}

static void FormatError(pl_file_t *fp, const char *name, const char *id)
{
  pl_fclose(fp);
  strcpy(gp_error, id);
  strcat(gp_error, " file format error in ");
  strncat(gp_error, name, 127-strlen(gp_error));
}

static char line[137];  /* longest allowed line is 136 characters */

/* ------------------------------------------------------------------------ */

static int SnarfColor(char *token)
     /* returns -1 if not unsigned char, -2 if missing */
{
  int color;
  char *suffix;

  if (!token) token= strtok(0, " \t\n");
  if (token) color= (int)strtol(token, &suffix, 0);
  else return -2;
  if (suffix==token || color<0 || color>255) return -1;
  else return color;
}

static int SnarfRGB(char *token, pl_col_t *cell)
{
  int red, blue, green;
  red= SnarfColor(token);
  if (red<0) return 1;
  green= SnarfColor(0);
  if (green<0) return 1;
  blue= SnarfColor(0);
  if (blue<0) return 1;
  cell[0] = PL_RGB(red, green, blue);
  return 0;
}

/* ARGSUSED */
static int SnarfGray(pl_col_t *cell, int lookingFor4)
{
  int gray= SnarfColor(0);
  if (gray==-2) return lookingFor4;
  else if (gray<0 || !lookingFor4) return 1;
  /* cell->gray= gray; */
  return 0;
}

int gp_read_palette(gp_engine_t *engine, const char *gpFile,
                  pl_col_t **palette, int maxColors)
{
  char *token, *suffix;
  pl_col_t *pal= 0;
  int iColor= -1,  nColors= 0,  ntsc= 0,  lookingFor4= 0;
  pl_file_t *gp= gp_open(gpFile);

  *palette= 0;
  if (!gp) return 0;

  for (;;) {  /* loop on lines in file */
    token= pl_fgets(gp, line, 137);
    if (!token) break;                      /* eof (or error) */

    token= strtok(token, " =\t\n");
    if (!token || token[0]=='#') continue;  /* blank or comment line */

    if (iColor<=0) {
      int *dest= 0;
      if (strcmp(token, "ncolors")==0) dest= &nColors;
      else if (strcmp(token, "ntsc")==0) dest= &ntsc;

      if (dest) {
        /* this is ncolors=... or ntsc=... line */
        token= strtok(0, " =\t\n");
        if (token) *dest= (int)strtol(token, &suffix, 0);
        else goto err;
        if (suffix==token || strtok(0, " \t\n")) goto err;

      } else {
        /* this must be the first rgb line */
        int gray;

        /* previous ncolors= is mandatory so palette can be allocated */
        if (nColors<=0) goto err;
        pal= pl_malloc(sizeof(pl_col_t)*nColors);
        if (!pal) goto merr;

        /* if first rgb line has 4 numbers, all must have 4, else 3 */
        if (SnarfRGB(token, pal)) goto err;
        gray= SnarfColor(0);
        if (gray==-1) goto err;
        if (gray>=0) {
          lookingFor4= 1;
          /* pal->gray= gray; */
          if (SnarfGray(pal, 0)) goto err;  /* just check for eol */
        } else {
          lookingFor4= 0;
          /* already got eol */
        }

        iColor= 1;
      }

    } else if (iColor<nColors) {
      /* read next rgb line */
      if (SnarfRGB(token, pal+iColor)) goto err;
      if (SnarfGray(pal+iColor, lookingFor4)) goto err;
      iColor++;

    } else {
      goto err;                  /* too many rgb for specified ncolors */
    }
  }
  if (iColor<nColors) goto err;  /* too few rgb for specified ncolors */

  pl_fclose(gp);

  if (nColors>maxColors && maxColors>1) {
    /* attempt to rescale the palette to maxColors */
    int oldCell, newCell, nextCell, r, g, b;
    double ratio= ((double)(nColors-1))/((double)(maxColors-1));
    double frac, frac1, old= 0.0;
    for (newCell=0 ; newCell<maxColors ; newCell++) {
      oldCell= (int)old;
      nextCell= oldCell+1;
      if (nextCell>=nColors) nextCell= oldCell;
      frac= old-(double)oldCell;
      frac1= 1.0-frac;
      r = (int)(frac1*PL_R(pal[oldCell]) + frac*PL_R(pal[nextCell]));
      g = (int)(frac1*PL_G(pal[oldCell]) + frac*PL_G(pal[nextCell]));
      b = (int)(frac1*PL_B(pal[oldCell]) + frac*PL_B(pal[nextCell]));
      pal[newCell] = PL_RGB(r, g, b);
      /*if (!lookingFor4)
        pal[newCell].gray= frac1*pal[oldCell].gray+frac*pal[nextCell].gray;*/
      old+= ratio;
    }
    nColors= maxColors;
  }

  if (!lookingFor4) {
    /* gray values were not explicitly specified */
    if (ntsc) gp_put_ntsc(nColors, pal);
    else gp_put_gray(nColors, pal);
  }

  *palette= pal;
  iColor= gp_set_palette(engine, pal, nColors);
  return iColor>nColors? nColors : iColor;

 err:
  FormatError(gp, gpFile, "palette");
  if (pal) pl_free(pal);
  return 0;

 merr:
  strcpy(gp_error, "memory manager failed to get space for palette");
  pl_fclose(gp);
  return 0;
}

/* ------------------------------------------------------------------------ */

#define OPEN_BRACE '{'
#define CLOSE_BRACE '}'

static pl_file_t *gs= 0;

static char *WhiteSkip(char *input)
{
  input+= strspn(input, " \t\n");

  while (!input[0] || input[0]=='#') { /* rest of line missing or comment */
    input= pl_fgets(gs, line, 137);
    if (input) input= line + strspn(line, " \t\n");
    else break;
  }

  return input;
}

static char *DelimitRead(char *input, int *closed, int nlOK)
{
  int nlFound= 0;

  if (nlOK) {
    input+= strspn(input, " \t");
    if (*input=='\n' || *input=='\0') nlFound= 1;
  }

  input= WhiteSkip(input);
  if (input) {
    if (*input == CLOSE_BRACE) {
      *closed= 1;
      input++;
    } else {
      *closed= 0;
      if (*input == ',') {
        input++;
      } else {
        if (!nlOK || !nlFound) input= 0;
      }
    }

  } else {
    /* distinguish end-of-file from comma not found */
    *closed= 1;
  }

  return input;
}

static char *
ColRead(char *input, pl_col_t *dest)
{
  long value;
  char *suffix;

  input = WhiteSkip(input);  /* may be on a new line */
  value = strtol(input, &suffix, 0);
  if (suffix==input) return 0;

  if (value<0) value += 256;
  *dest = value;
  return suffix;
}

static char *IntRead(char *input, int *dest)
{
  int value;
  char *suffix;

  input= WhiteSkip(input);  /* may be on a new line */
  value= (int)strtol(input, &suffix, 0);
  if (suffix==input) return 0;

  *dest= value;
  return suffix;
}

static char *RealRead(char *input, gp_real_t *dest)
{
  gp_real_t value;
  char *suffix;

  input= WhiteSkip(input);  /* may be on a new line */
  errno = 0;
  value= (gp_real_t)strtod(input, &suffix);
  if (errno || suffix==input)
    return 0;

  *dest= value;
  return suffix;
}

char legendString[41];

static char *StringRead(char *input, char **dest)
{
  input= WhiteSkip(input);
  if (input) {
    if (*input=='0') {
      *dest= 0;
      input++;
    } else if (*input=='\"') {
      long len= strcspn(++input, "\"");
      int nc= len>40? 40 : len;
      strncpy(legendString, input, nc);
      input+= len;
      if (*input=='\"') { *dest= legendString;  input++; }
      else input= 0;
    } else {
      input= 0;
    }
  }
  return input;
}

static char *MemberRead(char *input, char **member)
{
  input= WhiteSkip(input);
  *member= input;
  if (input) {
    int gotEqual= 0;
    input+= strcspn(input, "= \t\n");
    if (*input == '=') gotEqual= 1;
    if (*input) *input++= '\0';
    if (!gotEqual) {
      input= WhiteSkip(input);
      if (input && *input++!='=') input= 0;
    }
  }
  return input;
}

static char *ArrayRead(char *input, gp_real_t *dest, int narray)
{
  int foundClose;

  input= WhiteSkip(input);
  if (!input) return 0;

  if (*input++ != OPEN_BRACE) return 0;  /* no open brace */
  input= WhiteSkip(input);
  if (!input) return 0;                  /* eof after open brace */

  for (narray-- ; ; narray--) {
    if (narray<0) return 0;           /* too many numbers in aggregate */

    input= RealRead(input, dest++);
    if (!input) return 0;             /* token was not a number */

    input= DelimitRead(input, &foundClose, 0);
    if (!input) return 0;             /* neither comma nor close brace */
    if (foundClose) break;
  }

  return input;
}

static char *LineRead(char *input, gp_line_attribs_t *dest)
{
  int foundClose;
  char *member;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "color")==0) {
      input= ColRead(input, &dest->color);
    } else if (strcmp(member, "type")==0) {
      input= IntRead(input, &dest->type);
    } else if (strcmp(member, "width")==0) {
      input= RealRead(input, &dest->width);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

static char *TextRead(char *input, gp_text_attribs_t *dest)
{
  int foundClose;
  char *member;
  int ijunk;
  gp_real_t rjunk;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "color")==0) {
      input= ColRead(input, &dest->color);
    } else if (strcmp(member, "font")==0) {
      input= IntRead(input, &dest->font);
    } else if (strcmp(member, "prec")==0) {
      input= IntRead(input, &ijunk);
    } else if (strcmp(member, "height")==0) {
      input= RealRead(input, &dest->height);
    } else if (strcmp(member, "expand")==0) {
      input= RealRead(input, &rjunk);
    } else if (strcmp(member, "spacing")==0) {
      input= RealRead(input, &rjunk);
    } else if (strcmp(member, "upX")==0) {
      input= RealRead(input, &rjunk);
    } else if (strcmp(member, "upY")==0) {
      input= RealRead(input, &rjunk);
    } else if (strcmp(member, "path")==0 || strcmp(member, "orient")==0) {
      input= IntRead(input, &dest->orient);
    } else if (strcmp(member, "alignH")==0) {
      input= IntRead(input, &dest->alignH);
    } else if (strcmp(member, "alignV")==0) {
      input= IntRead(input, &dest->alignV);
    } else if (strcmp(member, "opaque")==0) {
      input= IntRead(input, &dest->opaque);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

static char *AxisRead(char *input, ga_axis_style_t *dest)
{
  int foundClose;
  char *member;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "nMajor")==0) {
      input= RealRead(input, &dest->nMajor);
    } else if (strcmp(member, "nMinor")==0) {
      input= RealRead(input, &dest->nMinor);
    } else if (strcmp(member, "logAdjMajor")==0) {
      input= RealRead(input, &dest->logAdjMajor);
    } else if (strcmp(member, "logAdjMinor")==0) {
      input= RealRead(input, &dest->logAdjMinor);
    } else if (strcmp(member, "nDigits")==0) {
      input= IntRead(input, &dest->nDigits);
    } else if (strcmp(member, "gridLevel")==0) {
      input= IntRead(input, &dest->gridLevel);
    } else if (strcmp(member, "flags")==0) {
      input= IntRead(input, &dest->flags);
    } else if (strcmp(member, "tickOff")==0) {
      input= RealRead(input, &dest->tickOff);
    } else if (strcmp(member, "labelOff")==0) {
      input= RealRead(input, &dest->labelOff);
    } else if (strcmp(member, "tickLen")==0) {
      input= ArrayRead(input, dest->tickLen, 5);
    } else if (strcmp(member, "tickStyle")==0) {
      input= LineRead(input, &dest->tickStyle);
    } else if (strcmp(member, "gridStyle")==0) {
      input= LineRead(input, &dest->gridStyle);
    } else if (strcmp(member, "textStyle")==0) {
      input= TextRead(input, &dest->textStyle);
    } else if (strcmp(member, "xOver")==0) {
      input= RealRead(input, &dest->xOver);
    } else if (strcmp(member, "yOver")==0) {
      input= RealRead(input, &dest->yOver);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

static char *TickRead(char *input, ga_tick_style_t *dest)
{
  int foundClose;
  char *member;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "horiz")==0) {
      input= AxisRead(input, &dest->horiz);
    } else if (strcmp(member, "vert")==0) {
      input= AxisRead(input, &dest->vert);
    } else if (strcmp(member, "frame")==0) {
      input= IntRead(input, &dest->frame);
    } else if (strcmp(member, "frameStyle")==0) {
      input= LineRead(input, &dest->frameStyle);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

/* defaultSystem is initialized to reasonable value for portrait mode */
#define DEF_XMIN 0.25
#define DEF_XMAX 0.60
#define DEF_YMIN 0.50
#define DEF_YMAX 0.85
struct GsysRead defaultSystem= {
  0, { DEF_XMIN, DEF_XMAX, DEF_YMIN, DEF_YMAX },
  {

    {7.5, 50., 1.2, 1.2, 3, 1, GA_TICK_L|GA_TICK_U|GA_TICK_OUT|GA_LABEL_L,
     0.0, 14.0*GP_ONE_POINT,
     {12.*GP_ONE_POINT, 8.*GP_ONE_POINT, 5.*GP_ONE_POINT, 3.*GP_ONE_POINT, 2.*GP_ONE_POINT},
     {PL_FG, GP_LINE_SOLID, GP_DEFAULT_LINE_WIDTH},
     {PL_FG, GP_LINE_DOT, GP_DEFAULT_LINE_WIDTH},
     {PL_FG, GP_FONT_HELVETICA, 14.*GP_ONE_POINT, GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_NORMAL, GP_VERT_ALIGN_NORMAL, 1},
     0.5*(DEF_XMIN+DEF_XMAX), DEF_YMIN-52.*GP_ONE_POINT},

    {7.5, 50., 1.2, 1.2, 4, 1, GA_TICK_L|GA_TICK_U|GA_TICK_OUT|GA_LABEL_L,
     0.0, 14.0*GP_ONE_POINT,
     {12.*GP_ONE_POINT, 8.*GP_ONE_POINT, 5.*GP_ONE_POINT, 3.*GP_ONE_POINT, 2.*GP_ONE_POINT},
     {PL_FG, GP_LINE_SOLID, GP_DEFAULT_LINE_WIDTH},
     {PL_FG, GP_LINE_DOT, GP_DEFAULT_LINE_WIDTH},
     {PL_FG, GP_FONT_HELVETICA, 14.*GP_ONE_POINT, GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_NORMAL, GP_VERT_ALIGN_NORMAL, 1},
     DEF_XMIN, DEF_YMIN-52.*GP_ONE_POINT},

  0, {PL_FG, GP_LINE_SOLID, GP_DEFAULT_LINE_WIDTH}
  }
};

struct GlegRead defaultLegends[2]= {
  /* Ordinary legends form two 36x22 character columns below viewport */
  { 0.5*GP_ONE_INCH, DEF_YMIN-64.*GP_ONE_POINT, 3.875*GP_ONE_INCH, 0.0,
    {PL_FG, GP_FONT_COURIER, 12.*GP_ONE_POINT, GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_LEFT, GP_VERT_ALIGN_TOP, 1},
    36, 22, 2 },
  /* Contour legends get a single 14x28 column to left of viewport */
  { DEF_XMAX+14.*GP_ONE_POINT, DEF_YMAX+12.*GP_ONE_POINT, 0.0, 0.0,
    {PL_FG, GP_FONT_COURIER, 12.*GP_ONE_POINT, GP_ORIENT_RIGHT, GP_HORIZ_ALIGN_LEFT, GP_VERT_ALIGN_TOP, 1},
    14, 28, 1 },
};

static char *SystemRead(char *input, struct GsysRead *dest)
{
  int foundClose;
  char *member;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "viewport")==0) {
      gp_real_t box[4];
      box[0]= box[1]= box[2]= box[3]= -1.0;
      input= ArrayRead(input, box, 4);
      if (box[3]<0.0) input= 0;       /* all four required */
      else {
        dest->viewport.xmin= box[0];
        dest->viewport.xmax= box[1];
        dest->viewport.ymin= box[2];
        dest->viewport.ymax= box[3];
      }
    } else if (strcmp(member, "ticks")==0) {
      input= TickRead(input, &dest->ticks);
    } else if (strcmp(member, "legend")==0) {
      input= StringRead(input, &dest->legend);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

static char *LegendsRead(char *input, struct GlegRead *dest)
{
  int foundClose;
  char *member;

  input= WhiteSkip(input);
  if (!input || *input++!=OPEN_BRACE) return 0;

  for (;;) {
    input= MemberRead(input, &member);
    if (!input) return 0;             /* couldn't find member = */

    if (strcmp(member, "x")==0) {
      input= RealRead(input, &dest->x);
    } else if (strcmp(member, "y")==0) {
      input= RealRead(input, &dest->y);
    } else if (strcmp(member, "dx")==0) {
      input= RealRead(input, &dest->dx);
    } else if (strcmp(member, "dy")==0) {
      input= RealRead(input, &dest->dy);
    } else if (strcmp(member, "textStyle")==0) {
      input= TextRead(input, &dest->textStyle);
    } else if (strcmp(member, "nchars")==0) {
      input= IntRead(input, &dest->nchars);
    } else if (strcmp(member, "nlines")==0) {
      input= IntRead(input, &dest->nlines);
    } else if (strcmp(member, "nwrap")==0) {
      input= IntRead(input, &dest->nwrap);
    } else {
      return 0;                       /* unknown member */
    }
    if (!input) return 0;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) return 0;             /* not comma, nl, or close brace */
    if (foundClose) break;
  }

  return input;
}

int gd_read_style(gd_drawing_t *drawing, const char *gsFile)
{
  int foundClose, sysIndex, landscape;
  char *input, *keyword;

  if (!gsFile) return 0;

  gs= gp_open(gsFile);
  if (!gs) return 1;

  tempSystem= defaultSystem;
  landscape= 0;

  input= pl_fgets(gs, line, 137);
  if (!input) goto err;                      /* eof (or error) */

  _gd_kill_systems();

  for (;;) {
    input= WhiteSkip(input);
    if (!input) break;

    input= MemberRead(input, &keyword);
    if (!input) goto err;             /* couldn't find keyword = */

    if (strcmp(keyword, "default")==0) {
      input= SystemRead(input, &tempSystem);
    } else if (strcmp(keyword, "system")==0) {
      modelSystem= tempSystem;
      input= SystemRead(input, &modelSystem);
      gd_properties.hidden= 0;
      gd_properties.legend= modelSystem.legend;
      sysIndex= gd_new_system(&modelSystem.viewport, &modelSystem.ticks);
      if (sysIndex<0) return 1;
    } else if (strcmp(keyword, "landscape")==0) {
      input= IntRead(input, &landscape);
    } else if (strcmp(keyword, "legends")==0) {
      modelLegends= defaultLegends[0];
      input= LegendsRead(input, &modelLegends);
      if (input) gd_set_legend_box(0, modelLegends.x, modelLegends.y,
                             modelLegends.dx, modelLegends.dy,
                             &modelLegends.textStyle, modelLegends.nchars,
                             modelLegends.nlines, modelLegends.nwrap);
    } else if (strcmp(keyword, "clegends")==0) {
      modelLegends= defaultLegends[1];
      input= LegendsRead(input, &modelLegends);
      if (input) gd_set_legend_box(1, modelLegends.x, modelLegends.y,
                             modelLegends.dx, modelLegends.dy,
                             &modelLegends.textStyle, modelLegends.nchars,
                             modelLegends.nlines, modelLegends.nwrap);
    } else {
      goto err;                       /* unknown keyword */
    }
    if (!input) goto err;             /* illegal format */

    input= DelimitRead(input, &foundClose, 1);
    if (!input) {
      if (foundClose) break;
      goto err;                       /* not comma, nl, or eof */
    }
    if (foundClose) goto err;         /* close brace not legal here */
  }

  if (landscape) gd_set_landscape(1);
  pl_fclose(gs);
  return 0;

 err:
  FormatError(gs, gsFile, "drawing style");
  return 1;
}

/* ------------------------------------------------------------------------ */
