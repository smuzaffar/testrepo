/*----------------------------------------------------------------------
$Id: AsciiOutputModule.cc,v 1.6 2005/07/14 21:32:14 wmtan Exp $
----------------------------------------------------------------------*/

#include <algorithm>
#include <iostream>
#include <iterator>
#include <ostream>

#include "FWCore/Services/src/AsciiOutputModule.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/OutputModule.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edm {

  AsciiOutputModule::AsciiOutputModule(ParameterSet const& pset, std::ostream* os) :
    OutputModule(pset.getUntrackedParameter("select", ParameterSet())),
    prescale_(pset.getUntrackedParameter("prescale", 1U)),
    verbosity_(pset.getUntrackedParameter("verbosity", 1U)),
    counter_(0),
    pout_(os) {
    if (prescale_ == 0) prescale_ = 1;
  }

  AsciiOutputModule::~AsciiOutputModule() {
    *pout_ << ">>> processed " << counter_ << " events" << std::endl;
  }

  void
  AsciiOutputModule::write(const EventPrincipal& e) {


    if ((++counter_ % prescale_) != 0 || verbosity_ <= 0) return;

    //  const Run & run = evt.getRun(); // this is still unused
    *pout_ << ">>> processing event # " << e.id() << std::endl;

    if (verbosity_ <= 1) return;

    // Write out non-EDProduct contents...

    // ... list of process-names
    std::copy(e.beginProcess(),
	      e.endProcess(),
	      std::ostream_iterator<EventPrincipal::ProcessNameList::value_type>(*pout_, " "));

    // ... collision id
    *pout_ << '\n' << e.id() << '\n';
    
    // Loop over groups, and write some output for each...

    for(EventPrincipal::const_iterator i = e.begin(); i != e.end(); ++i) {
      Provenance const& prov = (*i)->provenance();
      if (selected(prov)) {
        *pout_ << *i << '\n';
      }
    }
  }
}
