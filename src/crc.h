
#pragma once

#include "lcp_types.h"

U16 crc_init();
U16 crc_calc_byte(U8, U16);
U16 crc_calc(U8 const*, U16);

