/**
   \file
   test for ProductRegistry 

   \author Stefano ARGIRO
   \version $Id: productregistry.cppunit.cc,v 1.17 2006/12/05 23:56:18 paterno Exp $
   \date 21 July 2005
*/


#include <iostream>
#include <cppunit/extensions/HelperMacros.h>
#include "FWCore/Framework/interface/EventProcessor.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/Framework/src/SignallingProductRegistry.h"
#include "FWCore/Framework/interface/ConstProductRegistry.h"
#include "DataFormats/Common/interface/BranchDescription.h"
#include "DataFormats/Common/interface/ModuleDescription.h"
#include "FWCore/PluginManager/interface/ProblemTracker.h"

// namespace edm {
//   class EDProduct;
// }

class testProductRegistry: public CppUnit::TestFixture
{
CPPUNIT_TEST_SUITE(testProductRegistry);

CPPUNIT_TEST(testSignal);
CPPUNIT_TEST(testWatch);
CPPUNIT_TEST_EXCEPTION(testCircular,cms::Exception);

CPPUNIT_TEST(testProductRegistration);

CPPUNIT_TEST_SUITE_END();

public:
  testProductRegistry();
  void setUp();
  void tearDown();
  void testSignal();
  void testWatch();
  void testCircular();
  void testProductRegistration();

 private:
  edm::ModuleDescription* intModule_;
  edm::ModuleDescription* floatModule_;
  edm::BranchDescription* intBranch_;
  edm::BranchDescription* floatBranch_;
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testProductRegistry);

namespace {
   struct Listener {
      int* heard_;
      Listener(int& hear) :heard_(&hear) {}
      void operator()(const edm::BranchDescription&){
         ++(*heard_);
      }
   };

   struct Responder {
      std::string name_;
      edm::ProductRegistry* reg_;
      Responder(const std::string& iName,
                edm::ConstProductRegistry& iConstReg,
                edm::ProductRegistry& iReg):name_(iName),reg_(&iReg)
      {
        iConstReg.watchProductAdditions(this, &Responder::respond);
      }
      void respond(const edm::BranchDescription& iDesc){
         edm::BranchDescription prod(iDesc);
         prod.moduleLabel_ = name_;
         prod.productInstanceName_ = prod.productInstanceName()+"-"+prod.moduleLabel();
         reg_->addProduct(prod);
      }
   };
}

testProductRegistry::testProductRegistry() :
  intModule_(0),
  floatModule_(0),
  intBranch_(0),
  floatBranch_(0)
{ }


void testProductRegistry::setUp()
{
  intModule_ = new edm::ModuleDescription;
  intBranch_ = new edm::BranchDescription(edm::InEvent, "label", "PROD",
					  "int", "int", "int",
					  *intModule_);

  floatModule_ = new edm::ModuleDescription;
  floatBranch_ = new edm::BranchDescription(edm::InEvent, "label", "PROD",
					    "float", "float", "float",
					    *floatModule_);

}

namespace 
{
  template <class T> void kill_and_clear(T*& p) { delete p; p=0; }
}

void testProductRegistry::tearDown()
{
  kill_and_clear(floatBranch_);
  kill_and_clear(intBranch_);
  kill_and_clear(floatModule_);
  kill_and_clear(intModule_);
}

void  testProductRegistry:: testSignal(){
   using namespace edm;
   SignallingProductRegistry reg;
   
   int hear=0;
   Listener listening(hear);
   reg.productAddedSignal_.connect(listening);

   //BranchDescription prod(InEvent, "label", "PROD", "int", "int", "int", md);
   
   //   reg.addProduct(prod);
   reg.addProduct(*intBranch_);
   CPPUNIT_ASSERT(1==hear);
}

void  testProductRegistry:: testWatch(){
   using namespace edm;
   SignallingProductRegistry reg;
   ConstProductRegistry constReg(reg);
   
   int hear=0;
   Listener listening(hear);
   constReg.watchProductAdditions(listening);
   constReg.watchProductAdditions(listening, &Listener::operator());

   Responder one("one",constReg, reg);
                 
   //BranchDescription prod(InEvent, "label", "PROD", "int", "int", "int");
   //reg.addProduct(prod);
   reg.addProduct(*intBranch_);

   //BranchDescription prod2(InEvent, "label", "PROD", "float", "float", "float");
   //   reg.addProduct(prod2);
   reg.addProduct(*floatBranch_);
   
   //Should be 4 products
   // 1 from the 'int' in this routine
   // 1 from 'one' responding to this call
   // 1 from the 'float'
   // 1 from 'one' responding to the original call
   CPPUNIT_ASSERT(4*2==hear);
   CPPUNIT_ASSERT(4 == reg.size());
}
void  testProductRegistry:: testCircular(){
   using namespace edm;
   SignallingProductRegistry reg;
   ConstProductRegistry constReg(reg);
   
   int hear=0;
   Listener listening(hear);
   constReg.watchProductAdditions(listening);
   constReg.watchProductAdditions(listening, &Listener::operator());
   
   Responder one("one",constReg, reg);
   Responder two("two",constReg, reg);
   
   //BranchDescription prod(InEvent, "label","PROD","int","int","int");
   //reg.addProduct(prod);
   reg.addProduct(*intBranch_);

   //Should be 5 products
   // 1 from the original 'add' in this routine
   // 1 from 'one' responding to this call
   // 1 from 'two' responding to 'one'
   // 1 from 'two' responding to the original call
   // 1 from 'one' responding to 'two'
   CPPUNIT_ASSERT(5*2==hear);
   CPPUNIT_ASSERT(5 == reg.size());
}

void  testProductRegistry:: testProductRegistration(){
   edm::AssertHandler ah;

  const std::string config=

    "process TEST = { \n"
      "module m1 = TestPRegisterModule1{ } \n"
      "module m2 = TestPRegisterModule2{ } \n" 
      "path p = {m1,m2}\n"
      "source = DummySource{ untracked int32 maxEvents = 1 }\n"
    "}\n";

  try {
   edm::EventProcessor proc(config);
  } catch(const cms::Exception& iException) {
     std::cout <<"caught "<<iException.explainSelf()<<std::endl;
     throw;
  }
}
