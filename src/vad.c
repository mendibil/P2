#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"

const float FRAME_TIME        = 10.0F;

//constants a iterar

int   MIN_INIT  = 10;
float ALFA_L    = 99;
float ALFA_H    = 6; 
int   ZCR_LIFT  = 1600; 
int   MAX_MS    = 20;
int   MAX_MV    = 6;
int   MIN_S     = 1;   
int   MIN_V     = 5;
 




const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MAYBE_S", "MAYBE_V"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

typedef struct {
  float zcr;
  float p;
  float am;
} Features;
 

Features compute_features(const float *x, int N, float fm) {
  Features feat;
  feat.zcr  = compute_zcr(x,N, fm);
  feat.p    = compute_power(x,N);
  feat.am   = compute_am(x,N);

  return feat;
}


VAD_DATA * vad_open(  float rate, char *_ALFA_L, char *_ALFA_H, char *_MAX_MS, 
                      char *_MAX_MV, char *_ZCR_LIFT, char *_MIN_S, char *_MIN_V) { //per poder iterar amb el script
    VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
    vad_data->state = ST_INIT;
    vad_data->sampling_rate = rate;
    vad_data->frame_length = rate * FRAME_TIME * 1e-3;
    vad_data->umbral_1          = 0;
    vad_data->umbral_2          = 0;
    vad_data->umbral_zcr        = 0;
    vad_data->frames_in_maybe_v = 0;
    vad_data->frames_in_maybe_s = 0;
    vad_data->frames_in_init    = 0;
    vad_data->frames_in_s       = 0;
    vad_data->frames_in_v       = 0;
    ALFA_L    = (float) strtod(_ALFA_L, NULL) /100;
    ALFA_H    = (float) strtod(_ALFA_H, NULL) /100;
    ZCR_LIFT  = (float) strtod(_ZCR_LIFT, NULL);
    MAX_MS    = atoi(_MAX_MS);
    MAX_MV    = atoi(_MAX_MV);
    MIN_S     = atoi(_MIN_S);
    MIN_V     = atoi(_MIN_V);
    

    return vad_data;
}

VAD_DATA * vad_open_(float rate) { //constants fixades per nosaltres
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
    vad_data->state = ST_INIT;
    vad_data->sampling_rate = rate;
    vad_data->frame_length = rate * FRAME_TIME * 1e-3;
    vad_data->umbral_1          = 0;
    vad_data->umbral_2          = 0;
    vad_data->umbral_zcr        = 0;
    vad_data->frames_in_maybe_v = 0;
    vad_data->frames_in_maybe_s = 0;
    vad_data->frames_in_init    = 0;
    vad_data->frames_in_s       = 0;
    vad_data->frames_in_v       = 0;
    ALFA_L    = 6.87;
    ALFA_H    = 1.95;
    ZCR_LIFT  = 1630;
    MAX_MS    = 15;
    MAX_MV    = 3;
    MIN_S     = 1;
    MIN_V     = 1;
    
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


VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  Features f = compute_features(x, vad_data->frame_length, vad_data->sampling_rate);
  vad_data->last_feature = f.p;

  switch (vad_data->state) {
  case ST_INIT:
    vad_data->frames_in_init++;
    vad_data->umbral_1 +=  f.p;
    vad_data->umbral_zcr += f.zcr;

    if(vad_data->frames_in_init == MIN_INIT){
      vad_data->frames_in_init = 0;
      vad_data->state = ST_SILENCE;
      vad_data->umbral_1 = vad_data->umbral_1/MIN_INIT + ALFA_L;
      vad_data->umbral_2 = vad_data->umbral_1 + ALFA_H;
      vad_data->umbral_zcr = vad_data->umbral_zcr/MIN_INIT + ZCR_LIFT;
    }
    break;

  case ST_SILENCE:
    vad_data->frames_in_s++;
    if (vad_data->frames_in_s >= MIN_S && (f.p > vad_data->umbral_1)){//} || f.zcr > vad_data->umbral_zcr)) { //DONAVA MILLORS RESULTATS SENSE ZCR
        vad_data->state = ST_MAYBE_VOICE;
        vad_data->frames_in_s = 0;
    }
    break;

  case ST_MAYBE_VOICE:
    vad_data->frames_in_maybe_v++;
    /*
    if((f.p > vad_data->umbral_2 || f.zcr > vad_data->umbral_zcr) && vad_data->frames_in_maybe_v < MAX_MV) {
      vad_data->state = ST_VOICE;
      vad_data->frames_in_maybe_v = 0;
    }
    if(vad_data->frames_in_maybe_v == MAX_MV) {
      vad_data->state = ST_SILENCE;
      vad_data->frames_in_maybe_v = 0;
    }
    */

    if (f.p < vad_data->umbral_2 && f.zcr < vad_data->umbral_zcr) { //ns si acaba destar be el zcr be x fricatives pero altres idk
      vad_data->state = ST_SILENCE;
      vad_data->frames_in_maybe_v = 0;
    } else if(vad_data->frames_in_maybe_v == MAX_MV) {
      vad_data->state = ST_VOICE;
      vad_data->frames_in_maybe_v = 0;
    }

    break;

  case ST_VOICE:
    vad_data->frames_in_v++;
    if (vad_data->frames_in_v >= MIN_V && (f.p < vad_data->umbral_2 && /*||*/ f.zcr < vad_data->umbral_zcr)) {
      //COMPROVAR CRUCES AMB WAVESURFER
        vad_data->state = ST_MAYBE_SILENCE;
        vad_data->frames_in_v = 0;
    }
    break;

  case ST_MAYBE_SILENCE:
    vad_data->frames_in_maybe_s++;
    if (f.p > vad_data->umbral_1 || f.zcr > vad_data->umbral_zcr) {
      vad_data->state = ST_VOICE;
      vad_data->frames_in_maybe_s = 0;
    } else if(vad_data->frames_in_maybe_s == MAX_MS) {
      vad_data->state = ST_SILENCE;
      vad_data->frames_in_maybe_s = 0;
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
