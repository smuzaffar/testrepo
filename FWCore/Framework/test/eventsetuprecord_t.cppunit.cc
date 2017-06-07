/*
 *  eventsetuprecord_t.cc
 *  EDMProto
 *
 *  Created by Chris Jones on 3/29/05.
 *  Changed by Viji on 06/07/2005
 */

#include <cppunit/extensions/HelperMacros.h>

#include "FWCore/Framework/interface/EventSetupRecordImplementation.h"
#include "FWCore/Framework/interface/EventSetupRecordProviderTemplate.h"
#include "FWCore/Framework/interface/EventSetupRecordProviderFactoryManager.h"
#include "FWCore/Framework/interface/EventSetupRecordProviderFactoryTemplate.h"

#include "FWCore/Framework/interface/HCMethods.icc"
#include "FWCore/Framework/interface/HCTypeTagTemplate.icc"

#include "FWCore/Framework/interface/DataProxyTemplate.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/recordGetImplementation.icc"

using namespace edm;
using namespace edm::eventsetup;
namespace eventsetuprecord_t {
class DummyRecord : public edm::eventsetup::EventSetupRecordImplementation<DummyRecord> { public:
   const DataProxy* find(const edm::eventsetup::DataKey& iKey) const {
      return edm::eventsetup::EventSetupRecord::find(iKey);
   }
};
}
//HCMethods<T, T, EventSetup, EventSetupRecordKey, EventSetupRecordKey::IdTag >
namespace edm {
  namespace eventsetup {
    namespace heterocontainer {
	template<>
	const char*
	  HCTypeTagTemplate<eventsetuprecord_t::DummyRecord, edm::eventsetup::EventSetupRecordKey>::className() {
	   return "DummyRecord";
	}
    }
  }
}

//create an instance of the factory
static eventsetup::EventSetupRecordProviderFactoryTemplate<eventsetuprecord_t::DummyRecord> s_factory;

namespace eventsetuprecord_t {
class Dummy {};
}
using eventsetuprecord_t::Dummy;
using eventsetuprecord_t::DummyRecord;
typedef edm::eventsetup::MakeDataException ExceptionType;
typedef edm::eventsetup::NoDataException<Dummy> NoDataExceptionType;

class testEventsetupRecord: public CppUnit::TestFixture
{
CPPUNIT_TEST_SUITE(testEventsetupRecord);

CPPUNIT_TEST(factoryTest);
CPPUNIT_TEST(proxyTest);
CPPUNIT_TEST(getTest);
CPPUNIT_TEST(doGetTest);
CPPUNIT_TEST(proxyResetTest);
CPPUNIT_TEST(introspectionTest);

CPPUNIT_TEST_EXCEPTION(getNodataExpTest,NoDataExceptionType);
CPPUNIT_TEST_EXCEPTION(getExepTest,ExceptionType);
CPPUNIT_TEST_EXCEPTION(doGetExepTest,ExceptionType);

CPPUNIT_TEST_SUITE_END();
public:
  void setUp(){}
  void tearDown(){}

  void factoryTest();
  void proxyTest();
  void getTest();
  void doGetTest();
  void proxyResetTest();
  void introspectionTest();
  
  void getNodataExpTest();
  void getExepTest();
  void doGetExepTest();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testEventsetupRecord);

void testEventsetupRecord::factoryTest()
{
   std::auto_ptr<EventSetupRecordProvider> dummyProvider =
   EventSetupRecordProviderFactoryManager::instance().makeRecordProvider(
                              EventSetupRecordKey::makeKey<DummyRecord>());
   
   CPPUNIT_ASSERT(0 != dynamic_cast<EventSetupRecordProviderTemplate<DummyRecord>*>(&(*dummyProvider)));

}   

namespace edm {
  namespace eventsetup {
    namespace heterocontainer {
	template<>
	const char*
	edm::eventsetup::heterocontainer::HCTypeTagTemplate<Dummy, edm::eventsetup::DataKey>::className() {
	   return "Dummy";
	}
    }
  }
}

class FailingDummyProxy : public eventsetup::DataProxyTemplate<DummyRecord, Dummy> {
protected:
   const value_type* make(const record_type&, const DataKey&) {
      return 0 ;
   }
   void invalidateCache() {
   }   
};

class WorkingDummyProxy : public eventsetup::DataProxyTemplate<DummyRecord, Dummy> {
public:
   WorkingDummyProxy(const Dummy* iDummy) : data_(iDummy) {}

  void set(Dummy* iDummy) {
    data_ = iDummy;
  }
protected:
   
   const value_type* make(const record_type&, const DataKey&) {
      return data_ ;
   }
   void invalidateCache() {
   }
  
private:
   const Dummy* data_;
};

class WorkingDummyProvider : public edm::eventsetup::DataProxyProvider {
public:
  WorkingDummyProvider( const edm::eventsetup::DataKey& iKey, boost::shared_ptr<WorkingDummyProxy> iProxy) :
  m_key(iKey),
  m_proxy(iProxy) {
    usingRecord<DummyRecord>();
  }
  
  virtual void newInterval(const EventSetupRecordKey&,
                           const ValidityInterval&) {}

protected:
  virtual void registerProxies(const EventSetupRecordKey&,
                               KeyedProxies& aProxyList) {
    aProxyList.push_back(std::make_pair(m_key, m_proxy));
  }
private:
  edm::eventsetup::DataKey m_key;
  boost::shared_ptr<WorkingDummyProxy> m_proxy;

};

void testEventsetupRecord::proxyTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;
   
   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                              "");
   
   CPPUNIT_ASSERT(0 == dummyRecord.find(dummyDataKey));

   
   dummyRecord.add(dummyDataKey,
                    &dummyProxy);
   
   CPPUNIT_ASSERT(&dummyProxy == dummyRecord.find(dummyDataKey));

   const DataKey dummyFredDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                                  "fred");
   CPPUNIT_ASSERT(0 == dummyRecord.find(dummyFredDataKey));

}

void testEventsetupRecord::getTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;

   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                              "");

   ESHandle<Dummy> dummyPtr;
   //dummyRecord.get(dummyPtr);
   CPPUNIT_ASSERT_THROW(dummyRecord.get(dummyPtr), NoDataExceptionType) ;
   //CDJ do this replace
   //CPPUNIT_ASSERT_THROW(dummyRecord.get(dummyPtr),NoDataExceptionType);

   dummyRecord.add(dummyDataKey,
                    &dummyProxy);

   //dummyRecord.get(dummyPtr);
   CPPUNIT_ASSERT_THROW(dummyRecord.get(dummyPtr), ExceptionType);

   Dummy myDummy;
   WorkingDummyProxy workingProxy(&myDummy);
   
   const DataKey workingDataKey(DataKey::makeTypeTag<WorkingDummyProxy::value_type>(),
                              "working");

   dummyRecord.add(workingDataKey,
                    &workingProxy);

   dummyRecord.get("working",dummyPtr);
   
   CPPUNIT_ASSERT(&(*dummyPtr) == &myDummy);

   const std::string workingString("working");
   
   dummyRecord.get(workingString,dummyPtr);
   CPPUNIT_ASSERT(&(*dummyPtr) == &myDummy);
}

void testEventsetupRecord::getNodataExpTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;

   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),"");

   ESHandle<Dummy> dummyPtr;
   dummyRecord.get(dummyPtr);
   //CPPUNIT_ASSERT_THROW(dummyRecord.get(dummyPtr), NoDataExceptionType) ;

}

void testEventsetupRecord::getExepTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;

   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),"");

   ESHandle<Dummy> dummyPtr;
   
   dummyRecord.add(dummyDataKey,&dummyProxy);

   dummyRecord.get(dummyPtr);
   //CPPUNIT_ASSERT_THROW(dummyRecord.get(dummyPtr), ExceptionType);
}

void testEventsetupRecord::doGetTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;
   
   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                              "");
   
   CPPUNIT_ASSERT(!dummyRecord.doGet(dummyDataKey)) ;
   
   dummyRecord.add(dummyDataKey,
                   &dummyProxy);
   
   //dummyRecord.doGet(dummyDataKey);
   CPPUNIT_ASSERT_THROW(dummyRecord.doGet(dummyDataKey), ExceptionType);
   
   Dummy myDummy;
   WorkingDummyProxy workingProxy(&myDummy);
   
   const DataKey workingDataKey(DataKey::makeTypeTag<WorkingDummyProxy::value_type>(),
                                "working");
   
   dummyRecord.add(workingDataKey,
                   &workingProxy);
   
   CPPUNIT_ASSERT(dummyRecord.doGet(workingDataKey));
   
}

void testEventsetupRecord::introspectionTest()
{
  DummyRecord dummyRecord;
  FailingDummyProxy dummyProxy;
  
  const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                             "");

  std::vector<edm::eventsetup::DataKey> keys;
  dummyRecord.fillRegisteredDataKeys(keys);
  
  CPPUNIT_ASSERT(keys.empty()) ;
  
  dummyRecord.add(dummyDataKey,
                  &dummyProxy);
  
  dummyRecord.fillRegisteredDataKeys(keys);
  CPPUNIT_ASSERT(1 == keys.size());
  
  Dummy myDummy;
  WorkingDummyProxy workingProxy(&myDummy);
  
  const DataKey workingDataKey(DataKey::makeTypeTag<WorkingDummyProxy::value_type>(),
                               "working");
  
  dummyRecord.add(workingDataKey,
                  &workingProxy);
  
  dummyRecord.fillRegisteredDataKeys(keys);
  CPPUNIT_ASSERT(2 == keys.size());
}

void testEventsetupRecord::doGetExepTest()
{
   DummyRecord dummyRecord;
   FailingDummyProxy dummyProxy;
   
   const DataKey dummyDataKey(DataKey::makeTypeTag<FailingDummyProxy::value_type>(),
                              "");
   
   CPPUNIT_ASSERT(!dummyRecord.doGet(dummyDataKey)) ;
   
   dummyRecord.add(dummyDataKey,
                   &dummyProxy);
   
   //typedef edm::eventsetup::MakeDataException<DummyRecord,Dummy> ExceptionType;
   dummyRecord.doGet(dummyDataKey);
   //CPPUNIT_ASSERT_THROW(dummyRecord.doGet(dummyDataKey), ExceptionType);
   
}

void testEventsetupRecord::proxyResetTest()
{
  std::auto_ptr<EventSetupRecordProvider> dummyProvider =
  EventSetupRecordProviderFactoryManager::instance().makeRecordProvider(
                                                                        EventSetupRecordKey::makeKey<DummyRecord>());
  
  EventSetupRecordProviderTemplate<DummyRecord>* prov= dynamic_cast<EventSetupRecordProviderTemplate<DummyRecord>*>(&(*dummyProvider)); 
  CPPUNIT_ASSERT(0 !=prov);
  
  const DummyRecord& dummyRecord = prov->record();

  unsigned long long cacheID = dummyRecord.cacheIdentifier();
  Dummy myDummy;
  boost::shared_ptr<WorkingDummyProxy> workingProxy( new WorkingDummyProxy(&myDummy) );
  
  const DataKey workingDataKey(DataKey::makeTypeTag<WorkingDummyProxy::value_type>(),
                               "");

  boost::shared_ptr<WorkingDummyProvider> wdProv( new WorkingDummyProvider(workingDataKey, workingProxy) );
  prov->add( wdProv );

  //this causes the proxies to actually be placed in the Record
  edm::eventsetup::EventSetupRecordProvider::DataToPreferredProviderMap pref;
  prov->usePreferred(pref);
  
  CPPUNIT_ASSERT(dummyRecord.doGet(workingDataKey));

  edm::ESHandle<Dummy> hDummy;
  dummyRecord.get(hDummy);

  CPPUNIT_ASSERT(&myDummy == &(*hDummy));
  CPPUNIT_ASSERT(cacheID == dummyRecord.cacheIdentifier());
  
  Dummy myDummy2;
  workingProxy->set(&myDummy2);

  //should not change
  dummyRecord.get(hDummy);
  CPPUNIT_ASSERT(&myDummy == &(*hDummy));
  CPPUNIT_ASSERT(cacheID == dummyRecord.cacheIdentifier());

  prov->resetProxies();
  dummyRecord.get(hDummy);
  CPPUNIT_ASSERT(&myDummy2 == &(*hDummy));
  CPPUNIT_ASSERT(cacheID != dummyRecord.cacheIdentifier());
}
