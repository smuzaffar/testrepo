#ifndef FWCore_Sources_ExternalInputSource_h
#define FWCore_Sources_ExternalInputSource_h

/*----------------------------------------------------------------------
$Id: ExternalInputSource.h,v 1.3 2007/08/06 19:53:06 wmtan Exp $
----------------------------------------------------------------------*/

#include <memory>
#include <vector>
#include <string>

#include "FWCore/Framework/interface/ConfigurableInputSource.h"
#include "FWCore/Catalog/interface/InputFileCatalog.h"

namespace edm {
  class ExternalInputSource : public ConfigurableInputSource {
  public:
    explicit ExternalInputSource(ParameterSet const& pset, InputSourceDescription const& desc);
    virtual ~ExternalInputSource();

  std::vector<std::string> const& logicalFileNames() const {return catalog_.logicalFileNames();}
  std::vector<std::string> const& fileNames() const {return catalog_.fileNames();}
  InputFileCatalog& catalog() {return catalog_;}


  private:
    PoolCatalog poolCatalog_;
    InputFileCatalog catalog_;
  };
}
#endif
