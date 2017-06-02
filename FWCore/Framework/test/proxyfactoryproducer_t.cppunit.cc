/*
 *  proxyfactoryproducer_t.cc
 *  EDMProto
 *
 *  Created by Chris Jones on 4/8/05.
 *  Changed by Viji Sundararajan on 03-Jul-05.

*/

#include <cppunit/extensions/HelperMacros.h>
#include "FWCore/Framework/interface/ProxyFactoryProducer.h"
#include "FWCore/Framework/interface/ProxyFactoryTemplate.h"
#include "FWCore/Framework/interface/DataProxyTemplate.h"
#include "FWCore/Framework/test/DummyData.h"
#include "FWCore/Framework/test/DummyRecord.h"

using edm::eventsetup::test::DummyData;
using namespace edm::eventsetup;

class DummyProxy : public DataProxyTemplate<DummyRecord, DummyData> {
public:
   DummyProxy() {}
protected:
   const value_type* make(const record_type&, const DataKey&) {
      return static_cast<const value_type*>(0) ;
   }
   void invalidateCache() {
   }   
private:
};

class Test1Producer : public ProxyFactoryProducer {
public:
   Test1Producer() {
      std::auto_ptr<ProxyFactoryTemplate<DummyProxy> > pFactory(new 
                                                                 ProxyFactoryTemplate<DummyProxy>());
      registerFactory(pFactory);
   }
};

class testProxyfactor: public CppUnit::TestFixture
{
CPPUNIT_TEST_SUITE(testProxyfactor);

CPPUNIT_TEST(registerProxyfactorytemplateTest);

CPPUNIT_TEST_SUITE_END();
public:
  void setUp(){}
  void tearDown(){}

  void registerProxyfactorytemplateTest();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testProxyfactor);

void testProxyfactor::registerProxyfactorytemplateTest()
{
   Test1Producer testProd;
   EventSetupRecordKey dummyRecordKey = EventSetupRecordKey::makeKey<DummyRecord>();
   CPPUNIT_ASSERT(testProd.isUsingRecord(dummyRecordKey));

   const DataProxyProvider::KeyedProxies& keyedProxies =
      testProd.keyedProxies(dummyRecordKey);

   CPPUNIT_ASSERT(keyedProxies.size() == 1);
   CPPUNIT_ASSERT(0 != dynamic_cast<DummyProxy*>(&(*(keyedProxies.front().second))));
}
