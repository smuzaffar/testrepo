/*----------------------------------------------------------------------
$Id: DataViewImpl.cc,v 1.21 2007/07/09 07:29:51 llista Exp $
----------------------------------------------------------------------*/

#include <algorithm>

#include "FWCore/Framework/interface/DataViewImpl.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "FWCore/Framework/interface/Principal.h"
#include "DataFormats/Provenance/interface/BranchEntryDescription.h"
#include "FWCore/Framework/interface/Selector.h"

namespace edm {

  DataViewImpl::DataViewImpl(Principal & dbk,
	ModuleDescription const& md,
	BranchType const& branchType)  :
    put_products_(),
    gotProductIDs_(),
    dbk_(dbk),
    md_(md),
    branchType_(branchType)
  {  }

  struct deleter {
    void operator()(std::pair<EDProduct*, ConstBranchDescription const*> const p) const { delete p.first; }
  };

  DataViewImpl::~DataViewImpl() {
    // anything left here must be the result of a failure
    // let's record them as failed attempts in the event principal
    for_each(put_products_.begin(),put_products_.end(),deleter());
  }

  size_t
  DataViewImpl::size() const {
    return put_products_.size() + dbk_.size();
  }

  void 
  DataViewImpl::commit_() {
    // fill in guts of provenance here
    ProductPtrVec::iterator pit(put_products_.begin());
    ProductPtrVec::iterator pie(put_products_.end());

    while(pit!=pie) {
	std::auto_ptr<EDProduct> pr(pit->first);
	// note: ownership has been passed - so clear the pointer!
	pit->first = 0;

	boost::shared_ptr<BranchEntryDescription> event(new BranchEntryDescription(pit->second->productID(), BranchEntryDescription::Success));

	// set parts of provenance
	event->cid_ = 0; // TODO: what is this supposed to be?
	event->isPresent_ = true;
	event->parents_ = gotProductIDs_;
	event->moduleDescriptionID_ = pit->second->moduleDescriptionID();
	std::auto_ptr<Provenance> pv(new Provenance(*pit->second, event));
	dbk_.put(pr,pv);
	++pit;
    }

    // the cleanup is all or none
    put_products_.clear();
  }

  BasicHandle
  DataViewImpl::get_(ProductID const& oid) const
  {
    return dbk_.get(oid);
  }

  BasicHandle
  DataViewImpl::get_(TypeID const& tid, SelectorBase const& sel) const
  {
    return dbk_.getBySelector(tid, sel);
  }
    
  BasicHandle
  DataViewImpl::getByLabel_(TypeID const& tid,
		     std::string const& label,
                     std::string const& productInstanceName) const
  {
    return dbk_.getByLabel(tid, label, productInstanceName);
  }

  void 
  DataViewImpl::getMany_(TypeID const& tid, 
		  SelectorBase const& sel,
		  BasicHandleVec& results) const
  {
    dbk_.getMany(tid, sel, results);
  }

  BasicHandle
  DataViewImpl::getByLabel_(TypeID const& tid,
                     std::string const& label,
  	             std::string const& productInstanceName,
  	             std::string const& processName) const
  {
    return dbk_.getByLabel(tid, label, productInstanceName, processName);
  }

  BasicHandle
  DataViewImpl::getByType_(TypeID const& tid) const
  {
    return dbk_.getByType(tid);
  }

  void 
  DataViewImpl::getManyByType_(TypeID const& tid, 
		  BasicHandleVec& results) const
  {
    dbk_.getManyByType(tid, results);
  }

  int 
  DataViewImpl::getMatchingSequence_(TypeID const& typeID,
                                     SelectorBase const& selector,
                                     BasicHandleVec& results,
                                     bool stopIfProcessHasMatch) const
  {
    return dbk_.getMatchingSequence(typeID,
                                    selector,
                                    results,
                                    stopIfProcessHasMatch);
  }

  int 
  DataViewImpl::getMatchingSequenceByLabel_(TypeID const& typeID,
                                            std::string const& label,
                                            std::string const& productInstanceName,
                                            BasicHandleVec& results,
                                            bool stopIfProcessHasMatch) const
{
  edm::Selector sel(edm::ModuleLabelSelector(label) &&
                    edm::ProductInstanceNameSelector(productInstanceName));
  
  int n = dbk_.getMatchingSequence(typeID,
				   sel,
				   results,
				   stopIfProcessHasMatch);
  return n;
}

int 
DataViewImpl::getMatchingSequenceByLabel_(TypeID const& typeID,
                                          std::string const& label,
                                          std::string const& productInstanceName,
                                          std::string const& processName,
                                          BasicHandleVec& results,
                                          bool stopIfProcessHasMatch) const
{
  edm::Selector sel(edm::ModuleLabelSelector(label) &&
                    edm::ProductInstanceNameSelector(productInstanceName) &&
                    edm::ProcessNameSelector(processName) );
  
  int n = dbk_.getMatchingSequence(typeID,
				   sel,
				   results,
				   stopIfProcessHasMatch);
  return n;
}

  Provenance const&
  DataViewImpl::getProvenance(ProductID const& oid) const
  {
    return dbk_.getProvenance(oid);
  }

  void
  DataViewImpl::getAllProvenance(std::vector<Provenance const*> & provenances) const
  {
    dbk_.getAllProvenance(provenances);
  }

  ProcessHistory const&
  DataViewImpl::processHistory() const
  {
    return dbk_.processHistory();
  }

  ConstBranchDescription const&
  DataViewImpl::getBranchDescription(TypeID const& type,
				     std::string const& productInstanceName) const {
    std::string friendlyClassName = type.friendlyClassName();
        BranchKey bk(friendlyClassName, md_.moduleLabel(), productInstanceName, md_.processName());
    ProductRegistry::ConstProductList const& pl = dbk_.productRegistry().constProductList();
    ProductRegistry::ConstProductList::const_iterator it = pl.find(bk);
    if (it == pl.end()) {
      throw edm::Exception(edm::errors::InsertFailure)
	<< "Illegal attempt to 'put' an unregistered product.\n"
	<< "No product is registered for\n"
	<< "  process name:                '" << bk.processName_ << "'\n"
	<< "  module label:                '" << bk.moduleLabel_ << "'\n"
	<< "  product friendly class name: '" << bk.friendlyClassName_ << "'\n"
	<< "  product instance name:       '" << bk.productInstanceName_ << "'\n"

	<< "The ProductRegistry contains:\n"
	<< dbk_.productRegistry()
	<< '\n';	  
    }
    if(it->second.branchType() != branchType_) {
        throw edm::Exception(edm::errors::InsertFailure,"Not Registered")
          << "put: Problem found while adding product. "
          << "The product for ("
          << bk.friendlyClassName_ << ","
          << bk.moduleLabel_ << ","
          << bk.productInstanceName_ << ","
          << bk.processName_
          << ")\n"
          << "is registered for a(n) " << it->second.branchType()
          << " instead of for a(n) " << branchType_
          << ".\n";
    }
    return it->second;
  }

  EDProductGetter const*
  DataViewImpl::prodGetter() const{
    return dbk_.prodGetter();
  }
}
