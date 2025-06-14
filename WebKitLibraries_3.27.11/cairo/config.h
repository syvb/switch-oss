/* config.h.in.  Generated from configure.ac by autoheader.  */
#ifndef CAIRO_CONFIG_H
#define CAIRO_CONFIG_H 1

/* whether memory barriers are needed around atomic operations */
#undef ATOMIC_OP_NEEDS_MEMORY_BARRIER

/* Define to 1 if the PDF backend can be tested (need poppler and other
   dependencies for pdf2png) */
#undef CAIRO_CAN_TEST_PDF_SURFACE

/* Define to 1 if the PS backend can be tested (needs ghostscript) */
#undef CAIRO_CAN_TEST_PS_SURFACE

/* Define to 1 if the SVG backend can be tested */
#undef CAIRO_CAN_TEST_SVG_SURFACE

/* Define to 1 if dlsym is available */
#undef CAIRO_HAS_DLSYM

/* Define to 1 to enable cairo's cairo-script-interpreter feature */
#undef CAIRO_HAS_INTERPRETER

/* Define to 1 to enable cairo's pthread feature */
#undef CAIRO_HAS_PTHREAD

/* Define to 1 if we have full pthread support */
#undef CAIRO_HAS_REAL_PTHREAD

/* Define to 1 if libspectre is available */
#undef CAIRO_HAS_SPECTRE

/* Define to 1 to enable cairo's symbol-lookup feature */
#undef CAIRO_HAS_SYMBOL_LOOKUP

/* Define to 1 to enable cairo's cairo-trace feature */
#undef CAIRO_HAS_TRACE

/* Define to (0) if freetype2 does not support color fonts */
#undef FT_HAS_COLOR

/* Enable pixman glyph cache */
#undef HAS_PIXMAN_GLYPHS

/* Define to 1 if you have the `alarm' function. */
#undef HAVE_ALARM

/* Define to 1 if you have the binutils development files installed */
#undef HAVE_BFD

/* Define to 1 if you have the <byteswap.h> header file. */
#undef HAVE_BYTESWAP_H

/* Define to 1 if you have the `clock_gettime' function. */
#undef HAVE_CLOCK_GETTIME

/* Define to 1 if you have the `ctime_r' function. */
#undef HAVE_CTIME_R

/* Enable if your compiler supports the GCC __atomic_* atomic primitives */
#undef HAVE_CXX11_ATOMIC_PRIMITIVES

/* Define to 1 if you have the `drand48' function. */
#undef HAVE_DRAND48

/* Define to 1 if you have the `FcFini' function. */
#undef HAVE_FCFINI

/* Define to 1 if you have the `FcInit' function. */
#undef HAVE_FCINIT

/* Define to 1 if you have the <fcntl.h> header file. */
#undef HAVE_FCNTL_H

/* Define to 1 if you have the `feclearexcept' function. */
#undef HAVE_FECLEAREXCEPT

/* Define to 1 if you have the `fedisableexcept' function. */
#undef HAVE_FEDISABLEEXCEPT

/* Define to 1 if you have the `feenableexcept' function. */
#undef HAVE_FEENABLEEXCEPT

/* Define to 1 if you have the <fenv.h> header file. */
#undef HAVE_FENV_H

/* Define to 1 if you have the `flockfile' function. */
#undef HAVE_FLOCKFILE

/* Define to 1 if you have the `fork' function. */
#undef HAVE_FORK

/* Define to 1 if you have the `FT_Done_MM_Var' function. */
#undef HAVE_FT_DONE_MM_VAR

/* Define to 1 if you have the `FT_Get_Var_Design_Coordinates' function. */
#undef HAVE_FT_GET_VAR_DESIGN_COORDINATES

/* Define to 1 if you have the `FT_Get_X11_Font_Format' function. */
#undef HAVE_FT_GET_X11_FONT_FORMAT

/* Define to 1 if you have the `FT_GlyphSlot_Embolden' function. */
#undef HAVE_FT_GLYPHSLOT_EMBOLDEN

/* Define to 1 if you have the `FT_GlyphSlot_Oblique' function. */
#undef HAVE_FT_GLYPHSLOT_OBLIQUE

/* Define to 1 if you have the `FT_Library_SetLcdFilter' function. */
#undef HAVE_FT_LIBRARY_SETLCDFILTER

/* Define to 1 if you have the `FT_Load_Sfnt_Table' function. */
#undef HAVE_FT_LOAD_SFNT_TABLE

/* Define to 1 if you have the `funlockfile' function. */
#undef HAVE_FUNLOCKFILE

/* Enable if your compiler supports the legacy GCC __sync_* atomic primitives
   */
#undef HAVE_GCC_LEGACY_ATOMICS

/* Define to 1 if you have the `getline' function. */
#undef HAVE_GETLINE

/* Define to 1 if you have the `gmtime_r' function. */
#undef HAVE_GMTIME_R

/* Define to 1 if you have the <inttypes.h> header file. */
#undef HAVE_INTTYPES_H

/* Define to 1 if you have the <io.h> header file. */
#undef HAVE_IO_H

/* Define to 1 if you have the <libgen.h> header file. */
#undef HAVE_LIBGEN_H

/* Enable if you have libatomic-ops-dev installed */
#undef HAVE_LIB_ATOMIC_OPS

/* Define to 1 if you have the `link' function. */
#undef HAVE_LINK

/* Define to 1 if you have the `localtime_r' function. */
#undef HAVE_LOCALTIME_R

/* Define to 1 if you have lzo available */
#undef HAVE_LZO

/* Define to non-zero if your system has mkdir, and to 2 if your version of
   mkdir requires a mode parameter */
#undef HAVE_MKDIR

/* Define to 1 if you have the `mmap' function. */
#undef HAVE_MMAP

/* Define to 1 if you have the `newlocale' function. */
#undef HAVE_NEWLOCALE

/* Enable if you have MacOS X atomic operations */
#undef HAVE_OS_ATOMIC_OPS

/* Define to 1 if you have the <poll.h> header file. */
#undef HAVE_POLL_H

/* Define to 1 if you have the `raise' function. */
#undef HAVE_RAISE

/* Define to 1 if you have the `sched_getaffinity' function. */
#undef HAVE_SCHED_GETAFFINITY

/* Define to 1 if you have the <sched.h> header file. */
#undef HAVE_SCHED_H

/* Define to 1 if you have the <setjmp.h> header file. */
#undef HAVE_SETJMP_H

/* Define to 1 if you have the <signal.h> header file. */
#undef HAVE_SIGNAL_H

/* Define to 1 if you have the <stdint.h> header file. */
// #undef HAVE_STDINT_H

/* Define to 1 if you have the `strndup' function. */
#undef HAVE_STRNDUP

/* Define to 1 if you have the `strtod_l' function. */
#undef HAVE_STRTOD_L

/* Define to 1 if you have the <sys/int_types.h> header file. */
#undef HAVE_SYS_INT_TYPES_H

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#undef HAVE_SYS_IOCTL_H

/* Define to 1 if you have the <sys/mman.h> header file. */
#undef HAVE_SYS_MMAN_H

/* Define to 1 if you have the <sys/poll.h> header file. */
#undef HAVE_SYS_POLL_H

/* Define to 1 if you have the <sys/socket.h> header file. */
#undef HAVE_SYS_SOCKET_H

/* Define to 1 if you have the <sys/stat.h> header file. */
#undef HAVE_SYS_STAT_H

/* Define to 1 if you have the <sys/un.h> header file. */
#undef HAVE_SYS_UN_H

/* Define to 1 if you have the <sys/wait.h> header file. */
#undef HAVE_SYS_WAIT_H

/* Define to 1 if you have the <time.h> header file. */
#undef HAVE_TIME_H

/* Define to 1 if the system has the type `uint128_t'. */
#undef HAVE_UINT128_T

/* Define to 1 if the system has the type `uint64_t'. */
#ifdef _MSC_VER
# define HAVE_UINT64_T 1
#endif
#ifdef __ARMCC_VERSION
# define HAVE_UINT64_T 1
#endif


/* Define to 1 if you have the <unistd.h> header file. */
#undef HAVE_UNISTD_H

/* Define to 1 if you have the `waitpid' function. */
#undef HAVE_WAITPID

/* Define to 1 if you have the <X11/extensions/shmproto.h> header file. */
#undef HAVE_X11_EXTENSIONS_SHMPROTO_H

/* Define to 1 if you have the <X11/extensions/shmstr.h> header file. */
#undef HAVE_X11_EXTENSIONS_SHMSTR_H

/* Define to 1 if you have the <X11/extensions/XShm.h> header file. */
#undef HAVE_X11_EXTENSIONS_XSHM_H

/* Define to 1 if you have the <xlocale.h> header file. */
#undef HAVE_XLOCALE_H

/* Define to 1 if you have the `XRenderCreateConicalGradient' function. */
#undef HAVE_XRENDERCREATECONICALGRADIENT

/* Define to 1 if you have the `XRenderCreateLinearGradient' function. */
#undef HAVE_XRENDERCREATELINEARGRADIENT

/* Define to 1 if you have the `XRenderCreateRadialGradient' function. */
#undef HAVE_XRENDERCREATERADIALGRADIENT

/* Define to 1 if you have the `XRenderCreateSolidFill' function. */
#undef HAVE_XRENDERCREATESOLIDFILL

/* Define to 1 if you have zlib available */
#define HAVE_ZLIB 1

/* Define to 1 if the system has the type `__uint128_t'. */
#undef HAVE___UINT128_T

/* Define to 1 if shared memory segments are released deferred. */
#undef IPC_RMID_DEFERRED_RELEASE

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#ifdef __LP64__
#define SIZEOF_LONG 8
#else
#define SIZEOF_LONG 4
#endif

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `size_t', as computed by sizeof. */
#if defined(__x86_64__) || defined(_M_X64) || (defined(__arm64__) && defined(__APPLE__)) || defined(__aarch64__)
#define SIZEOF_SIZE_T 8
#else
#define SIZEOF_SIZE_T 4
#endif

/* The size of `void *', as computed by sizeof. */
#if defined(__x86_64__) || defined(_M_X64) || (defined(__arm64__) && defined(__APPLE__)) || defined(__aarch64__)
#define SIZEOF_VOID_P 8
#else
#define SIZEOF_VOID_P 4
#endif

/* Define to the value your compiler uses to support the warn-unused-result
   attribute */
#define WARN_UNUSED_RESULT

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
#  undef WORDS_BIGENDIAN
# endif
#endif


/* Deal with multiple architecture compiles on Mac OS X */
#ifdef __APPLE_CC__
#ifdef __BIG_ENDIAN__
#define WORDS_BIGENDIAN 1
#define FLOAT_WORDS_BIGENDIAN 1
#else
#undef WORDS_BIGENDIAN
#undef FLOAT_WORDS_BIGENDIAN
#endif
#endif

#undef HAVE_FT_SVG_DOCUMENT

#undef CAIRO_CAN_TEST_TTX_FONT

#undef HAVE_FT_COLR_V1

#undef HAVE_D2D1_3_H

#undef HAVE_PALETTE_SELECT

#undef INTSAFE_H

#define WKC_CAIRO_CUSTOMIZE 1

#undef HAVE_FT_LOAD_NO_SVG

#include <wkc/wkcclib.h>
#include <wkc/wkcfilepeer.h>
#define malloc  wkc_malloc_crashonfailure
#define calloc  wkc_calloc_crashonfailure
#define realloc wkc_realloc_crashonfailure
#define free    wkc_free

#define fopen(n,m)      ((FILE*)wkcFileFOpenPeer(WKC_FILEOPEN_USAGE_WEBCORE, (n), (m)))
#define fclose(f)       wkcFileFClosePeer((void*)(f))
#define fread(b,s,c,f)  wkcFileFReadPeer((b),(s),(c),(void*)(f))
#define fwrite(b,s,c,f) wkcFileFWritePeer((b),(s),(c),(void*)(f))
#define fflush(f)       wkcFileFFlushPeer((void*)(f))
#define ferror(f)       wkcFileFErrorPeer((void*)(f))
#define feof(f)         wkcFileFeofPeer((void*)(f))

#endif /* CAIRO_CONFIG_H */
