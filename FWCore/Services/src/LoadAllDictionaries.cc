// -*- C++ -*-
//
// Package:     Services
// Class  :     LoadAllDictionaries
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Thu Sep 15 09:47:48 EDT 2005
// $Id: LoadAllDictionaries.cc,v 1.8 2007/04/13 11:05:43 wmtan Exp $
//

// system include files
#include "Cintex/Cintex.h"

// user include files
#include "FWCore/Services/src/LoadAllDictionaries.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/PluginManager/interface/PluginManager.h"
#include "FWCore/PluginManager/interface/PluginCapabilities.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
edm::service::LoadAllDictionaries::LoadAllDictionaries(const edm::ParameterSet& iConfig)
{
   bool doLoad(iConfig.getUntrackedParameter<bool>("doLoad"));
   if(doLoad) {
     ROOT::Cintex::Cintex::Enable();

     edmplugin::PluginManager*db =  edmplugin::PluginManager::get();
     
     typedef edmplugin::PluginManager::CategoryToInfos CatToInfos;
     
     CatToInfos::const_iterator itFound = db->categoryToInfos().find("Capability");
     
     if(itFound == db->categoryToInfos().end()) {
       return;
     }
     std::string lastClass;
     const std::string cPrefix("LCGReflex/");
     const std::string mystring("edm::Wrapper");

     for (edmplugin::PluginManager::Infos::const_iterator itInfo = itFound->second.begin(),
          itInfoEnd = itFound->second.end(); 
          itInfo != itInfoEnd; ++itInfo)
     {
       if (lastClass == itInfo->name_) {
         continue;
       }
       
       lastClass = itInfo->name_;
       if (lastClass.find(mystring) != std::string::npos) { 
         edmplugin::PluginCapabilities::get()->load(lastClass);
       }
       //NOTE: since we have the library already, we could be more efficient if we just load it ourselves
     }
   }
}

void edm::service::LoadAllDictionaries::fillDescriptions(edm::ConfigurationDescriptions & descriptions) {
  edm::ParameterSetDescription desc;
  desc.addUntracked<bool>("doLoad", true);
  descriptions.add("LoadAllDictionaries", desc);
}

