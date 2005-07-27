/**
 * basicplay.c
 *
 * Copyright (C) 2004 Evan A. Sultanik
 * http://www.sultanik.com/
 *
 * The writeWave() function was modified from code originally written
 * by Paul Bourke and is Copyright (C) 2001 by him.
 * http://astronomy.swin.edu.au/~pbourke/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define VERSION "1.1 2005-07-27"

#define ABS(val) ((val < 0) ? (val * -1.0) : val)

#define PI 3.14159265358979

#define MAX_NUMBER_SEQUENCE_LENGTH 5

#define SOUND_HERTZ_LOWEST              32    /* lowest value BASIC allows for the hertz argument of the SOUND statement */
#define SOUND_HERTZ_HIGHEST             32767 /* highest value BASIC allows for the hertz argument of the SOUND statement */
#define SOUND_DURATION_TICKS_PER_SECOND 18.2  /* Ticks per second for the duration of the BASIC sound statement */
#define SOUND_DURATION(SECONDS)         (SECONDS * SOUND_DURATION_TICKS_PER_SECOND)

#define CONVERSION_NOT_SELECTED 0
#define CONVERT_TO_WAVE         1
#define CONVERT_TO_IC           2
#define CONVERT_TO_BAS          4

#define CODE_ERROR          0
#define CODE_DURATION       1   /*          1 */
#define CODE_OCTAVE         2   /*         10 */
#define CODE_MUSIC          4   /*        100 */
#define CODE_MUSIC_NORMAL   12  /*       1100 */
#define CODE_MUSIC_LEGATO   20  /*      10100 */
#define CODE_MUSIC_STACCATO 36  /*     100100 */
#define CODE_PAUSE          64  /*    1000000 */
#define CODE_L4_PER_MINUTE  128 /*   10000000 */
#define CODE_NOTE           256 /*  100000000 */
#define CODE_DOTTED_NOTE    512 /* 1000000000 */

#define NOTE_FLAT    1
#define NOTE_SHARP   2
#define NOTE_A       4
#define NOTE_B       8
#define NOTE_C       16
#define NOTE_D       32
#define NOTE_E       64
#define NOTE_F       128
#define NOTE_G       256

#define A(octave) 440.0 * pow(2, octave - 3)
#define B(octave) 493.88 * pow(2, octave - 3)
#define C(octave) 261.63 * pow(2, octave - 3)
#define D(octave) 293.66 * pow(2, octave - 3)
#define E(octave) 329.63 * pow(2, octave - 3)
#define F(octave) 349.23 * pow(2, octave - 3)
#define G(octave) 392.0 * pow(2, octave - 3)

#define AFLAT(octave) 415.30 * pow(2, octave - 3)
#define BFLAT(octave) 466.16 * pow(2, octave - 3)
#define ASHARP(octave) BFLAT(octave)
#define BSHARP(octave) C(octave)
#define CFLAT(octave) B(octave)
#define CSHARP(octave) 277.18 * pow(2, octave - 3)
#define DFLAT(octave) CSHARP(octave)
#define EFLAT(octave) 311.13 * pow(2, octave - 3)
#define DSHARP(octave) EFLAT(octave)
#define ESHARP(octave) F(octave)
#define FFLAT(octave) E(octave)
#define FSHARP(octave) 369.99 * pow(2, octave - 3)
#define GFLAT(octave) FSHARP(octave)
#define GSHARP(octave) AFLAT(octave)

const unsigned int power_of_ten[5] = { 1, 10, 100, 1000, 10000 };

#define CEILING(x) (unsigned long)(x + 1.0)

typedef struct tagNote
{
  int code;
  int value;
  struct tagNote* next;
  struct tagNote* last;
} Note;

typedef struct tagFrequency
{
  double hertz;
  double duration;
  struct tagFrequency* next;
} Frequency;

inline void logMessage(char* format, ...)
{
  va_list va_alist = {0};

  va_start(va_alist, format);

  vfprintf(stderr, format, va_alist);

  va_end( va_alist );
}

/**
 * Writes the specified samples to a WAVE sound file.  The WAVE file
 * is set up with one 16-bit channel.  Endian independent.
 *
 * fptr     - pointer to the file to which to write
 * samples  - array of sample values
 * nsamples - number of samples
 * nfreq    - sample frequency
 *
 * This function was modified from code originally written by Paul
 * Bourke.  Permission was granted by Bourke to include this code, and
 * for it to be licensed under the GPL.  The GPL allows for this code
 * to be used in any other program licensed under the GPL.  However, I
 * urge anyone interested in using or modifying this code in another
 * project to inform Mr. Bourke out of courtesy.  His contact
 * information is available on the following website:
 *
 * http://astronomy.swin.edu.au/~pbourke/
 */
void writeWave(FILE *fptr, double *samples, long nsamples, int nfreq)
{
   unsigned short v;
   int i;
   unsigned long totalsize, bytespersec;
   double themin, themax, scale, themid;

   /* Write the form chunk */
   fprintf(fptr, "RIFF");
   totalsize = 2 * nsamples + 36;
   fputc((totalsize & 0x000000ff), fptr);       /* File size */
   fputc((totalsize & 0x0000ff00) >> 8, fptr);
   fputc((totalsize & 0x00ff0000) >> 16, fptr);
   fputc((totalsize & 0xff000000) >> 24, fptr);
   fprintf(fptr, "WAVE");
   fprintf(fptr, "fmt ");                       /* fmt_ chunk */
   fputc(16, fptr);                             /* Chunk size */
   fputc(0, fptr);
   fputc(0, fptr);
   fputc(0, fptr);
   fputc(1, fptr);                              /* Format tag - uncompressed */
   fputc(0, fptr);
   fputc(1, fptr);                              /* Channels */
   fputc(0, fptr);
   fputc((nfreq & 0x000000ff), fptr);           /* Sample frequency (Hz) */
   fputc((nfreq & 0x0000ff00) >> 8, fptr);
   fputc((nfreq & 0x00ff0000) >> 16, fptr);
   fputc((nfreq & 0xff000000) >> 24, fptr);
   bytespersec = 2 * nfreq;
   fputc((bytespersec & 0x000000ff), fptr);     /* Average bytes per second */
   fputc((bytespersec & 0x0000ff00) >> 8, fptr);
   fputc((bytespersec & 0x00ff0000) >> 16, fptr);
   fputc((bytespersec & 0xff000000) >> 24, fptr);
   fputc(2, fptr);                              /* Block alignment */
   fputc(0, fptr);
   fputc(16, fptr);                             /* Bits per sample */
   fputc(0,fptr);
   fprintf(fptr, "data");
   totalsize = 2 * nsamples;
   fputc((totalsize & 0x000000ff), fptr);       /* Data size */
   fputc((totalsize & 0x0000ff00) >> 8, fptr);
   fputc((totalsize & 0x00ff0000) >> 16, fptr);
   fputc((totalsize & 0xff000000) >> 24, fptr);

   /* Find the range */
   themin = samples[0];
   themax = themin;
   for (i=1;i<nsamples;i++) {
      if (samples[i] > themax)
         themax = samples[i];
      if (samples[i] < themin)
         themin = samples[i];
   }
   if (themin >= themax) {
      themin -= 1;
      themax += 1;
   }
   themid = (themin + themax) / 2;
   themin -= themid;
   themax -= themid;
   if (ABS(themin) > ABS(themax))
      themax = ABS(themin);
   scale = 32760 / (themax);

   /* Write the data */
   for (i=0;i<nsamples;i++) {
      v = (unsigned short)(scale * (samples[i] - themid));
      fputc((v & 0x00ff),fptr);
      fputc((v & 0xff00) >> 8,fptr);
   }
}

/**
 * Deletes a notes linked list
 */
void freeNotes(Note* starting_note)
{
  if(starting_note == NULL)
    return;
  if(starting_note->next != NULL)
    freeNotes(starting_note->next);
  free(starting_note);
}

/**
 * Deletes a frequency linked list
 */
void freeFrequencies(Frequency* starting_frequency)
{
  if(starting_frequency == NULL)
    return;
  if(starting_frequency->next != NULL)
    freeFrequencies(starting_frequency->next);
  free(starting_frequency);
}

/**
 * Internal function used to add a new frequency to a frequency linked
 * list
 */
void addFrequency(double hertz, double duration, Frequency** current_frequency, Frequency** last_frequency) {
  if(*current_frequency == NULL) {
    *current_frequency = (Frequency*)malloc(sizeof(Frequency));
  }
  
  (*current_frequency)->hertz = hertz;
  (*current_frequency)->duration = duration;
  (*current_frequency)->next = NULL;
  if(*last_frequency != NULL)
    (*last_frequency)->next = *current_frequency;
  
  *last_frequency = *current_frequency;
  *current_frequency = NULL;
}

/**
 * Converts a linked list of notes to a linked list of frequencies.
 *
 * starting_note - pointer to the first note in the notes lined list
 * head          - pointer to what is to be the head of the new frequencies
 *                 linked list.  Note that the value of the head will be set;
 *                 any value it currently has will be overwritten.
 */
double notesToFrequency(Note* starting_note, Frequency* head)
{
  short octave = 0;
  double duration = 4;
  double tmp_duration, adjusted_duration;
  double l4_per_minute = 120;
  int music_code = CODE_MUSIC_NORMAL;
  Note* current_note = starting_note;
  double total_duration = 0;
  Frequency* last_frequency = NULL;
  Frequency* current_frequency = head;
  double hertz = 0;

  if(starting_note == NULL || head == NULL)
    return 0;

  while(current_note != NULL) {
    if(current_note->code & CODE_NOTE) {
      if(current_note->value & NOTE_A) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = ASHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = AFLAT(octave);
	}
	else
	  hertz = A(octave);
      }
      if(current_note->value & NOTE_B) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = BSHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = BFLAT(octave);
	}
	else
	  hertz = B(octave);
      }
      if(current_note->value & NOTE_C) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = CSHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = CFLAT(octave);
	}
	else
	  hertz = C(octave);
      }
      if(current_note->value & NOTE_D) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = DSHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = DFLAT(octave);
	}
	else
	  hertz = D(octave);
      }
      if(current_note->value & NOTE_E) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = ESHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = EFLAT(octave);
	}
	else
	  hertz = E(octave);
      }
      if(current_note->value & NOTE_F) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = FSHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = FFLAT(octave);
	}
	else
	  hertz = F(octave);
      }
      if(current_note->value & NOTE_G) {
	if(current_note->value & NOTE_SHARP) {
	  hertz = GSHARP(octave);
	}
	else if(current_note->value & NOTE_FLAT) {
	  hertz = GFLAT(octave);
	}
	else
	  hertz = G(octave);
      }

      tmp_duration = 1.0 / duration * 60.0 / l4_per_minute;
      if(current_note->code & CODE_DOTTED_NOTE) {
	tmp_duration = tmp_duration * 3.0 / 2.0;
      }
      adjusted_duration = tmp_duration;
      if(music_code == CODE_MUSIC_NORMAL) {
	adjusted_duration = tmp_duration * 7.0 / 8.0;
      }
      else if(music_code == CODE_MUSIC_STACCATO) {
	adjusted_duration = tmp_duration * 3.0 / 4.0;
      }
      addFrequency(hertz, adjusted_duration, &current_frequency, &last_frequency);
      total_duration += adjusted_duration;
      if(music_code == CODE_MUSIC_NORMAL) {
	addFrequency(0, tmp_duration / 8.0, &current_frequency, &last_frequency);
	total_duration += tmp_duration / 8.0;
      }
      else if(music_code == CODE_MUSIC_STACCATO) {
	addFrequency(0, tmp_duration / 4.0, &current_frequency, &last_frequency);
	total_duration += tmp_duration / 4.0;
      }
    }
    else if(current_note->code & CODE_L4_PER_MINUTE) {
      l4_per_minute = current_note->value;
      if(l4_per_minute < 32) {
	logMessage("WARNING: Quarter notes per minute set to %.4f; minimum is 32!\n", l4_per_minute);
	l4_per_minute = 32;
      }
      else if(l4_per_minute > 255) {
	logMessage("WARNING: Quarter notes per minute set to %.4f; maximum is 255!\n", l4_per_minute);
	l4_per_minute = 255;
      }
    }
    else if(current_note->code & CODE_DURATION) {
      duration = current_note->value;
    }
    else if(current_note->code & CODE_OCTAVE) {
      octave = current_note->value;
      if(octave < 0) {
	logMessage("WARNING: Octave set at %d; minimum is 0!\n", octave);
	octave = 0;
      }
      else if(octave > 6) {
	logMessage("WARNING: Octave set at %d; maximum is 6!\n", octave);
	octave = 6;
      }
    }
    else if(current_note->code & CODE_PAUSE) {
      tmp_duration = 1.0 / current_note->value * l4_per_minute / 60.0;
      if(current_note->code & CODE_DOTTED_NOTE) {
	tmp_duration = tmp_duration * 3.0 / 2.0;
      }
      total_duration += tmp_duration;
      addFrequency(0, tmp_duration, &current_frequency, &last_frequency);
    }
    else if(current_note->code & CODE_MUSIC) {
      music_code = current_note->code;
    }
    current_note = current_note->next;
  }

  return total_duration;
}

/**
 * Prints a syntax error message
 */
void syntaxError(char* comment, char* play, unsigned int play_length, unsigned int offset)
{
  char buffer[81];
  short leftover = 0;
  short rightover = 0;
  unsigned int i;
  if(play_length > 80) {
    if(offset < 40) {
      rightover = 1;
    }
    else if(offset > play_length - 40) {
      play += play_length - 80;
      offset -= play_length - 80;
      leftover = 1;
    }
    else {
      play += offset - 40;
      offset = 40;
      leftover = 1;
      rightover = 1;
    }

    play_length = 80;
  }
  strncpy(buffer, play, play_length);
  buffer[play_length] = '\0';
  if(leftover) {
    buffer[0] = '.';
    buffer[1] = '.';
    buffer[2] = '.';
  }
  if(rightover) {
    buffer[79] = '.';
    buffer[78] = '.';
    buffer[77] = '.';
  }

  logMessage("Syntax Error: %s\n\"%s\"\n", comment, buffer);

  for(i=0; i<=offset; i++) {
    logMessage(" ");
  }
  logMessage("^\n");
}

/**
 * Internal function to add a note to the end of a notes linked list.
 */
void addNote(int code, int value, Note** current_note, Note** last_note) {
  if(*current_note == NULL) {
    *current_note = (Note*)malloc(sizeof(Note));
  }
  
  (*current_note)->code = code;
  (*current_note)->value = value;
  (*current_note)->last = *last_note;
  (*current_note)->next = NULL;
  if(*last_note != NULL)
    (*last_note)->next = *current_note;
  
  *last_note = *current_note;
  *current_note = NULL;
}

unsigned long parsePlayStatement(char* play, unsigned int play_length, Note* first_note)
{
  unsigned long num_notes = 0;
  int code = CODE_ERROR;
  int value = 0;
  int lastvalue;
  short last_octave = 0;
  short last_duration = 4;
  short code_set = 0;
  unsigned int i, j;
  Note* last_note = NULL;
  Note* current_note = first_note;
  char curr_char, next_char, nextnext_char;
  short number_sequence_length = 0;
  int number_sequence[MAX_NUMBER_SEQUENCE_LENGTH];

  if(first_note != NULL)
    first_note->code = CODE_ERROR;

  for(i=0; i<play_length; i++) {
    curr_char = play[i];
    if(i < play_length - 1) {
      next_char = play[i+1];
    }
    else {
      next_char = '\0';
    }
    if(i < play_length - 2) {
      nextnext_char = play[i+2];
    }
    else {
      nextnext_char = '\0';
    }
    if(curr_char == 'L' || curr_char == 'l' || 
       curr_char == 'O' || curr_char == 'o' ||
       curr_char == 'P' || curr_char == 'p' ||
       curr_char == 'T' || curr_char == 't' ||
       curr_char == '<' || curr_char == '>'
       ) {
      if(!code_set) {
	/**
	 * We haven't read the code yet;
	 */
	switch(curr_char) {
	case 'L':
	case 'l':
	  code = CODE_DURATION;
	  break;
	case 'O':
	case 'o':
	  code = CODE_OCTAVE;
	  break;
	case 'P':
	case 'p':
	  code = CODE_PAUSE;
	  break;
	case 'T':
	case 't':
	  code = CODE_L4_PER_MINUTE;
	  break;
	case '>':
	  addNote(CODE_OCTAVE, ++last_octave, &current_note, &last_note);
	  break;
	case '<':
	  addNote(CODE_OCTAVE, --last_octave, &current_note, &last_note);
	  break;
	}
	if(curr_char != '>' && curr_char != '<')
	  code_set = 1;
      }
      else {
	syntaxError("Command not expected:", play, play_length, i);
      }
    }
    else if(curr_char == 'm' || curr_char == 'M') {
      i++;
      switch(next_char) {
      case 'b':
      case 'B':
      case 'f':
      case 'F':
	/**
	 * Music Background/Foreground (Ignore this)
	 */
	continue;
	break;
      case 'n':
      case 'N':
	addNote(CODE_MUSIC_NORMAL, 0, &current_note, &last_note);
	break;
      case 'l':
      case 'L':
	addNote(CODE_MUSIC_LEGATO, 0, &current_note, &last_note);
	break;
      case 's':
      case 'S':
	addNote(CODE_MUSIC_STACCATO, 0, &current_note, &last_note);
	break;
      }
    }
    else if((curr_char >= 'A' && curr_char <= 'G') ||
	    (curr_char >= 'a' && curr_char <= 'g')) {
      /**
       * This is a note!
       */

      if(code_set) {
	syntaxError("Value expected here:", play, play_length, i);
	continue;
      }

      code = CODE_NOTE;

      switch(curr_char) {
      case 'A':
      case 'a':
	value = NOTE_A;
	break;
      case 'B':
      case 'b':
	value = NOTE_B;
	break;
      case 'C':
      case 'c':
	value = NOTE_C;
	break;
      case 'D':
      case 'd':
	value = NOTE_D;
	break;
      case 'E':
      case 'e':
	value = NOTE_E;
	break;
      case 'F':
      case 'f':
	value = NOTE_F;
	break;
      case 'G':
      case 'g':
	value = NOTE_G;
	break;
      }

      if(next_char == '#' || next_char == '+') {
	value = value & NOTE_SHARP;
	next_char = nextnext_char;
	i++;
      }
      else if(next_char == '-') {
	value = value & NOTE_FLAT;
	next_char = nextnext_char;
	i++;
      }

      if(next_char == '.') {
	code = code & CODE_DOTTED_NOTE;
	next_char = nextnext_char;
	i++;
      }

      if(next_char >= '0' && next_char <= '9') {
	/**
	 * This is the duration of the note.  The note will actually
	 * be added after we are done parsing the duration.
	 */
	code_set = 1;
      }
      else {
	addNote(code, value, &current_note, &last_note);
	code_set = 0;
      }

      num_notes++;
    }
    else if(curr_char >= '0' && curr_char <= '9') {
      if(!code_set) {
	syntaxError("Number not expected:", play, play_length, i);
	continue;
      }
      if(number_sequence_length <= 0) {
	number_sequence_length = 1;
	number_sequence[0] = curr_char - '0';
      }
      else if(number_sequence_length >= MAX_NUMBER_SEQUENCE_LENGTH) {
	syntaxError("Numbers too big (number will be trunctuated):", play, play_length, i);
      }
      else {
	number_sequence[number_sequence_length++] = curr_char - '0';
      }

      if(next_char >= '0' && next_char <= '9')
	continue;

      lastvalue = value;
      value = 0;
      for(j=0; j<number_sequence_length; j++)
	value += power_of_ten[number_sequence_length - j - 1] * number_sequence[j];

      if(code == CODE_PAUSE || code & CODE_PAUSE)
	num_notes++;
      else if(code & CODE_OCTAVE) {
	last_octave = value;
      }
      else if(code & CODE_DURATION) {
	last_duration = value;
      }

      if(code & CODE_NOTE) {
	/**
	 * This note has a duration in and of itself.
	 */
	addNote(CODE_DURATION, value, &current_note, &last_note);
	addNote(code, lastvalue, &current_note, &last_note);
	addNote(CODE_DURATION, last_duration, &current_note, &last_note);
      }
      else {
	addNote(code, value, &current_note, &last_note);
      }
      code_set = 0;
      number_sequence_length = 0;
    }
    else if(curr_char == ' ') {

    }
    else {
      syntaxError("Symbol not expected:", play, play_length, i);
    }
  }

  return num_notes;
}

unsigned long addSound(double* data, unsigned long offset, double duration, double frequency, unsigned int wave_frequency)
{
  unsigned long i;
  unsigned long iterations = (int)((double)wave_frequency*duration);
  for(i=0; i<iterations; i++) {
    data[i + offset] = 32767.0 * sin(2.0 * PI * ((double)(i))*frequency/44100.0);
  }
  return offset + iterations;
}

void writeIC(FILE* file, Frequency* frequency)
{
  Frequency* current = frequency;

  fprintf(file, "/**\n * BASIC -> IC Play Statement Conversion\n * Using a Converter Written by Evan A. Sultanik\n * http://www.sultanik.com/\n */\n\n");
  fprintf(file, "int main()\n{\n");

  while(current != NULL) {
    if(current->duration > 0) {
      if(current->hertz <= 0)
	fprintf(file, "\tmsleep(%ldL);\n", (long)(current->duration*1000.0));
      else
	fprintf(file, "\ttone(%.4f, %.4f);\n", current->hertz, current->duration);
    }
    current = current->next;
  }

  fprintf(file, "\treturn 1;\n}\n");
}

void writeBAS(FILE* file, Frequency* frequency)
{
  Frequency* current = frequency;

  fprintf(file, "REM PLAY -> SOUND Statement Conversion\nREM Using a Converter Written by Evan A. Sultanik\nREM http://www.sultanik.com/\n\n");

  while(current != NULL) {
    if(current->duration > 0) {
      if(current->hertz < SOUND_HERTZ_LOWEST || current->hertz > SOUND_HERTZ_HIGHEST)
	fprintf(file, "Seconds = TIMER + %.4f\nDO\nLOOP WHILE TIMER <= Seconds\n", current->duration);
      else
	fprintf(file, "SOUND(%d, %.4f);\n", (int)current->hertz, SOUND_DURATION(current->duration));
    }
    current = current->next;
  }
}

char* getFileSuffix(char* string)
{
  int i, last_period = -1;
  if(string == NULL)
    return NULL;
  for(i=0; string[i] != '\0'; i++) {
    if(string[i] == '.')
      last_period = i;
  }
  if(last_period < 0 || last_period >= i - 1)
    return NULL;
  return string + last_period + 1;
}

void readFile(FILE* file, char** string)
{
  char buffer[255];
  char* temp;
  int num_read;
  int length = 0;

  *string = NULL;

  while((num_read = fread(buffer, sizeof(char), 255, file))) {
    length += num_read;
    temp = *string;
    *string = (char*)malloc(sizeof(char) * (length + 1));
    if(temp != NULL) {
      strcpy(*string, temp);
      strncpy(*string + strlen(temp), buffer, num_read);
      (*string + strlen(temp))[num_read] = '\0';
      free(temp);
    }
    else {
      strncpy(*string, buffer, num_read);
      (*string)[num_read] = '\0';
    }
  }
  if(*string == NULL) {
    *string = (char*)malloc(sizeof(char) * 1);
    (*string)[0] = '\0';
  }
}

int fileExists(char* filename)
{
  FILE* file = NULL;
  
  if(filename == NULL)
    return 0;

  file = fopen(filename, "rb");

  if(file == NULL)
    return 0;

  fclose(file);

  return 1;
}

int main(const int argc, char** argv)
{
  FILE* file = NULL;
  double* data;
  unsigned long num_notes;
  double total_duration, offset = 0;
  Frequency* first_frequency;
  Frequency* current_frequency;
  Note* head;
  short print_usage = 0;
  int i;
  char* input_string = NULL;
  int conversion_mode = CONVERSION_NOT_SELECTED;
  int force = 0;
  char* input_file = NULL;
  char* output_file = NULL;
  int use_stdout = 0;

  /**
   * Read the command line arguments
   */
  if(argc <= 1) {
    print_usage = 3;
  }
  for(i=1; i<argc; i++) {
    if(strncmp(argv[i], "-e", 2) == 0) {
      if(strcmp(argv[i], "-e") == 0) {
	if(argc - 1 == i) {
	  logMessage("Error: PLAY statement expected after -e option!\n\n");
	  print_usage = 1;
	  break;
	}
	/* The next argument should be the play statement */
	input_string = (char*)malloc(sizeof(char) * (strlen(argv[i+1]) + 1));
	strcpy(input_string, argv[i++]);
      }
      else {
	if(strlen(argv[i]) <= 2) {
	  logMessage("Error: PLAY statement expected after -e option!\n\n");
	  print_usage = 1;
	  break;
	}
	input_string = (char*)malloc(sizeof(char) * (strlen(argv[i]) - 1));
	strcpy(input_string, argv[i]+2);
      }
    }
    else if(strcmp(argv[i], "-?")     == 0 ||
	    strcmp(argv[i], "--help") == 0 ||
	    strcmp(argv[i], "-h")     == 0) {
      print_usage = 1;
      break;
    }
    else if(strcmp(argv[i], "-c") == 0) {
      use_stdout = 1;
    }
    else if(strcmp(argv[i], "-wav")  == 0) {
      conversion_mode = CONVERT_TO_WAVE;
    }
    else if(strcmp(argv[i], "-ic") == 0) {
      conversion_mode = CONVERT_TO_IC;
    }
    else if(strcmp(argv[i], "-bas") == 0) {
      conversion_mode = CONVERT_TO_BAS;
    }
    else if(strcmp(argv[i], "-f") == 0) {
      force = 1;
    }
    else if(strncmp(argv[i], "-", 1) == 0) {
      logMessage("Error: unknown option '%s'!\n\n", argv[i]);
    }
    else {
      if(input_file == NULL && input_string == NULL) {
	input_file = argv[i];
      }
      else if((input_file != NULL || input_string != NULL) && output_file == NULL) {
	output_file = argv[i];
      }
      else {
	logMessage("Error: unknown option '%s'!\n\n", argv[i]);
      }
    }
  }
  if(!print_usage) {
    if((input_file == NULL && input_string == NULL)) {
      logMessage("Error: input PLAY statement not provided!\n\n");
      print_usage = 1;
    }
    if(output_file == NULL && !use_stdout) {
      logMessage("Error: output filename not provided!\n\n");
      print_usage = 1;
    }
  }
  if(!print_usage && conversion_mode == CONVERSION_NOT_SELECTED) {
    if(use_stdout) {
      print_usage = 1;
      logMessage("Error: you must specify the type of conversion when outputting to STDOUT\n\n");
    }
    else {
      int error = 0;
      char* suffix = getFileSuffix(output_file);
      if(suffix == NULL) {
	error = 1;
      }
      else if((strcasecmp("c", suffix) == 0) || 
	      (strcasecmp("ic", suffix) == 0) ||
	      (strcasecmp("cc", suffix) == 0)) {
	conversion_mode = CONVERT_TO_IC;
      }
      else if((strcasecmp("wav", suffix) == 0) ||
	      (strcasecmp("wave", suffix) == 0)) {
      conversion_mode = CONVERT_TO_WAVE;
      }
      else if((strcasecmp("bas", suffix) == 0) ||
	      (strcasecmp("basic", suffix) == 0)) {
	conversion_mode = CONVERT_TO_BAS;
      }
      else {
	error = 1;
      }
      
      if(error) {
	print_usage = 1;
	logMessage("Error: filetype '%s' unknown; please specify the type of conversion\n\n", suffix);
      }
    }
  }
  if(!print_usage && !force && !use_stdout && fileExists(output_file)) {
    print_usage = 1;
    logMessage("Error: file '%s' is in the way!  Use '-f' option to force overwrite.\n\n", output_file);
  }
  if(print_usage) {
    logMessage("Version: BasicPlay %s\n", VERSION);
    logMessage("Copyright: Copyright (C) 2004 Evan Sultanik\n");
    logMessage("http://www.sultanik.com/\n\n");
    logMessage("Usage: basicplay [options ...] [file | -e 'statement'] [options ...] [file | -c]\n\n");
    logMessage("Where options include:\n");
    logMessage("  -wav render the input PLAY statement to a WAVE sound file\n");
    logMessage("  -ic  convert the input PLAY statement to Interactive C code\n");
    logMessage("       that will play the music on a device such as a Handyboard\n");
    logMessage("  -bas convert the input PLAY statement to BASIC code,\n");
    logMessage("       using the SOUND statement instead of PLAY\n");
    logMessage("  -e   used in place of an input file, this option must be followed by a string\n");
    logMessage("       containing the PLAY statement to be converted\n");
    logMessage("  -c   output to STDOUT instead of a file\n");
    logMessage("  -f   force an overwrite of the output file, even if it already exists\n");
    logMessage("\nIf neither -wav, -bas, nor -ic options are given, BasicPlay will determine the\n");
    logMessage("conversion by the output file suffix.  For example, *.wav[e] will result in a\n");
    logMessage("WAVE file, *.[i]c will result in an Interactive C file, and *.bas[ic] will\n");
    logMessage("result in a BASIC file.  If the output file suffix does not correspond to a\n");
    logMessage("file format, an error will be thrown and the conversion will not occur.\n");
    return -1;
  }

  if(input_string == NULL) {
    file = fopen(input_file, "rb");
    if(file == NULL) {
      logMessage("Error: could not open %s for reading!\n", input_file);
      return -2;
    }
    readFile(file, &input_string);
    fclose(file);
  }

  head = (Note*)malloc(sizeof(Note));
  first_frequency = (Frequency*)malloc(sizeof(Frequency));

  num_notes = parsePlayStatement(input_string, strlen(input_string), head);

  total_duration = notesToFrequency(head, first_frequency);

  freeNotes(head);

  data = (double*)malloc(sizeof(double) * CEILING(total_duration * 44100.0));

  if(data == NULL) {
    logMessage("ERROR: Could not allocate enough memory!\n");
    return -3;
  }

  current_frequency = first_frequency;

  while(current_frequency != NULL) {
    offset = addSound(data, offset, current_frequency->duration, current_frequency->hertz, 44100);
    current_frequency = current_frequency->next;
  }

  if(use_stdout)
    file = stdout;
  else
    file = fopen(output_file, "wb");

  switch(conversion_mode) {
  case CONVERT_TO_IC:
    writeIC(file, first_frequency);
    break;
  case CONVERT_TO_BAS:
    writeBAS(file, first_frequency);
    break;
  case CONVERT_TO_WAVE:
  default:
    writeWave(file, data, total_duration * 44100, 44100);
    break;    
  }

  if(!use_stdout)
    fclose(file);

  freeFrequencies(first_frequency);
  free(data);
  free(input_string);

  return 1;
}
