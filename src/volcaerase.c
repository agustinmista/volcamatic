// ----------------------------------------
//
//  volcaerase
//	==========
//
//  Sample eraser for Korg Volca Sample
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

#include "korg/korg_syro_volcasample.h"
#include "volcautils.h"

// ----------------------------------------
// Print samples erasing map
// ----------------------------------------
void print_samples_to_erase(bool to_erase[]) {
  printf("+----------------------------------------+\n");
  for (int i = 0; i < 10; i++) {
    printf("|");
    for (int j = 0; j < 10; j++) {
      to_erase[i*10 + j]
        ? printf(RED "[%02d]" RESET, i*10 + j)
        : printf(" %02d ", i*10 + j);
    }
    printf("|\n");
  }
  printf("+----------------------------------------+\n");
}

// ----------------------------------------
// Print usage message
// ----------------------------------------
static void print_usage(char *bin) {
  printf(
    "\nUsage: %s samples"
    "\nSample eraser for Korg Volca Sample"
    "\n"
    "\nOptional arguments:"
    "\n  -o FILE     specify the output file name (default: \"syro.wav\")"
    "\n  -t          print a table with the samples to modify"
    "\n  -h          print this help message"
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
	int to_erase_count = 0;
  bool to_erase[100] = { false };
  bool print_table = false;


  // Parse command line options
  int opt;
  char *outfile = "syro.wav";

  while ((opt = getopt (argc, argv, "o:th")) != -1){
    switch (opt) {
    case 'o':
      outfile = optarg;
      break;
    case 't':
      print_table = true;
      break;
    case 'h':
    case '?':
      print_usage(argv[0]);
      return 1;
    }
  }


  // Loop over the input arguments marking samples to erase
  for (int samples_arg = optind; samples_arg < argc; samples_arg++) {

    int n1, n2;
    char *arg = argv[samples_arg];

    // If the dash appears in the argument, we try to parse it as an interval.
    // Otherwise we try to parse it as a single number.
    if (strchr(arg, '-') != NULL) {
      if (sscanf(arg, "%d-%d", &n1, &n2) == 2 && VALID(n1) && VALID(n2)) {
        for (int i = MIN(n1, n2); i <= MAX(n1, n2); i++) {
          to_erase[i] = true;
          to_erase_count++;
        }
      } else {
        printf("invalid interval: %s\n", arg);
        return 1;
      }
    } else if (sscanf(arg, "%d", &n1) == 1 && VALID(n1)) {
      to_erase[n1] = true;
      to_erase_count++;
    } else {
      printf("invalid input: %s\n", arg);
      return 1;
    }
  }

	if (to_erase_count) {
    printf("marking %d samples to be erased\n", to_erase_count);
    if (print_table) print_samples_to_erase(to_erase);
	} else {
		printf("nothing to erase here\n");
		return 1;
  }

  // Load each sample to erase into the buffer
  for (int sample_number = 0; sample_number < 100; sample_number++) {
    if (to_erase[sample_number]) {
      syro_data_ptr->pData = NULL;
      syro_data_ptr->Size = 0;
      syro_data_ptr->DataType = DataType_Sample_Erase;
      syro_data_ptr->Number = sample_number;

      syro_data_ptr++;
    }
  }

	// Start conversion
  printf("starting Syro stream conversion... ");
	status = SyroVolcaSample_Start(&handle, syro_data, to_erase_count, 0, &frame);
	if (status != Status_Success) {
		printf("error starting conversion: %d\n", status);
		free_syrodata(syro_data, to_erase_count);
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
		buf_dest[write_pos++] = (uint8_t) left;
		buf_dest[write_pos++] = (uint8_t) (left >> 8);
		buf_dest[write_pos++] = (uint8_t) right;
		buf_dest[write_pos++] = (uint8_t) (right >> 8);
		frame--;
	}

  // End conversion
  SyroVolcaSample_End(handle);
	free_syrodata(syro_data, to_erase_count);
  printf("ok!\n");

	// Write the output file
  printf("writing Syro output to %s... ", outfile);
	if (write_file(outfile, buf_dest, size_dest)) printf("ok!\n");

	free(buf_dest);
	return 0;
}
