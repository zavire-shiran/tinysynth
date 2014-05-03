#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

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

/* Still not entirely happy with this, but it's better than what came before. */
const int32_t gaintable[] = {0x0, 0x2666666, 0x4cccccc, 0x7333333, 0x9999999,
			     0xc000000, 0xe666666, 0x10cccccc, 0x13333333,
			     0x15999999, 0x18000000, 0x1a666666, 0x1ccccccc,
			     0x1f333333, 0x21999999, 0x24000000, 0x26666666,
			     0x28cccccc, 0x2b333333, 0x2d999999, 0x30000000,
			     0x32666666, 0x34cccccc, 0x37333333, 0x39999999,
			     0x3c000000, 0x3e666666, 0x40cccccc, 0x43333333,
			     0x45999999, 0x48000000, 0x4a666666, 0x4ccccccc,
			     0x4f333333, 0x51999999, 0x54000000, 0x56666666,
			     0x58cccccc, 0x5b333333, 0x5d999999, 0x60000000,
			     0x62666666, 0x64cccccc, 0x67333333, 0x69999999,
			     0x6c000000, 0x6e666666, 0x70cccccc, 0x73333333,
			     0x75999999, 0x78000000, 0x7a666666, 0x7ccccccc,
			     0x7f333333, 0x81999999, 0x84000000, 0x86666666,
			     0x88cccccc, 0x8b333333, 0x8d999999, 0x90000000,
			     0x92666666, 0x94cccccc, 0x97333333, 0x99999999,
			     0x9c000000, 0x9e666666, 0xa0cccccc, 0xa3333333,
			     0xa5999999, 0xa8000000, 0xaa666666, 0xaccccccc,
			     0xaf333333, 0xb1999999, 0xb4000000, 0xb6666666,
			     0xb8cccccc, 0xbb333333, 0xbd999999, 0xc0000000,
			     0xc2666666, 0xc4cccccc, 0xc7333333, 0xc9999999,
			     0xcc000000, 0xce666666, 0xd0cccccc, 0xd3333333,
			     0xd5999999, 0xd8000000, 0xda666666, 0xdccccccc,
			     0xdf333333, 0xe1999999, 0xe4000000, 0xe6666666,
			     0xe8cccccc, 0xeb333333, 0xed999999, 0xf0000000};

typedef enum _oscillator_type {
    SQUARE = 0,
    SAWTOOTH = 1,
    TRIANGLE = 2,
    SINE = 3,
    FM = 4
} oscillator_type;

typedef struct _oscillator {
    oscillator_type type;
    uint32_t phase;
    int32_t frequency;
    uint32_t fm_phase;
    uint32_t fm_freq;
    int32_t fm_gain;
} oscillator;

int32_t generate_next_osc_sample(oscillator* osc, int32_t gain) {
    if(osc->type != FM) {
        osc->phase += ((UINT32_MAX / sample_rate) * osc->frequency);
    } else {
        osc->fm_phase += ((UINT32_MAX / sample_rate) * osc->fm_freq);
        int32_t freq = (int32_t)roundf(sinf(2 * pi * ((float)osc->fm_phase / UINT32_MAX)) * osc->fm_gain) + osc->frequency;
        osc->phase += ((UINT32_MAX / sample_rate) * freq);
    }

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
        return (int32_t)roundf(sinf(2 * pi * ((float)osc->phase / UINT32_MAX)) * gain);
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
    int16_t fm_numerator, fm_denominator, fm_gain;
    note* notes; // size is section.num_notes
} instrument;

typedef struct _section {
    int16_t tempo; /* in beats per minute */
    int8_t num_instruments;
    int8_t num_notes;
    instrument* instruments;
} section;

typedef struct _composition {
    int32_t num_play_order;
    int32_t* play_order;
    int32_t num_sections;
    section* sections;
} composition;

typedef struct _output_state {
    int8_t is_playing;
    int8_t section_playing;
    int8_t note_num;
    int32_t sample_num;
    int32_t section_num;
    int8_t num_oscillators;
    oscillator* oscillators;
} output_state;

output_state* create_output_state(int8_t num_oscillators) {
    output_state* ret = calloc(sizeof(output_state), 1);
    ret->is_playing = 1;
    ret->section_playing = 1;
    ret->num_oscillators = num_oscillators;
    ret->section_num = 0;
    ret->oscillators = calloc(sizeof(oscillator), num_oscillators);
    return ret;
}

const int16_t envelope_size = 400;

int32_t generate_next_section_sample(output_state* os, section* sec) {
    int32_t ret = 0;
    int8_t i;
    int32_t end_sample_num = ((sample_rate * 60) / sec->tempo);


    for(i = 0; i < os->num_oscillators && i < sec->num_instruments; ++i) {
        uint8_t current_pitch = sec->instruments[i].notes[os->note_num].pitch;
        uint8_t next_pitch;
        if(sec->num_notes > os->note_num + 1) {
            next_pitch = sec->instruments[i].notes[os->note_num + 1].pitch;
        } else {
            next_pitch = 0;
        }
        if(os->oscillators[i].frequency != 0) {
            int32_t sample = generate_next_osc_sample(os->oscillators + i,
                                                      gaintable[sec->instruments[i].notes[os->note_num].gain]);
            if(current_pitch != 255 && os->sample_num < envelope_size) {
                ret += (sample / envelope_size) * (os->sample_num + 1);
            } else if(next_pitch != 255 && os->sample_num + envelope_size > end_sample_num) {
                ret += (sample / envelope_size) * (end_sample_num - os->sample_num + 1);
            } else {
                ret += sample;
            }
        }
    }

    ++os->sample_num;
    if(os->sample_num >= end_sample_num) {
        ++os->note_num;
        printf("note %d\n", os->note_num);
        os->sample_num = 0;

        if(os->note_num >= sec->num_notes) {
            os->section_playing = 0;
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
                    if(sec->instruments[i].type == FM) {
                        os->oscillators[i].fm_freq = (os->oscillators[i].frequency / sec->instruments[i].fm_denominator) *  sec->instruments[i].fm_numerator;
                        os->oscillators[i].fm_gain = sec->instruments[i].fm_gain;
                    }
                }
            }
        }
    }

    return ret;
}

void setup_output_state_for_section(output_state* os, section* sec) {
    for(int i = 0; i < os->num_oscillators; ++i) {
        if(i < sec->num_instruments) {
            os->oscillators[i].type = sec->instruments[i].type;
            uint8_t pitch = sec->instruments[i].notes[os->note_num].pitch;
            uint8_t gain = sec->instruments[i].notes[os->note_num].gain;
            printf("osc %d pitch %d gain %d\n", i, pitch, gain);
            os->oscillators[i].frequency = freqtable[pitch];
            os->oscillators[i].phase = 0;
            if(sec->instruments[i].type == FM) {
                os->oscillators[i].fm_freq = (os->oscillators[i].frequency / sec->instruments[i].fm_denominator) *  sec->instruments[i].fm_numerator;
                os->oscillators[i].fm_gain = sec->instruments[i].fm_gain;
            }
        } else {
            os->oscillators[i].frequency = 0;
            os->oscillators[i].phase = 0;
        }
    }

}

void populate_test_section_one(section* sec) {
    sec->tempo = 240;
    sec->num_instruments = 4;
    sec->num_notes = 32;
    sec->instruments = calloc(sizeof(instrument), 4);

    for(int i = 0; i < 4; ++i) {
        sec->instruments[i].notes = calloc(sizeof(note), 32);
        for(int j = 0; j < 32; ++j) {
            sec->instruments[i].notes[j].pitch = 255;
            sec->instruments[i].notes[j].gain = 18;
        }
    }

    sec->instruments[0].type = TRIANGLE;
    sec->instruments[0].fm_numerator = 1;
    sec->instruments[0].fm_denominator = 1;
    sec->instruments[0].fm_gain = 100;
    sec->instruments[1].type = TRIANGLE;
    sec->instruments[1].fm_numerator = 1;
    sec->instruments[1].fm_denominator = 1;
    sec->instruments[1].fm_gain = 200;
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
/*    sec->instruments[2].notes[2].pitch = 52;
    sec->instruments[2].notes[4].pitch = 52;
    sec->instruments[2].notes[6].pitch = 52;
    sec->instruments[2].notes[8].pitch = 52;
    sec->instruments[2].notes[10].pitch = 52;*/

    sec->instruments[3].notes[0].pitch = 0;
}

void populate_test_section_two(section* sec) {
    sec->tempo = 240;
    sec->num_instruments = 4;
    sec->num_notes = 32;
    sec->instruments = calloc(sizeof(instrument), 4);

    for(int i = 0; i < 4; ++i) {
        sec->instruments[i].notes = calloc(sizeof(note), 32);
        for(int j = 0; j < 32; ++j) {
            sec->instruments[i].notes[j].pitch = 255;
            sec->instruments[i].notes[j].gain = 23;
        }
    }

    sec->instruments[0].type = TRIANGLE;
    sec->instruments[0].fm_numerator = 1;
    sec->instruments[0].fm_denominator = 1;
    sec->instruments[0].fm_gain = 200;
    sec->instruments[1].type = TRIANGLE;
    sec->instruments[1].fm_numerator = 1;
    sec->instruments[1].fm_denominator = 1;
    sec->instruments[1].fm_gain = 100;

    sec->instruments[0].notes[0].pitch = 72;
    sec->instruments[0].notes[1].pitch = 71;
    sec->instruments[0].notes[2].pitch = 72;
    sec->instruments[0].notes[3].pitch = 0;
    sec->instruments[0].notes[4].pitch = 67;
    sec->instruments[0].notes[5].pitch = 65;
    sec->instruments[0].notes[6].pitch = 67;
    sec->instruments[0].notes[7].pitch = 0;

    sec->instruments[0].notes[8].pitch = 64;
    sec->instruments[0].notes[9].pitch = 62;
    sec->instruments[0].notes[10].pitch = 64;
    sec->instruments[0].notes[11].pitch = 0;
    sec->instruments[0].notes[12].pitch = 60;
    sec->instruments[0].notes[15].pitch = 0;

    sec->instruments[0].notes[16].pitch = 60;
    sec->instruments[0].notes[17].pitch = 64;
    sec->instruments[0].notes[18].pitch = 67;
    sec->instruments[0].notes[19].pitch = 65;
    sec->instruments[0].notes[20].pitch = 64;
    sec->instruments[0].notes[21].pitch = 62;
    sec->instruments[0].notes[22].pitch = 64;

    sec->instruments[0].notes[24].pitch = 65;
    sec->instruments[0].notes[25].pitch = 62;
    sec->instruments[0].notes[26].pitch = 59;
    sec->instruments[0].notes[27].pitch = 62;
    sec->instruments[0].notes[28].pitch = 60;
    sec->instruments[0].notes[29].pitch = 59;
    sec->instruments[0].notes[30].pitch = 60;
    sec->instruments[0].notes[31].pitch = 0;

    sec->instruments[1].notes[0].pitch = 48;
    sec->instruments[1].notes[2].pitch = 55;
    sec->instruments[1].notes[4].pitch = 52;
    sec->instruments[1].notes[6].pitch = 55;

    sec->instruments[1].notes[8].pitch = 48;
    sec->instruments[1].notes[10].pitch = 55;
    sec->instruments[1].notes[12].pitch = 52;
    sec->instruments[1].notes[14].pitch = 55;

    sec->instruments[1].notes[16].pitch = 48;
    sec->instruments[1].notes[18].pitch = 55;
    sec->instruments[1].notes[20].pitch = 52;
    sec->instruments[1].notes[22].pitch = 55;

    sec->instruments[1].notes[24].pitch = 48;
    sec->instruments[1].notes[26].pitch = 55;
    sec->instruments[1].notes[28].pitch = 52;
    sec->instruments[1].notes[30].pitch = 55;

    sec->instruments[2].notes[0].pitch = 0;
    sec->instruments[3].notes[0].pitch = 0;
    
}

void populate_test_composition(composition* comp) {
    comp->num_play_order = 3;
    comp->play_order = calloc(sizeof(int32_t), 3);
    comp->play_order[0] = 0;
    comp->play_order[1] = 1;
    comp->play_order[0] = 0;

    comp->num_sections = 2;
    comp->sections = calloc(sizeof(section), 2);
    populate_test_section_one(comp->sections);
    populate_test_section_two(comp->sections + 1);
}

int main(int argc, char** argv) {
    FILE* outfile = fopen("sound.s32", "w");
    composition comp;
    output_state* os = create_output_state(16);
    /*writeAiffHeader(outfile, 1, 44100, 16, 44100.0);*/

    populate_test_composition(&comp);
    setup_output_state_for_section(os, comp.sections + comp.play_order[0]);

    while(os->is_playing) {
        int32_t signal = generate_next_section_sample(os, comp.sections + comp.play_order[os->section_num]);
        fwrite(&signal, 1, sizeof(signal), outfile);

        if(!os->section_playing) {
            os->section_num++;
            if(os->section_num < comp.num_play_order) {
                os->section_playing = 1;
                os->note_num = 0;
                os->sample_num = 0;
                setup_output_state_for_section(os, comp.sections + comp.play_order[os->section_num]);
            } else {
                os->is_playing = 0;
            }
        }
    }

    fclose(outfile);
}
