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
    int16_t frequency;
} oscillator;

int32_t generate_next_sample(oscillator* osc, int32_t gain) {
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

typedef struct _measure {
    int8_t num_notes;
} measure;

typedef struct _composition {
    int32_t num_measures;
    measure* measures;
    int16_t tempo; /* in beats per minute */
  
} composition;

int main(int argc, char** argv) {
    FILE* outfile = fopen("sound.s32", "w");
    /*writeAiffHeader(outfile, 1, 44100, 16, 44100.0);*/

    oscillator osc;
    osc.type = SAWTOOTH;
    osc.phase = 0;
    osc.frequency = 440;

    for(int i = 0; i < 441000; ++i) {
	int32_t signal = generate_next_sample(&osc, 300000000);
	fwrite(&signal, 1, sizeof(signal), outfile);
    }

    fclose(outfile);
}
