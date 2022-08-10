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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "syslog.h"
#include "twi_simple.h"
#include "shell.h"
#include "term.h"
#include "xmodem.h"
#include "util.h"

#include "FreeRTOS.h"
#include "task.h"

#include "context.h"

/* FIXME with "shell_priv.h" */
extern void shellh_show_help( const char *cmd, const char *helptext );
#define SHELL_SHOW_HELP( cmd )  shellh_show_help( #cmd, shell_help_##cmd )

/***********************************************************************
 * Main application context
 **********************************************************************/
static APP_CONTEXT *context = &mainAppContext;

/***********************************************************************
 * XMODEM helper functions
 **********************************************************************/
struct flashWriteState {
   const FLASH_INFO *flash;
   unsigned addr;
   unsigned maxAddr;
   unsigned eraseBlockSize;
};

int flashDataWrite(unsigned char *data, unsigned size, bool final, void *usr)
{
   struct flashWriteState *state = (struct flashWriteState *)usr;
   int err;

   if (size > 0) {
      if ((state->addr % state->eraseBlockSize) == 0) {
         err = flash_erase(state->flash, state->addr, state->eraseBlockSize);
         if (err != FLASH_OK) {
            return(XMODEM_ERROR_GENERIC);
         }
      }
      if ((state->addr + size) <= state->maxAddr) {
         err = flash_program(state->flash, state->addr, (const unsigned char *)data, size);
         if (err != FLASH_OK) {
            return(XMODEM_ERROR_GENERIC);
         }
         state->addr += size;
      }
   }
   return(XMODEM_ERROR_NONE);
}

struct fileWriteState {
    FILE *f;
};

int fileDataWrite(unsigned char *data, unsigned size, bool final, void *usr)
{
    struct fileWriteState *state = (struct fileWriteState *)usr;
    size_t wsize;

    if (size > 0) {
        wsize = fwrite(data, sizeof(*data), size, state->f);
        if (wsize != size) {
            return(XMODEM_ERROR_CALLBACK);
        }
    }
    return(XMODEM_ERROR_NONE);
}

int confirmDanger(char *warnStr)
{
    char c;

    printf( "%s\n", warnStr );
    printf( "Are you sure you want to continue? [y/n] " );

    c = term_getch( TERM_INPUT_WAIT );
    printf( "%c\n", isprint( c ) ? c : ' ' );

    if( tolower(c) == 'y' ) {
        return(1);
    }

    return(0);
}

/***********************************************************************
 * CMD: recv
 **********************************************************************/
const char shell_help_recv[] = "<file>\n"
    "  Transfer and save to file\n";
const char shell_help_summary_recv[] = "Receive a file via XMODEM";

void shell_recv( int argc, char **argv )
{
    struct fileWriteState fileState;
    long size;

    if( argc != 2 ) {
        printf( "Usage: recv <file>\n" );
        return;
    }

    fileState.f = fopen( argv[ 1 ], "wb");
    if( fileState.f == NULL) {
        printf( "unable to open file %s\n", argv[ 1 ] );
        return;
    }
    printf( "Waiting for file ... " );
    size = xmodem_receive(fileDataWrite, &fileState);
    if (size < 0) {
        printf( "XMODEM Error: %ld\n", size);
    } else {
        printf( "received and saved as %s\n", argv[ 1 ] );
    }
    fclose( fileState.f );
}

/***********************************************************************
 * CMD: i2c
 **********************************************************************/
const char shell_help_i2c[] = "<i2c_port> <i2c_addr> <reg_addr> <length>\n"
  "  i2c_port - I2C port to probe\n"
  "  i2c_addr - I2C device address\n"
  "  reg_addr - Starting address register dump\n"
  "  length - Number of bytes to dump\n";
const char shell_help_summary_i2c[] = "Executes an I2C write/read transaction";

void shell_i2c(int argc, char **argv)
{
    sTWI *twiHandle;
    TWI_SIMPLE_PORT twiPort;
    bool useLocalTwiHandle;
    uint16_t  reg_addr;
    uint8_t  i2c_addr;
    uint16_t  length;
    uint16_t  addrLength;
    uint8_t twiWrBuffer[2];
    uint8_t *twiRdBuffer;
    TWI_SIMPLE_RESULT result;
    int i;

    if (argc != 5) {
        SHELL_SHOW_HELP(i2c);
        return;
    }

    twiPort = (TWI_SIMPLE_PORT)strtol(argv[1], NULL, 0);
    twiHandle = NULL;

    if ((twiPort < TWI0) || (twiPort >= TWI_END)) {
        printf("Invalid I2C port!\n");
        return;
    }

    i2c_addr = strtol(argv[2], NULL, 0);
    reg_addr = strtol(argv[3], NULL, 0);
    length = strtol(argv[4], NULL, 0);

    if (length <= 0) {
        length = 1;
    }

    /* Allocate a buffer to read into */
    twiRdBuffer = SHELL_MALLOC(length);

    printf ( "I2C Device (0x%02x): addr 0x%04x, bytes %d (0x%02x)",
        i2c_addr, reg_addr, length, length);

    /* See if requested TWI port is already open globally */
    if ((twiPort == TWI0) && context->twi0Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi0Handle;
        result = TWI_SIMPLE_SUCCESS;
    } else if ((twiPort == TWI2) && context->twi2Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi2Handle;
        result = TWI_SIMPLE_SUCCESS;
    }
    if (twiHandle == NULL) {
        useLocalTwiHandle = true;
        result = twi_open(twiPort, &twiHandle);
    }

    /* Do write/read of peripheral at the specified address */
    if (result == TWI_SIMPLE_SUCCESS) {
        if (reg_addr > 255) {
            twiWrBuffer[0] = (reg_addr >> 8) & 0xFF;
            twiWrBuffer[1] = (reg_addr >> 0) & 0xFF;
            addrLength = 2;
        } else {
            twiWrBuffer[0] = reg_addr;
            addrLength = 1;
        }
        result = twi_writeRead(twiHandle, i2c_addr, twiWrBuffer, addrLength, twiRdBuffer, length);
        if (result == TWI_SIMPLE_SUCCESS) {
            for (i = 0; i < length; i++) {
                if ((i % 16) == 0) {
                    printf("\n");
                    printf("%02x: ", i + reg_addr);
                }
                printf("%02x ", twiRdBuffer[i]);
            }
            printf("\n");
        } else {
            printf("\n");
            printf("twi twi_writeRead() error %d\n", result);
        }
    } else {
        printf("\n");
        printf ("twi open error %d\n", result);
    }

    /* Free the read buffer */
    if (twiRdBuffer) {
        SHELL_FREE(twiRdBuffer);
    }

    if (useLocalTwiHandle && twiHandle) {
        twi_close(&twiHandle);
    }
}

/***********************************************************************
 * CMD: i2c_probe
 **********************************************************************/
const char shell_help_i2c_probe[] = "<i2c_port>\n"
  "  i2c_port - I2C port to probe\n";
const char shell_help_summary_i2c_probe[] = "Probe an I2C port for active devices";

void shell_i2c_probe(int argc, char **argv)
{
    bool useLocalTwiHandle;
    sTWI *twiHandle;
    TWI_SIMPLE_RESULT result;
    TWI_SIMPLE_PORT twiPort;
    int i;

    if (argc != 2) {
        SHELL_SHOW_HELP(i2c_probe);
        return;
    }

    twiPort = (TWI_SIMPLE_PORT)strtol(argv[1], NULL, 0);
    twiHandle = NULL;

    if ((twiPort < TWI0) || (twiPort >= TWI_END)) {
        printf("Invalid I2C port!\n");
        return;
    }

    printf ( "Probing I2C port %d:\n", twiPort);

    /* See if requested TWI port is already open globally */
    if ((twiPort == TWI0) && context->twi0Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi0Handle;
        result = TWI_SIMPLE_SUCCESS;
    } else if ((twiPort == TWI2) && context->twi2Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi2Handle;
        result = TWI_SIMPLE_SUCCESS;
    }
    if (twiHandle == NULL) {
        useLocalTwiHandle = true;
        result = twi_open(twiPort, &twiHandle);
    }

    if (result == TWI_SIMPLE_SUCCESS) {
        for (i = 0; i < 128; i++) {
            result = twi_write(twiHandle, i, NULL, 0);
            if (result == TWI_SIMPLE_SUCCESS) {
                printf(" Found device 0x%02x\n", i);
            }
        }
    } else {
        printf ("twi open error %d\n", result);
    }

    if (useLocalTwiHandle && twiHandle) {
        twi_close(&twiHandle);
    }
}

/***********************************************************************
 * CMD: syslog
 **********************************************************************/
const char shell_help_syslog[] = "\n";
const char shell_help_summary_syslog[] = "Show the live system log";

void shell_syslog(int argc, char **argv)
{
    int c;

    c = 0;
    do {
#ifdef FREE_RTOS
        if (c < 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
#endif
        syslog_dump();
        c = term_getch(TERM_INPUT_DONT_WAIT);
    } while (c < 0);
}

/***********************************************************************
 * CMD: ls
 **********************************************************************/
#include "fs_devman.h"

const char shell_help_ls[] = "<device>\n";
const char shell_help_summary_ls[] = "Shows a device directory listing";

static void shell_ls_helper( const char *crtname, int recursive, int *phasdirs )
{
  void *d;
  uint32_t total = 0;
  FS_DEVMAN_DIRENT *ent;
  int ndirs = 0;
  unsigned year ,month, day, hour, min, sec;

  if( ( d = fs_devman_opendir( crtname ) ) != NULL )
  {
    total = 0;
    printf( "%s", crtname );
    while( ( ent = fs_devman_readdir( d ) ) != NULL )
    {
      printf("\n");
      if( ent->flags & FS_DEVMAN_DIRENT_FLAG_DIR )
      {
        printf( "%12s ", "<DIR>" );
        ndirs = ndirs + 1;
        if( phasdirs )
          *phasdirs = 1;
      }
      else
      {
        printf( "%12u ", ( unsigned )ent->fsize );
        total = total + ent->fsize;
      }
      if (ent->fdate) {
          year = ((ent->fdate >> 9) & 0x7F) + 1980;
          month = (ent->fdate >> 5) & 0xF;
          day = (ent->fdate >> 0) & 0x1F;
          printf("%04u-%02u-%02u ", year, month, day);
    }
      if (ent->ftime) {
          hour = (ent->ftime >> 11) & 0x1F;
          min = (ent->ftime >> 5) & 0x3F;
          sec = ((ent->ftime >> 0) & 0x1F) * 2;
          printf("%02u:%02u:%02u ", hour, min, sec);
      }
      printf( " %s", ent->fname );
    }
    fs_devman_closedir( d );
    printf("\n");
  }
}

void shell_ls(int argc, char **argv)
{
    FS_DEVMAN_RESULT result;
    int phasdirs;
    char *device;

    if (argc == 1) {
        result = fs_devman_get_default((const char **)&device);
        if (result != FS_DEVMAN_OK) {
            device = "wo:";
        }
    } else {
        device = argv[1];
    }

    shell_ls_helper(device, 0, &phasdirs );
}

/***********************************************************************
 * CMD: format
 **********************************************************************/
const char shell_help_format[] = "[all]\n";
const char shell_help_summary_format[] = "Formats the internal flash filesystem";

#include "romfs.h"

void shell_format(int argc, char **argv)
{
    int all;

    all = 0;
    if ((argc > 1) && (strcmp(argv[1], "all") == 0)) {
        all = 1;
    }

    printf("Be patient, this may take a while.\n");
    printf("Formatting...\n");
    romfs_format(all);
    printf("Done.\n");
}

/***********************************************************************
 * CMD: discover
 **********************************************************************/
#include "init.h"
#include "a2b_xml.h"
#include "adi_a2b_cmdlist.h"

#define AD2425W_SAM_I2C_ADDR 0x68

const char shell_help_discover[] = "<a2b.xml> <verbose> <i2c_port> <i2c_addr> <reset>\n"
  "  a2b.xml  - A SigmaStudio A2B XML config export file\n"
  "             default 'a2b.xml'\n"
  "  verbose  - Print out results to 0:none, 1:stdout, 2:syslog\n"
  "             default: 1\n"
  "  i2c_port - Set the AD242x transceiver I2C port.\n"
  "             default: TWI0\n"
  "  i2c_addr - Set the AD2425 transceiver I2C address\n"
  "             default: 0x68\n"
  "  reset    - Reset the A2B transceiver(s) 'y':yes, 'n':no\n"
  "             default: 'y'\n";
const char shell_help_summary_discover[] = "Discovers an A2B network";

static ADI_A2B_CMDLIST_RESULT shell_discover_twi_read(
    void *twiHandle, uint8_t address,
    void *in, uint16_t inLen, void *usr)
{
    TWI_SIMPLE_RESULT twiResult;
    twiResult = twi_read(twiHandle, address, in, inLen);
    return (twiResult == TWI_SIMPLE_SUCCESS ?
        ADI_A2B_CMDLIST_SUCCESS : ADI_A2B_CMDLIST_A2B_I2C_READ_ERROR);
    }

static ADI_A2B_CMDLIST_RESULT shell_discover_twi_write(
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *usr)
{
    TWI_SIMPLE_RESULT twiResult;
    twiResult = twi_write(twiHandle, address, out, outLen);
    return (twiResult == TWI_SIMPLE_SUCCESS ?
        ADI_A2B_CMDLIST_SUCCESS : ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR);
    }

static ADI_A2B_CMDLIST_RESULT shell_discover_twi_write_read(
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *in, uint16_t inLen, void *usr)
{
    TWI_SIMPLE_RESULT twiResult;
    twiResult = twi_writeRead(twiHandle, address, out, outLen, in, inLen);
    return (twiResult == TWI_SIMPLE_SUCCESS ?
        ADI_A2B_CMDLIST_SUCCESS : ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR);
}

static void shell_discover_delay(uint32_t mS, void *usr)
{
    vTaskDelay(pdMS_TO_TICKS(mS));
}

static uint32_t shell_discover_get_time(void *usr)
{
    return(xTaskGetTickCount() * (1000 / configTICK_RATE_HZ));
}

static void *shell_discover_get_buffer(uint16_t size, void *usr)
{
    return(umm_malloc(size));
}

static void shell_discover_free_buffer(void *buffer, void *usr)
{
    umm_free(buffer);
}

typedef int (*PF)(const char *restrict format, ...);

void shell_discover(int argc, char **argv)
{
    const char *fileName = "a2b.xml";
    ADI_A2B_CMDLIST_RESULT cmdListResult;
    void *a2bInitSequence;
    uint32_t a2bIinitLength;
    int verbose;
    bool useLocalTwiHandle;
    sTWI *twiHandle;
    TWI_SIMPLE_RESULT result;
    TWI_SIMPLE_PORT twiPort;
    uint8_t ad2425I2CAddr;
    bool reset;
    A2B_CMD_TYPE a2bCmdType;
    ADI_A2B_CMDLIST *list;
    ADI_A2B_CMDLIST_EXECUTE_INFO execInfo;
    ADI_A2B_CMDLIST_SCAN_INFO scanInfo;
    ADI_A2B_CMDLIST_OVERRIDE_INFO overrideInfo;
    PF pf;

    ADI_A2B_CMDLIST_CFG cfg = {
        .twiRead = shell_discover_twi_read,
        .twiWrite = shell_discover_twi_write,
        .twiWriteRead = shell_discover_twi_write_read,
        .delay = shell_discover_delay,
        .getTime = shell_discover_get_time,
        .getBuffer = shell_discover_get_buffer,
        .freeBuffer = shell_discover_free_buffer,
        .usr = context
    };

    /* Determine file name */
    if (argc >= 2) {
        fileName = (const char *)argv[1];
    } else {
        fileName = "a2b.xml";
    }

    /* Determine verbosity */
    if (argc >= 3) {
        verbose = strtol(argv[2], NULL, 0);
    } else {
        verbose = 1;
    }
    if (verbose == 1) {
        pf = printf;
    } else if (verbose == 2) {
        pf = (PF)syslog_printf;
    } else {
        pf = NULL;
    }

    /* Determine TWI port */
    if (argc >= 4) {
        twiPort = strtol(argv[3], NULL, 0);
    } else {
        twiPort = 0;
        }
    if ((twiPort < TWI0) || (twiPort >= TWI_END)) {
        if (pf) {
            pf("Invalid I2C port!\n");
        }
        return;
    }

    /* See if requested TWI port is already open globally */
    if ((twiPort == TWI0) && context->twi0Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi0Handle;
        result = TWI_SIMPLE_SUCCESS;
    } else if ((twiPort == TWI2) && context->twi2Handle) {
        useLocalTwiHandle = false;
        twiHandle = context->twi2Handle;
        result = TWI_SIMPLE_SUCCESS;
    }
    if (twiHandle == NULL) {
        useLocalTwiHandle = true;
        result = twi_open(twiPort, &twiHandle);
    }

    /* Save the TWI handle to use */
    cfg.handle = (void *)twiHandle;

    /* Determine AD2425 I2C address */
    if (argc >= 5) {
        ad2425I2CAddr = strtol(argv[4], NULL, 0);
    } else {
        ad2425I2CAddr = AD2425W_SAM_I2C_ADDR;
    }

    /* Reset the transceiver */
    if (argc >= 6) {
        reset = toupper(argv[5][0]) == 'Y';
    } else {
        reset = true;
    }

    /* Reset the AD2425 and A2B network */
    if (reset) {
        ad2425_reset();
    }

    /* Load the A2B network init */
    a2bIinitLength = a2b_xml_load(fileName, &a2bInitSequence, &a2bCmdType);

    /* If successful, play out the binary init sequence */
    if (a2bIinitLength && a2bInitSequence) {

        /* Open a command list instance */
        cmdListResult = adi_a2b_cmdlist_open(&list, &cfg);

        /* Set the command list */
        cmdListResult =  adi_a2b_cmdlist_set(
            list, 0x68, a2bInitSequence, a2bIinitLength, a2bCmdType
        );

        /* Clear the overrides */
        memset(&overrideInfo, 0, sizeof(overrideInfo));

        /* Override default SigmaStudio address */
        overrideInfo.masterAddr_override = true;
        overrideInfo.masterAddr = ad2425I2CAddr;

        /* Confirm master I2S/TDM settings and override if they don't match */
        cmdListResult = adi_a2b_cmdlist_scan(
            list, &scanInfo
        );
        if (scanInfo.I2SGCFG_valid && (scanInfo.I2SGCFG != SYSTEM_I2SGCFG)) {
            if (pf) {
                pf("WARNING: I2SGCFG mismatch (expected %02x, got %02x)\n",
                    SYSTEM_I2SGCFG, scanInfo.I2SGCFG);
                pf("         Overriding...\n");
            }
            overrideInfo.I2SGCFG_override = true;
            overrideInfo.I2SGCFG = SYSTEM_I2SGCFG;
        }
        if (scanInfo.I2SCFG_valid && (scanInfo.I2SCFG != SYSTEM_I2SCFG)) {
            if (pf) {
                pf("WARNING: I2SCFG mismatch (expected %02x, got %02x)\n",
                    SYSTEM_I2SCFG, scanInfo.I2SCFG);
                pf("         Overriding...\n");
            }
            overrideInfo.I2SCFG_override = true;
            overrideInfo.I2SCFG = SYSTEM_I2SCFG;
        }

        /* Process any overrides */
        cmdListResult = adi_a2b_cmdlist_override(list, &overrideInfo);

        /* Run the command list */
        cmdListResult = adi_a2b_cmdlist_execute(list, &execInfo);

        if (pf) {
            pf("A2B config lines processed: %lu\n", execInfo.linesProcessed);
            pf("A2B discovery result: %s\n", execInfo.resultStr);
            pf("A2B nodes discovered: %d\n", execInfo.nodesDiscovered);
        }

        /* Close the command list */
        cmdListResult = adi_a2b_cmdlist_close(&list);

        /* Free the network config */
        a2b_xml_free(a2bInitSequence, a2bIinitLength, a2bCmdType);

    } else {
        if (pf) {
            pf("Error loading '%s' A2B init XML file", fileName);
        }
    }

    if (useLocalTwiHandle && twiHandle) {
        twi_close(&twiHandle);
    }

}

/***********************************************************************
 * CMD: df
 **********************************************************************/
const char shell_help_df[] = "\n";
const char shell_help_summary_df[] = "Shows internal filesystem disk full status";

void shell_df( int argc, char **argv )
{
   u32 size, used;
   u32 err;

   printf("%-10s %10s %10s %10s %5s\n", "Filesystem", "Size", "Used", "Available", "Use %");
   err = romfs_full(&size, &used);
   if (err > 0) {
      printf("%-10s %10u %10u %10u %5u\n", "wo:",
        (unsigned)size, (unsigned)used, (unsigned)(size - used),
        (unsigned)((100 * used) / size));
   }
}

/***********************************************************************
 * CMD: rm/del
 **********************************************************************/
#include <unistd.h>

const char shell_help_rm[] = "<file1> [<file2> ...]\n";
const char shell_help_summary_rm[] = "Removes a file";

void shell_rm( int argc, char **argv )
{
  int i;

  if (argc < 2) {
    printf( "Usage: rm <file1> [<file2> ...]\n" );
    return;
  }

  for (i = 1; i < argc; i++) {
    if (unlink(argv[i]) != 0) {
      printf("Unable to remove '%s'\n", argv[i]);
    }
  }
}

/***********************************************************************
 * CMD: cat
 **********************************************************************/
const char shell_help_cat[] = "\n";
const char shell_help_summary_cat[] = "Print file on standard output";

void shell_cat( int argc, char **argv )
{
    FILE *handle;
    unsigned i;
    unsigned char c;

    if( argc < 2 ) {
        printf( "Usage: cat <filename1> [<filename2> ...]\n" );
        return;
    }
    for( i = 1; i < argc; i ++ ) {
        if( ( handle = fopen( argv[ i ], "r" ) ) > 0 )
        {
            while (fread(&c, sizeof(c), 1, handle) > 0) {
                printf("%c", c);
            }
            fclose(handle);
        } else {
            printf( "Unable to open '%s'\n", argv[ i ] );
        }
    }
}

/***********************************************************************
 * CMD: copy/cp
 **********************************************************************/
const char shell_help_cp[] = "<src> <dst>\n";
const char shell_help_summary_cp[] = "Copy source file <src> to <dst>";

#define SHELL_COPY_BUFSIZE    256

void shell_cp( int argc, char **argv )
{
   FILE *fps = NULL, *fpd = NULL;
   void *buf = NULL;
   size_t datalen, datawrote, total = 0;

   if( argc != 3 ) {
      printf( "Usage: cp <source> <destination>\n" );
      return;
   }

   if( ( fps = fopen( argv[ 1 ], "r" ) ) == NULL ) {
      printf( "Unable to open %s for reading\n", argv[ 1 ] );
   } else {
      if( ( fpd = fopen( argv[ 2 ], "w" ) ) == NULL ) {
         printf( "Unable to open %s for writing\n", argv[ 2 ] );
      } else {
         if( ( buf = SHELL_MALLOC( SHELL_COPY_BUFSIZE ) ) == NULL ) {
            printf( "Not enough memory\n" );
         } else {
            while( 1 ) {
               datalen = fread( buf, 1, SHELL_COPY_BUFSIZE, fps );
               datawrote = fwrite( buf, 1, datalen, fpd );
               if( datawrote < datalen ) {
                  printf( "Copy error (no space left on target?)\n" );
                  break;
               }
               total += datalen;
               if( datalen < SHELL_COPY_BUFSIZE ) {
                  break;
               }
            }
            fflush( fpd );
            printf( "%u bytes copied\n", ( unsigned int )total );
         }
      }
   }

   if( fps ) {
      fclose( fps );
   }
   if( fpd ) {
      fclose( fpd );
   }
   if( buf ) {
      SHELL_FREE( buf );
   }
}


/***********************************************************************
 * CMD: run
 **********************************************************************/
const char shell_help_run[] = "<file1> [<file2> ...]\n";
const char shell_help_summary_run[] = "Runs a command file";

void shell_run( int argc, char **argv )
{
    FILE *f = NULL;
    char *cmd = NULL;
    char *ok = NULL;
    int i;

    if (argc < 2) {
        printf( "Usage: run <file1> [<file2> ...]\n" );
        return;
    }

    cmd = umm_malloc(SHELL_MAX_LINE_LEN);

    for (i = 1; i < argc; i++) {
        f = fopen(argv[i], "r");
        if (f) {
            ok = NULL;
            do {
                ok = fgets(cmd, SHELL_MAX_LINE_LEN, f);
                if (ok == cmd) {
                    shell_exec(NULL, cmd);
                }
            } while (ok);
            fclose(f);
        } else {
            printf("Failed to open '%s'\n", argv[i]);
        }
    }

    umm_free(cmd);
}

/***********************************************************************
 * CMD: stacks
 **********************************************************************/
const char shell_help_stacks[] = "\n";
const char shell_help_summary_stacks[] = "Report task stack usage";

void shell_stacks( int argc, char **argv )
{
    APP_CONTEXT *context = &mainAppContext;

    printf("High Water Marks are in 32-bit words (zero is bad).\n");
    printf("Task Stask High Water Marks:\n");
    if (context->startupTaskHandle) {
        printf(" Startup: %u\n",
            (unsigned)uxTaskGetStackHighWaterMark(context->startupTaskHandle));
    }
    if (context->houseKeepingTaskHandle) {
        printf(" Housekeeping: %u\n",
            (unsigned)uxTaskGetStackHighWaterMark(context->houseKeepingTaskHandle));
    }
    if (context->uac2TaskHandle) {
        printf(" UAC2: %u\n",
            (unsigned)uxTaskGetStackHighWaterMark(context->uac2TaskHandle));
    }
}

/***********************************************************************
 * CMD: cpu
 **********************************************************************/
#include "cpu_load.h"

const char shell_help_cpu[] = "\n";
const char shell_help_summary_cpu[] = "Report cpu usage";

void shell_cpu( int argc, char **argv )
{
    uint32_t percentCpuLoad, maxCpuLoad;
    percentCpuLoad = cpuLoadGetLoad(&maxCpuLoad, true);
    printf("CPU Load: %u%% (%u%% peak)\n",
        (unsigned)percentCpuLoad, (unsigned)maxCpuLoad);
    printf("SHARC0 Load: %u cycles\n",
        (unsigned)context->sharc0Cycles);
    printf("SHARC1 Load: %u cycles\n",
        (unsigned)context->sharc1Cycles);
}

/***********************************************************************
 * CMD: usb
 **********************************************************************/
#include "buffer_track.h"
#include "cpu_load.h"
#include "clocks.h"

const char shell_help_usb[] = "\n";
const char shell_help_summary_usb[] = "Displays USB performance tracking metrics";

void shell_usb( int argc, char **argv )
{
    bool showIn = true;
    bool showOut = true;

    if (argc == 2) {
        if (strcmp(argv[1], "out") == 0) {
            showIn = false;
        } else if (strcmp(argv[1], "in") == 0) {
            showOut = false;
        }
    }

    /* USB OUT Stats */
    if (showOut) {
        printf("USB OUT (Rx):\n");
        printf("  Total Pkts: %u\n", (unsigned)context->uac2stats.rx.ep.count);
        printf("  Overruns: %u\n", (unsigned)context->uac2stats.rx.usbRxOverRun);
        printf("  Underruns: %u\n", (unsigned)context->uac2stats.rx.usbRxUnderRun);
        printf("  Last Pkt Time: %uuS\n",
            (unsigned)cpuLoadCyclesToMicrosecond(context->uac2stats.rx.ep.lastPktTime)
        );
        printf("  Max Pkt Time: %uuS\n",
            (unsigned)cpuLoadCyclesToMicrosecond(context->uac2stats.rx.ep.maxPktTime)
        );
        printf("  Last Pkt Size: %u (%u, %u)\n",
            (unsigned)context->uac2stats.rx.ep.lastPktSize,
            (unsigned)context->uac2stats.rx.ep.minPktSize,
            (unsigned)context->uac2stats.rx.ep.maxPktSize
        );
        printf("  Buffer Fill: %u\n",
            (unsigned)bufferTrackGetFrames(UAC2_OUT_BUFFER_TRACK_IDX, context->cfg.usbOutChannels));
        printf("  Sample Rate Feedback: %u\n",
            (unsigned)bufferTrackGetSampleRate(UAC2_OUT_BUFFER_TRACK_IDX));
    }

    /* USB IN Stats */
    if (showIn) {
        printf("USB IN (Tx):\n");
        printf("  Total Pkts: %u\n", (unsigned)context->uac2stats.tx.ep.count);
        printf("    Failed: %u\n", (unsigned)context->uac2stats.tx.ep.failed);
        printf("    Aborted: %u\n", (unsigned)context->uac2stats.tx.ep.aborted);
        printf("    OK: %u\n", (unsigned)context->uac2stats.tx.ep.ok);
        printf("  Overruns: %u\n", (unsigned)context->uac2stats.tx.usbTxOverRun);
        printf("  Underruns: %u\n", (unsigned)context->uac2stats.tx.usbTxUnderRun);
        printf("  Last Pkt Time: %uuS\n",
            (unsigned)cpuLoadCyclesToMicrosecond(context->uac2stats.tx.ep.lastPktTime)
        );
        printf("  Max Pkt Time: %uuS\n",
            (unsigned)cpuLoadCyclesToMicrosecond(context->uac2stats.tx.ep.maxPktTime)
        );
        printf("  Last Pkt Size: %u (%u, %u)\n",
            (unsigned)context->uac2stats.tx.ep.lastPktSize,
            (unsigned)context->uac2stats.tx.ep.minPktSize,
            (unsigned)context->uac2stats.tx.ep.maxPktSize
        );
        printf("  Buffer Fill: %u\n", (unsigned)bufferTrackGetFrames(1, context->cfg.usbInChannels));
    }
}

/***********************************************************************
 * CMD: fsck
 **********************************************************************/
const char shell_help_fsck[] = "\n";
const char shell_help_summary_fsck[] = "Check the internal filesystem";

void shell_fsck(int argc, char **argv)
{
   int result;
   int repaired;

   result = romfs_fsck(1, &repaired);

   if (result == FS_OK) {
      printf("Filesystem OK\n");
   } else {
      printf("Filesystem corrupt: %s Repaired\n", repaired ? "" : "Not");
   }
}

/***********************************************************************
 * CMD: update
 **********************************************************************/
#include "flash_map.h"

const char shell_help_update[] = "<app,fs>\n";
const char shell_help_summary_update[] = "Updates the firmware via xmodem";

void shell_update(int argc, char **argv)
{
    char *warn = "Updating the firmware is DANGEROUS - DO NOT REMOVE POWER!";
    uint32_t flashBaseAddr;
    uint32_t flashSize;
    long size;
    struct flashWriteState state;
    const FLASH_INFO *flash;
    bool checkFs;
    p_xm_data_func dataWriteFunc;

    checkFs = false;
    dataWriteFunc = flashDataWrite;

    if (argc > 1) {
        if ((argc == 2) && (strcmp(argv[1], "app") == 0)) {
           flashBaseAddr = APP_OFFSET;
           flashSize = APP_SIZE;
        } else if ((argc == 2) && (strcmp(argv[1], "fs") == 0)) {
           flashBaseAddr = FS_OFFSET;
           flashSize = FS_SIZE;
           checkFs = true;
        }  else {
           printf( "Usage: %s %s", argv[0], shell_help_update);
           return;
        }
    } else {
        flashBaseAddr = APP_OFFSET;
        flashSize = APP_SIZE;
    }

    /* Confirm action */
    if (confirmDanger(warn) == 0) {
        return;
    }

    /* Get a handle to the system flash */
    flash = context->flashHandle;
    if (flash == NULL) {
        printf("Flash not initialized!\n");
        return;
    }

    /* Configure the update */
    state.flash = flash;
    state.addr = flashBaseAddr;
    state.maxAddr = flashBaseAddr + flashSize;
    state.eraseBlockSize = ERASE_BLOCK_SIZE;

    /* Start the update */
    printf( "Start XMODEM transfer now... ");
    size = xmodem_receive(dataWriteFunc, &state);

    /* Wait a bit */
    delay(100);

    /* Display results and erase any remaining space */
    if (size < 0) {
       printf( "XMODEM Error: %ld\n", size);
    } else {
       printf("Received %ld bytes.\n", size);
       if (size > 0) {
          printf("Erasing remaining sectors...\n");
          state.addr = ((state.addr / state.eraseBlockSize) + 1) * state.eraseBlockSize;
          while (state.addr < state.maxAddr) {
            flash_erase(state.flash, state.addr, state.eraseBlockSize);
            state.addr += state.eraseBlockSize;
          }
       }
    }

    if (checkFs) {
       shell_fsck(0, NULL);
    }

    printf("Done.\n");
}

/***********************************************************************
 * CMD: meminfo
 **********************************************************************/
const char shell_help_meminfo[] = "\n";
const char shell_help_summary_meminfo[] = "Displays UMM_MALLOC heap statistics";

#include "umm_malloc_cfg.h"
#include "umm_malloc_heaps.h"
const static char *heapNames[] = UMM_HEAP_NAMES;

void shell_meminfo(int argc, char **argv )
{
    UMM_HEAP_INFO ummHeapInfo;
    int i;
    int ok;

    for (i = 0; i < UMM_NUM_HEAPS; i++) {
        printf("Heap %s Info:\n", heapNames[i]);
        ok = umm_integrity_check((umm_heap_t)i);
        if (ok) {
            umm_info((umm_heap_t)i, &ummHeapInfo, NULL, 0);
            printf("  Entries: Total  %8i, Allocated %8i, Free %8i\n",
                ummHeapInfo.totalEntries,
                ummHeapInfo.usedEntries,
                ummHeapInfo.freeEntries
            );
            printf("   Blocks: Total  %8i, Allocated %8i, Free %8i\n",
                ummHeapInfo.totalBlocks,
                ummHeapInfo.usedBlocks,
                ummHeapInfo.freeBlocks
            );
            printf("   Contig: Blocks %8i,     Bytes %8i\n",
                ummHeapInfo.maxFreeContiguousBlocks,
                ummHeapInfo.maxFreeContiguousBlocks * umm_block_size()
            );
        }
        printf("  Heap Integrity: %s\n", ok ? "OK" : "Corrupt");
    }
}

/***********************************************************************
 * CMD: test
 **********************************************************************/
const char shell_help_test[] = "\n";
const char shell_help_summary_test[] = "Test command";

void shell_test(int argc, char **argv )
{
}

/***********************************************************************
 * CMD: reset
 **********************************************************************/
#include "init.h"

const char shell_help_reset[] = "\n";
const char shell_help_summary_reset[] = "Resets the system";

void shell_reset(int argc, char **argv)
{
    printf("Resetting...\n");
    delay(100);
    system_reset(context);
}

/***********************************************************************
 * CMD: route
 **********************************************************************/
const char shell_help_route[] = "[ <idx> <src> <src offset> <dst> <dst offset> <channels> ]\n"
    "  idx        - Routing index\n"
    "  src        - Source stream\n"
    "  src offset - Source stream offset\n"
    "  dst        - Destination stream\n"
    "  dst offset - Destination stream offset\n"
    "  channels   - Number of channels\n"
    " Valid Streams\n"
    "  usb        - USB Audio\n"
    "  a2b        - A2B Audio\n"
    "  codec      - Analog TRS line in/out\n"
    "  off        - Turn off the stream\n"
    " No arguments\n"
    "  Show routing table\n"
    " Single 'clear' argument\n"
    "  Clear routing table\n";
const char shell_help_summary_route[] = "Configures the audio routing table";

static char *stream2str(int streamID)
{
    char *str = "NONE";

    switch (streamID) {
        case IPC_STREAMID_UNKNOWN:
            str = "NONE";
            break;
        case IPC_STREAMID_CODEC_IN:
            str = "CODEC_IN";
            break;
        case IPC_STREAMID_CODEC_OUT:
            str = "CODEC_OUT";
            break;
        case IPC_STREAMID_A2B_IN:
            str = "A2B_IN";
            break;
        case IPC_STREAMID_A2B_OUT:
            str = "A2B_OUT";
            break;
        case IPC_STREAMID_USB_RX:
            str = "USB_RX";
            break;
        case IPC_STREAMID_USB_TX:
            str = "USB_TX";
            break;
        default:
            str = "UNKNOWN";
            break;
    }

    return(str);
}

int str2stream(char *stream, bool src)
{
    if (strcmp(stream, "usb") == 0) {
        return(src ? IPC_STREAMID_USB_RX : IPC_STREAMID_USB_TX);
    } else if (strcmp(stream, "codec") == 0) {
        return(src ? IPC_STREAMID_CODEC_IN : IPC_STREAMID_CODEC_OUT);
    } else if (strcmp(stream, "a2b") == 0) {
        return(src ? IPC_STREAMID_A2B_IN : IPC_STREAMID_A2B_OUT);
    } else if (strcmp(stream, "off") == 0) {
        return(IPC_STREAMID_UNKNOWN);
    }

    return(IPC_STREAM_ID_MAX);
}

void shell_route(int argc, char **argv)
{
    IPC_MSG_ROUTING *routeInfo = (IPC_MSG_ROUTING *)&context->routingMsg->routes;
    ROUTE_INFO *route;
    unsigned i;
    unsigned idx, srcOffset, sinkOffset, channels;
    int srcID, sinkID;

    if (argc == 1) {
        printf("Audio Routing\n");
        for (i = 0; i < routeInfo->numRoutes; i++) {
            route = &routeInfo->routes[i];
            printf(" [%02d]: %s[%u] -> %s[%u], CHANNELS: %u\n",
                i,
                stream2str(route->srcID), route->srcOffset,
                stream2str(route->sinkID), route->sinkOffset,
                route->channels
            );
        }
        return;
    } else if (argc == 2) {
        if (strcmp(argv[1], "clear") == 0) {
            for (i = 0; i < routeInfo->numRoutes; i++) {
                route = &routeInfo->routes[i];
                /* Configure the route atomically in a critical section */
                taskENTER_CRITICAL();
                route->srcID = IPC_STREAMID_UNKNOWN;
                route->srcOffset = 0;
                route->sinkID = IPC_STREAMID_UNKNOWN;
                route->sinkOffset = 0;
                route->channels = 0;
                taskEXIT_CRITICAL();
            }
        }
    }

    /* Confirm a valid route index */
    idx = atoi(argv[1]);
    if (idx > routeInfo->numRoutes) {
        printf("Invalid idx\n");
        return;
    }
    route = &routeInfo->routes[idx];
    srcID = route->srcID;
    srcOffset = route->srcOffset;
    sinkID = route->sinkID;
    sinkOffset = route->sinkOffset;
    channels = route->channels;

    /* Gather the source info */
    if (argc >= 3) {
        srcID = str2stream(argv[2], true);
        if (srcID == IPC_STREAM_ID_MAX) {
            printf("Invalid src\n");
            return;
        }
    }
    if (argc >= 4) {
        srcOffset = atoi(argv[3]);
    }

    /* Gather the sink info */
    if (argc >= 5) {
        sinkID = str2stream(argv[4], false);
        if (sinkID == IPC_STREAM_ID_MAX) {
            printf("Invalid sink\n");
            return;
        }
    }
    if (argc >= 6) {
        sinkOffset = atoi(argv[5]);
    }

    /* Get the number of channels */
    if (argc >= 7) {
        channels = atoi(argv[6]);
    }

    /* Configure the route atomically in a critical section */
    taskENTER_CRITICAL();
    route->srcID = srcID;
    route->srcOffset = srcOffset;
    route->sinkID = sinkID;
    route->sinkOffset = sinkOffset;
    route->channels = channels;
    taskEXIT_CRITICAL();
}
