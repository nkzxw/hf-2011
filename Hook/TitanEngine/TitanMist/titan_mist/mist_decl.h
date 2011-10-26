#pragma once
#include "config.h"

#if defined(MIST_HAS_DECLSPEC)
# if defined(MIST_DYN_LINK)
#   if defined(MIST_SOURCE)
#     define MIST_DECL __declspec(dllexport)
#   else
#     define MIST_DECL __declspec(dllimport)
#   endif
# endif
#endif // defined(MIST_HAS_DECLSPEC

#if !defined(MIST_DECL)
# define MIST_DECL
#endif
