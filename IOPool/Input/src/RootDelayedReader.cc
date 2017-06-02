/*----------------------------------------------------------------------
$Id: RootDelayedReader.cc,v 1.1 2006/01/06 02:35:46 wmtan Exp $
----------------------------------------------------------------------*/

#include "IOPool/Input/src/RootDelayedReader.h"
#include "IOPool/Common/interface/RefStreamer.h"
#include "FWCore/Framework/interface/BranchKey.h"

#include "TClass.h"

namespace edm {
  RootDelayedReader::~RootDelayedReader() {}

  std::auto_ptr<EDProduct>
  RootDelayedReader::get(BranchKey const& k, EDProductGetter const* ep) const {
    SetRefStreamer(ep);
    TBranch *br = branches().find(k)->second.second;
    TClass *cp = gROOT->GetClass(branches().find(k)->second.first.c_str());
    std::auto_ptr<EDProduct> p(static_cast<EDProduct *>(cp->New()));
    EDProduct *pp = p.get();
    br->SetAddress(&pp);
    br->GetEntry(entryNumber_);
    return p;
  }
}
