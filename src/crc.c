

#include "crc.h"

U16 crc_init()
{
  return 0xFFFFU;
}

U16 crc_calc_byte(U8 byte, U16 crc)
{
  int i;
  int xor_flag;

  /* For each bit in the data byte, starting from the leftmost bit */
  for (i = 7; i >= 0; i--) {
    /* If leftmost bit of the CRC is 1, we will XOR with
     * the polynomial later */
    xor_flag = crc & 0x8000;

    /* Shift the CRC, and append the next bit of the
     * message to the rightmost side of the CRC */
    crc <<= 1;
    crc |= (byte & (1 << i)) ? 1 : 0;

    /* Perform the XOR with the polynomial */
    if (xor_flag)
      crc ^= 0x1021;
  }

  return crc;
}

U16 crc_calc(U8 const *mem, U16 len)
{
  U8 const *b = mem;
  U16 i;
  U16 crc;

  crc = crc_init();

  for (i = 0; i < len; i++)
  {
    crc = crc_calc_byte(b[i], crc);
  }

  crc = crc_calc_byte(0, crc);

  return crc;
}

