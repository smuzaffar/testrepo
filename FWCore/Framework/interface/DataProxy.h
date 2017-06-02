#ifndef Framework_DataProxy_h
#define Framework_DataProxy_h
// -*- C++ -*-
//
// Package:     Framework
// Class  :     DataProxy
// 
/**\class DataProxy DataProxy.h FWCore/Framework/interface/DataProxy.h

 Description: Base class for data Proxies held by a EventSetupRecord

 Usage:
    <usage>

*/
//
// Author:      Chris Jones
// Created:     Thu Mar 31 12:43:01 EST 2005
// $Id: DataProxy.h,v 1.6 2005/09/01 23:30:48 wmtan Exp $
//

// system include files

// user include files

// forward declarations
namespace edm {
   namespace eventsetup {
      class EventSetupRecord;
      class DataKey;
      class ComponentDescription;
      
class DataProxy
{

   public:
      DataProxy();
      virtual ~DataProxy();

      // ---------- const member functions ---------------------
      bool cacheIsValid() const { return cacheIsValid_; }
      
      virtual void doGet(const EventSetupRecord& iRecord, const DataKey& iKey) const = 0;

      ///returns the description of the DataProxyProvider which owns this Proxy
      const ComponentDescription* providerDescription() const {
         return description_;
      }
      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------
      void invalidate() {
         clearCacheIsValid();
         invalidateCache();
      }

      void setProviderDescription(const ComponentDescription* iDesc) {
         description_ = iDesc;
      }
   protected:
      /** indicates that the Proxy should invalidate any cached information
           as that information has 'expired'
         */
      virtual void invalidateCache() = 0;

      void setCacheIsValid() { cacheIsValid_ = true; }
      void clearCacheIsValid() { cacheIsValid_ = false; }
      
   private:
      DataProxy(const DataProxy&); // stop default

      const DataProxy& operator=(const DataProxy&); // stop default

      // ---------- member data --------------------------------
      bool cacheIsValid_;
      const ComponentDescription* description_;
};
   }
}
#endif
