/*----------------------------------------------------------------------

Test of the EventPrincipal class.

$Id: generichandle_t.cppunit.cc,v 1.24 2007/10/05 22:00:38 chrjones Exp $

----------------------------------------------------------------------*/  
#include <string>
#include <iostream>

#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/GetPassID.h"
#include "FWCore/Utilities/interface/GetReleaseVersion.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/Timestamp.h"
#include "DataFormats/Common/interface/Wrapper.h"
#include "FWCore/Utilities/interface/TypeID.h"
#include "DataFormats/TestObjects/interface/ToyProducts.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"

#include "FWCore/Framework/interface/GenericHandle.h"
#include <cppunit/extensions/HelperMacros.h>

class testGenericHandle: public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(testGenericHandle);
CPPUNIT_TEST(failgetbyLabelTest);
CPPUNIT_TEST(getbyLabelTest);
CPPUNIT_TEST(failWrongType);
CPPUNIT_TEST_SUITE_END();
public:
  void setUp(){}
  void tearDown(){}
  void failgetbyLabelTest();
  void failWrongType();
  void getbyLabelTest();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testGenericHandle);

void testGenericHandle::failWrongType() {
   try {
      //intentionally misspelled type
      edm::GenericHandle h("edmtest::DmmyProduct");
      CPPUNIT_ASSERT("Failed to thow"==0);
   }
   catch (cms::Exception& x) {
      // nothing to do
   }
   catch (...) {
      CPPUNIT_ASSERT("Threw wrong kind of exception" == 0);
   }
}
void testGenericHandle::failgetbyLabelTest() {

  edm::EventID id;
  edm::Timestamp time;
  edm::ProcessConfiguration pc("PROD", edm::ParameterSetID(), edm::getReleaseVersion(), edm::getPassID());
  boost::shared_ptr<edm::ProductRegistry const> preg(new edm::ProductRegistry);
  boost::shared_ptr<edm::RunPrincipal> rp(new edm::RunPrincipal(id.run(), time, time, preg, pc));
  boost::shared_ptr<edm::LuminosityBlockPrincipal>lbp(new edm::LuminosityBlockPrincipal(1, time, time, preg, rp, pc));
  edm::EventPrincipal ep(id, time, preg, lbp, pc, true);
  edm::GenericHandle h("edmtest::DummyProduct");
  bool didThrow=true;
  try {
     edm::ModuleDescription modDesc;
     modDesc.moduleName_="Blah";
     modDesc.moduleLabel_="blahs"; 
     edm::Event event(ep, modDesc);
     
     std::string label("this does not exist");
     event.getByLabel(label,h);
     *h;
     didThrow=false;
  }
  catch (cms::Exception& x) {
    // nothing to do
  }
  catch (std::exception& x) {
    std::cout <<"caught std exception "<<x.what()<<std::endl;
    CPPUNIT_ASSERT("Threw std::exception!"==0);
  }
  catch (...) {
    CPPUNIT_ASSERT("Threw wrong kind of exception" == 0);
  }
  if( !didThrow) {
    CPPUNIT_ASSERT("Failed to throw required exception" == 0);      
  }
  
}

void testGenericHandle::getbyLabelTest() {
  std::string processName = "PROD";

  typedef edmtest::DummyProduct DP;
  typedef edm::Wrapper<DP> WDP;
  std::auto_ptr<DP> pr(new DP);
  std::auto_ptr<edm::EDProduct> pprod(new WDP(pr));
  std::string label("fred");
  std::string productInstanceName("Rick");

  edmtest::DummyProduct dp;
  edm::TypeID dummytype(dp);
  std::string className = dummytype.friendlyClassName();

  edm::BranchDescription product;

  product.fullClassName_ = dummytype.userClassName();
  product.friendlyClassName_ = className;

  edm::ModuleDescription modDesc;
  modDesc.moduleName_ = "Blah";

  product.moduleLabel_ = label;
  product.productInstanceName_ = productInstanceName;
  product.processName_ = processName;
  product.moduleDescriptionID_ = modDesc.id();
  product.init();

  edm::ProductRegistry *preg = new edm::ProductRegistry;
  preg->addProduct(product);
  preg->setProductIDs();

  edm::ProductRegistry::ProductList const& pl = preg->productList();
  edm::BranchKey const bk(product);
  edm::ProductRegistry::ProductList::const_iterator it = pl.find(bk);
  product.productID_ = it->second.productID_;

  edm::EventID col(1L, 1L);
  edm::Timestamp fakeTime;
  edm::ProcessConfiguration pc("PROD", edm::ParameterSetID(), edm::getReleaseVersion(), edm::getPassID());
  boost::shared_ptr<edm::ProductRegistry const> pregc(preg);
  boost::shared_ptr<edm::RunPrincipal> rp(new edm::RunPrincipal(col.run(), fakeTime, fakeTime, pregc, pc));
  boost::shared_ptr<edm::LuminosityBlockPrincipal>lbp(new edm::LuminosityBlockPrincipal(1, fakeTime, fakeTime, pregc, rp, pc));
  edm::EventPrincipal ep(col, fakeTime, pregc, lbp, pc, true);

  std::auto_ptr<edm::Provenance> pprov(new edm::Provenance(product, edm::BranchEntryDescription::Success));
  ep.put(pprod, pprov);
  
  edm::GenericHandle h("edmtest::DummyProduct");
  try {
    edm::ModuleDescription modDesc;
    modDesc.moduleName_="Blah";
    modDesc.moduleLabel_="blahs"; 
    edm::Event event(ep, modDesc);

    event.getByLabel(label, productInstanceName,h);
  }
  catch (cms::Exception& x) {
    std::cerr << x.explainSelf()<< std::endl;
    CPPUNIT_ASSERT("Threw cms::Exception unexpectedly" == 0);
  }
  catch(std::exception& x){
     std::cerr <<x.what()<<std::endl;
     CPPUNIT_ASSERT("threw std::exception"==0);
  }
  catch (...) {
    std::cerr << "Unknown exception type\n";
    CPPUNIT_ASSERT("Threw exception unexpectedly" == 0);
  }
  CPPUNIT_ASSERT(h.isValid());
  CPPUNIT_ASSERT(h.provenance()->moduleLabel() == label);
}

