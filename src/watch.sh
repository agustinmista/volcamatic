#!/bin/sh

#https://facebook.github.io/watchman/docs/watchman-make.html
watchman-make --make 'make' -p '**/[^.]*.c' '**/[^.]*.h' '**/Makefile' -t all
