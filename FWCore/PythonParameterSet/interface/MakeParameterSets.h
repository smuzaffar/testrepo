#ifndef FWCore_PythonParameterSet_MakeParameterSets_h
#define FWCore_PythonParameterSet_MakeParameterSets_h

//----------------------------------------------------------------------
// Declare functions used to create ParameterSets.
//----------------------------------------------------------------------

#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"

namespace edm
{
  class ParameterSet;
  class ProcessDesc;

  // input can either be a python file name or a python config string
  boost::shared_ptr<edm::ProcessDesc>
  readConfig(const std::string & config);

  /// same, but with arguments
  boost::shared_ptr<edm::ProcessDesc>
  readConfig(const std::string & config, int argc, char * argv[]);

  /// essentially the same as the previous method
  void
  makeParameterSets(std::string const& configtext,
                  boost::shared_ptr<ParameterSet>& main,
                  boost::shared_ptr<std::vector<ParameterSet> >& serviceparams);
} // namespace edm
#endif
