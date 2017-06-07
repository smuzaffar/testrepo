#ifndef FWCore_ParameterSet_PythonProcessDesc_h
#define FWCore_ParameterSet_PythonProcessDesc_h

#include "FWCore/ParameterSet/interface/BoostPython.h"
#include "FWCore/ParameterSet/interface/PythonParameterSet.h"
#include "FWCore/ParameterSet/interface/ProcessDesc.h"

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

  static bool initialized_;
  PythonParameterSet theProcessPSet;
  std::vector<PythonParameterSet> theServices;
  boost::python::object theMainModule;
  boost::python::object theMainNamespace;
};

#endif

