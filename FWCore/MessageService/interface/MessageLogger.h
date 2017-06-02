#ifndef FWCore_MessageService_MessageLogger_h
#define FWCore_MessageService_MessageLogger_h

// -*- C++ -*-
//
// Package:     Services
// Class  :     MessageLogger
//
/**\class MessageLogger MessageLogger.h FWCore/MessageService/interface/MessageLogger.h

 Description: <one line class summary>

 Usage:
    <usage>

*/
//
// Original Author:  W. Brown and M. Fischler
//         Created:  Fri Nov 11 16:38:19 CST 2005
//     Major Split:  Tue Feb 14 15:00:00 CST 2006
//			See FWCore/MessageLogger/MessageLogger.h
// $Id: MessageLogger.h,v 1.1 2006/02/15 00:30:31 fischler Exp $
//

// system include files

#include <memory>
#include <string>
#include <set>

// user include files

#include "FWCore/MessageLogger/interface/ErrorObj.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "DataFormats/Common/interface/EventID.h"

// forward declarations

namespace edm  {
namespace service  {


class MessageLogger
{
public:
  MessageLogger( ParameterSet const &, ActivityRegistry & );

  void  postBeginJob();
  void  postEndJob();

  void  preEventProcessing ( edm::EventID const &, edm::Timestamp const & );
  void  postEventProcessing( Event const &, EventSetup const & );

  void  preModuleConstruction ( ModuleDescription const & );
  void  postModuleConstruction( ModuleDescription const & );

  void  preSourceConstruction ( ModuleDescription const & );
  void  postSourceConstruction( ModuleDescription const & );

  void  preSource  ( ModuleDescription const & );
  void  postSource ( ModuleDescription const & );

  void  preModule ( ModuleDescription const & );
  void  postModule( ModuleDescription const & );

  void  fillErrorObj(edm::ErrorObj& obj) const;
  bool  debugEnabled() const { return debugEnabled_; }

  static 
  bool  anyDebugEnabled() { return anyDebugEnabled_; }
  
private:
  // put an ErrorLog object here, and maybe more

  edm::EventID curr_event_;
  std::string curr_module_;

  std::set<std::string> debugEnabledModules_;
  bool debugEnabled_;
  static bool   anyDebugEnabled_;
  static bool everyDebugEnabled_;

};  // MessageLogger


}  // namespace service

}  // namespace edm



#endif  // FWCore_MessageService_MessageLogger_h

