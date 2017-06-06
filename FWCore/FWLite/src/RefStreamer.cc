#include "RefStreamer.h"
#include "DataFormats/Common/interface/RefCore.h"
#include "TROOT.h"
#include <assert.h>

class TBuffer;

namespace fwlite {
  void 
  RefStreamer::operator()(TBuffer &R__b, void *objp) {
    using edm::RefCore;
    if (R__b.IsReading()) {
      cl_->ReadBuffer(R__b, objp);
      RefCore* obj = static_cast<RefCore *>(objp);
      obj->setProductGetter(prodGetter_);
      obj->setProductPtr(0);
    } else {
      cl_->WriteBuffer(R__b, objp);
    }
  }

  edm::EDProductGetter const* setRefStreamer(edm::EDProductGetter const* ep) {
    using namespace edm;
    static TClass *cl = gROOT->GetClass("edm::RefCore");
    assert(cl);
    RefStreamer *st = static_cast<RefStreamer *>(cl->GetStreamer());
    edm::EDProductGetter const* pOld = 0;
    if (st == 0) {
      cl->AdoptStreamer(new RefStreamer(ep));
    } else {
      pOld = st->setProductGetter(ep);
    }
    return pOld;
  }
}
