/**
   \file
   test for ProductRegistry 

   \author Stefano ARGIRO
   \version $Id: edproducer_productregistry_callback.cc,v 1.1 2005/10/11 20:04:58 chrjones Exp $
   \date 21 July 2005
*/


#include <iostream>
#include <cppunit/extensions/HelperMacros.h>
#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/Framework/src/SignallingProductRegistry.h"
#include "FWCore/Framework/interface/ConstProductRegistry.h"
#include "FWCore/Framework/interface/ModuleDescription.h"

#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Handle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Actions.h"
#include "FWCore/Framework/interface/ProductRegistry.h"
#include "FWCore/Framework/src/WorkerMaker.h"
#include "FWCore/Framework/src/Factory.h"
#include "FWCore/Framework/src/WorkerParams.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/parse.h"
#include "FWCore/ParameterSet/interface/Makers.h"

#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#include "FWCore/Framework/src/TypeID.h"


class testEDProducerProductRegistryCallback: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(testEDProducerProductRegistryCallback);

   CPPUNIT_TEST_EXCEPTION(testCircularRef,cms::Exception);
   CPPUNIT_TEST_EXCEPTION(testCircularRef2,cms::Exception);
   CPPUNIT_TEST(testTwoListeners);

CPPUNIT_TEST_SUITE_END();

public:
  void setUp(){}
  void tearDown(){}
  void testCircularRef();
  void testCircularRef2();
  void testTwoListeners();

};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testEDProducerProductRegistryCallback);

using namespace edm;

namespace {
   class TestMod : public EDProducer
   {
     public:
      explicit TestMod(ParameterSet const& p);
      
      void produce(Event& e, EventSetup const&);
      
      void listen(BranchDescription const&);
   };
   
   TestMod::TestMod(ParameterSet const&)
   { produces<int>();}
   
   void TestMod::produce(Event&, EventSetup const&)
   {
   }

   class ListenMod : public EDProducer
   {
     public:
      explicit ListenMod(ParameterSet const&);
      void produce(Event& e, EventSetup const&);
      void listen(BranchDescription const&);
   };
   
   ListenMod::ListenMod(ParameterSet const&)
   {
      callWhenNewProductsRegistered(this,&ListenMod::listen);
   }
   void ListenMod::produce(Event&, EventSetup const&)
   {
   }
   
   void ListenMod::listen(BranchDescription const& iDesc)
   {
      edm::TypeID intType(typeid(int));
      //std::cout <<"see class "<<iDesc.friendlyClassName_<<std::endl;
      if(iDesc.friendlyClassName_ == intType.friendlyClassName()) {
         produces<int>(iDesc.module.moduleLabel_+"-"+iDesc.productInstanceName_);
         //std::cout <<iDesc.module.moduleLabel_<<"-"<<iDesc.productInstanceName_<<std::endl;
      }
   }

   class ListenFloatMod : public EDProducer
   {
public:
      explicit ListenFloatMod(ParameterSet const&);
      void produce(Event& e, EventSetup const&);
      void listen(BranchDescription const&);
   };
   
   ListenFloatMod::ListenFloatMod(ParameterSet const&)
   {
      callWhenNewProductsRegistered(this,&ListenFloatMod::listen);
   }
   void ListenFloatMod::produce(Event&, EventSetup const&)
   {
   }
   
   void ListenFloatMod::listen(BranchDescription const& iDesc)
   {
      edm::TypeID intType(typeid(int));
      //std::cout <<"see class "<<iDesc.friendlyClassName_<<std::endl;
      if(iDesc.friendlyClassName_ == intType.friendlyClassName()) {
         produces<float>(iDesc.module.moduleLabel_+"-"+iDesc.productInstanceName_);
         //std::cout <<iDesc.module.moduleLabel_<<"-"<<iDesc.productInstanceName_<<std::endl;
      }
   }
   
}

void  testEDProducerProductRegistryCallback::testCircularRef(){
   using namespace edm;
   using namespace std;
   
   SignallingProductRegistry preg;

   //Need access to the ConstProductRegistry service
   auto_ptr<ConstProductRegistry> cReg(new ConstProductRegistry(preg));
   ServiceToken token = ServiceRegistry::createContaining(cReg);
   ServiceRegistry::Operate startServices(token);
   
   auto_ptr<Maker> f(new WorkerMaker<TestMod>);
   
   ParameterSet p1;
   p1.addParameter("@module_type",std::string("TestMod") );
   p1.addParameter("@module_label",std::string("t1") );
   
   ParameterSet p2;
   p2.addParameter("@module_type",std::string("TestMod") );
   p2.addParameter("@module_label",std::string("t2") );
   
   edm::ActionTable table;
   
   edm::WorkerParams params1(p1, preg, table, "PROD", 0, 0);
   edm::WorkerParams params2(p2, preg, table, "PROD", 0, 0);

   
   auto_ptr<Maker> lM(new WorkerMaker<ListenMod>);
   ParameterSet l1;
   l1.addParameter("@module_type",std::string("ListenMod") );
   l1.addParameter("@module_label",std::string("l1") );
   
   ParameterSet l2;
   l2.addParameter("@module_type",std::string("ListenMod") );
   l2.addParameter("@module_label",std::string("l2") );

   edm::WorkerParams paramsl1(l1, preg, table, "PROD", 0, 0);
   edm::WorkerParams paramsl2(l2, preg, table, "PROD", 0, 0);

   
   auto_ptr<Worker> w1 = f->makeWorker(params1);
   auto_ptr<Worker> wl1 = lM->makeWorker(paramsl1);
   auto_ptr<Worker> wl2 = lM->makeWorker(paramsl2);
   auto_ptr<Worker> w2 = f->makeWorker(params2);

   //Should be 5 products
   // 1 from the module 't1'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1'
   //    1 from 'l2' in response to 't1'
   //       1 from 'l1' in response to 'l2'
   // 1 from the module 't2'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1'
   //    1 from 'l2' in response to 't2'
   //       1 from 'l1' in response to 'l2'
   //std::cout <<"# products "<<preg.productList().size()<<std::endl;
   CPPUNIT_ASSERT(10 == preg.productList().size());
}

void  testEDProducerProductRegistryCallback::testCircularRef2(){
   using namespace edm;
   using namespace std;
   
   SignallingProductRegistry preg;
   
   //Need access to the ConstProductRegistry service
   auto_ptr<ConstProductRegistry> cReg(new ConstProductRegistry(preg));
   ServiceToken token = ServiceRegistry::createContaining(cReg);
   ServiceRegistry::Operate startServices(token);
   
   auto_ptr<Maker> f(new WorkerMaker<TestMod>);
   
   ParameterSet p1;
   p1.addParameter("@module_type",std::string("TestMod") );
   p1.addParameter("@module_label",std::string("t1") );
   
   ParameterSet p2;
   p2.addParameter("@module_type",std::string("TestMod") );
   p2.addParameter("@module_label",std::string("t2") );
   
   edm::ActionTable table;
   
   edm::WorkerParams params1(p1, preg, table, "PROD", 0, 0);
   edm::WorkerParams params2(p2, preg, table, "PROD", 0, 0);
   
   
   auto_ptr<Maker> lM(new WorkerMaker<ListenMod>);
   ParameterSet l1;
   l1.addParameter("@module_type",std::string("ListenMod") );
   l1.addParameter("@module_label",std::string("l1") );
   
   ParameterSet l2;
   l2.addParameter("@module_type",std::string("ListenMod") );
   l2.addParameter("@module_label",std::string("l2") );
   
   edm::WorkerParams paramsl1(l1, preg, table, "PROD", 0, 0);
   edm::WorkerParams paramsl2(l2, preg, table, "PROD", 0, 0);
   
   
   auto_ptr<Worker> wl1 = lM->makeWorker(paramsl1);
   auto_ptr<Worker> wl2 = lM->makeWorker(paramsl2);
   auto_ptr<Worker> w1 = f->makeWorker(params1);
   auto_ptr<Worker> w2 = f->makeWorker(params2);
   
   //Would be 10 products
   // 1 from the module 't1'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1' <-- circular 
   //    1 from 'l2' in response to 't1'                  | 
   //       1 from 'l1' in response to 'l2' <-- circular / 
   // 1 from the module 't2'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1'
   //    1 from 'l2' in response to 't2'
   //       1 from 'l1' in response to 'l2'
   //std::cout <<"# products "<<preg.productList().size()<<std::endl;
   CPPUNIT_ASSERT(10 == preg.productList().size());
}

void  testEDProducerProductRegistryCallback::testTwoListeners(){
   using namespace edm;
   using namespace std;
   
   SignallingProductRegistry preg;
   
   //Need access to the ConstProductRegistry service
   auto_ptr<ConstProductRegistry> cReg(new ConstProductRegistry(preg));
   ServiceToken token = ServiceRegistry::createContaining(cReg);
   ServiceRegistry::Operate startServices(token);
   
   auto_ptr<Maker> f(new WorkerMaker<TestMod>);
   
   ParameterSet p1;
   p1.addParameter("@module_type",std::string("TestMod") );
   p1.addParameter("@module_label",std::string("t1") );
   
   ParameterSet p2;
   p2.addParameter("@module_type",std::string("TestMod") );
   p2.addParameter("@module_label",std::string("t2") );
   
   edm::ActionTable table;
   
   edm::WorkerParams params1(p1, preg, table, "PROD", 0, 0);
   edm::WorkerParams params2(p2, preg, table, "PROD", 0, 0);
   
   
   auto_ptr<Maker> lM(new WorkerMaker<ListenMod>);
   ParameterSet l1;
   l1.addParameter("@module_type",std::string("ListenMod") );
   l1.addParameter("@module_label",std::string("l1") );
   
   auto_ptr<Maker> lFM(new WorkerMaker<ListenFloatMod>);
   ParameterSet l2;
   l2.addParameter("@module_type",std::string("ListenMod") );
   l2.addParameter("@module_label",std::string("l2") );
   
   edm::WorkerParams paramsl1(l1, preg, table, "PROD", 0, 0);
   edm::WorkerParams paramsl2(l2, preg, table, "PROD", 0, 0);
   
   
   auto_ptr<Worker> w1 = f->makeWorker(params1);
   auto_ptr<Worker> wl1 = lM->makeWorker(paramsl1);
   auto_ptr<Worker> wl2 = lFM->makeWorker(paramsl2);
   auto_ptr<Worker> w2 = f->makeWorker(params2);
   
   //Should be 8 products
   // 1 from the module 't1'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1'
   //    1 from 'l2' in response to 't1'
   // 1 from the module 't2'
   //    1 from 'l1' in response
   //       1 from 'l2' in response to 'l1'
   //    1 from 'l2' in response to 't2'
   //std::cout <<"# products "<<preg.productList().size()<<std::endl;
   CPPUNIT_ASSERT(8 == preg.productList().size());
}
