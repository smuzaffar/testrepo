#ifndef FWCore_Framework_LuminosityBlockPrincipal_h
#define FWCore_Framework_LuminosityBlockPrincipal_h

/*----------------------------------------------------------------------

LuminosityBlockPrincipal: This is the class responsible for management of
per luminosity block EDProducts. It is not seen by reconstruction code;
such code sees the LuminosityBlock class, which is a proxy for LuminosityBlockPrincipal.

The major internal component of the LuminosityBlockPrincipal
is the DataBlock.

----------------------------------------------------------------------*/

#include <memory>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "DataFormats/Provenance/interface/BranchMapper.h"
#include "DataFormats/Provenance/interface/LuminosityBlockAuxiliary.h"
#include "DataFormats/Provenance/interface/RunID.h"
#include "FWCore/Framework/interface/NoDelayedReader.h"
#include "FWCore/Framework/interface/Principal.h"

namespace edm {
  class RunPrincipal;
  class UnscheduledHandler;
  class LuminosityBlockPrincipal : public Principal {
  public:
    typedef LuminosityBlockAuxiliary Auxiliary;
    typedef Principal Base;
    LuminosityBlockPrincipal(
        boost::shared_ptr<LuminosityBlockAuxiliary> aux,
        boost::shared_ptr<ProductRegistry const> reg,
        ProcessConfiguration const& pc,
        boost::shared_ptr<RunPrincipal> rp);

    ~LuminosityBlockPrincipal() {}

    void fillLuminosityBlockPrincipal(
        boost::shared_ptr<BranchMapper> mapper = boost::shared_ptr<BranchMapper>(),
        boost::shared_ptr<DelayedReader> rtrv = boost::shared_ptr<DelayedReader>());

    RunPrincipal const& runPrincipal() const {
      return *runPrincipal_;
    }

    RunPrincipal& runPrincipal() {
      return *runPrincipal_;
    }

    LuminosityBlockID id() const {
      return aux().id();
    }

    Timestamp const& beginTime() const {
      return aux().beginTime();
    }

    Timestamp const& endTime() const {
      return aux().endTime();
    }

    void setEndTime(Timestamp const& time) {
      aux_->setEndTime(time);
    }

    LuminosityBlockNumber_t luminosityBlock() const {
      return aux().luminosityBlock();
    }

    LuminosityBlockAuxiliary const& aux() const {
      return *aux_;
    }

    RunNumber_t run() const {
      return aux().run();
    }

    void mergeAuxiliary(LuminosityBlockAuxiliary const& aux) {
      return aux_->mergeAuxiliary(aux);
    }

    void setUnscheduledHandler(boost::shared_ptr<UnscheduledHandler>) {}

    void put(
        ConstBranchDescription const& bd,
        WrapperHolder const& edp,
        std::auto_ptr<ProductProvenance> productProvenance);

    void readImmediate() const;

  private:
    virtual bool unscheduledFill(std::string const&) const {return false;}

    void resolveProductImmediate(Group const& g) const;

    boost::shared_ptr<RunPrincipal> runPrincipal_;

    boost::shared_ptr<LuminosityBlockAuxiliary> aux_;
  };
}
#endif

