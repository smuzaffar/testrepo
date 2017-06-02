
#include "FWCore/Framework/src/Path.h"
#include "FWCore/Framework/interface/Actions.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <string>

using namespace std;

namespace edm
{
  namespace
  {
    class CallPrePost
    {
    public:
      CallPrePost(ActivityRegistry* areg,const string& name, int bit):
	a_(areg),name_(name),bit_(bit)
      {
	//a_->prePathSignal(name_,bit_);
      }
      ~CallPrePost()
      {
	//a_->postPathSignal(name_,bit_);
      }
    private:
      ActivityRegistry* a_;
      std::string name_;
      int bit_;
    };
  }
  

  Path::Path(int bitpos, const std::string& path_name,
	     const Workers& workers,
	     BitMaskPtr bitmask,
	     ParameterSet const& proc_pset,
	     ActionTable& actions,
	     ActivityRegistryPtr areg):
    stopwatch_(new TStopwatch),
    timesRun_(),
    timesPassed_(),
    timesFailed_(),
    timesExcept_(),
    abortWorker_(),
    state_(Ready),
    bitpos_(bitpos),
    name_(path_name),
    bitmask_(bitmask),
    act_reg_(areg),
    act_table_(&actions),
    workers_(workers)
  {
    stopwatch_->Stop();
  }

#if 0
  void Worker::connect(ActivityRegistry::PrePath& pre,
		       ActivityRegistry::PostPath& post)
  {
    sigs_.prePathSignal.connect(pre);
    sigs_.postPathSignal.connect(post);
  }
#endif
  
  void Path::runOneEvent(EventPrincipal& ep, EventSetup const& es)
  {
    RunStopwatch stopwatch(stopwatch_);
    ++timesRun_;
    state_ = Ready;
    abortWorker_=0;
    CallPrePost cpp(act_reg_.get(),name_,bitpos_);
    Workers::iterator i(workers_.begin()),e(workers_.end());
    bool rc = true;
    for(;i!=e && rc==true;++i)
      {
	try
	  {
	    rc = i->runWorker(ep,es);
	  }
	catch(cms::Exception& e)
	  {
	    // there is no support as of yet for specific paths having
	    // different exception behavior

	    actions::ActionCodes code = act_table_->find(e.rootCause());

	    switch(code)
	      {
	      case actions::IgnoreCompletely:
		{
		  rc=true;
		  LogWarning(e.category())
		    << "Ignoring Exception in path " << name_
		    << ", message:\n" << e.what() << "\n";
		  break;
		}
	      case actions::FailPath:
		{
		  rc=false;
		  LogWarning(e.category())
		    << "Failing path " << name_
		    << ", due to exception, message:\n"
		    << e.what() << "\n";
		  break;
		}
	      default:
		{
		  LogError(e.category())
		    << "Exception going through path " << name_ << "\n";
                  ++timesExcept_;
                  state_ = Exception;
		  throw edm::Exception(errors::ScheduleExecutionFailure,
				       "ProcessingStopped", e)
		    << "Exception going through path " << name_ << "\n";
		}
	      }
	  }
	catch(...)
	  {
	    LogError("PassingThrough")
	      << "Exception passing through path " << name_ << "\n";
            ++timesExcept_;
            state_ = Exception;
	    throw;
	  }
        ++abortWorker_;
      }

    if(rc)
      {
        ++timesPassed_;
        state_=Pass;
      }
    else
      {
        ++timesFailed_;
        state_=Fail;
      }

    (*bitmask_)[bitpos_]=rc;

  }

}
