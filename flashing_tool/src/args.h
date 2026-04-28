#ifndef ARGS_H
#define ARGS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    /* required */
    unsigned char   dartt_address;

    /* comms */
    const char *    port;           /* --port, NULL triggers autoconnect */

    /* input file */
    const char *    filename;       /* .bin or .elf, NULL if not provided */

    /* output file */
    const char *    output_file;    /* -o: destination for read operations */

    /* operation flags */
    bool            launch;         /* --launch / --start */
    bool            eraseall;       /* --eraseall */
    bool            verify_only;    /* --verify */
    bool            no_verify;      /* --no-verify */
    bool            no_application; /* --no-application */
    bool            skip_save;      /* --skip-save */
    bool            get_version;    /* --version */
    bool            enable_autoboot;  /* --enable-autoboot */
    bool            disable_autoboot; /* --disable-autoboot */
    bool            set_address;      /* --set_address */
    unsigned char   new_address;      /* value for --set_address */

    /* address overrides */
    uintptr_t        origin_addr;     /* --origin (bin only) */
    bool            has_origin_addr;

    uintptr_t        rorigin;         /* --rorigin: read start address */
    bool            has_rorigin;

    size_t        rlen;           /* --rlen: read length in bytes */
    bool            has_rlen;

    int             baudrate;       /* --baudrate, default 921600 */
} args_t;

/* Parse command-line arguments.
 * Prints help and exits on -h/--help.
 * Prints error and exits on missing required arguments or invalid input. */
args_t parse_args(int argc, char **argv);

#endif
