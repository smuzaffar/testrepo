#ifndef Streamer_StreamedProducts_h
#define Streamer_StreamedProducts_h

/*
  Simple packaging of all the event data that is needed to be serialized
  for transfer.

  The "other stuff in the SendEvent still needs to be
  populated.

  The product is paired with its provenance, and the entire event
  is captured in the SendEvent structure.
 */

#include <vector>

#include "FWCore/EDProduct/interface/EDProduct.h"
#include "FWCore/Framework/interface/Provenance.h"
#include "FWCore/Framework/interface/ProductDescription.h"
#include "FWCore/EDProduct/interface/Timestamp.h"
#include "FWCore/EDProduct/interface/EventID.h"

namespace edm {

  // ------------------------------------------

  struct ProdPair
  {
    ProdPair():
      prod_(),prov_(),desc_() { }
    explicit ProdPair(const EDProduct* p):
      prod_(p),prov_(),desc_() { }
    ProdPair(const EDProduct* prod,
	     const Provenance* prov):
      prod_(prod),
      prov_(&prov->event),
      desc_(&prov->product) { }

    const BranchEntryDescription* prov() const { return prov_; }
    const EDProduct* prod() const { return prod_; }
    const ProductDescription* desc() const { return desc_; }

    void clear() { prod_=0; prov_=0; desc_=0; }

    const EDProduct* prod_;
    const BranchEntryDescription* prov_;
    const ProductDescription* desc_;
  };

  // ------------------------------------------

  typedef std::vector<ProdPair> SendProds;

  // ------------------------------------------

  struct SendEvent
  {
    SendEvent() { }
    SendEvent(const EventID& id, const Timestamp& t):id_(id),time_(t) { }

    EventID id_;
    Timestamp time_;
    SendProds prods_;

    // other tables necessary for provenance lookup
  };

  typedef std::vector<ProductDescription> SendDescs;

  struct SendJobHeader
  {
    SendJobHeader() { }

    SendDescs descs_;
    // trigger bit descriptions will be added here and permanent
    //  provenance values
  };

}
#endif

