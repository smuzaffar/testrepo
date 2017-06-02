
/*----------------------------------------------------------------------
$Id: OutputWorker.cc,v 1.16 2006/04/15 04:45:43 wmtan Exp $
----------------------------------------------------------------------*/

#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/OutputModule.h"
#include "FWCore/Framework/interface/Actions.h"
#include "DataFormats/Common/interface/ModuleDescription.h"
#include "FWCore/Framework/src/WorkerParams.h"
#include "FWCore/Framework/src/OutputWorker.h"
#include "FWCore/Utilities/interface/Exception.h"

#include <iostream>

using namespace std;

namespace edm {
  OutputWorker::OutputWorker(std::auto_ptr<OutputModule> mod,
			     ModuleDescription const& md,
			     WorkerParams const& wp):
      Worker(md,wp),
      mod_(mod) {
  }

  OutputWorker::~OutputWorker() {
  }

  bool 
  OutputWorker::implDoWork(EventPrincipal& ep, EventSetup const&,
			   CurrentProcessingContext const* cpc) {
    // EventSetup is not (yet) used. Should it be passed to the
    // OutputModule?
    bool rc = false;

    mod_->writeEvent(ep,description(), cpc);
    rc=true;
    return rc;
  }

  void 
  OutputWorker::implBeginJob(EventSetup const& es) {
    mod_->selectProducts();
    mod_->beginJob(es);
  }

  void 
  OutputWorker::implEndJob() {
    mod_->endJob();
  }

  std::string OutputWorker::workerType() const {
    return "OutputModule";
  }
  
}
