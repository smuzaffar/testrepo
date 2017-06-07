#include "DataFormats/Provenance/interface/EventEntryDescription.h"
#include "DataFormats/Provenance/interface/ModuleDescriptionRegistry.h"
#include "DataFormats/Provenance/interface/EntryDescriptionRegistry.h"
#include <ostream>

/*----------------------------------------------------------------------

----------------------------------------------------------------------*/

namespace edm {
  EventEntryDescription::EventEntryDescription() :
    parents_(),
    moduleDescriptionID_(),
    transients_()
  { }

  void
  EventEntryDescription::init() const {
    if (!moduleDescriptionPtr()) {
      moduleDescriptionPtr().reset(new ModuleDescription);
      bool found = ModuleDescriptionRegistry::instance()->getMapped(moduleDescriptionID_, *moduleDescriptionPtr());

      assert(found);
    }
  }

  EntryDescriptionID
  EventEntryDescription::id() const
  {
    // This implementation is ripe for optimization.
    std::ostringstream oss;
    oss << moduleDescriptionID_ << ' ';
    for (std::vector<BranchID>::const_iterator 
	   i = parents_.begin(),
	   e = parents_.end();
	 i != e;
	 ++i)
      {
	oss << *i << ' ';
      }
    
    std::string stringrep = oss.str();
    cms::Digest md5alg(stringrep);
    return EntryDescriptionID(md5alg.digest().toString());
  }


  void
  EventEntryDescription::write(std::ostream& os) const {
    // This is grossly inadequate, but it is not critical for the
    // first pass.
    os << "Module Description ID = " << moduleDescriptionID() << '\n';
  }
    
  bool
  operator==(EventEntryDescription const& a, EventEntryDescription const& b) {
    return
      a.parents() == b.parents()
      && a.moduleDescriptionID() == b.moduleDescriptionID();
  }
}
