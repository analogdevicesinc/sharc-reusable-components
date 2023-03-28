# simple-services/adi-a2b-cmdlist

## Overview

The adi-a2b-cmdlist service discovers an A2B network using a SigmaStudio A2B exported network configuration.

This component replaces the 'a2b-ad2425' component.

## Required components

None

## Recommended components

- a2b-xml
- umm_malloc
- twi simple-driver

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project
- Copy the 'inc' directory into a project include directory.  The header files in the 'inc' directory contain the configurable options for the adi-a2b-cmdlist service.

## Configure

The adi-a2b-cmdlist service has a number of convenient compile-time configuration options.  See 'inc/ adi_a2b_cmdlist_cfg.h' for the tunable options.

## Run

- See the example below

## Example

```C
#define AD2425_DEFAULT_I2C_ADDR  (0x68)
#define AD2425_I2C_ADDR          (0x6A)

static ADI_A2B_CMDLIST_RESULT discover_twi_write(
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *usr)
{
    TWI_SIMPLE_RESULT twiResult;
    twiResult = twi_write(twiHandle, address, out, outLen);
    return (twiResult == TWI_SIMPLE_SUCCESS ?
        ADI_A2B_CMDLIST_SUCCESS : ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR);
}

static ADI_A2B_CMDLIST_RESULT discover_twi_write_read(
    void *twiHandle, uint8_t address,
    void *out, uint16_t outLen, void *in, uint16_t inLen, void *usr)
{
    TWI_SIMPLE_RESULT twiResult;
    twiResult = twi_writeRead(twiHandle, address, out, outLen, in, inLen);
    return (twiResult == TWI_SIMPLE_SUCCESS ?
        ADI_A2B_CMDLIST_SUCCESS : ADI_A2B_CMDLIST_A2B_I2C_WRITE_ERROR);
}

static void discover_delay(uint32_t mS, void *usr)
{
    vTaskDelay(pdMS_TO_TICKS(mS));
}

static uint32_t discover_get_time(void *usr)
{
    return(xTaskGetTickCount() * (1000 / configTICK_RATE_HZ));
}

static void *discover_get_buffer(uint16_t size, void *usr)
{
    return(umm_malloc(size));
}

static void discover_free_buffer(void *buffer, void *usr)
{
    umm_free(buffer);
}

void discover(void) {
    ADI_A2B_CMDLIST_RESULT cmdListResult;
    sTWI *ad2425TwiHandle;
    TWI_SIMPLE_RESULT result;
    void *a2bInitSequence;
    uint32_t a2bIinitLength;
    A2B_CMD_TYPE a2bCmdType;
    ADI_A2B_CMDLIST *list;
    ADI_A2B_CMDLIST_EXECUTE_INFO execInfo;
    ADI_A2B_CMDLIST_SCAN_INFO scanInfo;
    ADI_A2B_CMDLIST_OVERRIDE_INFO overrideInfo;
    ADI_A2B_CMDLIST_CFG cfg = {
        .twiWrite = shell_discover_twi_write,
        .twiWriteRead = shell_discover_twi_write_read,
        .delay = shell_discover_delay,
        .getTime = shell_discover_get_time,
        .getBuffer = shell_discover_get_buffer,
        .freeBuffer = shell_discover_free_buffer,
        .usr = NULL
    };

    /* Initialize the simple TWI driver */
    twiResult = twi_init();

    /* Open up a device handle for TWI0 @ 400KHz */
    twiResult = twi_open(TWI0, &ad2425TwiHandle);
    twi_setSpeed(ad2425TwiHandle, TWI_SIMPLE_SPEED_400);

    //
    //  Initialize 'a2bInitSequence' and 'a2bIinitLength' appropriately
    //  and set 'a2bCmdType' to the proper value.
    //

    /* Open a command list instance */
    cmdListResult = adi_a2b_cmdlist_open(&list, &cfg);

    /* Set the command list */
    cmdListResult =  adi_a2b_cmdlist_set(
        list, AD2425_DEFAULT_I2C_ADDR, a2bInitSequence, a2bIinitLength, a2bCmdType
    );

    /* Clear the overrides */
    memset(&overrideInfo, 0, sizeof(overrideInfo));

    /* Override default SigmaStudio address */
    overrideInfo.masterAddr_override = true;
    overrideInfo.masterAddr = AD2425_I2C_ADDR;

    /* Confirm master I2S/TDM settings and override if they don't match */
    cmdListResult = adi_a2b_cmdlist_scan(
        list, &scanInfo
    );
    if (scanInfo.I2SGCFG_valid && (scanInfo.I2SGCFG != SYSTEM_I2SGCFG)) {
        printf("WARNING: I2SGCFG mismatch (expected %02x, got %02x)\n",
            SYSTEM_I2SGCFG, scanInfo.I2SGCFG);
        printf("         Overriding...\n");
        overrideInfo.I2SGCFG_override = true;
        overrideInfo.I2SGCFG = SYSTEM_I2SGCFG;
    }
    if (scanInfo.I2SCFG_valid && (scanInfo.I2SCFG != SYSTEM_I2SCFG)) {
        printf("WARNING: I2SCFG mismatch (expected %02x, got %02x)\n",
            SYSTEM_I2SCFG, scanInfo.I2SCFG);
        printf("         Overriding...\n");
        overrideInfo.I2SCFG_override = true;
        overrideInfo.I2SCFG = SYSTEM_I2SCFG;
    }

    /* Process any overrides */
    cmdListResult = adi_a2b_cmdlist_override(list, &overrideInfo);

    /* Run the command list */
    cmdListResult = adi_a2b_cmdlist_execute(list, &execInfo);

    /* Print the results */
    printf("A2B config lines processed: %lu\n", execInfo.linesProcessed);
    printf("A2B discovery result: %s\n", execInfo.resultStr);
    printf("A2B nodes discovered: %d\n", execInfo.nodesDiscovered);

    /* Close the command list */
    cmdListResult = adi_a2b_cmdlist_close(&list);

}
```

## Info

- See 'src/adi_a2b_cmdlist.h' for a more detailed API description.
