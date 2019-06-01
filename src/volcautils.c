// ----------------------------------------
//  Utility functions to handle Syro stream
// ----------------------------------------

#include <stdlib.h>
#include <stdio.h>

#include "volcautils.h"

// ----------------------------------------
// Wav header
// ----------------------------------------
const uint8_t wav_header[52] = {
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
void set_32bit_value(uint8_t *ptr, uint32_t data) {
	for (int i = 0; i < 4; i++) {
		*ptr++ = (uint8_t) data;
		data >>= 8;
	}
}

uint32_t get_32bit_value(uint8_t *ptr) {
	uint32_t data = 0;
	for (int i = 0; i < 4; i++) {
		data <<= 8;
		data |= (uint32_t) ptr[3-i];
	}
	return data;
}

uint16_t get_16bit_value(uint8_t *ptr) {
	uint16_t data;
	data = (uint16_t) ptr[1];
	data <<= 8;
	data |= (uint16_t) ptr[0];
	return data;
}

// ----------------------------------------
// Files I/O
// ----------------------------------------
uint8_t *read_file(char *filename, uint32_t *psize) {

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


int write_file(char *filename, uint8_t *buffer, uint32_t size) {

	FILE *fp;
  fp = fopen(filename, "wb");

  if (!fp) {
		printf("Error opening file: %s\n", filename);
		return 0;
	}

	fwrite(buffer, 1, size, fp);
	fclose(fp);

	return 1;
}

// ----------------------------------------
// Deallocate SyroData
// ----------------------------------------
void free_syrodata(SyroData *syro_data, int samples_count) {
	for (int i = 0; i < samples_count; i++) {
		if (syro_data->pData) {
			free(syro_data->pData);
			syro_data->pData = NULL;
		}
		syro_data++;
	}
}
