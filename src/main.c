/*
 * main.c: entry point for stonealert
 * Creation date: Sat Nov  3 11:24:08 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <stonealert.h>
#include "alsa.h"

SUBOOL
sa_graph_init(
    struct sa_graph *graph,
    const char *title,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height)
{
  graph->title = title;
  graph->x = x;
  graph->y = y;
  graph->width = width;
  graph->height = height;

  graph->alpha = 1 - exp(-1. / (.1 * width));

  return SU_TRUE;
}

/* Warning: may cause aliasing! */
void
sa_graph_draw_data(
    struct sa_graph *graph,
    display_t *disp,
    SUFLOAT xmin, SUFLOAT xmax,
    const SUCOMPLEX *data,
    unsigned int len)
{
  unsigned int i;
  unsigned int x, x_prev;
  unsigned int yr, yr_prev;
  unsigned int yi, yi_prev;

  SUFLOAT ymax = 0;

  /* Paint box */
  fbox(
      disp,
      graph->x,
      graph->y,
      graph->x + graph->width,
      graph->y + graph->height,
      SCREEN_COLOR);

  box(
      disp,
      graph->x,
      graph->y,
      graph->x + graph->width,
      graph->y + graph->height,
      FRAME_COLOR);

  /* Detect scale factor */
  for (i = 0; i < len - 1; ++i)
    if (SU_C_ABS(data[i]) > ymax)
      ymax += graph->alpha * (SU_C_ABS(data[i]) - ymax);

  /* Paint data */
  for (i = 0; i < len; ++i) {
    yi = graph->y - graph->height * .5 * (SU_C_IMAG(data[i]) / ymax - 1);
    yr = graph->y - graph->height * .5 * (SU_C_REAL(data[i]) / ymax - 1);
    x = graph->x + (graph->width - 1) * i / (SUFLOAT) (len - 1);

    if (yi < graph->y)
      yi = graph->y;
    else if (yi >= graph->y + graph->height)
      yi = graph->y + graph->height - 1;

    if (yr < graph->y)
      yr = graph->y;
    else if (yr >= graph->y + graph->height)
      yr = graph->y + graph->height - 1;

    if (i > 1) {
      line(disp, x_prev, yi_prev, x, yi, IMAG_COLOR);
      line(disp, x_prev, yr_prev, x, yr, REAL_COLOR);
    }

    yr_prev = yr;
    yi_prev = yi;
    x_prev = x;
  }

  /* Paint title */
  display_printf(
      disp,
      graph->x,
      graph->y - 8,
      FRAME_COLOR,
      SCREEN_COLOR,
      "%s - %lg peak",
      graph->title,
      ymax);
}

SUCOMPLEX *
sa_app_process_chirp(
    struct sa_app *app,
    SUFLOAT fs,
    const SUCOMPLEX *data,
    SUSCOUNT len,
    SUFLOAT N0,
    struct sa_chirp_properties *result)
{
  SUCOMPLEX *doppler;
  SUFLOAT offset, this_doppler, doppler_sum = 0;
  SUFLOAT S0 = 0;
  SUCOMPLEX prev;

  unsigned int i;

  SU_TRYCATCH(doppler = calloc(sizeof(SUCOMPLEX), len), return NULL);

  for (i = 0; i < len; ++i) {
    S0 += SU_C_REAL(data[i] * SU_C_CONJ(data[i]));

    /* Get Doppler in Hertz */
    offset = SU_C_ARG(data[i] * SU_C_CONJ(prev)) / (2 * M_PI) * fs;

    /* Convert it to m/s */
    this_doppler = SPEED_OF_LIGHT * offset / GRAVES_CENTER_FREQ;

    doppler_sum += this_doppler;
    doppler[i] = this_doppler;

    prev = data[i];
  }

  result->SNR = SU_POWER_DB_RAW(S0 / (GRAVES_POWER_RATIO * len * N0));
  result->mean_doppler = doppler_sum / len;
  result->duration = len / fs;

  return doppler;
}

SUPRIVATE SUBOOL
sa_app_on_chirp(
       void *private,
       SUFLOAT fs,
       SUSCOUNT start,
       const SUCOMPLEX *data,
       SUSCOUNT len,
       SUFLOAT N0)
{
  struct sa_app *app = (struct sa_app *) private;
  struct sa_chirp_properties prop;
  SUCOMPLEX *doppler;
  unsigned int seconds;
  char date[32];
  time_t time;
  int i, blocks;

  ++app->event_count;

  SU_TRYCATCH(
      doppler = sa_app_process_chirp(app, fs, data, len, N0, &prop),
      return SU_FALSE);

  /* Update limits */
  if (prop.SNR > app->peak_snr)
    app->peak_snr = prop.SNR;

  if (len / fs > app->peak_duration)
    app->peak_duration = len / fs;

  if (SU_ABS(prop.mean_doppler) > app->peak_doppler)
    app->peak_doppler = SU_ABS(prop.mean_doppler);

  /* Display */
  seconds = start;
  time = app->start + start;

  strncpy(date, ctime(&time), sizeof(date));

  date[strlen(date) - 1] = '\0';

  sa_graph_draw_data(
      &app->chirp,
      app->disp,
      start / fs,
      (start + len) / fs,
      data,
      len);

  sa_graph_draw_data(
      &app->doppler,
      app->disp,
      start / fs,
      (start + len) / fs,
      doppler,
      len);

  textarea_set_fore_color(app->history, FRAME_COLOR);

  cprintf(
      app->history,
      "+%02d:%02d:%02d \xb3 %+7.1lf m/s \xb3 %6.3lf s \xb3 %+5.1lf dB ",
      (seconds / 3600),
      (seconds / 60) % 60,
      seconds % 60,
      prop.mean_doppler,
      len / fs,
      prop.SNR);

  blocks = prop.SNR;
  if (blocks > 20)
    blocks = 20;

  for (i = 0; i < blocks; ++i) {
    textarea_set_fore_color(
        app->history,
        OPAQUE(calc_color(1536 + 768 - i * 38.4)));
    cputs(app->history, "\xfe");
  }

  cputs(app->history, "\n");

  display_puts(
      app->disp,
      app->table_x,
      app->table_y,
      FRAME_COLOR,
      SCREEN_COLOR,
      "   TIME   \xb3 AVG DOPPLER \xb3 Duration \xb3 Integrated SNR");

  display_puts(
      app->disp,
      app->table_x,
      app->table_y + 8,
      FRAME_COLOR,
      SCREEN_COLOR,
      "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc5"
      "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc5"
      "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc5"
      "\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4");

  display_printf(
      app->disp,
      SCREEN_PADDING,
      SCREEN_PADDING,
      FRAME_COLOR,
      SCREEN_COLOR,
      "%s    =    STONE DETECTOR    =   %5d events registered",
      date,
      app->event_count);


  display_printf(
      app->disp,
      SCREEN_PADDING,
      3 * SCREEN_PADDING - 8,
      FRAME_COLOR,
      SCREEN_COLOR,
      "PEAK SNR: %.1lf dB dB - PEAK DURATION: %.3lf s - PEAK DOPPLER: %.1lf m/s",
      app->peak_snr,
      app->peak_duration,
      app->peak_doppler);

  fprintf(
      app->fp,
      "%ld;%lg;%lg;",
      time,
      prop.SNR,
      prop.mean_doppler);

  for (i = 0; i < len; ++i)
    fprintf(app->fp, "%.8lf%+.8lfi,", SU_C_REAL(data[i]), SU_C_IMAG(data[i]));

  fprintf(app->fp, "\n");

  fflush(app->fp);

  display_refresh(app->disp);

done:
  free(doppler);

  return SU_TRUE;
}

SUBOOL
sa_app_init(struct sa_app *app, SUFLOAT fs, SUFLOAT fc)
{
  unsigned int i;
  unsigned int height, y;
  time_t t;
  char *graves_path = NULL;

  memset(app, 0, sizeof(struct sa_app));

  time(&app->start);

  SU_TRYCATCH(app->path = strbuild("chirps_%d.dat", app->start), return SU_FALSE);

  if ((app->fp = fopen(app->path, "w")) == NULL) {
    fprintf(stderr, "su_app_init: cannot open %s for writing\n", app->path);
    return SU_FALSE;
  }

  time(&t);

  SU_TRYCATCH(
      app->disp = display_new(SCREEN_WIDTH, SCREEN_HEIGHT),
      return SU_FALSE);

  y = 3 * SCREEN_PADDING;

  height = .2 * SCREEN_HEIGHT - 2 * SCREEN_PADDING;
  sa_graph_init(
      &app->chirp,
      "Reflected amplitude (complex)",
      SCREEN_PADDING,
      SCREEN_PADDING + y,
      SCREEN_WIDTH  - 2 * SCREEN_PADDING,
      height);

  y += 2 * SCREEN_PADDING + height;

  height = .2 * SCREEN_HEIGHT - 2 * SCREEN_PADDING;
  sa_graph_init(
      &app->doppler,
      "Doppler shift (m/s)",
      SCREEN_PADDING,
      SCREEN_PADDING + y,
      SCREEN_WIDTH - 2 * SCREEN_PADDING,
      .2 * SCREEN_HEIGHT - 2 * SCREEN_PADDING);
  y += 2 * SCREEN_PADDING + height;

  app->table_x = SCREEN_PADDING;
  app->table_y = SCREEN_PADDING + y;

  SU_TRYCATCH(
      app->history = display_textarea_new(
          app->disp,
          app->table_x,
          app->table_y + 16,
          (SCREEN_WIDTH - 2 * SCREEN_PADDING) / 8,
          (SCREEN_HEIGHT - app->table_y - 2 * SCREEN_PADDING) / 8,
          NULL,
          850,
          8),
      return SU_FALSE);

  textarea_set_fore_color(app->history, FRAME_COLOR);
  textarea_set_back_color(app->history, SCREEN_COLOR);

  SU_TRYCATCH(
      app->detector = graves_det_new(fs, fc, sa_app_on_chirp, app),
      return SU_FALSE);

  SDL_WM_SetCaption("GRAVES echo detector", "GRAVES echo detector");

  clear(app->disp, SCREEN_COLOR);
  display_printf(
      app->disp,
      app->table_x,
      app->table_y,
      FRAME_COLOR,
      SCREEN_COLOR,
      "No GRAVES echoes yet...");

  display_refresh(app->disp);

  return SU_TRUE;
}

int
main (int argc, char *argv[], char *envp[])
{
  struct sa_app app;
  SUCOMPLEX sample;
  struct alsa_params params = alsa_params_INITIALIZER;
  struct alsa_state *state = NULL;
  char *opath;
  int got, i;

  FILE *fp;

  if (argc == 2) {
    if ((fp = fopen(argv[1], "rb")) == NULL) {
      fprintf(
          stderr,
          "%s: cannot open `%s': %s\n",
          argv[0],
          argv[1],
          strerror(errno));
      exit(EXIT_FAILURE);
    }

    if (!sa_app_init(&app, 8000, 1000)) {
      fprintf(stderr, "%s: failed to initialize application\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    while (fread(&sample, sizeof(sample), 1, fp) == 1)
      graves_det_feed(app.detector, sample);

  } else {
    SU_TRYCATCH(state = alsa_state_new(&params), exit(EXIT_FAILURE));

    if (!sa_app_init(&app, params.samp_rate, 1000)) {
      fprintf(stderr, "%s: failed to initialize application\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    SU_TRYCATCH(
      opath = strbuild("capture_%d_%dsps.raw", time(NULL), params.samp_rate),
      exit(EXIT_FAILURE));
                
    if ((fp = fopen(opath, "wb")) == NULL) {
      fprintf(
        stderr,
        "%s: cannot open output capture file %s: %s\n",
        argv[0],
        opath,
        strerror(errno));
      exit(EXIT_FAILURE);
    }
    
    while ((got = snd_pcm_readi(
        state->handle,
        state->buffer,
        ALSA_INTEGER_BUFFER_SIZE)) > 0)
      for (i = 0; i < got; ++i) {
        sample = state->buffer[i] / 32768.0;
        graves_det_feed(app.detector, sample);
        fwrite(&sample, sizeof(SUCOMPLEX), 1, fp);
      }

    display_poll_events(app.disp);
  }

  display_end(app.disp);

  return 0;
}

