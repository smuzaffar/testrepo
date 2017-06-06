
// $Id: ThingWithMergeProducer.cc,v 1.1 2008/02/04 20:15:40 wdd Exp $
//
// Puts some simple test objects in the event, run, and lumi
// principals.  The values put into these objects are just
// arbitrary and meaningless.  Later we check that what get
// out when reading the file after merging is what is expected.
//
// Original Author: David Dagenhart, Fermilab, February 2008

#include "FWCore/Integration/test/ThingWithMergeProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "DataFormats/TestObjects/interface/Thing.h"
#include "DataFormats/TestObjects/interface/ThingWithMerge.h"
#include "DataFormats/TestObjects/interface/ThingWithIsEqual.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/ParameterSet/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

namespace edmtest {
  ThingWithMergeProducer::ThingWithMergeProducer(edm::ParameterSet const& pset) :
    changeIsEqualValue_(pset.getUntrackedParameter<bool>("changeIsEqualValue", false)),
    labelToGet_(pset.getUntrackedParameter<std::string>("labelToGet", std::string()))
{
    produces<Thing>("event");
    produces<Thing, edm::InLumi>("beginLumi");
    produces<Thing, edm::InLumi>("endLumi");
    produces<Thing, edm::InRun>("beginRun");
    produces<Thing, edm::InRun>("endRun");

    produces<ThingWithMerge>("event");
    produces<ThingWithMerge, edm::InLumi>("beginLumi");
    produces<ThingWithMerge, edm::InLumi>("endLumi");
    produces<ThingWithMerge, edm::InRun>("beginRun");
    produces<ThingWithMerge, edm::InRun>("endRun");

    produces<ThingWithIsEqual>("event");
    produces<ThingWithIsEqual, edm::InLumi>("beginLumi");
    produces<ThingWithIsEqual, edm::InLumi>("endLumi");
    produces<ThingWithIsEqual, edm::InRun>("beginRun");
    produces<ThingWithIsEqual, edm::InRun>("endRun");
  }

  ThingWithMergeProducer::~ThingWithMergeProducer() {}  

  void ThingWithMergeProducer::produce(edm::Event& e, edm::EventSetup const&) {

    // The purpose of this first getByLabel call is to cause the products
    // that are "put" below to have a parent so we can test the parentage
    // provenance.
    if (labelToGet_ != "") {
      edm::Handle<Thing> h;
      edm::InputTag tag(labelToGet_, "event", "PROD");
      e.getByLabel(tag, h);
    }

    std::auto_ptr<Thing> result(new Thing);
    result->a = 11;
    e.put(result, std::string("event"));

    std::auto_ptr<ThingWithMerge> result2(new ThingWithMerge);
    result2->a = 12;
    e.put(result2, std::string("event"));

    std::auto_ptr<ThingWithIsEqual> result3(new ThingWithIsEqual);
    result3->a = 13;
    if (changeIsEqualValue_) result3->a = 14;
    e.put(result3, std::string("event"));
  }

  void ThingWithMergeProducer::beginLuminosityBlock(edm::LuminosityBlock& lb, edm::EventSetup const&) {

    if (labelToGet_ != "") {
      edm::Handle<Thing> h;
      edm::InputTag tag(labelToGet_, "beginLumi", "PROD");
      lb.getByLabel(tag, h);
    }

    std::auto_ptr<Thing> result(new Thing);
    result->a = 101;
    lb.put(result, "beginLumi");

    std::auto_ptr<ThingWithMerge> result2(new ThingWithMerge);
    result2->a = 102;
    lb.put(result2, "beginLumi");

    std::auto_ptr<ThingWithIsEqual> result3(new ThingWithIsEqual);
    result3->a = 103;
    if (changeIsEqualValue_) result3->a = 104;
    lb.put(result3, "beginLumi");
  }

  void ThingWithMergeProducer::endLuminosityBlock(edm::LuminosityBlock& lb, edm::EventSetup const&) {

    if (labelToGet_ != "") {
      edm::Handle<Thing> h;
      edm::InputTag tag(labelToGet_, "endLumi", "PROD");
      lb.getByLabel(tag, h);
    }

    std::auto_ptr<Thing> result(new Thing);
    result->a = 1001;
    lb.put(result, "endLumi");

    std::auto_ptr<ThingWithMerge> result2(new ThingWithMerge);
    result2->a = 1002;
    lb.put(result2, "endLumi");

    std::auto_ptr<ThingWithIsEqual> result3(new ThingWithIsEqual);
    result3->a = 1003;
    if (changeIsEqualValue_) result3->a = 1004;
    lb.put(result3, "endLumi");
  }

  // Functions that gets called by framework every run
  void ThingWithMergeProducer::beginRun(edm::Run& r, edm::EventSetup const&) {

    if (labelToGet_ != "") {
      edm::Handle<Thing> h;
      edm::InputTag tag(labelToGet_, "beginRun", "PROD");
      r.getByLabel(tag, h);
    }

    std::auto_ptr<Thing> result(new Thing);
    result->a = 10001;
    r.put(result, "beginRun");

    std::auto_ptr<ThingWithMerge> result2(new ThingWithMerge);
    result2->a = 10002;
    r.put(result2, "beginRun");

    std::auto_ptr<ThingWithIsEqual> result3(new ThingWithIsEqual);
    result3->a = 10003;
    if (changeIsEqualValue_) result3->a = 10004;
    r.put(result3, "beginRun");
  }

  void ThingWithMergeProducer::endRun(edm::Run& r, edm::EventSetup const&) {

    if (labelToGet_ != "") {
      edm::Handle<Thing> h;
      edm::InputTag tag(labelToGet_, "endRun", "PROD");
      r.getByLabel(tag, h);
    }

    std::auto_ptr<Thing> result(new Thing);
    result->a = 100001;
    r.put(result, "endRun");

    std::auto_ptr<ThingWithMerge> result2(new ThingWithMerge);
    result2->a = 100002;
    r.put(result2, "endRun");

    std::auto_ptr<ThingWithIsEqual> result3(new ThingWithIsEqual);
    result3->a = 100003;
    if (changeIsEqualValue_) result3->a = 100004;
    r.put(result3, "endRun");
  }
}
using edmtest::ThingWithMergeProducer;
DEFINE_FWK_MODULE(ThingWithMergeProducer);
