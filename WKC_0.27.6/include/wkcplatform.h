/*
 * wkcplatform.h
 *
 * Copyright (C) 2006-2009, 2013-2015 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 * Copyright (c) 2010-2018, 2022 ACCESS CO., LTD. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WKCPLATFORM_H_
#define _WKCPLATFORM_H_

// prevent to include original wtf/Platform.h
#ifndef WTF_Platform_h
# define WTF_Platform_h
#endif // WTF_Platform_h

#include <wkc/wkcpeer.h>
#include <wkc/wkcmpeer.h>

#define WTF_PLATFORM_WKC 1

/* Include compiler specific macros */
#include <wtf/Compiler.h>

// defines in makefile
#define ENABLE_CHANNEL_MESSAGING 1
#define ENABLE_CONTENT_EXTENSIONS 0
#define ENABLE_CONTEXT_MENUS 0
#define ENABLE_CUSTOM_SCHEME_HANDLER 0
#define ENABLE_CSP_NEXT 1
#define ENABLE_DASHBOARD_SUPPORT 0
#define ENABLE_DATA_TRANSFER_ITEMS 0
#define ENABLE_DOM4_EVENTS_CONSTRUCTOR 1
#define ENABLE_DRAG_SUPPORT 1
#define ENABLE_ES6_ARROWFUNCTION_SYNTAX 1
#define ENABLE_ES6_CLASS_SYNTAX 1
#define ENABLE_ES6_TEMPLATE_LITERAL_SYNTAX 1
#define ENABLE_FONT_LOAD_EVENTS 1
#define ENABLE_FTPDIR 0
#define ENABLE_FULLSCREEN_API 1
#define ENABLE_INTL 0
// ref.http://code.google.com/p/chromium/issues/detail?id=152430
#define ENABLE_MATHML 0
#define ENABLE_MHTML 1
#define ENABLE_DETAILS_ELEMENT 1
#define ENABLE_METER_ELEMENT 1
#define ENABLE_PAN_SCROLLING 0
#define ENABLE_POINTER_LOCK 0
#define ENABLE_PROMISES 1
#define ENABLE_SMOOTH_SCROLLING 0
#define ENABLE_SUBTLE_CRYPTO 1
#define ENABLE_TEMPLATE_ELEMENT 1
#define ENABLE_TEXT_CARET 1
#define ENABLE_TOUCH_EVENTS 1
#define ENABLE_REQUEST_AUTOCOMPLETE 0
#define ENABLE_STREAMS_API 0
#define ENABLE_HIDDEN_PAGE_DOM_TIMER_THROTTLING 1
// only for Safari
#define ENABLE_WEB_ARCHIVE 0
#define ENABLE_WEB_SOCKETS 1
#define ENABLE_XHR_TIMEOUT 1
#define ENABLE_XSLT 0

#define HAVE_ACCESSIBILITY 0
#define USE_PROTECTION_SPACE_AUTH_CALLBACK 0

// disable plug-ins
#define ENABLE_NETSCAPE_PLUGIN_API 0
#define ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE 0
#define ENABLE_PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC 0

#define ENABLE_PURGEABLE_MEMORY 0
#define ENABLE_OPCODE_STATS 0

#define ENABLE_INPUT_TYPE_COLOR 1
#define ENABLE_INPUT_TYPE_MONTH 1
#define ENABLE_INPUT_TYPE_WEEK 1
#define ENABLE_INPUT_TYPE_DATE 1
// see http://lists.webkit.org/pipermail/webkit-dev/2013-January/023404.html
//#define ENABLE_INPUT_TYPE_DATETIME_INCOMPLETE 1
#define ENABLE_INPUT_TYPE_DATETIMELOCAL 1
#define ENABLE_INPUT_TYPE_TIME 1

#define ENABLE_SQL_DATABASE 1
#define ENABLE_INDEXED_DATABASE 0
#define ENABLE_QUOTA 0

#define ENABLE_TOUCH_ICON_LOADING 0
#define ENABLE_ICONDATABASE 1

#define ENABLE_BATTERY_STATUS 0
#define ENABLE_GAMEPAD 1
//#define ENABLE_GAMEPAD_DEPRECATED 1
#define ENABLE_GEOLOCATION 0
#define ENABLE_NAVIGATOR_CONTENT_UTILS 1
#define ENABLE_NOTIFICATIONS 0
#define ENABLE_LEGACY_NOTIFICATIONS 0
#define ENABLE_ORIENTATION_EVENTS 1
#define ENABLE_DEVICE_ORIENTATION 1
#define ENABLE_PROXIMITY_EVENTS 1
#define ENABLE_MOUSE_FORCE_EVENTS 1
#define ENABLE_VIBRATION 1
#define ENABLE_SPEECH_SYNTHESIS 0

#define ENABLE_MEDIA_SOURCE 1
#define ENABLE_MEDIA_STREAM 0
#define ENABLE_VIDEO 1
#define ENABLE_VIDEO_TRACK 1
#define ENABLE_WEBVTT_REGIONS 1
#define ENABLE_MEDIA_STATISTICS 1
#define ENABLE_MEDIA_CONTROLS_SCRIPT 1
#define ENABLE_ENCRYPTED_MEDIA 0
#define ENABLE_ENCRYPTED_MEDIA_V2 0
#define ENABLE_WEB_AUDIO 1

#define ENABLE_FILTERS_LEVEL_2 0
#define ENABLE_SVG_FONTS 1
#define ENABLE_CANVAS_PATH 1
#define ENABLE_CANVAS_PROXY 1
#define ENABLE_REQUEST_ANIMATION_FRAME 1
#define USE_REQUEST_ANIMATION_FRAME_TIMER 0
#define ENABLE_TEXT_AUTOSIZING 0
#define ENABLE_SUBPIXEL_LAYOUT 0
#define ENABLE_PICTURE_SIZES 1

#ifdef WKC_ENABLE_WEBGL
# define ENABLE_WEBGL 1
# define ENABLE_GRAPHICS_CONTEXT_3D 1
#endif
#define ENABLE_WEBGL2 0
// not maintained
#define ENABLE_ACCELERATED_2D_CANVAS 0
#ifdef WKC_ENABLE_ACCELERATED_COMPOSITING
# define USE_ACCELERATED_COMPOSITING 1
# define ENABLE_3D_TRANSFORMS 1
# define USE_TEXTURE_MAPPER 1
# ifdef WKC_ENABLE_COMPOSITED_FIXED_ELEMENTS
#  define ENABLE_COMPOSITED_FIXED_ELEMENTS 1
# endif
# define USE_OPENGL_ES_2 1
#if !defined(_MSC_VER)
# define USE_EGL 1
#endif
#endif

#ifdef WKC_ENABLE_ACCELERATED_COMPOSITING
# define ENABLE_CSS_COMPOSITING 1
#endif
#define ENABLE_CSS_EXCLUSIONS 1
#define ENABLE_CSS_REGIONS 1
#define ENABLE_CSS_GRID_LAYOUT 1
#define ENABLE_CSS_SHAPES 1
#define ENABLE_CSS_IMAGE_ORIENTATION 1
#define ENABLE_CSS_IMAGE_RESOLUTION 1
#define ENABLE_RESOLUTION_MEDIA_QUERY 1
#define ENABLE_CSS_IMAGE_SET 1
#define ENABLE_CSS_DEVICE_ADAPTATION 1
#define ENABLE_CSS_BOX_DECORATION_BREAK 1
#define ENABLE_VIEW_MODE_CSS_MEDIA 1
#define ENABLE_CSS3_TEXT 1
#define ENABLE_CSS3_TEXT_DECORATION_SKIP_INK 1
#define ENABLE_CSS_TRAILING_WORD 1

#define ENABLE_WEB_TIMING 1
#define ENABLE_USER_TIMING 1
#define ENABLE_RESOURCE_TIMING 1
#define ENABLE_PERFORMANCE_TIMELINE 1

#define ENABLE_APNG 1
#define USE_WEBP 1
#define ENABLE_WKC_BLINK_AWEBP 1

#define ENABLE_WKC_WEB_NFC 0

#define ENABLE_WKC_DEVICE_MODE_CSS_MEDIA 1
#define ENABLE_WKC_PERFORMANCE_MODE_CSS_MEDIA 1

// wkc configurations

#define USE_OS_RANDOMNESS 1
#define USE_WKC_UNICODE 1

#ifdef WKC_USE_REMOTEWEBINSPECTOR
# define ENABLE_INSPECTOR 1
# define ENABLE_REMOTE_INSPECTOR 1
#else
# define ENABLE_INSPECTOR 0
# define ENABLE_REMOTE_INSPECTOR 0
#endif
#define ENABLE_INSPECTOR_ALTERNATE_DISPATCHERS 0

#define HAVE_ERRNO_H 0
#define HAVE_LANGINFO_H 0
#define HAVE_MMAP 0
#define HAVE_SBRK 0
#define HAVE_STRINGS_H 0
#define HAVE_SYS_PARAM_H 0
#define HAVE_SYS_TIME_H 0

#define USE_WKC_CUSTOM_BACKFORWARD_LIST 1

// We enable this even in release build for ease of debugging.
#define ENABLE_TREE_DEBUGGING 1

/* ==== PLATFORM handles OS, operating environment, graphics API, and
   CPU. This macro will be phased out in favor of platform adaptation
   macros, policy decision macros, and top-level port definitions. ==== */
#define PLATFORM(WTF_FEATURE) (defined WTF_PLATFORM_##WTF_FEATURE  && WTF_PLATFORM_##WTF_FEATURE)


/* ==== Platform adaptation macros: these describe properties of the target environment. ==== */

/* CPU() - the target CPU architecture */
#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)
/* HAVE() - specific system features (headers, functions or similar) that are present or not */
#define HAVE(WTF_FEATURE) (defined HAVE_##WTF_FEATURE  && HAVE_##WTF_FEATURE)
/* OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit */
#define OS(WTF_FEATURE) (defined WTF_OS_##WTF_FEATURE  && WTF_OS_##WTF_FEATURE)


/* ==== Policy decision macros: these define policy choices for a particular port. ==== */

/* USE() - use a particular third-party library or optional OS service */
#define USE(WTF_FEATURE) (defined USE_##WTF_FEATURE && USE_##WTF_FEATURE)
/* ENABLE() - turn on a specific feature of WebKit */
#define ENABLE(WTF_FEATURE) (defined ENABLE_##WTF_FEATURE  && ENABLE_##WTF_FEATURE)


/* ==== CPU() - the target CPU architecture ==== */

/* This also defines CPU(BIG_ENDIAN) or CPU(MIDDLE_ENDIAN) or neither, as appropriate. */

/* CPU(ALPHA) - DEC Alpha */
#if defined(__alpha__)
#define WTF_CPU_ALPHA 1
#endif

/* CPU(HPPA) - HP PA-RISC */
#if defined(__hppa__) || defined(__hppa64__)
#define WTF_CPU_HPPA 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(IA64) - Itanium / IA-64 */
#if defined(__ia64__)
#define WTF_CPU_IA64 1
/* 32-bit mode on Itanium */
#if !defined(__LP64__)
#define WTF_CPU_IA64_32 1
#endif
#endif

/* CPU(MIPS) - MIPS 32-bit */
/* Note: Only O32 ABI is tested, so we enable it for O32 ABI for now.  */
#if (defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_) || defined(__mips64))
#if defined(_ABI64) && (_MIPS_SIM == _ABI64)
#define WTF_CPU_MIPS64 1
#define WTF_MIPS_ARCH __mips64
#else
#define WTF_CPU_MIPS 1
#define WTF_MIPS_ARCH __mips
#endif
#if defined(__MIPSEB__)
#define WTF_CPU_BIG_ENDIAN 1
#endif
#define WTF_MIPS_PIC (defined __PIC__)
#define WTF_MIPS_ARCH __mips
#define WTF_MIPS_ISA(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH == v)
#define WTF_MIPS_ISA_AT_LEAST(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH >= v)
#define WTF_MIPS_ARCH_REV __mips_isa_rev
#define WTF_MIPS_ISA_REV(v) (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == v)
#define WTF_MIPS_DOUBLE_FLOAT (defined __mips_hard_float && !defined __mips_single_float)
#define WTF_MIPS_FP64 (defined __mips_fpr && __mips_fpr == 64)
/* MIPS requires allocators to use aligned memory */
#define USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif /* MIPS */

/* CPU(PPC) - PowerPC 32-bit */
#if (  defined(__ppc__)        \
    || defined(__PPC__)        \
    || defined(__powerpc__)    \
    || defined(__powerpc)      \
    || defined(__POWERPC__)    \
    || defined(_M_PPC)         \
    || defined(__PPC))         \
    && defined(__BYTE_ORDER__) \
    && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define WTF_CPU_PPC 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(PPC64) - PowerPC 64-bit Big Endian */
#if (  defined(__ppc64__)      \
    || defined(__PPC64__))     \
    && defined(__BYTE_ORDER__) \
    && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define WTF_CPU_PPC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(PPC64) - PowerPC 64-bit Little Endian */
#if (   defined(__ppc64__)     \
    || defined(__PPC64__)      \
    || defined(__ppc64le__)    \
    || defined(__PPC64LE__))   \
    && defined(__BYTE_ORDER__) \
    && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define WTF_CPU_PPC64LE 1
#endif

/* CPU(SH4) - SuperH SH-4 */
#if defined(__SH4__)
#define WTF_CPU_SH4 1
#endif

/* CPU(S390X) - S390 64-bit */
#if defined(__s390x__)
#define WTF_CPU_S390X 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(S390) - S390 32-bit */
#if defined(__s390__)
#define WTF_CPU_S390 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(X86) - i386 / x86 32-bit */
#if   defined(__i386__) \
    || defined(i386)     \
    || defined(_M_IX86)  \
    || defined(_X86_)    \
    || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif

/* CPU(X86_64) - AMD64 / Intel64 / x86_64 64-bit */
#if   defined(__x86_64__) \
    || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif

/* CPU(ARM64) - Apple */
#if (defined(__arm64__) && defined(__APPLE__)) || defined(__aarch64__)
#define WTF_CPU_ARM64 1
#endif

/* CPU(ARM) - ARM, any version*/
#define WTF_ARM_ARCH_AT_LEAST(N) (CPU(ARM) && WTF_ARM_ARCH_VERSION >= N)

#if   defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define WTF_CPU_ARM 1

#if defined(__ARM_PCS_VFP)
#define WTF_CPU_ARM_HARDFP 1
#endif

#if defined(__ARMEB__)
#define WTF_CPU_BIG_ENDIAN 1

#elif !defined(__ARM_EABI__) \
    && !defined(__EABI__) \
    && !defined(__VFP_FP__) \
    && !defined(_WIN32_WCE)
#define WTF_CPU_MIDDLE_ENDIAN 1

#endif

/* Set WTF_ARM_ARCH_VERSION */
#if   defined(__ARM_ARCH_4__) \
    || defined(__ARM_ARCH_4T__) \
    || defined(__MARM_ARMV4__)
#define WTF_ARM_ARCH_VERSION 4

#elif defined(__ARM_ARCH_5__) \
    || defined(__ARM_ARCH_5T__) \
    || defined(__MARM_ARMV5__)
#define WTF_ARM_ARCH_VERSION 5

#elif defined(__ARM_ARCH_5E__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_ARM_ARCH_VERSION 5
/*ARMv5TE requires allocators to use aligned memory*/
#define USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1

#elif defined(__ARM_ARCH_6__) \
    || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6T2__) \
    || defined(__ARMV6__)
#define WTF_ARM_ARCH_VERSION 6

#elif defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7K__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7S__)
#define WTF_ARM_ARCH_VERSION 7

#elif defined(__ARM_ARCH_8__)
#define WTF_ARM_ARCH_VERSION 8

/* MSVC sets _M_ARM */
#elif defined(_M_ARM)
#define WTF_ARM_ARCH_VERSION _M_ARM

/* RVCT sets _TARGET_ARCH_ARM */
#elif defined(__TARGET_ARCH_ARM)
#define WTF_ARM_ARCH_VERSION __TARGET_ARCH_ARM

#if defined(__TARGET_ARCH_5E) \
    || defined(__TARGET_ARCH_5TE) \
    || defined(__TARGET_ARCH_5TEJ)
/*ARMv5TE requires allocators to use aligned memory*/
#define USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif

#else
#define WTF_ARM_ARCH_VERSION 0

#endif

/* Set WTF_THUMB_ARCH_VERSION */
#if   defined(__ARM_ARCH_4T__)
#define WTF_THUMB_ARCH_VERSION 1

#elif defined(__ARM_ARCH_5T__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_THUMB_ARCH_VERSION 2

#elif defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6M__)
#define WTF_THUMB_ARCH_VERSION 3

#elif defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_7__) \
    || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7K__) \
    || defined(__ARM_ARCH_7M__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7S__)
#define WTF_THUMB_ARCH_VERSION 4

/* RVCT sets __TARGET_ARCH_THUMB */
#elif defined(__TARGET_ARCH_THUMB)
#define WTF_THUMB_ARCH_VERSION __TARGET_ARCH_THUMB

#else
#define WTF_THUMB_ARCH_VERSION 0
#endif


/* CPU(ARMV5_OR_LOWER) - ARM instruction set v5 or earlier */
/* On ARMv5 and below the natural alignment is required. 
   And there are some other differences for v5 or earlier. */
#if !defined(ARMV5_OR_LOWER) && !WTF_ARM_ARCH_AT_LEAST(6)
#define WTF_CPU_ARMV5_OR_LOWER 1
#endif


/* CPU(ARM_TRADITIONAL) - Thumb2 is not available, only traditional ARM (v4 or greater) */
/* CPU(ARM_THUMB2) - Thumb2 instruction set is available */
/* Only one of these will be defined. */
#if !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2)
#  if defined(thumb2) || defined(__thumb2__) \
    || ((defined(__thumb) || defined(__thumb__)) && WTF_THUMB_ARCH_VERSION == 4)
#    define WTF_CPU_ARM_TRADITIONAL 0
#    define WTF_CPU_ARM_THUMB2 1
#  elif WTF_ARM_ARCH_AT_LEAST(4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  else
#    error "Not supported ARM architecture"
#  endif
#elif CPU(ARM_TRADITIONAL) && CPU(ARM_THUMB2) /* Sanity Check */
#  error "Cannot use both of WTF_CPU_ARM_TRADITIONAL and WTF_CPU_ARM_THUMB2 platforms"
#endif /* !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2) */

#if defined(__ARM_NEON__) && !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 1
#endif

#if CPU(ARM_NEON) && (!COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 7, 0))
/* All NEON intrinsics usage can be disabled by this macro. */
#define HAVE_ARM_NEON_INTRINSICS 1
#endif

#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
#define WTF_CPU_ARM_VFP 1
#endif

#if defined(__ARM_ARCH_7K__)
#define WTF_CPU_APPLE_ARMV7K 1
#endif

#if defined(__ARM_ARCH_7S__)
#define WTF_CPU_APPLE_ARMV7S 1
#endif

#if defined(__ARM_ARCH_EXT_IDIV__) || CPU(APPLE_ARMV7S)
#define HAVE_ARM_IDIV_INSTRUCTIONS 1
#endif

#endif /* ARM */

#if CPU(ARM) || CPU(MIPS) || CPU(SH4)
#define WTF_CPU_NEEDS_ALIGNED_ACCESS 1
#endif

/* ==== OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit ==== */

/* OS(AIX) - AIX */
#ifdef _AIX
#define WTF_OS_AIX 1
#endif

/* OS(DARWIN) - Any Darwin-based OS, including Mac OS X and iPhone OS */
#ifdef __APPLE__
#define WTF_OS_DARWIN 1

#include <Availability.h>
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#endif

/* OS(IOS) - iOS */
/* OS(MAC_OS_X) - Mac OS X (not including iOS) */
#if OS(DARWIN) && ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) \
    || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                 \
    || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#define WTF_OS_IOS 1
#elif OS(DARWIN) && defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define WTF_OS_MAC_OS_X 1
#endif

/* OS(FREEBSD) - FreeBSD */
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
//#define WTF_OS_FREEBSD 1
#endif

/* OS(HURD) - GNU/Hurd */
#ifdef __GNU__
//#define WTF_OS_HURD 1
#endif

/* OS(LINUX) - Linux */
#ifdef __linux__
//#define WTF_OS_LINUX 1
#endif

/* OS(NETBSD) - NetBSD */
#if defined(__NetBSD__)
//#define WTF_OS_NETBSD 1
#endif

/* OS(OPENBSD) - OpenBSD */
#ifdef __OpenBSD__
//#define WTF_OS_OPENBSD 1
#endif

/* OS(SOLARIS) - Solaris */
#if defined(sun) || defined(__sun)
//#define WTF_OS_SOLARIS 1
#endif

/* OS(WINDOWS) - Any version of Windows */
#if defined(WIN32) || defined(_WIN32)
#if !PLATFORM(WKC)
#define WTF_OS_WINDOWS 1
#else
#define WTF_OS_WINDOWS_WKC 1
#endif
#endif

#define WTF_OS_WIN ERROR "USE WINDOWS WITH OS NOT WIN"
#define WTF_OS_MAC ERROR "USE MAC_OS_X WITH OS NOT MAC"

/* OS(UNIX) - Any Unix-like system */
#if    OS(AIX)              \
    || OS(DARWIN)           \
    || OS(FREEBSD)          \
    || OS(HURD)             \
    || OS(LINUX)            \
    || OS(NETBSD)           \
    || OS(OPENBSD)          \
    || OS(SOLARIS)          \
    || defined(unix)        \
    || defined(__unix)      \
    || defined(__unix__)
#if !PLATFORM(WKC)
#define WTF_OS_UNIX 1
#else
#define WTF_OS_WKC_UNIX 1
#endif
#endif

/* Operating environments */

/* FIXME: these are all mixes of OS, operating environment and policy choices. */
/* PLATFORM(EFL) */
/* PLATFORM(GTK) */
/* PLATFORM(MAC) */
/* PLATFORM(IOS) */
/* PLATFORM(IOS_SIMULATOR) */
/* PLATFORM(WIN) */
#if defined(BUILDING_EFL__)
#define WTF_PLATFORM_EFL 1
#elif defined(BUILDING_GTK__)
#define WTF_PLATFORM_GTK 1
#elif OS(MAC_OS_X)
#define WTF_PLATFORM_MAC 1
#elif OS(IOS)
#define WTF_PLATFORM_IOS 1
#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define WTF_PLATFORM_IOS_SIMULATOR 1
#endif
#elif OS(WINDOWS)
#define WTF_PLATFORM_WIN 1
#elif OS(WINDOWS_WKC)
#define WTF_PLATFORM_WKC_WIN 1
#endif

/* PLATFORM(COCOA) */
#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_PLATFORM_COCOA 1
#endif

/* Graphics engines */

#if PLATFORM(COCOA)
#if defined __has_include && __has_include(<CoreFoundation/CFPriv.h>)
#define USE_APPLE_INTERNAL_SDK 1
#endif
#endif

/* PLATFORM(APPLETV) */
#if defined(TARGET_OS_TV) && TARGET_OS_TV
#define WTF_PLATFORM_APPLETV 1
#endif

/* PLATFORM(WATCHOS) */
#if defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
#define WTF_PLATFORM_WATCHOS 1
#endif

/* USE(CG) and PLATFORM(CI) */
#if PLATFORM(COCOA) || (PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO))
#define USE_CG 1
#endif
#if PLATFORM(COCOA) || (PLATFORM(WIN) && USE(CG))
#define USE_CA 1
#endif

#if PLATFORM(GTK) || PLATFORM(EFL)
#define USE_CAIRO 1
#define USE_GLIB 1
#define USE_FREETYPE 1
#define USE_HARFBUZZ 1
#define USE_SOUP 1
#define USE_WEBP 1
#endif

/* On Windows, use QueryPerformanceCounter by default */
#if OS(WINDOWS) || (PLATFORM(WKC) && COMPILER(MSVC) && !defined(UNDER_CE))
#define USE_QUERY_PERFORMANCE_COUNTER  1
#endif

#if PLATFORM(COCOA)

#define USE_CF 1
#define USE_FOUNDATION 1
#define ENABLE_USER_MESSAGE_HANDLERS 1
#define HAVE_OUT_OF_PROCESS_LAYER_HOSTING 1
#define HAVE_DTRACE 1

#if !PLATFORM(WATCHOS) && !PLATFORM(APPLETV)
#define HAVE_AVKIT 1
#endif

#if !PLATFORM(WATCHOS)
#define HAVE_PARENTAL_CONTROLS 1
#endif

#if PLATFORM(COCOA)
#define HAVE_OUT_OF_PROCESS_LAYER_HOSTING 1
#endif

#endif

#if PLATFORM(MAC)

#define USE_APPKIT 1
#define HAVE_RUNLOOP_TIMER 1
#define HAVE_SEC_IDENTITY 1
#define HAVE_SEC_KEYCHAIN 1

#if CPU(X86_64)
#define USE_PLUGIN_HOST_PROCESS 1
#endif

/* OS X defines a series of platform macros for debugging. */
/* Some of them are really annoying because they use common names (e.g. check()). */
/* Disable those macros so that we are not limited in how we name methods and functions. */
#undef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0

#endif /* PLATFORM(MAC) */

#if PLATFORM(IOS)

#define HAVE_READLINE 1
#if USE(APPLE_INTERNAL_SDK)
#define USE_CFNETWORK 1
#endif
#define USE_UIKIT_EDITING 1
#define USE_WEB_THREAD 1
#define USE_QUICK_LOOK 1

#if defined(TARGET_OS_IOS) && TARGET_OS_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= 90000
#define HAVE_APP_LINKS 1
#endif

#if CPU(ARM64)
#define ENABLE_JIT_CONSTANT_BLINDING 0
#endif

#if CPU(ARM_NEON)
#undef HAVE_ARM_NEON_INTRINSICS
#define HAVE_ARM_NEON_INTRINSICS 0
#endif

#endif /* PLATFORM(IOS) */

#if PLATFORM(WIN) && !USE(WINGDI)
#define USE_CF 1
#endif

#if PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO)
#define USE_CFNETWORK 1
#endif

#if USE(CFNETWORK) || PLATFORM(COCOA)
#define USE_CFURLCACHE 1
#endif

#if !defined(HAVE_ACCESSIBILITY)
#if PLATFORM(COCOA) || PLATFORM(WIN) || PLATFORM(GTK) || PLATFORM(EFL)
#define HAVE_ACCESSIBILITY 1
#endif
#endif /* !defined(HAVE_ACCESSIBILITY) */

#if OS(UNIX)
#define HAVE_ERRNO_H 1
#define HAVE_LANGINFO_H 1
#define HAVE_MMAP 1
#define HAVE_SIGNAL_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1 
#define USE_PTHREADS 1
#endif /* OS(UNIX) */

#if (OS(FREEBSD) || OS(OPENBSD)) && !defined(__GLIBC__)
#define HAVE_PTHREAD_NP_H 1
#endif

#if !defined(HAVE_VASPRINTF)
#if !COMPILER(MSVC) &&!COMPILER(RVCT) && !COMPILER(MINGW) && !PLATFORM(WKC)
#define HAVE_VASPRINTF 1
#endif
#endif

#if !defined(HAVE_STRNSTR)
#if OS(DARWIN) || (OS(FREEBSD) && !defined(__GLIBC__))
#define HAVE_STRNSTR 1
#endif
#endif

#if OS(DARWIN) || OS(FREEBSD) || OS(NETBSD)
#define HAVE_STAT_BIRTHTIME 1
#endif

#if !OS(WINDOWS) && !OS(SOLARIS) && !PLATFORM(WKC)
#define HAVE_TM_GMTOFF 1
#define HAVE_TM_ZONE 1
#define HAVE_TIMEGM 1
#endif

#if OS(DARWIN)

#define HAVE_DISPATCH_H 1
#define HAVE_MADV_FREE 1
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MERGESORT 1
#define HAVE_PTHREAD_SETNAME_NP 1
#define HAVE_READLINE 1
#define HAVE_SYS_TIMEB_H 1
#define USE_ACCELERATE 1

#if !PLATFORM(IOS)
#define HAVE_HOSTED_CORE_ANIMATION 1
#endif

#endif /* OS(DARWIN) */

#if OS(WINDOWS) && !PLATFORM(WKC)

#define HAVE_SYS_TIMEB_H 1
#define HAVE_ALIGNED_MALLOC 1
#define HAVE_ISDEBUGGERPRESENT 1

#include <WTF/WTFHeaderDetection.h>

#endif

#if OS(WINDOWS)
#define HAVE_VIRTUALALLOC 1
#endif

/* ENABLE macro defaults */

/* FIXME: move out all ENABLE() defines from here to FeatureDefines.h */

/* Include feature macros */
#include <wtf/FeatureDefines.h>

#if OS(WINDOWS)
#define USE_SYSTEM_MALLOC 1
#endif

#define ENABLE_DEBUG_WITH_BREAKPOINT 0
#define ENABLE_SAMPLING_COUNTERS 0
#define ENABLE_SAMPLING_FLAGS 0
#define ENABLE_SAMPLING_REGIONS 0
#define ENABLE_OPCODE_SAMPLING 0
#define ENABLE_CODEBLOCK_SAMPLING 0
#if ENABLE(CODEBLOCK_SAMPLING) && !ENABLE(OPCODE_SAMPLING)
#error "CODEBLOCK_SAMPLING requires OPCODE_SAMPLING"
#endif
#if ENABLE(OPCODE_SAMPLING) || ENABLE(SAMPLING_FLAGS) || ENABLE(SAMPLING_REGIONS)
#define ENABLE_SAMPLING_THREAD 1
#endif

#if !defined(USE_JSVALUE64) && !defined(USE_JSVALUE32_64)
#if (CPU(X86_64) && (OS(UNIX) || OS(WINDOWS))) \
    || (CPU(IA64) && !CPU(IA64_32)) \
    || CPU(ALPHA) \
    || CPU(ARM64) \
    || CPU(S390X) \
    || CPU(PPC64) \
    || CPU(MIPS64) \
    || CPU(PPC64) \
    || CPU(PPC64LE) \
    || (CPU(X86_64) && PLATFORM(WKC))
#define USE_JSVALUE64 1
#else
#define USE_JSVALUE32_64 1
#endif
#endif /* !defined(USE_JSVALUE64) && !defined(USE_JSVALUE32_64) */

#ifdef WKC_ENABLE_JIT
# define ENABLE_JIT 1
# define ENABLE_YARR_JIT 1
# if CPU(X86_64) || CPU(ARM64) || CPU(ARM_THUMB2)
#  define ENABLE_CSS_SELECTOR_JIT 1
# else
#  define ENABLE_CSS_SELECTOR_JIT 0
# endif
#else
# define ENABLE_JIT 0
# define ENABLE_YARR_JIT 0
# define ENABLE_CSS_SELECTOR_JIT 0
# define ENABLE_LLINT_C_LOOP 1
#endif

/* The JIT is enabled by default on all x86, x86-64, ARM & MIPS platforms except ARMv7k and Windows. */
#if !defined(ENABLE_JIT) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM) || CPU(ARM64) || CPU(MIPS)) && !CPU(APPLE_ARMV7K)
#define ENABLE_JIT 1
#endif

/* Do we have LLVM? */
#if !defined(HAVE_LLVM) && OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK) && !PLATFORM(WKC) && ENABLE(FTL_JIT) && (CPU(X86_64) || CPU(ARM64))
#define HAVE_LLVM 1
#endif

#if PLATFORM(GTK) && HAVE(LLVM) && ENABLE(JIT) && !defined(ENABLE_FTL_JIT) && CPU(X86_64)
#define ENABLE_FTL_JIT 1
#endif

/* The FTL *does not* work on 32-bit platforms. Disable it even if someone asked us to enable it. */
#if USE(JSVALUE32_64)
#undef ENABLE_FTL_JIT
#define ENABLE_FTL_JIT 0
#endif

/* The FTL is disabled on the iOS simulator, mostly for simplicity. */
#if PLATFORM(IOS_SIMULATOR)
#undef ENABLE_FTL_JIT
#define ENABLE_FTL_JIT 0
#endif

/* If possible, try to enable the LLVM disassembler. This is optional and we can
   fall back on UDis86 if necessary. */
#if !defined(USE_LLVM_DISASSEMBLER) && HAVE(LLVM) && (CPU(X86_64) || CPU(X86) || CPU(ARM64))
#define USE_LLVM_DISASSEMBLER 1
#endif

/* If possible, try to enable a disassembler. This is optional. We proceed in two
   steps: first we try to find some disassembler that we can use, and then we
   decide if the high-level disassembler API can be enabled. */
#if !defined(USE_UDIS86) && ENABLE(JIT) && ((OS(DARWIN) && !PLATFORM(EFL) && !PLATFORM(GTK)) || (OS(LINUX) && (PLATFORM(EFL) || PLATFORM(GTK)))) \
    && (CPU(X86) || CPU(X86_64))
#define USE_UDIS86 1
#endif

#if !defined(ENABLE_DISASSEMBLER) && (USE(UDIS86) || USE(LLVM_DISASSEMBLER))
#define ENABLE_DISASSEMBLER 1
#endif

#if !defined(USE_ARM64_DISASSEMBLER) && ENABLE(JIT) && PLATFORM(IOS) && CPU(ARM64) && !USE(LLVM_DISASSEMBLER)
#define USE_ARM64_DISASSEMBLER 1
#endif

#if !defined(USE_ARMV7_DISASSEMBLER) && ENABLE(JIT) && (PLATFORM(IOS) || PLATFORM(GTK)) && CPU(ARM_THUMB2)
#define USE_ARMV7_DISASSEMBLER 1
#endif

#if !defined(ENABLE_DISASSEMBLER) && (USE(UDIS86) || USE(ARMV7_DISASSEMBLER) || USE(ARM64_DISASSEMBLER))
#define ENABLE_DISASSEMBLER 1
#endif

#if !defined(ENABLE_DFG_JIT) && ENABLE(JIT)
/* Enable the DFG JIT on X86 and X86_64. */
#if (CPU(X86) || CPU(X86_64)) && (OS(DARWIN) || OS(LINUX) || OS(FREEBSD) || OS(WINDOWS)  || OS(HURD) || PLATFORM(WKC))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARMv7.  Only tested on iOS and Qt/GTK+ Linux. */
#if (CPU(ARM_THUMB2) || CPU(ARM64)) && (PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(EFL))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARM, MIPS and SH4. */
#if CPU(ARM_TRADITIONAL) || CPU(MIPS) || CPU(SH4)
#define ENABLE_DFG_JIT 1
#endif
#endif

/* Concurrent JIT only works on 64-bit platforms because it requires that
   values get stored to atomically. This is trivially true on 64-bit platforms,
   but not true at all on 32-bit platforms where values are composed of two
   separate sub-values. */
#if (OS(DARWIN) || PLATFORM(EFL) || PLATFORM(GTK)) && ENABLE(DFG_JIT) && USE(JSVALUE64)
#define ENABLE_CONCURRENT_JIT 1
#endif

/* If the baseline jit is not available, then disable upper tiers as well: */
#if !ENABLE(JIT)
#undef ENABLE_DFG_JIT      /* Undef so that we can redefine it. */
#undef ENABLE_FTL_JIT      /* Undef so that we can redefine it. */
#define ENABLE_DFG_JIT 0
#define ENABLE_FTL_JIT 0
#endif

#if !defined(ENABLE_FTL_NATIVE_CALL_INLINING)
#define ENABLE_FTL_NATIVE_CALL_INLINING 0
#endif

/* Used to make GCC's optimization not throw away a symbol that we would need for native inlining */
#if ENABLE(FTL_NATIVE_CALL_INLINING) && COMPILER(GCC) && !COMPILER(CLANG)
#define ATTR_USED __attribute__ ((used))
#else
#define ATTR_USED
#endif

/* Generational collector for JSC */
#if !defined(ENABLE_GGC)
#if CPU(X86_64) || CPU(X86) || CPU(ARM64) || CPU(ARM)
#define ENABLE_GGC 1
#else
#define ENABLE_GGC 0
#endif /* CPU(X86_64) || CPU(X86) || CPU(ARM64) || CPU(ARM) */
#endif /* !defined(ENABLE_GGC) */

/* Counts uses of write barriers using sampling counters. Be sure to also
   set ENABLE_SAMPLING_COUNTERS to 1. */
#if !defined(ENABLE_WRITE_BARRIER_PROFILING)
#define ENABLE_WRITE_BARRIER_PROFILING 0
#endif

/* Logs all allocation-related activity that goes through fastMalloc or the
   JSC GC (both cells and butterflies). Also logs marking. Note that this
   isn't a completely accurate view of the heap since it doesn't include all
   butterfly resize operations, doesn't tell you what is going on with weak
   references (other than to tell you when they're marked), and doesn't
   track direct mmap() allocations or things like JIT allocation. */
#if !defined(ENABLE_ALLOCATION_LOGGING)
#define ENABLE_ALLOCATION_LOGGING 0
#endif

/* Enable verification that that register allocations are not made within generated control flow.
   Turned on for debug builds. */
#if !defined(ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION) && ENABLE(DFG_JIT)
#if !defined(NDEBUG)
#define ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION 1
#else
#define ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION 0
#endif
#endif

/* Configure the JIT */
#if CPU(X86) && COMPILER(MSVC)
#define JSC_HOST_CALL __fastcall
#elif CPU(X86) && COMPILER(GCC)
#define JSC_HOST_CALL __attribute__ ((fastcall))
#else
#define JSC_HOST_CALL
#endif

/* Configure the interpreter */
#if COMPILER(GCC) || (COMPILER(RVCT) && defined(__GNUC__))
#define HAVE_COMPUTED_GOTO 1
#endif

/* Determine if we need to enable Computed Goto Opcodes or not: */
#if HAVE(COMPUTED_GOTO) || ENABLE(JIT)
#define ENABLE_COMPUTED_GOTO_OPCODES 1
#endif

/* Regular Expression Tracing - Set to 1 to trace RegExp's in jsc.  Results dumped at exit */
#define ENABLE_REGEXP_TRACING 0

/* Yet Another Regex Runtime - turned on by default for JIT enabled ports. */
#if !defined(ENABLE_YARR_JIT) && ENABLE(JIT)
#define ENABLE_YARR_JIT 1

/* Setting this flag compares JIT results with interpreter results. */
#define ENABLE_YARR_JIT_DEBUG 0
#endif

/* If either the JIT or the RegExp JIT is enabled, then the Assembler must be
   enabled as well: */
#if ENABLE(JIT) || ENABLE(YARR_JIT)
#if defined(ENABLE_ASSEMBLER) && !ENABLE_ASSEMBLER
#error "Cannot enable the JIT or RegExp JIT without enabling the Assembler"
#else
#undef ENABLE_ASSEMBLER
#define ENABLE_ASSEMBLER 1
#endif
#endif

/* If the Disassembler is enabled, then the Assembler must be enabled as well: */
#if ENABLE(DISASSEMBLER)
#if defined(ENABLE_ASSEMBLER) && !ENABLE_ASSEMBLER
#error "Cannot enable the Disassembler without enabling the Assembler"
#else
#undef ENABLE_ASSEMBLER
#define ENABLE_ASSEMBLER 1
#endif
#endif

#if ENABLE(JIT)
/* Enable the following if you want to use the MacroAssembler::probe() facility
   to do JIT debugging. */
#define ENABLE_MASM_PROBE 0
#endif

/* Pick which allocator to use; we only need an executable allocator if the assembler is compiled in.
   On non-Windows x86-64, iOS, and ARM64 we use a single fixed mmap, on other platforms we mmap on demand. */
#if ENABLE(ASSEMBLER)
#if (CPU(X86_64) || CPU(ARM64)) && (!OS(WINDOWS) && !OS(WINDOWS_WKC)) || PLATFORM(IOS)
#define ENABLE_EXECUTABLE_ALLOCATOR_FIXED 1
#else
#define ENABLE_EXECUTABLE_ALLOCATOR_DEMAND 1
#endif
#endif

/* CSS Selector JIT Compiler */
#if !defined(ENABLE_CSS_SELECTOR_JIT)
#if (CPU(X86_64) || CPU(ARM64) || (CPU(ARM_THUMB2) && PLATFORM(IOS))) && ENABLE(JIT) && (OS(DARWIN) || PLATFORM(GTK) || PLATFORM(EFL))
#define ENABLE_CSS_SELECTOR_JIT 1
#else
#define ENABLE_CSS_SELECTOR_JIT 0
#endif
#endif

#if ENABLE(WEBGL) && PLATFORM(WIN)
#define USE_OPENGL 1
#define USE_OPENGL_ES_2 1
#define USE_EGL 1
#endif

#if ENABLE(VIDEO) && PLATFORM(WIN_CAIRO)
#if ENABLE(GSTREAMER_WINCAIRO)
#define USE_MEDIA_FOUNDATION 0
#define USE_GLIB 1
#define USE_GSTREAMER 1
#else
#define USE_MEDIA_FOUNDATION 1
#endif
#endif

#if PLATFORM(WIN_CAIRO)
#define USE_TEXTURE_MAPPER 1
#endif

#if USE(TEXTURE_MAPPER) && ENABLE(GRAPHICS_CONTEXT_3D) && !defined(USE_TEXTURE_MAPPER_GL)
#define USE_TEXTURE_MAPPER_GL 1
#endif

/* Compositing on the UI-process in WebKit2 */
#if PLATFORM(COCOA)
#define USE_PROTECTION_SPACE_AUTH_CALLBACK 1
#endif

/* Set up a define for a common error that is intended to cause a build error -- thus the space after Error. */
#define WTF_PLATFORM_CFNETWORK Error USE_macro_should_be_used_with_CFNETWORK

#if PLATFORM(WIN)
#define USE_CROSS_PLATFORM_CONTEXT_MENUS 1
#endif

#if PLATFORM(COCOA) && HAVE(ACCESSIBILITY)
#define USE_ACCESSIBILITY_CONTEXT_MENUS 1
#endif

#if CPU(ARM_THUMB2) || CPU(ARM64)
#define ENABLE_BRANCH_COMPACTION 1
#endif

#if !defined(ENABLE_THREADING_LIBDISPATCH) && HAVE(DISPATCH_H)
#define ENABLE_THREADING_LIBDISPATCH 1
#elif !defined(ENABLE_THREADING_OPENMP) && defined(_OPENMP)
#define ENABLE_THREADING_OPENMP 1
#elif !defined(THREADING_GENERIC)
#define ENABLE_THREADING_GENERIC 1
#endif

#if USE(GLIB)
#include <wtf/gobject/GTypedefs.h>
#endif

#if PLATFORM(EFL)
#include <wtf/efl/EflTypedefs.h>
#endif

/* FIXME: This define won't be needed once #27551 is fully landed. However, 
   since most ports try to support sub-project independence, adding new headers
   to WTF causes many ports to break, and so this way we can address the build
   breakages one port at a time. */
#if !defined(USE_EXPORT_MACROS) && (PLATFORM(COCOA) || PLATFORM(WIN))
#define USE_EXPORT_MACROS 1
#endif

#if !defined(USE_EXPORT_MACROS_FOR_TESTING) && (PLATFORM(GTK) || PLATFORM(WIN))
#define USE_EXPORT_MACROS_FOR_TESTING 1
#endif

#if PLATFORM(GTK) || PLATFORM(EFL)
#define USE_UNIX_DOMAIN_SOCKETS 1
#endif

#if !defined(USE_IMLANG_FONT_LINK2) && !OS(WINCE)
#define USE_IMLANG_FONT_LINK2 1
#endif

#if !defined(ENABLE_COMPARE_AND_SWAP) && (OS(WINDOWS) || (COMPILER(GCC) && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2) || CPU(ARM64))))
#define ENABLE_COMPARE_AND_SWAP 1
#endif

#if !defined(ENABLE_PARALLEL_GC) && (OS(DARWIN) || PLATFORM(EFL) || PLATFORM(GTK)) && ENABLE(COMPARE_AND_SWAP)
#define ENABLE_PARALLEL_GC 1
#endif

#if !defined(ENABLE_GC_VALIDATION) && !defined(NDEBUG)
#define ENABLE_GC_VALIDATION 1
#endif

#if !defined(ENABLE_BINDING_INTEGRITY) && !OS(WINDOWS) && !PLATFORM(WKC)
#define ENABLE_BINDING_INTEGRITY 1
#endif

#if PLATFORM(COCOA)
#define USE_AVFOUNDATION 1
#endif

#if !defined(ENABLE_TREE_DEBUGGING)
#if !defined(NDEBUG)
#define ENABLE_TREE_DEBUGGING 1
#else
#define ENABLE_TREE_DEBUGGING 0
#endif
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000) || PLATFORM(MAC)
#define USE_COREMEDIA 1
#define HAVE_AVFOUNDATION_VIDEO_OUTPUT 1
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 50000) || (PLATFORM(MAC) || (OS(WINDOWS) && USE(CG)) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080)
#define HAVE_AVFOUNDATION_MEDIA_SELECTION_GROUP 1
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 70000) || ((PLATFORM(MAC) || (OS(WINDOWS) && USE(CG))) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090)
#define HAVE_AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT 1
#define HAVE_MEDIA_ACCESSIBILITY_FRAMEWORK 1
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000) || PLATFORM(MAC)
#define HAVE_AVFOUNDATION_LOADER_DELEGATE 1
#endif

#if PLATFORM(MAC)
#define USE_VIDEOTOOLBOX 1
#endif

#if PLATFORM(COCOA) || PLATFORM(GTK) || (PLATFORM(WIN) && !USE(WINGDI))
#define USE_REQUEST_ANIMATION_FRAME_TIMER 1
#endif

#if PLATFORM(COCOA)
#define USE_REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR 1
#endif

#if PLATFORM(MAC)
#define USE_COREAUDIO 1
#endif

#if !defined(USE_ZLIB)
#define USE_ZLIB 1
#endif

#if PLATFORM(COCOA)
#define USE_CONTENT_FILTERING 1
#endif

#ifndef HAVE_QOS_CLASSES
#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
#define HAVE_QOS_CLASSES 1
#endif
#endif

#ifndef HAVE_VOUCHERS
#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000)
#define HAVE_VOUCHERS 1
#endif
#endif

#define USE_GRAMMAR_CHECKING 1

#if PLATFORM(COCOA) || PLATFORM(EFL) || PLATFORM(GTK)
#define USE_UNIFIED_TEXT_CHECKING 1
#endif
#if PLATFORM(MAC)
#define USE_AUTOMATIC_TEXT_REPLACEMENT 1
#endif

#if PLATFORM(MAC)
/* Some platforms provide UI for suggesting autocorrection. */
#define USE_AUTOCORRECTION_PANEL 1
#endif

#if PLATFORM(COCOA)
/* Some platforms use spelling and autocorrection markers to provide visual cue. On such platform, if word with marker is edited, we need to remove the marker. */
#define USE_MARKER_REMOVAL_UPON_EDITING 1
#endif

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
#define USE_INSERTION_UNDO_GROUPING 1
#endif

#if (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101100) || (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 90000)
#define HAVE_TIMINGDATAOPTIONS 1
#endif

#if PLATFORM(IOS)
#define USE_PLATFORM_TEXT_TRACK_MENU 1
#endif

#if PLATFORM(COCOA)
#define USE_AUDIO_SESSION 1
#endif

#if PLATFORM(COCOA) && !PLATFORM(IOS_SIMULATOR)
#define USE_IOSURFACE 1
#endif

#if PLATFORM(GTK) || PLATFORM(EFL)
#undef ENABLE_OPENTYPE_VERTICAL
#define ENABLE_OPENTYPE_VERTICAL 1
#define ENABLE_CSS3_TEXT_DECORATION_SKIP_INK 1
#endif

#if PLATFORM(COCOA)
#define ENABLE_CSS3_TEXT_DECORATION_SKIP_INK 1
#endif

#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED > 101000)
#define ENABLE_PLATFORM_FONT_LOOKUP 1
#endif

#if COMPILER(MSVC) && !PLATFORM(WKC)
#undef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#undef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 1
#endif

#if PLATFORM(MAC)
#define HAVE_NS_ACTIVITY 1
#endif

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
#define USE_ASYNC_NSTEXTINPUTCLIENT 1
#endif

#if PLATFORM(COCOA)
#if defined __has_include && __has_include(<CoreFoundation/CFPriv.h>)
#define USE_APPLE_INTERNAL_SDK 1
#endif
#endif

#if (OS(DARWIN) && USE(CG)) || USE(FREETYPE) || (PLATFORM(WIN) && (USE(CG) || USE(CAIRO)))
#undef ENABLE_OPENTYPE_MATH
#define ENABLE_OPENTYPE_MATH 1
#endif

/* Set TARGET_OS_IPHONE to 0 by default to allow using it as a guard 
 * in cross-platform the same way as it is used in OS(DARWIN) code. */ 
#if !defined(TARGET_OS_IPHONE) && !OS(DARWIN)
#define TARGET_OS_IPHONE 0
#endif

#if PLATFORM(COCOA)
#define USE_MEDIATOOLBOX 1
#endif

#if PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 90000
#define ENABLE_VIDEO_PRESENTATION_MODE 1
#endif

/* While 10.10 has support for fences, it is missing some API important for our integration of them. */
#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101100)
#define HAVE_COREANIMATION_FENCES 1
#endif

// static configurations for WKC

#define USE_TLSF 1
#define USE_PTHREADS 0
#define USE_JSC 1
#define USE_V8 0

#define USE_WKC_UNICODE 1

#define HAVE_PTHREAD_NP_H 0
#undef WTF_FEATURE_WIN_OS
#undef WTF_PLATFORM_WIN_OS
#undef WTF_PLATFORM_WIN
#undef USE_WININET
#undef HAVE_STRINGS_H
#undef HAVE_SYS_TIME_H
#undef HAVE_MMAP
#undef HAVE_VIRTUALALLOC
#undef HAVE_LANGINFO_H
#undef HAVE_SYS_PARAM_H
#undef ENABLE_WREC
#undef WTF_PLATFORM_CAIRO
#undef USE_SYSTEM_MALLOC

//#define ENABLE_IMAGE_DECODER_DOWN_SAMPLING 1

#define USE_CURL 1

// enable optimization to ignore fixed background images when scrolling a page.
#define ENABLE_FAST_MOBILE_SCROLLING 0

#undef ENABLE_REPLACEMENT_SYSTEMMEMORY

#ifndef DISABLE_WKC_REPLACE_NEWDELETE_WITH_FASTMALLOC
# define ENABLE_WKC_REPLACE_NEWDELETE_WITH_FASTMALLOC 1
#endif

// enable HTTPCache
#define ENABLE_WKC_HTTPCACHE 1

#define JS_EXPORTDATA
#define __N p__N

#ifndef __BUILDING_IN_VS__
# if CPU(X86_64) || CPU(ARM64)
#  include <stddef.h>
# else
typedef unsigned int size_t;
# endif
#endif

typedef void* LPVOID;
#define _CRT_TERMINATE_DEFINED
#define MAX_PATH            260
#if !defined(_TRUNCATE)
#define _TRUNCATE ((size_t)-1)
#endif

#define WEBCORE_EXPORT

// Disable CRASH in ASSERT by Debug build.
#define CRASH_DEBUG_ASSERT_DISABLED 1

#endif /* WKCPLATFORM_H */
