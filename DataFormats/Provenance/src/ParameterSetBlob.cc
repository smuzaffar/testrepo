#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include <iosfwd>

namespace edm {
  std::ostream&
  operator<<(std::ostream& os, ParameterSetBlob const& blob) {
    os << blob.pset_;
    return os;
  }
}
