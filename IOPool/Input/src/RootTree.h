#ifndef IOPool_Input_RootTree_h
#define IOPool_Input_RootTree_h

/*----------------------------------------------------------------------

RootTree.h // used by ROOT input sources

----------------------------------------------------------------------*/

#include <memory>
#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"
#include "boost/utility.hpp"

#include "Inputfwd.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "DataFormats/Provenance/interface/ProvenanceFwd.h"
#include "DataFormats/Provenance/interface/ProductProvenance.h"
#include "DataFormats/Provenance/interface/BranchKey.h"
#include "IOPool/Input/src/BranchMapperWithReader.h"
#include "TBranch.h"
#include "TTree.h"
class TFile;

namespace edm {

  class RootTree : private boost::noncopyable {
  public:
    typedef input::BranchMap BranchMap;
    typedef input::EntryNumber EntryNumber;
    RootTree(boost::shared_ptr<TFile> filePtr, BranchType const& branchType);
    ~RootTree() {}
    
    bool isValid() const;
    void addBranch(BranchKey const& key,
		   BranchDescription const& prod,
		   std::string const& oldBranchName);
    void dropBranch(std::string const& oldBranchName);
    void setPresence(BranchDescription const& prod);
    bool next() {return ++entryNumber_ < entries_;} 
    bool previous() {return --entryNumber_ >= 0;} 
    bool current() {return entryNumber_ < entries_ && entryNumber_ >= 0;} 
    void rewind() {entryNumber_ = 0;} 
    EntryNumber const& entryNumber() const {return entryNumber_;}
    EntryNumber const& entries() const {return entries_;}
    void setEntryNumber(EntryNumber theEntryNumber);
    std::vector<std::string> const& branchNames() const {return branchNames_;}
    template <typename T>
    void fillGroups(T& item);
    boost::shared_ptr<DelayedReader> makeDelayedReader(FileFormatVersion const& fileFormatVersion) const;
    //TBranch *auxBranch() {return auxBranch_;}
    template <typename T>
    void fillAux(T *& pAux) const {
      auxBranch_->SetAddress(&pAux);
      input::getEntry(auxBranch_, entryNumber_);
    }
    TTree const* tree() const {return tree_;}
    TTree const* metaTree() const {return metaTree_;}
    void setCacheSize(unsigned int cacheSize) const;
    void setTreeMaxVirtualSize(int treeMaxVirtualSize);
    BranchMap const& branches() const {return *branches_;}
    std::vector<ProductStatus> const& productStatuses() const {return productStatuses_;} // backward compatibility

    // below for backward compatibility
    void fillStatus() { // backward compatibility
      statusBranch_->SetAddress(&pProductStatuses_); // backward compatibility
      input::getEntry(statusBranch_, entryNumber_); // backward compatibility
    } // backward compatibility

    TBranch *const branchEntryInfoBranch() const {return branchEntryInfoBranch_;}

  private:
    boost::shared_ptr<TFile> filePtr_;
// We use bare pointers for pointers to some ROOT entities.
// Root owns them and uses bare pointers internally.
// Therefore,using smart pointers here will do no good.
    TTree *const tree_;
    TTree *const metaTree_;
    BranchType branchType_;
    TBranch *const auxBranch_;
    TBranch *const branchEntryInfoBranch_;
    EntryNumber entries_;
    EntryNumber entryNumber_;
    std::vector<std::string> branchNames_;
    boost::shared_ptr<BranchMap> branches_;

    // below for backward compatibility
    std::vector<ProductStatus> productStatuses_; // backward compatibility
    std::vector<ProductStatus>* pProductStatuses_; // backward compatibility
    TTree *const infoTree_; // backward compatibility
    TBranch *const statusBranch_; // backward compatibility
  };

  template <typename T>
  void
  RootTree::fillGroups(T& item) {
    if (metaTree_ == 0 || metaTree_->GetNbranches() == 0) return;
    // Loop over provenance
    for (BranchMap::const_iterator pit = branches_->begin(), pitEnd = branches_->end(); pit != pitEnd; ++pit) {
      item.addGroup(pit->second.branchDescription_);
    }
  }
}
#endif
