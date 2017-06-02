// -*- C++ -*-
//
// Package:    DoodadESProducer
// Class:      DoodadESProducer
// 
/**\class DoodadESProducer DoodadESProducer.h test/DoodadESProducer/interface/DoodadESProducer.h

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Chris Jones
//         Created:  Fri Jun 24 14:33:04 EDT 2005
// $Id: DoodadESProducer.cc,v 1.7 2005/10/15 01:46:18 wmtan Exp $
//
//


// system include files
#include <memory>
#include "boost/shared_ptr.hpp"

// user include files
#include "FWCore/Framework/interface/ModuleFactory.h"
#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "FWCore/Integration/test/Doodad.h"
#include "FWCore/Integration/test/Doodad.h"
#include "FWCore/Integration/test/GadgetRcd.h"


//
// class decleration
//
namespace edmreftest {

class DoodadESProducer : public edm::ESProducer {
   public:
      DoodadESProducer(const edm::ParameterSet&);
      ~DoodadESProducer();

      typedef std::auto_ptr<Doodad> ReturnType;

      ReturnType produce(const GadgetRcd &);
   private:
      // ----------member data ---------------------------
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
DoodadESProducer::DoodadESProducer(const edm::ParameterSet& /*iConfig*/)
{
   //the following line is needed to tell the framework what
   // data is being produced
   setWhatProduced(this);

   //now do what ever other initialization is needed
}


DoodadESProducer::~DoodadESProducer()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to produce the data  ------------
DoodadESProducer::ReturnType
DoodadESProducer::produce(const GadgetRcd& iRecord)
{
   using namespace edmreftest;

   std::auto_ptr<Doodad> pDoodad(new Doodad) ;

   pDoodad->a = 1;

   return pDoodad ;
}
}

using namespace edmreftest;
//define this as a plug-in
DEFINE_FWK_EVENTSETUP_MODULE(DoodadESProducer)
