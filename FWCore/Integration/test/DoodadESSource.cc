// -*- C++ -*-
//
// Package:     Integration
// Class  :     DoodadESSource
// 
// Implementation:
//     <Notes on implementation>
//
// Original Author:  Chris Jones
//         Created:  Fri Jun 24 14:39:39 EDT 2005
// $Id: DoodadESSource.cc,v 1.6 2005/10/01 21:16:51 chrjones Exp $
//

// system include files

// user include files
#include "FWCore/Framework/interface/EventSetupRecordIntervalFinder.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/SourceFactory.h"


#include "FWCore/Integration/src/GadgetRcd.h"
#include "FWCore/Integration/src/Doodad.h"

namespace edmreftest {
class DoodadESSource :
   public edm::EventSetupRecordIntervalFinder, 
   public edm::ESProducer
{
   
public:
   DoodadESSource(const edm::ParameterSet&);
   
   std::auto_ptr<Doodad> produce(const GadgetRcd&) ;
   
protected:
   
   virtual void setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                const edm::IOVSyncValue& iTime, 
                                edm::ValidityInterval& iInterval);
   
private:
   DoodadESSource(const DoodadESSource&); // stop default
   
   const DoodadESSource& operator=(const DoodadESSource&); // stop default
   
   // ---------- member data --------------------------------
   unsigned int nCalls_;
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
DoodadESSource::DoodadESSource(const edm::ParameterSet&)
: nCalls_(0) {
   this->findingRecord<GadgetRcd>();
   setWhatProduced(this);
}

//DoodadESSource::~DoodadESSource()
//{
//}

//
// member functions
//

std::auto_ptr<Doodad> 
DoodadESSource::produce(const GadgetRcd&) {
   std::auto_ptr<Doodad> data(new Doodad());
   data->a = nCalls_;
   ++nCalls_;
   return data;
}


void 
DoodadESSource::setIntervalFor(const edm::eventsetup::EventSetupRecordKey&,
                                const edm::IOVSyncValue& iTime, 
                                edm::ValidityInterval& iInterval) {
   //Be valid for 3 runs 
   edm::EventID newTime = edm::EventID((iTime.eventID().run() - 1) - ((iTime.eventID().run() - 1) %3) +1);
   edm::EventID endTime = newTime.nextRun().nextRun().nextRun().previousRunLastEvent();
   iInterval = edm::ValidityInterval(edm::IOVSyncValue(newTime),
                                      edm::IOVSyncValue(endTime));
}

//
// const member functions
//

//
// static member functions
//
}
using namespace edmreftest;

DEFINE_FWK_EVENTSETUP_SOURCE(DoodadESSource)

