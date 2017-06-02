#ifndef ParameterSet_ProcessDesc_h
#define ParameterSet_ProcessDesc_h

#include "boost/shared_ptr.hpp"
#include <vector>
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/WrapperNode.h"

namespace edm
{
  
  typedef boost::shared_ptr<pset::WrapperNode> WrapperNodePtr;
  class ScheduleValidator;

  class ProcessDesc
  {

  public:
    /// This class was previously just a dumb structure,
    /// so keep a default ctor in case we have to roll back
    ProcessDesc() {}

    /// construct from the configuration language string
    explicit ProcessDesc(const std::string& config);

    ~ProcessDesc();

    /// get the ParameterSet that describes the process
    boost::shared_ptr<ParameterSet> getProcessPSet() const;

    /// get the dependencies for this module
    /** the return string is a list of comma-separated
      * names of the modules on which modulename depends*/
    std::string  getDependencies(const std::string& modulename);

    /// get the descriptions of the services
    /// OBSOLETE
    boost::shared_ptr<std::vector<ParameterSet> > getServicesPSets() const;


    //Path and sequence information
    typedef std::vector< WrapperNodePtr > PathContainer;
    PathContainer pathFragments() const {return pathFragments_;}
    void addPathFragment(const WrapperNodePtr & wn) { pathFragments_.push_back(wn);}


    /// makes an entry in the bookkeeping table under this index
    void record(const std::string & index, const std::string & name);

    /// puts bookkeeping information into the ParameterSet
    void writeBookkeeping(const std::string & index);

  private:

    typedef std::vector<std::string> Strs;
    typedef std::map<std::string, edm::WrapperNodePtr > SeqMap;
    typedef boost::shared_ptr<pset::Node> NodePtr;

    /// recursively extract names of modules and store them in Strs;
    void getNames(const pset::Node* n, Strs& out);

    /// perform sequence substitution for this node
    void sequenceSubstitution(NodePtr& node, SeqMap&  sequences);

    /// Take a path Wrapper node and extract names
    /** put the name of this path in @param paths
     *  and put the names of the modules for this path in @param out */
    void fillPath(WrapperNodePtr n, Strs& paths);

    /// diagnostic function
    void dumpTree(NodePtr& node);

    /// the validation object
    ScheduleValidator*  validator_;


    //Path and sequence information
    PathContainer pathFragments_;
    boost::shared_ptr<ParameterSet> pset_;
    boost::shared_ptr<std::vector< ParameterSet> > services_;

    typedef std::map<std::string, std::vector<std::string> > Bookkeeping;
    Bookkeeping bookkeeping_;
  };
}

#endif
