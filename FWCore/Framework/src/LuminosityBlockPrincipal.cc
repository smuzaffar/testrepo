#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"

#include "boost/shared_ptr.hpp"

namespace edm {

  LuminosityBlockPrincipal::LuminosityBlockPrincipal(LuminosityBlockNumber_t const& lb,
	ProductRegistry const& reg,
        boost::shared_ptr<RunPrincipal> rp,
        ProcessConfiguration const& pc,
	ProcessHistoryID const& hist,
	boost::shared_ptr<DelayedReader> rtrv) :
        Base(reg, pc, hist, rtrv), runPrincipal_(rp), aux_(rp->run(), lb) {}

  LuminosityBlockPrincipal::LuminosityBlockPrincipal(LuminosityBlockNumber_t const& lb,
	ProductRegistry const& reg,
        RunNumber_t run,
        ProcessConfiguration const& pc,
	ProcessHistoryID const& hist,
	boost::shared_ptr<DelayedReader> rtrv) :
        Base(reg, pc, hist, rtrv), runPrincipal_(new RunPrincipal(run, reg, pc)), aux_(run, lb) {}
}

