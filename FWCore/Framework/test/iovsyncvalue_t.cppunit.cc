/*
 *  eventid_t.cppunit.cc
 *  CMSSW
 *
 *  Created by Chris Jones on 8/8/05.
 *  Copyright 2005 __MyCompanyName__. All rights reserved.
 *
 */

#include <cppunit/extensions/HelperMacros.h>

#include "FWCore/Framework/interface/IOVSyncValue.h"

using namespace edm;

class testIOVSyncValue: public CppUnit::TestFixture
{
   CPPUNIT_TEST_SUITE(testIOVSyncValue);
   
   CPPUNIT_TEST(constructTest);
   CPPUNIT_TEST(constructTimeTest);
   CPPUNIT_TEST(comparisonTest);
   CPPUNIT_TEST(comparisonTimeTest);
   
   CPPUNIT_TEST_SUITE_END();
public:
      void setUp(){}
   void tearDown(){}
   
   void constructTest();
   void comparisonTest();
   void constructTimeTest();
   void comparisonTimeTest();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testIOVSyncValue);


void testIOVSyncValue::constructTest()
{
   const EventID t(2,0);

   IOVSyncValue temp( t);
   
   CPPUNIT_ASSERT( temp.eventID() == t );
   
   CPPUNIT_ASSERT( IOVSyncValue::invalidIOVSyncValue() < IOVSyncValue::beginOfTime() );
   CPPUNIT_ASSERT( IOVSyncValue::beginOfTime() < IOVSyncValue::endOfTime() );

   CPPUNIT_ASSERT( IOVSyncValue::invalidIOVSyncValue() != temp );
   CPPUNIT_ASSERT( !(IOVSyncValue::invalidIOVSyncValue() == temp) );
   CPPUNIT_ASSERT( IOVSyncValue::beginOfTime() < temp );
   CPPUNIT_ASSERT( IOVSyncValue::endOfTime() > temp );
}

void testIOVSyncValue::constructTimeTest()
{
   const Timestamp t(2);
   
   IOVSyncValue temp( t);
   
   CPPUNIT_ASSERT( temp.time() == t );
   
   CPPUNIT_ASSERT( IOVSyncValue::invalidIOVSyncValue() < IOVSyncValue::beginOfTime() );
   CPPUNIT_ASSERT( IOVSyncValue::beginOfTime() < IOVSyncValue::endOfTime() );
   
   CPPUNIT_ASSERT( IOVSyncValue::invalidIOVSyncValue() != temp );
   CPPUNIT_ASSERT( !(IOVSyncValue::invalidIOVSyncValue() == temp) );
   CPPUNIT_ASSERT( IOVSyncValue::beginOfTime() < temp );
   CPPUNIT_ASSERT( IOVSyncValue::endOfTime() > temp );
}

void testIOVSyncValue::comparisonTest()
{
   const IOVSyncValue small( EventID(1,1));
   const IOVSyncValue med( EventID(2,2) );
   
   CPPUNIT_ASSERT( small < med);
   CPPUNIT_ASSERT( small <= med);
   CPPUNIT_ASSERT( !(small == med) );
   CPPUNIT_ASSERT( small != med);
   CPPUNIT_ASSERT( !(small > med) );
   CPPUNIT_ASSERT( !(small >= med) );
   
}


void testIOVSyncValue::comparisonTimeTest()
{
   const IOVSyncValue small( Timestamp(1));
   const IOVSyncValue med( Timestamp(2) );
   
   CPPUNIT_ASSERT( small < med);
   CPPUNIT_ASSERT( small <= med);
   CPPUNIT_ASSERT( !(small == med) );
   CPPUNIT_ASSERT( small != med);
   CPPUNIT_ASSERT( !(small > med) );
   CPPUNIT_ASSERT( !(small >= med) );

}
