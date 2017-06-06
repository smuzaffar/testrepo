/*----------------------------------------------------------------------
$Id: RootFile.cc,v 1.89 2007/10/09 17:46:36 wmtan Exp $
----------------------------------------------------------------------*/

#include "RootFile.h"


#include "IOPool/Common/interface/FileIdentifier.h"
#include "DataFormats/Provenance/interface/BranchDescription.h"
#include "DataFormats/Provenance/interface/BranchType.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Framework/interface/LuminosityBlockPrincipal.h"
#include "FWCore/Framework/interface/RunPrincipal.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"
#include "DataFormats/Provenance/interface/ParameterSetBlob.h"
#include "DataFormats/Provenance/interface/ModuleDescriptionRegistry.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/RunID.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"
//used for friendlyName translation
#include "FWCore/Utilities/interface/FriendlyName.h"

//used for backward compatibility
#include "DataFormats/Provenance/interface/EventAux.h"
#include "DataFormats/Provenance/interface/LuminosityBlockAux.h"
#include "DataFormats/Provenance/interface/RunAux.h"

#include "TFile.h"
#include "TTree.h"
#include "Rtypes.h"

namespace edm {
//---------------------------------------------------------------------
  RootFile::RootFile(std::string const& fileName,
		     std::string const& catalogName,
		     ProcessConfiguration const& processConfiguration,
		     std::string const& logicalFileName) :
      file_(fileName),
      logicalFile_(logicalFileName),
      catalog_(catalogName),
      processConfiguration_(processConfiguration),
      filePtr_(file_.empty() ? 0 : TFile::Open(file_.c_str())),
      fileFormatVersion_(),
      reportToken_(0),
      eventAux_(),
      lumiAux_(),
      runAux_(),
      eventTree_(filePtr_, InEvent),
      lumiTree_(filePtr_, InLumi),
      runTree_(filePtr_, InRun),
      treePointers_(),
      productRegistry_(),
      forcedRunNumber_(0),
      forcedRunNumberOffset_(0) {
    treePointers_[InEvent] = &eventTree_;
    treePointers_[InLumi]  = &lumiTree_;
    treePointers_[InRun]   = &runTree_;

    open();

    // Set up buffers for registries.
    // Need to read to a temporary registry so we can do a translation of the BranchKeys.
    // This preserves backward compatibility against friendly class name algorithm changes.
    ProductRegistry tempReg;
    ProductRegistry *ppReg = &tempReg;
    typedef std::map<ParameterSetID, ParameterSetBlob> PsetMap;
    PsetMap psetMap;
    ProcessHistoryMap pHistMap;
    ModuleDescriptionMap mdMap;
    PsetMap *psetMapPtr = &psetMap;
    ProcessHistoryMap *pHistMapPtr = &pHistMap;
    ModuleDescriptionMap *mdMapPtr = &mdMap;
    FileFormatVersion *fftPtr = &fileFormatVersion_;
    FileID *fidPtr = &fid_;

    // Read the metadata tree.
    TTree *metaDataTree = dynamic_cast<TTree *>(filePtr_->Get(poolNames::metaDataTreeName().c_str()));
    assert(metaDataTree != 0);

    metaDataTree->SetBranchAddress(poolNames::productDescriptionBranchName().c_str(),(&ppReg));
    metaDataTree->SetBranchAddress(poolNames::parameterSetMapBranchName().c_str(), &psetMapPtr);
    metaDataTree->SetBranchAddress(poolNames::processHistoryMapBranchName().c_str(), &pHistMapPtr);
    metaDataTree->SetBranchAddress(poolNames::moduleDescriptionMapBranchName().c_str(), &mdMapPtr);
    metaDataTree->SetBranchAddress(poolNames::fileFormatVersionBranchName().c_str(), &fftPtr);
    if (metaDataTree->FindBranch(poolNames::fileIdentifierBranchName().c_str()) != 0) {
      metaDataTree->SetBranchAddress(poolNames::fileIdentifierBranchName().c_str(), &fidPtr);
    }

    metaDataTree->GetEntry(0);

    validateFile();

    // freeze our temporary product registry
    tempReg.setFrozen();

    ProductRegistry *newReg = new ProductRegistry;
    // Do the translation from the old registry to the new one
    std::map<std::string,std::string> newBranchToOldBranch;
    {
      ProductRegistry::ProductList const& prodList = tempReg.productList();
      for (ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
           it != itEnd; ++it) {
        BranchDescription const& prod = it->second;
        std::string newFriendlyName = friendlyname::friendlyName(prod.className());
	if (newFriendlyName == prod.friendlyClassName_) {
	  prod.init();
          newReg->addProduct(prod);
	  newBranchToOldBranch[prod.branchName()] = prod.branchName();
	} else {
          BranchDescription newBD(prod);
          newBD.friendlyClassName_ = newFriendlyName;
	  newBD.init();
          newReg->addProduct(newBD);
	  // Need to call init to get old branch name.
	  prod.init();
	  newBranchToOldBranch[newBD.branchName()] = prod.branchName();
	}
      }
      // freeze the product registry
      newReg->setFrozen();
      productRegistry_ = boost::shared_ptr<ProductRegistry const>(newReg);
    }

    // Merge into the registries. For now, we do NOT merge the product registry.
    pset::Registry& psetRegistry = *pset::Registry::instance();
    for (PsetMap::const_iterator i = psetMap.begin(), iEnd = psetMap.end(); i != iEnd; ++i) {
      psetRegistry.insertMapped(ParameterSet(i->second.pset_));
    } 
    ProcessHistoryRegistry & processNameListRegistry = *ProcessHistoryRegistry::instance();
    for (ProcessHistoryMap::const_iterator j = pHistMap.begin(), jEnd = pHistMap.end(); j != jEnd; ++j) {
      processNameListRegistry.insertMapped(j->second);
    } 
    ModuleDescriptionRegistry & moduleDescriptionRegistry = *ModuleDescriptionRegistry::instance();
    for (ModuleDescriptionMap::const_iterator k = mdMap.begin(), kEnd = mdMap.end(); k != kEnd; ++k) {
      moduleDescriptionRegistry.insertMapped(k->second);
    } 

    // Set up information from the product registry.
    ProductRegistry::ProductList const& prodList = productRegistry()->productList();
    for (ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
        it != itEnd; ++it) {
      BranchDescription const& prod = it->second;
      treePointers_[prod.branchType()]->addBranch(it->first, prod,
						 newBranchToOldBranch[prod.branchName()]);
    }
  }

  RootFile::~RootFile() {
  }

  void RootFile::validateFile() {
    if (!fileFormatVersion_.isValid()) {
      fileFormatVersion_.value_ = 0;
    }
    if (!fid_.isValid()) {
      fid_ = FileID(createFileIdentifier());
    }
    assert(eventTree().isValid());
    if (fileFormatVersion_.value_ >= 3) {
      eventTree().checkAndFixIndex();
      if (lumiTree().isValid()) {
        lumiTree().checkAndFixIndex();
      }
      if (runTree().isValid()) {
        runTree().checkAndFixIndex();
      }
      // assert(lumiTree().isValid());
      // assert(runTree().isValid());
    }
  }

  void
  RootFile::open() {
    if (filePtr_ == 0) {
      throw cms::Exception("FileNotFound","RootFile::RootFile()")
        << "File " << file_ << " was not found or could not be opened.\n";
    }
    // Report file opened.
    std::string const label = "source";
    std::string moduleName = "PoolSource";
    Service<JobReport> reportSvc;
    reportToken_ = reportSvc->inputFileOpened(file_,
               logicalFile_,
               catalog_,
               moduleName,
               label,
               eventTree().branchNames()); 
  }

  void
  RootFile::close(bool reallyClose) {
    if (reallyClose) {
      filePtr_->Close();
    }
    Service<JobReport> reportSvc;
    reportSvc->inputFileClosed(reportToken_);
  }

  void
  RootFile::fillEventAuxiliary() {
    if (fileFormatVersion_.value_ >= 3) {
      EventAuxiliary *pEvAux = &eventAux_;
      eventTree().fillAux<EventAuxiliary>(pEvAux);
    } else {
      // for backward compatibility.
      EventAux eventAux;
      EventAux *pEvAux = &eventAux;
      eventTree().fillAux<EventAux>(pEvAux);
      conversion(eventAux, eventAux_);
      if (eventAux_.luminosityBlock_ == 0 || fileFormatVersion_.value_ <= 1) {
        eventAux_.luminosityBlock_ = 1;
      }
    }
    overrideRunNumber(eventAux_.id_, eventAux_.isRealData());
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
  std::auto_ptr<EventPrincipal>
  RootFile::readEvent(boost::shared_ptr<ProductRegistry const> pReg, boost::shared_ptr<LuminosityBlockPrincipal> lbp) {
    if (!eventTree().next()) {
      return std::auto_ptr<EventPrincipal>(0);
    }
    fillEventAuxiliary();

    if (lbp.get() == 0) {
	boost::shared_ptr<RunPrincipal> rp(
	  new RunPrincipal(eventAux_.run(), eventAux_.time(), eventAux_.time(), pReg, processConfiguration_));
	lbp = boost::shared_ptr<LuminosityBlockPrincipal>(
	  new LuminosityBlockPrincipal(eventAux_.luminosityBlock(),
				       eventAux_.time(),
				       eventAux_.time(),
				       pReg,
				       rp,
				       processConfiguration_));
    }

    if (eventAux_.run() != lbp->runNumber() ||
	eventAux_.luminosityBlock() != lbp->luminosityBlock()) {
      // The event is in a different run or lumi block.  Back up, and return a null pointer.
      eventTree().previous();
      return std::auto_ptr<EventPrincipal>(0);
    }
    // We're not done ... so prepare the EventPrincipal
    std::auto_ptr<EventPrincipal> thisEvent(new EventPrincipal(
                eventID(),
		eventAux_.time(), pReg,
		lbp, processConfiguration_,
		eventAux_.isRealData(),
		eventAux_.experimentType(),
		eventAux_.bunchCrossing(),
                eventAux_.storeNumber(),
		eventAux_.processHistoryID_,
		eventTree().makeDelayedReader()));

    // Create a group in the event for each product
    eventTree().fillGroups(thisEvent->groupGetter());

    // report event read from file
    Service<JobReport> reportSvc;
    reportSvc->eventReadFromFile(reportToken_, eventID().run(), eventID().event());
    return thisEvent;
  }

  boost::shared_ptr<RunPrincipal>
  RootFile::readRun(boost::shared_ptr<ProductRegistry const> pReg) {
    if (!runTree().isValid()) {
      // prior to the support of run trees, the run number must be retrieved from the next event.
      if (!eventTree().next()) {
        return boost::shared_ptr<RunPrincipal>();
      }
      EventAux eventAux;
      EventAux *pEvAux = &eventAux;
      eventTree().fillAux<EventAux>(pEvAux);
      overrideRunNumber(eventAux.id_, false);
      // back up, so event will not be skipped.
      eventTree().previous();
      return boost::shared_ptr<RunPrincipal>(
          new RunPrincipal(eventAux.id_.run(),
	  eventAux.time_,
	  Timestamp::invalidTimestamp(), pReg,
	  processConfiguration_));
    }
    if (!runTree().next()) {
      return boost::shared_ptr<RunPrincipal>();
    }
    if (fileFormatVersion_.value_ >= 3) {
      RunAuxiliary *pRunAux = &runAux_;
      runTree().fillAux<RunAuxiliary>(pRunAux);
    } else {
      RunAux runAux;
      RunAux *pRunAux = &runAux;
      runTree().fillAux<RunAux>(pRunAux);
      conversion(runAux, runAux_);
    } 
    overrideRunNumber(runAux_.id_);
    if (runAux_.beginTime() == Timestamp::invalidTimestamp()) {
      // RunAuxiliary did not contain a valid timestamp.  Take it from the next event.
      if (eventTree().next()) {
        fillEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree().previous();
      }
      runAux_.beginTime_ = eventAux_.time(); 
      runAux_.endTime_ = Timestamp::invalidTimestamp();
    }
    boost::shared_ptr<RunPrincipal> thisRun(
	new RunPrincipal(runAux_.run(),
			 runAux_.beginTime(),
			 runAux_.endTime(),
			 pReg,
			 processConfiguration_,
			 runAux_.processHistoryID_,
			 runTree().makeDelayedReader()));
    // Create a group in the run for each product
    runTree().fillGroups(thisRun->groupGetter());
    return thisRun;
  }

  boost::shared_ptr<LuminosityBlockPrincipal>
  RootFile::readLumi(boost::shared_ptr<ProductRegistry const> pReg, boost::shared_ptr<RunPrincipal> rp) {
    if (!lumiTree().isValid()) {
      // prior to the support of lumi trees, the run number must be retrieved from the next event.
      if (!eventTree().next()) {
        return boost::shared_ptr<LuminosityBlockPrincipal>();
      }
      EventAux eventAux;
      EventAux *pEvAux = &eventAux;
      eventTree().fillAux<EventAux>(pEvAux);
      overrideRunNumber(eventAux.id_, false);
      // back up, so event will not be skipped.
      eventTree().previous();
      if (eventAux.id_.run() != rp->run()) {
        // The next event is in a different run.  Return a null pointer.
        return boost::shared_ptr<LuminosityBlockPrincipal>();
      }
      // Prior to support of lumi blocks, always use 1 for lumi block number.
      return boost::shared_ptr<LuminosityBlockPrincipal>(
	new LuminosityBlockPrincipal(1,
				     eventAux.time_,
				     Timestamp::invalidTimestamp(),
				     pReg,
				     rp,
				     processConfiguration_));
    }
    if (!lumiTree().next()) {
      return boost::shared_ptr<LuminosityBlockPrincipal>();
    }
    if (fileFormatVersion_.value_ >= 3) {
      LuminosityBlockAuxiliary *pLumiAux = &lumiAux_;
      lumiTree().fillAux<LuminosityBlockAuxiliary>(pLumiAux);
    } else {
      LuminosityBlockAux lumiAux;
      LuminosityBlockAux *pLumiAux = &lumiAux;
      lumiTree().fillAux<LuminosityBlockAux>(pLumiAux);
      conversion(lumiAux, lumiAux_);
    }
    overrideRunNumber(lumiAux_.id_);

    if (lumiAux_.run() != rp->run()) {
      // The lumi block is in a different run.  Back up, and return a null pointer.
      lumiTree().previous();
      return boost::shared_ptr<LuminosityBlockPrincipal>();
    }
    if (lumiAux_.beginTime() == Timestamp::invalidTimestamp()) {
      // LuminosityBlockAuxiliary did not contain a timestamp. Take it from the next event.
      if (eventTree().next()) {
        fillEventAuxiliary();
        // back up, so event will not be skipped.
        eventTree().previous();
      }
      lumiAux_.beginTime_ = eventAux_.time();
      lumiAux_.endTime_ = Timestamp::invalidTimestamp();
    }
    boost::shared_ptr<LuminosityBlockPrincipal> thisLumi(
	new LuminosityBlockPrincipal(lumiAux_.luminosityBlock(),
				     lumiAux_.beginTime(),
				     lumiAux_.endTime(),
				     pReg, rp, processConfiguration_,
				     lumiAux_.processHistoryID_,
				     lumiTree().makeDelayedReader()));
    // Create a group in the lumi for each product
    lumiTree().fillGroups(thisLumi->groupGetter());
    return thisLumi;
  }

  void
  RootFile::overrideRunNumber(RunID & id) {
    if (forcedRunNumber_ != 0) {
       forcedRunNumberOffset_ = forcedRunNumber_ - id.run();
       forcedRunNumber_ = 0;
    }
    if (forcedRunNumberOffset_ != 0) {
      id = RunID(id.run() + forcedRunNumberOffset_);
    } 
    if (id.run() == 0) id = RunID::firstValidRun();
  }
  void
  RootFile::overrideRunNumber(LuminosityBlockID & id) {
    if (forcedRunNumberOffset_ != 0) {
      id = LuminosityBlockID(id.run() + forcedRunNumberOffset_, id.luminosityBlock());
    } 
    if (id.run() == 0) id = LuminosityBlockID(RunID::firstValidRun().run(), id.luminosityBlock());
  }
  void
  RootFile::overrideRunNumber(EventID & id, bool isRealData) {
    if (forcedRunNumberOffset_ != 0) {
      if (isRealData) {
        throw cms::Exception("Configuration","RootFile::RootFile()")
          << "The 'setRunNumber' parameter of PoolSource cannot be used with real data.\n";
      }
      id = EventID(id.run() + forcedRunNumberOffset_, id.event());
    } 
    if (id.run() == 0) id = EventID(RunID::firstValidRun().run(), id.event());
  }
}
