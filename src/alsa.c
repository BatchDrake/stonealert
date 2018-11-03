/*

  Copyright (C) 2018 Gonzalo José Carracedo Carballal

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program.  If not, see
  <http://www.gnu.org/licenses/>

*/

#include "alsa.h"

void
alsa_state_destroy(struct alsa_state *state)
{
  if (state->handle != NULL)
    snd_pcm_close(state->handle);

  free(state);
}

struct alsa_state *
alsa_state_new(const struct alsa_params *params)
{
  struct alsa_state *new = NULL;
  snd_pcm_hw_params_t *hw_params = NULL;
  int err = 0;
  int dir;
  unsigned int rate;
  SUBOOL ok = SU_FALSE;

  SU_TRYCATCH(new = calloc(1, sizeof(struct alsa_state)), goto done);

  new->fc = params->fc;
  new->dc_remove = params->dc_remove;

  SU_TRYCATCH(
      (err = snd_pcm_open(
          &new->handle,
          params->device,
          SND_PCM_STREAM_CAPTURE,
          0)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_malloc(&hw_params)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_any(new->handle, hw_params)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_set_access(
          new->handle,
          hw_params,
          SND_PCM_ACCESS_RW_INTERLEAVED)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_set_format(
          new->handle,
          hw_params,
          SND_PCM_FORMAT_S16_LE)) >= 0,
      goto done);

  rate = params->samp_rate;

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_set_rate_near(
          new->handle,
          hw_params,
          &rate,
          0)) >= 0,
      goto done);


  new->samp_rate = rate;

  SU_TRYCATCH(
      (err = snd_pcm_hw_params_set_channels(
          new->handle,
          hw_params,
          1)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_hw_params(new->handle, hw_params)) >= 0,
      goto done);

  SU_TRYCATCH(
      (err = snd_pcm_prepare(new->handle)) >= 0,
      goto done);

  ok = SU_TRUE;

done:
  if (err < 0)
    SU_ERROR("ALSA source initialization failed: %s\n", snd_strerror(err));

  if (hw_params != NULL)
    snd_pcm_hw_params_free(hw_params);

  if (!ok && new != NULL) {
    alsa_state_destroy(new);
    new = NULL;
  }

  return new;
}
