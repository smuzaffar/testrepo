// -*- C++ -*-
//
// Package:     PluginManager
// Class  :     PluginManager
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Wed Apr  4 14:28:58 EDT 2007
// $Id: PluginManager.cc,v 1.4 2007/04/19 12:44:20 chrjones Exp $
//

// system include files
#include <boost/bind.hpp>
#include <boost/mem_fn.hpp>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>

#include <fstream>
#include <iostream>
#include <set>

// user include files
#include "FWCore/PluginManager/interface/PluginManager.h"
#include "FWCore/PluginManager/interface/PluginFactoryManager.h"
#include "FWCore/PluginManager/interface/CacheParser.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "FWCore/PluginManager/interface/standard.h"

namespace edmplugin {
//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
PluginManager::PluginManager(const PluginManager::Config& iConfig) :
  searchPath_( iConfig.searchPath() )
{
    const boost::filesystem::path kCacheFile(standard::cachefileName());
    //NOTE: This may not be needed :/
    PluginFactoryManager* pfm = PluginFactoryManager::get();
    pfm->newFactory_.connect(boost::bind(boost::mem_fn(&PluginManager::newFactory),this,_1));
    
    //read in the files
    //Since we are looping in the 'precidence' order then the lists in categoryToInfos_ will also be
    // in that order
    std::set<std::string> alreadySeen;
    for(SearchPath::const_iterator itPath=searchPath_.begin(), itEnd = searchPath_.end();
        itPath != itEnd;
        ++itPath) {
      //take care of the case where the same path is passed in multiple times
      if (alreadySeen.find(*itPath) != alreadySeen.end() ) {
        continue;
      }
      alreadySeen.insert(*itPath);
      boost::filesystem::path dir(*itPath,boost::filesystem::no_check);
      if( exists( dir) ) {
        if(not is_directory(dir) ) {
          throw cms::Exception("PluginManagerBadPath") <<"The path '"<<dir.native_file_string()<<"' for the PluginManager is not a directory";
        }
        boost::filesystem::path cacheFile = dir/kCacheFile;
        
        if(exists(cacheFile) ) {
          std::ifstream file(cacheFile.native_file_string().c_str());
          if(not file) {
            throw cms::Exception("PluginMangerCacheProblem")<<"Unable to open the cache file '"<<cacheFile.native_file_string()
            <<"'. Please check permissions on file";
          }
          CacheParser::read(file, dir, categoryToInfos_);          
        }
      }
    }
    
    //sort the results by category, plugin then directory priority
}

// PluginManager::PluginManager(const PluginManager& rhs)
// {
//    // do actual copying here;
// }

PluginManager::~PluginManager()
{
}

//
// assignment operators
//
// const PluginManager& PluginManager::operator=(const PluginManager& rhs)
// {
//   //An exception safe implementation is
//   PluginManager temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//
void
PluginManager::newFactory(const PluginFactoryBase*)
{
}
//
// const member functions
//
namespace {
  struct PICompare {
    bool operator()(const PluginInfo& iLHS,
                    const PluginInfo& iRHS) const {
                      return iLHS.name_ < iRHS.name_;
                    }
  };
}

const boost::filesystem::path& 
PluginManager::loadableFor(const std::string& iCategory,
                             const std::string& iPlugin)
{
  CategoryToInfos::iterator itFound = categoryToInfos_.find(iCategory);
  if(itFound == categoryToInfos_.end()) {
    throw cms::Exception("PluginNotFound")<<"Unable to find plugin '"<<iPlugin<<
    "' because the category '"<<iCategory<<"' has no known plugins";
  }
  
  PluginInfo i;
  i.name_ = iPlugin;
  typedef std::vector<PluginInfo>::iterator PIItr;
  std::pair<PIItr,PIItr> range = std::equal_range(itFound->second.begin(),
                                                  itFound->second.end(),
                                                  i,
                                                  PICompare() );
  
  if(range.first == range.second) {
    throw cms::Exception("PluginNotFound")<<"Unable to find plugin '"<<iPlugin
    <<"'. Please check spelling of name.";
  }
  
  if(range.second - range.first > 1 ) {
    //see if the come from the same directory
    if(range.first->loadable_.branch_path() == (range.first+1)->loadable_.branch_path()) {
      //std::cout<<range.first->name_ <<" " <<(range.first+1)->name_<<std::endl;
      throw cms::Exception("MultiplePlugins")<<"The plugin '"<<iPlugin<<"' is found in multiple files \n"
      " '"<<range.first->loadable_.leaf()<<"'\n '"
      <<(range.first+1)->loadable_.leaf()<<"'\n"
      "in directory '"<<range.first->loadable_.branch_path().native_file_string()<<"'.\n"
      "The code must be changed so the plugin only appears in one plugin file. "
      "You will need to remove the macro which registers the plugin so it only appears in"
      " one of these files.\n"
      "  If none of these files register such a plugin, "
      "then the problem originates in a library to which all these files link.\n"
      "The plugin registration must be removed from that library since plugins are not allowed in regular libraries.";
    }
  }
  
  return range.first->loadable_;
}

const SharedLibrary& 
PluginManager::load(const std::string& iCategory,
                      const std::string& iPlugin)
{
  const boost::filesystem::path& p = loadableFor(iCategory,iPlugin);
  
  //have we already loaded this?
  std::map<boost::filesystem::path, boost::shared_ptr<SharedLibrary> >::iterator itLoaded = 
    loadables_.find(p);
  if(itLoaded == loadables_.end()) {
    //try to make one
    loadingLibraryNamed_()=p.native_file_string();
    goingToLoad_(p);
    boost::shared_ptr<SharedLibrary> ptr( new SharedLibrary(p.native_file_string()) );
    loadables_[p]=ptr;
    justLoaded_(*ptr);
    loadingLibraryNamed_()="<loaded by another plugin system>";
    return *ptr;
  }
  return *(itLoaded->second);
}

//
// static member functions
//
PluginManager* 
PluginManager::get()
{
  PluginManager* manager = singleton();
  if(0==manager) {
    throw cms::Exception("PluginManagerNotConfigured")<<"PluginManager::get() was called before PluginManager::configure.";
  }
  return manager;
}

PluginManager& 
PluginManager::configure(const Config& iConfig )
{
  PluginManager*& s = singleton();
  if( 0 != s ){
    throw cms::Exception("PluginManagerReconfigured");
  }
  
  Config realConfig = iConfig;
  if (realConfig.searchPath().empty() ) {
    throw cms::Exception("PluginManagerEmptySearchPath");
  }
  s = new PluginManager (realConfig);
  return *s;
}


const std::string& 
PluginManager::staticallyLinkedLoadingFileName()
{
  static std::string s_name("static");
  return s_name;
}

std::string& 
PluginManager::loadingLibraryNamed_()
{
  static std::string s_name(staticallyLinkedLoadingFileName());
  return s_name;
}

PluginManager*& PluginManager::singleton()
{
  static PluginManager* s_singleton;
  return s_singleton;
}

bool
PluginManager::isAvailable()
{
  return 0 != singleton();
}

}
