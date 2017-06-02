/*----------------------------------------------------------------------
$Id: PoolSource.cc,v 1.24 2006/03/15 21:22:54 wmtan Exp $
----------------------------------------------------------------------*/

#include "IOPool/Input/src/PoolSource.h"
#include "IOPool/Input/src/RootFile.h"
#include "IOPool/Common/interface/ClassFiller.h"

#include "DataFormats/Common/interface/BranchDescription.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "DataFormats/Common/interface/ProductRegistry.h"
#include "DataFormats/Common/interface/ProductID.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/Registry.h"

namespace edm {
  PoolRASource::PoolRASource(ParameterSet const& pset, InputSourceDescription const& desc) :
    VectorInputSource(pset, desc),
    catalog_(PoolCatalog::READ, pset.getUntrackedParameter("catalog", std::string())),
    fileIter_(fileNames().begin()),
    rootFile_(),
    rootFiles_(),
    mainInput_(pset.getParameter<std::string>("@module_label") == std::string("@main_input"))
  {
    ClassFiller();
    init(*fileIter_);
    if (mainInput_) {
      updateProductRegistry();
    }
  }

  void PoolRASource::init(std::string const& file) {

    // For the moment, we keep all old files open.
    // FIX: We will need to limit the number of open files.
    // rootFiles[file]_.reset();

    RootFileMap::const_iterator it = rootFiles_.find(file);
    if (it == rootFiles_.end()) {
      std::string pfn;
      catalog_.findFile(pfn, file);
      rootFile_ = RootFileSharedPtr(new RootFile(pfn));
      rootFiles_.insert(std::make_pair(file, rootFile_));
      if (mainInput_) {
        rootFile_->fillParameterSetRegistry(*pset::Registry::instance());
      }
    } else {
      rootFile_ = it->second;
      rootFile_->setEntryNumber(-1);
    }
  }

  void PoolRASource::updateProductRegistry() const {
    if (rootFile_->productRegistry().nextID() > productRegistry().nextID()) {
      productRegistry().setNextID(rootFile_->productRegistry().nextID());
    }
    ProductRegistry::ProductList const& prodList = rootFile_->productRegistry().productList();
    for (ProductRegistry::ProductList::const_iterator it = prodList.begin();
	it != prodList.end(); ++it) {
      productRegistry().copyProduct(it->second);
    }
  }

  bool PoolRASource::next() {
    if(rootFile_->next()) return true;
    ++fileIter_;
    if(fileIter_ == fileNames().end()) {
      if (mainInput_) {
	return false;
      } else {
	fileIter_ = fileNames().begin();
      }
    }

    // save the product registry from the current file, temporarily
    boost::shared_ptr<ProductRegistry const> pReg(rootFile_->productRegistrySharedPtr());

    init(*fileIter_);

    // make sure the new product registry is identical to the old one
    if (*pReg != rootFile_->productRegistry()) {
      throw cms::Exception("MismatchedInput","PoolSource::next()")
	<< "File " << *fileIter_ << "\nhas different product registry than previous files\n";
    }
    return next();
  }

  bool PoolRASource::previous() {
    if(rootFile_->previous()) return true;
    if(fileIter_ == fileNames().begin()) {
      if (mainInput_) {
	return false;
      } else {
	fileIter_ = fileNames().end();
      }
    }
    --fileIter_;

    // save the product registry from the current file, temporarily
    boost::shared_ptr<ProductRegistry const> pReg(rootFile_->productRegistrySharedPtr());

    init(*fileIter_);

    // make sure the new product registry is identical to the old one
    if (*pReg != rootFile_->productRegistry()) {
      throw cms::Exception("MismatchedInput","PoolSource::previous()")
	<< "File " << *fileIter_ << "\nhas different product registry than previous files\n";
    }
    rootFile_->setEntryNumber(rootFile_->entries());
    return previous();
  }

  PoolRASource::~PoolRASource() {
  }

  // readOneEvent() is responsible for creating, and setting up, the
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
  PoolRASource::readOneEvent() {
    if (!next()) {
      if (!mainInput_) {
	repeat();
      }
      return std::auto_ptr<EventPrincipal>(0);
    }
    return rootFile_->read(mainInput_ ? productRegistry() : rootFile_->productRegistry()); 
  }

  std::auto_ptr<EventPrincipal>
  PoolRASource::readIt(EventID const& id) {
    RootFile::EntryNumber entry = rootFile_->getEntryNumber(id);
    if (entry >= 0) {
      rootFile_->setEntryNumber(entry - 1);
      return read();
    } else {
      return std::auto_ptr<EventPrincipal>(0);
    }
  }

  // Advance "offset" events. Entry numbers begin at 0.
  // The current entry number is the last one read, not the next one read.
  // The current entry number may be -1, if none have been read yet.
  void
  PoolRASource::skip(int offset) {
    EntryNumber newEntry = rootFile_->entryNumber() + offset;
    if (newEntry >= rootFile_->entries()) {

      // We must go to the next file
      // Calculate how much we will advance in this file,
      // including one for the next() call below
      int increment = rootFile_->entries() - rootFile_->entryNumber();    

      // Set the entry to the last entry in this file
      rootFile_->setEntryNumber(rootFile_->entries() -1);

      // Advance to the first entry of the next file, if there is a next file.
      if(!next()) return;

      // Now skip the remaining offset.
      skip(offset - increment);

    } else if (newEntry < -1) {

      // We must go to the previous file
      // Calculate how much we will back up in this file,
      // including one for the previous() call below
      int decrement = rootFile_->entryNumber() + 1;    

      // Set the entry to the first entry in this file
      rootFile_->setEntryNumber(0);

      // Back up to the last entry of the previous file, if there is a previous file.
      if(!previous()) return;

      // Now skip the remaining offset.
      skip(offset + decrement);
    } else {
      // The same file.
      rootFile_->setEntryNumber(newEntry);
    }
  }

  void
  PoolRASource::readMany_(int number, EventPrincipalVector& result) {
    for (int i = 0; i < number; ++i) {
      std::auto_ptr<EventPrincipal> ev = read();
      if (ev.get() == 0) {
	return;
      }
      EventPrincipalVectorElement e(ev.release());
      result.push_back(e);
    }
  }
}
