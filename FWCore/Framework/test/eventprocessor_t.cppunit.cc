/*----------------------------------------------------------------------

Test of the EventProcessor class.

$Id: eventprocessor_t.cppunit.cc,v 1.25 2007/03/17 02:30:49 jbk Exp $

----------------------------------------------------------------------*/  
#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include "boost/regex.hpp"
#include "SealBase/Error.h"

//I need to open a 'back door' in order to test the functionality
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#define private public
#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#undef private
#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/Framework/interface/EventProcessor.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/Utilities/interface/Presence.h"
#include "FWCore/PluginManager/interface/PresenceFactory.h"
#include "FWCore/Framework/test/stubs/TestBeginEndJobAnalyzer.h"

#include "FWCore/PluginManager/interface/ProblemTracker.h"

#include "FWCore/Framework/interface/Frameworkfwd.h"

#include "DataFormats/Provenance/interface/ModuleDescription.h"


#include "cppunit/extensions/HelperMacros.h"

class testeventprocessor: public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testeventprocessor);
  CPPUNIT_TEST(parseTest);
  CPPUNIT_TEST(prepostTest);
  CPPUNIT_TEST(beginEndJobTest);
  CPPUNIT_TEST(cleanupJobTest);
  CPPUNIT_TEST(activityRegistryTest);
  CPPUNIT_TEST(moduleFailureTest);
  CPPUNIT_TEST(endpathTest);
  CPPUNIT_TEST(asyncTest);
  CPPUNIT_TEST_SUITE_END();

 public:

  void setUp()
  {
    m_handler = std::auto_ptr<edm::AssertHandler>(new edm::AssertHandler());
    sleep_secs_ = 0;
  }

  void tearDown(){ m_handler.reset();}
  void parseTest();
  void prepostTest();
  void beginEndJobTest();
  void cleanupJobTest();
  void activityRegistryTest();
  void moduleFailureTest();
  void endpathTest();

  void asyncTest();
  bool asyncRunAsync(edm::EventProcessor& ep);
  bool asyncRunTimeout(edm::EventProcessor& ep);
  void driveAsyncTest( bool (testeventprocessor::*)(edm::EventProcessor&),
		       const std::string& config_string);

 private:
  std::auto_ptr<edm::AssertHandler> m_handler;
  void work()
  {
    std::string configuration("process p = {\n"
			      "untracked PSet maxEvents = {untracked int32 input = 5}\n"
			      "source = EmptySource { }\n"
			      "module m1 = TestMod { int32 ivalue = 10 }\n"
			      "module m2 = TestMod { int32 ivalue = -3 }\n"
			      "path p1 = { m1,m2 }\n"
			      "}\n");
    edm::EventProcessor proc(configuration);
    proc.beginJob();
    proc.run();
    proc.endJob();
  }

  int sleep_secs_;
};

///registration of the test so that the runner can find it
CPPUNIT_TEST_SUITE_REGISTRATION(testeventprocessor);

static std::string makeConfig(int event_count)
{
  static const std::string start = "process p = {\n"
    "untracked PSet maxEvents = {untracked int32 input = ";
  static const std::string finish = " }\n"
    "service=MessageLogger{ untracked vstring destinations={\n"
    "\"cout\",\"cerr\"}\n"
    "untracked vstring categories={\"FwkJob\",\"FwkReport\"}\n"
    "untracked PSet cout={untracked string thresold=\"INFO\"\n"
    "untracked PSet FwkReport={untracked int32 limit=0}}\n"
    "untracked PSet cerr={untracked string thresold=\"INFO\"\n"
    "untracked PSet FwkReport={untracked int32 limit=0}}\n"
    "} source = EmptySource { }\n"
    "module m1 = IntProducer { int32 ivalue = 10 }\n"
    "path p1 = { m1 }\n"
    "}\n";

  std::ostringstream ost;
  ost << start << event_count << finish;
  return ost.str();
}

void testeventprocessor::asyncTest()
{
  std::string test_config_2 = makeConfig(2);
  std::string test_config_80k = makeConfig(20000);
  
  // Load the message service plug-in
  boost::shared_ptr<edm::Presence> theMessageServicePresence;
  try {
    theMessageServicePresence =
      boost::shared_ptr<edm::Presence>(edm::PresenceFactory::get()->makePresence("MessageServicePresence").release());
  } catch(seal::Error& e) {
    std::cerr << e.explainSelf() << std::endl;
    return;
  }

  sleep_secs_=0;
  std::cerr << "asyncRunAsync 2 event\n";
  driveAsyncTest(&testeventprocessor::asyncRunAsync,test_config_2);
  std::cerr << "asyncRunAsync 80k event\n";
  driveAsyncTest(&testeventprocessor::asyncRunAsync,test_config_80k);
  sleep_secs_=3;
  std::cerr << "asyncRunAsync 2 event with sleep 3\n";
  driveAsyncTest(&testeventprocessor::asyncRunAsync,test_config_2);
  sleep_secs_=0;
  std::cerr << "asyncRunTimeout 80k event\n";
  // cannot run the following test from scram because of a runtime
  // library error:
  // libgcc_s.so.1 must be installed for pthread_cancel to work
  // driveAsyncTest(&testeventprocessor::asyncRunTimeout,test_config_80k);
}

bool testeventprocessor::asyncRunAsync(edm::EventProcessor& ep)
{
  for(int i=0;i<3;++i)
    {
      ep.setRunNumber(i+1);
      ep.runAsync();
      if(sleep_secs_>0) sleep(sleep_secs_);
      edm::EventProcessor::StatusCode rc = ep.waitTillDoneAsync(1000);
      std::cerr << " ep runAsync run " << i << " done\n";
  
      switch(rc)
	{
	case edm::EventProcessor::epSuccess:
	case edm::EventProcessor::epInputComplete:
	  break;
	case edm::EventProcessor::epTimedOut:
	default:
	  {
	    std::cerr << "rc from run "<< i <<", doneAsync = " << rc << "\n";
	    CPPUNIT_ASSERT("Bad rc from doneAsync"==0);
	  }
	}
    }
  return true;
}

bool testeventprocessor::asyncRunTimeout(edm::EventProcessor& ep)
{
  ep.setRunNumber(1);
  ep.runAsync();
  edm::EventProcessor::StatusCode rc = ep.waitTillDoneAsync(1);
  std::cerr << " ep runAsync run " << 1 << " done\n";
  
  switch(rc)
    {
    case edm::EventProcessor::epTimedOut:
      break;
    case edm::EventProcessor::epSuccess:
    case edm::EventProcessor::epInputComplete:
      break;
    default:
      {
	std::cerr << "rc from run "<< 1 <<", doneAsync = " << rc << "\n";
	CPPUNIT_ASSERT("Bad rc from doneAsync"==0);
      }
    }
  return false;
}

void testeventprocessor::driveAsyncTest( bool(testeventprocessor::*func)(edm::EventProcessor& ep),const std::string& config_str )
{

  try {
    edm::EventProcessor proc(config_str);
    proc.beginJob();
    if ((this->*func)(proc))
      proc.endJob();
    else
      {
	std::cerr << "event processor is in error state\n";
      }
  }
  catch(cms::Exception& e)
    {
      std::cerr << "cms exception: " << e.explainSelf() << "\n";
      CPPUNIT_ASSERT("cms exeption"==0);
    }
  catch(std::exception& e)
    {
      std::cerr << "std exception: " << e.what() << "\n";
      CPPUNIT_ASSERT("std exeption"==0);
    }
  catch(...)
    {
      std::cerr << "unknown exception " << "\n";
      CPPUNIT_ASSERT("unknown exeption"==0);
    }
  std::cerr << "*********************** driveAsyncTest ending ------\n";
}

void testeventprocessor::parseTest()
{
  int rc = -1;                // we should never return this value!
  try { work(); rc = 0;}
  catch (cms::Exception& e) {
      std::cerr << "cms exception caught: "
		<< e.explainSelf() << std::endl;
      CPPUNIT_ASSERT("Caught cms::Exception " == 0);
  }
  catch (seal::Error& e) {
      std::cerr << "Application exception caught: "
		<< e.explainSelf() << std::endl;
      CPPUNIT_ASSERT("Caught seal::Error " == 0);
  }
  catch (std::exception& e) {
      std::cerr << "Standard library exception caught: "
		<< e.what() << std::endl;
      CPPUNIT_ASSERT("Caught std::exception " == 0);
  }
  catch (...) {
      CPPUNIT_ASSERT("Caught unknown exception " == 0);
  }
}

static int g_pre = 0;
static int g_post = 0;

static
void doPre(const edm::EventID&, const edm::Timestamp&) 
{
  ++g_pre;
}

static
void doPost(const edm::Event&, const edm::EventSetup&) 
{
  CPPUNIT_ASSERT(g_pre == ++g_post);
}

void testeventprocessor::prepostTest()
{
  std::string configuration("process p = {\n"
			    "untracked PSet maxEvents = {untracked int32 input = 5}\n"
			    "source = EmptySource { }\n"
			    "module m1 = TestMod { int32 ivalue = -3 }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
  edm::EventProcessor proc(configuration);
   
  proc.preProcessEventSignal.connect(&doPre);
  proc.postProcessEventSignal.connect(&doPost);
  proc.beginJob();
  proc.run();
  proc.endJob();
  CPPUNIT_ASSERT(5 == g_pre);
  CPPUNIT_ASSERT(5 == g_post);
  {
    edm::EventProcessor const& crProc(proc);
    typedef std::vector<edm::ModuleDescription const*> ModuleDescs;
    ModuleDescs  allModules = crProc.getAllModuleDescriptions();
    CPPUNIT_ASSERT(1 == allModules.size());
    std::cout << "\nModuleDescriptions in testeventprocessor::prepostTest()---\n";
    for (ModuleDescs::const_iterator i = allModules.begin(),
	    e = allModules.end() ; 
	  i != e ; 
	  ++i)
      {
	CPPUNIT_ASSERT(*i != 0);
	std::cout << **i << '\n';
      }
    std::cout << "--- end of ModuleDescriptions\n";

    CPPUNIT_ASSERT(5 == crProc.totalEvents());
    CPPUNIT_ASSERT(5 == crProc.totalEventsPassed());    
  }
}

void testeventprocessor::beginEndJobTest()
{
  std::string configuration("process p = {\n"
			    "untracked PSet maxEvents = {untracked int32 input = 2}\n"
			    "source = EmptySource { }\n"
			    "module m1 = TestBeginEndJobAnalyzer { }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
  {
    edm::EventProcessor proc(configuration);
      
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::beginJobCalled);
    proc.beginJob();
    CPPUNIT_ASSERT(TestBeginEndJobAnalyzer::beginJobCalled);
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::endJobCalled);
    proc.endJob();
    CPPUNIT_ASSERT(TestBeginEndJobAnalyzer::endJobCalled);
  }
  {
    TestBeginEndJobAnalyzer::beginJobCalled = false;
    TestBeginEndJobAnalyzer::endJobCalled = false;

    edm::EventProcessor proc(configuration);
    //run should call beginJob if it hasn't happened already
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::beginJobCalled);
    proc.run(1);
    CPPUNIT_ASSERT(TestBeginEndJobAnalyzer::beginJobCalled);

    //second call to run should NOT call beginJob
    TestBeginEndJobAnalyzer::beginJobCalled = false;
    proc.run(1);
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::beginJobCalled);

  }
  //In this case, endJob should not have been called since was not done explicitly
  CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::endJobCalled);
   
}

void testeventprocessor::cleanupJobTest()
{
  std::string configuration("process p = {\n"
			    "untracked PSet maxEvents = {untracked int32 input = 2}\n"
			    "source = EmptySource { }\n"
			    "module m1 = TestBeginEndJobAnalyzer { }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
  {
    TestBeginEndJobAnalyzer::destructorCalled = false;
    edm::EventProcessor proc(configuration);
      
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);
    proc.beginJob();
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);
    proc.endJob();
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);
  }
  CPPUNIT_ASSERT(TestBeginEndJobAnalyzer::destructorCalled);
  {
    TestBeginEndJobAnalyzer::destructorCalled = false;
    edm::EventProcessor proc(configuration);

    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);
    proc.run(1);
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);
    proc.run(1);
    CPPUNIT_ASSERT(!TestBeginEndJobAnalyzer::destructorCalled);

  }
  CPPUNIT_ASSERT(TestBeginEndJobAnalyzer::destructorCalled);
}

namespace {
  struct Listener{
    Listener(edm::ActivityRegistry& iAR) :
      postBeginJob_(false),
      postEndJob_(false),
      preEventProcessing_(false),
      postEventProcessing_(false),
      preModule_(false),
      postModule_(false){
	iAR.watchPostBeginJob(this,&Listener::postBeginJob);
	iAR.watchPostEndJob(this,&Listener::postEndJob);

	iAR.watchPreProcessEvent(this,&Listener::preEventProcessing);
	iAR.watchPostProcessEvent(this,&Listener::postEventProcessing);

	iAR.watchPreModule(this, &Listener::preModule);
	iAR.watchPostModule(this, &Listener::postModule);
      }
         
    void postBeginJob() {postBeginJob_=true;}
    void postEndJob() {postEndJob_=true;}
      
    void preEventProcessing(const edm::EventID&, const edm::Timestamp&){
      preEventProcessing_=true;}
    void postEventProcessing(const edm::Event&, const edm::EventSetup&){
      postEventProcessing_=true;}
      
    void preModule(const edm::ModuleDescription&){
      preModule_=true;
    }
    void postModule(const edm::ModuleDescription&){
      postModule_=true;
    }
      
    bool allCalled() const {
      return postBeginJob_&&postEndJob_
	&&preEventProcessing_&&postEventProcessing_
	&&preModule_&&postModule_;
    }
      
    bool postBeginJob_;
    bool postEndJob_;
    bool preEventProcessing_;
    bool postEventProcessing_;
    bool preModule_;
    bool postModule_;      
  };
}

void 
testeventprocessor::activityRegistryTest()
{
  std::string configuration("process p = {\n"
			    "untracked PSet maxEvents = {untracked int32 input = 5}\n"
			    "source = EmptySource { }\n"
			    "module m1 = TestMod { int32 ivalue = -3 }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
   
  //We don't want any services, we just want an ActivityRegistry to be created
  // We then use this ActivityRegistry to 'spy on' the signals being produced
  // inside the EventProcessor
  std::vector<edm::ParameterSet> serviceConfigs;
  edm::ServiceToken token = edm::ServiceRegistry::createSet(serviceConfigs);

  edm::ActivityRegistry ar;
  token.connect(ar);
  Listener listener(ar);
   
  edm::EventProcessor proc(configuration,token, edm::serviceregistry::kOverlapIsError);
   
  proc.beginJob();
  proc.run();
  proc.endJob();
   
  CPPUNIT_ASSERT(listener.postBeginJob_);
  CPPUNIT_ASSERT(listener.postEndJob_);
  CPPUNIT_ASSERT(listener.preEventProcessing_);
  CPPUNIT_ASSERT(listener.postEventProcessing_);
  CPPUNIT_ASSERT(listener.preModule_);
  CPPUNIT_ASSERT(listener.postModule_);      
   
  CPPUNIT_ASSERT(listener.allCalled());
}

static
bool
findModuleName(const std::string& iMessage) {
  static const boost::regex expr("TestFailuresAnalyzer");
  return regex_search(iMessage,expr);
}

void 
testeventprocessor::moduleFailureTest()
{
  try {
    const std::string preC("process p = {\n"
			   "untracked PSet maxEvents = {untracked int32 input = 2}\n"
			   "source = EmptySource { }\n"
			   "module m1 = TestFailuresAnalyzer { int32 whichFailure =");
    const std::string postC(" }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
    {
      const std::string configuration = preC +"0"+postC;
      bool threw = true;
      try {
	edm::EventProcessor proc(configuration);
	threw = false;
      } catch(const cms::Exception& iException){
	if(!findModuleName(iException.explainSelf())) {
	  std::cout <<iException.explainSelf()<<std::endl;
	  CPPUNIT_ASSERT(0 == "module name not in exception message");
	}
      }
      CPPUNIT_ASSERT(threw && 0 != "exception never thrown");
    }
    {
      const std::string configuration = preC +"1"+postC;
      bool threw = true;
      edm::EventProcessor proc(configuration);
         
      try {
	proc.beginJob();
	threw = false;
      } catch(const cms::Exception& iException){
	if(!findModuleName(iException.explainSelf())) {
	  std::cout <<iException.explainSelf()<<std::endl;
	  CPPUNIT_ASSERT(0 == "module name not in exception message");
	}
      }
      CPPUNIT_ASSERT(threw && 0 != "exception never thrown");
    }
      
    {
      const std::string configuration = preC +"2"+postC;
      bool threw = true;
      edm::EventProcessor proc(configuration);
         
      proc.beginJob();
      try {
	proc.run(1);
	threw = false;
      } catch(const cms::Exception& iException){
	if(!findModuleName(iException.explainSelf())) {
	  std::cout <<iException.explainSelf()<<std::endl;
	  CPPUNIT_ASSERT(0 == "module name not in exception message");
	}
      }
      CPPUNIT_ASSERT(threw && 0 != "exception never thrown");
      proc.endJob();
    }
    {
      const std::string configuration = preC +"3"+postC;
      bool threw = true;
      edm::EventProcessor proc(configuration);
         
      proc.beginJob();
      try {
	proc.endJob();
	threw = false;
      } catch(const cms::Exception& iException){
	if(!findModuleName(iException.explainSelf())) {
	  std::cout <<iException.explainSelf()<<std::endl;
	  CPPUNIT_ASSERT(0 == "module name not in exception message");
	}
      }
      CPPUNIT_ASSERT(threw && 0 != "exception never thrown");
    }
    ///
    {
      bool threw = true;
      try {
        const std::string configuration("process p = {\n"
			       "untracked PSet maxEvents = {untracked int32 input = 2}\n"
                               "source = EmptySource { }\n"
                                "path p1 = { m1 }\n"
                                "}\n");
        edm::EventProcessor proc(configuration);
      
	threw = false;
      } catch(const cms::Exception& iException){
        static const boost::regex expr("m1");
	if(!regex_search(iException.explainSelf(),expr)) {
	  std::cout <<iException.explainSelf()<<std::endl;
	  CPPUNIT_ASSERT(0 == "module name not in exception message");
	}
      }
      CPPUNIT_ASSERT(threw && 0 != "exception never thrown");
    }
    
  } catch(const cms::Exception& iException) {
    std::cout <<"Unexpected exception "<<iException.explainSelf()<<std::endl;
    throw;
  }
}

void
testeventprocessor::endpathTest()
{
  std::string configuration("process p = {\n"
			    "untracked PSet maxEvents = {untracked int32 input = 5}\n"
			    "source = EmptySource { }\n"
			    "module m1 = TestMod { int32 ivalue = -3 }\n"
			    "path p1 = { m1 }\n"
			    "}\n");
  
}
