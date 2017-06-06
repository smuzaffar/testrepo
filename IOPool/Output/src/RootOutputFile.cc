// $Id: RootOutputFile.cc,v 1.36 2008/01/04 17:07:01 wmtan Exp $

#include "RootOutputFile.h"
#include "PoolOutputModule.h"

#include "FWCore/Utilities/interface/GlobalIdentifier.h"

#include "DataFormats/Provenance/interface/EventAuxiliary.h" 
#include "DataFormats/Provenance/interface/FileFormatVersion.h"
#include "DataFormats/Provenance/interface/FileIndex.h"
#include "FWCore/Utilities/interface/GetFileFormatVersion.h"
#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/Algorithms.h"
#include "FWCore/Framework/interface/FileBlock.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"
#include "DataFormats/Provenance/interface/ModuleDescriptionRegistry.h"
#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/Provenance.h"
#include "DataFormats/Common/interface/BasicHandle.h"
#include "FWCore/Framework/interface/ConstProductRegistry.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/ServiceRegistry/interface/Service.h"

#include "TROOT.h"
#include "TTree.h"
#include "TFile.h"
#include "TClass.h"
#include "Rtypes.h"

#include <algorithm>
#include <map>
#include <iomanip>

namespace edm {
  RootOutputFile::RootOutputFile(PoolOutputModule *om, std::string const& fileName, std::string const& logicalFileName) :
      outputItemList_(), 
      file_(fileName),
      logicalFile_(logicalFileName),
      reportToken_(0),
      eventCount_(0),
      fileSizeCheckEvent_(100),
      om_(om),
      currentlyFastCloning_(),
      filePtr_(TFile::Open(file_.c_str(), "recreate", "", om_->compressionLevel())),
      fid_(),
      fileIndex_(),
      eventEntryNumber_(0LL),
      lumiEntryNumber_(0LL),
      runEntryNumber_(0LL),
      eventProcessHistoryIDs_(),
      metaDataTree_(0),
      eventAux_(),
      lumiAux_(),
      runAux_(),
      pEventAux_(&eventAux_),
      pLumiAux_(&lumiAux_),
      pRunAux_(&runAux_),
      eventTree_(filePtr_, InEvent, pEventAux_, om_->basketSize(), om_->splitLevel(), om_->fastCloning(),
        om_->fileBlock_->tree(), om_->fileBlock_->metaTree(),
	om_->droppedPriorProducts()[InEvent], om_->fileBlock_->oldBranchNames()),
      lumiTree_(filePtr_, InLumi, pLumiAux_, om_->basketSize(), om_->splitLevel()),
      runTree_(filePtr_, InRun, pRunAux_, om_->basketSize(), om_->splitLevel()),
      treePointers_(),
      provenances_(),
      newFileAtEndOfRun_(false) {
    treePointers_[InEvent] = &eventTree_;
    treePointers_[InLumi]  = &lumiTree_;
    treePointers_[InRun]   = &runTree_;
    TTree::SetMaxTreeSize(kMaxLong64);

    for (int i = InEvent; i < NumBranchTypes; ++i) {
      BranchType branchType = static_cast<BranchType>(i);
      fillItemList(om_->keptProducts()[branchType], om_->droppedProducts()[branchType], outputItemList_[branchType]);
      for (OutputItemList::const_iterator it = outputItemList_[branchType].begin(),
	  itEnd = outputItemList_[branchType].end();
	  it != itEnd; ++it) {
	treePointers_[branchType]->addBranch(*it->branchDescription_, it->selected_, it->branchEntryDescription_, it->product_);
      }
    }
    // Don't split metadata tree.
    metaDataTree_ = RootOutputTree::makeTTree(filePtr_.get(), poolNames::metaDataTreeName(), 0);

    fid_ = FileID(createGlobalIdentifier());

    // Register the output file with the JobReport service
    // and get back the token for it.
    std::string moduleName = "PoolOutputModule";
    Service<JobReport> reportSvc;
    reportToken_ = reportSvc->outputFileOpened(
		      file_, logicalFile_,  // PFN and LFN
		      om_->catalog_,  // catalog
		      moduleName,   // module class name
		      om_->moduleLabel_,  // module label
		      fid_.fid(), // file id (guid)
		      eventTree_.branchNames()); // branch names being written
  }

  void RootOutputFile::fillItemList(Selections const& keptVector,
				    Selections const& droppedVector,
				    OutputItemList & outputItemList) {

    std::vector<std::string> const& renamed = om_->fileBlock_->sortedNewBranchNames();
    for (Selections::const_iterator it = keptVector.begin(), itEnd = keptVector.end(); it != itEnd; ++it) {
      BranchDescription const& prod = **it;
      outputItemList.push_back(OutputItem(&prod, true, binary_search_all(renamed, prod.branchName())));
    }
    for (Selections::const_iterator it = droppedVector.begin(), itEnd = droppedVector.end(); it != itEnd; ++it) {
      BranchDescription const& prod = **it;
      outputItemList.push_back(OutputItem(&prod, false, binary_search_all(renamed, prod.branchName())));
    }
    sort_all(outputItemList);
  }


  void RootOutputFile::openFile(FileBlock const& fb, bool fastCloneThisOne) {
    currentlyFastCloning_ = om_->fastCloning() && fb.fastClonable() && fastCloneThisOne;
    eventTree_.openFile(currentlyFastCloning_);
    if (currentlyFastCloning_) {
      eventTree_.fastCloneTree(fb.tree(), fb.metaTree());
    }
  }

  void RootOutputFile::respondToCloseInputFile(FileBlock const&) {
    eventTree_.setEntries();
    lumiTree_.setEntries();
    runTree_.setEntries();
  }

  void RootOutputFile::writeOne(EventPrincipal const& e) {
    ++eventCount_;

    // Auxiliary branch
    pEventAux_ = &e.aux();

    // Add event to index
    fileIndex_.addEntry(pEventAux_->run(), pEventAux_->luminosityBlock(), pEventAux_->event(), eventEntryNumber_);
    ++eventEntryNumber_;

    // Add event proces history
    eventProcessHistoryIDs_.push_back(EventProcessHistoryID(pEventAux_->id(), pEventAux_->processHistoryID()));
    // Store an invailid process history ID in EventAuxiliary for obsolete field.
    pEventAux_->processHistoryID_ = ProcessHistoryID();

    fillBranches(InEvent, e.groupGetter());

    // Report event written 
    Service<JobReport> reportSvc;
    reportSvc->eventWrittenToFile(reportToken_, e.id().run(), e.id().event());

    if (eventCount_ >= fileSizeCheckEvent_) {
	unsigned int const oneK = 1024;
	Long64_t size = filePtr_->GetSize()/oneK;
	unsigned int eventSize = std::max(size/eventCount_, 1LL);
	if (size + 2*eventSize >= om_->maxFileSize_) {
	  newFileAtEndOfRun_ = true;
	} else {
	  unsigned int increment = (om_->maxFileSize_ - size)/eventSize;
	  increment -= increment/8;	// Prevents overshoot
	  fileSizeCheckEvent_ = eventCount_ + increment;
	}
    }
  }

  void RootOutputFile::writeLuminosityBlock(LuminosityBlockPrincipal const& lb) {
    // Auxiliary branch
    pLumiAux_ = &lb.aux();
    // Add lumi to index.
    fileIndex_.addEntry(pLumiAux_->run(), pLumiAux_->luminosityBlock(), 0U, lumiEntryNumber_);
    ++lumiEntryNumber_;
    fillBranches(InLumi, lb.groupGetter());
  }

  bool RootOutputFile::writeRun(RunPrincipal const& r) {
    // Auxiliary branch
    pRunAux_ = &r.aux();
    // Add run to index.
    fileIndex_.addEntry(pRunAux_->run(), 0U, 0U, runEntryNumber_);
    ++runEntryNumber_;
    fillBranches(InRun, r.groupGetter());
    return newFileAtEndOfRun_;
  }

  void RootOutputFile::writeFileFormatVersion() {
    FileFormatVersion fileFormatVersion(edm::getFileFormatVersion());
    FileFormatVersion * pFileFmtVsn = &fileFormatVersion;
    TBranch* b = metaDataTree_->Branch(poolNames::fileFormatVersionBranchName().c_str(), &pFileFmtVsn, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeFileIdentifier() {
    FileID *fidPtr = &fid_;
    TBranch* b = metaDataTree_->Branch(poolNames::fileIdentifierBranchName().c_str(), &fidPtr, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeFileIndex() {
    fileIndex_.sort();
    FileIndex *findexPtr = &fileIndex_;
    TBranch* b = metaDataTree_->Branch(poolNames::fileIndexBranchName().c_str(), &findexPtr, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeEventHistory() {
    edm::sort_all(eventProcessHistoryIDs_);
    std::vector<EventProcessHistoryID> *phPtr = &eventProcessHistoryIDs_;
    TBranch* b = metaDataTree_->Branch(poolNames::eventHistoryBranchName().c_str(), &phPtr, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeProcessConfigurationRegistry() {
    // We don't do this yet; currently we're storing a slightly bloated ProcessHistoryRegistry.
  }

  void RootOutputFile::writeProcessHistoryRegistry() { 
    ProcessHistoryMap *pProcHistMap = &ProcessHistoryRegistry::instance()->data();
    TBranch* b = metaDataTree_->Branch(poolNames::processHistoryMapBranchName().c_str(), &pProcHistMap, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeModuleDescriptionRegistry() { 
    ModuleDescriptionMap *pModDescMap = &ModuleDescriptionRegistry::instance()->data();
    TBranch* b = metaDataTree_->Branch(poolNames::moduleDescriptionMapBranchName().c_str(), &pModDescMap, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeParameterSetRegistry() { 
    typedef std::map<ParameterSetID, ParameterSetBlob> ParameterSetMap;
    ParameterSetMap psetMap;
    pset::Registry const* psetRegistry = pset::Registry::instance();    
    for (pset::Registry::const_iterator it = psetRegistry->begin(), itEnd = psetRegistry->end(); it != itEnd; ++it) {
      psetMap.insert(std::make_pair(it->first, ParameterSetBlob(it->second.toStringOfTracked())));
    }
    ParameterSetMap *pPsetMap = &psetMap;
    TBranch* b = metaDataTree_->Branch(poolNames::parameterSetMapBranchName().c_str(), &pPsetMap, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::writeProductDescriptionRegistry() { 
    Service<ConstProductRegistry> reg;
    ProductRegistry pReg = reg->productRegistry();
    ProductRegistry * ppReg = &pReg;
    TBranch* b = metaDataTree_->Branch(poolNames::productDescriptionBranchName().c_str(), &ppReg, om_->basketSize(), 0);
    assert(b);
    b->Fill();
  }

  void RootOutputFile::finishEndFile() { 
    metaDataTree_->SetEntries(-1);
    RootOutputTree::writeTTree(metaDataTree_);
    for (int i = InEvent; i < NumBranchTypes; ++i) {
      BranchType branchType = static_cast<BranchType>(i);
      buildIndex(treePointers_[branchType]->tree(), branchType);
      setBranchAliases(treePointers_[branchType]->tree(), om_->keptProducts()[branchType]);
      treePointers_[branchType]->writeTree();
    }

    // close the file -- mfp
    filePtr_->Close();
    filePtr_.reset();

    // report that file has been closed
    Service<JobReport> reportSvc;
    reportSvc->outputFileClosed(reportToken_);

  }

  void RootOutputFile::RootOutputFile::fillBranches(BranchType const& branchType, Principal const& principal) const {

    // Clear the provenance cache for the previous event/lumi/run
    provenances_.clear();

    bool const fastCloning = (branchType == InEvent) && currentlyFastCloning_;

    OutputItemList const& items = outputItemList_[branchType];
    // Loop over EDProduct branches, fill the provenance, and write the branch.
    for (OutputItemList::const_iterator i = items.begin(), iEnd = items.end(); i != iEnd; ++i) {

      // If fast cloning, process only produced or renamed branches.
      if (fastCloning && !i->branchDescription_->produced() && !i->renamed_) continue;

      ProductID const& id = i->branchDescription_->productID_;

      if (id == ProductID()) {
	throw edm::Exception(edm::errors::ProductNotFound,"InvalidID")
	  << "PoolOutputModule::write: invalid ProductID supplied in productRegistry\n";
      }

      EDProduct const* product = 0;
      BasicHandle const bh = principal.getForOutput(id, i->selected_);
      if (bh.provenance() == 0) {
	// No group with this ID is in the event.
	// Create and write the provenance.
	if (i->branchDescription_->produced_) {
          BranchEntryDescription provenance;
	  provenance.moduleDescriptionID_ = i->branchDescription_->moduleDescriptionID_;
	  provenance.productID_ = id;
	  provenance.status_ = BranchEntryDescription::CreatorNotRun;
	  provenance.isPresent_ = false;
	  provenance.cid_ = 0;
	  
	  provenances_.push_front(provenance); 
	  i->branchEntryDescription_ = &*provenances_.begin();
	} else {
	    throw edm::Exception(errors::ProductNotFound,"NoMatch")
	      << "PoolOutputModule: Unexpected internal error.  Contact the framework group.\n"
	      << "No group for branch" << i->branchDescription_->branchName_ << '\n';
	}
      } else {
	product = bh.wrapper();
        BranchEntryDescription const& provenance = bh.provenance()->event();
	// There is a group with this ID is in the event.  Write the provenance.
	bool present = i->selected_ && product && product->isPresent();
	if (present == provenance.isPresent()) {
	  // The provenance can be written out as is, saving a copy. 
	  i->branchEntryDescription_ = &provenance;
	} else {
	  // We need to make a private copy of the provenance so we can set isPresent_ correctly.
	  provenances_.push_front(provenance);
	  provenances_.begin()->isPresent_ = present;
	  i->branchEntryDescription_ = &*provenances_.begin();
	}
      }
      if (i->selected_) {
	if (product == 0) {
	  // Add a null product.
	  TClass *cp = gROOT->GetClass(i->branchDescription_->wrappedName().c_str());
	  product = static_cast<EDProduct *>(cp->New());
	}
	i->product_ = product;
      }
    }
    treePointers_[branchType]->fillTree();
  }



  void
  RootOutputFile::buildIndex(TTree * tree, BranchType const& branchType) {

    if (tree->GetNbranches() == 0) return;
    tree->SetEntries(-1);
    if (tree->GetEntries() == 0) return;

    // BuildIndex must read the auxiliary branch, so the
    // buffers need to be set to point to allocated memory.
    pEventAux_ = &eventAux_;
    pLumiAux_ = &lumiAux_;
    pRunAux_ = &runAux_;

    if (BranchTypeToMinorIndexName(branchType).empty()) {
      tree->BuildIndex(BranchTypeToMajorIndexName(branchType).c_str());
    } else {
      tree->BuildIndex(BranchTypeToMajorIndexName(branchType).c_str(),
	 	       BranchTypeToMinorIndexName(branchType).c_str());
    }
  }
  
  void
  RootOutputFile::setBranchAliases(TTree *tree, Selections const& branches) const {
    if (tree && tree->GetNbranches() != 0) {
      for (Selections::const_iterator i = branches.begin(), iEnd = branches.end();
	  i != iEnd; ++i) {
	BranchDescription const& pd = **i;
	std::string const& full = pd.branchName() + "obj";
	if (pd.branchAliases().empty()) {
	  std::string const& alias =
	      (pd.productInstanceName().empty() ? pd.moduleLabel() : pd.productInstanceName());
	  tree->SetAlias(alias.c_str(), full.c_str());
	} else {
	  std::set<std::string>::const_iterator it = pd.branchAliases().begin(), itEnd = pd.branchAliases().end();
	  for (; it != itEnd; ++it) {
	    tree->SetAlias((*it).c_str(), full.c_str());
	  }
	}
      }
    }
  }
}
