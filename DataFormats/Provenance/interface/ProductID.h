#ifndef DataFormats_Provenance_ProductID_h
#define DataFormats_Provenance_ProductID_h

/*----------------------------------------------------------------------
  
ProductID: A unique identifier for each EDProduct within a process.

$Id: ProductID.h,v 1.1 2007/03/04 04:48:09 wmtan Exp $

----------------------------------------------------------------------*/

#include <iosfwd>

namespace edm {
  struct ProductID {
    ProductID() : id_(0) {}
    explicit ProductID(unsigned int id) : id_(id) {}
    bool isValid() const { return id_ != 0U; }
    unsigned int id() const { return id_; }
    unsigned int id_;
    bool operator<(ProductID const& rh) const {return id_ < rh.id_;}
    bool operator>(ProductID const& rh) const {return id_ > rh.id_;}
    bool operator==(ProductID const& rh) const {return id_ == rh.id_;}
    bool operator!=(ProductID const& rh) const {return id_ != rh.id_;}
  };

  std::ostream&
  operator<<(std::ostream& os, ProductID const& id);
}
#endif
