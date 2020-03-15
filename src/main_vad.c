#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sndfile.h>

#include "vad.h"
#include "vad_docopt.h"
#include "pav_analysis.h"

#define DEBUG_VAD 0x1

int main(int argc, char *argv[]) {
  int verbose = 0; /* To show internal state of vad: verbose = DEBUG_VAD; */

  SNDFILE *sndfile_in, *sndfile_out = 0;
  SF_INFO sf_info;
  FILE *vadfile;
  int n_read = 0, i;
  int frame_num;

  VAD_DATA *vad_data;
  VAD_STATE state, last_state, last_printed_state;

  float *buffer, *buffer_zeros, *buffer_out;
  int frame_size;         /* in samples */
  float frame_duration;   /* in seconds */
  unsigned int t, last_t, start_t; /* in frames */

  char	*input_wav, *output_vad, *output_wav;

  DocoptArgs args = docopt(argc, argv, /* help */ 1, /* version */ "2.0");

  verbose    = args.verbose ? DEBUG_VAD : 0;
  input_wav  = args.input_wav;
  output_vad = args.output_vad;
  output_wav = args.output_wav;

  if (input_wav == 0 || output_vad == 0) {
    fprintf(stderr, "%s\n", args.usage_pattern);
    return -1;
  }

  /* Open input sound file */
  if ((sndfile_in = sf_open(input_wav, SFM_READ, &sf_info)) == 0) {
    fprintf(stderr, "Error opening input file %s (%s)\n", input_wav, strerror(errno));
    return -1;
  }

  if (sf_info.channels != 1) {
    fprintf(stderr, "Error: the input file has to be mono: %s\n", input_wav);
    return -2;
  }

  /* Open vad file */
  if ((vadfile = fopen(output_vad, "wt")) == 0) {
    fprintf(stderr, "Error opening output vad file %s (%s)\n", output_vad, strerror(errno));
    return -1;
  }

  /* Open output sound file, with same format, channels, etc. than input */
  if (output_wav) {
    if ((sndfile_out = sf_open(output_wav, SFM_WRITE, &sf_info)) == 0) {
      fprintf(stderr, "Error opening output wav file %s (%s)\n", output_wav, strerror(errno));
      return -1;
    }
  }

  if(argc <= 7) 
    vad_data = vad_open_(sf_info.samplerate);
  else vad_data = vad_open(sf_info.samplerate,  args.alpha1, args.alpha2, args.max_frames_maybe_silence, 
  args.max_frames_maybe_voice, args.zeros, args.min_frames_silence, args.min_frames_voice);

  
  /* Allocate memory for buffers */
  frame_size   = vad_frame_size(vad_data);
  buffer       = (float *) malloc(frame_size * sizeof(float));
  buffer_zeros = (float *) malloc(frame_size * sizeof(float));
  buffer_out   = (float *) malloc(frame_size * sizeof(float)*1e3);
  for (int j = 0; j< frame_size; ++j) buffer_zeros[j] = 0.0F;

//Meu main
//***********

  frame_duration = (float) frame_size/ (float) sf_info.samplerate;
  last_state = ST_UNDEF;

  frame_num = 0;
  for (t = last_t = 0; ; t++) { // For each frame ... 
    // End loop when file has finished (or there is an error)

    if ((n_read = sf_read_float(sndfile_in, buffer, frame_size)) != frame_size) break;
    if (sndfile_out != 0) memcpy(buffer_out + frame_num*frame_size,buffer,frame_size * sizeof(float));

    state = vad(vad_data, buffer);
    if (verbose & DEBUG_VAD) vad_show_state(vad_data, stdout);

    if (state != last_state) {
      if (t != last_t) {
        if((last_state == ST_SILENCE || last_state == ST_VOICE) && (state == ST_MAYBE_SILENCE || state == ST_MAYBE_VOICE)) {
          start_t = t;
        } else {
          if(last_state == ST_MAYBE_VOICE && state == ST_VOICE) {
            
            fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, start_t * frame_duration, state2str(ST_SILENCE));

            for(int i = 0; i< frame_num; i++) {
                if (sndfile_out != 0) sf_write_float(sndfile_out,buffer_zeros,frame_size);
            }
            frame_num = 0;
            
            last_t = start_t;
          }
          
          if(last_state == ST_MAYBE_SILENCE && state == ST_SILENCE){
            fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, start_t * frame_duration, state2str(ST_VOICE));
            if (sndfile_out != 0) {
              sf_write_float(sndfile_out,buffer_out,frame_size*frame_num);
              for (i=0; i < sizeof(buffer_out); i++) buffer_out[i] = 0.0F;
            }

            frame_num = 0;
            last_t = start_t;
          }
        }
      }
      last_state = state;
    }
    frame_num++;
  }



  state = vad_close(vad_data);
  
  if (t != last_t) {
    if(state == ST_SILENCE) {
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(ST_SILENCE));
      for(int i = 0; i< frame_num; i++) {
        if (sndfile_out != 0) sf_write_float(sndfile_out,buffer_zeros,frame_size);
      }
    } else {
      fprintf(vadfile, "%.5f\t%.5f\t%s\n", last_t * frame_duration, t * frame_duration + n_read / (float) sf_info.samplerate, state2str(ST_VOICE));
      if (sndfile_out != 0) sf_write_float(sndfile_out,buffer_out,frame_size*frame_num);
    }
  }

  free(buffer);
  free(buffer_zeros);
  free(buffer_out);
  sf_close(sndfile_in);
  fclose(vadfile);
  if (sndfile_out) sf_close(sndfile_out);
  return 0;
}
