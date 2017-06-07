#ifndef FWLite_BranchMapReader_h
#define FWLite_BranchMapReader_h
// -*- C++ -*-
//
// Package:     FWLite
// Class  :     BranchMapReader
// 
/**\class BranchMapReader BranchMapReader.h FWCore/FWLite/interface/BranchMapReader.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  Dan Riley
//         Created:  Tue May 20 10:31:32 EDT 2008
// $Id: BranchMapReader.h,v 1.4 2008/09/23 20:40:04 dsr Exp $
//

// system include files
#include <map>
//#include "boost/shared_ptr.hpp"
#include "TUUID.h"

// user include files
#include "DataFormats/Provenance/interface/BranchMapper.h"
#include "DataFormats/Provenance/interface/BranchDescription.h"
#include "DataFormats/Provenance/interface/EventEntryInfo.h"

// forward declarations
class TFile;
class TTree;
class TBranch;

namespace fwlite {

  class BranchMapReader {
  public:
    typedef edm::BranchMapper eeiMap;
    typedef std::map<edm::BranchID, edm::BranchDescription> bidToDesc;

    BranchMapReader(TFile* file);
    BranchMapReader() : strategy_(0) {}

    class Strategy {
    public:
      Strategy(TFile* file, int fileVersion, eeiMap& eventInfoMap, bidToDesc& branchDescriptionMap);
      virtual ~Strategy();
      virtual bool updateFile(TFile* file);
      virtual bool updateEvent(Long_t eventEntry) { eventEntry_ = eventEntry; return true; }
      virtual bool updateMap() { return true; }
      
      TBranch* getBranchRegistry(edm::ProductRegistry** pReg);
      
      TFile* currentFile_;
      TTree* eventTree_;
      TUUID fileUUID_;
      int fileVersion_;
      Long_t eventEntry_;
      eeiMap& eventInfoMap_;
      bidToDesc& branchDescriptionMap_;
      bool mapperFilled_;
    };

      // ---------- const member functions ---------------------
      
      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
    bool updateFile(TFile* file);
    bool updateEvent(Long_t eventEntry);
    const edm::BranchDescription productToBranch(const edm::ProductID& pid) const {
      strategy_->updateMap();
      edm::BranchID bid = eventInfoMap_.productToBranch(pid);
      bidToDesc::const_iterator bdi = branchDescriptionMap_.find(bid);
      if (branchDescriptionMap_.end() == bdi) {
        return edm::BranchDescription();
      }
      return bdi->second;
    }
    int getFileVersion(TFile* file) const;

    TFile* getFile() const { return strategy_->currentFile_; }
    TTree* getEventTree() const { return strategy_->eventTree_; }
    TUUID getFileUUID() const { return strategy_->fileUUID_; }
    Long_t getEventEntry() const { return strategy_->eventEntry_; }
    const std::vector<edm::BranchDescription>& getBranchDescriptions();

      // ---------- member data --------------------------------
    private:
      std::auto_ptr<Strategy> newStrategy(TFile* file, int fileVersion);

      std::auto_ptr<Strategy> strategy_;
      eeiMap eventInfoMap_;
      bidToDesc branchDescriptionMap_;
      std::vector<edm::BranchDescription> bDesc_;
  };
}

#endif
