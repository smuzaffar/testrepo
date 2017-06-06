// -*- C++ -*-
//
// Package:     ParameterSet
// Class  :     ParameterSetDescription
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Tue Jul 31 15:30:35 EDT 2007
// $Id$
//

// system include files

// user include files
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/Exception.h"

//
// constants, enums and typedefs
//

//
// static data member definitions
//
namespace edm {
//
// constructors and destructor
//
  ParameterSetDescription::ParameterSetDescription():
  anythingAllowed_(false),
  unknown_(false)
{
}

// ParameterSetDescription::ParameterSetDescription(const ParameterSetDescription& rhs)
// {
//    // do actual copying here;
// }

ParameterSetDescription::~ParameterSetDescription()
{
}

//
// assignment operators
//
// const ParameterSetDescription& ParameterSetDescription::operator=(const ParameterSetDescription& rhs)
// {
//   //An exception safe implementation is
//   ParameterSetDescription temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//
void 
ParameterSetDescription::setAllowAnything()
{
  anythingAllowed_ = true;
}

void 
ParameterSetDescription::setUnknown()
{
  unknown_ = true;
}

//
// const member functions
//
void
ParameterSetDescription::validate(const edm::ParameterSet& ) const
{ 
  //do nothing for now
  return;
  /*
  //Change this to 'not unknown_' when you want to enforce that all
  // parameterizables must declare their parameters
  if (unknown_) {
    return;
  }
  if( not anythingAllowed() ) {
    throw cms::Exception("InvalidConfiguration");
  }
   */
}

//
// static member functions
//
}
