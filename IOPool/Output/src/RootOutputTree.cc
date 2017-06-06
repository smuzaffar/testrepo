#include "RootOutputTree.h"
#include "TFile.h"
#include "TChain.h"
#include "FWCore/Utilities/interface/for_all.h"

#include "boost/bind.hpp"
#include <algorithm>

namespace edm {

  TTree *
  RootOutputTree::makeTree(TFile * filePtr,
			   std::string const& name,
			   int splitLevel,
			   TChain * chain,
			   Selections const& keepList) {
    TTree *tree;
    if (chain != 0) {
      pruneTTree(chain, keepList);
      tree = chain->CloneTree(-1, "fast");
      tree->SetBranchStatus("*", 1);
    } else {
      tree = new TTree(name.c_str(), "", splitLevel);
    }
    tree->SetDirectory(filePtr);
    return tree;
    
  }

  void
  RootOutputTree::writeTTree(TTree *tree) {
    if (tree->GetNbranches() != 0) {
      tree->SetEntries(-1);
    }
    tree->AutoSave();
  }

  void
  RootOutputTree::fillTTree(TTree * tree, std::vector<TBranch *> const& branches) {
    for_all(branches, boost::bind(&TBranch::Fill, _1));
  }

  void
  RootOutputTree::writeTree() const {
    writeTTree(tree_);
    writeTTree(metaTree_);
  }

  void
  RootOutputTree::fillTree() const {
    fillTTree(metaTree_, metaBranches_);
    fillTTree(tree_, branches_);
  }

  void
  RootOutputTree::pruneTTree(TTree *tree, Selections const& keepList) {
  // Since we don't know the history, make sure all branches are deactivated.
    tree->SetBranchStatus("*", 0);
  
  // Iterate over the list of branch names to keep
  
   for(Selections::const_iterator it = keepList.begin(), itEnd=keepList.end(); it != itEnd; ++it) {
     std::string branchName = (*it)->branchName();
     char lastchar = branchName.at(branchName.size()-1);
     if(lastchar == '.') {
       branchName += "*";
     } else {
       branchName += ".*";
     }
     tree->SetBranchStatus(branchName.c_str(), 1);
    }
  }

  void
  RootOutputTree::addBranch(BranchDescription const& prod, bool selected, BranchEntryDescription const*& pProv, void const*& pProd) {
      prod.init();
      TBranch * meta = metaTree_->Branch(prod.branchName().c_str(), &pProv, basketSize_, 0);
      metaBranches_.push_back(meta);
      if (selected) {
	TBranch * branch = tree_->Branch(prod.branchName().c_str(),
		       prod.wrappedName().c_str(),
		       &pProd,
		       (prod.basketSize() == BranchDescription::invalidBasketSize ? basketSize_ : prod.basketSize()),
		       (prod.splitLevel() == BranchDescription::invalidSplitLevel ? splitLevel_ : prod.splitLevel()));
        branches_.push_back(branch);
	// we want the new branch name for the JobReport
	branchNames_.push_back(prod.branchName());
      }
  }
}
