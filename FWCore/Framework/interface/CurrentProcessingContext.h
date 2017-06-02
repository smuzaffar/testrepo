#ifndef FWCore_Framework_CurrentProcessingContext
#define FWCore_Framework_CurrentProcessingContext

/// CurrentProcessingContext is a class that carries information about
/// the current event processing context. Each module in a framework
/// job can access its CurrentProcessingContext *when that module is
/// active in event processing*. At such a time, the
/// CurrentProcessingContext will provide information about that
/// module's place in the schedule, *as seen at that moment*.
///
/// N.B.: An individual module instance can appear in more than one
/// path; this is why CurrentProcessingContext reports the module's
/// place in the schedule as seen at the time of execution. This is
/// also why the module can not be queried for this information when
/// it is not active in processing.

#include <cstddef> // for size_t
#include <string>

#include "FWCore/Framework/interface/Frameworkfwd.h"

namespace edm
{
  class CurrentProcessingContext
  {
  public:

    /// Default-constructed objects reflect the inactive state.
    CurrentProcessingContext();

    /// Create a CurrentProcessingContext ready to handle the Path
    /// with given name and bit position (slot in Schedule).

    CurrentProcessingContext(std::string const* name,
			     int bitpos);			     

    /// The compiler-generated copy c'tor and d'tor are correct,
    /// because all our resources are contained by value. We do not
    /// own the resources to which we point; we own only the pointers.
    
    /// Return the address of the moduleLabel if the module is active,
    /// and null otherwise.
    std::string const* moduleLabel() const;


    /// Return the name of the current path if the module is active,
    /// and null otherwise.
    std::string const* pathName() const;

    /// Return the address of the ModuleDescription describing this
    /// module if active, and null otherwise.
    ModuleDescription const* moduleDescription() const;


    /// Set the context to reflect the active state.
    void activate(std::size_t              slotInPath,
		  ModuleDescription const* mod);

    /// Set all data to reflect inactive state.
    void deactivate();

  private:

    // N.B.: We own none of the pointed-to resources!
    int                      pathInSchedule_;
    std::size_t              slotInPath_;
    ModuleDescription const* moduleDescription_;
    std::string const*       pathName_;
  };
}


#endif
