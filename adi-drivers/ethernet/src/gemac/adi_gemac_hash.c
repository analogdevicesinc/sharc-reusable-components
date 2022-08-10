
/*********************************************************************************
 *
 * Copyright(c) 2011 Analog Devices, Inc. All Rights Reserved.
 *
 * This software is proprietary and confidential.By using this software you agree
 * to the terms of the associated Analog Devices License Agreement.
 *
 * Description: ADSP-BF560x Ethernet controller driver
 *
 **********************************************************************************/
/*
 *
 *  Description : This files can be used to generate
 *
 *          CRC32 of the Destination ethernet address for Hash Table population.
 *
 *          When GMAC is configured for Hash filtering for Destination ethernet
 *          addresses (HPF HMC HUC in Mac Frame Filter), Hash Table High register
 *          and Hash Table Low registers are used for group address filtering.
 *          Upper 6 bits of computed CRC is used for Filtering.
 *          MS bit of this 6 bits is used for selecting Hash table high register
 *          (if it is 1)or Hash table low register(if it is zero)
 *          Remaining LS 5 bits are used as index to the bit position in the
 *          selected hash table register
 *
 *
 *
 */
#include "adi_gemac_int.h"

#define FALSE   0
#define TRUE    !FALSE

typedef unsigned long  crc;

#define CRC_NAME        "CRC-32"
#define POLYNOMIAL      0x04C11DB7
#define INITIAL_REMAINDER   0xFFFFFFFF
#define FINAL_XOR_VALUE     0xFFFFFFFF
#define REVERSE_DATA        TRUE
#define REVERSE_REMAINDER   FALSE

#define WIDTH    (8 * sizeof(crc))
#define TOPBIT   (1ul << (WIDTH - 1))
#if (REVERSE_DATA == TRUE)
#undef  REVERSE_DATA
#define REVERSE_DATA(X)     ((unsigned char) reverse((X), 8))
#else
#undef  REVERSE_DATA
#define REVERSE_DATA(X)     (X)
#endif

#if (REVERSE_REMAINDER == TRUE)
#undef  REVERSE_REMAINDER
#define REVERSE_REMAINDER(X)    ((crc) reverse((X), WIDTH))
#else
#undef  REVERSE_REMAINDER
#define REVERSE_REMAINDER(X)    (X)
#endif

#define IS_HASHBIN_HIGH(hash)  ((hash) & (0x20))
#define HASHBIN_INDEX(hash) ((hash) & (0x1F))

static crc crcTable[256];

/*
 * Reverse the data
 *
 * Input1: Data to be reversed
 * Input2: number of bits in the data
 * Output: The reversed data
 *
 */

unsigned long reverse(unsigned long data, unsigned char nBits)
{
    unsigned long  reversed = 0x00000000;
    unsigned char  bit;

    /*
     * Reverse the data about the center bit.
     */
    for (bit = 0; bit < nBits; ++bit)
    {
        /*
         * If the LSB bit is set, set the reflection of it.
         */
        if (data & 0x01)
        {
            reversed |= (1 << ((nBits - 1) - bit));
        }

        data = (data >> 1);
    }
    return (reversed);
}

/*
 * This Initializes the partial CRC look up table
 */

void initCrcTable(void)
{
    crc  remainder;
    int  dividend;
    unsigned char  bit;


    /*
     * Compute the remainder of each possible dividend.
     */
    for (dividend = 0; dividend < 256; ++dividend)
    {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = (crc)(dividend << (WIDTH - 8));

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        crcTable[dividend] = remainder;
    }

}

crc computeCrc(unsigned int const message[], int nBytes)
{
    crc remainder = INITIAL_REMAINDER;
    unsigned char  data;
    int            byte;

    /*
     * Divide the message by the polynomial, a byte at a time.
     */
    for (byte = 0; byte < nBytes; ++byte)
    {
        data = REVERSE_DATA(message[byte]) ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    /*
     * The final remainder is the CRC.
     */
    return (REVERSE_REMAINDER(remainder) ^ FINAL_XOR_VALUE);

}

static unsigned int GetMultiCastHashBin(char *pMultiCastMac,const int length,int *pBin)
{
    uint32_t hash;
    unsigned long int computed_crc;
    static bool crt_initialized=false;

    if(!crt_initialized)
    {
        initCrcTable();
        crt_initialized = true;
    }

    computed_crc = computeCrc((unsigned int*)pMultiCastMac,length);
    hash  = (computed_crc >> 26) & 0x1F;
    *pBin = (computed_crc >> 31) & 0x1;
    return (hash);
}

/**
 * @brief       Compute multicast MAC address from the Multicast IP Address
 *
 * @details     Multicast MAC address is computed from the multicast IP address
 *
 * @param [in]  GroupIpAddress   Multicast Group IP address
 *
 * @param [out] pMultiCastMac    Memory address where multicast MAC address is stored
 *                               Supplied memory should be six bytes long
 *
 * @return      Status
 *                      - ADI_ETHER_RESULT_SUCCESS  successfully opened the device
 *
 * @note        Internal Driver function.
 */
static void GetMultiCastMAC(uint32_t GroupIpAddress, char *pMultiCastMac)
{
    char *p=(char*)&GroupIpAddress;

   /* compute the destination multi cast mac address */
   memset(pMultiCastMac,0,6);

   /* copy the ip address */
   memcpy((pMultiCastMac+2),p,4);

   /* convert the incoming group ip address to multi cast mac address
   * 0-23 bits are fixed we get the rest from the group ipaddress
   */
   *(pMultiCastMac + 0) = 0x01;
   *(pMultiCastMac + 1) = 0x00;
   *(pMultiCastMac + 2) = 0x5E;
   *(pMultiCastMac + 3) &= 0x7F;

   return;
}

/**
 * @brief       Adds multicast address filter
 *
 * @details     Multicast MAC address is derived from the multicast group ip address
 *              CRC32 is computed on the multicast MAC address. Upper 6 bits of the
 *              CRC32 is used to configure the apporopriate hash-bin register.
 *
 * @param [in] phDevice        Device Handle.
 *
 * @param [in] GroupIpAddress  Multicast group IP address
 *
 * @param [in] bAddAddress     If true adds the multicast filter if fasle deletes the
 *                             multicast filter
 * @return      Status
 *                      -ADI_ETHER_RESULT_SUCCESS  successfully opened the device
 *
 * @note        Internal Driver function.
 */
ADI_ETHER_RESULT add_multicastmac_filter(ADI_ETHER_HANDLE * const phDevice,
                                         uint32_t GroupIpAddress,
                                         bool bAddAddress)
{
    ADI_EMAC_DEVICE*    const  pDev      = (ADI_EMAC_DEVICE*)phDevice;
    ADI_EMAC_REGISTERS* const  pEmacRegs = pDev->pEMAC_REGS;
    char MultiCastMac[6];
    int HashBinIndex;
    int Bin=-1;


    /* Get the multicast MAC address for the given address */
    GetMultiCastMAC(GroupIpAddress,&MultiCastMac[0]);

    /* Get the hash index for the multicast address */
    HashBinIndex = GetMultiCastHashBin((char*)&MultiCastMac[0],6,&Bin);

    // FIXIT Check whether we ever get incorrect index?
    if(bAddAddress)
    {
        pDev->MulticastBinCount[HashBinIndex] += 1;

        // hash index is in HASHHI register
        if(Bin == 0x1)
        {
            pEmacRegs->EMAC_HASHTBL_HI |= (1UL << HashBinIndex);
        }
        else
            pEmacRegs->EMAC_HASHTBL_LO |= (1UL << HashBinIndex);
    }
    else /* remove the address */
    {
        pDev->MulticastBinCount[HashBinIndex] -= 1;

        if(pDev->MulticastBinCount[HashBinIndex]==0)
        {
            /* HASHHI bit */
            if(Bin == 0x1)
            {
                pEmacRegs->EMAC_HASHTBL_HI &= ~(1UL << HashBinIndex);
            }
            else
                pEmacRegs->EMAC_HASHTBL_LO  &= ~(1UL << HashBinIndex);
        }
    }

    return(ADI_ETHER_RESULT_SUCCESS);
}
