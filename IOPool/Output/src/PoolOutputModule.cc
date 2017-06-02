// $Id: PoolOutputModule.cc,v 1.24 2006/05/05 01:00:42 wmtan Exp $

#include "IOPool/Output/src/PoolOutputModule.h"
#include "IOPool/Common/interface/PoolDataSvc.h"
#include "IOPool/Common/interface/ClassFiller.h"
#include "IOPool/Common/interface/RefStreamer.h"

#include "DataFormats/Common/interface/BranchKey.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "DataFormats/Common/interface/EventProvenance.h"
#include "DataFormats/Common/interface/ProductRegistry.h"
#include "DataFormats/Common/interface/ParameterSetBlob.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/MessageLogger/interface/JobReport.h"

#include "DataSvc/Ref.h"
#include "DataSvc/IDataSvc.h"
#include "PersistencySvc/ITransaction.h"
#include "PersistencySvc/ISession.h"
#include "StorageSvc/DbType.h"

#include "TTree.h"
#include "TFile.h"

#include <vector>
#include <string>
#include <iomanip>

namespace edm {
  PoolOutputModule::PoolOutputModule(ParameterSet const& pset) :
    OutputModule(pset),
    catalog_(pset),
    context_(catalog_, false),
    fileName_(FileCatalog::toPhysical(pset.getUntrackedParameter<std::string>("fileName"))),
    logicalFileName_(pset.getUntrackedParameter<std::string>("logicalFileName", std::string())),
    commitInterval_(pset.getUntrackedParameter<unsigned int>("commitInterval", 100U)),
    maxFileSize_(pset.getUntrackedParameter<int>("maxSize", 0x7f000000)),
    moduleLabel_(pset.getParameter<std::string>("@module_label")),
    fileCount_(0),
    poolFile_() {
    // We need to set a custom streamer for edm::RefCore so that it will not be split.
    // even though a custom streamer is not otherwise necessary.
    ClassFiller();
    SetRefStreamer();
  }

  void PoolOutputModule::beginJob(EventSetup const&) {
    poolFile_ = boost::shared_ptr<PoolFile>(new PoolFile(this));
  }

  void PoolOutputModule::endJob() {
    poolFile_->endFile();
  }

  PoolOutputModule::~PoolOutputModule() {
  }

  void PoolOutputModule::write(EventPrincipal const& e) {
      if (poolFile_->writeOne(e)) {
	++fileCount_;
	poolFile_ = boost::shared_ptr<PoolFile>(new PoolFile(this));
      }
  }

  PoolOutputModule::PoolFile::PoolFile(PoolOutputModule *om) :
    outputItemList_(), branchNames_(),
      file_(), lfn_(),
      reportToken_(0), eventCount_(0),
      fileSizeCheckEvent_(100),
      provenancePlacement_(), auxiliaryPlacement_(), productDescriptionPlacement_(),
      om_(om) {
    std::string const suffix(".root");
    std::string::size_type offset = om->fileName_.rfind(suffix);
    bool ext = (offset == om->fileName_.size() - suffix.size());
    std::string fileBase(ext ? om->fileName_.substr(0, offset): om->fileName_);
    if (om->fileCount_) {
      std::ostringstream ofilename;
      ofilename << fileBase << std::setw(3) << std::setfill('0') << om->fileCount_ << suffix;
      file_ = ofilename.str();
      if (!om->logicalFileName_.empty()) {
	std::ostringstream lfilename;
	lfilename << om->logicalFileName_ << std::setw(3) << std::setfill('0') << om->fileCount_;
	lfn_ = lfilename.str();
      }
    } else {
      file_ = fileBase + suffix;
      lfn_ = om->logicalFileName_;
    }
    makePlacement(poolNames::eventTreeName(), poolNames::provenanceBranchName(), provenancePlacement_);
    makePlacement(poolNames::eventTreeName(), poolNames::auxiliaryBranchName(), auxiliaryPlacement_);
    makePlacement(poolNames::metaDataTreeName(), poolNames::productDescriptionBranchName(), productDescriptionPlacement_);
    makePlacement(poolNames::parameterSetTreeName(), poolNames::parameterSetIDBranchName(), parameterSetIDPlacement_);
    makePlacement(poolNames::parameterSetTreeName(), poolNames::parameterSetBranchName(), parameterSetPlacement_);
    ProductRegistry pReg;
    pReg.setNextID(om->nextID());
   
    for (Selections::const_iterator it = om->descVec_.begin();
      it != om->descVec_.end(); ++it) {
      pReg.copyProduct(**it);
      pool::Placement placement;
      makePlacement(poolNames::eventTreeName(), (*it)->branchName_, placement);
      outputItemList_.push_back(std::make_pair(*it, placement));
      branchNames_.push_back((*it)->branchName_);
    }
    std::vector<boost::shared_ptr<ParameterSetBlob> > psets;
    startTransaction();
    pool::Ref<ProductRegistry const> rp(om->context(), &pReg);
    rp.markWrite(productDescriptionPlacement_);
    pset::Registry const* psetRegistry = pset::Registry::instance();    
    for (pset::Registry::const_iterator it = psetRegistry->begin(); it != psetRegistry->end(); ++it) {
      pool::Ref<ParameterSetID const> rpsetid(om->context(), &it->first);
      boost::shared_ptr<ParameterSetBlob> pset(new ParameterSetBlob(it->second.toStringOfTracked()));
      psets.push_back(pset); // Keeps ParameterSetBlob alive until after the commit.
      pool::Ref<ParameterSetBlob const> rpset(om->context(), pset.get());
      rpset.markWrite(parameterSetPlacement_);
      rpsetid.markWrite(parameterSetIDPlacement_);
    }
    commitAndFlushTransaction();
    // Register the output file with the JobReport service
    // and get back the token for it.
    std::string moduleName = "PoolOutputModule";
    Service<JobReport> reportSvc;
    reportToken_ = reportSvc->outputFileOpened(
		      file_, lfn_,  // PFN and LFN
		      om->catalog_.url(),  // catalog
		      moduleName,   // module class name
		      om->moduleLabel_,  // module label
		      branchNames_); // branch names being written
    om->catalog_.registerFile(file_, lfn_);
  }

  void PoolOutputModule::PoolFile::startTransaction() const {
    context()->transaction().start(pool::ITransaction::UPDATE);
  }

  void PoolOutputModule::PoolFile::commitTransaction() const {
    context()->transaction().commitAndHold();
  }

  void PoolOutputModule::PoolFile::commitAndFlushTransaction() const {
    context()->transaction().commit();
  }

  void PoolOutputModule::PoolFile::makePlacement(std::string const& treeName_, std::string const& branchName_, pool::Placement& placement) {
    placement.setTechnology(pool::ROOTTREE_StorageType.type());
    placement.setDatabase(file_, pool::DatabaseSpecification::PFN);
    placement.setContainerName(poolNames::containerName(treeName_, branchName_));
  }

  bool PoolOutputModule::PoolFile::writeOne(EventPrincipal const& e) {
    ++eventCount_;
    startTransaction();
    // Write auxiliary branch
    EventAux aux;
    aux.process_history_ = e.processHistory();
    aux.id_ = e.id();
    aux.time_ = e.time();

    pool::Ref<const EventAux> ra(context(), &aux);
    ra.markWrite(auxiliaryPlacement_);	

    EventProvenance eventProvenance;
    // Loop over EDProduct branches, fill the provenance, and write the branch.
    for (OutputItemList::const_iterator i = outputItemList_.begin();
	 i != outputItemList_.end(); ++i) {
      ProductID const& id = i->first->productID_;

      if (id == ProductID()) {
	throw edm::Exception(edm::errors::ProductNotFound,"InvalidID")
	  << "PoolOutputModule::write: invalid ProductID supplied in productRegistry\n";
      }
      EventPrincipal::SharedGroupPtr const g = e.getGroup(id);
      if (g.get() == 0) {
	// No product with this ID is in the event.  Add a null one.
	BranchEntryDescription event;
	event.status = BranchEntryDescription::CreatorNotRun;
	event.productID_ = id;
	eventProvenance.data_.push_back(event);
	if (i->first->productPtr_.get() == 0) {
	   std::auto_ptr<EDProduct> edp(e.store()->get(BranchKey(*i->first), &e));
	   i->first->productPtr_ = boost::shared_ptr<EDProduct>(edp.release());
	   if (i->first->productPtr_.get() == 0) {
	    throw edm::Exception(edm::errors::ProductNotFound,"Invalid")
	      << "PoolOutputModule::write: invalid BranchDescription supplied in productRegistry\n";
	   }
	}
	pool::Ref<EDProduct const> ref(context(), i->first->productPtr_.get());
	ref.markWrite(i->second);
      } else {
	eventProvenance.data_.push_back(g->provenance().event);
	pool::Ref<EDProduct const> ref(context(), g->product());
	ref.markWrite(i->second);
      }
    }
    // Write the provenance branch
    pool::Ref<EventProvenance const> rp(context(), &eventProvenance);
    rp.markWrite(provenancePlacement_);

    commitTransaction();
    // Report event written 
    Service<JobReport> reportSvc;
    reportSvc->eventWrittenToFile(reportToken_, e.id());

    if (eventCount_ >= fileSizeCheckEvent_) {
	size_t size = om_->context_.getFileSize(file_);
	unsigned long eventSize = size/eventCount_;
	if (size + 2*eventSize >= om_->maxFileSize_) {
	  endFile();
	  return true;
	} else {
	  unsigned long increment = (om_->maxFileSize_ - size)/eventSize;
	  increment -= increment/8;	// Prevents overshoot
	  fileSizeCheckEvent_ = eventCount_ + increment;
	}
    }
    if (eventCount_ % om_->commitInterval_ == 0) {
      commitAndFlushTransaction();
      startTransaction();
    }
    return false;
  }

  void PoolOutputModule::PoolFile::endFile() {
    commitAndFlushTransaction();
    om_->catalog_.commitCatalog();
    context()->session().disconnectAll();
    setBranchAliases();
    // report that file has been closed
    Service<JobReport> reportSvc;
    reportSvc->outputFileClosed(reportToken_);
  }


  // For now, we must use root directly to set branch aliases, since there is no way to do this in POOL
  // We do this after POOL has closed the file.
  void
  PoolOutputModule::PoolFile::setBranchAliases() const {
    TFile f(file_.c_str(), "update");
    TTree *t = dynamic_cast<TTree *>(f.Get(poolNames::eventTreeName().c_str()));
    if (t) {
      t->BuildIndex("id_.run_", "id_.event_");
      for (Selections::const_iterator it = om_->descVec_.begin();
	it != om_->descVec_.end(); ++it) {
	BranchDescription const& pd = **it;
	std::string const& full = pd.branchName_ + "obj";
	std::string const& alias = (pd.branchAlias_.empty() ?
        (pd.productInstanceName_.empty() ? pd.module.moduleLabel_ : pd.productInstanceName_)
        : pd.branchAlias_);
	t->SetAlias(alias.c_str(), full.c_str());
      }
      t->Write(t->GetName(), TObject::kWriteDelete);
    }
    f.Purge();
    f.Close();
  }
}
