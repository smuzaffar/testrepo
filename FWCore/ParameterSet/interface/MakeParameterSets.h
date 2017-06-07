#ifndef FWCoreParameterSet_MakeParameterSets_h
#define FWCoreParameterSet_MakeParameterSets_h


//----------------------------------------------------------------------
// Declare functions used to create ParameterSets.
//
// $Id: MakeParameterSets.h,v 1.8 2008/10/31 23:17:13 rpw Exp $
//
//----------------------------------------------------------------------

#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"

#include "FWCore/ParameterSet/interface/ProcessDesc.h"

namespace edm
{
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


  // deprecated
  boost::shared_ptr<edm::ProcessDesc>
  readConfigFile(const std::string & fileName) {return readConfig(fileName);}


} // namespace edm

#endif
