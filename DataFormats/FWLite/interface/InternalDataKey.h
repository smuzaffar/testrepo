#ifndef DataFormats_FWLite_InternalDataKey_h
#define DataFormats_FWLite_InternalDataKey_h

// -*- C++ -*-
//
// Package:     FWLite
// Class  :     internal::DataKey
//
/**\class DataKey InternalDataKey.h DataFormats/FWLite/interface/InternalDataKey.h

   Description: Split from fwlite::Event to be reused in Event, LuminosityBlock, Run

   Usage:
   <usage>

*/
//
// Original Author:  Eric Vaandering
//         Created:  Jan 29 09:01:20 CDT 2009
// $Id: InternalDataKey.h,v 1.1 2010/01/29 16:26:57 ewv Exp $
//
#if !defined(__CINT__) && !defined(__MAKECINT__)

#include "DataFormats/Common/interface/EDProduct.h"
#include "TBranch.h"
#include "Reflex/Object.h"

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
               module_(iModule!=0? iModule:kEmpty()),
               product_(iProduct!=0?iProduct:kEmpty()),
               process_(iProcess!=0?iProcess:kEmpty()) {}

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
            const char* kEmpty()  const {return "";}
            const char* module()  const {return module_;}
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
            Long64_t lastProduct_;
            Reflex::Object obj_;
            void * pObj_; //ROOT requires the address of the pointer be stable
            edm::EDProduct* pProd_;

            ~Data() {
               obj_.Destruct();
            }
      };

      class ProductGetter;
   }

}

#endif /*__CINT__ */
#endif
