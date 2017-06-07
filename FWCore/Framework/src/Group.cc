/*----------------------------------------------------------------------
----------------------------------------------------------------------*/
#include <string>
#include "DataFormats/Provenance/interface/ProductStatus.h"
#include "FWCore/Framework/interface/Group.h"
#include "FWCore/Utilities/interface/ReflexTools.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

using ROOT::Reflex::Type;
using ROOT::Reflex::TypeTemplate;

namespace edm {

  Group::Group() :
    product_(),
    branchDescription_(),
    entryInfo_(),
    prov_(),
    dropped_(false),
    onDemand_(false) {}

  Group::Group(std::auto_ptr<EDProduct> edp, ConstBranchDescription const& bd, std::auto_ptr<EventEntryInfo> entryInfo) :
    product_(edp.release()),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(entryInfo.release()),
    prov_(new Provenance(*branchDescription_, entryInfo_)),
    dropped_(!branchDescription_->present()),
    onDemand_(false) {
  }

  Group::Group(ConstBranchDescription const& bd, std::auto_ptr<EventEntryInfo> entryInfo) :
    product_(),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(entryInfo.release()),
    prov_(new Provenance(*branchDescription_, entryInfo_)),
    dropped_(!branchDescription_->present()),
    onDemand_(false) {
  }

  Group::Group(std::auto_ptr<EDProduct> edp, ConstBranchDescription const& bd, boost::shared_ptr<EventEntryInfo> entryInfo) :
    product_(edp.release()),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(entryInfo),
    prov_(new Provenance(*branchDescription_, entryInfo_)),
    dropped_(!branchDescription_->present()),
    onDemand_(false) {
  }

  Group::Group(ConstBranchDescription const& bd, boost::shared_ptr<EventEntryInfo> entryInfo) :
    product_(),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(entryInfo),
    prov_(new Provenance(*branchDescription_, entryInfo_)),
    dropped_(!branchDescription_->present()),
    onDemand_(false) {
  }

  Group::Group(ConstBranchDescription const& bd, bool demand) :
    product_(),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(),
    prov_(),
    dropped_(!branchDescription_->present()),
    onDemand_(demand) {
  }

  Group::Group(ConstBranchDescription const& bd) :
    product_(),
    branchDescription_(new ConstBranchDescription(bd)),
    entryInfo_(),
    prov_(),
    dropped_(!branchDescription_->present()),
    onDemand_(false) {
  }

  Group::~Group() {
  }

  ProductStatus
  Group::status() const {
    if (dropped_) return productstatus::dropped();
    if (!entryInfo_) {
      if (product_) return product_->isPresent() ? productstatus::present() : productstatus::neverCreated();
      else return productstatus::unknown();
    }
    if (product_) {
      // for backward compatibility
      product_->isPresent() ? entryInfo_->setPresent() : entryInfo_->setNotPresent();
    }
    return entryInfo_->productStatus();
  }

  bool
  Group::onDemand() const {
    return onDemand_;
  }

  bool 
  Group::productUnavailable() const { 
    if (onDemand()) return false;
    if (dropped_) return true;
    if (productstatus::unknown(status())) return false;
    return not productstatus::present(status());

  }

  bool 
  Group::provenanceAvailable() const { 
    if (onDemand()) return false;
    return true;    
  }

  void 
  Group::setProduct(std::auto_ptr<EDProduct> prod) const {
    assert (product() == 0);
    product_.reset(prod.release());  // Group takes ownership
  }
  
  void 
  Group::setProvenance(boost::shared_ptr<EventEntryInfo> entryInfo) const {
    entryInfo_ = entryInfo;  // Group takes ownership
    if (entryInfo_) {
      prov_.reset(new Provenance(*branchDescription_, entryInfo_));
    } else {
      prov_.reset(new Provenance(*branchDescription_));
    }
  }

  void  
  Group::swap(Group& other) {
    std::swap(product_, other.product_);
    std::swap(branchDescription_, other.branchDescription_);
    std::swap(entryInfo_, other.entryInfo_);
    std::swap(prov_, other.prov_);
    std::swap(dropped_, other.dropped_);
    std::swap(onDemand_, other.onDemand_);
  }

  void
  Group::replace(Group& g) {
    this->swap(g);
  }

  Type
  Group::productType() const
  {
    return Type::ByTypeInfo(typeid(*product()));
  }

  bool
  Group::isMatchingSequence(Type const& wantedElementType) const
  {
    Type value_type;
    bool is_sequence = is_sequence_wrapper(productType(), value_type);
        
    // If our product is not a sequence, we can't match...
    if (!is_sequence) return false;

    Type elementType = value_type; // this is not true for RefVector...

    TypeTemplate valueTypeTemplate = value_type.TemplateFamily();

    return 
      is_sequence 
      ? (elementType==wantedElementType || 
	 elementType.HasBase(wantedElementType))
      : false;      
  }

  Provenance const *
  Group::provenance() const {
    if (!entryInfo_) {
      prov_.reset(new Provenance(*branchDescription_, entryInfo_));
    }
    return prov_.get();
  }

  void
  Group::write(std::ostream& os) const 
  {
    // This is grossly inadequate. It is also not critical for the
    // first pass.
    os << std::string("Group for product with ID: ")
       << entryInfo_->productID();
  }

  void
  Group::mergeGroup(Group * newGroup) {

    if (status() != newGroup->status()) {
      throw edm::Exception(edm::errors::Unknown, "Merging")
        << "Group::mergeGroup(), Trying to merge two run products or two lumi products.\n"
        << "The products have different creation status's.\n"
        << "For example \"present\" and \"notCreated\"\n"
        << "The Framework does not currently support this and probably\n"
        << "should not support this case.\n"
        << "Likely a problem exists in the producer that created these\n"
        << "products.  If this problem cannot be reasonably resolved by\n"
        << "fixing the producer module, then contact the Framework development\n"
        << "group with details so we can discuss whether and how to support this\n"
        << "use case.\n"
        << "className = " << branchDescription_->className() << "\n"
        << "moduleLabel = " << moduleLabel() << "\n"
        << "instance = " << productInstanceName() << "\n"
        << "process = " << processName() << "\n";
    }

    if (!entryInfo_) {
      return;
    }

    // Don't support specifying multple modules.  So just null the description
    // if they are different.    

    if (entryInfo_->moduleDescriptionID() != newGroup->entryInfo_->moduleDescriptionID()) {
      entryInfo_->setModuleDescriptionID(ModuleDescriptionID());
    }

    if (!productUnavailable() && !newGroup->productUnavailable()) {

      if (product_->isMergeable()) {
        product_->mergeProduct(newGroup->product_.get());
      }
      else if (product_->hasIsProductEqual()) {

        if (!product_->isProductEqual(newGroup->product_.get())) {
          edm::LogWarning  ("RunLumiMerging") 
            << "Group::mergeGroup\n" 
            << "Two run/lumi products for the same run/lumi which should be equal are not\n"
            << "Using the first, ignoring the second\n"
            << "className = " << branchDescription_->className() << "\n"
            << "moduleLabel = " << moduleLabel() << "\n"
            << "instance = " << productInstanceName() << "\n"
            << "process = " << processName() << "\n";
        }
      }
      else {
        edm::LogWarning  ("RunLumiMerging") 
          << "Group::mergeGroup\n" 
          << "Run/lumi product has neither a mergeProduct nor isProductEqual function\n"
          << "Using the first, ignoring the second in merge\n"
          << "className = " << branchDescription_->className() << "\n"
          << "moduleLabel = " << moduleLabel() << "\n"
          << "instance = " << productInstanceName() << "\n"
          << "process = " << processName() << "\n";
      }
    }
  }
}
