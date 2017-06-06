#ifndef IOPool_Output_RootOutputFile_h
#define IOPool_Output_RootOutputFile_h

//////////////////////////////////////////////////////////////////////
//
// $Id: RootOutputFile.h,v 1.10 2007/09/13 16:29:50 paterno Exp $
//
// Class PoolOutputModule. Output module to POOL file
//
// Oringinal Author: Luca Lista
// Current Author: Bill Tanenbaum
//
//////////////////////////////////////////////////////////////////////

#include <memory>
#include <string>
#include <iosfwd>
#include <vector>
#include "boost/array.hpp"
#include "boost/shared_ptr.hpp"

#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/MessageLogger/interface/JobReport.h"
#include "DataFormats/Provenance/interface/BranchType.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"
#include "DataFormats/Provenance/interface/FileID.h"
#include "DataFormats/Provenance/interface/LuminosityBlockAuxiliary.h"
#include "DataFormats/Provenance/interface/RunAuxiliary.h"
#include "DataFormats/Provenance/interface/Selections.h"
#include "IOPool/Output/src/RootOutputTree.h"

class TTree;
class TFile;

namespace edm {
  class RootChains;
  class ParameterSet;
  class PoolOutputModule;

  class RootOutputFile {
  public:
    typedef boost::array<RootOutputTree *, NumBranchTypes> RootOutputTreePtrArray;
    explicit RootOutputFile(PoolOutputModule * om, std::string const& fileName, std::string const& logicalFileName);
    ~RootOutputFile() {}
    void writeOne(EventPrincipal const& e);
    //void endFile();
    void writeLuminosityBlock(LuminosityBlockPrincipal const& lb);
    bool writeRun(RunPrincipal const& r);

    void writeFileFormatVersion();
    void writeFileIdentifier();
    void writeProcessConfigurationRegistry();
    void writeProcessHistoryRegistry();
    void writeModuleDescriptionRegistry();
    void writeParameterSetRegistry();
    void writeProductDescriptionRegistry();
    void finishEndFile();

    bool isFileFull() const {return newFileAtEndOfRun_;}

  private:
    void buildIndex(TTree *tree, BranchType const& branchType);
    void setBranchAliases(TTree *tree, Selections const& branches) const;

  private:
    struct OutputItem {
      OutputItem() : branchDescription_(0), selected_(false) {}
      OutputItem(BranchDescription const* bd, bool sel) :
	branchDescription_(bd), selected_(sel), branchEntryDescription_(0), product_(0) {}
      ~OutputItem() {}
      BranchDescription const* branchDescription_;
      bool selected_;
      mutable BranchEntryDescription const* branchEntryDescription_;
      mutable void const* product_;
    };
    typedef std::vector<OutputItem> OutputItemList;
    typedef boost::array<OutputItemList, NumBranchTypes> OutputItemListArray;

    void fillBranches(BranchType const& branchType, Principal const& principal) const;

    RootChains const& chains_;
    OutputItemListArray outputItemList_;
    std::string file_;
    std::string logicalFile_;
    JobReport::Token reportToken_;
    unsigned int eventCount_;
    unsigned int fileSizeCheckEvent_;
    PoolOutputModule const* om_;
    boost::shared_ptr<TFile> filePtr_;
    FileID fid_;
    TTree * metaDataTree_;
    EventAuxiliary eventAux_;
    LuminosityBlockAuxiliary lumiAux_;
    RunAuxiliary runAux_;
    EventAuxiliary const* pEventAux_;
    LuminosityBlockAuxiliary const* pLumiAux_;
    RunAuxiliary const* pRunAux_;
    RootOutputTree eventTree_;
    RootOutputTree lumiTree_;
    RootOutputTree runTree_;
    RootOutputTreePtrArray treePointers_;
    boost::shared_ptr<ProductRegistry const> productRegistry_;
    mutable std::list<BranchEntryDescription> provenances_;
    mutable bool newFileAtEndOfRun_;
  };
}

#endif
