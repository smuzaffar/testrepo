#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"

#include <algorithm>

namespace edm {
  EventPrincipal::EventPrincipal(EventID const& id,
	Timestamp const& time,
	ProductRegistry const& reg,
        boost::shared_ptr<LuminosityBlockPrincipal> lbp,
	ProcessConfiguration const& pc,
	ProcessHistoryID const& hist,
	boost::shared_ptr<DelayedReader> rtrv) :
	  Base(reg, pc, hist, rtrv),
	  aux_(id, time, lbp->luminosityBlock()),
	  luminosityBlockPrincipal_(lbp),
	  unscheduledHandler_() { }

  EventPrincipal::EventPrincipal(EventID const& id,
	Timestamp const& time,
	ProductRegistry const& reg,
	LuminosityBlockNumber_t lumi,
	ProcessConfiguration const& pc,
	ProcessHistoryID const& hist,
	boost::shared_ptr<DelayedReader> rtrv) :
	  Base(reg, pc, hist, rtrv),
	  aux_(id, time, lumi),
	  luminosityBlockPrincipal_(new LuminosityBlockPrincipal(lumi, reg, id.run(), pc)),
	  unscheduledHandler_() { }

  RunPrincipal const&
  EventPrincipal::runPrincipal() const {
    return luminosityBlockPrincipal().runPrincipal();
  }

  RunPrincipal &
  EventPrincipal::runPrincipal() {
    return luminosityBlockPrincipal().runPrincipal();
  }

  void
  EventPrincipal::setUnscheduledHandler(boost::shared_ptr<UnscheduledHandler> iHandler) {
    unscheduledHandler_ = iHandler;
  }

  bool
  EventPrincipal::unscheduledFill(Group const& group) const {

    // If it is a module already currently running in unscheduled
    // mode, then there is a circular dependency related to which
    // EDProducts modules require and produce.  There is no safe way
    // to recover from this.  Here we check for this problem and throw
    // an exception.
    std::vector<std::string>::const_iterator i =
      std::find(moduleLabelsRunning_.begin(),
                moduleLabelsRunning_.end(),
                group.moduleLabel());

    if (i != moduleLabelsRunning_.end()) {
      throw edm::Exception(errors::LogicError)
        << "Hit circular dependency while trying to run an unscheduled module.\n"
        << "Current implementation of unscheduled execution cannot always determine\n"
        << "the proper order for module execution.  It is also possible the modules\n"
        << "have a built in circular dependence that will not work with any order.\n"
        << "In the first case, scheduling some or all required modules in paths will help.\n"
        << "In the second case, the modules themselves will have to be fixed.\n";
    }

    moduleLabelsRunning_.push_back(group.moduleLabel());

    if (unscheduledHandler_) {
      unscheduledHandler_->tryToFill(group.provenance(), *const_cast<EventPrincipal *>(this));
    }
    moduleLabelsRunning_.pop_back();
    return true;
  }
}
