#ifndef _ARCH_CC_H_
#define _ARCH_CC_H_

#include <sys/time.h>

#define LWIP_TIMEVAL_PRIVATE            0
#define LWIP_PROVIDE_ERRNO

#define PACK_STRUCT_FIELD(x)            x
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_BEGIN               _Pragma("pack(1)")
#define PACK_STRUCT_END                 _Pragma("pack()")

#endif /* _ARCH_CC_H_ */
