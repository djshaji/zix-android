// Copyright 2021-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/*
  Configuration header that defines reasonable defaults at compile time.

  This allows compile-time configuration from the command line, while still
  allowing the source to be built "as-is" without any configuration.  The idea
  is to support an advanced build system with configuration checks, while still
  allowing the code to be simply "thrown at a compiler" with features
  determined from the compiler or system headers.  Everything can be
  overridden, so it should never be necessary to edit this file to build
  successfully.

  To ensure that all configure checks are performed, the build system can
  define ZIX_NO_DEFAULT_CONFIG to disable defaults.  In this case, it must
  define all HAVE_FEATURE symbols below to 1 or 0 to enable or disable
  features.  Any missing definitions will generate a compiler warning.

  To ensure that this header is always included properly, all code that uses
  configuration variables includes this header and checks their value with #if
  (not #ifdef).  Variables like USE_FEATURE are internal and should never be
  defined on the command line.
*/

#ifndef ZIX_CONFIG_H
#define ZIX_CONFIG_H

#if !defined(ZIX_NO_DEFAULT_CONFIG)

// We need unistd.h to check _POSIX_VERSION
#  ifndef ZIX_NO_POSIX
#    ifdef __has_include
#      if __has_include(<unistd.h>)
#        include <unistd.h>
#      endif
#    elif defined(__APPLE__) || defined(__unix__)
#      include <unistd.h>
#    endif
#  endif

// POSIX.1-2001: clock_gettime()
#  ifndef HAVE_CLOCK_GETTIME
#    if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#      define HAVE_CLOCK_GETTIME 1
#    else
#      define HAVE_CLOCK_GETTIME 0
#    endif
#  endif

// POSIX.1-2001: mlock()
#  ifndef HAVE_MLOCK
#    if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#      define HAVE_MLOCK 1
#    else
#      define HAVE_MLOCK 0
#    endif
#  endif

// POSIX.1-2001: posix_memalign()
#  ifndef HAVE_POSIX_MEMALIGN
#    if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#      define HAVE_POSIX_MEMALIGN 1
#    else
#      define HAVE_POSIX_MEMALIGN 0
#    endif
#  endif

// POSIX.1-2001: sem_timedwait()
#  ifndef HAVE_SEM_TIMEDWAIT
#    if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200112L
#      define HAVE_SEM_TIMEDWAIT 1
#    else
#      define HAVE_SEM_TIMEDWAIT 0
#    endif
#  endif

#endif // !defined(ZIX_NO_DEFAULT_CONFIG)

/*
  Make corresponding USE_FEATURE defines based on the HAVE_FEATURE defines from
  above or the command line.  The code checks for these using #if (not #ifdef),
  so there will be an undefined warning if it checks for an unknown feature,
  and this header is always required by any code that checks for features, even
  if the build system defines them all.
*/

#if HAVE_CLOCK_GETTIME
#  define USE_CLOCK_GETTIME 1
#else
#  define USE_CLOCK_GETTIME 0
#endif

#if HAVE_MLOCK
#  define USE_MLOCK 1
#else
#  define USE_MLOCK 0
#endif

#if HAVE_POSIX_MEMALIGN
#  define USE_POSIX_MEMALIGN 1
#else
#  define USE_POSIX_MEMALIGN 0
#endif

#if HAVE_SEM_TIMEDWAIT
#  define USE_SEM_TIMEDWAIT 1
#else
#  define USE_SEM_TIMEDWAIT 0
#endif

#endif // ZIX_CONFIG_H
