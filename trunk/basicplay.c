#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <string.h>

#define ABS(val) ((val < 0) ? (val * -1.0) : val)

#define PI 3.14159265358979

#define MAX_NUMBER_SEQUENCE_LENGTH 5

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
#define CFLAT(octave) BSHARP(octave)
#define CSHARP(octave) 277.18 * pow(2, octave - 3)
#define DFLAT(octave) CSHARP(octave)
#define EFLAT(octave) 311.13 * pow(2, octave - 3)
#define DSHARP(octave) EFLAT(octave)
#define ESHARP(octave) F(octave)
#define FFLAT(octave) ESHARP(octave)
#define FSHARP(octave) 369.99 * pow(2, octave - 3)
#define GFLAT(octave) FSHARP(octave)
#define GSHARP(octave) AFLAT(octave)

unsigned int power_of_ten[5] = { 1, 10, 100, 1000, 10000 };

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

/*
   Write an WAVE sound file
   Only do one channel, only support 16 bit.
   Supports any (reasonable) sample frequency
   Little/big endian independent!
*/
void writeWave(FILE *fptr, double *samples, long nsamples, int nfreq)
{
   unsigned short v;
   int i;
   unsigned long totalsize,bytespersec;
   double themin,themax,scale,themid;

   /* Write the form chunk */
   fprintf(fptr,"RIFF");
   totalsize = 2 * nsamples + 36;
   fputc((totalsize & 0x000000ff),fptr);        /* File size */
   fputc((totalsize & 0x0000ff00) >> 8,fptr);
   fputc((totalsize & 0x00ff0000) >> 16,fptr);
   fputc((totalsize & 0xff000000) >> 24,fptr);
   fprintf(fptr,"WAVE");
   fprintf(fptr,"fmt ");                        /* fmt_ chunk */
   fputc(16,fptr);                              /* Chunk size */
   fputc(0,fptr);
   fputc(0,fptr);
   fputc(0,fptr);
   fputc(1,fptr);                               /* Format tag - uncompressed */
   fputc(0,fptr);
   fputc(1,fptr);                               /* Channels */
   fputc(0,fptr);
   fputc((nfreq & 0x000000ff),fptr);            /* Sample frequency (Hz) */
   fputc((nfreq & 0x0000ff00) >> 8,fptr);
   fputc((nfreq & 0x00ff0000) >> 16,fptr);
   fputc((nfreq & 0xff000000) >> 24,fptr);
   bytespersec = 2 * nfreq;
   fputc((bytespersec & 0x000000ff),fptr);      /* Average bytes per second */
   fputc((bytespersec & 0x0000ff00) >> 8,fptr);
   fputc((bytespersec & 0x00ff0000) >> 16,fptr);
   fputc((bytespersec & 0xff000000) >> 24,fptr);
   fputc(2,fptr);                               /* Block alignment */
   fputc(0,fptr);
   fputc(16,fptr);                              /* Bits per sample */
   fputc(0,fptr);
   fprintf(fptr,"data");
   totalsize = 2 * nsamples;
   fputc((totalsize & 0x000000ff),fptr);        /* Data size */
   fputc((totalsize & 0x0000ff00) >> 8,fptr);
   fputc((totalsize & 0x00ff0000) >> 16,fptr);
   fputc((totalsize & 0xff000000) >> 24,fptr);

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

void freeNotes(Note* starting_note)
{
  if(starting_note == NULL)
    return;
  if(starting_note->next != NULL)
    freeNotes(starting_note->next);
  free(starting_note);
}

void freeFrequencies(Frequency* starting_frequency)
{
  if(starting_frequency == NULL)
    return;
  if(starting_frequency->next != NULL)
    freeFrequencies(starting_frequency->next);
  free(starting_frequency);
}

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
	printf("WARNING: Quarter notes per minute set to %.4f; minimum is 32!\n", l4_per_minute);
	l4_per_minute = 32;
      }
      else if(l4_per_minute > 255) {
	printf("WARNING: Quarter notes per minute set to %.4f; maximum is 255!\n", l4_per_minute);
	l4_per_minute = 255;
      }
    }
    else if(current_note->code & CODE_DURATION) {
      duration = current_note->value;
    }
    else if(current_note->code & CODE_OCTAVE) {
      octave = current_note->value;
      if(octave < 0) {
	printf("WARNING: Octave set at %d; minimum is 0!\n", octave);
	octave = 0;
      }
      else if(octave > 6) {
	printf("WARNING: Octave set at %d; maximum is 6!\n", octave);
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

  printf("Syntax Error: %s\n\"%s\"\n", comment, buffer);

  for(i=0; i<=offset; i++) {
    printf(" ");
  }
  printf("^\n");
}

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

  fprintf(file, "/**\n * BASIC -> IC Play Statement Conversion\n * Using a Converter Written by Evan A. Sultanik \n * http://www.sultanik.com/ \n */\n\n");
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

int main(const int argc, const char** argv)
{
  FILE* out = NULL;
  double* data;
  unsigned long num_notes;
  double total_duration, offset = 0;
  Frequency* first_frequency;
  Frequency* current_frequency;
  Note* head;

  head = (Note*)malloc(sizeof(Note));
  first_frequency = (Frequency*)malloc(sizeof(Frequency));

  char* notes = "t40 o4 c2 L4 eg<b.>l16cd 12c>a14g>c<gl16gfef12e<a8l16b>cdefgagfedc<bag8ab>cdefgfedc<bagf8gab>cdefedc<bagfe8fgab>cdedc<bagfed8efgab>c#d<ab>c#defgab>c<bagfefgagfedc<l8bms>gecmldgmsecd4g4<g2g2>c4e4g2l16agfefedcedededededededcdc4c<g>cegecefd<b>dc4c<g>cegecefd<b>dc4>c4c2";
  

  num_notes = parsePlayStatement(notes, strlen(notes), head);

  total_duration = notesToFrequency(head, first_frequency);

  freeNotes(head);

  data = (double*)malloc(sizeof(double) * CEILING(total_duration * 44100.0));

  if(data == NULL) {
    printf("ERROR: Could not allocate enough memory!\n");
    return -1;
  }

  current_frequency = first_frequency;

  while(current_frequency != NULL) {
    offset = addSound(data, offset, current_frequency->duration, current_frequency->hertz, 44100);
    current_frequency = current_frequency->next;
  }

  /*offset = addSound(data, offset, 1, C(octave), 44100);
  offset = addSound(data, offset, 1, D(octave), 44100);
  offset = addSound(data, offset, 1, E(octave), 44100);
  offset = addSound(data, offset, 1, F(octave), 44100);
  offset = addSound(data, offset, 1, G(octave), 44100);
  offset = addSound(data, offset, 1, A(octave + 1), 44100);
  offset = addSound(data, offset, 1, B(octave + 1), 44100);
  offset = addSound(data, offset, 1, C(octave + 1), 44100);*/

  out = fopen("t.wav", "wb");
  writeWave(out, data, total_duration * 44100, 44100);
  fclose(out);

  out = fopen("t.ic", "wb");
  writeIC(out, first_frequency);
  fclose(out);

  freeFrequencies(first_frequency);
  free(data);

  return 1;
}
