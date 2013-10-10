#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <endian.h>

typedef int8_t ID[4];
typedef long double extended;

const int8_t aiffMagicNumber[4] = {'F', 'O', 'R', 'M'};
const int8_t aiffFileType[4] = {'A', 'I', 'F', 'F'};

typedef struct {
    ID magicNumber;
    uint32_t size;
    ID fileType;
} FileHeader;

const int8_t commonChunkId[4] = {'C', 'O', 'M', 'M'};

typedef struct {
    ID chunkID;
    int32_t chunkSize;
    int16_t numChannels;
    uint32_t numSampleFrames;
    int16_t sampleSize;
    extended sampleRate;
} CommonChunk;

const int8_t soundDataChunkId[4] = {'S', 'S', 'N', 'D'};

typedef struct {
    ID chunkID;
    int32_t chunkSize;

    uint32_t offset;
    uint32_t blockSize;
    void* WaveformData;
}  SoundDataChunk;

uint32_t computeFileSize(int16_t numChannels,
                         uint32_t numSampleFrames,
                         int16_t sampleSize) {
    /* converting from bits to bytes */
    if (sampleSize <= 8) {
        sampleSize = 1;
    } else if (sampleSize <= 16) {
        sampleSize = 2;
    } else if (sampleSize <= 24) {
        sampleSize = 3;
    } else if (sampleSize <= 32) {
        sampleSize = 4;
    }
  
    return
        4 + /*fileType size*/
        26 + /*Common chunk size*/
        16 + /*SoundData chunk header size*/
        numChannels * numSampleFrames * sampleSize; /*sound data size*/
}

uint32_t computeSoundChunkSize(int16_t numChannels,
                               uint32_t numSampleFrames,
                               int16_t sampleSize) {
    /* converting from bits to bytes */
    if (sampleSize <= 8) {
        sampleSize = 1;
    } else if (sampleSize <= 16) {
        sampleSize = 2;
    } else if (sampleSize <= 24) {
        sampleSize = 3;
    } else if (sampleSize <= 32) {
        sampleSize = 4;
    }
 
    return 8 + numChannels * numSampleFrames * sampleSize;
}

void writeAiffHeader(FILE* outfile,
                     int16_t numChannels,
                     uint32_t numSampleFrames,
                     int16_t sampleSize,
                     extended sampleRate) {
    uint32_t filesize = computeFileSize(numChannels, numSampleFrames, sampleSize);
    uint32_t commonChunkSize = 18; /* fixed size */
    uint32_t soundDataChunkSize = computeSoundChunkSize(numChannels, numSampleFrames, sampleSize);
    uint32_t zero = 0;

    /* IFF file header */
    fwrite(aiffMagicNumber, 4, sizeof(int8_t), outfile);
    fwrite(&filesize, 1, sizeof(uint32_t), outfile);
    fwrite(aiffFileType, 4, sizeof(int8_t), outfile);

    /* Common Chunk */
    fwrite(commonChunkId, 4, sizeof(int8_t), outfile);
    fwrite(&commonChunkSize, 1, sizeof(commonChunkSize), outfile);
    fwrite(&numChannels, 1, sizeof(numChannels), outfile);
    fwrite(&numSampleFrames, 1, sizeof(numSampleFrames), outfile);
    fwrite(&sampleSize, 1, sizeof(sampleSize), outfile);

    /* Sound Data Chunk */
    fwrite(soundDataChunkId, 4, sizeof(int8_t), outfile);
    fwrite(&soundDataChunkSize, 1, sizeof(soundDataChunkSize), outfile);
    fwrite(&zero, 1, sizeof(zero), outfile);
    fwrite(&zero, 1, sizeof(zero), outfile);
}

const double pi = 3.141592653589793238462643383279502;

const int32_t sample_rate = 44100;

/* A table of frequencies indexed by piano keyboard pitches.
   Note the aliasing at the low pitches. Luckily, those are mostly below the
   normal human hearing range.
   0 is special, because that must disable playing. */
const int32_t freqtable[] = {0, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16,
                             17, 18, 19, 20, 21, 23, 24, 25, 27, 29, 30, 32,
                             34, 36, 38, 41, 43, 46, 48, 51, 55, 58, 61, 65,
                             69, 73, 77, 82, 87, 92, 97, 103, 110, 116, 123,
                             130, 138, 146, 155, 164, 174, 184, 195, 207, 220,
                             233, 246, 261, 277, 293, 311, 329, 349, 369, 391,
                             415, 440, 466, 493, 523, 554, 587, 622, 659, 698,
                             739, 783, 830, 880, 932, 987, 1046, 1108, 1174,
                             1244, 1318, 1396, 1479, 1567, 1661, 1760, 1864,
                             1975, 2093, 2217, 2349, 2489, 2637, 2793, 2959,
                             3135, 3322, 3520, 3729, 3951, 4186, 4434, 4698,
                             4978, 5274, 5587, 5919, 6271, 6644, 7040, 7458,
                             7902, 8372, 8869, 9397, 9956, 10548, 11175, 11839,
                             12543, 13289, 14080, 14917, 15804, 16744, 17739,
                             18794, 19912, 21096, 22350, 23679, 25087, 26579,
                             28160, 29834, 31608, 33488, 35479, 37589, 39824,
                             42192, 44701, 47359, 50175, 53159, 56320, 59668,
                             63217, 66976, 70958, 75178, 79648, 84384, 89402,
                             94718, 100350, 106318, 112640, 119337, 126434,
                             133952, 141917, 150356, 159297, 168769, 178804,
                             189437, 200701, 212636, 225280, 238675, 252868,
                             267904, 283835, 300712, 318594, 337538, 357609,
                             378874, 401403, 425272, 450560, 477351, 505736,
                             535809, 567670, 601425, 637188, 675077, 715219,
                             757748, 802806, 850544, 901120, 954703, 1011473,
                             1071618, 1135340, 1202850, 1274376, 1350154,
                             1430438, 1515497, 1605613, 1701088, 1802240,
                             1909406, 2022946, 2143236, 2270680, 2405701,
                             2548752, 2700308, 2860877, 3030994, 3211226,
                             3402176, 3604480, 3818813, 4045892, 4286473,
                             4541360, 4811403, 5097504, 5400617, 5721755,
                             6061988, 6422453, 6804352, 7208960, 7637627,
                             8091784, 8572946, 9082720, 9622807, 10195009,
                             10801235, 11443510, 12123977, 12844906, 13608704,
                             14417920, 15275254, 16183568, 17145893, 18165440,
                             19245614};

const int32_t gaintable[] = {0,
                             0x10000,
                             0x18000,
                             0x20000,
                             0x30000,
                             0x40000,
                             0x60000,
                             0x80000,
                             0xc0000,
                             0x100000,
                             0x180000,
                             0x200000,
                             0x300000,
                             0x400000,
                             0x600000,
                             0x800000,
                             0xc00000,
                             0x1000000,
                             0x1800000,
                             0x2000000,
                             0x3000000,
                             0x4000000,
                             0x6000000,
                             0x8000000,
                             0xc000000,
                             0x10000000,
                             0x18000000,
                             0x20000000,
                             0x30000000,
                             0x40000000,
                             0x60000000,
                             0x80000000,
                             0xc0000000};

typedef enum _oscillator_type {
    SQUARE,
    SAWTOOTH,
    TRIANGLE,
    SINE,
    FM
} oscillator_type;

typedef struct _oscillator {
    oscillator_type type;
    uint32_t phase;
    int32_t frequency;
} oscillator;

int32_t generate_next_osc_sample(oscillator* osc, int32_t gain) {
    osc->phase += ((UINT32_MAX / sample_rate) * osc->frequency);

    switch(osc->type) {
    case SQUARE:
        if(osc->phase > (UINT32_MAX / 2)) {
            return gain;
        } else {
            return -gain;
        }
        break;

    case SAWTOOTH:
        return (int32_t)round(((float)osc->phase / UINT32_MAX - 0.5) * gain);
        break;

    case TRIANGLE:
    {
        int32_t p = 0;
        if(osc->phase < (UINT32_MAX / 2)) {
            p = UINT32_MAX / 2 - osc->phase;      
        } else {
            p = osc->phase -  UINT32_MAX / 2;
        }
        return round(gain * ((float)p - INT32_MAX / 2) / (INT32_MAX / 2));
    }
    break;

    case SINE:
        return (int32_t)roundf(sinf(2 * pi * ((float)osc->phase / UINT32_MAX)) * gain);
        break;

    case FM:
        /* AHAHAHAHAHAHAH */
        return 0;
        break;

    default:
        return 0;
        break;
    };
}

typedef struct _note {
    uint8_t pitch;
    uint8_t gain;
} note;

typedef struct _instrument {
    oscillator_type type;
    note* notes; // size is section.num_notes
} instrument;

typedef struct _section {
    int16_t tempo; /* in beats per minute */
    int8_t num_instruments;
    int8_t num_notes;
    instrument* instruments;
} section;

typedef struct _composition {
    int32_t num_sections;
    section* sections;
} composition;

typedef struct _output_state {
    int8_t is_playing;
    int8_t note_num;
    int32_t sample_num;
    int8_t num_oscillators;
    oscillator* oscillators;
} output_state;

output_state* create_output_state(int8_t num_oscillators) {
    output_state* ret = calloc(sizeof(output_state), 1);
    ret->is_playing = 1;
    ret->num_oscillators = num_oscillators;
    ret->oscillators = calloc(sizeof(oscillator), num_oscillators);
    return ret;
}

int32_t generate_next_section_sample(output_state* os, section* sec) {
    int32_t ret = 0;
    int8_t i;

    for(i = 0; i < os->num_oscillators && i < sec->num_instruments; ++i) {
        if(os->oscillators[i].frequency != 0) {
            ret += generate_next_osc_sample(os->oscillators + i,
                                            gaintable[sec->instruments[i].notes[os->note_num].gain]);
        }
    }

    ++os->sample_num;
    if(os->sample_num >= ((sample_rate * 60) / sec->tempo)) {
        ++os->note_num;
        printf("note %d\n", os->note_num);
        os->sample_num = 0;

        if(os->note_num >= sec->num_notes) {
            os->is_playing = 0;
        } else {
            for(i = 0; i < os->num_oscillators; ++i) {
                if(i >= sec->num_instruments) {
                    printf("osc %d silence\n", i);
                    os->oscillators[i].frequency = 0;
                    os->oscillators[i].phase = 0;
                } else if(sec->instruments[i].notes[os->note_num].pitch != 255) {
                    uint8_t pitch = sec->instruments[i].notes[os->note_num].pitch;
                    uint8_t gain = sec->instruments[i].notes[os->note_num].gain;
                    printf("osc %d pitch %d gain %d\n", i, pitch, gain);
                    os->oscillators[i].frequency = freqtable[pitch];
                    os->oscillators[i].phase = 0;
                }
            }
        }
    }

    return ret;
}

void populate_test_section(section* sec) {
    sec->tempo = 240;
    sec->num_instruments = 4;
    sec->num_notes = 34;
    sec->instruments = calloc(sizeof(instrument), 4);

    for(int i = 0; i < 4; ++i) {
        sec->instruments[i].notes = calloc(sizeof(note), 34);
        for(int j = 0; j < 34; ++j) {
            sec->instruments[i].notes[j].pitch = 255;
            sec->instruments[i].notes[j].gain = 25;
        }
    }

    sec->instruments[0].type = TRIANGLE;
    sec->instruments[1].type = TRIANGLE;
    sec->instruments[2].type = TRIANGLE;
    sec->instruments[3].type = TRIANGLE;

    sec->instruments[0].notes[0].pitch = 48;
    sec->instruments[0].notes[2].pitch = 55;
    sec->instruments[0].notes[4].pitch = 48;
    sec->instruments[0].notes[6].pitch = 55;
    sec->instruments[0].notes[8].pitch = 48;
    sec->instruments[0].notes[10].pitch = 55;
    sec->instruments[0].notes[12].pitch = 48;
    sec->instruments[0].notes[14].pitch = 55;
    sec->instruments[0].notes[16].pitch = 48;
    sec->instruments[0].notes[18].pitch = 55;
    sec->instruments[0].notes[20].pitch = 48;
    sec->instruments[0].notes[22].pitch = 55;
    sec->instruments[0].notes[24].pitch = 48;
    sec->instruments[0].notes[26].pitch = 55;
    sec->instruments[0].notes[28].pitch = 48;
    sec->instruments[0].notes[30].pitch = 55;
    sec->instruments[0].notes[32].pitch = 48;

    sec->instruments[1].notes[0].pitch = 60;
    sec->instruments[1].notes[1].pitch = 62;
    sec->instruments[1].notes[2].pitch = 64;
    sec->instruments[1].notes[3].pitch = 62;
    sec->instruments[1].notes[4].pitch = 60;
    sec->instruments[1].notes[5].pitch = 62;
    sec->instruments[1].notes[6].pitch = 64;
    sec->instruments[1].notes[7].pitch = 62;
    sec->instruments[1].notes[8].pitch = 60;
    sec->instruments[1].notes[10].pitch = 67;
    sec->instruments[1].notes[15].pitch = 0;
    sec->instruments[1].notes[16].pitch = 67;
    sec->instruments[1].notes[17].pitch = 65;
    sec->instruments[1].notes[18].pitch = 64;
    sec->instruments[1].notes[19].pitch = 65;
    sec->instruments[1].notes[20].pitch = 67;
    sec->instruments[1].notes[21].pitch = 65;
    sec->instruments[1].notes[22].pitch = 64;
    sec->instruments[1].notes[23].pitch = 65;
    sec->instruments[1].notes[24].pitch = 67;
    sec->instruments[1].notes[26].pitch = 60;
    sec->instruments[1].notes[27].pitch = 0;
    sec->instruments[1].notes[28].pitch = 60;

    sec->instruments[2].notes[0].pitch = 0;

    sec->instruments[3].notes[0].pitch = 0;
}

int main(int argc, char** argv) {
    FILE* outfile = fopen("sound.s32", "w");
    section sec;
    output_state* os = create_output_state(16);
    /*writeAiffHeader(outfile, 1, 44100, 16, 44100.0);*/

    populate_test_section(&sec);

    for(int i = 0; i < os->num_oscillators && i < sec.num_instruments; ++i) {
        os->oscillators[i].type = sec.instruments[i].type;
        uint8_t pitch = sec.instruments[i].notes[os->note_num].pitch;
        uint8_t gain = sec.instruments[i].notes[os->note_num].gain;
        printf("osc %d pitch %d gain %d\n", i, pitch, gain);
        os->oscillators[i].frequency = freqtable[pitch];
        os->oscillators[i].phase = 0;
    }

    while(os->is_playing) {
        int32_t signal = generate_next_section_sample(os, &sec);
        fwrite(&signal, 1, sizeof(signal), outfile);
    }

    fclose(outfile);
}
