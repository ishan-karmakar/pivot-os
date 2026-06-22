#pragma once

// #include <inttypes.h> is messed up on zig translate-c so we just define the necessary defines right here
// TODO: Move to separate file
# if __WORDSIZE == 64
#  define __PRI64_PREFIX	"l"
#  define __PRIPTR_PREFIX	"l"
# else
#  define __PRI64_PREFIX	"ll"
#  define __PRIPTR_PREFIX
# endif

# define PRIu8		"u"
# define PRIu16		"u"
# define PRIu32		"u"
# define PRIu64		__PRI64_PREFIX "u"

# define PRId8		"d"
# define PRId16		"d"
# define PRId32		"d"
# define PRId64		__PRI64_PREFIX "d"

# define PRIx8		"x"
# define PRIx16		"x"
# define PRIx32		"x"
# define PRIx64		__PRI64_PREFIX "x"

# define PRIdPTR	__PRIPTR_PREFIX "d"
# define PRIiPTR	__PRIPTR_PREFIX "i"
# define PRIoPTR	__PRIPTR_PREFIX "o"
# define PRIuPTR	__PRIPTR_PREFIX "u"
# define PRIxPTR	__PRIPTR_PREFIX "x"
# define PRIXPTR	__PRIPTR_PREFIX "X"

#define X8_F  "02" PRIx8
#define U16_F PRIu16
#define S16_F PRId16
#define X16_F PRIx16
#define U32_F PRIu32
#define S32_F PRId32
#define X32_F PRIx32
#define SZT_F PRIuPTR

#define LWIP_PLATFORM_DIAG(x) lwip_print x
#define LWIP_PLATFORM_ASSERT(x) lwip_assert(x)
#define LWIP_NO_INTTYPES_H 1
#define LWIP_NO_CTYPE_H 1

void lwip_assert(const char *str);
static void lwip_print(const char *str, ...) {
    lwip_assert(str);
}