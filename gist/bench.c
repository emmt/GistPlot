/*
 * $Id: bench.c,v 1.1 2005-09-18 22:04:22 dhmunro Exp $
 * Exercise GIST graphics
 */
/* Copyright (c) 2005, The Regents of the University of California.
 * All rights reserved.
 * This file is part of yorick (http://yorick.sourceforge.net).
 * Read the accompanying LICENSE file for details.
 */

#include "play/std.h"
#include "play/io.h"
#include "gist/hlevel.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#ifndef NO_XLIB
#include "gist/xfancy.h"
#else
#include "play2.h"
/* Animated versions of low level tests use these directly--
   ordinarily they are used indirectly via gist/hlevel.h, as in the animate
   command in on_stdin.  */
static int gx_animate(gp_engine_t *engine, gp_box_t *viewport);
static int gx_strobe(gp_engine_t *engine, int clear);
static int gx_direct(gp_engine_t *engine);
static int gx_animate(gp_engine_t *engine, gp_box_t *viewport)
{
  return 0;
}
static int gx_strobe(gp_engine_t *engine, int clear)
{
  return 0;
}
static int gx_direct(gp_engine_t *engine)
{
  return 0;
}
#endif

#define PRINTF pl_stdout
#define PRINTF1(f,x) sprintf(pl_wkspc.c,f,x);pl_stdout(pl_wkspc.c)
#define PRINTF2(f,x,y) sprintf(pl_wkspc.c,f,x,y);pl_stdout(pl_wkspc.c)
#define PRINTF3(f,x,y,z) sprintf(pl_wkspc.c,f,x,y,z);pl_stdout(pl_wkspc.c)

#define DPI 75

#define PI 3.14159265359

/* Mesh for benchmarks 0-4 */
#define NCOLORS 240
#define NFRAMES 50
#define AMPLITUDE 0.5
#define NMID 25
#define N (2*NMID+1)
#define DPHI (2.0*PI/8.3)
gp_real_t cosine[N], amplitude[NFRAMES];
ga_mesh_t mesh;
gp_real_t *xmesh=0, *ymesh=0;
int *ireg=0;
gp_color_t *cmesh=0;
int meshReg= 0;
int meshBnd= 0;

#define PALETTE_FILE "earth.gp"
int actualColors;
pl_col_t *palette;

/* Level function and vector function */
gp_real_t *z=0, *u=0, *v=0;
gp_real_t zLevels[10]= { -.9, -.7, -.5, -.3, -.1, .1, .3, .5, .7, .9 };

/* Lissajous test */
#define LISSPTS 400
gp_real_t xliss[LISSPTS], yliss[LISSPTS];
int animate= 0, markers= 0, markType= GP_MARKER_POINT;
#define LISSFRAMES 50
#define LISSSIZE 40.0
#define NA1 1
#define NB1 5
#define NA2 2
#define NB2 7
#define RC1 40.0
#define RC2 160.0
#define DPHASE (2*PI/(LISSFRAMES-1))
#define DTHETA (PI/(LISSFRAMES-1))
#define DELPHI (2*PI/(LISSPTS-1))

gp_real_t xSmall[7]= { 1.1, 1.2, 9.1, 9.5, 11.5, 12.5, 13.6 };
gp_real_t ySmall[7]= { 1.0, 10., 5.0, 9.0,  8.0,  5.0,  4.0 };
gp_real_t xqSmall[7];
gp_real_t yqSmall[7];

static void GenerateLiss(int i, gp_real_t rc, gp_real_t size, int na, int nb);

static void GenerateLiss(int i, gp_real_t rc, gp_real_t size, int na, int nb)
{
  double theta= i*DTHETA;
  double phase= i*DPHASE;
  double xoff= rc*cos(theta),  yoff= rc*sin(theta);
  int j;
  for (j=0 ; j<LISSPTS ; j++) {
    xliss[j]= xoff+size*cos(na*DELPHI*j);
    yliss[j]= yoff+size*sin(nb*DELPHI*j+phase);
  }
}

static void GenerateMesh(int firstPass, gp_real_t amplFrame);

static void GenerateMesh(int firstPass, gp_real_t amplFrame)
{
  int i, j, k;
  gp_real_t xsplit;

  if (firstPass) {
    for (i=k=0 ; i<N ; i++, k=0)
      for (j=0 ; j<N*N ; j+=N) ymesh[j+i]= k++/(N-1.0);
  }

  k= 0;
  for (j=0 ; j<N*N ; j+=N) {
    xsplit= 0.5*(1.0+amplFrame*cosine[k++]);
    for (i=0 ; i<NMID ; i++) xmesh[j+i]= i*xsplit/NMID;
    for (i=NMID ; i<N ; i++) xmesh[j+i]= xsplit+(i-NMID)*(1.0-xsplit)/NMID;
  }
}

static void GenerateColors(void);

static void GenerateColors(void)
{
  gp_real_t dxmin= (1.0-AMPLITUDE)/(2.0*NMID);
  gp_real_t dxmax= (1.0+AMPLITUDE)/(2.0*NMID);
  gp_real_t range= dxmax-dxmin;
  int i, j;

  for (j=N ; j<N*N ; j+=N) {
    cmesh[j]= 0;
    for (i=1 ; i<N ; i++) cmesh[j+i]=
      (gp_color_t)((actualColors-1)*(xmesh[j+i]-xmesh[j+i-1]-dxmin)/range);
  }
}

static void SetOddReg(int r);

static void SetOddReg(int r)
{
  int i, j;
  for (j=N ; j<N*N ; j+=N) {
    ireg[j]= 0;
    for (i=1 ; i<N ; i++) ireg[j+i]= 1;
    if (j<N*N/4)
      for (i=3*N/8 ; i<5*N/8 ; i++) ireg[j+i]= r;
    else if (j<N*N/2)
      for (i=N/4 ; i<3*N/4 ; i++) ireg[j+i]= r;
    else if (j<5*N*N/8)
      for (i=1 ; i<N/4 ; i++) ireg[j+i]= r;
    else if (j<7*N*N/8)
      for (i=N/2 ; i<N ; i++) ireg[j+i]= r;
    else
      for (i=3*N/8 ; i<5*N/8 ; i++) ireg[j+i]= r;
  }
}

static void TenseZ(void);

static void TenseZ(void)
{
  /* Set diagonal from (10,40) to (20,30) to -1, and diagonal from
     (25,15) to (35,25) to +1.  */
  int i;
  for (i=0 ; i<11 ; i++) {
    z[10+40*N - (N-1)*i]= -1.0;  /* 10+i+N*(40-i) */
    z[25+15*N + (N+1)*i]= +1.0;  /* 25+i+N*(15+i) */
  }
}

static gp_real_t zs[N];
#define WGT_C 0.50
#define WGT_E (1./3.)
#define WGT_I 0.25

static void RelaxZ(void);

static void RelaxZ(void)
{
  /* Average each point with its 4 nearest neighbors.  */
  int i, j;
  register gp_real_t zss;

  /* Do 1st row */
  zss= z[0];
  z[0]= (1.-2.*WGT_C)*zss + WGT_C*(z[1]+z[N]);
  zs[0]= zss;
  for (i=1 ; i<N-1 ; i++) {
    zss= z[i];
    z[i]= (1.-3.*WGT_E)*zss + WGT_E*(zs[i-1]+z[i+1]+z[i+N]);
    zs[i]= zss;
  }
  zss= z[i];
  z[i]= 0.5*zss + 0.25*(zs[i-1]+z[i+N]);
  zs[i]= zss;

  /* Do interior rows */
  for (j=N ; j<N*(N-1) ; j+=N) {
    zss= z[j];
    z[j]= (1.-3.*WGT_E)*zss + WGT_E*(zs[0]+z[j+1]+z[j+N]);
    zs[0]= zss;
    for (i=1 ; i<N-1 ; i++) {
      zss= z[j+i];
      z[j+i]= (1.-4.*WGT_I)*zss + WGT_I*(zs[i-1]+zs[i]+z[j+i+1]+z[j+i+N]);
      zs[i]= zss;
    }
    zss= z[j+i];
    z[j+i]= (1.-3.*WGT_E)*zss + WGT_E*(zs[i-1]+zs[i]+z[j+i+N]);
    zs[i]= zss;
  }

  /* Do last row */
  zss= z[j];
  z[j]= (1.-2.*WGT_C)*zss + WGT_C*(zs[0]+z[j+1]);
  zs[0]= zss;
  for (i=1 ; i<N-1 ; i++) {
    zss= z[j+i];
    z[j+i]= (1.-3.*WGT_E)*zss + WGT_E*(zs[i-1]+zs[i]+z[j+i+1]);
    zs[i]= zss;
  }
  zss= z[j+i];
  z[j+i]= (1.-2.*WGT_C)*zss + WGT_C*(zs[i-1]+zs[i]);
  zs[i]= zss;
}

gp_real_t onePixel= 1.00001/512.5;

extern void GenerateImage(int now, gp_real_t cx, gp_real_t cy,
                          gp_real_t *px, gp_real_t *py, gp_real_t *qx, gp_real_t *qy);

void GenerateImage(int now, gp_real_t cx, gp_real_t cy,
                   gp_real_t *px, gp_real_t *py, gp_real_t *qx, gp_real_t *qy)
{
  gp_real_t halfwid= 0.5*onePixel*(now+1)*N;
  if (now==0) {
    int i, j;
    for (j=N ; j<N*N ; j+=N)
      for (i=1 ; i<N ; i++)
        cmesh[j+i]= (gp_color_t)((actualColors-1)*(z[j+i]+1.0)*0.5);
  }
  *px= cx - halfwid;
  *py= cy - halfwid;
  *qx= cx + halfwid;
  *qy= cy + halfwid;
}

static void get_time(double *cpu, double *sys, double *wall);
static double initWall= -1.0e31;

static void
get_time(double *cpu, double *sys, double *wall)
{
  *cpu = pl_cpu_secs(sys);
  *wall = pl_wall_secs();
  if (initWall<-1.0e30) initWall = *wall;
  *wall -= initWall;
}

static gp_box_t unitBox= {0.0, 1.0, 0.0, 1.0};
static gp_box_t zoomBox= {0.25, 0.75, 0.25, 0.75};
static gp_transform_t meshTrans;

static int calibFont= GP_FONT_TIMES;

gp_engine_t *engine= 0;

int clear= 1;        /* interframe clear flag */
int colorMode= 0;    /* colorMode for hcp device */
int noCopy= 0;       /* noCopy mode flags */

int noKeyboard= 0;
char *noKeyTest[]= { "plg1", "fma", "plm1", "fma", "plf1", "fma",
                     "plc1", "fma", "pli", "fma", "plt", "fma", "pls", "fma",
                     "pldj", "fma", "txin", "txout", "fma", "calib", "fma",
                     "cfont", "calib", "fma", "quit", (char *)0 };

extern int on_idle(void);
extern int on_quit(void);
extern void on_stdin(char *line);
extern void on_exception(int signal, char *errmsg);

static int prompt_issued = 0;

void
on_stdin(char *line)
{
  double user, system, wall, user0, system0, wall0;
  int i;
  char *token;
  prompt_issued = 0;

  gd_revert_limits(1);  /* undo mouse zooms, if any */

  token= strtok(line, ", \t\n");
  if (!token) return;
  if (strcmp(token, "q")==0 || strcmp(token, "quit")==0) {
    pl_quit();
    return;
  }

/* ------- A and P level tests -------- */

  if (strcmp(token, "0")==0) {
    PRINTF("   Begin benchmark 0 -- ga_draw_mesh (direct).\n");
    get_time(&user0, &system0, &wall0);
    ga_attributes.l.color= PL_FG;
    gp_set_transform(&meshTrans);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      if (clear) gp_clear(0, GP_ALWAYS);
      ga_draw_mesh(&mesh, meshReg, meshBnd, 0);
      gp_flush(engine);
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "1")==0) {
    PRINTF("   Begin benchmark 1 -- ga_draw_mesh (animated).\n");
    get_time(&user0, &system0, &wall0);
    ga_attributes.l.color= PL_FG;
    gp_set_transform(&meshTrans);
    gx_animate(engine, &meshTrans.viewport);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      ga_draw_mesh(&mesh, meshReg, meshBnd, 0);
      gx_strobe(engine, 1);
      gp_flush(engine);
    }
    gx_direct(engine);  /* turn off animation mode */
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "2")==0) {
    PRINTF("   Begin benchmark 2 -- ga_fill_mesh (direct).\n");
    get_time(&user0, &system0, &wall0);
    gp_set_transform(&meshTrans);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      GenerateColors();
      if (clear) gp_clear(0, GP_ALWAYS);
      ga_fill_mesh(&mesh, meshReg, cmesh+N+1, N);
      gp_flush(engine);
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "3")==0) {
    PRINTF("   Begin benchmark 3 -- ga_fill_mesh (animated).\n");
    get_time(&user0, &system0, &wall0);
    gp_set_transform(&meshTrans);
    gx_animate(engine, &meshTrans.viewport);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      GenerateColors();
      ga_fill_mesh(&mesh, meshReg, cmesh+N+1, N);
      gx_strobe(engine, 1);
      gp_flush(engine);
    }
    gx_direct(engine);  /* turn off animation mode */
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "im")==0) {
    gp_real_t cx= 0.5*(meshTrans.window.xmin+meshTrans.window.xmax);
    gp_real_t cy= 0.5*(meshTrans.window.ymin+meshTrans.window.ymax);
    gp_real_t px, qx, py, qy;
    PRINTF("   Begin image benchmark.\n");
    get_time(&user0, &system0, &wall0);
    gp_set_transform(&meshTrans);
    gp_clear(0, GP_ALWAYS);
    for (i=0 ; i<10 ; i++) {
      GenerateImage(i, cx, cy, &px, &py, &qx, &qy);
      gp_draw_cells(px, py, qx, qy, N-1, N-1, N, cmesh+N+1);
      gp_flush(engine);
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", 10./(wall-wall0));

  } else if (strcmp(token, "clr")==0) {
    if (clear) {
      clear= 0;
      PRINTF("   Toggle interframe clear (now off)\n");
    } else {
      clear= 1;
      PRINTF("   Toggle interframe clear (now on)\n");
    }

  } else if (strcmp(token, "clip")==0) {
    if (gp_clipping) {
      gp_clipping= 0;
      PRINTF("   Toggle floating point clip (now off)\n");
    } else {
      gp_clipping= 1;
      PRINTF("   Toggle floating point clip (now on)\n");
    }

  } else if (strcmp(token, "zoom")==0) {
    if (meshTrans.window.xmax<1.0) {
      meshTrans.window= unitBox;
      PRINTF("   Toggle zoom (now off)\n");
    } else {
      meshTrans.window= zoomBox;
      PRINTF("   Toggle zoom (now on)\n");
    }

  } else if (strcmp(token, "c")==0) {
    PRINTF("   Clear\n");
    gp_clear(0, GP_CONDITIONALLY);

/* ------- attribute and property toggles -------- */

  } else if (strcmp(token, "bnd")==0) {
    if (meshBnd) {
      meshBnd= 0;
      PRINTF("   Toggle mesh boundary (now off)\n");
    } else {
      meshBnd= 1;
      PRINTF("   Toggle mesh boundary (now on)\n");
    }

  } else if (strcmp(token, "odd0")==0) {
    PRINTF("   Set mesh odd region to 0\n");
    SetOddReg(0);
    if (meshReg==2) meshReg= 0;

  } else if (strcmp(token, "odd2")==0) {
    PRINTF("   Set mesh odd region to 2\n");
    SetOddReg(2);

  } else if (strcmp(token, "reg0")==0) {
    PRINTF("   Set mesh plot to region 0\n");
    meshReg= 0;

  } else if (strcmp(token, "reg1")==0) {
    PRINTF("   Set mesh plot to region 1\n");
    meshReg= 1;

  } else if (strcmp(token, "reg2")==0) {
    PRINTF("   Set mesh plot to region 2\n");
    meshReg= 2;

  } else if (strcmp(token, "cmode")==0) {
    if (colorMode) {
      colorMode= 0;
      PRINTF("   Toggle hcp color mode (now no dump)\n");
    } else {
      colorMode= 1;
      PRINTF("   Toggle hcp color mode (now dump)\n");
    }
    gp_dump_colors(gh_hcp_default, colorMode);
    gp_dump_colors(engine, colorMode);

  } else if (strcmp(token, "nocopy")==0) {
    if (noCopy) {
      noCopy= 0;
      PRINTF("   Toggle noCopy mode (now copies mesh)\n");
    } else {
      noCopy= GD_NOCOPY_MESH|GD_NOCOPY_COLORS|GD_NOCOPY_UV|GD_NOCOPY_Z;
      PRINTF("   Toggle noCopy mode (now no mesh copies)\n");
    }

  } else if (strcmp(token, "earth")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "earth.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "stern")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "stern.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "rainbow")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "rainbow.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "heat")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "heat.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "gray")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "gray.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "yarg")==0) {
    PRINTF("OK\n");
    if (palette) pl_free(palette);
    actualColors= gp_read_palette(engine, "yarg.gp", &palette, 240);
    gp_set_palette(gh_hcp_default, palette, actualColors);

  } else if (strcmp(token, "work")==0) {
    PRINTF("OK\n");
    gd_read_style(gh_devices[0].drawing, "work.gs");

  } else if (strcmp(token, "boxed")==0) {
    PRINTF("OK\n");
    gd_read_style(gh_devices[0].drawing, "boxed.gs");

  } else if (strcmp(token, "axes")==0) {
    PRINTF("OK\n");
    gd_read_style(gh_devices[0].drawing, "axes.gs");

  } else if (strcmp(token, "wide")==0) {
    gh_get_lines();
    if (ga_attributes.l.width>1.0) {
      ga_attributes.l.width= 1.0;
      PRINTF("   Toggle wide lines (now off)\n");
    } else {
      ga_attributes.l.width= 8.0;
      PRINTF("   Toggle wide lines (now on)\n");
    }
    gh_set_lines();

  } else if (strcmp(token, "linetype")==0) {
    gh_get_lines();
    ga_attributes.l.type++;
    if (ga_attributes.l.type>GP_LINE_DASHDOTDOT) ga_attributes.l.type= GP_LINE_SOLID;
    if (ga_attributes.l.type==GP_LINE_SOLID)
      PRINTF("   Toggle line type (now solid)\n");
    else if (ga_attributes.l.type==GP_LINE_DASH)
      PRINTF("   Toggle line type (now dashed)\n");
    else if (ga_attributes.l.type==GP_LINE_DOT)
      PRINTF("   Toggle line type (now dotted)\n");
    else if (ga_attributes.l.type==GP_LINE_DASHDOT)
      PRINTF("   Toggle line type (now dash-dot)\n");
    else if (ga_attributes.l.type==GP_LINE_DASHDOTDOT)
      PRINTF("   Toggle line type (now dash-dot-dot)\n");
    else
      PRINTF("   Toggle line type (now bad type?)\n");
    gh_set_lines();

  } else if (strcmp(token, "closed")==0) {
    gh_get_lines();
    if (ga_attributes.dl.closed) {
      ga_attributes.dl.closed= 0;
      PRINTF("   Toggle closed lines (now off)\n");
    } else {
      ga_attributes.dl.closed= 1;
      PRINTF("   Toggle closed lines (now on)\n");
    }
    gh_set_lines();

  } else if (strcmp(token, "smooth")==0) {
    gh_get_lines();
    if (ga_attributes.dl.smooth==4) {
      ga_attributes.dl.smooth= 0;
      PRINTF("   Toggle smooth lines (now off)\n");
    } else if (ga_attributes.dl.smooth==1) {
      ga_attributes.dl.smooth= 4;
      PRINTF("   Toggle smooth lines (now max)\n");
    } else {
      ga_attributes.dl.smooth= 1;
      PRINTF("   Toggle smooth lines (now min)\n");
    }
    gh_set_lines();

  } else if (strcmp(token, "rays")==0) {
    gh_get_lines();
    if (ga_attributes.dl.rays) {
      ga_attributes.dl.rays= 0;
      PRINTF("   Toggle rays (now off)\n");
    } else {
      ga_attributes.dl.rays= 1;
      PRINTF("   Toggle rays (now on)\n");
    }
    gh_set_lines();

  } else if (strcmp(token, "edges")==0) {
    gh_get_fill();
    if (ga_attributes.e.type!=GP_LINE_NONE) {
      ga_attributes.e.type= GP_LINE_NONE;
      PRINTF("   Toggle edges on plf (now off)\n");
    } else {
      ga_attributes.e.type= GP_LINE_SOLID;
      PRINTF("   Toggle edges on plf (now on)\n");
    }
    gh_set_fill();

  } else if (strcmp(token, "limits")==0) {
    char *suffix;
    gp_real_t limits[4], value;
    int i= 0;
    if (gd_get_limits()) {
      PRINTF("****** No drawing yet\n");
      return;
    }
    PRINTF("   Setting limits\n");
    limits[0]= gd_properties.limits.xmin;
    limits[1]= gd_properties.limits.xmax;
    limits[2]= gd_properties.limits.ymin;
    limits[3]= gd_properties.limits.ymax;
    while ((token= strtok(0, "=, \t\n"))) {
      if (strcmp(token, "nice")==0) {
        token= strtok(0, "= \t\n");
        if (!token) break;
        if (strcmp(token, "0")==0) gd_properties.flags&= ~GD_LIM_NICE;
        else gd_properties.flags|= GD_LIM_NICE;
      } else if (strcmp(token, "square")==0) {
        token= strtok(0, "= \t\n");
        if (!token) break;
        if (strcmp(token, "0")==0) gd_properties.flags&= ~GD_LIM_SQUARE;
        else gd_properties.flags|= GD_LIM_SQUARE;
      } else if (strcmp(token, "restrict")==0) {
        token= strtok(0, "= \t\n");
        if (!token) break;
        if (strcmp(token, "0")==0) gd_properties.flags&= ~GD_LIM_RESTRICT;
        else gd_properties.flags|= GD_LIM_RESTRICT;
      } else if (i<4) {
        if (strcmp(token, "e")==0) {
          gd_properties.flags|= (GD_LIM_XMIN<<i);
        } else {
          value= strtod(token, &suffix);
          if (suffix!=token) limits[i]= value;
          else break;
          gd_properties.flags&= ~(GD_LIM_XMIN<<i);
        }
        i++;
      }
    }
    gd_properties.limits.xmin= limits[0];
    gd_properties.limits.xmax= limits[1];
    gd_properties.limits.ymin= limits[2];
    gd_properties.limits.ymax= limits[3];
    gd_set_limits();
    gh_before_wait();

  } else if (strcmp(token, "logxy")==0) {
    if (gd_get_limits()) {
      PRINTF("****** No drawing yet\n");
      return;
    }
    PRINTF("   Setting logxy\n");
    token= strtok(0, ", \t\n");
    if (token) {
      if (strcmp(token, "0")==0) gd_properties.flags&= ~GD_LIM_LOGX;
      else gd_properties.flags|= GD_LIM_LOGX;
      token= strtok(0, ", \t\n");
      if (token) {
        if (strcmp(token, "0")==0) gd_properties.flags&= ~GD_LIM_LOGY;
        else gd_properties.flags|= GD_LIM_LOGY;
      }
    }
    gd_set_limits();
    gh_before_wait();

/* ------- D and H level tests -------- */

  } else if (strcmp(token, "plg")==0) {
    PRINTF("   Lissajous test\n");
    get_time(&user0, &system0, &wall0);
    for (i=0 ; i<LISSFRAMES ; i++) {
      GenerateLiss(i, RC1, LISSSIZE, NA1, NB1);
      gh_get_lines();
      gd_properties.legend= "#1 Lissajous pattern";
      ga_attributes.dl.marks= markers;
      gd_add_lines(LISSPTS, xliss, yliss);
      GenerateLiss(i, RC2, LISSSIZE, NA2, NB2);
      gd_properties.legend= "#2 Lissajous pattern";
      ga_attributes.m.type= 0;  /* default marker */
      gd_add_lines(LISSPTS, xliss, yliss);
      gh_fma();
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", LISSFRAMES/(wall-wall0));

  } else if (strcmp(token, "plm")==0) {
    PRINTF("   Begin mesh benchmark.\n");
    get_time(&user0, &system0, &wall0);
    gh_get_lines();
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      gd_properties.legend= "Mesh lines";
      gd_add_mesh(noCopy, &mesh, meshReg, meshBnd, 0);
      gh_fma();
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "plf")==0) {
    PRINTF("   Begin filled mesh benchmark.\n");
    get_time(&user0, &system0, &wall0);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      GenerateColors();
      gd_properties.legend= "Filled mesh";
      gd_add_fillmesh(noCopy, &mesh, meshReg, cmesh+N+1, N);
      gh_fma();
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "plv")==0) {
    gp_real_t scale= 0.02;
    PRINTF("   Begin vector benchmark.\n");
    get_time(&user0, &system0, &wall0);
    gh_get_vectors();
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      gd_properties.legend= "Vectors";
      gd_add_vectors(noCopy, &mesh, meshReg, u, v, scale);
      gh_fma();
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "plc")==0) {
    PRINTF("   Begin contour benchmark.\n");
    get_time(&user0, &system0, &wall0);
    for (i=0 ; i<NFRAMES ; i++) {
      GenerateMesh(i==0, amplitude[i]);
      gh_get_lines();
      gd_properties.legend= "Contours";
      ga_attributes.dl.marks= markers;
      gd_add_contours(noCopy, &mesh, meshReg, z, zLevels, 10);
      gh_fma();
    }
    get_time(&user, &system, &wall);
    PRINTF3("elapsed seconds: %f user, %f system, %f wall\n",
           user-user0, system-system0, wall-wall0);
    PRINTF1("Plots per wall second= %f\n", NFRAMES/(wall-wall0));

  } else if (strcmp(token, "pli")==0) {
    gp_real_t cx= 0.5*(meshTrans.window.xmin+meshTrans.window.xmax);
    gp_real_t cy= 0.5*(meshTrans.window.ymin+meshTrans.window.ymax);
    gp_real_t px, qx, py, qy;
    PRINTF("   Plot image.\n");
    GenerateImage(noCopy, cx, cy, &px, &py, &qx, &qy);
    gd_properties.legend= "Image";
    gd_add_cells(px, py, qx, qy, N-1, N-1, N, cmesh+N+1);
    gh_before_wait();

  } else if (strcmp(token, "plg1")==0) {
    int i;
    PRINTF("   Plot small graph.\n");
    gh_get_lines();
    gd_properties.legend= "7 point graph";
    ga_attributes.dl.marks= markers;
    gd_add_lines(7, xSmall, ySmall);
    /* change curve for next pass */
    for (i=0 ; i<7 ; i++) ySmall[i]+= 1.0;
    gh_before_wait();

  } else if (strcmp(token, "pls")==0) {
    int i;
    PRINTF("   Plot small scatter.\n");
    gh_get_lines();
    gd_properties.legend= "7 point scatter";
    ga_attributes.dl.marks= 1;
    ga_attributes.l.type= GP_LINE_NONE;
    ga_attributes.m.type= markType++;
    if (markType>GP_MARKER_CROSS) {
      if (markType>='C') markType= GP_MARKER_POINT;
      else if (markType<'A') markType= 'A';
    }
    gd_add_lines(7, xSmall, ySmall);
    /* change curve for next pass */
    for (i=0 ; i<7 ; i++) ySmall[i]+= 1.0;
    gh_before_wait();

  } else if (strcmp(token, "pldj")==0) {
    int i;
    PRINTF("   Plot disjoint lines.\n");
    gh_get_lines();
    gd_properties.legend= "7 point disjoint";
    for (i=0 ; i<7 ; i++) {
      xqSmall[i]= xSmall[i]+1.5;
      yqSmall[i]= ySmall[i]+0.3;
    }
    gd_add_disjoint(7, xSmall, ySmall, xqSmall, yqSmall);
    /* change curve for next pass */
    for (i=0 ; i<7 ; i++) ySmall[i]+= 1.0;
    gh_before_wait();

  } else if (strcmp(token, "plc1")==0) {
    PRINTF("   Plot contours.\n");
    GenerateMesh(1, amplitude[NFRAMES-2]);
    gh_get_lines();
    gd_properties.legend= "Contours";
    ga_attributes.dl.marks= markers;
    gd_add_contours(noCopy, &mesh, meshReg, z, zLevels, 10);
    gh_before_wait();

  } else if (strcmp(token, "animate")==0) {
    if (animate) {
      animate= 0;
      gh_fma_mode(2, 0);
      PRINTF("   Toggle animation mode (now off)\n");
    } else {
      animate= 1;
      gh_fma_mode(2, 1);
      PRINTF("   Toggle animation mode (now on)\n");
    }
    gh_before_wait();

  } else if (strcmp(token, "marks")==0) {
    if (markers) {
      markers= 0;
      PRINTF("   Toggle markers (now off)\n");
    } else {
      markers= 1;
      PRINTF("   Toggle markers (now on)\n");
    }

  } else if (strcmp(token, "legends")==0) {
    if (gh_devices[0].doLegends) {
      gh_devices[0].doLegends= 0;
      PRINTF("   Toggle hcp legends (now off)\n");
    } else {
      gh_devices[0].doLegends= 1;
      PRINTF("   Toggle hcp legends (now on)\n");
    }

  } else if (strcmp(token, "plm1")==0) {
    PRINTF("   gd_add_mesh test\n");
    ga_attributes.l.color= PL_FG;
    gh_get_lines();
    GenerateMesh(1, amplitude[NFRAMES-2]);
    gd_properties.legend= "Test mesh";
    gd_add_mesh(noCopy, &mesh, meshReg, meshBnd, 0);
    gh_before_wait();

  } else if (strcmp(token, "plf1")==0) {
    PRINTF("   gd_add_fillmesh test\n");
    GenerateMesh(1, amplitude[NFRAMES-2]);
    GenerateColors();
    gd_properties.legend= "Test filled mesh";
    gd_add_fillmesh(noCopy, &mesh, meshReg, cmesh+N+1, N);
    gh_before_wait();

  } else if (strcmp(token, "txout")==0) {
    PRINTF("   Exterior text\n");
    gh_get_text();
    ga_attributes.t.color= PL_MAGENTA;
    ga_attributes.t.font= GP_FONT_NEWCENTURY;
    ga_attributes.t.height= 24.*GP_ONE_POINT;
    ga_attributes.t.opaque= 1;
    gd_properties.legend= 0;
    gd_add_text(0.2, 0.5, "Outside", 0);
    gh_before_wait();

  } else if (strcmp(token, "txin")==0) {
    PRINTF("   Interior text\n");
    gh_get_text();
    ga_attributes.t.color= PL_GREEN;
    ga_attributes.t.font= GP_FONT_NEWCENTURY;
    ga_attributes.t.height= 10.*GP_ONE_POINT;
    ga_attributes.t.opaque= 1;
    gd_properties.legend= 0;
    gd_add_text(0.5, 0.5, "Inside", 1);
    gh_before_wait();

  } else if (strcmp(token, "calib")==0) {
    gp_real_t y0= 0.85;
    gp_real_t x[2], y[2], x2[2];
    PRINTF("   Text calibration frame\n");
    gh_get_text();
    ga_attributes.t.font= calibFont;
    ga_attributes.t.height= 14.*GP_ONE_POINT;
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_NORMAL;
    ga_attributes.t.alignV= GP_VERT_ALIGN_NORMAL;
    ga_attributes.t.opaque= 0;
    gd_properties.legend= 0;
    gd_add_text(0.15, y0-0.0*ga_attributes.t.height, "Times 14 pt gjpqy", 0);
    gd_add_text(0.15, y0-1.0*ga_attributes.t.height, "Times 14 pt EFTZB", 0);
    gd_add_text(0.15, y0-2.0*ga_attributes.t.height, "Third line, 14 pt", 0);
    gd_set_system(-1);
    x[0]= x[1]= .30;
    y[0]= y0= 0.75;
    y[1]= y[0]+ga_attributes.t.height;
    gh_get_lines();
    gd_add_lines(2, x, y);
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_LEFT;
    gd_add_text(0.30, y0-1.0*ga_attributes.t.height, "Horizontal (lf)", 0);
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_CENTER;
    gd_add_text(0.30, y0-2.5*ga_attributes.t.height, "Horizontal (cn)", 0);
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_RIGHT;
    gd_add_text(0.30, y0-4.0*ga_attributes.t.height, "Horizontal (rt)", 0);
    y[0]= y0-4.5*ga_attributes.t.height;
    y[1]= y0-5.5*ga_attributes.t.height;
    gd_add_lines(2, x, y);
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_NORMAL;
    x[0]= 0.50;
    x[1]= 0.50-ga_attributes.t.height;
    x2[0]= 0.60;
    x2[1]= 0.60+ga_attributes.t.height;
    y[0]= y[1]= y0= .85;
    gd_add_lines(2, x, y);
    gd_add_lines(2, x2, y);
    ga_attributes.t.alignV= GP_VERT_ALIGN_TOP;
    gd_add_text(0.50, y0-0.0*ga_attributes.t.height, "Vertical (tp)", 0);
    y[0]= y[1]= y0-2.0*ga_attributes.t.height;
    gd_add_lines(2, x, y);
    gd_add_lines(2, x2, y);
    ga_attributes.t.alignV= GP_VERT_ALIGN_CAP;
    gd_add_text(0.50, y0-2.0*ga_attributes.t.height, "Vertical (cp)", 0);
    y[0]= y[1]= y0-4.0*ga_attributes.t.height;
    gd_add_lines(2, x, y);
    gd_add_lines(2, x2, y);
    ga_attributes.t.alignV= GP_VERT_ALIGN_HALF;
    gd_add_text(0.50, y0-4.0*ga_attributes.t.height, "Vertical (hf)", 0);
    y[0]= y[1]= y0-6.0*ga_attributes.t.height;
    gd_add_lines(2, x, y);
    gd_add_lines(2, x2, y);
    ga_attributes.t.alignV= GP_VERT_ALIGN_BASE;
    gd_add_text(0.50, y0-6.0*ga_attributes.t.height, "Vertical (ba)", 0);
    y[0]= y[1]= y0-8.0*ga_attributes.t.height;
    gd_add_lines(2, x, y);
    gd_add_lines(2, x2, y);
    ga_attributes.t.alignV= GP_VERT_ALIGN_BOTTOM;
    gd_add_text(0.50, y0-8.0*ga_attributes.t.height, "Vertical (bt)", 0);
    gd_set_system(0);
    gh_before_wait();

  } else if (strcmp(token, "cfont")==0) {
    if (calibFont!=GP_FONT_TIMES) {
      calibFont= GP_FONT_TIMES;
      PRINTF("   Toggle calib font (now Times)\n");
    } else {
      calibFont= GP_FONT_HELVETICA;
      PRINTF("   Toggle calib font (now Helvetica)\n");
    }

  } else if (strcmp(token, "fma")==0) {
    PRINTF("   Frame advance\n");
    gh_fma();

  } else if (strcmp(token, "hcp")==0) {
    PRINTF("   Sent frame to hcp\n");
    gh_hcp();

  } else if (strcmp(token, "hcpon")==0) {
    PRINTF("   hcp on fma\n");
    gh_fma_mode(1, 2);

  } else if (strcmp(token, "hcpoff")==0) {
    PRINTF("   NO hcp on fma\n");
    gh_fma_mode(0, 2);

  } else if (strcmp(token, "redraw")==0) {
    PRINTF("   Redraw\n");
    gh_redraw();

  } else if (strcmp(token, "plt")==0) {
    /* Test text primitive */
    gp_real_t ynow= 0.90;
    PRINTF("   Text test\n");
    gh_get_text();
    ga_attributes.t.color= PL_FG;
    ga_attributes.t.font= GP_FONT_HELVETICA;
    ga_attributes.t.height= GP_ONE_POINT*12.0;
    gd_add_text(0.25, ynow, "Helvetica 12 pt?", 0);
    ynow-= ga_attributes.t.height;
    gd_add_text(0.25, ynow, "Helvetica 12 pt, second line", 0);
    ga_attributes.t.height= GP_ONE_POINT*18.0;
    ynow-= ga_attributes.t.height;
    gd_add_text(0.25, ynow, "Helvetica 18 pt?", 0);
    ga_attributes.t.font= GP_FONT_NEWCENTURY;
    ga_attributes.t.height= GP_ONE_POINT*12.0;
    ynow-= ga_attributes.t.height;
    gd_add_text(0.25, ynow, "New Century 12 pt, first line", 0);
    ynow-= ga_attributes.t.height;
    ga_attributes.t.font|= GP_FONT_BOLD | GP_FONT_ITALIC;
    gd_add_text(0.25, ynow, "New Century 12 pt, bold italic", 0);
    ga_attributes.t.font= GP_FONT_NEWCENTURY;
    ga_attributes.t.height= GP_ONE_POINT*14.0;
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_RIGHT;
    ynow-= ga_attributes.t.height;
    gd_add_text(0.65, ynow, "New Century 14 pt,\nthree lines\nright justified", 0);
    ynow-= 3*ga_attributes.t.height;
    ga_attributes.t.alignH= GP_HORIZ_ALIGN_NORMAL;
    ga_attributes.t.orient= GP_ORIENT_DOWN;
    gd_add_text(0.25, ynow, "Path down", 0);
    gh_before_wait();

  } else if (strcmp(token, "help")==0 || strcmp(token, "?")==0) {
    PRINTF("\n   This is the GIST benchmark/test program, commands are:\n");
    PRINTF("\n    Movies to test low level Gist functions--\n");
    PRINTF("0   - raw performance test ga_draw_mesh (direct)\n");
    PRINTF("1   - raw performance test ga_draw_mesh (animated)\n");
    PRINTF("2   - raw performance test ga_fill_mesh (direct)\n");
    PRINTF("3   - raw performance test ga_fill_mesh (animated)\n");
    PRINTF("clr - toggle interframe clear for tests 0-3\n");
    PRINTF("c   - clear screen\n");
    PRINTF("im  - raw performance test gp_draw_cells\n");
    PRINTF("clip  - toggle floating point clip\n");
    PRINTF("zoom  - toggle zoom for 0-3, im, pli\n");
    PRINTF("\n    Property toggles for high level tests--\n");
    PRINTF("bnd  - mesh boundary          odd0  - mesh region to 0\n");
    PRINTF("odd2  - mesh region to 2      reg0  - plot region to 0\n");
    PRINTF("reg1  - plot region to 1      reg2  - plot region to 2\n");
    PRINTF("nocopy - toggle noCopy mode for mesh-based tests\n");
    PRINTF("cmode - toggle dumping of colormap in hcp file\n");
    PRINTF("earth - use earthtones palette (default).  Others are:\n");
    PRINTF("     stern   rainbow   heat   gray   yarg\n");
    PRINTF("work  - use work stylesheet (default).  Others are:\n");
    PRINTF("     boxed   axes\n");
    PRINTF("wide      - toggle wide lines in all line drawings\n");
    PRINTF("linetype  - cycle through 5 line types in all line drawings\n");
    PRINTF("closed    - toggle closed lines in plg tests\n");
    PRINTF("rays      - toggle ray arrows in plg tests\n");
    PRINTF("smooth    - cycle through 3 hcp smoothnesses in plg tests\n");
    PRINTF("limits, xmin, xmax, ymin, ymax, nice=1/0, square=1/0,\n");
    PRINTF("        restrict=1/0   - set limits (default limits,e,e,e,e)\n");
    PRINTF("logxy, 1/0, 1/0   - set/reset log scaling for x,y axes\n");
    PRINTF("animate  - toggle animation mode\n");
    PRINTF("marks    - toggle occasional curve markers for line drawings\n");
    PRINTF("legends  - toggle dumping of legends into hcp file\n");
    PRINTF("\n    Movies to test high level Gist functions--\n");
    PRINTF("plg  - test gd_add_lines\n");
    PRINTF("plm  - test gd_add_mesh\n");
    PRINTF("plf  - test gd_add_fillmesh\n");
    PRINTF("plv  - test gd_add_vectors\n");
    PRINTF("plc  - test gd_add_contours\n");
    PRINTF("\n    Single frame tests of high level Gist functions--\n");
    PRINTF("plg1  - test gd_add_lines\n");
    PRINTF("plm1  - test gd_add_mesh\n");
    PRINTF("plf1  - test gd_add_fillmesh\n");
    PRINTF("plc1  - test gd_add_contours\n");
    PRINTF("pli   - test gd_add_cells\n");
    PRINTF("plt   - test gd_add_text\n");
    PRINTF("pls   - test gd_add_lines in polymarker mode\n");
    PRINTF("pldj  - test gd_add_disjoint\n");
    PRINTF("txin  - test gd_add_text     txout  - 2nd test of gd_add_text\n");
    PRINTF("calib - text calibration frame\n");
    PRINTF("cfont - toggle font for calibration frame\n");
    PRINTF("\n    Tests of high level Gist control functions--\n");
    PRINTF("fma    - frame advance\n");
    PRINTF("hcp    - send current frame to hardcopy file\n");
    PRINTF("hcpon  - send every frame to hardcopy file\n");
    PRINTF("hcpoff - stop sending every frame to hardcopy file\n");
    PRINTF("redraw - redraw the X window\n");
    PRINTF("\nquit  - exit bench program (just q works too)\n");

  } else {
    PRINTF("  NO ACTION TAKEN -- type help for (long) list of commands\n");
  }

  return;
}

static int waitTest= 0;
static void window_exposed(void);
static int no_key = 0;

int
on_idle(void)
{
  if (noKeyboard) {
    if (noKeyTest[no_key]) {
      char line[24];
      strcpy(line, noKeyTest[no_key++]);
      on_stdin(line);
    } else {
      pl_quit();
    }
    return 1;
  }
  if (waitTest || !prompt_issued) {
    gh_before_wait();
    pl_stdout("bench> ");
    prompt_issued = 1;
  }
  return 0;
}

static char *default_path = "~/gist:~/Gist:../g";

int
pl_on_launch(int argc, char *argv[])
{
  int i, j;
  int usePS= (argc>1 && argv[1][0]!='0');

  gp_real_t ypage= 1.033461;  /* 11.0 inches in NDC */
  gp_real_t xpage= 0.798584;  /* 8.5 inches in NDC */
  gp_real_t meshSize;

  noKeyboard = waitTest = (argc>1 && argv[1][0]=='-');

  gp_default_path = default_path;

  pl_stdinit(&on_stdin);
  pl_idler(&on_idle);
  pl_handler(&on_exception);
  pl_quitter(&on_quit);

#ifndef NO_XLIB
  gp_initializer(&argc, argv);
  engine= gp_new_fx_engine("Gist bench test", 0, DPI, argc>2? argv[2] : 0);

  gh_hcp_default= usePS? gp_new_ps_engine("Gist bench test", 0, 0, "bench.ps") :
                     gp_new_cgm_engine("Gist bench test", 0, 0, "bench.cgm");
#else
  engine= gp_new_cgm_engine("Gist bench test", 0, 0, "bench.cgm");

  gh_hcp_default= gp_new_ps_engine("Gist bench test", 0, 0, "bench.ps");
#endif

  gh_devices[0].drawing= gd_new_drawing("work.gs");
  gh_devices[0].display= engine;
  gh_devices[0].hcp= 0;
  gh_devices[0].doLegends= 0;

  actualColors= gp_read_palette(engine, PALETTE_FILE, &palette, 240);
  gp_set_palette(gh_hcp_default, palette, actualColors);

  /*  gp_activate(engine);  */
  gh_set_plotter(0);

  meshTrans.window= unitBox;

  /* Set up viewport to be 512-by-512 pixels at 100 dpi,
     centered for portrait mode.  */
  meshSize= GP_ONE_INCH*5.125;
  meshTrans.viewport.xmin= 0.5*(xpage-meshSize);
  meshTrans.viewport.xmax= 0.5*(xpage+meshSize);
  meshTrans.viewport.ymin= ypage-0.5*(xpage+meshSize);
  meshTrans.viewport.ymax= ypage-0.5*(xpage-meshSize);

  /* Initialize mesh arrays */
  mesh.iMax= N;
  mesh.jMax= N;
  mesh.x= xmesh= (gp_real_t *)pl_malloc(sizeof(gp_real_t)*N*N);
  mesh.y= ymesh= (gp_real_t *)pl_malloc(sizeof(gp_real_t)*N*N);
  mesh.reg= ireg= (int *)pl_malloc(sizeof(int)*(N*N+N+1));
  mesh.triangle= 0;
  cmesh= (gp_color_t *)pl_malloc(sizeof(gp_color_t)*N*N);
  z= (gp_real_t *)pl_malloc(sizeof(gp_real_t)*N*N);
  u= (gp_real_t *)pl_malloc(sizeof(gp_real_t)*N*N);
  v= (gp_real_t *)pl_malloc(sizeof(gp_real_t)*N*N);
  j= 0;
  for (i=0 ; i<N*N+N+1 ; i++) {
    if (j==0 || i<N || i>=N*N) ireg[i]= 0;
    else ireg[i]= 1;
    j++;
    if (j==N) j= 0;
  }

  /* Initialize cosines and sines for perturbation amplitude */
  for (i=0 ; i<N ; i++) cosine[i]= cos(i*PI/(N-1.0));
  for (i=0 ; i<NFRAMES ; i++) amplitude[i]= AMPLITUDE*sin(i*DPHI);

  /* Initialize contour level function */
  PRINTF2("Initializing %dx%d contour function, STANDBY...", N, N);
  for (i=0 ; i<N*N ; i++) z[i]= 0.0;
  TenseZ();
  for (i=0 ; i<300 ; i++) { RelaxZ(); TenseZ(); }
  PRINTF("  DONE\n");

  /* Initialize vector function */
  for (i=0 ; i<N*N ; i++) { u[i]= z[i];  v[N*N-1-i]= z[i]; }

  if (waitTest || noKeyboard) gp_expose_wait(engine, window_exposed);

  return 0;
}

static void
window_exposed(void)
{
  waitTest = 0;
}

int
on_quit(void)
{

  gp_deactivate(gh_hcp_default);
  gp_kill_engine(gh_hcp_default);
  gh_hcp_default= 0;

  gp_deactivate(engine);
  gp_kill_engine(engine);
  gh_devices[0].display= 0;

  return 0;
}

void
on_exception(int signal, char *errmsg)
{
  PRINTF2("\n\nbench: signal %d, msg = %s\n\n", signal,
          errmsg? errmsg : "<none>");
  pl_quit();
}
