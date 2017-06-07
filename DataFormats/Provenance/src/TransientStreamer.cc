#include "DataFormats/Provenance/interface/BranchDescription.h"
#include "DataFormats/Provenance/interface/ProductProvenance.h"
#include "DataFormats/Provenance/interface/FileIndex.h"
#include "DataFormats/Provenance/interface/ProcessHistory.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/Transient.h"
#include "DataFormats/Provenance/interface/TransientStreamer.h"

#include "DataFormats/Provenance/interface/EventEntryDescription.h" // backward compatibility
#include "DataFormats/Provenance/interface/EventEntryInfo.h" // backward compatibility

namespace edm {
  void setTransientStreamers() {
    SetTransientStreamer<Transient<BranchDescription::Transients> >();
    SetTransientStreamer<Transient<ProductProvenance::Transients> >();
    SetTransientStreamer<Transient<FileIndex::Transients> >();
    SetTransientStreamer<Transient<ProcessHistory::Transients> >();
    SetTransientStreamer<Transient<ProductRegistry::Transients> >();
    SetTransientStreamer<Transient<EventEntryDescription::Transients> >(); // backward compatibility
    SetTransientStreamer<Transient<EventEntryInfo::Transients> >(); // backward compatibility
  }
}
