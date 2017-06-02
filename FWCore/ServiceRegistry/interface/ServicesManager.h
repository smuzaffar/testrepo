#ifndef ServiceRegistry_ServicesManager_h
#define ServiceRegistry_ServicesManager_h
// -*- C++ -*-
//
// Package:     ServiceRegistry
// Class  :     ServicesManager
// 
/**\class ServicesManager ServicesManager.h FWCore/ServiceRegistry/interface/ServicesManager.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  Chris Jones
//         Created:  Mon Sep  5 13:33:01 EDT 2005
// $Id: ServicesManager.h,v 1.3 2005/09/12 19:07:21 chrjones Exp $
//

// system include files
#include <vector>
#include <cassert>
#include "boost/shared_ptr.hpp"

// user include files
#include "FWCore/Utilities/interface/TypeIDBase.h"
#include "FWCore/ServiceRegistry/interface/ServiceWrapper.h"
#include "FWCore/ServiceRegistry/interface/ServiceMakerBase.h"
#include "FWCore/ServiceRegistry/interface/ServiceLegacy.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"

#include "FWCore/Utilities/interface/EDMException.h"

// forward declarations
namespace edm {
   class ParameterSet;
   class ServiceToken;
   
   namespace serviceregistry {
      
      class ServicesManager
      {

public:
         struct MakerHolder {
            MakerHolder(boost::shared_ptr<ServiceMakerBase> iMaker,
                        const edm::ParameterSet& iPSet,
                        edm::ActivityRegistry&) ;
            bool add(ServicesManager&) const;
            
            boost::shared_ptr<ServiceMakerBase> maker_;
            const edm::ParameterSet* pset_;
            edm::ActivityRegistry* registry_;
            mutable bool wasAdded_;
         };
         typedef std::map< TypeIDBase, boost::shared_ptr<ServiceWrapperBase> > Type2Service;
         typedef std::map< TypeIDBase, MakerHolder > Type2Maker;
         
         ServicesManager(const std::vector<edm::ParameterSet>& iConfiguration);

         /** Takes the services described by iToken and places them into the manager.
             Conflicts over Services provided by both the iToken and iConfiguration 
             are resolved based on the value of iLegacy
         */
         ServicesManager(ServiceToken iToken,
                         ServiceLegacy iLegacy,
                         const std::vector<edm::ParameterSet>& iConfiguration);
         //virtual ~ServicesManager();
         
         // ---------- const member functions ---------------------
         template<class T>
         T& get() const {
            Type2Service::const_iterator itFound = type2Service_.find(TypeIDBase(typeid(T)));
            Type2Maker::const_iterator itFoundMaker ;
            if(itFound == type2Service_.end()) {
               //do on demand building of the service
               if(0 == type2Maker_.get() || 
                   type2Maker_->end() == (itFoundMaker = type2Maker_->find(TypeIDBase(typeid(T))))) {
                      throw edm::Exception(edm::errors::NotFound,"Service Request") 
                      <<" unable to find requested service";
               } else {
                  itFoundMaker->second.add(const_cast<ServicesManager&>(*this));
                  itFound = type2Service_.find(TypeIDBase(typeid(T)));
                  //the 'add()' should have put the service into the list
                  assert(itFound != type2Service_.end());
               }
            }
            //convert it to its actual type
            boost::shared_ptr<ServiceWrapper<T> > ptr(boost::dynamic_pointer_cast<ServiceWrapper<T> >(itFound->second));
            assert(0 != ptr.get());
            return ptr->get();
         }

         ///returns true of the particular service is accessible
         template<class T>
            bool isAvailable() const {
               Type2Service::const_iterator itFound = type2Service_.find(TypeIDBase(typeid(T)));
               Type2Maker::const_iterator itFoundMaker ;
               if(itFound == type2Service_.end()) {
                  //do on demand building of the service
                  if(0 == type2Maker_.get() || 
                      type2Maker_->end() == (itFoundMaker = type2Maker_->find(TypeIDBase(typeid(T))))) {
                     return false;
                  } else {
                     //Actually create the service in order to 'flush out' any 
                     // configuration errors for the service
                     itFoundMaker->second.add(const_cast<ServicesManager&>(*this));
                     itFound = type2Service_.find(TypeIDBase(typeid(T)));
                     //the 'add()' should have put the service into the list
                     assert(itFound != type2Service_.end());
                  }
               }
               return true;
            }
         
         // ---------- static member functions --------------------
         
         // ---------- member functions ---------------------------
         ///returns false if put fails because a service of this type already exists
         template<class T>
            bool put(boost::shared_ptr<ServiceWrapper<T> > iPtr) {
               Type2Service::const_iterator itFound = type2Service_.find(TypeIDBase(typeid(T)));
               if(itFound != type2Service_.end()) {
                  return false;
               }
               type2Service_[ TypeIDBase(typeid(T)) ] = iPtr;
               return true;
            }
        
         ///causes our ActivityRegistry's signals to be forwarded to iOther
         void connect(ActivityRegistry& iOther);
         
         ///causes iOther's signals to be forward to us
         void connectTo(ActivityRegistry& iOther);
private:
         ServicesManager(const ServicesManager&); // stop default
         
         const ServicesManager& operator=(const ServicesManager&); // stop default
         
         void fillListOfMakers(const std::vector<edm::ParameterSet>&);
         void createServices();

         // ---------- member data --------------------------------
         edm::ActivityRegistry registry_;
         Type2Service type2Service_;
         std::auto_ptr<Type2Maker> type2Maker_;
         
         //hold onto the Manager passed in from the ServiceToken so that
         // the ActivityRegistry of that Manager does not go out of scope
         boost::shared_ptr<ServicesManager> associatedManager_;
      };
   }
}


#endif
