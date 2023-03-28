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

#ifndef _uac2_descriptors_h
#define _uac2_descriptors_h

#include <stdint.h>
#include <stdbool.h>

#include "uac2_descriptors_cfg.h"
#include "cld_lib.h"

/* Input terminal types */
#define UAC_TERMINAL_TYPE_USB_STREAMING     0x0101
#define UAC_TERMINAL_TYPE_MICROPHONE        0x0201
#define UAC_TERMINAL_TYPE_GENERIC_DIGITAL   0x0602

/* Output terminal types */
#define UAC_TERMINAL_TYPE_SPEAKER           0x0301

/* Audio Class Specific Interface Descriptors */
#define UAC_CS_INTERFACE                    0x24
#define UAC_CS_ENDPOINT                     0x25

/* Audio Class Specific Interface Descriptor Subtypes */
#define UAC_INPUT_TERMINAL                  0x02
#define UAC_OUTPUT_TERMINAL                 0x03
#define UAC_FEATURE_UNIT                    0x06
#define UAC_CLOCK_SOURCE                    0x0A

/* Clock source types */
#define UAC_CLOCK_SOURCE_TYPE_INT_FIXED     0x01

/* Clock source controls */
#define UAC_CLOCK_SOURCE_CONTROL_READ_ONLY  0x01

/* Audio Class Specific Interface Descriptor Subtypes */
#define UAC_FORMAT_TYPE                     0x02

/* Feature Unit Control types */
#define UAC_FU_MUTE_CONTROL                 0x01
#define UAC_FU_VOLUME_CONTROL               0x02

#pragma pack(1)

/* Input terminal descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bTerminalID;
    uint16_t wTerminalType;
    uint8_t bAssocTerminal;
    uint8_t bCSourceID;
    uint8_t bNrChannels;
    uint32_t bmChannelConfig;
    uint8_t iChannelNames;
    uint16_t bmControls;
    uint8_t iTerminal;
} uac2_input_terminal_descriptor;

/* Output terminal descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bTerminalID;
    uint16_t wTerminalType;
    uint8_t bAssocTerminal;
    uint8_t bSourceID;
    uint8_t bCSourceID;
    uint16_t bmControls;
    uint8_t iTerminal;
} uac2_output_terminal_descriptor;

/* Clock source descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bClockID;
    uint8_t bmAttributes;
    uint8_t bmControls;
    uint8_t bAssocTerminal;
    uint8_t iClockSource;
} uac2_clock_source_descriptor;

/* Feature unit descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bUnitID;
    uint8_t bSourceID;
    uint32_t bmaControls[0];
} uac2_feature_unit_descriptor;

/* Format type I descriptor */
typedef struct {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDescriptorSubtype;
    uint8_t bFormatType;
    uint8_t bSubslotSize;
    uint8_t bBitResolution;
} uac2_format_type_i_descriptor;

/* Structure used to report 32-bit USB Audio 2.0 range values */
typedef struct {
    uint16_t  wNumSubRanges;
    struct {
        uint32_t   wMIN;
        uint32_t   wMAX;
        uint32_t   wRES;
    } sub_ranges[1];
} uac2_4_byte_control_range_parameter_block;

/* Structure used to report 16-bit USB Audio 2.0 range values */
typedef struct
{
    uint16_t  wNumSubRanges;
    struct
    {
        uint16_t  wMIN;
        uint16_t  wMAX;
        uint16_t  wRES;
    } sub_ranges[1];
} uac2_2_byte_control_range_parameter_block;

/* Structure used to report 8-bit USB Audio 2.0 range values */
typedef struct
{
    uint16_t  wNumSubRanges;
    struct
    {
        uint8_t  wMIN;
        uint8_t  wMAX;
        uint8_t  wRES;
    } sub_ranges[1];
} uac2_1_byte_control_range_parameter_block;

#pragma pack()

#ifdef __cplusplus
extern "C"{
#endif

uac2_input_terminal_descriptor *newInputTerminalDescriptor (
    uint8_t bTerminalID, uint16_t wTerminalType,
    uint8_t bNRChannels, uint8_t bCSourceID
);

uac2_output_terminal_descriptor *newOutputTerminalDescriptor (
    uint8_t bTerminalID, uint16_t wTerminalType,
    uint8_t bSourceID, uint8_t bCSourceID
);

uac2_feature_unit_descriptor *newFeatureUnitDescriptor (
    uint8_t bUnitID, uint16_t bSourceID,
    uint8_t bNRChannels, bool readOnly
);

uac2_clock_source_descriptor *newClockSourceDescriptor (
    uint8_t bClockID
);

uac2_format_type_i_descriptor *newFormatTypeIDescriptor (
    uint8_t bBitResolution
);

CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor
*newAudioStreamEndpointDescriptor(
    uint8_t bLockDelayUnits, uint8_t wLockDelay
);

CLD_SC58x_Audio_2_0_Stream_Interface_Params *newStreamInterfaceParams(
    uint8_t endpointNumber, uint8_t bTerminalID,
    uint16_t *minPacketSizeFull, uint16_t *maxPacketSizeFull,
    uint16_t *minPacketSizeHigh, uint16_t *maxPacketSizeHigh,
    uint32_t sampleRate, uint8_t bNRChannels, bool lowLatency,
    uac2_format_type_i_descriptor *formatDescriptor,
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor *endpointDescriptor
);

CLD_SC58x_Audio_2_0_Rate_Feedback_Params *newRateFeedbackParams(
    uint32_t updateMs
);

void freeStreamInterfaceParams(
    CLD_SC58x_Audio_2_0_Stream_Interface_Params *p
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
