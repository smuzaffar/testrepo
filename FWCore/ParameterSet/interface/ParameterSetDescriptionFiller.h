#ifndef FWCore_ParameterSet_ParameterSetDescriptionFiller_h
#define FWCore_ParameterSet_ParameterSetDescriptionFiller_h
// -*- C++ -*-
//
// Package:     ParameterSet
// Class  :     ParameterSetDescriptionFiller
// 
/**\class ParameterSetDescriptionFiller ParameterSetDescriptionFiller.h FWCore/ParameterSet/interface/ParameterSetDescriptionFiller.h

 Description: A concrete ParameterSetDescription filler which calls a static function of the template argument

 Usage:
    This is an ParameterSetDescription filler adapter class which calls the 

void fillDescription(edm::ParameterSetDescription&)

method of the templated argument.  This allows the ParameterSetDescriptionFillerPluginFactory to communicate with existing plugins.

*/
//
// Original Author:  Chris Jones
//         Created:  Wed Aug  1 16:46:56 EDT 2007
// $Id: ParameterSetDescriptionFiller.h,v 1.2 2008/11/14 19:41:22 wdd Exp $
//

// system include files

// user include files
#include "FWCore/ParameterSet/interface/ParameterSetDescriptionFillerBase.h"

#include <string>

// forward declarations

namespace edm {
template< typename T>
  class ParameterSetDescriptionFiller : public ParameterSetDescriptionFillerBase
{

   public:
      ParameterSetDescriptionFiller() {}
      //virtual ~ParameterSetDescriptionFiller();

      // ---------- const member functions ---------------------
      virtual void fill(ConfigurationDescriptions & descriptions) const {
        T::fillDescriptions(descriptions);
      }

      virtual std::string baseType() const {
        return T::baseType();
      }

      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------

   private:
      ParameterSetDescriptionFiller(const ParameterSetDescriptionFiller&); // stop default

      const ParameterSetDescriptionFiller& operator=(const ParameterSetDescriptionFiller&); // stop default

      // ---------- member data --------------------------------

};

}
#endif
