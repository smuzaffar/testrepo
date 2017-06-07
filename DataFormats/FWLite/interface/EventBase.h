#ifndef DataFormats_FWLite_EventBase_h
#define DataFormats_FWLite_EventBase_h
// -*- C++ -*-
//
// Package:     FWLite
// Class  :     EventBase
// 
/**\class EventBase EventBase.h DataFormats/FWLite/interface/EventBase.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  Charles Plager
//         Created:  Tue May  8 15:01:20 EDT 2007
// $Id: 
//
#if !defined(__CINT__) && !defined(__MAKECINT__)
// system include files
#include <string>
#include <typeinfo>
// #include <typeinfo>
// #include <map>
// #include <vector>
// #include <boost/shared_ptr.hpp>
// #include <memory>
// 
// #include "TBranch.h"
// #include "Rtypes.h"
// #include "Reflex/Object.h"
// 
// // user include files
// #include "FWCore/Utilities/interface/TypeID.h"
// #include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
// #include "DataFormats/Provenance/interface/EventProcessHistoryID.h"
// #include "DataFormats/Provenance/interface/EventAuxiliary.h"
// #include "DataFormats/Provenance/interface/EventID.h"
// #include "DataFormats/Provenance/interface/ProductID.h"
// #include "DataFormats/Provenance/interface/FileIndex.h"
// #include "FWCore/FWLite/interface/BranchMapReader.h"

namespace fwlite 
{
   class EventBase
   {
      public:
         EventBase();

         virtual ~EventBase() {}

         virtual bool getByLabel (const std::type_info&, 
                                  const char*, 
                                  const char*, 
                                  const char*, 
                                  void*) const = 0;

         virtual const std::string getBranchNameFor (const std::type_info&, 
                                                     const char*, 
                                                     const char*, 
                                                     const char*) const = 0;

         virtual bool atEnd() const = 0;

      protected:
         
         // The meat of operator++, but with no return value.  Note
         // that C++ could handle just overwriting operator++, but
         // we're afraid that CInt can't.
         virtual void toNext() = 0;
         
         // This is meant for the meat of the toBegin function, but
         // with no return value.  Same explanation as above.
         virtual void toBeginImpl() = 0;

   };
} // fwlite namespace


#endif /*__CINT__ */
#endif
