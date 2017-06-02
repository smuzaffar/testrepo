#ifndef Input_RootFile_h
#define Input_RootFile_h

/*----------------------------------------------------------------------

RootFile.h // used by ROOT input sources

$Id: RootFile.h,v 1.4 2006/03/14 23:33:01 wmtan Exp $

----------------------------------------------------------------------*/

#include <memory>
#include <string>

#include "IOPool/Input/src/Inputfwd.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "TBranch.h"
#include "TFile.h"

#include "boost/shared_ptr.hpp"

namespace edm {

  //------------------------------------------------------------
  // Class RootFile: supports file reading.

  class RootFile {
  public:
    typedef input::BranchMap BranchMap;
    typedef input::EntryNumber EntryNumber;
    typedef std::map<ProductID, BranchDescription> ProductMap;
    BranchMap const& branches() const {return branches_;}
    explicit RootFile(std::string const& fileName);
    ~RootFile();
    bool next() {return ++entryNumber_ < entries_;} 
    bool previous() {return --entryNumber_ >= 0;} 
    std::auto_ptr<EventPrincipal> read(ProductRegistry const& pReg);
    ProductRegistry const& productRegistry() const {return *productRegistry_;}
    boost::shared_ptr<ProductRegistry> productRegistrySharedPtr() const {return productRegistry_;}
    void fillParameterSetRegistry(pset::Registry & psetRegistry) const;
    TBranch *auxBranch() {return auxBranch_;}
    TBranch *provBranch() {return provBranch_;}
    EventID & eventID() {return eventID_;}
    EntryNumber const& entryNumber() const {return entryNumber_;}
    EntryNumber const& entries() const {return entries_;}
    void setEntryNumber(EntryNumber entryNumber) {entryNumber_ = entryNumber;}
    EntryNumber getEntryNumber(EventID const& eventID) const;

  private:
    RootFile(RootFile const&); // disable copy construction
    RootFile & operator=(RootFile const&); // disable assignment
    std::string const file_;
    EventID eventID_;
    EntryNumber entryNumber_;
    EntryNumber entries_;
    boost::shared_ptr<ProductRegistry> productRegistry_;
    BranchMap branches_;
    ProductMap productMap_;
// We use bare pointers for pointers to ROOT entities.
// Root owns them and uses bare pointers internally.
// Therefore,using shared pointers here will do no good.
    TTree *eventTree_;
    TBranch *auxBranch_;
    TBranch *provBranch_;
    TFile *filePtr_;
  }; // class RootFile


}
#endif
