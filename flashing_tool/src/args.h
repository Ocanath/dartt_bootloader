#ifndef ARGS_H
#define ARGS_H
#include <stdint.h>

typedef struct {
    const char*   filename;
    unsigned char dartt_address;
    int           baudrate;
} args_t;

/* Parse command-line arguments.
 * Prints help and exits on -h/--help.
 * Prints missing required arguments and exits on error. */
args_t parse_args(int argc, char** argv);

#endif
