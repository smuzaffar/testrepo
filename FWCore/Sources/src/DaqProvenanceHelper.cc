#include <algorithm>
#include <vector>

#include "FWCore/Sources/interface/DaqProvenanceHelper.h"

#include "DataFormats/Provenance/interface/BranchChildren.h"
#include "DataFormats/Provenance/interface/BranchIDList.h"
#include "DataFormats/Provenance/interface/ProcessConfigurationRegistry.h"
#include "DataFormats/Provenance/interface/ProcessHistory.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/GetPassID.h"
#include "FWCore/Version/interface/GetReleaseVersion.h"

namespace edm {
  DaqProvenanceHelper::DaqProvenanceHelper(TypeID const& rawDataType)
        : constBranchDescription_(BranchDescription(InEvent
                                                  , "rawDataCollector"
                                                  //, "source"
                                                  , "LHC"
                                                  // , "HLT"
                                                  , "FEDRawDataCollection"
                                                  , "FEDRawDataCollection"
                                                  , ""
                                                  , "DaqSource"
                                                  , ParameterSetID()
                                                  , rawDataType
                                                  , false))
        , dummyProvenance_(constBranchDescription_.branchID())
        , processParameterSet_()
        , oldProcessName_()
        , oldBranchID_()
        , newBranchID_()
        , oldProcessHistoryID_(0)
        , processConfiguration_()
        , phidMap_() {
    
    // Now we create a process parameter set for the "LHC" process.
    // We don't currently use the untracked parameters, However, we make them available, just in case.
    std::string const& moduleLabel = constBranchDescription_.moduleLabel();
    std::string const& processName = constBranchDescription_.processName();
    std::string const& moduleName = constBranchDescription_.moduleName();
    typedef std::vector<std::string> vstring;
    vstring empty;

    vstring modlbl;
    modlbl.reserve(1);
    modlbl.push_back(moduleLabel);
    processParameterSet_.addParameter("@all_sources", modlbl);

    ParameterSet triggerPaths;
    triggerPaths.addParameter<vstring>("@trigger_paths", empty);
    processParameterSet_.addParameter<ParameterSet>("@trigger_paths", triggerPaths);

    ParameterSet pseudoInput;
    pseudoInput.addParameter<std::string>("@module_edm_type", "Source");
    pseudoInput.addParameter<std::string>("@module_label", moduleLabel);
    pseudoInput.addParameter<std::string>("@module_type", moduleName);
    processParameterSet_.addParameter<ParameterSet>(moduleLabel, pseudoInput);

    processParameterSet_.addParameter<vstring>("@all_esmodules", empty);
    processParameterSet_.addParameter<vstring>("@all_esprefers", empty);
    processParameterSet_.addParameter<vstring>("@all_essources", empty);
    processParameterSet_.addParameter<vstring>("@all_loopers", empty);
    processParameterSet_.addParameter<vstring>("@all_modules", empty);
    processParameterSet_.addParameter<vstring>("@end_paths", empty);
    processParameterSet_.addParameter<vstring>("@paths", empty);
    processParameterSet_.addParameter<std::string>("@process_name", processName);
    // Now we register the process parameter set.
    processParameterSet_.registerIt();

    //std::cerr << processParameterSet_.dump() << std::endl;
  }

  ProcessHistoryID
  DaqProvenanceHelper::daqInit(ProductRegistry& productRegistry) const {
    // Now we need to set all the metadata
    // Add the product to the product registry  
    productRegistry.copyProduct(constBranchDescription_.me());

    // Insert an entry for this process in the process configuration registry
    ProcessConfiguration pc(constBranchDescription_.processName(), processParameterSet_.id(), getReleaseVersion(), getPassID());
    ProcessConfigurationRegistry::instance()->insertMapped(pc);

    // Insert an entry for this process in the process history registry
    ProcessHistory ph;
    ph.push_back(pc);
    ProcessHistoryRegistry::instance()->insertMapped(ph);

    // Save the process history ID for use every event.
    return ph.id();
  }

  void
  DaqProvenanceHelper::fixMetaData(std::vector<ProcessConfiguration>& pcv) {
    bool found = false;
    for(std::vector<ProcessConfiguration>::const_iterator it = pcv.begin(), itEnd = pcv.end(); it != itEnd; ++it) {
       if(it->processName() == oldProcessName_) {
         processConfiguration_ = ProcessConfiguration(constBranchDescription_.processName(),
                                                      processParameterSet_.id(),
                                                      it->releaseVersion(), it->passID());
         pcv.push_back(processConfiguration_);
         found = true;
         break;
       }
    }
    assert(found);
  }

  void
  DaqProvenanceHelper::fixMetaData(std::vector<ProcessHistory>& phv) {
    phv.reserve(phv.size() + 1);
    phv.push_back(ProcessHistory()); // For new processHistory, containing only processConfiguration_.
    for(std::vector<ProcessHistory>::iterator it = phv.begin(), itEnd = phv.end(); it != itEnd; ++it) {
      ProcessHistoryID oldPHID = it->id();
      it->push_front(processConfiguration_);
      ProcessHistoryID newPHID = it->id();
      phidMap_.insert(std::make_pair(oldPHID, newPHID));
    }
  }

  void
  DaqProvenanceHelper::fixMetaData(std::vector<BranchID>& branchID) const {
    std::replace(branchID.begin(), branchID.end(), oldBranchID_, newBranchID_);
  }

  void
  DaqProvenanceHelper::fixMetaData(BranchIDLists const& branchIDLists) const {
    BranchID::value_type oldID = oldBranchID_.id();
    BranchID::value_type newID = newBranchID_.id();
    // The const_cast is ugly, but it beats the alternatives.
    BranchIDLists& lists = const_cast<BranchIDLists&>(branchIDLists);
    for(BranchIDLists::iterator it = lists.begin(), itEnd = lists.end(); it != itEnd; ++it) {
      std::replace(it->begin(), it->end(), oldID, newID);
    }
  }

  void
  DaqProvenanceHelper::fixMetaData(BranchChildren& branchChildren) const {
    typedef std::map<BranchID, std::set<BranchID> > BCMap;
    // The const_cast is ugly, but it beats the alternatives.
    BCMap& childLookup = const_cast<BCMap&>(branchChildren.childLookup());
    // First fix any old branchID's in the key.
    {
      BCMap::iterator i = childLookup.find(oldBranchID_);
      if(i != childLookup.end()) {
        childLookup.insert(std::make_pair(newBranchID_, i->second));
        childLookup.erase(i);
      }
    }
    // Now fix any old branchID's in the sets;
    for(BCMap::iterator it = childLookup.begin(), itEnd = childLookup.end(); it != itEnd; ++it) {
      if(it->second.erase(oldBranchID_) != 0) {
        it->second.insert(newBranchID_);
      }
    }
  }

  // Replace process history ID.
  ProcessHistoryID const&
  DaqProvenanceHelper::mapProcessHistoryID(ProcessHistoryID const& phid) {
    ProcessHistoryIDMap::const_iterator it = phidMap_.find(phid);
    assert(it != phidMap_.end());
    oldProcessHistoryID_ = &it->first;
    return it->second;
  }

  // Replace parentage ID.
  ParentageID const&
  DaqProvenanceHelper::mapParentageID(ParentageID const& parentageID) const {
    ParentageIDMap::const_iterator it = parentageIDMap_.find(parentageID);
    if(it == parentageIDMap_.end()) {
      return parentageID;
    }
    return it->second;
  }

  // Replace branch ID if necessary.
  BranchID const&
  DaqProvenanceHelper::mapBranchID(BranchID const& branchID) const {
    return(branchID == oldBranchID_ ? newBranchID_ : branchID);
  }
}
