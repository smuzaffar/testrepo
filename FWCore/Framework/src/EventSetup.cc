// -*- C++ -*-
//
// Package:     CoreFramework
// Module:      EventSetup
// 
// Description: <one line class summary>
//
// Implementation:
//     <Notes on implementation>
//
// Author:      Chris Jones
// Created:     Thu Mar 24 16:27:10 EST 2005
//

// system include files

// user include files
#include "FWCore/CoreFramework/interface/EventSetup.h"

namespace edm {
//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
   EventSetup::EventSetup() : timestamp_(Timestamp::invalidTimestamp())
{
}

// EventSetup::EventSetup( EventSetup const& rhs )
// {
//    // do actual copying here;
// }

EventSetup::~EventSetup()
{
}

//
// assignment operators
//
// EventSetup const& EventSetup::operator=( EventSetup const& rhs )
// {
//   //An exception safe implementation is
//   EventSetup temp(rhs);
//   swap( rhs );
//
//   return *this;
// }

//
// member functions
//
void
EventSetup::setTimestamp(const Timestamp& iTime ) {
   //will ultimately build our list of records
   timestamp_ = iTime;
}

void 
EventSetup::insert(const eventsetup::EventSetupRecordKey& iKey,
                const eventsetup::EventSetupRecord* iRecord)
{
   recordMap_[iKey]= iRecord;
}

void
EventSetup::clear()
{
   recordMap_.clear();
}
//
// const member functions
//
const eventsetup::EventSetupRecord* 
EventSetup::find( const eventsetup::EventSetupRecordKey& iKey ) const
{
   std::map<eventsetup::EventSetupRecordKey, eventsetup::EventSetupRecord const *>::const_iterator itFind
   = recordMap_.find( iKey );
   if( itFind == recordMap_.end() ) {
      return 0;
   }
   return itFind->second;
}

//
// static member functions
//
}
