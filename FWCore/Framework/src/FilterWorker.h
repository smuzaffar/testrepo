#ifndef EDM_EDFILTERWORKER_INCLUDED
#define EDM_EDFILTERWORKER_INCLUDED

/*----------------------------------------------------------------------
  
FilterWorker: The EDFilter as the schedule sees it.  The job of
this object is to call the filter.
According to our current definition, a single filter can only
appear in one worker.

$Id: FilterWorker.h,v 1.3 2005/07/06 18:48:18 wmtan Exp $

----------------------------------------------------------------------*/

#include <memory>

#include "boost/shared_ptr.hpp"

#include "FWCore/CoreFramework/src/Worker.h"
#include "FWCore/CoreFramework/interface/ModuleDescription.h"

namespace edm
{
  class EventPrincipal;
  class ModuleDescription;
  class EDFilter;

  class FilterWorker : public Worker
  {
  public:
    FilterWorker(std::auto_ptr<EDFilter>, const ModuleDescription&);
    virtual ~FilterWorker();

  private:
    virtual bool doWork(EventPrincipal& e, EventSetup const& c);
    virtual void beginJob( EventSetup const& ) ;
    virtual void endJob() ;

    ModuleDescription md_;
    boost::shared_ptr<EDFilter> filter_;
  };

  template <> 
  struct WorkerType<EDFilter>
  {
    typedef EDFilter ModuleType;
    typedef FilterWorker worker_type;
  };

}

#endif
