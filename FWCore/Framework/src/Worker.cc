
/*----------------------------------------------------------------------
$Id: Worker.cc,v 1.23 2007/06/05 04:02:32 wmtan Exp $
----------------------------------------------------------------------*/

#include "FWCore/Framework/src/Worker.h"

namespace edm {
  namespace {
    class CallPrePostBeginJob {
public:
      CallPrePostBeginJob(Worker::Sigs& s, ModuleDescription& md):s_(&s),md_(&md) {
        (*(s_->preModuleBeginJobSignal))(*md_);
      }
      ~CallPrePostBeginJob() {
        (*(s_->postModuleBeginJobSignal))(*md_);
      }
private:
      Worker::Sigs* s_;
      ModuleDescription* md_;
    };

    class CallPrePostEndJob {
public:
      CallPrePostEndJob(Worker::Sigs& s, ModuleDescription& md):s_(&s),md_(&md) {
        (*(s_->preModuleEndJobSignal))(*md_);
      }
      ~CallPrePostEndJob() {
        (*(s_->postModuleEndJobSignal))(*md_);
      }
private:
      Worker::Sigs* s_;
      ModuleDescription* md_;
    };
    
  }

  static ActivityRegistry::PreModule defaultPreModuleSignal;
  static ActivityRegistry::PostModule defaultPostModuleSignal;
  static ActivityRegistry::PreModuleBeginJob defaultPreModuleBeginJobSignal;
  static ActivityRegistry::PostModuleBeginJob defaultPostModuleBeginJobSignal;
  static ActivityRegistry::PreModuleEndJob defaultPreModuleEndJobSignal;
  static ActivityRegistry::PostModuleEndJob defaultPostModuleEndJobSignal;
  
  Worker::Sigs::Sigs() : preModuleSignal( &defaultPreModuleSignal ),
    postModuleSignal( &defaultPostModuleSignal ),
    preModuleBeginJobSignal(&defaultPreModuleBeginJobSignal),
    postModuleBeginJobSignal(&defaultPostModuleBeginJobSignal),
    preModuleEndJobSignal(&defaultPreModuleEndJobSignal),
    postModuleEndJobSignal(&defaultPostModuleEndJobSignal){}
  
  Worker::Worker(ModuleDescription const& iMD, 
		 WorkerParams const& iWP):
    stopwatch_(new RunStopwatch::StopwatchPointer::element_type),
    timesRun_(),
    timesVisited_(),
    timesPassed_(),
    timesFailed_(),
    timesExcept_(),
    state_(Ready),
    md_(iMD),
    actions_(iWP.actions_),
    cached_exception_(),
    sigs_()
  {
  }

  Worker::~Worker() {
  }

  void Worker::connect(ActivityRegistry::PreModule& pre,
		       ActivityRegistry::PostModule& post,
                       ActivityRegistry::PreModuleBeginJob& preBJ,
                       ActivityRegistry::PostModuleBeginJob& postBJ,
                       ActivityRegistry::PreModuleEndJob& preEJ,
                       ActivityRegistry::PostModuleEndJob& postEJ) {
    sigs_.preModuleSignal= &pre;
    sigs_.postModuleSignal= &post;
    sigs_.preModuleBeginJobSignal = &preBJ;
    sigs_.postModuleBeginJobSignal = &postBJ;
    sigs_.preModuleEndJobSignal = &preEJ;
    sigs_.postModuleEndJobSignal = &postEJ;
  }

  void Worker::beginJob(EventSetup const& es) {
    
    try {
        CallPrePostBeginJob cpp(sigs_,md_);
	implBeginJob(es);
    }
    catch(cms::Exception& e) {
	// should event id be included?
	LogError("BeginJob")
	  << "A cms::Exception is going through "<< workerType()<<":\n";

	e << "A cms::Exception is going through "<< workerType()<<":\n"
	  << description();
	throw;
    }
    catch(std::exception& e) {
	LogError("BeginJob")
	  << "An std::exception is going through "<< workerType()<<":\n"
	  << description() << "\n";
	throw;
    }
    catch(std::string& s) {
	LogError("BeginJob") 
	  << "module caught an std::string during beginJob\n";

	throw cms::Exception("BadExceptionType","std::string") 
	  << "string = " << s << "\n"
	  << description() << "\n" ;
    }
    catch(char const* c) {
	LogError("BeginJob") 
	  << "module caught an const char* during beginJob\n";

	throw cms::Exception("BadExceptionType","const char*") 
	  << "cstring = " << c << "\n"
	  << description() ;
    }
    catch(...) {
	LogError("BeginJob")
	  << "An unknown Exception occured in\n" << description() << "\n";
	throw;
    }
  }
  
  void Worker::endJob() {
    using namespace std;
    
    try {
        CallPrePostEndJob cpp(sigs_,md_);
	implEndJob();
    }
    catch(cms::Exception& e) {
	LogError("EndJob")
	  << "A cms::Exception is going through "<< workerType()<<":\n";

	// should event id be included?
	e << "A cms::Exception is going through "<< workerType()<<":\n"
	  << description();
	throw;
    }
    catch(std::exception& e) {
	LogError("EndJob")
	  << "An std::exception is going through "<< workerType()<<":\n"
	  << description() << "\n";
	throw;
    }
    catch(std::string& s) {
	LogError("EndJob") 
	  << "module caught an std::string during endJob\n";

	throw cms::Exception("BadExceptionType","std::string") 
	  << "string = " << s << "\n"
	  << description() << "\n";
    }
    catch(char const* c) {
	LogError("EndJob") 
	  << "module caught an const char* during endJob\n";

	throw cms::Exception("BadExceptionType","const char*") 
	  << "cstring = " << c << "\n"
	  << description() << "\n";
    }
    catch(...) {
	LogError("EndJob")
	  << "An unknown Exception occured in\n" << description() << "\n";
	throw;
    }
  }
  
}
