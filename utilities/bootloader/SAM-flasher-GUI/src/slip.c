/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 *
 * Portions of this code were taken from RFC 1055, Serial Line IP,
 * June 1988.
 *
 */

/* SLIP special character codes
*/
#define END             0300    /* indicates end of packet */
#define ESC             0333    /* indicates byte stuffing */
#define ESC_END         0334    /* ESC ESC_END means END data byte */
#define ESC_ESC         0335    /* ESC ESC_ESC means ESC data byte */

void send_char(unsigned char c, unsigned char **pOut)
{
    **pOut = c;
    (*pOut)++;
}

int slip(unsigned char *pIn, int lenIn, unsigned char *pOut, int lenOut)
{

    unsigned char *pOutOrig = pOut;

    /* send an initial END character to flush out any data that may
     * have accumulated in the receiver due to line noise
     */
    send_char(END, &pOut);

    /* for each byte in the packet, send the appropriate character
     * sequence
     */
    while(lenIn--) {

           switch(*pIn) {
               /* if it's the same code as an END character, we send a
                * special two character code so as not to make the
                * receiver think we sent an END
                */
               case END:
                   send_char(ESC, &pOut);
                   send_char(ESC_END, &pOut);
                   break;

               /* if it's the same code as an ESC character,
                * we send a special two character code so as not
                * to make the receiver think we sent an ESC
                */
               case ESC:
                   send_char(ESC, &pOut);
                   send_char(ESC_ESC, &pOut);
                   break;

               /* otherwise, we just send the character
                */
               default:
                   send_char(*pIn, &pOut);
           }

           pIn++;
    }

    /* tell the receiver that we're done sending the packet
    */
    send_char(END, &pOut);

    return((int)(pOut - pOutOrig));
}

int unslip(unsigned char *pIn, int lenIn, int *offsetIn, unsigned char *pOut, int lenOut, int *offsetOut)
{
    static int decoding = 0;
    static int escape = 0;

    unsigned char c;
    int inOffset;
    int outOffset;
    int ret;
    int exit;

    inOffset = *offsetIn;
    outOffset = *offsetOut;
    ret = 0;
    exit = 0;

    while ((inOffset < lenIn) && (exit == 0)) {

        c = pIn[inOffset];
        inOffset++;

        switch(c) {

            case END:
                if (decoding == 0) {
                    decoding = 1;
                } else {
                    if (outOffset > 0) {
                        decoding = 0;
                        exit = 1;
                        ret = 1;
                    }
                }
                break;

            case ESC:
                if (decoding) {
                    escape = 1;
                }
                break;

            default:
                if (decoding) {
                    if (escape) {
                        switch (c) {
                            case ESC_END:
                                c = END;
                                break;
                            case ESC_ESC:
                                c = ESC;
                                break;
                        }
                    }
                    if (outOffset < lenOut) {
                        pOut[outOffset] = c;
                        outOffset++;
                    }
                    escape = 0;
                }
                break;
        }
    }

    *offsetIn = inOffset;
    *offsetOut = outOffset;

    return(ret);
}
