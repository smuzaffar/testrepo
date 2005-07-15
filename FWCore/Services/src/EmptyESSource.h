#ifndef FWCORESERVICES_EMPTYESSOURCE_H
#define FWCORESERVICES_EMPTYESSOURCE_H
// -*- C++ -*-
//
// Package:     Services
// Class  :     EmptyESSource
// 
/**\class EmptyESSource EmptyESSource.h FWCore/Services/src/EmptyESSource.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Sat Jun 25 17:23:12 EDT 2005
// $Id: EmptyESSource.h,v 1.1 2005/06/25 22:30:35 chrjones Exp $
//

// system include files
#include <set>

// user include files
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"

// forward declarations
namespace edm {
class ParameterSet;
   
class EmptyESSource : public  eventsetup::EventSetupRecordIntervalFinder
{

   public:
      EmptyESSource(const ParameterSet& );
      //virtual ~EmptyESSource();

      // ---------- const member functions ---------------------

      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
   void setIntervalFor( const edm::eventsetup::EventSetupRecordKey&,
                        const edm::Timestamp& iTime, 
                        edm::ValidityInterval& oInterval );
      
   private:
      EmptyESSource( const EmptyESSource& ); // stop default

      const EmptyESSource& operator=( const EmptyESSource& ); // stop default
      
      // ---------- member data --------------------------------
      std::string recordName_;
      std::set <edm::Timestamp> setOfIOV_;
      
};
}

#endif /* FWCORESERVICES_EMPTYESSOURCE_H */
