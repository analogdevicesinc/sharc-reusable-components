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

#include "uac2_descriptors.h"

uac2_input_terminal_descriptor *newInputTerminalDescriptor (
    uint8_t bTerminalID, uint16_t wTerminalType,
    uint8_t bNRChannels, uint8_t bCSourceID
)
{
    uac2_input_terminal_descriptor *it;

    it = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*it));

    it->bLength = sizeof(*it);
    it->bDescriptorType = UAC_CS_INTERFACE;
    it->bDescriptorSubtype = UAC_INPUT_TERMINAL;
    it->bTerminalID = bTerminalID;
    it->wTerminalType = wTerminalType;
    it->bCSourceID = bCSourceID;
    it->bNrChannels = bNRChannels;
    it->bmChannelConfig = 0x00000003;

    /* Fixed / Unsuported values */
    it->bAssocTerminal = 0x00;
    it->iChannelNames = 0x00;
    it->bmControls = 0x0000;
    it->iTerminal = 0x00;

    return(it);
}

uac2_output_terminal_descriptor *newOutputTerminalDescriptor (
    uint8_t bTerminalID, uint16_t wTerminalType,
    uint8_t bSourceID, uint8_t bCSourceID
)
{
    uac2_output_terminal_descriptor *ot;

    ot = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*ot));

    ot->bLength = sizeof(*ot);
    ot->bDescriptorType = UAC_CS_INTERFACE;
    ot->bDescriptorSubtype = UAC_OUTPUT_TERMINAL;
    ot->bTerminalID = bTerminalID;
    ot->wTerminalType = wTerminalType;
    ot->bSourceID = bSourceID;
    ot->bCSourceID = bCSourceID;

    /* Fixed / Unsuported values */
    ot->bAssocTerminal = 0x00;
    ot->bmControls = 0x0000;
    ot->iTerminal = 0x00;

    return(ot);
}

uac2_feature_unit_descriptor *newFeatureUnitDescriptor (
    uint8_t bUnitID, uint16_t bSourceID,
    uint8_t bNRChannels, bool readOnly
)
{
    uac2_feature_unit_descriptor *fu;
    uint8_t length;
    uint8_t *iFeature;
    int i;

    length = sizeof(*fu) + 1 + (bNRChannels + 1) * sizeof(uint32_t);
    fu = UAC20_DESCRIPTORS_CALLOC(1, length);

    fu->bLength = length;
    fu->bDescriptorType = UAC_CS_INTERFACE;
    fu->bDescriptorSubtype = UAC_FEATURE_UNIT;
    fu->bUnitID = bUnitID;
    fu->bSourceID = bSourceID;

    fu->bmaControls[0] = 0x00000000;
    for (i = 0; i < bNRChannels; i++) {
        fu->bmaControls[ 1 + i ] = readOnly ? 0x05 : 0x0f;
    }

    /* Fixed / Unsuported values */
    iFeature = (uint8_t *)&fu->bmaControls[ bNRChannels + 1 ];
    *iFeature = 0x00;

    return(fu);
}

uac2_clock_source_descriptor *newClockSourceDescriptor (
    uint8_t bClockID
)
{
    uac2_clock_source_descriptor *cs;

    cs = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*cs));

    cs->bLength = sizeof(*cs);
    cs->bDescriptorType = UAC_CS_INTERFACE;
    cs->bDescriptorSubtype = UAC_CLOCK_SOURCE;
    cs->bClockID = bClockID;

    /* Fixed / Unsuported values */
    cs->bmAttributes = UAC_CLOCK_SOURCE_TYPE_INT_FIXED;
    cs->bmControls = UAC_CLOCK_SOURCE_CONTROL_READ_ONLY;
    cs->bAssocTerminal = 0;
    cs->iClockSource = 0;

    return(cs);
}

uac2_format_type_i_descriptor *newFormatTypeIDescriptor (
    uint8_t bBitResolution
)
{
    uac2_format_type_i_descriptor *d;

    d = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*d));

    d->bLength = sizeof(*d);
    d->bDescriptorType = UAC_CS_INTERFACE;
    d->bDescriptorSubtype = UAC_FORMAT_TYPE;
    d->bFormatType = 0x01;

    if (bBitResolution <= 8) {
        d->bSubslotSize = 1;
    } else if (bBitResolution <= 16) {
        d->bSubslotSize = 2;
    } else {
        d->bSubslotSize = 4;
    }

    d->bBitResolution = bBitResolution;

    return(d);
}

CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor
*newAudioStreamEndpointDescriptor(
    uint8_t bLockDelayUnits, uint8_t wLockDelay
)
{
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor *d;

    d = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*d));

    d->b_length = sizeof(CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor);
    d->b_descriptor_type = UAC_CS_ENDPOINT;  /* Class Specific Endpoint */
    d->b_descriptor_subtype = 0x01;          /* Endpoint - General */
    d->bm_attributes = 0x00;                 /* max packet only set to 0 */
    d->bm_controls = 0x00;
    d->b_lock_delay_units = bLockDelayUnits;
    d->w_lock_delay = wLockDelay;

    return(d);
}

/* Convert raw USB (micro)frame period to USB bInterval where
 *   Period = 2 ** (bInterval - 1)
 * USB 2.0 Spec, 9.6.6 Endpoint, Table 9-13 */
static uint8_t bIntervalLimit(uint32_t periodIn, bool highSpeed)
{
    uint32_t bInterval;
    uint32_t period;

    bInterval = 1;
    while (bInterval < 16) {
        period = 1 << bInterval;
        if (period > periodIn) {
            break;
        }
        bInterval++;
    }

    /* Intervals greater than 4 cause problems under Windows */
    if (bInterval > 4) {
        bInterval = 4;
    }

    return(bInterval);
}

void freeStreamInterfaceParams(CLD_SC58x_Audio_2_0_Stream_Interface_Params *p)
{
    if (p) {
        if (p->p_format_descriptor) {
            UAC20_DESCRIPTORS_FREE(p->p_format_descriptor);
        }
        if (p->p_audio_stream_endpoint_data_descriptor) {
            UAC20_DESCRIPTORS_FREE(p->p_audio_stream_endpoint_data_descriptor);
        }
        UAC20_DESCRIPTORS_FREE(p);
    }
}

uint16_t calcMaxPktSize(uint16_t maxPktSize, uint16_t pktRate,
    uint32_t sampleRate, uint16_t frameSize,
    bool lowLatency, uint8_t *bInterval)
{
    uint16_t framesPerPkt;
    uint16_t pktSize;

    /* Calculate frames per USB packet */
    framesPerPkt = sampleRate / pktRate;

    /* Calculate the packet size for a single interval */
    pktSize = framesPerPkt * frameSize;
    *bInterval = 1;

    /* Stretch the intervals to better utilize a full USB packet if
     * latency is not a priority.  Linux requires the isochronous audio
     * bInterval to be less than or equal to 4.
     */
    if (!lowLatency) {
        while ((*bInterval < 4) && ((2 * pktSize) <= (maxPktSize - frameSize))) {
            pktSize *= 2;
            (*bInterval)++;
        }
    }

    pktSize += frameSize;

    /* If this code ever fires, there will be bandwidth problems so return
     * zero as the max packet size as an error indicator.
     */
    if (pktSize > maxPktSize) {
        pktSize = 0;
    }

    return(pktSize);
}

CLD_SC58x_Audio_2_0_Stream_Interface_Params *newStreamInterfaceParams(
    uint8_t endpointNumber, uint8_t bTerminalID,
    uint16_t *minPacketSizeFull, uint16_t *maxPacketSizeFull,
    uint16_t *minPacketSizeHigh, uint16_t *maxPacketSizeHigh,
    uint32_t sampleRate, uint8_t bNRChannels, bool lowLatency,
    uac2_format_type_i_descriptor *formatDescriptor,
    CLD_SC58x_Audio_2_0_Audio_Stream_Data_Endpoint_Descriptor *endpointDescriptor
)
{
    CLD_SC58x_Audio_2_0_Stream_Interface_Params *i;
    uint16_t frameSize;

    i = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*i));

    /* Calculate the frame size in bytes */
    frameSize = formatDescriptor->bSubslotSize * bNRChannels;

    /* Calculate max packet size based off of the latency priority */
    i->max_packet_size_full_speed = calcMaxPktSize(
        1023, 1000, sampleRate, frameSize,
        lowLatency, &i->b_interval_full_speed
    );
    i->max_packet_size_high_speed = calcMaxPktSize(
        1024, 8000, sampleRate, frameSize,
        lowLatency, &i->b_interval_high_speed
    );

    /* Set members */
    i->endpoint_number = endpointNumber;
    i->b_terminal_link = bTerminalID;
    i->b_format_type = 1;                      /* Type 1 Format */
    i->bm_formats  = 0x00000001;               /* Type 1 - PCM format */
    i->b_nr_channels = bNRChannels;
    i->bm_channel_config = 0x00000003;         /* L/R Spatial channels */
    i->p_encoder_descriptor = CLD_NULL;
    i->p_decoder_descriptor = CLD_NULL;
    i->p_format_descriptor = (unsigned char *)formatDescriptor;
    i->p_audio_stream_endpoint_data_descriptor = endpointDescriptor;

    /* Return min/max packet sizes if requested */
    if (maxPacketSizeFull) {
        *maxPacketSizeFull = i->max_packet_size_full_speed;
    }
    if (minPacketSizeFull) {
        if (i->max_packet_size_full_speed) {
            *minPacketSizeFull = i->max_packet_size_full_speed - 2 * frameSize;
        } else {
            *minPacketSizeFull = 0;
        }
    }
    if (maxPacketSizeHigh) {
        *maxPacketSizeHigh = i->max_packet_size_high_speed;
    }
    if (minPacketSizeHigh) {
        if (i->max_packet_size_high_speed) {
            *minPacketSizeHigh = i->max_packet_size_high_speed - 2 * frameSize;
        } else {
            *minPacketSizeHigh = 0;
        }
    }

    return(i);
}

CLD_SC58x_Audio_2_0_Rate_Feedback_Params *newRateFeedbackParams(
    uint32_t updateMs
)
{
    CLD_SC58x_Audio_2_0_Rate_Feedback_Params *f;

    f = UAC20_DESCRIPTORS_CALLOC(1, sizeof(*f));

    f->b_interval_full_speed = bIntervalLimit(updateMs, false);
    f->b_interval_high_speed = bIntervalLimit(updateMs*8, true);

    /* Fixed / Unsuported values */
    f->max_packet_size_full_speed = 32;
    f->max_packet_size_high_speed = 32;

    return(f);
}
