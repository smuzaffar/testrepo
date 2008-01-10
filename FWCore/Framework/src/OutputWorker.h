#ifndef Framework_OutputWorker_h
#define Framework_OutputWorker_h

/*----------------------------------------------------------------------
  
OutputWorker: The OutputModule as the schedule sees it.  The job of
this object is to call the output module.

According to our current definition, a single output module can only
appear in one worker.

$Id: OutputWorker.h,v 1.30 2008/01/08 21:51:43 wmtan Exp $
----------------------------------------------------------------------*/

#include <memory>

#include "boost/shared_ptr.hpp"

#include "FWCore/Framework/src/Worker.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"

namespace edm {

  class OutputWorker : public Worker {
  public:
    OutputWorker(std::auto_ptr<OutputModule> mod, 
		 ModuleDescription const&,
		 WorkerParams const&);

    virtual ~OutputWorker();

    template <class ModType>
    static std::auto_ptr<OutputModule> makeOne(ModuleDescription const& md,
					WorkerParams const& wp);

    // Call maybeEndFile() on the controlled OutputModule.
    void maybeEndFile();

    // Call closeFile() on the controlled OutputModule.
    void closeFile();

    void openNewFileIfNeeded();

    bool wantAllEvents() const;

    // These next functions take pointers rather than references
    // as arguments to avoid a copy when used in generic algorithms.

    void openFile(FileBlock const* fb);

    void writeRun(RunPrincipal const* rp);

    void writeLumi(LuminosityBlockPrincipal const* lbp);

    void respondToOpenInputFile(FileBlock const* fb);

    void respondToCloseInputFile(FileBlock const* fb);

    bool limitReached() const;

    void configure(OutputModuleDescription const& desc);

  private:
    virtual bool implDoWork(EventPrincipal& e, EventSetup const& c,
			    BranchActionType,
			    CurrentProcessingContext const* cpc);
    virtual bool implDoWork(RunPrincipal& rp, EventSetup const& c,
			    BranchActionType bat,
			    CurrentProcessingContext const* cpc);
    virtual bool implDoWork(LuminosityBlockPrincipal& lbp, EventSetup const& c,
			    BranchActionType bat,
			    CurrentProcessingContext const* cpc);

    virtual void implBeginJob(EventSetup const&) ;
    virtual void implEndJob() ;
    virtual std::string workerType() const;
    
    boost::shared_ptr<OutputModule> mod_;
  };

  template <> 
  struct WorkerType<OutputModule> {
    typedef OutputModule ModuleType;
    typedef OutputWorker worker_type;
  };

  template <class ModType>
  std::auto_ptr<OutputModule> OutputWorker::makeOne(ModuleDescription const& md,
						    WorkerParams const& wp) {
    std::auto_ptr<ModType> module = std::auto_ptr<ModType>(new ModType(*wp.pset_));
    module->setModuleDescription(md);
    return std::auto_ptr<OutputModule>(module.release());
  }

}

#endif
