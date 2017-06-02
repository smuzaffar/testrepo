#ifndef Integration_OtherThingAlgorithm_h
#define Integration_OtherThingAlgorithm_h

#include <string>
#include "DataFormats/TestObjects/interface/OtherThingCollectionfwd.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"

namespace edmtest {

  class OtherThingAlgorithm {
  public:
    OtherThingAlgorithm() : theDebugLevel(0) {}
  
    /// Runs the algorithm and returns a list of OtherThings
    /// The user declares the vector and calls this method.
    void run(edm::DataViewImpl const& dv, 
	     OtherThingCollection& otherThingCollection, 
	     std::string const& thingLabel = std::string("Thing"),
	     std::string const& instance = std::string());
  
  private:
    int    theDebugLevel;
  };

}
  
#endif
