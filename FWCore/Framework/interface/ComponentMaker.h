#ifndef Framework_ComponentMaker_h
#define Framework_ComponentMaker_h
// -*- C++ -*-
//
// Package:     Framework
// Class  :     ComponentMaker
// 
/**\class ComponentMaker ComponentMaker.h FWCore/Framework/interface/ComponentMaker.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Author:      Chris Jones
// Created:     Wed May 25 16:56:05 EDT 2005
// $Id: ComponentMaker.h,v 1.10.2.1 2006/07/04 14:03:42 wmtan Exp $
//

// system include files
#include <string>
#include "boost/shared_ptr.hpp"

// user include files
#include "FWCore/Framework/interface/ComponentDescription.h"

// forward declarations

namespace edm {
   class ParameterSet;
   namespace eventsetup {
      class EventSetupProvider;
      class DataProxyProvider;
     
template <class T>
      class ComponentMakerBase {
public:
         virtual void addTo(EventSetupProvider& iProvider,
                     ParameterSet const& iConfiguration,
                     std::string const& iProcessName,
                     ReleaseVersion const& iVersion,
                     PassID const& iPass) const = 0;
      };
      
template <class T, class TComponent>
   class ComponentMaker : public ComponentMakerBase<T>
{

   public:
   ComponentMaker() {}
      //virtual ~ComponentMaker();

      // ---------- const member functions ---------------------
   virtual void addTo(EventSetupProvider& iProvider,
                       ParameterSet const& iConfiguration,
                       std::string const& iProcessName,
                       ReleaseVersion const& iVersion,
                       PassID const& iPass) const;
   
      // ---------- static member functions --------------------

      // ---------- member functions ---------------------------

   private:
      ComponentMaker(const ComponentMaker&); // stop default

      const ComponentMaker& operator=(const ComponentMaker&); // stop default

      void setDescription(DataProxyProvider* iProv, const ComponentDescription& iDesc) const {
        iProv->setDescription(iDesc);
      }
      void setDescription(void*, const ComponentDescription&) const {
      }
      // ---------- member data --------------------------------

};

template< class T, class TComponent>
void
ComponentMaker<T,TComponent>:: addTo(EventSetupProvider& iProvider,
                                        ParameterSet const& iConfiguration,
                                        std::string const& iProcessName,
                                        ReleaseVersion const& iVersion,
                                        PassID const& iPass) const
{
   boost::shared_ptr<TComponent> component(new TComponent(iConfiguration));
   
   ComponentDescription description;
   description.type_  = iConfiguration.template getParameter<std::string>("@module_type");
   description.label_ = iConfiguration.template getParameter<std::string>("@module_label");

   description.releaseVersion_ = iVersion;
   description.pid_           = iConfiguration.id();
   description.processName_   = iProcessName;
   description.passID_          = iPass;
      
   setDescription(component.get(),description);
   T::addTo(iProvider, component);
}
   }
}
#endif
