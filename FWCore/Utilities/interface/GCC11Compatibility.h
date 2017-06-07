#ifndef FWCORE_GCC11COMPATIBILITY_H
#define FWCORE_GCC11COMPATIBILITY_H
/*
 * set of macro to control the level of comaptibility with c++11 standard
 * includes also other macro more specific to gcc

 */

#include "FWCore/Utilities/interface/Visibility.h"
#include "FWCore/Utilities/interface/Likely.h"


#if defined(__REFLEX__) || defined(__CINT__)
  #define CMS_NOCXX11
#endif
#if !GCC_PREREQUISITE(4,6,0)
  #define CMS_NOCXX11
#endif

#ifndef CMS_NOCXX11
  #if GCC_PREREQUISITE(4,7,0)
     #define GCC11_FINAL final
  #else
     #define GCC11_FINAL  
  #endif // gcc 4.7
#else
  #define GCC11_FINAL
  #define constexpr
  #define noexcept
#endif // NOCXX11

#endif  // FWCORE_GCC11COMPATIBILITY_H
