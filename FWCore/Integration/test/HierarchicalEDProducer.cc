#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Integration/test/HierarchicalEDProducer.h"

namespace edmtest {

  HierarchicalEDProducer::HierarchicalEDProducer(edm::ParameterSet const& ps) :
    radius_ (ps.getParameter<double>("radius")),
    outer_alg_(ps.getParameter<edm::ParameterSet>("nest_1"))
  { }

  // Virtual destructor needed.
  HierarchicalEDProducer::~HierarchicalEDProducer() {}  

  // Functions that gets called by framework every event
  void HierarchicalEDProducer::produce(edm::Event&, edm::EventSetup const&) {
    // nothing to do ... is just a dummy!
  }
}
using edmtest::HierarchicalEDProducer;
DEFINE_FWK_MODULE(HierarchicalEDProducer)
