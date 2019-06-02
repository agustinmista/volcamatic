// ----------------------------------------
//  Utility functions to handle Syro stream
// ----------------------------------------

#ifndef __VOLCAUTILS_H__
#define __VOLCAUTILS_H__

#include "korg/korg_syro_volcasample.h"

#define WAVFMT_POS_ENCODE	 0x00
#define WAVFMT_POS_CHANNEL 0x02
#define WAVFMT_POS_FS		   0x04
#define WAVFMT_POS_BIT		 0x0E

#define WAV_POS_RIFF_SIZE	 0x04
#define WAV_POS_WAVEFMT		 0x08
#define WAV_POS_DATA_SIZE	 0x28

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

#define MIN(x, y) (x <  y ? x : y)
#define MAX(x, y) (x >= y ? x : y)
#define VALID(x) (x >= 0 && x < 100)

// ----------------------------------------
// Wav header
// ----------------------------------------
extern const uint8_t wav_header[52];

// ----------------------------------------
// Decoding helpers
// ----------------------------------------
void     set_32bit_value(uint8_t *ptr, uint32_t data);
uint32_t get_32bit_value(uint8_t *ptr);
uint16_t get_16bit_value(uint8_t *ptr);

// ----------------------------------------
// Files I/O
// ----------------------------------------
uint8_t *read_file(char *filename, uint32_t *psize);
int     write_file(char *filename, uint8_t *buffer, uint32_t size);

// ----------------------------------------
// Deallocate SyroData
// ----------------------------------------
void free_syrodata(SyroData *syro_data, int samples_count);


#endif  // #ifndef VOLCAUTILS_H__
