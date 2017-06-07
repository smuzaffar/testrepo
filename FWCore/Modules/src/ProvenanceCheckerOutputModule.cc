// -*- C++ -*-
//
// Package:     Modules
// Class  :     ProvenanceCheckerOutputModule
// 
// Implementation:
//     Checks the consistency of provenance stored in the framework
//
// Original Author:  Chris Jones
//         Created:  Thu Sep 11 19:24:13 EDT 2008
//

// system include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/OutputModule.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/EventPrincipal.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/Provenance/interface/ProductRegistry.h"

// user include files

namespace edm {
  class ParameterSet;
  class ProvenanceCheckerOutputModule : public OutputModule {
  public:
    // We do not take ownership of passed stream.
    explicit ProvenanceCheckerOutputModule(ParameterSet const& pset);
    virtual ~ProvenanceCheckerOutputModule();
      
  private:
    virtual void write(EventPrincipal const& e);
    virtual void writeLuminosityBlock(LuminosityBlockPrincipal const&){}
    virtual void writeRun(RunPrincipal const&){}
  };

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
  ProvenanceCheckerOutputModule::ProvenanceCheckerOutputModule(ParameterSet const& pset) : OutputModule(pset) {
  }

//ProvenanceCheckerOutputModule::ProvenanceCheckerOutputModule(const ProvenanceCheckerOutputModule& rhs) {
//    // do actual copying here;
//}

  ProvenanceCheckerOutputModule::~ProvenanceCheckerOutputModule() {
  }

//
// assignment operators
//
//ProvenanceCheckerOutputModule const& ProvenanceCheckerOutputModule::operator=(const ProvenanceCheckerOutputModule& rhs) {
//  //An exception safe implementation is
//  ProvenanceCheckerOutputModule temp(rhs);
//  swap(rhs);
//
//  return *this;
// }

//
// member functions
//
  static void markAncestors(ProductProvenance const& iInfo,
                            BranchMapper const& iMapper,
                            std::map<BranchID, bool>& oMap, 
                            std::set<BranchID>& oMapperMissing) {
    for(std::vector<BranchID>::const_iterator it = iInfo.parentage().parents().begin(),
          itEnd = iInfo.parentage().parents().end();
          it != itEnd;
          ++it) {
      //Don't look for parents if we've previously looked at the parents
      if(oMap.find(*it) == oMap.end()) {
        //use side effect of calling operator[] which is if the item isn't there it will add it as 'false'
        oMap[*it];
        boost::shared_ptr<ProductProvenance> pInfo = iMapper.branchIDToProvenance(*it);
        if(pInfo.get()) {
          markAncestors(*pInfo, iMapper, oMap, oMapperMissing);
        } else {
          oMapperMissing.insert(*it);
        }
      }
    }
  }
   
  void 
  ProvenanceCheckerOutputModule::write(EventPrincipal const& e) {
    //check ProductProvenance's parents to see if they are in the ProductProvenance list
    boost::shared_ptr<BranchMapper> mapperPtr = e.branchMapperPtr();
                       
    std::map<BranchID, bool> seenParentInPrincipal;
    std::set<BranchID> missingFromMapper;
    std::set<BranchID> missingProductProvenance;

    for(EventPrincipal::const_iterator it = e.begin(), itEnd = e.end();
          it != itEnd;
          ++it) {
      if(it->second && !it->second->productUnavailable()) {
        //This call seems to have a side effect of filling the 'ProductProvenance' in the Group
        OutputHandle const oh = e.getForOutput(it->first, false);

        if(!it->second->productProvenancePtr().get()) {
          missingProductProvenance.insert(it->first);
          continue;
        }
        boost::shared_ptr<ProductProvenance> pInfo = mapperPtr->branchIDToProvenance(it->first);
        if(!pInfo.get()) {
          missingFromMapper.insert(it->first);
        }
        markAncestors(*it->second->productProvenancePtr(), *mapperPtr, seenParentInPrincipal, missingFromMapper);
      }
      seenParentInPrincipal[it->first] = true;
    }
      
    //Determine what BranchIDs are in the product registry
    ProductRegistry const& reg = e.productRegistry();
    ProductRegistry::ProductList const& prodList = reg.productList();
    std::set<BranchID> branchesInReg;
    for(ProductRegistry::ProductList::const_iterator it = prodList.begin(), itEnd = prodList.end();
          it != itEnd;
          ++it) {
      branchesInReg.insert(it->second.branchID());
    }
     
    std::set<BranchID> missingFromPrincipal;
    std::set<BranchID> missingFromReg;
    for(std::map<BranchID, bool>::iterator it = seenParentInPrincipal.begin(), itEnd = seenParentInPrincipal.end();
          it != itEnd;
          ++it) {
      if(!it->second) {
        missingFromPrincipal.insert(it->first);
      }
      if(branchesInReg.find(it->first) == branchesInReg.end()) {
        missingFromReg.insert(it->first);
      }
    }
      
    if(missingFromMapper.size()) {
      LogError("ProvenanceChecker") << "Missing the following BranchIDs from BranchMapper\n";
      for(std::set<BranchID>::iterator it = missingFromMapper.begin(), itEnd = missingFromMapper.end();
             it!=itEnd;
             ++it) {
        LogProblem("ProvenanceChecker")<<*it;
      }
    }
    if(missingFromPrincipal.size()) {
      LogError("ProvenanceChecker") <<"Missing the following BranchIDs from EventPrincipal\n";
      for(std::set<BranchID>::iterator it = missingFromPrincipal.begin(), itEnd = missingFromPrincipal.end();
             it!=itEnd;
             ++it) {
        LogProblem("ProvenanceChecker")<<*it;
      }
    }
      
    if(missingProductProvenance.size()) {
      LogError("ProvenanceChecker") <<"The Groups for the following BranchIDs have no ProductProvenance\n";
      for(std::set<BranchID>::iterator it = missingProductProvenance.begin(), itEnd = missingProductProvenance.end();
             it!=itEnd;
             ++it) {
        LogProblem("ProvenanceChecker")<<*it;
      }      
    }

    if(missingFromReg.size()) {
      LogError("ProvenanceChecker") <<"Missing the following BranchIDs from ProductRegistry\n";
      for(std::set<BranchID>::iterator it = missingFromReg.begin(), itEnd = missingFromReg.end();
             it!=itEnd;
             ++it) {
        LogProblem("ProvenanceChecker")<<*it;
      }
    }
      
    if(missingFromMapper.size() || missingFromPrincipal.size() || missingProductProvenance.size() ||  missingFromReg.size()) {
      throw cms::Exception("ProvenanceError")
         << (missingFromMapper.size() || missingFromPrincipal.size() ? "Having missing ancestors" : "")
         << (missingFromMapper.size() ? " from BranchMapper" : "")
         << (missingFromMapper.size() && missingFromPrincipal.size() ? " and" : "")
         << (missingFromPrincipal.size() ? " from EventPrincipal" : "")
         << (missingFromMapper.size() || missingFromPrincipal.size() ? ".\n" : "")
         << (missingProductProvenance.size() ? " Have missing ProductProvenance's from Group in EventPrincipal.\n" : "")
         << (missingFromReg.size() ? " Have missing info from ProductRegistry.\n" : "");
    }
  }

//
// const member functions
//

//
// static member functions
//
}
using edm::ProvenanceCheckerOutputModule;
DEFINE_FWK_MODULE(ProvenanceCheckerOutputModule);
