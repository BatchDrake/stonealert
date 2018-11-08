/*
 * stonealert.h: headers, prototypes and declarations for stonealert
 * Creation date: Sat Nov  3 11:24:08 2018
 */

#ifndef _MAIN_INCLUDE_H
#define _MAIN_INCLUDE_H

#include <config.h> /* General compile-time configuration parameters */
#include <util.h> /* From util: Common utility library */
#include <sigutils/sigutils.h> /* From sigutils: Signal utils */
#include <sigutils/log.h>
#include <wbmp.h> /* From sim-static: Built-in simulation library over SDL */
#include <draw.h> /* From sim-static: Built-in simulation library over SDL */
#include <cpi.h> /* From sim-static: Built-in simulation library over SDL */
#include <ega9.h> /* From sim-static: Built-in simulation library over SDL */
#include <hook.h> /* From sim-static: Built-in simulation library over SDL */
#include <layout.h> /* From sim-static: Built-in simulation library over SDL */
#include <pearl-m68k.h> /* From sim-static: Built-in simulation library over SDL */
#include <pixel.h> /* From sim-static: Built-in simulation library over SDL */
#include <time.h>
#include "graves.h"

#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define SCREEN_PADDING 10

#define SCREEN_COLOR OPAQUE(0)

#define FRAME_COLOR OPAQUE(RGB(0xff, 0xa5, 0x00))
#define REAL_COLOR  OPAQUE(RGB(0xff, 0xff, 0x00))
#define IMAG_COLOR  OPAQUE(RGB(0x00, 0x7f, 0x00))

struct sa_graph {
  const char *title;
  unsigned int x, y;
  unsigned int width, height;
  SUFLOAT alpha;
};

struct sa_chirp_properties {
  SUFLOAT SNR;
  SUFLOAT mean_doppler;
  SUFLOAT duration;
};

struct sa_app {
  display_t    *disp;
  textarea_t   *history;
  graves_det_t *detector;

  time_t  start;
  SUFLOAT peak_snr;
  SUFLOAT peak_duration;
  SUFLOAT peak_doppler;

  unsigned int event_count;

  int table_x;
  int table_y;

  struct sa_graph chirp;
  struct sa_graph doppler;

  char *path;
  FILE *fp;
};

#endif /* _MAIN_INCLUDE_H */
