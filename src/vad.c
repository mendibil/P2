#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const int MIN_FRAMES_VOICE    = 10;
const int MIN_FRAMES_SILENCE  = 10;
const int MIN_FRAMES_INIT     = 10;
const float THRESHOLD = 8;


const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MAYBE_S", "MAYBE_V"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

 
// DONE: TODO: Delete and use your own features!
Features compute_features(const float *x, int N, float fm) {
  Features feat;

  feat.zcr  = compute_zcr(x,N, fm);
  feat.p    = compute_power(x,N);
  feat.am   = compute_am(x,N);

  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */
VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA)); //se sap aquesta size? sha de modificar?
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->umbral_1 = 0;
  vad_data->umbral_2 = 0;
  vad_data->umbral_zcr=1000;
  vad_data->frames_in_maybe_v = 0;
  vad_data->frames_in_maybe_s = 0;
  vad_data->frames_in_init =    0;
  return vad_data;
}


VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->state;

  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length, vad_data->sampling_rate);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {
  case ST_INIT:
    vad_data->frames_in_init++;
    vad_data->umbral_1 = vad_data->umbral_1 + f.p;
    if(vad_data->frames_in_init == MIN_FRAMES_INIT){
      vad_data->state = ST_SILENCE;
      vad_data->umbral_1 = vad_data->umbral_1/MIN_FRAMES_INIT + 4; //Umbral per provar
      vad_data->umbral_2 = vad_data->umbral_1 + 4;
    }
    break;

  case ST_SILENCE:
    if (f.p >= vad_data->umbral_1 || f.zcr > vad_data->umbral_zcr)
      vad_data->state = ST_MAYBE_VOICE;
    break;

  case ST_MAYBE_VOICE:
    if (f.p < vad_data->umbral_2 && f.zcr < vad_data->umbral_zcr) {
      vad_data->state = ST_SILENCE;
    } else {
      vad_data->frames_in_maybe_v++;
      if(vad_data->frames_in_maybe_v == MIN_FRAMES_VOICE) {
        vad_data->state = ST_VOICE;
        vad_data->frames_in_maybe_v = 0;
      }
    }
    break;

  case ST_VOICE:
    if (f.p < vad_data->umbral_2 && f.zcr < vad_data->umbral_zcr)
      vad_data->state = ST_MAYBE_SILENCE;
    break;

  case ST_MAYBE_SILENCE:
    if (f.p > vad_data->umbral_1 || f.zcr > vad_data->umbral_zcr) {
      vad_data->state = ST_VOICE;
    } else {
      vad_data->frames_in_maybe_s++;
      if(vad_data->frames_in_maybe_s == MIN_FRAMES_SILENCE) {
        vad_data->state = ST_SILENCE;
        vad_data->frames_in_maybe_s = 0;
      }
    }
  break;


  case ST_UNDEF:
    break;
  }
  return vad_data->state;
/*
  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE)
    return vad_data->state;
  else if
    return ST_UNDEF;
    */
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
