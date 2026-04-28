#include "args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(const char *prog)
{
    printf(
        "Usage: %s <address> <port> [file] [options]\n"
        "\n"
        "  address              DARTT address (0-255)\n"
        "  file                 Input .bin or .elf file\n"
        "\n"
        "Operations:\n"
        "  --launch             Trigger START_APPLICATION and exit\n"
		"  --start              Equivalent to --launch, starts application"
        "  --eraseall           Erase all pages from application start\n"
        "  --verify             Verify only, skip flashing\n"
        "  --version            Retrieve and print bootloader version\n"
        "  -o <file>            Read memory out to .bin or .elf\n"
		"  --set-address		Set the bootloader address\n"
		"  --enable-autoboot    Set the boot mode word\n"		
		"  --disable-autoboot   Clear the boot mode word\n"
        "\n"
        "Flashing options:\n"
        "  --origin <addr>       Override start address for .bin files (hex or decimal)\n"
        "  --no-application     Skip application start address check, implies --skip-save. Applies to elf only\n"
        "  --no-verify          Skip post-flash verification, implies --skip-save\n"
        "\n"
        "Read options:\n"
        "  --rorigin <addr>      Read start address (absolute, hex or decimal)\n"
        "  --rlen <len>         Read length in bytes (hex or decimal)\n"
        "\n"
        "Comms options:\n"
        "  --port <port>        Serial port (e.g. /dev/ttyUSB0, COM3); autoconnects if omitted\n"
        "  --baudrate <rate>    Baud rate (default 921600)\n"
        "\n"
        "  -h, --help           Print this help and exit\n",
        prog
    );
}

static void die(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
    exit(1);
}

static uint32_t parse_uint32(const char *s, const char *name)
{
    char *end;
    unsigned long v = strtoul(s, &end, 0);
    if (*end != '\0')
    {
        fprintf(stderr, "error: invalid value for %s: '%s'\n", name, s);
        exit(1);
    }
    return (uint32_t)v;
}

args_t parse_args(int argc, char **argv)
{
    if (argc < 2)
    {
        print_help(argv[0]);
        exit(1);
    }

    args_t args = {};
    args.baudrate = 921600;

    int positional = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_help(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "--launch") == 0)
        {
            args.launch = true;
        }
        else if (strcmp(argv[i], "--start") == 0)
        {
            args.launch = true;
        }
        else if (strcmp(argv[i], "--eraseall") == 0)
        {
            args.eraseall = true;
        }
        else if (strcmp(argv[i], "--verify") == 0)
        {
            args.verify_only = true;
        }
        else if (strcmp(argv[i], "--no-verify") == 0)
        {
            args.no_verify = true;
            args.skip_save = true;
        }
        else if (strcmp(argv[i], "--no-application") == 0)
        {
            args.no_application = true;
            args.skip_save = true;
        }
        else if (strcmp(argv[i], "--skip-save") == 0)
        {
            args.skip_save = true;
        }
        else if (strcmp(argv[i], "--version") == 0)
        {
            args.get_version = true;
        }
        else if (strcmp(argv[i], "--enable-autoboot") == 0)
        {
            args.enable_autoboot = true;
        }
        else if (strcmp(argv[i], "--disable-autoboot") == 0)
        {
            args.disable_autoboot = true;
        }
        else if (strcmp(argv[i], "--set-address") == 0)
        {
            if (i + 1 >= argc) die("'--set-address' requires an address argument");
            unsigned long v = strtoul(argv[++i], NULL, 0);
            if (v > 255)
            {
                fprintf(stderr, "error: address '%s' out of range (0-255)\n", argv[i]);
                exit(1);
            }
            args.new_address = (unsigned char)v;
            args.set_address = true;
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 >= argc) die("'-o' requires a filename argument");
            args.output_file = argv[++i];
        }
        else if (strcmp(argv[i], "--origin") == 0)
        {
            if (i + 1 >= argc) die("'--origin' requires an address argument");
            args.origin_addr = parse_uint32(argv[++i], "--origin");
            args.has_origin_addr = true;
        }
        else if (strcmp(argv[i], "--rorigin") == 0)
        {
            if (i + 1 >= argc) die("'--rorigin' requires an address argument");
            args.rorigin = parse_uint32(argv[++i], "--rorigin");
            args.has_rorigin = true;
        }
        else if (strcmp(argv[i], "--rlen") == 0)
        {
            if (i + 1 >= argc) die("'--rlen' requires a length argument");
            args.rlen = parse_uint32(argv[++i], "--rlen");
            args.has_rlen = true;
        }
        else if (strcmp(argv[i], "--port") == 0)
        {
            if (i + 1 >= argc) die("'--port' requires a port argument");
            args.port = argv[++i];
        }
        else if (strcmp(argv[i], "--baudrate") == 0)
        {
            if (i + 1 >= argc) die("'--baudrate' requires a value argument");
            args.baudrate = (int)parse_uint32(argv[++i], "--baudrate");
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "error: unknown option '%s'\n", argv[i]);
            exit(1);
        }
        else
        {
            switch (positional++)
            {
                case 0:
                {
                    unsigned long addr = strtoul(argv[i], NULL, 0);
                    if (addr > 255)
                    {
                        fprintf(stderr, "error: address '%s' out of range (0-255)\n", argv[i]);
                        exit(1);
                    }
                    args.dartt_address = (unsigned char)addr;
                    break;
                }
                case 1:
                    args.filename = argv[i];
                    break;
                default:
                    fprintf(stderr, "error: unexpected argument '%s'\n", argv[i]);
                    exit(1);
            }
        }
    }

    if (positional < 1) die("address is required");

    /* --origin is invalid for .elf files */
    if (args.has_origin_addr && args.filename != NULL)
    {
        const char *ext = strrchr(args.filename, '.');
        if (ext != NULL && strcmp(ext, ".elf") == 0)
            die("'--origin' is invalid for .elf files");
    }

    return args;
}
