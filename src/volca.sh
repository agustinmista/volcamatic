#!/bin/bash

INPUT_FILE=aaa
OUTPUT_FILE=bbb

# Converting/Downsampling
ffmpeg -i $INPUT_FILE\
       -acodec pcm_s16le\
       -filter:a "atempo=2.0"\
       -ar 44100\
       $OUTPUT_FILE
