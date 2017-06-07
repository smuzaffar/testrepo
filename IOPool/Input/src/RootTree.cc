#include "RootTree.h"
#include "RootDelayedReader.h"
#include "FWCore/Framework/interface/Principal.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/Provenance/interface/BranchDescription.h"
#include "DataFormats/Provenance/interface/ConstBranchDescription.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeIndex.h"
#include "TTreeCache.h"

#include <iostream>

namespace edm {
  namespace {
    TBranch* getAuxiliaryBranch(TTree* tree, BranchType const& branchType) {
      TBranch* branch = tree->GetBranch(BranchTypeToAuxiliaryBranchName(branchType).c_str());
      if (branch == 0) {
        branch = tree->GetBranch(BranchTypeToAuxBranchName(branchType).c_str());
      }
      return branch;
    }
    TBranch* getProductProvenanceBranch(TTree* tree, BranchType const& branchType) {
      TBranch* branch = tree->GetBranch(BranchTypeToBranchEntryInfoBranchName(branchType).c_str());
      return branch;
    }
    TBranch* getStatusBranch(TTree* tree, BranchType const& branchType) { // backward compatibility
      TBranch* branch = tree->GetBranch(BranchTypeToProductStatusBranchName(branchType).c_str()); // backward compatibility
      return branch; // backward compatibility
    } // backward compatibility
  }
  RootTree::RootTree(boost::shared_ptr<TFile> filePtr,
                     BranchType const& branchType,
                     unsigned int maxVirtualSize,
                     unsigned int cacheSize,
                     unsigned int learningEntries) :
    filePtr_(filePtr),
    tree_(dynamic_cast<TTree*>(filePtr_.get() != 0 ? filePtr->Get(BranchTypeToProductTreeName(branchType).c_str()) : 0)),
    metaTree_(dynamic_cast<TTree*>(filePtr_.get() != 0 ? filePtr->Get(BranchTypeToMetaDataTreeName(branchType).c_str()) : 0)),
    branchType_(branchType),
    auxBranch_(tree_ ? getAuxiliaryBranch(tree_, branchType_) : 0),
    branchEntryInfoBranch_(metaTree_ ? getProductProvenanceBranch(metaTree_, branchType_) : getProductProvenanceBranch(tree_, branchType_)),
    treeCache_(),
    entries_(tree_ ? tree_->GetEntries() : 0),
    entryNumber_(-1),
    branchNames_(),
    branches_(new BranchMap),
    trained_(kFALSE),
    learningEntries_(learningEntries),
    productStatuses_(), // backward compatibility
    pProductStatuses_(&productStatuses_), // backward compatibility
    infoTree_(dynamic_cast<TTree*>(filePtr_.get() != 0 ? filePtr->Get(BranchTypeToInfoTreeName(branchType).c_str()) : 0)), // backward compatibility
    statusBranch_(infoTree_ ? getStatusBranch(infoTree_, branchType_) : 0) { // backward compatibility
      setTreeMaxVirtualSize(maxVirtualSize);
      setCacheSize(cacheSize);
  }

  RootTree::~RootTree() {}

  bool
  RootTree::isValid() const {
    if (metaTree_ == 0 || metaTree_->GetNbranches() == 0) {
      return tree_ != 0 && auxBranch_ != 0;
    }
    if (tree_ != 0 && auxBranch_ != 0 && metaTree_ != 0) { // backward compatibility
      if (branchEntryInfoBranch_ != 0 || statusBranch_ != 0) return true; // backward compatibility
      return (entries_ == metaTree_->GetEntries() && tree_->GetNbranches() <= metaTree_->GetNbranches() + 1);  // backward compatibility
    } // backward compatibility
    return false;
  }

  void
  RootTree::setPresence(BranchDescription const& prod) {
      assert(isValid());
      prod.init();
      if(tree_->GetBranch(prod.branchName().c_str()) == 0){
	prod.setDropped();
      }
  }

  void
  RootTree::addBranch(BranchKey const& key,
		      BranchDescription const& prod,
		      std::string const& oldBranchName) {
      assert(isValid());
      prod.init();
      //use the translated branch name
      TBranch* branch = tree_->GetBranch(oldBranchName.c_str());
      input::BranchInfo info = input::BranchInfo(ConstBranchDescription(prod));
      info.productBranch_ = 0;
      if (prod.present()) {
        info.productBranch_ = branch;
        //we want the new branch name for the JobReport
        branchNames_.push_back(prod.branchName());
      }
      TTree* provTree = (metaTree_ != 0 ? metaTree_ : tree_);
      info.provenanceBranch_ = provTree->GetBranch(oldBranchName.c_str());
      branches_->insert(std::make_pair(key, info));
  }

  void
  RootTree::dropBranch(std::string const& oldBranchName) {
      //use the translated branch name
      TBranch* branch = tree_->GetBranch(oldBranchName.c_str());
      if (branch != 0) {
	TObjArray* leaves = tree_->GetListOfLeaves();
	int entries = leaves->GetEntries();
	for (int i = 0; i < entries; ++i) {
	  TLeaf* leaf = (TLeaf*)(*leaves)[i];
	  if (leaf == 0) continue;
	  TBranch* br = leaf->GetBranch();
	  if (br == 0) continue;
	  if (br->GetMother() == branch) {
	    leaves->Remove(leaf);
	  }
	}
	leaves->Compress();
	tree_->GetListOfBranches()->Remove(branch);
	tree_->GetListOfBranches()->Compress();
	delete branch;
      }
  }

  input::BranchMap const&
  RootTree::branches() const {return *branches_;}

  boost::shared_ptr<DelayedReader>
  RootTree::makeDelayedReader(FileFormatVersion const& fileFormatVersion) const {
    boost::shared_ptr<DelayedReader>
        store(new RootDelayedReader(entryNumber_, branches_, treeCache_, filePtr_, fileFormatVersion));
    return store;
  }

  void
  RootTree::setCacheSize(unsigned int cacheSize) {
    tree_->SetCacheSize(static_cast<Long64_t>(cacheSize));
    treeCache_.reset(dynamic_cast<TTreeCache*>(filePtr_->GetCacheRead()));
    filePtr_->SetCacheRead(0);
  }

  void
  RootTree::setTreeMaxVirtualSize(int treeMaxVirtualSize) {
    if (treeMaxVirtualSize >= 0) tree_->SetMaxVirtualSize(static_cast<Long64_t>(treeMaxVirtualSize));
  }

  void
  RootTree::setEntryNumber(EntryNumber theEntryNumber) {
    filePtr_->SetCacheRead(treeCache_.get());
    entryNumber_ = theEntryNumber;
    tree_->LoadTree(theEntryNumber);
    if (treeCache_ && !trained_ && theEntryNumber >= 0) {
      assert(treeCache_->GetOwner() == tree_);
      treeCache_->SetLearnEntries(learningEntries_);
      treeCache_->SetEntryRange(theEntryNumber, tree_->GetEntries());
      treeCache_->AddBranch(BranchTypeToAuxiliaryBranchName(branchType_).c_str());
      if (branchType_ == edm::InEvent) {
        treeCache_->AddBranch(poolNames::branchListIndexesBranchName().c_str());
      }
      trained_ = kTRUE;
    }
    filePtr_->SetCacheRead(0);
  }


  void
  RootTree::close () {
    // The TFile is about to be closed, and destructed.
    // Just to play it safe, zero all pointers to quantities that are owned by the TFile.
    auxBranch_  = branchEntryInfoBranch_ = statusBranch_ = 0;
    tree_ = metaTree_ = infoTree_ = 0;
    // We own the treeCache_.
    // We make sure the treeCache_ is detatched from the file,
    // so that ROOT does not also delete it.
    filePtr_->SetCacheRead(0);
    trained_ = kFALSE;
    // We give up our shared ownership of the TFile itself.
    filePtr_.reset();
  }

  namespace input {
    Int_t
    getEntry(TBranch* branch, EntryNumber entryNumber) {
      Int_t n = 0;
      try {
        n = branch->GetEntry(entryNumber);
      }
      catch(cms::Exception const& e) {
	throw edm::Exception(edm::errors::FileReadError) << e.explainSelf() << "\n";
      }
      return n;
    }

    Int_t
    getEntry(TTree* tree, EntryNumber entryNumber) {
      Int_t n = 0;
      try {
        n = tree->GetEntry(entryNumber);
      }
      catch(cms::Exception const& e) {
	throw edm::Exception(edm::errors::FileReadError) << e.explainSelf() << "\n";
      }
      return n;
    }

    Int_t
    getEntryWithCache(TBranch* branch, EntryNumber entryNumber, TTreeCache* tc, TFile* filePtr) {
      if (tc == 0) {
        return getEntry(branch, entryNumber);
      }
      filePtr->SetCacheRead(tc);
      Int_t n = getEntry(branch, entryNumber);
      filePtr->SetCacheRead(0);
      return n;
    }

    Int_t
    getEntryWithCache(TTree* tree, EntryNumber entryNumber, TTreeCache* tc, TFile* filePtr) {
      if (tc == 0) {
        return getEntry(tree, entryNumber);
      }
      filePtr->SetCacheRead(tc);
      Int_t n = getEntry(tree, entryNumber);
      filePtr->SetCacheRead(0);
      return n;
    }
  }
}
