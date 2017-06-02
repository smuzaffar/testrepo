#ifndef Framework_WorkerRegistry_h
#define Framework_WorkerRegistry_h

/**
   \file
   Declaration of class ModuleRegistry

   \author Stefano ARGIRO
   \version $Id: WorkerRegistry.h,v 1.8 2006/07/06 19:11:43 wmtan Exp $
   \date 18 May 2005
*/

#include "DataFormats/Provenance/interface/PassID.h"
#include "DataFormats/Provenance/interface/ReleaseVersion.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/src/WorkerParams.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"

#include "boost/shared_ptr.hpp"

#include <map>
#include <string>


namespace edm {

  class Worker;

  /**
     \class ModuleRegistry ModuleRegistry.h "edm/ModuleRegistry.h"

     \brief The Registry of all workers that where requested
     Holds all instances of workers. In this implementation, Workers 
     are owned.

     \author Stefano ARGIRO
     \date 18 May 200
  */

  class WorkerRegistry {

  public:

    WorkerRegistry();
    explicit WorkerRegistry(boost::shared_ptr<ActivityRegistry> areg);
    ~WorkerRegistry();
        
    /// Retrieve the particular instance of the worker
    /** If the worker with that set of parameters does not exist,
        create it
        @note Workers are owned by this class, do not delete them*/
    Worker*  getWorker(WorkerParams const&);
    void clear();
    
  private:
    // Disable Assignment and copy construction
    WorkerRegistry(WorkerRegistry const&);         // not implemented
    void operator=(WorkerRegistry const&) const;   // not implemented
  
    /// Get a unique name for the worker
    /** Form a string to be used as a key in the map of workers */
    std::string mangleWorkerParameters(ParameterSet const& parameterSet,
				       std::string const& processName,
				       ReleaseVersion const& releaseVersion,
				       PassID const& passID);

    /// the container of workers
    typedef std::map<std::string, boost::shared_ptr<Worker> > WorkerMap;

    /// internal map of registered workers (owned). 
    WorkerMap m_workerMap;
    boost::shared_ptr<ActivityRegistry> act_reg_;
     
  }; // WorkerRegistry


} // edm


#endif
