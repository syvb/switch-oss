/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * secport.h - portability interfaces for security libraries
 */

#ifndef _SECPORT_H_
#define _SECPORT_H_

#include "utilrename.h"
#include "prlink.h"

/*
 * define XP_WIN, XP_BEOS, or XP_UNIX, in case they are not defined
 * by anyone else
 */
#ifndef NN_NINTENDO_SDK
#ifdef _WINDOWS
#ifndef XP_WIN
#define XP_WIN
#endif
#if defined(_WIN32) || defined(WIN32)
#ifndef XP_WIN32
#define XP_WIN32
#endif
#endif
#endif

#ifdef __BEOS__
#ifndef XP_BEOS
#define XP_BEOS
#endif
#endif

#ifdef unix
#ifndef XP_UNIX
#define XP_UNIX
#endif
#endif
#endif    /*  !NN_NINTENDO_SDK  */

#include <sys/types.h>

#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "prtypes.h"
#include "prlog.h" /* for PR_ASSERT */
#include "plarena.h"
#include "plstr.h"

/*
 * HACK for NSS 2.8 to allow Admin to compile without source changes.
 */
#ifndef SEC_BEGIN_PROTOS
#include "seccomon.h"
#endif

/*
 * The PORT_*Arena* function signatures mostly involve PLArenaPool* arguments.
 * But this is misleading! It's not actually safe to use vanilla PLArenaPools
 * with them. There are two "subclasses" of PLArenaPool that should be used
 * instead.
 *
 * - PORTArenaPool (defined in secport.c): this "subclass" is always
 *   heap-allocated and uses a (heap-allocated) lock to protect all accesses.
 *   Use PORT_NewArena() and PORT_FreeArena() to create and destroy
 *   PORTArenaPools.
 *
 * - PORTCheapArenaPool (defined here): this "subclass" can be stack-allocated
 *   and does not use a lock to protect accesses. This makes it cheaper but
 *   less general. It is best used for arena pools that (a) are hot, (b) have
 *   lifetimes bounded within a single function, and (c) don't need locking.
 *   Use PORT_InitCheapArena() and PORT_DestroyCheapArena() to initialize and
 *   finalize PORTCheapArenaPools.
 *
 * All the other PORT_Arena* functions will operate safely with either
 * subclass.
 */
typedef struct PORTCheapArenaPool_str {
    PLArenaPool arena;
    PRUint32 magic; /* This is used to distinguish the two subclasses. */
} PORTCheapArenaPool;

SEC_BEGIN_PROTOS

extern void *PORT_Alloc(size_t len);
extern void *PORT_Realloc(void *old, size_t len);
extern void *PORT_ZAlloc(size_t len);
extern void *PORT_ZAllocAligned(size_t bytes, size_t alignment, void **mem);
extern void *PORT_ZAllocAlignedOffset(size_t bytes, size_t alignment,
                                      size_t offset);
extern void PORT_Free(void *ptr);
extern void PORT_ZFree(void *ptr, size_t len);
extern char *PORT_Strdup(const char *s);
extern void PORT_SetError(int value);
extern int PORT_GetError(void);

/* These functions are for use with PORTArenaPools. */
extern PLArenaPool *PORT_NewArena(unsigned long chunksize);
extern void PORT_FreeArena(PLArenaPool *arena, PRBool zero);

/* These functions are for use with PORTCheapArenaPools. */
extern void PORT_InitCheapArena(PORTCheapArenaPool *arena,
                                unsigned long chunksize);
extern void PORT_DestroyCheapArena(PORTCheapArenaPool *arena);

/* These functions work with both kinds of arena pool. */
extern void *PORT_ArenaAlloc(PLArenaPool *arena, size_t size);
extern void *PORT_ArenaZAlloc(PLArenaPool *arena, size_t size);
extern void *PORT_ArenaGrow(PLArenaPool *arena, void *ptr,
                            size_t oldsize, size_t newsize);
extern void *PORT_ArenaMark(PLArenaPool *arena);
extern void PORT_ArenaRelease(PLArenaPool *arena, void *mark);
extern void PORT_ArenaZRelease(PLArenaPool *arena, void *mark);
extern void PORT_ArenaUnmark(PLArenaPool *arena, void *mark);
extern char *PORT_ArenaStrdup(PLArenaPool *arena, const char *str);

SEC_END_PROTOS

#define PORT_Assert PR_ASSERT
/* This is a variation of PORT_Assert where the arguments will be always
 * used either in Debug or not. But, in optimized mode the result will be
 * ignored. See more details in Bug 1588015. */
#define PORT_AssertArg PR_ASSERT_ARG

/* This runs a function that should return SECSuccess.
 * Intended for NSS internal use only.
 * The return value is asserted in a debug build, otherwise it is ignored.
 * This is no substitute for proper error handling.  It is OK only if you
 * have ensured that the function cannot fail by other means such as checking
 * prerequisites.  In that case this can be used as a safeguard against
 * unexpected changes in a function.
 */
#ifdef DEBUG
#define PORT_CheckSuccess(f) PR_ASSERT((f) == SECSuccess)
#else
#define PORT_CheckSuccess(f) (f)
#endif
#define PORT_ZNew(type) (type *)PORT_ZAlloc(sizeof(type))
#define PORT_ZNewAligned(type, alignment, mem) \
    (type *)PORT_ZAllocAlignedOffset(sizeof(type), alignment, offsetof(type, mem))
#define PORT_New(type) (type *)PORT_Alloc(sizeof(type))
#define PORT_ArenaNew(poolp, type) \
    (type *)PORT_ArenaAlloc(poolp, sizeof(type))
#define PORT_ArenaZNew(poolp, type) \
    (type *)PORT_ArenaZAlloc(poolp, sizeof(type))
#define PORT_NewArray(type, num) \
    (type *)PORT_Alloc(sizeof(type) * (num))
#define PORT_ZNewArray(type, num) \
    (type *)PORT_ZAlloc(sizeof(type) * (num))
#define PORT_ArenaNewArray(poolp, type, num) \
    (type *)PORT_ArenaAlloc(poolp, sizeof(type) * (num))
#define PORT_ArenaZNewArray(poolp, type, num) \
    (type *)PORT_ArenaZAlloc(poolp, sizeof(type) * (num))

/* Please, keep these defines sorted alphabetically.  Thanks! */

#define PORT_Atoi(buff) (int)strtol(buff, NULL, 10)

/* Returns a UTF-8 encoded constant error string for err.
 * Returns NULL if initialization of the error tables fails
 * due to insufficient memory.
 *
 * This string must not be modified by the application.
 */
#define PORT_ErrorToString(err) PR_ErrorToString((err), PR_LANGUAGE_I_DEFAULT)

#define PORT_ErrorToName PR_ErrorToName

#define PORT_Memcmp memcmp
#define PORT_Memcpy memcpy
#ifndef SUNOS4
#define PORT_Memmove memmove
#else /*SUNOS4*/
#define PORT_Memmove(s, ct, n) bcopy((ct), (s), (n))
#endif /*SUNOS4*/
#define PORT_Memset memset

#define PORT_Strcasecmp PL_strcasecmp
#define PORT_Strcat strcat
#define PORT_Strchr strchr
#define PORT_Strrchr strrchr
#define PORT_Strcmp strcmp
#define PORT_Strcpy strcpy
#define PORT_Strlen(s) strlen(s)
#define PORT_Strncasecmp PL_strncasecmp
#define PORT_Strncat strncat
#define PORT_Strncmp strncmp
#define PORT_Strncpy strncpy
#define PORT_Strpbrk strpbrk
#define PORT_Strstr strstr
#define PORT_Strtok strtok

#define PORT_Tolower tolower

typedef PRBool(PR_CALLBACK *PORTCharConversionWSwapFunc)(PRBool toUnicode,
                                                         unsigned char *inBuf, unsigned int inBufLen,
                                                         unsigned char *outBuf, unsigned int maxOutBufLen,
                                                         unsigned int *outBufLen, PRBool swapBytes);

typedef PRBool(PR_CALLBACK *PORTCharConversionFunc)(PRBool toUnicode,
                                                    unsigned char *inBuf, unsigned int inBufLen,
                                                    unsigned char *outBuf, unsigned int maxOutBufLen,
                                                    unsigned int *outBufLen);

SEC_BEGIN_PROTOS

void PORT_SetUCS4_UTF8ConversionFunction(PORTCharConversionFunc convFunc);
void PORT_SetUCS2_ASCIIConversionFunction(PORTCharConversionWSwapFunc convFunc);
PRBool PORT_UCS4_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
                                unsigned int inBufLen, unsigned char *outBuf,
                                unsigned int maxOutBufLen, unsigned int *outBufLen);
PRBool PORT_UCS2_ASCIIConversion(PRBool toUnicode, unsigned char *inBuf,
                                 unsigned int inBufLen, unsigned char *outBuf,
                                 unsigned int maxOutBufLen, unsigned int *outBufLen,
                                 PRBool swapBytes);
void PORT_SetUCS2_UTF8ConversionFunction(PORTCharConversionFunc convFunc);
PRBool PORT_UCS2_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
                                unsigned int inBufLen, unsigned char *outBuf,
                                unsigned int maxOutBufLen, unsigned int *outBufLen);

/* One-way conversion from ISO-8859-1 to UTF-8 */
PRBool PORT_ISO88591_UTF8Conversion(const unsigned char *inBuf,
                                    unsigned int inBufLen, unsigned char *outBuf,
                                    unsigned int maxOutBufLen, unsigned int *outBufLen);

extern PRBool
sec_port_ucs4_utf8_conversion_function(
    PRBool toUnicode,
    unsigned char *inBuf,
    unsigned int inBufLen,
    unsigned char *outBuf,
    unsigned int maxOutBufLen,
    unsigned int *outBufLen);

extern PRBool
sec_port_ucs2_utf8_conversion_function(
    PRBool toUnicode,
    unsigned char *inBuf,
    unsigned int inBufLen,
    unsigned char *outBuf,
    unsigned int maxOutBufLen,
    unsigned int *outBufLen);

/* One-way conversion from ISO-8859-1 to UTF-8 */
extern PRBool
sec_port_iso88591_utf8_conversion_function(
    const unsigned char *inBuf,
    unsigned int inBufLen,
    unsigned char *outBuf,
    unsigned int maxOutBufLen,
    unsigned int *outBufLen);

extern int NSS_PutEnv(const char *envVarName, const char *envValue);

extern int NSS_SecureMemcmp(const void *a, const void *b, size_t n);
extern unsigned int NSS_SecureMemcmpZero(const void *mem, size_t n);

/*
 * Load a shared library called "newShLibName" in the same directory as
 * a shared library that is already loaded, called existingShLibName.
 * A pointer to a static function in that shared library,
 * staticShLibFunc, is required.
 *
 * existingShLibName:
 *   The file name of the shared library that shall be used as the
 *   "reference library". The loader will attempt to load the requested
 *   library from the same directory as the reference library.
 *
 * staticShLibFunc:
 *   Pointer to a static function in the "reference library".
 *
 * newShLibName:
 *   The simple file name of the new shared library to be loaded.
 *
 * We use PR_GetLibraryFilePathname to get the pathname of the loaded
 * shared lib that contains this function, and then do a
 * PR_LoadLibraryWithFlags with an absolute pathname for the shared
 * library to be loaded.
 *
 * On Windows, the "alternate search path" strategy is employed, if available.
 * On Unix, if existingShLibName is a symbolic link, and no link exists for the
 * new library, the original link will be resolved, and the new library loaded
 * from the resolved location.
 *
 * If the new shared library is not found in the same location as the reference
 * library, it will then be loaded from the normal system library path.
 */
PRLibrary *
PORT_LoadLibraryFromOrigin(const char *existingShLibName,
                           PRFuncPtr staticShLibFunc,
                           const char *newShLibName);

SEC_END_PROTOS

#endif /* _SECPORT_H_ */
