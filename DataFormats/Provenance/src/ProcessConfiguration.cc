#include <sstream>

#include "FWCore/Utilities/interface/Digest.h"
#include "DataFormats/Provenance/interface/ProcessConfiguration.h"
#include <ostream>

/*----------------------------------------------------------------------

$Id: ProcessConfiguration.cc,v 1.4.8.2 2008/12/18 00:17:06 wmtan Exp $

----------------------------------------------------------------------*/

namespace edm {


  ProcessConfiguration::ProcessConfiguration() : processName_(), parameterSetID_(), releaseVersion_(), passID_() {}

  ProcessConfiguration::ProcessConfiguration(
                        std::string const& procName,
                        ParameterSetID const& pSetID,
                        ReleaseVersion const& relVersion,
                        PassID const& pass) :
      processName_(procName),
      parameterSetID_(pSetID),
      releaseVersion_(relVersion),
      passID_(pass) { }


  ProcessConfigurationID
  ProcessConfiguration::id() const
  {
    // This implementation is ripe for optimization.
    std::ostringstream oss;
    oss << *this;
    std::string stringrep = oss.str();
    cms::Digest md5alg(stringrep);
    return ProcessConfigurationID(md5alg.digest().toString());
  }

  bool operator<(ProcessConfiguration const& a, ProcessConfiguration const& b) {
    if (a.processName() < b.processName()) return true;
    if (b.processName() < a.processName()) return false;
    if (a.parameterSetID() < b.parameterSetID()) return true;
    if (b.parameterSetID() < a.parameterSetID()) return false;
    if (a.releaseVersion() < b.releaseVersion()) return true;
    if (b.releaseVersion() < a.releaseVersion()) return false;
    if (a.passID() < b.passID()) return true;
    return false;
  }

  std::ostream&
  operator<< (std::ostream& os, ProcessConfiguration const& pc) {
    os << pc.processName() << ' ' 
       << pc.parameterSetID() << ' '
       << pc.releaseVersion() << ' '
       << pc.passID();
    return os;
  }
}
