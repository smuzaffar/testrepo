#include "DataFormats/Provenance/interface/BranchType.h"
#include "DataFormats/Provenance/interface/Provenance.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/ParameterSet/interface/Registry.h"

namespace edm {

    Event::Event(EventPrincipal& ep, ModuleDescription const& md) :
	provRecorder_(ep, md),
	aux_(ep.aux()),
        luminosityBlock_(new LuminosityBlock(ep.luminosityBlockPrincipal(), md)),
	gotBranchIDs_(),
	gotViews_() {
    }

    EventPrincipal &
    Event::eventPrincipal() {
      return dynamic_cast<EventPrincipal &>(provRecorder_.principal());
    }

    EventPrincipal const &
    Event::eventPrincipal() const {
      return dynamic_cast<EventPrincipal const&>(provRecorder_.principal());
    }

    ProductID
    Event::makeProductID(ConstBranchDescription const& desc) const {
      return eventPrincipal().branchIDToProductID(desc.branchID());
    }

    Run const&
    Event::getRun() const {
      return getLuminosityBlock().getRun();
    }

//   History const& 
//   Event::history() const {
//     PrincipalGetAdapter const& dvi = me();
//     EDProductGetter const* pg = dvi.prodGetter(); // certain to be non-null
//     assert(pg);
//     EventPrincipal const& ep = dynamic_cast<EventPrincipal const&>(*pg);
//     return ep.history();
//   }
  History const&
  Event::history() const {
    return eventPrincipal().history();
  }

  ProcessHistoryID const&
  Event::processHistoryID() const {
    return eventPrincipal().history().processHistoryID();
  }


  Provenance
  Event::getProvenance(BranchID const& bid) const {
    return provRecorder_.principal().getProvenance(bid);
  }

  Provenance
  Event::getProvenance(ProductID const& pid) const {
    return eventPrincipal().getProvenance(pid);
  }

  void
  Event::getAllProvenance(std::vector<Provenance const*> & provenances) const {
    provRecorder_.principal().getAllProvenance(provenances);
  }

  bool
  Event::getProcessParameterSet(std::string const& processName,
				ParameterSet& ps) const {
    // Get the ProcessHistory for this event.
    ProcessHistoryRegistry* phr = ProcessHistoryRegistry::instance();
    ProcessHistory ph;
    if (!phr->getMapped(processHistoryID(), ph)) {
      throw Exception(errors::NotFound) 
	<< "ProcessHistoryID " << processHistoryID()
	<< " is claimed to describe " << id()
	<< "\nbut is not found in the ProcessHistoryRegistry.\n"
	<< "This file is malformed.\n";
    }

    ProcessConfiguration config;
    bool process_found = ph.getConfigurationForProcess(processName, config);
    if (process_found) {
      pset::Registry::instance()->getMapped(config.parameterSetID(), ps);
      assert(!ps.empty());
    }
    return process_found;
  }

  BasicHandle
  Event::getByProductID_(ProductID const& oid) const {
    return eventPrincipal().getByProductID(oid);
  }


  void
  Event::commit_(std::vector<BranchID>* previousParentage, ParentageID* previousParentageId) {
    commit_aux(putProducts(), true,previousParentage,previousParentageId);
    commit_aux(putProductsWithoutParents(), false);
  }

  void
  Event::commit_aux(Event::ProductPtrVec& products, bool record_parents,
                    std::vector<BranchID>* previousParentage, ParentageID* previousParentageId) {
    // fill in guts of provenance here
    EventPrincipal & ep = eventPrincipal();

    ProductPtrVec::iterator pit(products.begin());
    ProductPtrVec::iterator pie(products.end());

    std::vector<BranchID> gotBranchIDVector;

    // Note that gotBranchIDVector will remain empty if
    // record_parents is false (and may be empty if record_parents is
    // true).

    //Check that previousParentageId is still good by seeing if previousParentage matches gotBranchIDs_
    bool sameAsPrevious=( (0 != previousParentage) && (previousParentage->size() == gotBranchIDs_.size()) );
    if (record_parents && !gotBranchIDs_.empty()) {
      gotBranchIDVector.reserve(gotBranchIDs_.size());
      std::vector<BranchID>::const_iterator itPrevious;
      if(previousParentage) {
        itPrevious = previousParentage->begin();
      }
      for (BranchIDSet::const_iterator it = gotBranchIDs_.begin(), itEnd = gotBranchIDs_.end();
	  it != itEnd; ++it) {
        gotBranchIDVector.push_back(*it);
        if(sameAsPrevious) {
          if(*it != *itPrevious) {
            sameAsPrevious=false;
          } else {
            ++itPrevious;
          }
        }
      }
      if(!sameAsPrevious && 0!=previousParentage) {
        previousParentage->assign(gotBranchIDVector.begin(),gotBranchIDVector.end());
      }
    }

    //If we don't have a valid previousParentage then we want to use a temp value in order to
    // avoid constantly recalculating the ParentageID which is a time consuming operation
    ParentageID temp;
    if(!previousParentage) {
      assert(!sameAsPrevious);
      previousParentageId = &temp;
    }
    while(pit!=pie) {
      // set provenance
      std::auto_ptr<ProductProvenance> productProvenancePtr;
      if(!sameAsPrevious) {
        productProvenancePtr = std::auto_ptr<ProductProvenance>(
                                                                new ProductProvenance(pit->second->branchID(),
                                                                                      productstatus::present(),
                                                                                      gotBranchIDVector));
        *previousParentageId = productProvenancePtr->parentageID();
        sameAsPrevious=true;
      } else {
        productProvenancePtr = std::auto_ptr<ProductProvenance>(
                                                                new ProductProvenance(pit->second->branchID(),
                                                                                      productstatus::present(),
                                                                                      *previousParentageId));
      }
      ep.put(*pit->second, pit->first, productProvenancePtr);
      ++pit;
    }

    // the cleanup is all or none
    products.clear();
  }

  void
  Event::addToGotBranchIDs(Provenance const& prov) const {
    gotBranchIDs_.insert(prov.branchID());
  }

  ProcessHistory const&
  Event::processHistory() const {
    return provRecorder_.processHistory();
  }
  
size_t 
  Event::size() const {
    return putProducts().size()+provRecorder_.principal().size()+putProductsWithoutParents().size();
  } 
  
}
