/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

/*
 * Linux:
 *   cat /etc/udev/rules.d/99-SAM-Flasher.rules
 *   KERNEL=="ttyACM0", MODE="0666"
 *   apt remove modemmanager
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <libgen.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "bool.h"
#include "serial.h"
#include "slip.h"
#include "flash_cmd.h"

static struct option long_option[] =
{
   {"port", 0, NULL, 'p'},
   {"file", 0, NULL, 'f'},
   {"help", 0, NULL, 'h'},
   {"verbose", 0, NULL, 'v'},
   {NULL, 0, NULL, 0},
};

/***********************************************************************
 * Generic helper functions
 **********************************************************************/
static void help(const char *name)
{
    char *bname = strdup(name);
    fprintf(stderr,
        "\n"
        "Usage: %s <options>\n"
        "  -%c,--%s\t COM port (ex. \"COM1\", \"/dev/ttyUSB0\")\n"
        "  -%c,--%s\t LDR file\n"
        "  -%c,--%s\t help\n"
        "  -%c,--%s\t increase debug verbosity\n"
        "\n", basename(bname),
        long_option[0].val, long_option[0].name,
        long_option[1].val, long_option[1].name,
        long_option[2].val, long_option[2].name,
        long_option[3].val, long_option[3].name
    );
    free(bname);
}

/***********************************************************************
 * Main
 **********************************************************************/
int main(int argc, char **argv)
{
    int helpLevel;
    int verbose;

    ser_handle serial;
    unsigned char *commPortStr;

    FILE *f;
    int fileLen, progLen, readLen;
    unsigned char *ldrPath;

    unsigned char dBuf[512];
    int ok;
    int done;
    int percent;

    /* Initialize some variables */
    commPortStr = NULL;
    ldrPath = NULL;
    f = NULL;
    helpLevel = 0;
    verbose = 0;
    ok = 0;

    /* Parse program arguments */
    while (1) {
        int c;
        if ((c = getopt_long(argc, argv, "hvp:f:", long_option, NULL)) < 0)
            break;
        switch (c) {
            case 'h':
                helpLevel++;
                break;
            case 'v':
                verbose++;
                break;
            case 'p':
                commPortStr = strdup(optarg);
                break;
            case 'f':
                ldrPath = strdup(optarg);
                break;
            default:
                helpLevel++;
                break;
        }
    }

    /* Print help if requested */
    if (helpLevel > 0) {
        help(argv[0]);
        exit(0);
    }

    /* Make sure an LDR file has been defined */
    if (ldrPath == NULL) {
        printf("LDR file (-f) required\n");
        exit(-1);
    }

    /* Make sure a COM port has been defined */
    if (commPortStr == NULL) {
        printf("COM port (-p) required\n");
        exit(-2);
    }

    /* Open LDR file and get its length */
    f = fopen(ldrPath, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        fileLen = (int)ftell(f);
        if (fileLen < 0) {
            fileLen = 0;
            ok = 0;
        } else {
            fseek(f, 0, SEEK_SET);
            ok = 1;
        }
    } else {
        perror("Unable to open ldr file");
        ok = 0;
    }

    /* Open selected com port */
    if (ok) {
        serial = ser_open(commPortStr);
        ok = (serial != (ser_handle)-1);
        if (ok) {
            ser_setup(serial, 115200, SER_DATABITS_8, SER_PARITY_NONE, SER_STOPBITS_1);
            ser_set_timeout_ms(serial, 100);
        } else {
            printf("Unable to open com port\n");
        }
    }

    /* Ping bootloader */
    if (ok) {
        send_nop(serial);
        ok = waitfor(serial, CMD_ID_NOP);
        if (ok) {
            if (verbose) {
                printf("SAM Bootloader detected...\n");  fflush(stdout);
            }
        } else {
            printf("SAM Bootloader not detected!\n");
        }
    }

    /* Erase application */
    if (ok) {
        if (verbose) {
            printf("Erasing flash...\n"); fflush(stdout);
        }
        send_erase_application(serial);
        do {
            ok = waitfor(serial, CMD_ID_ERASE_STATUS);
            if (ok) {
                done = process_erase_status(&percent);
                if (verbose >= 3) {
                    printf("Erase %d%%\n", percent); fflush(stdout);
                }
            }
        } while (ok && !done);
    }

    /* Program application */
    if (ok) {
        if (verbose) {
            printf("Programming flash...\n"); fflush(stdout);
        }
        progLen = 0;
        ok = 1;
        while (ok && ((readLen = fread(dBuf, 1, 256, f)) > 0)) {
            send_program(serial, progLen, dBuf, readLen);
            ok = waitfor(serial, CMD_ID_PROGRAM_ACK);
            if (ok) {
                ok = process_program_ack(progLen);
                if (ok) {
                    progLen += readLen;
                }
                if (verbose >= 3) {
                    printf("Programming %d%%\n", (100*progLen)/fileLen); fflush(stdout);
                }
            }
        }

        if ((ok) && (fileLen == progLen)) {
            if (verbose) {
                printf("Programmed %d bytes\n", progLen); fflush(stdout);
            }
        } else {
            printf("Programming failed!\n");
            ok = 0;
        }

    }

    /* Close up everything */
    if (serial != (ser_handle)-1) {
        ser_close(serial);
    }

    if (f) {
        fclose(f);
    }

    if (ldrPath) {
        free(ldrPath);
    }

    if (commPortStr) {
        free(commPortStr);
    }

    if (ok) {
        printf("Success!\n");
    }

    return(0);
}
