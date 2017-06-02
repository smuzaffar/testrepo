// -*- C++ -*-
//
// Package:    EventSetupRecordDataGetter
// Class:      EventSetupRecordDataGetter
// 
/**\class EventSetupRecordDataGetter EventSetupRecordDataGetter.cc src/EventSetupRecordDataGetter/src/EventSetupRecordDataGetter.cc

 Description: Can be configured to 'get' any Data in any EventSetup Record.  Primarily used for testing.

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Chris Jones
//         Created:  Tue Jun 28 11:10:24 EDT 2005
// $Id: EventSetupRecordDataGetter.cc,v 1.1 2005/06/28 19:35:49 chrjones Exp $
//
//


// system include files
#include <memory>
#include <map>
#include <vector>
#include <iostream>

// user include files
#include "FWCore/FWCoreServices/src/EventSetupRecordDataGetter.h"

#include "FWCore/CoreFramework/interface/Event.h"
#include "FWCore/CoreFramework/interface/EventSetup.h"
#include "FWCore/CoreFramework/interface/EventSetupRecord.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"


//
// class decleration
//
namespace edm {
//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
   EventSetupRecordDataGetter::EventSetupRecordDataGetter( const edm::ParameterSet& iConfig ):
   pSet_( iConfig ),
   verbose_( iConfig.getUntrackedParameter("verbose",false) )
{
}


EventSetupRecordDataGetter::~EventSetupRecordDataGetter()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to produce the data  ------------
void
EventSetupRecordDataGetter::analyze( const edm::Event& iEvent, const edm::EventSetup& iSetup )
{
   if( 0 == recordToDataKeys_.size() ) {
      typedef std::vector< ParameterSet > Parameters;
      Parameters toGet = pSet_.getParameter<Parameters>("toGet");
      
      for(Parameters::iterator itToGet = toGet.begin(); itToGet != toGet.end(); ++itToGet ) {
         std::string recordName = itToGet->getParameter<std::string>("record");
         
         eventsetup::EventSetupRecordKey recordKey(eventsetup::EventSetupRecordKey::TypeTag::findType( recordName ) );
         if( recordKey.type() == eventsetup::EventSetupRecordKey::TypeTag() ) {
            //record not found
            std::cout <<"Record \""<< recordName <<"\" does not exist "<<std::endl;
            continue;
         }
         typedef std::vector< std::string > Strings;
         Strings dataNames = itToGet->getParameter< Strings >("data");
         std::vector< eventsetup::DataKey > dataKeys;
         for( Strings::iterator itDatum = dataNames.begin(); itDatum != dataNames.end(); ++itDatum ) {
            eventsetup::TypeTag datumType = eventsetup::TypeTag::findType(*itDatum);
            if( datumType == eventsetup::TypeTag() ) {
               //not found
               std::cout <<"data item \""<< *itDatum <<"\" does not exist"<<std::endl;
               continue;
            }
            eventsetup::DataKey datumKey( datumType, "");
            dataKeys.push_back( datumKey ); 
         }
         recordToDataKeys_.insert( std::make_pair( recordKey, dataKeys ) );
         recordToTimestamp_.insert( std::make_pair( recordKey, new Timestamp( Timestamp::invalidTimestamp() ) ) );
      }
   }
   
   using namespace edm;
   using namespace edm::eventsetup;

   //For each requested Record get the requested data only if the Record is in a new IOV
   
   for( RecordToDataKeys::iterator itRecord = recordToDataKeys_.begin();
        itRecord != recordToDataKeys_.end();
        ++itRecord ) {
      const EventSetupRecord* pRecord = iSetup.find( itRecord->first );
      
      if( 0 != pRecord && pRecord->validityInterval().first() != *(recordToTimestamp_[itRecord->first])) {
         *(recordToTimestamp_[itRecord->first]) = pRecord->validityInterval().first();
         typedef std::vector< DataKey > Keys;
         const Keys& keys = itRecord->second;
         for( Keys::const_iterator itKey = keys.begin();
              itKey != keys.end();
              ++itKey ) {
            if( ! pRecord->doGet( *itKey ) ) {
               std::cout << "No data of type \""<<itKey->type().name() <<"\" with name \""<< itKey->name().value()<<"\" in record "<<itRecord->first.type().name() <<" found "<< std::endl;
            } else {
               if( verbose_ ) {
                  std::cout << "got data of type \""<<itKey->type().name() <<"\" with name \""<< itKey->name().value()<<"\" in record "<<itRecord->first.type().name() << std::endl;
               }
            }
         }
      }
   }
}
}
//define this as a plug-in
//DEFINE_FWK_MODULE(EventSetupRecordDataGetter)
