#ifndef Framework_SelectorBase_h
#define Framework_SelectorBase_h

/*----------------------------------------------------------------------
  
Selector: Base class for all "selector" objects, used to select
EDProducts based on information in the associated Provenance.

Developers who make their own Selectors should inherit from SelectorBase.

$Id: Selector.h,v 1.12 2006/10/04 14:53:20 paterno Exp $

----------------------------------------------------------------------*/

#include "DataFormats/Common/interface/Provenance.h"
#include "FWCore/Framework/interface/ProvenanceAccess.h"

namespace edm 
{
  //------------------------------------------------------------------
  //
  //// Abstract base class SelectorBase
  //
  //------------------------------------------------------------------

  class SelectorBase {
  public:
    virtual ~SelectorBase();
    bool match(ProvenanceAccess const& p) const;
    bool match(Provenance const& p) const;
    virtual SelectorBase* clone() const = 0;

  private:
    virtual bool doMatch(Provenance const& p) const = 0;
  };
}

#endif
