/*----------------------------------------------------------------------
$Id: InputSource.cc,v 1.22 2007/05/01 23:22:32 wmtan Exp $
----------------------------------------------------------------------*/
#include <cassert> 
#include "FWCore/Framework/interface/InputSource.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Utilities/interface/RandomNumberGenerator.h"

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
  }
  InputSource::InputSource(ParameterSet const& pset, InputSourceDescription const& desc) :
      ProductRegistryHelper(),
      maxEvents_(desc.maxEvents_),
      remainingEvents_(maxEvents_),
      readCount_(0),
      unlimited_(maxEvents_ < 0),
      isDesc_(desc),
      primary_(pset.getParameter<std::string>("@module_label") == std::string("@main_input")) {
    // Secondary input sources currently do not have a product registry.
    if (primary_) {
      assert(isDesc_.productRegistry_ != 0);
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
      addToRegistry(typeLabelList().begin(), typeLabelList().end(), moduleDescription(), productRegistry());
    }
  }

  std::auto_ptr<EventPrincipal>
  InputSource::readEvent() {

    std::auto_ptr<EventPrincipal> result(0);

    if (remainingEvents_ != 0) {
      preRead();
      result = read();
      if (result.get() != 0) {
        Event event(*result, moduleDescription());
        postRead(event);
        if (!unlimited_) --remainingEvents_;
	++readCount_;
	issueReports(result->id());
      }
    }
    return result;
  }

  std::auto_ptr<EventPrincipal>
  InputSource::readEvent(EventID const& eventID) {

    std::auto_ptr<EventPrincipal> result(0);

    if (remainingEvents_ != 0) {
      preRead();
      result = readIt(eventID);
      if (result.get() != 0) {
        Event event(*result, moduleDescription());
        postRead(event);
        if (!unlimited_) --remainingEvents_;
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
}
