// -*- C++ -*-
//
// Package:     Services
// Class  :     Tracer
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Thu Sep  8 14:17:58 EDT 2005
// $Id: Tracer.cc,v 1.12 2007/01/09 17:33:06 chrjones Exp $
//

// system include files
#include <iostream>

// user include files
#include "FWCore/Services/src/Tracer.h"

#include "DataFormats/Provenance/interface/ModuleDescription.h"
#include "DataFormats/Provenance/interface/EventID.h"
#include "DataFormats/Provenance/interface/Timestamp.h"

using namespace edm::service;
//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
Tracer::Tracer(const ParameterSet& iPS, ActivityRegistry&iRegistry):
indention_(iPS.getUntrackedParameter<std::string>("indention","++")),
depth_(0)
{
   iRegistry.watchPostBeginJob(this,&Tracer::postBeginJob);
   iRegistry.watchPostEndJob(this,&Tracer::postEndJob);

   iRegistry.watchPreProcessEvent(this,&Tracer::preEventProcessing);
   iRegistry.watchPostProcessEvent(this,&Tracer::postEventProcessing);

   iRegistry.watchPreModule(this,&Tracer::preModule);
   iRegistry.watchPostModule(this,&Tracer::postModule);
   
   iRegistry.watchPreSource(this,&Tracer::preSource);
   iRegistry.watchPostSource(this,&Tracer::postSource);
   
   iRegistry.watchPreProcessPath(this, &Tracer::prePath);
   iRegistry.watchPostProcessPath(this, &Tracer::postPath);

   iRegistry.watchPreModuleConstruction(this, &Tracer::preModuleConstruction);
   iRegistry.watchPostModuleConstruction(this, &Tracer::postModuleConstruction);

   iRegistry.watchPreModuleBeginJob(this, &Tracer::preModuleBeginJob);
   iRegistry.watchPostModuleBeginJob(this, &Tracer::postModuleBeginJob);

   iRegistry.watchPreModuleEndJob(this, &Tracer::preModuleEndJob);
   iRegistry.watchPostModuleEndJob(this, &Tracer::postModuleEndJob);
}

// Tracer::Tracer(const Tracer& rhs)
// {
//    // do actual copying here;
// }

//Tracer::~Tracer()
//{
//}

//
// assignment operators
//
// const Tracer& Tracer::operator=(const Tracer& rhs)
// {
//   //An exception safe implementation is
//   Tracer temp(rhs);
//   swap(rhs);
//
//   return *this;
// }

//
// member functions
//
void 
Tracer::postBeginJob()
{
   std::cout <<indention_<<" Job started"<<std::endl;
}
void 
Tracer::postEndJob()
{
   std::cout <<indention_<<" Job ended"<<std::endl;
}

void
Tracer::preSource()
{
  std::cout <<indention_<<indention_<<"source"<<std::endl;
}
void
Tracer::postSource()
{
  std::cout <<indention_<<indention_<<"finished: source"<<std::endl;
}

void 
Tracer::preEventProcessing(const edm::EventID& iID, const edm::Timestamp& iTime)
{
   depth_=0;
   std::cout <<indention_<<indention_<<" processing event:"<< iID<<" time:"<<iTime.value()<< std::endl;
}
void 
Tracer::postEventProcessing(const Event&, const EventSetup&)
{
   std::cout <<indention_<<indention_<<" finished event:"<<std::endl;
}

void 
Tracer::prePath(const std::string& iName)
{
  std::cout <<indention_<<indention_<<indention_<<" processing path:"<<iName<<std::endl;
}

void 
Tracer::postPath(const std::string& iName, const edm::HLTPathStatus&)
{
  std::cout <<indention_<<indention_<<indention_<<" finished path:"<<std::endl;
}

void 
Tracer::preModule(const ModuleDescription& iDescription)
{
   ++depth_;
   std::cout <<indention_<<indention_<<indention_;
   for(unsigned int depth = 0; depth !=depth_; ++depth) {
      std::cout<<indention_;
   }
   std::cout<<" module:" <<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::postModule(const ModuleDescription& iDescription)
{
   --depth_;
   std::cout <<indention_<<indention_<<indention_<<indention_;
   for(unsigned int depth = 0; depth !=depth_; ++depth) {
      std::cout<<indention_;
   }
   
   std::cout<<" finished:"<<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::preModuleConstruction(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" constructing module:" <<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::postModuleConstruction(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" construction finished:"<<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::preModuleBeginJob(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" beginJob module:" <<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::postModuleBeginJob(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" beginJob finished:"<<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::preModuleEndJob(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" endJob module:" <<iDescription.moduleLabel_<<std::endl;
}

void 
Tracer::postModuleEndJob(const ModuleDescription& iDescription)
{
  std::cout <<indention_;
  std::cout<<" endJob finished:"<<iDescription.moduleLabel_<<std::endl;
}

//
// const member functions
//

//
// static member functions
//
