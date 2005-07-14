/*----------------------------------------------------------------------
$Id: Selector.cc,v 1.1 2005/05/29 02:29:54 wmtan Exp $
----------------------------------------------------------------------*/

#include "FWCore/Framework/interface/Selector.h"

namespace edm
{
  Selector::~Selector()
  { }

  bool
  Selector::match(const Provenance& p) const
  {
    return doMatch(p);
  }
}
