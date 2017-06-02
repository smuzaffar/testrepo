/*----------------------------------------------------------------------

Test of the EventProcessor class.

$Id: eventprocessor2_t.cppunit.cc,v 1.7 2006/05/01 16:59:10 wmtan Exp $

----------------------------------------------------------------------*/  
#include <exception>
#include <iostream>
#include <string>
#include <stdexcept>
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Framework/interface/EventProcessor.h"
#include <cppunit/extensions/HelperMacros.h>


class testeventprocessor2: public CppUnit::TestFixture
{
CPPUNIT_TEST_SUITE(testeventprocessor2);
CPPUNIT_TEST(eventprocessor2Test);
CPPUNIT_TEST_SUITE_END();
public:
  void setUp(){}
  void tearDown(){}
  void eventprocessor2Test();
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testeventprocessor2);

void work()
{
  std::string configuration("process PROD = {\n"
		    "untracked PSet maxEvents = {untracked int32 input = 5}\n"
		    "source = EmptySource { }\n"
		    "module m1 = IntProducer { int32 ivalue = 10 }\n"
		    "module m2 = DoubleProducer { double dvalue = 3.3 }\n"
		    "module out = AsciiOutputModule { }\n"
                    "path p1 = { m1,m2,out }\n"
		    "}\n");
  edm::EventProcessor proc(configuration);
  proc.beginJob();
  proc.run();
  proc.endJob();
}

void testeventprocessor2::eventprocessor2Test()
//int main()
{
  /*try { work(); rc = 0;}
  int rc = -1;                // we should never return this value!
  catch (cms::Exception& e) {
      std::cerr << "CMS exception caught: "
		<< e.explainSelf() << std::endl;
      rc = 1;
  }
  catch (seal::Error& e) {
      std::cerr << "Application exception caught: "
		<< e.explainSelf() << std::endl;
      rc = 1;
  }
  catch (std::runtime_error& e) {
      std::cerr << "Standard library exception caught: "
		<< e.what() << std::endl;
      rc = 1;
  }
  catch (...) {
      std::cerr << "Unknown exception caught" << std::endl;
      rc = 2;
  }
  return rc;*/
}
