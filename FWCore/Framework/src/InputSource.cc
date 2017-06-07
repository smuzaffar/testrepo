/*----------------------------------------------------------------------
----------------------------------------------------------------------*/
#include <cassert> 
#include "FWCore/Framework/interface/InputSource.h"
#include "FWCore/Framework/interface/InputSourceDescription.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Framework/interface/FileBlock.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "FWCore/Utilities/interface/GlobalIdentifier.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"

namespace edm {

  namespace {
	int const improbable = -65783927;
	std::string const& suffix(int count) {
	  static std::string const st("st");
	  static std::string const nd("nd");
	  static std::string const rd("rd");
	  static std::string const th("th");
	  // *0, *4 - *9 use "th".
	  int lastDigit = count % 10;
	  if (lastDigit >= 4 || lastDigit == 0) return th;
	  // *11, *12, or *13 use "th".
	  if (count % 100 - lastDigit == 10) return th;
	  return (lastDigit == 1 ? st : (lastDigit == 2 ? nd : rd));
        }
	struct do_nothing_deleter {
	  void  operator () (void const*) const {}
	};
	template <typename T>
	boost::shared_ptr<T> createSharedPtrToStatic(T * ptr) {
	  return  boost::shared_ptr<T>(ptr, do_nothing_deleter());
	}
  }
  InputSource::InputSource(ParameterSet const& pset, InputSourceDescription const& desc) :
      ProductRegistryHelper(),
      maxEvents_(desc.maxEvents_),
      remainingEvents_(maxEvents_),
      maxLumis_(desc.maxLumis_),
      remainingLumis_(maxLumis_),
      readCount_(0),
      moduleDescription_(desc.moduleDescription_),
      productRegistry_(createSharedPtrToStatic<ProductRegistry const>(desc.productRegistry_)),
      primary_(pset.getParameter<std::string>("@module_label") == std::string("@main_input")),
      processGUID_(primary_ ? createGlobalIdentifier() : std::string()),
      time_(),
      doneReadAhead_(false),
      state_(IsInvalid),
      runPrincipal_(),
      lumiPrincipal_() {
    // Secondary input sources currently do not have a product registry.
    if (primary_) {
      assert(desc.productRegistry_ != 0);
    }
    int maxEventsOldStyle = pset.getUntrackedParameter<int>("maxEvents", improbable);
    if (maxEventsOldStyle != improbable) {
      throw edm::Exception(edm::errors::Configuration)
        << "InputSource::InputSource()\n"
	<< "The 'maxEvents' parameter for sources is no longer supported.\n"
        << "Please use instead the process level parameter set\n"
        << "'untracked PSet maxEvents = {untracked int32 input = " << maxEventsOldStyle << "}'\n";
    }
  }

  InputSource::~InputSource() {}

  InputSource::ItemType
  InputSource::nextItemType() {
    if (doneReadAhead_) {
      return state_;
    }
    doneReadAhead_ = true;
    ItemType oldState = state_;
    if (eventLimitReached()) {
      // If the maximum event limit has been reached, stop.
      state_ = IsStop;
    } else if (lumiLimitReached()) {
      // If the maximum lumi limit has been reached, stop
      // when reaching a new file, run, or lumi.
      if (oldState == IsInvalid || oldState == IsFile || oldState == IsRun) {
        state_ = IsStop;
      } else {
        ItemType newState = getNextItemType();
	if (newState == IsEvent) {
          state_ = IsEvent;
	} else {
          state_ = IsStop;
	}
      }
    } else {
      ItemType newState = getNextItemType();
      if (newState == IsStop) {
        state_ = IsStop;
      } else if (newState == IsFile || oldState == IsInvalid) {
        state_ = IsFile;
      } else if (newState == IsRun || oldState == IsFile) {
        setRunPrincipal(readRun_());
        state_ = IsRun;
      } else if (newState == IsLumi || oldState == IsRun) {
        setLuminosityBlockPrincipal(readLuminosityBlock_());
        state_ = IsLumi;
      } else {
        state_ = IsEvent;
      }
    }
    if (state_ == IsStop) {
      lumiPrincipal_.reset();
      runPrincipal_.reset();
    }
    return state_;
  }

  void
  InputSource::doBeginJob(EventSetup const& c) {
    beginJob(c);
  }

  void
  InputSource::doEndJob() {
    endJob();
  }

  void
  InputSource::registerProducts() {
    if (!typeLabelList().empty()) {
      addToRegistry(typeLabelList().begin(), typeLabelList().end(), moduleDescription(), productRegistryUpdate());
    }
  }

  // Return a dummy file block.
  boost::shared_ptr<FileBlock>
  InputSource::readFile() {
    assert(doneReadAhead_);
    assert(state_ == IsFile);
    assert(!limitReached());
    doneReadAhead_ = false;
    return readFile_();
  }

  void
  InputSource::closeFile() {
    return closeFile_();
  }

  // Return a dummy file block.
  // This function must be overridden for any input source that reads a file
  // containing Products.
  boost::shared_ptr<FileBlock>
  InputSource::readFile_() {
    if (primary()) {
      productRegistryUpdate().setProductIDs(1U);
    }
    return boost::shared_ptr<FileBlock>(new FileBlock);
  }

  boost::shared_ptr<RunPrincipal>
  InputSource::readRun() {
    // Note: For the moment, we do not support saving and restoring the state of the
    // random number generator if random numbers are generated during processing of runs
    // (e.g. beginRun(), endRun())
    assert(doneReadAhead_);
    assert(state_ == IsRun);
    assert(!limitReached());
    doneReadAhead_ = false;
    return runPrincipal_;
  }

  boost::shared_ptr<LuminosityBlockPrincipal>
  InputSource::readLuminosityBlock(boost::shared_ptr<RunPrincipal> rp) {
    // Note: For the moment, we do not support saving and restoring the state of the
    // random number generator if random numbers are generated during processing of lumi blocks
    // (e.g. beginLuminosityBlock(), endLuminosityBlock())
    assert(doneReadAhead_);
    assert(state_ == IsLumi);
    assert(!limitReached());
    doneReadAhead_ = false;
    --remainingLumis_;
    lumiPrincipal_->setRunPrincipal(rp);
    return lumiPrincipal_;
  }

  std::auto_ptr<EventPrincipal>
  InputSource::readEvent(boost::shared_ptr<LuminosityBlockPrincipal> lbp) {
    assert(doneReadAhead_);
    assert(state_ == IsEvent);
    assert(!eventLimitReached());
    doneReadAhead_ = false;

    preRead();
    std::auto_ptr<EventPrincipal> result = readEvent_(lbp);
    result->setLuminosityBlockPrincipal(lbp);
    if (result.get() != 0) {
      Event event(*result, moduleDescription());
      postRead(event);
      if (remainingEvents_ > 0) --remainingEvents_;
      ++readCount_;
      setTimestamp(result->time());
      issueReports(result->id());
    }
    return result;
  }

  std::auto_ptr<EventPrincipal>
  InputSource::readEvent(EventID const& eventID) {

    std::auto_ptr<EventPrincipal> result(0);

    if (!limitReached()) {
      preRead();
      result = readIt(eventID);
      if (result.get() != 0) {
        Event event(*result, moduleDescription());
        postRead(event);
        if (remainingEvents_ > 0) --remainingEvents_;
	++readCount_;
	issueReports(result->id());
      }
    }
    return result;
  }

  void
  InputSource::skipEvents(int offset) {
    this->skip(offset);
  }

  void
  InputSource::issueReports(EventID const& eventID) {
    LogInfo("FwkReport") << "Begin processing the " << readCount_
			 << suffix(readCount_) << " record. Run " << eventID.run()
			 << ", Event " << eventID.event();
      // At some point we may want to initiate checkpointing here
  }

  std::auto_ptr<EventPrincipal>
  InputSource::readIt(EventID const&) {
      throw edm::Exception(edm::errors::LogicError)
        << "InputSource::readIt()\n"
        << "Random access is not implemented for this type of Input Source\n"
        << "Contact a Framework Developer\n";
  }

  void
  InputSource::setRun(RunNumber_t) {
      throw edm::Exception(edm::errors::LogicError)
        << "InputSource::setRun()\n"
        << "Run number cannot be modified for this type of Input Source\n"
        << "Contact a Framework Developer\n";
  }

  void
  InputSource::setLumi(LuminosityBlockNumber_t) {
      throw edm::Exception(edm::errors::LogicError)
        << "InputSource::setLumi()\n"
        << "Luminosity Block ID  cannot be modified for this type of Input Source\n"
        << "Contact a Framework Developer\n";
  }

  void
  InputSource::skip(int) {
      throw edm::Exception(edm::errors::LogicError)
        << "InputSource::skip()\n"
        << "Random access is not implemented for this type of Input Source\n"
        << "Contact a Framework Developer\n";
  }

  void
  InputSource::rewind_() {
      throw edm::Exception(edm::errors::LogicError)
        << "InputSource::rewind()\n"
        << "Rewind is not implemented for this type of Input Source\n"
        << "Contact a Framework Developer\n";
  }

  void 
  InputSource::preRead() {

    if (primary()) {
      Service<RandomNumberGenerator> rng;
      if (rng.isAvailable()) {
        rng->snapShot();
      }
    }
  }

  void 
  InputSource::postRead(Event& event) {

    if (primary()) {
      Service<RandomNumberGenerator> rng;
      if (rng.isAvailable()) {
        rng->restoreState(event);
      }
    }
  }

  void
  InputSource::doEndRun(RunPrincipal& rp) {
    rp.setEndTime(time_);
    Run run(rp, moduleDescription());
    endRun(run);
    run.commit_();
  }

  void
  InputSource::doEndLumi(LuminosityBlockPrincipal & lbp) {
    lbp.setEndTime(time_);
    LuminosityBlock lb(lbp, moduleDescription());
    endLuminosityBlock(lb);
    lb.commit_();
  }

  void 
  InputSource::wakeUp_() {}

  void
  InputSource::endLuminosityBlock(LuminosityBlock &) {}

  void
  InputSource::endRun(Run &) {}

  void
  InputSource::beginJob(EventSetup const&) {}

  void
  InputSource::endJob() {}

  RunNumber_t
  InputSource::run() const {
    assert(runPrincipal());
    return runPrincipal()->run();
  }

  LuminosityBlockNumber_t
  InputSource::luminosityBlock() const {
    assert(luminosityBlockPrincipal());
    return luminosityBlockPrincipal()->luminosityBlock();
  }
}
