// -*- C++ -*-
//
// Package:     Framework
// Class  :     IOVSyncValue
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Wed Aug  3 18:35:35 EDT 2005
// $Id: IOVSyncValue.cc,v 1.3 2005/09/01 04:30:51 wmtan Exp $
//

// system include files

// user include files
#include "FWCore/Framework/interface/IOVSyncValue.h"


//
// constants, enums and typedefs
//
namespace edm {
   namespace eventsetup {

//
// static data member definitions
//


//
// constructors and destructor
//
IOVSyncValue::IOVSyncValue(): eventID_(), time_(),
haveID_(true), haveTime_(true)
{
}

IOVSyncValue::IOVSyncValue(const EventID& iID) : eventID_(iID), time_(),
haveID_(true), haveTime_(false)
{
}

IOVSyncValue::IOVSyncValue(const Timestamp& iTime) : eventID_(), time_(iTime),
haveID_(false), haveTime_(true)
{
}

IOVSyncValue::IOVSyncValue(const EventID& iID, const Timestamp& iTime) :
eventID_(iID), time_(iTime),
haveID_(true), haveTime_(true)
{
}

// IOVSyncValue::IOVSyncValue(const IOVSyncValue& rhs)
// {
//    // do actual copying here;
// }

//IOVSyncValue::~IOVSyncValue()
//{
//}

//
// assignment operators
//
// const IOVSyncValue& IOVSyncValue::operator=(const IOVSyncValue& rhs)
// {
//   //An exception safe implementation is
//   IOVSyncValue temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//

//
// const member functions
//

//
// static member functions
//
const IOVSyncValue&
IOVSyncValue::invalidIOVSyncValue() {
   static IOVSyncValue s_invalid;
   return s_invalid;
}
const IOVSyncValue&
IOVSyncValue::endOfTime() {
   static IOVSyncValue s_endOfTime(EventID(0xFFFFFFFFUL, EventID::maxEventNumber()),
                                   Timestamp::endOfTime());
   return s_endOfTime;
}
const IOVSyncValue&
IOVSyncValue::beginOfTime() {
   static IOVSyncValue s_beginOfTime(EventID(1,0), Timestamp::beginOfTime());
   return s_beginOfTime;
}
   }
}
