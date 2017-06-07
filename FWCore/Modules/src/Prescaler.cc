
#include "FWCore/Framework/interface/EDFilter.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

namespace edm {
  class Prescaler : public EDFilter {
  public:
    explicit Prescaler(ParameterSet const&);
    virtual ~Prescaler();

    static void fillDescriptions(ConfigurationDescriptions& descriptions);
    virtual bool filter(Event& e, EventSetup const& c);
    void endJob();

  private:
    int count_;
    int n_; // accept one in n
    int offset_; // with offset, ie. sequence of events does not have to start at first event
  };

  Prescaler::Prescaler(ParameterSet const& ps) :
    count_(),
    n_(ps.getParameter<int>("prescaleFactor")),
    offset_(ps.getParameter<int>("prescaleOffset")) {
  }

  Prescaler::~Prescaler() {
  }

  bool Prescaler::filter(Event&, EventSetup const&) {
    ++count_;
    return count_ % n_ == offset_ ? true : false;
  }

  void Prescaler::endJob() {
  }

  void
  Prescaler::fillDescriptions(ConfigurationDescriptions& descriptions) {
    ParameterSetDescription desc;
    desc.add<int>("prescaleFactor")->setComment("Accept one event every N events");
    desc.add<int>("prescaleOffset")->setComment("The first event to accept should be the Mth one. Choose 'prescaleFactor'=1 to accept the first event from the source.");
    descriptions.add("preScaler", desc);
  }
}

using edm::Prescaler;
DEFINE_FWK_MODULE(Prescaler);
