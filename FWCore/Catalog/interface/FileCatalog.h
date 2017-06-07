#ifndef FWCore_Catalog_FileCatalog_h
#define FWCore_Catalog_FileCatalog_h
//////////////////////////////////////////////////////////////////////
//
// Class FileCatalog. Common services to manage File catalog
//
//////////////////////////////////////////////////////////////////////

#include "FWCore/Catalog/interface/FileLocator.h"
#include <string>

namespace edm {

  class FileCatalogItem {
  public:
    FileCatalogItem() : pfn_(), lfn_() {}
    FileCatalogItem(std::string const& pfn, std::string const& lfn) : pfn_(pfn), lfn_(lfn) {}
    std::string const& fileName() const {return pfn_;}
    std::string const& logicalFileName() const {return lfn_;}
  private:
    std::string pfn_;
    std::string lfn_;
  };

  class FileCatalog {
  public:
    explicit FileCatalog();
    virtual ~FileCatalog() = 0;
    FileLocator const & fileLocator() const { return fl;}
    static bool isPhysical(std::string const& name) {
      return (name.empty() || name.find(':') != std::string::npos);
    }
  private:
    FileLocator fl;
  };

}

#endif
