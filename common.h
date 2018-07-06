
#ifndef PSBT_COMMON_H
#define PSBT_COMMON_H

#include <stdint.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef uint64_t u64;
typedef unsigned int u32;

static const unsigned int MAX_SERIALIZE_SIZE = 0x02000000;

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)


#endif /* PSBT_COMMON_H */
