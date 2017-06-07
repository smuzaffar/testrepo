#ifndef DataFormats_Provenance_ConstBranchDescription_h
#define DataFormats_Provenance_ConstBranchDescription_h

/*----------------------------------------------------------------------
  
ConstBranchDescription: A class containing a constant shareable branch description
that is inexpensive to copy.
This class is not persistable.

----------------------------------------------------------------------*/
#include <iosfwd>
#include <string>
#include <set>

#include "boost/shared_ptr.hpp"

#include "DataFormats/Provenance/interface/BranchDescription.h"

#include "Reflex/Type.h"

/*
  ConstBranchDescription

*/

namespace edm {
  class ConstBranchDescription {
  public:
    explicit ConstBranchDescription(BranchDescription const& bd) :
      ptr_(new BranchDescription(bd)) {}

    void init() const {ptr_->init();}

    void write(std::ostream& os) const {ptr_->write(os);}

    std::string const& moduleLabel() const {return ptr_->moduleLabel();}
    std::string const& processName() const {return ptr_->processName();}
    BranchID const& branchID() const {return ptr_->branchID();}
    ProductID const& productIDtoAssign() const {return ptr_->productIDtoAssign();}
    std::string const& fullClassName() const {return ptr_->fullClassName();}
    std::string const& className() const {return ptr_->fullClassName();}
    std::string const& friendlyClassName() const {return ptr_->friendlyClassName();}
    std::string const& productInstanceName() const {return ptr_->productInstanceName();} 
    bool const& produced() const {return ptr_->produced();}
    bool const& present() const {return ptr_->present();}
    bool const& transient() const {return ptr_->transient();}
    Reflex::Type const& type() const {return ptr_->type();}
    int const& splitLevel() const {return ptr_->splitLevel();}
    int const& basketSize() const {return ptr_->basketSize();}

    ModuleDescriptionID const& moduleDescriptionID() const {return ptr_->moduleDescriptionID();}
    std::set<ParameterSetID> const& psetIDs() const {return ptr_->psetIDs();}
    ParameterSetID const& psetID() const {return ptr_->psetID();}
    bool isPsetIDUnique() const {return ptr_->psetIDs().size() == 1;}
    std::set<ProcessConfigurationID> const& processConfigurationIDs() const {return ptr_->processConfigurationIDs();}
    std::set<std::string> const& branchAliases() const {return ptr_->branchAliases();}
    std::string const& branchName() const {return ptr_->branchName();}
    BranchType const& branchType() const {return ptr_->branchType();}
    std::string const& wrappedName() const {return ptr_->wrappedName();}

    BranchDescription const& me() const {return *ptr_;}

  private:
    boost::shared_ptr<BranchDescription> ptr_;
  };
  
  inline
  std::ostream&
  operator<<(std::ostream& os, ConstBranchDescription const& p) {
    os << p.me();   
    return os;
  }

  inline
  bool operator<(ConstBranchDescription const& a, ConstBranchDescription const& b) {
    return a.me() < b.me();
  }

  inline
  bool operator==(ConstBranchDescription const& a, ConstBranchDescription const& b) {
    return a.me() == b.me();
  }

  inline
  std::string match(ConstBranchDescription const& a,
	ConstBranchDescription const& b,
	std::string const& fileName,
	BranchDescription::MatchMode m) {
    return match(a.me(), b.me(), fileName, m);
  }
}
#endif
