/*----------------------------------------------------------------------
----------------------------------------------------------------------*/

#include "RootFile.h"
#include "DuplicateChecker.h"
#include "ProvenanceAdaptor.h"

#include "FWCore/Utilities/interface/EDMException.h"
#include "FWCore/Utilities/interface/GlobalIdentifier.h"
#include "DataFormats/Provenance/interface/BranchDescription.h"
#include "DataFormats/Provenance/interface/BranchType.h"
#include "FWCore/Framework/interface/FileBlock.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/GroupSelector.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"
#include "FWCore/ParameterSet/interface/FillProductRegistryTransients.h"
#include "FWCore/Sources/interface/EventSkipperByID.h"
#include "DataFormats/Provenance/interface/BranchChildren.h"
#include "DataFormats/Provenance/interface/MinimalEventID.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include "DataFormats/Provenance/interface/ParentageRegistry.h"
#include "DataFormats/Provenance/interface/ProcessConfigurationRegistry.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/RunID.h"
#include "DataFormats/Common/interface/RefCoreStreamer.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/Utilities/interface/Algorithms.h"
#include "DataFormats/Common/interface/EDProduct.h"
//used for friendlyName translation
#include "FWCore/Utilities/interface/FriendlyName.h"

//used for backward compatibility
#include "DataFormats/Provenance/interface/BranchEntryDescription.h"
#include "DataFormats/Provenance/interface/EntryDescriptionRegistry.h"
#include "DataFormats/Provenance/interface/EventAux.h"
#include "DataFormats/Provenance/interface/LuminosityBlockAux.h"
#include "DataFormats/Provenance/interface/RunAux.h"
#include "DataFormats/Provenance/interface/RunLumiEntryInfo.h"
#include "FWCore/ParameterSet/interface/ParameterSetConverter.h"

#include "TROOT.h"
#include "TClass.h"
#include "TFile.h"
#include "TTree.h"
#include "Rtypes.h"
#include <algorithm>
#include <map>
#include <list>

namespace edm {
  namespace {
    int
    forcedRunOffset(RunNumber_t const& forcedRunNumber, IndexIntoFile::IndexIntoFileItr inxBegin, IndexIntoFile::IndexIntoFileItr inxEnd) {
      if(inxBegin == inxEnd) return 0;
      int defaultOffset = (inxBegin.run() != 0 ? 0 : 1);
      int offset = (forcedRunNumber != 0U ? forcedRunNumber - inxBegin.run() : defaultOffset);
      if(offset < 0) {
        throw edm::Exception(errors::Configuration)
          << "The value of the 'setRunNumber' parameter must not be\n"
          << "less than the first run number in the first input file.\n"
          << "'setRunNumber' was " << forcedRunNumber <<", while the first run was "
          << forcedRunNumber - offset << ".\n";
      }
      return offset;
    }
  }

  // This is a helper class for IndexIntoFile.
  class RootFileEventFinder : public IndexIntoFile::EventFinder {
  public:
    explicit RootFileEventFinder(RootTree& eventTree) : eventTree_(eventTree) {}
    virtual ~RootFileEventFinder() {}
    virtual
    EventNumber_t getEventNumberOfEntry(input::EntryNumber entry) const {
      eventTree_.setEntryNumber(entry);
      EventAuxiliary eventAux;
      EventAuxiliary *pEvAux = &eventAux;
      eventTree_.fillAux<EventAuxiliary>(pEvAux);
      eventTree_.setEntryNumber(-1);
      return eventAux.event();
    }

  private:
     RootTree& eventTree_;
  };


//---------------------------------------------------------------------
  RootFile::RootFile(std::string const& fileName,
		     ProcessConfiguration const& processConfiguration,
		     std::string const& logicalFileName,
		     boost::shared_ptr<TFile> filePtr,
		     boost::shared_ptr<EventSkipperByID> eventSkipperByID,
		     bool skipAnyEvents,
		     int remainingEvents,
		     int remainingLumis,
		     unsigned int treeCacheSize,
                     int treeMaxVirtualSize,
		     InputSource::ProcessingMode processingMode,
		     RunNumber_t const& forcedRunNumber,
                     bool noEventSort,
		     GroupSelectorRules const& groupSelectorRules,
                     bool secondaryFile,
                     boost::shared_ptr<DuplicateChecker> duplicateChecker,
                     bool dropDescendants,
                     std::vector<boost::shared_ptr<IndexIntoFile> > const& indexesIntoFiles,
                     std::vector<boost::shared_ptr<IndexIntoFile> >::size_type currentIndexIntoFile,
                     std::vector<ProcessHistoryID>& orderedProcessHistoryIDs) :
      file_(fileName),
      logicalFile_(logicalFileName),
      processConfiguration_(processConfiguration),
      filePtr_(filePtr),
      eventSkipperByID_(eventSkipperByID),
      fileFormatVersion_(),
      fid_(),
      indexIntoFileSharedPtr_(new IndexIntoFile),
      indexIntoFile_(*indexIntoFileSharedPtr_),
      orderedProcessHistoryIDs_(orderedProcessHistoryIDs),
      indexIntoFileBegin_(indexIntoFile_.begin(noEventSort ? IndexIntoFile::firstAppearanceOrder : IndexIntoFile::numericalOrder)),
      indexIntoFileEnd_(indexIntoFileBegin_),
      indexIntoFileIter_(indexIntoFileBegin_),
      eventProcessHistoryIDs_(),
      eventProcessHistoryIter_(eventProcessHistoryIDs_.begin()),
      skipAnyEvents_(skipAnyEvents),
      noEventSort_(noEventSort),
      whyNotFastClonable_(0),
      reportToken_(0),
      eventAux_(),
      eventTree_(filePtr_, InEvent),
      lumiTree_(filePtr_, InLumi),
      runTree_(filePtr_, InRun),
      treePointers_(),
      lastEventEntryNumberRead_(-1LL),
      productRegistry_(),
      branchIDLists_(),
      processingMode_(processingMode),
      forcedRunOffset_(0),
      newBranchToOldBranch_(),
      eventHistoryTree_(0),
      history_(new History),
      branchChildren_(new BranchChildren),
      duplicateChecker_(duplicateChecker),
      provenanceAdaptor_() {

    eventTree_.setCacheSize(treeCacheSize);

    eventTree_.setTreeMaxVirtualSize(treeMaxVirtualSize);
    lumiTree_.setTreeMaxVirtualSize(treeMaxVirtualSize);
    runTree_.setTreeMaxVirtualSize(treeMaxVirtualSize);

    treePointers_[InEvent] = &eventTree_;
    treePointers_[InLumi]  = &lumiTree_;
    treePointers_[InRun]   = &runTree_;

    // Read the metadata tree.
    TTree *metaDataTree = dynamic_cast<TTree *>(filePtr_->Get(poolNames::metaDataTreeName().c_str()));
    if(!metaDataTree)
      throw edm::Exception(errors::FileReadError) << "Could not find tree " << poolNames::metaDataTreeName()
							 << " in the input file.\n";

    // To keep things simple, we just read in every possible branch that exists.
    // We don't pay attention to which branches exist in which file format versions

    FileFormatVersion *fftPtr = &fileFormatVersion_;
    if(metaDataTree->FindBranch(poolNames::fileFormatVersionBranchName().c_str()) != 0) {
      TBranch *fft = metaDataTree->GetBranch(poolNames::fileFormatVersionBranchName().c_str());
      fft->SetAddress(&fftPtr);
      input::getEntry(fft, 0);
      metaDataTree->SetBranchAddress(poolNames::fileFormatVersionBranchName().c_str(), &fftPtr);
    }

    setRefCoreStreamer(0, !fileFormatVersion().splitProductIDs(), !fileFormatVersion().productIDIsInt()); // backward compatibility

    FileID *fidPtr = &fid_;
    if(metaDataTree->FindBranch(poolNames::fileIdentifierBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::fileIdentifierBranchName().c_str(), &fidPtr);
    }

    IndexIntoFile *iifPtr = &indexIntoFile_;
    if(metaDataTree->FindBranch(poolNames::indexIntoFileBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::indexIntoFileBranchName().c_str(), &iifPtr);
    }

    // Need to read to a temporary registry so we can do a translation of the BranchKeys.
    // This preserves backward compatibility against friendly class name algorithm changes.
    ProductRegistry inputProdDescReg;
    ProductRegistry *ppReg = &inputProdDescReg;
    metaDataTree->SetBranchAddress(poolNames::productDescriptionBranchName().c_str(),(&ppReg));

    typedef std::map<ParameterSetID, ParameterSetBlob> PsetMap;
    PsetMap psetMap;
    PsetMap *psetMapPtr = &psetMap;
    if(metaDataTree->FindBranch(poolNames::parameterSetMapBranchName().c_str()) != 0) {
      //backward compatibility
      assert(!fileFormatVersion().parameterSetsTree());
      metaDataTree->SetBranchAddress(poolNames::parameterSetMapBranchName().c_str(), &psetMapPtr);
    } else {
      assert(fileFormatVersion().parameterSetsTree());
      TTree* psetTree = dynamic_cast<TTree *>(filePtr_->Get(poolNames::parameterSetsTreeName().c_str()));
      if(0 == psetTree) {
        throw Exception(errors::FileReadError) << "Could not find tree " << poolNames::parameterSetsTreeName()
        << " in the input file.\n";
      }
      typedef std::pair<ParameterSetID, ParameterSetBlob> IdToBlobs;
      IdToBlobs idToBlob;
      IdToBlobs* pIdToBlob = &idToBlob;
      psetTree->SetBranchAddress(poolNames::idToParameterSetBlobsBranchName().c_str(), &pIdToBlob);
      for(Long64_t i = 0; i != psetTree->GetEntries(); ++i) {
        psetTree->GetEntry(i);
        psetMap.insert(idToBlob);
      }
    }

    // backward compatibility
    ProcessHistoryRegistry::collection_type pHistMap;
    ProcessHistoryRegistry::collection_type *pHistMapPtr = &pHistMap;
    if(metaDataTree->FindBranch(poolNames::processHistoryMapBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::processHistoryMapBranchName().c_str(), &pHistMapPtr);
    }

    ProcessHistoryRegistry::vector_type pHistVector;
    ProcessHistoryRegistry::vector_type *pHistVectorPtr = &pHistVector;
    if(metaDataTree->FindBranch(poolNames::processHistoryBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::processHistoryBranchName().c_str(), &pHistVectorPtr);
    }

    ProcessConfigurationVector procConfigVector;
    ProcessConfigurationVector* procConfigVectorPtr = &procConfigVector;
    if(metaDataTree->FindBranch(poolNames::processConfigurationBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::processConfigurationBranchName().c_str(), &procConfigVectorPtr);
    }

    std::auto_ptr<BranchIDListRegistry::collection_type> branchIDListsAPtr(new BranchIDListRegistry::collection_type);
    BranchIDListRegistry::collection_type *branchIDListsPtr = branchIDListsAPtr.get();
    if(metaDataTree->FindBranch(poolNames::branchIDListBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::branchIDListBranchName().c_str(), &branchIDListsPtr);
    }

    BranchChildren* branchChildrenBuffer = branchChildren_.get();
    if(metaDataTree->FindBranch(poolNames::productDependenciesBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::productDependenciesBranchName().c_str(), &branchChildrenBuffer);
    }

    // backward compatibility
    std::vector<EventProcessHistoryID> *eventHistoryIDsPtr = &eventProcessHistoryIDs_;
    if(metaDataTree->FindBranch(poolNames::eventHistoryBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::eventHistoryBranchName().c_str(), &eventHistoryIDsPtr);
    }

    if(metaDataTree->FindBranch(poolNames::moduleDescriptionMapBranchName().c_str()) != 0) {
      if(metaDataTree->GetBranch(poolNames::moduleDescriptionMapBranchName().c_str())->GetSplitLevel() != 0) {
        metaDataTree->SetBranchStatus((poolNames::moduleDescriptionMapBranchName() + ".*").c_str(), 0);
      } else {
        metaDataTree->SetBranchStatus(poolNames::moduleDescriptionMapBranchName().c_str(), 0);
      }
    }

    // Here we read the metadata tree
    input::getEntry(metaDataTree, 0);

    eventProcessHistoryIter_ = eventProcessHistoryIDs_.begin();

    // Here we read the event history tree
    readEventHistoryTree();

    ParameterSetConverter::ParameterSetIdConverter psetIdConverter;
    if(!fileFormatVersion().triggerPathsTracked()) {
      ParameterSetConverter converter(psetMap, psetIdConverter, fileFormatVersion().parameterSetsByReference());
    } else {
      // Merge into the parameter set registry.
      pset::Registry& psetRegistry = *pset::Registry::instance();
      for(PsetMap::const_iterator i = psetMap.begin(), iEnd = psetMap.end(); i != iEnd; ++i) {
        ParameterSet pset(i->second.pset());
        pset.setID(i->first);
        psetRegistry.insertMapped(pset);
      }
    }
    if(!fileFormatVersion().splitProductIDs()) {
      // Old provenance format input file.  Create a provenance adaptor.
      provenanceAdaptor_.reset(new ProvenanceAdaptor(
	    inputProdDescReg, pHistMap, pHistVector, procConfigVector, psetIdConverter, true));
      // Fill in the branchIDLists branch from the provenance adaptor
      branchIDLists_ = provenanceAdaptor_->branchIDLists();
    } else {
      if(!fileFormatVersion().triggerPathsTracked()) {
        // New provenance format, but change in ParameterSet Format. Create a provenance adaptor.
        provenanceAdaptor_.reset(new ProvenanceAdaptor(
	    inputProdDescReg, pHistMap, pHistVector, procConfigVector, psetIdConverter, false));
      }
      // New provenance format input file. The branchIDLists branch was read directly from the input file.
      if(metaDataTree->FindBranch(poolNames::branchIDListBranchName().c_str()) == 0) {
	throw edm::Exception(errors::EventCorruption)
	  << "Failed to find branchIDLists branch in metaData tree.\n";
      }
      branchIDLists_.reset(branchIDListsAPtr.release());
    }

    // Merge into the hashed registries.
    ProcessHistoryRegistry::instance()->insertCollection(pHistVector);
    ProcessConfigurationRegistry::instance()->insertCollection(procConfigVector);

    validateFile(secondaryFile);

    // Read the parentage tree.  Old format files are handled internally in readParentageTree().
    readParentageTree();

    if (eventSkipperByID_ && eventSkipperByID_->somethingToSkip()) {
      whyNotFastClonable_ += FileBlock::EventsOrLumisSelectedByID;
    }

    initializeDuplicateChecker(indexesIntoFiles, currentIndexIntoFile);
    indexIntoFileIter_ = indexIntoFileBegin_ = indexIntoFile_.begin(noEventSort ? IndexIntoFile::firstAppearanceOrder : IndexIntoFile::numericalOrder);
    indexIntoFileEnd_ = indexIntoFile_.end(noEventSort ? IndexIntoFile::firstAppearanceOrder : IndexIntoFile::numericalOrder);
    forcedRunOffset_ = forcedRunOffset(forcedRunNumber, indexIntoFileBegin_, indexIntoFileEnd_);
    eventProcessHistoryIter_ = eventProcessHistoryIDs_.begin();

    // Set product presence information in the product registry.
    ProductRegistry::ProductList const& pList = inputProdDescReg.productList();
    for(ProductRegistry::ProductList::const_iterator it = pList.begin(), itEnd = pList.end();
        it != itEnd; ++it) {
      BranchDescription const& prod = it->second;
      treePointers_[prod.branchType()]->setPresence(prod);
    }

    fillProductRegistryTransients(procConfigVector, inputProdDescReg);

    std::auto_ptr<ProductRegistry> newReg(new ProductRegistry);

    // Do the translation from the old registry to the new one
    {
      ProductRegistry::ProductList const& prodList = inputProdDescReg.productList();
      for(ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
           it != itEnd; ++it) {
        BranchDescription const& prod = it->second;
        std::string newFriendlyName = friendlyname::friendlyName(prod.className());
	if(newFriendlyName == prod.friendlyClassName()) {
          newReg->copyProduct(prod);
	} else {
          if(fileFormatVersion().splitProductIDs()) {
	    throw edm::Exception(errors::UnimplementedFeature)
	      << "Cannot change friendly class name algorithm without more development work\n"
	      << "to update BranchIDLists.  Contact the framework group.\n";
	  }
          BranchDescription newBD(prod);
          newBD.updateFriendlyClassName();
          newReg->copyProduct(newBD);
	  // Need to call init to get old branch name.
	  prod.init();
	  newBranchToOldBranch_.insert(std::make_pair(newBD.branchName(), prod.branchName()));
	}
      }
      dropOnInput(*newReg, groupSelectorRules, dropDescendants, secondaryFile);
      // freeze the product registry
      newReg->setFrozen();
      productRegistry_.reset(newReg.release());
    }


    // Set up information from the product registry.
    ProductRegistry::ProductList const& prodList = productRegistry()->productList();
    for(ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
        it != itEnd; ++it) {
      BranchDescription const& prod = it->second;
      treePointers_[prod.branchType()]->addBranch(it->first, prod,
						  newBranchToOldBranch(prod.branchName()));
    }

    // Determine if this file is fast clonable.
    setIfFastClonable(remainingEvents, remainingLumis);

    setRefCoreStreamer(true);  // backward compatibility
  }

  RootFile::~RootFile() {
  }

  void
  RootFile::readEntryDescriptionTree() {
    // Called only for old format files.
    if(!fileFormatVersion().perEventProductIDs()) return;
    TTree* entryDescriptionTree = dynamic_cast<TTree*>(filePtr_->Get(poolNames::entryDescriptionTreeName().c_str()));
    if(!entryDescriptionTree)
      throw edm::Exception(errors::FileReadError) << "Could not find tree " << poolNames::entryDescriptionTreeName()
							 << " in the input file.\n";


    EntryDescriptionID idBuffer;
    EntryDescriptionID* pidBuffer = &idBuffer;
    entryDescriptionTree->SetBranchAddress(poolNames::entryDescriptionIDBranchName().c_str(), &pidBuffer);

    EntryDescriptionRegistry& oldregistry = *EntryDescriptionRegistry::instance();

    EventEntryDescription entryDescriptionBuffer;
    EventEntryDescription *pEntryDescriptionBuffer = &entryDescriptionBuffer;
    entryDescriptionTree->SetBranchAddress(poolNames::entryDescriptionBranchName().c_str(), &pEntryDescriptionBuffer);

    // Fill in the parentage registry.
    ParentageRegistry& registry = *ParentageRegistry::instance();

    for(Long64_t i = 0, numEntries = entryDescriptionTree->GetEntries(); i < numEntries; ++i) {
      input::getEntry(entryDescriptionTree, i);
      if(idBuffer != entryDescriptionBuffer.id())
	throw edm::Exception(errors::EventCorruption) << "Corruption of EntryDescription tree detected.\n";
      oldregistry.insertMapped(entryDescriptionBuffer);
      Parentage parents;
      parents.parents() = entryDescriptionBuffer.parents();
      registry.insertMapped(parents);
    }
    entryDescriptionTree->SetBranchAddress(poolNames::entryDescriptionIDBranchName().c_str(), 0);
    entryDescriptionTree->SetBranchAddress(poolNames::entryDescriptionBranchName().c_str(), 0);
  }

  void
  RootFile::readParentageTree() {
    if(!fileFormatVersion().splitProductIDs()) {
      // Old format file.
      readEntryDescriptionTree();
      return;
    }
    // New format file
    TTree* parentageTree = dynamic_cast<TTree*>(filePtr_->Get(poolNames::parentageTreeName().c_str()));
    if(!parentageTree)
      throw edm::Exception(errors::FileReadError) << "Could not find tree " << poolNames::parentageTreeName()
							 << " in the input file.\n";

    Parentage parentageBuffer;
    Parentage *pParentageBuffer = &parentageBuffer;
    parentageTree->SetBranchAddress(poolNames::parentageBranchName().c_str(), &pParentageBuffer);

    ParentageRegistry& registry = *ParentageRegistry::instance();

    for(Long64_t i = 0, numEntries = parentageTree->GetEntries(); i < numEntries; ++i) {
      input::getEntry(parentageTree, i);
      registry.insertMapped(parentageBuffer);
    }
    parentageTree->SetBranchAddress(poolNames::parentageBranchName().c_str(), 0);
  }

  void
  RootFile::setIfFastClonable(int remainingEvents, int remainingLumis) {
    if(!fileFormatVersion().splitProductIDs()) {
      whyNotFastClonable_ += FileBlock::FileTooOld;
      return;
    }
    if(processingMode_ != InputSource::RunsLumisAndEvents) {
      whyNotFastClonable_ += FileBlock::NotProcessingEvents;
      return;
    }
    // Find entry for first event in file
    IndexIntoFile::IndexIntoFileItr it = indexIntoFileBegin_;
    while(it != indexIntoFileEnd_ && it.getEntryType() != IndexIntoFile::kEvent) {
      ++it;
    }
    if(it == indexIntoFileEnd_) {
      whyNotFastClonable_ += FileBlock::NoEventsInFile;
      return;
    }

    // From here on, record all reasons we can't fast clone.
    IndexIntoFile::SortOrder sortOrder = IndexIntoFile::numericalOrder;
    if (noEventSort_) sortOrder = IndexIntoFile::firstAppearanceOrder;
    if(!indexIntoFile_.iterationWillBeInEntryOrder(sortOrder)) {
      whyNotFastClonable_ += FileBlock::EventsToBeSorted;
    }
    if(skipAnyEvents_) {
      whyNotFastClonable_ += FileBlock::InitialEventsSkipped;
    }
    if(remainingEvents >= 0 && eventTree_.entries() > remainingEvents) {
      whyNotFastClonable_ += FileBlock::MaxEventsTooSmall;
    }
    if(remainingLumis >= 0 && lumiTree_.entries() > remainingLumis) {
      whyNotFastClonable_ += FileBlock::MaxLumisTooSmall;
    }
    // We no longer fast copy the EventAuxiliary branch, so there
    // is no longer any need to disable fast copying because the run
    // number is being modified.   Also, this check did not work anyway
    // because this function is called before forcedRunOffset_ is set.

    // if(forcedRunOffset_ != 0) {
    //   whyNotFastClonable_ += FileBlock::RunNumberModified;
    // }
    if(duplicateChecker_ &&
      !duplicateChecker_->checkDisabled() &&
      !duplicateChecker_->noDuplicatesInFile()) {
      whyNotFastClonable_ += FileBlock::DuplicateEventsRemoved;
    }
  }

  boost::shared_ptr<FileBlock>
  RootFile::createFileBlock() const {
    return boost::shared_ptr<FileBlock>(new FileBlock(fileFormatVersion(),
						     eventTree_.tree(),
						     eventTree_.metaTree(),
						     lumiTree_.tree(),
						     lumiTree_.metaTree(),
						     runTree_.tree(),
						     runTree_.metaTree(),
						     whyNotFastClonable(),
						     file_,
						     branchChildren_));
  }

  std::string const&
  RootFile::newBranchToOldBranch(std::string const& newBranch) const {
    std::map<std::string, std::string>::const_iterator it = newBranchToOldBranch_.find(newBranch);
    if(it != newBranchToOldBranch_.end()) {
      return it->second;
    }
    return newBranch;
  }

  IndexIntoFile::IndexIntoFileItr
  RootFile::indexIntoFileIter() const {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    return indexIntoFileIter_;
  }

  bool
  RootFile::skipThisEntry() {
    if(indexIntoFileIter_ == indexIntoFileEnd_) {
        return false;
    }
    if (eventSkipperByID_ && eventSkipperByID_->somethingToSkip()) {
      // See first if the entire lumi is skipped, so we won't have to read the event Auxiliary in that case.
      bool skipTheLumi = eventSkipperByID_->skipIt(indexIntoFileIter_.run(), indexIntoFileIter_.lumi(), 0U);
      if (skipTheLumi) {
        return true;
      }
      // The Lumi is not skipped.  If this is an event, see if the event is skipped.
      if (indexIntoFileIter_.getEntryType() == IndexIntoFile::kEvent) {
        fillEventAuxiliary();
        return(eventSkipperByID_->skipIt(indexIntoFileIter_.run(),
					 indexIntoFileIter_.lumi(),
					 eventAux_.id().event()));
      }
    }
    return false;
  }

  IndexIntoFile::EntryType
  RootFile::getEntryTypeWithSkipping() {
    while(skipThisEntry()) {
      ++indexIntoFileIter_;
    }
    if(indexIntoFileIter_ == indexIntoFileEnd_) {
      return IndexIntoFile::kEnd;
    }
    return indexIntoFileIter_.getEntryType();
  }

  bool
  RootFile::isDuplicateEvent() {
    assert (indexIntoFileIter_.getEntryType() == IndexIntoFile::kEvent);
    if (duplicateChecker_.get() == 0) {
      return false;
    }
    fillEventAuxiliary();
    return duplicateChecker_->isDuplicateAndCheckActive(indexIntoFileIter_.processHistoryIDIndex(),
	indexIntoFileIter_.run(), indexIntoFileIter_.lumi(), eventAux_.id().event(), file_);
  }

  IndexIntoFile::EntryType
  RootFile::getNextEntryTypeWanted() {
    IndexIntoFile::EntryType entryType = getEntryTypeWithSkipping();
    if(entryType == IndexIntoFile::kEnd) {
      return IndexIntoFile::kEnd;
    }
    if(entryType == IndexIntoFile::kRun) {
      return IndexIntoFile::kRun;
    } else if(processingMode_ == InputSource::Runs) {
      indexIntoFileIter_.advanceToNextRun();
      return getNextEntryTypeWanted();
    }
    if(entryType == IndexIntoFile::kLumi) {
      return IndexIntoFile::kLumi;
    } else if(processingMode_ == InputSource::RunsAndLumis) {
      indexIntoFileIter_.advanceToNextLumiOrRun();
      return getNextEntryTypeWanted();
    }
    if(isDuplicateEvent()) {
      ++indexIntoFileIter_;
      return getNextEntryTypeWanted();
    }
    return IndexIntoFile::kEvent;
  }

  namespace {
    typedef IndexIntoFile::EntryNumber_t  EntryNumber_t;
    struct RunItem {
      RunItem(ProcessHistoryID const& phid, RunNumber_t const& run) :
	phid_(phid), run_(run) {}
      ProcessHistoryID phid_;
      RunNumber_t run_;
    };
    struct RunItemSortByRun {
      bool operator()(RunItem const& a, RunItem const& b) const {
	return a.run_ < b.run_;
      }
    };
    struct RunItemSortByRunPhid {
      bool operator()(RunItem const& a, RunItem const& b) const {
	return a.run_ < b.run_ || (!(b.run_ < a.run_) && a.phid_ < b.phid_);
      }
    };
    struct LumiItem {
      LumiItem(ProcessHistoryID const& phid, RunNumber_t const& run,
		 LuminosityBlockNumber_t const& lumi, EntryNumber_t const& entry) :
	phid_(phid), run_(run), lumi_(lumi), firstEventEntry_(entry),
	lastEventEntry_(entry == -1LL ? -1LL : entry + 1) {}
      ProcessHistoryID phid_;
      RunNumber_t run_;
      LuminosityBlockNumber_t lumi_;
      EntryNumber_t firstEventEntry_;
      EntryNumber_t lastEventEntry_;
    };
    struct LumiItemSortByRunLumi {
      bool operator()(LumiItem const& a, LumiItem const& b) const {
	return a.run_ < b.run_ || (!(b.run_ < a.run_) && a.lumi_ < b.lumi_);
      }
    };
    struct LumiItemSortByRunLumiPhid {
      bool operator()(LumiItem const& a, LumiItem const& b) const {
	if (a.run_ < b.run_) return true;
	if (b.run_ < a.run_) return false;
	if (a.lumi_ < b.lumi_) return true;
	if (b.lumi_ < a.lumi_) return false;
	return a.phid_ < b.phid_;
      }
    };
  }

  void
  RootFile::fillIndexIntoFile() {
    // This function is for backward compatibility only.
    // If reading a current format file, indexIntoFile_ is read from the input file.
    //

    typedef std::list<LumiItem> LumiList;
    LumiList lumis; // (declare 1)

    typedef std::set<LuminosityBlockID> RunLumiSet;
    RunLumiSet runLumiSet; // (declare 2)

    typedef std::list<RunItem> RunList;
    RunList runs; // (declare 5)

    typedef std::set<RunNumber_t> RunSet;
    RunSet runSet; // (declare 4)

    typedef std::set<RunItem, RunItemSortByRunPhid> RunItemSet;
    RunItemSet runItemSet; // (declare 3)

    RunNumber_t prevRun = 0;
    LuminosityBlockNumber_t prevLumi = 0;
    ProcessHistoryID prevPhid;

    // First, loop through the event tree.
    while(eventTree_.next()) {
      bool newRun = false;
      bool newLumi = false;
      fillThisEventAuxiliary();
      fillHistory();
      if (prevPhid != history_->processHistoryID() || prevRun != eventAux().run()) {
	newRun = newLumi = true;
      } else if(prevLumi != eventAux().luminosityBlock()) {
	newLumi = true;
      }
      prevPhid = history_->processHistoryID();
      prevRun = eventAux().run();
      prevLumi = eventAux().luminosityBlock();
      if (newLumi) {
	lumis.push_back(LumiItem(history_->processHistoryID(),
          eventAux().run(), eventAux().luminosityBlock(), eventTree_.entryNumber())); // (insert 1)
	runLumiSet.insert(LuminosityBlockID(eventAux().run(), eventAux().luminosityBlock())); // (insert 2)
      } else {
	LumiItem& currentLumi = lumis.back();
	assert(currentLumi.lastEventEntry_ == eventTree_.entryNumber());
	++currentLumi.lastEventEntry_;
      }
      if (newRun) {
	// Insert run in list if it is not already there.
	RunItem item(history_->processHistoryID(), eventAux().run());
	if (runItemSet.insert(item).second) { // (check 3, insert 3)
	  runs.push_back(RunItem(history_->processHistoryID(), eventAux().run())); // (insert 5)
	  runSet.insert(eventAux().run()); // (insert 4)
	}
      }
    }
    // now clean up.
    eventTree_.setEntryNumber(-1);
    eventAux_ = EventAuxiliary();

    // Loop over luminosity block entries and fill information.

    typedef std::vector<LumiItem> LumiVector;
    LumiVector emptyLumis; // (declare 7)

    typedef std::vector<RunItem> RunVector;
    RunVector emptyRuns; // (declare 12)

    typedef std::map<LuminosityBlockID, EntryNumber_t> RunLumiMap;
    RunLumiMap runLumiMap; // (declare 6)

    if(lumiTree_.isValid()) {
      while(lumiTree_.next()) {
	// Note: adjacent duplicates will be skipped without an explicit check.
        boost::shared_ptr<LuminosityBlockAuxiliary> lumiAux = fillLumiAuxiliary();
	LuminosityBlockID lumiID = LuminosityBlockID(lumiAux->run(), lumiAux->luminosityBlock());
	if (runLumiSet.insert(lumiID).second) { // (check 2, insert 2)
	  // This lumi was not assciated with any events.
	  emptyLumis.push_back(LumiItem(lumiAux->processHistoryID(), lumiAux->run(), lumiAux->luminosityBlock(), -1LL)); // (insert 7)
	  if (runSet.insert(lumiAux->run()).second) { // (check 4, insert 4)
	    // This run was not assciated with any events.
	    emptyRuns.push_back(RunItem(lumiAux->processHistoryID(), lumiAux->run())); // (insert 12)
	  }
	}
	runLumiMap.insert(std::make_pair(lumiID, lumiTree_.entryNumber()));
      }
      // now clean up.
      lumiTree_.setEntryNumber(-1);
    }

    // Loop over run entries and fill information.
    typedef std::map<RunNumber_t, EntryNumber_t> RunMap;
    RunMap runMap; // (declare 11)

    if(runTree_.isValid()) {
      while(runTree_.next()) {
	// Note: adjacent duplicates will be skipped without an explicit check.
        boost::shared_ptr<RunAuxiliary> runAux = fillRunAuxiliary();
	if (runSet.insert(runAux->run()).second) { // (check 4, insert 4)
	  // This run was not assciated with any events or lumis.
	  emptyRuns.push_back(RunItem(runAux->processHistoryID(), runAux->run())); // (insert 12)
	}
	runMap.insert(std::make_pair(runAux->run(), runTree_.entryNumber())); // (insert 11)
      }
      // now clean up.
      runTree_.setEntryNumber(-1);
    }

    // Insert the ordered empty lumis into the lumi list.
    LumiItemSortByRunLumi lumiItemSortByRunLumi;
    stable_sort_all(emptyLumis, lumiItemSortByRunLumi);

    LumiList::iterator itLumis = lumis.begin(), endLumis = lumis.end();
    for (LumiVector::const_iterator i = emptyLumis.begin(), iEnd = emptyLumis.end(); i != iEnd; ++i) {
      for (; itLumis != endLumis; ++itLumis) {
	if (lumiItemSortByRunLumi(*i, *itLumis)) {
	  break;
	}
      }
      lumis.insert(itLumis, *i);
    }

    // Insert the ordered empty runs into the run list.
    RunItemSortByRun runItemSortByRun;
    stable_sort_all(emptyRuns, runItemSortByRun);

    RunList::iterator itRuns = runs.begin(), endRuns = runs.end();
    for (RunVector::const_iterator i = emptyRuns.begin(), iEnd = emptyRuns.end(); i != iEnd; ++i) {
      for (; itRuns != endRuns; ++itRuns) {
	if (runItemSortByRun(*i, *itRuns)) {
	  break;
	}
      }
      runs.insert(itRuns, *i);
    }


    // Create a map of RunItems that gives the order of first appearance in the list.
    // Also fill in the vector of process history IDs
    typedef std::map<RunItem, int, RunItemSortByRunPhid> RunCountMap;
    RunCountMap runCountMap; // Declare (17)
    std::vector<ProcessHistoryID>& phids = indexIntoFile_.setProcessHistoryIDs();
    assert(phids.empty());
    std::vector<IndexIntoFile::RunOrLumiEntry>& entries = indexIntoFile_.setRunOrLumiEntries();
    assert(entries.empty());
    int rcount = 0;
    for (RunList::iterator it = runs.begin(), itEnd = runs.end(); it != itEnd; ++it) {
      RunCountMap::const_iterator countMapItem = runCountMap.find(*it);
      if (countMapItem == runCountMap.end()) {
	countMapItem = runCountMap.insert(std::make_pair(*it, rcount)).first; // Insert (17)
        assert(countMapItem != runCountMap.end());
	++rcount;
      }
      std::vector<ProcessHistoryID>::const_iterator phidItem = find_in_all(phids, it->phid_);
      if (phidItem == phids.end()) {
	phids.push_back(it->phid_);
        phidItem = phids.end() - 1;
      }
      entries.push_back(IndexIntoFile::RunOrLumiEntry(
	countMapItem->second, // use (17)
	-1LL,
	runMap[it->run_], // use (11)
	phidItem - phids.begin(),
	it->run_,
	0U,
	-1LL,
	-1LL));
    }

    // Create a map of LumiItems that gives the order of first appearance in the list.
    typedef std::map<LumiItem, int, LumiItemSortByRunLumiPhid> LumiCountMap;
    LumiCountMap lumiCountMap; // Declare (19)
    int lcount = 0;
    for (LumiList::iterator it = lumis.begin(), itEnd = lumis.end(); it != itEnd; ++it) {
      RunCountMap::const_iterator runCountMapItem = runCountMap.find(RunItem(it->phid_, it->run_));
      assert(runCountMapItem != runCountMap.end());
      LumiCountMap::const_iterator countMapItem = lumiCountMap.find(*it);
      if (countMapItem == lumiCountMap.end()) {
	countMapItem = lumiCountMap.insert(std::make_pair(*it, lcount)).first; // Insert (17)
        assert(countMapItem != lumiCountMap.end());
	++lcount;
      }
      std::vector<ProcessHistoryID>::const_iterator phidItem = find_in_all(phids, it->phid_);
      assert(phidItem != phids.end());
      entries.push_back(IndexIntoFile::RunOrLumiEntry(
	runCountMapItem->second,
	countMapItem->second,
	runLumiMap[LuminosityBlockID(it->run_, it->lumi_)],
	phidItem - phids.begin(),
	it->run_,
	it->lumi_,
	it->firstEventEntry_,
	it->lastEventEntry_));
    }
    stable_sort_all(entries);
  }

  void
  RootFile::validateFile(bool secondaryFile) {
    if(!fid_.isValid()) {
      fid_ = FileID(createGlobalIdentifier());
    }
    if(!eventTree_.isValid()) {
      throw edm::Exception(errors::EventCorruption) <<
	 "'Events' tree is corrupted or not present\n" << "in the input file.\n";
    }

    if (indexIntoFile_.empty()) {
      fillIndexIntoFile();
    }

    indexIntoFile_.fixIndexes(orderedProcessHistoryIDs_);
    indexIntoFile_.setNumberOfEvents(eventTree_.entries());
    indexIntoFile_.setEventFinder(boost::shared_ptr<IndexIntoFile::EventFinder>(new RootFileEventFinder(eventTree_)));
    // We fill the event numbers explicitly if we need to find events in closed files,
    // such as for secondary files (or secondary sources) or if duplicate checking across files.
    if (secondaryFile || (duplicateChecker_ && duplicateChecker_->checkingAllFiles())) {
      indexIntoFile_.fillEventNumbers();
    }
  }

  void
  RootFile::reportOpened(std::string const& inputType) {
    // Report file opened.
    std::string const label = "source";
    std::string moduleName = "PoolSource";
    std::string catalogName; // No more catalog
    Service<JobReport> reportSvc;
    reportToken_ = reportSvc->inputFileOpened(file_,
               logicalFile_,
               catalogName,
               inputType,
               moduleName,
               label,
	       fid_.fid(),
               eventTree_.branchNames());
  }

  void
  RootFile::close(bool reallyClose) {
    if(reallyClose) {
      // Just to play it safe, zero all pointers to objects in the TFile to be closed.
      eventHistoryTree_ = 0;
      for(RootTreePtrArray::iterator it = treePointers_.begin(), itEnd = treePointers_.end(); it != itEnd; ++it) {
	(*it)->close();
	(*it) = 0;
      }
      filePtr_->Close();
      filePtr_.reset();
    }
    Service<JobReport> reportSvc;
    reportSvc->inputFileClosed(reportToken_);
  }

  void
  RootFile::fillThisEventAuxiliary() {
    if (lastEventEntryNumberRead_ == eventTree_.entryNumber()) {
      // Already read.
      return;
    }
    if(fileFormatVersion().newAuxiliary()) {
      EventAuxiliary *pEvAux = &eventAux_;
      eventTree_.fillAux<EventAuxiliary>(pEvAux);
    } else {
      // for backward compatibility.
      EventAux eventAux;
      EventAux *pEvAux = &eventAux;
      eventTree_.fillAux<EventAux>(pEvAux);
      conversion(eventAux, eventAux_);
    }
    lastEventEntryNumberRead_ = eventTree_.entryNumber();
  }

  void
  RootFile::fillEventAuxiliary() {
    eventTree_.setEntryNumber(indexIntoFileIter_.entry());
    fillThisEventAuxiliary();
  }

  void
  RootFile::fillHistory() {
    // We could consider doing delayed reading, but because we have to
    // store this History object in a different tree than the event
    // data tree, this is too hard to do in this first version.

    if(fileFormatVersion().eventHistoryTree()) {
      History* pHistory = history_.get();
      TBranch* eventHistoryBranch = eventHistoryTree_->GetBranch(poolNames::eventHistoryBranchName().c_str());
      if(!eventHistoryBranch)
	throw edm::Exception(errors::EventCorruption)
	  << "Failed to find history branch in event history tree.\n";
      eventHistoryBranch->SetAddress(&pHistory);
      input::getEntry(eventHistoryTree_, eventTree_.entryNumber());
    } else {
      // for backward compatibility.
      if(eventProcessHistoryIDs_.empty()) {
	history_->setProcessHistoryID(eventAux().processHistoryID());
      } else {
	// Lumi block number was not in EventID for the relevant releases.
        EventID id(eventAux().id().run(), 0, eventAux().id().event());
        if(eventProcessHistoryIter_->eventID() != id) {
          EventProcessHistoryID target(id, ProcessHistoryID());
          eventProcessHistoryIter_ = lower_bound_all(eventProcessHistoryIDs_, target);
          assert(eventProcessHistoryIter_->eventID() == id);
        }
	history_->setProcessHistoryID(eventProcessHistoryIter_->processHistoryID());
        ++eventProcessHistoryIter_;
      }
    }
    if(provenanceAdaptor_) {
      history_->setProcessHistoryID(provenanceAdaptor_->convertID(history_->processHistoryID()));
      EventSelectionIDVector& esids = history_->eventSelectionIDs();
      for(EventSelectionIDVector::iterator i = esids.begin(), e = esids.end(); i != e; ++i) {
	(*i) = provenanceAdaptor_->convertID(*i);
      }
    }
    if(!fileFormatVersion().splitProductIDs()) {
      // old format.  branchListIndexes_ must be filled in from the ProvenanceAdaptor.
      provenanceAdaptor_->branchListIndexes(history_->branchListIndexes());
    }
  }

  boost::shared_ptr<LuminosityBlockAuxiliary>
  RootFile::fillLumiAuxiliary() {
    boost::shared_ptr<LuminosityBlockAuxiliary> lumiAuxiliary(new LuminosityBlockAuxiliary);
    if(fileFormatVersion().newAuxiliary()) {
      LuminosityBlockAuxiliary *pLumiAux = lumiAuxiliary.get();
      lumiTree_.fillAux<LuminosityBlockAuxiliary>(pLumiAux);
    } else {
      LuminosityBlockAux lumiAux;
      LuminosityBlockAux *pLumiAux = &lumiAux;
      lumiTree_.fillAux<LuminosityBlockAux>(pLumiAux);
      conversion(lumiAux, *lumiAuxiliary);
    }
    if(provenanceAdaptor_) {
      lumiAuxiliary->setProcessHistoryID(provenanceAdaptor_->convertID(lumiAuxiliary->processHistoryID()));
    }
    if(lumiAuxiliary->luminosityBlock() == 0 && !fileFormatVersion().runsAndLumis()) {
      lumiAuxiliary->id() = LuminosityBlockID(RunNumber_t(1), LuminosityBlockNumber_t(1));
    }
    return lumiAuxiliary;
  }

  boost::shared_ptr<RunAuxiliary>
  RootFile::fillRunAuxiliary() {
    boost::shared_ptr<RunAuxiliary> runAuxiliary(new RunAuxiliary);
    if(fileFormatVersion().newAuxiliary()) {
      RunAuxiliary *pRunAux = runAuxiliary.get();
      runTree_.fillAux<RunAuxiliary>(pRunAux);
    } else {
      RunAux runAux;
      RunAux *pRunAux = &runAux;
      runTree_.fillAux<RunAux>(pRunAux);
      conversion(runAux, *runAuxiliary);
    }
    if(provenanceAdaptor_) {
      runAuxiliary->setProcessHistoryID(provenanceAdaptor_->convertID(runAuxiliary->processHistoryID()));
    }
    return runAuxiliary;
  }

  bool
  RootFile::skipEvents(int& offset) {
    while (offset > 0 && indexIntoFileIter_ != indexIntoFileEnd_) {

      int phIndexOfSkippedEvent = IndexIntoFile::invalidIndex;
      RunNumber_t runOfSkippedEvent = IndexIntoFile::invalidRun;
      LuminosityBlockNumber_t lumiOfSkippedEvent = IndexIntoFile::invalidLumi;
      IndexIntoFile::EntryNumber_t skippedEventEntry = IndexIntoFile::invalidEntry;

      indexIntoFileIter_.skipEventForward(phIndexOfSkippedEvent,
                                          runOfSkippedEvent,
                                          lumiOfSkippedEvent,
                                          skippedEventEntry);

      // At the end of the file and there were no more events to skip
      if (skippedEventEntry == IndexIntoFile::invalidEntry) break;

      if (eventSkipperByID_ && eventSkipperByID_->somethingToSkip()) {
        eventTree_.setEntryNumber(skippedEventEntry);
        fillThisEventAuxiliary();
	if (eventSkipperByID_->skipIt(runOfSkippedEvent, lumiOfSkippedEvent, eventAux_.id().event())) {
	    continue;
        }
      }
      if(duplicateChecker_ &&
         !duplicateChecker_->checkDisabled() &&
         !duplicateChecker_->noDuplicatesInFile()) {

        eventTree_.setEntryNumber(skippedEventEntry);
        fillThisEventAuxiliary();
        if (duplicateChecker_->isDuplicateAndCheckActive(phIndexOfSkippedEvent,
                                                         runOfSkippedEvent,
                                                         lumiOfSkippedEvent,
                                                         eventAux_.id().event(),
                                                         file_)) {
          continue;
        }
      }
      --offset;
    }

    /*
    while(offset < 0 && indexIntoFileIter_ != indexIntoFileBegin_) {
      --indexIntoFileIter_;
      if(indexIntoFileIter_->getEntryType() == IndexIntoFile::kEvent) {
        if (eventSkipperByID_ && eventSkipperByID_->somethingToSkip()) {
	  fillEventAuxiliary();
	  if (eventSkipperByID_->skipIt(indexIntoFileIter_->run(), indexIntoFileIter_->lumi(), eventAux_.id().event())) {
	    continue;
          }
        }
	// if(isDuplicateEvent()) {
	//  continue;
	// }
        ++offset;
      }
    }
    */
    eventTree_.resetTraining();

    return (indexIntoFileIter_ == indexIntoFileEnd_);
  }

  // readEvent() is responsible for creating, and setting up, the
  // EventPrincipal.
  //
  //   1. create an EventPrincipal with a unique EventID
  //   2. For each entry in the provenance, put in one Group,
  //      holding the Provenance for the corresponding EDProduct.
  //   3. set up the caches in the EventPrincipal to know about this
  //      Group.
  //
  // We do *not* create the EDProduct instance (the equivalent of reading
  // the branch containing this EDProduct. That will be done by the Delayed Reader,
  //  when it is asked to do so.
  //
  EventPrincipal*
  RootFile::readEvent(EventPrincipal& cache, boost::shared_ptr<LuminosityBlockPrincipal> lb) {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    assert(indexIntoFileIter_.getEntryType() == IndexIntoFile::kEvent);
    // Set the entry in the tree, and read the event at that entry.
    eventTree_.setEntryNumber(indexIntoFileIter_.entry());
    EventPrincipal* ep = readCurrentEvent(cache, lb);

    assert(ep != 0);
    assert(eventAux().run() == indexIntoFileIter_.run() + forcedRunOffset_);
    assert(eventAux().luminosityBlock() == indexIntoFileIter_.lumi());

    ++indexIntoFileIter_;
    return ep;
  }

  // Reads event at the current entry in the file index.
  EventPrincipal*
  RootFile::readCurrentEvent(EventPrincipal& cache, boost::shared_ptr<LuminosityBlockPrincipal> lb) {
    eventTree_.setEntryNumber(indexIntoFileIter_.entry());
    if(!eventTree_.current()) {
      return 0;
    }
    fillThisEventAuxiliary();
    if(!fileFormatVersion().lumiInEventID()) {
	//ugly, but will disappear when the backward compatibility is done with schema evolution.
	const_cast<EventID&>(eventAux_.id()).setLuminosityBlockNumber(eventAux_.oldLuminosityBlock());
	eventAux_.resetObsoleteInfo();
    }
    fillHistory();
    overrideRunNumber(eventAux_.id(), eventAux().isRealData());

    std::auto_ptr<EventAuxiliary> aux(new EventAuxiliary(eventAux()));
    // We're not done ... so prepare the EventPrincipal
    cache.fillEventPrincipal(aux,
			     lb,
			     history_,
			     makeBranchMapper(eventTree_, InEvent),
			     eventTree_.makeDelayedReader(fileFormatVersion()));

    // report event read from file
    Service<JobReport> reportSvc;
    reportSvc->eventReadFromFile(reportToken_, eventID().run(), eventID().event());
    return &cache;
  }

  void
  RootFile::setAtEventEntry(IndexIntoFile::EntryNumber_t entry) {
    eventTree_.setEntryNumber(entry);
  }

  boost::shared_ptr<RunAuxiliary>
  RootFile::readRunAuxiliary_() {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    assert(indexIntoFileIter_.getEntryType() == IndexIntoFile::kRun);
    // Begin code for backward compatibility before the existence of run trees.
    if(!runTree_.isValid()) {
      // prior to the support of run trees.
      // RunAuxiliary did not contain a valid timestamp.  Take it from the next event.
      if(eventTree_.next()) {
        fillThisEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree_.previous();
      }
      RunID run = RunID(indexIntoFileIter_.run());
      overrideRunNumber(run);
      return boost::shared_ptr<RunAuxiliary>(new RunAuxiliary(run.run(), eventAux().time(), Timestamp::invalidTimestamp()));
    }
    // End code for backward compatibility before the existence of run trees.
    runTree_.setEntryNumber(indexIntoFileIter_.entry());
    boost::shared_ptr<RunAuxiliary> runAuxiliary = fillRunAuxiliary();
    assert(runAuxiliary->run() == indexIntoFileIter_.run());
    overrideRunNumber(runAuxiliary->id());
    Service<JobReport> reportSvc;
    reportSvc->reportInputRunNumber(runAuxiliary->run());
    if(runAuxiliary->beginTime() == Timestamp::invalidTimestamp()) {
      // RunAuxiliary did not contain a valid timestamp.  Take it from the next event.
      if(eventTree_.next()) {
        fillThisEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree_.previous();
      }
      runAuxiliary->setBeginTime(eventAux().time());
      runAuxiliary->setEndTime(Timestamp::invalidTimestamp());
    }
    ProcessHistoryID phid = indexIntoFile_.processHistoryID(indexIntoFileIter_.processHistoryIDIndex());
    if(fileFormatVersion().processHistorySameWithinRun()) {
      assert(runAuxiliary->processHistoryID() == phid);
    } else {
      runAuxiliary->setProcessHistoryID(phid);
    }
    return runAuxiliary;
  }

  boost::shared_ptr<RunPrincipal>
  RootFile::readRun_(boost::shared_ptr<RunPrincipal> rpCache) {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    assert(indexIntoFileIter_.getEntryType() == IndexIntoFile::kRun);
    // Begin code for backward compatibility before the existence of run trees.
    if(!runTree_.isValid()) {
      ++indexIntoFileIter_;
      return rpCache;
    }
    // End code for backward compatibility before the existence of run trees.
    rpCache->fillRunPrincipal(makeBranchMapper(runTree_, InRun), runTree_.makeDelayedReader(fileFormatVersion()));
    // Read in all the products now.
    rpCache->readImmediate();
    ++indexIntoFileIter_;
    return rpCache;
  }

  boost::shared_ptr<LuminosityBlockAuxiliary>
  RootFile::readLuminosityBlockAuxiliary_() {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    assert(indexIntoFileIter_.getEntryType() == IndexIntoFile::kLumi);
    // Begin code for backward compatibility before the existence of lumi trees.
    if(!lumiTree_.isValid()) {
      if(eventTree_.next()) {
        fillThisEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree_.previous();
      }

      LuminosityBlockID lumi = LuminosityBlockID(indexIntoFileIter_.run(), indexIntoFileIter_.lumi());
      overrideRunNumber(lumi);
      return boost::shared_ptr<LuminosityBlockAuxiliary>(new LuminosityBlockAuxiliary(lumi.run(), lumi.luminosityBlock(), eventAux().time(), Timestamp::invalidTimestamp()));
    }
    // End code for backward compatibility before the existence of lumi trees.
    lumiTree_.setEntryNumber(indexIntoFileIter_.entry());
    boost::shared_ptr<LuminosityBlockAuxiliary> lumiAuxiliary = fillLumiAuxiliary();
    assert(lumiAuxiliary->run() == indexIntoFileIter_.run());
    assert(lumiAuxiliary->luminosityBlock() == indexIntoFileIter_.lumi());
    overrideRunNumber(lumiAuxiliary->id());
    Service<JobReport> reportSvc;
    reportSvc->reportInputLumiSection(lumiAuxiliary->run(), lumiAuxiliary->luminosityBlock());

    if(lumiAuxiliary->beginTime() == Timestamp::invalidTimestamp()) {
      // LuminosityBlockAuxiliary did not contain a timestamp. Take it from the next event.
      if(eventTree_.next()) {
        fillThisEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree_.previous();
      }
      lumiAuxiliary->setBeginTime(eventAux().time());
      lumiAuxiliary->setEndTime(Timestamp::invalidTimestamp());
    }
    if(fileFormatVersion().processHistorySameWithinRun()) {
      ProcessHistoryID phid = indexIntoFile_.processHistoryID(indexIntoFileIter_.processHistoryIDIndex());
      assert(lumiAuxiliary->processHistoryID() == phid);
    } else {
      ProcessHistoryID phid = indexIntoFile_.processHistoryID(indexIntoFileIter_.processHistoryIDIndex());
      lumiAuxiliary->setProcessHistoryID(phid);
    }
    return lumiAuxiliary;
  }

  boost::shared_ptr<LuminosityBlockPrincipal>
  RootFile::readLumi(boost::shared_ptr<LuminosityBlockPrincipal> lbCache) {
    assert(indexIntoFileIter_ != indexIntoFileEnd_);
    assert(indexIntoFileIter_.getEntryType() == IndexIntoFile::kLumi);
    // Begin code for backward compatibility before the existence of lumi trees.
    if(!lumiTree_.isValid()) {
      ++indexIntoFileIter_;
      return lbCache;
    }
    // End code for backward compatibility before the existence of lumi trees.
    lumiTree_.setEntryNumber(indexIntoFileIter_.entry());
    lbCache->fillLuminosityBlockPrincipal(makeBranchMapper(lumiTree_, InLumi),
					 lumiTree_.makeDelayedReader(fileFormatVersion()));
    // Read in all the products now.
    lbCache->readImmediate();
    ++indexIntoFileIter_;
    return lbCache;
  }

  bool
  RootFile::setEntryAtEvent(RunNumber_t run, LuminosityBlockNumber_t lumi, EventNumber_t event) {
    indexIntoFileIter_ = indexIntoFile_.findEventPosition(run, lumi, event);
    if(indexIntoFileIter_ == indexIntoFileEnd_) return false;
    eventTree_.setEntryNumber(indexIntoFileIter_.entry());
    return true;
  }

  bool
  RootFile::setEntryAtLumi(RunNumber_t run, LuminosityBlockNumber_t lumi) {
    indexIntoFileIter_ = indexIntoFile_.findLumiPosition(run, lumi);
    if(indexIntoFileIter_ == indexIntoFileEnd_) return false;
    lumiTree_.setEntryNumber(indexIntoFileIter_.entry());
    return true;
  }

  bool
  RootFile::setEntryAtRun(RunNumber_t run) {
    indexIntoFileIter_ = indexIntoFile_.findRunPosition(run);
    if(indexIntoFileIter_ == indexIntoFileEnd_) return false;
    runTree_.setEntryNumber(indexIntoFileIter_.entry());
    return true;
  }

  void
  RootFile::overrideRunNumber(RunID& id) {
    if(forcedRunOffset_ != 0) {
      id = RunID(id.run() + forcedRunOffset_);
    }
    if(id < RunID::firstValidRun()) id = RunID::firstValidRun();
  }

  void
  RootFile::overrideRunNumber(LuminosityBlockID& id) {
    if(forcedRunOffset_ != 0) {
      id = LuminosityBlockID(id.run() + forcedRunOffset_, id.luminosityBlock());
    }
    if(RunID(id.run()) < RunID::firstValidRun()) id = LuminosityBlockID(RunID::firstValidRun().run(), id.luminosityBlock());
  }

  void
  RootFile::overrideRunNumber(EventID& id, bool isRealData) {
    if(forcedRunOffset_ != 0) {
      if(isRealData) {
        throw edm::Exception(errors::Configuration,"RootFile::RootFile()")
          << "The 'setRunNumber' parameter of PoolSource cannot be used with real data.\n";
      }
      id = EventID(id.run() + forcedRunOffset_, id.luminosityBlock(), id.event());
    }
    if(RunID(id.run()) < RunID::firstValidRun()) {
      id = EventID(RunID::firstValidRun().run(), LuminosityBlockID::firstValidLuminosityBlock().luminosityBlock(),id.event());
    }
  }


  void
  RootFile::readEventHistoryTree() {
    // Read in the event history tree, if we have one...
    if(fileFormatVersion().eventHistoryTree()) {
      eventHistoryTree_ = dynamic_cast<TTree*>(filePtr_->Get(poolNames::eventHistoryTreeName().c_str()));
      if(!eventHistoryTree_) {
        throw edm::Exception(errors::EventCorruption)
	  << "Failed to find the event history tree.\n";
      }
    }
  }

  void
  RootFile::initializeDuplicateChecker(
    std::vector<boost::shared_ptr<IndexIntoFile> > const& indexesIntoFiles,
    std::vector<boost::shared_ptr<IndexIntoFile> >::size_type currentIndexIntoFile) {
    if(duplicateChecker_) {
      if(eventTree_.next()) {
        fillThisEventAuxiliary();
        duplicateChecker_->inputFileOpened(eventAux().isRealData(),
                                           indexIntoFile_,
                                           indexesIntoFiles,
                                           currentIndexIntoFile);
      }
      eventTree_.setEntryNumber(-1);
    }
  }

  void
  RootFile::dropOnInput (ProductRegistry& reg, GroupSelectorRules const& rules, bool dropDescendants, bool secondaryFile) {
    // This is the selector for drop on input.
    GroupSelector groupSelector;
    groupSelector.initialize(rules, reg.allBranchDescriptions());

    ProductRegistry::ProductList& prodList = reg.productListUpdator();
    // Do drop on input. On the first pass, just fill in a set of branches to be dropped.
    std::set<BranchID> branchesToDrop;
    for(ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
        it != itEnd; ++it) {
      BranchDescription const& prod = it->second;
      if(!groupSelector.selected(prod)) {
        if(dropDescendants) {
          branchChildren_->appendToDescendants(prod.branchID(), branchesToDrop);
        } else {
          branchesToDrop.insert(prod.branchID());
        }
      }
    }

    // On this pass, actually drop the branches.
    std::set<BranchID>::const_iterator branchesToDropEnd = branchesToDrop.end();
    for(ProductRegistry::ProductList::iterator it = prodList.begin(), itEnd = prodList.end(); it != itEnd;) {
      BranchDescription const& prod = it->second;
      bool drop = branchesToDrop.find(prod.branchID()) != branchesToDropEnd;
      if(drop) {
	if(groupSelector.selected(prod)) {
          LogWarning("RootFile")
            << "Branch '" << prod.branchName() << "' is being dropped from the input\n"
            << "of file '" << file_ << "' because it is dependent on a branch\n"
            << "that was explicitly dropped.\n";
	}
        treePointers_[prod.branchType()]->dropBranch(newBranchToOldBranch(prod.branchName()));
        ProductRegistry::ProductList::iterator icopy = it;
        ++it;
        prodList.erase(icopy);
      } else {
        ++it;
      }
    }

    // Drop on input mergeable run and lumi products, this needs to be invoked for secondary file input
    if(secondaryFile) {
      for(ProductRegistry::ProductList::iterator it = prodList.begin(), itEnd = prodList.end(); it != itEnd;) {
        BranchDescription const& prod = it->second;
        if(prod.branchType() != InEvent) {
          TClass *cp = gROOT->GetClass(prod.wrappedName().c_str());
          boost::shared_ptr<EDProduct> dummy(static_cast<EDProduct *>(cp->New()));
          if(dummy->isMergeable()) {
            treePointers_[prod.branchType()]->dropBranch(newBranchToOldBranch(prod.branchName()));
            ProductRegistry::ProductList::iterator icopy = it;
            ++it;
            prodList.erase(icopy);
          } else {
            ++it;
          }
        }
        else ++it;
      }
    }
  }

  // backward compatibility

  namespace {
    boost::shared_ptr<BranchMapper>
    makeBranchMapperInRelease180(RootTree& rootTree, BranchType const& type, ProductRegistry const& preg) {
      boost::shared_ptr<BranchMapperWithReader> mapper(new BranchMapperWithReader());
      mapper->setDelayedRead(false);

      for(ProductRegistry::ProductList::const_iterator it = preg.productList().begin(),
          itEnd = preg.productList().end(); it != itEnd; ++it) {
        if(type == it->second.branchType() && !it->second.transient()) {
	  TBranch *br = rootTree.branches().find(it->first)->second.provenanceBranch_;
	  std::auto_ptr<BranchEntryDescription> pb(new BranchEntryDescription);
	  BranchEntryDescription* ppb = pb.get();
	  br->SetAddress(&ppb);
	  input::getEntry(br, rootTree.entryNumber());
	  ProductStatus status = (ppb->creatorStatus() == BranchEntryDescription::Success ? productstatus::present() : productstatus::neverCreated());
	  // Not providing parentage!!!
	  ProductProvenance entry(it->second.branchID(), status, ParentageID());
	  mapper->insert(entry);
	  mapper->insertIntoMap(it->second.oldProductID(), it->second.branchID());
        }
      }
      return mapper;
    }

    boost::shared_ptr<BranchMapper>
    makeBranchMapperInRelease200(RootTree& rootTree, BranchType const& type, ProductRegistry const& preg) {
      rootTree.fillStatus();
      boost::shared_ptr<BranchMapperWithReader> mapper(new BranchMapperWithReader());
      mapper->setDelayedRead(false);
      for(ProductRegistry::ProductList::const_iterator it = preg.productList().begin(),
          itEnd = preg.productList().end(); it != itEnd; ++it) {
        if(type == it->second.branchType() && !it->second.transient()) {
	  std::vector<ProductStatus>::size_type index = it->second.oldProductID().oldID() - 1;
	  // Not providing parentage!!!
	  ProductProvenance entry(it->second.branchID(), rootTree.productStatuses()[index], ParentageID());
	  mapper->insert(entry);
	  mapper->insertIntoMap(it->second.oldProductID(), it->second.branchID());
        }
      }
      return mapper;
    }

    boost::shared_ptr<BranchMapper>
    makeBranchMapperInRelease210(RootTree& rootTree, BranchType const& type) {
      boost::shared_ptr<BranchMapperWithReader> mapper(new BranchMapperWithReader());
      mapper->setDelayedRead(false);
      if(type == InEvent) {
	std::auto_ptr<std::vector<EventEntryInfo> > infoVector(new std::vector<EventEntryInfo>);
	std::vector<EventEntryInfo> *pInfoVector = infoVector.get();
        rootTree.branchEntryInfoBranch()->SetAddress(&pInfoVector);
        setRefCoreStreamer(0, true, false);
        input::getEntry(rootTree.branchEntryInfoBranch(), rootTree.entryNumber());
        setRefCoreStreamer(true);
	for(std::vector<EventEntryInfo>::const_iterator it = infoVector->begin(), itEnd = infoVector->end();
	    it != itEnd; ++it) {
	  EventEntryDescription eed;
	  EntryDescriptionRegistry::instance()->getMapped(it->entryDescriptionID(), eed);
	  Parentage parentage(eed.parents());
	  ProductProvenance entry(it->branchID(), it->productStatus(), parentage.id());
	  mapper->insert(entry);
	  mapper->insertIntoMap(it->productID(), it->branchID());
	}
      } else {
	std::auto_ptr<std::vector<RunLumiEntryInfo> > infoVector(new std::vector<RunLumiEntryInfo>);
	std::vector<RunLumiEntryInfo> *pInfoVector = infoVector.get();
        rootTree.branchEntryInfoBranch()->SetAddress(&pInfoVector);
        setRefCoreStreamer(0, true, false);
        input::getEntry(rootTree.branchEntryInfoBranch(), rootTree.entryNumber());
        setRefCoreStreamer(true);
        for(std::vector<RunLumiEntryInfo>::const_iterator it = infoVector->begin(), itEnd = infoVector->end();
            it != itEnd; ++it) {
	  ProductProvenance entry(it->branchID(), it->productStatus(), ParentageID());
          mapper->insert(entry);
        }
      }
      return mapper;
    }

    boost::shared_ptr<BranchMapper>
    makeBranchMapperInRelease300(RootTree& rootTree) {
      boost::shared_ptr<BranchMapperWithReader> mapper(
	new BranchMapperWithReader(rootTree.branchEntryInfoBranch(), rootTree.entryNumber()));
      mapper->setDelayedRead(true);
      return mapper;
    }
  }

  boost::shared_ptr<BranchMapper>
  RootFile::makeBranchMapper(RootTree& rootTree, BranchType const& type) const {
    if(fileFormatVersion().splitProductIDs()) {
      return makeBranchMapperInRelease300(rootTree);
    } else if(fileFormatVersion().perEventProductIDs()) {
      return makeBranchMapperInRelease210(rootTree, type);
    } else if(fileFormatVersion().eventHistoryTree()) {
      return makeBranchMapperInRelease200(rootTree, type, *productRegistry_);
    } else {
      return makeBranchMapperInRelease180(rootTree, type, *productRegistry_);
    }
  }
  // end backward compatibility
}
