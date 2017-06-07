#ifndef DataFormats_FWLite_Event_h
#define DataFormats_FWLite_Event_h
// -*- C++ -*-
//
// Package:     FWLite
// Class  :     Event
// 
/**\class Event Event.h DataFormats/FWLite/interface/Event.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Tue May  8 15:01:20 EDT 2007
// $Id: Event.h,v 1.11 2008/06/12 22:27:19 dsr Exp $
//
#if !defined(__CINT__) && !defined(__MAKECINT__)
// system include files
#include <typeinfo>
#include <map>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <memory>

#include "TBranch.h"
#include "Rtypes.h"
#include "Reflex/Object.h"

// user include files
#include "FWCore/Utilities/interface/TypeID.h"
#include "DataFormats/Provenance/interface/ProcessHistoryRegistry.h"
#include "DataFormats/Provenance/interface/EventProcessHistoryID.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"
#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/ProductID.h"
#include "DataFormats/Provenance/interface/FileIndex.h"
#include "FWCore/FWLite/interface/BranchMapReader.h"

// forward declarations
namespace edm {
  class EDProduct;
  class ProductRegistry;
  class BranchDescription;
  class EDProductGetter;
  class EventAux;
  class Timestamp;
}

namespace fwlite {
  namespace internal {
  class DataKey {
public:
    //NOTE: Do not take ownership of strings.  This is done to avoid
    // doing 'new's and string copies when we just want to lookup the data
    // This means something else is responsible for the pointers remaining
    // valid for the time for which the pointers are still in use
    DataKey(const edm::TypeID& iType,
            const char* iModule,
            const char* iProduct,
            const char* iProcess) :
    type_(iType),
    module_(iModule!=0? iModule:kEmpty),
    product_(iProduct!=0?iProduct:kEmpty),
    process_(iProcess!=0?iProcess:kEmpty) {}
    
    ~DataKey() {
    }

    bool operator<( const DataKey& iRHS) const {
      if( type_ < iRHS.type_) {
        return true;
      }
      if( iRHS.type_ < type_ ) {
        return false;
      }
      int comp = std::strcmp(module_,iRHS.module_);
      if( 0!= comp) {
        return comp <0;
      }
      comp = std::strcmp(product_,iRHS.product_);
      if( 0!= comp) {
        return comp <0;
      }
      comp = std::strcmp(process_,iRHS.process_);
      return comp <0;
    }
    static const char* const kEmpty;
    const  char* module() const {return module_;}
    const char* product() const {return product_;}
    const char* process() const {return process_;}
    const edm::TypeID& typeID() const {return type_;}
    
private:
    edm::TypeID type_;
    const char* module_;
    const char* product_;
    const char* process_;
  };
  
  struct Data {
    TBranch* branch_;
    Long64_t lastEvent_;
    ROOT::Reflex::Object obj_;
    void * pObj_; //ROOT requires the address of the pointer be stable
    edm::EDProduct* pProd_;
    
    ~Data() {
      obj_.Destruct();
    }
  };
  
  class ProductGetter;
  }
class Event
{

   public:
      /**NOTE: Does NOT take ownership so iFile must remain around at least as long as Event
  */
      Event(TFile* iFile);
      virtual ~Event();

      const Event& operator++();

      ///Go to the event at index iIndex
      const Event& to(Long64_t iIndex);
      
      //Go to event by Run & Event number
      bool to(edm::EventID id);
      bool to(edm::RunNumber_t run, edm::EventNumber_t event);

      /** Go to the very first Event*/
      const Event& toBegin();
      
      // ---------- const member functions ---------------------
      /** This function should only be called by fwlite::Handle<>*/
      void getByLabel(const std::type_info&, const char*, const char*, const char*, void*) const;
      //void getByBranchName(const std::type_info&, const char*, void*&) const;

      bool isValid() const;
      operator bool () const;
      bool atEnd() const;
      
      Long64_t size() const;

      edm::EventID id() const;
      const edm::Timestamp& time() const;
   
      const std::vector<edm::BranchDescription>& getBranchDescriptions() const { 
        return branchMap_.getBranchDescriptions();
      }

      // ---------- static member functions --------------------
      static void throwProductNotFoundException(const std::type_info&, const char*, const char*, const char*);

      // ---------- member functions ---------------------------

   private:
        friend class internal::ProductGetter;
      
      Event(const Event&); // stop default

      const Event& operator=(const Event&); // stop default

      const edm::ProcessHistory& history() const;
      void updateAux(Long_t eventIndex) const;
      void fillFileIndex() const;
      
      edm::EDProduct const* getByProductID(edm::ProductID const&) const;
      // ---------- member data --------------------------------
//      TFile* file_;
//      TTree* eventTree_;
      TTree* eventHistoryTree_;
//      Long64_t eventIndex_;
      mutable fwlite::BranchMapReader branchMap_;
      

      typedef std::map<internal::DataKey, boost::shared_ptr<internal::Data> > KeyToDataMap;
      mutable KeyToDataMap data_;
      //takes ownership of the strings used by the DataKey keys in data_
      mutable std::vector<const char*> labels_;
      mutable edm::ProcessHistoryMap historyMap_;
      mutable std::vector<edm::EventProcessHistoryID> eventProcessHistoryIDs_;
      mutable edm::EventAuxiliary aux_;
      mutable edm::FileIndex fileIndex_;
      edm::EventAuxiliary* pAux_;
      edm::EventAux* pOldAux_;
      TBranch* auxBranch_;
      int fileVersion_;
      
      //references data in data_;
      mutable std::map<edm::ProductID,boost::shared_ptr<internal::Data> > idToData_; 
//      mutable edm::ProductRegistry* prodReg_;
      //references branch descriptions in prodReg_;
//      mutable std::map<edm::ProductID,const edm::BranchDescription*> idToBD_;
      
      std::auto_ptr<edm::EDProductGetter> getter_;
};

}
#endif /*__CINT__ */
#endif
