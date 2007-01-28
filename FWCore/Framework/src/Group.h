#ifndef Framework_Group_h
#define Framework_Group_h

/*----------------------------------------------------------------------
  
Group: A collection of information related to a single EDProduct. This
is the storage unit of such information.

$Id: Group.h,v 1.15 2007/01/11 23:39:20 paterno Exp $

----------------------------------------------------------------------*/

#include <memory>

#include "DataFormats/Common/interface/EDProduct.h"
#include "DataFormats/Common/interface/Provenance.h"

namespace edm {
  class Group {
  public:
    explicit Group(std::auto_ptr<Provenance> prov,
 	  bool acc = true, bool onDemand = false);

    Group(std::auto_ptr<EDProduct> edp,
	  std::auto_ptr<Provenance> prov,
	  bool acc = true);

    ~Group();

    void swap(Group& other);

    Provenance& provenance() { return *provenance_; }
    
    // drop (hide) on input: means hide, but provenance is available
    // drop (write) on output: choose not to write, output module
    //   still chooses on a per product to include or exclude
    //   the provenance

    // these and not hidden and data will be populated if retrieved
    bool productAvailable() const;

    // provenance is currently available
    bool provenanceAvailable() const;

    bool onDemand() const { return onDemand_; }

    EDProduct const* product() const { return product_; }

    Provenance const& provenance() const { return *provenance_; }

    BranchDescription const& productDescription() const { return provenance_->product; }

    std::string const& moduleLabel() const 
    { return provenance_->moduleLabel(); }

    std::string const& productInstanceName() const 
    { return provenance_->productInstanceName(); }

    std::string const& processName() const
    { return provenance_->processName(); }

    // The following is const because we can add an EDProduct to the
    // cache after creation of the Group, without changing the meaning
    // of the Group.
    void setProduct(std::auto_ptr<EDProduct> prod) const;

    // Write the group to the stream.
    void write(std::ostream& os) const;

  private:
    Group(const Group&);
    void operator=(const Group&);

    mutable EDProduct* product_;
    Provenance*        provenance_;
    bool               accessible_;
    bool               onDemand_;
  };

  // Free swap function
  inline
  void
  swap(Group& a, Group& b) {
    a.swap(b);
  }

  inline
  std::ostream&
  operator<<(std::ostream& os, Group const& g) {
    g.write(os);
    return os;
  }

}
#endif
