/*----------------------------------------------------------------------
  
$Id: EDProducer.cc,v 1.14 2007/09/18 18:06:47 chrjones Exp $

----------------------------------------------------------------------*/

#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/src/CPCSentry.h"

#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

namespace edm {
  EDProducer::EDProducer() :
      ProducerBase(),
      moduleDescription_(),
      current_context_(0) {}

  EDProducer::~EDProducer() { }

  void
  EDProducer::doProduce(Event& e, EventSetup const& c,
			     CurrentProcessingContext const* cpc) {
    detail::CPCSentry sentry(current_context_, cpc);
    this->produce(e, c);
  }

  void 
  EDProducer::doBeginJob(EventSetup const& es) {
    this->beginJob(es);
  }
  
  void 
  EDProducer::doEndJob() {
    this->endJob();
  }

  void
  EDProducer::doBeginRun(Run & r, EventSetup const& c,
			CurrentProcessingContext const* cpc) {
    detail::CPCSentry sentry(current_context_, cpc);
    this->beginRun(r, c);
  }

  void
  EDProducer::doEndRun(Run & r, EventSetup const& c,
			CurrentProcessingContext const* cpc) {
    detail::CPCSentry sentry(current_context_, cpc);
    this->endRun(r, c);
  }

  void
  EDProducer::doBeginLuminosityBlock(LuminosityBlock & lb, EventSetup const& c,
			CurrentProcessingContext const* cpc) {
    detail::CPCSentry sentry(current_context_, cpc);
    this->beginLuminosityBlock(lb, c);
  }

  void
  EDProducer::doEndLuminosityBlock(LuminosityBlock & lb, EventSetup const& c,
			CurrentProcessingContext const* cpc) {
    detail::CPCSentry sentry(current_context_, cpc);
    this->endLuminosityBlock(lb, c);
  }

  void
  EDProducer::doRespondToOpenInputFile(FileBlock const& fb) {
    respondToOpenInputFile(fb);
  }

  void
  EDProducer::doRespondToCloseInputFile(FileBlock const& fb) {
    respondToCloseInputFile(fb);
  }

  void 
  EDProducer::doRespondToOpenOutputFiles(FileBlock const& fb) {
    respondToOpenOutputFiles(fb);
  }

  void
  EDProducer::doRespondToCloseOutputFiles(FileBlock const& fb) {
    respondToCloseOutputFiles(fb);
  }

  CurrentProcessingContext const*
  EDProducer::currentContext() const {
    return current_context_;
  }

  void
  EDProducer::fillDescription(ParameterSetDescription& iDesc) {
    iDesc.setUnknown();
  }
}
