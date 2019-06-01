// ----------------------------------------
//
//  volcamatic
//	===============
//
//  Sample uploader for Korg Volca Sample
//
//  original author: Joseph Ernest (twitter: @JosephErnest)
//  adapted by: Agustín Mista (GitHub: @agustinmista)
//  date: 2019 June 1st
//  url: http://github.com/agustinmista/volcamatic
//  license: MIT license
//
//  Note: this program uses the Korg Volca SDK:
//        http://github.com/korginc/volcasample
//
// ----------------------------------------

// For C99!
#define _POSIX_C_SOURCE 2

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>

#include "korg_syro_volcasample.h"
#include "korg_syro_comp.h"

#define WAVFMT_POS_ENCODE	0x00
#define WAVFMT_POS_CHANNEL	0x02
#define WAVFMT_POS_FS		0x04
#define WAVFMT_POS_BIT		0x0E

#define WAV_POS_RIFF_SIZE	0x04
#define WAV_POS_WAVEFMT		0x08
#define WAV_POS_DATA_SIZE	0x28

static const uint8_t wav_header[] = {
	'R' , 'I' , 'F',  'F',		// 'RIFF'
	0x00, 0x00, 0x00, 0x00,		// Size (data size + 0x24)
	'W',  'A',  'V',  'E',		// 'WAVE'
	'f',  'm',  't',  ' ',		// 'fmt '
	0x10, 0x00, 0x00, 0x00,		// Fmt chunk size
	0x01, 0x00,					// encode(wav)
	0x02, 0x00,					// channel = 2
	0x44, 0xAC, 0x00, 0x00,		// Fs (44.1kHz)
	0x10, 0xB1, 0x02, 0x00,		// Bytes per sec (Fs * 4)
	0x04, 0x00,					// Block Align (2ch,16Bit -> 4)
	0x10, 0x00,					// 16Bit
	'd',  'a',  't',  'a',		// 'data'
	0x00, 0x00, 0x00, 0x00		// data size(bytes)
};



// ----------------------------------------
// Decoding helpers
// ----------------------------------------

static void set_32bit_value(uint8_t *ptr, uint32_t data) {
	for (int i = 0; i < 4; i++) {
		*ptr++ = (uint8_t) data;
		data >>= 8;
	}
}


static uint32_t get_32bit_value(uint8_t *ptr) {
	uint32_t data = 0;
	for (int i = 0; i < 4; i++) {
		data <<= 8;
		data |= (uint32_t) ptr[3-i];
	}
	return data;
}

static uint16_t get_16bit_value(uint8_t *ptr) {
	uint16_t data;
	data = (uint16_t) ptr[1];
	data <<= 8;
	data |= (uint16_t) ptr[0];
	return data;
}


// ----------------------------------------
// Files I/O
// ----------------------------------------
static uint8_t *read_file(char *filename, uint32_t *psize) {

  FILE *fp;
	uint8_t *buffer;
	uint32_t size;

	fp = fopen((const char *) filename, "rb");

  if (!fp) {
		printf("File not found: %s\n", filename);
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buffer = malloc(size);
	fread(buffer, 1, size, fp);
	fclose(fp);

	*psize = size;
	return buffer;
}

static bool write_file(char *filename, uint8_t *buffer, uint32_t size) {

	FILE *fp;

  fp = fopen(filename, "wb");

  if (!fp) {
		printf("Error opening file: %s\n", filename);
		return false;
	}

	fwrite(buffer, 1, size, fp);
	fclose(fp);

	return true;
}


// ----------------------------------------
// Sample loader
// ----------------------------------------
static int load_sample_file(char *filename, SyroData *syro_data) {

	uint8_t *src, *poss;
	uint16_t channel_count, sample_byte, bit_depth;
  int16_t *posd;
	uint32_t wav_pos, wav_frames, frame_count, file_size, payload_size;
  int32_t data, datf;

  src = read_file(filename, &file_size);
	if (!src) return 0;

  // Validate header
	if (file_size <= sizeof(wav_header)) {
		printf("error! header too small\n");
		free(src);
		return 0;
	}

	if (memcmp(src, wav_header, 4)) {
		printf("error!, missing 'RIFF' header\n");
		free(src);
		return 0;
	}

	if (memcmp(src + WAV_POS_WAVEFMT, wav_header + WAV_POS_WAVEFMT, 8)) {
		printf("error! missing 'WAVE' or 'fmt ' header\n");
		free(src);
		return 0;
	}

	wav_pos = WAV_POS_WAVEFMT + 4;

	if (get_16bit_value(src + wav_pos + 8 + WAVFMT_POS_ENCODE) != 1) {
		printf("error! bad encoding bit\n");
		free(src);
		return 0;
	}

	channel_count = get_16bit_value(src + wav_pos + 8 + WAVFMT_POS_CHANNEL);
	if ((channel_count != 1) && (channel_count != 2)) {
		printf("error! too many channels: %d (max=2)\n", channel_count);
		free(src);
		return 0;
	}


  bit_depth = get_16bit_value(src+wav_pos+8+WAVFMT_POS_BIT);
  if ((bit_depth != 16) && (bit_depth != 24)) {
    printf("error! invalid bit depth: %d (supported: 16,24)\n", bit_depth);
    free(src);
    return 0;
  }

  sample_byte = bit_depth / 8;
	wav_frames = get_32bit_value(src + wav_pos + 8 + WAVFMT_POS_FS);


  while (true) {
    payload_size = get_32bit_value(src + wav_pos + 4);
		if (!memcmp((src+wav_pos), "data", 4)) break;

		wav_pos += payload_size + 8;
		if (wav_pos + 8 > file_size) {
			printf("error! missing 'data' header.\n");
			free(src);
			return 0;
		}
	}

	if (wav_pos + payload_size + 8 > file_size) {
		printf("error! payload size mismatch.\n");
		free(src);
		return 0;
	}

	// Allocate memory for the new payload
	frame_count = payload_size  / (channel_count * sample_byte);
	payload_size = frame_count * 2;
	syro_data->pData = malloc(payload_size);

	// Convert to 1ch, 16bits
  poss = src + wav_pos + 8;
  posd = (int16_t *) syro_data->pData;

  do {

    datf = 0;
    for (int ch = 0; ch < channel_count; ch++) {

      data = ((int8_t *) poss)[sample_byte - 1];
      for (int sbyte = 1; sbyte < sample_byte; sbyte++) {
        data <<= 8;
        data |= poss[sample_byte-1-sbyte];
      }
      poss += sample_byte;
      datf += data;
    }
    datf /= channel_count;
    *posd++ = (int16_t) datf;

  } while (--frame_count);

	syro_data->Size = payload_size;
	syro_data->Fs = wav_frames;
	syro_data->SampleEndian = LittleEndian;

  free(src);
	return payload_size;
}


// ----------------------------------------
// Deallocate SyroData
// ----------------------------------------
static void free_syrodata(SyroData *syro_data, int samples_count) {
	for (int i = 0; i < samples_count; i++) {
		if (syro_data->pData) {
			free(syro_data->pData);
			syro_data->pData = NULL;
		}
		syro_data++;
	}
}


// ----------------------------------------
// Print usage message
// ----------------------------------------
void print_usage(char *bin) {
  printf(
    "\nUsage: %s [OPTIONS] sample1 sample2 ..."
    "\nSample uploader for Korg Volca Sample"
    "\n"
    "\nSample names must begin with a number from 0 to 99 to be used as"
    "\nthe sample number, e.g., 001_kick.wav, 02-fx.wav, 5-hihat.wav"
    "\n"
    "\nOptional arguments:"
    "\n  -o [FILE]   specify the output file name (default: \"syro.wav\")"
    "\n"
    , bin);
}


// ----------------------------------------
// Main
// ----------------------------------------
int main(int argc, char **argv) {

	SyroData syro_data[100];
	SyroData *syro_data_ptr = syro_data;
	SyroStatus status;
	SyroHandle handle;
	uint8_t *buf_dest;
	int16_t left, right;
	uint32_t size_dest, frame, write_pos;
	int sample_number, samples_count = 0, tot_samples_bytes = 0;


  // Parse command line options
  int opt;
  char *outfile = "syro.wav";

  while ((opt = getopt (argc, argv, "o:")) != -1){
    switch (opt) {
    case 'o':
      outfile = optarg;
      break;
    case '?':
      print_usage(argv[0]);
      return 1;
    }
  }


  // Load each command line sample into the buffer
  for (int sample_arg = optind; sample_arg < argc; sample_arg++) {

    char *sample_path = argv[sample_arg];

    // Check that the sample starts with a number
    if (sscanf(basename(sample_path), "%d", &sample_number) == 1) {

      // Alert about invalid sample numbers
      if (sample_number < 00 || sample_number > 99) {
        printf("Ignoring: %s (bad sample number)", sample_path);
      }

      int sample_bytes = load_sample_file(sample_path, syro_data_ptr);
      if (!sample_bytes)  { continue; }

      // Store the sample data into the buffer
      syro_data_ptr->DataType = DataType_Sample_Liner;
      syro_data_ptr->Number = sample_number;

      printf("Loaded sample [%02d] from: %s (%d bytes)\n",
             sample_number, sample_path, sample_bytes);

      syro_data_ptr++;
      samples_count++;
      tot_samples_bytes += sample_bytes;
    }

  }


	if (!samples_count) {
		printf("Nothing to see here\n");
		return 1;
	}

	// Start conversion
	status = SyroVolcaSample_Start(&handle, syro_data, samples_count, 0, &frame);
	if (status != Status_Success) {
		printf("Error starting conversion: %d\n", status);
		free_syrodata(syro_data, samples_count);
		return 1;
	}

  // Allocate memory for the output file
	size_dest = frame * 4 + sizeof(wav_header);
	buf_dest = malloc(size_dest);

  // Copy the wav header to the output file
	memcpy(buf_dest, wav_header, sizeof(wav_header));
	set_32bit_value(buf_dest + WAV_POS_RIFF_SIZE, frame * 4 + 0x24);
	set_32bit_value(buf_dest + WAV_POS_DATA_SIZE, frame * 4);

	// Get each converted sample from the SDK
	write_pos = sizeof(wav_header);
	while (frame) {
		SyroVolcaSample_GetSample(handle, &left, &right);
		buf_dest[write_pos++] = (uint8_t)left;
		buf_dest[write_pos++] = (uint8_t)(left >> 8);
		buf_dest[write_pos++] = (uint8_t)right;
		buf_dest[write_pos++] = (uint8_t)(right >> 8);
		frame--;
	}

  printf("Loaded %d samples [%d bytes] [~%.2f%%]\n",
         samples_count, tot_samples_bytes, (100.0 * tot_samples_bytes) / 4194304);

  // End conversion
  SyroVolcaSample_End(handle);
	free_syrodata(syro_data, samples_count);

	// Write the output file
  printf("Writing Syro output to %s\n", outfile);
	if (write_file(outfile, buf_dest, size_dest)) printf("Done!\n");

	free(buf_dest);
	return 0;
}
