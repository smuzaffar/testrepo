#include "FWCore/Integration/test/OtherThingProducer.h"
#include "DataFormats/TestObjects/interface/OtherThingCollection.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"


namespace edmtest {
  OtherThingProducer::OtherThingProducer(edm::ParameterSet const&): alg_() {
    produces<OtherThingCollection>("testUserTag");
  }

  // Virtual destructor needed.
  OtherThingProducer::~OtherThingProducer() {}  

  // Functions that gets called by framework every event
  void OtherThingProducer::produce(edm::Event& e, edm::EventSetup const&) {
    // Step A: Get Inputs 

    // Step B: Create empty output 
    std::auto_ptr<OtherThingCollection> result(new OtherThingCollection);  //Empty

    // Step C: Invoke the algorithm, passing in inputs (NONE) and getting back outputs.
    alg_.run(e, *result);

    // Step D: Put outputs into event
    e.put(result, std::string("testUserTag"));
  }
}
using edmtest::OtherThingProducer;
DEFINE_FWK_MODULE(OtherThingProducer)
