// ----------------------------------------
//
//  volcaload
//	=========
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#include "korg_syro_volcasample.h"
#include "volcautils.h"


// ----------------------------------------
// Print samples to update
// ----------------------------------------
void print_samples_to_load(bool to_load[]) {
  printf("+----------------------------------------+\n");
  for (int i = 0; i < 10; i++) {
    printf("|");
    for (int j = 0; j < 10; j++) {
      to_load[i*10 + j]
        ? printf(GREEN "[%02d]" RESET, i*10 + j)
        : printf(" %02d ", i*10 + j);
    }
    printf("|\n");
  }
  printf("+----------------------------------------+\n");
}

// ----------------------------------------
// Load a single sample file into a buffer
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
// Print usage message
// ----------------------------------------
static void print_usage(char *bin) {
  printf(
    "\nUsage: %s [OPTIONS] sample1 sample2 ..."
    "\nSample uploader for Korg Volca Sample"
    "\n"
    "\nSample names must begin with a number from 0 to 99 to be used as"
    "\nthe sample number, e.g., 001_kick.wav, 02-fx.wav, 5-hihat.wav"
    "\n"
    "\nOptional arguments:"
    "\n  -o [FILE]   specify the output file name (default: \"syro.wav\")"
    "\n",
    bin);
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
  bool to_load[100] = { false };

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
      if (!VALID(sample_number)) {
        printf("Ignoring: %s (bad sample number)", sample_path);
        continue;
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
      to_load[sample_number] = true;
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
		printf("Error starting Syro stream conversion: %d\n", status);
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

  // End conversion
  SyroVolcaSample_End(handle);
	free_syrodata(syro_data, samples_count);

  printf("Loaded %d samples [%d bytes] [~%.2f%% memory]\n",
         samples_count, tot_samples_bytes, (100.0 * tot_samples_bytes) / 4194304);
  print_samples_to_load(to_load);

	// Write the output file
  printf("Writing Syro output to %s\n", outfile);
	if (write_file(outfile, buf_dest, size_dest)) printf("Done!\n");

	free(buf_dest);
	return 0;
}
