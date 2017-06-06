
// $Id: TestMergeResults.cc,v 1.1 2008/02/04 20:14:20 wdd Exp $
//
// Reads some simple test objects in the event, run, and lumi
// principals.  Then checks to see if the values in these
// objects match what we expect.  Intended to be used to
// test the values in a file that has merged run and lumi
// products.
//
// Original Author: David Dagenhart, Fermilab, February 2008

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/TestObjects/interface/Thing.h"
#include "DataFormats/TestObjects/interface/ThingWithIsEqual.h"
#include "DataFormats/TestObjects/interface/ThingWithMerge.h"
#include "FWCore/ParameterSet/interface/InputTag.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/Framework/interface/MakerMacros.h"

#include <string>
#include <vector>
#include <iostream>

namespace edm {
  class EventSetup;
}

using namespace edm;

namespace edmtest
{

  class TestMergeResults : public edm::EDAnalyzer
  {
  public:

    explicit TestMergeResults(edm::ParameterSet const&);
    virtual ~TestMergeResults();

    virtual void analyze(edm::Event const& e, edm::EventSetup const& c);
    virtual void beginRun(Run const&, EventSetup const&);
    virtual void endRun(Run const&, EventSetup const&);
    virtual void beginLuminosityBlock(LuminosityBlock const&, EventSetup const&);
    virtual void endLuminosityBlock(LuminosityBlock const&, EventSetup const&);
    virtual void respondToOpenInputFile(FileBlock const& fb);
    virtual void respondToCloseInputFile(FileBlock const& fb);
    virtual void respondToOpenOutputFiles(FileBlock const& fb);
    virtual void respondToCloseOutputFiles(FileBlock const& fb);
    void endJob();

  private:

    void abortWithMessage(char* whichFunction, char* product, int expectedValue, int actualValue);

    std::vector<int> default_;
    std::vector<std::string> defaultvstring_;

    std::vector<int> expectedBeginRunProd_;
    std::vector<int> expectedEndRunProd_;
    std::vector<int> expectedBeginLumiProd_;
    std::vector<int> expectedEndLumiProd_;

    std::vector<int> expectedBeginRunNew_;
    std::vector<int> expectedEndRunNew_;
    std::vector<int> expectedBeginLumiNew_;
    std::vector<int> expectedEndLumiNew_;

    std::vector<std::string> expectedParents_;

    bool verbose_;

    unsigned int index0_;
    unsigned int index1_;
    unsigned int index2_;
    unsigned int index3_;
    unsigned int index4_;
    unsigned int index5_;
    unsigned int index6_;
    unsigned int index7_;

    edm::Handle<edmtest::Thing> h_thing;
    edm::Handle<edmtest::ThingWithMerge> h_thingWithMerge;
    edm::Handle<edmtest::ThingWithIsEqual> h_thingWithIsEqual;
  };

  // -----------------------------------------------------------------

  TestMergeResults::TestMergeResults(edm::ParameterSet const& ps):
    default_(),
    defaultvstring_(),
    expectedBeginRunProd_(ps.getUntrackedParameter<std::vector<int> >("expectedBeginRunProd", default_)),
    expectedEndRunProd_(ps.getUntrackedParameter<std::vector<int> >("expectedEndRunProd", default_)),
    expectedBeginLumiProd_(ps.getUntrackedParameter<std::vector<int> >("expectedBeginLumiProd", default_)),
    expectedEndLumiProd_(ps.getUntrackedParameter<std::vector<int> >("expectedEndLumiProd", default_)),

    expectedBeginRunNew_(ps.getUntrackedParameter<std::vector<int> >("expectedBeginRunNew", default_)),
    expectedEndRunNew_(ps.getUntrackedParameter<std::vector<int> >("expectedEndRunNew", default_)),
    expectedBeginLumiNew_(ps.getUntrackedParameter<std::vector<int> >("expectedBeginLumiNew", default_)),
    expectedEndLumiNew_(ps.getUntrackedParameter<std::vector<int> >("expectedEndLumiNew", default_)),

    expectedParents_(ps.getUntrackedParameter<std::vector<std::string> >("expectedParents", defaultvstring_)),

    verbose_(ps.getUntrackedParameter<bool>("verbose", false)),

    index0_(0),
    index1_(0),
    index2_(0),
    index3_(0),
    index4_(0),
    index5_(0),
    index6_(0),
    index7_(0)
  {
  }

  // -----------------------------------------------------------------

  TestMergeResults::~TestMergeResults()
  {
  }

  // -----------------------------------------------------------------

  void TestMergeResults::analyze(edm::Event const& e,edm::EventSetup const&)
  {
    if (verbose_) edm::LogInfo("TestMergeResults") << "analyze";
  }

  void TestMergeResults::beginRun(Run const& run, EventSetup const&) {

    if (verbose_) edm::LogInfo("TestMergeResults") << "beginRun";

    edm::InputTag tag("thingWithMergeProducer", "beginRun", "PROD");

    if (expectedParents_.size() != 0 && run.run() == 1) {

      run.getByLabel(tag, h_thing);
      std::vector<edm::ProductID> parents = h_thing.provenance()->event().parents_;
      assert(parents.size() == expectedParents_.size());
      for (unsigned int i = 0; i < parents.size(); ++i) {
        run.get(parents[i], h_thing);
        assert(h_thing.provenance()->moduleLabel() == expectedParents_[i]);
        assert(h_thing->a == 10001);
      }

      run.getByLabel(tag, h_thingWithMerge);
      parents = h_thingWithMerge.provenance()->event().parents_;
      assert(parents.size() == expectedParents_.size());
      for (unsigned int i = 0; i < parents.size(); ++i) {
        run.get(parents[i], h_thing);
        assert(h_thing.provenance()->moduleLabel() == expectedParents_[i]);
        assert(h_thing->a == 10001);
      }

      run.getByLabel(tag, h_thingWithIsEqual);
      parents = h_thingWithIsEqual.provenance()->event().parents_;
      assert(parents.size() == expectedParents_.size());
      for (unsigned int i = 0; i < parents.size(); ++i) {
        run.get(parents[i], h_thing);
        assert(h_thing.provenance()->moduleLabel() == expectedParents_[i]);
        assert(h_thing->a == 10001);
      }
    }


    if ((index0_ + 2) < expectedBeginRunProd_.size()) {

      run.getByLabel(tag, h_thing);
      int expected = expectedBeginRunProd_[index0_++];
      if (h_thing->a != expected) 
        abortWithMessage("beginRun", "Thing", expected, h_thing->a);

      run.getByLabel(tag, h_thingWithMerge);
      expected = expectedBeginRunProd_[index0_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("beginRun", "thingWithMerge", expected, h_thingWithMerge->a);

      run.getByLabel(tag, h_thingWithIsEqual);
      expected = expectedBeginRunProd_[index0_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("beginRun", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }

    // Repeat without requiring a process (get most recent)
    edm::InputTag tagnew("thingWithMergeProducer", "beginRun");
    if ((index4_ + 2) < expectedBeginRunNew_.size()) {

      run.getByLabel(tagnew, h_thing);
      int expected = expectedBeginRunNew_[index4_++];
      if (h_thing->a != expected) 
        abortWithMessage("beginRun", "Thing", expected, h_thing->a);

      run.getByLabel(tagnew, h_thingWithMerge);
      expected = expectedBeginRunNew_[index4_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("beginRun", "thingWithMerge", expected, h_thingWithMerge->a);

      run.getByLabel(tagnew, h_thingWithIsEqual);
      expected = expectedBeginRunNew_[index4_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("beginRun", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }
  }

  void TestMergeResults::endRun(Run const& run, EventSetup const&) {

    if (verbose_) edm::LogInfo("TestMergeResults") << "endRun";

    edm::InputTag tag("thingWithMergeProducer", "endRun", "PROD");
    if ((index1_ + 2) < expectedEndRunProd_.size()) {

      run.getByLabel(tag, h_thing);
      int expected = expectedEndRunProd_[index1_++];
      if (h_thing->a != expected) 
        abortWithMessage("endRun", "Thing", expected, h_thing->a);

      run.getByLabel(tag, h_thingWithMerge);
      expected = expectedEndRunProd_[index1_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("endRun", "thingWithMerge", expected, h_thingWithMerge->a);

      run.getByLabel(tag, h_thingWithIsEqual);
      expected = expectedEndRunProd_[index1_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("endRun", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }

    // Repeat without requiring a process (get most recent)
    edm::InputTag tagnew("thingWithMergeProducer", "endRun");
    if ((index5_ + 2) < expectedEndRunNew_.size()) {

      run.getByLabel(tagnew, h_thing);
      int expected = expectedEndRunNew_[index5_++];
      if (h_thing->a != expected) 
        abortWithMessage("endRun", "Thing", expected, h_thing->a);

      run.getByLabel(tagnew, h_thingWithMerge);
      expected = expectedEndRunNew_[index5_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("endRun", "thingWithMerge", expected, h_thingWithMerge->a);

      run.getByLabel(tagnew, h_thingWithIsEqual);
      expected = expectedEndRunNew_[index5_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("endRun", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }
  }

  void TestMergeResults::beginLuminosityBlock(LuminosityBlock const& lumi, EventSetup const&) {

    if (verbose_) edm::LogInfo("TestMergeResults") << "beginLuminosityBlock";

    edm::InputTag tag("thingWithMergeProducer", "beginLumi", "PROD");
    if ((index2_ + 2) < expectedBeginLumiProd_.size()) {

      lumi.getByLabel(tag, h_thing);
      int expected = expectedBeginLumiProd_[index2_++];
      if (h_thing->a != expected) 
        abortWithMessage("beginLumi", "Thing", expected, h_thing->a);

      lumi.getByLabel(tag, h_thingWithMerge);
      expected = expectedBeginLumiProd_[index2_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("beginLumi", "thingWithMerge", expected, h_thingWithMerge->a);

      lumi.getByLabel(tag, h_thingWithIsEqual);
      expected = expectedBeginLumiProd_[index2_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("beginLumi", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }

    // Repeat without requiring a process (get most recent)
    edm::InputTag tagnew("thingWithMergeProducer", "beginLumi");
    if ((index6_ + 2) < expectedBeginLumiNew_.size()) {

      lumi.getByLabel(tagnew, h_thing);
      int expected = expectedBeginLumiNew_[index6_++];
      if (h_thing->a != expected) 
        abortWithMessage("beginLumi", "Thing", expected, h_thing->a);

      lumi.getByLabel(tagnew, h_thingWithMerge);
      expected = expectedBeginLumiNew_[index6_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("beginLumi", "thingWithMerge", expected, h_thingWithMerge->a);

      lumi.getByLabel(tagnew, h_thingWithIsEqual);
      expected = expectedBeginLumiNew_[index6_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("beginLumi", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }
  }

  void TestMergeResults::endLuminosityBlock(LuminosityBlock const& lumi, EventSetup const&) {

    if (verbose_) edm::LogInfo("TestMergeResults") << "endLuminosityBlock";

    edm::InputTag tag("thingWithMergeProducer", "endLumi", "PROD");
    if ((index3_ + 2) < expectedEndLumiProd_.size()) {

      lumi.getByLabel(tag, h_thing);
      int expected = expectedEndLumiProd_[index3_++];
      if (h_thing->a != expected) 
        abortWithMessage("endLumi", "Thing", expected, h_thing->a);

      lumi.getByLabel(tag, h_thingWithMerge);
      expected = expectedEndLumiProd_[index3_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("endLumi", "thingWithMerge", expected, h_thingWithMerge->a);

      lumi.getByLabel(tag, h_thingWithIsEqual);
      expected = expectedEndLumiProd_[index3_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("endLumi", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }

    // Repeat without requiring a process (get most recent)
    edm::InputTag tagnew("thingWithMergeProducer", "endLumi");
    if ((index7_ + 2) < expectedEndLumiNew_.size()) {

      lumi.getByLabel(tagnew, h_thing);
      int expected = expectedEndLumiNew_[index7_++];
      if (h_thing->a != expected) 
        abortWithMessage("endLumi", "Thing", expected, h_thing->a);

      lumi.getByLabel(tagnew, h_thingWithMerge);
      expected = expectedEndLumiNew_[index7_++];
      if (h_thingWithMerge->a != expected) 
        abortWithMessage("endLumi", "thingWithMerge", expected, h_thingWithMerge->a);

      lumi.getByLabel(tagnew, h_thingWithIsEqual);
      expected = expectedEndLumiNew_[index7_++];
      if (h_thingWithIsEqual->a != expected) 
        abortWithMessage("endLumi", "thingWithIsEqual", expected, h_thingWithIsEqual->a);
    }
  }

  void TestMergeResults::respondToOpenInputFile(FileBlock const& fb) {
    if (verbose_) edm::LogInfo("TestMergeResults") << "respondToOpenInputFile";
  }

  void TestMergeResults::respondToCloseInputFile(FileBlock const& fb) {
    if (verbose_) edm::LogInfo("TestMergeResults") << "respondToCloseInputFile";
  }

  void TestMergeResults::respondToOpenOutputFiles(FileBlock const& fb) {
    if (verbose_) edm::LogInfo("TestMergeResults") << "respondToOpenOutputFiles";
  }

  void TestMergeResults::respondToCloseOutputFiles(FileBlock const& fb) {
    if (verbose_) edm::LogInfo("TestMergeResults") << "respondToCloseOutputFiles";
  }

  void TestMergeResults::endJob() {
    if (verbose_) edm::LogInfo("TestMergeResults") << "endJob";
  }

  void TestMergeResults::abortWithMessage(char* whichFunction, char* product, int expectedValue, int actualValue) {
    std::cerr << "Error while testing merging of run/lumi products in TestMergeResults.cc\n"
              << "In " << whichFunction << " merging product " << product << "\n"
              << "Expected value = " << expectedValue << " actual value = " << actualValue << std::endl; 
    abort();
  }
}

using edmtest::TestMergeResults;

DEFINE_FWK_MODULE(TestMergeResults);
