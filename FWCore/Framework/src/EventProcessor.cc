#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>

#include <signal.h>

#include "boost/shared_ptr.hpp"
#include "boost/bind.hpp"
#include "boost/mem_fn.hpp"
#include "boost/thread/xtime.hpp"

#include "FWCore/PluginManager/interface/PluginManager.h"
#include "SealBase/Error.h"

#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include "FWCore/Utilities/interface/DebugMacros.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/GetReleaseVersion.h"
#include "FWCore/Utilities/interface/GetPassID.h"
#include "FWCore/Utilities/interface/UnixSignalHandlers.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventProcessor.h"
#include "FWCore/Framework/interface/IOVSyncValue.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/SourceFactory.h"
#include "FWCore/Framework/interface/ModuleFactory.h"
#include "FWCore/Framework/interface/LooperFactory.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/ConstProductRegistry.h"
#include "FWCore/Framework/interface/TriggerNamesService.h"

#include "FWCore/Framework/src/Worker.h"
#include "FWCore/Framework/src/InputSourceFactory.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/MakeParameterSets.h"
#include "FWCore/ParameterSet/interface/Registry.h"

#include "FWCore/ServiceRegistry/interface/ServiceRegistry.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "FWCore/Framework/interface/Schedule.h"
#include "FWCore/Framework/interface/EDLooperHelper.h"
#include "FWCore/Framework/interface/EDLooper.h"

using namespace std;
using boost::shared_ptr;
using edm::serviceregistry::ServiceLegacy; 
using edm::serviceregistry::kOverlapIsError;

namespace edm {

  namespace event_processor {
    class StateSentry
    {
    public:
      StateSentry(EventProcessor* ep):ep_(ep),success_(false) { }
      ~StateSentry() { if(!success_) ep_->changeState(mException); }
      void succeeded() { success_ = true; }

    private:
      EventProcessor* ep_;
      bool success_;
    };
  }

  using namespace event_processor;
  using namespace edm::service;

  typedef vector<string>   StrVec;
  typedef list<string>     StrList;
  typedef Worker*          WorkerPtr;
  typedef list<WorkerPtr>  WorkerList;
  typedef list<WorkerList> PathList;


  namespace {

    // the next two tables must be kept in sync with the state and
    // message enums from the header

    char const* stateNames[] = {
      "Init",
      "JobReady",
      "RunGiven",
      "Running",
      "Stopping",
      "ShuttingDown",
      "Done",
      "JobEnded",
      "Error",
      "End",
      "Invalid"
    };

    char const* msgNames[] = {
      "SetRun",
      "Skip",
      "RunAsync",
      "Run(ID)",
      "Run(count)",
      "BeginJob",
      "StopAsync",
      "ShutdownAsync",
      "EndJob",
      "CountComplete",
      "InputExhausted",
      "StopSignal",
      "ShutdownSignal",
      "Finished",
      "Any",
      "dtor",
      "Exception",
      "Rewind"
    };
  }
    // IMPORTANT NOTE:
    // the mAny messages are special, they must appear last in the
    // table if multiple entries for a CurrentState are present.
    // the changeState function does not use the mAny yet!!!

    struct TransEntry
    {
      State current;
      Msg   message;
      State final;
    };

    // we should use this information to initialize a two dimensional
    // table of t[CurrentState][Message] = FinalState

    /*
      the way this is current written, the async run can thread function
      can return in the "JobReady" state - but not yet cleaned up.  The
      problem is that only when stop/shutdown async is called is the 
      thread cleaned up. But the stop/shudown async functions attempt
      first to change the state using messages that are not valid in
      "JobReady" state.

      I think most of the problems can be solved by using two states
      for "running": RunningS and RunningA (sync/async). The problems
      seems to be the all the transitions out of running for both
      modes of operation.  The other solution might be to only go to
      "Stopping" from Running, and use the return code from "run_p" to
      set the final state.  If this is used, then in the sync mode the
      "Stopping" state would be momentary.

     */

    TransEntry table[] = {
    // CurrentState   Message         FinalState
    // -----------------------------------------
      { sInit,          mException,      sError },
      { sInit,          mBeginJob,       sJobReady },
      { sJobReady,      mException,      sError },
      { sJobReady,      mSetRun,         sRunGiven },
      { sJobReady,      mSkip,           sRunning },
      { sJobReady,      mRunID,          sRunning },
      { sJobReady,      mRunCount,       sRunning },
      { sJobReady,      mEndJob,         sJobEnded },
      { sJobReady,      mBeginJob,       sJobReady },
      { sJobReady,      mDtor,           sEnd },    // should this be allowed?

      { sJobReady,      mStopAsync,      sJobReady },
      { sJobReady,      mCountComplete,  sJobReady },

      { sRunGiven,      mException,      sError },
      { sRunGiven,      mRunAsync,       sRunning },
      { sRunGiven,      mBeginJob,       sRunGiven },
      { sRunGiven,      mShutdownAsync,  sShuttingDown },
      { sRunGiven,      mStopAsync,      sStopping },
      { sRunning,       mException,      sError },
      { sRunning,       mStopAsync,      sStopping },
      { sRunning,       mShutdownAsync,  sShuttingDown },
      { sRunning,       mShutdownSignal, sShuttingDown },
      { sRunning,       mCountComplete,  sStopping }, // sJobReady 
      { sRunning,       mInputExhausted, sStopping }, // sJobReady

      { sStopping,       mInputRewind, sJobReady },
      { sStopping,      mException,      sError },
      { sStopping,      mFinished,       sJobReady },
      { sStopping,      mCountComplete,  sJobReady },
      { sStopping,      mShutdownSignal, sShuttingDown },
      { sStopping,      mStopAsync,      sStopping },     // stay
      //{ sStopping,      mAny,            sJobReady },     // <- ??????
      { sShuttingDown,  mException,      sError },
      { sShuttingDown,  mCountComplete,  sDone }, // needed?
      { sShuttingDown,  mInputExhausted, sDone }, // needed?
      { sShuttingDown,  mFinished,       sDone },
      //{ sShuttingDown,  mShutdownAsync,  sShuttingDown }, // only one at
      //{ sShuttingDown,  mStopAsync,      sShuttingDown }, // a time
      //{ sShuttingDown,  mAny,            sDone },         // <- ??????
      { sDone,          mEndJob,         sJobEnded },
      { sDone,          mException,      sError },
      { sJobEnded,      mDtor,           sEnd },
      { sJobEnded,      mException,      sError },
      { sError,         mEndJob,         sError },   // funny one here
      { sError,         mDtor,           sError },   // funny one here
      { sInit,          mDtor,           sEnd },     // for StorM dummy EP
      { sStopping,      mShutdownAsync,  sShuttingDown }, // For FUEP tests
      { sInvalid,       mAny,            sInvalid }
    };


    // Note: many of the messages generate the mBeginJob message first 
    //  mRunID, mRunCount, mSetRun

  // ---------------------------------------------------------------
  shared_ptr<InputSource> 
  makeInput(ParameterSet const& params,
	    EventProcessor::CommonParams const& common,
	    ProductRegistry& preg,
            ActivityRegistry& areg)
  {
    // find single source
    bool sourceSpecified = false;
    try {
      ParameterSet main_input =
	params.getParameter<ParameterSet>("@main_input");
      
      // Fill in "ModuleDescription", in case the input source produces
      // any EDproducts,which would be registered in the ProductRegistry.
      // Also fill in the process history item for this process.
      ModuleDescription md;
      md.parameterSetID_ = main_input.id();
      md.moduleName_ =
	main_input.getParameter<std::string>("@module_type");
      // There is no module label for the unnamed input source, so 
      // just use "source".
      md.moduleLabel_ = "source";
      md.processConfiguration_ = ProcessConfiguration(common.processName_,
				params.id(), getReleaseVersion(), getPassID());

      sourceSpecified = true;
      InputSourceDescription isdesc(md, preg, common.maxEventsInput_);
      areg.preSourceConstructionSignal_(md);
      shared_ptr<InputSource> input(InputSourceFactory::get()->makeInputSource(main_input, isdesc).release());
      areg.postSourceConstructionSignal_(md);
      
      return input;
    } 
    catch(edm::Exception const& iException) {
 	if(sourceSpecified == false && 
	   errors::Configuration == iException.categoryCode()) {
 	    throw edm::Exception(errors::Configuration, "FailedInputSource")
	      << "Configuration of main input source has failed\n"
	      << iException;
 	} else {
 	    throw;
 	}
    }
    return shared_ptr<InputSource>();
  }
  
  // ---------------------------------------------------------------
  static
  std::auto_ptr<eventsetup::EventSetupProvider>
  makeEventSetupProvider(ParameterSet const& params)
  {
    using namespace std;
    using namespace edm::eventsetup;
    vector<string> prefers =
      params.getParameter<vector<string> >("@all_esprefers");

    if(prefers.empty()) {
      return std::auto_ptr<EventSetupProvider>(new EventSetupProvider());
    }

    EventSetupProvider::PreferredProviderInfo preferInfo;
    EventSetupProvider::RecordToDataMap recordToData;

    //recordToData.insert(std::make_pair(std::string("DummyRecord"),
    //      std::make_pair(std::string("DummyData"),std::string())));
    //preferInfo[ComponentDescription("DummyProxyProvider","",false)]=
    //      recordToData;

    for(vector<string>::iterator itName = prefers.begin(), itNameEnd = prefers.end();
	itName != itNameEnd;
	++itName) 
      {
        recordToData.clear();
	ParameterSet preferPSet = params.getParameter<ParameterSet>(*itName);
        std::vector<std::string> recordNames = preferPSet.getParameterNames();
        for(vector<string>::iterator itRecordName = recordNames.begin(),
	    itRecordNameEnd = recordNames.end();
            itRecordName != itRecordNameEnd;
            ++itRecordName) {

	  if((*itRecordName)[0] == '@') {
	    //this is a 'hidden parameter' so skip it
	    continue;
	  }

	  //this should be a record name with its info
	  try {
	    std::vector<std::string> dataInfo =
	      preferPSet.getParameter<vector<std::string> >(*itRecordName);
	    
	    if(dataInfo.empty()) {
	      //FUTURE: empty should just mean all data
	      throw cms::Exception("Configuration")
		<< "The record named "
		<< *itRecordName << " specifies no data items";
	    }
	    //FUTURE: 'any' should be a special name
	    for(std::vector<std::string>::iterator itDatum = dataInfo.begin(),
	        itDatumEnd = dataInfo.end();
		itDatum != itDatumEnd;
		++itDatum){
	      std::string datumName(*itDatum, 0, itDatum->find_first_of("/"));
	      std::string labelName;

	      if(itDatum->size() != datumName.size()) {
		labelName = std::string(*itDatum, datumName.size()+1);
	      }
	      recordToData.insert(std::make_pair(std::string(*itRecordName),
						 std::make_pair(datumName,
								labelName)));
	    }
	  } catch(cms::Exception const& iException) {
	    cms::Exception theError("ESPreferConfigurationError");
	    theError << "While parsing the es_prefer statement for type="
		     << preferPSet.getParameter<std::string>("@module_type")
		     << " label=\""
		     << preferPSet.getParameter<std::string>("@module_label")
		     << "\" an error occurred.";
	    theError.append(iException);
	    throw theError;
	  }
        }
        preferInfo[ComponentDescription(preferPSet.getParameter<std::string>("@module_type"),
                                        preferPSet.getParameter<std::string>("@module_label"),
                                        false)]
	  = recordToData;
      }
    return std::auto_ptr<EventSetupProvider>(new EventSetupProvider(&preferInfo));
  }
  
  // ---------------------------------------------------------------
  void 
  fillEventSetupProvider(edm::eventsetup::EventSetupProvider& cp,
			 ParameterSet const& params,
			 EventProcessor::CommonParams const& common)
  {
    using namespace std;
    using namespace edm::eventsetup;
    vector<string> providers =
      params.getParameter<vector<string> >("@all_esmodules");

    for(vector<string>::iterator itName = providers.begin(), itNameEnd = providers.end();
	itName != itNameEnd;
	++itName) 
      {
	ParameterSet providerPSet = params.getParameter<ParameterSet>(*itName);
	ModuleFactory::get()->addTo(cp, 
				    providerPSet, 
				    common.processName_, 
				    common.releaseVersion_, 
				    common.passID_);
      }
    
    vector<string> sources = 
      params.getParameter<vector<string> >("@all_essources");

    for(vector<string>::iterator itName = sources.begin(), itNameEnd = sources.end();
	itName != itNameEnd;
	++itName) 
      {
	ParameterSet providerPSet = params.getParameter<ParameterSet>(*itName);
	SourceFactory::get()->addTo(cp, 
				    providerPSet, 
				    common.processName_, 
				    common.releaseVersion_, 
				    common.passID_);
    }
  }

  // ---------------------------------------------------------------
  //need a wrapper to let me 'copy' references to EventSetup

  namespace eventprocessor 
  {
    struct ESRefWrapper 
    {
      EventSetup const& es_;
      ESRefWrapper(EventSetup const& iES) : es_(iES) {}
      operator EventSetup const&() { return es_; }
    };
  }

  using eventprocessor::ESRefWrapper;

  // ---------------------------------------------------------------
  EventProcessor::DoPluginInit::DoPluginInit()
  { 
    seal::PluginManager::get()->initialise();
    // std::cerr << "Initialized plugin manager" << std::endl;

    // for now, install sigusr2 function.
    installSig(SIGUSR2,edm::ep_sigusr2);
  }


  // ---------------------------------------------------------------
  // a bit of a hack to make sure that at least a minimal parameter
  // set for the message logger is included in the services list

  // Add a service to the services list
  void adjustForService(vector<ParameterSet>& adjust,
			string const& service)
  {
    FDEBUG(1) << "Adding default " << service << " Service\n";
    ParameterSet newpset;
    newpset.addParameter<string>("@service_type",service);
    adjust.push_back(newpset);
    // Record this new ParameterSet in the Registry!
    pset::Registry::instance()->insertMapped(newpset);
  }

  // Add a service to the services list if it is not already there
  void adjustForDefaultService(vector<ParameterSet>& adjust,
			       string const& service)
  {
    typedef std::vector<edm::ParameterSet>::iterator Iter;
    for(Iter it = adjust.begin(), itEnd = adjust.end(); it != itEnd; ++it) {
	string name = it->getParameter<std::string>("@service_type");

	if (name == service) {
 
          // If the service is already there move it to the end so
          // it will be created before all the others already there
          // This means we use the order from the default services list
          // and the parameters from the configuration file
          while (true) {
            Iter iterNext = it + 1;
            if (iterNext == itEnd) return;
            iter_swap(it, iterNext);
            ++it;
          }
        }
    }
    adjustForService(adjust, service);
  }


  // ---------------------------------------------------------------
  boost::shared_ptr<edm::EDLooper> 
  fillLooper(edm::eventsetup::EventSetupProvider& cp,
			 ParameterSet const& params,
			 EventProcessor::CommonParams const& common)
  {
    using namespace std;
    using namespace edm::eventsetup;
    boost::shared_ptr<edm::EDLooper> vLooper;
    
    vector<string> loopers =
      params.getParameter<vector<string> >("@all_loopers");

    if(loopers.size() == 0) {
       return vLooper;
    }
   
    assert(1 == loopers.size());

    for(vector<string>::iterator itName = loopers.begin(), itNameEnd = loopers.end();
	itName != itNameEnd;
	++itName) 
      {
	ParameterSet providerPSet = params.getParameter<ParameterSet>(*itName);
	vLooper = LooperFactory::get()->addTo(cp, 
				    providerPSet, 
				    common.processName_, 
				    common.releaseVersion_, 
				    common.passID_);
        vLooper->setLooperName(common.processName_);
        vLooper->setLooperPassID(common.passID_);
      }
      return vLooper;
    
  }

  // ---------------------------------------------------------------

  EventProcessor::EventProcessor(string const& config,
				ServiceToken const& iToken, 
				serviceregistry::ServiceLegacy iLegacy,
			        vector<string> const& defaultServices,
				vector<string> const& forcedServices) :
    preProcessEventSignal(),
    postProcessEventSignal(),
    plug_init_(),
    maxEventsPset_(),
    maxEventsInput_(-1),
    actReg_(new ActivityRegistry),
    wreg_(actReg_),
    preg_(),
    serviceToken_(),
    input_(),
    schedule_(),
    esp_(),
    act_table_(),
    state_(sInit),
    event_loop_(),
    state_lock_(),
    stop_lock_(),
    stopper_(),
    stop_count_(-1),
    last_rc_(epSuccess),
    last_error_text_(),
    id_set_(false),
    event_loop_id_(),
    my_sig_num_(getSigNum()),
    looper_()
  {
    init(config, iToken, iLegacy, defaultServices, forcedServices);
  }

  EventProcessor::EventProcessor(string const& config,
			        vector<string> const& defaultServices,
				vector<string> const& forcedServices) :
    preProcessEventSignal(),
    postProcessEventSignal(),
    plug_init_(),
    maxEventsPset_(),
    maxEventsInput_(-1),
    actReg_(new ActivityRegistry),
    wreg_(actReg_),
    preg_(),
    serviceToken_(),
    input_(),
    schedule_(),
    esp_(),
    act_table_(),
    state_(sInit),
    event_loop_(),
    state_lock_(),
    stop_lock_(),
    stopper_(),
    stop_count_(-1),
    last_rc_(epSuccess),
    last_error_text_(),
    id_set_(false),
    event_loop_id_(),
    my_sig_num_(getSigNum()),
    looper_()
  {
    init(config, ServiceToken(), serviceregistry::kOverlapIsError, defaultServices, forcedServices);
  }

  void
  EventProcessor::init(string const& config,
			ServiceToken const& iToken, 
			serviceregistry::ServiceLegacy iLegacy,
		        vector<string> const& defaultServices,
			vector<string> const& forcedServices) {
    // TODO: Fix const-correctness. The ParameterSets that are
    // returned here should be const, so that we can be sure they are
    // not modified.

    shared_ptr<ParameterSet> parameterSet;
    shared_ptr<vector<ParameterSet> > pServiceSets;
    makeParameterSets(config, parameterSet, pServiceSets);
    maxEventsPset_ = parameterSet->getUntrackedParameter<ParameterSet>("maxEvents", ParameterSet());
    maxEventsInput_ = maxEventsPset_.getUntrackedParameter<int>("input", -1);

    // Add the forced and default services to pServiceSets.
    // In pServiceSets, we want the default services first, then the forced
    // services, then the services from the configuration.  It is efficient
    // and convenient to add them in reverse order.  Then after we are done
    // adding, we reverse the vector again to get the desired order.
    std::reverse(pServiceSets->begin(), pServiceSets->end());
    for(vector<string>::const_reverse_iterator j = forcedServices.rbegin(),
                                            jEnd = forcedServices.rend();
	 j != jEnd; ++j) {
      adjustForService(*(pServiceSets.get()), *j);
    }
    for(vector<string>::const_reverse_iterator i = defaultServices.rbegin(),
                                            iEnd = defaultServices.rend();
	 i != iEnd; ++i) {
      adjustForDefaultService(*(pServiceSets.get()), *i);
    }
    std::reverse(pServiceSets->begin(), pServiceSets->end());

    //create the services
    ServiceToken tempToken(ServiceRegistry::createSet(*pServiceSets, iToken, iLegacy));

    // Copy slots that hold all the registered callback functions like
    // PostBeginJob into an ActivityRegistry that is owned by EventProcessor
    tempToken.copySlotsTo(*actReg_); 
    
    //add the ProductRegistry as a service ONLY for the construction phase
    typedef serviceregistry::ServiceWrapper<ConstProductRegistry> w_CPR;
    shared_ptr<w_CPR>
      reg(new w_CPR(std::auto_ptr<ConstProductRegistry>(new ConstProductRegistry(preg_))));
    ServiceToken tempToken2(ServiceRegistry::createContaining(reg, 
							      tempToken, 
							      kOverlapIsError));

    // the next thing is ugly: pull out the trigger path pset and 
    // create a service and extra token for it
    string processName = parameterSet->getParameter<string>("@process_name");

    typedef edm::service::TriggerNamesService TNS;
    typedef serviceregistry::ServiceWrapper<TNS> w_TNS;

    shared_ptr<w_TNS> tnsptr
      (new w_TNS(std::auto_ptr<TNS>(new TNS(*parameterSet))));

    serviceToken_ = ServiceRegistry::createContaining(tnsptr, 
						    tempToken2, 
						    kOverlapIsError);

    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);
     
    //parameterSet = builder.getProcessPSet();
    act_table_ = ActionTable(*parameterSet);
    CommonParams common = CommonParams(processName,
			   getReleaseVersion(),
			   getPassID(),
			   maxEventsInput_);

    esp_ = makeEventSetupProvider(*parameterSet);
    fillEventSetupProvider(*esp_, *parameterSet, common);
    looper_ = fillLooper(*esp_, *parameterSet, common);
     
    input_= makeInput(*parameterSet, common, preg_,*actReg_);
    schedule_ = std::auto_ptr<Schedule>
      (new Schedule(*parameterSet,
		    ServiceRegistry::instance().get<TNS>(),
		    wreg_,
		    preg_,
		    act_table_,
		    actReg_));

    //   initialize(iToken,iLegacy);
    FDEBUG(2) << parameterSet->toString() << std::endl;
    connectSigs(this);
  }

  EventProcessor::~EventProcessor()
  {
    try {
      changeState(mDtor);
    }
    catch(cms::Exception& e)
      {
	LogError("System")
	  << e.explainSelf() << "\n";
      }

    // Make the services available while everything is being deleted.
    ServiceToken token = getToken();
    ServiceRegistry::Operate op(token); 
    // manually destroy all these thing that may need the services around
    esp_.reset();
    schedule_.reset();
    input_.reset();
    wreg_.clear();
    actReg_.reset();
  }

  namespace {
    class CallPrePost {
    public:
      CallPrePost(ActivityRegistry& a): a_(&a) { 
        a_->preSourceSignal_(); }
      ~CallPrePost() { 
        a_->postSourceSignal_();
      }
    
    private:
      ActivityRegistry* a_;
    };
  }  

  void
  EventProcessor::rewind()
  {
    changeState(mInputRewind);
    input_->repeat();
    input_->rewind();
    return;
  }
  
  EventHelperDescription
  EventProcessor::runOnce()
  {
    try {
       // Job should be in sJobReady state, then we send mRunCount message and move job sRunning state
       if(state_ == sJobReady) {
          changeState(mRunCount);
       }
    } catch(...) {
       actReg_->postEndJobSignal_();
       throw;
    }
    EventHelperDescription evtDesc;
    if(state_ != sRunning) {
       return evtDesc;
    }
    StateSentry toerror(this);

    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);

//  Lay on a lock.
//  N. B. It's a scoped lock so be sure to give it a scope.
//  That's the reason for the apparently gratuitous { ... }.
//  They are NOT gratuitous! Bad things will happen without them!
    {
      boost::mutex::scoped_lock sl(usr2_lock);
      if(edm::shutdown_flag)
      {
         changeState(mShutdownSignal);
         toerror.succeeded();
         return evtDesc;
      }
    }

    std::auto_ptr<EventPrincipal> pep;
    CallPrePost holder(*actReg_);

    pep = input_->readEvent();

    if(pep.get() == 0) {
      changeState(mInputExhausted);
      toerror.succeeded();
      return evtDesc;
    }

    IOVSyncValue ts(pep->id(), pep->time());
    EventSetup const& es = esp_->eventSetupForInstance(ts);
    
    schedule_->runOneEvent(*pep.get(), es, BranchActionEvent);
    toerror.succeeded();
    return EventHelperDescription(pep,&es);
  }
  
  void
  EventProcessor::endLumiAndRun(EventPrincipal & ep, bool isNewRun) const {
    IOVSyncValue ts(ep.id(), ep.time());
    input_->doEndLumiAndRun();
    EventSetup const& es = esp_->eventSetupForInstance(ts);
    schedule_->runOneEvent(ep, es, BranchActionEndLumi);
    if (isNewRun) schedule_->runOneEvent(ep, es, BranchActionEndRun);
  }

  EventProcessor::StatusCode
  EventProcessor::run_p(int numberToProcess, Msg m)
  {
    changeState(m);
    StateSentry toerror(this);

    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);

    bool runforever = numberToProcess < 0;
    bool got_sig = false;
    int eventcount = 0;
    StatusCode rc = epSuccess;

    std::auto_ptr<EventPrincipal> previousPep;

    while(state_ == sRunning) {

//  Lay on a lock
      {
        boost::mutex::scoped_lock sl(usr2_lock);
        if(edm::shutdown_flag) {
          if (previousPep.get() != 0) endLumiAndRun(*previousPep.get());
          changeState(mShutdownSignal);
          rc = epSignal;
          got_sig = true;
          continue;
        }
      }

      if(!runforever && eventcount >= numberToProcess) {
	if (previousPep.get() != 0) {
	  endLumiAndRun(*previousPep.get());
	}
	changeState(mCountComplete);
	continue;
      }

      ++eventcount;
      FDEBUG(1) << eventcount << std::endl;
      std::auto_ptr<EventPrincipal> pep;
      {
        CallPrePost holder(*actReg_);
        pep = input_->readEvent();
      }
        
      if (pep.get() == 0) {
	if (previousPep.get() != 0) endLumiAndRun(*previousPep.get());
	changeState(mInputExhausted);
	rc = epInputComplete;
	continue;
      }

      bool isANewLumi = !isSameLumi(previousPep.get(), pep.get());
      bool isANewRun = !isSameRun(previousPep.get(), pep.get());
      if(isANewLumi) {
      if (previousPep.get() != 0) endLumiAndRun(*previousPep.get(), isANewRun);
      }

      IOVSyncValue ts(pep->id(), pep->time());
      EventSetup const& es = esp_->eventSetupForInstance(ts);
	
      if (isANewLumi) {
        if (isANewRun) {
	  schedule_->runOneEvent(*pep.get(), es, BranchActionBeginRun);
        }
	schedule_->runOneEvent(*pep.get(), es, BranchActionBeginLumi);
      }
      schedule_->runOneEvent(*pep.get(), es, BranchActionEvent);
      if (schedule_->terminate()) {
        endLumiAndRun(*pep.get(), true);
	changeState(mCountComplete);
      }

      previousPep = pep;
    }

    // check once more for shutdown signal
    {
      boost::mutex::scoped_lock sl(usr2_lock);
      if(!got_sig && edm::shutdown_flag) {
        changeState(mShutdownSignal);
        rc = epSignal;
      }
    }

    toerror.succeeded();
    return rc;
  }

  EventProcessor::StatusCode
  EventProcessor::run(int numberToProcess)
  {
    beginJob(); //make sure this was called
    StatusCode rc = epInputComplete;
    if(looper_) {
       EDLooperHelper looperHelper(this);
       looper_->loop(looperHelper,numberToProcess);
    } else {
       rc = run_p(numberToProcess,mRunCount);
    }
    changeState(mFinished);
    return rc;
  }
  
  EventProcessor::StatusCode
  EventProcessor::run(EventID const& id)
  {
    beginJob(); //make sure this was called
    changeState(mRunID);
    StateSentry toerror(this);
    Status rc = epSuccess;

    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);

    std::auto_ptr<EventPrincipal> pep;
    {
      CallPrePost holder(*actReg_);
      pep = input_->readEvent(id);
    }

    if(pep.get() == 0) {
	changeState(mInputExhausted);
	rc = epInputComplete;
    }
    else
    {
	IOVSyncValue ts(pep->id(), pep->time());
	EventSetup const& es = esp_->eventSetupForInstance(ts);

	schedule_->runOneEvent(*pep.get(), es, BranchActionEvent);
	changeState(mCountComplete);
    }

    toerror.succeeded();
    changeState(mFinished);
    return rc;
  }

  EventProcessor::StatusCode
  EventProcessor::skip(int numberToSkip)
  {
    beginJob(); //make sure this was called
    changeState(mSkip);
    {
      StateSentry toerror(this);

      //make the services available
      ServiceRegistry::Operate operate(serviceToken_);
      
      {
        CallPrePost holder(*actReg_);
        input_->skipEvents(numberToSkip);
      }
      changeState(mCountComplete);
      toerror.succeeded();
    }
    changeState(mFinished);
    return epSuccess;
  }

  void
  EventProcessor::beginJob() 
  {
    if(state_ != sInit) return;
    // can only be run if in the initial state
    changeState(mBeginJob);

    // StateSentry toerror(this); // should we add this ? 
    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);
    
    //NOTE:  This implementation assumes 'Job' means one call 
    // the EventProcessor::run
    // If it really means once per 'application' then this code will
    // have to be changed.
    // Also have to deal with case where have 'run' then new Module 
    // added and do 'run'
    // again.  In that case the newly added Module needs its 'beginJob'
    // to be called.
    EventSetup const& es =
      esp_->eventSetupForInstance(IOVSyncValue::beginOfTime());
    try {
    input_->doBeginJob(es);
    } catch(cms::Exception& e) {
      LogError("BeginJob") << "A cms::Exception happened while processing the beginJob of the 'source'\n";
      e << "A cms::Exception happened while processing the beginJob of the 'source'\n";
      throw;
    } catch(std::exception& e)
    {
      LogError("BeginJob") << "A std::exception happened while processing the beginJob of the 'source'\n";
      throw;
    } catch(...)
    {
      LogError("BeginJob") << "An unknown exception happened while processing the beginJob of the 'source'\n";
      throw;
    }
    schedule_->beginJob(es);
    actReg_->postBeginJobSignal_();
    if(looper_) {
       looper_->beginOfJob(es);
    }
    // toerror.succeeded(); // should we add this?
  }

  void
  EventProcessor::endJob() 
  {
    // only allowed to run if state is sIdle,sJobReady,sRunGiven
    changeState(mEndJob);

    //make the services available
    ServiceRegistry::Operate operate(serviceToken_);  

    if(looper_) {
       looper_->endOfJob();
    }
    try {
	schedule_->endJob();
    }
    catch(...) {
      try {
	input_->doEndJob();
      }
      catch (...) {
	// If schedule_->endJob() and input_->doEndJob() both throw, we will
	// lose the exception information from input_->doEndJob().  So what!
      }
      actReg_->postEndJobSignal_();
      throw;
    }
    try {
	input_->doEndJob();
    }
    catch(...) {
      actReg_->postEndJobSignal_();
      throw;
    }
    actReg_->postEndJobSignal_();
  }

  ServiceToken
  EventProcessor::getToken()
  {
    return serviceToken_;
  }

  void
  EventProcessor::connectSigs(EventProcessor* ep)
  {
    // When the FwkImpl signals are given, pass them to the
    // appropriate EventProcessor signals so that the outside world
    // can see the signal.
    actReg_->preProcessEventSignal_.connect(ep->preProcessEventSignal);
    actReg_->postProcessEventSignal_.connect(ep->postProcessEventSignal);
  }

  InputSource&
  EventProcessor::getInputSource()
  {
    return *input_;
  }

  std::vector<ModuleDescription const*>
  EventProcessor::getAllModuleDescriptions() const
  {
    return schedule_->getAllModuleDescriptions();
  }

  int
  EventProcessor::totalEvents() const
  {
    return schedule_->totalEvents();
  }

  int
  EventProcessor::totalEventsPassed() const
  {
    return schedule_->totalEventsPassed();
  }

  int
  EventProcessor::totalEventsFailed() const
  {
    return schedule_->totalEventsFailed();
  }

  void 
  EventProcessor::enableEndPaths(bool active)
  {
    schedule_->enableEndPaths(active);
  }

  bool 
  EventProcessor::endPathsEnabled() const
  {
    return schedule_->endPathsEnabled();
  }
  
  void
  EventProcessor::getTriggerReport(TriggerReport& rep) const
  {
    schedule_->getTriggerReport(rep);
  }

  char const* EventProcessor::currentStateName() const
  {
    return stateName(getState());
  }

  char const* EventProcessor::stateName(State s) const
  {
    return stateNames[s];
  }

  char const* EventProcessor::msgName(Msg m) const
  {
    return msgNames[m];
  }

  State EventProcessor::getState() const
  {
    return state_;
  }

  EventProcessor::StatusCode EventProcessor::statusAsync() const
  {
    // the thread will record exception/error status in the event processor
    // for us to look at and report here
    return last_rc_;
  }

  void EventProcessor::setRunNumber(RunNumber_t runNumber)
  {
    beginJob();
    changeState(mSetRun);

    // interface not correct yet
    getInputSource().setRunNumber(runNumber);

    LogWarning("state")
      << "EventProcessor::setRunNumber not yet implemented\n";
  }

  EventProcessor::StatusCode 
  EventProcessor::waitForAsyncCompletion(unsigned int timeout_seconds)
  {
    bool rc = true;
    boost::xtime timeout;
    boost::xtime_get(&timeout, boost::TIME_UTC); 
    timeout.sec += timeout_seconds;

    // make sure to include a timeout here so we don't wait forever
    // I suspect there are still timing issues with thread startup
    // and the setting of the various control variables (stop_count,id_set)
    {
      boost::mutex::scoped_lock sl(stop_lock_);

      // look here - if runAsync not active, just return the last return code
      if(stop_count_ < 0) return last_rc_;

      if(timeout_seconds==0)
	while(stop_count_==0) stopper_.wait(sl);
      else
	while(stop_count_==0 &&
	      (rc = stopper_.timed_wait(sl,timeout)) == true);
      
      if(rc == false)
	{
	  // timeout occurred
	  // if(id_set_) pthread_kill(event_loop_id_,my_sig_num_);
	  // this is a temporary hack until we get the input source
	  // upgraded to allow blocking input sources to be unblocked

	  // the next line is dangerous and causes all sorts of trouble
	  if(id_set_) pthread_cancel(event_loop_id_);

	  // we will not do anything yet
	  LogWarning("timeout")
	    << "An asynchronous request was made to shut down "
	    << "the event loop "
	    << "and the event loop did not shutdown after "
	    << timeout_seconds << " seconds\n";
	}
      else
	{
	  event_loop_->join();
	  event_loop_.reset();
	  id_set_ = false;
	  stop_count_ = -1;
	}
    }
    return rc==false?epTimedOut:last_rc_;
  }

  EventProcessor::StatusCode 
  EventProcessor::waitTillDoneAsync(unsigned int timeout_value_secs)
  {
    StatusCode rc = waitForAsyncCompletion(timeout_value_secs);
    if(rc!=epTimedOut) changeState(mCountComplete);
    else errorState();
    return rc;
  }

  
  EventProcessor::StatusCode EventProcessor::stopAsync(unsigned int secs)
  {
    changeState(mStopAsync);
    StatusCode rc = waitForAsyncCompletion(secs);
    if(rc!=epTimedOut) changeState(mFinished);
    else errorState();
    return rc;
  }
  
  EventProcessor::StatusCode EventProcessor::shutdownAsync(unsigned int secs)
  {
    changeState(mShutdownAsync);
    StatusCode rc = waitForAsyncCompletion(secs);
    if(rc!=epTimedOut) changeState(mFinished);
    else errorState();
    return rc;
  }
  
  void EventProcessor::errorState()
  {
    state_ = sError;
  }

  // next function irrelevant now
  EventProcessor::StatusCode EventProcessor::doneAsync(Msg m)
  {
    // make sure to include a timeout here so we don't wait forever
    // I suspect there are still timing issues with thread startup
    // and the setting of the various control variables (stop_count,id_set)
    changeState(m);
    return waitForAsyncCompletion(60*2);
  }
  
  void EventProcessor::changeState(Msg msg)
  {
    // most likely need to serialize access to this routine

    boost::mutex::scoped_lock sl(state_lock_);
    State curr = state_;
    int rc;
    // found if (not end of table) and 
    // (state == table.state && (msg == table.message || msg == any))
    for(rc = 0;
	table[rc].current != sInvalid && 
	  (curr != table[rc].current || 
	   (curr == table[rc].current && 
	     msg != table[rc].message && table[rc].message != mAny));
	++rc);

    if(table[rc].current == sInvalid)
      throw cms::Exception("BadState")
	<< "A member function of EventProcessor has been called in an"
	<< " inappropriate order.\n"
	<< "Bad transition from " << stateName(curr) << " "
	<< "using message " << msgName(msg) << "\n"
	<< "No where to go from here.\n";

    FDEBUG(1) << "changeState: current=" << stateName(curr)
	      << ", message=" << msgName(msg) 
	      << " -> new=" << stateName(table[rc].final) << "\n";

    state_ = table[rc].final;
  }

  void EventProcessor::runAsync()
  {
    using boost::thread;
    beginJob();
    {
      boost::mutex::scoped_lock sl(stop_lock_);
      if(id_set_==true)
	{
	  string err("runAsync called while async event loop already running\n");
	  edm::LogError("FwkJob") << err;
	  throw cms::Exception("BadState") << err;
	}

      stop_count_=0;
      last_rc_=epSuccess; // forget the last value!
      event_loop_.reset(new thread(boost::bind(EventProcessor::asyncRun,this)));
      boost::xtime timeout;
      boost::xtime_get(&timeout, boost::TIME_UTC); 
      timeout.sec += 60; // 60 seconds to start!!!!
      if(starter_.timed_wait(sl,timeout)==false)
	{
	  // yikes - the thread did not start
	  throw cms::Exception("BadState")
	    << "Async run thread did not start in 60 seconds\n";
	}      
    }
  }

  void EventProcessor::asyncRun(EventProcessor* me)
  {
    // set up signals to allow for interruptions
    // ignore all other signals
    // make sure no exceptions escape out

    // temporary hack until we modify the input source to allow
    // wakeup calls from other threads.  This mimics the solution
    // in EventFilter/Processor, which I do not like.
    // allowing cancels means that the thread just disappears at
    // certain points.  This is bad for C++ stack variables.
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,0);
    //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,0);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);

    {
      boost::mutex::scoped_lock(me->stop_lock_);
      me->event_loop_id_ = pthread_self();
      me->id_set_ = true;
      me->starter_.notify_all();
    }

    Status rc = epException;
    FDEBUG(2) << "asyncRun starting >>>>>>>>>>>>>>>>>>>>>>\n";

    try {
	rc = me->run_p(-1, mRunAsync);
    }
    catch (cms::Exception& e) {
      edm::LogError("FwkJob") << "cms::Exception caught in "
			      << "EventProcessor::asyncRun" 
			      << "\n"
			      << e.explainSelf();
      me->last_error_text_ = e.explainSelf();
    }
    catch (seal::Error& e) {
      edm::LogError("FwkJob") << "seal::Exception caught in " 
			      << "EventProcessor::asyncRun" 
			      << "\n"
			      << e.explainSelf();
      me->last_error_text_ = e.explainSelf();
    }
    catch (std::exception& e) {
      edm::LogError("FwkJob") << "Standard library exception caught in " 
			      << "EventProcessor::asyncRun" 
			      << "\n"
			      << e.what();
      me->last_error_text_ = e.what();
    }
    catch (...) {
      edm::LogError("FwkJob") << "Unknown exception caught in "
			      << "EventProcessor::asyncRun" 
			      << "\n";
      me->last_error_text_ = "Unknown exception caught";
      rc = epOther;
    }

    me->last_rc_ = rc;

    {
      // notify anyone waiting for exit that we are doing so now
      boost::mutex::scoped_lock sl(me->stop_lock_);
      ++me->stop_count_;
      me->stopper_.notify_all();
    }
    FDEBUG(2) << "asyncRun ending >>>>>>>>>>>>>>>>>>>>>>\n";
  }
}
