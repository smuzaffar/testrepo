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
// $Id: BranchMapReader.h,v 1.9 2010/01/28 15:45:19 ewv Exp $
//

// system include files
#include <memory>
#include "TUUID.h"

// user include files
#include "DataFormats/Provenance/interface/BranchDescription.h"

// forward declarations
class TFile;
class TTree;
class TBranch;

namespace fwlite {
  namespace internal {
    class BMRStrategy {
    public:
      BMRStrategy(TFile* file, int fileVersion);
      virtual ~BMRStrategy();

      virtual bool updateFile(TFile* file) = 0;
      virtual bool updateEvent(Long_t eventEntry) = 0;
      virtual bool updateLuminosityBlock(Long_t luminosityBlockEntry) = 0;
      virtual bool updateMap() = 0;
      virtual edm::BranchID productToBranchID(const edm::ProductID& pid) = 0;
      virtual const edm::BranchDescription productToBranch(const edm::ProductID& pid) = 0;
      virtual const std::vector<edm::BranchDescription>& getBranchDescriptions() = 0;

      TFile* currentFile_;
      TTree* eventTree_;
      TTree* luminosityBlockTree_;
      TUUID fileUUID_;
      Long_t eventEntry_;
      Long_t luminosityBlockEntry_;
      int fileVersion_;
    };
  }

  class BranchMapReader {
  public:
    BranchMapReader(TFile* file);
    BranchMapReader() : strategy_(0) {}

      // ---------- const member functions ---------------------

      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
    bool updateFile(TFile* file);
    bool updateEvent(Long_t eventEntry);
    bool updateLuminosityBlock(Long_t luminosityBlockEntry);
    const edm::BranchDescription productToBranch(const edm::ProductID& pid);
    int getFileVersion(TFile* file);
    int getFileVersion() const { return  fileVersion_;}

    TFile* getFile() const { return strategy_->currentFile_; }
    TTree* getEventTree() const { return strategy_->eventTree_; }
    TTree* getLuminosityBlockTree() const { return strategy_->luminosityBlockTree_; }
    TUUID getFileUUID() const { return strategy_->fileUUID_; }
    Long_t getEventEntry() const { return strategy_->eventEntry_; }
    Long_t getLuminosityBlockEntry() const { return strategy_->luminosityBlockEntry_; }
    const std::vector<edm::BranchDescription>& getBranchDescriptions();

      // ---------- member data --------------------------------
  private:
    std::auto_ptr<internal::BMRStrategy> newStrategy(TFile* file, int fileVersion);
    std::auto_ptr<internal::BMRStrategy> strategy_;
    int fileVersion_;
  };
}

#endif
