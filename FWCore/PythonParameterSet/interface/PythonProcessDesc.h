#ifndef FWCore_PythonParameterSet_PythonProcessDesc_h
#define FWCore_PythonParameterSet_PythonProcessDesc_h

#include "FWCore/PythonParameterSet/interface/BoostPython.h"
#include "FWCore/PythonParameterSet/interface/PythonParameterSet.h"

#include "boost/shared_ptr.hpp"

#include <string>
#include <vector>

namespace edm {
  class ProcessDesc;
}

class PythonProcessDesc
{
public:
  PythonProcessDesc();
  /** This constructor will parse the given file or string
      and create two objects in python-land:
    * a PythonProcessDesc named 'processDesc'
    * a PythonParameterSet named 'processPSet'
    It decides whether it's a file or string by seeing if
    it ends in '.py'
  */
  PythonProcessDesc(std::string const& config);

  PythonProcessDesc(std::string const& config, int argc, char * argv[]);

  void addService(PythonParameterSet& pset) {theServices.push_back(pset);}

  PythonParameterSet newPSet() const {return PythonParameterSet();}

  std::string dump() const;

  // makes a new (copy) of the ProcessDesc
  boost::shared_ptr<edm::ProcessDesc> processDesc();

private:
  void prepareToRead();
  void read(std::string const& config);
  void readFile(std::string const& fileName);
  void readString(std::string const& pyConfig);

  PythonParameterSet theProcessPSet;
  std::vector<PythonParameterSet> theServices;
  boost::python::object theMainModule;
  boost::python::object theMainNamespace;
};

#endif
